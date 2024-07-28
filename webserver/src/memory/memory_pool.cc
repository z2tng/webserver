#include "memory/memory_pool.h"

namespace memory {

MemoryPool::MemoryPool() {}

MemoryPool::~MemoryPool() {
    Slot* curr = current_block_;
    while (curr) {
        Slot* next = curr->next;
        operator delete(reinterpret_cast<void*>(curr));
        curr = next;
    }
}

void MemoryPool::Init(int slot_size) {
    slot_size_ = slot_size;
    current_block_ = nullptr;
    current_slot_ = nullptr;
    last_slot_ = nullptr;
    free_slot_ = nullptr;
}

Slot* MemoryPool::Allocate() {
    if (free_slot_) {
        {
            std::lock_guard<std::mutex> lock(mutex_free_slot_);
            if (free_slot_) {
                Slot* result = free_slot_;
                free_slot_ = free_slot_->next;
                return result;
            }
        }
    }
    return NoFreeSolve();
}


void MemoryPool::Deallocate(Slot* p) {
    if (p){
        std::lock_guard<std::mutex> lock(mutex_free_slot_);
        p->next = free_slot_;
        free_slot_ = p;
    }
}


Slot* MemoryPool::AllocateBlock() {
    char *new_block = reinterpret_cast<char*>(operator new(kBlockSize));
    char *body = new_block + sizeof(Slot*);
    size_t body_padding = PadPointer(body, static_cast<size_t>(slot_size_));

    Slot* use_slot;
    {
        std::lock_guard<std::mutex> lock(mutex_other_);
        reinterpret_cast<Slot*>(new_block)->next = current_block_;
        current_block_ = reinterpret_cast<Slot*>(new_block);
        current_slot_ = reinterpret_cast<Slot*>(body + body_padding);
        last_slot_ = reinterpret_cast<Slot*>(new_block + kBlockSize - slot_size_ + 1);

        use_slot = current_slot_;
        current_slot_ += (slot_size_ >> 3);
    }
    return use_slot;
}

Slot* MemoryPool::NoFreeSolve() {
    if (current_slot_ >= last_slot_) {
        return AllocateBlock();
    }

    Slot* use_slot;
    {
        std::lock_guard<std::mutex> lock(mutex_other_);
        use_slot = current_slot_;
        current_slot_ += (slot_size_ >> 3);
    }
    return use_slot;
}

MemoryPool& GetMemoryPool(int index) {
    static MemoryPool memory_pool[64];
    return memory_pool[index];
}

void InitMemoryPool() {
    for (int i = 0; i < 64; ++i) {
        GetMemoryPool(i).Init((i + 1) << 3);
    }
}

void* UseMemory(size_t size) {
    if (!size) return nullptr;
    if (size > 512) return operator new(size);

    int index = ((size + 7) >> 3) - 1;
    return reinterpret_cast<void*>(GetMemoryPool(index).Allocate());
}

void FreeMemory(size_t size, void *p) {
    if (!p) return;
    if (size > 512) {
        operator delete(p);
        return;
    }

    int index = ((size + 7) >> 3) - 1;
    GetMemoryPool(index).Deallocate(reinterpret_cast<Slot*>(p));
}

} // namespace memory
