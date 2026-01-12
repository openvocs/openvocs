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
        @file           ov_mc_interconnect_session.c
        @author         Markus

        @date           2023-12-13


        ------------------------------------------------------------------------
*/
#include "../include/ov_mc_interconnect_session.h"
#include "../include/ov_mc_interconnect_dtls_filter.h"

#include <limits.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <openssl/conf.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/ssl.h>

#include <ov_base/ov_convert.h>
#include <ov_base/ov_dict.h>
#include <ov_base/ov_id.h>
#include <ov_base/ov_rtp_frame.h>
#include <ov_base/ov_socket.h>
#include <ov_base/ov_string.h>

#include <ov_stun/ov_stun_binding.h>
#include <ov_stun/ov_stun_frame.h>

#define OV_MC_INTERCONNECT_SESSION_MAGIC_BYTES 0x5e55

static const char *label_extractor_srtp = "EXTRACTOR-dtls_srtp";

/*----------------------------------------------------------------------------*/

struct ov_mc_interconnect_session {

    uint16_t magic_bytes;
    ov_mc_interconnect_session_config config;

    ov_id id;

    ov_dict *ssrcs;
    ov_dict *loops;

    struct {

        ov_dtls_type type;
        ov_dtls *dtls;
        bool handshaked;

        SSL *ssl;
        SSL_CTX *ctx;

        BIO *read;
        BIO *write;

    } dtls;

    struct {

        char fingerprint[OV_DTLS_FINGERPRINT_MAX];

    } remote;

    struct {

        bool ready;
        char *profile;

        uint32_t key_len;
        uint32_t salt_len;

        struct {

            uint8_t key[OV_DTLS_KEY_MAX];
            uint8_t salt[OV_DTLS_SALT_MAX];

        } server;

        struct {

            uint8_t key[OV_DTLS_KEY_MAX];
            uint8_t salt[OV_DTLS_SALT_MAX];

        } client;

        struct {

            srtp_t session;
            srtp_policy_t policy;
            uint8_t key[OV_DTLS_KEY_MAX];

        } local;

        struct {

            srtp_policy_t policy;
            uint8_t key[OV_DTLS_KEY_MAX];

        } remote;

    } srtp;

    struct {

        uint32_t handshake;
        uint32_t keepalive;

    } timer;
};

/*----------------------------------------------------------------------------*/

static bool send_stun_binding_request(ov_mc_interconnect_session *session) {

    size_t size = OV_UDP_PAYLOAD_OCTETS;
    uint8_t buffer[size];
    memset(buffer, 0, size);

    uint8_t *ptr = buffer;
    uint8_t *nxt = NULL;

    uint8_t transaction_id[14] = {0};

    if (!session)
        goto error;

    int socket = ov_mc_interconnect_get_media_socket(session->config.base);
    ov_socket_data local = {0};
    ov_socket_get_data(socket, &local, NULL);

    if (!ov_stun_frame_generate_transaction_id(transaction_id))
        goto error;

    if (!ov_stun_generate_binding_request_plain(
            ptr, OV_UDP_PAYLOAD_OCTETS, &nxt, transaction_id,
            (uint8_t *)"ov_mc_interconnect", strlen("ov_mc_interconnect"),
            false)) {

        goto error;
    }

    struct sockaddr_storage dest = {0};

    if (!ov_socket_fill_sockaddr_storage(&dest, local.sa.ss_family,
                                         session->config.remote.media.host,
                                         session->config.remote.media.port))
        goto error;

    socklen_t len = sizeof(struct sockaddr_in);
    if (dest.ss_family == AF_INET6)
        len = sizeof(struct sockaddr_in6);

    ssize_t out =
        sendto(socket, ptr, nxt - ptr, 0, (struct sockaddr *)&dest, len);

    UNUSED(out);
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool send_keepalive(uint32_t timer, void *data) {

    UNUSED(timer);
    ov_mc_interconnect_session *self = ov_mc_interconnect_session_cast(data);
    if (!self)
        goto error;

    send_stun_binding_request(self);

    ov_log_debug("sending STUN keepalive for %s", self->id);

    self->timer.keepalive = ov_event_loop_timer_set(
        self->config.loop, self->config.keepalive_trigger_usec, self,
        send_keepalive);

    return true;
error:
    return false;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_mc_interconnect_session *
ov_mc_interconnect_session_create(ov_mc_interconnect_session_config config) {

    ov_mc_interconnect_session *self = NULL;

    if (!config.loop)
        goto error;
    if (!config.base)
        goto error;

    if (0 == config.reconnect_interval_usecs)
        config.reconnect_interval_usecs = 100000;

    if (0 == config.keepalive_trigger_usec)
        config.keepalive_trigger_usec = 300000000;

    self = calloc(1, sizeof(ov_mc_interconnect_session));
    if (!self)
        goto error;

    self->magic_bytes = OV_MC_INTERCONNECT_SESSION_MAGIC_BYTES;
    self->config = config;

    ov_id_fill_with_uuid(self->id);

    ov_dict_config d_config = ov_dict_intptr_key_config(255);
    d_config.value.data_function.free = ov_data_pointer_free;

    self->ssrcs = ov_dict_create(d_config);
    if (!self->ssrcs)
        goto error;

    d_config = ov_dict_string_key_config(255);
    d_config.value.data_function.free = NULL;

    self->loops = ov_dict_create(d_config);
    if (!self->loops)
        goto error;

    ov_log_debug("created session %s", self->id);

    if (!ov_mc_interconnect_register_session(self->config.base, self->id,
                                             self->config.remote.signaling,
                                             self->config.remote.media, self))
        goto error;

    self->timer.keepalive = ov_event_loop_timer_set(
        self->config.loop, self->config.keepalive_trigger_usec, self,
        send_keepalive);

    return self;
error:
    ov_mc_interconnect_session_free(self);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_mc_interconnect_session *
ov_mc_interconnect_session_free(ov_mc_interconnect_session *self) {

    if (!ov_mc_interconnect_session_cast(self))
        goto error;

    ov_log_debug("freeing session %s", self->id);

    ov_mc_interconnect_unregister_session(self->config.base,
                                          self->config.remote.signaling,
                                          self->config.remote.media);

    if (OV_TIMER_INVALID != self->timer.handshake) {
        ov_event_loop_timer_unset(self->config.loop, self->timer.handshake,
                                  NULL);
        self->timer.handshake = OV_TIMER_INVALID;
    }

    if (OV_TIMER_INVALID != self->timer.keepalive) {
        ov_event_loop_timer_unset(self->config.loop, self->timer.keepalive,
                                  NULL);
        self->timer.keepalive = OV_TIMER_INVALID;
    }

    self->ssrcs = ov_dict_free(self->ssrcs);
    self->loops = ov_dict_free(self->loops);

    if (self->dtls.ssl) {
        SSL_free(self->dtls.ssl);
        self->dtls.ssl = NULL;
    }

    if (self->dtls.ctx) {
        SSL_CTX_free(self->dtls.ctx);
        self->dtls.ctx = NULL;
    }

    self->srtp.profile = ov_data_pointer_free(self->srtp.profile);
    self = ov_data_pointer_free(self);
error:
    return self;
}

/*----------------------------------------------------------------------------*/

void *ov_mc_interconnect_session_free_void(void *self) {

    return ov_mc_interconnect_session_free(
        ov_mc_interconnect_session_cast(self));
}

/*----------------------------------------------------------------------------*/

ov_mc_interconnect_session *ov_mc_interconnect_session_cast(const void *data) {

    if (!data)
        return NULL;

    if (*(uint16_t *)data != OV_MC_INTERCONNECT_SESSION_MAGIC_BYTES)
        return NULL;

    return (ov_mc_interconnect_session *)data;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      #DTLS FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

bool ov_mc_interconnect_session_handshake_passive(
    ov_mc_interconnect_session *self, const uint8_t *buffer, size_t size) {

    int r = 0, n = 0;

    if (!self)
        goto error;

    ov_dtls *dtls = self->config.dtls;

    if (self->dtls.ssl) {
        SSL_free(self->dtls.ssl);
        self->dtls.ssl = NULL;
    }

    self->dtls.dtls = dtls;
    self->dtls.handshaked = false;
    self->dtls.type = OV_DTLS_PASSIVE;
    self->dtls.ssl = SSL_new(ov_dtls_get_ctx(dtls));

    if (!self->dtls.ssl)
        goto error;

    SSL_set_accept_state(self->dtls.ssl);

    r = SSL_set_tlsext_use_srtp(self->dtls.ssl, ov_dtls_get_srtp_profile(dtls));
    if (0 != r)
        goto error;

    self->dtls.read = BIO_new(BIO_s_mem());
    self->dtls.write = ov_mc_interconnect_dtls_bio_create(self);
    BIO_set_mem_eof_return(self->dtls.read, -1);
    BIO_set_mem_eof_return(self->dtls.write, -1);

    SSL_set_bio(self->dtls.ssl, self->dtls.read, self->dtls.write);
    SSL_set_options(self->dtls.ssl, SSL_OP_COOKIE_EXCHANGE);

    r = BIO_write(self->dtls.read, buffer, size);
    if (r < 0)
        goto error;

    BIO_ADDR *peer_bio = BIO_ADDR_new();

    r = DTLSv1_listen(self->dtls.ssl, peer_bio);

    if (peer_bio) {
        BIO_ADDR_free(peer_bio);
        peer_bio = NULL;
    }

    if (r >= 1) {

        self->dtls.handshaked = true;
        goto done;

    } else if (r == 0) {

        /*
         *      Non Fatal error, usercode is expected to
         *      retry operation. (man DTLSv1_listen)
         */

        self->dtls.dtls = NULL;

        SSL_free(self->dtls.ssl);
        self->dtls.ssl = NULL;

    } else {

        n = SSL_get_error(self->dtls.ssl, r);

        switch (n) {

        case SSL_ERROR_NONE:
        case SSL_ERROR_WANT_READ:
        case SSL_ERROR_WANT_CONNECT:
        case SSL_ERROR_WANT_ACCEPT:
        case SSL_ERROR_WANT_X509_LOOKUP:
        case SSL_ERROR_WANT_WRITE:
            break;

        case SSL_ERROR_ZERO_RETURN:
            // connection close
            // ov_log_debug("FD %i connection closed", socket);
            goto error;
            break;

        case SSL_ERROR_SYSCALL:
            goto error;

            break;

        case SSL_ERROR_SSL:
            goto error;

            break;

        default:

            goto error;
            break;
        }
    }

done:
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static int dummy_client_hello_cb(SSL *s, int *al, void *arg) {

    /*
     *      At some point @perform_ssl_client_handshake
     *      may stop with SSL_ERROR_WANT_CLIENT_HELLO_CB,
     *      so we add some dummy callback, to resume
     *      standard SSL operation.
     */
    if (!s)
        return SSL_CLIENT_HELLO_ERROR;

    if (al || arg) { /* ignore */
    };
    return SSL_CLIENT_HELLO_SUCCESS;
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

    if (!source)
        return NULL;

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
        if (i < length - 1)
            fingerprint[(i * 3) + 2] = ':';
    }

    return fingerprint;

error:
    if (fingerprint)
        free(fingerprint);
    return NULL;
}

/*----------------------------------------------------------------------------*/

static char *X509_fingerprint_create(const X509 *cert, ov_hash_function type) {

    unsigned char mdigest[EVP_MAX_MD_SIZE] = {0};
    unsigned int mdigest_size = 0;

    char *fingerprint = NULL;

    const EVP_MD *func = ov_hash_function_to_EVP(type);
    if (!func || !cert)
        return NULL;

    if (0 < X509_digest(cert, func, mdigest, &mdigest_size)) {
        fingerprint = fingerprint_format_RFC8122((char *)mdigest, mdigest_size);
    }

    return fingerprint;
}

/*----------------------------------------------------------------------------*/

static int check_cert(ov_mc_interconnect_session *self) {

    char hash_finger[OV_DTLS_FINGERPRINT_MAX] = {0};

    OV_ASSERT(self);
    OV_ASSERT(self->dtls.ssl);

    if (!self || !self->dtls.ssl)
        goto error;

    int r = 0;

    X509 *cert = SSL_get_peer_certificate(self->dtls.ssl);
    if (!cert) {

        ov_log_debug("get peer certificate failed for session");
        goto close;
    }

    r = SSL_get_verify_result(self->dtls.ssl);
    if (r != X509_V_OK) {

        ov_log_debug("Verify peer certificate failed for session");
        goto close;
    }

    char *finger = X509_fingerprint_create(cert, OV_HASH_SHA256);

    if (!finger) {

        ov_log_debug("Fingerprint peer certificate failed for session");
        X509_free(cert);
        goto close;
    }

    X509_free(cert);

    if (!snprintf(hash_finger, OV_DTLS_FINGERPRINT_MAX, "%s %s",
                  ov_hash_function_to_RFC8122_string(OV_HASH_SHA256), finger)) {

        finger = ov_data_pointer_free(finger);
        goto close;
    }

    finger = ov_data_pointer_free(finger);

    if (0 != strcmp(hash_finger, self->remote.fingerprint)) {

        ov_log_debug("Fingerprint verification failed for session");
        goto close;
    }

    return 1;

close:
    return 0;
error:
    return -1;
}

/*----------------------------------------------------------------------------*/

static bool perform_ssl_client_handshake_triggered(uint32_t id, void *userdata);

/*----------------------------------------------------------------------------*/

static bool perform_ssl_client_handshake(ov_mc_interconnect_session *self) {

    int r = 0, n = 0;

    bool shutdown = true;

    OV_ASSERT(self);
    if (!self)
        goto error;

    ov_event_loop *loop = self->config.loop;

    if (OV_TIMER_INVALID != self->timer.handshake) {

        loop->timer.unset(loop, self->timer.handshake, NULL);
        self->timer.handshake = OV_TIMER_INVALID;
    }

    r = SSL_connect(self->dtls.ssl);

    switch (r) {

    case 1:

        r = check_cert(self);
        switch (r) {

        case 1:
            goto success;

        case 0:
            goto close;
        default:
            OV_ASSERT(1 == 0);
            goto error;
        }

    default:

        n = SSL_get_error(self->dtls.ssl, r);
        switch (n) {

        case SSL_ERROR_NONE:
            OV_ASSERT(1 == 0);
            break;

        case SSL_ERROR_ZERO_RETURN:
            // ov_log_debug("SSL_ERROR_ZERO_RETURN");
            goto close;
            break;

        case SSL_ERROR_WANT_READ:
            // ov_log_debug("SSL_ERROR_WANT_READ");
            break;
        case SSL_ERROR_WANT_WRITE:
            // ov_log_debug("SSL_ERROR_WANT_WRITE");
            break;
        case SSL_ERROR_WANT_CONNECT:
            // ov_log_debug("SSL_ERROR_WANT_CONNECT");
            break;
        case SSL_ERROR_WANT_X509_LOOKUP:
            // ov_log_debug("SSL_ERROR_WANT_X509_LOOKUP");
            break;
        case SSL_ERROR_WANT_ASYNC:
            // ov_log_debug("SSL_ERROR_WANT_ASYNC");
            break;
        case SSL_ERROR_WANT_ASYNC_JOB:
            // ov_log_debug("SSL_ERROR_WANT_ASYNC_JOB");
            break;
        case SSL_ERROR_WANT_CLIENT_HELLO_CB:
            // ov_log_debug("SSL_ERROR_WANT_CLIENT_HELLO_CB");
            goto call_again_later;
            break;

        case SSL_ERROR_SYSCALL:
            goto close;
            break;

        case SSL_ERROR_SSL:
            shutdown = false;

            goto close;
            break;

        case SSL_ERROR_WANT_ACCEPT:
            break;
        }
        break;
    }

call_again_later:

    /*
     *      Handshake not successfull, but also not failed yet.
     *      Need to recall the function. This will be done
     *      via a timed callback.
     */

    self->timer.handshake =
        loop->timer.set(loop, self->config.reconnect_interval_usecs, self,
                        perform_ssl_client_handshake_triggered);

    if (OV_TIMER_INVALID == self->timer.handshake)
        goto error;

success:
    return true;

close:

    if (shutdown)
        SSL_shutdown(self->dtls.ssl);

error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool perform_ssl_client_handshake_triggered(uint32_t id, void *userdata) {

    if (!userdata)
        return false;

    ov_mc_interconnect_session *session =
        (ov_mc_interconnect_session *)(userdata);
    session->timer.handshake = OV_TIMER_INVALID;
    UNUSED(id);
    // handshake function MUST reset any timer if required
    return perform_ssl_client_handshake(session);
}

/*----------------------------------------------------------------------------*/

bool ov_mc_interconnect_session_handshake_active(
    ov_mc_interconnect_session *self, const char *fingerprint) {

    int r = 0;

    if (!self || !fingerprint)
        goto error;
    ov_dtls *dtls = self->config.dtls;

    memcpy(self->remote.fingerprint, fingerprint, OV_DTLS_FINGERPRINT_MAX);

    if (self->dtls.ssl) {

        SSL_free(self->dtls.ssl);
        self->dtls.ssl = NULL;
    }

    self->dtls.dtls = dtls;
    self->dtls.handshaked = false;
    self->dtls.type = OV_DTLS_ACTIVE;

    self->dtls.ctx = SSL_CTX_new(DTLS_client_method());
    if (!self->dtls.ctx)
        goto error;

    if (1 != SSL_CTX_set_min_proto_version(self->dtls.ctx, DTLS1_2_VERSION))
        goto error;

    // use default SSL verify
    SSL_CTX_set_verify(self->dtls.ctx, SSL_VERIFY_PEER, NULL);

    // load verify data path from ov_ice_ssl config

    const char *file = ov_dtls_get_verify_file(dtls);
    const char *path = ov_dtls_get_verify_path(dtls);

    if (file || path) {

        if (0 == ov_string_len(file))
            file = NULL;
        if (0 == ov_string_len(path))
            path = NULL;

        r = SSL_CTX_load_verify_locations(self->dtls.ctx, file, path);

        if (r != 1) {
            goto error;
        }
    }

    // create the DTLS session context
    self->dtls.ssl = SSL_new(self->dtls.ctx);
    if (!self->dtls.ssl)
        goto error;

    r = SSL_set_tlsext_use_srtp(self->dtls.ssl, ov_dtls_get_srtp_profile(dtls));
    if (0 != r)
        goto error;

    self->dtls.read = BIO_new(BIO_s_mem());
    self->dtls.write = ov_mc_interconnect_dtls_bio_create(self);
    BIO_set_mem_eof_return(self->dtls.read, -1);
    BIO_set_mem_eof_return(self->dtls.write, -1);

    SSL_set_bio(self->dtls.ssl, self->dtls.read, self->dtls.write);

    SSL_set_options(self->dtls.ssl, SSL_OP_COOKIE_EXCHANGE);
    SSL_set_connect_state(self->dtls.ssl);
    SSL_CTX_set_client_hello_cb(self->dtls.ctx, dummy_client_hello_cb, NULL);

    ov_event_loop *loop = self->config.loop;
    self->timer.handshake =
        loop->timer.set(loop, self->config.reconnect_interval_usecs, self,
                        perform_ssl_client_handshake_triggered);

    if (OV_TIMER_INVALID == self->timer.handshake)
        goto error;

    return true;
error:
    return false;
}

/*---------------------------------------------------------------------------*/

static bool
srtp_get_key_length_of_profile(const SRTP_PROTECTION_PROFILE *profile,
                               uint32_t *keylen, uint32_t *saltlen) {

    if (!profile || !keylen || !saltlen)
        goto error;

    if (!profile->id)
        goto error;

    switch (profile->id) {

    case SRTP_AES128_CM_SHA1_80:
        *keylen = 16;
        *saltlen = 14;
        break;

    case SRTP_AES128_CM_SHA1_32:
        *keylen = 16;
        *saltlen = 14;
        break;

    case SRTP_AEAD_AES_128_GCM:
        *keylen = 16;
        *saltlen = 12;
        break;

    case SRTP_AEAD_AES_256_GCM:
        *keylen = 32;
        *saltlen = 12;
        break;

    default:
        goto error;
    }

    return true;
error:
    return false;
}

/*---------------------------------------------------------------------------*/

static const char *ov_mc_interconnect_session_get_srtp_keys(
    ov_mc_interconnect_session *self, uint32_t *key_len, uint32_t *salt_len,
    uint8_t *server_key, uint8_t *server_salt, uint8_t *client_key,
    uint8_t *client_salt) {

    size_t size = 2 * OV_DTLS_KEY_MAX + 2 * OV_DTLS_SALT_MAX + 1;

    uint8_t buffer[size];
    memset(buffer, 0, size);

    if (!self || !self->dtls.ssl || !key_len || !salt_len || !server_key ||
        !server_salt || !client_key || !client_salt)
        goto error;

    uint32_t keylen = 0;
    uint32_t saltlen = 0;

    SRTP_PROTECTION_PROFILE *profile =
        SSL_get_selected_srtp_profile(self->dtls.ssl);

    if (!srtp_get_key_length_of_profile(profile, &keylen, &saltlen))
        goto error;

    if (keylen > *key_len)
        goto error;

    if (saltlen > *salt_len)
        goto error;

    /*      The total length of keying material obtained
            should be equal to two times the sum of
            the master key length and the salt length
            as defined for the protection profile in use.

            This provides the client write master key,
            the server write master key,
            the client write master salt and
            the server write master salt in that order.

    */

    if (1 != SSL_export_keying_material(
                 self->dtls.ssl, buffer, size, label_extractor_srtp,
                 strlen(label_extractor_srtp), NULL, 0, 0))
        goto error;

    *key_len = keylen;
    *salt_len = saltlen;

    uint8_t *ptr = buffer;

    if (!memcpy(client_key, ptr, keylen))
        return false;
    ptr += keylen;
    if (!memcpy(server_key, ptr, keylen))
        return false;
    ptr += keylen;
    if (!memcpy(client_salt, ptr, saltlen))
        return false;
    ptr += saltlen;
    if (!memcpy(server_salt, ptr, saltlen))
        return false;
    ptr += saltlen;

    return profile->name;
error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

static bool srtp_unset_data(ov_mc_interconnect_session *self) {

    if (!self)
        return false;

    self->srtp.ready = false;
    self->srtp.profile = ov_data_pointer_free(self->srtp.profile);
    self->srtp.key_len = OV_DTLS_KEY_MAX;
    self->srtp.salt_len = OV_DTLS_SALT_MAX;

    memset(self->srtp.server.key, 0, OV_DTLS_KEY_MAX);
    memset(self->srtp.client.key, 0, OV_DTLS_KEY_MAX);
    memset(self->srtp.server.salt, 0, OV_DTLS_SALT_MAX);
    memset(self->srtp.client.salt, 0, OV_DTLS_SALT_MAX);

    return true;
}

/*----------------------------------------------------------------------------*/

static bool add_srtp_stream(const void *key, void *val, void *data) {

    if (!key)
        return true;
    uint32_t remote_ssrc = (uint32_t)(intptr_t)key;
    const char *name = (const char *)val;
    ov_mc_interconnect_session *self = ov_mc_interconnect_session_cast(data);

    if (!self || !name)
        goto error;

    ov_mc_interconnect_loop *loop =
        ov_mc_interconnect_get_loop(self->config.base, name);

    if (!loop)
        goto done;

    uint32_t local_ssrc = ov_mc_interconnect_loop_get_ssrc(loop);
    if (0 == local_ssrc)
        goto done;

    srtp_t srtp_session = self->srtp.local.session;

    self->srtp.local.policy.ssrc.type = ssrc_any_inbound;
    self->srtp.local.policy.ssrc.value = remote_ssrc;
    self->srtp.local.policy.key = self->srtp.local.key;
    self->srtp.local.policy.next = NULL;

    self->srtp.remote.policy.ssrc.type = ssrc_any_inbound;
    self->srtp.remote.policy.ssrc.value = local_ssrc;
    self->srtp.remote.policy.key = self->srtp.remote.key;
    self->srtp.remote.policy.next = NULL;

    int r = srtp_add_stream(srtp_session, &self->srtp.local.policy);

    switch (r) {

    case srtp_err_status_ok:
        break;

    default:
        goto done;
        break;
    }

    r = srtp_add_stream(srtp_session, &self->srtp.remote.policy);

    switch (r) {

    case srtp_err_status_ok:
        break;

    default:
        goto done;
        break;
    }

done:
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool prepare_streams(ov_mc_interconnect_session *session) {

    if (!session)
        goto error;

    srtp_t srtp_session = session->srtp.local.session;
    if (!srtp_session)
        goto error;

    /* Step 1 - prepare policy */

    if (!session->srtp.profile)
        goto error;

    srtp_crypto_policy_set_rtp_default(&session->srtp.local.policy.rtp);
    srtp_crypto_policy_set_rtcp_default(&session->srtp.local.policy.rtcp);

    srtp_crypto_policy_set_rtp_default(&session->srtp.remote.policy.rtp);
    srtp_crypto_policy_set_rtcp_default(&session->srtp.remote.policy.rtcp);

    if (0 ==
        ov_string_compare("SRTP_AES128_CM_SHA1_80", session->srtp.profile)) {

        srtp_crypto_policy_set_aes_cm_128_hmac_sha1_80(
            &session->srtp.local.policy.rtp);
        srtp_crypto_policy_set_aes_cm_128_hmac_sha1_80(
            &session->srtp.local.policy.rtcp);

        srtp_crypto_policy_set_aes_cm_128_hmac_sha1_80(
            &session->srtp.remote.policy.rtp);
        srtp_crypto_policy_set_aes_cm_128_hmac_sha1_80(
            &session->srtp.remote.policy.rtcp);

    } else if (0 == ov_string_compare("SRTP_AES128_CM_SHA1_32",
                                      session->srtp.profile)) {

        srtp_crypto_policy_set_aes_cm_128_hmac_sha1_32(
            &session->srtp.local.policy.rtp);
        srtp_crypto_policy_set_aes_cm_128_hmac_sha1_32(
            &session->srtp.local.policy.rtcp);

        srtp_crypto_policy_set_aes_cm_128_hmac_sha1_80(
            &session->srtp.remote.policy.rtp);
        srtp_crypto_policy_set_aes_cm_128_hmac_sha1_80(
            &session->srtp.remote.policy.rtcp);

    } else {

        goto done;
    }

    /* Step 2 - prepare keys */

    memset(session->srtp.local.key, 0x00, OV_DTLS_KEY_MAX);
    memset(session->srtp.remote.key, 0x00, OV_DTLS_KEY_MAX);

    switch (session->dtls.type) {

    case OV_DTLS_ACTIVE:

        /* Local policy is client policy,
         * incoming streams will use the server key. */

        memcpy(session->srtp.local.key, session->srtp.server.key,
               session->srtp.key_len);

        memcpy(session->srtp.remote.key, session->srtp.client.key,
               session->srtp.key_len);

        srtp_append_salt_to_key(session->srtp.local.key, session->srtp.key_len,
                                session->srtp.server.salt,
                                session->srtp.salt_len);

        srtp_append_salt_to_key(session->srtp.remote.key, session->srtp.key_len,
                                session->srtp.client.salt,
                                session->srtp.salt_len);

        break;

    case OV_DTLS_PASSIVE:

        /* Local policy is server policy,
         * incoming streams will use the client key. */

        memcpy(session->srtp.local.key, session->srtp.client.key,
               session->srtp.key_len);

        memcpy(session->srtp.remote.key, session->srtp.server.key,
               session->srtp.key_len);

        srtp_append_salt_to_key(session->srtp.local.key, session->srtp.key_len,
                                session->srtp.client.salt,
                                session->srtp.salt_len);

        srtp_append_salt_to_key(session->srtp.remote.key, session->srtp.key_len,
                                session->srtp.server.salt,
                                session->srtp.salt_len);

        break;

        break;
    default:
        goto error;
    }

    /* Step 3 prepare streams */

    if (!ov_dict_for_each(session->ssrcs, session, add_srtp_stream))
        goto error;

done:
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_mc_interconnect_session_dtls_io(ov_mc_interconnect_session *self,
                                        const uint8_t *buffer, size_t size) {

    char buf[OV_DTLS_SSL_BUFFER_SIZE] = {0};

    int r = 0;
    ssize_t out = 0;

    if (!self || !buffer || size < 1)
        goto error;

    r = BIO_write(self->dtls.read, buffer, size);
    if (r < 0)
        goto error;

    if (!SSL_is_init_finished(self->dtls.ssl)) {

        switch (self->dtls.type) {

        case OV_DTLS_PASSIVE:

            r = SSL_do_handshake(self->dtls.ssl);

            if (SSL_is_init_finished(self->dtls.ssl)) {
                self->dtls.handshaked = true;
            }

            break;

        case OV_DTLS_ACTIVE:
            perform_ssl_client_handshake(self);
            break;
        }

        srtp_unset_data(self);

        const char *profile = ov_mc_interconnect_session_get_srtp_keys(
            self, &self->srtp.key_len, &self->srtp.salt_len,
            self->srtp.server.key, self->srtp.server.salt,
            self->srtp.client.key, self->srtp.client.salt);

        if (profile) {

            self->srtp.ready = true;
            self->srtp.profile = ov_data_pointer_free(self->srtp.profile);
            self->srtp.profile = strdup(profile);

            if (!self->srtp.local.session) {

                srtp_err_status_t r =
                    srtp_create(&self->srtp.local.session, NULL);

                switch (r) {

                case srtp_err_status_ok:
                    ov_log_debug("Session %s SRTP READY", self->id);
                    prepare_streams(self);
                    ov_mc_interconnect_srtp_ready(self->config.base, self);
                    break;

                default:
                    ov_log_error("Session %s srtp failed %i", self->id, r);
                    goto error;
                }
            }
        }

    } else {

        out = SSL_read(self->dtls.ssl, buf, OV_DTLS_SSL_BUFFER_SIZE);
        UNUSED(out);
    }

    return true;

error:
    return false;
}

/*----------------------------------------------------------------------------*/

int ov_mc_interconnect_session_get_signaling_socket(
    ov_mc_interconnect_session *self) {

    if (!self)
        return -1;
    return self->config.signaling;
}

/*----------------------------------------------------------------------------*/

int ov_mc_interconnect_session_send(ov_mc_interconnect_session *self,
                                    const uint8_t *buffer, size_t size) {

    if (!self || !buffer || !size)
        goto error;

    int socket = ov_mc_interconnect_get_media_socket(self->config.base);
    if (-1 == socket)
        goto error;

    int type = AF_INET;
    if (!ov_socket_destination_address_type(self->config.remote.media.host,
                                            &type))
        goto error;

    struct sockaddr_storage dest = {0};

    if (!ov_socket_fill_sockaddr_storage(&dest, type,
                                         self->config.remote.media.host,
                                         self->config.remote.media.port))
        goto error;

    socklen_t len = sizeof(struct sockaddr_in);
    if (dest.ss_family == AF_INET6)
        len = sizeof(struct sockaddr_in6);

    int out = sendto(socket, buffer, size, 0, (struct sockaddr *)&dest, len);

    return out;
error:
    return -1;
}

/*----------------------------------------------------------------------------*/

ov_dtls *ov_mc_interconnect_session_get_dtls(ov_mc_interconnect_session *self) {

    if (!self)
        return NULL;
    return self->dtls.dtls;
}

/*----------------------------------------------------------------------------*/

bool ov_mc_interconnect_session_add(ov_mc_interconnect_session *self,
                                    const char *loop_name,
                                    uint32_t remote_ssrc) {

    if (!self || !loop_name || !remote_ssrc)
        goto error;

    uintptr_t key_ssrc = remote_ssrc;
    char *key_loop = ov_string_dup(loop_name);

    if (!ov_dict_set(self->ssrcs, (void *)key_ssrc, key_loop, NULL)) {
        key_loop = ov_data_pointer_free(key_loop);
        goto error;
    }

    key_loop = ov_string_dup(loop_name);
    if (!ov_dict_set(self->loops, key_loop, (void *)key_ssrc, NULL)) {
        key_loop = ov_data_pointer_free(key_loop);
        goto error;
    }

    /* ADD SSRC STREAM FOR LOOP */

    ov_mc_interconnect_loop *loop =
        ov_mc_interconnect_get_loop(self->config.base, loop_name);

    if (!loop)
        goto done;

    uint32_t local_ssrc = ov_mc_interconnect_loop_get_ssrc(loop);
    if (0 == local_ssrc)
        goto done;

    srtp_t srtp_session = self->srtp.local.session;

    self->srtp.local.policy.ssrc.type = ssrc_specific;
    self->srtp.local.policy.ssrc.value = remote_ssrc;
    self->srtp.local.policy.key = self->srtp.local.key;
    self->srtp.local.policy.next = NULL;

    self->srtp.remote.policy.ssrc.type = ssrc_specific;
    self->srtp.remote.policy.ssrc.value = local_ssrc;
    self->srtp.remote.policy.key = self->srtp.remote.key;
    self->srtp.remote.policy.next = NULL;

    int r = srtp_add_stream(srtp_session, &self->srtp.local.policy);

    switch (r) {

    case srtp_err_status_ok:
        ov_log_debug("add srtp_stream local policy %s", loop_name);
        break;

    default:
        ov_log_error("Failed to add srtp_stream local policy %s", loop_name);
        break;
    }

    r = srtp_add_stream(srtp_session, &self->srtp.remote.policy);

    switch (r) {

    case srtp_err_status_ok:
        ov_log_debug("add srtp_stream remote policy %s", loop_name);
        break;

    default:
        ov_log_error("Failed to add srtp_stream remote policy %s", loop_name);
        break;
    }

done:

    ov_log_debug("%s added loop %s|%" PRIu32, self->id, loop_name, remote_ssrc);
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_mc_interconnect_session_remove(ov_mc_interconnect_session *self,
                                       const char *loop) {

    if (!self || !loop)
        goto error;

    intptr_t key_ssrc = (intptr_t)ov_dict_get(self->loops, loop);
    ov_dict_del(self->loops, loop);
    ov_dict_del(self->ssrcs, (void *)key_ssrc);

    ov_log_debug("%s removed loop %s", self->id, loop);
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/
/*
static bool remove_and_readd_srtp_stream(ov_mc_interconnect_session *self) {

  if (!self)
    goto error;

  srtp_err_status_t r = srtp_remove_stream(self->srtp.local.session,
                                           self->srtp.local.policy.ssrc.value);

  switch (r) {

  case srtp_err_status_ok:
    break;

  default:
    break;
  }

  r = srtp_remove_stream(self->srtp.local.session,
                         self->srtp.remote.policy.ssrc.value);

  switch (r) {

  case srtp_err_status_ok:
    break;

  default:
    break;
  }

  r = srtp_add_stream(self->srtp.local.session, &self->srtp.local.policy);

  switch (r) {

  case srtp_err_status_ok:
    break;

  default:
    break;
  }

  r = srtp_add_stream(self->srtp.local.session, &self->srtp.remote.policy);

  switch (r) {

  case srtp_err_status_ok:
    break;

  default:
    break;
  }

  return true;
error:
  return false;
}
*/

/*----------------------------------------------------------------------------*/

bool ov_mc_interconnect_session_forward_rtp_external_to_internal(
    ov_mc_interconnect_session *self, uint8_t *buffer, size_t size,
    const ov_socket_data *remote) {

    /* Remote is external media remote */

    ov_rtp_frame *frame = NULL;

    if (!self || !buffer || !size || !remote)
        goto error;

    frame = ov_rtp_frame_decode(buffer, size);
    if (!frame){
        ov_log_error("Not a RTP frame.");
        goto ignore;
    }

    switch (buffer[1]) {

    case 200:
        // ov_log_debug("RTCP sender report received - ignoring");
        goto ignore;

    case 201:
        // ov_log_debug("RTCP receiver report received - ignoring");
        goto ignore;

    case 202:
        // ov_log_debug("RTCP SDES received - ignoring");
        goto ignore;

    case 203:
        // ov_log_debug("RTCP GOOD BYE received - ignoring");
        goto ignore;

    case 204:
        // ov_log_debug("RTCP APP DATA received - ignoring");
        goto ignore;

    default:
        break;
    }

    int l = size;

    srtp_t srtp_session = self->srtp.local.session;

    if (!srtp_session){
        ov_log_error("Could not get SRTP session.");
        goto error;
    }
    /*
    if (!remove_and_readd_srtp_stream(self))
      goto error;
    */
    /*
      srtp_err_status_t r = srtp_unprotect(srtp_session, buffer, &l);

      switch (r) {

      case srtp_err_status_ok:
        // ov_log_debug("SRTP unprotect success");
        break;

      default:
        // ov_log_error("SRTP unprotect error");
        goto ignore;
        break;
      }
    */

    char *loop_name =
        ov_dict_get(self->ssrcs, (void *)(uintptr_t)frame->expanded.ssrc);

    if (!loop_name){
        ov_log_error("Could not find loopname.");
        goto ignore;
    }

    ov_mc_interconnect_loop *loop =
        ov_mc_interconnect_get_loop(self->config.base, loop_name);

    if (!loop){
        ov_log_error("Could not find loop");
        goto ignore;
    }

    uint32_t ssrc_to_set = ov_mc_interconnect_loop_get_ssrc(loop);

    /* set SSRC to internal SSRC */
    uint32_t u32 = htonl(ssrc_to_set);
    memcpy(buffer + 8, &u32, 4);

    if (!ov_mc_interconnect_loop_send(loop, buffer, l)){
        ov_log_error("Could not send at loop %s", loop_name);
        goto ignore;
    }
/*
    ov_log_debug("%s to internal RTP send %zi bytes for %s",
        self->id,
        l,
        loop_name);
*/
ignore:
    frame = ov_rtp_frame_free(frame);
    return true;
error:
    frame = ov_rtp_frame_free(frame);
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_mc_interconnect_session_forward_multicast_to_external(
    ov_mc_interconnect_session *self, ov_mc_interconnect_loop *loop,
    uint8_t *buffer, size_t size) {

    if (!self || !loop || !buffer || !size)
        goto error;

    const char *name = ov_mc_interconnect_loop_get_name(loop);

    /* (1) check if the session is interessted in the loop */
    uintptr_t ssrc_remote = (uintptr_t)ov_dict_get(self->loops, name);
    if (ssrc_remote < 1)
        goto done;

    /* (2) Get remote data to be used */

    uint32_t ssrc_to_set = ov_mc_interconnect_loop_get_ssrc(loop);

    /* (3) set ssrc and payload type*/
    uint32_t u32 = htonl(ssrc_to_set);
    memcpy(buffer + 8, &u32, 4);

    // we set payload type to 100
    buffer[1] = 0x80 & buffer[1];
    buffer[1] |= 0X64;

    int out = size;
    /*
    if (!remove_and_readd_srtp_stream(self))
      goto error;
    */

    /*
    srtp_err_status_t r = srtp_protect(self->srtp.local.session, buffer, &out);

    switch (r) {

    case srtp_err_status_ok:
      break;

    default:
      ov_log_error("SRTP protect error %i for %s cannot send.", r , name);
      goto done;
      break;
    }
    */

    ssize_t bytes = ov_mc_interconnect_session_send(self, buffer, out);
    UNUSED(bytes);
/*
    ov_log_debug("%s to %s external SRTP send %zi bytes for %s to %s:%i",
        self->id,
        name,
        bytes,
        name,
        self->config.remote.media.host,
        self->config.remote.media.port);
*/
done:
    return true;
error:
    return false;
}
