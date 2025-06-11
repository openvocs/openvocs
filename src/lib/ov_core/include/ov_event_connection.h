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
        @file           ov_event_connection.h
        @author         Markus

        @date           2024-01-09


        ------------------------------------------------------------------------
*/
#ifndef ov_event_connection_h
#define ov_event_connection_h

#include "ov_event_io.h"

/*----------------------------------------------------------------------------*/

typedef struct ov_event_connection ov_event_connection;

/*----------------------------------------------------------------------------*/

typedef struct ov_event_connection_config {

    int socket;
    ov_event_parameter params;

} ov_event_connection_config;

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_event_connection *ov_event_connection_create(
    ov_event_connection_config config);

/*----------------------------------------------------------------------------*/

ov_event_connection *ov_event_connection_free(ov_event_connection *self);

/*----------------------------------------------------------------------------*/

void *ov_event_connection_free_void(void *self);

/*----------------------------------------------------------------------------*/

ov_event_connection *ov_event_connection_cast(const void *data);

/*----------------------------------------------------------------------------*/

bool ov_event_connection_send(ov_event_connection *self,
                              const ov_json_value *json);

/*----------------------------------------------------------------------------*/

int ov_event_connection_get_socket(ov_event_connection *self);

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC DATA
 *
 *      ------------------------------------------------------------------------
 */

bool ov_event_connection_set(ov_event_connection *self,
                             const char *key,
                             const char *value);

/*----------------------------------------------------------------------------*/

const char *ov_event_connection_get(ov_event_connection *self, const char *key);

/*----------------------------------------------------------------------------*/

bool ov_event_connection_set_json(ov_event_connection *self,
                                  const char *key,
                                  const ov_json_value *value);

/*----------------------------------------------------------------------------*/

const ov_json_value *ov_event_connection_get_json(ov_event_connection *self,
                                                  const char *key);

#endif /* ov_event_connection_h */
