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
        @file           ov_web_server.h
        @author         Töpfer, Markus

        @date           2025-11-06

        Standard basic webserver for openvocs


        ------------------------------------------------------------------------
*/
#ifndef ov_web_server_h
#define ov_web_server_h

#include <ov_base/ov_event_loop.h>

#include <ov_core/ov_http_pointer.h>
#include <ov_core/ov_websocket_pointer.h>
#include <ov_core/ov_websocket_message.h>

/*----------------------------------------------------------------------------*/

typedef struct ov_web_server ov_web_server;

/*----------------------------------------------------------------------------*/

typedef struct ov_web_server_config {

    ov_event_loop *loop;

    bool debug;

    ov_socket_configuration socket;   // default 443 HTTPS

    char name[PATH_MAX];
    char domain_config_path[PATH_MAX];

    struct {

        uint32_t max_content_bytes_per_websocket_frame;

    } limits;

    ov_http_message_config http_message;
    ov_websocket_frame_config websocket_frame;


} ov_web_server_config;

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_web_server *ov_web_server_create(ov_web_server_config config);
ov_web_server *ov_web_server_free(ov_web_server *self);
ov_web_server *ov_web_server_cast(const void *data);

ov_web_server_config ov_web_server_get_config(ov_web_server *self);


#endif /* ov_web_server_h */
