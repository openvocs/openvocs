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
        @file           ov_event_app.h
        @author         Töpfer, Markus

        @date           2026-08-21


        ------------------------------------------------------------------------
*/
#ifndef ov_event_app_h
#define ov_event_app_h

#include <ov_base/ov_event_loop.h>
#include <ov_base/ov_json.h>
#include <ov_base/ov_socket.h>

#include "ov_io.h"

/*----------------------------------------------------------------------------*/

typedef struct ov_event_app ov_event_app;

/*----------------------------------------------------------------------------*/

typedef struct ov_event_app_config {

    ov_event_loop *loop;
    ov_io *io;

    ov_socket_configuration command_and_control;

    char name[OV_HOST_NAME_MAX];
    char password_path[PATH_MAX];

    struct {

        void *userdata;
        void (*connected)(void *userdata, int socket);
        void (*close)(void *userdata, int socket);

    } callbacks;

} ov_event_app_config;

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_event_app *ov_event_app_create(ov_event_app_config config);
ov_event_app *ov_event_app_free(ov_event_app *app);
ov_event_app *ov_event_app_cast(const void *data);

/*
 *      ------------------------------------------------------------------------
 *
 *      EVENT FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

bool ov_event_app_push(ov_event_app *self, int socket, ov_json_value *msg);

/*----------------------------------------------------------------------------*/

bool ov_event_app_register(ov_event_app *self, 
                              const char *name, 
                              void *userdata,
                              void (*callback)(void *userdata, const char *name,
                                            int socket, const ov_json_value *input));

/*----------------------------------------------------------------------------*/

bool ov_event_app_enable_websocket_events(ov_event_app *self,
                                       const char *domain,
                                       const char *uri);

/*----------------------------------------------------------------------------*/

ov_json_value *ov_event_app_get_functions(const ov_event_app *self);

/*
 *      ------------------------------------------------------------------------
 *
 *      SOCKET FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

bool ov_event_app_close(ov_event_app *self, int socket);

/*----------------------------------------------------------------------------*/

bool ov_event_app_open_cc(ov_event_app *self, 
    ov_io_socket_config config);

/*----------------------------------------------------------------------------*/

int ov_event_app_open_listener(ov_event_app *self, 
    ov_io_socket_config config);

/*----------------------------------------------------------------------------*/

int ov_event_app_open_connection(ov_event_app *self, 
    ov_io_socket_config config);

/*----------------------------------------------------------------------------*/

bool ov_event_app_send(ov_event_app *self, int socket, const ov_json_value *msg);

/*----------------------------------------------------------------------------*/

ov_json_value *ov_event_app_get_socket_data(ov_event_app *self, int socket);

/*----------------------------------------------------------------------------*/

ov_json_value *ov_event_app_get_clients(ov_event_app *self);

#endif /* ov_event_app_h */
