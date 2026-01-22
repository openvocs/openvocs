/***
        ------------------------------------------------------------------------

        Copyright (c) 2025 German Aerospace Center DLR e.V. (GSOC)

        Licensed under the Apache License, Version 2.0 (the "License");
        you may not use this file except in compliance with the License.
        You may obtain a copy of the License at

                http://www.apache.org/licenses/LICENSE-2.0

        Unless required by applicable law or agreed to in writing, software
        distributed under the License is distributed on an "AS IS" BASIS,
        WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
        See the License for the specific language governing permissions and
        limitations under the License.

        This file is part of the opendtn project. https://opendtn.com

        ------------------------------------------------------------------------
*//**
        @file           ov_webserver.h
        @author         Töpfer, Markus

        @date           2025-12-17

        This is a minimal implementation of a webserver able to 
        process HTTP GET requests and websocket connections of 
        some domains. 

        ------------------------------------------------------------------------
*/
#ifndef ov_webserver_h
#define ov_webserver_h

#include "ov_http_pointer.h"
#include "ov_io.h"
#include "ov_websocket_pointer.h"
#include <ov_base/ov_event_loop.h>
#include <ov_base/ov_json.h>

/*----------------------------------------------------------------------------*/

typedef struct ov_webserver ov_webserver;

/*----------------------------------------------------------------------------*/

typedef struct ov_webserver_config {

    ov_event_loop *loop;
    ov_io *io;

    char name[PATH_MAX];

    ov_socket_configuration socket;

    ov_http_message_config http;
    ov_websocket_frame_config frame;

} ov_webserver_config;

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_webserver *ov_webserver_create(ov_webserver_config config);
ov_webserver *ov_webserver_free(ov_webserver *self);

/*----------------------------------------------------------------------------*/

bool ov_webserver_set_debug(ov_webserver *self, bool on);

/*----------------------------------------------------------------------------*/

bool ov_webserver_enable_callback(ov_webserver *self, const char *domain,
                                   void *userdata,
                                   void (*callback)(void *userdata, int socket,
                                                    ov_json_value *msg));

/*----------------------------------------------------------------------------*/

bool ov_webserver_enable_domains(ov_webserver *self, const ov_json_value *config);

/*----------------------------------------------------------------------------*/

ov_webserver_config ov_webserver_config_from_item(const ov_json_value *config);

/*----------------------------------------------------------------------------*/

bool ov_webserver_send(ov_webserver *self, int socket, const ov_json_value *msg);

/*----------------------------------------------------------------------------*/

ov_event_loop *ov_webserver_get_eventloop(const ov_webserver *self);

/*----------------------------------------------------------------------------*/

bool ov_webserver_register_close(ov_webserver *self, void *userdata,
                                  void(callback)(void *userdata, int socket));

/*----------------------------------------------------------------------------*/

bool ov_webserver_close(ov_webserver *self, int socket);

#endif /* ov_webserver_h */
