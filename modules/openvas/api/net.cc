#include "../../net/con.hpp"
#include "../api.hpp"

Value open_priv_sock_tcp(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    NOT_IMPLEMENT();
    return Value();
}

Value open_priv_sock_udp(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    NOT_IMPLEMENT();
    return Value();
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
    if(length == 0){
        return Value("");
    }
    std::string data = "";
    unsigned char* buffer = new u_char[length];
    if (!net::Conn::IsConn(args[0].resource)) {
        throw RuntimeException("recv resource type mismatch");
    }
    net::Conn* con = (net::Conn*)args[0].resource.get();
    //con->SetReadTimeout(timeout);
    int size = ReadAtleast(con, buffer, length, minSize);
    if (size <= 0) {
        delete[] buffer;
        return Value();
    }
    data.assign((char*)buffer, size);
    delete[] buffer;
    return data;
}

//socket,length,timeout
//Value recv_line(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
//    NOT_IMPLEMENT();
//   return Value();
//}

//socket,data,length,option
Value send(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(2);
    CHECK_PARAMETER_RESOURCE(0);
    CHECK_PARAMETER_STRING_OR_BYTES(1);
    std::string data = GetString(args, 1);
    int length = GetInt(args, 2, 0);
    int option = GetInt(args, 3, 0);
    if (length <= 0 || length > (int)args[1].Length()) {
        length = (int)args[1].Length();
    }
    if (!net::Conn::IsConn(args[0].resource)) {
        throw RuntimeException("recv resource type mismatch");
    }
    //LOG_DEBUG("send:", length, HexEncode(args[1].bytes.c_str(), length, "\\x"));
    net::Conn* con = (net::Conn*)args[0].resource.get();
    return con->Write(data.c_str(), length);
}

Value socket_get_error(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(1);
    CHECK_PARAMETER_RESOURCE(0);
    if (!net::Conn::IsConn(args[0].resource)) {
        throw RuntimeException("get_source_port resource type mismatch");
    }
    net::Conn* con = (net::Conn*)args[0].resource.get();
    int err = con->GetBaseConn()->GetLastError();

    switch (err) {
    case 0:
        return 0;
    case ETIMEDOUT:
        return 1;
    case EBADF:
    case EPIPE:
    case ECONNRESET:
    case ENOTSOCK:
        return 2;
    case ENETUNREACH:
    case EHOSTUNREACH:
        return 3;
    }

    return err;
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
    CHECK_PARAMETER_COUNT(1);
    CHECK_PARAMETER_RESOURCE(0);
    if (!net::Conn::IsConn(args[0].resource)) {
        throw RuntimeException("get_source_port resource type mismatch");
    }
    net::Conn* con = (net::Conn*)args[0].resource.get();
    struct sockaddr_in ia;
    socklen_t l = sizeof(ia);
    if (getsockname(con->GetBaseConn()->GetFD(), (struct sockaddr*)&ia, &l) < 0) {
        return Value();
    }
    return Value((long)ia.sin_port);
}

std::string SockAddressToString(struct sockaddr* addr) {
    char buffer[128] = {0};
    if (addr == NULL) {
        return "";
    }
    switch (addr->sa_family) {
    case AF_INET: {
        struct sockaddr_in* saddr = (struct sockaddr_in*)addr;
        inet_ntop(AF_INET, &saddr->sin_addr, buffer, INET6_ADDRSTRLEN);
    } break;
    case AF_INET6: {
        struct sockaddr_in6* s6addr = (struct sockaddr_in6*)addr;
        if (IN6_IS_ADDR_V4MAPPED(&s6addr->sin6_addr))
            inet_ntop(AF_INET, &s6addr->sin6_addr.s6_addr[12], buffer, INET6_ADDRSTRLEN);
        else
            inet_ntop(AF_INET6, &s6addr->sin6_addr, buffer, INET6_ADDRSTRLEN);
    } break;

    default:
        break;
    }
    return buffer;
}

std::string ResolveHostName(const std::string& host, int family) {
    std::string result = "";
    struct addrinfo hints, *info, *p;
    memset(&hints,0, sizeof(hints));
    hints.ai_family = family;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = 0;
    if ((getaddrinfo(host.c_str(), NULL, &hints, &info)) != 0) {
        return result;
    }
    p = info;
    while (p) {
        if (p->ai_family == family || family == AF_UNSPEC) {
            result = SockAddressToString(p->ai_addr);
            break;
        }
        p = p->ai_next;
    }
    freeaddrinfo(info);
    return result;
}

std::vector<std::string> ResolveHostNameToList(const std::string& host, int family) {
    std::vector<std::string> result;
    struct addrinfo hints, *info, *p;
    memset(&hints,0, sizeof(hints));
    hints.ai_family = family;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = 0;
    if ((getaddrinfo(host.c_str(), NULL, &hints, &info)) != 0) {
        return result;
    }
    p = info;
    while (p) {
        if (p->ai_family == family || family == AF_UNSPEC) {
            result.push_back(SockAddressToString(p->ai_addr));
        }
        p = p->ai_next;
    }
    freeaddrinfo(info);
    return result;
}

Value ResolveHostName(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(0);
    CHECK_PARAMETER_STRING(0);
    return ResolveHostName(args[0].text, AF_UNSPEC);
}

Value ResolveHostNameToList(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(1);
    CHECK_PARAMETER_STRING(0);
    std::vector<std::string> res = ResolveHostNameToList(args[0].text, AF_UNSPEC);
    if (res.size() == 0) {
        return Value();
    }
    Value ret = Value::MakeArray();
    for (auto v : res) {
        ret._array().push_back(v);
    }
    return ret;
}
