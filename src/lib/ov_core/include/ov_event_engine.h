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
        @file           ov_event_engine.h
        @author         Markus Töpfer

        @date           2023-10-08

        ov_event_engine is a dict of events, which can be registered and
        unregistered. on push some event may be executed.


        ------------------------------------------------------------------------
*/
#ifndef ov_event_engine_h
#define ov_event_engine_h

#include "ov_event_api.h"
#include "ov_event_io.h"

typedef struct ov_event_engine ov_event_engine;

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_event_engine *ov_event_engine_create();
ov_event_engine *ov_event_engine_free(ov_event_engine *self);
ov_event_engine *ov_event_engine_cast(const void *data);

/*----------------------------------------------------------------------------*/

bool ov_event_engine_register(
    ov_event_engine *self,
    const char *name,
    void *userdata,
    bool (*process)(void *userdata,
                    const int socket,
                    const ov_event_parameter *parameter,
                    ov_json_value *input));

/*----------------------------------------------------------------------------*/

bool ov_event_engine_unregister(ov_event_engine *self, const char *name);

/*----------------------------------------------------------------------------*/

bool ov_event_engine_push(ov_event_engine *self,
                          int socket,
                          ov_event_parameter parameter,
                          ov_json_value *input);

#endif /* ov_event_engine_h */
