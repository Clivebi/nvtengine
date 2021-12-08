
#include <array>

#include "./net/dial.hpp"
#include "./net/tcp.hpp"
#include "check.hpp"
using namespace Interpreter;
Value TCPConnect(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(4);
    CHECK_PARAMETER_STRING(0);
    std::string host = args[0].bytes, port;
    if (args[1].IsInteger()) {
        port = args[1].ToString();
    } else {
        CHECK_PARAMETER_STRING(1);
        port = args[1].bytes;
    }
    CHECK_PARAMETER_INTEGER(2);
    CHECK_PARAMETER_INTEGER(3);
    Resource* res = net::Dial("tcp", host, port, (int)args[2].Integer, args[3].ToBoolean(), true);
    if (res == NULL) {
        return Value();
    }
    return Value(res);
}

Value UDPConnect(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(4);
    CHECK_PARAMETER_STRING(0);
    std::string host = args[0].bytes, port;
    if (args[1].IsInteger()) {
        port = args[1].ToString();
    } else {
        CHECK_PARAMETER_STRING(1);
        port = args[1].bytes;
    }
    CHECK_PARAMETER_INTEGER(2);
    CHECK_PARAMETER_INTEGER(3);
    Resource* res = net::Dial("udp", host, port, (int)args[2].Integer, args[3].ToBoolean(), false);
    if (res == NULL) {
        return Value();
    }
    return Value(res);
}

Value ConnRead(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(2);
    CHECK_PARAMETER_RESOURCE(0);
    CHECK_PARAMETER_INTEGER(1);
    if (!net::Conn::IsConn(args[0].resource)) {
        throw RuntimeException("ConnRead resource type mismatch");
    }
    if (args[1].Integer > 1 * 1024 * 1024) {
        throw RuntimeException("ConnRead length must less 1M");
    }
    unsigned char* buffer = new unsigned char[args[1].Integer];
    net::Conn* con = (net::Conn*)(args[0].resource.get());
    int size = con->Read(buffer, (int)args[1].Integer);
    if (size < 0) {
        delete[] buffer;
        return Value();
    }
    Value ret = Value::make_bytes("");
    ret.bytes.assign((char*)buffer, size);
    delete[] buffer;
    return ret;
}

Value ConnSetReadTimeout(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(2);
    CHECK_PARAMETER_RESOURCE(0);
    CHECK_PARAMETER_INTEGER(1);
    if (!net::Conn::IsConn(args[0].resource)) {
        throw RuntimeException("ConnRead resource type mismatch");
    }
    net::Conn* con = (net::Conn*)(args[0].resource.get());
    con->SetReadTimeout(args[1].Integer);
    return Value();
}

Value ConnSetWriteTimeout(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(2);
    CHECK_PARAMETER_RESOURCE(0);
    CHECK_PARAMETER_INTEGER(1);
    if (!net::Conn::IsConn(args[0].resource)) {
        throw RuntimeException("ConnRead resource type mismatch");
    }
    net::Conn* con = (net::Conn*)(args[0].resource.get());
    con->SetWriteTimeout(args[1].Integer);
    return Value();
}

Value ConnWrite(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(2);
    CHECK_PARAMETER_RESOURCE(0);
    CHECK_PARAMETER_STRING(1);
    if (!net::Conn::IsConn(args[0].resource)) {
        throw RuntimeException("ConnWrite resource type mismatch");
    }
    net::Conn* con = (net::Conn*)(args[0].resource.get());
    int size = con->Write(args[1].bytes.c_str(), args[1].bytes.size());
    return Value(size);
}

Value ConnReadUntil(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(3);
    CHECK_PARAMETER_RESOURCE(0);
    CHECK_PARAMETER_STRING(1);
    CHECK_PARAMETER_INTEGER(2);
    if (!net::Conn::IsConn(args[0].resource)) {
        throw RuntimeException("ConnReadUntil resource type mismatch");
    }
    net::Conn* con = (net::Conn*)(args[0].resource.get());
    std::string cache = "";
    int limit = args[2].Integer;
    if (limit == 0) {
        limit = 4 * 1024 * 1024;
    }
    char buffer[2];
    std::string& key = args[1].bytes;
    while (true) {
        int size = con->Read(buffer, 1);
        if (size != 1) {
            return Value();
        }
        cache += buffer[0];
        if (buffer[0] == key.back()) {
            if (cache.size() >= key.size()) {
                if (!strncmp(cache.c_str() + (cache.size() - key.size()), key.c_str(),
                             key.size())) {
                    return cache;
                }
            }
        }
        if (cache.size() > limit) {
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

BuiltinMethod netConnMethod[] = {{"TCPConnect", TCPConnect},
                                 {"UDPConnect", UDPConnect},
                                 {"ConnRead", ConnRead},
                                 {"ConnWrite", ConnWrite},
                                 {"ConnReadUntil", ConnReadUntil},
                                 {"ConnTLSHandshake", ConnTLSHandshake},
                                 {"ConnSetReadTimeout", ConnSetReadTimeout},
                                 {"ConnSetWriteTimeout", ConnSetWriteTimeout}};

void RegisgerNetConnBuiltinMethod(Executor* vm) {
    vm->RegisgerFunction(netConnMethod, COUNT_OF(netConnMethod));
}