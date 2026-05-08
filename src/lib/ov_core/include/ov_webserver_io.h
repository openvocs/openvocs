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
        @file           ov_webserver_io.h
        @author         Töpfer, Markus

        @date           2026-01-31


        ------------------------------------------------------------------------
*/
#ifndef ov_webserver_io_h
#define ov_webserver_io_h

#include "ov_http_pointer.h"
#include "ov_io.h"
#include "ov_websocket_pointer.h"
#include <ov_base/ov_event_loop.h>
#include <ov_base/ov_json.h>

/*----------------------------------------------------------------------------*/

typedef struct ov_webserver_io ov_webserver_io;

/*----------------------------------------------------------------------------*/

typedef struct ov_webserver_io_config {

    ov_event_loop *loop;
    ov_io *io;

    char name[PATH_MAX];

    ov_socket_configuration socket;

    ov_http_message_config http;
    ov_websocket_frame_config frame;

    struct {

        void *userdata;
        void (*close)(void *userdata, int socket);
        bool (*io)(void *userdata, int socket, const ov_http_message *msg);

    } callbacks;

} ov_webserver_io_config;

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_webserver_io *ov_webserver_io_create(ov_webserver_io_config config);
ov_webserver_io *ov_webserver_io_free(ov_webserver_io *self);

/*
 *      ------------------------------------------------------------------------
 *
 *      CONFIGURATION FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

bool ov_webserver_io_set_debug(ov_webserver_io *self, bool on);

/*----------------------------------------------------------------------------*/

ov_webserver_io_config
ov_webserver_io_config_from_json(const ov_json_value *config);

/*----------------------------------------------------------------------------*/

bool ov_webserver_io_enable_domains(ov_webserver_io *self,
                                    const ov_json_value *config);

/*----------------------------------------------------------------------------*/

bool ov_webserver_io_event_callback(ov_webserver_io *self, const char *domain,
                                    const char *uri, void *userdata,
                                    void (*callback)(void *userdata, int socket,
                                                     ov_json_value *msg));

/*----------------------------------------------------------------------------*/

bool ov_webserver_io_close(ov_webserver_io *self, int socket);

/*----------------------------------------------------------------------------*/

bool ov_webserver_io_clean_path(ov_webserver_io *self, int socket,
                                const ov_http_message *msg, char *path_out,
                                size_t path_out_len);

/*
 *      ------------------------------------------------------------------------
 *
 *      SEND FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

bool ov_webserver_io_send_json(ov_webserver_io *self, int socket,
                               const ov_json_value *msg);

/*----------------------------------------------------------------------------*/

bool ov_webserver_io_send_http(ov_webserver_io *self, int socket,
                               const ov_http_message *msg);

/*----------------------------------------------------------------------------*/

bool ov_webserver_io_send(ov_webserver_io *self, int socket, const char *buffer,
                          size_t size);

/*
 *      ------------------------------------------------------------------------
 *
 *      GETTER FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_event_loop *ov_webserver_io_get_eventloop(const ov_webserver_io *self);

#endif /* ov_webserver_io_h */
