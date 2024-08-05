#pragma once

#include "utils/uncopyable.h"
#include "net/tcp_server.h"
#include "http/http_request_parser.h"
#include "http/http_response.h"

#include <functional>
#include <memory>

namespace http {

class HttpServer : utils::Uncopyable {
public:
    using HttpCallback = std::function<void(const HttpRequestParser&, HttpResponse*)>;

    HttpServer(event::EventLoop *loop,
               const net::InetAddress &addr,
               const std::string &name,
               net::TcpServer::Option option = net::TcpServer::kNoReusePort);
    ~HttpServer() = default;

    void SetHttpCallback(const HttpCallback &cb) { http_callback_ = cb; }

    void SetThreadNum(int num_threads) { server_.SetThreadNum(num_threads); }

    void Start();

private:
    void onConnection(const net::TcpConnectionPtr &conn);
    void onMessage(const net::TcpConnectionPtr &conn, net::Buffer *buf);
    void onRequest(const net::TcpConnectionPtr &conn, const HttpRequestParser &req);

    net::TcpServer server_;
    HttpCallback http_callback_;
};
    
} // namespace http
