#include "logging.h"

#include <string.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/tcp.h>    // TCP_NODELAY
#include <fcntl.h>

#define LOG()               LOG_PRINT("clnt")

// 发送报文, 支持 4 字节报头
ssize_t tcp_send(int fd, void * data, size_t size) {
    char buff[1024] = { 0 };
    memcpy(buff, &size, 4);             // 拼接报文头部
    memcpy(buff + 4, data, size);       // 拼接报文内容
    return send(fd, buff, size + 4, 0);
}

// 接收报文, 支持 4 字节报头
ssize_t tcp_recv(int fd, void * data)
{
    int len = 0;
    recv(fd, &len, 4, 0);               // 读取报文头部
    return recv(fd, data, len, 0);      // 读取报文内容
}

int main(int argc, const char * argv[]) {
    if (argc != 3) {
        LOG() << "usage: ./x-client 192.168.1.5 5005";
        return -1;
    }

    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        perror("socket");
        return -1;
    }

    sockaddr_in sock_addr;
    memset(&sock_addr, 0, sizeof(sock_addr));
    sock_addr.sin_family = AF_INET;
    sock_addr.sin_addr.s_addr = inet_addr(argv[1]);
    sock_addr.sin_port = htons(atoi(argv[2]));

    if (connect(sock_fd, (struct sockaddr*)&sock_addr, sizeof(sock_addr)) != 0) {
        LOG() << "connect failed, ip: " << argv[1] << ", port: " << argv[2];
        close(sock_fd);
        return -1;
    }

    LOG() << "connected: ip: " << argv[1] << ", port: " << argv[2];

    char buf[1024] = { 0 };

    //////////////////////////////////////////////
    // 登录
    memset(buf, 0, sizeof(buf));
    sprintf(buf, "<bizcode>00101</bizcode><username>xiaoshuai</username><password>12345</password>");
    if (tcp_send(sock_fd, buf, strlen(buf)) <= 0) {
        LOG() << "tcp_send failed, errno: " << errno;
        return -1;
    }

    memset(buf, 0, sizeof(buf));
    if (tcp_recv(sock_fd, buf) <= 0) {
        LOG() << "tcp_recv failed, errno: " << errno;
        return -1;
    }

    LOG() << "recv: " << buf;

    //////////////////////////////////////////////
    // 查询余额
    memset(buf, 0, sizeof(buf));
    sprintf(buf, "<bizcode>00201</bizcode><cardid>1234567890</cardid>");
    if (tcp_send(sock_fd, buf, strlen(buf)) <= 0) {
        LOG() << "tcp_send failed, errno: " << errno;
        return -1;
    }

    memset(buf, 0, sizeof(buf));
    if (tcp_recv(sock_fd, buf) <= 0) {
        LOG() << "tcp_recv failed, errno: " << errno;
        return -1;
    }

    LOG() << "recv: " << buf;

    //////////////////////////////////////////////
    // 心跳
    while (true) {
        memset(buf, 0, sizeof(buf));
        sprintf(buf, "<bizcode>00001</bizcode>");
        if (tcp_send(sock_fd, buf, strlen(buf)) <= 0) {
            LOG() << "tcp_send failed, errno: " << errno;
            return -1;
        }

        memset(buf, 0, sizeof(buf));
        if (tcp_recv(sock_fd, buf) <= 0) {
            LOG() << "tcp_recv failed, errno: " << errno;
            return -1;
        }

        LOG() << "recv: " << buf;
        sleep(3);
    }

    //////////////////////////////////////////////
    // 注销
    memset(buf, 0, sizeof(buf));
    sprintf(buf, "<bizcode>00901</bizcode>");
    if (tcp_send(sock_fd, buf, strlen(buf)) <= 0) {
        LOG() << "tcp_send failed, errno: " << errno;
        return -1;
    }

    memset(buf, 0, sizeof(buf));
    if (tcp_recv(sock_fd, buf) <= 0) {
        LOG() << "tcp_recv failed, errno: " << errno;
        return -1;
    }

    LOG() << "recv: " << buf;

    close(sock_fd);
    return 0;
}