#include "sslhelper.hpp"
namespace OpenSSLHelper {

int password_callback(char* buf, int num, int rwflag, void* userdata) {
    if (num > (int)strlen((char*)userdata)) {
        strncpy(buf, (char*)userdata, num);
    }
    return (int)strlen((char*)userdata);
}

scoped_refptr<SSLObject<X509>> LoadX509FromBuffer(std::string& buffer, std::string& password,
                                                  int type) {
    BIO* in;
    X509* x509 = NULL;
    in = BIO_new_mem_buf((char*)buffer.c_str(), (int)buffer.size());
    if (in == NULL) {
        return new SSLObject<X509>(NULL, X509_free);
    }
    if (type == SSL_FILETYPE_ASN1) {
        x509 = d2i_X509_bio(in, NULL);
    } else {
        x509 = PEM_read_bio_X509(in, NULL, password_callback, (void*)password.c_str());
    }
    BIO_free(in);
    return new SSLObject<X509>(x509, X509_free);
}

scoped_refptr<SSLObject<EVP_PKEY>> LoadPrivateKeyFromBuffer(std::string& buffer,
                                                            std::string& password, int type) {
    BIO* in;
    EVP_PKEY* pKey = NULL;
    in = BIO_new_mem_buf((char*)buffer.c_str(),(int)buffer.size());
    if (in == NULL) {
        return new SSLObject<EVP_PKEY>(NULL, EVP_PKEY_free);
    }
    if (type == SSL_FILETYPE_ASN1) {
        pKey = d2i_PrivateKey_bio(in, NULL);
    } else {
        pKey = PEM_read_bio_PrivateKey(in, NULL, password_callback, (void*)password.c_str());
    }
    BIO_free(in);
    return new SSLObject<EVP_PKEY>(pKey, EVP_PKEY_free);
}
} // namespace OpenSSLHelper