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
        @file           ov_ice_pair.c
        @author         Markus

        @date           2024-09-18


        ------------------------------------------------------------------------
*/
#include "../include/ov_ice_pair.h"

#include <openssl/conf.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/ssl.h>

#include <ov_stun/ov_stun_attributes_rfc5245.h> // RFC ICE
#include <ov_stun/ov_stun_attributes_rfc5389.h> // RFC STUN
#include <ov_stun/ov_stun_attributes_rfc8489.h>
#include <ov_stun/ov_stun_attributes_rfc8656.h>
#include <ov_stun/ov_stun_binding.h>
#include <ov_stun/ov_stun_frame.h>
#include <ov_stun/ov_turn_allocate.h>
#include <ov_stun/ov_turn_channel_bind.h>
#include <ov_stun/ov_turn_create_permission.h>
#include <ov_stun/ov_turn_data.h>
#include <ov_stun/ov_turn_permission.h>
#include <ov_stun/ov_turn_refresh.h>
#include <ov_stun/ov_turn_send.h>

#include <ov_encryption/ov_hash.h>

static const char *label_extractor_srtp = "EXTRACTOR-dtls_srtp";

#define OV_ICE_SSL_ERROR_STRING_BUFFER_SIZE 200

bool ov_ice_pair_calculate_priority(ov_ice_pair *pair,
                                    ov_ice_session *session) {

    if (!pair || !session)
        return false;

    uint32_t g = 0;
    uint32_t d = 0;
    uint32_t min = 0;
    uint32_t max = 0;
    uint32_t add = 0;

    if (!session->controlling) {

        g = pair->remote->priority;
        d = pair->local->priority;

    } else {

        d = pair->remote->priority;
        g = pair->local->priority;
    }

    min = d;
    max = d;
    add = 0;

    if (g > d) {

        add = 1;
        max = g;
        min = d;

    } else {

        max = d;
        min = g;
    }

    pair->priority = (0x100000000 * min) + (2 * max) + add;
    return true;
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

static bool create_turn_permission(ov_ice_pair *pair);

/*----------------------------------------------------------------------------*/

static bool renew_permission(uint32_t timer, void *data) {

    ov_ice_pair *pair = (ov_ice_pair *)data;
    OV_ASSERT(pair->timer.permission_renew == timer);
    pair->timer.permission_renew = OV_TIMER_INVALID;

    return create_turn_permission(pair);
}

/*----------------------------------------------------------------------------*/

static bool create_turn_permission(ov_ice_pair *pair) {

    uint8_t buffer[OV_UDP_PAYLOAD_OCTETS] = {0};
    uint8_t *next = NULL;

    if (!pair)
        goto error;
    if (!pair->local)
        goto error;
    if (pair->local->type != OV_ICE_RELAYED)
        goto error;

    ov_ice *ice = pair->local->base->stream->session->ice;
    ov_event_loop *loop = ov_ice_get_event_loop(ice);

    const ov_ice_candidate *local = pair->local;
    const ov_ice_candidate *remote = pair->remote;

    if (!local->turn.nonce || !local->turn.realm)
        goto timer_set;

    struct sockaddr_storage address = {0};

    if (!ov_socket_fill_sockaddr_storage(&address,
                                         local->base->local.data.sa.ss_family,
                                         remote->addr, remote->port))
        goto error;

    if (!ov_ice_transaction_create(ice, pair->transaction_id, 13, pair))
        goto error;

    if (!ov_turn_create_permission(
            buffer, OV_UDP_PAYLOAD_OCTETS, &next, pair->transaction_id, NULL, 0,
            (uint8_t *)local->server.auth.user, strlen(local->server.auth.user),
            (uint8_t *)local->turn.realm, strlen(local->turn.realm),
            (uint8_t *)local->turn.nonce, strlen(local->turn.nonce),
            (uint8_t *)local->server.auth.pass, strlen(local->server.auth.pass),
            &address, true))
        goto error;

    struct sockaddr_storage dest = {0};

    if (!ov_socket_fill_sockaddr_storage(
            &dest, local->base->local.data.sa.ss_family,
            local->server.socket.host, local->server.socket.port))
        goto error;

    socklen_t len = sizeof(struct sockaddr_in);
    if (dest.ss_family == AF_INET6)
        len = sizeof(struct sockaddr_in6);

    ssize_t out = sendto(local->base->socket, buffer, next - buffer, 0,
                         (struct sockaddr *)&dest, len);

    if (out <= 0) {

        ov_ice_transaction_unset(ice, pair->transaction_id);
    }

timer_set:
    pair->timer.permission_renew =
        loop->timer.set(loop, 4 * 1000 * 1000, pair, renew_permission);

    if (OV_TIMER_INVALID == pair->timer.permission_renew)
        pair->state = OV_ICE_PAIR_FAILED;

    return true;

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

ov_ice_pair *ov_ice_pair_create(ov_ice_stream *stream,
                                const ov_ice_candidate *local,
                                const ov_ice_candidate *remote) {

    ov_ice_pair *pair = NULL;

    if (!stream || !local || !remote)
        goto error;

    ov_socket_configuration config = (ov_socket_configuration){
        .type = remote->transport, .port = remote->port};

    memcpy(config.host, remote->addr, OV_HOST_NAME_MAX);

    int socket = ov_socket_create(config, true, NULL);
    if (socket < 0)
        goto error;
    if (!ov_sockets_are_similar(socket, local->base->socket)) {
        close(socket);
        goto error;
    }
    close(socket);

    pair = calloc(1, sizeof(ov_ice_pair));
    pair->node.type = OV_ICE_PAIR_MAGIC_BYTES;

    pair->state = OV_ICE_PAIR_FROZEN;
    pair->stream = stream;
    pair->local = local;
    pair->remote = remote;
    pair->priority = 0;
    pair->success_count = 0;

    if (!ov_ice_pair_calculate_priority(pair, stream->session))
        goto error;

    switch (local->type) {

    case OV_ICE_RELAYED:

        // create permission for remote at TURN server
        create_turn_permission(pair);

        break;

    default:
        break;
    }

    if (!ov_node_push((void **)&stream->pairs, pair))
        goto error;

    ov_log_info("ICE session %s|%i created pair %s:%i remote %s:%i %s:%i %s",
                stream->session->uuid, stream->index, pair->local->addr,
                pair->local->port, pair->remote->addr, pair->remote->port,
                pair->remote->raddr, pair->remote->rport,
                ov_ice_candidate_type_to_string(pair->local->type));

    return pair;

error:
    ov_ice_pair_free(pair);
    return NULL;
}

/*----------------------------------------------------------------------------*/

const char *ov_ice_pair_state_to_string(ov_ice_pair_state state) {

    switch (state) {

    case OV_ICE_PAIR_FROZEN:
        return OV_KEY_FROZEN;

    case OV_ICE_PAIR_WAITING:
        return OV_KEY_WAITING;

    case OV_ICE_PAIR_PROGRESS:
        return OV_KEY_PROGRESS;

    case OV_ICE_PAIR_SUCCESS:
        return OV_KEY_SUCCESS;

    case OV_ICE_PAIR_FAILED:
        return OV_KEY_FAILED;
    }

    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_ice_pair *ov_ice_pair_cast(const void *data) {

    if (!data)
        goto error;

    ov_node *node = (ov_node *)data;

    if (node->type == OV_ICE_PAIR_MAGIC_BYTES)
        return (ov_ice_pair *)data;
error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

void *ov_ice_pair_free(void *self) {

    ov_ice_pair *pair = ov_ice_pair_cast(self);
    if (!pair)
        return self;

    pair->srtp.profile = ov_data_pointer_free(pair->srtp.profile);

    if (pair->stream) {

        ov_node_remove_if_included((void **)&pair->stream->pairs, pair);
        ov_list_remove_if_included(pair->stream->valid, pair);
        ov_list_remove_if_included(pair->stream->trigger, pair);
        if (pair == pair->stream->selected)
            pair->stream->selected = NULL;

        ov_event_loop *loop = ov_ice_get_event_loop(pair->stream->session->ice);

        if (OV_TIMER_INVALID != pair->timer.permission_renew) {

            loop->timer.unset(loop, pair->timer.permission_renew, NULL);
            pair->timer.permission_renew = OV_TIMER_INVALID;
        }

        if (OV_TIMER_INVALID != pair->timer.handshake) {

            loop->timer.unset(loop, pair->timer.handshake, NULL);
            pair->timer.handshake = OV_TIMER_INVALID;
        }

        if (OV_TIMER_INVALID != pair->timer.keepalive) {

            loop->timer.unset(loop, pair->timer.keepalive, NULL);
            pair->timer.keepalive = OV_TIMER_INVALID;
        }
    }

    if (pair->dtls.ssl) {

        SSL_free(pair->dtls.ssl);
        pair->dtls.ssl = NULL;
    }

    if (pair->dtls.ctx) {
        SSL_CTX_free(pair->dtls.ctx);
        pair->dtls.ctx = NULL;
    }

    pair = ov_data_pointer_free(pair);
    return NULL;
}

/*----------------------------------------------------------------------------*/

static size_t pair_send_turn(const ov_ice_pair *pair, const uint8_t *input,
                             size_t size) {

    uint8_t buffer[OV_UDP_PAYLOAD_OCTETS] = {0};
    uint8_t transaction_id[OV_UDP_PAYLOAD_OCTETS] = {0};

    if (!pair || !input || (size < 1))
        goto error;

    ov_ice_base *base = pair->local->base;

    OV_ASSERT(base);
    OV_ASSERT(pair->stream);
    OV_ASSERT(pair->stream->session);
    OV_ASSERT(pair->stream->session->ice);

    ov_ice *ice = pair->stream->session->ice;

    struct sockaddr_storage dest = {0};
    struct sockaddr_storage turn = {0};

    if (!ov_socket_fill_sockaddr_storage(&turn, base->local.data.sa.ss_family,
                                         pair->local->server.socket.host,
                                         pair->local->server.socket.port))
        goto error;

    if (!ov_socket_fill_sockaddr_storage(&dest, base->local.data.sa.ss_family,
                                         pair->remote->addr,
                                         pair->remote->port))
        goto error;

    socklen_t len = sizeof(struct sockaddr_in);
    if (turn.ss_family == AF_INET6)
        len = sizeof(struct sockaddr_in6);

    uint8_t *next = NULL;

    if (!ov_ice_transaction_create(ice, transaction_id, 13, NULL))
        goto error;

    if (!ov_turn_create_send(buffer, OV_UDP_PAYLOAD_OCTETS, &next,
                             transaction_id, &dest, input, size, false))
        goto error;

    ssize_t out = sendto(base->socket, buffer, next - buffer, 0,
                         (struct sockaddr *)&turn, len);

    return out;

error:
    return -1;
}

/*----------------------------------------------------------------------------*/

ssize_t ov_ice_pair_send(ov_ice_pair *pair, const uint8_t *buffer,
                         size_t size) {

    if (!pair || !buffer || !size)
        goto error;

    if (!pair->local)
        goto error;
    if (!pair->remote)
        goto error;

    if (OV_ICE_TURN_SERVER == pair->local->server.type)
        return pair_send_turn(pair, buffer, size);

    ov_ice_base *base = pair->local->base;
    OV_ASSERT(base);

    struct sockaddr_storage dest = {0};

    if (!ov_socket_fill_sockaddr_storage(&dest, base->local.data.sa.ss_family,
                                         pair->remote->addr,
                                         pair->remote->port))
        goto error;

    socklen_t len = sizeof(struct sockaddr_in);
    if (dest.ss_family == AF_INET6)
        len = sizeof(struct sockaddr_in6);

    ssize_t out =
        sendto(base->socket, buffer, size, 0, (struct sockaddr *)&dest, len);

    return out;

error:
    return -1;
}

/*----------------------------------------------------------------------------*/

bool ov_ice_pair_handshake_passive(ov_ice_pair *pair, const uint8_t *buffer,
                                   size_t size) {

    char errorstring[OV_ICE_SSL_ERROR_STRING_BUFFER_SIZE] = {0};
    int errorcode = -1;
    int n = 0, r = 0;

    if (!pair || !buffer || !size)
        goto error;

    ov_ice *ice = pair->stream->session->ice;
    SSL_CTX *ctx = ov_ice_get_dtls_ctx(ice);
    ov_ice_config config = ov_ice_get_config(ice);

    if (pair->dtls.ssl) {

        SSL_free(pair->dtls.ssl);
        pair->dtls.ssl = NULL;
    }

    pair->dtls.handshaked = false;
    pair->dtls.type = OV_ICE_DTLS_PASSIVE;
    pair->dtls.ssl = SSL_new(ctx);
    if (!pair->dtls.ssl)
        goto error;

    SSL_set_accept_state(pair->dtls.ssl);

    r = SSL_set_tlsext_use_srtp(pair->dtls.ssl, config.dtls.srtp.profile);
    if (0 != r)
        goto error;

    pair->dtls.read = BIO_new(BIO_s_mem());
    pair->dtls.write = ov_ice_dtls_filter_pair_bio_create(pair);
    BIO_set_mem_eof_return(pair->dtls.read, -1);
    BIO_set_mem_eof_return(pair->dtls.write, -1);

    SSL_set_bio(pair->dtls.ssl, pair->dtls.read, pair->dtls.write);
    SSL_set_options(pair->dtls.ssl, SSL_OP_COOKIE_EXCHANGE);

    r = BIO_write(pair->dtls.read, buffer, size);
    if (r < 0)
        goto error;

    BIO_ADDR *peer_bio = BIO_ADDR_new();

    r = DTLSv1_listen(pair->dtls.ssl, peer_bio);

    if (peer_bio) {
        BIO_ADDR_free(peer_bio);
        peer_bio = NULL;
    }

    if (r >= 1) {

        pair->dtls.handshaked = true;
        pair->stream->dtls = OV_ICE_COMPLETED;

        ov_log_info("ICE %s|%i completed DTLS on pair %s %s:%i %s:%i",
                    pair->stream->session->uuid, pair->stream->index,
                    ov_ice_candidate_type_to_string(pair->local->type),
                    pair->local->addr, pair->local->port, pair->remote->addr,
                    pair->remote->port);

        goto done;

    } else if (r == 0) {

        /*
         *      Non Fatal error, usercode is expected to
         *      retry operation. (man DTLSv1_listen)
         */

        SSL_free(pair->dtls.ssl);
        pair->dtls.ssl = NULL;

    } else {

        n = SSL_get_error(pair->dtls.ssl, r);

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

            ov_log_error("socket %i - SSL_ERROR_SYSCALL - "
                         "errno %d | %s",
                         socket, errno, strerror(errno));
            goto error;

            break;

        case SSL_ERROR_SSL:

            errorcode = ERR_get_error();
            ERR_error_string_n(errorcode, errorstring,
                               OV_ICE_SSL_ERROR_STRING_BUFFER_SIZE);

            ov_log_error("socket %i - SSL_ERROR_SSL - "
                         "%s",
                         socket, errorstring);
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
    if (pair)
        pair->state = OV_ICE_PAIR_FAILED;
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

/*----------------------------------------------------------------------------*/

static int check_cert(ov_ice_pair *pair) {

    char hash_finger[OV_ICE_DTLS_FINGERPRINT_MAX] = {0};

    OV_ASSERT(pair);
    OV_ASSERT(pair->dtls.ssl);

    if (!pair || !pair->dtls.ssl)
        goto error;

    int r = 0;

    X509 *cert = SSL_get_peer_certificate(pair->dtls.ssl);
    if (!cert) {

        ov_log_debug("Get peer certificate failed for pair %s:%i | %s:%i",
                     pair->local->addr, pair->local->port, pair->remote->addr,
                     pair->remote->port);

        goto close;
    }

    r = SSL_get_verify_result(pair->dtls.ssl);
    if (r != X509_V_OK) {

        ov_log_debug("Verify peer certificate failed for pair %s:%i | %s:%i",
                     pair->local->addr, pair->local->port, pair->remote->addr,
                     pair->remote->port);

        goto close;
    }

    char *finger = X509_fingerprint_create(cert, OV_HASH_SHA256);

    if (!finger) {

        ov_log_debug("Fingerprint peer certificate failed for pair %s:%i | "
                     "%s:%i",
                     pair->local->addr, pair->local->port, pair->remote->addr,
                     pair->remote->port);

        X509_free(cert);
        goto close;
    }

    X509_free(cert);

    if (!snprintf(hash_finger, OV_ICE_DTLS_FINGERPRINT_MAX, "%s %s",
                  ov_hash_function_to_RFC8122_string(OV_HASH_SHA256), finger)) {

        finger = ov_data_pointer_free(finger);
        goto close;
    }

    finger = ov_data_pointer_free(finger);

    if (0 != strcmp(hash_finger, pair->stream->remote.fingerprint)) {

        ov_log_debug("Fingerprint verification failed for pair %s:%i | %s:%i",
                     pair->local->addr, pair->local->port, pair->remote->addr,
                     pair->remote->port);

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

static bool perform_ssl_client_handshake(ov_ice_pair *pair) {

    char errorstring[OV_ICE_SSL_ERROR_STRING_BUFFER_SIZE] = {0};
    int errorcode = -1, r = 0, n = 0;

    bool shutdown = true;

    OV_ASSERT(pair);
    if (!pair)
        goto error;

    ov_event_loop *loop = ov_ice_get_event_loop(pair->stream->session->ice);
    ov_ice_config config = ov_ice_get_config(pair->stream->session->ice);

    if (OV_TIMER_INVALID != pair->timer.handshake) {

        loop->timer.unset(loop, pair->timer.handshake, NULL);
        pair->timer.handshake = OV_TIMER_INVALID;
    }

    r = SSL_connect(pair->dtls.ssl);

    switch (r) {

    case 1:

        r = check_cert(pair);
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

        n = SSL_get_error(pair->dtls.ssl, r);
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
            errorcode = ERR_get_error();
            ERR_error_string_n(errorcode, errorstring,
                               OV_ICE_SSL_ERROR_STRING_BUFFER_SIZE);

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

    pair->timer.handshake =
        loop->timer.set(loop, config.dtls.reconnect_interval_usec, pair,
                        perform_ssl_client_handshake_triggered);

    if (OV_TIMER_INVALID == pair->timer.handshake)
        goto error;

success:
    return true;

close:

    if (shutdown)
        SSL_shutdown(pair->dtls.ssl);

error:
    if (pair)
        pair->state = OV_ICE_PAIR_FAILED;
    return false;
}

/*----------------------------------------------------------------------------*/

bool perform_ssl_client_handshake_triggered(uint32_t id, void *userdata) {

    if (!userdata)
        return false;

    ov_ice_pair *pair = (ov_ice_pair *)(userdata);
    OV_ASSERT(id == pair->timer.handshake);
    pair->timer.handshake = OV_TIMER_INVALID;

    // handshake function MUST reset any timer if required
    return perform_ssl_client_handshake(pair);
}

/*----------------------------------------------------------------------------*/

static bool srtp_unset_data(ov_ice_pair *pair) {

    if (!pair)
        return false;

    pair->srtp.ready = false;
    pair->srtp.profile = ov_data_pointer_free(pair->srtp.profile);
    pair->srtp.key_len = OV_ICE_SRTP_KEY_MAX;
    pair->srtp.salt_len = OV_ICE_SRTP_SALT_MAX;

    memset(pair->srtp.server.key, 0, OV_ICE_SRTP_KEY_MAX);
    memset(pair->srtp.client.key, 0, OV_ICE_SRTP_KEY_MAX);
    memset(pair->srtp.server.salt, 0, OV_ICE_SRTP_SALT_MAX);
    memset(pair->srtp.client.salt, 0, OV_ICE_SRTP_SALT_MAX);

    return true;
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

static const char *get_srtp_keys(ov_ice_pair *pair, uint32_t *key_len,
                                 uint32_t *salt_len, uint8_t *server_key,
                                 uint8_t *server_salt, uint8_t *client_key,
                                 uint8_t *client_salt) {

    size_t size = 2 * OV_ICE_SRTP_KEY_MAX + 2 * OV_ICE_SRTP_SALT_MAX + 1;
    uint8_t buffer[size];
    memset(buffer, 0, size);

    if (!pair || !pair->dtls.ssl || !key_len || !salt_len || !server_key ||
        !server_salt || !client_key || !client_salt)
        goto error;

    uint32_t keylen = 0;
    uint32_t saltlen = 0;

    SRTP_PROTECTION_PROFILE *profile =
        SSL_get_selected_srtp_profile(pair->dtls.ssl);
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
                 pair->dtls.ssl, buffer, size, label_extractor_srtp,
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

bool ov_ice_pair_dtls_io(ov_ice_pair *pair, const uint8_t *buffer,
                         size_t size) {

    char buf[OV_ICE_SSL_BUFFER_SIZE] = {0};

    int r = 0;
    ssize_t out = 0;

    if (!pair || !buffer || size < 1)
        goto error;

    r = BIO_write(pair->dtls.read, buffer, size);
    if (r < 0)
        goto error;

    if (!SSL_is_init_finished(pair->dtls.ssl)) {

        switch (pair->dtls.type) {

        case OV_ICE_DTLS_PASSIVE:

            r = SSL_do_handshake(pair->dtls.ssl);

            if (SSL_is_init_finished(pair->dtls.ssl)) {
                pair->dtls.handshaked = true;
            }

            break;

        case OV_ICE_DTLS_ACTIVE:
            perform_ssl_client_handshake(pair);
            break;
        }

        srtp_unset_data(pair);

        const char *profile =
            get_srtp_keys(pair, &pair->srtp.key_len, &pair->srtp.salt_len,
                          pair->srtp.server.key, pair->srtp.server.salt,
                          pair->srtp.client.key, pair->srtp.client.salt);

        if (profile) {

            pair->srtp.ready = true;
            pair->srtp.profile = ov_data_pointer_free(pair->srtp.profile);
            pair->srtp.profile = strdup(profile);

            if (pair == pair->stream->selected)
                ov_ice_stream_cb_selected_srtp_ready(pair->stream);
        }

    } else {

        out = SSL_read(pair->dtls.ssl, buf, OV_ICE_SSL_BUFFER_SIZE);

        if (out > 0) {
            ov_log_debug("%zi bytes of unexpected SSL data received at pair "
                         "%s:%i | %s:%i",
                         out, pair->local->addr, pair->local->port,
                         pair->remote->addr, pair->remote->port);
        }
    }

    return true;

error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_ice_pair_handshake_active(ov_ice_pair *pair) {

    char errorstring[OV_ICE_SSL_ERROR_STRING_BUFFER_SIZE] = {0};
    int errorcode = -1;
    int r = 0;

    if (!pair)
        goto error;

    ov_ice_config config = ov_ice_get_config(pair->stream->session->ice);

    if (0 == pair->stream->remote.fingerprint[0])
        goto error;

    if (pair->dtls.ssl) {

        SSL_free(pair->dtls.ssl);
        pair->dtls.ssl = NULL;
    }

    pair->dtls.handshaked = false;
    pair->dtls.type = OV_ICE_DTLS_ACTIVE;

    pair->dtls.ctx = SSL_CTX_new(DTLS_client_method());
    if (!pair->dtls.ctx)
        goto error;

    if (1 != SSL_CTX_set_min_proto_version(pair->dtls.ctx, DTLS1_2_VERSION))
        goto error;

    // use default SSL verify
    SSL_CTX_set_verify(pair->dtls.ctx, SSL_VERIFY_PEER, NULL);

    // load verify data path from ov_ice_ssl config

    const char *file = NULL;
    const char *path = NULL;

    if (0 != config.dtls.ca.file[0])
        file = config.dtls.ca.file;

    if (0 != config.dtls.ca.path[0])
        path = config.dtls.ca.path;

    if (file || path) {

        r = SSL_CTX_load_verify_locations(pair->dtls.ctx, file, path);

        if (r != 1) {

            errorcode = ERR_get_error();
            ERR_error_string_n(errorcode, errorstring,
                               OV_ICE_SSL_ERROR_STRING_BUFFER_SIZE);

            ov_log_error("SSL_CTX_load_verify_locations failed "
                         "at socket %i | %s",
                         socket, errorstring);
            goto error;
        }
    }

    // create the DTLS session context
    pair->dtls.ssl = SSL_new(pair->dtls.ctx);
    if (!pair->dtls.ssl)
        goto error;

    r = SSL_set_tlsext_use_srtp(pair->dtls.ssl, config.dtls.srtp.profile);
    if (0 != r)
        goto error;

    pair->dtls.read = BIO_new(BIO_s_mem());
    pair->dtls.write = ov_ice_dtls_filter_pair_bio_create(pair);
    BIO_set_mem_eof_return(pair->dtls.read, -1);
    BIO_set_mem_eof_return(pair->dtls.write, -1);

    SSL_set_bio(pair->dtls.ssl, pair->dtls.read, pair->dtls.write);

    SSL_set_options(pair->dtls.ssl, SSL_OP_COOKIE_EXCHANGE);
    SSL_set_connect_state(pair->dtls.ssl);
    SSL_CTX_set_client_hello_cb(pair->dtls.ctx, dummy_client_hello_cb, NULL);

    ov_event_loop *loop = ov_ice_get_event_loop(pair->stream->session->ice);
    pair->timer.handshake =
        loop->timer.set(loop, config.dtls.reconnect_interval_usec, pair,
                        perform_ssl_client_handshake_triggered);

    if (OV_TIMER_INVALID == pair->timer.handshake)
        goto error;

    return true;
error:
    if (pair)
        pair->state = OV_ICE_PAIR_FAILED;
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_ice_pair_send_stun_binding_request(ov_ice_pair *pair) {

    size_t size = OV_UDP_PAYLOAD_OCTETS;
    uint8_t buffer[OV_UDP_PAYLOAD_OCTETS] = {0};
    uint8_t *ptr = buffer;

    if (!pair)
        goto error;

    ov_ice_base *base = pair->local->base;
    ov_ice_stream *stream = base->stream;
    ov_ice_session *session = stream->session;

    if (!ov_ice_transaction_create(session->ice, pair->transaction_id, 13,
                                   pair))
        goto error;

    bool usecandidate = false;

    if (session->controlling)
        usecandidate = pair->nominated;

    const char *user = stream->uuid;
    const char *pass = stream->local.pass;
    const char *peer_user = stream->remote.user;
    const char *peer_pass = stream->remote.pass;

    if (!user || !pass || !peer_user || !peer_pass)
        goto error;

    size_t required = 20; // header
    size_t len = 0;

    char username[513] = {0};

    if (!snprintf(username, 513, "%s:%s", peer_user, user))
        goto error;

    required += ov_stun_message_integrity_encoding_length();
    required += ov_stun_fingerprint_encoding_length();
    required += ov_stun_ice_priority_encoding_length();

    len =
        ov_stun_username_encoding_length((uint8_t *)username, strlen(username));

    if (0 == len)
        goto error;

    required += len;

    if (usecandidate)
        required += ov_stun_ice_use_candidate_encoding_length();

    if (session->controlling) {
        required += ov_stun_ice_controlling_encoding_length();
    } else {
        required += ov_stun_ice_controlled_encoding_length();
    }

    /*
     *      Prepare the buffer
     */

    if (required > OV_UDP_PAYLOAD_OCTETS)
        goto error;

    size = required;
    uint8_t *start = buffer;

    /*
     *      Write content to the buffer.
     */

    if (!ov_stun_frame_set_request(ptr, size))
        goto error;

    if (!ov_stun_frame_set_method(ptr, size, STUN_BINDING))
        goto error;

    if (!ov_stun_frame_set_magic_cookie(ptr, size))
        goto error;

    if (!ov_stun_frame_set_length(ptr, size, size - 20))
        goto error;

    if (!ov_stun_frame_set_transaction_id(ptr, size, pair->transaction_id))
        goto error;

    ptr = buffer + 20;

    if (session->controlling) {

        if (!ov_stun_ice_controlling_encode(ptr, size - (ptr - start), &ptr,
                                            session->tiebreaker))
            goto error;

    } else {

        if (!ov_stun_ice_controlled_encode(ptr, size - (ptr - start), &ptr,
                                           session->tiebreaker))
            goto error;
    }

    if (usecandidate) {

        if (!ov_stun_ice_use_candidate_encode(ptr, size - (ptr - start), &ptr))
            goto error;
    }

    if (!ov_stun_ice_priority_encode(ptr, size - (ptr - start), &ptr,
                                     pair->priority))
        goto error;

    if (!ov_stun_username_encode(ptr, size - (ptr - start), &ptr,
                                 (uint8_t *)username, strlen(username)))
        goto error;

    if (!ov_stun_add_message_integrity(start, size, ptr, &ptr,
                                       (uint8_t *)peer_pass, strlen(peer_pass)))
        goto error;

    if (!ov_stun_add_fingerprint(start, size, ptr, &ptr))
        goto error;

    ssize_t out = ov_ice_pair_send(pair, start, ptr - start);

    if (out < 0) {

        ov_ice_transaction_unset(session->ice, pair->transaction_id);
        goto error;
    }

    if (ov_ice_debug_stun(session->ice))
        ov_log_debug("STUN send binding request from %s:%i to %s:%i",
                     base->local.data.host, base->local.data.port,
                     pair->remote->addr, pair->remote->port);

    pair->progress_count++;

    if (pair->state != OV_ICE_PAIR_SUCCESS)
        pair->state = OV_ICE_PAIR_PROGRESS;

    if (pair->progress_count > 100)
        goto error;

    return true;
error:
    if (pair)
        pair->state = OV_ICE_PAIR_FAILED;
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_ice_pair_create_turn_permission(ov_ice_pair *pair) {

    if (!pair)
        goto error;

    TODO("... to be implemented.");

error:
    return false;
}