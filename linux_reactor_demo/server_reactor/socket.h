#pragma once
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "inet_address.h"

int create_nonblocking_socket();

class Socket {
public:
    Socket(int fd, const std::string& ip, uint16_t port);
    ~Socket();

public:
    int fd() const;
    std::string ip() const;
    uint16_t port() const;

    void set_opt_reuseaddr(bool enable);            // SO_REUSEADDR
    void set_opt_reuseport(bool enable);            // SO_REUSEPORT
    void set_opt_keepalive(bool enable);            // SO_KEEPALIVE
    void set_opt_nodelay(bool enable);              // TCP_NODELAY

    void bind();
    void listen();
    int accept(InetAddress & client_addr);

private:
    int fd_;
    std::string ip_;                                // 如果是 listen_fd, 保存的是服务端监听的 ip;
                                                    // 如果是 客户端连接的 fd, 保存的是对端的 ip;
    uint16_t port_;                                 
};