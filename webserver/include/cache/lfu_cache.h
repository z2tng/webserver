#pragma once

#include <unordered_map>
#include <string>
#include <mutex>

namespace cache {

template <typename T>
class Node {
public:
    Node() : prev_(nullptr), next_(nullptr) {}
    Node(const T &data) : data_(data), prev_(nullptr), next_(nullptr) {}
    Node(const T &data, Node *prev, Node *next) : data_(data), prev_(prev), next_(next) {}

    T& data() { return data_; }
    const T& data() const { return data_; }

    Node* prev() { return prev_; }
    const Node* prev() const { return prev_; }

    Node* next() { return next_; }
    const Node* next() const { return next_; }

    void set_prev(Node *prev) { prev_ = prev; }
    void set_next(Node *next) { next_ = next; }

private:
    T data_;
    Node *prev_;
    Node *next_;
};

struct Key {
    std::string key_;
    std::string value_;
};

using KeyNode = Node<Key>;

class KeyList {
public:
    KeyList() = default;
    ~KeyList() = default;

    void init(int freq);
    void clear();

    int freq() const { return freq_; }
    bool empty() const { return dummy_head_->next() == tail_; }
    KeyNode* back() const { return tail_; }

    void Add(KeyNode *node);
    void Remove(KeyNode *node);

private:
    int freq_;
    KeyNode *dummy_head_;
    KeyNode *tail_;
};

using FreqNode = Node<KeyList>;

class LfuCache {
public:
    static LfuCache& instance();

    void Set(const std::string &key, const std::string &value);
    bool Get(const std::string &key, std::string &value);

private:
    LfuCache(size_t capacity = 10);
    ~LfuCache();

    void AddFreqNode(KeyNode *key_node, FreqNode *freq_node);
    void RemoveFreqNode(FreqNode *freq_node);

    size_t capacity_;
    FreqNode *dummy_head_;

    std::unordered_map<std::string, KeyNode*> key_table_;
    std::unordered_map<std::string, FreqNode*> freq_table_;

    std::mutex mutex_;
};

} // namespace cache
