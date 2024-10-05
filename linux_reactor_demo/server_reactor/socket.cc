#include "socket.h"

int create_nonblocking_socket() {
    int fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);
    if (fd < 0) {
        perror("socket");
        exit(-1);
    }
    return fd;
}

Socket::Socket(int fd, const std::string& ip, uint16_t port) {
    fd_ = fd;
    ip_ = ip;
    port_ = port;
}

Socket::~Socket() {
    close(fd_);
}

void Socket::set_opt_reuseaddr(bool enable) {
    int opt = 1;
    setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &opt, static_cast<socklen_t>(sizeof opt));
}

void Socket::set_opt_reuseport(bool enable) {
    int opt = 1;
    setsockopt(fd_, SOL_SOCKET, SO_REUSEPORT, &opt, static_cast<socklen_t>(sizeof opt));
}

void Socket::set_opt_keepalive(bool enable) {
    int opt = 1;
    setsockopt(fd_, SOL_SOCKET, SO_KEEPALIVE, &opt, static_cast<socklen_t>(sizeof opt));
}

void Socket::set_opt_nodelay(bool enable) {
    int opt = 1;
    setsockopt(fd_, SOL_SOCKET, TCP_NODELAY, &opt, static_cast<socklen_t>(sizeof opt));
}

void Socket::bind() {
    InetAddress serv_addr(ip_, port_);
    if (::bind(fd_, serv_addr.addr(), sizeof(sockaddr)) < 0) {
        perror("bind");
        close(fd_);
        exit(-1);
    }

    ip_ = serv_addr.ip();
    port_ = serv_addr.port();
}

void Socket::listen() {
    if (::listen(fd_, 128) != 0) {
        perror("listen");
        close(fd_);
        exit(-1);
    }
}

int  Socket::accept(InetAddress & client_addr) {
    sockaddr_in peer_addr;
    socklen_t len = sizeof(peer_addr);

    int client_fd = accept4(fd_, (sockaddr*)&peer_addr, &len, SOCK_NONBLOCK);

    client_addr = InetAddress(peer_addr);

    return client_fd;
}

int Socket::fd() const {
    return fd_;
}

std::string Socket::ip() const {
    return ip_;
}

uint16_t Socket::port() const {
    return port_;
}