/***
        ------------------------------------------------------------------------

        Copyright (c) 2024 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_ice_base.h
        @author         Markus Töpfer

        @date           2024-09-18


        ------------------------------------------------------------------------
*/
#ifndef ov_ice_base_h
#define ov_ice_base_h

typedef struct ov_ice_base ov_ice_base;

#include "ov_ice_stream.h"
#include "ov_ice_candidate.h"

#include <ov_base/ov_id.h>
#include <ov_base/ov_node.h>
#include <ov_base/ov_socket.h>

struct ov_ice_base {

    ov_node node;
    ov_id uuid;

    ov_ice_stream *stream;

    int socket;

    struct {

        ov_socket_data data;

    } local;

    ov_list *candidates;
};

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_ice_base *ov_ice_base_create(ov_ice_stream *stream, int socket);
ov_ice_base *ov_ice_base_cast(const void *data);
void *ov_ice_base_free(void *self);

bool ov_ice_base_send_stun_binding_request(ov_ice_base *base,
                                           ov_ice_candidate *candidate);
bool ov_ice_base_send_turn_allocate_request(ov_ice_base *base,
                                            ov_ice_candidate *candidate);

#endif /* ov_ice_base_h */
