#include "dial.hpp"

#include "tcp.hpp"

namespace net {

Conn* Dial(std::string proto, std::string host, std::string port, int timeout, bool tls,
           bool enableCache) {
    if (proto == "tcp") {
        int socket = Socket::DialTCP(host.c_str(), port.c_str(), timeout);
        if (socket <= 0) {
            //NVT_LOG_ERROR("DialTCP error :" + host + ":" + port, "timeout =", timeout);
            return NULL;
        }
        TCPConn* con = new TCPConn(socket);
        if (tls) {
            if (!con->SSLHandshake()) {
                delete con;
                //NVT_LOG_ERROR("SSLHandshake error :" + host + ":" + port, " timeout =", timeout);
                return NULL;
            }
        }
        return new Conn(con, enableCache);
    }
    if (proto == "udp") {
        int socket = Socket::DialUDP(host.c_str(), port.c_str());
        if (socket <= 0) {
            return NULL;
        }
        BaseConn* con = new BaseConn(socket, 60, 60, "udp");
        return new Conn(con, false);
    }
    return NULL;
}
} // namespace net
