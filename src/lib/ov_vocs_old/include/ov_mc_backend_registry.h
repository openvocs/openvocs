/***
        ------------------------------------------------------------------------

        Copyright (c) 2023 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_mc_backend_registry.h
        @author         Markus Töpfer

        @date           2023-01-21


        ------------------------------------------------------------------------
*/
#ifndef ov_mc_backend_registry_h
#define ov_mc_backend_registry_h

#include <stdbool.h>

#include "ov_mc_mixer_data.h"

/*----------------------------------------------------------------------------*/

typedef struct ov_mc_backend_registry ov_mc_backend_registry;

/*----------------------------------------------------------------------------*/

typedef struct ov_mc_backend_registry_config {

    bool debug;

} ov_mc_backend_registry_config;

/*----------------------------------------------------------------------------*/

typedef struct ov_mc_backend_registry_count {

    size_t mixers;
    size_t used;

} ov_mc_backend_registry_count;

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_mc_backend_registry *ov_mc_backend_registry_create(
    ov_mc_backend_registry_config config);
ov_mc_backend_registry *ov_mc_backend_registry_free(
    ov_mc_backend_registry *self);
ov_mc_backend_registry *ov_mc_backend_registry_cast(const void *self);

/*----------------------------------------------------------------------------*/

bool ov_mc_backend_registry_register_mixer(ov_mc_backend_registry *self,
                                           ov_mc_mixer_data data);

/*----------------------------------------------------------------------------*/

bool ov_mc_backend_registry_unregister_mixer(ov_mc_backend_registry *self,
                                             int socket);

/*----------------------------------------------------------------------------*/

ov_mc_mixer_data ov_mc_backend_registry_acquire_user(
    ov_mc_backend_registry *self, const char *uuid);
/*----------------------------------------------------------------------------*/

ov_mc_mixer_data ov_mc_backend_registry_get_user(ov_mc_backend_registry *self,
                                                 const char *uuid);

/*----------------------------------------------------------------------------*/

ov_mc_mixer_data ov_mc_backend_registry_get_socket(ov_mc_backend_registry *self,
                                                   int socket);

/*----------------------------------------------------------------------------*/

bool ov_mc_backend_registry_release_user(ov_mc_backend_registry *self,
                                         const char *uuid);

/*----------------------------------------------------------------------------*/

ov_mc_backend_registry_count ov_mc_backend_registry_count_mixers(
    ov_mc_backend_registry *self);

#endif /* ov_mc_backend_registry_h */
