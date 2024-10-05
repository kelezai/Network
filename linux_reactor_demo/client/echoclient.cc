#include "logging.h"

#include <string.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/tcp.h>    // TCP_NODELAY
#include <fcntl.h>

#define LOG()               LOG_PRINT("clnt")

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

    int MAX_TIMES = 10;
    char buffer[1024] = { 0 };
    for (int i = 0; i < MAX_TIMES; ++i) {
        //
        // 发送
        memset(buffer, 0, sizeof(buffer));
    #if 0
        // 从命令行输入内容
        LOG() << "please input: ";
        scanf("%s", buffer);
    #else
        sprintf(buffer, "这是第 %d 条消息", i);
    #endif
        char temp[1024] = { 0 };
        int len_send = strlen(buffer);
        memcpy(temp, &len_send, 4);                                      // 报文头
        memcpy(temp + 4, buffer, len_send);                              // 报文内容

        if (send(sock_fd, temp, len_send + 4, 0) <= 0) {
            perror("send");
            return -1;
        }

        //
        // 接收
        memset(buffer, 0, sizeof(buffer));
        int len_recv;
        recv(sock_fd, &len_recv, 4, 0);                                  // 先读 4 bytes 报文头
        if (recv(sock_fd, buffer, len_recv, 0) <= 0) {                   // 读取报文内容
            perror("recv");
            return -1;
        }

        LOG() << "recv[fd: " << sock_fd << "]: (" << strlen(buffer) << ") " << buffer;
    }

    close(sock_fd);
    return 0;
}