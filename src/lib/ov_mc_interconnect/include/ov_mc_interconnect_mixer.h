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
        @file           ov_mc_interconnect_mixer.h
        @author         Töpfer, Markus

        @date           2026-01-13


        ------------------------------------------------------------------------
*/
#ifndef ov_mc_interconnect_mixer_h
#define ov_mc_interconnect_mixer_h

#include <ov_base/ov_event_loop.h>
#include <ov_base/ov_socket.h>
#include <ov_core/ov_mixer_registry.h>
#include <ov_core/ov_mixer_msg.h>

typedef struct ov_mc_interconnect_mixer ov_mc_interconnect_mixer;

typedef struct ov_mc_interconnect_mixer_config {

    ov_event_loop *loop;
    ov_socket_configuration socket;

    ov_mixer_config mixer;

    struct {

        void *userdata;

        void (*mixer_available)(void *userdata, int socket);
        void (*mixer_dropped)(void *userdata, int socket);

        void (*aquire)(void *userdata, int socket, bool success);
        void (*forward)(void *userdata, int socket, bool success);
        void (*join)(void *userdata, int socket, bool success);

    } callbacks;

} ov_mc_interconnect_mixer_config;

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_mc_interconnect_mixer *ov_mc_interconnect_mixer_create(
    ov_mc_interconnect_mixer_config config);

ov_mc_interconnect_mixer *ov_mc_interconnect_mixer_free(
    ov_mc_interconnect_mixer *self);

ov_mixer_data ov_mc_interconnect_mixer_assign_mixer(
    ov_mc_interconnect_mixer *self,
    const char *name);

bool ov_mc_interconnect_mixer_send_aquire(
    ov_mc_interconnect_mixer *self,
    ov_mixer_data data,
    ov_mixer_forward forward);

bool ov_mc_interconnect_mixer_send_forward(
    ov_mc_interconnect_mixer *self,
    int socket,
    const char *name, 
    ov_mixer_forward forward);

bool ov_mc_interconnect_mixer_send_switch(
    ov_mc_interconnect_mixer *self,
    int socket,
    ov_mc_loop_data data);


#endif /* ov_mc_interconnect_mixer_h */
