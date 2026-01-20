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
        @file           ov_interconnect_loop.h
        @author         Töpfer, Markus

        @date           2026-01-15


        ------------------------------------------------------------------------
*/
#ifndef ov_interconnect_loop_h
#define ov_interconnect_loop_h

typedef struct ov_interconnect_loop ov_interconnect_loop;

/*----------------------------------------------------------------------------*/

#include "ov_interconnect.h"

/*----------------------------------------------------------------------------*/

typedef struct ov_interconnect_loop_config {

    ov_event_loop *loop;
    ov_interconnect *base;
    char name[OV_HOST_NAME_MAX];
    ov_socket_configuration multicast;
    ov_socket_configuration internal;

} ov_interconnect_loop_config;

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_interconnect_loop *
ov_interconnect_loop_create(ov_interconnect_loop_config config);

void *ov_interconnect_loop_free(void *self);

uint32_t ov_interconnect_loop_get_ssrc(const ov_interconnect_loop *self);
const char *ov_interconnect_loop_get_name(const ov_interconnect_loop *self);

bool ov_interconnect_loop_has_mixer(const ov_interconnect_loop *self);

bool ov_interconnect_loop_assign_mixer(ov_interconnect_loop *self);

int ov_interconnect_loop_get_mixer(const ov_interconnect_loop *self);

ov_mixer_forward
ov_interconnect_loop_get_forward(const ov_interconnect_loop *self);

ov_mc_loop_data
ov_interconnect_loop_get_loop_data(const ov_interconnect_loop *self);

bool ov_interconnect_loop_send(const ov_interconnect_loop *self,
                               const uint8_t *buffer, size_t size);

#endif /* ov_interconnect_loop_h */
