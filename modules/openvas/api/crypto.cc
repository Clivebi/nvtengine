#include <openssl/hmac.h>
#include <openssl/md5.h>
#include <openssl/rsa.h>
#include <openssl/sha.h>

#include "../api.hpp"

Value Md5Buffer(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(1);
    CHECK_PARAMETER_STRING_OR_BYTES(0);
    std::string p0 = GetString(args, 0);
    BYTE hash[MD5_DIGEST_LENGTH] = {0};
    MD5_CTX shCtx = {0};
    MD5_Init(&shCtx);
    MD5_Update(&shCtx, p0.c_str(), p0.size());
    MD5_Final(hash, &shCtx);
    Value ret = Value::make_bytes(sizeof(hash));
    ret.bytesView.CopyFrom(hash, sizeof(hash));
    return ret;
}

Value SHA1Buffer(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(1);
    CHECK_PARAMETER_STRING_OR_BYTES(0);
    std::string p0 = GetString(args, 0);
    BYTE hash[SHA_DIGEST_LENGTH] = {0};
    SHA_CTX shCtx = {0};
    SHA1_Init(&shCtx);
    SHA1_Update(&shCtx, args[0].text.c_str(), args[0].text.size());
    SHA1_Final(hash, &shCtx);
    Value ret = Value::make_bytes(sizeof(hash));
    ret.bytesView.CopyFrom(hash, sizeof(hash));
    return ret;
}

Value SHA256Buffer(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(1);
    CHECK_PARAMETER_STRING_OR_BYTES(0);
    std::string p0 = GetString(args, 0);
    BYTE hash[SHA256_DIGEST_LENGTH] = {0};
    SHA256_CTX shCtx = {0};
    SHA256_Init(&shCtx);
    SHA256_Update(&shCtx, p0.c_str(), p0.size());
    SHA256_Final(hash, &shCtx);
    Value ret = Value::make_bytes(sizeof(hash));
    ret.bytesView.CopyFrom(hash, sizeof(hash));
    return ret;
}

std::string GetHMAC(std::string hashMethod, const std::string& key, const std::string input) {
    const EVP_MD* engine = NULL;
    BYTE buffer[EVP_MAX_MD_SIZE] = {0};
    unsigned int finalSize = EVP_MAX_MD_SIZE;
    if (hashMethod == "sha512") {
        engine = EVP_sha512();
    } else if (hashMethod == "sha256") {
        engine = EVP_sha256();
    } else if (hashMethod == "sha224") {
        engine = EVP_sha224();
    } else if (hashMethod == "sha384") {
        engine = EVP_sha384();
    } else if (hashMethod == "sha1") {
        engine = EVP_sha1();
    } else if (hashMethod == "sha") {
        engine = EVP_sha1();
    } else if (hashMethod == "md5") {
        engine = EVP_md5();
    } else {
        return "";
    }
    HMAC_CTX* hCtx = HMAC_CTX_new();
    HMAC_Init_ex(hCtx, key.c_str(), key.size(), engine, NULL);
    HMAC_Update(hCtx, (BYTE*)input.c_str(), input.size());
    HMAC_Final(hCtx, buffer, &finalSize);
    HMAC_CTX_free(hCtx);
    std::string ret;
    ret.assign((char*)buffer, finalSize);
    return ret;
}

std::string TlsPRF(const std::string& secret, const std::string& seed, std::string hashMethod,
                   size_t needSize) {
    std::string ret = "";
    std::string Ai = seed; //A0
    std::string input = "";
    do {
        Ai = GetHMAC(hashMethod, secret, Ai); // A(i) -->hmac(secret,A(i-1))
        input = Ai;
        input += seed;
        ret += GetHMAC(hashMethod, secret, input); // result = hmac(secret,A(1)+seed)+
    } while (ret.size() < needSize);               //          hmac(secret,A(2)+seed)+...
    return ret.substr(0, needSize);
}

std::string Tls1PRF(const std::string& secret, const std::string& seed, size_t needSize) {
    std::string secret0, secret1;
    if (secret.size() % 2 == 0) {
        secret0 = secret.substr(0, secret.size() / 2);
        secret1 = secret.substr(secret.size() / 2);
    } else {
        secret0 = secret.substr(0, (secret.size() + 1) / 2);
        secret1 = secret.substr((secret.size() + 1) / 2 - 1);
    }
    std::string part0 = TlsPRF(secret0, seed, "md5", needSize);
    std::string part1 = TlsPRF(secret1, seed, "sha1", needSize);
    for (int i = 0; i < needSize; i++) {
        BYTE c = (BYTE)part0[i];
        BYTE c1 = (BYTE)part1[i];
        c ^= c1;
        part0[i] = (char)c;
    }
    return part0;
}

Value HMACMethod(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(3);
    CHECK_PARAMETER_STRING(0);
    CHECK_PARAMETER_STRING(1);
    CHECK_PARAMETER_STRING(2);
    return GetHMAC(args[0].text, args[1].text, args[2].text);
}

Value TLSPRF(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(4);
    CHECK_PARAMETER_STRING(0);
    CHECK_PARAMETER_STRING(1);
    CHECK_PARAMETER_STRING(2);
    CHECK_PARAMETER_INTEGER(3);
    return TlsPRF(args[0].text, args[1].text, args[2].text, args[3].Integer);
}

Value TLS1PRF(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(4);
    CHECK_PARAMETER_STRING(0);
    CHECK_PARAMETER_STRING(1);
    CHECK_PARAMETER_INTEGER(2);
    return Tls1PRF(args[0].text, args[1].text, args[2].Integer);
}

class SSLCipher : public Resource {
protected:
    EVP_CIPHER_CTX* mCtx;

public:
    explicit SSLCipher(const EVP_CIPHER* cipher, int pading, const std::string& key,
                       const std::string& iv, int ForEnc) {
        mCtx = EVP_CIPHER_CTX_new();
        EVP_CipherInit(mCtx, cipher, (BYTE*)key.c_str(), (BYTE*)iv.c_str(), ForEnc);
        EVP_CIPHER_CTX_set_padding(mCtx, pading);
    }
    std::string Update(const std::string& data) {
        BYTE* out = new BYTE[data.size() + EVP_MAX_BLOCK_LENGTH];
        int outSize = data.size() + EVP_MAX_BLOCK_LENGTH;
        EVP_CipherUpdate(mCtx, out, &outSize, (const BYTE*)data.c_str(), data.size());
        std::string ret = "";
        ret.assign((char*)out, outSize);
        delete[] out;
        return ret;
    }
    std::string Final() {
        BYTE out[EVP_MAX_BLOCK_LENGTH];
        int outSize = EVP_MAX_BLOCK_LENGTH;
        EVP_CipherFinal(mCtx, out, &outSize);
        std::string ret = "";
        ret.assign((char*)out, outSize);
        return ret;
    }
    virtual void Close() {
        if (mCtx) {
            EVP_CIPHER_CTX_free(mCtx);
            mCtx = NULL;
        }
    };
    virtual bool IsAvaliable() { return mCtx != NULL; }
    virtual std::string TypeName() { return "Cipher"; };
};

/*
#define EVP_PADDING_PKCS7       1
#define EVP_PADDING_ISO7816_4   2
#define EVP_PADDING_ANSI923     3
#define EVP_PADDING_ISO10126    4
#define EVP_PADDING_ZERO        5
*/

Value CipherOpen(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(5);
    CHECK_PARAMETER_STRING(0);
    CHECK_PARAMETER_INTEGER(1);
    CHECK_PARAMETER_STRING_OR_BYTES(2);
    CHECK_PARAMETER_STRING_OR_BYTES(3);
    CHECK_PARAMETER_INTEGER(4);
    OpenSSL_add_all_algorithms();
    std::string cipherName = GetString(args, 0);
    std::string key = GetString(args, 2);
    std::string iv = GetString(args, 3);
    int padding = GetInt(args, 1, 1);
    const EVP_CIPHER* cipher = EVP_get_cipherbyname(cipherName.c_str());
    if (cipher == NULL) {
        LOG("invalid chiper name ", cipherName);
        return Value();
    }
    if (padding < 0 || padding > 5) {
        LOG("invalid chiper name ", cipherName);
        return Value();
    }
    if (EVP_CIPHER_key_length(cipher) > key.size()) {
        LOG("invalid key size  ");
        return Value();
    }
    if (EVP_CIPHER_iv_length(cipher) > iv.size()) {
        LOG("invalid iv size  ");
        return Value();
    }
    return Value((Resource*)new SSLCipher(cipher, padding, key, iv, args[4].Integer));
}

Value CipherUpdate(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(2);
    CHECK_PARAMETER_STRING_OR_BYTES(1);
    std::string resname;
    if (!args[0].IsResource(resname) || resname != "Cipher") {
        return Value();
    }
    SSLCipher* cipher = (SSLCipher*)args[0].resource.get();
    return cipher->Update(GetString(args, 1));
}

Value CipherFinal(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(1);
    std::string resname;
    if (!args[0].IsResource(resname) || resname != "Cipher") {
        return Value();
    }
    SSLCipher* cipher = (SSLCipher*)args[0].resource.get();
    return cipher->Final();
}