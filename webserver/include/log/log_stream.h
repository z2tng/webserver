#pragma once

#include "utils/uncopyable.h"

#include <strings.h>
#include <string.h>
#include <string>

namespace logging {

const int kSmallBufferSize = 4 * 1024;
const int kLargeBufferSize = 4 * 1024 * 1024;

template <int buffer_size>
class FixedBuffer : utils::Uncopyable {
public:
    FixedBuffer() : size_(0) {
        bzero(buffer_, sizeof(buffer_));
        current_ = buffer_;
    }

    ~FixedBuffer() {}

    void write(const char* buffer, int size) {
        if (capacity() > size) {
            memcpy(current_, buffer, size);
            current_ += size;
            size_ += size;
        }
    }

    void add(int size) {
        current_ += size;
        size_ += size;
    }

    void clear() {
        current_ = buffer_;
        size_ = 0;
    }

    const char* buffer() const { return buffer_; }
    char* current() { return current_; }
    int size() const { return size_; }
    int capacity() const { return buffer_size - size_; }

private:
    char buffer_[buffer_size];
    char* current_;
    int size_;
};

class LogStream : utils::Uncopyable {
public:
    using Buffer = FixedBuffer<kSmallBufferSize>;

    LogStream() {}
    ~LogStream() {}

    LogStream& operator<<(bool);

    LogStream& operator<<(short);
    LogStream& operator<<(unsigned short);
    LogStream& operator<<(int);
    LogStream& operator<<(unsigned int);
    LogStream& operator<<(long);
    LogStream& operator<<(unsigned long);
    LogStream& operator<<(long long);
    LogStream& operator<<(unsigned long long);

    LogStream& operator<<(float);
    LogStream& operator<<(double);
    LogStream& operator<<(long double);

    LogStream& operator<<(char);
    LogStream& operator<<(const char*);
    LogStream& operator<<(const unsigned char*);
    LogStream& operator<<(const std::string&);

    void Write(const char* buffer, int size) { buffer_.write(buffer, size); }
    const Buffer& buffer() const { return buffer_; }
    void clear() { buffer_.clear(); }

private:
    template <typename T>
    void FormatInteger(T value);

    static const int kMaxNumberSize = 32;

    Buffer buffer_;
};
    
} // namespace log
