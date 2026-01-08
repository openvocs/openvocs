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
        @file           ov_vocs_loop.h
        @author         Markus Töpfer

        @date           2022-11-08


        ------------------------------------------------------------------------
*/
#ifndef ov_vocs_loop_h
#define ov_vocs_loop_h

#include <inttypes.h>
#include <stdbool.h>

#include <ov_base/ov_json.h>

typedef struct ov_vocs_loop ov_vocs_loop;

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_vocs_loop *ov_vocs_loop_create(const char *name);
ov_vocs_loop *ov_vocs_loop_free(ov_vocs_loop *self);
ov_vocs_loop *ov_vocs_loop_cast(const void *self);

void *ov_vocs_loop_free_void(void *self);

/*----------------------------------------------------------------------------*/

int64_t ov_vocs_loop_get_participants_count(ov_vocs_loop *self);

ov_json_value *ov_vocs_loop_get_participants(ov_vocs_loop *self);

/*----------------------------------------------------------------------------*/

bool ov_vocs_loop_add_participant(ov_vocs_loop *self, int socket,
                                  const char *client, const char *user,
                                  const char *role);

/*----------------------------------------------------------------------------*/

bool ov_vocs_loop_drop_participant(ov_vocs_loop *self, int socket);

#endif /* ov_vocs_loop_h */
