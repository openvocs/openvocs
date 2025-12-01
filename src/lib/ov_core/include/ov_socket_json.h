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
        @file           ov_socket_json.h
        @author         Markus

        @date           2024-10-11


        ------------------------------------------------------------------------
*/
#ifndef ov_socket_json_h
#define ov_socket_json_h

#include <ov_base/ov_event_loop.h>

/*----------------------------------------------------------------------------*/

typedef struct ov_socket_json ov_socket_json;

/*----------------------------------------------------------------------------*/

typedef struct ov_socket_json_config {

  ov_event_loop *loop;

  struct {

    uint64_t threadlock_timeout_usec;

  } limits;

} ov_socket_json_config;

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_socket_json *ov_socket_json_create(ov_socket_json_config config);
ov_socket_json *ov_socket_json_cast(const void *data);
ov_socket_json *ov_socket_json_free(ov_socket_json *self);

/**
 *  This function will return a copy of the current data set at socket.
 *  NOTE outgoing value is unconnected with socket data, to store or restore
 *  any changed data use ov_socket_json_set.
 */
ov_json_value *ov_socket_json_get(ov_socket_json *self, int socket);

/*----------------------------------------------------------------------------*/

/**
 *  Set value at slot position of socket. Will override any existing data at
 *  socket. Do not use value anymore after set.
 */
bool ov_socket_json_set(ov_socket_json *self, int socket,
                        ov_json_value **value);

/*----------------------------------------------------------------------------*/

/**
 *  Drop all data stored at socket.
 */
bool ov_socket_json_drop(ov_socket_json *self, int socket);

/*----------------------------------------------------------------------------*/

bool ov_socket_json_for_each_set_data(ov_socket_json *self, ov_json_value *out);

/*----------------------------------------------------------------------------*/

bool ov_socket_json_for_each(ov_socket_json *self, void *data,
                             bool (*function)(const void *key, void *val,
                                              void *data));

#endif /* ov_socket_json_h */
