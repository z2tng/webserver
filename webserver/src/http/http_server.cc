#include "http/http_server.h"
#include "log/logger.h"

namespace http {

HttpServer::HttpServer(event::EventLoop *loop,
                       const net::InetAddress &addr,
                       const std::string &name,
                       net::TcpServer::Option option)
        : server_(loop, addr, name, option) {
    server_.SetConnectionCallback(
        std::bind(&HttpServer::onConnection, this, std::placeholders::_1)
    );
    server_.SetMessageCallback(
        std::bind(&HttpServer::onMessage, this, std::placeholders::_1, std::placeholders::_2)
    );
}

void HttpServer::Start() {
    LOG_INFO << "HttpServer[" << server_.name() << "] starts listening on " << server_.ip_port();
    server_.Start();
}

void HttpServer::onConnection(const net::TcpConnectionPtr &conn) {
    LOG_INFO << "HttpServer - " << conn->local_addr().GetIpPort() << " -> "
             << conn->peer_addr().GetIpPort() << " is "
             << (conn->Connected() ? "UP" : "DOWN");
}

void HttpServer::onMessage(const net::TcpConnectionPtr &conn, net::Buffer *buf) {
    HttpRequestParser parser;
    if (parser.ParseRequest(buf)) {
        if (parser.GotAll()) {
            onRequest(conn, parser);
            parser.reset();
        }
    } else {
        conn->Shutdown();
    }
}

void HttpServer::onRequest(const net::TcpConnectionPtr &conn, const HttpRequestParser &req) {
    const std::string &connection = req.GetHeader("Connection");
    bool close = connection == "close" ||
                 (req.version() == "HTTP/1.0" && connection != "Keep-Alive");
    HttpResponse response(close);
    if (http_callback_) {
        http_callback_(req, &response);
    } else {
        response.SetStatusCode(HttpResponse::k404NotFound);
        response.SetStatusMessage("Not Found");
        response.SetCloseConnection(true);
    }

    net::Buffer buf;
    response.AppendToBuffer(&buf);
    conn->Send(&buf);
    if (response.close_connection()) {
        conn->Shutdown();
    }
}
    
} // namespace http
