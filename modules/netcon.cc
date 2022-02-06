
#include <array>

#include "../engine/check.hpp"
#include "../engine/funcpref.hpp"
#include "./net/dial.hpp"
#include "./net/tcp.hpp"
#include "streamsearch.hpp"
using namespace Interpreter;
Value TCPConnect(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(4);
    std::string host = GetString(args, 0);
    std::string port = GetString(args, 1);
    int timeout = GetInt(args, 2, 30);
    int isSSL = GetInt(args, 3, 0);
    int enableCache = GetInt(args, 4, 1);
    Resource* res = net::Dial("tcp", host, port, timeout, isSSL, enableCache);
    if (res == NULL) {
        return Value();
    }
    return Value(res);
}

Value UDPConnect(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(4);
    std::string host = GetString(args, 0);
    std::string port = GetString(args, 1);
    Resource* res = net::Dial("udp", host, port, 0, 0, false);
    if (res == NULL) {
        NVT_LOG_ERROR("Connect Error:", host, ":", port);
        return Value();
    }
    return Value(res);
}

Value ConnRead(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(2);
    CHECK_PARAMETER_RESOURCE(0);
    int length = GetInt(args, 1);
    if (!net::Conn::IsConn(args[0].resource)) {
        throw RuntimeException("ConnRead resource type mismatch");
    }
    if (length > 1 * 1024 * 1024 || length < 0) {
        throw RuntimeException("ConnRead length must less 1M");
    }
    unsigned char* buffer = new unsigned char[length];
    net::Conn* con = (net::Conn*)(args[0].resource.get());
    int size = con->Read(buffer, (int)length);
    if (size < 0) {
        delete[] buffer;
        return Value();
    }
    std::string ret = "";
    ret.append((char*)buffer, size);
    delete[] buffer;
    return ret;
}

Value ConnSetReadTimeout(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(2);
    CHECK_PARAMETER_RESOURCE(0);
    int timeout = GetInt(args, 1);
    if (!net::Conn::IsConn(args[0].resource)) {
        throw RuntimeException("ConnRead resource type mismatch");
    }
    net::Conn* con = (net::Conn*)(args[0].resource.get());
    con->SetReadTimeout(timeout);
    return Value();
}

Value ConnSetWriteTimeout(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(2);
    CHECK_PARAMETER_RESOURCE(0);
    int timeout = GetInt(args, 1);
    if (!net::Conn::IsConn(args[0].resource)) {
        throw RuntimeException("ConnRead resource type mismatch");
    }
    net::Conn* con = (net::Conn*)(args[0].resource.get());
    con->SetWriteTimeout(timeout);
    return Value();
}

Value ConnWrite(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(2);
    CHECK_PARAMETER_RESOURCE(0);
    std::string data = GetString(args, 1);
    if (!net::Conn::IsConn(args[0].resource)) {
        throw RuntimeException("ConnWrite resource type mismatch");
    }
    net::Conn* con = (net::Conn*)(args[0].resource.get());
    int size = con->Write(data.c_str(), (int)data.size());
    return Value(size);
}

Value ConnReadUntil(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(3);
    CHECK_PARAMETER_RESOURCE(0);
    StreamSearch search(GetString(args, 1).c_str());
    int limit = GetInt(args, 2, 0);
    if (!net::Conn::IsConn(args[0].resource)) {
        throw RuntimeException("ConnReadUntil resource type mismatch");
    }
    net::Conn* con = (net::Conn*)(args[0].resource.get());
    std::string cache = "";
    if (limit == 0) {
        limit = 4 * 1024 * 1024;
    }
    char buffer[2];
    while (true) {
        int size = con->Read(buffer, 1);
        if (size != 1) {
            return Value();
        }
        cache += buffer[0];
        if (search.process(buffer, 1)) {
            return cache;
        }
        if (cache.size() > (size_t)limit) {
            return cache;
        }
    }
}

Value ConnTLSHandshake(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(1);
    CHECK_PARAMETER_RESOURCE(0);
    if (args[0].resource->TypeName() != "tcp") {
        throw RuntimeException("ConnTLSHandshake must used on tcp conn");
    }
    net::Conn* con = (net::Conn*)(args[0].resource.get());
    net::TCPConn* tcp = (net::TCPConn*)con->GetBaseConn();
    if (!tcp->SSLHandshake()) {
        return Value(false);
    }
    return Value(true);
}

Value ConnGetPeerCert(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(1);
    CHECK_PARAMETER_RESOURCE(0);
    if (args[0].resource->TypeName() != "tcp") {
        return Value();
    }
    net::Conn* con = (net::Conn*)(args[0].resource.get());
    net::TCPConn* tcp = (net::TCPConn*)con->GetBaseConn();
    scoped_refptr<Resource> res = tcp->GetPeerCert();
    if (res == NULL) {
        return Value();
    }
    return Value(res);
}

BuiltinMethod netConnMethod[] = {{"TCPConnect", TCPConnect},
                                 {"UDPConnect", UDPConnect},
                                 {"ConnRead", ConnRead},
                                 {"ConnWrite", ConnWrite},
                                 {"ConnReadUntil", ConnReadUntil},
                                 {"ConnTLSHandshake", ConnTLSHandshake},
                                 {"ConnSetReadTimeout", ConnSetReadTimeout},
                                 {"ConnSetWriteTimeout", ConnSetWriteTimeout},
                                 {"ConnGetPeerCert", ConnGetPeerCert}};

void RegisgerNetConnBuiltinMethod(Executor* vm) {
    vm->RegisgerFunction(netConnMethod, COUNT_OF(netConnMethod));
}