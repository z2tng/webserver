#pragma once

#include "utils/uncopyable.h"

#include <string>
#include <vector>

namespace connection {

class Buffer : utils::Uncopyable {
public:
    static const size_t kPrependSize = 8;
    static const size_t kInitialSize = 1024;

    explicit Buffer(size_t initial_size = kInitialSize)
        : buffer_(kPrependSize + initial_size),
          read_index_(kPrependSize),
          write_index_(kPrependSize) {}
    
    void swap(Buffer& rhs) {
        buffer_.swap(rhs.buffer_);
        std::swap(read_index_, rhs.read_index_);
        std::swap(write_index_, rhs.write_index_);
    }

    size_t ReadableBytes() const { return write_index_ - read_index_; }
    size_t WritableBytes() const { return buffer_.size() - write_index_; }
    size_t PrependableBytes() const { return read_index_; }

    const char* Peek() const { return begin() + read_index_; }

    void Retrieve(size_t len) {
        if (len < ReadableBytes()) {
            read_index_ += len;
        } else {
            RetrieveAll();
        }
    }

    void RetrieveAll() {
        read_index_ = kPrependSize;
        write_index_ = kPrependSize;
    }

    void Append(const char* data, size_t len) {
        EnsureWritableBytes(len);
        std::copy(data, data + len, begin() + write_index_);
        write_index_ += len;
    }

    void EnsureWritableBytes(size_t len) {
        if (WritableBytes() < len) {
            MakeSpace(len);
        }
    }

    ssize_t ReadFd(int fd, int* saved_errno);
    ssize_t WriteFd(int fd, int* saved_errno);

private:
    char* begin() { return &buffer_[0]; }
    const char* begin() const { return &buffer_[0]; }

    void MakeSpace(size_t len) {
        if (WritableBytes() + PrependableBytes() < len + kPrependSize) {
            buffer_.resize(write_index_ + len);
        } else {
            size_t readable = ReadableBytes();
            std::copy(begin() + read_index_, begin() + write_index_, begin() + kPrependSize);
            read_index_ = kPrependSize;
            write_index_ = read_index_ + readable;
        }
    }

    std::vector<char> buffer_;
    size_t read_index_;
    size_t write_index_;
};
    
} // namespace connection

