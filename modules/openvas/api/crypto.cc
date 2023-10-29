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
    unsigned int hashSize = MD5_DIGEST_LENGTH;
    EVP_MD_CTX* shCtx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(shCtx, EVP_md5(), NULL);
    EVP_DigestUpdate(shCtx, p0.c_str(), p0.size());
    EVP_DigestFinal_ex(shCtx,hash,&hashSize);
    EVP_MD_CTX_free(shCtx);
    std::string ret = "";
    ret.append((char*)hash, sizeof(hash));
    return ret;
}

Value SHA1Buffer(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(1);
    CHECK_PARAMETER_STRING_OR_BYTES(0);
    std::string p0 = GetString(args, 0);
    BYTE hash[SHA_DIGEST_LENGTH] = {0};
    unsigned int hashSize = SHA_DIGEST_LENGTH;
    EVP_MD_CTX* shCtx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(shCtx, EVP_sha1(), NULL);
    EVP_DigestUpdate(shCtx, p0.c_str(), p0.size());
    EVP_DigestFinal_ex(shCtx,hash,&hashSize);
    EVP_MD_CTX_free(shCtx);
    std::string ret = "";
    ret.append((char*)hash, sizeof(hash));
    return ret;
}

Value SHA256Buffer(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(1);
    CHECK_PARAMETER_STRING_OR_BYTES(0);
    std::string p0 = GetString(args, 0);
    BYTE hash[SHA256_DIGEST_LENGTH] = {0};
    unsigned int hashSize = SHA256_DIGEST_LENGTH;
    EVP_MD_CTX* shCtx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(shCtx, EVP_sha256(), NULL);
    EVP_DigestUpdate(shCtx, p0.c_str(), p0.size());
    EVP_DigestFinal_ex(shCtx,hash,&hashSize);
    EVP_MD_CTX_free(shCtx);
    std::string ret = "";
    ret.append((char*)hash, sizeof(hash));
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
    if(!HMAC(EVP_sha256(), key.c_str(), (int)key.size(),(BYTE*)input.c_str(), input.size(), buffer, &finalSize)){
      return "";
    }
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
    for (size_t i = 0; i < needSize; i++) {
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
    return TlsPRF(args[0].text, args[1].text, args[2].text, (size_t)args[3].Integer);
}

Value TLS1PRF(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(4);
    CHECK_PARAMETER_STRING(0);
    CHECK_PARAMETER_STRING(1);
    CHECK_PARAMETER_INTEGER(2);
    return Tls1PRF(args[0].text, args[1].text, (size_t)args[2].Integer);
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
    ~SSLCipher() { Close(); }
    std::string Update(const std::string& data) {
        BYTE* out = new BYTE[(int)data.size() + EVP_MAX_BLOCK_LENGTH];
        int outSize = (int)data.size() + EVP_MAX_BLOCK_LENGTH;
        EVP_CipherUpdate(mCtx, out, &outSize, (const BYTE*)data.c_str(), (int)data.size());
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
    void Close() {
        if (mCtx) {
            EVP_CIPHER_CTX_free(mCtx);
            mCtx = NULL;
        }
    };
    bool IsAvaliable() { return mCtx != NULL; }
    std::string TypeName() { return "Cipher"; };
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
        NVT_LOG_DEBUG("invalid chiper name ", cipherName);
        return Value();
    }
    if (padding < 0 || padding > 5) {
        NVT_LOG_DEBUG("invalid padding type ", padding);
        return Value();
    }
    if (EVP_CIPHER_key_length(cipher) > (int)key.size()) {
        NVT_LOG_DEBUG("invalid key size  ");
        return Value();
    }
    if (EVP_CIPHER_iv_length(cipher) > (int)iv.size()) {
        NVT_LOG_DEBUG("invalid iv size  ");
        return Value();
    }
    return Value((Resource*)new SSLCipher(cipher, padding, key, iv, (int)args[4].Integer));
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