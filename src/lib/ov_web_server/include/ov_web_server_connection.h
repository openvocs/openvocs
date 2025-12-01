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
        @file           ov_web_server_connection.h
        @author         Töpfer, Markus

        @date           2025-11-06


        ------------------------------------------------------------------------
*/
#ifndef ov_web_server_connection_h
#define ov_web_server_connection_h

#include <openssl/ssl.h>

/*----------------------------------------------------------------------------*/

typedef struct ov_web_server_connection ov_web_server_connection;

/*----------------------------------------------------------------------------*/

#include "ov_web_server.h"

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_web_server_connection *ov_web_server_connection_create(ov_web_server *srv,
                                                          int socket);
ov_web_server_connection *ov_web_server_connection_cast(const void *self);
void *ov_web_server_connection_free(void *self);

bool ov_web_server_connection_set_ssl(ov_web_server_connection *self, SSL *ssl);

bool ov_web_server_connection_send(ov_web_server_connection *self,
                                   const ov_buffer *data);

bool ov_web_server_connection_uri_file_path(ov_web_server_connection *self,
                                            const ov_http_message *request,
                                            size_t length, char *path);

ov_domain *ov_web_server_connection_get_domain(ov_web_server_connection *self);

#endif /* ov_web_server_connection_h */
