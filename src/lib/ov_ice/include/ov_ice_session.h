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
        @file           ov_ice_session.h
        @author         Markus Töpfer

        @date           2024-09-18


        ------------------------------------------------------------------------
*/
#ifndef ov_ice_session_h
#define ov_ice_session_h

typedef struct ov_ice_session ov_ice_session;

#include "ov_ice.h"
#include "ov_ice_candidate.h"
#include "ov_ice_state.h"
#include "ov_ice_stream.h"

#include <ov_base/ov_id.h>
#include <ov_base/ov_node.h>

#include <srtp2/srtp.h>

struct ov_ice_session {

  ov_node node;
  ov_id uuid;

  ov_ice_state state;
  bool controlling;
  bool nominate_started;
  bool trickling_started;

  uint64_t tiebreaker;

  ov_ice *ice;

  struct {

    srtp_t session;

  } srtp;

  struct {

    uint32_t session_timeout;
    uint32_t nominate_timeout;
    uint32_t trickling;
    uint32_t connectivity;

  } timer;

  ov_ice_stream *streams;
};

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_ice_session *ov_ice_session_create_offer(ov_ice *ice, ov_sdp_session *sdp);
ov_ice_session *ov_ice_session_create_answer(ov_ice *ice, ov_sdp_session *sdp);
ov_ice_session *ov_ice_session_cast(const void *data);
void *ov_ice_session_free(void *self);

bool ov_ice_session_process_answer_in(ov_ice_session *self,
                                      const ov_sdp_session *sdp);

bool ov_ice_session_candidate(ov_ice_session *session, int stream_id,
                              const ov_ice_candidate *candidate);
bool ov_ice_session_end_of_candidates(ov_ice_session *session, int stream_id);

uint32_t ov_ice_session_get_stream_ssrc(ov_ice_session *session, int stream_id);
ssize_t ov_ice_session_stream_send(ov_ice_session *session, int stream_id,
                                   const uint8_t *buffer, size_t size);

bool ov_ice_session_set_foundation(ov_ice_session *session,
                                   ov_ice_candidate *candidate);
bool ov_ice_session_set_state_on_foundation(ov_ice_session *self,
                                            ov_ice_pair *pair);
bool ov_ice_session_unfreeze_foundation(ov_ice_session *session,
                                        const uint8_t *foundation);

bool ov_ice_session_update(ov_ice_session *session);

bool ov_ice_session_change_role(ov_ice_session *session, uint64_t tiebreaker);

bool ov_ice_session_checklists_run(ov_ice_session *session);

void ov_ice_session_dump_pairs(ov_ice_session *session);

void ov_ice_session_state_change(ov_ice_session *session);

#endif /* ov_ice_session_h */
