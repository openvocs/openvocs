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
        @file           ov_vad_core.h
        @author         Markus Töpfer

        @date           2025-01-10


        ------------------------------------------------------------------------
*/
#ifndef ov_vad_core_h
#define ov_vad_core_h

#include <ov_base/ov_constants.h>
#include <ov_base/ov_event_loop.h>
#include <ov_base/ov_socket.h>
#include <ov_base/ov_vad_config.h>
#include <ov_codec/ov_codec_factory.h>
#include <stdbool.h>

typedef struct ov_vad_core ov_vad_core;

typedef struct ov_vad_core_config {

    ov_event_loop *loop;
    ov_vad_config vad;

    struct {

        uint32_t frames_activate;
        uint32_t frames_deactivate;

        uint32_t threads;
        uint64_t threadlock_timeout_usec;
        uint64_t message_queue_capacity;

    } limits;

    struct {

        void *userdata;

        void (*vad)(void *userdata, const char *loop, bool on);

    } callbacks;

} ov_vad_core_config;

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_vad_core *ov_vad_core_create(ov_vad_core_config config);
ov_vad_core *ov_vad_core_free(ov_vad_core *self);
ov_vad_core *ov_vad_core_cast(const void *data);

ov_vad_core_config ov_vad_core_config_from_json(const ov_json_value *in);

bool ov_vad_core_add_loop(ov_vad_core *self, const char *loop,
                          ov_socket_configuration socket);

bool ov_vad_core_set_vad(ov_vad_core *self, ov_vad_config config);

#endif /* ov_vad_core_h */
