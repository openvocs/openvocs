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
        @file           ov_event_app.h
        @author         Markus Töpfer

        @date           2024-12-28

        ------------------------------------------------------------------------
*/
#ifndef ov_event_app_h
#define ov_event_app_h

#include "ov_io.h"

typedef struct ov_event_app ov_event_app;

typedef struct ov_event_app_config {

  ov_io *io;

  struct {

    /* Socket level callbacks for non IO messages (if required) */

    void *userdata;

    bool (*accept)(void *userdata, int listener, int connection);
    void (*close)(void *userdata, int connection);
    void (*connected)(void *userdata, int connection);

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
ov_event_app *ov_event_app_free(ov_event_app *self);
ov_event_app *ov_event_app_cast(const void *data);

/*----------------------------------------------------------------------------*/

int ov_event_app_open_listener(ov_event_app *self, ov_io_socket_config config);
int ov_event_app_open_connection(ov_event_app *self,
                                 ov_io_socket_config config);

/*----------------------------------------------------------------------------*/

bool ov_event_app_close(ov_event_app *self, int socket);

/*----------------------------------------------------------------------------*/

bool ov_event_app_send(ov_event_app *self, int socket,
                       const ov_json_value *msg);

/*
 *      ------------------------------------------------------------------------
 *
 *      EVENT FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

/**
    Standard register for event messages.

    @param app          instance pointer
    @param name         event name
    @param callback     event callback, userdata will be the userdata set in
                        config of ov_event_app.

    NOTE If no event handler is set for some incoming event name, the input
    JSON will be freed in ov_event_app. If some event callback is defined, that
    callback MUST free the input JSON transported.
*/
bool ov_event_app_register(ov_event_app *app, const char *name, void *userdata,
                           void (*callback)(void *userdata, const char *name,
                                            int socket, ov_json_value *input));

/*----------------------------------------------------------------------------*/

/**
    Standard deregister for event messages.

    @param app          instance pointer
    @param name         event name
*/
bool ov_event_app_deregister(ov_event_app *app, const char *name);

/*----------------------------------------------------------------------------*/

bool ov_event_app_push(ov_event_app *self, int socket, ov_json_value *json);

#endif /* ov_event_app_h */
