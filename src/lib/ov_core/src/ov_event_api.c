/***
        ------------------------------------------------------------------------

        Copyright (c) 2021 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_event_api.c
        @author         Markus Töpfer

        @date           2021-02-04


        ------------------------------------------------------------------------
*/
#include "../include/ov_event_api.h"

#include <ov_base/ov_id.h>
#include <ov_base/ov_time.h>

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

bool ov_event_api_event_is(const ov_json_value *val, const char *event) {

    const char *check = ov_event_api_get_event(val);
    if (!check || !event) goto error;

    if (0 == strcmp(check, event)) return true;

error:
    return false;
}

/*----------------------------------------------------------------------------*/

const char *ov_event_api_get_event(const ov_json_value *val) {
    return ov_json_string_get(ov_json_object_get(val, OV_EVENT_API_KEY_EVENT));
}

/*----------------------------------------------------------------------------*/

const char *ov_event_api_get_uuid(const ov_json_value *val) {
    return ov_json_string_get(ov_json_object_get(val, OV_EVENT_API_KEY_UUID));
}

/*----------------------------------------------------------------------------*/

const char *ov_event_api_get_type(const ov_json_value *val) {
    return ov_json_string_get(ov_json_object_get(val, OV_EVENT_API_KEY_TYPE));
}

/*----------------------------------------------------------------------------*/

const char *ov_event_api_get_channel(const ov_json_value *val) {
    return ov_json_string_get(
        ov_json_object_get(val, OV_EVENT_API_KEY_CHANNEL));
}

/*----------------------------------------------------------------------------*/

double ov_event_api_get_version(const ov_json_value *val) {
    return ov_json_number_get(
        ov_json_object_get(val, OV_EVENT_API_KEY_VERSION));
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_event_api_get_error(const ov_json_value *val) {
    return ov_json_object_get(val, OV_EVENT_API_KEY_ERROR);
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_event_api_get_request(const ov_json_value *val) {
    return ov_json_object_get(val, OV_EVENT_API_KEY_REQUEST);
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_event_api_get_response(const ov_json_value *val) {
    return ov_json_object_get(val, OV_EVENT_API_KEY_RESPONSE);
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_event_api_get_parameter(const ov_json_value *val) {
    return ov_json_object_get(val, OV_EVENT_API_KEY_PARAMETER);
}

/*----------------------------------------------------------------------------*/

uint64_t ov_event_api_get_error_code(const ov_json_value *val) {

    return (uint64_t)ov_json_number_get(
        ov_json_object_get(ov_event_api_get_error(val), OV_EVENT_API_KEY_CODE));
}

/*----------------------------------------------------------------------------*/

bool ov_event_api_get_error_parameter(const ov_json_value *val,
                                      uint64_t *code,
                                      const char **desc) {

    if (!val || !code || !desc) goto error;

    ov_json_value const *error = ov_json_get(val, "/" OV_EVENT_API_KEY_ERROR);

    if (!error) {

        *code = 0;
        *desc = NULL;

    } else {

        *code =
            ov_json_number_get(ov_json_get(error, "/" OV_EVENT_API_KEY_CODE));
        *desc = ov_json_string_get(
            ov_json_get(error, "/" OV_EVENT_API_KEY_DESCRIPTION));
    }

    return true;

error:
    return false;
}
/*----------------------------------------------------------------------------*/

bool ov_event_api_set_event(ov_json_value *val, const char *str) {

    ov_json_value *data = ov_json_string(str);
    if (ov_json_object_set(val, OV_EVENT_API_KEY_EVENT, data)) return true;
    data = ov_json_value_free(data);
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_event_api_set_type(ov_json_value *val, const char *str) {

    ov_json_value *data = ov_json_string(str);
    if (ov_json_object_set(val, OV_EVENT_API_KEY_TYPE, data)) return true;
    data = ov_json_value_free(data);
    return false;
}
/*----------------------------------------------------------------------------*/

bool ov_event_api_set_uuid(ov_json_value *val, const char *str) {

    ov_json_value *data = ov_json_string(str);
    if (ov_json_object_set(val, OV_EVENT_API_KEY_UUID, data)) return true;
    data = ov_json_value_free(data);
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_event_api_set_channel(ov_json_value *val, const char *str) {

    ov_json_value *data = ov_json_string(str);
    if (ov_json_object_set(val, OV_EVENT_API_KEY_CHANNEL, data)) return true;
    data = ov_json_value_free(data);
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_event_api_set_version(ov_json_value *val, double version) {

    ov_json_value *data = ov_json_number(version);
    if (ov_json_object_set(val, OV_EVENT_API_KEY_VERSION, data)) return true;
    data = ov_json_value_free(data);
    return false;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_event_api_set_parameter(ov_json_value *val) {

    ov_json_value *data = ov_json_object();
    if (ov_json_object_set(val, OV_EVENT_API_KEY_PARAMETER, data)) return data;

    data = ov_json_value_free(data);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_event_api_message_create(const char *name,
                                           const char *id,
                                           double version) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;

    ov_id uuid = {0};
    ov_id_fill_with_uuid(uuid);

    if (!name) goto error;

    out = ov_json_object();
    if (!out) goto error;

    val = ov_json_string(name);
    if (!ov_json_object_set(out, OV_EVENT_API_KEY_EVENT, val)) goto error;

    if (!id) {
        id = uuid;
    }

    val = ov_json_string(id);
    if (!ov_json_object_set(out, OV_EVENT_API_KEY_UUID, val)) goto error;

    if (0 != version) {
        val = ov_json_number(version);
        if (!ov_json_object_set(out, OV_EVENT_API_KEY_VERSION, val)) goto error;
    }

    return out;
error:
    ov_json_value_free(val);
    ov_json_value_free(out);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_event_api_create_success_response(
    const ov_json_value *input) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;

    if (!input) goto error;

    const char *event = ov_event_api_get_event(input);
    const char *id = ov_event_api_get_uuid(input);
    double version = ov_event_api_get_version(input);
    const char *client =
        ov_json_string_get(ov_json_object_get(input, OV_EVENT_API_KEY_CLIENT));

    if (!event) goto error;

    out = ov_event_api_message_create(event, id, version);
    if (!out) goto error;

    if (!ov_json_value_copy((void **)&val, input)) goto error;

    if (!ov_json_object_set(out, OV_EVENT_API_KEY_REQUEST, val)) goto error;

    val = ov_json_object();
    if (!ov_json_object_set(out, OV_EVENT_API_KEY_RESPONSE, val)) goto error;

    if (client) {

        val = ov_json_string(client);
        if (!ov_json_object_set(out, OV_EVENT_API_KEY_CLIENT, val)) goto error;
    }
    return out;
error:
    ov_json_value_free(out);
    ov_json_value_free(val);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_event_api_create_error_response(const ov_json_value *input,
                                                  uint64_t code,
                                                  const char *desc) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *obj = NULL;

    if (!input) return ov_event_api_create_error(code, desc);

    out = ov_event_api_create_success_response(input);
    if (!out) goto error;

    val = ov_json_object();
    if (!ov_json_object_set(out, OV_EVENT_API_KEY_ERROR, val)) goto error;

    obj = val;

    val = ov_json_number(code);
    if (!ov_json_object_set(obj, OV_EVENT_API_KEY_CODE, val)) goto error;

    val = ov_json_string(desc);
    if (!ov_json_object_set(obj, OV_EVENT_API_KEY_DESCRIPTION, val)) goto error;

    return out;
error:
    ov_json_value_free(out);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_event_api_create_error(uint64_t code, const char *desc) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *obj = NULL;

    out = ov_json_object();
    if (!out) goto error;

    val = ov_json_object();
    if (!ov_json_object_set(out, OV_EVENT_API_KEY_ERROR, val)) goto error;

    obj = val;

    val = ov_json_number(code);
    if (!ov_json_object_set(obj, OV_EVENT_API_KEY_CODE, val)) goto error;

    val = ov_json_string(desc);
    if (!ov_json_object_set(obj, OV_EVENT_API_KEY_DESCRIPTION, val)) goto error;

    return out;
error:
    ov_json_value_free(out);
    return NULL;
}

/*----------------------------------------------------------------------------*/

bool ov_event_api_add_error(ov_json_value *msg,
                            uint64_t code,
                            const char *desc) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *obj = NULL;

    out = ov_json_object();
    if (!out) goto error;

    val = ov_json_object();
    if (!ov_json_object_set(msg, OV_EVENT_API_KEY_ERROR, val)) goto error;

    obj = val;

    val = ov_json_number(code);
    if (!ov_json_object_set(obj, OV_EVENT_API_KEY_CODE, val)) goto error;

    val = ov_json_string(desc);
    if (!ov_json_object_set(obj, OV_EVENT_API_KEY_DESCRIPTION, val)) goto error;

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_event_api_create_error_code(uint64_t code, const char *desc) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;

    out = ov_json_object();
    if (!out) goto error;

    val = ov_json_number(code);
    if (!ov_json_object_set(out, OV_EVENT_API_KEY_CODE, val)) goto error;

    val = ov_json_string(desc);
    if (!ov_json_object_set(out, OV_EVENT_API_KEY_DESCRIPTION, val)) goto error;

    return out;
error:
    ov_json_value_free(out);
    return NULL;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      COMMON API EVENT MESSAGES
 *
 *      ------------------------------------------------------------------------
 */

ov_json_value *ov_event_api_create_ping() {

    ov_json_value *out = ov_json_object();
    ov_json_value *val = ov_json_string(OV_EVENT_API_PING);
    if (!ov_json_object_set(out, OV_EVENT_API_KEY_EVENT, val)) goto error;

    return out;
error:
    ov_json_value_free(out);
    ov_json_value_free(val);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_event_api_create_pong() {

    char *timestamp = ov_timestamp(true);

    ov_json_value *out = ov_json_object();
    ov_json_value *val = ov_json_string(OV_EVENT_API_PONG);
    if (!ov_json_object_set(out, OV_EVENT_API_KEY_EVENT, val)) goto error;

    val = ov_json_string(timestamp);
    if (!ov_json_object_set(out, OV_EVENT_API_KEY_TIMESTAMP, val)) goto error;

    timestamp = ov_data_pointer_free(timestamp);
    return out;
error:
    ov_json_value_free(out);
    ov_json_value_free(val);
    timestamp = ov_data_pointer_free(timestamp);
    return NULL;
}

/*----------------------------------------------------------------------------*/

bool ov_event_api_set_current_timestamp(ov_json_value *value) {

    if (!value) return false;

    char *timestamp = ov_timestamp(true);

    ov_json_value *val = ov_json_string(timestamp);
    if (ov_json_object_set(value, OV_EVENT_API_KEY_TIMESTAMP, val)) return true;

    val = ov_json_value_free(val);
    return false;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_event_api_create_path(const char *path) {

    if (!path) return NULL;

    ov_json_value *out = ov_json_object();
    ov_json_value *val = ov_json_string(path);

    if (!ov_json_object_set(out, OV_EVENT_API_KEY_PATH, val)) goto error;

    return out;
error:
    ov_json_value_free(out);
    ov_json_value_free(val);
    return NULL;
}

/*----------------------------------------------------------------------------*/

const char *ov_event_api_get_path(const ov_json_value *input) {

    return ov_json_string_get(ov_json_object_get(input, OV_EVENT_API_KEY_PATH));
}
