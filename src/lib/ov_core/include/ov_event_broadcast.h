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
        @file           ov_event_broadcast.h
        @author         Markus Töpfer

        @date           2021-01-23

        ov_event_broadcast is some ov_event_handler implementation which
        broadcasts received JSON IO to sockets.

        The implementation is done with a focus on maximum simplicity,
        functionality, resource usage and performance and may be seen
        as some reference implementation for ov_event_handler.

        It uses ALL functionality of ALL interfaces of ov_event_handler,
        in particular the ov_event_send function in combination with some
        socket state storage.

        EXAMPLE USAGE -> see end of header

        ------------------------------------------------------------------------
*/
#ifndef ov_event_broadcast_h
#define ov_event_broadcast_h

#include "ov_event_io.h"

typedef struct ov_event_broadcast ov_event_broadcast;

#define OV_BROADCAST_UNSET 0x00

/* NOTE These names are defined, as they are used within ov_* implementations.
 *
 * 8 different broadcast types may be used,
 * and send may be done in parallel on several types,
 * if the types are OR connected
 *
 * Each bit of the type input will identify a specific custom broadcast type.
 */

#define OV_BROADCAST 0x01
#define OV_USER_BROADCAST 0x02
#define OV_ROLE_BROADCAST 0x04
#define OV_LOOP_BROADCAST 0x08
#define OV_LOOP_SENDER_BROADCAST 0x10
#define OV_PROJECT_BROADCAST 0x20
#define OV_DOMAIN_BROADCAST 0x40
#define OV_SYSTEM_BROADCAST 0x80

#define OV_BROADCAST_KEY_BROADCAST "broadcast"
#define OV_BROADCAST_KEY_USER_BROADCAST "user_broadcast"
#define OV_BROADCAST_KEY_ROLE_BROADCAST "role_broadcast"
#define OV_BROADCAST_KEY_LOOP_BROADCAST "loop_broadcast"
#define OV_BROADCAST_KEY_LOOP_SENDER_BROADCAST "loop_sender_broadcast"
#define OV_BROADCAST_KEY_PROJECT_BROADCAST "project_broadcast"
#define OV_BROADCAST_KEY_DOMAIN_BROADCAST "domain_broadcast"
#define OV_BROADCAST_KEY_SYSTEM_BROADCAST "system_broadcast"

typedef struct ov_event_broadcast_config {

    uint64_t lock_timeout_usec;
    uint32_t max_sockets;

} ov_event_broadcast_config;

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_event_broadcast *ov_event_broadcast_create(ov_event_broadcast_config config);
ov_event_broadcast *ov_event_broadcast_free(ov_event_broadcast *self);

/*----------------------------------------------------------------------------*/

/**
    Create some ov_event_handler_config for some instance

    @param self         instance pointer
    @param name         some name to be set for the URI

    @returns ov_event_handler_config on success
    @returns empty ov_event_handler_config on error
*/
ov_event_io_config
ov_event_broadcast_configure_uri_event_io(ov_event_broadcast *self,
                                          const char *name);

/*----------------------------------------------------------------------------*/

/**
    Add some socket to a connection broadcast.

    @param self         instance pointer
    @param socket       socket to add
    @param type         type byte (or combined types possible)

    @returns true if the socket was added to the broadcast instance
*/
bool ov_event_broadcast_set(ov_event_broadcast *self, int socket, uint8_t type);

/*----------------------------------------------------------------------------*/

/**
    Add some socket to a connection broadcast including a send definition.

    @param self         instance pointer
    @param socket       socket to add
    @param type         type byte (or combined types possible)
    @param send         send definition for socket

    @returns true if the socket was added to the broadcast instance
*/
bool ov_event_broadcast_set_send(ov_event_broadcast *self, int socket,
                                 uint8_t type, ov_event_parameter_send send);

/*----------------------------------------------------------------------------*/

/**
    Get the broadcast setting for some socket

    @param self         instance pointer
    @param socket       socket_id
*/
uint8_t ov_event_broadcast_get(ov_event_broadcast *self, int socket);

/*----------------------------------------------------------------------------*/

/**
    Count broadcasts of type

    NOTE will count OR based type

    @param self         instance pointer
    @param type         type to count

    @returns -1 on error
*/
int64_t ov_event_broadcast_count(ov_event_broadcast *self, uint8_t type);

/*----------------------------------------------------------------------------*/

/**
    NOTE This function is pretty time consuming and should be used with care
    if many sockets are used.
*/
ov_json_value *ov_event_broadcast_state(ov_event_broadcast *self);

/*----------------------------------------------------------------------------*/

bool ov_event_broadcast_is_empty(ov_event_broadcast *self);

/*----------------------------------------------------------------------------*/

/**
    Returns a list of intptr_t sockets with interest in type
*/
ov_list *ov_event_broadcast_get_sockets(ov_event_broadcast *self, uint8_t type);

/*----------------------------------------------------------------------------*/

/**
    Send some broadcast message to all sockets, which are listening to the type.

    8 types are supported in parallel, each type is identified over 1 bit of the
    uint8_t. If the input is using several bits in parallel, several broadcast
    will be send. e.g. a type of 3 will send types 0x01 and 0x02 a type of 0xff
    will send on all possible types.

    NOTE will send OR based type
    NOTE will use previously set send parameter for each socket individually

    @param self         instance pointer
    @param input        input to send
    @param type         type byte (or combined types possible)

    @returns true if the broadcast was sent to all sockets (of broadcast type)
    @returns false if some send failed (fail on first)

    EXAMPLE USAGE:

    ov_event_broadcast_send(self, input, OV_LOOP_BROADCAST);
    ov_event_broadcast_send(self, input, OV_LOOP_BROADCAST|OV_USER_BROADCAST);
*/
bool ov_event_broadcast_send(ov_event_broadcast *self,
                             const ov_json_value *input, uint8_t type);

/*----------------------------------------------------------------------------*/

/**
    Send some broadcast message to all sockets, which are listening to the type.

    8 types are supported in parallel, each type is identified over 1 bit of the
    uint8_t. If the input is using several bits in parallel, several broadcast
    will be send. e.g. a type of 3 will send types 0x01 and 0x02 a type of 0xff
    will send on all possible types.

    NOTE will send OR based type

    @param self         instance pointer
    @param parameter    ov_event_parameter (only send is used)
    @param input        input to send
    @param type         type byte (or combined types possible)

    @returns true if the broadcast was sent to all sockets (of broadcast type)
    @returns false if some send failed (fail on first)

    EXAMPLE USAGE:

    ov_event_broadcast_send(self, input, OV_LOOP_BROADCAST);
    ov_event_broadcast_send(self, input, OV_LOOP_BROADCAST|OV_USER_BROADCAST);
*/
bool ov_event_broadcast_send_params(ov_event_broadcast *self,
                                    const ov_event_parameter *parameter,
                                    const ov_json_value *input, uint8_t type);

/*----------------------------------------------------------------------------*/

/*

    EXAMPLE USAGE

        ov_event_broadcast MAY be used within ANY custom event processing.
        Examples without error cleanup.

        struct custom {

            ov_event_broadcast *broadcast;
            ...
        }

        bool custom_clear(struct *custom){

            custom->broadcast = ov_event_broadcast_free(custom->broadcast)
        }

// (1) example init with ov_webserver_multithreaded for uri "broadcast"
// will serve htts://domain/broadcast and broadcasts over all connections

        bool custom_init(struct *custom,
                         ov_webserver_multithreaded *server,
                         const ov_memory_pointer domain){

            custom->broadcast = ov_event_broadcast_create();
            if (!custom->broadcast)
                goto error;

            if (!ov_webserver_multithreaded_configure_event_handler(
                server,
                domain,
                ov_event_broadcast_config(custom->broadcast, "broadcast"))
                goto error;

            return true;
        error:
            return error;
        }

// (2) example for internal use in custom
// will serve https://domain/custom and may broadcasts based on custom events

        static bool my_custom_process(void *userdata,
                        const int socket,
                        const ov_event_parameter parameter,
                        ov_json_value *input){

            struct custom *custom = (struct custom*) userdata;

            ...

            // some socket to add for broadcast
            ov_event_broadcast_set(custom->broadcast, socket, OV_BROADCAST);

            ...

            // someting to broadcast
            ov_event_broadcast_send(custom->broadcast,
                &parameter, input, OV_BROADCAST);

            ...

            return true;
        }

        bool custom_init(struct *custom,
                         ov_webserver_multithreaded *server,
                         const ov_memory_pointer domain){

            custom->broadcast = ov_event_broadcast_create();
            if (!custom->broadcast)
                goto error;

            if (!ov_webserver_multithreaded_configure_event_handler(
                server,
                domain,
                my_custom_handler)
                goto error;

            return true;

        error:
            return error;
        }
*/

#endif /* ov_event_broadcast_h */
