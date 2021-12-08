#pragma once
#include "../io.hpp"
#include "socket.hpp"

namespace net {
class BaseConn : public ReadWriter {
protected:
    int mReadTimeout;
    int mWriteTimeout;
    int mSocket;
    std::string mName;

public:
    BaseConn(int Socket, int ReadTimeout, int WriteTimeout, std::string name)
            : mReadTimeout(ReadTimeout),
              mWriteTimeout(WriteTimeout),
              mSocket(Socket),
              mName(name) {}
    virtual ~BaseConn() { Close(); }
    virtual void SetReadTimeout(int timeout) { mReadTimeout = timeout; }
    virtual void SetWriteTimeout(int timeout) { mWriteTimeout = timeout; }
    bool IsAvaliable() { return mSocket != -1; }
    virtual void Close() {
        if (mSocket != -1) {
            shutdown(mSocket, SHUT_RDWR);
            mSocket = -1;
        }
    }
    virtual int Read(void* buffer, int size) {
        if (!Socket::WaitSocketAvaliable(mSocket, mReadTimeout, true)) {
            LOG("Wait read timeout:", mReadTimeout);
            return -1;
        }
        int nSize = ::recv(mSocket, buffer, size, 0);
        if (nSize == -1) {
            LOG("recv error ", errno);
        }
        return nSize;
    }

    virtual int Write(const void* buffer, int size) {
        if (!Socket::WaitSocketAvaliable(mSocket, mWriteTimeout, false)) {
            LOG("Wait write timeout:", mReadTimeout);
            return -1;
        }
        int nSize = ::send(mSocket, buffer, size, 0);
        if (nSize == -1) {
            LOG("send error ", errno);
        }
        return nSize;
    }

    virtual std::string TypeName() { return mName; }
};

class Conn : public Interpreter::Resource, public ReadWriter {
protected:
    BufferedReader* mReader;
    BaseConn* mCon;

public:
    explicit Conn(BaseConn* con, bool bufferedReader) {
        mCon = con;
        mReader = NULL;
        if (bufferedReader) {
            mReader = new BufferedReader(con);
        }
    }

    BaseConn* GetBaseConn() { return mCon; }

    ~Conn() {
        delete mCon;
        delete mReader;
    }

    void SetReadTimeout(int timeout) { mCon->SetReadTimeout(timeout); }
    void SetWriteTimeout(int timeout) { mCon->SetWriteTimeout(timeout); }

    int Read(void* buffer, int size) {
        if (mReader) {
            return mReader->Read(buffer, size);
        }
        return mCon->Read(buffer, size);
    }

    int Write(const void* buffer, int size) { return mCon->Write(buffer, size); }

    std::string TypeName() { return mCon->TypeName(); }

    static bool IsConn(Interpreter::Resource* res) {
        return res->TypeName() == "tcp" || res->TypeName() == "udp";
    }

    bool IsAvaliable() { return mCon->IsAvaliable(); }
    void Close() { mCon->Close(); }

private:
    DISALLOW_COPY_AND_ASSIGN(Conn);
};

}; // namespace net