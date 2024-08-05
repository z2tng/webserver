#include "http/http_request_parser.h"
#include "net/buffer.h"

#include <string>
#include <algorithm>

namespace http {

bool HttpRequestParser::ParseRequest(net::Buffer *input) {
    bool ok = true;
    bool has_more = true;
    while (has_more) {
        if (state_ == kExpectRequestLine) {
            const char *crlf = input->FindCRLF();
            if (crlf) {
                ok = ParseRequestLine(input->Peek(), crlf);
                if (ok) {
                    input->RetrieveUntil(crlf + 2);
                    state_ = kExpectHeaders;
                } else {
                    has_more = false;
                }
            } else {
                has_more = false;
            }
        } else if (state_ == kExpectHeaders) {
            const char *crlf = input->FindCRLF();
            if (crlf) {
                const char *colon = std::find(input->Peek(), crlf, ':');
                if (colon != crlf) {
                    headers_[std::string(input->Peek(), colon)] = std::string(colon + 2, crlf);
                } else {
                    state_ = kGotAll;
                    has_more = false;
                }
                input->RetrieveUntil(crlf + 2);
            } else {
                has_more = false;
            }
        } else if (state_ == kExpectBody) {
            // TODO
        }
    }
    return ok;
}

bool HttpRequestParser::ParseRequestLine(const char *begin, const char *end) {
    bool ok = false;
    const char *start = begin;
    const char *space = std::find(begin, end, ' ');

    if (space != end) {
        method_ = std::string(start, space);
        start = space + 1;
        space = std::find(start, end, ' ');
        if (space != end) {
            path_ = std::string(start, space);
            start = space + 1;
            ok = end - start == 8 && std::equal(start, end - 1, "HTTP/1.");
            if (ok) {
                if (*(end - 1) == '1') {
                    version_ = "HTTP/1.1";
                } else if (*(end - 1) == '0') {
                    version_ = "HTTP/1.0";
                } else {
                    ok = false;
                }
            }
        }
    }
    return ok;
}

void HttpRequestParser::reset() {
    state_ = kExpectRequestLine;
    method_.clear();
    path_.clear();
    version_.clear();
    headers_.clear();
}
    
} // namespace http
