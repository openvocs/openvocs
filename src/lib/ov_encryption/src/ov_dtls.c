/***
        ------------------------------------------------------------------------

        Copyright (c) 2023 German Aerospace Center DLR e.V. (GSOC)

        Licensed under the Apache License, Version 2.0 (the "License");
        you may not use this file except in compliance with the License.
        You may obtain a copy of the License at

                http://www.apache.org/licenses/LICENSE-2.0

        Unless required by applicable law or agreed to in writing, software
        distributed under the License is distributed on an "AS IS" BASIS,
        WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
        See the License for the specific language governing permissions and
        limitations under the License.

        This file is part of the openvocs project. https://openvocs.org

        ------------------------------------------------------------------------
*//**
        @file           ov_dtls.c
        @author         Markus

        @date           2023-12-13


        ------------------------------------------------------------------------
*/
#include "../include/ov_dtls.h"

#define OV_DTLS_KEYS_QUANTITY_DEFAULT 10
#define OV_DTLS_KEYS_LENGTH_DEFAULT 20
#define OV_DTLS_KEYS_LIFETIME_DEFAULT 300000000
#define OV_DTLS_RECONNECT_DEFAULT 50000 // 50ms

#define OV_DTLS_SSL_ERROR_STRING_BUFFER_SIZE 200

/*----------------------------------------------------------------------------*/

#include <ov_base/ov_buffer.h>
#include <ov_base/ov_config_keys.h>
#include <ov_base/ov_convert.h>
#include <ov_base/ov_file.h>
#include <ov_base/ov_random.h>
#include <ov_base/ov_socket.h>
#include <ov_base/ov_utils.h>

#include <openssl/conf.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/ssl.h>

/*----------------------------------------------------------------------------*/

static ov_list *dtls_keys = NULL;

/*----------------------------------------------------------------------------*/

struct ov_dtls {

    ov_dtls_config config;

    struct {

        SSL_CTX *ctx;

        struct {

            uint32_t key_renew;

        } timer;

    } dtls;

    char fingerprint[OV_DTLS_FINGERPRINT_MAX];
};

/*----------------------------------------------------------------------------*/

static bool init_dtls_cookie_keys(size_t quantity, size_t length) {

    if ((0 == quantity) || (0 == length)) return false;

    if (dtls_keys) {
        ov_list_clear(dtls_keys);
    } else {

        dtls_keys = ov_list_create(
            (ov_list_config){.item = ov_buffer_data_functions()});
    }

    if (!dtls_keys) goto error;

    ov_buffer *buffer = NULL;

    for (size_t i = 0; i < quantity; i++) {

        buffer = ov_buffer_create(length);
        if (!buffer) goto error;

        if (!ov_list_push(dtls_keys, buffer)) {
            buffer = ov_buffer_free(buffer);
            goto error;
        }

        if (!ov_random_bytes(buffer->start, buffer->capacity)) goto error;

        buffer->length = buffer->capacity;
    }

    return true;
error:
    dtls_keys = ov_list_free(dtls_keys);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool renew_dtls_keys(uint32_t id, void *data) {

    if (0 == id) goto error;

    ov_dtls *ssl = (ov_dtls *)data;
    if (!ssl) goto error;

    dtls_keys = ov_list_free(dtls_keys);

    if (!init_dtls_cookie_keys(
            ssl->config.dtls.keys.quantity, ssl->config.dtls.keys.length)) {

        ov_log_error("Failed to reinit DTLS key cookies");

        goto error;
    }

    if (!ssl->config.loop || !ssl->config.loop->timer.set ||
        !ssl->config.loop->timer.set(ssl->config.loop,
                                     ssl->config.dtls.keys.lifetime_usec,
                                     ssl,
                                     renew_dtls_keys)) {

        ov_log_error("Failed to reenable DTLS key renew timer");

        goto error;
    }

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool write_cookie(unsigned char *cookie,
                         unsigned int *cookie_len,
                         const ov_buffer *key) {

    if (!cookie || !cookie_len || !key) return false;

    const char *array[] = {(char *)key->start};

    size_t outlen = DTLS1_COOKIE_LENGTH;

    if (!ov_hash(OV_HASH_MD5, array, 1, &cookie, &outlen)) return false;

    *cookie_len = outlen;
    return true;
}

/*----------------------------------------------------------------------------*/

static bool check_cookie(const unsigned char *cookie,
                         unsigned int cookie_len,
                         ov_list *list) {

    if (!cookie || cookie_len < 1 || !list) return false;

    size_t hlen = OV_MD5_SIZE;
    uint8_t hash[hlen];
    memset(hash, 0, hlen);

    uint8_t *ptr = hash;

    const char *array[] = {"null"};

    void *next = list->iter(list);
    ov_buffer *buffer = NULL;

    while (next) {

        next = list->next(list, next, (void **)&buffer);
        if (!buffer) return false;

        array[0] = (char *)buffer->start;

        hlen = OV_MD5_SIZE;
        memset(hash, 0, hlen);

        if (!ov_hash(OV_HASH_MD5, array, 1, &ptr, &hlen)) return false;

        if (0 == memcmp(hash, cookie, hlen)) return true;
    }

    return false;
}

/*----------------------------------------------------------------------------*/

static int generate_dtls_cookie(SSL *ssl,
                                unsigned char *cookie,
                                unsigned int *cookie_len) {

    if (!ssl || !cookie || !cookie_len) goto error;

    /*
     *      To create a DTLS cookie, we choose a random
     *      key from the dtls_keys and concat the port string and
     *      some min length of the host
     *
     *      This way we do have some random UTF8 cookie,
     *      some random cookie selection, but some distinct
     *      identification over port and host.
     *
     *      The amount of random cookies, as well as the default
     *      cookie length is configurable.
     *
     *      write_cookie is an external function to support different
     *      (content) testing purposes.
     *
     */

    if (!dtls_keys) {

        if (!init_dtls_cookie_keys(
                OV_DTLS_KEYS_QUANTITY_DEFAULT, OV_DTLS_KEYS_LENGTH_DEFAULT))
            goto error;
    }

    srand(time(NULL));
    long int number = rand();
    number = (number * (ov_list_count(dtls_keys))) / RAND_MAX;

    if (number == 0) number = 1;

    ov_buffer *buffer = ov_list_get(dtls_keys, number);
    if (!buffer) goto error;

    if (!write_cookie(cookie, cookie_len, buffer)) goto error;

    return 1;
error:
    return 0;
}

/*---------------------------------------------------------------------------*/

static int verify_dtls_cookie(SSL *ssl,
                              const unsigned char *cookie,
                              unsigned int cookie_len) {

    if (!ssl || !cookie || !dtls_keys) goto error;

    if (cookie_len < 1) goto error;

    if (!dtls_keys) goto error;

    if (!check_cookie(cookie, cookie_len, dtls_keys)) goto error;

    return 1;
error:
    return 0;
}

/*----------------------------------------------------------------------------*/

static bool load_certificates(SSL_CTX *ctx,
                              const ov_dtls_config *config,
                              const char *type) {

    if (!ctx || !config || !type) goto error;

    if (SSL_CTX_use_certificate_chain_file(ctx, config->cert) != 1) {

        ov_log_error(
            "ICE %s config failure load certificate "
            "from %s | error %d | %s",
            type,
            config->cert,
            errno,
            strerror(errno));
        goto error;
    }

    if (SSL_CTX_use_PrivateKey_file(ctx, config->key, SSL_FILETYPE_PEM) != 1) {

        ov_log_error(
            "ICE %s config failure load key "
            "from %s | error %d | %s",
            type,
            config->key,
            errno,
            strerror(errno));
        goto error;
    }

    if (SSL_CTX_check_private_key(ctx) != 1) {

        ov_log_error(
            "ICE %s config failure private key for\n"
            "CERT | %s\n"
            " KEY | %s",
            type,
            config->cert,
            config->key);
        goto error;
    }

    ov_log_info("DTLS loaded %s certificate \n file %s\n key %s\n",
                type,
                config->cert,
                config->key);

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool configure_dtls(ov_dtls *ssl) {

    OV_ASSERT(ssl);

    if (!ssl) goto error;

    if (!ssl->dtls.ctx) {
        ssl->dtls.ctx = SSL_CTX_new(DTLS_server_method());
    }

    if (!ssl->dtls.ctx) goto error;

    if (OV_TIMER_INVALID != ssl->dtls.timer.key_renew) {

        ssl->config.loop->timer.unset(
            ssl->config.loop, ssl->dtls.timer.key_renew, NULL);
    }

    ssl->dtls.timer.key_renew = OV_TIMER_INVALID;

    if (!load_certificates(ssl->dtls.ctx, &ssl->config, "DTLS")) goto error;

    if (!init_dtls_cookie_keys(
            ssl->config.dtls.keys.quantity, ssl->config.dtls.keys.length)) {
        goto error;
    }

    SSL_CTX_set_min_proto_version(ssl->dtls.ctx, DTLS1_2_VERSION);
    SSL_CTX_set_max_proto_version(ssl->dtls.ctx, DTLS1_2_VERSION);
    SSL_CTX_set_cookie_generate_cb(ssl->dtls.ctx, generate_dtls_cookie);

    SSL_CTX_set_cookie_verify_cb(ssl->dtls.ctx, verify_dtls_cookie);

    ssl->dtls.timer.key_renew =
        ssl->config.loop->timer.set(ssl->config.loop,
                                    ssl->config.dtls.keys.lifetime_usec,
                                    ssl,
                                    renew_dtls_keys);

    if (OV_TIMER_INVALID == ssl->dtls.timer.key_renew) goto error;

    if (0 ==
        SSL_CTX_set_tlsext_use_srtp(ssl->dtls.ctx, ssl->config.srtp.profile)) {

    } else {

        goto error;
    }

    return true;
error:
    return false;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      #FINGERPRINT
 *
 *      ------------------------------------------------------------------------
 */

static char *fingerprint_format_RFC8122(const char *source, size_t length) {

    /*
    hash-func              =  "sha-1" / "sha-224" / "sha-256" /
                         "sha-384" / "sha-512" /
                         "md5" / "md2" / token
                         ; Additional hash functions can only come
                         ; from updates to RFC 3279

    fingerprint            =  2UHEX *(":" 2UHEX)
                         ; Each byte in upper-case hex, separated
                         ; by colons.

    UHEX                   =  DIGIT / %x41-46 ; A-F uppercase
    */

    char *fingerprint = NULL;

    if (!source) return NULL;

    size_t hex_len = 2 * length + 1;
    char hex[hex_len + 1];
    memset(hex, 0, hex_len);
    uint8_t *ptr = (uint8_t *)hex;

    if (!ov_convert_binary_to_hex((uint8_t *)source, length, &ptr, &hex_len))
        goto error;

    size_t size = hex_len + length;
    fingerprint = calloc(size + 1, sizeof(char));

    for (size_t i = 0; i < length; i++) {

        fingerprint[(i * 3) + 0] = toupper(hex[(i * 2) + 0]);
        fingerprint[(i * 3) + 1] = toupper(hex[(i * 2) + 1]);
        if (i < length - 1) fingerprint[(i * 3) + 2] = ':';
    }

    return fingerprint;

error:
    if (fingerprint) free(fingerprint);
    return NULL;
}

/*----------------------------------------------------------------------------*/

static char *X509_fingerprint_create(const X509 *cert, ov_hash_function type) {

    unsigned char mdigest[EVP_MAX_MD_SIZE] = {0};
    unsigned int mdigest_size = 0;

    char *fingerprint = NULL;

    const EVP_MD *func = ov_hash_function_to_EVP(type);
    if (!func || !cert) return NULL;

    if (0 < X509_digest(cert, func, mdigest, &mdigest_size)) {
        fingerprint = fingerprint_format_RFC8122((char *)mdigest, mdigest_size);
    }

    return fingerprint;
}

/*----------------------------------------------------------------------------*/

static bool create_fingerprint_cert(const char *path,
                                    ov_hash_function hash,
                                    char *out) {

    char *x509_fingerprint = NULL;

    if (!path || !out) goto error;

    const char *hash_string = ov_hash_function_to_RFC8122_string(hash);
    if (!hash_string) goto error;

    X509 *x = NULL;

    FILE *fp = fopen(path, "r");

    if (!PEM_read_X509(fp, &x, NULL, NULL)) {
        fclose(fp);
        goto error;
    }

    x509_fingerprint = X509_fingerprint_create(x, hash);
    fclose(fp);
    X509_free(x);

    if (!x509_fingerprint) goto error;

    size_t size = strlen(x509_fingerprint) + strlen(hash_string) + 2;

    if (size >= OV_DTLS_FINGERPRINT_MAX) goto error;

    memset(out, 0, OV_DTLS_FINGERPRINT_MAX);

    if (!snprintf(out,
                  OV_DTLS_FINGERPRINT_MAX,
                  "%s %s",
                  hash_string,
                  x509_fingerprint))
        goto error;

    out[OV_DTLS_FINGERPRINT_MAX - 1] = 0;
    x509_fingerprint = ov_data_pointer_free(x509_fingerprint);
    return true;
error:
    x509_fingerprint = ov_data_pointer_free(x509_fingerprint);
    return false;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_dtls_config ov_dtls_config_from_json(const ov_json_value *input) {

    ov_dtls_config out = (ov_dtls_config){0};
    if (!input) goto error;

    const ov_json_value *conf = ov_json_get(
        input, "/" OV_KEY_INTERCONNECT "/" OV_KEY_TLS "/" OV_KEY_DTLS);

    if (!conf) conf = input;

    /*
     *      We perform a read access on the cert and key,
     *      to ensure the config is valid.
     */

    const char *cert =
        ov_json_string_get(ov_json_object_get(conf, OV_KEY_CERTIFICATE));

    const char *key = ov_json_string_get(ov_json_object_get(conf, OV_KEY_KEY));

    if (!cert || !key) {
        ov_log_error("SSL JSON config without cert or key");
        goto error;
    }

    size_t bytes = 0;

    const char *error = ov_file_read_check(cert);

    if (error) {
        ov_log_error(
            "SSL config cannot read certificate "
            "at %s error %s",
            cert,
            error);
        goto error;
    }

    error = ov_file_read_check(key);

    if (error) {
        ov_log_error(
            "SSL config cannot read key "
            "at %s error %s",
            key,
            error);
        goto error;
    }

    bytes = snprintf(out.cert, PATH_MAX, "%s", cert);
    if (bytes != strlen(cert)) goto error;

    bytes = snprintf(out.key, PATH_MAX, "%s", key);
    if (bytes != strlen(key)) goto error;

    const char *string =
        ov_json_string_get(ov_json_object_get(conf, OV_KEY_CA_FILE));

    if (string) {

        error = ov_file_read_check(string);

        if (error) {
            ov_log_error(
                "SSL config cannot read CA FILE "
                "at %s error %s",
                string,
                error);
            goto error;
        }

        bytes = snprintf(out.ca.file, PATH_MAX, "%s", string);
        if (bytes != strlen(string)) goto error;
    }

    string = ov_json_string_get(ov_json_object_get(conf, OV_KEY_CA_PATH));

    if (string) {

        error = ov_file_read_check(string);

        if (!error) {

            ov_log_error(
                "SSL config wrong path for CA PATH "
                "at %s error %s",
                string,
                error);
            goto error;

        } else if (0 != strcmp(error, OV_FILE_IS_DIR)) {

            ov_log_error(
                "SSL config wrong path for CA PATH "
                "at %s error %s",
                string,
                error);
            goto error;
        }

        bytes = snprintf(out.ca.path, PATH_MAX, "%s", string);
        if (bytes != strlen(string)) goto error;
    }

    ov_json_value *keys = ov_json_object_get(conf, OV_KEY_KEYS);
    if (!keys) goto key_defaults;

    out.dtls.keys.quantity =
        ov_json_number_get(ov_json_object_get(keys, OV_KEY_QUANTITY));

    out.dtls.keys.length =
        ov_json_number_get(ov_json_object_get(keys, OV_KEY_LENGTH));

    out.dtls.keys.lifetime_usec =
        ov_json_number_get(ov_json_object_get(keys, OV_KEY_LIFETIME));

key_defaults:

    if (0 == out.dtls.keys.quantity)
        out.dtls.keys.quantity = OV_DTLS_KEYS_QUANTITY_DEFAULT;

    if (0 == out.dtls.keys.length)
        out.dtls.keys.length = OV_DTLS_KEYS_LENGTH_DEFAULT;

    if (DTLS1_COOKIE_LENGTH < out.dtls.keys.length)
        out.dtls.keys.length = DTLS1_COOKIE_LENGTH;

    if (0 == out.dtls.keys.lifetime_usec)
        out.dtls.keys.lifetime_usec = OV_DTLS_KEYS_LIFETIME_DEFAULT;

    return out;

error:
    return (ov_dtls_config){0};
}

/*----------------------------------------------------------------------------*/

ov_dtls *ov_dtls_create(ov_dtls_config config) {

    ov_dtls *self = NULL;

    if (!config.loop) goto error;

    if (0 == config.cert[0]) goto error;

    if (0 == config.key[0]) goto error;

    self = calloc(1, sizeof(ov_dtls));
    if (!self) goto error;

    if (0 == config.srtp.profile[0])
        snprintf(
            config.srtp.profile, OV_DTLS_PROFILE_MAX, OV_DTLS_SRTP_PROFILES);

    if (0 == config.dtls.keys.quantity)
        config.dtls.keys.quantity = OV_DTLS_KEYS_QUANTITY_DEFAULT;

    if (0 == config.dtls.keys.length)
        config.dtls.keys.length = OV_DTLS_KEYS_LENGTH_DEFAULT;

    if (DTLS1_COOKIE_LENGTH < config.dtls.keys.length)
        config.dtls.keys.length = DTLS1_COOKIE_LENGTH;

    if (0 == config.dtls.keys.lifetime_usec)
        config.dtls.keys.lifetime_usec = OV_DTLS_KEYS_LIFETIME_DEFAULT;

    if (0 == config.reconnect_interval_usec)
        config.reconnect_interval_usec = OV_DTLS_RECONNECT_DEFAULT;

    self->config = config;

    // init openssl
    SSL_library_init();
    SSL_load_error_strings();

    if (!configure_dtls(self)) goto error;

    if (!create_fingerprint_cert(
            self->config.cert, OV_HASH_SHA256, self->fingerprint))
        goto error;

    return self;
error:
    ov_dtls_free(self);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_dtls *ov_dtls_free(ov_dtls *self) {

    if (!self) goto error;

    if (self->dtls.ctx) {
        SSL_CTX_free(self->dtls.ctx);
        self->dtls.ctx = NULL;
    }

    // free used timers
    if (OV_TIMER_INVALID != self->dtls.timer.key_renew) {
        if (self->config.loop && self->config.loop->timer.unset)
            self->config.loop->timer.unset(
                self->config.loop, self->dtls.timer.key_renew, NULL);
    }

    self->dtls.timer.key_renew = OV_TIMER_INVALID;

    // clean other openssl initializations
    // TBD check if this are all required
    // FIPS_mode_set(0);
    EVP_cleanup();
    CRYPTO_cleanup_all_ex_data();
    ERR_free_strings();
    CONF_modules_finish();
    RAND_cleanup();

    // free global DTLS keys
    dtls_keys = ov_list_free(dtls_keys);

    self = ov_data_pointer_free(self);

error:
    return self;
}

/*----------------------------------------------------------------------------*/

const char *ov_dtls_get_fingerprint(const ov_dtls *ssl) {

    if (!ssl) goto error;

    return ssl->fingerprint;

error:
    return NULL;
}

/*---------------------------------------------------------------------------*/

const char *ov_dtls_type_to_string(ov_dtls_type type) {

    const char *out = NULL;

    switch (type) {

        case OV_DTLS_ACTIVE:
            out = OV_KEY_ACTIVE;
            break;

        case OV_DTLS_PASSIVE:
            out = OV_KEY_PASSIVE;
            break;

        default:
            out = OV_KEY_UNSET;
            break;
    }

    return out;
}

/*----------------------------------------------------------------------------*/

SSL_CTX *ov_dtls_get_ctx(ov_dtls *self) {

    if (!self) return NULL;
    return self->dtls.ctx;
}

/*----------------------------------------------------------------------------*/

const char *ov_dtls_get_srtp_profile(ov_dtls *self) {

    if (!self) return NULL;
    return self->config.srtp.profile;
}

/*----------------------------------------------------------------------------*/
const char *ov_dtls_get_verify_file(ov_dtls *self) {

    if (!self) return NULL;
    return self->config.ca.file;
}

/*----------------------------------------------------------------------------*/

const char *ov_dtls_get_verify_path(ov_dtls *self) {

    if (!self) return NULL;
    return self->config.ca.path;
}

/*----------------------------------------------------------------------------*/
