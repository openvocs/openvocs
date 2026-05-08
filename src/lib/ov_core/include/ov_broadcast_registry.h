/***
        ------------------------------------------------------------------------

        Copyright (c) 2021 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_named_broadcast.h
        @author         Markus Töpfer

        @date           2021-02-24

        This module contains a registry for named broadcasts
        e.g. like used in ov_auth to handle the participation within some loop.

        Example: set client_socket in loop to monitor

        ov_broadcast_registry_set(
            reg, "loop_id", client_socket, OV_LOOP_BROADCAST);

        Example: set client_socket in loop to talk

        ov_broadcast_registry_set(
            reg, "loop_id", client_socket,
                OV_LOOP_BROADCAST & OV_LOOP_SENDER_BROADCAST);

        The registry extends the functionality ov ov_event_broadcast to named
        broadcast domains (in the example the domain of some loop)

        Implementation is threadsafe with lock_timeout_usec as input.

        ------------------------------------------------------------------------
*/
#ifndef ov_named_broadcast_h
#define ov_named_broadcast_h

#include <inttypes.h>
#include <stdbool.h>

#include "ov_event_broadcast.h"
#include "ov_event_io.h"
#include <ov_base/ov_json.h>

typedef struct ov_broadcast_registry ov_broadcast_registry;

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_broadcast_registry *
ov_broadcast_registry_create(ov_event_broadcast_config config);

ov_broadcast_registry *ov_broadcast_registry_free(ov_broadcast_registry *self);

/*----------------------------------------------------------------------------*/

/**
    Set some named broadcast socket.

    @param registriy    instance pointer
    @param name         name of the broadcast
    @param socket       socket id
    @param type         type of broadcast (use OV_NO_BROADCAST to unset)

    NOTE if all entries at name are unset, the name will be deleted within
    the registry.
*/
bool ov_broadcast_registry_set(ov_broadcast_registry *registry,
                               const char *name, int socket, uint8_t type);

/*----------------------------------------------------------------------------*/

/**
    Del some named broadcast socket.

    @param registriy    instance pointer
    @param name         name of the broadcast

*/
bool ov_broadcast_registry_del(ov_broadcast_registry *registry,
                               const char *name);

/*----------------------------------------------------------------------------*/

/**
    Get the current broadcast setting out of the registry for some name
    and socket.

    @param registriy    instance pointer
    @param name         name of the broadcast
    @param socket       socket id
    @param result       result set on success
*/
bool ov_broadcast_registry_get(ov_broadcast_registry *reg, const char *name,
                               int socket, uint8_t *result);

/*----------------------------------------------------------------------------*/

/**
    Count all entries with type.
*/
int64_t ov_broadcast_registry_count(ov_broadcast_registry *reg,
                                    const char *name, uint8_t type);

/*----------------------------------------------------------------------------*/

/**
    Get the current broadcast setting out of the registry for some name
    and socket.

    @param registriy    instance pointer
*/
ov_json_value *ov_broadcast_registry_state(ov_broadcast_registry *reg);

/*----------------------------------------------------------------------------*/

/**
    Get the current broadcast setting out of the registry for some name
    and socket.

    @param registriy    instance pointer
    @param name         name of the broadcast
*/
ov_json_value *ov_broadcast_registry_named_state(ov_broadcast_registry *reg,
                                                 const char *name);

/*----------------------------------------------------------------------------*/

/**
    Unset all broadcasts of socket

    @param registriy    instance pointer
    @param socket       socket id
*/
bool ov_broadcast_registry_unset(ov_broadcast_registry *registry, int socket);

/*----------------------------------------------------------------------------*/

/**
    Send some named broadcast.

    NOTE This will block the whole registry during the send process,
    better use ov_broadcast_registry_get_sockets and send external,
    within some thread.

    @param self         instance pointer
    @param name         name of the broadcast
    @param parameter    ov_event_parameter (only send is used)
    @param input        input to send
    @param type         type byte (or combined types possible)
*/
bool ov_broadcast_registry_send(ov_broadcast_registry *self, const char *name,
                                const ov_event_parameter *parameter,
                                const ov_json_value *input, uint8_t type);

/*----------------------------------------------------------------------------*/

/**
    Get a list of sockets for all types of a named broadcast.

    return NULL if the name is not contained within the broadcast registry,
    otherwise a list (which may be empty for the type requested of intptr_t)
*/
ov_list *ov_broadcast_registry_get_sockets(ov_broadcast_registry *self,
                                           const char *name, uint8_t type);

#endif /* ov_named_broadcast_h */
