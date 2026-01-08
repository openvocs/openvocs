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
        @file           ov_ice_proxy_vocs_stream_forward.h
        @author         Markus Toepfer

        @date           2024-05-08


        ------------------------------------------------------------------------
*/
#ifndef ov_ice_proxy_vocs_stream_forward_h
#define ov_ice_proxy_vocs_stream_forward_h

#include <ov_base/ov_config_keys.h>
#include <ov_base/ov_json.h>
#include <ov_base/ov_json_value.h>
#include <ov_base/ov_socket.h>

typedef struct {

  uint16_t id;
  uint32_t ssrc;
  ov_socket_configuration socket;

} ov_ice_proxy_vocs_stream_forward_data;

/*----------------------------------------------------------------------------*/

/**
    Create the forward data from some resmgr response.

    e.g.
    {
        "response": {
            "media": {
                "port": 38737,
                "host": "10.61.11.225",
                "type": "UDP"
            },
            "msid": 32000
        }
    }

    This function will set the id to 1 on success.
*/
ov_ice_proxy_vocs_stream_forward_data
ov_ice_proxy_vocs_stream_forward_data_from_resmgr_response(
    const ov_json_value *response);

/*----------------------------------------------------------------------------*/

ov_ice_proxy_vocs_stream_forward_data
ov_ice_proxy_vocs_stream_forward_data_from_json(const ov_json_value *input);

/*----------------------------------------------------------------------------*/

ov_json_value *ov_ice_proxy_vocs_stream_forward_data_to_json(
    ov_ice_proxy_vocs_stream_forward_data data);

/*----------------------------------------------------------------------------*/

bool ov_ice_proxy_vocs_stream_forward_data_check_valid(
    const ov_ice_proxy_vocs_stream_forward_data *data);

#endif /* ov_ice_proxy_vocs_stream_forward_h */
