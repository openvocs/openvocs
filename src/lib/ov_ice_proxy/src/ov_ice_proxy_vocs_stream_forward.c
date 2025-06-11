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
        @file           ov_ice_proxy_vocs_stream_forward.c
        @author         Markus

        @date           2024-05-08


        ------------------------------------------------------------------------
*/
#include "../include/ov_ice_proxy_vocs_stream_forward.h"

#include <ov_core/ov_event_api.h>

ov_ice_proxy_vocs_stream_forward_data
ov_ice_proxy_vocs_stream_forward_data_from_resmgr_response(
    const ov_json_value *input) {

    ov_ice_proxy_vocs_stream_forward_data out =
        (ov_ice_proxy_vocs_stream_forward_data){0};

    if (!input) goto error;

    const ov_json_value *conf = ov_event_api_get_response(input);
    if (!conf) conf = input;

    out.id = 1;
    out.ssrc = ov_json_number_get(ov_json_object_get(conf, OV_KEY_MSID));
    out.socket = ov_socket_configuration_from_json(
        ov_json_object_get(conf, OV_KEY_MEDIA), (ov_socket_configuration){0});

    return out;
error:
    return (ov_ice_proxy_vocs_stream_forward_data){0};
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_ice_proxy_vocs_stream_forward_data_to_json(
    ov_ice_proxy_vocs_stream_forward_data data) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;

    out = ov_json_object();

    val = ov_json_number(data.id);
    if (!ov_json_object_set(out, OV_KEY_ID, val)) goto error;

    val = ov_json_number(data.ssrc);
    if (!ov_json_object_set(out, OV_KEY_SSRC, val)) goto error;

    val = NULL;
    if (!ov_socket_configuration_to_json(data.socket, &val)) goto error;

    if (!ov_json_object_set(out, OV_KEY_SOCKET, val)) goto error;

    return out;

error:
    ov_json_value_free(out);
    ov_json_value_free(val);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_ice_proxy_vocs_stream_forward_data
ov_ice_proxy_vocs_stream_forward_data_from_json(const ov_json_value *input) {

    ov_ice_proxy_vocs_stream_forward_data out =
        (ov_ice_proxy_vocs_stream_forward_data){0};

    if (!input) goto error;

    out.id = ov_json_number_get(ov_json_object_get(input, OV_KEY_ID));
    out.ssrc = ov_json_number_get(ov_json_object_get(input, OV_KEY_SSRC));
    out.socket = ov_socket_configuration_from_json(
        ov_json_object_get(input, OV_KEY_SOCKET), (ov_socket_configuration){0});

    return out;
error:
    return (ov_ice_proxy_vocs_stream_forward_data){0};
}

/*----------------------------------------------------------------------------*/

bool ov_ice_proxy_vocs_stream_forward_data_check_valid(
    const ov_ice_proxy_vocs_stream_forward_data *data) {

    if (!data) goto error;

    if (0 == data->ssrc) goto error;

    if (0 == data->socket.host[0]) goto error;

    if (0 == data->socket.port) goto error;

    if (NETWORK_TRANSPORT_TYPE_ERROR == data->socket.type) goto error;

    if (data->socket.type >= NETWORK_TRANSPORT_TYPE_OOB) goto error;

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/