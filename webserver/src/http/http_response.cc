#include "http/http_response.h"
#include "net/buffer.h"

#include <cstring>

namespace http {

void HttpResponse::AppendToBuffer(net::Buffer *output) const {
    char buf[32];
    snprintf(buf, sizeof(buf), "HTTP/1.1 %d ", status_code_);
    output->Append(buf, strlen(buf));
    output->Append(status_message_);
    output->Append("\r\n");

    if (close_connection_) {
        output->Append("Connection: close\r\n");
    } else {
        output->Append("Connection: Keep-Alive\r\n");
    }

    for (const auto &header : headers_) {
        output->Append(header.first);
        output->Append(": ");
        output->Append(header.second);
        output->Append("\r\n");
    }

    if (!body_.empty()) {
        output->Append("Content-Length: ");
        output->Append(std::to_string(body_.size()));
        output->Append("\r\n\r\n");
        output->Append(body_);
    } else {
        output->Append("Content-Length: 0\r\n\r\n");
    }
}
    
} // namespace http
