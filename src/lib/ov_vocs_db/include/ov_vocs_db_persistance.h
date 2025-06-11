/***
        ------------------------------------------------------------------------

        Copyright (c) 2022 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_vocs_db_persistance.h
        @author         Markus Töpfer

        @date           2022-07-26


        ------------------------------------------------------------------------
*/
#ifndef ov_vocs_db_persistance_h
#define ov_vocs_db_persistance_h

#define OV_VOCS_DB_PERSISTANCE_CONFIG_FILE "config.json"

#include "ov_vocs_db.h"

#include <ov_base/ov_event_loop.h>

/*----------------------------------------------------------------------------*/

typedef struct ov_vocs_db_persistance ov_vocs_db_persistance;

/*----------------------------------------------------------------------------*/

typedef struct ov_vocs_db_persistance_config {

    ov_event_loop *loop;
    ov_vocs_db *db;

    char path[PATH_MAX];

    struct {

        uint64_t thread_lock_usec;
        uint64_t ldap_request_usec;

        uint64_t state_snapshot_seconds;
        uint64_t auth_snapshot_seconds;

    } timeout;

} ov_vocs_db_persistance_config;

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_vocs_db_persistance *ov_vocs_db_persistance_create(
    ov_vocs_db_persistance_config config);
ov_vocs_db_persistance *ov_vocs_db_persistance_free(
    ov_vocs_db_persistance *self);
ov_vocs_db_persistance *ov_vocs_db_persistance_cast(const void *self);

bool ov_vocs_db_persistance_load(ov_vocs_db_persistance *self);
bool ov_vocs_db_persistance_save(ov_vocs_db_persistance *self);

ov_vocs_db_persistance_config ov_vocs_db_persistance_config_from_json(
    const ov_json_value *val);

bool ov_vocs_db_persistance_ldap_import(ov_vocs_db_persistance *self,
                                        const char *host,
                                        const char *base,
                                        const char *user,
                                        const char *pass,
                                        const char *domain);

#endif /* ov_vocs_db_persistance_h */
