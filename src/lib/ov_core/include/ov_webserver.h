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
        @file           ov_webserver.h
        @author         Töpfer, Markus

        @date           2026-01-31


        ------------------------------------------------------------------------
*/
#ifndef ov_webserver_h
#define ov_webserver_h

#include <ov_base/ov_event_loop.h>
#include <ov_base/ov_json.h>
#include <ov_base/ov_socket.h>

#include "ov_http_pointer.h"
#include "ov_websocket_pointer.h"
#include "ov_io.h"

/*----------------------------------------------------------------------------*/

#define OV_WEBSERVER_MAGIC_BYTES 0xfe89

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

    struct {

        void *userdata;
        void (*close)(void *userdata, int socket);

    } callbacks;

} ov_webserver_config;

/*----------------------------------------------------------------------------*/

struct ov_webserver {

    uint16_t magic_bytes;
    uint8_t type;

    ov_webserver_config config;

    ov_webserver *(*free) (ov_webserver *self);

    bool (*debug) (ov_webserver *self, bool on);

    bool (*enable_domains)(ov_webserver *self, const ov_json_value* config);

    bool (*enable_events)(ov_webserver *self, 
                    const char *domain,
                    const char *uri,
                    void *userdata,
                    void (*callback)(void *userdata, 
                                     int socket, 
                                     ov_json_value *msg));

    bool (*close) (ov_webserver *self, int socket);

    struct {

        bool (*json) (ov_webserver *self, 
                      int socket, 
                      const ov_json_value *msg);

        bool (*http) (ov_webserver *self, 
                      int socket, 
                      const ov_http_message *msg);
        
        bool (*plain) (ov_webserver *self, 
                       int socket, 
                       const char *buffer, 
                       size_t size);

    } send;

};

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_webserver *ov_webserver_create(ov_webserver_config config);
ov_webserver *ov_webserver_free(ov_webserver *self);
ov_webserver *ov_webserver_cast(const void *data);

/*
 *      ------------------------------------------------------------------------
 *
 *      CONFIGURATION FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_webserver_config ov_webserver_config_from_json(
        const ov_json_value *config);

/*----------------------------------------------------------------------------*/

bool ov_webserver_set_debug(ov_webserver *self, bool on);

/*----------------------------------------------------------------------------*/

bool ov_webserver_enable_domains(
        ov_webserver *self,
        const ov_json_value *config);

/*----------------------------------------------------------------------------*/

bool ov_webserver_enable_event_callback(
        ov_webserver *self,
        const char *domain, 
        const char *uri,
        void *userdata,
        void (*callback)(void *userdata, int socket, ov_json_value *msg));

/*----------------------------------------------------------------------------*/

bool ov_webserver_close(ov_webserver *self, int socket);

/*----------------------------------------------------------------------------*/

bool ov_webserver_register_close(ov_webserver *self, void *userdata,
    void (*callback)(void *userdata, int socket));

/*
 *      ------------------------------------------------------------------------
 *
 *      SEND FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

bool ov_webserver_send_json(
        ov_webserver *self, int socket, const ov_json_value *msg);

/*----------------------------------------------------------------------------*/

bool ov_webserver_send_http(
        ov_webserver *self, int socket, const ov_http_message *msg);

/*----------------------------------------------------------------------------*/

bool ov_webserver_send(
        ov_webserver *self, int socket, const char *buffer, size_t size);

#endif /* ov_webserver_h */
