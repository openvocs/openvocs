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
        @file           ov_io.h
        @author         Markus Töpfer

        @date           2024-12-20


        ------------------------------------------------------------------------
*/
#ifndef ov_io_h
#define ov_io_h

#include "ov_domain.h"

#include <ov_base/ov_event_loop.h>
#include <ov_base/ov_json.h>
#include <ov_base/ov_memory_pointer.h>
#include <ov_base/ov_socket.h>

/*----------------------------------------------------------------------------*/

typedef struct ov_io ov_io;
typedef struct ov_io_callback ov_io_callback;
typedef struct ov_io_ssl_config ov_io_ssl_config;
typedef struct ov_io_socket_config ov_io_socket_config;

/*----------------------------------------------------------------------------*/

typedef struct ov_io_config {

    ov_event_loop *loop;

    struct {

        char path[PATH_MAX];

    } domain;

    struct {

        uint64_t reconnect_interval_usec;
        uint64_t timeout_usec;

    } limits;

} ov_io_config;

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_io *ov_io_create(ov_io_config config);
ov_io *ov_io_free(ov_io *self);
ov_io *ov_io_cast(const void *data);

/*----------------------------------------------------------------------------*/

ov_io_config ov_io_config_from_json(const ov_json_value *input);

/*
 *      ------------------------------------------------------------------------
 *
 *      SOCKET FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

struct ov_io_callback {

    void *userdata;

    // will be called back in listener based setup
    bool (*accept)(void *userdata, int listener, int connection);

    bool (*io)(void *userdata, int connection,
               const char *optional_domain, // only transmitted for TLS
               const ov_memory_pointer data);

    void (*close)(void *userdata, int connection);

    // will be called back in connection based setup
    void (*connected)(void *userdata, int connection);
};

/*----------------------------------------------------------------------------*/

struct ov_io_ssl_config {

    char domain[PATH_MAX]; // hostname to use in handshake

    struct {

        char cert[PATH_MAX];
        char key[PATH_MAX];

    } certificate;

    struct {

        char file[PATH_MAX]; // path to CA verify file
        char path[PATH_MAX]; // path to CAs to use

        char client_ca[PATH_MAX]; // client CA to request for certificate auth

    } ca;

    uint8_t verify_depth;
};

/*----------------------------------------------------------------------------*/

struct ov_io_socket_config {

    bool auto_reconnect; // only used for clients

    ov_socket_configuration socket;

    ov_io_callback callbacks;

    ov_io_ssl_config ssl;
};

/*----------------------------------------------------------------------------*/

/**
 *  open a listener socket.
 *
 *  if config.ssl contains a cert this dedicated cert configuration will be
 *  used. It will build a context with certificate based authentication instead
 *  of the common SNI based Domain authentication configured.
 */
int ov_io_open_listener(ov_io *self, ov_io_socket_config config);

/*----------------------------------------------------------------------------*/

int ov_io_open_connection(ov_io *self, ov_io_socket_config config);

/*----------------------------------------------------------------------------*/

bool ov_io_close(ov_io *self, int socket);

/*----------------------------------------------------------------------------*/

bool ov_io_send(ov_io *self, int socket, const ov_memory_pointer buffer);

/*----------------------------------------------------------------------------*/

ov_domain *ov_io_get_domain(ov_io *self, const char *name);

#endif /* ov_io_h */
