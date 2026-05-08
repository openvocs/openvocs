/***
        ------------------------------------------------------------------------

        Copyright (c) 2019 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_signaling_app.h
        @author         Markus Toepfer

        @date           2019-05-02

        @ingroup        ov_core

        @brief          Definition of an ov_app which is able to
                        register command callbacks.

        Any socket, which SHALL be used as a signaling
        socket MUST be created using:

        app->socket.open(
                app,
                (ov_app_socket_config){
                        ...
                        .socket_config.type = TCP | UDP | DTLS | TLS | LOCAL,
                        ...
                        .parser.config  = (ov_parser_config) { .factory = json },
                        .parser.create  = ov_parser_json_create_default
                        .callback.io    = ov_signaling_app_io_signaling,
                        ...
                },
                ... ,
                ...
        );

        ANY other part of the ov_app_socket_config MAY be custom due to
        implementation needs.

        @NOTE minimum valid input is: {"event":"name"}

        ov_signaling_app_io_signaling expects ov_json_value as input,
        so the parser MUST output ov_json_value.

        An example parser create would be @see ov_parser_json_create_default

        ------------------------------------------------------------------------
*/
#ifndef ov_signaling_app_h
#define ov_signaling_app_h

#include "ov_app.h"
#include <ov_base/ov_config_keys.h>
#include <ov_core/ov_event_api.h>

/**
        Create the signaling APP and deliver an ov_app interface.

        @param config   standard ov_app config, including an
                        ov_event_loop, as well as an ov_json instance.

        @returns created app or NULL
 */
ov_app *ov_signaling_app_create(ov_app_config config);

/*----------------------------------------------------------------------------*/

/**
        The signaling app is using the app->config.userdata
        slot for it's own instantiation, so ANY user provided
        userdata MUST be requested using this function,
        which will return whatever was set in ov_app_config as userdata.

        @param app      instance of the signaling app
 */
void *ov_signaling_app_get_userdata(ov_app *app);

/*----------------------------------------------------------------------------*/

/**
        Set some userdata within the app.
        @param app      instance of the signaling app
        @param userdata data to be set as userdata
 */
bool ov_signaling_app_set_userdata(ov_app *app, void *userdata);

/*----------------------------------------------------------------------------*/

/**
        Callback to be set in ov_app_socket_config during
        app->socket.open(app, app_socket_config, NULL, NULL) to use the
        opened socket as signaling socket.
 */
bool ov_signaling_app_io_signaling(ov_app *app, int socket, const char *uuid,
                                   const ov_socket_data *remote,
                                   void **parsed_io_data);

/*----------------------------------------------------------------------------*/

/**
        Register a new command at the app

        @param app              signaling app
        @param name             event name
        @param description      event description
        @param callback         event callback

        @returns true if the event was set.
*/
bool ov_signaling_app_register_command(
    ov_app *app, const char *name, const char *description,
    ov_json_value *(*callback)(ov_app *app, const char *name,
                               const ov_json_value *value, int socket,
                               const ov_socket_data *remote));

/*----------------------------------------------------------------------------*/

/**
        Get the commands of the app.
*/
const ov_json_value *ov_signaling_app_get_commands(const ov_app *app);

/*----------------------------------------------------------------------------*/

/**
 * Attach a monitor that will receive all signaling traffic flowing to/from
 * this signaling app.
 * Intended mainly for debugging reasons, but feel free to be creative...
 * @return Old monitor if it was set.
 */
bool ov_signaling_app_set_monitor(ov_app *,
                                  void (*monitor)(void *, ov_direction, int,
                                                  const ov_socket_data *,
                                                  const ov_json_value *),
                                  void *userdata);

/*----------------------------------------------------------------------------*/

/**
        Enable {"event":"help"} to get a list of all available commands.
*/
bool ov_signaling_app_enable_help(ov_app *app);

/*----------------------------------------------------------------------------*/

/**
        Enable {"event":"shutdown"}
        @NOTE this event will stop the eventloop.
*/
bool ov_signaling_app_enable_shutdown(ov_app *app);

/*----------------------------------------------------------------------------*/

/**
 * Open a socket. Acts just as `ov_app_connect`, but automatically
 * set the io callback to use signaling instrumentation.
 *
 * TODO: Not sure whether to keep it here - mainly serves as a reminder on
 * how to actually use the signaling app
 */
bool ov_signaling_app_connect(ov_app *self, ov_app_socket_config config);

#endif /* ov_signaling_app_h */
