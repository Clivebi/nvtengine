
#ifndef TCP_IMPL
#include "./network/tcpstream.cc"
#endif
#include <brotli/decode.h>
#include <zlib.h>

#include <array>
#include "check.hpp"
#include "thirdpart/http-parser/http_parser.h"
using namespace Interpreter;

struct HeaderValue {
    std::string Name;
    std::string Value;
};

class HTTPResponse {
public:
    static const int kFIELD = 2;
    static const int kVALUE = 3;

public:
    explicit HTTPResponse()
            : Version(""),
              Status(""),
              HeaderField(-1),
              IsHeadCompleteCalled(false),
              IsMessageCompleteCalled(false),
              ChunkedTotalBodySize(0),
              Reason(""),
              Body(""),
              RawHeader(""),
              Header() {}
    ~HTTPResponse() {
        auto iter = Header.begin();
        while (iter != Header.end()) {
            delete *iter;
            iter++;
        }
    }
    std::string Version;
    std::string Status;
    std::string Reason;
    std::string Body;
    std::string RawHeader;
    std::vector<HeaderValue*> Header;

    Value ToValue() {
        Value ret = Value::make_map();
        ret._map()["version"] = Version;
        ret._map()["status"] = Value(strtol(Status.c_str(), NULL, 0));
        ret._map()["reason"] = Reason;
        ret._map()["raw_header"] = RawHeader;
        ret._map()["body"] = Value::make_bytes(Body);
        Value h = Value::make_map();
        std::vector<HeaderValue*>::iterator iter = Header.begin();
        while (iter != Header.end()) {
            auto exist = h._map().find((*iter)->Name);
            if (exist != h._map().end()) {
                exist->second._array().push_back((*iter)->Value);
            } else {
                Value val = Value::make_array();
                val._array().push_back((*iter)->Value);
                h._map()[(*iter)->Name] = val;
            }
            iter++;
        }
        ret._map()["headers"] = h;
        return ret;
    }
    //helper
    int HeaderField;
    bool IsHeadCompleteCalled;
    bool IsMessageCompleteCalled;
    int64_t ChunkedTotalBodySize;

public:
    std::string GetHeaderValue(std::string name) {
        auto iter = Header.begin();
        while (iter != Header.end()) {
            if ((*iter)->Name == name) {
                return (*iter)->Value;
            }
            iter++;
        }
        return "";
    }
    void ParserFirstLine() {
        size_t i = RawHeader.find("\r\n");
        ParseStatus(RawHeader.substr(0, i));
    }

protected:
    bool ParseStatus(std::string line) {
        size_t i = line.find(" ");
        if (i == std::string::npos) {
            return false;
        }
        Version = line.substr(0, i);
        if (i + 1 >= line.size()) {
            return false;
        }
        line = line.substr(i + 1);
        i = line.find(" ");
        if (i == std::string::npos) {
            return false;
        }
        Status = line.substr(0, i);
        if (i + 1 >= line.size()) {
            return false;
        }
        Reason = line.substr(i + 1);
        return true;
    }
};

int header_status_cb(http_parser* p, const char* buf, size_t len) {
    HTTPResponse* resp = (HTTPResponse*)p->data;
    return 0;
}

int header_field_cb(http_parser* p, const char* buf, size_t len) {
    HTTPResponse* resp = (HTTPResponse*)p->data;
    HeaderValue* element = NULL;
    if (resp->HeaderField != HTTPResponse::kFIELD) {
        element = new HeaderValue();
        resp->Header.push_back(element);
    } else {
        element = resp->Header.back();
    }
    element->Name.append(buf, len);
    resp->HeaderField = HTTPResponse::kFIELD;
    return 0;
}

int header_value_cb(http_parser* p, const char* buf, size_t len) {
    HTTPResponse* resp = (HTTPResponse*)p->data;
    HeaderValue* element = resp->Header.back();
    element->Value.append(buf, len);
    resp->HeaderField = HTTPResponse::kVALUE;
    return 0;
}

int headers_complete_cb(http_parser* p) {
    HTTPResponse* resp = (HTTPResponse*)p->data;
    resp->IsHeadCompleteCalled = true;
    return 0;
}

int on_chunk_header(http_parser* p) {
    HTTPResponse* resp = (HTTPResponse*)p->data;
    resp->ChunkedTotalBodySize += p->content_length;
    return 0;
}

int on_body_cb(http_parser* p, const char* buf, size_t len) {
    HTTPResponse* resp = (HTTPResponse*)p->data;
    resp->Body.append(buf, len);
    return 0;
}

int on_message_complete_cb(http_parser* p) {
    HTTPResponse* resp = (HTTPResponse*)p->data;
    resp->IsMessageCompleteCalled = true;
    return 0;
}

bool DeflateStream(std::string& src, std::string& out) {
    z_stream strm = {0};
    inflateInit2(&strm, 32 + 15);
    size_t process_size = 0;
    Bytef* data = (Bytef*)src.c_str();

    int ret = Z_OK;
    strm.avail_in = src.size();
    strm.next_in = data;
    std::array<char, 8192> buffer {};
    while (strm.avail_in > 0) {
        strm.avail_out = buffer.size();
        strm.next_out = (Bytef*)buffer.data();
        ret = inflate(&strm, Z_NO_FLUSH);
        assert(ret != Z_STREAM_ERROR);
        switch (ret) {
        case Z_NEED_DICT:
        case Z_DATA_ERROR:
        case Z_MEM_ERROR:
            inflateEnd(&strm);
            return false;
        }
        out.append(buffer.data(), buffer.size() - strm.avail_out);
    }
    inflateEnd(&strm);
    return true;
}

bool BrotliDecompress(std::string& src, std::string& out) {
    BrotliDecoderResult result;
    BrotliDecoderState* state = BrotliDecoderCreateInstance(0, 0, 0);
    if (state == NULL) {
        return false;
    }
    const unsigned char* next_in = (const unsigned char*)src.c_str();
    size_t avail_in = src.size();
    size_t total_out = 0;
    std::array<char, 8192> buffer {};

    result = BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT;
    out.clear();
    while (result == BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT) {
        char* next_out = buffer.data();
        size_t avail_out = buffer.size();

        result = BrotliDecoderDecompressStream(state, &avail_in, &next_in, &avail_out,
                                               reinterpret_cast<uint8_t**>(&next_out), &total_out);

        out.append(next_out, total_out);
    }
    BrotliDecoderDestroyInstance(state);
    return result == BROTLI_DECODER_RESULT_SUCCESS;
}

struct StreamSearch {
    std::string keyword;
    int pos;
    StreamSearch(const char* key) : keyword(key), pos(0) {}
    int process(const char* buf, int size) {
        for (int i = 0; i < size; i++) {
            if (keyword[pos] != buf[i]) {
                pos = 0;
                continue;
            }
            pos++;
            if (pos == keyword.size()) {
                return i;
            }
        }
        return 0;
    }
};

bool DoReadHttpResponse(scoped_refptr<TCPStream> stream, HTTPResponse* resp) {
    StreamSearch search("\r\n\r\n");
    http_parser parser;
    http_parser_init(&parser, HTTP_RESPONSE);
    parser.data = resp;
    http_parser_settings settings = {0};
    settings.on_body = on_body_cb;
    settings.on_chunk_header = on_chunk_header;
    settings.on_status = header_status_cb;
    settings.on_header_field = header_field_cb;
    settings.on_header_value = header_value_cb;
    settings.on_headers_complete = headers_complete_cb;
    settings.on_message_complete = on_message_complete_cb;
    std::array<char, 8192> buffer {};
    int matched = 0;
    while (true) {
        int size = stream->Recv(buffer.data(), buffer.size());
        if (size < 0) {
            return false;
        }
        if (!matched) {
            matched = search.process(buffer.data(), size);
            if (matched) {
                resp->RawHeader.append(buffer.data(), matched);
                resp->ParserFirstLine();
            } else {
                resp->RawHeader.append(buffer.data(), size);
            }
        }
        int parse_size = http_parser_execute(&parser, &settings, buffer.data(), size);
        if (parser.http_errno != 0) {
            return false;
        }
        if (resp->IsMessageCompleteCalled) {
            break;
        }
    }
    std::string encoding = resp->GetHeaderValue("Content-Encoding");
    if (-1 != encoding.find("gzip") || -1 != encoding.find("deflate")) {
        std::string out = "";
        if (!DeflateStream(resp->Body, out)) {
            return false;
        }
        resp->Body = out;
        return true;
    }
    if (-1 != encoding.find("br")) {
        std::string out = "";
        if (!BrotliDecompress(resp->Body, out)) {
            return false;
        }
        resp->Body = out;
        return true;
    }
    return true;
}

bool DoHttpRequest(std::string& host, std::string& port, bool isSSL, std::string& req,
                   HTTPResponse* resp) {
    scoped_refptr<TCPStream> tcp = NewTCPStream(host, port, 60, isSSL);
    if (tcp.get() == NULL) {
        return false;
    }
    int size = tcp->Send(req.c_str(), req.size());
    if (size != req.size()) {
        return false;
    }
    return DoReadHttpResponse(tcp, resp);
}

enum encoding {

    encodePath = 1,
    encodePathSegment,
    encodeHost,
    encodeZone,
    encodeUserPassword,
    encodeQueryComponent,
    encodeFragment
};

bool containsChar(char c, std::string& list) {
    return list.find(c) != std::string::npos;
}

bool ishex(char c) {
    if ('0' <= c && c <= '9') {
        return true;
    }
    if ('a' <= c && c <= 'f') {
        return true;
    }
    if ('A' <= c && c <= 'F') {
        return true;
    }
    return false;
}

char http_unhex(char c) {
    if ('0' <= c && c <= '9') {
        return c - '0';
    }
    if ('a' <= c && c <= 'f') {
        return c - 'a' + 10;
    }
    if ('A' <= c && c <= 'F') {
        return c - 'A' + 10;
    }
    return false;
}

bool shouldEscape(char c, encoding mode) {
    if ('a' <= c && c <= 'z' || 'A' <= c && c <= 'Z' || '0' <= c && c <= '9') {
        return false;
    }
    if (mode == encodeHost || mode == encodeZone) {
        static std::string key1 = "!$&\'()*+,;=:[]<>\"";
        if (containsChar(c, key1)) {
            return false;
        }
    }
    static std::string key2 = "-_.~";
    if (containsChar(c, key2)) {
        return false;
    }
    static std::string key3 = "$&+,/:;=?@";
    if (containsChar(c, key3)) {
        switch (mode) {
        case encodePath:
            return c == '?';
        case encodePathSegment:
            return c == '/' || c == ';' || c == ',' || c == '?';
        case encodeUserPassword: // §3.2.1
            return c == '@' || c == '/' || c == '?' || c == ':';
        case encodeQueryComponent: // §3.4
            return true;
        case encodeFragment:
            return false;
        default:
            break;
        }
    }
    if (mode == encodeFragment) {
        if (c == '!' || c == '(' || c == ')' || c == '*') {
            return false;
        }
    }
    return true;
}

std::string escape(const std::string& s, encoding mode) {
    char c;
    int spaceCount = 0, hexCount = 0;
    for (int i = 0; i < s.size(); i++) {
        c = s[i];
        if (shouldEscape(c, mode)) {
            if (c == ' ' && mode == encodeQueryComponent) {
                spaceCount++;
            } else {
                hexCount++;
            }
        }
    }
    if (spaceCount == 0 && hexCount == 0) {
        return s;
    }
    std::string t = "";
    if (hexCount == 0) {
        t = s;
        for (int i = 0; i < t.size(); i++) {
            if (s[i] == ' ') {
                t[i] = '+';
            }
        }
        return t;
    }
    for (int i = 0; i < s.size(); i++) {
        static char upperhex[] = "0123456789ABCDEF";
        int c = s[i] & 0xFF;
        if (c == ' ' && mode == encodeQueryComponent) {
            t += '+';
        } else if (shouldEscape(c, mode)) {
            t += '%';
            t += upperhex[c >> 4];
            t += upperhex[c & 15];
        } else {
            t += c;
        }
    }
    return t;
}

bool unescape(const std::string s, std::string& t, encoding mode) {
    int n = 0;
    bool hashPlus = false;
    t.clear();
    for (size_t i = 0; i < s.size(); i++) {
        if (s[i] == '%') {
            n++;
            if (i + 2 > s.size() || !ishex(s[i + 1]) || !ishex(s[i + 2])) {
                return false;
            }
            if (mode == encodeHost && http_unhex(s[i + 1]) < 8 && s.substr(i, 3) != "%25") {
                return false;
            }
            if (mode == encodeZone) {
                int v = http_unhex(s[i + 1]) << 4 | http_unhex(s[i + 2]);
                if (s.substr(i, 3) != "%25" && v != ' ' && shouldEscape(v, encodeHost)) {
                    return false;
                }
            }
            i += 2;
        } else if (s[i] == '+') {
            hashPlus = (mode == encodeQueryComponent);
        } else {
            if ((mode == encodeHost || mode == encodeZone) && (int)s[i] < 0x80 &&
                shouldEscape(s[i], mode)) {
                return false;
            }
        }
    }
    if (n == 0 && !hashPlus) {
        t = s;
        return true;
    }
    for (size_t i = 0; i < s.size(); i++) {
        if (s[i] == '%') {
            t.append(1, http_unhex(s[i + 1]) << 4 | http_unhex(s[i + 2]));
            i += 2;
        } else if (s[i] == '+') {
            if (mode == encodeQueryComponent) {
                t += " ";
            } else {
                t += "+";
            }
        } else {
            t += s[i];
        }
    }
    return true;
}

bool parser_querys(std::string& query, std::list<std::pair<std::string, std::string>>& result) {
    std::list<std::string> pairs = split(query, '&');
    auto iter = pairs.begin();
    while (iter != pairs.end()) {
        if (containsChar(';', *iter)) {
            return false;
        }
        if (iter->size() != 0) {
            size_t i = iter->find('=');
            if (i != iter->npos) {
                std::string key = iter->substr(0, i);
                std::string value = "";
                if (i + 1 < iter->size()) {
                    value = iter->substr(i + 1);
                }
                if (unescape(value, value, encodeQueryComponent) &&
                    unescape(key, key, encodeQueryComponent)) {
                    result.push_back(std::pair<std::string, std::string>(key, value));
                }
            }
        }
        iter++;
    }
    return true;
}

bool parser_url(std::string& url, std::string& scheme, std::string& host, std::string& port,
                std::string& path, std::map<std::string, std::string>& querys) {
    std::string args = "";
    size_t i = url.find(":");
    if (i == std::string::npos) {
        return false;
    }
    scheme = url.substr(0, i);
    i += 3;
    if (i > url.size()) {
        return false;
    }
    std::string part = url.substr(i);
    i = part.find("/");
    std::string host_with_port = "";
    if (i == std::string::npos) {
        host_with_port = part;
        part = "/";
    } else {
        host_with_port = part.substr(0, i);
        part = part.substr(i);
    }
    std::list<std::string> host_port = split(host_with_port, ':');
    if (host_port.size() > 0) {
        host = host_port.front();
    }
    if (host_port.size() > 1) {
        port = host_port.back();
    }
    if (port.size() == 0) {
        if (scheme == "https") {
            port = "443";
        } else if (scheme == "http") {
            port = "80";
        }
    }
    i = part.find("?");
    if (i == std::string::npos) {
        path = part;
        return true;
    }
    path = part.substr(0, i);
    if (part.size() > i + 1) {
        args = part.substr(i + 1);
    }
    if (args.size() > 0) {
        std::list<std::pair<std::string, std::string>> result;
        if (parser_querys(args, result)) {
            auto iter = result.begin();
            while (iter != result.end()) {
                querys.insert(*iter);
                iter++;
            }
        }
    }
    return true;
}

Value URLQueryUnescape(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(1);
    CHECK_PARAMETER_STRING(0);
    std::string result = "";
    if (!unescape(args[0].bytes, result, encodeQueryComponent)) {
        return Value();
    }
    return Value(result);
}

Value URLQueryEscape(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(1);
    CHECK_PARAMETER_STRING(0);
    return Value(escape(args[0].bytes, encodeQueryComponent));
}

Value URLPathEscape(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(1);
    CHECK_PARAMETER_STRING(0);
    return Value(escape(args[0].bytes, encodePathSegment));
}
Value URLPathUnescape(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(1);
    CHECK_PARAMETER_STRING(0);
    std::string result = "";
    if (!unescape(args[0].bytes, result, encodePathSegment)) {
        return Value();
    }
    return Value(result);
}

Value URLQueryDecode(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(1);
    CHECK_PARAMETER_STRING(0);
    Value ret = Value::make_map();
    std::list<std::pair<std::string, std::string>> result;
    if (parser_querys(args[0].bytes, result)) {
        auto iter = result.begin();
        while (iter != result.end()) {
            ret._map()[iter->first] = iter->second;
            iter++;
        }
    }
    return ret;
}

std::string query_encode(std::map<std::string, std::string>& querys) {
    std::string result = "";
    auto iter = querys.begin();
    while (iter != querys.end()) {
        if (result.size()) {
            result += "&";
        }
        result += escape(iter->first, encodeQueryComponent);
        result += "=";
        result += escape(iter->second, encodeQueryComponent);
        iter++;
    }
    return result;
}

Value URLQueryEncode(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(1);
    CHECK_PARAMETER_MAP(0);
    std::string result = "";
    auto iter = args[0]._map().begin();
    while (iter != args[0]._map().end()) {
        if (iter->first.Type != ValueType::kString && iter->first.Type != ValueType::kBytes) {
            throw RuntimeException("URLQueryEncode map key must string or bytes");
        }
        if (result.size()) {
            result += "&";
        }
        result += escape(iter->first.bytes, encodeQueryComponent);
        result += "=";
        result += escape(iter->second.ToString(), encodeQueryComponent);
        iter++;
    }
    return Value(result);
}

Value ReadHttpResponse(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(1);
    CHECK_PARAMETER_RESOURCE(0);
    scoped_refptr<TCPStream> tcp = (TCPStream*)args[0].resource.get();
    HTTPResponse resp;
    if (DoReadHttpResponse(tcp, &resp)) {
        return resp.ToValue();
    }
    return Value();
}

Value DeflateBytes(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(1);
    CHECK_PARAMETER_STRING(0);
    Value ret = Value::make_bytes("");
    ret.Type = args[0].Type;
    DeflateStream(args[0].bytes, ret.bytes);
    return ret;
}

Value BrotliDecompressBytes(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(1);
    CHECK_PARAMETER_STRING(0);
    Value ret = Value::make_bytes("");
    ret.Type = args[0].Type;
    BrotliDecompress(args[0].bytes, ret.bytes);
    return ret;
}

Value URLParse(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(1);
    CHECK_PARAMETER_STRING(0);
    std::string host, port, scheme, path;
    std::map<std::string, std::string> querys;
    parser_url(args[0].bytes, scheme, host, port, path, querys);
    Value ret = Value::make_map();
    ret._map()["host"] = host;
    ret._map()["port"] = port;
    ret._map()["scheme"] = scheme;
    ret._map()["path"] = path;
    Value q = Value::make_map();
    auto iter = querys.begin();
    while (iter != querys.end()) {
        q._map()[iter->first] = iter->second;
        iter++;
    }
    ret._map()["query"] = q;
    return ret;
}

void BuildRequestHeader(Value val, std::string& forHost, std::string& port,
                        std::map<std::string, std::string>& result) {
    if (val.Type == ValueType::kMap) {
        auto iter = val._map().begin();
        while (iter != val._map().end()) {
            if (iter->first.Type != ValueType::kString && iter->first.Type != ValueType::kBytes) {
                throw RuntimeException("the httpheader map key must string or bytes");
            }
            result[iter->first.bytes] = iter->second.ToString();
            iter++;
        }
    }
    auto iter = result.find("User-Agent");
    if (iter == result.end()) {
        result["User-Agent"] = "One Script HTTP module V 1.0";
    }
    iter = result.find("Connection");
    if (iter == result.end()) {
        result["Connection"] = "close";
    }
    iter = result.find("Host");
    if (iter == result.end()) {
        if (port != "80" && port != "443") {
            result["Host"] = forHost + ":" + port;
        } else {
            result["Host"] = forHost;
        }
    }
}

Value HttpGet(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(1);
    CHECK_PARAMETER_STRING(0);
    std::map<std::string, std::string> headers;
    std::string host, port, scheme, path;
    std::map<std::string, std::string> querys;
    parser_url(args[0].bytes, scheme, host, port, path, querys);
    if (args.size() == 2 && args[1].Type == ValueType::kMap) {
        BuildRequestHeader(args[1], host, port, headers);
    } else {
        BuildRequestHeader(Value(), host, port, headers);
    }

    std::stringstream o;
    o << "GET " << escape(path, encodePath);
    if (querys.size()) {
        o << "?" << query_encode(querys);
    }
    o << " HTTP/1.1\r\n";
    auto iter = headers.begin();
    while (iter != headers.end()) {
        o << iter->first << ": " << iter->second << "\r\n";
        iter++;
    }
    o << "\r\n";
    std::string req = o.str();
    HTTPResponse resp;
    if (!DoHttpRequest(host, port, scheme == "https", req, &resp)) {
        return Value();
    }
    return resp.ToValue();
}

Value HttpPost(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(3);
    CHECK_PARAMETER_STRING(0); //URL
    CHECK_PARAMETER_STRING(1); //contentType
    CHECK_PARAMETER_STRING(2); //content
    std::map<std::string, std::string> headers;
    std::map<std::string, std::string> querys;
    std::string host, port, scheme, path;
    parser_url(args[0].bytes, scheme, host, port, path, querys);
    if (args.size() == 4 && args[3].Type == ValueType::kMap) {
        BuildRequestHeader(args[3], host, port, headers);
    } else {
        BuildRequestHeader(Value(), host, port, headers);
    }
    headers["Content-Type"] = args[1].bytes;
    headers["Content-Length"] = Value(args[2].bytes.size()).ToString();
    std::stringstream o;
    o << "POST " << escape(path, encodePath);
    if (querys.size()) {
        o << "?" << query_encode(querys);
    }
    o << " HTTP/1.1\r\n";
    auto iter = headers.begin();
    while (iter != headers.end()) {
        o << iter->first << ": " << iter->second << "\r\n";
        iter++;
    }
    o << "\r\n";
    o << args[2].bytes;

    std::string req = o.str();
    HTTPResponse resp;
    if (!DoHttpRequest(host, port, scheme == "https", req, &resp)) {
        return Value();
    }
    return resp.ToValue();
}

Value HttpPostForm(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(2);
    CHECK_PARAMETER_STRING(0); //URL
    CHECK_PARAMETER_MAP(1);    //URLValues
    //CHECK_PARAMETER_MAP(2); //addtions headers
    std::vector<Value> querys;
    std::map<std::string, std::string> uq;
    querys.push_back(args[1]);
    Value query = URLQueryEncode(querys, ctx, vm);
    std::map<std::string, std::string> headers;
    std::string host, port, scheme, path;
    parser_url(args[0].bytes, scheme, host, port, path, uq);
    if (args.size() == 3 && args[2].Type == ValueType::kMap) {
        BuildRequestHeader(args[2], host, port, headers);
    } else {
        BuildRequestHeader(Value(), host, port, headers);
    }
    headers["Content-Type"] = "application/x-www-form-urlencoded";
    headers["Content-Length"] = Value(query.bytes.size()).ToString();
    std::stringstream o;
    o << "POST " << escape(path, encodePath);
    if (uq.size()) {
        o << "?" << query_encode(uq);
    }
    o << " HTTP/1.1\r\n";
    auto iter = headers.begin();
    while (iter != headers.end()) {
        o << iter->first << ": " << iter->second << "\r\n";
        iter++;
    }
    o << "\r\n";
    o << query.bytes;

    std::string req = o.str();
    std::cout << req << std::endl;
    HTTPResponse resp;
    if (!DoHttpRequest(host, port, scheme == "https", req, &resp)) {
        return Value();
    }
    return resp.ToValue();
}

BuiltinMethod httpMethod[] = {{"HttpGet", HttpGet},
                              {"HttpPost", HttpPost},
                              {"HttpPostForm", HttpPostForm},
                              {"DeflateBytes", DeflateBytes},
                              {"DeflateString", DeflateBytes},
                              {"BrotliDecompressBytes", BrotliDecompressBytes},
                              {"BrotliDecompressString", BrotliDecompressBytes},
                              {"URLParse", URLParse},
                              {"URLQueryDecode", URLQueryDecode},
                              {"URLQueryEncode", URLQueryEncode},
                              {"URLQueryEscape", URLQueryEscape},
                              {"URLPathEscape", URLPathEscape},
                              {"URLQueryUnescape", URLQueryUnescape},
                              {"URLPathUnescape", URLPathUnescape},
                              {"ReadHttpResponse", ReadHttpResponse}};

void RegisgerHttpBuiltinMethod(Executor* vm) {
    vm->RegisgerFunction(httpMethod, COUNT_OF(httpMethod));
}