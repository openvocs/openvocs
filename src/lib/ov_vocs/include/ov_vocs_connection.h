/***
        ------------------------------------------------------------------------

        Copyright (c) 2021 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_vocs_connection.h
        @author         Markus Töpfer

        @date           2021-10-09


        ------------------------------------------------------------------------
*/
#ifndef ov_vocs_connection_h
#define ov_vocs_connection_h

#include <stdbool.h>
#include <strings.h>

#include <ov_base/ov_config_keys.h>
#include <ov_base/ov_json_value.h>
#include <ov_base/ov_socket.h>

#include <ov_core/ov_event_io.h>

#include <ov_vocs_db/ov_vocs_permission.h>

/*----------------------------------------------------------------------------*/

typedef struct ov_vocs_connection {

  int socket;

  ov_socket_data local;
  ov_socket_data remote;

  ov_json_value *data;

  bool ice_success;
  bool media_ready;

} ov_vocs_connection;

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

bool ov_vocs_connection_clear(ov_vocs_connection *self);

bool ov_vocs_connection_is_authenticated(ov_vocs_connection *self);
bool ov_vocs_connection_is_authorized(ov_vocs_connection *self);

const char *ov_vocs_connection_get_user(ov_vocs_connection *self);
bool ov_vocs_connection_set_user(ov_vocs_connection *self, const char *user);

const char *ov_vocs_connection_get_role(ov_vocs_connection *self);
bool ov_vocs_connection_set_role(ov_vocs_connection *self, const char *role);

const char *ov_vocs_connection_get_session(ov_vocs_connection *self);
bool ov_vocs_connection_set_session(ov_vocs_connection *self,
                                    const char *session);

const char *ov_vocs_connection_get_client(ov_vocs_connection *self);
bool ov_vocs_connection_set_client(ov_vocs_connection *self,
                                   const char *client);

ov_vocs_permission ov_vocs_connection_get_loop_state(ov_vocs_connection *self,
                                                     const char *loop);
bool ov_vocs_connection_set_loop_state(ov_vocs_connection *self,
                                       const char *loop,
                                       ov_vocs_permission state);

uint8_t ov_vocs_connection_get_loop_volume(ov_vocs_connection *self,
                                           const char *loop);
bool ov_vocs_connection_set_loop_volume(ov_vocs_connection *self,
                                        const char *loop, uint8_t percent);

ov_json_value *ov_vocs_connection_get_state(const ov_vocs_connection *self);

#endif /* ov_vocs_connection_h */
