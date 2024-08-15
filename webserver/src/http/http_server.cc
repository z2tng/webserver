#include "http/http_server.h"
#include "cache/lfu_cache.h"
#include "log/logger.h"

#include <sys/stat.h>

namespace http {


void DefaultHttpCallback(const HttpRequestParser &req,
                         HttpResponse &resp,
                         const std::string &web_root) {
    resp.SetStatusCode(HttpResponse::k404NotFound);
    resp.SetStatusMessage("Not Found");
    resp.SetCloseConnection(true);
}

void CacheTestHttpCallback(const HttpRequestParser &req,
                           HttpResponse &resp,
                           const std::string &web_root) {
    resp.SetStatusCode(HttpResponse::k200Ok);
    resp.SetStatusMessage("OK");
    resp.SetCloseConnection(false);
    resp.AddHeader("Server", "LFU Cache Server");

    std::string file_name = req.path();
    if (file_name == "/" || file_name.empty()) {
        file_name = "/index.html";
    }
    size_t question_mark = file_name.find('?');
    if (question_mark != std::string::npos) {
        file_name = file_name.substr(0, question_mark);
    }

    std::string file_path = web_root + file_name;
    struct stat file_stat;
    if (stat(file_path.c_str(), &file_stat) < 0) {
        // 文件不存在
        resp.SetStatusCode(HttpResponse::k403Forbidden);
        resp.SetStatusMessage("Forbidden");
        resp.SetCloseConnection(true);
        return;
    }

    if (!(S_ISREG(file_stat.st_mode) || !(S_IRUSR & file_stat.st_mode))) {
        // 文件不是普通文件或者文件不可读
        resp.SetStatusCode(HttpResponse::k403Forbidden);
        resp.SetStatusMessage("Forbidden");
        resp.SetCloseConnection(true);
        return;
    }

    int pos = file_name.find_last_of('.');
    std::string file_type;
    if (pos != std::string::npos) {
        file_type = file_name.substr(pos);
    } else {
        file_type = "text/plain";
    }

    resp.AddHeader("Content-Type", file_type);
    resp.AddHeader("Content-Length", std::to_string(file_stat.st_size));

    if (req.method() == "HEAD") {
        return;
    }

    std::string file_content;
    if (!cache::LfuCache::instance().Get(file_name, file_content)) {
        FILE *fp = fopen(file_path.c_str(), "rb");
        if (fp == nullptr) {
            resp.SetStatusCode(HttpResponse::k404NotFound);
            resp.SetStatusMessage("Not Found");
            resp.SetCloseConnection(true);
            return;
        }

        char buffer[4096];
        size_t nread;
        while ((nread = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
            file_content.append(buffer, nread);
        }
        cache::LfuCache::instance().Set(file_name, file_content);
        fclose(fp);
    }

    resp.SetBody(file_content);
}

HttpServer::HttpServer(event::EventLoop *loop, const net::InetAddress &addr)
        : server_(loop, addr, "HttpServer"),
          http_callback_(CacheTestHttpCallback) {
    server_.SetConnectionCallback(
        std::bind(&HttpServer::onConnection, this, std::placeholders::_1)
    );
    server_.SetMessageCallback(
        std::bind(&HttpServer::onMessage, this, std::placeholders::_1, std::placeholders::_2)
    );

    char cwd[128];
    if (getcwd(cwd, sizeof(cwd)) == nullptr) {
        LOG_FATAL << "getcwd error";
    }
    web_root_ = std::string(cwd) + "/pages";
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
        http_callback_(req, response, web_root_);
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
