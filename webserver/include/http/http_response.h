#pragma once

#include <string>
#include <unordered_map>

namespace net {
    
class Buffer;

} // namespace net


namespace http {

class HttpResponse {
public:
    enum HttpStatusCode {
        kUnknown,
        k200Ok = 200,
        k301MovedPermanently = 301,
        k400BadRequest = 400,
        k404NotFound = 404
    };

    explicit HttpResponse(bool close_connection)
        : status_code_(kUnknown),
          close_connection_(close_connection) {}
    ~HttpResponse() = default;

    void SetStatusCode(HttpStatusCode status_code) { status_code_ = status_code; }
    void SetStatusMessage(const std::string &message) { status_message_ = message; }
    void SetCloseConnection(bool on) { close_connection_ = on; }
    void SetBody(const std::string &body) { body_ = body; }

    bool close_connection() const { return close_connection_; }

    void AddHeader(const std::string &key, const std::string &value) {
        headers_[key] = value;
    }

    void AppendToBuffer(net::Buffer *output) const;

private:
    std::unordered_map<std::string, std::string> headers_;
    HttpStatusCode status_code_;
    std::string status_message_;
    bool close_connection_;
    std::string body_;
};
    
} // namespace http
