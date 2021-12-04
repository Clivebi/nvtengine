#pragma once
#include "con.hpp"
namespace net {
/*
    @brief dial tcp or udp
    @param proto  tcp or udp
    @param host  dest ip or host name
    @param port  dest port
    @param timeout connect timeout (only tcp )
    @param tls  use tls or not (only tcp implement)
    @param enableCache enable read cache to reduce syscall
    */
Conn* Dial(std::string proto, std::string host, std::string port, int timeout, bool tls,
           bool enableCache);
} // namespace net