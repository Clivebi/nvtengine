#pragma once
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#endif
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

namespace net {
class Socket {
public:
    static void InitLibrary() {
        #ifdef _WIN32
        static bool isInited = FALSE;
        WSAData wsaData = {sizeof(WSAData)}; 
        if (isInited) {
            return;
        }
        WSAStartup(MAKEWORD(1, 1), &wsaData);
        isInited = TRUE;
        #endif
    }
    static void SetBlock(int socket, int on) {
     
        #ifdef _WIN32  
        unsigned long flags;
        if (on) {
            flags = 0;
        } else {
            flags = 1;
        }
        ioctlsocket(socket, FIONBIO, &flags);
        #else
        int flags = on;
        flags = fcntl(socket, F_GETFL, 0);
        if (on == 0) {
            fcntl(socket, F_SETFL, flags | O_NONBLOCK);
        } else {
            flags &= ~O_NONBLOCK;
            fcntl(socket, F_SETFL, flags);
        }
        #endif
    }
    static void Close(int fd) {
        if (fd == -1) {
            return;
        }
#ifdef _WIN32
        shutdown(fd, SD_BOTH);
        closesocket(fd);
#else
        shutdown(fd, SHUT_RDWR);
        close(fd);
#endif
    }
    // >0 for success
    static int WaitSocketAvaliable(int fd, int timeout_sec, bool read) {
        if (timeout_sec > 0) {
            fd_set fdset;
            struct timeval timeout;
            timeout.tv_usec = 0;
            timeout.tv_sec = timeout_sec;
            FD_ZERO(&fdset);
            FD_SET(fd, &fdset);
            if (read) {
                return select(fd + 1, &fdset, NULL, NULL, &timeout) > 0;
            }
            return select(fd + 1, NULL, &fdset, NULL, &timeout) > 0;
        }
        return true;
    }

    static int ConnectWithTimeout(int sockfd, const unsigned char* addr, int addr_len,
                                  int timeout) {
        int error = -1;
        int set = 1;
        #ifndef _WIN32
        setsockopt(sockfd, SOL_SOCKET, SO_NOSIGPIPE, (void*)&set, sizeof(int));
        #endif
        error = connect(sockfd, (struct sockaddr*)addr, addr_len);
        if (error == 0) {
            return 0;
        }
        #ifdef _WIN32
        if (error == -1 && WSAGetLastError() != WSAEWOULDBLOCK) {
            LOG_ERROR("connect error :", WSAGetLastError());
            return error;
        }
        #else
        if (error == -1 && errno != EINPROGRESS) {
            LOG_ERROR("connect error :", errno);
            return error;
        }
        #endif
        fd_set fdwrite;
        struct timeval tvSelect;
        FD_ZERO(&fdwrite);
        FD_SET(sockfd, &fdwrite);
        tvSelect.tv_sec = timeout;
        tvSelect.tv_usec = 0;

        int retval = select(sockfd + 1, NULL, &fdwrite, NULL, &tvSelect);
        if (retval < 0) {
            return -2;
        } else if (retval == 0) { //timeout
            return -3;
        } else {
            #ifdef _WIN32
            int error = 0;
            int errlen = sizeof(error);
            getsockopt(sockfd, SOL_SOCKET, SO_ERROR, (char*) & error, &errlen);
            #else
            int error = 0;
            int errlen = sizeof(error);
            getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, (socklen_t*)&errlen);
            #endif
            if (error != 0) {
                return -4; //connect fail
            }
            return 0;
        }
    }

    static int DialTCP(const char* host, const char* port, int timeout_sec) {
        struct addrinfo hints, *servinfo, *p;
        int rv, sockfd;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        InitLibrary();
        if ((rv = getaddrinfo(host, port, &hints, &servinfo)) != 0) {
            return rv;
        }

        for (p = servinfo; p != NULL; p = p->ai_next) {
            if ((sockfd = (int)socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
                continue;
            }
            SetBlock(sockfd, 0);
            if (ConnectWithTimeout(sockfd, (unsigned char*)p->ai_addr, (int)p->ai_addrlen,
                                   timeout_sec) == -1) {
                Close(sockfd);
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

    static int DialUDP(const char* host, const char* port) {
        struct addrinfo hints, *servinfo, *p;
        int rv, sockfd;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_DGRAM;
        InitLibrary();
        if ((rv = getaddrinfo(host, port, &hints, &servinfo)) != 0) {
            return rv;
        }

        for (p = servinfo; p != NULL; p = p->ai_next) {
            if ((sockfd = (int)socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
                continue;
            }
            SetBlock(sockfd, 0);
            if (connect(sockfd, (sockaddr*)p->ai_addr,(int) p->ai_addrlen)) {
                Close(sockfd);
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
};

} // namespace net
