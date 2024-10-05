/**
 * epoll + socket/accpet4 直接设置 非阻塞 io
 */

#include "logging.h"

#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/tcp.h>    // TCP_NODELAY
#include <fcntl.h>
#include <sys/epoll.h>

#define LOG()               LOG_PRINT("serv")

int main(int argc, const char * argv[]) {
    if (argc != 2) {
        LOG() << "usage: ./x-server port";
        return -1;
    }

    int listen_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);
    if (listen_fd < 0) {
        perror("socket");
        return -1;
    }

    // 设置属性
    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, static_cast<socklen_t>(sizeof opt));  // 必须: 重用地址+port
    setsockopt(listen_fd, SOL_SOCKET, TCP_NODELAY,  &opt, static_cast<socklen_t>(sizeof opt));  // 必须: 禁用 Nagle 算法
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEPORT, &opt, static_cast<socklen_t>(sizeof opt));  // 有用: 但是在 reactor 模型中意义不大
    setsockopt(listen_fd, SOL_SOCKET, SO_KEEPALIVE, &opt, static_cast<socklen_t>(sizeof opt));  // 可能有用, 但是建议自己做心跳

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(atoi(argv[1]));

    if (bind(listen_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("bind");
        return -1;
    }

    if (listen(listen_fd, 128) != 0) {                                          // 128：全连接队列大小, 高并发网络, 尽量大一些
        perror("listen");
        return -1;
    }

    LOG() << "listening...";

    int epoll_fd = epoll_create(1);                                             // epoll 句柄

    epoll_event evt;
    evt.data.fd = listen_fd;                                                    // 自定义数据结构, 会随着 epoll_wait 返回事件一起分会
    evt.events = EPOLLIN;                                                       // epoll 监控读事件, 水平触发
                                                                                // 因为连接不是非常频繁, 所以水平触发也行

    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &evt);                        // epoll 监控 listen_fd 和它对应的事件

    epoll_event events[10];                                                     // 存放 epoll_wait 返回事件的数组

    while (true) {
        int count = epoll_wait(epoll_fd, events, 10, -1);

        if (count < 0) {
            perror("epoll_wait() failed.");                                     // 失败
            break;
        }

        if (count == 0) {
            perror("epoll_wait() timeout.");                                    // 超时
            break;
        }

        for (int i = 0; i < count; ++i) {                                       // 有事件发生, count 表示发生事件的 fd 的数量
            // 对方已关闭, 有些系统检测不到, 可以用 EPOLLIN, recv() == 0
            if (events[i].events & EPOLLHUP) {
                LOG() << "client[fd: " << events[i].data.fd << "]: discconnected, EPOLLHUB";
                close(events[i].data.fd);                                       // 关闭客户端连接的 fd
            }
            else if (events[i].events & (EPOLLIN | EPOLLPRI)) {
                // listen_fd 有事件, 表示有新的客户端连上来
                if (events[i].data.fd == listen_fd) {
                    sockaddr_in clnt_addr;
                    socklen_t len = sizeof(clnt_addr);
                    int clnt_fd = accept4(listen_fd, (struct sockaddr*)&clnt_addr, &len, SOCK_NONBLOCK);

                    LOG() << "accept client fd: " << clnt_fd
                    << ", ip: " << inet_ntoa(clnt_addr.sin_addr)
                    << ", port: " << ntohs(clnt_addr.sin_port);

                    evt.data.fd = clnt_fd;
                    evt.events = EPOLLIN | EPOLLET;                                 // 边沿触发
                    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, clnt_fd, &evt);              // epoll 监控 clnt_fd
                }
                // 客户端连接的 fd 有事件
                else {
                    // 接收缓冲区有数据可以读
                    char buffer[1024];
                    while (true) {                                              // 非阻塞IO + 边沿触发, 把数据全部读出来
                        memset(buffer, 0, sizeof(buffer));
                        int bytes_read = recv(events[i].data.fd, buffer, sizeof(buffer), 0);

                        // 成功读到了数据
                        if (bytes_read > 0) {
                            LOG() << "recv[fd: " << events[i].data.fd << "]: (" << bytes_read << ") " << buffer;
                            send(events[i].data.fd, buffer, strlen(buffer), 0); // 把接收到的数据回复回去
                        }
                        // 读数据的时候, 被信号中断, 继续读数据
                        else if (bytes_read == -1 && errno == EINTR) {
                            continue;
                        }
                        // 接收缓冲区中数据都读完
                        else if (bytes_read == -1 && ((errno == EAGAIN || (errno == EWOULDBLOCK)))) {
                            break;
                        }
                        // 客户端断开连接
                        else if (bytes_read == 0) {
                            LOG() << "client[fd: " << events[i].data.fd << "]: disconnected, recv() == 0";
                            close(events[i].data.fd);                           // 关闭客户端连接的 fd
                            break;
                        }
                    }
                }
            }
            // 发送缓冲区有空间
            else if (events[i].events & EPOLLOUT) {

            }
            else {
                // 错误
                LOG() << "client[fd: " << events[i].data.fd << "], disconnected, error";
                close(events[i].data.fd);
            }
        }
    }

    return 0;
}