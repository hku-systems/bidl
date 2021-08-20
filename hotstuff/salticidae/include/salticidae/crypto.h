/**
 * Copyright (c) 2018 Cornell University.
 *
 * Author: Ted Yin <tederminant@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef _SALTICIDAE_CRYPTO_H
#define _SALTICIDAE_CRYPTO_H

#include "salticidae/type.h"
#include "salticidae/util.h"

#ifdef __cplusplus
#include <openssl/sha.h>
#include <openssl/ssl.h>
#include <openssl/bn.h>

namespace salticidae {

class SHA256 {
    SHA256_CTX ctx;

    public:
    SHA256() { reset(); }

    void reset() {
        if (!SHA256_Init(&ctx))
            throw std::runtime_error("openssl SHA256 init error");
    }

    template<typename T>
    void update(const T &data) {
        update(reinterpret_cast<const uint8_t *>(&*data.begin()), data.size());
    }

    void update(const bytearray_t::const_iterator &it, size_t length) {
        update(&*it, length);
    }

    void update(const uint8_t *ptr, size_t length) {
        if (!SHA256_Update(&ctx, ptr, length))
            throw std::runtime_error("openssl SHA256 update error");
    }

    void _digest(bytearray_t &md) {
        if (!SHA256_Final(&*md.begin(), &ctx))
            throw std::runtime_error("openssl SHA256 error");
    }

    void digest(bytearray_t &md) {
        md.resize(32);
        _digest(md);
    }

    bytearray_t digest() {
        bytearray_t md(32);
        _digest(md);
        return md;
    }
};

class SHA1 {
    SHA_CTX ctx;

    public:
    SHA1() { reset(); }

    void reset() {
        if (!SHA1_Init(&ctx))
            throw std::runtime_error("openssl SHA1 init error");
    }

    template<typename T>
    void update(const T &data) {
        update(reinterpret_cast<const uint8_t *>(&*data.begin()), data.size());
    }

    void update(const bytearray_t::const_iterator &it, size_t length) {
        update(&*it, length);
    }

    void update(const uint8_t *ptr, size_t length) {
        if (!SHA1_Update(&ctx, ptr, length))
            throw std::runtime_error("openssl SHA1 update error");
    }

    void _digest(bytearray_t &md) {
        if (!SHA1_Final(&*md.begin(), &ctx))
            throw std::runtime_error("openssl SHA1 error");
    }

    void digest(bytearray_t &md) {
        md.resize(32);
        _digest(md);
    }

    bytearray_t digest() {
        bytearray_t md(32);
        _digest(md);
        return md;
    }
};

static thread_local const char *_passwd;
static inline int _tls_pem_no_passswd(char *, int, int, void *) {
    return -1;
}
static inline int _tls_pem_with_passwd(char *buf, int size, int, void *) {
    size_t _size = strlen(_passwd) + 1;
    if (_size > (size_t)size)
        throw SalticidaeError(SALTI_ERROR_TLS_X509);
    memmove(buf, _passwd, _size);
    return _size - 1;
}

static int _skip_CA_check(int, X509_STORE_CTX *) {
    return 1;
}

class PKey {
    EVP_PKEY *key;
    friend class TLSContext;
    friend class X509;
    public:
    PKey(EVP_PKEY *key): key(key) {}
    PKey(const PKey &) = delete;
    PKey(PKey &&other): key(other.key) { other.key = nullptr; }

    static PKey create_privkey_rsa(size_t nbits = 2048) {
        auto key = EVP_PKEY_new();
        if (key == nullptr)
            throw SalticidaeError(SALTI_ERROR_TLS_KEY);
        auto e = BN_new();
        BN_set_word(e, 17);
        auto rsa = RSA_new();
        auto ret = RSA_generate_key_ex(rsa, nbits, e, nullptr);
        BN_free(e);
        if (!ret)
        {
            RSA_free(rsa);
            throw SalticidaeError(SALTI_ERROR_TLS_KEY);
        }
        EVP_PKEY_set1_RSA(key, rsa);
        RSA_free(rsa);
        return PKey(key);
    }
    
    static PKey create_privkey_from_pem_file(std::string pem_fname, std::string *passwd = nullptr) {
        FILE *fp = fopen(pem_fname.c_str(), "r");
        EVP_PKEY *key;
        if (fp == nullptr)
            throw SalticidaeError(SALTI_ERROR_TLS_KEY);
        if (passwd)
        {
            _passwd = passwd->c_str();
            key = PEM_read_PrivateKey(fp, NULL, _tls_pem_with_passwd, NULL);
        }
        else
        {
            key = PEM_read_PrivateKey(fp, NULL, _tls_pem_no_passswd, NULL);
        }
        if (key == nullptr)
            throw SalticidaeError(SALTI_ERROR_TLS_KEY);
        fclose(fp);
        return PKey(key);
    }

    static PKey create_privkey_from_der(const bytearray_t &der) {
        const auto *_der = &der[0];
        EVP_PKEY *key;
        key = d2i_AutoPrivateKey(NULL, (const unsigned char **)&_der, der.size());
        if (key == nullptr)
            throw SalticidaeError(SALTI_ERROR_TLS_KEY);
        return PKey(key);
    }

    bytearray_t get_pubkey_der() const {
        uint8_t *der = nullptr;
        auto ret = i2d_PublicKey(key, &der);
        if (ret <= 0)
            throw SalticidaeError(SALTI_ERROR_TLS_KEY);
        bytearray_t res(der, der + ret);
        OPENSSL_cleanse(der, ret);
        OPENSSL_free(der);
        return res;
    }

    bytearray_t get_privkey_der() const {
        uint8_t *der = nullptr;
        auto ret = i2d_PrivateKey(key, &der);
        if (ret <= 0)
            throw SalticidaeError(SALTI_ERROR_TLS_KEY);
        bytearray_t res(der, der + ret);
        OPENSSL_cleanse(der, ret);
        OPENSSL_free(der);
        return res;
    }

    void save_privkey_to_file(const std::string &fname) {
        FILE *fp = fopen(fname.c_str(), "wb");
        auto ret = PEM_write_PrivateKey(fp, key, nullptr, nullptr, 0, nullptr, nullptr);
        fclose(fp);
        if (!ret)
            throw SalticidaeError(SALTI_ERROR_TLS_X509);
    }

    ~PKey() { if (key) EVP_PKEY_free(key); }
};

class X509 {
    ::X509 *x509;
    friend class TLSContext;
    public:
    X509(::X509 *x509): x509(x509) {}
    X509(const X509 &) = delete;
    X509(X509 &&other): x509(other.x509) { other.x509 = nullptr; }

    static X509 create_self_signed_from_pubkey(const PKey &pkey, const char *country = "US", const char *common_name = "localhost") {
        auto x509 = X509_new();
        ASN1_INTEGER_set(X509_get_serialNumber(x509), 1);
        X509_set_pubkey(x509, pkey.key);
        X509_gmtime_adj(X509_get_notBefore(x509), 0);
        X509_gmtime_adj(X509_get_notAfter(x509), 0);
        auto name = X509_get_subject_name(x509);
        X509_NAME_add_entry_by_txt(name, "C", MBSTRING_ASC,
                                    (unsigned char *)country, -1, -1, 0);
        X509_NAME_add_entry_by_txt(name, "O", MBSTRING_ASC,
                                    (unsigned char *)"libsalticidae", -1, -1, 0);
        X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC,
                                    (unsigned char *)common_name, -1, -1, 0);
        X509_set_issuer_name(x509, name);
        X509_sign(x509, pkey.key, EVP_sha1());
        return X509(x509);
    }
    
    static X509 create_from_pem_file(std::string pem_fname, std::string *passwd = nullptr) {
        FILE *fp = fopen(pem_fname.c_str(), "r");
        ::X509 *x509;
        if (fp == nullptr)
            throw SalticidaeError(SALTI_ERROR_TLS_X509);
        if (passwd)
        {
            _passwd = passwd->c_str();
            x509 = PEM_read_X509(fp, NULL, _tls_pem_with_passwd, NULL);
        }
        else
        {
            x509 = PEM_read_X509(fp, NULL, _tls_pem_no_passswd, NULL);
        }
        if (x509 == nullptr)
            throw SalticidaeError(SALTI_ERROR_TLS_X509);
        fclose(fp);
        return X509(x509);
    }

    static X509 create_from_der(const bytearray_t &der) {
        const auto *_der = &der[0];
        ::X509 *x509;
        x509 = d2i_X509(NULL, (const unsigned char **)&_der, der.size());
        if (x509 == nullptr)
            throw SalticidaeError(SALTI_ERROR_TLS_X509);
        return X509(x509);
    }

    PKey get_pubkey() const {
        auto key = X509_get_pubkey(x509);
        if (key == nullptr)
            throw SalticidaeError(SALTI_ERROR_TLS_X509);
        return PKey(key);
    }

    bytearray_t get_der() const {
        uint8_t *der = nullptr;
        auto ret = i2d_X509(x509, &der);
        if (ret <= 0)
            throw SalticidaeError(SALTI_ERROR_TLS_X509);
        bytearray_t res(der, der + ret);
        OPENSSL_cleanse(der, ret);
        OPENSSL_free(der);
        return res;
    }

    void save_to_file(const std::string &fname) {
        FILE *fp = fopen(fname.c_str(), "wb");
        auto ret = PEM_write_X509(fp, x509);
        fclose(fp);
        if (!ret)
            throw SalticidaeError(SALTI_ERROR_TLS_X509);
    }

    ~X509() { if (x509) X509_free(x509); }
};

class TLSContext {
    SSL_CTX *ctx;
    friend class TLS;
    public:
    TLSContext(): ctx(SSL_CTX_new(TLS_method())) {
        if (ctx == nullptr)
            throw std::runtime_error("TLSContext init error");
    }

    TLSContext(const TLSContext &) = delete;
    TLSContext(TLSContext &&other): ctx(other.ctx) { other.ctx = nullptr; }

    void use_cert_file(const std::string &fname) {
        auto ret = SSL_CTX_use_certificate_file(ctx, fname.c_str(), SSL_FILETYPE_PEM);
        if (ret <= 0)
            throw SalticidaeError(SALTI_ERROR_TLS_LOAD_CERT);
    }

    void use_cert(const X509 &x509) {
        auto ret = SSL_CTX_use_certificate(ctx, x509.x509);
        if (ret <= 0)
            throw SalticidaeError(SALTI_ERROR_TLS_LOAD_CERT);
    }

    void use_privkey_file(const std::string &fname) {
        auto ret = SSL_CTX_use_PrivateKey_file(ctx, fname.c_str(), SSL_FILETYPE_PEM);
        if (ret <= 0)
            throw SalticidaeError(SALTI_ERROR_TLS_LOAD_KEY);
    }

    void use_privkey(const PKey &key) {
        auto ret = SSL_CTX_use_PrivateKey(ctx, key.key);
        if (ret <= 0)
            throw SalticidaeError(SALTI_ERROR_TLS_LOAD_KEY);
    }

    void set_verify(bool skip_ca_check = true, SSL_verify_cb verify_callback = nullptr) {
        SSL_CTX_set_verify(ctx,
                SSL_VERIFY_PEER, skip_ca_check ? _skip_CA_check : verify_callback);
    }

    bool check_privkey() {
        return SSL_CTX_check_private_key(ctx) > 0;
    }

    ~TLSContext() { if (ctx) SSL_CTX_free(ctx); }
};

using tls_context_t = ArcObj<TLSContext>;

class TLS {
    SSL *ssl;
    public:
    TLS(const tls_context_t &ctx, int fd, bool accept): ssl(SSL_new(ctx->ctx)) {
        if (ssl == nullptr)
            throw std::runtime_error("TLS init error");
        if (!SSL_set_fd(ssl, fd))
            throw SalticidaeError(SALTI_ERROR_TLS_GENERIC);
        if (accept)
            SSL_set_accept_state(ssl);
        else
            SSL_set_connect_state(ssl);
    }

    TLS(const TLS &) = delete;
    TLS(TLS &&other): ssl(other.ssl) { other.ssl = nullptr; }

    bool do_handshake(int &want_io_type) {
        /* want_io_type: 0 for read, 1 for write */
        /* return true if handshake is completed */
        auto ret = SSL_do_handshake(ssl);
        if (ret == 1) return true;
        auto err = SSL_get_error(ssl, ret);
        if (err == SSL_ERROR_WANT_WRITE)
            want_io_type = 1;
        else if (err == SSL_ERROR_WANT_READ)
            want_io_type = 0;
        else
            throw SalticidaeError(SALTI_ERROR_TLS_GENERIC);
        return false;
    }

    X509 get_peer_cert() {
        ::X509 *x509 = SSL_get_peer_certificate(ssl);
        if (x509 == nullptr)
            throw SalticidaeError(SALTI_ERROR_TLS_GENERIC);
        return X509(x509);
    }

    inline int send(const void *buff, size_t size) {
        return SSL_write(ssl, buff, size);
    }

    inline int recv(void *buff, size_t size) {
        return SSL_read(ssl, buff, size);
    }

    int get_error(int ret) {
        return SSL_get_error(ssl, ret);
    }

    void shutdown() { SSL_shutdown(ssl); }

    ~TLS() { if (ssl) SSL_free(ssl); }
};

}

#ifdef SALTICIDAE_CBINDINGS
using x509_t = salticidae::X509;
using pkey_t = salticidae::PKey;
#endif

#else

#ifdef SALTICIDAE_CBINDINGS
typedef struct x509_t x509_t;
typedef struct pkey_t pkey_t;
#endif

#endif

#ifdef SALTICIDAE_CBINDINGS
#ifdef __cplusplus
extern "C" {
#endif

x509_t *x509_new_from_pem_file(const char *pem_fname, const char *passwd, SalticidaeCError *err);
x509_t *x509_new_from_der(const bytearray_t *der, SalticidaeCError *err);
void x509_free(const x509_t *self);
pkey_t *x509_get_pubkey(const x509_t *self);
bytearray_t *x509_get_der(const x509_t *self);

pkey_t *pkey_new_privkey_from_pem_file(const char *pem_fname, const char *passwd, SalticidaeCError *err);
pkey_t *pkey_new_privkey_from_der(const bytearray_t *der, SalticidaeCError *err);
void pkey_free(const pkey_t *self);
bytearray_t *pkey_get_pubkey_der(const pkey_t *self);
bytearray_t *pkey_get_privkey_der(const pkey_t *self);

#ifdef __cplusplus
}
#endif
#endif

#endif
