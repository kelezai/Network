#pragma once

#include <memory>

#include "event_loop.h"
#include "socket.h"
#include "channel.h"
#include "acceptor.h"
#include "connection.h"
#include "thread_pool.h"

#include <map>

class Connection;

// 网络服务类
class TcpServer {
public:
    TcpServer(const std::string& ip, uint16_t port, int thread_num);
    ~TcpServer();

    void start();                                                       // 运行事件循环
    void stop();                                                        // 停止事件循环、IO 线程

    void handle_connect(std::unique_ptr<Socket> client_sock);           // 处理新客户端连接请求, Acceptor 类中回调该函数
    void handle_close(SpConnection conn);                               // 处理客户端断开连接, 在 Connection 类中回调此函数
    void handle_error(SpConnection conn);                               // 处理客户端出错连接, 在 Connecttion 类中回调此函数
    void handle_message(SpConnection conn, const std::string& message); // 处理客户端发送的消息
    void send_complete(SpConnection conn);                              // 数据发送完成回调
    void handle_idle(EventLoop * loop);                                 // epoll_wait 超时, epoll 没有收到任何事件

public:
    void set_connect_callback(std::function<void(SpConnection)> fn);
    void set_disconnect_callback(std::function<void(SpConnection)> fn);
    void set_message_callback(std::function<void(SpConnection, const std::string&)> fn);
    void set_idle_callback(std::function<void(EventLoop*)> fn);
    void set_send_complete_callback(std::function<void(SpConnection)> fn);

protected:
    void remove_connect(SpConnection conn);
    void handle_timer(EventLoop * sub_loop);

private:
    std::unique_ptr<EventLoop>   main_loop_;                            // 关联主线程,   用于 Acceptor
    std::vector<std::unique_ptr<EventLoop>> sub_loops_;                 // 关联子线程池, 用于 Connection

    ThreadPool * sub_loops_thread_pool_;                                // sub_loops_ 关联的线程池

    Acceptor acceptor_;                                                 // 一个 TcpServer 只有一个 Acceptor 对象

    std::map<int, SpConnection> conns_;                                 // 一个 TcpServer 可以有多个 Connections
    std::mutex conns_mtx_;

    struct ConnSet {
        std::mutex mtx;
        std::vector<WpConnection> conns;                                // 一个 EventLoop 上的 Connection
    };
    std::map<EventLoop*, ConnSet*> sub_loops_conns_;

private:
    std::function<void(SpConnection)> connect_callback_;
    std::function<void(SpConnection)> disconnect_callback_;
    // std::function<void(SpConnection)> close_callback_;
    // std::function<void(SpConnection)> error_callback_;
    std::function<void(SpConnection, const std::string&)> message_callback_;
    std::function<void(SpConnection)> send_complete_callback_;
    std::function<void(EventLoop*)> idle_callback_;

};