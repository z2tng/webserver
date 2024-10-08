#include "net/buffer.h"

#include <errno.h>
#include <sys/uio.h>
#include <unistd.h>

namespace net {

const char Buffer::kCRLF[] = "\r\n";

ssize_t Buffer::ReadFd(int fd, int *saved_errno) {
    char buffer[65536];
    struct iovec vec[2];
    const size_t writable = WritableBytes();

    vec[0].iov_base = begin() + write_index_;
    vec[0].iov_len = writable;
    vec[1].iov_base = buffer;
    vec[1].iov_len = sizeof(buffer);

    const int iovcnt = (writable < sizeof(buffer)) ? 2 : 1;
    const ssize_t n = readv(fd, vec, iovcnt);
    if (n < 0) {
        *saved_errno = errno;
    } else if (static_cast<size_t>(n) <= writable) {
        write_index_ += n;
    } else {
        write_index_ = buffer_.size();
        Append(buffer, n);
    }

    return n;
}

ssize_t Buffer::WriteFd(int fd, int *saved_errno) {
    const size_t readable = ReadableBytes();
    const ssize_t n = write(fd, Peek(), readable);
    if (n < 0) {
        *saved_errno = errno;
    }

    return n;
}

void Buffer::MakeSpace(size_t len) {
    if (WritableBytes() + PrependableBytes() < len + kPrependSize) {
        buffer_.resize(write_index_ + len);
    } else {
        size_t readable = ReadableBytes();
        std::copy(begin() + read_index_, begin() + write_index_, begin() + kPrependSize);
        read_index_ = kPrependSize;
        write_index_ = read_index_ + readable;
    }
}
    
} // namespace connection
