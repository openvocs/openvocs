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
        @file           ov_event_cc.h
        @author         Töpfer, Markus

        @date           2026-08-24


        ------------------------------------------------------------------------
*/
#ifndef ov_event_cc_h
#define ov_event_cc_h

#include <ov_base/ov_event_loop.h>
#include "ov_io.h"

/*----------------------------------------------------------------------------*/

typedef struct ov_event_cc ov_event_cc;

/*----------------------------------------------------------------------------*/

typedef struct ov_event_cc_config {

    ov_event_loop *loop;
    ov_io *io;

    char name[PATH_MAX];

} ov_event_cc_config;


/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_event_cc *ov_event_cc_create(ov_event_cc_config config);
ov_event_cc *ov_event_cc_free(ov_event_cc *self);
ov_event_cc *ov_event_cc_cast(const void *data);

/*----------------------------------------------------------------------------*/

bool ov_event_cc_connect(ov_event_cc *self, ov_socket_configuration socket);

/*----------------------------------------------------------------------------*/

bool ov_event_cc_log(ov_event_cc *self, int socket, const ov_json_value *msg);

#endif /* ov_event_cc_h */
