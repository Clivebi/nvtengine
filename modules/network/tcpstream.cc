#include "tcpstream.hpp"

namespace OpenSSLHelper {

int password_callback(char* buf, int num, int rwflag, void* userdata) {
    if (num > strlen((char*)userdata)) {
        strncpy(buf, (char*)userdata, num);
    }
    return strlen((char*)userdata);
}

scoped_refptr<SSLObject<X509>> LoadX509FromBuffer(std::string& buffer, std::string& password,
                                                  int type) {
    BIO* in;
    X509* x509 = NULL;
    in = BIO_new_mem_buf((char*)buffer.c_str(), buffer.size());
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
    in = BIO_new_mem_buf((char*)buffer.c_str(), buffer.size());
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

TCPStream* NewTCPStream(std::string& host, std::string& port, int timeout_sec, bool isSSL) {
    int sockfd = Socket::OpenTCPConnection(host.c_str(), port.c_str(), timeout_sec);
    if (sockfd < 0) {
        return NULL;
    }
    if (!isSSL) {
        return new TCPStream(sockfd, false);
    }
    TCPStream* stream = new TCPStream(sockfd, true);
    Socket::SetBlock(stream->mSocket, 1);
    if (SSL_connect(stream->mSSL) < 0) {
        delete stream;
        return NULL;
    }
    Socket::SetBlock(stream->mSocket, 0);
    return stream;
}