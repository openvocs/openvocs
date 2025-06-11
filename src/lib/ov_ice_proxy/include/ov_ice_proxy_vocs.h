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
        @file           ov_ice_proxy_vocs.h
        @author         Markus Toepfer

        @date           2024-05-08

        Implementation of a VOCS conform ICE proxy with internal and 
        external connection handling. 

        ------------------------------------------------------------------------
*/
#ifndef ov_ice_proxy_vocs_h
#define ov_ice_proxy_vocs_h

#include "ov_ice_proxy_generic.h"
#include "ov_ice_proxy_vocs_session_data.h"

#include <ov_core/ov_mc_loop_data.h>

/*----------------------------------------------------------------------------*/

typedef struct ov_ice_proxy_vocs ov_ice_proxy_vocs;

/*----------------------------------------------------------------------------*/

typedef struct ov_ice_proxy_vocs_config {

    ov_event_loop *loop;

    bool multiplexing;

    struct {

        ov_socket_configuration internal;

    } socket;

    ov_ice_proxy_generic_config proxy;

    struct {

        void *userdata;
        bool (*send_candidate)(void *userdata, ov_json_value *out);
        bool (*send_end_of_candidates)(void *userdata, const char *session_id);
        bool (*session_completed)(void *userdata,
                                  const char *uuid,
                                  ov_ice_state state);

    } callback;

} ov_ice_proxy_vocs_config;

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_ice_proxy_vocs *ov_ice_proxy_vocs_create(ov_ice_proxy_vocs_config config);
ov_ice_proxy_vocs *ov_ice_proxy_vocs_free(ov_ice_proxy_vocs *self);
ov_ice_proxy_vocs *ov_ice_proxy_vocs_cast(const void *data);

/*----------------------------------------------------------------------------*/

ov_ice_proxy_vocs_config ov_ice_proxy_vocs_config_from_json(
    const ov_json_value *input);

/*
 *      ------------------------------------------------------------------------
 *
 *      EXTERNAL FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_ice_proxy_vocs_session_data ov_ice_proxy_vocs_create_session(
    ov_ice_proxy_vocs *self);

/*----------------------------------------------------------------------------*/

bool ov_ice_proxy_vocs_drop_session(ov_ice_proxy_vocs *self, const char *uuid);

/*----------------------------------------------------------------------------*/

bool ov_ice_proxy_vocs_update_answer(ov_ice_proxy_vocs *self,
                                     const char *session_id,
                                     ov_sdp_session *sdp);

/*----------------------------------------------------------------------------*/

bool ov_ice_proxy_vocs_candidate_in(ov_ice_proxy_vocs *self,
                                    const char *session_id,
                                    uint32_t stream_id,
                                    const ov_ice_candidate *candidate);

/*----------------------------------------------------------------------------*/

bool ov_ice_proxy_vocs_end_of_candidates_in(ov_ice_proxy_vocs *self,
                                            const char *session_id,
                                            uint32_t stream_id);

/*----------------------------------------------------------------------------*/

bool ov_ice_proxy_vocs_talk(ov_ice_proxy_vocs *self,
                            const char *session_id,
                            bool on,
                            ov_mc_loop_data data);

#endif /* ov_ice_proxy_vocs_h */
