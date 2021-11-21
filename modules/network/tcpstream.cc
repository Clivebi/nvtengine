#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <openssl/crypto.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <string.h>
#include <sys/socket.h>

#define TCP_IMPL 1
#include "../../engine/value.hpp"

using namespace Interpreter;
class TCPStream : public Interpreter::Resource {
public:
    int mSocket;
    SSL_CTX* mSSLContext;
    SSL* mSSL;

public:
    explicit TCPStream(int fd, bool bSSL) {
        mSocket = fd;
        mSSLContext = NULL;
        mSSL = NULL;
        if (bSSL) {
            mSSLContext = SSL_CTX_new(SSLv23_method());
            mSSL = SSL_new(mSSLContext);
            SSL_set_fd(mSSL, mSocket);
        }
    }
    void Close() {
        if (mSSL) {
            SSL_free(mSSL);
            SSL_CTX_free(mSSLContext);
            mSSLContext = NULL;
            mSSL = NULL;
        }
        if (mSocket != -1) {
            shutdown(mSocket, SHUT_RDWR);
            mSocket = -1;
        }
    }
    bool IsAvaliable() { return mSocket != -1; }

    int Recv(void* buf, int size) {
        if (mSSL) {
            return SSL_read(mSSL, buf, size);
        }
        return recv(mSocket, buf, size, 0);
    }

    int Send(const void* buf, int size) {
        if (mSSL) {
            return SSL_write(mSSL, buf, size);
        }
        return send(mSocket, buf, size, 0);
    }
    std::string TypeName() { return "TCPStream"; }
};

int open_connection(const char* host, const char* port, int timeout_sec) {
    struct addrinfo hints, *servinfo, *p;
    int rv, sockfd;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    if ((rv = getaddrinfo(host, port, &hints, &servinfo)) != 0) {
        return rv;
    }

    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            continue;
        }
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            shutdown(sockfd, SHUT_RDWR);
            continue;
        }
        break;
    }
    freeaddrinfo(servinfo);
    if (p == NULL) {
        return -1;
    }
    return sockfd;
}

TCPStream* NewTCPStream(std::string& host, std::string& port, int timeout_sec, bool isSSL) {
    int sockfd = open_connection(host.c_str(), port.c_str(), timeout_sec);
    if (sockfd < 0) {
        return NULL;
    }
    if (!isSSL) {
        return new TCPStream(sockfd, false);
    }
    TCPStream* stream = new TCPStream(sockfd, true);
    if (SSL_connect(stream->mSSL) < 0) {
        delete stream;
        return NULL;
    }
    //SSL_get_peer_certificate copy x509
    return stream;
}