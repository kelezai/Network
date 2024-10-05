#include "bank_server.h"
#include "logging.h"

#define  LOG()          LOG_PRINT("serv")

BankServer::BankServer(const std::string& ip, uint16_t port, int work_thread_num) 
    : tcp_server_(ip, port, 4)
    , work_thread_pool_(work_thread_num, "Work") {

    tcp_server_.set_connect_callback(std::bind(&BankServer::handle_connect, this, std::placeholders::_1));
    tcp_server_.set_disconnect_callback(std::bind(&BankServer::handle_disconnect, this, std::placeholders::_1));
    tcp_server_.set_message_callback(std::bind(&BankServer::handle_message, this, std::placeholders::_1, std::placeholders::_2));
    tcp_server_.set_send_complete_callback(std::bind(&BankServer::handle_send_complete, this, std::placeholders::_1));
    tcp_server_.set_idle_callback(std::bind(&BankServer::handle_idle, this, std::placeholders::_1));
}

BankServer::~BankServer() {

}

void BankServer::Start() {
    tcp_server_.start();
}

void BankServer::Stop() {
    // 停止工作线程
    work_thread_pool_.stop();
    LOG() << "work_thread_pool stopped";

    // 停止IO线程
    tcp_server_.stop();
}

void BankServer::handle_connect(SpConnection conn) {
    LOG() << "client connect(fd: " << conn->fd()
    << ", ip: " << conn->ip()
    << ", port: " << conn->port() << ")"
    << ", time: " << Timestamp::now().to_string();

    // 新客户端连接, 保存客户端信息
    SpUserInfo userinfo = std::make_shared<UserInfo>(conn->fd(), conn->ip());
    {
        std::lock_guard<std::mutex> lock(userinfo_dict_mtx_);
        userinfo_dict_[conn->fd()] = userinfo;
    }
}

void BankServer::handle_disconnect(SpConnection conn) {
    LOG() << "client disconnect(fd: " << conn->fd()
    << ", ip: " << conn->ip()
    << ", port: " << conn->port() << ")"
    << ", time: " << Timestamp::now().to_string();

    // 关闭客户端连接, 删除客户端信息
    std::lock_guard<std::mutex> lock(userinfo_dict_mtx_);
    userinfo_dict_.erase(conn->fd());
}

void BankServer::handle_message(SpConnection conn, const std::string& message) {
    LOG() << "recv[fd: " << conn->fd() << "]: (" << message.size() << ") " << message.c_str()
    << ", time: " << Timestamp::now().to_string();

    if (work_thread_pool_.size() == 0) {
        process_message(conn, message);
    }
    else {
        work_thread_pool_.add_task(std::bind(&BankServer::process_message, this, conn, message));
    }
}

bool get_xml_buffer(const std::string &xml_buffer, const std::string &filed_name, std::string &value, const int len = 0) {
    std::string start = "<" + filed_name + ">";         // 开始标签
    std::string end = "</" + filed_name + ">";          // 结束标签

    auto start_pos = xml_buffer.find(start);
    if (start_pos == std::string::npos) {
        return false;
    }

    auto end_pos = xml_buffer.find(end);
    if (end_pos == std::string::npos) {
        return false;
    }

    int item_len = end_pos - start_pos - start.length();
    value = xml_buffer.substr(start_pos + start.length(), item_len);
    return true;
}

// 处理客户端报文请求
void BankServer::process_message(SpConnection conn, const std::string& message) {
    // 获得客户端信息
    std::lock_guard<std::mutex> lock(userinfo_dict_mtx_);
    SpUserInfo userinfo = userinfo_dict_[conn->fd()];

    // 解析客户端请求报文, 处理业务
    // <bizcode>00101</bizcode><username>xiaoshuai</username><password>12345</password>
    std::string bizcode;
    std::string reply_message;
    get_xml_buffer(message, "bizcode", bizcode);

    if (bizcode == "00101") {                       // 登录业务
        std::string username;
        std::string password;
        get_xml_buffer(message, "username", username);
        get_xml_buffer(message, "password", password);

        if ((username == "xiaoshuai") && (password == "12345")) {
            reply_message = "<bizcode>00102</bizcode><retcode>0</retcode><message>ok</message>";
            userinfo->SetLogin(true);               // 设置用户登录状态
        }
        else {
            // 用户名和密码不正确
            reply_message = "<bizcode>00102</bizcode><retcode>-1</retcode><message>用户名或密码错误</message>";
        }
    }
    else if (bizcode == "00201") {                  // 查询余额业务
        if (userinfo->IsLogin()) {
            // 查询用户的余额
            reply_message = "<bizcode>00202</bizcode><retcode>0</retcode><message>1234.56</message>";
        }
        else {
            reply_message = "<bizcode>00202</bizcode><retcode>-1</retcode><message>用户未登录</message>";
        }
    }
    else if (bizcode == "00901") {                  // 注销业务
        if (userinfo->IsLogin()) {
            reply_message = "<bizcode>00902</bizcode><retcode>0</retcode><message>ok</message>";
            userinfo->SetLogin(false);
        }
        else {
            reply_message = "<bizcode>00902</bizcode><retcode>-1</retcode><message>用户未登录</message>";
        }
    }
    else if (bizcode == "00001") {                  // 心跳
        if (userinfo->IsLogin()) {
            reply_message = "<bizcode>00002</bizcode><retcode>0</retcode><message>ok</message>";
        }
        else {
            reply_message = "<bizcode>00002</bizcode><retcode>-1</retcode><message>用户未登录</message>";
        }
    }

    conn->send(reply_message.data(), reply_message.size());
}

void BankServer::handle_send_complete(SpConnection conn) {
    // LOG() << __func__;
}

void BankServer::handle_idle(EventLoop * loop) {
    // LOG() << ".";
}