/***
        ------------------------------------------------------------------------

        Copyright (c) 2023 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_event_socket.h
        @author         Markus Töpfer

        @date           2023-10-08


        ------------------------------------------------------------------------
*/
#ifndef ov_event_socket_h
#define ov_event_socket_h

#include <ov_base/ov_event_loop.h>
#include <ov_base/ov_socket.h>

#include "ov_event_engine.h"

/*----------------------------------------------------------------------------*/

typedef struct ov_event_socket ov_event_socket;

/*----------------------------------------------------------------------------*/

typedef struct ov_event_socket_config {

  ov_event_loop *loop;
  ov_event_engine *engine;

  struct {

    uint64_t io_timeout_usec;
    uint64_t accept_to_io_timeout_usec;
    uint64_t reconnect_interval_usec;

  } timer;

  struct {

    void *userdata;
    void (*close)(void *userdata, int socket);
    void (*connected)(void *userdata, int socket, bool result);

  } callback;

} ov_event_socket_config;

/*----------------------------------------------------------------------------*/

typedef struct ov_event_socket_server_config {

  ov_socket_configuration socket;

} ov_event_socket_server_config;

/*----------------------------------------------------------------------------*/

typedef struct ov_event_socket_client_config {

  ov_socket_configuration socket;

  uint64_t client_connect_trigger_usec;

  struct {

    /* SSL client data */

    char domain[PATH_MAX]; // hostname to use in handshake

    struct {

      char file[PATH_MAX]; // path to CA verify file
      char path[PATH_MAX]; // path to CAs to use

    } ca;

  } ssl;

  bool auto_reconnect;

} ov_event_socket_client_config;

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_event_socket *ov_event_socket_create(ov_event_socket_config config);
ov_event_socket *ov_event_socket_free(ov_event_socket *self);
ov_event_socket *ov_event_socket_cast(const void *self);

/*----------------------------------------------------------------------------*/

bool ov_event_socket_load_ssl_config(ov_event_socket *self, const char *path);

/*----------------------------------------------------------------------------*/

int ov_event_socket_create_listener(ov_event_socket *self,
                                    ov_event_socket_server_config config);

/*----------------------------------------------------------------------------*/

int ov_event_socket_create_connection(ov_event_socket *self,
                                      ov_event_socket_client_config config);

/*----------------------------------------------------------------------------*/

bool ov_event_socket_close(ov_event_socket *self, int socket);

/*----------------------------------------------------------------------------*/

bool ov_event_socket_send(ov_event_socket *self, int socket,
                          const ov_json_value *value);

/*----------------------------------------------------------------------------*/

bool ov_event_socket_set_debug(ov_event_socket *self, bool on);

#endif /* ov_event_socket_h */
