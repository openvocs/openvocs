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
        @file           ov_vad_app.h
        @author         Markus Töpfer

        @date           2025-01-10


        ------------------------------------------------------------------------
*/
#ifndef ov_vad_app_h
#define ov_vad_app_h

#include "ov_vad_core.h"
#include <ov_base/ov_socket.h>
#include <ov_core/ov_io.h>

typedef struct ov_vad_app ov_vad_app;

typedef struct ov_vad_app_config {

    ov_event_loop *loop;
    ov_io *io;

    ov_socket_configuration manager;

    ov_vad_core_config core;

} ov_vad_app_config;

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_vad_app *ov_vad_app_create(ov_vad_app_config config);
ov_vad_app *ov_vad_app_free(ov_vad_app *self);
ov_vad_app *ov_vad_app_cast(const void *data);

ov_vad_app_config ov_vad_app_config_from_json(const ov_json_value *v);

#endif /* ov_vad_app_h */
