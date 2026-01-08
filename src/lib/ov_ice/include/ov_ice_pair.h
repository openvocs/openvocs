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
        @file           ov_ice_pair.h
        @author         Markus Töpfer

        @date           2024-09-18


        ------------------------------------------------------------------------
*/
#ifndef ov_ice_pair_h
#define ov_ice_pair_h

typedef struct ov_ice_pair ov_ice_pair;

#include "ov_ice_candidate.h"
#include "ov_ice_definitions.h"
#include "ov_ice_state.h"

#include <ov_base/ov_node.h>

struct ov_ice_pair {

    ov_node node;

    ov_ice_pair_state state;
    ov_ice_stream *stream;

    uint8_t transaction_id[13];

    uint64_t priority;

    bool nominated;

    const ov_ice_candidate *local;
    const ov_ice_candidate *remote;

    uint16_t success_count;
    uint16_t progress_count;

    struct {

        bool handshaked;
        ov_ice_dtls_type type;

        SSL *ssl;
        SSL_CTX *ctx;

        BIO *read;
        BIO *write;

    } dtls;

    struct {

        bool ready;
        bool selected_callback_done;
        char *profile;

        uint32_t key_len;
        uint32_t salt_len;

        struct {

            uint8_t key[OV_ICE_SRTP_KEY_MAX];
            uint8_t salt[OV_ICE_SRTP_SALT_MAX];

        } server;

        struct {

            uint8_t key[OV_ICE_SRTP_KEY_MAX];
            uint8_t salt[OV_ICE_SRTP_SALT_MAX];

        } client;

    } srtp;

    struct {

        uint32_t permission_renew;
        uint32_t handshake;
        uint32_t keepalive;

    } timer;
};

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_ice_pair *ov_ice_pair_create(ov_ice_stream *stream,
                                const ov_ice_candidate *local,
                                const ov_ice_candidate *remote);
ov_ice_pair *ov_ice_pair_cast(const void *data);
void *ov_ice_pair_free(void *self);

ssize_t ov_ice_pair_send(ov_ice_pair *self, const uint8_t *buffer, size_t size);

const char *ov_ice_pair_state_to_string(ov_ice_pair_state state);

bool ov_ice_pair_handshake_passive(ov_ice_pair *pair, const uint8_t *buffer,
                                   size_t size);

bool ov_ice_pair_handshake_active(ov_ice_pair *pair);

bool ov_ice_pair_dtls_io(ov_ice_pair *pair, const uint8_t *buffer, size_t size);

bool ov_ice_pair_create_turn_permission(ov_ice_pair *pair);

bool ov_ice_pair_calculate_priority(ov_ice_pair *pair, ov_ice_session *session);

bool ov_ice_pair_send_stun_binding_request(ov_ice_pair *pair);

#endif /* ov_ice_pair_h */
