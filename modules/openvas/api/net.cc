#include "../../net/con.hpp"
#include "../api.hpp"

#define NOT_IMPLEMENT() throw RuntimeException("Not Implement:" + std::string(__FUNCTION__))

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
    std::string data = "";
    unsigned char* buffer = new u_char[length];
    if (!net::Conn::IsConn(args[0].resource)) {
        throw RuntimeException("recv resource type mismatch");
    }
    net::Conn* con = (net::Conn*)args[0].resource.get();
    con->SetReadTimeout(timeout);
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
    CHECK_PARAMETER_STRING(1);
    int length = GetInt(args, 2, 0);
    int option = GetInt(args, 3, 0);
    if (length <= 0 || length > args[1].Length()) {
        length = args[1].Length();
    }
    if (!net::Conn::IsConn(args[0].resource)) {
        throw RuntimeException("recv resource type mismatch");
    }
    net::Conn* con = (net::Conn*)args[0].resource.get();
    return con->Write(args[1].bytes.c_str(), length);
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
    bzero(&hints, sizeof(hints));
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
    bzero(&hints, sizeof(hints));
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
    CHECK_PARAMETER_STRING(1);
    return ResolveHostName(args[0].bytes, AF_UNSPEC);
}

Value ResolveHostNameToList(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(0);
    CHECK_PARAMETER_STRING(1);
    std::vector<std::string> res = ResolveHostNameToList(args[0].bytes, AF_UNSPEC);
    if (res.size() == 0) {
        return Value();
    }
    Value ret = Value::make_array();
    for (auto v : res) {
        ret._array().push_back(v);
    }
    return ret;
}
