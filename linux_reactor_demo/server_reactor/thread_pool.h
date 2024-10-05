#pragma once

#include <vector>
#include <thread>
#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <atomic>

class ThreadPool {
public:
    ThreadPool(size_t thread_num, const std::string& tag);
    ~ThreadPool();

    void add_task(std::function<void()> task);                      // 把任务添加到队列里

    void stop();

    size_t size();                                                  // 获得线程池大小

private:
    std::vector<std::thread> threads_;
    std::queue<std::function<void()>> task_queue_;
    std::mutex task_queue_mtx_;
    std::condition_variable condition_;
    std::atomic_bool stop_;
};