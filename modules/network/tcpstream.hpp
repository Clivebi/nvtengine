#pragma once
#include <openssl/crypto.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <string.h>

#include "../../engine/value.hpp"
#include "socket.hpp"

using namespace Interpreter;

namespace OpenSSLHelper {
template <class T>
class SSLObject : public Resource {
    using free_routine = void (*)(T*);

protected:
    T* mRaw;
    free_routine mFree;
    DISALLOW_COPY_AND_ASSIGN(SSLObject<T>);

public:
    SSLObject(T* t, free_routine release) : mRaw(t), mFree(release) {}
    ~SSLObject() {
        Close();
        mRaw = NULL;
    }
    operator T*() { return mRaw; }

    virtual void Close() {
        if (mRaw != NULL) {
            mFree(mRaw);
            mRaw = NULL;
        }
    };
    virtual bool IsAvaliable() { return mRaw != NULL; }
    virtual std::string TypeName() { return "SSLObject"; }
};

scoped_refptr<SSLObject<X509>> LoadX509FromBuffer(std::string& buffer, std::string& password,
                                                  int type);

scoped_refptr<SSLObject<EVP_PKEY>> LoadPrivateKeyFromBuffer(std::string& buffer,
                                                            std::string& password, int type);
} // namespace OpenSSLHelper
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

    int Recv(void* buf, int size, int timeout = 60, int option = 0) {
        if (!Socket::WaitSocketAvaliable(mSocket, timeout, true)) {
            return -1;
        }
        if (mSSL) {
            return SSL_read(mSSL, buf, size);
        }
        return recv(mSocket, buf, size, option);
    }

    int Send(const void* buf, int size, int timeout = 60, int option = 0) {
        if (!Socket::WaitSocketAvaliable(mSocket, timeout, false)) {
            return -1;
        }
        if (mSSL) {
            return SSL_write(mSSL, buf, size);
        }
        return send(mSocket, buf, size, option);
    }

    scoped_refptr<Resource> GetPeerCert() {
        if (mSSL) {
            X509* ptr = SSL_get_peer_certificate(mSSL);
            Resource* res = new OpenSSLHelper::SSLObject<X509>(ptr, X509_free);
            return res;
        }
        return NULL;
    }

    std::string TypeName() { return "TCPStream"; }
};

TCPStream* NewTCPStream(std::string& host, std::string& port, int timeout_sec, bool isSSL);