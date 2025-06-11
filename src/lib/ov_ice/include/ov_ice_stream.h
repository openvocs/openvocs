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
        @file           ov_ice_stream.h
        @author         Markus Töpfer

        @date           2024-09-18


        ------------------------------------------------------------------------
*/
#ifndef ov_ice_stream_h
#define ov_ice_stream_h

typedef struct ov_ice_stream ov_ice_stream;

#include "ov_ice_base.h"
#include "ov_ice_candidate.h"
#include "ov_ice_session.h"
#include "ov_ice_state.h"

#include <ov_base/ov_id.h>
#include <ov_base/ov_node.h>
#include <ov_base/ov_sdp.h>

#include <srtp2/srtp.h>

struct ov_ice_stream {

    ov_node node;
    ov_id uuid;

    ov_ice_dtls_type type;
    ov_ice_state state;

    ov_ice_state stun;
    ov_ice_state dtls;
    ov_ice_state srtp;

    int index;

    ov_ice_session *session;

    struct {

        uint32_t ssrc;
        bool gathered;

        char pass[OV_ICE_STUN_PASS_MAX];

        srtp_policy_t policy;
        uint8_t key[OV_ICE_SRTP_KEY_MAX];

    } local;

    struct {

        uint32_t ssrc;
        bool gathered;

        char user[OV_ICE_STUN_USER_MAX];
        char pass[OV_ICE_STUN_PASS_MAX];

        char fingerprint[OV_ICE_DTLS_FINGERPRINT_MAX];

        srtp_policy_t policy;
        uint8_t key[OV_ICE_SRTP_KEY_MAX];

    } remote;

    struct {

        uint32_t nominate;

    } timer;

    ov_list *valid;
    ov_list *trigger;

    ov_ice_pair *pairs;
    ov_ice_pair *selected;

    struct {

        ov_ice_candidate *local;
        ov_ice_candidate *remote;

    } candidates;

    ov_ice_base *bases;
};

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_ice_stream *ov_ice_stream_create(ov_ice_session *session, int index);
ov_ice_stream *ov_ice_stream_cast(const void *data);
void *ov_ice_stream_free(void *self);

ov_sdp_connection ov_ice_stream_get_connection(ov_ice_stream *stream,
                                               ov_sdp_description *desc);

bool ov_ice_stream_candidate(ov_ice_stream *stream, const ov_ice_candidate *c);
bool ov_ice_stream_end_of_candidates(ov_ice_stream *stream);
uint32_t ov_ice_stream_get_stream_ssrc(ov_ice_stream *stream);
ssize_t ov_ice_stream_send_stream(ov_ice_stream *stream,
                                  const uint8_t *buffer,
                                  size_t size);

bool ov_ice_stream_set_active(ov_ice_stream *stream, const char *fingerprint);
bool ov_ice_stream_set_passive(ov_ice_stream *stream);

bool ov_ice_stream_order(ov_ice_stream *stream);
bool ov_ice_stream_prune(ov_ice_stream *stream);

bool ov_ice_stream_cb_selected_srtp_ready(ov_ice_stream *stream);

bool ov_ice_stream_process_remote_gathered(ov_ice_stream *stream,
                                           ov_ice_candidate *candidate);

bool ov_ice_stream_trickle_candidates(ov_ice_stream *stream);

bool ov_ice_stream_update(ov_ice_stream *stream);

bool ov_ice_stream_sort_priority(ov_ice_stream *stream);

bool ov_ice_stream_recalculate_priorities(ov_ice_stream *stream);

bool ov_ice_stream_gathering_stop(ov_ice_stream *stream);

#endif /* ov_ice_stream_h */
