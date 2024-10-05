#pragma once
#include <memory>
#include <functional>
#include <sys/epoll.h>

class Socket;

// Channel : Epoll, 多对一的关系
class Channel {
public:
    Channel(int fd, bool use_et);

    int fd() const;

public:
    void enable_read();                                 // epoll_wait 监视 fd 的读事件
    void disable_read();

    void enable_write();                                // epoll_wait 监视 fd 的写事件
    void disable_write();

    void set_in_epoll(bool is_in_epoll);
    bool is_in_epoll();

    uint32_t events();                                  // epoll_wait 监视的所有事件

public:
    void set_read_callback(std::function<void()> fn);
    void set_close_callback(std::function<void()> fn);
    void set_error_callback(std::function<void()> fn);
    void set_write_callback(std::function<void()> fn);

public:
    void handle_event(uint32_t revents);                // 处理 epoll 返回事件

private:
    int         fd_ = -1;                               // Channel 和 fd   : 一对一的关系
    uint32_t    events_ = 0;                            // fd 需要监视的事件, listen_fd 需要监视: EPOLLIN, client_fd 需要监视 EPOLLIN | EPOLLET

private:
    std::function<void()> close_callback_;              // fd 关闭事件的回调函数, 将回调 Connection::handle_close();
    std::function<void()> error_callback_;              // fd 错误事件的回调函数, 将回调 Connection::handle_error();
    std::function<void()> read_callback_;               // fd 读事件的回调函数, 如果是 Accept Channel, 回调 Acceptor::handle_connect;
                                                        //                   如果是 Connection, 回调 Connection::handle_message;
    std::function<void()> write_callback_;              // fd 写事件的回调函数, 将回调 Connection::handle_write();

private:
    bool is_in_epoll_ = false;
};