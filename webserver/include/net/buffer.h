#pragma once

#include "utils/uncopyable.h"

#include <string>
#include <vector>
#include <algorithm>
#include <cstring>

namespace net {

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

    const char* FindCRLF() const {
        const char* crlf = std::search(Peek(), begin() + write_index_, kCRLF, kCRLF + 2);
        return crlf == begin() + write_index_ ? nullptr : crlf;
    }

    const char* FindCRLF(const char* start) const {
        const void* crlf = memmem(start, begin() + write_index_ - start, kCRLF, 2);
        return static_cast<const char*>(crlf);
    }

    const char* FindEOL() const {
        const char* eol = std::find(Peek(), begin() + write_index_, '\n');
        return eol == begin() + write_index_ ? nullptr : eol;
    }

    const char* FindEOL(const char* start) const {
        const void* eol = memchr(start, '\n', begin() + write_index_ - start);
        return static_cast<const char*>(eol);
    }

    void Retrieve(size_t len) {
        if (len < ReadableBytes()) {
            read_index_ += len;
        } else {
            RetrieveAll();
        }
    }

    void RetrieveUntil(const char* end) {
        Retrieve(end - Peek());
    }

    void RetrieveAll() {
        read_index_ = kPrependSize;
        write_index_ = kPrependSize;
    }

    std::string RetrieveAsString(size_t len) {
        std::string str(Peek(), len);
        Retrieve(len);
        return str;
    }

    std::string RetrieveAllAsString() {
        std::string str(Peek(), ReadableBytes());
        RetrieveAll();
        return str;
    }

    void Append(const char* data, size_t len) {
        EnsureWritableBytes(len);
        std::copy(data, data + len, begin() + write_index_);
        write_index_ += len;
    }

    void Append(const std::string& str) {
        Append(str.data(), str.size());
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

    void MakeSpace(size_t len);

    std::vector<char> buffer_;
    size_t read_index_;
    size_t write_index_;

    static const char kCRLF[];
};
    
} // namespace connection

