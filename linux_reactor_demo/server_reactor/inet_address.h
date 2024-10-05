#pragma once

#include <string>

#include <sys/socket.h>
#include <netinet/in.h>

class InetAddress {
public:
    InetAddress() = default;
    InetAddress(const std::string& ip, uint16_t port);
    InetAddress(sockaddr_in addr);

    const char * ip() const;
    uint16_t port() const;
    const sockaddr * addr() const;

private:
    sockaddr_in addr_;
};