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
        @file           ov_vocs_db_app.h
        @author         Markus Töpfer

        @date           2023-11-16


        ------------------------------------------------------------------------
*/
#ifndef ov_vocs_db_app_h
#define ov_vocs_db_app_h

#include "ov_vocs_db.h"
#include "ov_vocs_db_persistance.h"
#include "ov_vocs_env.h"

#include <ov_ldap/ov_ldap.h>

#include <ov_core/ov_event_io.h>

#define OV_VOCS_DB_UPDATE_DB "update_db"
#define OV_VOCS_DB_UPDATE_PASSWORD "update_password"
#define OV_VOCS_DB_ADMIN_DOMAINS "admin_domains"
#define OV_VOCS_DB_ADMIN_PROJECTS "admin_projects"
#define OV_VOCS_DB_ID_EXISTS "check_id_exists"
#define OV_VOCS_DB_GET "get"
#define OV_VOCS_DB_DELETE "delete"
#define OV_VOCS_DB_CREATE "create"
#define OV_VOCS_DB_VERIFY "verify"
#define OV_VOCS_DB_UPDATE "update"
#define OV_VOCS_DB_LOAD "load"
#define OV_VOCS_DB_SAVE "save"
#define OV_VOCS_DB_SET_LAYOUT "set_layout"
#define OV_VOCS_DB_GET_LAYOUT "get_layout"
#define OV_VOCS_DB_GET_KEY "get_key"
#define OV_VOCS_DB_UPDATE_KEY "update_key"
#define OV_VOCS_DB_DELETE_KEY "delete_key"
#define OV_VOCS_DB_ADD_DOMAIN_ADMIN "add_domain_admin"
#define OV_VOCS_DB_ADD_PROJECT_ADMIN "add_project_admin"
#define OV_VOCS_DB_LDAP_IMPORT "ldap_import"

#define OV_VOCS_DB_SET_KEYSET_LAYOUT "set_keyset_layout"
#define OV_VOCS_DB_GET_KEYSET_LAYOUT "get_keyset_layout"

#define OV_VOCS_DB_SET_USER_DATA "set_user_data"
#define OV_VOCS_DB_GET_USER_DATA "get_user_data"

/*----------------------------------------------------------------------------*/

typedef struct ov_vocs_db_app ov_vocs_db_app;

/*----------------------------------------------------------------------------*/

typedef struct ov_vocs_db_app_config {

    ov_event_loop *loop;
    ov_vocs_db *db;
    ov_vocs_db_persistance *persistance;

    ov_vocs_env env;

    struct {

        bool enable;
        ov_ldap_config config;
        uint64_t timeout_usec;

    } ldap;

} ov_vocs_db_app_config;

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_vocs_db_app *ov_vocs_db_app_create(ov_vocs_db_app_config config);
ov_vocs_db_app *ov_vocs_db_app_free(ov_vocs_db_app *app);
ov_vocs_db_app *ov_vocs_db_app_cast(const void *data);

ov_event_io_config ov_vocs_db_app_io_uri_config(ov_vocs_db_app *self);

ov_vocs_db_app_config ov_vocs_db_app_config_from_json(const ov_json_value *v);

bool ov_vocs_db_app_send_broadcast(
        ov_vocs_db_app *self,
        const ov_json_value *broadcast);

#endif /* ov_vocs_db_app_h */
