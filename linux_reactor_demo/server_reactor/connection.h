#pragma once

#include <functional>
#include <memory>
#include <atomic>
#include <queue>
#include <mutex>

#include "socket.h"
#include "inet_address.h"
#include "channel.h"
#include "event_loop.h"
#include "buffer.h"
#include "time_stamp.h"

class Connection;
using SpConnection = std::shared_ptr<Connection>;
using WpConnection = std::weak_ptr<Connection>;

class Connection : public std::enable_shared_from_this<Connection> {
public:
    Connection(EventLoop * loop, std::unique_ptr<Socket> client_sock);
    ~Connection();
    void start();

public:
    int fd() const;
    std::string ip() const;
    uint16_t port() const;

    void handle_message();                                                      // 处理对端发送来的数据,           供 Channel 回调
    void handle_close();                                                        // TCP 连接关闭 (断开) 的回调函数,  供 Channel 回调
    void handle_error();                                                        // TCP 连接错误的回调函数,         供 Channel 回调
    void handle_write();                                                        // 处理写事件回调函数,             供 Channel 回调

    void set_close_callback(std::function<void(SpConnection)> fn);
    void set_error_callback(std::function<void(SpConnection)> fn);
    void set_message_callback(std::function<void(SpConnection, const std::string&)> fn);
    void set_send_complete_callback(std::function<void(SpConnection)> fn);

    void send(const char * data, size_t size);                                  // 发送数据, 不考虑当前线程

    bool is_timeout(int thresh);

public:
    void send_in_loop(const char * data, size_t size);                          // 发送数据, 如果当前线程是 IO 线程, 直接发送; 如果当前线程是 work 线程, 把这个函数转到 IO 线程

private:
    EventLoop               * loop_;                                            // Connection 对应的事件循环, 构造函数中传入
    std::unique_ptr<Socket>   client_sock_;                                     // 与客户端通信的 socket, 构造函数中传入
    std::unique_ptr<Channel>  client_channel_;                                  // Connection 对应的 channel, 构造函数中创建

    std::function<void(SpConnection)> close_callback_;                          // fd 关闭时的回调函数, 回调 TcpServer::handle_close
    std::function<void(SpConnection)> error_callback_;                          // fd 错误时的回调函数, 回调 TcpServer::handle_error
    std::function<void(SpConnection, const std::string&)> message_callback_;    // fd 错误时的回调函数, 回调 TcpServer::handle_error
    std::function<void(SpConnection)> send_complete_callback_;                  // 发送数据完毕的回调,  回调 TcpServer::send_complate

private:
    InputBuffer input_buffer_;                                                  // 接收缓冲区
    OutputBuffer output_buffer_;                                                // 发送缓冲区

private:
    Timestamp last_time_;                                                       // 创建 Connection 时, 为当前时间戳; 以后每收到一个报文, 时间戳更新为当前时间;

private:
    std::atomic_bool disconnect_;
};