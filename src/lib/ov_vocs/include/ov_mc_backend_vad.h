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
        @file           ov_mc_backend_vad.h
        @author         Markus Töpfer

        @date           2025-01-11


        ------------------------------------------------------------------------
*/
#ifndef ov_mc_backend_vad_h
#define ov_mc_backend_vad_h

#include <ov_base/ov_config_keys.h>
#include <ov_base/ov_dict.h>
#include <ov_base/ov_event_loop.h>
#include <ov_base/ov_vad_config.h>
#include <ov_core/ov_event_api.h>
#include <ov_core/ov_io.h>
#include <ov_vocs_db/ov_vocs_db.h>

/*----------------------------------------------------------------------------*/

typedef struct ov_mc_backend_vad ov_mc_backend_vad;

/*----------------------------------------------------------------------------*/

typedef struct ov_mc_backend_vad_config {

    ov_event_loop *loop;
    ov_io *io;
    ov_vocs_db *db;

    ov_socket_configuration socket;

    ov_vad_config vad;

    struct {

        void *userdata;

        void (*vad)(void *userdata, const char *loop, bool on);

    } callbacks;

} ov_mc_backend_vad_config;

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_mc_backend_vad *ov_mc_backend_vad_create(ov_mc_backend_vad_config config);
ov_mc_backend_vad *ov_mc_backend_vad_free(ov_mc_backend_vad *self);
ov_mc_backend_vad *ov_mc_backend_vad_cast(const void *data);

ov_mc_backend_vad_config
ov_mc_backend_vad_config_from_json(const ov_json_value *in);

#endif /* ov_mc_backend_vad_h */
