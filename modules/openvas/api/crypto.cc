#include <openssl/hmac.h>
#include <openssl/md5.h>
#include <openssl/rsa.h>
#include <openssl/sha.h>

#include "../api.hpp"

Value Md5Buffer(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(1);
    CHECK_PARAMETER_STRING(1);
    BYTE hash[MD5_DIGEST_LENGTH] = {0};
    MD5_CTX shCtx = {0};
    MD5_Init(&shCtx);
    MD5_Update(&shCtx, args[0].bytes.c_str(), args[0].bytes.size());
    MD5_Final(hash, &shCtx);
    Value ret = Value::make_bytes("");
    ret.bytes.assign((char*)hash, MD5_DIGEST_LENGTH);
    return ret;
}

Value SHA1Buffer(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(1);
    CHECK_PARAMETER_STRING(1);
    BYTE hash[SHA_DIGEST_LENGTH] = {0};
    SHA_CTX shCtx = {0};
    SHA1_Init(&shCtx);
    SHA1_Update(&shCtx, args[0].bytes.c_str(), args[0].bytes.size());
    SHA1_Final(hash, &shCtx);
    Value ret = Value::make_bytes("");
    ret.bytes.assign((char*)hash, SHA_DIGEST_LENGTH);
    return ret;
}

Value SHA256Buffer(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(1);
    CHECK_PARAMETER_STRING(1);
    BYTE hash[SHA256_DIGEST_LENGTH] = {0};
    SHA256_CTX shCtx = {0};
    SHA256_Init(&shCtx);
    SHA256_Update(&shCtx, args[0].bytes.c_str(), args[0].bytes.size());
    SHA256_Final(hash, &shCtx);
    Value ret = Value::make_bytes("");
    ret.bytes.assign((char*)hash, SHA256_DIGEST_LENGTH);
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
    HMAC_Init_ex(hCtx, key.c_str(), key.size(), engine,NULL);
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
    return GetHMAC(args[0].bytes, args[1].bytes, args[2].bytes);
}

Value TLSPRF(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(4);
    CHECK_PARAMETER_STRING(0);
    CHECK_PARAMETER_STRING(1);
    CHECK_PARAMETER_STRING(2);
    CHECK_PARAMETER_INTEGER(3);
    return TlsPRF(args[0].bytes, args[1].bytes, args[2].bytes, args[3].Integer);
}

Value TLS1PRF(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(4);
    CHECK_PARAMETER_STRING(0);
    CHECK_PARAMETER_STRING(1);
    CHECK_PARAMETER_INTEGER(2);
    return Tls1PRF(args[0].bytes, args[1].bytes, args[2].Integer);
}