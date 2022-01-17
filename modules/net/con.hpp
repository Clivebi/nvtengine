#pragma once
#include <errno.h>

#include "../io.hpp"
#include "socket.hpp"

namespace net {
class BaseConn : public ReadWriter {
protected:
    int mReadTimeout;
    int mWriteTimeout;
    int mSocket;
    int mLastError;
    std::string mName;
    std::map<std::string, void*> mUserData;

    void UpdateLastError() {
#ifdef _WIN32
        mLastError = WSAGetLastError();
#else
        mLastError = errno;
#endif
    }

public:
    BaseConn(int Socket, int ReadTimeout, int WriteTimeout, std::string name)
            : mReadTimeout(ReadTimeout),
              mWriteTimeout(WriteTimeout),
              mSocket(Socket),
              mName(name),
              mLastError(0) {}
    virtual int GetFD() { return mSocket; }
    virtual int GetLastError() { return mLastError; }
    virtual ~BaseConn() { Close(); }
    void SetUserData(std::string key, void* data) { mUserData[key] = data; }
    void* GetUserData(std::string key) {
        auto iter = mUserData.find(key);
        if (iter != mUserData.end()) {
            return iter->second;
        }
        return NULL;
    }
    virtual void SetReadTimeout(int timeout) { mReadTimeout = timeout; }
    virtual void SetWriteTimeout(int timeout) { mWriteTimeout = timeout; }
    bool IsAvaliable() { return mSocket != -1; }
    virtual void Close() {
        if (mSocket != -1) {
            Socket::Close(mSocket);
            mSocket = -1;
        }
    }
    virtual int Read(void* buffer, int size) {
        if (!Socket::WaitSocketAvaliable(mSocket, mReadTimeout, true)) {
            UpdateLastError();
            LOG_DEBUG("BaseConn read timeout:", mReadTimeout);
            return -1;
        }
        #ifdef _WIN32
        int nSize = ::recv(mSocket, (char*)buffer, size, 0);
        #else
        int nSize = ::recv(mSocket, buffer, size, 0);
        #endif
        if (nSize == -1) {
            UpdateLastError();
            LOG_DEBUG("BaseConn recv error ", mLastError);
        }
        return nSize;
    }

    virtual int Write(const void* buffer, int size) {
        if (!Socket::WaitSocketAvaliable(mSocket, mWriteTimeout, false)) {
            LOG_DEBUG("BaseConn write timeout:", mReadTimeout);
            UpdateLastError();
            return -1;
        }
        #ifdef _WIN32
        int nSize = ::send(mSocket, (char*)buffer, size, 0);
        #else
        int nSize = ::send(mSocket, buffer, size, 0);
        #endif
        if (nSize == -1) {
            UpdateLastError();
            LOG_DEBUG("BaseConn recv error ", mLastError);
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