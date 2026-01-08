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
        @file           ov_mc_backend_sip_registry.h
        @author         Markus Töpfer

        @date           2023-03-31


        ------------------------------------------------------------------------
*/
#ifndef ov_mc_backend_sip_registry_h
#define ov_mc_backend_sip_registry_h

#include <stdbool.h>

/*----------------------------------------------------------------------------*/

typedef struct ov_mc_backend_sip_registry ov_mc_backend_sip_registry;

/*----------------------------------------------------------------------------*/

typedef struct ov_mc_backend_sip_registry_config {

    bool debug;

} ov_mc_backend_sip_registry_config;

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_mc_backend_sip_registry *
ov_mc_backend_sip_registry_create(ov_mc_backend_sip_registry_config config);
ov_mc_backend_sip_registry *
ov_mc_backend_sip_registry_free(ov_mc_backend_sip_registry *self);
ov_mc_backend_sip_registry *ov_mc_backend_sip_registry_cast(const void *self);

/*----------------------------------------------------------------------------*/

bool ov_mc_backend_sip_registry_register_proxy(ov_mc_backend_sip_registry *self,
                                               int socket, const char *uuid);

/*----------------------------------------------------------------------------*/

bool ov_mc_backend_sip_registry_unregister_proxy(
    ov_mc_backend_sip_registry *self, int socket);

/*----------------------------------------------------------------------------*/

int ov_mc_backend_sip_registry_get_proxy_socket(
    ov_mc_backend_sip_registry *self);

/*----------------------------------------------------------------------------*/

bool ov_mc_backend_sip_registry_register_call(ov_mc_backend_sip_registry *self,
                                              int socket, const char *call_id,
                                              const char *loop_id,
                                              const char *peer_id);

/*----------------------------------------------------------------------------*/

bool ov_mc_backend_sip_registry_unregister_call(
    ov_mc_backend_sip_registry *self, const char *call_id);

/*----------------------------------------------------------------------------*/

int ov_mc_backend_sip_registry_get_call_proxy(ov_mc_backend_sip_registry *self,
                                              const char *call_id);

/*----------------------------------------------------------------------------*/

const char *
ov_mc_backend_sip_registry_get_call_loop(ov_mc_backend_sip_registry *self,
                                         const char *call_id);

#endif /* ov_mc_backend_sip_registry_h */
