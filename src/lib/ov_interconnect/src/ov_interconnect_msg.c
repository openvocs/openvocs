/***
        ------------------------------------------------------------------------

        Copyright (c) 2026 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_interconnect_msg.c
        @author         Töpfer, Markus

        @date           2026-01-15


        ------------------------------------------------------------------------
*/
#include "../include/ov_interconnect_msg.h"

#include <ov_base/ov_config_keys.h>
#include <ov_base/ov_error_codes.h>
#include <ov_core/ov_event_api.h>

ov_json_value *ov_interconnect_msg_register(const char *name,
                                            const char *pass) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *par = NULL;

    if (!name || !pass)
        goto error;

    out = ov_event_api_message_create("register", NULL, 0);
    par = ov_event_api_set_parameter(out);

    val = ov_json_string(name);

    if (!ov_json_object_set(par, "name", val))
        goto error;

    val = ov_json_string(pass);
    if (!ov_json_object_set(par, "password", val))
        goto error;

    return out;

error:
    ov_json_value_free(out);
    ov_json_value_free(val);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_interconnect_msg_connect_media(const char *name,
                                                 const char *codec,
                                                 const char *host,
                                                 uint32_t port) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *par = NULL;

    if (!name || !codec || !host || !port)
        goto error;

    out = ov_event_api_message_create("connect_media", NULL, 0);
    par = ov_event_api_set_parameter(out);

    val = ov_json_string(name);

    if (!ov_json_object_set(par, "name", val))
        goto error;

    val = ov_json_string(codec);
    if (!ov_json_object_set(par, "codec", val))
        goto error;

    val = ov_json_string(host);
    if (!ov_json_object_set(par, "host", val))
        goto error;

    val = ov_json_number(port);
    if (!ov_json_object_set(par, "port", val))
        goto error;

    return out;

error:
    ov_json_value_free(out);
    ov_json_value_free(val);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_interconnect_msg_connect_loops() {

    ov_json_value *out = NULL;

    out = ov_event_api_message_create("connect_loops", NULL, 0);

    return out;
}
