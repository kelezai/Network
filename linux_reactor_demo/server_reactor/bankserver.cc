/**
 * sockaddr_in 封装, socket 封装, epoll 封装, channel 分装 epoll, callback, eventloop, acceptor, connection
 */

#include "logging.h"

#include "inet_address.h"
#include "socket.h"
#include "epoll.h"
#include "channel.h"
#include "event_loop.h"
#include "tcp_server.h"
#include "echo_server.h"
#include "bank_server.h"

#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/tcp.h>    // TCP_NODELAY
#include <fcntl.h>
#include <sys/epoll.h>
#include <signal.h>

#define LOG()               LOG_PRINT("serv")

BankServer * bank_server;

// 处理信号 2 和 信号 15
void sig_stop_proc(int signo) {
    LOG() << "sig: " << signo;
    bank_server->Stop();
    delete bank_server;
    exit(0);
}

int main(int argc, const char * argv[]) {
    if (argc != 3) {
        LOG() << "usage: ./x-server 10.0.2.15 5005";
        return -1;
    }

    signal(SIGINT, sig_stop_proc);
    signal(SIGTERM, sig_stop_proc);

    bank_server = new BankServer(argv[1], atoi(argv[2]), 2);
    bank_server->Start();

    return 0;
}