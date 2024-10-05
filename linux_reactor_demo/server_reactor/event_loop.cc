#include "event_loop.h"
#include "logging.h"

#include <vector>
#include <unistd.h>
#include <syscall.h>
#include <sys/eventfd.h>

#include "time_stamp.h"
#include "channel.h"
#include "connection.h"

#define LOG()           LOG_PRINT("serv")

EventLoop::EventLoop(int timer_interval)
    : epoll_(new Epoll())
    , task_event_fd_(eventfd(0, EFD_NONBLOCK))
    , task_event_channel_(std::make_unique<Channel>(task_event_fd_, true))
    , timer_fd_(timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK)) 
    , timer_channel_(std::make_unique<Channel>(timer_fd_, true))
    , stop_(false) {

    // task event
    task_event_channel_->set_read_callback(std::bind(&EventLoop::handle_task_event, this));
    task_event_channel_->enable_read();
    update_channel(task_event_channel_.get());

    // timer
    timer_spec_.it_value.tv_sec = timer_interval;
    timer_spec_.it_value.tv_nsec = 0;
    timerfd_settime(timer_fd_, 0, &timer_spec_, 0);

    timer_channel_->set_read_callback(std::bind(&EventLoop::handle_timer, this));
    timer_channel_->enable_read();
    update_channel(timer_channel_.get());
}

EventLoop::~EventLoop() {
    close(task_event_fd_);
}

void EventLoop::run() {
    loop_thread_id_ = syscall(SYS_gettid);                      // 获取事件循环所在线程的 id

    while (!stop_) {
        std::vector<std::pair<Channel*, uint32_t>> channel_revents = epoll_->loop(5*1000);

        // 如果 channles 为空, 表示超时, 回调 TcpServer::handle_idle();
        if (channel_revents.empty()) {
            idle_callback_(this);
        }
        else {
            for (auto & cr : channel_revents) {
                cr.first->handle_event(cr.second);
            }
        }
    }
}

// 停止事件循环
void EventLoop::stop() {
    stop_ = true;
    invoke_task_event();                                        // 因为阻塞在 epoll_wait 所以需要唤醒它
}

void EventLoop::update_channel(Channel * chn) {
    epoll_->update_channel(chn);
}

void EventLoop::remove_channel(Channel * chn) {
    epoll_->remove_channel(chn);
}

void EventLoop::set_idle_callback(std::function<void(EventLoop*)> fn) {
    idle_callback_ = fn;
}

void EventLoop::set_timer_callback(std::function<void(EventLoop*)> fn) {
    timer_callback_ = fn;
}

bool EventLoop::current_thread_is_loop_thread() {
    return syscall(SYS_gettid) == loop_thread_id_;
}

void EventLoop::add_to_task_queue(std::function<void(void)> fn) {
    // 任务入队
    {
        std::lock_guard<std::mutex> lock(task_queue_mtx_);
        task_queue_.push(fn);
    }

    // 唤醒消息循环
    invoke_task_event();
}

void EventLoop::invoke_task_event() {
    uint64_t val = 1;
    write(task_event_fd_, &val, sizeof(uint64_t));
}

void EventLoop::handle_task_event() {
    // 从 eventfd 中读取数据, 如果不读取, 在水平触发的模式下, eventfd 的读事件会一直触发
    uint64_t val = 0;
    read(task_event_fd_, &val, sizeof(uint64_t));

    // 执行所有任务
    std::function<void()> fn;
    std::lock_guard<std::mutex> lock(task_queue_mtx_);
    while (!task_queue_.empty()) {
        fn = std::move(task_queue_.front());
        task_queue_.pop();
        fn();
    }
}

void EventLoop::handle_timer() {
    if (timer_callback_) {
        timerfd_settime(timer_fd_, 0, &timer_spec_, 0);
        timer_callback_(this);
    }
}