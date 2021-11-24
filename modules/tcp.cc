
#include <array>

#include "./network/tcpstream.hpp"
#include "check.hpp"
using namespace Interpreter;
Value TCPConnect(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(4);
    CHECK_PARAMETER_STRING(0);
    std::string host = args[0].bytes, port;
    if (args[1].Type == ValueType::kInteger) {
        port = args[1].ToString();
    } else {
        CHECK_PARAMETER_STRING(1);
        port = args[1].bytes;
    }
    CHECK_PARAMETER_INTEGER(2);
    CHECK_PARAMETER_INTEGER(3);
    Resource* res = NewTCPStream(host, port, (int)args[2].Integer, args[3].ToBoolean());
    if (res == NULL) {
        return Value();
    }
    return Value(res);
}

Value TCPRead(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(2);
    CHECK_PARAMETER_RESOURCE(0);
    CHECK_PARAMETER_INTEGER(1);
    if (args[1].Integer > 1 * 1024 * 1024) {
        throw RuntimeException("TCPRead length must less 1M");
    }

    unsigned char* buffer = (unsigned char*)malloc((size_t)args[1].Integer);
    if (buffer == NULL) {
        return Value();
    }
    TCPStream* stream = (TCPStream*)(args[0].resource.get());
    int size = stream->Recv(buffer, (int)args[1].Integer);
    if (size < 0) {
        free(buffer);
        return Value();
    }
    Value ret = Value::make_bytes("");
    ret.bytes.assign((char*)buffer, size);
    free(buffer);
    return ret;
}

Value TCPWrite(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(2);
    CHECK_PARAMETER_RESOURCE(0);
    CHECK_PARAMETER_STRING(1);
    TCPStream* stream = (TCPStream*)(args[0].resource.get());
    int size = stream->Send(args[1].bytes.c_str(), args[1].bytes.size());
    return Value(size);
}

BuiltinMethod tcpMethod[] = {
        {"TCPConnect", TCPConnect},
        {"TCPWrite", TCPWrite},
        {"TCPRead", TCPRead},
};

void RegisgerTcpBuiltinMethod(Executor* vm) {
    vm->RegisgerFunction(tcpMethod, COUNT_OF(tcpMethod));
}