#include "net/tcp_server.h"
#include "log/logger.h"

namespace net {

static int CreateSocket() {
    int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
    if (sockfd < 0) {
        LOG_FATAL << "CreateSocket";
    }
    return sockfd;
}

TcpServer::TcpServer(event::EventLoop *loop,
                     const InetAddress &addr,
                     const std::string &name,
                     Option option)
        : loop_(loop),
          name_(name),
          addr_(std::make_shared<InetAddress>(addr)),
          accept_socket_(std::make_shared<Socket>(CreateSocket())),
          accept_channel_(std::make_shared<event::Channel>(loop_, accept_socket_->fd())),
          thread_pool_(std::make_shared<event::EventLoopThreadPool>(loop_)),
          connection_callback_(),
          message_callback_(),
          started_(false),
          next_conn_id_(1) {
    accept_socket_->SetReuseAddr(true);
    accept_socket_->SetReusePort(option == kReusePort);
    accept_socket_->Bind(*addr_);
    accept_channel_->SetReadCallback(std::bind(&TcpServer::HandleNewConnection, this));
}

TcpServer::~TcpServer() {
    accept_channel_->DisableAll();
    accept_channel_->Remove();

    for (auto &item: connection_map_) {
        TcpConnectionPtr conn = item.second;
        item.second.reset();
        conn->GetLoop()->RunInLoop(
            std::bind(&TcpConnection::ConnectionDestroyed, conn)
        );
    }
}

void TcpServer::SetThreadNum(int num_threads) {
    thread_pool_->SetThreadNum(num_threads);
}

void TcpServer::Start() {
    thread_pool_->Start(thread_init_callback_);
    loop_->RunInLoop(
        [this]() {
            accept_socket_->Listen();
            accept_channel_->EnableReading();
        }
    );
}

void TcpServer::HandleNewConnection() {
    InetAddress peer_addr;
    int connfd = accept_socket_->Accept(&peer_addr);

    if (connfd < 0) {
        LOG_FATAL << "Accept error: " << errno;
        if (errno == EMFILE) {
            LOG_FATAL << "Too many open files";
        }
        return;
    }

    event::EventLoop *loop = thread_pool_->GetNextLoop();
    std::string conn_name = name_ + "-" + std::to_string(next_conn_id_++);
    LOG_INFO << "TcpServer::HandleNewConnection [" << name_
             << "] - new connection [" << conn_name
             << "] from " << peer_addr.GetIpPort();

    TcpConnectionPtr conn = std::make_shared<TcpConnection>(
        loop, connfd, *addr_, peer_addr
    );
    connection_map_.insert({connfd, conn});

    conn->SetConnectionCallback(connection_callback_);
    conn->SetMessageCallback(message_callback_);
    conn->SetWriteCompleteCallback(write_complete_callback_);

    conn->SetCloseCallback(
        std::bind(&TcpServer::RemoveConnection, this, std::placeholders::_1)
    );
    loop->RunInLoop(
        std::bind(&TcpConnection::ConnectionEstablished, conn)
    );
}

void TcpServer::RemoveConnection(const TcpConnectionPtr &conn) {
    loop_->RunInLoop(
        std::bind(&TcpServer::RemoveConnectionInLoop, this, conn)
    );
}

void TcpServer::RemoveConnectionInLoop(const TcpConnectionPtr &conn) {
    LOG_INFO << "TcpServer::RemoveConnection [" << name_
             << "] - connection [" << conn->fd()
             << "] from " << conn->peer_addr().GetIpPort();

    connection_map_.erase(conn->fd());
    event::EventLoop *loop = conn->GetLoop();
    loop->QueueInLoop(
        std::bind(&TcpConnection::ConnectionDestroyed, conn)
    );
}

} // namespace net
