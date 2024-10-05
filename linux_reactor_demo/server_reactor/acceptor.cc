#include "acceptor.h"
#include "logging.h"
#include "connection.h"

#define  LOG()                   LOG_PRINT("serv")

Acceptor::Acceptor(EventLoop * loop, const std::string & server_ip, uint16_t server_port) 
    : loop_(loop)
    , server_sock_(create_nonblocking_socket(), server_ip, server_port)
    , accept_channel_(server_sock_.fd(), true) {

    InetAddress server_addr(server_ip, server_port);

    server_sock_.set_opt_reuseaddr(true);
    server_sock_.set_opt_reuseport(true);
    server_sock_.set_opt_keepalive(true);
    server_sock_.set_opt_nodelay(true);
    server_sock_.bind();
    server_sock_.listen();

    LOG() << "server(" << server_sock_.ip() << ", " << server_sock_.port() << ") is listening...";

    accept_channel_.set_read_callback(std::bind(&Acceptor::handle_connect, this));
    accept_channel_.enable_read();

    loop->update_channel(&accept_channel_);
}

Acceptor::~Acceptor() {
    loop_->remove_channel(&accept_channel_);
}

void Acceptor::set_connect_callback(std::function<void(std::unique_ptr<Socket>)> fn) {
    connect_callback_ = fn;
}

void Acceptor::handle_connect() {
    InetAddress client_addr;
    int client_fd = server_sock_.accept(client_addr);

    auto client_socket = std::make_unique<Socket>(client_fd, client_addr.ip(), client_addr.port());
    connect_callback_(std::move(client_socket));
}