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
        @file           ov_client_registry.h
        @author         Töpfer, Markus

        @date           2026-04-21


        ------------------------------------------------------------------------
*/
#ifndef ov_client_registry_h
#define ov_client_registry_h

#include <ov_base/ov_json.h>

typedef struct ov_client_registry ov_client_registry;

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_client_registry *ov_client_registry_create();
ov_client_registry *ov_client_registry_free(ov_client_registry *self);

/*----------------------------------------------------------------------------*/

bool ov_client_registry_register(ov_client_registry *self, 
    const char *uuid, int socket);

/*----------------------------------------------------------------------------*/

bool ov_client_registry_unregister(ov_client_registry *self, int socket);

/*----------------------------------------------------------------------------*/

int ov_client_registry_get_socket(ov_client_registry *self, const char *uuid);

/*----------------------------------------------------------------------------*/

bool ov_client_registry_set_data(ov_client_registry *self, const char *uuid, 
    ov_json_value *data);

/*----------------------------------------------------------------------------*/

ov_json_value *ov_client_registry_get_data(ov_client_registry *self, 
    const char *uuid);




#endif /* ov_client_registry_h */
