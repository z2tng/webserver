#pragma once

#include <string>
#include <unordered_map>

namespace net {

class Buffer;
    
} // namespace net


namespace http {

class HttpRequestParser {
public:
    enum HttpRequestParseState {
        kExpectRequestLine,
        kExpectHeaders,
        kExpectBody,
        kGotAll
    };

    HttpRequestParser() : state_(kExpectRequestLine) {}

    bool ParseRequest(net::Buffer *input);
    bool GotAll() const { return state_ == kGotAll; }
    void reset();

    const std::string method() const { return method_; }
    const std::string path() const { return path_; }
    const std::string version() const { return version_; }
    const std::unordered_map<std::string, std::string> headers() const { return headers_; }

    const std::string GetHeader(const std::string &field) const {
        auto it = headers_.find(field);
        return it == headers_.end() ? std::string() : it->second;
    }

private:
    bool ParseRequestLine(const char *begin, const char *end);

    HttpRequestParseState state_;
    std::string method_;
    std::string path_;
    std::string version_;
    std::unordered_map<std::string, std::string> headers_;
};
    
} // namespace http
