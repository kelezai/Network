#include "echo_server.h"
#include "logging.h"

#define  LOG()          LOG_PRINT("serv")

EchoServer::EchoServer(const std::string& ip, uint16_t port, int work_thread_num) 
    : tcp_server_(ip, port, 4)
    , work_thread_num_(work_thread_num)
    , work_thread_pool_(work_thread_num, "Work") {

    tcp_server_.set_connect_callback(std::bind(&EchoServer::handle_connect, this, std::placeholders::_1));
    tcp_server_.set_disconnect_callback(std::bind(&EchoServer::handle_disconnect, this, std::placeholders::_1));
    tcp_server_.set_message_callback(std::bind(&EchoServer::handle_message, this, std::placeholders::_1, std::placeholders::_2));
    tcp_server_.set_send_complete_callback(std::bind(&EchoServer::handle_send_complete, this, std::placeholders::_1));
    tcp_server_.set_idle_callback(std::bind(&EchoServer::handle_idle, this, std::placeholders::_1));
}

EchoServer::~EchoServer() {

}

void EchoServer::Start() {
    tcp_server_.start();
}

void EchoServer::Stop() {
    // 停止工作线程
    work_thread_pool_.stop();
    LOG() << "work_thread_pool stopped";

    // 停止IO线程
    tcp_server_.stop();
}

void EchoServer::handle_connect(SpConnection conn) {
    LOG() << "client connect(fd: " << conn->fd()
    << ", ip: " << conn->ip()
    << ", port: " << conn->port() << ")"
    << ", time: " << Timestamp::now().to_string();
}

void EchoServer::handle_disconnect(SpConnection conn) {
    LOG() << "client disconnect(fd: " << conn->fd()
    << ", ip: " << conn->ip()
    << ", port: " << conn->port() << ")"
    << ", time: " << Timestamp::now().to_string();
}

void EchoServer::handle_message(SpConnection conn, const std::string& message) {
    LOG() << "recv[fd: " << conn->fd() << "]: (" << message.size() << ") " << message.c_str()
    << ", time: " << Timestamp::now().to_string();

    if (work_thread_pool_.size() == 0) {
        process_message(conn, message);
    }
    else {
        work_thread_pool_.add_task(std::bind(&EchoServer::process_message, this, conn, message));
    }
}

// 处理客户端报文请求
void EchoServer::process_message(SpConnection conn, const std::string& message) {
    // 处理业务逻辑, 耗时操作
    std::string reply = "reply: " + message;
    conn->send(reply.data(), reply.size());
}

void EchoServer::handle_send_complete(SpConnection conn) {
    // LOG() << __func__;
}

void EchoServer::handle_idle(EventLoop * loop) {
    // LOG() << ".";
}