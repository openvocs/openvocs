/***
        ------------------------------------------------------------------------

        Copyright (c) 2025 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_mc_backend_sip_static.h
        @author         Töpfer, Markus

        @date           2025-04-14


        ------------------------------------------------------------------------
*/
#ifndef ov_mc_backend_sip_static_h
#define ov_mc_backend_sip_static_h

#include "ov_mc_backend.h"
#include <ov_base/ov_event_loop.h>
#include <ov_base/ov_socket.h>
#include <ov_core/ov_io.h>
#include <ov_vocs_db/ov_vocs_db.h>

/*----------------------------------------------------------------------------*/

typedef struct ov_mc_backend_sip_static ov_mc_backend_sip_static;

/*----------------------------------------------------------------------------*/

typedef struct ov_mc_backend_sip_static_config {

    ov_event_loop *loop;
    ov_vocs_db *db;
    ov_io *io;
    ov_mc_backend *backend;

    struct {

        ov_socket_configuration manager; // manager liege socket

    } socket;

} ov_mc_backend_sip_static_config;

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_mc_backend_sip_static *
ov_mc_backend_sip_static_create(ov_mc_backend_sip_static_config config);
ov_mc_backend_sip_static *
ov_mc_backend_sip_static_free(ov_mc_backend_sip_static *self);
ov_mc_backend_sip_static *ov_mc_backend_sip_static_cast(const void *self);

/*----------------------------------------------------------------------------*/

ov_mc_backend_sip_static_config
ov_mc_backend_sip_static_config_from_json(const ov_json_value *val);

#endif /* ov_mc_backend_sip_static_h */
