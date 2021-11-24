#include "../../network/tcpstream.hpp"
#include "../api.hpp"

#define NOT_IMPLEMENT() throw RuntimeException("Not Implement:" + std::string(__FUNCTION__))

class UDPConnection : public Interpreter::Resource {
public:
    int mSocket;

public:
    explicit UDPConnection(int fd) { mSocket = fd; }
    void Close() {
        if (mSocket != -1) {
            shutdown(mSocket, SHUT_RDWR);
            mSocket = -1;
        }
    }
    bool IsAvaliable() { return mSocket != -1; }

    int Recv(void* buf, int size, int timeout = 60, int option = 0) {
        if (!Socket::WaitSocketAvaliable(mSocket, timeout, true)) {
            return -1;
        }
        return recv(mSocket, buf, size, option);
    }

    int Send(const void* buf, int size, int timeout = 60, int option = 0) {
        if (!Socket::WaitSocketAvaliable(mSocket, timeout, false)) {
            return -1;
        }
        return send(mSocket, buf, size, option);
    }

    std::string TypeName() { return "UDPConnection"; }
};

UDPConnection* NewUDPConnection(std::string& host, std::string& port) {
    int sockfd = Socket::OpenUDPConnection(host.c_str(), port.c_str());
    if (sockfd < 0) {
        return NULL;
    }
    Socket::SetBlock(sockfd, 0);
    return new UDPConnection(sockfd);
}

//port,transport,timeout,
Value open_sock_tcp(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    OVAContext* script = GetOVAContext(vm);
    CHECK_PARAMETER_COUNT(3);
    bool isSSL = false;
    int transport = GetInt(args, 1, 0);
    int timeout = GetInt(args, 2, 60);
    if (transport == 0) {
        isSSL = (args[0].Integer == 443);
    }
    if (transport == 1) {
        isSSL = false;
    }
    std::string port = args[0].ToString();
    //TODO set NSI
    Resource* res = NewTCPStream(script->Host, port, timeout, isSSL);
    if (res == NULL) {
        return Value();
    }
    return Value(res);
}

//
Value open_sock_udp(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    OVAContext* script = GetOVAContext(vm);
    CHECK_PARAMETER_COUNT(1);
    CHECK_PARAMETER_INTEGER(0);
    std::string port = args[0].ToString();
    Resource* res = NewUDPConnection(script->Host, port);
    if (res == NULL) {
        return Value();
    }
    return Value(res);
}

Value open_priv_sock_tcp(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    NOT_IMPLEMENT();
    return Value();
}

Value open_priv_sock_udp(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    NOT_IMPLEMENT();
    return Value();
}

template <class T>
int ReadAtleast(T* x, int timeout, BYTE* buffer, int bufferSize, int minSize) {
    int nPos = 0, i;
    while (true) {
        i = x->Recv(buffer + nPos, bufferSize - nPos, timeout);
        if (i <= 0) {
            return i;
        }
        nPos += i;
        if (nPos > minSize) {
            break;
        }
    }
    return nPos;
}

//socket,length,timeout,min_length
Value recv(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(2);
    CHECK_PARAMETER_RESOURCE(0);
    CHECK_PARAMETER_INTEGER(1);
    int length = GetInt(args, 1, 0);
    int timeout = GetInt(args, 2, 15);
    int minSize = GetInt(args, 3, 0);
    if (length < minSize) {
        length = minSize;
    }
    std::string name = args[0].resource->TypeName();
    unsigned char* buffer = (unsigned char*)malloc(length);
    if (buffer == NULL) {
        throw RuntimeException(std::string("allocate memory error ") + __FUNCTION__);
    }
    if (name == "TPCStream") {
        TCPStream* tcp = (TCPStream*)args[0].resource.get();
        int nPos = ReadAtleast<TCPStream>(tcp, timeout, buffer, length, minSize);
        if (nPos <= 0) {
            free(buffer);
            return NULL;
        }
        name.assign((char*)buffer, nPos);
        free(buffer);
        return name;
    } else {
        UDPConnection* udp = (UDPConnection*)args[0].resource.get();
        int nPos = ReadAtleast<UDPConnection>(udp, timeout, buffer, length, minSize);
        if (nPos <= 0) {
            free(buffer);
            return NULL;
        }
        name.assign((char*)buffer, nPos);
        free(buffer);
        return name;
    }
    return Value();
}

//socket,length,timeout
Value recv_line(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    NOT_IMPLEMENT();
    return Value();
}

//socket,data,length,option
Value send(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(2);
    CHECK_PARAMETER_RESOURCE(0);
    CHECK_PARAMETER_STRING(1);
    int length = GetInt(args, 2, 0);
    int option = GetInt(args, 3, 0);
    if (length <= 0 || length > args[1].Length()) {
        length = args[1].Length();
    }
    std::string name = args[0].resource->TypeName();
    if (name == "TPCStream") {
        TCPStream* tcp = (TCPStream*)args[0].resource.get();
        return tcp->Send(args[1].bytes.c_str(), length, 60, option);
    } else {
        UDPConnection* udp = (UDPConnection*)args[0].resource.get();
        return udp->Send(args[1].bytes.c_str(), length, 60, option);
    }
    return Value();
}

//socket_negotiate_ssl
Value socket_negotiate_ssl(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    NOT_IMPLEMENT();
    return Value();
}
//socket_get_cert
Value socket_get_cert(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    NOT_IMPLEMENT();
    return Value();
}

Value socket_get_error(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    NOT_IMPLEMENT();
    return Value();
}

//这几个函数没有使用到，不实现
//socket_get_ssl_version
//socket_get_ssl_ciphersuite
//socket_get_ssl_session_id
//socket_cert_verify

Value join_multicast_group(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    NOT_IMPLEMENT();
    return Value();
}

Value leave_multicast_group(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    NOT_IMPLEMENT();
    return Value();
}

//get_source_port
Value get_source_port(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    NOT_IMPLEMENT();
    return Value();
}