/***
        ------------------------------------------------------------------------

        Copyright (c) 2024 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_ice.c
        @author         Markus

        @date           2024-09-18


        ------------------------------------------------------------------------
*/
#include "../include/ov_ice.h"

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <ov_base/ov_utils.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <openssl/conf.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/ssl.h>

#include <ov_base/ov_convert.h>
#include <ov_base/ov_dict.h>
#include <ov_base/ov_time.h>

#include <ov_stun/ov_stun_frame.h>

#include <ov_encryption/ov_hash.h>

/*----------------------------------------------------------------------------*/

struct ov_ice {

    uint16_t magic_bytes;

    ov_ice_config config;

    bool autodiscovery;

    struct {

        uint32_t min;
        uint32_t max;

    } ports;

    struct {

        bool stun;
        bool ice;
        bool dtls;

    } debug;

    struct {

        uint32_t dtls_key_renew;

    } timer;

    struct {

        ov_ice_dtls_cookie_store *cookies;
        SSL_CTX *ctx;
        char fingerprint[OV_ICE_DTLS_FINGERPRINT_MAX];

    } dtls;

    ov_ice_server *server;

    ov_dict *interfaces;
    ov_dict *transactions;
    ov_dict *sessions;
};

SSL_CTX *ov_ice_get_dtls_ctx(const ov_ice *self) {

    if (!self) return NULL;
    return self->dtls.ctx;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      #interfaces
 *
 *      ------------------------------------------------------------------------
 */

static void *interface_free(void *self) {

    self = ov_data_pointer_free(self);
    return self;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      #transactions
 *
 *      ------------------------------------------------------------------------
 */

typedef struct Transaction {

    uint64_t created;
    void *data;

} Transaction;

/*----------------------------------------------------------------------------*/

bool ov_ice_transaction_unset(ov_ice *self, const uint8_t *transaction_id) {

    char key[13] = {0};

    if (!self || !transaction_id) goto error;

    strncpy(key, (char *)transaction_id, 12);

    return ov_dict_del(self->transactions, key);
error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_ice_transaction_create(ov_ice *self,
                               uint8_t *buffer,
                               size_t size,
                               void *userdata) {

    char key[13] = {0};

    Transaction *transaction = NULL;
    if (!self || !buffer || size < 12) goto error;

    transaction = calloc(1, sizeof(Transaction));
    if (!transaction) goto error;

    transaction->created = ov_time_get_current_time_usecs();
    transaction->data = userdata;

    ov_stun_frame_generate_transaction_id((uint8_t *)key);

    if (!ov_dict_set(self->transactions, ov_string_dup(key), transaction, NULL))
        goto error;

    memcpy(buffer, key, 12);

    return true;
error:
    transaction = ov_data_pointer_free(transaction);
    return false;
}

/*----------------------------------------------------------------------------*/

void *ov_ice_transaction_get(ov_ice *self, const uint8_t *transaction_id) {

    char key[13] = {0};

    if (!self || !transaction_id) goto error;

    strncpy(key, (char *)transaction_id, 12);

    Transaction *t = ov_dict_remove(self->transactions, key);
    if (!t) goto error;

    void *p = t->data;
    t = ov_data_pointer_free(t);
    return p;
error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

static void *transaction_free(void *self) {

    self = ov_data_pointer_free(self);
    return self;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      #config
 *
 *      ------------------------------------------------------------------------
 */

static bool init_config(ov_ice_config *config) {

    if (!config) goto error;

    if (!config->loop) {

        ov_log_error("ICE core without loop unsupported.");
        goto error;
    }

    if (!ov_ice_dtls_config_init(&config->dtls)) {

        ov_log_error("ICE core DTLS config unsupported.");
        goto error;
    }

    if (0 == config->limits.transaction_lifetime_usecs)
        config->limits.transaction_lifetime_usecs =
            OV_ICE_DEFAULT_TRANSACTION_LIFETIME_USECS;

    if (0 == config->limits.stun.connectivity_pace_usecs)
        config->limits.stun.connectivity_pace_usecs =
            OV_ICE_DEFAULT_CONNECTIVITY_PACE_USECS;

    if (0 == config->limits.stun.session_timeout_usecs)
        config->limits.stun.session_timeout_usecs =
            OV_ICE_DEFAULT_SESSION_TIMEOUT_USECS;

    if (0 == config->limits.stun.keepalive_usecs)
        config->limits.stun.keepalive_usecs = OV_ICE_DEFAULT_KEEPALIVE_USECS;

    return true;

error:
    return false;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      #dtls FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

static bool load_certificates(SSL_CTX *ctx,
                              const ov_ice_config *config,
                              const char *type) {

    if (!ctx || !config || !type) goto error;

    if (SSL_CTX_use_certificate_chain_file(ctx, config->dtls.cert) != 1) {

        ov_log_error(
            "ICE core %s config failure load certificate "
            "from %s | error %d | %s",
            type,
            config->dtls.cert,
            errno,
            strerror(errno));
        goto error;
    }

    if (SSL_CTX_use_PrivateKey_file(ctx, config->dtls.key, SSL_FILETYPE_PEM) !=
        1) {

        ov_log_error(
            "ICE core %s config failure load key "
            "from %s | error %d | %s",
            type,
            config->dtls.key,
            errno,
            strerror(errno));
        goto error;
    }

    if (SSL_CTX_check_private_key(ctx) != 1) {

        ov_log_error(
            "ICE core %s config failure private key for\n"
            "CERT | %s\n"
            " KEY | %s",
            type,
            config->dtls.cert,
            config->dtls.key);
        goto error;
    }

    ov_log_info("ICE core loaded %s certificate \n file %s\n key %s\n",
                type,
                config->dtls.cert,
                config->dtls.key);

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool renew_dtls_keys(uint32_t timer_id, void *data) {

    UNUSED(timer_id);
    ov_ice *self = ov_ice_cast(data);

    if (!ov_ice_dtls_cookie_store_initialize(
            self->dtls.cookies,
            self->config.dtls.dtls.keys.quantity,
            self->config.dtls.dtls.keys.length))
        goto error;

    self->timer.dtls_key_renew =
        self->config.loop->timer.set(self->config.loop,
                                     self->config.dtls.dtls.keys.lifetime_usec,
                                     self,
                                     renew_dtls_keys);

    if (OV_TIMER_INVALID == self->timer.dtls_key_renew) goto error;

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

    OV_ASSERT(size < OV_ICE_DTLS_FINGERPRINT_MAX);
    if (size >= OV_ICE_DTLS_FINGERPRINT_MAX) goto error;

    memset(out, 0, OV_ICE_DTLS_FINGERPRINT_MAX);

    if (!snprintf(out,
                  OV_ICE_DTLS_FINGERPRINT_MAX,
                  "%s %s",
                  hash_string,
                  x509_fingerprint))
        goto error;

    out[OV_ICE_DTLS_FINGERPRINT_MAX - 1] = 0;
    x509_fingerprint = ov_data_pointer_free(x509_fingerprint);
    return true;
error:
    x509_fingerprint = ov_data_pointer_free(x509_fingerprint);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool init_dtls(ov_ice *self) {

    if (!self) goto error;

    if (!self->dtls.ctx) {
        self->dtls.ctx = SSL_CTX_new(DTLS_server_method());
    }

    if (!self->dtls.ctx) goto error;

    if (!self->dtls.cookies) {
        self->dtls.cookies = ov_ice_dtls_cookie_store_create();
        if (!self->dtls.cookies) goto error;
    }

    if (OV_TIMER_INVALID != self->timer.dtls_key_renew) {

        self->config.loop->timer.unset(
            self->config.loop, self->timer.dtls_key_renew, NULL);
    }

    self->timer.dtls_key_renew = OV_TIMER_INVALID;

    if (!load_certificates(self->dtls.ctx, &self->config, "DTLS")) goto error;

    if (!ov_ice_dtls_cookie_store_initialize(
            self->dtls.cookies,
            self->config.dtls.dtls.keys.quantity,
            self->config.dtls.dtls.keys.length))
        goto error;

    SSL_CTX_set_min_proto_version(self->dtls.ctx, DTLS1_2_VERSION);
    SSL_CTX_set_max_proto_version(self->dtls.ctx, DTLS1_2_VERSION);

    SSL_CTX_set_cookie_generate_cb(self->dtls.ctx, ov_ice_dtls_cookie_generate);

    SSL_CTX_set_cookie_verify_cb(self->dtls.ctx, ov_ice_dtls_cookie_verify);

    self->timer.dtls_key_renew =
        self->config.loop->timer.set(self->config.loop,
                                     self->config.dtls.dtls.keys.lifetime_usec,
                                     self,
                                     renew_dtls_keys);

    if (OV_TIMER_INVALID == self->timer.dtls_key_renew) goto error;

    if (0 == SSL_CTX_set_tlsext_use_srtp(
                 self->dtls.ctx, self->config.dtls.srtp.profile)) {

        ov_log_debug("SSL context enabled SRTP profiles %s",
                     self->config.dtls.srtp.profile);

    } else {

        ov_log_error("SSL failed to enabled SRTP profiles %s",
                     self->config.dtls.srtp.profile);

        goto error;
    }

    if (!create_fingerprint_cert(
            self->config.dtls.cert, OV_HASH_SHA256, self->dtls.fingerprint)) {

        ov_log_error("SSL failed to create certificate fingerprint.");
        goto error;
    }

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

ov_ice *ov_ice_create(ov_ice_config *config) {

    ov_ice *self = NULL;

    if (!init_config(config)) {
        ov_log_error("Configuration error, cannot create ICE.");
        goto error;
    }

    // init openssl
    SSL_library_init();
    SSL_load_error_strings();
    ov_ice_dtls_filter_init();

    srtp_init();

    self = calloc(1, sizeof(ov_ice));
    if (!self) goto error;

    self->magic_bytes = OV_ICE_MAGIC_BYTES;
    self->config = *config;

    if (!init_dtls(self)) {

        ov_log_error("Failed to init DTLS");
        goto error;
    }

    ov_dict_config d_config = ov_dict_string_key_config(255);

    d_config.value.data_function.free = interface_free;
    self->interfaces = ov_dict_create(d_config);
    if (!self->interfaces) goto error;

    d_config.value.data_function.free = transaction_free;
    self->transactions = ov_dict_create(d_config);
    if (!self->transactions) goto error;

    d_config.value.data_function.free = ov_ice_session_free;
    self->sessions = ov_dict_create(d_config);
    if (!self->sessions) goto error;

    return self;
error:
    ov_ice_free(self);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_ice *ov_ice_cast(const void *data) {

    if (!data) goto error;

    if (*(uint16_t *)data == OV_ICE_MAGIC_BYTES) return (ov_ice *)data;
error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_ice *ov_ice_free(ov_ice *self) {

    if (!ov_ice_cast(self)) goto error;

    self->sessions = ov_dict_free(self->sessions);
    self->interfaces = ov_dict_free(self->interfaces);
    self->transactions = ov_dict_free(self->transactions);

    if (OV_TIMER_INVALID != self->timer.dtls_key_renew)
        ov_event_loop_timer_unset(
            self->config.loop, self->timer.dtls_key_renew, NULL);

    self->dtls.cookies = ov_ice_dtls_cookie_store_free(self->dtls.cookies);

    if (self->dtls.ctx) {
        SSL_CTX_free(self->dtls.ctx);
    }

    ov_ice_dtls_filter_deinit();
    // clean other openssl initializations
    // TBD check if this are all required
    // FIPS_mode_set(0);
    EVP_cleanup();
    CRYPTO_cleanup_all_ex_data();
    ERR_free_strings();
    CONF_modules_finish();
    RAND_cleanup();

    ov_ice_server *server = NULL;

    while (self->server) {

        server = ov_node_pop((void **)&self->server);
        server = ov_ice_server_free(server);
    }

    self = ov_data_pointer_free(self);
error:
    return self;
}

/*----------------------------------------------------------------------------*/

ov_ice_config ov_ice_config_from_json(const ov_json_value *input) {

    ov_ice_config out = {0};

    if (!input) goto error;

    const ov_json_value *conf = ov_json_object_get(input, OV_KEY_ICE);
    if (!conf) conf = input;

    out.dtls = ov_ice_dtls_config_from_json(conf);

    ov_json_value *limits = ov_json_object_get(conf, OV_KEY_LIMITS);
    if (!limits) goto done;

    out.limits.transaction_lifetime_usecs = ov_json_number_get(
        ov_json_get(limits, "/" OV_ICE_KEY_TRANSACTION_LIFETIME_USECS));

    out.limits.stun.session_timeout_usecs = ov_json_number_get(ov_json_get(
        limits, "/" OV_KEY_STUN "/" OV_ICE_KEY_SESSION_TIMEOUT_USECS));

    out.limits.stun.connectivity_pace_usecs = ov_json_number_get(
        ov_json_get(limits, "/" OV_KEY_STUN "/" OV_ICE_KEY_STUN_PACE_USECS));

    out.limits.stun.keepalive_usecs = ov_json_number_get(
        ov_json_get(limits, "/" OV_KEY_STUN "/" OV_ICE_KEY_KEEPALIVE_USECS));
done:
    return out;
error:
    return (ov_ice_config){0};
}

/*----------------------------------------------------------------------------*/

bool ov_ice_add_server(ov_ice *self, ov_ice_server server) {

    ov_ice_server *srv = NULL;

    if (!self) goto error;

    if (0 == server.socket.host[0]) goto error;
    if (0 == server.socket.port) goto error;

    switch (server.type) {

        case OV_ICE_STUN_SERVER:
            break;

        case OV_ICE_TURN_SERVER:

            if (0 == server.auth.user[0]) goto error;

            if (0 == server.auth.pass[0]) goto error;

            break;

        default:
            goto error;
    }

    srv = ov_ice_server_create(NULL);
    if (!srv) goto error;

    *srv = server;
    srv->node.type = OV_ICE_SERVER_MAGIC_BYTES;
    srv->loop = self->config.loop;
    srv->node.next = NULL;
    srv->node.prev = NULL;

    if (!ov_node_push((void **)&self->server, srv)) {
        srv = ov_ice_server_free(srv);
        goto error;
    }

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_ice_add_interface(ov_ice *self, const char *host) {

    char *key = NULL;

    if (!self || !host) goto error;

    self->autodiscovery = false;

    key = ov_string_dup(host);
    if (!key) goto error;

    if (!ov_dict_set(self->interfaces, key, NULL, NULL)) goto error;

    return true;
error:
    key = ov_data_pointer_free(key);
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_ice_set_autodiscovery(ov_ice *self, bool on) {

    if (!self) return false;

    self->autodiscovery = on;
    return true;
}

/*----------------------------------------------------------------------------*/

bool ov_ice_set_debug_stun(ov_ice *self, bool on) {

    if (!self) return false;

    self->debug.stun = on;
    return true;
}

/*----------------------------------------------------------------------------*/

bool ov_ice_set_debug_ice(ov_ice *self, bool on) {

    if (!self) return false;

    self->debug.ice = on;
    return true;
}

/*----------------------------------------------------------------------------*/

bool ov_ice_set_debug_dtls(ov_ice *self, bool on) {

    if (!self) return false;

    self->debug.dtls = on;
    return true;
}

/*----------------------------------------------------------------------------*/

bool ov_ice_debug_stun(const ov_ice *self) {

    if (!self) return false;
    return self->debug.stun;
}

/*----------------------------------------------------------------------------*/

bool ov_ice_debug_ice(const ov_ice *self) {

    if (!self) return false;
    return self->debug.ice;
}

/*----------------------------------------------------------------------------*/

bool ov_ice_debug_dtls(const ov_ice *self) {

    if (!self) return false;
    return self->debug.dtls;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      EXTERNAL USE FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

const char *ov_ice_create_session_offer(ov_ice *self, ov_sdp_session *sdp) {

    if (!self || !sdp) goto error;

    ov_ice_session *session = ov_ice_session_create_offer(self, sdp);

    if (!session) goto error;

    char *key = ov_string_dup(session->uuid);
    if (!ov_dict_set(self->sessions, key, session, NULL)) {

        key = ov_data_pointer_free(key);
        session = ov_ice_session_free(session);
        goto error;
    }

    return session->uuid;
error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

const char *ov_ice_create_session_answer(ov_ice *self, ov_sdp_session *sdp) {

    if (!self || !sdp) goto error;

    ov_ice_session *session = ov_ice_session_create_answer(self, sdp);

    if (!session) goto error;

    char *key = ov_string_dup(session->uuid);
    if (!ov_dict_set(self->sessions, key, session, NULL)) {

        key = ov_data_pointer_free(key);
        session = ov_ice_session_free(session);
        goto error;
    }

    return session->uuid;
error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

bool ov_ice_session_process_answer(ov_ice *self,
                                   const char *session_id,
                                   const ov_sdp_session *sdp) {

    if (!self || !session_id || !sdp) goto error;

    ov_ice_session *session = ov_dict_get(self->sessions, session_id);
    if (!session) goto error;

    return ov_ice_session_process_answer_in(session, sdp);
error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_ice_drop_session(ov_ice *self, const char *uuid) {

    if (!self || !uuid) return false;

    return ov_dict_del(self->sessions, uuid);
}

/*----------------------------------------------------------------------------*/

bool ov_ice_add_candidate(ov_ice *self,
                          const char *session_uuid,
                          const int stream_id,
                          const ov_ice_candidate *candidate) {

    if (!self || !session_uuid || !candidate) goto error;

    ov_ice_session *session = ov_dict_get(self->sessions, session_uuid);
    return ov_ice_session_candidate(session, stream_id, candidate);
error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_ice_add_end_of_candidates(ov_ice *self,
                                  const char *session_uuid,
                                  const int stream_id) {

    if (!self || !session_uuid) goto error;

    ov_ice_session *session = ov_dict_get(self->sessions, session_uuid);
    return ov_ice_session_end_of_candidates(session, stream_id);
error:
    return false;
}

/*----------------------------------------------------------------------------*/

uint32_t ov_ice_get_stream_ssrc(ov_ice *self,
                                const char *session_uuid,
                                const int stream_id) {

    if (!self || !session_uuid) return 0;

    ov_ice_session *session = ov_dict_get(self->sessions, session_uuid);
    return ov_ice_session_get_stream_ssrc(session, stream_id);
}

/*----------------------------------------------------------------------------*/

ssize_t ov_ice_stream_send(ov_ice *self,
                           const char *session_uuid,
                           const int stream_id,
                           uint8_t *buffer,
                           size_t size) {

    if (!self || !session_uuid) return -1;

    ov_ice_session *session = ov_dict_get(self->sessions, session_uuid);
    return ov_ice_session_stream_send(session, stream_id, buffer, size);
}

/*----------------------------------------------------------------------------*/

bool ov_ice_set_port_range(ov_ice *self, uint32_t min, uint32_t max) {

    if (!self) goto error;

    self->ports.min = min;
    self->ports.max = max;

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

ov_event_loop *ov_ice_get_event_loop(const ov_ice *self) {

    if (!self) return NULL;
    return self->config.loop;
}

/*----------------------------------------------------------------------------*/

const char *ov_ice_get_fingerprint(const ov_ice *self) {

    if (!self) return NULL;
    return self->dtls.fingerprint;
}

/*----------------------------------------------------------------------------*/

static bool interface_permit_ipv4(const struct sockaddr_in *sock) {

    OV_ASSERT(sock);

    if (!sock) goto error;

    char host[OV_HOST_NAME_MAX + 1] = {0};

    if (!inet_ntop(AF_INET, &sock->sin_addr, host, OV_HOST_NAME_MAX))
        goto error;
    /*
     *      ignore local IP4 interfaces
     */

    if (0 == strncmp("127", host, 3)) goto error;

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool interface_permit_ipv6(const struct sockaddr_in6 *sock) {

    OV_ASSERT(sock);

    if (!sock) goto error;

    char host[OV_HOST_NAME_MAX + 1] = {0};

    if (!inet_ntop(AF_INET6, &sock->sin6_addr, host, OV_HOST_NAME_MAX))
        goto error;

    /*
     *      (1) ignore local IP6 interfaces
     */

    bool zero = true;

    if (0xFE & sock->sin6_addr.s6_addr[0]) {

        zero = false;
        if (0x80 == sock->sin6_addr.s6_addr[1]) {

            zero = true;
            for (size_t i = 2; i < 8; i++) {

                if (sock->sin6_addr.s6_addr[i] != 0x00) {
                    zero = false;
                    break;
                }
            }
        }

        // Link-Local IPv6 Unicast Addresses
        if (zero) goto error;

        // Site-Local IPv6 Unicast Addresses
        if (0xC0 & sock->sin6_addr.s6_addr[1]) goto error;
    }

    zero = true;
    for (size_t i = 0; i < 10; i++) {

        if (sock->sin6_addr.s6_addr[i] != 0x00) {
            zero = false;
            break;
        }
    }

    if (zero) {

        // ignore IPv4 compatible
        if ((sock->sin6_addr.s6_addr[10] == 0x00) &&
            (sock->sin6_addr.s6_addr[11] == 0x00))
            goto error;

        // ignore IPv4 mapped
        if ((sock->sin6_addr.s6_addr[10] == 0xFF) &&
            (sock->sin6_addr.s6_addr[11] == 0xFF))
            goto error;

        for (size_t i = 10; i < 15; i++) {

            if (sock->sin6_addr.s6_addr[i] != 0x00) {
                zero = false;
                break;
            }
        }

        if (zero) {

            // loopback address ::1
            if (sock->sin6_addr.s6_addr[15] & 0x01) goto error;

            // any address ::0 | ::
            if (sock->sin6_addr.s6_addr[15] & 0x00) goto error;
        }
    }

    // done:
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static int socket_create_range(ov_ice *core, ov_socket_configuration config) {

    if (!core) goto error;

    int socket = -1;

    if (0 != core->ports.min) {

        for (uint32_t i = core->ports.min; i < core->ports.max; i++) {

            config.port = i;
            socket = ov_socket_create(config, false, NULL);
            if (-1 != socket) return socket;
        }

        goto error;
    }

    config.port = 0;
    socket = ov_socket_create(config, false, NULL);

    return socket;

error:
    return -1;
}

/*----------------------------------------------------------------------------*/

static bool core_gather_autodiscovery(ov_ice *self, ov_ice_stream *stream) {

    ov_ice_base *base = NULL;

    struct ifaddrs *ifaddr, *ifa;

    if (!self || !stream) goto error;

    ov_socket_configuration socket_config = {0};
    int socket = -1, n = 0;

    if (getifaddrs(&ifaddr) == -1) {
        goto error;
    }

    for (ifa = ifaddr, n = 0; ifa != NULL; ifa = ifa->ifa_next, n++) {

        if (ifa->ifa_addr == NULL) continue;

        socket = -1;
        socket_config = (ov_socket_configuration){0};
        socket_config.type = UDP;

        bool ignore = true;

        switch (ifa->ifa_addr->sa_family) {

            case AF_INET:

                if (interface_permit_ipv4(
                        (struct sockaddr_in *)ifa->ifa_addr)) {

                    if (0 == getnameinfo(ifa->ifa_addr,
                                         sizeof(struct sockaddr_in),
                                         socket_config.host,
                                         OV_HOST_NAME_MAX,
                                         NULL,
                                         0,
                                         NI_NUMERICHOST)) {

                        ignore = false;
                    }
                }

                break;

            case AF_INET6:

                if (interface_permit_ipv6(
                        (struct sockaddr_in6 *)ifa->ifa_addr)) {

                    if (0 == getnameinfo(ifa->ifa_addr,
                                         sizeof(struct sockaddr_in6),
                                         socket_config.host,
                                         OV_HOST_NAME_MAX,
                                         NULL,
                                         0,
                                         NI_NUMERICHOST)) {

                        ignore = false;
                    }
                }

                break;

            default:
                break;
        }

        if (ignore) continue;

        if (0 == socket_config.host[0]) continue;

        socket = socket_create_range(self, socket_config);

        if (-1 == socket) {

            continue;

        } else {

            base = ov_ice_base_create(stream, socket);

            if (!base) {
                close(socket);
                goto error;
            }
        }
    }

    freeifaddrs(ifaddr);
    ifaddr = NULL;

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

struct container {

    ov_ice *core;
    ov_ice_stream *stream;
};

/*----------------------------------------------------------------------------*/

static bool gather_interface(const void *key, void *value, void *data) {

    if (!key) return true;
    UNUSED(value);

    struct container *container = (struct container *)data;

    ov_socket_configuration config = {.type = UDP};
    strncpy(config.host, key, OV_HOST_NAME_MAX);

    int socket = socket_create_range(container->core, config);
    if (-1 == socket) {

        ov_log_error("failed to gather socket at interface %s in range %i:%i",
                     key,
                     container->core->ports.min,
                     container->core->ports.max);

        return true;
    }

    ov_ice_base *base = ov_ice_base_create(container->stream, socket);
    if (!base) {

        ov_log_error("failed to gather base at interface %s in range %i:%i",
                     key,
                     container->core->ports.min,
                     container->core->ports.max);

        return true;
    }

    return true;
}

/*----------------------------------------------------------------------------*/

static bool base_gather_remote(ov_ice *ice,
                               ov_ice_base *base,
                               const ov_ice_server *server) {

    if (!ice || !base || !server) goto error;

    ov_ice_candidate_type type = OV_ICE_INVALID;

    switch (server->type) {

        case OV_ICE_STUN_SERVER:

            type = OV_ICE_SERVER_REFLEXIVE;

            break;

        case OV_ICE_TURN_SERVER:

            type = OV_ICE_RELAYED;

            if ((0 == server->auth.user[0]) || (0 == server->auth.pass[0])) {

                ov_log_error("No auth data for TURN server, skipping %s",
                             server->socket.host);

                goto done;
            }

            break;

        default:
            ov_log_error("Failure in remote server config.");
            OV_ASSERT(1 == 0);
            goto error;
    }

    ov_socket_data socket_data = {0};

    int socket = ov_socket_create(server->socket, true, NULL);
    if (socket < 0) goto done;

    if (!ov_socket_get_data(socket, &socket_data, NULL)) {

        close(socket);
        goto error;
    }

    close(socket);

    if (base->local.data.sa.ss_family != socket_data.sa.ss_family) goto done;

    /* We found some base with the same socket type as the remote server */

    ov_ice_candidate *candidate = ov_ice_candidate_create(base->stream);
    if (!candidate) goto error;

    candidate->base = base;
    candidate->gathering = OV_ICE_GATHERING;
    candidate->server = *server;
    candidate->server.timer.keepalive = ice->config.limits.stun.keepalive_usecs;

    candidate->type = type;
    candidate->transport = UDP;
    candidate->component_id = 1;
    candidate->rport = base->local.data.port;
    memcpy(candidate->raddr, base->local.data.host, OV_HOST_NAME_MAX);
    ov_ice_candidate_calculate_priority(
        candidate, ov_ice_get_interfaces(base->stream->session->ice));

    ov_node_push((void **)&base->stream->candidates.local, candidate);
    ov_list_push(base->candidates, candidate);

    if (!ov_ice_session_set_foundation(base->stream->session, candidate))
        goto error;

    switch (server->type) {

        case OV_ICE_STUN_SERVER:

            if (!ov_ice_base_send_stun_binding_request(base, candidate)) {

                ov_ice_candidate_free(candidate);
                goto error;
            }

            break;

        case OV_ICE_TURN_SERVER:

            if (!ov_ice_base_send_turn_allocate_request(base, candidate)) {

                ov_ice_candidate_free(candidate);
                goto error;
            }

            break;

        default:
            ov_log_error("Failure in remote server config.");
            OV_ASSERT(1 == 0);
            goto error;
    }

done:
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool start_gather_remote_candidates(ov_ice *self,
                                           ov_ice_stream *stream) {

    if (!self || !stream) goto error;

    ov_ice_base *base = stream->bases;
    ov_ice_server *server = NULL;

    while (base) {

        server = self->server;
        while (server) {

            base_gather_remote(self, base, server);
            server = ov_node_next(server);
        }

        base = ov_node_next(base);
    }

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_ice_gather_candidates(ov_ice *self, ov_ice_stream *stream) {

    bool local_gathering = false;

    if (!self || !stream) goto error;

    if (self->autodiscovery) {

        local_gathering = core_gather_autodiscovery(self, stream);

    } else {

        struct container container = (struct container){

            .core = self, .stream = stream};

        local_gathering =
            ov_dict_for_each(self->interfaces, &container, gather_interface);
    }

    if (local_gathering) {

        start_gather_remote_candidates(self, stream);
    }

    return local_gathering;

error:
    return false;
}

/*----------------------------------------------------------------------------*/

static int interfaces_count(const ov_ice *core) {

    OV_ASSERT(core);
    if (!core) goto error;

    struct ifaddrs *ifaddr, *ifa;

    if (getifaddrs(&ifaddr) == -1) {
        ov_log_error("getifaddrs failed");
        goto error;
    }

    char hostname[OV_HOST_NAME_MAX + 1] = {0};
    int counter = 0;
    int n = 0;

    for (ifa = ifaddr, n = 0; ifa != NULL; ifa = ifa->ifa_next, n++) {

        if (ifa->ifa_addr == NULL) continue;

        memset(hostname, 0, OV_HOST_NAME_MAX);

        switch (ifa->ifa_addr->sa_family) {

            case AF_INET:

                if (!interface_permit_ipv4((struct sockaddr_in *)ifa->ifa_addr))
                    continue;

                if (0 != getnameinfo(ifa->ifa_addr,
                                     sizeof(struct sockaddr_in),
                                     hostname,
                                     OV_HOST_NAME_MAX,
                                     NULL,
                                     0,
                                     NI_NUMERICHOST)) {
                    continue;
                }

                counter++;
                break;

            case AF_INET6:

                if (!interface_permit_ipv6(
                        (struct sockaddr_in6 *)ifa->ifa_addr))
                    continue;

                if (0 != getnameinfo(ifa->ifa_addr,
                                     sizeof(struct sockaddr_in6),
                                     hostname,
                                     OV_HOST_NAME_MAX,
                                     NULL,
                                     0,
                                     NI_NUMERICHOST)) {
                    continue;
                }

                counter++;
                break;

            default:
                break;
        }
    }

    freeifaddrs(ifaddr);
    return counter;

error:
    return 0;
}

/*----------------------------------------------------------------------------*/

int ov_ice_get_interfaces(const ov_ice *self) {

    if (!self) goto error;

    if (self->autodiscovery) return interfaces_count(self);

    return ov_dict_count(self->interfaces);

error:
    return 0;
}

/*----------------------------------------------------------------------------*/

bool ov_ice_trickle_candidate(ov_ice *self,
                              const char *session_uuid,
                              const char *ufrag,
                              int stream_id,
                              const ov_ice_candidate *candidate) {

    if (!self || !session_uuid || !candidate) goto error;

    if (!self->config.callbacks.candidates.new) goto done;

    return self->config.callbacks.candidates.new(
        self->config.callbacks.userdata,
        session_uuid,
        ufrag,
        stream_id,
        *candidate);

done:
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_ice_trickle_end_of_candidates(ov_ice *self,
                                      const char *session_uuid,
                                      int stream_id) {

    if (!self || !session_uuid) goto error;

    if (!self->config.callbacks.candidates.end_of_candidates) goto done;

    return self->config.callbacks.candidates.end_of_candidates(
        self->config.callbacks.userdata, session_uuid, stream_id);
done:
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_ice_session_drop_callback(ov_ice *self, const char *session_uuid) {

    if (!self || !session_uuid) goto error;

    if (self->config.callbacks.session.drop)
        self->config.callbacks.session.drop(
            self->config.callbacks.userdata, session_uuid);

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

ov_ice_config ov_ice_get_config(const ov_ice *self) {

    if (!self) return (ov_ice_config){0};
    return self->config;
}
