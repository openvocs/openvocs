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
        @file           ov_mc_sip_msg.c
        @author         Markus Töpfer
        @author         Michael Beer

        @date           2023-03-24


        ------------------------------------------------------------------------
*/
#include "../include/ov_mc_sip_msg.h"

#include <ov_base/ov_config_keys.h>
#include <ov_base/ov_error_codes.h>
#include <ov_base/ov_id.h>
#include <ov_base/ov_mc_socket.h>
#include <ov_base/ov_result.h>
#include <ov_base/ov_string.h>
#include <ov_core/ov_event_api.h>

/*----------------------------------------------------------------------------*/

ov_json_value *ov_mc_sip_msg_register(const char *uuid) {

    ov_json_value *msg = ov_event_api_message_create(OV_KEY_REGISTER, uuid, 0);
    ov_json_object_set(ov_event_api_set_parameter(msg), OV_KEY_TYPE,
                       ov_json_string("sip_gateway"));
    return msg;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_mc_sip_msg_acquire(const char *uuid, const char *user) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *par = NULL;

    if (!user)
        goto error;

    out = ov_event_api_message_create(OV_KEY_ACQUIRE, uuid, 0);
    par = ov_event_api_set_parameter(out);
    val = ov_json_string(user);
    if (!ov_json_object_set(par, OV_KEY_USER, val))
        goto error;

    return out;

error:
    ov_json_value_free(out);
    ov_json_value_free(val);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_mc_sip_msg_release(const char *uuid, const char *user) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *par = NULL;

    if (!user)
        goto error;

    out = ov_event_api_message_create(OV_KEY_RELEASE, uuid, 0);
    par = ov_event_api_set_parameter(out);
    val = ov_json_string(user);
    if (!ov_json_object_set(par, OV_KEY_USER, val))
        goto error;

    return out;

error:
    ov_json_value_free(out);
    ov_json_value_free(val);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_mc_mixer_core_forward
ov_mc_sip_msg_get_loop_socket(const ov_json_value *val) {

    ov_mc_mixer_core_forward socket = {0};

    if (!val)
        goto error;

    ov_json_value *par = ov_event_api_get_parameter(val);

    socket.socket =
        ov_socket_configuration_from_json(par, (ov_socket_configuration){0});
    socket.payload_type =
        ov_json_number_get(ov_json_get(par, "/" OV_KEY_PAYLOAD_TYPE));
    socket.ssrc = ov_json_number_get(ov_json_get(par, "/" OV_KEY_SSRC));

    return socket;

error:
    return (ov_mc_mixer_core_forward){0};
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_mc_sip_msg_get_multicast(const char *uuid, const char *loop) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *par = NULL;

    if (!loop)
        goto error;

    out = ov_event_api_message_create(OV_SIP_EVENT_GET_MULTICAST, uuid, 0);
    par = ov_event_api_set_parameter(out);
    val = ov_json_string(loop);
    if (!ov_json_object_set(par, OV_KEY_LOOP, val))
        goto error;

    return out;

error:
    ov_json_value_free(out);
    ov_json_value_free(val);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_json_value *
ov_mc_sip_msg_get_multicast_response(const ov_json_value *msg,
                                     ov_socket_configuration loop_socket) {

    ov_json_value *out = ov_event_api_create_success_response(msg);
    ov_json_value *res = ov_event_api_get_response(out);

    if (!ov_ptr_valid(res, "Could not create response message") ||
        !ov_cond_valid(ov_socket_configuration_to_json(loop_socket, &res),
                       "Could not turn multicast socket to json")) {

        out = ov_json_value_free(out);
    }

    return out;
}

/*----------------------------------------------------------------------------*/

ov_response_state
ov_mc_sip_msg_get_response(ov_json_value const *message,
                           ov_socket_configuration *loop_socket) {

    ov_response_state state = {
        .result.error_code = OV_ERROR_CODE_PROCESSING_ERROR,
        .result.message = "Could not parse JSON request"};

    ov_json_value *parameters = ov_event_api_get_response(message);

    ov_socket_configuration scfg = ov_socket_configuration_from_json(
        parameters, (ov_socket_configuration){0});

    if (!ov_response_state_from_message(message, &state) ||
        !ov_ptr_valid(scfg.host,
                      "Could not parse socket information from event")) {

        state.result.error_code = OV_ERROR_CODE_PROCESSING_ERROR;
        state.result.message = "Could not parse JSON request";

    } else if (0 != loop_socket) {

        *loop_socket = scfg;
    }

    return state;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_mc_sip_msg_set_sc(const char *uuid, const char *user,
                                    const char *loop,
                                    ov_mc_mixer_core_forward socket) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *par = NULL;

    out = ov_event_api_message_create(OV_SIP_EVENT_SET_SINGLECAST, uuid, 0);

    if (!ov_socket_configuration_to_json(socket.socket, &val))
        goto error;
    if (!ov_json_object_set(out, OV_KEY_PARAMETER, val))
        goto error;
    par = ov_event_api_get_parameter(out);

    val = ov_json_number(socket.payload_type);
    if (!ov_json_object_set(par, OV_KEY_PAYLOAD_TYPE, val))
        goto error;

    val = ov_json_number(socket.ssrc);
    if (!ov_json_object_set(par, OV_KEY_SSRC, val))
        goto error;

    val = ov_json_string(user);
    if (!ov_json_object_set(par, OV_KEY_USER, val))
        goto error;

    val = ov_json_string(loop);
    if (!ov_json_object_set(par, OV_KEY_LOOP, val))
        goto error;

    return out;

error:
    ov_json_value_free(out);
    ov_json_value_free(val);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_mc_sip_msg_create_call(const char *loop,
                                         const char *destination,
                                         const char *source_uuid) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *par = NULL;

    if (!loop || !destination)
        goto error;

    out = ov_event_api_message_create(OV_SIP_EVENT_CALL, NULL, 0);
    par = ov_event_api_set_parameter(out);

    val = ov_json_string(loop);
    if (!ov_json_object_set(par, OV_KEY_LOOP, val))
        goto error;

    val = ov_json_string(destination);
    if (!ov_json_object_set(par, OV_KEY_CALLEE, val))
        goto error;

    if (source_uuid) {

        val = ov_json_string(source_uuid);
        if (!ov_json_object_set(par, OV_KEY_CALLER, val))
            goto error;
    }

    return out;

error:
    ov_json_value_free(out);
    ov_json_value_free(val);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_mc_sip_msg_terminate_call(const char *call_id) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *par = NULL;

    if (!call_id)
        goto error;

    out = ov_event_api_message_create(OV_KEY_HANGUP, NULL, 0);
    par = ov_event_api_set_parameter(out);

    val = ov_json_string(call_id);
    if (!ov_json_object_set(par, OV_KEY_CALL, val))
        goto error;

    return out;

error:
    ov_json_value_free(out);
    ov_json_value_free(val);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_mc_sip_msg_permit(ov_sip_permission permission) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;

    out = ov_event_api_message_create(OV_KEY_PERMIT, NULL, 0);
    val = ov_sip_permission_to_json(permission);
    if (!ov_json_object_set(out, OV_KEY_PARAMETER, val))
        goto error;

    return out;
error:
    ov_json_value_free(val);
    ov_json_value_free(out);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_mc_sip_msg_revoke(ov_sip_permission permission) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;

    out = ov_event_api_message_create(OV_KEY_REVOKE, NULL, 0);
    val = ov_sip_permission_to_json(permission);
    if (!ov_json_object_set(out, OV_KEY_PARAMETER, val))
        goto error;

    return out;
error:
    ov_json_value_free(val);
    ov_json_value_free(out);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_mc_sip_msg_list_calls(const char *uuid) {

    return ov_event_api_message_create(OV_KEY_LIST_CALLS, uuid, 0);
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_mc_sip_msg_list_permissions(const char *uuid) {

    return ov_event_api_message_create(OV_KEY_LIST_PERMISSIONS, uuid, 0);
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_mc_sip_msg_get_status(const char *uuid) {

    return ov_event_api_message_create(OV_KEY_GET_STATUS, uuid, 0);
}
