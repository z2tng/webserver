#include "connection/inet_address.h"

#include <strings.h>

namespace connection {

InetAddress::InetAddress(uint16_t port, const std::string &ip) {
    bzero(&addr_, sizeof(addr_));
    addr_.sin_family = AF_INET;
    addr_.sin_port = htons(port);
    addr_.sin_addr.s_addr = inet_addr(ip.c_str());
    port_ = port;
    ip_ = ip;
}

InetAddress::InetAddress(const sockaddr_in &addr) : addr_(addr) {
    port_ = ntohs(addr_.sin_port);
    ip_ = inet_ntoa(addr_.sin_addr);
}
    
} // namespace connection
