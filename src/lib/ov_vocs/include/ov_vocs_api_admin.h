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
        @file           ov_vocs_api_admin.h
        @author         Töpfer, Markus

        @date           2025-11-13


        ------------------------------------------------------------------------
*/
#ifndef ov_vocs_api_admin_h
#define ov_vocs_api_admin_h

#include "ov_vocs_app.h"
#include "ov_vocs_core.h"

#include <ov_core/ov_event_engine.h>
#include <ov_core/ov_broadcast_registry.h>
#include <ov_core/ov_event_api.h>
#include <ov_core/ov_event_async.h>
#include <ov_core/ov_event_engine.h>
#include <ov_core/ov_event_session.h>
#include <ov_core/ov_socket_json.h>

/*----------------------------------------------------------------------------*/

typedef struct ov_vocs_api_admin ov_vocs_api_admin;

/*----------------------------------------------------------------------------*/

typedef struct ov_vocs_api_admin_config {

    ov_event_loop *loop;
    ov_vocs_app *app;
    ov_vocs_core *core;

    ov_vocs_db *db;
    ov_vocs_db_persistance *persistance;

    ov_event_engine *engine;

    ov_dict *loops;
    ov_ldap *ldap;
    
    ov_event_session *user_sessions;
    ov_socket_json *connections;

    ov_event_async_store *async;
    ov_broadcast_registry *broadcasts;

    struct {

        uint64_t response_usec;

    } limits;

} ov_vocs_api_admin_config;

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_vocs_api_admin *ov_vocs_api_admin_create(ov_vocs_api_admin_config config);
ov_vocs_api_admin *ov_vocs_api_admin_free(ov_vocs_api_admin *self);
ov_vocs_api_admin *ov_vocs_api_admin_cast(const void *self);


#endif /* ov_vocs_api_admin_h */
