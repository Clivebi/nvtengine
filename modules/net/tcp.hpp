#pragma once
#include "con.hpp"
#include "sslhelper.hpp"
namespace net {
class TCPConn : public BaseConn {
public:
    SSL_CTX* mSSLContext;
    SSL* mSSL;

public:
    explicit TCPConn(int Socket) : BaseConn(Socket, 20, 20, "tcp") {
        mSSLContext = NULL;
        mSSL = NULL;
    }
    ~TCPConn() { Close(); }
    void Close() {
        BaseConn::Close();
        if (mSSL) {
            SSL_free(mSSL);
            SSL_CTX_free(mSSLContext);
            mSSLContext = NULL;
            mSSL = NULL;
        }
    }
    int Read(void* buffer, int size) {
        int n = 0;
        if (size <= 0) {
            return 0;
        }
        if (!mSSL) {
            return BaseConn::Read(buffer, size);
        }
        while ((n = SSL_read(mSSL, buffer, size)) < 0) {
            int err = SSL_get_error(mSSL, n);
            if (err == SSL_ERROR_WANT_READ) {
                if (!Socket::WaitSocketAvaliable(mSocket, mReadTimeout, true)) {
                    NVT_LOG_DEBUG("SSL read timeout....");
                    UpdateLastError();
                    return -1;
                }
                continue;
            }
            if (err == SSL_ERROR_WANT_WRITE) {
                if (!Socket::WaitSocketAvaliable(mSocket, mWriteTimeout, false)) {
                    NVT_LOG_DEBUG("SSL Write timeout....");
                    UpdateLastError();
                    return -1;
                }
                continue;
            }
            UpdateLastError();
            break;
        }
        return n;
    }

    int Write(const void* buffer, int size) {
        int n = 0;
        if (size <= 0) {
            return 0;
        }
        if (!mSSL) {
            return BaseConn::Write(buffer, size);
        }
        while ((n = SSL_write(mSSL, buffer, size)) < 0) {
            int err = SSL_get_error(mSSL, n);
            if (err == SSL_ERROR_WANT_READ) {
                if (!Socket::WaitSocketAvaliable(mSocket, mReadTimeout, true)) {
                    NVT_LOG_DEBUG("SSL read timeout....");
                    UpdateLastError();
                    return -1;
                }
                continue;
            }
            if (err == SSL_ERROR_WANT_WRITE) {
                if (!Socket::WaitSocketAvaliable(mSocket, mWriteTimeout, false)) {
                    NVT_LOG_DEBUG("SSL Write timeout....");
                    UpdateLastError();
                    return -1;
                }
                continue;
            }
            UpdateLastError();
            break;
        }
        return n;
    }

    scoped_refptr<Resource> GetPeerCert() {
        if (mSSL) {
            X509* ptr = SSL_get_peer_certificate(mSSL);
            Resource* res = new OpenSSLHelper::SSLObject<X509>(ptr, X509_free);
            return res;
        }
        return NULL;
    }

    bool SSLHandshake() {
        int err = 0;
        if (mSSL != NULL) {
            return true;
        }
        time_t start = time(NULL);
        mSSLContext = SSL_CTX_new(SSLv23_method());
        mSSL = SSL_new(mSSLContext);
        SSL_set_fd(mSSL, mSocket);
        SSL_set_connect_state(mSSL);
        while ((err = SSL_do_handshake(mSSL)) != 1) {
            err = SSL_get_error(mSSL, err);
            if (err == SSL_ERROR_WANT_WRITE) {
                if (!Socket::WaitSocketAvaliable(mSocket, mWriteTimeout, false)) {
                    NVT_LOG_DEBUG("select write timeout");
                    UpdateLastError();
                    return false;
                }
                continue;
            } else if (err == SSL_ERROR_WANT_READ) {
                if (!Socket::WaitSocketAvaliable(mSocket, mWriteTimeout, true)) {
                    NVT_LOG_DEBUG("select read timeout");
                    UpdateLastError();
                    return false;
                }
                continue;
            } else {
                NVT_LOG_DEBUG("SSL_do_handshake error " + ToString(err));
                //ERR_print_errors_fp(stdout);
                UpdateLastError();
                return false;
            }
        }
        return true;
    }
};
} // namespace net
