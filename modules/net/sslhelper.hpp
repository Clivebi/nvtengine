#pragma once
#include <openssl/crypto.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <string.h>

#include "../../engine/value.hpp"
#include "../io.hpp"
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
    explicit SSLObject(T* t, free_routine release) : mRaw(t), mFree(release) {}
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
