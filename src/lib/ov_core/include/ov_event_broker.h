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
        @file           ov_event_broker.h
        @author         Töpfer, Markus

        @date           2026-04-07


        ------------------------------------------------------------------------
*/
#ifndef ov_event_broker_h
#define ov_event_broker_h

#include "ov_io.h"

#include <ov_base/ov_event_loop.h>
#include <ov_base/ov_json.h>

#include <limits.h>

/*----------------------------------------------------------------------------*/

typedef struct ov_event_broker ov_event_broker;

/*----------------------------------------------------------------------------*/

typedef struct ov_event_broker_config {

    ov_event_loop *loop;
    ov_io *io;

    char password_path[PATH_MAX];

} ov_event_broker_config;


/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_event_broker *ov_event_broker_create(ov_event_broker_config config);
ov_event_broker *ov_event_broker_free(ov_event_broker *self);
ov_event_broker *ov_event_broker_cast(const void *data);

/*
 *      ------------------------------------------------------------------------
 *
 *      EVENT FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

/*----------------------------------------------------------------------------*/

bool ov_event_broker_push(ov_event_broker *self, int socket, ov_json_value *msg);

/*----------------------------------------------------------------------------*/

bool ov_event_broker_register(ov_event_broker *self, 
                              const char *name, 
                              void *userdata,
                              void (*callback)(void *userdata, const char *name,
                                            int socket, const ov_json_value *input));

/*----------------------------------------------------------------------------*/

bool ov_event_broker_enable_websocket_events(ov_event_broker *self,
                                       const char *domain,
                                       const char *uri);

/*
 *      ------------------------------------------------------------------------
 *
 *      SOCKET FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

int ov_event_broker_open_listener(ov_event_broker *self, 
    ov_io_socket_config config);

#endif /* ov_event_broker_h */
