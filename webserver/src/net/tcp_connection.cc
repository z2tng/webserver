#include "net/tcp_connection.h"
#include "net/socket.h"
#include "event/channel.h"
#include "event/event_loop.h"
#include "log/logger.h"

namespace net {

TcpConnection::TcpConnection(event::EventLoop *loop,
                             int sockfd,
                             const InetAddress &local_addr,
                             const InetAddress &peer_addr)
        : loop_(loop),
          state_(kConnecting),
          socket_(new Socket(sockfd)),
          channel_(new event::Channel(loop_, sockfd)),
          local_addr_(local_addr),
          peer_addr_(peer_addr) {
    channel_->SetReadCallback(std::bind(&TcpConnection::HandleRead, this));
    channel_->SetWriteCallback(std::bind(&TcpConnection::HandleWrite, this));
    channel_->SetCloseCallback(std::bind(&TcpConnection::HandleClose, this));
    channel_->SetErrorCallback(std::bind(&TcpConnection::HandleError, this));
    socket_->SetKeepAlive(true);

    LOG_INFO << "TcpConnection::ctor[" << local_addr_.GetIpPort() << "] at " << channel_->fd();
}

TcpConnection::~TcpConnection() {}

void TcpConnection::HandleRead() {
    int saved_errno = 0;
    ssize_t n = input_buffer_.ReadFd(channel_->fd(), &saved_errno);
    if (n > 0) {
        message_callback_(shared_from_this(), &input_buffer_);
    } else if (n == 0) {
        HandleClose();
    } else {
        errno = saved_errno;
        LOG_ERROR << "TcpConnection::HandleRead";
        HandleError();
    }
}

void TcpConnection::HandleWrite() {
    int saved_errno = 0;
    if (channel_->IsWriting()) {
        ssize_t n = output_buffer_.WriteFd(channel_->fd(), &saved_errno);
        if (n > 0) {
            output_buffer_.Retrieve(n);
            if (output_buffer_.ReadableBytes() == 0) {
                channel_->DisableWriting();
                if (write_complete_callback_) {
                    loop_->RunInLoop(std::bind(write_complete_callback_, shared_from_this()));
                }
                if (state_ == kDisconnecting) {
                    ShutdownInLoop();
                }
            }
        } else {
            LOG_ERROR << "TcpConnection::HandleWrite";
        }
    } else {
        LOG_ERROR << "Connection is down, no more writing";
    }
}

void TcpConnection::HandleClose() {
    LOG_INFO << "TcpConnection::HandleClose state = " << state_;
    SetState(kDisconnected);
    channel_->DisableAll();
    TcpConnectionPtr guard_this(shared_from_this());
    if (connection_callback_) connection_callback_(guard_this);
    if (close_callback_) close_callback_(guard_this);
}

void TcpConnection::HandleError() {
    int optval;
    socklen_t optlen = sizeof(optval);
    int err = 0;
    if (::getsockopt(channel_->fd(), SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0) {
        err = errno;
    } else {
        err = optval;
    }
    LOG_ERROR << "TcpConnection::HandleError [" << local_addr_.GetIpPort()
              << "] - SO_ERROR = " << err;
}

void TcpConnection::Send(const std::string &message) {
    if (state_ == kConnected) {
        if (loop_->is_in_loop_thread()) {
            SendInLoop(message.c_str(), message.size());
        } else {
            loop_->QueueInLoop(std::bind(&TcpConnection::SendInLoop, this, message.c_str(), message.size()));
        }
    }
}

void TcpConnection::Send(Buffer *buffer) {
    if (state_ == kConnected) {
        if (loop_->is_in_loop_thread()) {
            SendInLoop(buffer->Peek(), buffer->ReadableBytes());
            buffer->RetrieveAll();
        } else {
            loop_->QueueInLoop([this, &buffer]() {
                SendInLoop(buffer->Peek(), buffer->ReadableBytes());
                buffer->RetrieveAll();
            });
        }
    }
}

void TcpConnection::Shutdown() {
    if (state_== kConnected) {
        SetState(kDisconnecting);
        loop_->QueueInLoop(std::bind(&TcpConnection::ShutdownInLoop, this));
    }
}

void TcpConnection::SendInLoop(const void *data, size_t len) {
    ssize_t n = 0;
    size_t remaining = len;
    bool fault_error = false;

    if (state_ == kDisconnected) {
        LOG_ERROR << "disconnected, give up writing";
        return;
    }

    // 如果 output_buffer_ 没有数据, 尝试直接写
    if (!channel_->IsWriting() && output_buffer_.ReadableBytes() == 0) {
        n = ::write(channel_->fd(), data, len);
        if (n >= 0) {
            remaining = len - n;
            if (remaining == 0 && write_complete_callback_) {
                loop_->QueueInLoop(std::bind(write_complete_callback_, shared_from_this()));
            }
        } else {
            n = 0;
            LOG_ERROR << "TcpConnection::SendInLoop";
            if (errno == EPIPE || errno == ECONNRESET) {
                fault_error = true;
            }
        }
    }

    // 如果没有写完, 或者出现错误
    if (remaining > 0 && !fault_error) {
        size_t old_len = output_buffer_.ReadableBytes();
        if (old_len + remaining > high_water_mark_
                && old_len < high_water_mark_
                && high_water_mark_callback_) {
            loop_->QueueInLoop(std::bind(high_water_mark_callback_, shared_from_this(), old_len + remaining));
        }
        // 将剩余数据写入 output_buffer_
        output_buffer_.Append(static_cast<const char*>(data) + n, remaining);
        // 如果没有注册写事件, 则注册写事件
        if (!channel_->IsWriting()) {
            channel_->EnableWriting();
        }
    }
}

void TcpConnection::ShutdownInLoop() {
    // 如果没有注册写事件, 则直接关闭写端
    if (!channel_->IsWriting()) {
        // 关闭写端, 但是仍然可以读取数据
        socket_->ShutdownWrite();
    }
}

void TcpConnection::ConnectionEstablished() {
    SetState(kConnected);
    channel_->set_holder(shared_from_this());
    channel_->EnableReading();
    if (connection_callback_) connection_callback_(shared_from_this());
}

void TcpConnection::ConnectionDestroyed() {
    if (state_ == kConnected) {
        SetState(kDisconnected);
        channel_->DisableAll();
        if (connection_callback_) connection_callback_(shared_from_this());
    }
    channel_->Remove();
}
    
} // namespace connection
