/***
        ------------------------------------------------------------------------

        Copyright (c) 2022 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_vocs_msg.h
        @author         Markus Töpfer

        @date           2022-11-25


        ------------------------------------------------------------------------
*/
#ifndef ov_vocs_msg_h
#define ov_vocs_msg_h

#include <ov_base/ov_config_keys.h>
#include <ov_base/ov_json.h>

#include <ov_core/ov_event_api.h>
#include <ov_core/ov_media_definitions.h>

#include <ov_ice/ov_ice_candidate.h>
#include <ov_ice/ov_ice_string.h>

#include <ov_vocs_db/ov_vocs_permission.h>

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

/**
    {
        "event":"logout",
        "parameter":{},
        "uuid":"528cc46d-e7cb-4b41-9104-a3e5ff1102b2"
    }
*/
ov_json_value *ov_vocs_msg_logout();

/*----------------------------------------------------------------------------*/

/**
    {
       "event":"login",
       "client":"client",
       "parameter":
       {
           "password":"pass",
           "user":"user"
       },
       "uuid":"9fc88431-dc85-46d3-bfbd-1139ab92d2b1"
    }
*/
ov_json_value *ov_vocs_msg_login(const char *user,
                                 const char *password,
                                 const char *optional_client_id);

/*----------------------------------------------------------------------------*/

/**
    {
       "event":"media",
       "parameter":
       {
           "sdp":"sdp",
           "type":"request"
       },
       "uuid":"efdc2769-a8c6-48d2-aed1-663ce2d094bb"
    }
*/
ov_json_value *ov_vocs_msg_media(ov_media_type type, const char *sdp);

/*----------------------------------------------------------------------------*/

/**
    {
       "event":"candidate",
       "parameter":
       {
           "candidate":"candidate",
           "sdpMLineIndex":0,
           "sdpMid":0,
           "ufrag":"ufrag"
       },
       "uuid":"ee63ba56-6144-4b66-9351-16a10a6f176d"
    }
*/
ov_json_value *ov_vocs_msg_candidate(ov_ice_candidate_info info);

ov_json_value *ov_vocs_msg_end_of_candidates(const char *session_id);

/*----------------------------------------------------------------------------*/

/**
    {
        "event":"authorise",
        "parameter":
        {
            "role":"role"
        },
        "uuid":"d776ce5c-00b5-4bc8-b826-f540d4b9ab45"
    }
*/
ov_json_value *ov_vocs_msg_authorise(const char *role);

/*----------------------------------------------------------------------------*/

/**
    {
        "event":"get",
        "parameter":
        {
            "type":"user"
        },
        "uuid":"7b7f4ae2-55cf-42f1-8bf5-57c41968056b"
    }
*/
ov_json_value *ov_vocs_msg_get(const char *type);

/*----------------------------------------------------------------------------*/

/**
    {
        "event":"user_roles",
        "parameter":{},
        "uuid":"dca6c81e-4518-4676-9963-d8c8364d7b00"
    }
*/
ov_json_value *ov_vocs_msg_client_user_roles();

/*----------------------------------------------------------------------------*/

/**
    {
        "event":"role_loops",
        "parameter":{},
        "uuid":"25277278-28f7-47d4-b68c-6c0368f61354"
    }
*/
ov_json_value *ov_vocs_msg_client_role_loops();

/*----------------------------------------------------------------------------*/

/**
    {
        "event":"switch_loop_state",
        "parameter":
        {
            "loop":"loop",
            "state":"send"
        },
        "uuid":"4052c27e-60a7-4bc0-b7af-ac1f5da1afe7"
    }
*/
ov_json_value *ov_vocs_msg_switch_loop_state(const char *loop,
                                             ov_vocs_permission state);

/*----------------------------------------------------------------------------*/

/**
    {
        "event":"switch_loop_volume",
        "parameter":
        {
            "loop":"loop",
            "volume":50
        },
        "uuid":"cb2010c0-27c7-472c-931c-8969aff8c5c0"
    }
*/
ov_json_value *ov_vocs_msg_switch_loop_volume(const char *loop, uint8_t volume);

/*----------------------------------------------------------------------------*/

/**
    {
        "event":"talking",
        "parameter":
        {
            "loop":"loop",
            "state":false
        },
        "uuid":"de919ff7-e501-4f30-a8a4-243480fa41a8"
    }
*/
ov_json_value *ov_vocs_msg_talking(const char *loop, bool on);

#endif /* ov_vocs_msg_h */
