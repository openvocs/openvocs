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
        @file           ov_ice_proxy_vocs_app.h
        @author         Markus Toepfer

        @date           2024-05-08


        ------------------------------------------------------------------------
*/
#ifndef ov_ice_proxy_vocs_app_h
#define ov_ice_proxy_vocs_app_h

#include <stdbool.h>

#include <ov_base/ov_event_loop.h>
#include <ov_core/ov_event_io.h>

#include "ov_ice_proxy_vocs.h"

/*----------------------------------------------------------------------------*/

typedef struct ov_ice_proxy_vocs_app ov_ice_proxy_vocs_app;

/*----------------------------------------------------------------------------*/

typedef struct ov_ice_proxy_vocs_app_config {

    ov_event_loop *loop;

    ov_ice_proxy_vocs_config proxy;

    ov_socket_configuration manager;

    struct {

        uint64_t io_timeout_usec;
        uint64_t accept_to_io_timeout_usec;
        uint64_t reconnect_interval_usec;
        uint64_t client_connect_sec;

    } timer;

} ov_ice_proxy_vocs_app_config;

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_ice_proxy_vocs_app *ov_ice_proxy_vocs_app_create(
    ov_ice_proxy_vocs_app_config config);
ov_ice_proxy_vocs_app *ov_ice_proxy_vocs_app_free(ov_ice_proxy_vocs_app *self);
ov_ice_proxy_vocs_app *ov_ice_proxy_vocs_app_cast(const void *userdata);

/*----------------------------------------------------------------------------*/

ov_ice_proxy_vocs_app_config ov_ice_proxy_vocs_app_config_from_json(
    const ov_json_value *v);

/*----------------------------------------------------------------------------*/

ov_event_io_config ov_ice_proxy_vocs_app_io_uri_config(
    ov_ice_proxy_vocs_app *self);

#endif /* ov_ice_proxy_vocs_app_h */
