#include "connection/socket.h"
#include "connection/inet_address.h"
#include "log/logger.h"

#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <strings.h>

namespace connection {

Socket::~Socket() {
    ::close(sockfd_);
}

void Socket::Bind(const InetAddress &addr) {
    if (0 > ::bind(sockfd_,
                   reinterpret_cast<const sockaddr*>(addr.GetSockAddr()),
                   sizeof(sockaddr))) {
        LOG_FATAL << "bind socket: " << sockfd_ << " error";
    }
}

void Socket::Listen() {
    if (0 > ::listen(sockfd_, SOMAXCONN)) {
        LOG_FATAL << "listen socket: " << sockfd_ << " error";
    }
}

int Socket::Accept(InetAddress *peer_addr) {
    sockaddr_in addr;
    socklen_t len = sizeof(addr);
    bzero(&addr, sizeof(addr));

    int connfd = ::accept4(sockfd_,
                           reinterpret_cast<sockaddr*>(&addr),
                           &len,
                           SOCK_NONBLOCK | SOCK_CLOEXEC);
    if (connfd >= 0) peer_addr->SetSockAddr(addr);
    return connfd;
}

void Socket::ShutdownWrite() {
    if (0 > ::shutdown(sockfd_, SHUT_WR)) {
        LOG_ERROR << "shutdown write socket: " << sockfd_ << " error";
    }
}

void Socket::SetTcpNoDelay(bool on) {
    int optval = on ? 1 : 0;
    if (0 > ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY, &optval, static_cast<socklen_t>(sizeof(optval)))) {
        LOG_ERROR << "setsockopt TCP_NODELAY socket: " << sockfd_ << " error";
    }
}

void Socket::SetReuseAddr(bool on) {
    int optval = on ? 1 : 0;
    if (0 > ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &optval, static_cast<socklen_t>(sizeof(optval)))) {
        LOG_ERROR << "setsockopt SO_REUSEADDR socket: " << sockfd_ << " error";
    }
}

void Socket::SetReusePort(bool on) {
    int optval = on ? 1 : 0;
    if (0 > ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT, &optval, static_cast<socklen_t>(sizeof(optval)))) {
        LOG_ERROR << "setsockopt SO_REUSEPORT socket: " << sockfd_ << " error";
    }
}

void Socket::SetKeepAlive(bool on) {
    int optval = on ? 1 : 0;
    if (0 > ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE, &optval, static_cast<socklen_t>(sizeof(optval)))) {
        LOG_ERROR << "setsockopt SO_KEEPALIVE socket: " << sockfd_ << " error";
    }
}
    
} // namespace connection

