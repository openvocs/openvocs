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
        @file           ov_socket_storage.h
        @author         Töpfer, Markus

        @date           2026-04-08


        ------------------------------------------------------------------------
*/
#ifndef ov_socket_storage_h
#define ov_socket_storage_h

#include <ov_base/ov_json.h>

typedef struct ov_socket_storage ov_socket_storage;

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_socket_storage *ov_socket_storage_create();
ov_socket_storage *ov_socket_storage_free(ov_socket_storage *self);

/*----------------------------------------------------------------------------*/

ov_json_value *ov_socket_storage_get(ov_socket_storage *self, int socket);
bool ov_socket_storage_drop(ov_socket_storage *self, int socket);


#endif /* ov_socket_storage_h */
