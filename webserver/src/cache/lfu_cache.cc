#include "cache/lfu_cache.h"
#include "memory/memory_pool.h"

namespace cache {

using memory::NewElement;
using memory::DeleteElement;

KeyList::~KeyList() {
    clear();
    DeleteElement(dummy_head_);
}

void KeyList::init(int freq) {
    freq_ = freq;
    dummy_head_ = NewElement<KeyNode>();
    tail_ = dummy_head_;
}

void KeyList::clear() {
    while (tail_ != dummy_head_) {
        tail_ = tail_->prev();
        DeleteElement(tail_->next());
    }
}

void KeyList::Add(KeyNode *node) {
    if (dummy_head_->next() == nullptr) {
        tail_ = node;
    } else {
        dummy_head_->next()->set_prev(node);
    }
    node->set_prev(dummy_head_);
    node->set_next(dummy_head_->next());
    dummy_head_->set_next(node);
}

void KeyList::Remove(KeyNode *node) {
    node->prev()->set_next(node->next());
    if (node->next() != nullptr) {
        node->next()->set_prev(node->prev());
    } else {
        tail_ = node->prev();
    }
}

LfuCache::~LfuCache() {
    for (auto &pair : key_table_) {
        DeleteElement(pair.second);
    }
    for (auto &pair : freq_table_) {
        DeleteElement(pair.second);
    }
    DeleteElement(dummy_head_);
}

void LfuCache::Init(size_t capacity) {
    capacity_ = capacity;
    dummy_head_ = NewElement<FreqNode>();
    dummy_head_->data().init(0);
}

LfuCache& LfuCache::instance() {
    static LfuCache cache;
    return cache;
}

void LfuCache::Set(const std::string &key, const std::string &value) {
    if (!capacity_) return;

    std::lock_guard<std::mutex> lock(mutex_);

    if (key_table_.size() == capacity_) {
        auto min_freq_list = dummy_head_->next();
        auto min_freq_node = min_freq_list->data().back();

        min_freq_list->data().Remove(min_freq_node);
        key_table_.erase(min_freq_node->data().key_);
        freq_table_.erase(min_freq_node->data().key_);

        DeleteElement(min_freq_node);
        if (min_freq_list->data().empty()) {
            RemoveFreqNode(min_freq_list);
        }
    }

    KeyNode *key_node = NewElement<KeyNode>(Key{key, value});
    AddFreqNode(key_node, dummy_head_);
    key_table_[key] = key_node;
}

bool LfuCache::Get(const std::string &key, std::string &value) {
    if (!capacity_) return false;

    std::lock_guard<std::mutex> lock(mutex_);

    if (freq_table_.find(key) != freq_table_.end()) {
        auto key_node = key_table_[key];
        auto freq_node = freq_table_[key];
        value = key_node->data().value_;

        AddFreqNode(key_node, freq_node);
        return true;
    }
    return false;
}

void LfuCache::AddFreqNode(KeyNode *key_node, FreqNode *freq_node) {
    FreqNode *next_freq = nullptr;

    if (freq_node->next() == nullptr ||
            freq_node->next()->data().freq() != freq_node->data().freq() + 1) {
        next_freq = NewElement<FreqNode>();
        next_freq->data().init(freq_node->data().freq() + 1);

        if (freq_node->next() != nullptr) {
            freq_node->next()->set_prev(next_freq);
        }
        next_freq->set_next(freq_node->next());
        next_freq->set_prev(freq_node);
        freq_node->set_next(next_freq);
    } else {
        next_freq = freq_node->next();
    }

    freq_table_[key_node->data().key_] = next_freq;
    if (freq_node != dummy_head_) {
        freq_node->data().Remove(key_node);
        if (freq_node->data().empty()) {
            RemoveFreqNode(freq_node);
        }
    }
    next_freq->data().Add(key_node);
}

void LfuCache::RemoveFreqNode(FreqNode *freq_node) {
    freq_node->prev()->set_next(freq_node->next());
    if (freq_node->next() != nullptr) {
        freq_node->next()->set_prev(freq_node->prev());
    }
    DeleteElement(freq_node);
}

} // namespace cache