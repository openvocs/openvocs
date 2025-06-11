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
        @file           ov_webserver_mt.h
        @author         Töpfer, Markus

        @date           2025-06-02


        ------------------------------------------------------------------------
*/
#ifndef ov_webserver_mt_h
#define ov_webserver_mt_h

#include <ov_base/ov_event_loop.h>
#include <ov_base/ov_socket.h>

#include <ov_core/ov_io.h>
#include <ov_core/ov_http_pointer.h>
#include <ov_core/ov_websocket_pointer.h>
#include <ov_core/ov_event_io.h>

typedef struct ov_webserver_mt ov_webserver_mt;

typedef struct ov_webserver_mt_config {

    ov_event_loop *loop;
    ov_io *io;

    char name[PATH_MAX];

    ov_http_message_config http_config;
    ov_websocket_frame_config frame_config;

    ov_socket_configuration socket;

    struct {

        char path[PATH_MAX];
        char ext[PATH_MAX];

    } mime;

    struct {

        int sockets;
        int threads;

        uint64_t thread_lock_timeout_usecs;
        uint64_t message_queue_capacity;

    } limits;

} ov_webserver_mt_config;

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_webserver_mt *ov_webserver_mt_create(ov_webserver_mt_config config);
ov_webserver_mt *ov_webserver_mt_free(ov_webserver_mt *self);
ov_webserver_mt *ov_webserver_mt_cast(const void *self);

/*----------------------------------------------------------------------------*/

ov_webserver_mt_config ov_webserver_mt_config_from_json(const ov_json_value *in);

/*----------------------------------------------------------------------------*/

bool ov_webserver_mt_close(ov_webserver_mt *self, int socket);

/*----------------------------------------------------------------------------*/

bool ov_webserver_mt_configure_uri_event_io(
    ov_webserver_mt *self,
    const char *hostname,
    const ov_event_io_config handler);

/*----------------------------------------------------------------------------*/

bool ov_webserver_mt_send_json(ov_webserver_mt *self,
                                    int socket,
                                    ov_json_value const *const data);
#endif /* ov_webserver_mt_h */
