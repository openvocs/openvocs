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

#include <ov_core/ov_domain.h>
#include <ov_core/ov_event_io.h>
#include <ov_core/ov_http_pointer.h>
#include <ov_core/ov_websocket_message.h>
#include <ov_core/ov_websocket_pointer.h>

/*----------------------------------------------------------------------------*/

typedef struct ov_web_server ov_web_server;

/*----------------------------------------------------------------------------*/

typedef struct ov_web_server_config {

    ov_event_loop *loop;

    bool debug;

    ov_socket_configuration socket; // default 443 HTTPS

    char name[PATH_MAX];
    char domain_config_path[PATH_MAX];

    struct {

        char path[PATH_MAX];
        char ext[PATH_MAX];

    } mime;

    struct {

        uint32_t max_content_bytes_per_websocket_frame;

    } limits;

    ov_http_message_config http_message;
    ov_websocket_frame_config websocket_frame;

    struct {

        void *userdata;

        /*
         *  This callback SHOULD be set to process incoming HTTPs messages.
         *
         *  NOTE msg MUST be freed by the callback handler.
         *  NOTE an error return will close the connection socket
         *  NOTE websocket upgrades are already processed within
         * ov_webserver_base NOTE this function will get the buffer as read from
         * the wire and validated as some valid HTTP content with all pointer of
         * the msg pointing to the data read. Nothing is copied, but data is
         * preparsed.
         */
        bool (*https)(void *userdata, int connection_socket,
                      ov_http_message *msg);

        /*
         * Close callback for connection_sockets to
         * cleanup connection based settings in userdata.
         */
        void (*close)(void *userdata, int connection_socket);

    } callback;

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

bool ov_web_server_close_connection(ov_web_server *server, int socket);

/*
 *      ------------------------------------------------------------------------
 *
 *      DOMAIN FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_domain *ov_web_server_get_default_domain(ov_web_server *self);
ov_domain *ov_web_server_find_domain(ov_web_server *self,
                                     const uint8_t *hostname, size_t len);

bool ov_web_server_uri_file_path(ov_web_server *self, int socket,
                                 const ov_http_message *request, size_t length,
                                 char *path);

/*
 *      ------------------------------------------------------------------------
 *
 *      SEND FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

bool ov_web_server_send(ov_web_server *self, int socket, const ov_buffer *data);

bool ov_web_server_send_json(ov_web_server *self, int socket,
                             ov_json_value const *const data);

/*
 *      ------------------------------------------------------------------------
 *
 *      CONFIG FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

bool ov_web_server_configure_websocket_callback(
    ov_web_server *self, const ov_memory_pointer hostname,
    ov_websocket_message_config config);

bool ov_web_server_configure_uri_event_io(
    ov_web_server *self, const ov_memory_pointer hostname,
    const ov_event_io_config config, const ov_websocket_message_config *wss_io);

ov_web_server_config ov_web_server_config_from_json(const ov_json_value *value);

#endif /* ov_web_server_h */
