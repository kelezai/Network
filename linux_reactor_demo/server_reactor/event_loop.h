#pragma once

#include <atomic>
#include <functional>
#include <queue>
#include <mutex>
#include <memory>
#include <map>
#include <sys/timerfd.h>

#include "epoll.h"

class Channel;
class Connection;

// 事件循环
class EventLoop {
public:
    EventLoop(int timer_interval);
    ~EventLoop();

    void run();                                                         // 运行事件循环
    void stop();                                                        // 退出事件循环

    void update_channel(Channel * chn);                                 // 把 channel 更新到 epoll 的红黑树上
    void remove_channel(Channel * chn);                                 // 把 channel 

    void set_idle_callback(std::function<void(EventLoop*)> fn);         // idle,  epoll_wait 超时
    void set_timer_callback(std::function<void(EventLoop *)> fn);       // timer, timer_fd 被触发

    bool current_thread_is_loop_thread();                               // 判断当前线程是否为时间循环所在线程
    void add_to_task_queue(std::function<void(void)> fn);

protected:
    void invoke_task_event();
    void handle_task_event();
    void handle_timer();

private:
    std::unique_ptr<Epoll>  epoll_;                                     // 每个 EventLoop 只有一个 Epoll

private:
    pid_t loop_thread_id_;                                              // 事件循环所在线程的 id

private:
    std::function<void(EventLoop*)> idle_callback_;

private:
    std::queue<std::function<void(void)>> task_queue_;                  // 事件循环被 eventfd 唤醒后, 执行任务队列里的任务.
    std::mutex task_queue_mtx_;

    int task_event_fd_ = -1;                                            // 唤醒事件循环线程的 eventfd
    std::unique_ptr<Channel> task_event_channel_;                       // eventfd 的channel

private:
    itimerspec timer_spec_ = { 0 };
    int timer_fd_ = -1;
    std::unique_ptr<Channel> timer_channel_;

private:
    std::function<void(EventLoop *)> timer_callback_;

private:
    std::atomic_bool stop_;                                             // 退出事件循环表示
};