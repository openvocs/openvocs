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
        @file           ov_event_trigger.h
        @author         Markus

        @date           2024-09-25

        @brief          ov_event_trigger may be used to connect unconnected
                        parts of the code. 


        ------------------------------------------------------------------------
*/
#ifndef ov_event_trigger_h
#define ov_event_trigger_h

#include <ov_base/ov_json.h>

typedef struct ov_event_trigger ov_event_trigger;

/*----------------------------------------------------------------------------*/

typedef struct ov_event_trigger_config {

    bool debug;

} ov_event_trigger_config;

/*----------------------------------------------------------------------------*/

typedef struct ov_event_trigger_data {

    void *userdata;
    void (*process)(void *userdata, ov_json_value *event);

} ov_event_trigger_data;

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_event_trigger *ov_event_trigger_create(ov_event_trigger_config config);
ov_event_trigger *ov_event_trigger_cast(const void *data);
void *ov_event_trigger_free(void *self);

bool ov_event_trigger_register_listener(ov_event_trigger *self,
                                        const char *key,
                                        ov_event_trigger_data data);

bool ov_event_trigger_send(ov_event_trigger *self,
                           const char *key,
                           ov_json_value *event);

#endif /* ov_event_trigger_h */
