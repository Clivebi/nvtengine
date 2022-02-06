#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/md5.h>
#include <openssl/rsa.h>
#include <openssl/sha.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#ifdef _WIN32
#else
#include <unistd.h>
#endif

#include <algorithm>
#include <sstream>

#include "../../net/sslhelper.hpp"
#include "../api.hpp"

Value match(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(2);
    CHECK_PARAMETER_STRING_OR_BYTES(0);
    CHECK_PARAMETER_STRING(1);
    bool icase = false;
    if (args.size() > 2 && args[2].ToBoolean()) {
        icase = true;
    }
    std::string src = GetString(args, 0);
    if (icase) {
        std::string pattern = args[1].text;
        std::transform(src.begin(), src.end(), src.begin(), tolower);
        std::transform(pattern.begin(), pattern.end(), pattern.begin(), tolower);
        return Interpreter::IsMatchString(src, pattern);
    }
    return Interpreter::IsMatchString(src, args[1].text);
}

Value Rand(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    return rand();
}

Value USleep(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(1);
    CHECK_PARAMETER_INTEGER(0);
#ifdef _WIN32
    DWORD nValue = (DWORD)args[0].Integer;
    nValue = nValue * 1000 / 1000000;
    Sleep(nValue);
#else
    usleep(args[0].Integer);
#endif
    return Value();
}

Value Sleep(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(1);
    CHECK_PARAMETER_INTEGER(0);
#ifdef _WIN32
    Sleep((DWORD)args[0].Integer * 1000);
#else
    sleep(args[0].Integer);
#endif
    return Value();
}

Value vendor_version(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    return Value("NVTEngine 0.1");
}

Value GetHostName(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    char name[260] = {0};
    unsigned int size = 260;
    gethostname(name, size);
    return Value(name);
}

bool ishex(char c);

char http_unhex(char c);

std::string decode_string(const std::string& src) {
    bool warning = false;
    std::stringstream o;
    size_t start = 0;
    for (size_t i = 0; i < src.size();) {
        if (src[i] != '\\') {
            i++;
            continue;
        }
        if (start < i) {
            o << src.substr(start, i - start);
        }
        if (i + 1 >= src.size()) {
            return src;
        }
        i++;
        switch (src[i]) {
        case 'r':
            o << "\r";
            i++;
            break;
        case 'n':
            o << "\n";
            i++;
            break;
        case 't':
            o << "\t";
            i++;
            break;
        case '\"':
            o << "\"";
            i++;
            break;
        case '\\':
            o << "\\";
            i++;
            break;
        default: {
            if (src.size() > i + 2 && src[i] == 'x' && ishex(src[i + 1]) && ishex(src[i + 2])) {
                int x = http_unhex(src[i + 1]);
                x *= 16;
                x += http_unhex(src[i + 2]);
                o << (char)(x & 0xFF);
                i += 3;
            } else {
                //原样输出
                o << "\\";
                o << src[i];
                i++;
                warning = true;
            }
        }
        }
        start = i;
    }
    if (start < src.size()) {
        o << src.substr(start);
    }
    if(warning){
         NVT_LOG_WARNING("Parse string warning input=" + src, " result=", o.str());   
    }
    return o.str();
}

Value NASLString(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    std::string ret = "";
    for (auto iter : args) {
        if (iter.Type == ValueType::kByte) {
            ret += (char)iter.Byte;
        } else {
            ret += decode_string(iter.ToString());
        }
    }
    return ret;
}

Value X509Open(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(1);
    std::string pass = "";
    std::string cert = GetString(args, 0);
    if (cert.size() == 0) {
        return Value();
    }
    scoped_refptr<OpenSSLHelper::SSLObject<X509>> x509 =
            OpenSSLHelper::LoadX509FromBuffer(cert, pass, SSL_FILETYPE_ASN1);
    if (x509 == NULL) {
        return Value();
    }
    return Value((Resource*)x509.get());
}

std::string _asn1int(ASN1_INTEGER* bs) {
    return HexEncode((char*)bs->data, bs->length);
}

std::string _asn1string(ASN1_STRING* d) {
    std::string asn1_string;
    if (ASN1_STRING_type(d) != V_ASN1_UTF8STRING) {
        unsigned char* utf8;
        int length = ASN1_STRING_to_UTF8(&utf8, d);
        asn1_string = std::string((char*)utf8, length);
        OPENSSL_free(utf8);
    } else {
        asn1_string = std::string((char*)ASN1_STRING_get0_data(d), ASN1_STRING_length(d));
    }
    return asn1_string;
}

std::string _subject_as_line(X509_NAME* subj_or_issuer) {
    BIO* bio_out = BIO_new(BIO_s_mem());
    X509_NAME_print(bio_out, subj_or_issuer, 0);
    BUF_MEM* bio_buf;
    BIO_get_mem_ptr(bio_out, &bio_buf);
    std::string issuer = std::string(bio_buf->data, bio_buf->length);
    BIO_free(bio_out);
    return issuer;
}

std::string _subject_entry_at_index(X509_NAME* subj_or_issuer, int index) {
    if (index == 0) {
        return _subject_as_line(subj_or_issuer);
    }
    if (index >= X509_NAME_entry_count(subj_or_issuer)) {
        return "";
    }
    X509_NAME_ENTRY* e = X509_NAME_get_entry(subj_or_issuer, index);
    ASN1_STRING* d = X509_NAME_ENTRY_get_data(e);
    return _asn1string(d);
}

void _asn1dateparse(const ASN1_TIME* time, int& year, int& month, int& day, int& hour, int& minute,
                    int& second) {
    const char* str = (const char*)time->data;
    size_t i = 0;
    if (time->type == V_ASN1_UTCTIME) { /* two digit year */
        year = (str[i++] - '0') * 10;
        year += (str[i++] - '0');
        year += (year < 70 ? 2000 : 1900);
    } else if (time->type == V_ASN1_GENERALIZEDTIME) { /* four digit year */
        year = (str[i++] - '0') * 1000;
        year += (str[i++] - '0') * 100;
        year += (str[i++] - '0') * 10;
        year += (str[i++] - '0');
    }
    month = (str[i++] - '0') * 10;
    month += (str[i++] - '0') - 1; // -1 since January is 0 not 1.
    day = (str[i++] - '0') * 10;
    day += (str[i++] - '0');
    hour = (str[i++] - '0') * 10;
    hour += (str[i++] - '0');
    minute = (str[i++] - '0') * 10;
    minute += (str[i++] - '0');
    second = (str[i++] - '0') * 10;
    second += (str[i++] - '0');
}

std::string asn1datetime_isodatetime(const ASN1_TIME* tm) {
    int year = 0, month = 0, day = 0, hour = 0, min = 0, sec = 0;
    _asn1dateparse(tm, year, month, day, hour, min, sec);

    char buf[25] = "";
    snprintf(buf, sizeof(buf) - 1, "%04d%02d%02dT%02d%02d%02d GMT", year, month, day, hour, min,
             sec);
    return std::string(buf);
}

std::string _asn1time(ASN1_TIME* tm) {
    std::string str = "";
    str.assign((const char*)tm->data, tm->length);
    return str;
}

std::string X509DER(X509* x509) {
    int size = i2d_X509(x509, NULL);
    BYTE* der = NULL;
    i2d_X509(x509, &der);
    std::string str = "";
    str.assign((char*)der, size);
    OPENSSL_free(der);
    return str;
}

std::vector<std::string> subject_alt_names(X509* x509) {
    std::vector<std::string> list;
    GENERAL_NAMES* subjectAltNames =
            (GENERAL_NAMES*)X509_get_ext_d2i(x509, NID_subject_alt_name, NULL, NULL);
    for (int i = 0; i < sk_GENERAL_NAME_num(subjectAltNames); i++) {
        GENERAL_NAME* gen = sk_GENERAL_NAME_value(subjectAltNames, i);
        if (gen->type == GEN_URI || gen->type == GEN_DNS || gen->type == GEN_EMAIL) {
            ASN1_IA5STRING* asn1_str = gen->d.uniformResourceIdentifier;
            std::string san = std::string((char*)ASN1_STRING_get0_data(asn1_str),
                                          ASN1_STRING_length(asn1_str));
            list.push_back(san);
        } else if (gen->type == GEN_IPADD) {
            unsigned char* p = gen->d.ip->data;
            if (gen->d.ip->length == 4) {
                std::stringstream ip;
                ip << (int)p[0] << '.' << (int)p[1] << '.' << (int)p[2] << '.' << (int)p[3];
                list.push_back(ip.str());
            } else //if(gen->d.ip->length == 16) //ipv6?
            {
                //std::cerr << "Not implemented: parse sans ("<< __FILE__ << ":" << __LINE__ << ")" << endl;
            }
        } else {
            //std::cerr << "Not implemented: parse sans ("<< __FILE__ << ":" << __LINE__ << ")" << endl;
        }
    }
    GENERAL_NAMES_free(subjectAltNames);
    return list;
}

Value X509Query(std::vector<Value>& args, VMContext* ctx, Executor* vm) {
    CHECK_PARAMETER_COUNT(2);
    CHECK_PARAMETER_RESOURCE(0);
    std::string cmd = GetString(args, 1);
    int nIndex = GetInt(args, 2, 0);
    OpenSSLHelper::SSLObject<X509>* x509Obj =
            (OpenSSLHelper::SSLObject<X509>*)args[0].resource.get();
    X509* x509 = x509Obj->operator X509*();
    if (cmd == "serial") {
        ASN1_INTEGER* bs = X509_get_serialNumber(x509);
        return _asn1int(bs);
    }
    if (cmd == "issuer") {
        std::string ret = _subject_entry_at_index(X509_get_issuer_name(x509), nIndex);
        if (ret.size() == 0) {
            return Value();
        }
        return ret;
    }

    if (cmd == "subject") {
        std::string ret = _subject_entry_at_index(X509_get_subject_name(x509), nIndex);
        if (ret.size() == 0) {
            return Value();
        }
        return ret;
    }

    if (cmd == "not-before") {
        return asn1datetime_isodatetime(X509_get_notBefore(x509));
    }

    if (cmd == "not-after") {
        return asn1datetime_isodatetime(X509_get_notAfter(x509));
    }

    if (cmd == "fpr-sha-256") {
        unsigned char hash[SHA256_DIGEST_LENGTH];
        std::string der = X509DER(x509);
        SHA256_CTX shCtx = {0};
        SHA256_Init(&shCtx);
        SHA256_Update(&shCtx, der.c_str(), der.size());
        SHA256_Final(hash, &shCtx);
        return HexEncode((char*)hash, SHA224_DIGEST_LENGTH);
    }

    if (cmd == "fpr-sha-1") {
        unsigned char hash[SHA256_DIGEST_LENGTH];
        std::string der = X509DER(x509);
        SHA_CTX shCtx = {0};
        SHA1_Init(&shCtx);
        SHA1_Update(&shCtx, der.c_str(), der.size());
        SHA1_Final(hash, &shCtx);
        return HexEncode((char*)hash, SHA224_DIGEST_LENGTH);
    }

    if (cmd == "hostnames") {
        Value ret = Value::MakeArray();
        std::vector<std::string> list = subject_alt_names(x509);
        for (auto v : list) {
            ret._array().push_back(v);
        }
        return ret;
    }

    if (cmd == "image") {
        std::string der = X509DER(x509);
        return Value(der);
    }

    if (cmd == "algorithm-name") {
        int sig_nid = X509_get_signature_nid(x509);
        return std::string(OBJ_nid2ln(sig_nid));
    }

    if (cmd == "modulus") {
        EVP_PKEY* pkey = X509_get0_pubkey(x509);
        if (EVP_PKEY_id(pkey) == EVP_PKEY_RSA) {
            RSA* rsa = EVP_PKEY_get0_RSA(pkey);
            const BIGNUM* n = RSA_get0_n(rsa);
            int size = BN_num_bytes(n);
            unsigned char* buffer = new unsigned char[size];
            BN_bn2bin(n, buffer);
            std::string str;
            str.assign((char*)buffer, size);
            delete[] buffer;
            return str;
        }
        return Value();
    }

    if (cmd == "exponent") {
        EVP_PKEY* pkey = X509_get0_pubkey(x509);
        if (EVP_PKEY_id(pkey) == EVP_PKEY_RSA) {
            RSA* rsa = EVP_PKEY_get0_RSA(pkey);
            const BIGNUM* e = RSA_get0_e(rsa);
            int size = BN_num_bytes(e);
            unsigned char* buffer = new unsigned char[size];
            BN_bn2bin(e, buffer);
            std::string str;
            str.assign((char*)buffer, size);
            delete[] buffer;
            return str;
        }
        return Value();
    }

    if (cmd == "key-size") {
        EVP_PKEY* pkey = X509_get0_pubkey(x509);
        int keysize = EVP_PKEY_bits(pkey);
        return keysize;
    }
    return Value();
}