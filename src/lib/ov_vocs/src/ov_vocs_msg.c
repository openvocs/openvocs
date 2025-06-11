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
        @file           ov_vocs_msg.c
        @author         Markus Töpfer

        @date           2022-11-25


        ------------------------------------------------------------------------
*/
#include "../include/ov_vocs_msg.h"

ov_json_value *ov_vocs_msg_logout() {

    ov_json_value *out = NULL;
    ov_json_value *par = NULL;

    out = ov_event_api_message_create(OV_EVENT_API_LOGOUT, NULL, 0);
    par = ov_event_api_set_parameter(out);
    if (!par) goto error;

    return out;
error:
    ov_json_value_free(out);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_vocs_msg_login(const char *user,
                                 const char *password,
                                 const char *optional_client_id) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *par = NULL;

    out = ov_event_api_message_create(OV_EVENT_API_LOGIN, NULL, 0);
    par = ov_event_api_set_parameter(out);
    if (!par) goto error;

    if (user) {

        val = ov_json_string(user);
        if (!ov_json_object_set(par, OV_KEY_USER, val)) goto error;
    }

    if (password) {

        val = ov_json_string(password);
        if (!ov_json_object_set(par, OV_KEY_PASSWORD, val)) goto error;
    }

    if (optional_client_id) {

        val = ov_json_string(optional_client_id);
        if (!ov_json_object_set(out, OV_KEY_CLIENT, val)) goto error;
    }

    return out;
error:
    ov_json_value_free(val);
    ov_json_value_free(out);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_vocs_msg_media(ov_media_type type, const char *sdp) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *par = NULL;

    if (type == OV_MEDIA_ERROR) goto error;

    out = ov_event_api_message_create(OV_EVENT_API_MEDIA, NULL, 0);
    par = ov_event_api_set_parameter(out);
    if (!par) goto error;

    val = ov_json_string(ov_media_type_to_string(type));
    if (!ov_json_object_set(par, OV_KEY_TYPE, val)) goto error;

    if (sdp) {

        val = ov_json_string(sdp);
        if (!ov_json_object_set(par, OV_KEY_SDP, val)) goto error;
    }

    return out;
error:
    ov_json_value_free(val);
    ov_json_value_free(out);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_vocs_msg_candidate(ov_ice_candidate_info info) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;

    out = ov_event_api_message_create(OV_ICE_STRING_CANDIDATE, NULL, 0);
    val = ov_ice_candidate_info_to_json(info);
    if (!ov_json_object_set(out, OV_KEY_PARAMETER, val)) goto error;

    return out;
error:
    ov_json_value_free(val);
    ov_json_value_free(out);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_vocs_msg_end_of_candidates(const char *session_id) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *par = NULL;

    out = ov_event_api_message_create(OV_ICE_STRING_END_OF_CANDIDATES, NULL, 0);
    par = ov_event_api_set_parameter(out);
    if (!par) goto error;

    val = ov_json_string(session_id);
    if (!ov_json_object_set(par, OV_KEY_SESSION, val)) goto error;

    return out;
error:
    ov_json_value_free(val);
    ov_json_value_free(out);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_vocs_msg_authorise(const char *role) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *par = NULL;

    out = ov_event_api_message_create(OV_EVENT_API_AUTHORISE, NULL, 0);
    par = ov_event_api_set_parameter(out);
    if (!par) goto error;

    if (role) {

        val = ov_json_string(role);
        if (!ov_json_object_set(par, OV_KEY_ROLE, val)) goto error;
    }

    return out;
error:
    ov_json_value_free(val);
    ov_json_value_free(out);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_vocs_msg_get(const char *type) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *par = NULL;

    out = ov_event_api_message_create(OV_EVENT_API_GET, NULL, 0);
    par = ov_event_api_set_parameter(out);
    if (!par) goto error;

    if (type) {

        val = ov_json_string(type);
        if (!ov_json_object_set(par, OV_KEY_TYPE, val)) goto error;
    }

    return out;
error:
    ov_json_value_free(val);
    ov_json_value_free(out);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_vocs_msg_client_user_roles() {

    ov_json_value *out = NULL;
    ov_json_value *par = NULL;

    out = ov_event_api_message_create(OV_EVENT_API_USER_ROLES, NULL, 0);
    par = ov_event_api_set_parameter(out);
    if (!par) goto error;

    return out;
error:
    ov_json_value_free(out);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_vocs_msg_client_role_loops() {

    ov_json_value *out = NULL;
    ov_json_value *par = NULL;

    out = ov_event_api_message_create(OV_EVENT_API_ROLE_LOOPS, NULL, 0);
    par = ov_event_api_set_parameter(out);
    if (!par) goto error;

    return out;
error:
    ov_json_value_free(out);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_vocs_msg_switch_loop_state(const char *loop,
                                             ov_vocs_permission state) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *par = NULL;

    out = ov_event_api_message_create(OV_EVENT_API_SWITCH_LOOP_STATE, NULL, 0);
    par = ov_event_api_set_parameter(out);
    if (!par) goto error;

    if (loop) {

        val = ov_json_string(loop);
        if (!ov_json_object_set(par, OV_KEY_LOOP, val)) goto error;
    }

    val = ov_json_string(ov_vocs_permission_to_string(state));
    if (!ov_json_object_set(par, OV_KEY_STATE, val)) goto error;

    return out;
error:
    ov_json_value_free(val);
    ov_json_value_free(out);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_vocs_msg_switch_loop_volume(const char *loop,
                                              uint8_t volume) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *par = NULL;

    out = ov_event_api_message_create(OV_EVENT_API_SWITCH_LOOP_VOLUME, NULL, 0);
    par = ov_event_api_set_parameter(out);
    if (!par) goto error;

    if (loop) {

        val = ov_json_string(loop);
        if (!ov_json_object_set(par, OV_KEY_LOOP, val)) goto error;
    }

    val = ov_json_number(volume);
    if (!ov_json_object_set(par, OV_KEY_VOLUME, val)) goto error;

    return out;
error:
    ov_json_value_free(val);
    ov_json_value_free(out);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_vocs_msg_talking(const char *loop, bool on) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *par = NULL;

    out = ov_event_api_message_create(OV_EVENT_API_TALKING, NULL, 0);
    par = ov_event_api_set_parameter(out);
    if (!par) goto error;

    if (loop) {

        val = ov_json_string(loop);
        if (!ov_json_object_set(par, OV_KEY_LOOP, val)) goto error;
    }

    if (on) {
        val = ov_json_true();
    } else {
        val = ov_json_false();
    }
    if (!ov_json_object_set(par, OV_KEY_STATE, val)) goto error;

    return out;
error:
    ov_json_value_free(val);
    ov_json_value_free(out);
    return NULL;
}
