/***
        ------------------------------------------------------------------------

        Copyright (c) 2022 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_mc_loop.h
        @author         Markus Töpfer

        @date           2022-11-10

        IO interface for some mulicast loop. Will deliver RAW IO received at 
        some Multicast socket. 


        ------------------------------------------------------------------------
*/
#ifndef ov_mc_loop_h
#define ov_mc_loop_h

#include <ov_base/ov_event_loop.h>
#include <ov_core/ov_mc_loop_data.h>

/*----------------------------------------------------------------------------*/

typedef struct ov_mc_loop ov_mc_loop;

/*----------------------------------------------------------------------------*/

typedef struct ov_mc_loop_config {

    ov_event_loop *loop;
    ov_mc_loop_data data;

    struct {

        void *userdata;
        void (*io)(void *userdata, const ov_mc_loop_data *data,
                   const uint8_t *buffer, size_t bytes,
                   const ov_socket_data *remote);

    } callback;

} ov_mc_loop_config;

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_mc_loop *ov_mc_loop_create(ov_mc_loop_config config);
ov_mc_loop *ov_mc_loop_free(ov_mc_loop *self);
ov_mc_loop *ov_mc_loop_cast(const void *self);

void *ov_mc_loop_free_void(void *self);

bool ov_mc_loop_set_volume(ov_mc_loop *self, uint8_t volume);
uint8_t ov_mc_loop_get_volume(ov_mc_loop *self);

ov_json_value *ov_mc_loop_to_json(ov_mc_loop *self);

#endif /* ov_mc_loop_h */
