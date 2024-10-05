#include "logging.h"
#include <pthread.h>
#include <atomic>
#include <unistd.h>
#include <memory>
#include <map>
#include <sys/eventfd.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <sys/timerfd.h>
#include <signal.h>
#include <sys/signalfd.h>
#include <shared_mutex>
#include <future>

#include "../server_15/thread_pool.h"

#define LOG()               LOG_PRINT("main")



int main(int argc, const char * argv[]) {
    std::cout << "demo" << std::endl;


    return 0;
}
