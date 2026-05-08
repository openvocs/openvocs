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
        @file           ov_ice_proxy_vocs_msg.c
        @author         Markus

        @date           2024-05-08


        ------------------------------------------------------------------------
*/
#include "../include/ov_ice_proxy_vocs_msg.h"

#include <ov_base/ov_config_keys.h>
#include <ov_base/ov_id.h>
#include <ov_core/ov_event_api.h>
#include <ov_core/ov_media_definitions.h>

/*
 *      ------------------------------------------------------------------------
 *
 *      Messages
 *
 *      ------------------------------------------------------------------------
 */

static ov_json_value *api_message_create(const char *name) {

    ov_json_value *out = NULL;

    if (!name)
        goto error;

    ov_id uuid = {0};
    ov_id_fill_with_uuid(uuid);

    out =
        ov_event_api_message_create(name, uuid, ov_ice_proxy_vocs_API_VERSION);

    if (!out)
        goto error;

    return out;
error:
    out = ov_json_value_free(out);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_ice_proxy_vocs_msg_create_session() {

    ov_json_value *out = NULL;

    out = api_message_create(OV_KEY_ICE_SESSION_CREATE);
    if (!out)
        goto error;

    return out;
error:
    ov_json_value_free(out);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_ice_proxy_vocs_msg_create_session_with_sdp(const char *uuid,
                                                             const char *sdp) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;

    if (!uuid || !sdp)
        goto error;

    out = ov_event_api_message_create(OV_KEY_ICE_SESSION_CREATE, uuid, 0);

    ov_json_value *params = ov_event_api_set_parameter(out);
    if (!params)
        goto error;

    if (!ov_event_api_set_type(params, ov_media_type_to_string(OV_MEDIA_OFFER)))
        goto error;

    val = ov_json_string(sdp);
    if (!ov_json_object_set(params, OV_KEY_SDP, val))
        goto error;

    return out;
error:
    ov_json_value_free(out);
    ov_json_value_free(val);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_ice_proxy_vocs_msg_session_failed(const char *session_id) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;

    if (!session_id)
        goto error;

    out = api_message_create(OV_KEY_ICE_SESSION_FAILED);
    if (!out)
        goto error;

    ov_json_value *par = ov_event_api_set_parameter(out);
    val = ov_json_string(session_id);
    if (!ov_json_object_set(par, OV_KEY_SESSION, val))
        goto error;

    return out;
error:
    ov_json_value_free(out);
    ov_json_value_free(val);
    return NULL;
}
/*----------------------------------------------------------------------------*/

ov_json_value *ov_ice_proxy_vocs_msg_drop_session(const char *session_uuid) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;

    if (!session_uuid)
        goto error;

    out = api_message_create(OV_KEY_ICE_SESSION_DROP);
    if (!out)
        goto error;

    ov_json_value *par = ov_event_api_set_parameter(out);
    if (!par)
        goto error;

    val = ov_json_string(session_uuid);
    if (!ov_json_object_set(par, OV_KEY_SESSION, val))
        goto error;

    return out;
error:
    ov_json_value_free(val);
    ov_json_value_free(out);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_ice_proxy_vocs_msg_client_response_from_ice_session_create(
    const ov_json_value *client_request,
    const ov_json_value *session_response) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;

    if (!client_request || !session_response)
        goto error;

    /*  Create following JSON

        {
            "event" : "media",
            "uuid" : "<uuid of request>",
            "request" : { ... },
            "response" :
            {
                "session" : "<ice_session_uuid>",
                "type" : "offer",
                "sdp" : "<sdp_string>"
            }
        }
    */

    /* Check session response message data */

    if (NULL != ov_event_api_get_error(session_response))
        goto error;

    ov_json_value *res = ov_event_api_get_response(session_response);
    if (!res)
        goto error;

    if (OV_MEDIA_OFFER != ov_media_type_from_string(ov_event_api_get_type(res)))
        goto error;

    const char *sdp = ov_json_string_get(ov_json_object_get(res, OV_KEY_SDP));
    if (!sdp)
        goto error;

    const char *ice_session_id =
        ov_json_string_get(ov_json_object_get(res, OV_KEY_SESSION));

    if (!ice_session_id)
        goto error;

    /* create client response */

    out = ov_event_api_create_success_response(client_request);
    if (!out)
        goto error;

    ov_json_value *response = ov_event_api_get_response(out);
    if (!response)
        goto error;

    if (!ov_event_api_set_type(response,
                               ov_media_type_to_string(OV_MEDIA_OFFER)))
        goto error;

    val = ov_json_string(ice_session_id);
    if (!ov_json_object_set(response, OV_KEY_SESSION, val))
        goto error;

    val = ov_json_string(sdp);
    if (!ov_json_object_set(response, OV_KEY_SDP, val))
        goto error;

    return out;
error:
    ov_json_value_free(out);
    ov_json_value_free(val);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_ice_proxy_vocs_msg_register(const char *uuid) {

    ov_json_value *out = NULL;

    if (!uuid)
        goto error;

    /*  Create following JSON

        {
            "event" : "register",
            "uuid" : "<uuid of request>",
            "request" : {
                "uuid" : "<input uuid>"
            }
        }
    */

    out = ov_event_api_message_create(OV_EVENT_API_REGISTER, NULL, 0);

    if (!out)
        goto error;

    ov_json_value *params = ov_event_api_set_parameter(out);
    if (!params)
        goto error;

    if (!ov_event_api_set_uuid(params, uuid))
        goto error;

    return out;
error:
    ov_json_value_free(out);
    return NULL;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      DIREKT PARAMETER ACCESS
 *
 *      ------------------------------------------------------------------------
 */

const char *ov_ice_proxy_vocs_msg_get_session_id(const ov_json_value *msg) {

    if (!msg)
        goto error;

    ov_json_value *params = ov_event_api_get_parameter(msg);
    if (!params)
        goto error;

    return ov_json_string_get(ov_json_object_get(params, OV_KEY_SESSION));

error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

const char *
ov_ice_proxy_vocs_msg_get_response_session_id(const ov_json_value *msg) {

    if (!msg)
        goto error;

    ov_json_value *params = ov_event_api_get_response(msg);
    if (!params)
        goto error;

    return ov_json_string_get(ov_json_object_get(params, OV_KEY_SESSION));

error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_ice_proxy_vocs_msg_forward_stream(
    const char *session_uuid, ov_ice_proxy_vocs_stream_forward_data data) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;

    if ((0 == data.socket.host[0]) || (0 == data.socket.port) ||
        (0 == data.ssrc))
        goto error;

    out =
        ov_event_api_message_create(OV_KEY_ICE_SESSION_FORWARD_STREAM, NULL, 0);

    if (!out)
        goto error;

    ov_json_value *params = ov_event_api_set_parameter(out);
    if (!params)
        goto error;

    if (session_uuid) {
        val = ov_json_string(session_uuid);
        if (!ov_json_object_set(params, OV_KEY_SESSION, val))
            goto error;
    }

    val = ov_ice_proxy_vocs_stream_forward_data_to_json(data);
    if (!ov_json_object_set(params, OV_KEY_STREAM, val))
        goto error;

    return out;

error:
    ov_json_value_free(out);
    ov_json_value_free(val);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_json_value *
ov_ice_proxy_vocs_msg_candidate(const char *session_uuid,
                                const ov_ice_candidate_info *info) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;

    if (!info)
        goto error;

    if (!info->ufrag || !info->candidate)
        goto error;

    out = ov_event_api_message_create(OV_ICE_STRING_CANDIDATE, NULL, 0);

    if (!out)
        goto error;

    ov_json_value *params = ov_event_api_set_parameter(out);
    if (!params)
        goto error;

    if (session_uuid) {
        val = ov_json_string(session_uuid);
        if (!ov_json_object_set(params, OV_KEY_SESSION, val))
            goto error;
    }

    val = ov_json_number(info->SDPMid);
    if (!ov_json_object_set(params, OV_ICE_STRING_SDP_MID, val))
        goto error;

    val = ov_json_number(info->SDPMlineIndex);
    if (!ov_json_object_set(params, OV_ICE_STRING_SDP_MLINEINDEX, val))
        goto error;

    val = ov_json_string(info->ufrag);
    if (!ov_json_object_set(params, OV_ICE_STRING_UFRAG, val))
        goto error;

    val = ov_json_string(info->candidate);
    if (!ov_json_object_set(params, OV_ICE_STRING_CANDIDATE, val))
        goto error;

    return out;
error:
    ov_json_value_free(out);
    ov_json_value_free(val);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_json_value *
ov_ice_proxy_vocs_msg_end_of_candidates(const char *session_uuid) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;

    out = ov_event_api_message_create(OV_ICE_STRING_END_OF_CANDIDATES, NULL, 0);

    if (!out)
        goto error;

    ov_json_value *params = ov_event_api_set_parameter(out);
    if (!params)
        goto error;

    if (session_uuid) {
        val = ov_json_string(session_uuid);
        if (!ov_json_object_set(params, OV_KEY_SESSION, val))
            goto error;
    }

    return out;
error:
    ov_json_value_free(out);
    ov_json_value_free(val);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_ice_proxy_vocs_msg_media(const char *session_id,
                                           ov_media_type type,
                                           const char *sdp) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;

    if (!sdp)
        goto error;

    switch (type) {

    case OV_MEDIA_ANSWER:
    case OV_MEDIA_OFFER:
        break;

    default:
        goto error;
    }

    out = ov_event_api_message_create(OV_KEY_MEDIA, NULL, 0);

    if (!out)
        goto error;

    ov_json_value *params = ov_event_api_set_parameter(out);
    if (!params)
        goto error;

    if (session_id) {
        val = ov_json_string(session_id);
        if (!ov_json_object_set(params, OV_KEY_SESSION, val))
            goto error;
    }

    val = ov_json_string(ov_media_type_to_string(type));
    if (!ov_json_object_set(params, OV_KEY_TYPE, val))
        goto error;

    val = ov_json_string(sdp);
    if (!ov_json_object_set(params, OV_KEY_SDP, val))
        goto error;

    return out;
error:
    ov_json_value_free(val);
    ov_json_value_free(out);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_ice_proxy_vocs_msg_session_completed(const char *uuid,
                                                       ov_ice_state state) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;

    if (!uuid)
        goto error;

    out = ov_event_api_message_create(OV_KEY_ICE_SESSION_COMPLETED, NULL, 0);

    ov_json_value *params = ov_event_api_set_parameter(out);
    if (!params)
        goto error;

    val = ov_json_string(uuid);
    if (!ov_json_object_set(params, OV_KEY_SESSION, val))
        goto error;

    val = ov_json_string(ov_ice_state_to_string(state));
    if (!ov_json_object_set(params, OV_KEY_STATE, val))
        goto error;

    return out;
error:
    ov_json_value_free(out);
    ov_json_value_free(val);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_ice_proxy_vocs_msg_update_session(const char *uuid,
                                                    const char *session_uuid,
                                                    ov_media_type type,
                                                    const char *sdp) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;

    if (!uuid || !sdp)
        goto error;

    out = ov_event_api_message_create(OV_KEY_ICE_SESSION_UPDATE, uuid, 0);

    ov_json_value *params = ov_event_api_set_parameter(out);
    if (!params)
        goto error;

    if (!ov_event_api_set_type(params, ov_media_type_to_string(type)))
        goto error;

    val = ov_json_string(session_uuid);
    if (!ov_json_object_set(params, OV_KEY_SESSION, val))
        goto error;

    val = ov_json_string(sdp);
    if (!ov_json_object_set(params, OV_KEY_SDP, val))
        goto error;

    return out;
error:
    ov_json_value_free(out);
    ov_json_value_free(val);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_ice_proxy_vocs_msg_talk(const char *uuid,
                                          const char *session_uuid, bool on,
                                          ov_mc_loop_data data) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;

    if (!uuid || !session_uuid)
        goto error;
    if (0 == data.socket.host[0])
        goto error;
    if (0 == data.name[0])
        goto error;

    out = ov_event_api_message_create(OV_KEY_TALK, uuid, 0);

    ov_json_value *params = ov_event_api_set_parameter(out);
    if (!params)
        goto error;

    val = ov_json_string(session_uuid);
    if (!ov_json_object_set(params, OV_KEY_SESSION, val))
        goto error;

    val = ov_mc_loop_data_to_json(data);
    if (!ov_json_object_set(params, OV_KEY_LOOP, val))
        goto error;

    if (on) {
        val = ov_json_true();
    } else {
        val = ov_json_false();
    }
    if (!ov_json_object_set(params, OV_KEY_ON, val))
        goto error;

    return out;
error:
    ov_json_value_free(out);
    ov_json_value_free(val);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_ice_proxy_vocs_msg_session_state(const char *session_uuid) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;

    if (!session_uuid)
        goto error;

    out = ov_event_api_message_create(OV_KEY_ICE_SESSION_STATE, NULL, 0);

    ov_json_value *params = ov_event_api_set_parameter(out);
    if (!params)
        goto error;

    val = ov_json_string(session_uuid);
    if (!ov_json_object_set(params, OV_KEY_SESSION, val))
        goto error;

    return out;
error:
    ov_json_value_free(out);
    ov_json_value_free(val);
    return NULL;
}