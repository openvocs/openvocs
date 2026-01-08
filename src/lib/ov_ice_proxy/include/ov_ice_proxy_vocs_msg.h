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
        @file           ov_ice_proxy_vocs_msg.h
        @author         Markus Toepfer

        @date           2024-05-08


        ------------------------------------------------------------------------
*/
#ifndef ov_ice_proxy_vocs_msg_h
#define ov_ice_proxy_vocs_msg_h

#include "ov_ice_proxy_vocs_stream_forward.h"
#define ov_ice_proxy_vocs_API_VERSION 1

#define OV_KEY_ICE_SESSION_CREATE "ice_session_create"
#define OV_KEY_ICE_SESSION_DROP "ice_session_drop"
#define OV_KEY_ICE_SESSION_UPDATE "ice_session_update"
#define OV_KEY_ICE_SESSION_STATE "ice_session_state"
#define OV_KEY_ICE_SESSION_FORWARD_STREAM "forward_stream"

#define OV_KEY_ICE_SESSION_COMPLETED "ice_session_completed"
#define OV_KEY_ICE_SESSION_FAILED "ice_session_failed"

#define OV_KEY_ICE_DROP "drop"

#include <ov_base/ov_json_value.h>
#include <ov_core/ov_mc_loop_data.h>
#include <ov_core/ov_media_definitions.h>
#include <ov_ice/ov_ice_candidate.h>
#include <ov_ice/ov_ice_state.h>
#include <ov_ice/ov_ice_string.h>

/*
 *      ------------------------------------------------------------------------
 *
 *      Messages
 *
 *      ------------------------------------------------------------------------
 */

/**
        Message to create a session,
        without any specific SDP settings (use default)

        {
            "uuid"  : "<new generated uuid>,
            "event" : "ice_session_create",
            "version" : 1
        }
*/
ov_json_value *ov_ice_proxy_vocs_msg_create_session();

/*----------------------------------------------------------------------------*/

ov_json_value *ov_ice_proxy_vocs_msg_create_session_with_sdp(const char *uuid,
                                                             const char *sdp);

/*----------------------------------------------------------------------------*/

/**
        Message to create a session,
        without any specific SDP settings (use default)

        {
            "uuid"  : "<new generated uuid>,
            "event" : "ice_session_failed",
            "version" : 1
        }
*/
ov_json_value *ov_ice_proxy_vocs_msg_session_failed(const char *uuid);

/*----------------------------------------------------------------------------*/

/**
        Message to drop a session.

        {
            "uuid"  : "<new generated uuid>,
            "event" : "ice_session_drop",
            "version" : 1,
            "parameter" :
            {
                "session" : "<uuid input>"
            }
        }
*/
ov_json_value *ov_ice_proxy_vocs_msg_drop_session(const char *session_uuid);

/*----------------------------------------------------------------------------*/

/**
        Message to set forward proxy data for a specific stream.

        {
            "uuid"  : "<new generated uuid>,
            "event" : "forward_stream",
            "version" : 1,
            "parameter" :
            {
                "session" : "<uuid input>",
                "stream" :
                {
                    "id" : 1,
                    "ssrc": 32000
                    "socket": {
                        "port": 38737,
                        "host": "10.61.11.225",
                        "type": "UDP"
                    }
                }
            }
        }
*/
ov_json_value *ov_ice_proxy_vocs_msg_forward_stream(
    const char *session_uuid, ov_ice_proxy_vocs_stream_forward_data data);

/*----------------------------------------------------------------------------*/

ov_json_value *
ov_ice_proxy_vocs_msg_candidate(const char *session_uuid,
                                const ov_ice_candidate_info *info);

/*----------------------------------------------------------------------------*/

ov_json_value *
ov_ice_proxy_vocs_msg_end_of_candidates(const char *session_uuid);

/*----------------------------------------------------------------------------*/

ov_json_value *ov_ice_proxy_vocs_msg_media(const char *session_id,
                                           ov_media_type type, const char *sdp);

ov_json_value *ov_ice_proxy_vocs_msg_update_session(const char *uuid,
                                                    const char *session_uuid,
                                                    ov_media_type type,
                                                    const char *sdp);

/*----------------------------------------------------------------------------*/

/**
        Create a client response msg from some ice_session_create
*/
ov_json_value *ov_ice_proxy_vocs_msg_client_response_from_ice_session_create(
    const ov_json_value *original_client_request,
    const ov_json_value *ice_session_create_response);

/*----------------------------------------------------------------------------*/

/**
        Create a REGISTER message of some ICE proxy instance for some
        ICE PROXY manager.
*/
ov_json_value *ov_ice_proxy_vocs_msg_register(const char *uuid);

/*----------------------------------------------------------------------------*/

ov_json_value *ov_ice_proxy_vocs_msg_session_completed(const char *uuid,
                                                       ov_ice_state state);

/*
 *      ------------------------------------------------------------------------
 *
 *      DIREKT PARAMETER ACCESS
 *
 *      ------------------------------------------------------------------------
 */

const char *ov_ice_proxy_vocs_msg_get_session_id(const ov_json_value *msg);

/*----------------------------------------------------------------------------*/

const char *
ov_ice_proxy_vocs_msg_get_response_session_id(const ov_json_value *msg);

/*----------------------------------------------------------------------------*/

ov_json_value *ov_ice_proxy_vocs_msg_talk(const char *uuid,
                                          const char *session_uuid, bool on,
                                          ov_mc_loop_data data);

/*----------------------------------------------------------------------------*/

ov_json_value *ov_ice_proxy_vocs_msg_session_state(const char *session_uuid);

#endif /* ov_ice_proxy_vocs_msg_h */
