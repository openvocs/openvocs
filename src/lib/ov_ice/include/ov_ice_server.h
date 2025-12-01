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
        @file           ov_ice_server.h
        @author         Markus Töpfer

        @date           2024-09-18


        ------------------------------------------------------------------------
*/
#ifndef ov_ice_server_h
#define ov_ice_server_h

typedef struct ov_ice_server ov_ice_server;

#include <ov_base/ov_event_loop.h>
#include <ov_base/ov_node.h>
#include <ov_base/ov_socket.h>

#include "ov_ice_definitions.h"

/*----------------------------------------------------------------------------*/

typedef enum {

  OV_ICE_STUN_SERVER = 0,
  OV_ICE_TURN_SERVER = 1

} ov_ice_server_type;

/*----------------------------------------------------------------------------*/

struct ov_ice_server {

  ov_node node;
  ov_ice_server_type type;
  ov_event_loop *loop;

  ov_socket_configuration socket;

  struct {

    char user[OV_ICE_STUN_USER_MAX];
    char pass[OV_ICE_STUN_PASS_MAX];

  } auth;

  struct {

    uint64_t keepalive_time_usecs;

  } limits;

  struct {

    uint32_t keepalive;

  } timer;
};

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_ice_server *ov_ice_server_create(ov_event_loop *loop);
ov_ice_server *ov_ice_server_cast(const void *data);
void *ov_ice_server_free(void *self);

/*----------------------------------------------------------------------------*/

ov_json_value *ov_ice_server_config_to_json(const ov_ice_server *config);

/*----------------------------------------------------------------------------*/

ov_ice_server ov_ice_server_config_from_json(ov_json_value *value);

/*----------------------------------------------------------------------------*/

ov_ice_server_type ov_ice_server_type_from_string(const char *string);

/*----------------------------------------------------------------------------*/

const char *ov_ice_server_type_to_string(ov_ice_server_type type);

#endif /* ov_ice_server_h */
