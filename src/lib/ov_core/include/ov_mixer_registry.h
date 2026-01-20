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
        @file           ov_mixer_registry.h
        @author         Töpfer, Markus

        @date           2026-01-14


        ------------------------------------------------------------------------
*/
#ifndef ov_mixer_registry_h
#define ov_mixer_registry_h

#include <inttypes.h>
#include <ov_base/ov_socket.h>
#include <stdbool.h>

typedef struct ov_mixer_registry ov_mixer_registry;

/*----------------------------------------------------------------------------*/

typedef struct ov_mixer_registry_config {

    struct {

        uint64_t threadlock_timeout_usec;

    } limits;

} ov_mixer_registry_config;

/*----------------------------------------------------------------------------*/

typedef struct ov_mixer_data {

    int socket;
    ov_socket_data remote;
    char user[OV_HOST_NAME_MAX];

} ov_mixer_data;

/*----------------------------------------------------------------------------*/

typedef struct ov_mixer_registry_count {

    size_t mixers;
    size_t used;

} ov_mixer_registry_count;

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_mixer_registry *ov_mixer_registry_create(ov_mixer_registry_config config);
ov_mixer_registry *ov_mixer_registry_free(ov_mixer_registry *self);

/*----------------------------------------------------------------------------*/

bool ov_mixer_registry_register_mixer(ov_mixer_registry *self, int socket,
                                      const ov_socket_data *remote);

/*----------------------------------------------------------------------------*/

bool ov_mixer_registry_unregister_mixer(ov_mixer_registry *self, int socket);

/*----------------------------------------------------------------------------*/

ov_mixer_data ov_mixer_registry_acquire_user(ov_mixer_registry *self,
                                             const char *uuid);

/*----------------------------------------------------------------------------*/

ov_mixer_data ov_mixer_registry_get_user(ov_mixer_registry *self,
                                         const char *uuid);

/*----------------------------------------------------------------------------*/

ov_mixer_data ov_mixer_registry_get_socket(ov_mixer_registry *self, int socket);

/*----------------------------------------------------------------------------*/

bool ov_mixer_registry_release_user(ov_mixer_registry *self, const char *uuid);

/*----------------------------------------------------------------------------*/

ov_mixer_registry_count ov_mixer_registry_count_mixers(ov_mixer_registry *self);

#endif /* ov_mixer_registry_h */
