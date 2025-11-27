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
        @file           ov_mc_sip_msg.h
        @author         Markus Töpfer

        @date           2023-03-24


        ------------------------------------------------------------------------
*/
#ifndef ov_mc_sip_msg_h
#define ov_mc_sip_msg_h

#include <ov_base/ov_json.h>
#include <ov_base/ov_socket.h>
#include <ov_sip/ov_sip_permission.h>

#include <ov_core/ov_response_state.h>

#include "ov_mc_mixer_core.h"

#define OV_SIP_EVENT_GET_MULTICAST "get_multicast_ip"
#define OV_SIP_EVENT_SET_SINGLECAST "set_singlecast_ip"
#define OV_SIP_EVENT_CALL "call"

/*----------------------------------------------------------------------------*/

typedef struct ov_sip_socket {

    ov_socket_configuration config;
    uint8_t payload_type;

} ov_sip_socket;

/*----------------------------------------------------------------------------*/

/**
        The register event must be send immediately when a connection is opened.
{
        "event":"register",
        "uuid":"550a4101-a209-41a6-aab9-f469793d1929"
}
*/
ov_json_value *ov_mc_sip_msg_register(const char *uuid);

/*----------------------------------------------------------------------------*/

/**
{
        "event":"acquire",
        "uuid":"1e3a6be0-30c5-4121-9f5f-bf72db81e54c"
}
*/
ov_json_value *ov_mc_sip_msg_acquire(const char *uuid, const char *user);

/*----------------------------------------------------------------------------*/

/**
{
        "event":"release",
        "parameter":
        {
                "user":"user1"
        },
        "uuid":"cbbed413-3c43-482c-aabf-f557ec18b1b2"
}
*/
ov_json_value *ov_mc_sip_msg_release(const char *uuid, const char *user);

/*----------------------------------------------------------------------------*/

/**

        Get Multicast IP message
{
        "event":"get_multicast_ip",
        "parameter":
        {
                "loop":"loop1"
        },
        "uuid":"6e009760-f8a5-4353-afe5-eebf8105d4f5"
}
*/
ov_json_value *ov_mc_sip_msg_get_multicast(const char *uuid, const char *loop);

/*----------------------------------------------------------------------------*/

ov_json_value *ov_mc_sip_msg_get_multicast_response(
    const ov_json_value *msg, ov_socket_configuration loop_socket);

/*----------------------------------------------------------------------------*/

ov_response_state ov_mc_sip_msg_get_response(
    ov_json_value const *message, ov_socket_configuration *loop_socket);

/*----------------------------------------------------------------------------*/

/**
        Set Singelcast IP message
{
        "event":"set_singlecast_ip",
        "parameter":
        {
                "host":"127.0.0.1",
                "payload_type":7,
                "port":1234,
                "type":"UDP",
                "ssrc": 1234,
                "loop" : "loop1"
        },
        "uuid":"22ef7f13-3548-4b3a-b2f7-bd9d9a6ca48f"
}
*/
ov_json_value *ov_mc_sip_msg_set_sc(const char *uuid,
                                    const char *user,
                                    const char *loop,
                                    ov_mc_mixer_core_forward socket);

/*----------------------------------------------------------------------------*/

/**

        Get ov_sip_socket_parameter out of some event message parameter.

{
        "event":"set_singlecast_ip",
        "parameter":
        {
                "host":"127.0.0.1",
                "payload_type":7,
                "port":1234,
                "type":"UDP"
        },
        "uuid":"22ef7f13-3548-4b3a-b2f7-bd9d9a6ca48f"
}
*/
ov_mc_mixer_core_forward ov_mc_sip_msg_get_loop_socket(
    const ov_json_value *val);

/*----------------------------------------------------------------------------*/

/**

{
    "event":"call",
    "parameter":
    {
        "callee":"destination",
        "caller":"70c3e835-3f3b-4bf2-aa78-4449b14f378b",
        "loop":"loop"
    },
    "uuid":"e3d4883f-e567-44c4-b855-95b3c61be07b"
}
*/
ov_json_value *ov_mc_sip_msg_create_call(const char *loop,
                                         const char *destination,
                                         const char *source_uuid);

/*----------------------------------------------------------------------------*/

/**
 {
    "event":"hangup",
    "parameter":
    {
        "call":"id"
    },
    "uuid":"8347be3d-c7a3-460a-b61d-5153a6660acb"
}
*/
ov_json_value *ov_mc_sip_msg_terminate_call(const char *call_id);

/*----------------------------------------------------------------------------*/

/**
{
        "event":"permit",
        "parameter":
        {
                "caller":"caller",
                "loop":"loop",
                "valid_from":1,
                "valid_until":2
        },
        "uuid":"56c697af-6924-4549-9b5a-e5a17b6a87ea"
}
*/
ov_json_value *ov_mc_sip_msg_permit(const ov_sip_permission permission);

/*----------------------------------------------------------------------------*/

/**
{
        "event":"revoke",
        "parameter":
        {
                "caller":"caller",
                "loop":"loop",
                "valid_from":1,
                "valid_until":2
        },
        "uuid":"d2c3c559-b55e-4638-b65d-1721d64c425a"
}

*/
ov_json_value *ov_mc_sip_msg_revoke(const ov_sip_permission permission);

/*----------------------------------------------------------------------------*/

ov_json_value *ov_mc_sip_msg_list_calls(const char *uuid);

/*----------------------------------------------------------------------------*/

ov_json_value *ov_mc_sip_msg_list_permissions(const char *uuid);

/*----------------------------------------------------------------------------*/

ov_json_value *ov_mc_sip_msg_get_status(const char *uuid);

#endif /* ov_mc_sip_msg_h */
