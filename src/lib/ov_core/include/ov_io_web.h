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

        This file is part of the openvocs project. https://openvocs.org

        ------------------------------------------------------------------------
*//**
        @file           ov_io_web.h
        @author         Töpfer, Markus

        @date           2025-05-28


        ------------------------------------------------------------------------
*/
#ifndef ov_io_web_h
#define ov_io_web_h

#include <ov_base/ov_event_loop.h>

#include "ov_event_io.h"
#include "ov_http_pointer.h"
#include "ov_io.h"
#include "ov_websocket_pointer.h"

/*----------------------------------------------------------------------------*/

typedef struct ov_io_web ov_io_web;

/*----------------------------------------------------------------------------*/

typedef struct ov_io_web_config {

  ov_event_loop *loop;
  ov_io *io;

  char name[PATH_MAX];

  ov_http_message_config http_config;
  ov_websocket_frame_config frame_config;

  ov_socket_configuration socket;

  struct {

    int sockets;

  } limits;

  struct {

    void *userdata;
    bool (*accept)(void *userdata, int listener, int socket);
    void (*close)(void *userdata, int socket);

    bool (*https)(void *userdata, int socket, ov_http_message *msg);

  } callbacks;

} ov_io_web_config;

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_io_web *ov_io_web_create(ov_io_web_config config);
ov_io_web *ov_io_web_free(ov_io_web *self);
ov_io_web *ov_io_web_cast(const void *self);

/*----------------------------------------------------------------------------*/

ov_io_web_config ov_io_web_config_from_json(const ov_json_value *input);

/*
 *      ------------------------------------------------------------------------
 *
 *      SOCKET FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

bool ov_io_web_close(ov_io_web *self, int socket);

/*----------------------------------------------------------------------------*/

bool ov_io_web_send(ov_io_web *self, int socket, const ov_http_message *msg);

/*----------------------------------------------------------------------------*/

bool ov_io_web_send_json(ov_io_web *self, int socket, const ov_json_value *msg);

/*
 *      ------------------------------------------------------------------------
 *
 *      CONFIG FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

bool ov_io_web_configure_websocket_callback(ov_io_web *self,
                                            const char *hostname,
                                            ov_websocket_message_config config);

/*----------------------------------------------------------------------------*/

bool ov_io_web_configure_uri_event_io(ov_io_web *self, const char *hostname,
                                      const ov_event_io_config config);

/*
 *      ------------------------------------------------------------------------
 *
 *      DOMAIN FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

bool ov_io_web_uri_file_path(ov_io_web *self, int socket,
                             const ov_http_message *request, size_t length,
                             char *path);

#endif /* ov_io_web_h */
