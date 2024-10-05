#pragma once

#include "tcp_server.h"
#include "event_loop.h"
#include "connection.h"
#include "thread_pool.h"


/**
 * EchoServer 回显服务器
 */

class EchoServer {
public:
    EchoServer(const std::string& ip, uint16_t port, int work_thread_num);
    ~EchoServer();

    void Start();                                                           // 启动服务
    void Stop();                                                            // 停止服务

    void handle_connect(SpConnection conn);                                 // 处理新客户端连接请求, Acceptor 类中回调该函数
    void handle_disconnect(SpConnection conn);                              // 处理客户端断连事件, Acceptor 类中回调该函数
    void handle_message(SpConnection conn, const std::string& message);     // 处理收到的消息
    void handle_send_complete(SpConnection conn);                           // 数据发送完成回调
    void handle_idle(EventLoop * loop);                                     // epoll_wait 超时, 没有任何事件发生

protected:
    void process_message(SpConnection conn, const std::string& message);

private:
    TcpServer   tcp_server_;
    int         work_thread_num_;
    ThreadPool  work_thread_pool_;
};