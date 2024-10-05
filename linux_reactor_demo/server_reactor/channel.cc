#include "channel.h"
#include "logging.h"
#include "inet_address.h"
#include "connection.h"
#include "socket.h"

#define LOG()               LOG_PRINT("serv")

Channel::Channel(int fd, bool use_et) : fd_(fd) {
    if (use_et) {
        events_ |= EPOLLET;
    }
    else {
        events_ &= EPOLLET;
    }
}

int Channel::fd() const {
    return fd_;
}

void Channel::enable_read() {
    events_ |= EPOLLIN;
}

void Channel::disable_read() {
    events_ &= ~EPOLLIN;
}

void Channel::enable_write() {
    events_ |= EPOLLOUT;
}

void Channel::disable_write() {
    events_ &= ~EPOLLOUT;
}

void Channel::set_read_callback(std::function<void()> fn) {
    read_callback_ = fn;
}

void Channel::set_close_callback(std::function<void()> fn) {
    close_callback_ = fn;
}

void Channel::set_error_callback(std::function<void()> fn) {
    error_callback_ = fn;
}

void Channel::set_write_callback(std::function<void()> fn) {
    write_callback_ = fn;
}

uint32_t Channel::events() {
    return events_;
}

void Channel::handle_event(uint32_t revents) {
    // 对方已关闭, 有些系统检测不到, 可以用 EPOLLIN, recv() == 0
    if (revents & EPOLLHUP) {
        close_callback_();
    }
    else if (revents & (EPOLLIN | EPOLLPRI)) {              // 针对 listen fd, 事件表示有新的连接
        read_callback_();                                   // 对于和客户端连接的 fd, 根据 readn = recv() 结果:
    }                                                       //      1) readn > 0 接收缓冲区有数据。
                                                            //      2) readn = 0 客户端关闭连接。
                                                            //      3) readn == -1 && errno == EINTR 被信号中断
                                                            //      4) readn == -1 && (errno == EAGAIN || errno == EWOULDBLOCK) 接收缓冲区空
    // 发送缓冲区有空间
    else if (revents & EPOLLOUT) {
        write_callback_();
    }
    else {
        error_callback_();
    }
}

void Channel::set_in_epoll(bool is_in_epoll) {
    is_in_epoll_ = is_in_epoll;
}

bool Channel::is_in_epoll() {
    return is_in_epoll_;
}