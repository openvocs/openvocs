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
        @file           ov_turn_permission.h
        @author         Markus Töpfer

        @date           2022-01-30

        ov_turn_permission is a list of permissions enabled at some connection.

        ------------------------------------------------------------------------
*/
#ifndef ov_turn_permission_h
#define ov_turn_permission_h

#include <inttypes.h>

#include <ov_base/ov_node.h>
#include <ov_base/ov_socket.h>

/*----------------------------------------------------------------------------*/

typedef struct ov_turn_permission {

    ov_node node;
    uint64_t updated;
    uint64_t lifetime_usec;

    char host[OV_HOST_NAME_MAX];

} ov_turn_permission;

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_turn_permission *ov_turn_permission_create(const char *host,
                                              uint64_t expire_usec);

/*----------------------------------------------------------------------------*/

ov_turn_permission *ov_turn_permission_free(ov_turn_permission *self);

#endif /* ov_turn_permission_h */
