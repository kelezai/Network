#include "thread_pool.h"
#include "logging.h"

#include <unistd.h>
#include <syscall.h>

#define  LOG()          LOG_PRINT("serv")

ThreadPool::ThreadPool(size_t thread_num, const std::string& tag) 
    : stop_(false) {

    for (size_t i = 0; i < thread_num; ++i) {
        threads_.emplace_back([this, tag] {
            LOG() << tag << ", create thread: " << syscall(SYS_gettid);

            while (!stop_) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(task_queue_mtx_);

                    condition_.wait(lock, [this] {
                        return stop_ || task_queue_.size() > 0;             // 一直等到 stop 或者 task_queue 有内容
                    });

                    if (stop_ && task_queue_.empty()) {
                        LOG() << tag << ", thread " << syscall(SYS_gettid) << " exit, stop and empty";
                        return;
                    }

                    task = std::move(task_queue_.front());
                    task_queue_.pop();
                }

                task();
            }

            {
                std::unique_lock<std::mutex> lock(task_queue_mtx_);
                while (!task_queue_.empty()) {
                    auto task = std::move(task_queue_.front());
                    task_queue_.pop();
                    task();
                } 
            }

            LOG() << tag << ", thread " << syscall(SYS_gettid) << " exit";
        });
    }
}

ThreadPool::~ThreadPool() {
    stop();
}

void ThreadPool::add_task(std::function<void()> task) {
    {
        std::unique_lock<std::mutex> lock(task_queue_mtx_);
        task_queue_.push(task);
        condition_.notify_one();
    }
}

size_t ThreadPool::size() {
    return threads_.size();
}

void ThreadPool::stop() {
    {
        std::unique_lock<std::mutex> lock(task_queue_mtx_);
        if (stop_) {
            return;
        }
        stop_ = true;
        condition_.notify_all();
    }

    for (auto & thd : threads_) {
        if (thd.joinable()) {
            thd.join();
        }
    }
}
