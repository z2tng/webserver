#pragma once

#include "utils/uncopyable.h"

namespace net {

class InetAddress;

class Socket : utils::Uncopyable {
public:
    explicit Socket(int sockfd) : sockfd_(sockfd) {}
    ~Socket();

    void Bind(const InetAddress &addr);
    void Listen();
    int Accept(InetAddress *peer_addr);

    void ShutdownWrite();

    void SetTcpNoDelay(bool on);
    void SetReuseAddr(bool on);
    void SetReusePort(bool on);
    void SetKeepAlive(bool on);

    int fd() const { return sockfd_; }
private:
    int sockfd_;
};
    
} // namespace connection
