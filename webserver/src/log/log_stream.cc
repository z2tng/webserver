#include "log/log_stream.h"

#include <algorithm>

namespace logging {

template <typename T>
size_t IntToString(char *buffer, T value) {
    char *buf = buffer;
    bool is_negative = false;

    if (value < 0) {
        is_negative = true;
        value = -value;
    }

    while (value) {
        *buf++ = '0' + value % 10;
        value /= 10;
    }

    if (is_negative) *buf++ = '-';
    *buf = '\0';

    std::reverse(buffer, buf);
    return buf - buffer;
}

template <typename T>
void LogStream::FormatInteger(T value) {
    if (buffer_.capacity() >= kMaxNumberSize) {
        char buf[kMaxNumberSize];
        int len = IntToString(buf, value);
        Write(buf, len);
    }
}

LogStream& LogStream::operator<<(bool value) {
    Write(value ? "1" : "0", 1);
    return *this;
}

LogStream& LogStream::operator<<(short value) {
    return *this << static_cast<int>(value);
}

LogStream& LogStream::operator<<(unsigned short value) {
    return *this << static_cast<unsigned int>(value);
}

LogStream& LogStream::operator<<(int value) {
    FormatInteger(value);
    return *this;
}

LogStream& LogStream::operator<<(unsigned int value) {
    FormatInteger(value);
    return *this;
}

LogStream& LogStream::operator<<(long value) {
    FormatInteger(value);
    return *this;
}

LogStream& LogStream::operator<<(unsigned long value) {
    FormatInteger(value);
    return *this;
}

LogStream& LogStream::operator<<(long long value) {
    FormatInteger(value);
    return *this;
}

LogStream& LogStream::operator<<(unsigned long long value) {
    FormatInteger(value);
    return *this;
}

LogStream& LogStream::operator<<(float value) {
    return *this << static_cast<double>(value);
}

LogStream& LogStream::operator<<(double value) {
    if (buffer_.capacity() >= kMaxNumberSize) {
        char buf[kMaxNumberSize];
        int len = snprintf(buf, kMaxNumberSize, "%.12g", value);
        Write(buf, len);
    }
    return *this;
}

LogStream& LogStream::operator<<(long double value) {
    if (buffer_.capacity() >= kMaxNumberSize) {
        char buf[kMaxNumberSize];
        int len = snprintf(buf, kMaxNumberSize, "%.12Lg", value);
        Write(buf, len);
    }
    return *this;
}

LogStream& LogStream::operator<<(char value) {
    Write(&value, 1);
    return *this;
}

LogStream& LogStream::operator<<(const char* value) {
    if (value) {
        Write(value, strlen(value));
    } else {
        Write("(null)", 6);
    }
    return *this;
}

LogStream& LogStream::operator<<(const unsigned char* value) {
    return operator<<(reinterpret_cast<const char*>(value));
}

LogStream& LogStream::operator<<(const std::string& value) {
    Write(value.c_str(), value.size());
    return *this;
}

}