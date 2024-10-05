#include "tcp_server.h"
#include "logging.h"

#define     LOG()                   LOG_PRINT("serv")

TcpServer::TcpServer(const std::string& ip, uint16_t port, int thread_num) 
    : main_loop_(std::make_unique<EventLoop>(5))
    , acceptor_(main_loop_.get(), ip, port) {

    main_loop_->set_idle_callback(std::bind(&TcpServer::handle_idle, this, std::placeholders::_1));
    acceptor_.set_connect_callback(std::bind(&TcpServer::handle_connect, this, std::placeholders::_1));

    sub_loops_thread_pool_ = new ThreadPool(thread_num, "IO");
    sub_loops_.reserve(thread_num);
    for (int i = 0; i < thread_num; ++i) {
        auto loop = std::make_unique<EventLoop>(5);
    
        loop->set_idle_callback(std::bind(&TcpServer::handle_idle, this, std::placeholders::_1));
        loop->set_timer_callback(std::bind(&TcpServer::handle_timer, this, std::placeholders::_1));

        sub_loops_conns_[loop.get()] = new ConnSet();
        sub_loops_thread_pool_->add_task(std::bind(&EventLoop::run, loop.get()));

        sub_loops_.emplace_back(std::move(loop));
    }
}

TcpServer::~TcpServer() {
    delete sub_loops_thread_pool_;

    for (auto & kv : sub_loops_conns_) {
        delete kv.second;
    }
}

void TcpServer::start() {
    main_loop_->run();
}

void TcpServer::stop() {
    // 停止主事件循环
    main_loop_->stop();
    LOG() << "main loop stopped";

    // 停止从事件循环
    for (size_t i = 0; i < sub_loops_.size(); ++i) {
        sub_loops_[i]->stop();
    }
    LOG() << "sub loop stopped";

    // 停止 IO 线程
    sub_loops_thread_pool_->stop();
    LOG() << "IO_thread_pool stopped";
}

void TcpServer::handle_connect(std::unique_ptr<Socket> client_sock) {
    int rand_num = client_sock->fd() % sub_loops_thread_pool_->size();

    auto sub_loop = sub_loops_[rand_num].get();

    SpConnection conn = std::make_shared<Connection>(sub_loop, std::move(client_sock));
    conn->set_close_callback(std::bind(&TcpServer::handle_close, this, std::placeholders::_1));
    conn->set_error_callback(std::bind(&TcpServer::handle_error, this, std::placeholders::_1));
    conn->set_message_callback(std::bind(&TcpServer::handle_message, this, std::placeholders::_1, std::placeholders::_2));
    conn->set_send_complete_callback(std::bind(&TcpServer::send_complete, this, std::placeholders::_1));

    {
        auto * connset = sub_loops_conns_[sub_loop];
        std::unique_lock<std::mutex> lock(connset->mtx);
        connset->conns.push_back(conn);
    }
    {
        std::unique_lock<std::mutex> lock(conns_mtx_);
        conns_[conn->fd()] = conn;
    }

    conn->start();

    connect_callback_(conn);
}

void TcpServer::handle_close(SpConnection conn) {
    remove_connect(conn);
}

void TcpServer::handle_error(SpConnection conn) {
    remove_connect(conn);
}

void TcpServer::handle_message(SpConnection conn, const std::string& message) {
    if (message_callback_) {
        message_callback_(conn, message);
    }
}

void TcpServer::send_complete(SpConnection conn) {
    if (send_complete_callback_) {
        send_complete_callback_(conn);
    }
}

// `epoll_wait` 超时, EventLoop 中回调此函数
void TcpServer::handle_idle(EventLoop * loop) {
    if (idle_callback_) {
        idle_callback_(loop);
    }
}

void TcpServer::remove_connect(SpConnection conn) {
    {
        std::lock_guard<std::mutex> lock(conns_mtx_);
        conns_.erase(conn->fd());
    }

    if (disconnect_callback_) {
        disconnect_callback_(conn);
    }
}

void TcpServer::handle_timer(EventLoop * sub_loop) {
    std::vector<SpConnection> conns_to_remove;

    auto * connset = sub_loops_conns_[sub_loop];
    std::unique_lock<std::mutex> lock(connset->mtx);
    auto & conns = connset->conns;
    for (auto it = conns.begin(); it != conns.end();) {
        auto conn = (*it).lock();
        if (!conn) {
            it = conns.erase(it);
            continue;
        }

        if (conn->is_timeout(5)) {
            conns_to_remove.push_back(conn);
            it = conns.erase(it);
            continue;
        }
        
        it++;
    } 

    for (auto & conn : conns_to_remove) {         
        LOG() << "client timeout(fd: " << conn->fd() << ", ip: " << conn->ip() << ", port: " << conn->port(); 
        remove_connect(conn);
    }
}

void TcpServer::set_connect_callback(std::function<void(SpConnection)> fn) {
    connect_callback_ = fn;
}

void TcpServer::set_disconnect_callback(std::function<void(SpConnection)> fn) {
    disconnect_callback_ = fn;
}

void TcpServer::set_message_callback(std::function<void(SpConnection, const std::string&)> fn) {
    message_callback_ = fn;
}

void TcpServer::set_send_complete_callback(std::function<void(SpConnection)> fn) {
    send_complete_callback_ = fn;
}

void TcpServer::set_idle_callback(std::function<void(EventLoop*)> fn) {
    idle_callback_ = fn;
}
