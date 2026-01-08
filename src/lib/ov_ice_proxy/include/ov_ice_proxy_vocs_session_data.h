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
        @file           ov_ice_proxy_vocs_session_data.h
        @author         Markus Toepfer

        @date           2024-05-08


        ------------------------------------------------------------------------
*/
#ifndef ov_ice_proxy_vocs_session_data_h
#define ov_ice_proxy_vocs_session_data_h

#include <ov_base/ov_id.h>
#include <ov_base/ov_json.h>
#include <ov_base/ov_sdp.h>
#include <ov_base/ov_socket.h>

/*----------------------------------------------------------------------------*/

typedef struct ov_ice_proxy_vocs_session_data {

    ov_id uuid;
    uint32_t ssrc;
    ov_sdp_session *desc;
    ov_socket_data proxy;

} ov_ice_proxy_vocs_session_data;

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_ice_proxy_vocs_session_data
ov_ice_proxy_vocs_session_data_clear(ov_ice_proxy_vocs_session_data *self);

/*---------------------------------------------------------------------------*/

ov_json_value *ov_ice_proxy_vocs_session_data_description_to_json(
    const ov_ice_proxy_vocs_session_data *data);

/*---------------------------------------------------------------------------*/

ov_json_value *ov_ice_proxy_vocs_session_data_to_json(
    const ov_ice_proxy_vocs_session_data *data);

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

#endif /* ov_ice_proxy_vocs_session_data_h */
