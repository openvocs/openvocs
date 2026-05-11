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
        @file           ov_event_mt.h
        @author         Töpfer, Markus

        @date           2026-05-07


        ------------------------------------------------------------------------
*/
#ifndef ov_event_mt_h
#define ov_event_mt_h

#include <ov_base/ov_event_loop.h>
#include "ov_io.h"

typedef struct ov_event_mt ov_event_mt;

typedef struct ov_event_mt_config {

    ov_event_loop *loop;
    ov_io *io;

    struct {

        uint64_t threadlock_timeout_usec;
        uint64_t message_queue_capacity;
        uint64_t threads;
    
    } limits;

} ov_event_mt_config;

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_event_mt *ov_event_mt_create(ov_event_mt_config config);
ov_event_mt *ov_event_mt_free(ov_event_mt *self);
ov_event_mt *ov_event_mt_cast(const void *data);

bool ov_event_mt_debug(ov_event_mt *self, bool on);

/*
 *      ------------------------------------------------------------------------
 *
 *      EVENT FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

bool ov_event_mt_push(ov_event_mt *self, int socket, ov_json_value *msg);

/*----------------------------------------------------------------------------*/

bool ov_event_mt_register(ov_event_mt *self, 
                          const char *name,
                          void *userdata,
                          void (*callback)(void *userdata,
                            int socket, const ov_json_value *input));

/*----------------------------------------------------------------------------*/

bool ov_event_mt_enable_websocket_events(ov_event_mt *self,
                                       const char *domain,
                                       const char *uri);

/*
 *      ------------------------------------------------------------------------
 *
 *      SOCKET FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

int ov_event_mt_open_listener(ov_event_mt *self, 
    ov_io_socket_config config);

int ov_event_mt_open_connection(ov_event_mt *self, 
    ov_io_socket_config config);

#endif /* ov_event_mt_h */
