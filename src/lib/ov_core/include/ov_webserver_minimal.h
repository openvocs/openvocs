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
        @file           ov_webserver_minimal.h
        @author         Markus Töpfer

        @date           2021-12-15

        Implementation of some minimal webserver for openvocs,
        supporting HTTPS GET and configurable URI based websocket messaging.

        Features:

                1. HTTPS / websocket IO multiplexing is done (ov_webserver_base)
                2. Server Name Indication (SNI) is done (ov_webserver_base)
                3. URI callbacks for independent domain name
                   for JSON IO @see ov_webserver_minimal_configure_channel
                4. HTTPS GET is implemented (ONLY GET no other method!)


        USAGE:

                1. HTTPs based on config

                2. URI based JSON IO

                ov_webserver_minimal_configure_channel(

                    server,

                    (ov_memory_pointer){
                            .start = "my_domain",
                            .length = strlen("my_domain")
                    },

                    (ov_event_channel_handler){
                        .name = "my_uri",
                        .userdata = my_application,
                        .close = my_applicaction_close_notify,
                        .process = my_application_process
                    });


        ------------------------------------------------------------------------
*/
#ifndef ov_webserver_minimal_h
#define ov_webserver_minimal_h

#include "ov_http_pointer.h"
#include "ov_webserver_base.h"
#include "ov_websocket_message.h"
#include "ov_websocket_pointer.h"

#include <ov_base/ov_memory_pointer.h>

typedef struct ov_webserver_minimal ov_webserver_minimal;

typedef struct ov_webserver_minimal_config {

    ov_webserver_base_config base;

    struct {

        char path[PATH_MAX];
        char ext[PATH_MAX];

    } mime;

    struct {

        void *userdata;

        bool (*accept)(void *userdata, int server_socket, int accepted_socket);
        void (*close)(void *userdata, int connection_socket);

    } callback;

} ov_webserver_minimal_config;

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_webserver_minimal *ov_webserver_minimal_create(
    ov_webserver_minimal_config config);

ov_webserver_minimal *ov_webserver_minimal_cast(const void *self);

ov_webserver_minimal *ov_webserver_minimal_free(ov_webserver_minimal *self);

/*----------------------------------------------------------------------------*/

bool ov_webserver_minimal_close(ov_webserver_minimal *self, int socket);

/*----------------------------------------------------------------------------*/

/**
 *  Configure some JSON IO handler for some hostname
 *
 *  This function will enable callbacks at uri hostname/handler.name
 *
 *  Example     hostname = openvocs.org
 *              handler.name = chat
 *
 *  -> will callback all JSON IO arriving at https://openvocs.org/chat
 *
 *  @param self     instance pointer
 *  @param hostname hostname to use
 *  @param handler  handler to set
 *
 *  NOTE this is some serial implementation (not parallel, not threaded)
 */
bool ov_webserver_minimal_configure_uri_event_io(
    ov_webserver_minimal *self,
    const ov_memory_pointer hostname,
    const ov_event_io_config handler);

/*
 *      ------------------------------------------------------------------------
 *
 *      CONFIG FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_webserver_minimal_config ov_webserver_minimal_config_from_json(
    const ov_json_value *value);

/*----------------------------------------------------------------------------*/

/**
    Send some JSON over WSS sockets.

    @param self     instance
    @param socket   connection socket
    @param data     json message to send
*/
bool ov_webserver_minimal_send_json(ov_webserver_minimal *self,
                                    int socket,
                                    ov_json_value const *const data);
#endif /* ov_webserver_minimal_h */
