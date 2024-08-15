#pragma once

#include "utils/uncopyable.h"
#include "net/tcp_connection.h"
#include "net/socket.h"
#include "net/inet_address.h"
#include "event/channel.h"
#include "event/event_loop.h"
#include "event/event_loop_thread_pool.h"

#include <unordered_map>
#include <memory>
#include <atomic>

namespace net {

class TcpServer : utils::Uncopyable {
public:
    using ThreadInitCallback = std::function<void(event::EventLoop*)>;

    enum Option {
        kNoReusePort,
        kReusePort,
    };

    TcpServer(event::EventLoop *loop,
              const InetAddress &addr,
              const std::string &name,
              Option option = kNoReusePort);
    ~TcpServer();

    void SetThreadInitCallback(const ThreadInitCallback &cb) { thread_init_callback_ = cb; }
    void SetConnectionCallback(const ConnectionCallback &cb) { connection_callback_ = cb; }
    void SetMessageCallback(const MessageCallback &cb) { message_callback_ = cb; }
    void SetWriteCompleteCallback(const WriteCompleteCallback &cb) { write_complete_callback_ = cb; }

    void SetThreadNum(int num_threads);
    void Start();

    std::string name() const { return name_; }
    std::string ip_port() const { return addr_->GetIpPort(); }

private:
    using ConnectionMap = std::unordered_map<int, std::shared_ptr<TcpConnection>>;

    void HandleNewConnection();
    void RemoveConnection(const TcpConnectionPtr &conn);
    void RemoveConnectionInLoop(const TcpConnectionPtr &conn);

    event::EventLoop *loop_;

    const std::string name_;
    const std::shared_ptr<InetAddress> addr_;

    std::shared_ptr<Socket> accept_socket_;
    std::shared_ptr<event::Channel> accept_channel_;
    std::shared_ptr<event::EventLoopThreadPool> thread_pool_;

    ConnectionCallback connection_callback_;
    MessageCallback message_callback_;
    WriteCompleteCallback write_complete_callback_;
    ThreadInitCallback thread_init_callback_;

    std::atomic_int started_;

    int next_conn_id_;
    ConnectionMap connection_map_;
};

} // namespace net
