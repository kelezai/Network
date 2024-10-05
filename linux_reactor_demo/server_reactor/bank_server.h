#pragma once

#include "tcp_server.h"
#include "event_loop.h"
#include "connection.h"
#include "thread_pool.h"


/**
 * BankServer 网上银行服务器 
 */

// 用户信息(状态机): 记录客户端状态.
class UserInfo {
public:
    UserInfo(int fd, const std::string & ip) : fd_(fd), ip_(ip) {}

    void SetLogin(bool login) {
        login_ = login;
    }

    bool IsLogin() {
        return login_;
    }

private:
    int fd_;                            // 客户端 fd
    std::string ip_;                    // 客户端 ip 地址
    std::string user_name_;             // 客户端用户名
    bool login_ = false;                // 客户端的登录状态
};

class BankServer {
    using SpUserInfo = std::shared_ptr<UserInfo>;

public:
    BankServer(const std::string& ip, uint16_t port, int work_thread_num);
    ~BankServer();

    void Start();                                                           // 启动服务
    void Stop();                                                            // 停止服务

    void handle_connect(SpConnection conn);                                 // 处理新客户端连接请求, Acceptor 类中回调该函数
    void handle_disconnect(SpConnection conn);
    // void handle_close(SpConnection conn);                                // 处理客户端断开连接, 在 Connecttion 类中回调此函数
    // void handle_error(SpConnection conn);                                // 处理客户端出错连接, 在 Connecttion 类中回调此函数
    void handle_message(SpConnection conn, const std::string& message);
    void handle_send_complete(SpConnection conn);                           // 数据发送完成回调
    void handle_idle(EventLoop * loop);                                     // epoll_wait 超时

protected:
    void process_message(SpConnection conn, const std::string& message);

private:
    TcpServer   tcp_server_;
    ThreadPool  work_thread_pool_;

private:
    std::map<int, SpUserInfo> userinfo_dict_;                               // 用户状态机
    std::mutex userinfo_dict_mtx_;
};