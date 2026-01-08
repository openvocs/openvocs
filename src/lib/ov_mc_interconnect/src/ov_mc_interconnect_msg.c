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
        @file           ov_mc_interconnect_msg.c
        @author         Markus Töpfer

        @date           2023-12-11


        ------------------------------------------------------------------------
*/
#include "../include/ov_mc_interconnect_msg.h"

#include <ov_base/ov_config_keys.h>
#include <ov_base/ov_error_codes.h>
#include <ov_core/ov_event_api.h>

ov_json_value *ov_mc_interconnect_msg_register(const char *name,
                                               const char *pass) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *par = NULL;

    if (!name || !pass)
        goto error;

    out = ov_event_api_message_create(OV_MC_INTERCONNECT_REGISTER, NULL,
                                      OV_MC_INTERCONNECT_VERSION);
    par = ov_event_api_set_parameter(out);

    val = ov_json_string(name);

    if (!ov_json_object_set(par, OV_KEY_NAME, val))
        goto error;

    val = ov_json_string(pass);
    if (!ov_json_object_set(par, OV_KEY_PASSWORD, val))
        goto error;

    return out;

error:
    ov_json_value_free(out);
    ov_json_value_free(val);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_mc_interconnect_msg_connect_media(const char *name,
                                                    const char *codec,
                                                    const char *host,
                                                    uint32_t port) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *par = NULL;

    if (!name || !codec || !host || !port)
        goto error;

    out = ov_event_api_message_create(OV_MC_INTERCONNECT_CONNECT_MEDIA, NULL,
                                      OV_MC_INTERCONNECT_VERSION);
    par = ov_event_api_set_parameter(out);

    val = ov_json_string(name);

    if (!ov_json_object_set(par, OV_KEY_NAME, val))
        goto error;

    val = ov_json_string(codec);
    if (!ov_json_object_set(par, OV_KEY_CODEC, val))
        goto error;

    val = ov_json_string(host);
    if (!ov_json_object_set(par, OV_KEY_HOST, val))
        goto error;

    val = ov_json_number(port);
    if (!ov_json_object_set(par, OV_KEY_PORT, val))
        goto error;

    return out;

error:
    ov_json_value_free(out);
    ov_json_value_free(val);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_mc_interconnect_msg_connect_loops() {

    ov_json_value *out = NULL;

    out = ov_event_api_message_create(OV_MC_INTERCONNECT_CONNECT_LOOPS, NULL,
                                      OV_MC_INTERCONNECT_VERSION);

    return out;
}
