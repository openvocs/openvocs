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
        @file           ov_vocs_test_db.c
        @author         Markus Töpfer

        @date           2022-11-24


        ------------------------------------------------------------------------
*/
#include "../include/ov_vocs_test_db.h"

#include <ov_base/ov_config_keys.h>

static bool create_test_db(ov_vocs_db *db) {

    if (!db) goto error;

    if (!ov_vocs_db_create_entity(
            db, OV_VOCS_DB_DOMAIN, "localhost", OV_VOCS_DB_SCOPE_DOMAIN, NULL))
        goto error;

    if (!ov_vocs_db_create_entity(db,
                                  OV_VOCS_DB_PROJECT,
                                  "project@localhost",
                                  OV_VOCS_DB_SCOPE_DOMAIN,
                                  "localhost"))
        goto error;

    if (!ov_vocs_db_create_entity(db,
                                  OV_VOCS_DB_LOOP,
                                  "loop1",
                                  OV_VOCS_DB_SCOPE_PROJECT,
                                  "project@localhost"))
        goto error;

    if (!ov_vocs_db_create_entity(db,
                                  OV_VOCS_DB_LOOP,
                                  "loop2",
                                  OV_VOCS_DB_SCOPE_PROJECT,
                                  "project@localhost"))
        goto error;

    if (!ov_vocs_db_create_entity(db,
                                  OV_VOCS_DB_LOOP,
                                  "loop3",
                                  OV_VOCS_DB_SCOPE_PROJECT,
                                  "project@localhost"))
        goto error;

    if (!ov_vocs_db_create_entity(db,
                                  OV_VOCS_DB_LOOP,
                                  "loop4",
                                  OV_VOCS_DB_SCOPE_PROJECT,
                                  "project@localhost"))
        goto error;

    if (!ov_vocs_db_create_entity(db,
                                  OV_VOCS_DB_LOOP,
                                  "loop5",
                                  OV_VOCS_DB_SCOPE_PROJECT,
                                  "project@localhost"))
        goto error;

    if (!ov_vocs_db_create_entity(db,
                                  OV_VOCS_DB_ROLE,
                                  "admin",
                                  OV_VOCS_DB_SCOPE_PROJECT,
                                  "project@localhost"))
        goto error;

    if (!ov_vocs_db_create_entity(db,
                                  OV_VOCS_DB_ROLE,
                                  "role1",
                                  OV_VOCS_DB_SCOPE_PROJECT,
                                  "project@localhost"))
        goto error;

    if (!ov_vocs_db_create_entity(db,
                                  OV_VOCS_DB_ROLE,
                                  "role2",
                                  OV_VOCS_DB_SCOPE_PROJECT,
                                  "project@localhost"))
        goto error;

    if (!ov_vocs_db_create_entity(db,
                                  OV_VOCS_DB_ROLE,
                                  "role3",
                                  OV_VOCS_DB_SCOPE_PROJECT,
                                  "project@localhost"))
        goto error;

    if (!ov_vocs_db_create_entity(db,
                                  OV_VOCS_DB_USER,
                                  "user1",
                                  OV_VOCS_DB_SCOPE_PROJECT,
                                  "project@localhost"))
        goto error;

    if (!ov_vocs_db_create_entity(db,
                                  OV_VOCS_DB_USER,
                                  "user2",
                                  OV_VOCS_DB_SCOPE_PROJECT,
                                  "project@localhost"))
        goto error;

    if (!ov_vocs_db_create_entity(db,
                                  OV_VOCS_DB_USER,
                                  "user3",
                                  OV_VOCS_DB_SCOPE_PROJECT,
                                  "project@localhost"))
        goto error;

    if (!ov_vocs_db_set_password(db, "user1", "user1")) goto error;

    if (!ov_vocs_db_set_password(db, "user2", "user2")) goto error;

    if (!ov_vocs_db_set_password(db, "user3", "user3")) goto error;

    ov_json_value *out = ov_json_object();

    ov_json_object_set(out, "user1", ov_json_null());
    ov_json_object_set(out, "user2", ov_json_null());
    ov_json_object_set(out, "user3", ov_json_null());

    if (!ov_vocs_db_update_entity_key(
            db, OV_VOCS_DB_ROLE, "role1", OV_KEY_USERS, out))
        goto error;

    out = ov_json_value_free(out);
    out = ov_json_object();

    ov_json_object_set(out, "user1", ov_json_null());
    ov_json_object_set(out, "user2", ov_json_null());

    if (!ov_vocs_db_update_entity_key(
            db, OV_VOCS_DB_ROLE, "role2", OV_KEY_USERS, out))
        goto error;

    out = ov_json_value_free(out);
    out = ov_json_object();

    ov_json_object_set(out, "user1", ov_json_null());
    ov_json_object_set(out, "user3", ov_json_null());

    if (!ov_vocs_db_update_entity_key(
            db, OV_VOCS_DB_ROLE, "role3", OV_KEY_USERS, out))
        goto error;

    out = ov_json_value_free(out);
    out = ov_json_object();

    ov_json_object_set(out, "role1", ov_json_true());
    ov_json_object_set(out, "role2", ov_json_true());
    ov_json_object_set(out, "role3", ov_json_true());

    if (!ov_vocs_db_update_entity_key(
            db, OV_VOCS_DB_LOOP, "loop1", OV_KEY_ROLES, out))
        goto error;

    out = ov_json_value_free(out);
    out = ov_json_object();

    ov_json_object_set(out, "role1", ov_json_true());
    ov_json_object_set(out, "role2", ov_json_true());
    ov_json_object_set(out, "role3", ov_json_false());

    if (!ov_vocs_db_update_entity_key(
            db, OV_VOCS_DB_LOOP, "loop2", OV_KEY_ROLES, out))
        goto error;

    out = ov_json_value_free(out);
    out = ov_json_object();

    ov_json_object_set(out, "role1", ov_json_true());
    ov_json_object_set(out, "role2", ov_json_false());
    ov_json_object_set(out, "role3", ov_json_true());

    if (!ov_vocs_db_update_entity_key(
            db, OV_VOCS_DB_LOOP, "loop3", OV_KEY_ROLES, out))
        goto error;

    out = ov_json_value_free(out);
    out = ov_json_object();

    ov_json_object_set(out, "role1", ov_json_true());
    ov_json_object_set(out, "role2", ov_json_true());

    if (!ov_vocs_db_update_entity_key(
            db, OV_VOCS_DB_LOOP, "loop4", OV_KEY_ROLES, out))
        goto error;

    out = ov_json_value_free(out);
    out = ov_json_object();

    ov_json_object_set(out, "role2", ov_json_true());
    ov_json_object_set(out, "role3", ov_json_true());

    if (!ov_vocs_db_update_entity_key(
            db, OV_VOCS_DB_LOOP, "loop5", OV_KEY_ROLES, out))
        goto error;

    out = ov_json_value_free(out);

    if (!ov_vocs_db_add_domain_admin(db, "localhost", "user1")) goto error;

    if (!ov_vocs_db_add_project_admin(db, "project@localhost", "user2"))
        goto error;

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

ov_vocs_db *ov_vocs_test_db_create() {

    ov_vocs_db *db = ov_vocs_db_create((ov_vocs_db_config){0});

    if (!create_test_db(db)) goto error;

    return db;
error:
    ov_vocs_db_free(db);
    return NULL;
}