#pragma once

#include <functional>
#include <memory>

#include "socket.h"
#include "inet_address.h"
#include "channel.h"
#include "event_loop.h"

class Acceptor {
public:
    Acceptor(EventLoop * loop, const std::string & server_ip, uint16_t server_port);
    ~Acceptor();

    void handle_connect();

    void set_connect_callback(std::function<void(std::unique_ptr<Socket>)> fn);

private:
    EventLoop * loop_;                                                  // Acceptor 对应的事件循环, 构造函数中传入
    Socket      server_sock_;                                           // 服务端用于监听的 socket, 构造函数中创建
    Channel     accept_channel_;                                        // Acceptor 对应的 channel, 构造函数中创建

    std::function<void(std::unique_ptr<Socket>)> connect_callback_;     // 处理新客户端连接请求的的回调函数
};