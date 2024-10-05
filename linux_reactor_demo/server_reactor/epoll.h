#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <vector>

#include <unistd.h>
#include <errno.h>
#include <sys/epoll.h>

class Channel;

class Epoll {
public:
    Epoll();
    ~Epoll();

    void update_channel(Channel * channel);                             // 把 channel 添加/更新到 epoll 对象上, channel 中包含 fd
    void remove_channel(Channel * channel);

    std::vector<std::pair<Channel*, uint32_t>> loop(int timeout = -1);  // epoll_wait

private:
    static const int MaxEvents = 10;
    epoll_event events_[MaxEvents];
    int epoll_fd_ = -1;
};