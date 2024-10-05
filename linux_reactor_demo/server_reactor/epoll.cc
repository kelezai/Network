#include "epoll.h"
#include "logging.h"

#include "channel.h"

#define LOG()           LOG_PRINT("serv")

Epoll::Epoll() {
    if ((epoll_fd_ = epoll_create(1)) < 0) {
        perror("epoll_create");
        exit(-1);
    }
}

Epoll::~Epoll() {
    close(epoll_fd_);
}

void Epoll::update_channel(Channel * channel) {
    epoll_event event;
    event.data.ptr = channel;
    event.events =  channel->events();

    std::string op;
    int result = 0;

    if (channel->is_in_epoll()) {
        op = "EPOLL_CTL_MOD";
        result = epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, channel->fd(), &event);
    }
    else {
        op = "EPOLL_CTL_ADD";
        result = epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, channel->fd(), &event);
        channel->set_in_epoll(true);
    }

    if (result == -1) {
        std::string error_msg = "epoll_ctl(";
        error_msg += "fd: " + std::to_string(channel->fd());
        error_msg += op;
        error_msg += ")";
        perror(error_msg.c_str());
        exit(-1);
    }
}

void Epoll::remove_channel(Channel * channel) {
    if (channel->is_in_epoll()) {
        if (epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, channel->fd(), 0) == -1) {
            perror("epoll_ctl(EPOLL_CTL_DEL)");
            exit(-1);
        }
        channel->set_in_epoll(false);
    }
}

std::vector<std::pair<Channel*, uint32_t>> Epoll::loop(int timeout /*= -1*/) {
    std::vector<std::pair<Channel*, uint32_t>> channel_revents;

    int count = epoll_wait(epoll_fd_, events_, MaxEvents, timeout);

    // 返回失败
    if (count < 0) {
        // EBADF    : epoll_fd_ 是无效的 fd.
        // EFAULT   : 参数 events_ 指向内存区域不可写
        // EINVAL   : epoll_fd_ 不是 epoll 的 fd, 或者参数 MaxEvents <= 0
        // EINTR    : 阻塞过程中被信号中断, `epoll_pwait` 可以避免被信号中断; 或者在遇到 EINTR 之后重新调用 epoll_wait.
        perror("epoll_wait failed.");
        exit(-1);
    }

    // 超时
    if (count == 0) {
        // 表示关注的事件没有被触发, 返回的 channels 为空.
        return channel_revents;
    }

    // 有事件发生
    for (int i = 0; i < count; ++i) {
        Channel * channel = static_cast<Channel*>(events_[i].data.ptr);
        uint32_t revents = events_[i].events;
        channel_revents.push_back(std::make_pair(channel, revents));
    }

    return channel_revents;
}