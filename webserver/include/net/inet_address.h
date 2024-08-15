#pragma once

#include <arpa/inet.h>
#include <string>

namespace net {

class InetAddress {
public:
    explicit InetAddress(uint16_t port = 0, const std::string &ip = "127.0.0.1");
    explicit InetAddress(const sockaddr_in &addr);

    std::string GetIp() const { return ip_; }
    uint16_t GetPort() const { return port_; }
    std::string GetIpPort() const { return ip_ + ":" + std::to_string(port_); }

    const sockaddr_in* GetSockAddr() const { return &addr_; }
    void SetSockAddr(const sockaddr_in &addr) { addr_ = addr; }
private:
    sockaddr_in addr_;
    uint16_t port_;
    std::string ip_;
};
    
} // namespace connection
