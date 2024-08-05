#pragma once

#include "utils/uncopyable.h"
#include "net/inet_address.h"
#include "net/buffer.h"
#include "event/channel.h"

#include <functional>
#include <memory>
#include <atomic>
#include <string>

namespace event {

class EventLoop;
    
} // namespace event


namespace net {

class Buffer;
class TcpConnection;

using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
using ConnectionCallback = std::function<void(const TcpConnectionPtr&)>;
using MessageCallback = std::function<void(const TcpConnectionPtr&, Buffer*)>;
using CloseCallback = std::function<void(const TcpConnectionPtr&)>;
using WriteCompleteCallback = std::function<void(const TcpConnectionPtr&)>;
using HighWaterMarkCallback = std::function<void(const TcpConnectionPtr&, size_t)>;

class Socket;

class TcpConnection
        : utils::Uncopyable,
          public std::enable_shared_from_this<TcpConnection> {
public:
    TcpConnection(event::EventLoop *loop,
                  int sockfd,
                  const InetAddress &local_addr,
                  const InetAddress &peer_addr);
    ~TcpConnection();

    event::EventLoop* GetLoop() const { return loop_; }
    const int fd() const { return channel_->fd(); }
    const InetAddress& local_addr() const { return local_addr_; }
    const InetAddress& peer_addr() const { return peer_addr_; }

    bool Connected() const { return state_ == kConnected; }
    bool Disconnected() const { return state_ == kDisconnected; }

    void Send(const std::string &message);
    void Send(Buffer *buffer);
    void Shutdown();

    void SetConnectionCallback(const ConnectionCallback &cb) { connection_callback_ = cb; }
    void SetMessageCallback(const MessageCallback &cb) { message_callback_ = cb; }
    void SetCloseCallback(const CloseCallback &cb) { close_callback_ = cb; }
    void SetWriteCompleteCallback(const WriteCompleteCallback &cb) { write_complete_callback_ = cb; }
    void SetHighWaterMarkCallback(const HighWaterMarkCallback &cb) { high_water_mark_callback_ = cb; }

    Buffer* input_buffer() { return &input_buffer_; }
    Buffer* output_buffer() { return &output_buffer_; }

    void ConnectionEstablished();
    void ConnectionDestroyed();

private:
    enum State {
        kConnecting,
        kConnected,
        kDisconnecting,
        kDisconnected,
    };

    void HandleRead();
    void HandleWrite();
    void HandleClose();
    void HandleError();

    void SendInLoop(const void *data, size_t len);
    void ShutdownInLoop();

    void SetState(State state) { state_ = state; }

    event::EventLoop *loop_;
    std::atomic_int state_;

    std::unique_ptr<Socket> socket_;
    std::unique_ptr<event::Channel> channel_;

    const InetAddress local_addr_;
    const InetAddress peer_addr_;

    Buffer input_buffer_;
    Buffer output_buffer_;

    ConnectionCallback connection_callback_;
    MessageCallback message_callback_;
    CloseCallback close_callback_;
    WriteCompleteCallback write_complete_callback_;
    HighWaterMarkCallback high_water_mark_callback_;

    size_t high_water_mark_;
};
    
} // namespace connection

