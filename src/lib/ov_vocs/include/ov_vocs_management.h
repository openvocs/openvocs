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
        @file           ov_vocs_management.h
        @author         Töpfer, Markus

        @date           2025-09-17

        Management Interface for ov_vocs


        ------------------------------------------------------------------------
*/
#ifndef ov_vocs_management_h
#define ov_vocs_management_h

#include "ov_vocs.h"
#include <ov_vocs_db/ov_vocs_db.h>
#include <ov_core/ov_event_io.h>
#include <ov_base/ov_event_loop.h>
#include <ov_ldap/ov_ldap.h>

/*----------------------------------------------------------------------------*/

typedef struct ov_vocs_management ov_vocs_management;

/*----------------------------------------------------------------------------*/

typedef struct ov_vocs_management_config {

    ov_event_loop *loop;
    ov_vocs *vocs;
    ov_vocs_db *db;
    ov_vocs_db_persistance *persistance;

    ov_vocs_env env;

    ov_ldap *ldap;

    struct {

        char path[PATH_MAX];

    } sessions;

    struct {

        uint64_t response_usec;

    } timeout;

} ov_vocs_management_config;

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_vocs_management *ov_vocs_management_create(ov_vocs_management_config config);
ov_vocs_management *ov_vocs_management_free(ov_vocs_management *self);
ov_vocs_management *ov_vocs_management_cast(const void *data);

ov_event_io_config ov_vocs_management_io_uri_config(ov_vocs_management *self);

#endif /* ov_vocs_management_h */
