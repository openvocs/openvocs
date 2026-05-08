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
        @file           ov_ice.h
        @author         Markus Töpfer

        @date           2024-09-18


        ------------------------------------------------------------------------
*/
#ifndef ov_ice_h
#define ov_ice_h

typedef struct ov_ice ov_ice;

#include "ov_ice_definitions.h"
#include "ov_ice_dtls_config.h"
#include "ov_ice_dtls_cookie.h"
#include "ov_ice_dtls_filter.h"

#include "ov_ice_state.h"

#include "ov_ice_base.h"
#include "ov_ice_candidate.h"
#include "ov_ice_pair.h"
#include "ov_ice_server.h"
#include "ov_ice_session.h"
#include "ov_ice_stream.h"

#include <ov_base/ov_event_loop.h>
#include <ov_base/ov_sdp.h>

/*----------------------------------------------------------------------------*/

typedef struct ov_ice_callbacks {

    void *userdata;

    struct {

        void (*drop)(void *userdata, const char *uuid);
        void (*state)(void *userdata, const char *uuid, ov_ice_state state);

    } session;

    struct {

        void (*io)(void *userdata, const char *session_uuid, int stream_id,
                   uint8_t *buffer, size_t size);

    } stream;

    struct {

        bool (*new)(void *userdata, const char *session_uuid, const char *ufrag,
                    int stream_id, ov_ice_candidate candidate);

        bool (*end_of_candidates)(void *userdata, const char *session_uuid,
                                  int stream_id);

    } candidates;

} ov_ice_callbacks;

/*----------------------------------------------------------------------------*/

typedef struct ov_ice_config {

    ov_event_loop *loop;

    ov_ice_dtls_config dtls;

    struct {

        uint64_t transaction_lifetime_usecs;

        struct {

            uint64_t connectivity_pace_usecs;
            uint64_t session_timeout_usecs;
            uint64_t keepalive_usecs;

        } stun;

    } limits;

    ov_ice_callbacks callbacks;

} ov_ice_config;

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_ice *ov_ice_create(ov_ice_config *config);
ov_ice *ov_ice_cast(const void *data);
ov_ice *ov_ice_free(ov_ice *self);

/*
 *      ------------------------------------------------------------------------
 *
 *      CONFIG FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_ice_config ov_ice_config_from_json(const ov_json_value *input);

/**
 *      Set a port range to be used within ov_ice gathering.
 */
bool ov_ice_set_port_range(ov_ice *self, uint32_t min, uint32_t max);

/**
 *      Set a server to be used while gathering candidates.
 */
bool ov_ice_add_server(ov_ice *self, ov_ice_server server);

/**
 *      Add an interface to be used for ICE gathering,
 *      not required if autodiscovery is set.
 */
bool ov_ice_add_interface(ov_ice *self, const char *host);
bool ov_ice_set_autodiscovery(ov_ice *self, bool on);

/**
 *      Set some debug flags.
 */
bool ov_ice_set_debug_stun(ov_ice *self, bool on);
bool ov_ice_set_debug_ice(ov_ice *self, bool on);
bool ov_ice_set_debug_dtls(ov_ice *self, bool on);

/*
 *      ------------------------------------------------------------------------
 *
 *      EXTERNAL USE FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

/**
 *      Create a session as offer, SDP parameters of ICE will be set during the
 *      process.
 *
 *      @returns new session uuid
 */
const char *ov_ice_create_session_offer(ov_ice *self, ov_sdp_session *sdp);

/**
 *      Create a session as answer, SDP parameters of ICE will be set during the
 *      process.
 *
 *      @returns new session uuid
 */
const char *ov_ice_create_session_answer(ov_ice *self, ov_sdp_session *sdp);

bool ov_ice_session_process_answer(ov_ice *self, const char *session_id,
                                   const ov_sdp_session *sdp);

bool ov_ice_drop_session(ov_ice *self, const char *uuid);

bool ov_ice_add_candidate(ov_ice *self, const char *session_uuid,
                          const int stream_id,
                          const ov_ice_candidate *candidate);

bool ov_ice_add_end_of_candidates(ov_ice *self, const char *session_uuid,
                                  const int stream_id);

uint32_t ov_ice_get_stream_ssrc(ov_ice *self, const char *session_uuid,
                                const int stream_id);

ssize_t ov_ice_stream_send(ov_ice *self, const char *session_uuid,
                           const int stream_id, uint8_t *buffer, size_t size);

/*
 *      ------------------------------------------------------------------------
 *
 *      INTERNAL USE FUNCTIONS
 *
 *      the following functions are defined for internal use within ov_ice.
 *
 *      ------------------------------------------------------------------------
 */

ov_event_loop *ov_ice_get_event_loop(const ov_ice *self);
const char *ov_ice_get_fingerprint(const ov_ice *self);
SSL_CTX *ov_ice_get_dtls_ctx(const ov_ice *self);
int ov_ice_get_interfaces(const ov_ice *self);
ov_ice_config ov_ice_get_config(const ov_ice *self);

bool ov_ice_gather_candidates(ov_ice *self, ov_ice_stream *stream);

bool ov_ice_debug_stun(const ov_ice *self);
bool ov_ice_debug_ice(const ov_ice *self);
bool ov_ice_debug_dtls(const ov_ice *self);

bool ov_ice_transaction_unset(ov_ice *self, const uint8_t *transaction_id);

bool ov_ice_transaction_create(ov_ice *self, uint8_t *buffer, size_t size,
                               void *userdata);

void *ov_ice_transaction_get(ov_ice *self, const uint8_t *transaction_id);

bool ov_ice_session_drop_callback(ov_ice *self, const char *session_uuid);

bool ov_ice_trickle_candidate(ov_ice *self, const char *session_uuid,
                              const char *ufrag, int stream_id,
                              const ov_ice_candidate *candidate);

bool ov_ice_trickle_end_of_candidates(ov_ice *self, const char *session_uuid,
                                      int stream_id);

#endif /* ov_ice_h */
