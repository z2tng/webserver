#pragma once

#include <cstddef>
#include <mutex>

const int kBlockSize = 4096;

namespace memory {

struct Slot {
    Slot* next;
};

class MemoryPool {
public:
    MemoryPool();
    ~MemoryPool();

    void Init(int slot_size);

    Slot* Allocate();
    void Deallocate(Slot* p);

private:
    size_t PadPointer(char *p, size_t align) {
        // 计算对齐偏移量
        size_t result = reinterpret_cast<size_t>(p);
        return ((align - result) % align);
    }

    Slot* AllocateBlock();
    Slot* NoFreeSolve();

    int slot_size_;

    Slot* current_block_;  // 内存块链表的头指针
    Slot* current_slot_;   // 元素链表的头指针
    Slot* last_slot_;      // 可存放元素的链表尾指针
    Slot* free_slot_;      // 元素构造后的释放掉内存的链表头指针

    std::mutex mutex_free_slot_;
    std::mutex mutex_other_;
};

MemoryPool& GetMemoryPool(int index);

void InitMemoryPool();
void* UseMemory(size_t size);
void FreeMemory(size_t size, void *p);

template <typename T, typename... Args>
T* NewElement(Args&&... args) {
    T *p;
    if ((p = reinterpret_cast<T*>(UseMemory(sizeof(T)))) != nullptr) {
        new (p) T(std::forward<Args>(args)...);
    }
    return p;
}

template <typename T>
void DeleteElement(T* p) {
    if (p) p->~T();
    FreeMemory(sizeof(T), reinterpret_cast<void*>(p));
}

} // namespace memory
