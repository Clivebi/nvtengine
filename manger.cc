#include "manger.hpp"

#include "modules/net/socket.hpp"
#include "modules/streamsearch.hpp"
#include "modules/thirdpart/http-parser/http_parser.h"
extern "C" {
#include "thirdpart/masscan/pixie-threads.h"
}

using namespace Interpreter;

Value ParseJSON(std::string& str, bool unescape);

namespace manger_helper {
struct HeaderValue {
    std::string Name;
    std::string Value;
    HeaderValue() : Name(""), Value("") {}
};

class HttpRequest {
public:
    static const int kFIELD = 2;
    static const int kVALUE = 3;

    std::string Method;
    std::string Path;
    std::string Version;
    std::string Body;
    std::string RawHeader;
    std::vector<HeaderValue*> Header;
    int HeaderField;
    bool IsHeadCompleteCalled;
    bool IsMessageCompleteCalled;
    ~HttpRequest() {
        auto iter = Header.begin();
        while (iter != Header.end()) {
            delete *iter;
            iter++;
        }
    }
};

int header_url_cb(http_parser* p, const char* buf, size_t len) {
    HttpRequest* req = (HttpRequest*)p->data;
    req->Path.append(buf, len);
    return 0;
}

int header_field_cb(http_parser* p, const char* buf, size_t len) {
    HttpRequest* req = (HttpRequest*)p->data;
    HeaderValue* element = NULL;
    if (req->HeaderField != HttpRequest::kFIELD) {
        element = new HeaderValue();
        req->Header.push_back(element);
    } else {
        element = req->Header.back();
    }
    element->Name.append(buf, len);
    req->HeaderField = HttpRequest::kFIELD;
    return 0;
}

int header_value_cb(http_parser* p, const char* buf, size_t len) {
    HttpRequest* req = (HttpRequest*)p->data;
    HeaderValue* element = req->Header.back();
    element->Value.append(buf, len);
    req->HeaderField = HttpRequest::kVALUE;
    return 0;
}

int headers_complete_cb(http_parser* p) {
    HttpRequest* req = (HttpRequest*)p->data;
    req->IsHeadCompleteCalled = true;
    return 0;
}

int on_body_cb(http_parser* p, const char* buf, size_t len) {
    HttpRequest* req = (HttpRequest*)p->data;
    req->Body.append(buf, len);
    return 0;
}

int on_message_complete_cb(http_parser* p) {
    HttpRequest* req = (HttpRequest*)p->data;
    req->IsMessageCompleteCalled = true;
    return 0;
}

bool ReadHttpRequest(scoped_refptr<net::Conn> stream, HttpRequest* req) {
    StreamSearch search("\r\n\r\n");
    http_parser parser;
    http_parser_init(&parser, HTTP_REQUEST);
    parser.data = req;
    http_parser_settings settings = {0};
    settings.on_body = on_body_cb;
    settings.on_url = header_url_cb;
    settings.on_header_field = header_field_cb;
    settings.on_header_value = header_value_cb;
    settings.on_headers_complete = headers_complete_cb;
    settings.on_message_complete = on_message_complete_cb;
    std::array<char, 8192> buffer {};
    int matched = 0;
    while (true) {
        int size = stream->Read(buffer.data(), (int)buffer.size());
        if (size <= 0) {
            NVT_LOG_DEBUG("Read Error " + ToString(size));
            return false;
        }
        if (!matched) {
            matched = search.process(buffer.data(), size);
            if (matched) {
                req->RawHeader.append(buffer.data(), matched);
            } else {
                req->RawHeader.append(buffer.data(), size);
            }
        }
        int parse_size = (int)http_parser_execute(&parser, &settings, buffer.data(), (size_t)size);
        if (parser.http_errno != 0) {
            NVT_LOG_DEBUG("Http Parser Error");
            return false;
        }
        if (req->IsMessageCompleteCalled) {
            break;
        }
    }
    return true;
}
bool HttpWriteResponse(scoped_refptr<net::Conn> con, const std::string& body,
                       std::map<std::string, std::string>& headers,
                       std::string status_and_reason = "200 OK", std::string Version = "HTTP/1.1") {
    std::stringstream o;
    o << Version << " " << status_and_reason << "\r\n";
    headers["Content-Length"] = ToString(body.size());
    for (auto iter : headers) {
        o << iter.first << ": " << iter.second << "\r\n";
    }
    o << "\r\n";
    if (con->Write(o.str().c_str(), o.str().size()) <= 0) {
        return false;
    }
    if (body.size() == 0) {
        return true;
    }
    if (con->Write(body.c_str(), body.size()) <= 0) {
        return false;
    }
    return true;
}
} // namespace manger_helper

void MangerServer::OnNewConnection(scoped_refptr<net::Conn> con) {
    std::map<std::string, std::string> Header;
    Header["Connection"] = "Keep-Alive";
    Header["Content-type"] = "application/json";
    static std::string s_Parse_error =
            "{\"jsonrpc\": \"2.0\", \"error\": {\"code\": -32700, \"message\": \"Parse error\"}, "
            "\"id\": null}";
    con->SetReadTimeout(2 * 60);
    while (true) {
        manger_helper::HttpRequest req;
        if (!manger_helper::ReadHttpRequest(con, &req)) {
            break;
        }
        if (req.Body.size()) {
            manger_helper::HttpWriteResponse(con, s_Parse_error, Header);
            break;
        }
        Value request = ParseJSON(req.Body, true);
        if (!request.IsMap()) {
            manger_helper::HttpWriteResponse(con, s_Parse_error, Header);
            break;
        }
        Value response = this->ProcessRequest(request);
        std::string respText = response.ToJSONString();
        if (!manger_helper::HttpWriteResponse(con, respText, Header)) {
            break;
        }
    }
}

void MangerServer::AsyncNewConnection(scoped_refptr<net::Conn> con) {
    tcp_worker_thread_paramters* p = new tcp_worker_thread_paramters(this, con);
    pixie_begin_thread(MangerServer::tcp_worker_thread, 0, p);
}

void TCPForever(MangerServer* srv, std::string port, std::string host) {
    int fd = net::Socket::Listen(host.c_str(), port.c_str());
    sockaddr addr = {0};
    unsigned int addrLen = sizeof(sockaddr);
    if (fd <= 0) {
        NVT_LOG_ERROR("Listen on ", host + ":" + port, " failed");
        return;
    }
    while (true) {
        addrLen = sizeof(sockaddr);
        int con = net::Socket::Accept(fd, &addr, &addrLen);
        if (con <= 0) {
            NVT_LOG_ERROR("Accept on ", host + ":" + port, " failed");
            break;
        }
        net::BaseConn* base = new net::BaseConn(con, 5, 5, "tcp");
        scoped_refptr<net::Conn> conObj = new net::Conn(base, false);
        srv->AsyncNewConnection(conObj);
    }
}