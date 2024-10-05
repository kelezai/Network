#include "connection.h"
#include "logging.h"

#define  LOG()                   LOG_PRINT("serv")

Connection::Connection(EventLoop * loop, std::unique_ptr<Socket> client_sock) 
    : loop_(loop)
    , client_sock_(std::move(client_sock))
    , input_buffer_(1)
    , output_buffer_(1)
    , disconnect_(false) {

    // 为新客户端连接准备读事件, 并添加到 epoll 中.
    client_channel_ = std::make_unique<Channel>(client_sock_->fd(), true);
    client_channel_->set_read_callback(std::bind(&Connection::handle_message, this));
    client_channel_->set_close_callback(std::bind(&Connection::handle_close, this));
    client_channel_->set_error_callback(std::bind(&Connection::handle_error, this));
    client_channel_->set_write_callback(std::bind(&Connection::handle_write, this));
    client_channel_->enable_read();
}

Connection::~Connection() {
    int fd = this->fd();
    LOG() << "fd: " << fd << ", enter";
    disconnect_ = true;
    loop_->remove_channel(client_channel_.get());

    LOG() << "fd: " << fd << ", leave";
}

void Connection::start() {
    loop_->update_channel(client_channel_.get());
}

int Connection::fd() const {
    return client_sock_->fd();
}

std::string Connection::ip() const {
    return client_sock_->ip();
}

uint16_t Connection::port() const {
    return client_sock_->port();
}

void Connection::handle_close() {
    disconnect_ = true;
    loop_->remove_channel(client_channel_.get());
    close_callback_(shared_from_this());
}

void Connection::handle_error() {
    disconnect_ = true;
    loop_->remove_channel(client_channel_.get());
    error_callback_(shared_from_this());
}

void Connection::handle_message() {
     // 接收缓冲区有数据可以读
    char buffer[1024];
    while (true) {                                                          // 非阻塞IO + 边沿触发, 把数据全部读出来
        memset(buffer, 0, sizeof(buffer));
        int read_bytes = recv(fd(), buffer, sizeof(buffer), 0);

        // 客户端断开连接
        if (read_bytes == 0) {
            handle_close();
            break;
        }

        // 成功读到了数据
        if (read_bytes > 0) {
            input_buffer_.append(buffer, read_bytes);                       // 把读到的数据追加到 input_buffer_
        }
        // 读数据的时候, 被信号中断, 继续读数据
        else if (read_bytes == -1 && errno == EINTR) {
            continue;
        }
        // 接收缓冲区中数据都读完
        else if (read_bytes == -1 && ((errno == EAGAIN || (errno == EWOULDBLOCK)))) {
            while (true) {
                std::string message;
                if (!input_buffer_.get_message(message)) {
                    break;
                }

                last_time_ = Timestamp::now();
                message_callback_(shared_from_this(), message);
            }
            break;
        }
    }
}

void Connection::handle_write() {
    // 把 output_buffer_ 中的数据全部发送出去
    int write_bytes = ::send(fd(), output_buffer_.data(), output_buffer_.size(), 0);
    if (write_bytes > 0) {
        // 从 output_buffer_ 中删除已经发送出去的数据
        output_buffer_.erase(0, write_bytes);
    }

    // 如果 output_buffer_ 为空, 取消关注写事件
    if (output_buffer_.size() == 0) {
        client_channel_->disable_write();
        loop_->update_channel(client_channel_.get());
        send_complete_callback_(shared_from_this());
    }
}

void Connection::send(const char * data, size_t size) {
    if (disconnect_) {
        LOG() << "fd: " << fd() << ", send fail cuz disconnect";
        return;
    }

    if (loop_->current_thread_is_loop_thread()) {                   // 判断当前线程是否为事件循环线程 (IO 线程)
        // 如果当前线程是 IO 线程, 直接发送数据
        send_in_loop(data, size);
    }
    else {
        // 如果当前线程不是 IO 线程, 把发送数据的操作交给 IO 线程去执行
        loop_->add_to_task_queue([this, data = std::string(data), size](){
            send_in_loop(data.c_str(), size);
        });
    }
}

void Connection::send_in_loop(const char * data, size_t size) {
    output_buffer_.append_with_sep(data, size);
    client_channel_->enable_write();
    loop_->update_channel(client_channel_.get());
}

void Connection::set_close_callback(std::function<void(SpConnection)> fn) {
    close_callback_ = fn;
}

void Connection::set_error_callback(std::function<void(SpConnection)> fn) {
    error_callback_ = fn;
}

void Connection::set_message_callback(std::function<void(SpConnection, const std::string&)> fn) {
    message_callback_ = fn;
}

void Connection::set_send_complete_callback(std::function<void(SpConnection)> fn) {
    send_complete_callback_ = fn;
}

bool Connection::is_timeout(int thresh) {
    return (Timestamp::now().to_int() - last_time_.to_int()) > thresh;
}