#pragma once
#ifdef WIN32
#include <winsocks.h>
#else
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#endif
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

class Socket {
public:
    static void SetBlock(int socket, int on) {
        int flags;
        flags = fcntl(socket, F_GETFL, 0);
        if (on == 0) {
            fcntl(socket, F_SETFL, flags | O_NONBLOCK);
        } else {
            flags &= ~O_NONBLOCK;
            fcntl(socket, F_SETFL, flags);
        }
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
        SetBlock(sockfd, 0);
        error = connect(sockfd, (struct sockaddr*)addr, addr_len);
        if (error == 0) {
            return 0;
        }
        if (error == -1 && errno != EINPROGRESS) {
            return error;
        }
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
            int error = 0;
            int errlen = sizeof(error);
            getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, (socklen_t*)&errlen);
            if (error != 0) {
                return -4; //connect fail
            }

            int set = 1;
            setsockopt(sockfd, SOL_SOCKET, SO_NOSIGPIPE, (void*)&set, sizeof(int));
            return 0;
        }
    }

    static int OpenTCPConnection(const char* host, const char* port, int timeout_sec) {
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
            if (ConnectWithTimeout(sockfd, (unsigned char*)p->ai_addr, p->ai_addrlen,
                                   timeout_sec) == -1) {
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

    static int OpenUDPConnection(const char* host, const char* port) {
        struct addrinfo hints, *servinfo, *p;
        int rv, sockfd;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_DGRAM;
        if ((rv = getaddrinfo(host, port, &hints, &servinfo)) != 0) {
            return rv;
        }

        for (p = servinfo; p != NULL; p = p->ai_next) {
            if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
                continue;
            }
            if (connect(sockfd, (sockaddr*)p->ai_addr, p->ai_addrlen)) {
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
};