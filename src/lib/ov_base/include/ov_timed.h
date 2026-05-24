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
        @file           ov_timed.h
        @author         Töpfer, Markus

        @date           2026-05-23


        ------------------------------------------------------------------------
*/
#ifndef ov_timed_h
#define ov_timed_h

#include "ov_time.h"
#include "ov_event_loop.h"
#include "ov_id.h"

typedef struct ov_timed ov_timed;

typedef struct ov_timed_config{

    ov_event_loop *loop;

} ov_timed_config;

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_timed *ov_timed_create(ov_timed_config config);
ov_timed *ov_timed_free(ov_timed *self);

/*----------------------------------------------------------------------------*/

/**
 *  add a one time callback
 */
bool ov_timed_add(
    ov_timed *self, 
    ov_time time, 
    const ov_id id,
    void *userdata, 
    void (*callback)(void *userdata, const char *uuid));

/*----------------------------------------------------------------------------*/

bool ov_timed_del(
    ov_timed *self,
    const ov_id id);

#endif /* ov_timed_h */
