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
        @file           ov_vocs_db_test.c
        @author         Markus Töpfer

        @date           2022-07-23


        ------------------------------------------------------------------------
*/
#include "ov_vocs_db.c"
#include <ov_test/testrun.h>

#include <ov_base/ov_time.h>

static bool create_test_db(ov_vocs_db *db) {

    if (!db) goto error;

    if (!ov_vocs_db_create_entity(
            db, OV_VOCS_DB_DOMAIN, "domain1", OV_VOCS_DB_SCOPE_DOMAIN, NULL))
        goto error;

    if (!ov_vocs_db_create_entity(
            db, OV_VOCS_DB_DOMAIN, "domain2", OV_VOCS_DB_SCOPE_DOMAIN, NULL))
        goto error;

    if (!ov_vocs_db_create_entity(
            db, OV_VOCS_DB_DOMAIN, "domain3", OV_VOCS_DB_SCOPE_DOMAIN, NULL))
        goto error;

    if (!ov_vocs_db_create_entity(db,
                                  OV_VOCS_DB_PROJECT,
                                  "project1",
                                  OV_VOCS_DB_SCOPE_DOMAIN,
                                  "domain1"))
        goto error;

    if (!ov_vocs_db_create_entity(db,
                                  OV_VOCS_DB_PROJECT,
                                  "project2",
                                  OV_VOCS_DB_SCOPE_DOMAIN,
                                  "domain2"))
        goto error;

    if (!ov_vocs_db_create_entity(db,
                                  OV_VOCS_DB_PROJECT,
                                  "project3",
                                  OV_VOCS_DB_SCOPE_DOMAIN,
                                  "domain3"))
        goto error;

    if (!ov_vocs_db_create_entity(db,
                                  OV_VOCS_DB_LOOP,
                                  "loop11",
                                  OV_VOCS_DB_SCOPE_PROJECT,
                                  "project1"))
        goto error;

    if (!ov_vocs_db_create_entity(db,
                                  OV_VOCS_DB_LOOP,
                                  "loop12",
                                  OV_VOCS_DB_SCOPE_PROJECT,
                                  "project1"))
        goto error;

    if (!ov_vocs_db_create_entity(db,
                                  OV_VOCS_DB_LOOP,
                                  "loop13",
                                  OV_VOCS_DB_SCOPE_PROJECT,
                                  "project1"))
        goto error;

    if (!ov_vocs_db_create_entity(db,
                                  OV_VOCS_DB_LOOP,
                                  "loop21",
                                  OV_VOCS_DB_SCOPE_PROJECT,
                                  "project2"))
        goto error;

    if (!ov_vocs_db_create_entity(db,
                                  OV_VOCS_DB_LOOP,
                                  "loop22",
                                  OV_VOCS_DB_SCOPE_PROJECT,
                                  "project2"))
        goto error;

    if (!ov_vocs_db_create_entity(db,
                                  OV_VOCS_DB_LOOP,
                                  "loop23",
                                  OV_VOCS_DB_SCOPE_PROJECT,
                                  "project2"))
        goto error;

    if (!ov_vocs_db_create_entity(db,
                                  OV_VOCS_DB_LOOP,
                                  "loop31",
                                  OV_VOCS_DB_SCOPE_PROJECT,
                                  "project3"))
        goto error;

    if (!ov_vocs_db_create_entity(db,
                                  OV_VOCS_DB_LOOP,
                                  "loop32",
                                  OV_VOCS_DB_SCOPE_PROJECT,
                                  "project3"))
        goto error;

    if (!ov_vocs_db_create_entity(db,
                                  OV_VOCS_DB_LOOP,
                                  "loop33",
                                  OV_VOCS_DB_SCOPE_PROJECT,
                                  "project3"))
        goto error;

    if (!ov_vocs_db_create_entity(db,
                                  OV_VOCS_DB_ROLE,
                                  "role11",
                                  OV_VOCS_DB_SCOPE_PROJECT,
                                  "project1"))
        goto error;

    if (!ov_vocs_db_create_entity(db,
                                  OV_VOCS_DB_ROLE,
                                  "role12",
                                  OV_VOCS_DB_SCOPE_PROJECT,
                                  "project1"))
        goto error;

    if (!ov_vocs_db_create_entity(db,
                                  OV_VOCS_DB_ROLE,
                                  "role13",
                                  OV_VOCS_DB_SCOPE_PROJECT,
                                  "project1"))
        goto error;

    if (!ov_vocs_db_create_entity(db,
                                  OV_VOCS_DB_ROLE,
                                  "role21",
                                  OV_VOCS_DB_SCOPE_PROJECT,
                                  "project2"))
        goto error;

    if (!ov_vocs_db_create_entity(db,
                                  OV_VOCS_DB_ROLE,
                                  "role22",
                                  OV_VOCS_DB_SCOPE_PROJECT,
                                  "project2"))
        goto error;

    if (!ov_vocs_db_create_entity(db,
                                  OV_VOCS_DB_ROLE,
                                  "role23",
                                  OV_VOCS_DB_SCOPE_PROJECT,
                                  "project2"))
        goto error;

    if (!ov_vocs_db_create_entity(db,
                                  OV_VOCS_DB_ROLE,
                                  "role31",
                                  OV_VOCS_DB_SCOPE_PROJECT,
                                  "project3"))
        goto error;

    if (!ov_vocs_db_create_entity(db,
                                  OV_VOCS_DB_ROLE,
                                  "role32",
                                  OV_VOCS_DB_SCOPE_PROJECT,
                                  "project3"))
        goto error;

    if (!ov_vocs_db_create_entity(db,
                                  OV_VOCS_DB_ROLE,
                                  "role33",
                                  OV_VOCS_DB_SCOPE_PROJECT,
                                  "project3"))
        goto error;

    if (!ov_vocs_db_create_entity(db,
                                  OV_VOCS_DB_USER,
                                  "user11",
                                  OV_VOCS_DB_SCOPE_PROJECT,
                                  "project1"))
        goto error;

    if (!ov_vocs_db_create_entity(db,
                                  OV_VOCS_DB_USER,
                                  "user12",
                                  OV_VOCS_DB_SCOPE_PROJECT,
                                  "project1"))
        goto error;

    if (!ov_vocs_db_create_entity(db,
                                  OV_VOCS_DB_USER,
                                  "user13",
                                  OV_VOCS_DB_SCOPE_PROJECT,
                                  "project1"))
        goto error;

    if (!ov_vocs_db_create_entity(db,
                                  OV_VOCS_DB_USER,
                                  "user21",
                                  OV_VOCS_DB_SCOPE_PROJECT,
                                  "project2"))
        goto error;

    if (!ov_vocs_db_create_entity(db,
                                  OV_VOCS_DB_USER,
                                  "user22",
                                  OV_VOCS_DB_SCOPE_PROJECT,
                                  "project2"))
        goto error;

    if (!ov_vocs_db_create_entity(db,
                                  OV_VOCS_DB_USER,
                                  "user23",
                                  OV_VOCS_DB_SCOPE_PROJECT,
                                  "project2"))
        goto error;

    if (!ov_vocs_db_create_entity(db,
                                  OV_VOCS_DB_USER,
                                  "user31",
                                  OV_VOCS_DB_SCOPE_PROJECT,
                                  "project3"))
        goto error;

    if (!ov_vocs_db_create_entity(db,
                                  OV_VOCS_DB_USER,
                                  "user32",
                                  OV_VOCS_DB_SCOPE_PROJECT,
                                  "project3"))
        goto error;

    if (!ov_vocs_db_create_entity(db,
                                  OV_VOCS_DB_USER,
                                  "user33",
                                  OV_VOCS_DB_SCOPE_PROJECT,
                                  "project3"))
        goto error;

    if (!ov_vocs_db_create_entity(
            db, OV_VOCS_DB_USER, "user1", OV_VOCS_DB_SCOPE_DOMAIN, "domain1"))
        goto error;

    if (!ov_vocs_db_create_entity(
            db, OV_VOCS_DB_USER, "user2", OV_VOCS_DB_SCOPE_DOMAIN, "domain2"))
        goto error;

    if (!ov_vocs_db_create_entity(
            db, OV_VOCS_DB_USER, "user3", OV_VOCS_DB_SCOPE_DOMAIN, "domain3"))
        goto error;

    ov_json_value *out = ov_json_object();
    ov_json_object_set(out, "role11", ov_json_true());
    ov_json_object_set(out, "role21", ov_json_false());
    ov_json_object_set(out, "role31", ov_json_false());
    ov_json_object_set(out, "role12", ov_json_true());
    ov_json_object_set(out, "role13", ov_json_true());

    if (!ov_vocs_db_update_entity_key(
            db, OV_VOCS_DB_LOOP, "loop11", OV_KEY_ROLES, out))
        goto error;

    out = ov_json_value_free(out);
    out = ov_json_object();

    ov_json_object_set(out, "user31", ov_json_null());
    ov_json_object_set(out, "user32", ov_json_null());
    ov_json_object_set(out, "user33", ov_json_null());

    if (!ov_vocs_db_update_entity_key(
            db, OV_VOCS_DB_ROLE, "role31", OV_KEY_USERS, out))
        goto error;

    out = ov_json_value_free(out);

    return true;
error:
    return false;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

int test_ov_vocs_db_create() {

    ov_vocs_db *db = ov_vocs_db_create((ov_vocs_db_config){0});
    testrun(db);
    testrun(ov_vocs_db_cast(db));

    testrun(IMPL_DEFAULT_LOCK_USEC == db->config.timeout.thread_lock_usec);

    testrun(db->index.domains);
    testrun(db->index.projects);
    testrun(db->index.users);
    testrun(db->index.roles);
    testrun(db->index.loops);

    testrun(ov_thread_lock_try_lock(&db->lock));
    testrun(ov_thread_lock_unlock(&db->lock));

    testrun(NULL == ov_vocs_db_free(db));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_vocs_db_cast() {

    ov_vocs_db *db = ov_vocs_db_create((ov_vocs_db_config){0});
    testrun(db);
    testrun(ov_vocs_db_cast(db));

    for (size_t i = 0; i < 0xffff; i++) {

        db->magic_byte = i;
        if (i == OV_VOCS_DB_MAGIC_BYTE) {
            testrun(ov_vocs_db_cast(db));
        } else {
            testrun(!ov_vocs_db_cast(db));
        }
    }

    db->magic_byte = OV_VOCS_DB_MAGIC_BYTE;
    testrun(NULL == ov_vocs_db_free(db));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_vocs_db_free() {

    ov_vocs_db *db = ov_vocs_db_create((ov_vocs_db_config){0});
    testrun(db);
    testrun(ov_vocs_db_cast(db));
    testrun(NULL == ov_vocs_db_free(db));

    db = ov_vocs_db_create((ov_vocs_db_config){0});
    db->magic_byte = 0;
    testrun(db == ov_vocs_db_free(db));

    db->magic_byte = OV_VOCS_DB_MAGIC_BYTE;
    testrun(NULL == ov_vocs_db_free(db));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_vocs_db_dump() {

    ov_vocs_db *db = ov_vocs_db_create((ov_vocs_db_config){0});
    testrun(db);

    testrun(!ov_vocs_db_dump(NULL, NULL));
    testrun(!ov_vocs_db_dump(NULL, db));
    testrun(!ov_vocs_db_dump(stdout, NULL));

    testrun(ov_vocs_db_dump(stdout, db));

    testrun(ov_vocs_db_create_entity(
        db, OV_VOCS_DB_DOMAIN, "domain", OV_VOCS_DB_SCOPE_DOMAIN, NULL));

    testrun(ov_vocs_db_create_entity(
        db, OV_VOCS_DB_PROJECT, "project", OV_VOCS_DB_SCOPE_DOMAIN, "domain"));

    testrun(ov_vocs_db_create_entity(
        db, OV_VOCS_DB_LOOP, "loop", OV_VOCS_DB_SCOPE_DOMAIN, "domain"));

    testrun(ov_vocs_db_create_entity(
        db, OV_VOCS_DB_ROLE, "role", OV_VOCS_DB_SCOPE_DOMAIN, "domain"));

    testrun(ov_vocs_db_create_entity(
        db, OV_VOCS_DB_USER, "user", OV_VOCS_DB_SCOPE_DOMAIN, "domain"));

    testrun(ov_vocs_db_create_entity(db,
                                     OV_VOCS_DB_LOOP,
                                     "loop_project",
                                     OV_VOCS_DB_SCOPE_PROJECT,
                                     "project"));

    testrun(ov_vocs_db_create_entity(db,
                                     OV_VOCS_DB_ROLE,
                                     "role_project",
                                     OV_VOCS_DB_SCOPE_PROJECT,
                                     "project"));

    testrun(ov_vocs_db_create_entity(db,
                                     OV_VOCS_DB_USER,
                                     "user_project",
                                     OV_VOCS_DB_SCOPE_PROJECT,
                                     "project"));

    testrun(ov_vocs_db_dump(stdout, db));

    testrun(NULL == ov_vocs_db_free(db));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_vocs_db_create_entity() {

    ov_vocs_db *db = ov_vocs_db_create((ov_vocs_db_config){0});
    testrun(db);

    testrun(!ov_vocs_db_create_entity(
        NULL, OV_VOCS_DB_DOMAIN, "domain", OV_VOCS_DB_SCOPE_DOMAIN, NULL));

    testrun(!ov_vocs_db_create_entity(
        db, OV_VOCS_DB_DOMAIN, NULL, OV_VOCS_DB_SCOPE_DOMAIN, NULL));

    testrun(!ov_vocs_db_create_entity(
        db, OV_VOCS_DB_PROJECT, "domain", OV_VOCS_DB_SCOPE_DOMAIN, "project"));

    testrun(ov_vocs_db_create_entity(
        db, OV_VOCS_DB_DOMAIN, "domain", OV_VOCS_DB_SCOPE_DOMAIN, NULL));

    // already created domain
    testrun(!ov_vocs_db_create_entity(
        db, OV_VOCS_DB_DOMAIN, "domain", OV_VOCS_DB_SCOPE_DOMAIN, NULL));

    testrun(ov_vocs_db_create_entity(
        db, OV_VOCS_DB_PROJECT, "project", OV_VOCS_DB_SCOPE_DOMAIN, "domain"));

    // already created project
    testrun(!ov_vocs_db_create_entity(
        db, OV_VOCS_DB_PROJECT, "project", OV_VOCS_DB_SCOPE_DOMAIN, "domain"));

    testrun(ov_vocs_db_create_entity(
        db, OV_VOCS_DB_DOMAIN, "domain2", OV_VOCS_DB_SCOPE_DOMAIN, NULL));

    // already created project in domain
    testrun(!ov_vocs_db_create_entity(
        db, OV_VOCS_DB_PROJECT, "project", OV_VOCS_DB_SCOPE_DOMAIN, "domain2"));

    testrun(ov_vocs_db_create_entity(db,
                                     OV_VOCS_DB_PROJECT,
                                     "project2",
                                     OV_VOCS_DB_SCOPE_DOMAIN,
                                     "domain2"));

    testrun(ov_vocs_db_create_entity(
        db, OV_VOCS_DB_LOOP, "loop", OV_VOCS_DB_SCOPE_DOMAIN, "domain"));

    testrun(ov_vocs_db_create_entity(
        db, OV_VOCS_DB_ROLE, "role", OV_VOCS_DB_SCOPE_DOMAIN, "domain"));

    testrun(ov_vocs_db_create_entity(
        db, OV_VOCS_DB_USER, "user", OV_VOCS_DB_SCOPE_DOMAIN, "domain"));

    testrun(ov_vocs_db_create_entity(db,
                                     OV_VOCS_DB_LOOP,
                                     "loop_project",
                                     OV_VOCS_DB_SCOPE_PROJECT,
                                     "project"));

    testrun(ov_vocs_db_create_entity(db,
                                     OV_VOCS_DB_ROLE,
                                     "role_project",
                                     OV_VOCS_DB_SCOPE_PROJECT,
                                     "project"));

    testrun(ov_vocs_db_create_entity(db,
                                     OV_VOCS_DB_USER,
                                     "user_project",
                                     OV_VOCS_DB_SCOPE_PROJECT,
                                     "project"));

    testrun(ov_vocs_db_create_entity(db,
                                     OV_VOCS_DB_LOOP,
                                     "loop_project2",
                                     OV_VOCS_DB_SCOPE_PROJECT,
                                     "project2"));

    testrun(ov_vocs_db_create_entity(db,
                                     OV_VOCS_DB_ROLE,
                                     "role_project2",
                                     OV_VOCS_DB_SCOPE_PROJECT,
                                     "project2"));

    testrun(ov_vocs_db_create_entity(db,
                                     OV_VOCS_DB_USER,
                                     "user_project2",
                                     OV_VOCS_DB_SCOPE_PROJECT,
                                     "project2"));

    // check created JSON

    testrun(ov_json_get(db->data.domains, "/domain"));
    testrun(ov_json_get(db->data.domains, "/domain2"));
    testrun(ov_json_get(db->data.domains, "/domain/loops"));
    testrun(ov_json_get(db->data.domains, "/domain/loops/loop"));
    testrun(ov_json_get(db->data.domains, "/domain/roles"));
    testrun(ov_json_get(db->data.domains, "/domain/roles/role"));
    testrun(ov_json_get(db->data.domains, "/domain/users"));
    testrun(ov_json_get(db->data.domains, "/domain/users/user"));
    testrun(ov_json_get(db->data.domains, "/domain/projects"));
    testrun(ov_json_get(db->data.domains, "/domain/projects/project"));
    testrun(ov_json_get(db->data.domains, "/domain/projects/project/loops"));
    testrun(ov_json_get(
        db->data.domains, "/domain/projects/project/loops/loop_project"));
    testrun(ov_json_get(db->data.domains, "/domain/projects/project/roles"));
    testrun(ov_json_get(
        db->data.domains, "/domain/projects/project/roles/role_project"));
    testrun(ov_json_get(db->data.domains, "/domain/projects/project/users"));
    testrun(ov_json_get(
        db->data.domains, "/domain/projects/project/users/user_project"));

    testrun(ov_json_get(db->data.domains, "/domain2"));
    testrun(!ov_json_get(db->data.domains, "/domain2/loops"));
    testrun(!ov_json_get(db->data.domains, "/domain2/roles"));
    testrun(!ov_json_get(db->data.domains, "/domain2/users"));
    testrun(ov_json_get(db->data.domains, "/domain2/projects"));
    testrun(ov_json_get(db->data.domains, "/domain2/projects/project2"));
    testrun(ov_json_get(db->data.domains, "/domain2/projects/project2/loops"));
    testrun(ov_json_get(
        db->data.domains, "/domain2/projects/project2/loops/loop_project2"));
    testrun(ov_json_get(db->data.domains, "/domain2/projects/project2/roles"));
    testrun(ov_json_get(
        db->data.domains, "/domain2/projects/project2/roles/role_project2"));
    testrun(ov_json_get(db->data.domains, "/domain2/projects/project2/users"));
    testrun(ov_json_get(
        db->data.domains, "/domain2/projects/project2/users/user_project2"));

    // check created index

    testrun(2 == ov_dict_count(db->index.domains));
    testrun(2 == ov_dict_count(db->index.projects));
    testrun(3 == ov_dict_count(db->index.roles));
    testrun(3 == ov_dict_count(db->index.loops));
    testrun(3 == ov_dict_count(db->index.users));

    testrun(ov_dict_get(db->index.domains, "domain") ==
            ov_json_get(db->data.domains, "/domain"));
    testrun(ov_dict_get(db->index.domains, "domain2") ==
            ov_json_get(db->data.domains, "/domain2"));

    testrun(ov_dict_get(db->index.users, "user") ==
            ov_json_get(db->data.domains, "/domain/users/user"));
    testrun(ov_dict_get(db->index.users, "user_project") ==
            ov_json_get(db->data.domains,
                        "/domain/projects/project/users/user_project"));
    testrun(ov_dict_get(db->index.users, "user_project2") ==
            ov_json_get(db->data.domains,
                        "/domain2/projects/project2/users/user_project2"));

    testrun(ov_dict_get(db->index.roles, "role") ==
            ov_json_get(db->data.domains, "/domain/roles/role"));
    testrun(ov_dict_get(db->index.roles, "role_project") ==
            ov_json_get(db->data.domains,
                        "/domain/projects/project/roles/role_project"));
    testrun(ov_dict_get(db->index.roles, "role_project2") ==
            ov_json_get(db->data.domains,
                        "/domain2/projects/project2/roles/role_project2"));

    testrun(ov_dict_get(db->index.loops, "loop") ==
            ov_json_get(db->data.domains, "/domain/loops/loop"));
    testrun(ov_dict_get(db->index.loops, "loop_project") ==
            ov_json_get(db->data.domains,
                        "/domain/projects/project/loops/loop_project"));
    testrun(ov_dict_get(db->index.loops, "loop_project2") ==
            ov_json_get(db->data.domains,
                        "/domain2/projects/project2/loops/loop_project2"));

    /*
        // basic Performance testing

        uint64_t start = 0;
        uint64_t stop = 0;

        uint64_t max = 0;
        uint64_t avg = 0;
        uint64_t x = 0;

        for (int i = 3; i < 13; i++){

            char buffer[100] = {0};
            snprintf(buffer, 100, "domain%i", i);

            start = ov_time_get_current_time_usecs();
            testrun(ov_vocs_db_create_entity(
                db,
                OV_VOCS_DB_DOMAIN,
                buffer,
                OV_VOCS_DB_SCOPE_DOMAIN,
                NULL));
            stop = ov_time_get_current_time_usecs();

            x = stop - start;

            if ( x > max) max = x;
            avg+=x;

        }

        fprintf(stdout, "10 domains avg %"PRIu64" max %"PRIu64" usec\n",
       avg/1000, max);

        avg = 0;
        max = 0;

        for (int i = 3; i < 1003; i++){

            char buffer[100] = {0};
            snprintf(buffer, 100, "user%i", i);

            start = ov_time_get_current_time_usecs();
            testrun(ov_vocs_db_create_entity(
                db,
                OV_VOCS_DB_USER,
                buffer,
                OV_VOCS_DB_SCOPE_DOMAIN,
                "domain"));
            stop = ov_time_get_current_time_usecs();

            x = stop - start;

            if ( x > max) max = x;
            avg+=x;

        }

        fprintf(stdout, "1k user avg %"PRIu64" max %"PRIu64" usec\n", avg/1000,
       max);

        avg = 0;
        max = 0;

        for (int i = 3; i < 1003; i++){

            char buffer[100] = {0};
            snprintf(buffer, 100, "user_project%i", i);

            start = ov_time_get_current_time_usecs();
            testrun(ov_vocs_db_create_entity(
                db,
                OV_VOCS_DB_USER,
                buffer,
                OV_VOCS_DB_SCOPE_PROJECT,
                "project"));
            stop = ov_time_get_current_time_usecs();

            x = stop - start;

            if ( x > max) max = x;
            avg+=x;

        }

        fprintf(stdout, "1k project user avg %"PRIu64" max %"PRIu64" usec\n",
       avg/1000, max);
    */
    testrun(NULL == ov_vocs_db_free(db));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static ov_json_value *create_id_object(const char *id) {

    ov_json_value *out = ov_json_object();
    ov_json_value *val = ov_json_string(id);
    ov_json_object_set(out, "id", val);
    return out;
}

/*----------------------------------------------------------------------------*/

int test_ov_vocs_db_update_entity_key() {

    ov_vocs_db *db = ov_vocs_db_create((ov_vocs_db_config){0});
    testrun(db);

    testrun(ov_vocs_db_create_entity(
        db, OV_VOCS_DB_DOMAIN, "domain", OV_VOCS_DB_SCOPE_DOMAIN, NULL));

    ov_json_value *par = NULL;
    ov_json_value *itm = NULL;
    ov_json_value *out = NULL;
    ov_json_value *val = ov_json_true();

    testrun(ov_vocs_db_update_entity_key(
        db, OV_VOCS_DB_DOMAIN, "domain", "key", val));

    testrun(ov_json_get(db->data.domains, "/domain"));
    testrun(ov_json_get(db->data.domains, "/domain/id"));
    testrun(ov_json_get(db->data.domains, "/domain/key"));
    testrun(ov_json_is_true(ov_json_get(db->data.domains, "/domain/key")));

    val = ov_json_value_free(val);
    val = ov_json_false();

    testrun(ov_vocs_db_update_entity_key(
        db, OV_VOCS_DB_DOMAIN, "domain", "key", val));

    testrun(ov_json_get(db->data.domains, "/domain"));
    testrun(ov_json_get(db->data.domains, "/domain/id"));
    testrun(ov_json_get(db->data.domains, "/domain/key"));
    testrun(ov_json_is_false(ov_json_get(db->data.domains, "/domain/key")));

    val = ov_json_value_free(val);

    // check projects update

    out = ov_json_object();
    val = create_id_object("project1");
    testrun(ov_json_object_set(out, "project1", val));
    val = create_id_object("project2");
    testrun(ov_json_object_set(out, "project2", val));

    testrun(ov_vocs_db_update_entity_key(
        db, OV_VOCS_DB_DOMAIN, "domain", OV_KEY_PROJECTS, out));

    testrun(ov_json_get(db->data.domains, "/domain"));
    testrun(ov_json_get(db->data.domains, "/domain/id"));
    testrun(ov_json_get(db->data.domains, "/domain/key"));
    testrun(ov_json_get(db->data.domains, "/domain/projects/project1"));
    testrun(ov_json_get(db->data.domains, "/domain/projects/project2"));
    testrun(ov_dict_get(db->index.projects, "project1") ==
            ov_json_get(db->data.domains, "/domain/projects/project1"));
    testrun(ov_dict_get(db->index.projects, "project2") ==
            ov_json_get(db->data.domains, "/domain/projects/project2"));

    // check projects update with project content

    itm = create_id_object("user1");
    par = ov_json_object();
    testrun(ov_json_object_set(par, "user1", itm));
    itm = create_id_object("user1");
    testrun(ov_json_object_set(par, "user2", itm));
    testrun(ov_json_object_set(val, OV_KEY_USERS, par));

    itm = create_id_object("role1");
    par = ov_json_object();
    testrun(ov_json_object_set(par, "role1", itm));
    itm = create_id_object("role2");
    testrun(ov_json_object_set(par, "role2", itm));
    testrun(ov_json_object_set(val, OV_KEY_ROLES, par));

    itm = create_id_object("loop1");
    par = ov_json_object();
    testrun(ov_json_object_set(par, "loop1", itm));
    itm = create_id_object("loop2");
    testrun(ov_json_object_set(par, "loop2", itm));
    testrun(ov_json_object_set(val, OV_KEY_LOOPS, par));

    testrun(ov_vocs_db_update_entity_key(
        db, OV_VOCS_DB_DOMAIN, "domain", OV_KEY_PROJECTS, out));

    // testrun(ov_vocs_db_dump(stdout, db));

    testrun(ov_json_get(db->data.domains, "/domain"));
    testrun(ov_json_get(db->data.domains, "/domain/id"));
    testrun(ov_json_get(db->data.domains, "/domain/key"));
    testrun(ov_json_get(db->data.domains, "/domain/projects/project1"));
    testrun(ov_json_get(db->data.domains, "/domain/projects/project2"));
    testrun(ov_json_get(db->data.domains, "/domain/projects/project2/users"));
    testrun(
        ov_json_get(db->data.domains, "/domain/projects/project2/users/user1"));
    testrun(
        ov_json_get(db->data.domains, "/domain/projects/project2/users/user2"));
    testrun(ov_json_get(db->data.domains, "/domain/projects/project2/roles"));
    testrun(
        ov_json_get(db->data.domains, "/domain/projects/project2/roles/role1"));
    testrun(
        ov_json_get(db->data.domains, "/domain/projects/project2/roles/role2"));
    testrun(ov_json_get(db->data.domains, "/domain/projects/project2/loops"));
    testrun(
        ov_json_get(db->data.domains, "/domain/projects/project2/loops/loop1"));
    testrun(ov_dict_get(db->index.projects, "project1") ==
            ov_json_get(db->data.domains, "/domain/projects/project1"));
    testrun(ov_dict_get(db->index.projects, "project2") ==
            ov_json_get(db->data.domains, "/domain/projects/project2"));
    testrun(
        ov_dict_get(db->index.users, "user1") ==
        ov_json_get(db->data.domains, "/domain/projects/project2/users/user1"));
    testrun(
        ov_dict_get(db->index.users, "user2") ==
        ov_json_get(db->data.domains, "/domain/projects/project2/users/user2"));
    testrun(
        ov_dict_get(db->index.roles, "role1") ==
        ov_json_get(db->data.domains, "/domain/projects/project2/roles/role1"));
    testrun(
        ov_dict_get(db->index.roles, "role2") ==
        ov_json_get(db->data.domains, "/domain/projects/project2/roles/role2"));
    testrun(
        ov_dict_get(db->index.loops, "loop1") ==
        ov_json_get(db->data.domains, "/domain/projects/project2/loops/loop1"));

    out = ov_json_value_free(out);

    out = ov_json_object();
    val = create_id_object("project1");
    testrun(ov_json_object_set(out, "project1", val));

    // check projects update with project content moved to project1

    itm = create_id_object("user1");
    par = ov_json_object();
    testrun(ov_json_object_set(par, "user1", itm));
    itm = create_id_object("user1");
    testrun(ov_json_object_set(par, "user2", itm));
    testrun(ov_json_object_set(val, OV_KEY_USERS, par));

    itm = create_id_object("role1");
    par = ov_json_object();
    testrun(ov_json_object_set(par, "role1", itm));
    itm = create_id_object("role2");
    testrun(ov_json_object_set(par, "role2", itm));
    testrun(ov_json_object_set(val, OV_KEY_ROLES, par));

    itm = create_id_object("loop1");
    par = ov_json_object();
    testrun(ov_json_object_set(par, "loop1", itm));
    itm = create_id_object("loop2");
    testrun(ov_json_object_set(par, "loop2", itm));
    testrun(ov_json_object_set(val, OV_KEY_LOOPS, par));

    testrun(ov_vocs_db_update_entity_key(
        db, OV_VOCS_DB_DOMAIN, "domain", OV_KEY_PROJECTS, out));

    testrun(ov_json_get(db->data.domains, "/domain"));
    testrun(ov_json_get(db->data.domains, "/domain/id"));
    testrun(ov_json_get(db->data.domains, "/domain/key"));
    testrun(ov_json_get(db->data.domains, "/domain/projects/project1"));
    testrun(!ov_json_get(db->data.domains, "/domain/projects/project2"));
    testrun(!ov_json_get(db->data.domains, "/domain/projects/project2/users"));
    testrun(!ov_json_get(
        db->data.domains, "/domain/projects/project2/users/user1"));
    testrun(!ov_json_get(
        db->data.domains, "/domain/projects/project2/users/user2"));
    testrun(!ov_json_get(db->data.domains, "/domain/projects/project2/roles"));
    testrun(!ov_json_get(
        db->data.domains, "/domain/projects/project2/roles/role1"));
    testrun(!ov_json_get(
        db->data.domains, "/domain/projects/project2/roles/role2"));
    testrun(!ov_json_get(db->data.domains, "/domain/projects/project2/loops"));
    testrun(!ov_json_get(
        db->data.domains, "/domain/projects/project2/loops/loop1"));
    testrun(ov_json_get(db->data.domains, "/domain/projects/project1/users"));
    testrun(
        ov_json_get(db->data.domains, "/domain/projects/project1/users/user1"));
    testrun(
        ov_json_get(db->data.domains, "/domain/projects/project1/users/user2"));
    testrun(ov_json_get(db->data.domains, "/domain/projects/project1/roles"));
    testrun(
        ov_json_get(db->data.domains, "/domain/projects/project1/roles/role1"));
    testrun(
        ov_json_get(db->data.domains, "/domain/projects/project1/roles/role2"));
    testrun(ov_json_get(db->data.domains, "/domain/projects/project1/loops"));
    testrun(
        ov_json_get(db->data.domains, "/domain/projects/project1/loops/loop1"));
    testrun(ov_dict_get(db->index.projects, "project1") ==
            ov_json_get(db->data.domains, "/domain/projects/project1"));
    testrun(ov_dict_get(db->index.projects, "project2") ==
            ov_json_get(db->data.domains, "/domain/projects/project2"));
    testrun(
        ov_dict_get(db->index.users, "user1") ==
        ov_json_get(db->data.domains, "/domain/projects/project1/users/user1"));
    testrun(
        ov_dict_get(db->index.users, "user2") ==
        ov_json_get(db->data.domains, "/domain/projects/project1/users/user2"));
    testrun(
        ov_dict_get(db->index.roles, "role1") ==
        ov_json_get(db->data.domains, "/domain/projects/project1/roles/role1"));
    testrun(
        ov_dict_get(db->index.roles, "role2") ==
        ov_json_get(db->data.domains, "/domain/projects/project1/roles/role2"));
    testrun(
        ov_dict_get(db->index.loops, "loop1") ==
        ov_json_get(db->data.domains, "/domain/projects/project1/loops/loop1"));

    // update with empty projects

    out = ov_json_value_free(out);
    out = ov_json_object();

    testrun(ov_vocs_db_update_entity_key(
        db, OV_VOCS_DB_DOMAIN, "domain", OV_KEY_PROJECTS, out));

    // testrun(ov_vocs_db_dump(stdout, db));

    testrun(ov_json_get(db->data.domains, "/domain"));
    testrun(ov_json_get(db->data.domains, "/domain/id"));
    testrun(ov_json_get(db->data.domains, "/domain/key"));
    testrun(ov_json_get(db->data.domains, "/domain/projects"));
    testrun(1 == ov_json_object_count(db->data.domains));
    testrun(3 ==
            ov_json_object_count(ov_json_get(db->data.domains, "/domain")));
    testrun(0 == ov_json_object_count(
                     ov_json_get(db->data.domains, "/domain/projects")));
    testrun(0 == ov_dict_count(db->index.projects));
    testrun(0 == ov_dict_count(db->index.users));
    testrun(0 == ov_dict_count(db->index.roles));
    testrun(0 == ov_dict_count(db->index.loops));

    // update domain users

    out = ov_json_value_free(out);
    out = ov_json_object();

    itm = create_id_object("user1");
    testrun(ov_json_object_set(out, "user1", itm));
    itm = create_id_object("user2");
    testrun(ov_json_object_set(out, "user2", itm));

    testrun(ov_vocs_db_update_entity_key(
        db, OV_VOCS_DB_DOMAIN, "domain", OV_KEY_USERS, out));

    // testrun(ov_vocs_db_dump(stdout, db));

    testrun(ov_json_get(db->data.domains, "/domain"));
    testrun(ov_json_get(db->data.domains, "/domain/id"));
    testrun(ov_json_get(db->data.domains, "/domain/key"));
    testrun(ov_json_get(db->data.domains, "/domain/projects"));
    testrun(ov_json_get(db->data.domains, "/domain/users"));
    testrun(1 == ov_json_object_count(db->data.domains));
    testrun(4 ==
            ov_json_object_count(ov_json_get(db->data.domains, "/domain")));
    testrun(0 == ov_json_object_count(
                     ov_json_get(db->data.domains, "/domain/projects")));
    testrun(2 == ov_json_object_count(
                     ov_json_get(db->data.domains, "/domain/users")));
    testrun(0 == ov_dict_count(db->index.projects));
    testrun(2 == ov_dict_count(db->index.users));
    testrun(0 == ov_dict_count(db->index.roles));
    testrun(0 == ov_dict_count(db->index.loops));

    testrun(ov_json_get(db->data.domains, "/domain/users/user1"));
    testrun(ov_json_get(db->data.domains, "/domain/users/user2"));
    testrun(ov_dict_get(db->index.users, "user1") ==
            ov_json_get(db->data.domains, "/domain/users/user1"));
    testrun(ov_dict_get(db->index.users, "user2") ==
            ov_json_get(db->data.domains, "/domain/users/user2"));

    // update domain roles

    out = ov_json_value_free(out);
    out = ov_json_object();

    itm = create_id_object("role1");
    testrun(ov_json_object_set(out, "role1", itm));
    itm = create_id_object("role2");
    testrun(ov_json_object_set(out, "role2", itm));
    itm = create_id_object("role3");
    testrun(ov_json_object_set(out, "role3", itm));

    testrun(ov_vocs_db_update_entity_key(
        db, OV_VOCS_DB_DOMAIN, "domain", OV_KEY_ROLES, out));

    // testrun(ov_vocs_db_dump(stdout, db));

    testrun(ov_json_get(db->data.domains, "/domain"));
    testrun(ov_json_get(db->data.domains, "/domain/id"));
    testrun(ov_json_get(db->data.domains, "/domain/key"));
    testrun(ov_json_get(db->data.domains, "/domain/projects"));
    testrun(ov_json_get(db->data.domains, "/domain/users"));
    testrun(ov_json_get(db->data.domains, "/domain/roles"));
    testrun(1 == ov_json_object_count(db->data.domains));
    testrun(5 ==
            ov_json_object_count(ov_json_get(db->data.domains, "/domain")));
    testrun(0 == ov_json_object_count(
                     ov_json_get(db->data.domains, "/domain/projects")));
    testrun(2 == ov_json_object_count(
                     ov_json_get(db->data.domains, "/domain/users")));
    testrun(3 == ov_json_object_count(
                     ov_json_get(db->data.domains, "/domain/roles")));
    testrun(0 == ov_dict_count(db->index.projects));
    testrun(2 == ov_dict_count(db->index.users));
    testrun(3 == ov_dict_count(db->index.roles));
    testrun(0 == ov_dict_count(db->index.loops));

    testrun(ov_json_get(db->data.domains, "/domain/roles/role1"));
    testrun(ov_json_get(db->data.domains, "/domain/roles/role2"));
    testrun(ov_json_get(db->data.domains, "/domain/roles/role3"));
    testrun(ov_dict_get(db->index.roles, "role1") ==
            ov_json_get(db->data.domains, "/domain/roles/role1"));
    testrun(ov_dict_get(db->index.roles, "role2") ==
            ov_json_get(db->data.domains, "/domain/roles/role2"));
    testrun(ov_dict_get(db->index.roles, "role3") ==
            ov_json_get(db->data.domains, "/domain/roles/role3"));

    // update domain loops

    out = ov_json_value_free(out);
    out = ov_json_object();

    itm = create_id_object("loop1");
    testrun(ov_json_object_set(out, "loop1", itm));
    itm = create_id_object("loop2");
    testrun(ov_json_object_set(out, "loop2", itm));
    itm = create_id_object("loop3");
    testrun(ov_json_object_set(out, "loop3", itm));
    itm = create_id_object("loop4");
    testrun(ov_json_object_set(out, "loop4", itm));

    testrun(ov_vocs_db_update_entity_key(
        db, OV_VOCS_DB_DOMAIN, "domain", OV_KEY_LOOPS, out));

    // testrun(ov_vocs_db_dump(stdout, db));

    testrun(ov_json_get(db->data.domains, "/domain"));
    testrun(ov_json_get(db->data.domains, "/domain/id"));
    testrun(ov_json_get(db->data.domains, "/domain/key"));
    testrun(ov_json_get(db->data.domains, "/domain/projects"));
    testrun(ov_json_get(db->data.domains, "/domain/users"));
    testrun(ov_json_get(db->data.domains, "/domain/roles"));
    testrun(ov_json_get(db->data.domains, "/domain/loops"));
    testrun(1 == ov_json_object_count(db->data.domains));
    testrun(6 ==
            ov_json_object_count(ov_json_get(db->data.domains, "/domain")));
    testrun(0 == ov_json_object_count(
                     ov_json_get(db->data.domains, "/domain/projects")));
    testrun(2 == ov_json_object_count(
                     ov_json_get(db->data.domains, "/domain/users")));
    testrun(3 == ov_json_object_count(
                     ov_json_get(db->data.domains, "/domain/roles")));
    testrun(4 == ov_json_object_count(
                     ov_json_get(db->data.domains, "/domain/loops")));
    testrun(0 == ov_dict_count(db->index.projects));
    testrun(2 == ov_dict_count(db->index.users));
    testrun(3 == ov_dict_count(db->index.roles));
    testrun(4 == ov_dict_count(db->index.loops));

    testrun(ov_json_get(db->data.domains, "/domain/loops/loop1"));
    testrun(ov_json_get(db->data.domains, "/domain/loops/loop2"));
    testrun(ov_json_get(db->data.domains, "/domain/loops/loop3"));
    testrun(ov_json_get(db->data.domains, "/domain/loops/loop4"));
    testrun(ov_dict_get(db->index.loops, "loop1") ==
            ov_json_get(db->data.domains, "/domain/loops/loop1"));
    testrun(ov_dict_get(db->index.loops, "loop2") ==
            ov_json_get(db->data.domains, "/domain/loops/loop2"));
    testrun(ov_dict_get(db->index.loops, "loop3") ==
            ov_json_get(db->data.domains, "/domain/loops/loop3"));
    testrun(ov_dict_get(db->index.loops, "loop4") ==
            ov_json_get(db->data.domains, "/domain/loops/loop4"));

    out = ov_json_value_free(out);
    out = ov_json_object();
    val = create_id_object("project");
    testrun(ov_json_object_set(out, "project", val));

    // entity unknown

    testrun(!ov_vocs_db_update_entity_key(
        db, OV_VOCS_DB_DOMAIN, "unknown", OV_KEY_PROJECT, out));

    // create project for testing

    testrun(ov_vocs_db_update_entity_key(
        db, OV_VOCS_DB_DOMAIN, "domain", OV_KEY_PROJECTS, out));

    // testrun(ov_vocs_db_dump(stdout, db));

    testrun(1 == ov_dict_count(db->index.domains));
    testrun(1 == ov_dict_count(db->index.projects));
    testrun(2 == ov_dict_count(db->index.users));
    testrun(3 == ov_dict_count(db->index.roles));
    testrun(4 == ov_dict_count(db->index.loops));

    // update some key in project

    out = ov_json_value_free(out);
    out = ov_json_true();

    testrun(ov_vocs_db_update_entity_key(
        db, OV_VOCS_DB_PROJECT, "project", "key", out));

    testrun(ov_json_get(db->data.domains, "/domain/projects"));
    testrun(ov_json_get(db->data.domains, "/domain/projects/project"));
    testrun(ov_json_get(db->data.domains, "/domain/projects/project/key"));
    testrun(ov_json_is_true(
        ov_json_get(db->data.domains, "/domain/projects/project/key")));
    testrun(!ov_json_get(db->data.domains, "/domain/projects/project/users"));
    testrun(!ov_json_get(db->data.domains, "/domain/projects/project/roles"));
    testrun(!ov_json_get(db->data.domains, "/domain/projects/project/loops"));

    // update project users

    out = ov_json_value_free(out);
    out = ov_json_object();

    itm = create_id_object("user3");
    testrun(ov_json_object_set(out, "user3", itm));
    itm = create_id_object("user4");
    testrun(ov_json_object_set(out, "user4", itm));

    testrun(ov_vocs_db_update_entity_key(
        db, OV_VOCS_DB_PROJECT, "project", OV_KEY_USERS, out));

    // testrun(ov_vocs_db_dump(stdout, db));

    testrun(1 == ov_dict_count(db->index.domains));
    testrun(1 == ov_dict_count(db->index.projects));
    testrun(4 == ov_dict_count(db->index.users));
    testrun(3 == ov_dict_count(db->index.roles));
    testrun(4 == ov_dict_count(db->index.loops));

    testrun(ov_json_get(db->data.domains, "/domain/projects/project/users"));
    testrun(
        ov_json_get(db->data.domains, "/domain/projects/project/users/user3"));
    testrun(
        ov_json_get(db->data.domains, "/domain/projects/project/users/user4"));

    testrun(ov_dict_get(db->index.users, "user1") ==
            ov_json_get(db->data.domains, "/domain/users/user1"));
    testrun(ov_dict_get(db->index.users, "user2") ==
            ov_json_get(db->data.domains, "/domain/users/user2"));
    testrun(
        ov_dict_get(db->index.users, "user3") ==
        ov_json_get(db->data.domains, "/domain/projects/project/users/user3"));
    testrun(
        ov_dict_get(db->index.users, "user4") ==
        ov_json_get(db->data.domains, "/domain/projects/project/users/user4"));

    // update project with existing users

    out = ov_json_value_free(out);
    out = ov_json_object();

    itm = create_id_object("user3");
    testrun(ov_json_object_set(out, "user3", itm));
    itm = create_id_object("user1");
    testrun(ov_json_object_set(out, "user1", itm));

    testrun(!ov_vocs_db_update_entity_key(
        db, OV_VOCS_DB_PROJECT, "project", OV_KEY_USERS, out));

    // testrun(ov_vocs_db_dump(stdout, db));

    testrun(1 == ov_dict_count(db->index.domains));
    testrun(1 == ov_dict_count(db->index.projects));
    testrun(4 == ov_dict_count(db->index.users));
    testrun(3 == ov_dict_count(db->index.roles));
    testrun(4 == ov_dict_count(db->index.loops));

    testrun(ov_json_get(db->data.domains, "/domain/projects/project/users"));
    testrun(
        ov_json_get(db->data.domains, "/domain/projects/project/users/user3"));
    testrun(
        ov_json_get(db->data.domains, "/domain/projects/project/users/user4"));

    testrun(ov_dict_get(db->index.users, "user1") ==
            ov_json_get(db->data.domains, "/domain/users/user1"));
    testrun(ov_dict_get(db->index.users, "user2") ==
            ov_json_get(db->data.domains, "/domain/users/user2"));
    testrun(
        ov_dict_get(db->index.users, "user3") ==
        ov_json_get(db->data.domains, "/domain/projects/project/users/user3"));
    testrun(
        ov_dict_get(db->index.users, "user4") ==
        ov_json_get(db->data.domains, "/domain/projects/project/users/user4"));

    // update project with project owned existing users

    out = ov_json_value_free(out);
    out = ov_json_object();

    itm = create_id_object("user3");
    testrun(ov_json_object_set(out, "user3", itm));

    testrun(ov_vocs_db_update_entity_key(
        db, OV_VOCS_DB_PROJECT, "project", OV_KEY_USERS, out));

    // testrun(ov_vocs_db_dump(stdout, db));

    testrun(1 == ov_dict_count(db->index.domains));
    testrun(1 == ov_dict_count(db->index.projects));
    testrun(3 == ov_dict_count(db->index.users));
    testrun(3 == ov_dict_count(db->index.roles));
    testrun(4 == ov_dict_count(db->index.loops));

    testrun(ov_json_get(db->data.domains, "/domain/projects/project/users"));
    testrun(
        ov_json_get(db->data.domains, "/domain/projects/project/users/user3"));
    testrun(
        !ov_json_get(db->data.domains, "/domain/projects/project/users/user4"));

    testrun(ov_dict_get(db->index.users, "user1") ==
            ov_json_get(db->data.domains, "/domain/users/user1"));
    testrun(ov_dict_get(db->index.users, "user2") ==
            ov_json_get(db->data.domains, "/domain/users/user2"));
    testrun(
        ov_dict_get(db->index.users, "user3") ==
        ov_json_get(db->data.domains, "/domain/projects/project/users/user3"));

    // update project roles

    out = ov_json_value_free(out);
    out = ov_json_object();

    itm = create_id_object("role4");
    testrun(ov_json_object_set(out, "role4", itm));
    itm = create_id_object("role5");
    testrun(ov_json_object_set(out, "role5", itm));

    testrun(ov_vocs_db_update_entity_key(
        db, OV_VOCS_DB_PROJECT, "project", OV_KEY_ROLES, out));

    // testrun(ov_vocs_db_dump(stdout, db));

    testrun(1 == ov_dict_count(db->index.domains));
    testrun(1 == ov_dict_count(db->index.projects));
    testrun(3 == ov_dict_count(db->index.users));
    testrun(5 == ov_dict_count(db->index.roles));
    testrun(4 == ov_dict_count(db->index.loops));

    testrun(ov_json_get(db->data.domains, "/domain/projects/project/roles"));
    testrun(
        ov_json_get(db->data.domains, "/domain/projects/project/roles/role4"));
    testrun(
        ov_json_get(db->data.domains, "/domain/projects/project/roles/role5"));

    testrun(ov_json_get(db->data.domains, "/domain/roles/role1"));
    testrun(ov_json_get(db->data.domains, "/domain/roles/role2"));
    testrun(ov_json_get(db->data.domains, "/domain/roles/role3"));
    testrun(ov_dict_get(db->index.roles, "role1") ==
            ov_json_get(db->data.domains, "/domain/roles/role1"));
    testrun(ov_dict_get(db->index.roles, "role2") ==
            ov_json_get(db->data.domains, "/domain/roles/role2"));
    testrun(ov_dict_get(db->index.roles, "role3") ==
            ov_json_get(db->data.domains, "/domain/roles/role3"));
    testrun(
        ov_dict_get(db->index.roles, "role4") ==
        ov_json_get(db->data.domains, "/domain/projects/project/roles/role4"));
    testrun(
        ov_dict_get(db->index.roles, "role5") ==
        ov_json_get(db->data.domains, "/domain/projects/project/roles/role5"));

    // update project with existing roles

    out = ov_json_value_free(out);
    out = ov_json_object();

    itm = create_id_object("role4");
    testrun(ov_json_object_set(out, "role4", itm));
    itm = create_id_object("role1");
    testrun(ov_json_object_set(out, "role1", itm));

    testrun(!ov_vocs_db_update_entity_key(
        db, OV_VOCS_DB_PROJECT, "project", OV_KEY_ROLES, out));

    // testrun(ov_vocs_db_dump(stdout, db));

    testrun(1 == ov_dict_count(db->index.domains));
    testrun(1 == ov_dict_count(db->index.projects));
    testrun(3 == ov_dict_count(db->index.users));
    testrun(5 == ov_dict_count(db->index.roles));
    testrun(4 == ov_dict_count(db->index.loops));

    testrun(ov_json_get(db->data.domains, "/domain/projects/project/roles"));
    testrun(
        ov_json_get(db->data.domains, "/domain/projects/project/roles/role4"));
    testrun(
        ov_json_get(db->data.domains, "/domain/projects/project/roles/role5"));

    testrun(ov_json_get(db->data.domains, "/domain/roles/role1"));
    testrun(ov_json_get(db->data.domains, "/domain/roles/role2"));
    testrun(ov_json_get(db->data.domains, "/domain/roles/role3"));
    testrun(ov_dict_get(db->index.roles, "role1") ==
            ov_json_get(db->data.domains, "/domain/roles/role1"));
    testrun(ov_dict_get(db->index.roles, "role2") ==
            ov_json_get(db->data.domains, "/domain/roles/role2"));
    testrun(ov_dict_get(db->index.roles, "role3") ==
            ov_json_get(db->data.domains, "/domain/roles/role3"));
    testrun(
        ov_dict_get(db->index.roles, "role4") ==
        ov_json_get(db->data.domains, "/domain/projects/project/roles/role4"));
    testrun(
        ov_dict_get(db->index.roles, "role5") ==
        ov_json_get(db->data.domains, "/domain/projects/project/roles/role5"));

    // update project with existing roles owned by project

    out = ov_json_value_free(out);
    out = ov_json_object();

    itm = create_id_object("role4");
    testrun(ov_json_object_set(out, "role4", itm));

    testrun(ov_vocs_db_update_entity_key(
        db, OV_VOCS_DB_PROJECT, "project", OV_KEY_ROLES, out));

    // testrun(ov_vocs_db_dump(stdout, db));

    testrun(1 == ov_dict_count(db->index.domains));
    testrun(1 == ov_dict_count(db->index.projects));
    testrun(3 == ov_dict_count(db->index.users));
    testrun(4 == ov_dict_count(db->index.roles));
    testrun(4 == ov_dict_count(db->index.loops));

    testrun(ov_json_get(db->data.domains, "/domain/projects/project/roles"));
    testrun(
        ov_json_get(db->data.domains, "/domain/projects/project/roles/role4"));
    testrun(
        !ov_json_get(db->data.domains, "/domain/projects/project/roles/role5"));

    testrun(ov_json_get(db->data.domains, "/domain/roles/role1"));
    testrun(ov_json_get(db->data.domains, "/domain/roles/role2"));
    testrun(ov_json_get(db->data.domains, "/domain/roles/role3"));
    testrun(ov_dict_get(db->index.roles, "role1") ==
            ov_json_get(db->data.domains, "/domain/roles/role1"));
    testrun(ov_dict_get(db->index.roles, "role2") ==
            ov_json_get(db->data.domains, "/domain/roles/role2"));
    testrun(ov_dict_get(db->index.roles, "role3") ==
            ov_json_get(db->data.domains, "/domain/roles/role3"));
    testrun(
        ov_dict_get(db->index.roles, "role4") ==
        ov_json_get(db->data.domains, "/domain/projects/project/roles/role4"));

    // update project loops

    out = ov_json_value_free(out);
    out = ov_json_object();

    itm = create_id_object("loop5");
    testrun(ov_json_object_set(out, "loop5", itm));
    itm = create_id_object("loop6");
    testrun(ov_json_object_set(out, "loop6", itm));

    testrun(ov_vocs_db_update_entity_key(
        db, OV_VOCS_DB_PROJECT, "project", OV_KEY_LOOPS, out));

    // testrun(ov_vocs_db_dump(stdout, db));

    testrun(1 == ov_dict_count(db->index.domains));
    testrun(1 == ov_dict_count(db->index.projects));
    testrun(3 == ov_dict_count(db->index.users));
    testrun(4 == ov_dict_count(db->index.roles));
    testrun(6 == ov_dict_count(db->index.loops));

    testrun(ov_json_get(db->data.domains, "/domain/projects/project/roles"));
    testrun(
        ov_json_get(db->data.domains, "/domain/projects/project/loops/loop5"));
    testrun(
        ov_json_get(db->data.domains, "/domain/projects/project/loops/loop6"));

    testrun(ov_json_get(db->data.domains, "/domain/loops/loop1"));
    testrun(ov_json_get(db->data.domains, "/domain/loops/loop2"));
    testrun(ov_json_get(db->data.domains, "/domain/loops/loop3"));
    testrun(ov_json_get(db->data.domains, "/domain/loops/loop4"));
    testrun(ov_dict_get(db->index.loops, "loop1") ==
            ov_json_get(db->data.domains, "/domain/loops/loop1"));
    testrun(ov_dict_get(db->index.loops, "loop2") ==
            ov_json_get(db->data.domains, "/domain/loops/loop2"));
    testrun(ov_dict_get(db->index.loops, "loop3") ==
            ov_json_get(db->data.domains, "/domain/loops/loop3"));
    testrun(ov_dict_get(db->index.loops, "loop4") ==
            ov_json_get(db->data.domains, "/domain/loops/loop4"));
    testrun(
        ov_dict_get(db->index.loops, "loop5") ==
        ov_json_get(db->data.domains, "/domain/projects/project/loops/loop5"));
    testrun(
        ov_dict_get(db->index.loops, "loop6") ==
        ov_json_get(db->data.domains, "/domain/projects/project/loops/loop6"));

    // update project with existing loops

    out = ov_json_value_free(out);
    out = ov_json_object();

    itm = create_id_object("loop1");
    testrun(ov_json_object_set(out, "loop1", itm));
    itm = create_id_object("loop6");
    testrun(ov_json_object_set(out, "loop6", itm));

    testrun(!ov_vocs_db_update_entity_key(
        db, OV_VOCS_DB_PROJECT, "project", OV_KEY_LOOPS, out));

    // testrun(ov_vocs_db_dump(stdout, db));

    testrun(1 == ov_dict_count(db->index.domains));
    testrun(1 == ov_dict_count(db->index.projects));
    testrun(3 == ov_dict_count(db->index.users));
    testrun(4 == ov_dict_count(db->index.roles));
    testrun(6 == ov_dict_count(db->index.loops));

    testrun(ov_json_get(db->data.domains, "/domain/projects/project/roles"));
    testrun(
        ov_json_get(db->data.domains, "/domain/projects/project/loops/loop5"));
    testrun(
        ov_json_get(db->data.domains, "/domain/projects/project/loops/loop6"));

    testrun(ov_json_get(db->data.domains, "/domain/loops/loop1"));
    testrun(ov_json_get(db->data.domains, "/domain/loops/loop2"));
    testrun(ov_json_get(db->data.domains, "/domain/loops/loop3"));
    testrun(ov_json_get(db->data.domains, "/domain/loops/loop4"));
    testrun(ov_dict_get(db->index.loops, "loop1") ==
            ov_json_get(db->data.domains, "/domain/loops/loop1"));
    testrun(ov_dict_get(db->index.loops, "loop2") ==
            ov_json_get(db->data.domains, "/domain/loops/loop2"));
    testrun(ov_dict_get(db->index.loops, "loop3") ==
            ov_json_get(db->data.domains, "/domain/loops/loop3"));
    testrun(ov_dict_get(db->index.loops, "loop4") ==
            ov_json_get(db->data.domains, "/domain/loops/loop4"));
    testrun(
        ov_dict_get(db->index.loops, "loop5") ==
        ov_json_get(db->data.domains, "/domain/projects/project/loops/loop5"));
    testrun(
        ov_dict_get(db->index.loops, "loop6") ==
        ov_json_get(db->data.domains, "/domain/projects/project/loops/loop6"));

    // update project with existing loops owned by project

    out = ov_json_value_free(out);
    out = ov_json_object();

    itm = create_id_object("loop5");
    testrun(ov_json_object_set(out, "loop5", itm));
    itm = create_id_object("loop6");
    testrun(ov_json_object_set(out, "loop6", itm));

    testrun(ov_vocs_db_update_entity_key(
        db, OV_VOCS_DB_PROJECT, "project", OV_KEY_LOOPS, out));

    // testrun(ov_vocs_db_dump(stdout, db));

    testrun(1 == ov_dict_count(db->index.domains));
    testrun(1 == ov_dict_count(db->index.projects));
    testrun(3 == ov_dict_count(db->index.users));
    testrun(4 == ov_dict_count(db->index.roles));
    testrun(6 == ov_dict_count(db->index.loops));

    testrun(ov_json_get(db->data.domains, "/domain/projects/project/roles"));
    testrun(
        ov_json_get(db->data.domains, "/domain/projects/project/loops/loop5"));
    testrun(
        ov_json_get(db->data.domains, "/domain/projects/project/loops/loop6"));

    testrun(ov_json_get(db->data.domains, "/domain/loops/loop1"));
    testrun(ov_json_get(db->data.domains, "/domain/loops/loop2"));
    testrun(ov_json_get(db->data.domains, "/domain/loops/loop3"));
    testrun(ov_json_get(db->data.domains, "/domain/loops/loop4"));
    testrun(ov_dict_get(db->index.loops, "loop1") ==
            ov_json_get(db->data.domains, "/domain/loops/loop1"));
    testrun(ov_dict_get(db->index.loops, "loop2") ==
            ov_json_get(db->data.domains, "/domain/loops/loop2"));
    testrun(ov_dict_get(db->index.loops, "loop3") ==
            ov_json_get(db->data.domains, "/domain/loops/loop3"));
    testrun(ov_dict_get(db->index.loops, "loop4") ==
            ov_json_get(db->data.domains, "/domain/loops/loop4"));
    testrun(
        ov_dict_get(db->index.loops, "loop5") ==
        ov_json_get(db->data.domains, "/domain/projects/project/loops/loop5"));
    testrun(
        ov_dict_get(db->index.loops, "loop6") ==
        ov_json_get(db->data.domains, "/domain/projects/project/loops/loop6"));

    // update user entity

    out = ov_json_value_free(out);
    out = ov_json_object();

    itm = create_id_object("something");
    testrun(ov_json_object_set(out, "something", itm));
    itm = create_id_object("name");
    testrun(ov_json_object_set(out, "username", itm));

    testrun(
        ov_vocs_db_update_entity_key(db, OV_VOCS_DB_USER, "user1", "key", out));

    // testrun(ov_vocs_db_dump(stdout, db));

    testrun(ov_json_get(db->data.domains, "/domain/users/user1/key"));
    testrun(
        ov_json_get(db->data.domains, "/domain/users/user1/key/something/id"));
    testrun(
        ov_json_get(db->data.domains, "/domain/users/user1/key/username/id"));

    // update role entity

    out = ov_json_value_free(out);
    out = ov_json_object();

    itm = create_id_object("something");
    testrun(ov_json_object_set(out, "something", itm));
    itm = create_id_object("name");
    testrun(ov_json_object_set(out, "username", itm));

    testrun(
        ov_vocs_db_update_entity_key(db, OV_VOCS_DB_ROLE, "role1", "key", out));

    // testrun(ov_vocs_db_dump(stdout, db));

    testrun(ov_json_get(db->data.domains, "/domain/roles/role1/key"));
    testrun(
        ov_json_get(db->data.domains, "/domain/roles/role1/key/something/id"));
    testrun(
        ov_json_get(db->data.domains, "/domain/roles/role1/key/username/id"));

    // update loop entity

    out = ov_json_value_free(out);
    out = ov_json_object();

    itm = create_id_object("something");
    testrun(ov_json_object_set(out, "something", itm));
    itm = create_id_object("name");
    testrun(ov_json_object_set(out, "username", itm));

    testrun(
        ov_vocs_db_update_entity_key(db, OV_VOCS_DB_LOOP, "loop1", "key", out));

    // testrun(ov_vocs_db_dump(stdout, db));

    testrun(ov_json_get(db->data.domains, "/domain/loops/loop1/key"));
    testrun(
        ov_json_get(db->data.domains, "/domain/loops/loop1/key/something/id"));
    testrun(
        ov_json_get(db->data.domains, "/domain/loops/loop1/key/username/id"));

    out = ov_json_value_free(out);

    testrun(1 == ov_dict_count(db->index.domains));
    testrun(1 == ov_dict_count(db->index.projects));
    testrun(3 == ov_dict_count(db->index.users));
    testrun(4 == ov_dict_count(db->index.roles));
    testrun(6 == ov_dict_count(db->index.loops));

    // test delete

    testrun(ov_json_get(db->data.domains, "/domain/loops/loop1"));
    testrun(ov_dict_get(db->index.loops, "loop1") ==
            ov_json_get(db->data.domains, "/domain/loops/loop1"));

    testrun(ov_vocs_db_delete_entity(db, OV_VOCS_DB_LOOP, "loop1"));

    testrun(!ov_json_get(db->data.domains, "/domain/loops/loop1"));
    testrun(ov_dict_get(db->index.loops, "loop1") == NULL);

    testrun(1 == ov_dict_count(db->index.domains));
    testrun(1 == ov_dict_count(db->index.projects));
    testrun(3 == ov_dict_count(db->index.users));
    testrun(4 == ov_dict_count(db->index.roles));
    testrun(5 == ov_dict_count(db->index.loops));

    testrun(
        ov_json_get(db->data.domains, "/domain/projects/project/loops/loop5"));

    testrun(ov_vocs_db_delete_entity(db, OV_VOCS_DB_LOOP, "loop5"));

    testrun(
        !ov_json_get(db->data.domains, "/domain/projects/project/loops/loop1"));
    testrun(ov_dict_get(db->index.loops, "loop5") == NULL);

    testrun(1 == ov_dict_count(db->index.domains));
    testrun(1 == ov_dict_count(db->index.projects));
    testrun(3 == ov_dict_count(db->index.users));
    testrun(4 == ov_dict_count(db->index.roles));
    testrun(4 == ov_dict_count(db->index.loops));

    testrun(ov_json_get(db->data.domains, "/domain/roles/role1"));
    testrun(ov_dict_get(db->index.roles, "role1") ==
            ov_json_get(db->data.domains, "/domain/roles/role1"));

    testrun(ov_vocs_db_delete_entity(db, OV_VOCS_DB_ROLE, "role1"));

    testrun(!ov_json_get(db->data.domains, "/domain/roles/role1"));
    testrun(ov_dict_get(db->index.loops, "role1") == NULL);

    testrun(1 == ov_dict_count(db->index.domains));
    testrun(1 == ov_dict_count(db->index.projects));
    testrun(3 == ov_dict_count(db->index.users));
    testrun(3 == ov_dict_count(db->index.roles));
    testrun(4 == ov_dict_count(db->index.loops));

    testrun(
        ov_json_get(db->data.domains, "/domain/projects/project/roles/role4"));

    testrun(ov_vocs_db_delete_entity(db, OV_VOCS_DB_ROLE, "role4"));

    testrun(
        !ov_json_get(db->data.domains, "/domain/projects/project/roles/role4"));
    testrun(ov_dict_get(db->index.loops, "role4") == NULL);

    testrun(1 == ov_dict_count(db->index.domains));
    testrun(1 == ov_dict_count(db->index.projects));
    testrun(3 == ov_dict_count(db->index.users));
    testrun(2 == ov_dict_count(db->index.roles));
    testrun(4 == ov_dict_count(db->index.loops));

    testrun(ov_json_get(db->data.domains, "/domain/users/user1"));
    testrun(ov_dict_get(db->index.users, "user1") ==
            ov_json_get(db->data.domains, "/domain/users/user1"));

    testrun(ov_vocs_db_delete_entity(db, OV_VOCS_DB_USER, "user1"));

    testrun(!ov_json_get(db->data.domains, "/domain/users/user1"));
    testrun(ov_dict_get(db->index.loops, "user1") == NULL);

    testrun(1 == ov_dict_count(db->index.domains));
    testrun(1 == ov_dict_count(db->index.projects));
    testrun(2 == ov_dict_count(db->index.users));
    testrun(2 == ov_dict_count(db->index.roles));
    testrun(4 == ov_dict_count(db->index.loops));

    testrun(ov_json_get(db->data.domains, "/domain/projects/project"));
    testrun(ov_dict_get(db->index.projects, "project") ==
            ov_json_get(db->data.domains, "/domain/projects/project"));

    testrun(ov_vocs_db_delete_entity(db, OV_VOCS_DB_PROJECT, "project"));

    testrun(!ov_json_get(db->data.domains, "/domain/projects/project"));
    testrun(ov_dict_get(db->index.loops, "project") == NULL);

    // testrun(ov_vocs_db_dump(stdout, db));

    testrun(1 == ov_dict_count(db->index.domains));
    testrun(0 == ov_dict_count(db->index.projects));
    testrun(1 == ov_dict_count(db->index.users));
    testrun(2 == ov_dict_count(db->index.roles));
    testrun(3 == ov_dict_count(db->index.loops));

    testrun(ov_vocs_db_delete_entity(db, OV_VOCS_DB_DOMAIN, "domain"));

    // testrun(ov_vocs_db_dump(stdout, db));

    testrun(0 == ov_dict_count(db->index.domains));
    testrun(0 == ov_dict_count(db->index.projects));
    testrun(0 == ov_dict_count(db->index.users));
    testrun(0 == ov_dict_count(db->index.roles));
    testrun(0 == ov_dict_count(db->index.loops));

    testrun(NULL == ov_vocs_db_free(db));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_vocs_db_delete_entity_key() {

    ov_vocs_db *db = ov_vocs_db_create((ov_vocs_db_config){0});
    testrun(db);

    testrun(create_test_db(db));

    testrun(3 == ov_dict_count(db->index.domains));
    testrun(3 == ov_dict_count(db->index.projects));
    testrun(12 == ov_dict_count(db->index.users));
    testrun(9 == ov_dict_count(db->index.roles));
    testrun(9 == ov_dict_count(db->index.loops));

    testrun(
        !ov_vocs_db_delete_entity_key(db, OV_VOCS_DB_USER, "unknown", "key"));

    testrun(ov_json_get(
        db->data.domains, "/domain3/projects/project3/users/user31/id"));
    testrun(ov_dict_get(db->index.users, "user31"));

    testrun(!ov_vocs_db_delete_entity_key(db, OV_VOCS_DB_USER, "user31", "id"));

    testrun(
        ov_vocs_db_delete_entity_key(db, OV_VOCS_DB_USER, "user31", "unknown"));

    testrun(ov_json_get(
        db->data.domains, "/domain3/projects/project3/users/user31/id"));
    testrun(ov_dict_get(db->index.users, "user31"));

    testrun(ov_json_get(
        db->data.domains, "/domain3/projects/project3/roles/role31/id"));
    testrun(ov_dict_get(db->index.roles, "role31"));

    testrun(!ov_vocs_db_delete_entity_key(db, OV_VOCS_DB_ROLE, "role31", "id"));

    testrun(
        ov_vocs_db_delete_entity_key(db, OV_VOCS_DB_ROLE, "role31", "unknown"));

    testrun(ov_json_get(
        db->data.domains, "/domain3/projects/project3/roles/role31/id"));
    testrun(ov_dict_get(db->index.roles, "role31"));

    testrun(ov_json_get(
        db->data.domains, "/domain3/projects/project3/loops/loop31/id"));

    testrun(!ov_vocs_db_delete_entity_key(db, OV_VOCS_DB_LOOP, "loop31", "id"));

    testrun(
        ov_vocs_db_delete_entity_key(db, OV_VOCS_DB_LOOP, "loop31", "unknown"));

    testrun(ov_json_get(
        db->data.domains, "/domain3/projects/project3/loops/loop31/id"));

    testrun(ov_json_get(db->data.domains, "/domain3/projects/project3/id"));

    testrun(!ov_vocs_db_delete_entity_key(
        db, OV_VOCS_DB_PROJECT, "project3", "id"));

    testrun(ov_vocs_db_delete_entity_key(
        db, OV_VOCS_DB_PROJECT, "project3", "unknown"));

    testrun(ov_json_get(db->data.domains, "/domain3/projects/project3/id"));

    testrun(3 == ov_dict_count(db->index.domains));
    testrun(3 == ov_dict_count(db->index.projects));
    testrun(12 == ov_dict_count(db->index.users));
    testrun(9 == ov_dict_count(db->index.roles));
    testrun(9 == ov_dict_count(db->index.loops));

    testrun(ov_vocs_db_delete_entity_key(
        db, OV_VOCS_DB_PROJECT, "project3", "users"));

    testrun(!ov_json_get(db->data.domains, "/domain3/projects/project3/users"));

    testrun(3 == ov_dict_count(db->index.domains));
    testrun(3 == ov_dict_count(db->index.projects));
    testrun(9 == ov_dict_count(db->index.users));
    testrun(9 == ov_dict_count(db->index.roles));
    testrun(9 == ov_dict_count(db->index.loops));

    testrun(ov_vocs_db_delete_entity_key(
        db, OV_VOCS_DB_PROJECT, "project3", "roles"));

    testrun(!ov_json_get(db->data.domains, "/domain3/projects/project3/roles"));

    testrun(3 == ov_dict_count(db->index.domains));
    testrun(3 == ov_dict_count(db->index.projects));
    testrun(9 == ov_dict_count(db->index.users));
    testrun(6 == ov_dict_count(db->index.roles));
    testrun(9 == ov_dict_count(db->index.loops));

    testrun(ov_vocs_db_delete_entity_key(
        db, OV_VOCS_DB_PROJECT, "project3", "loops"));

    testrun(!ov_json_get(db->data.domains, "/domain3/projects/project3/loops"));

    testrun(3 == ov_dict_count(db->index.domains));
    testrun(3 == ov_dict_count(db->index.projects));
    testrun(9 == ov_dict_count(db->index.users));
    testrun(6 == ov_dict_count(db->index.roles));
    testrun(6 == ov_dict_count(db->index.loops));

    testrun(ov_vocs_db_delete_entity_key(
        db, OV_VOCS_DB_DOMAIN, "domain3", "projects"));

    testrun(!ov_json_get(db->data.domains, "/domain3/projects"));

    testrun(3 == ov_dict_count(db->index.domains));
    testrun(2 == ov_dict_count(db->index.projects));
    testrun(9 == ov_dict_count(db->index.users));
    testrun(6 == ov_dict_count(db->index.roles));
    testrun(6 == ov_dict_count(db->index.loops));

    testrun(ov_vocs_db_delete_entity_key(
        db, OV_VOCS_DB_DOMAIN, "domain2", "projects"));

    testrun(!ov_json_get(db->data.domains, "/domain2/projects"));

    testrun(3 == ov_dict_count(db->index.domains));
    testrun(1 == ov_dict_count(db->index.projects));
    testrun(6 == ov_dict_count(db->index.users));
    testrun(3 == ov_dict_count(db->index.roles));
    testrun(3 == ov_dict_count(db->index.loops));

    testrun(NULL == ov_vocs_db_free(db));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_vocs_db_delete_entity() {

    ov_vocs_db *db = ov_vocs_db_create((ov_vocs_db_config){0});
    testrun(db);

    testrun(create_test_db(db));

    testrun(3 == ov_dict_count(db->index.domains));
    testrun(3 == ov_dict_count(db->index.projects));
    testrun(12 == ov_dict_count(db->index.users));
    testrun(9 == ov_dict_count(db->index.roles));
    testrun(9 == ov_dict_count(db->index.loops));

    testrun(ov_vocs_db_delete_entity(db, OV_VOCS_DB_DOMAIN, "domain3"));

    testrun(2 == ov_dict_count(db->index.domains));
    testrun(2 == ov_dict_count(db->index.projects));
    testrun(8 == ov_dict_count(db->index.users));
    testrun(6 == ov_dict_count(db->index.roles));
    testrun(6 == ov_dict_count(db->index.loops));

    // non existing entries

    testrun(ov_vocs_db_delete_entity(db, OV_VOCS_DB_DOMAIN, "domain3"));

    testrun(ov_vocs_db_delete_entity(db, OV_VOCS_DB_PROJECT, "domain3"));

    testrun(ov_vocs_db_delete_entity(db, OV_VOCS_DB_LOOP, "domain3"));

    testrun(ov_vocs_db_delete_entity(db, OV_VOCS_DB_ROLE, "domain3"));

    testrun(ov_vocs_db_delete_entity(db, OV_VOCS_DB_USER, "domain3"));

    testrun(2 == ov_dict_count(db->index.domains));
    testrun(2 == ov_dict_count(db->index.projects));
    testrun(8 == ov_dict_count(db->index.users));
    testrun(6 == ov_dict_count(db->index.roles));
    testrun(6 == ov_dict_count(db->index.loops));

    testrun(ov_vocs_db_delete_entity(db, OV_VOCS_DB_PROJECT, "project2"));

    testrun(2 == ov_dict_count(db->index.domains));
    testrun(1 == ov_dict_count(db->index.projects));
    testrun(5 == ov_dict_count(db->index.users));
    testrun(3 == ov_dict_count(db->index.roles));
    testrun(3 == ov_dict_count(db->index.loops));

    testrun(ov_vocs_db_delete_entity(db, OV_VOCS_DB_LOOP, "loop13"));

    testrun(2 == ov_dict_count(db->index.domains));
    testrun(1 == ov_dict_count(db->index.projects));
    testrun(5 == ov_dict_count(db->index.users));
    testrun(3 == ov_dict_count(db->index.roles));
    testrun(2 == ov_dict_count(db->index.loops));

    testrun(ov_vocs_db_delete_entity(db, OV_VOCS_DB_ROLE, "role13"));

    testrun(2 == ov_dict_count(db->index.domains));
    testrun(1 == ov_dict_count(db->index.projects));
    testrun(5 == ov_dict_count(db->index.users));
    testrun(2 == ov_dict_count(db->index.roles));
    testrun(2 == ov_dict_count(db->index.loops));

    testrun(ov_vocs_db_delete_entity(db, OV_VOCS_DB_USER, "user13"));

    testrun(2 == ov_dict_count(db->index.domains));
    testrun(1 == ov_dict_count(db->index.projects));
    testrun(4 == ov_dict_count(db->index.users));
    testrun(2 == ov_dict_count(db->index.roles));
    testrun(2 == ov_dict_count(db->index.loops));

    // testrun(ov_vocs_db_dump(stdout, db));

    // verify data state of test db
    testrun(2 == ov_json_object_count(db->data.domains));
    testrun(ov_json_get(db->data.domains, "/domain1"));
    testrun(ov_json_get(db->data.domains, "/domain2"));
    testrun(3 ==
            ov_json_object_count(ov_json_get(db->data.domains, "/domain1")));
    testrun(ov_json_get(db->data.domains, "/domain1/id"));
    testrun(ov_json_get(db->data.domains, "/domain1/projects"));
    testrun(3 ==
            ov_json_object_count(ov_json_get(db->data.domains, "/domain2")));
    testrun(ov_json_get(db->data.domains, "/domain2/id"));
    testrun(ov_json_get(db->data.domains, "/domain2/projects"));
    testrun(0 == ov_json_object_count(
                     ov_json_get(db->data.domains, "/domain2/projects")));
    testrun(1 == ov_json_object_count(
                     ov_json_get(db->data.domains, "/domain1/projects")));
    testrun(4 == ov_json_object_count(ov_json_get(
                     db->data.domains, "/domain1/projects/project1")));
    testrun(ov_json_get(db->data.domains, "/domain1/projects/project1/id"));
    testrun(ov_json_get(db->data.domains, "/domain1/projects/project1/users"));
    testrun(ov_json_get(db->data.domains, "/domain1/projects/project1/roles"));
    testrun(ov_json_get(db->data.domains, "/domain1/projects/project1/loops"));
    testrun(ov_json_get(
        db->data.domains, "/domain1/projects/project1/users/user11"));
    testrun(ov_json_get(
        db->data.domains, "/domain1/projects/project1/users/user12"));
    testrun(ov_json_get(
        db->data.domains, "/domain1/projects/project1/roles/role11"));
    testrun(ov_json_get(
        db->data.domains, "/domain1/projects/project1/roles/role12"));
    testrun(ov_json_get(
        db->data.domains, "/domain1/projects/project1/loops/loop11"));
    testrun(ov_json_get(
        db->data.domains, "/domain1/projects/project1/loops/loop12"));

    testrun(NULL == ov_vocs_db_free(db));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_vocs_db_verify_entity_item() {

    ov_vocs_db *db = ov_vocs_db_create((ov_vocs_db_config){0});
    testrun(db);

    testrun(create_test_db(db));

    testrun(3 == ov_dict_count(db->index.domains));
    testrun(3 == ov_dict_count(db->index.projects));
    testrun(12 == ov_dict_count(db->index.users));
    testrun(9 == ov_dict_count(db->index.roles));
    testrun(9 == ov_dict_count(db->index.loops));

    // user MUST not contain a password

    ov_json_value *out = ov_json_object();
    ov_json_value *val = NULL;
    ov_json_value *par = NULL;
    ov_json_value *err = NULL;

    testrun(ov_vocs_db_verify_entity_item(
        db, OV_VOCS_DB_USER, "unknown", out, &err));

    testrun(!err);

    testrun(ov_vocs_db_verify_entity_item(
        db, OV_VOCS_DB_USER, "user11", out, &err));

    val = ov_json_string("password");
    testrun(ov_json_object_set(out, OV_KEY_PASSWORD, val));

    testrun(!ov_vocs_db_verify_entity_item(
        db, OV_VOCS_DB_USER, "user11", out, &err));

    testrun(err);
    err = ov_json_value_free(err);
    out = ov_json_value_free(out);

    // role MAY contain everything

    out = ov_json_object();

    testrun(ov_vocs_db_verify_entity_item(
        db, OV_VOCS_DB_USER, "unknown", out, &err));

    testrun(!err);

    testrun(ov_vocs_db_verify_entity_item(
        db, OV_VOCS_DB_USER, "role11", out, &err));

    testrun(!err);

    // loop MAY contain everything

    testrun(ov_vocs_db_verify_entity_item(
        db, OV_VOCS_DB_LOOP, "unknown", out, &err));

    testrun(!err);

    testrun(ov_vocs_db_verify_entity_item(
        db, OV_VOCS_DB_LOOP, "loop11", out, &err));

    testrun(!err);

    // check project

    err = ov_json_value_free(err);
    out = ov_json_value_free(out);

    out = ov_json_object();

    testrun(ov_vocs_db_verify_entity_item(
        db, OV_VOCS_DB_PROJECT, "unknown", out, &err));

    testrun(!err);

    testrun(ov_vocs_db_verify_entity_item(
        db, OV_VOCS_DB_PROJECT, "project1", out, &err));

    testrun(!err);

    // project with non existing entries

    par = ov_json_object();
    ov_json_object_set(out, OV_KEY_USERS, par);
    val = create_id_object("unknown_user");
    ov_json_object_set(par, "unknown_user", val);

    testrun(ov_vocs_db_verify_entity_item(
        db, OV_VOCS_DB_PROJECT, "unknown", out, &err));

    testrun(!err);

    testrun(ov_vocs_db_verify_entity_item(
        db, OV_VOCS_DB_PROJECT, "project1", out, &err));

    testrun(!err);

    par = ov_json_object();
    ov_json_object_set(out, OV_KEY_ROLES, par);
    val = create_id_object("unknown_role");
    ov_json_object_set(par, "unknown_role", val);

    testrun(ov_vocs_db_verify_entity_item(
        db, OV_VOCS_DB_PROJECT, "unknown", out, &err));

    testrun(!err);

    testrun(ov_vocs_db_verify_entity_item(
        db, OV_VOCS_DB_PROJECT, "project1", out, &err));

    testrun(!err);

    par = ov_json_object();
    ov_json_object_set(out, OV_KEY_LOOPS, par);
    val = create_id_object("unknown_loop");
    ov_json_object_set(par, "unknown_loop", val);

    testrun(ov_vocs_db_verify_entity_item(
        db, OV_VOCS_DB_PROJECT, "unknown", out, &err));

    testrun(!err);

    testrun(ov_vocs_db_verify_entity_item(
        db, OV_VOCS_DB_PROJECT, "project1", out, &err));

    testrun(!err);

    // check with loop owned by project 1

    val = create_id_object("loop11");
    ov_json_object_set(par, "loop11", val);

    testrun(!ov_vocs_db_verify_entity_item(
        db, OV_VOCS_DB_PROJECT, "unknown", out, &err));

    testrun(err);
    err = ov_json_value_free(err);

    testrun(ov_vocs_db_verify_entity_item(
        db, OV_VOCS_DB_PROJECT, "project1", out, &err));

    testrun(!err);

    testrun(!ov_vocs_db_verify_entity_item(
        db, OV_VOCS_DB_DOMAIN, "unknown", out, &err));

    testrun(err);
    err = ov_json_value_free(err);

    // check with role owned by project 1

    ov_json_object_del(par, "loop11");
    par = ov_json_object_get(out, OV_KEY_ROLES);

    val = create_id_object("role11");
    ov_json_object_set(par, "role11", val);

    testrun(!ov_vocs_db_verify_entity_item(
        db, OV_VOCS_DB_PROJECT, "unknown", out, &err));

    testrun(err);
    err = ov_json_value_free(err);

    testrun(ov_vocs_db_verify_entity_item(
        db, OV_VOCS_DB_PROJECT, "project1", out, &err));

    testrun(!err);

    testrun(!ov_vocs_db_verify_entity_item(
        db, OV_VOCS_DB_DOMAIN, "unknown", out, &err));

    testrun(err);
    err = ov_json_value_free(err);

    // check with user owned by project 1

    ov_json_object_del(par, "role11");
    par = ov_json_object_get(out, OV_KEY_USERS);

    val = create_id_object("user11");
    ov_json_object_set(par, "user11", val);

    testrun(!ov_vocs_db_verify_entity_item(
        db, OV_VOCS_DB_PROJECT, "unknown", out, &err));

    testrun(err);
    err = ov_json_value_free(err);

    testrun(ov_vocs_db_verify_entity_item(
        db, OV_VOCS_DB_PROJECT, "project1", out, &err));

    testrun(!err);

    testrun(!ov_vocs_db_verify_entity_item(
        db, OV_VOCS_DB_DOMAIN, "unknown", out, &err));

    testrun(err);
    err = ov_json_value_free(err);

    ov_json_object_del(par, "user11");

    // unset all known items, verify unknown only content

    testrun(ov_vocs_db_verify_entity_item(
        db, OV_VOCS_DB_PROJECT, "unknown", out, &err));

    testrun(!err);

    testrun(ov_vocs_db_verify_entity_item(
        db, OV_VOCS_DB_PROJECT, "project1", out, &err));

    testrun(!err);

    testrun(ov_vocs_db_verify_entity_item(
        db, OV_VOCS_DB_DOMAIN, "unknown", out, &err));

    testrun(!err);

    testrun(ov_vocs_db_verify_entity_item(
        db, OV_VOCS_DB_DOMAIN, "domain1", out, &err));

    testrun(!err);

    val = ov_json_object();
    ov_json_object_set(out, OV_KEY_PROJECTS, val);
    ov_json_value *project = val;
    val = NULL;
    ov_json_object_copy((void **)&val, out);
    ov_json_object_set(project, "project1", val);
    par = val;

    testrun(ov_vocs_db_verify_entity_item(
        db, OV_VOCS_DB_DOMAIN, "unknown", out, &err));

    testrun(!err);

    testrun(ov_vocs_db_verify_entity_item(
        db, OV_VOCS_DB_DOMAIN, "domain1", out, &err));

    testrun(!err);

    // failure in project

    val = create_id_object("user31");
    ov_json_object_set(
        (ov_json_value *)ov_json_get(par, "/" OV_KEY_USERS), "user31", val);

    testrun(!ov_vocs_db_verify_entity_item(
        db, OV_VOCS_DB_DOMAIN, "unknown", out, &err));

    testrun(err);
    err = ov_json_value_free(err);

    testrun(!ov_vocs_db_verify_entity_item(
        db, OV_VOCS_DB_DOMAIN, "domain1", out, &err));

    testrun(err);
    err = ov_json_value_free(err);

    ov_json_object_del(
        (ov_json_value *)ov_json_get(par, "/" OV_KEY_USERS), "user31");

    testrun(ov_vocs_db_verify_entity_item(
        db, OV_VOCS_DB_DOMAIN, "unknown", out, &err));

    testrun(!err);

    testrun(ov_vocs_db_verify_entity_item(
        db, OV_VOCS_DB_DOMAIN, "domain1", out, &err));

    testrun(!err);

    val = create_id_object("role31");
    ov_json_object_set(
        (ov_json_value *)ov_json_get(par, "/" OV_KEY_ROLES), "role31", val);

    testrun(!ov_vocs_db_verify_entity_item(
        db, OV_VOCS_DB_DOMAIN, "unknown", out, &err));

    testrun(err);
    err = ov_json_value_free(err);

    testrun(!ov_vocs_db_verify_entity_item(
        db, OV_VOCS_DB_DOMAIN, "domain1", out, &err));

    testrun(err);
    err = ov_json_value_free(err);

    ov_json_object_del(
        (ov_json_value *)ov_json_get(par, "/" OV_KEY_ROLES), "role31");

    val = create_id_object("loop31");
    ov_json_object_set(
        (ov_json_value *)ov_json_get(par, "/" OV_KEY_LOOPS), "loop31", val);

    testrun(!ov_vocs_db_verify_entity_item(
        db, OV_VOCS_DB_DOMAIN, "unknown", out, &err));

    testrun(err);
    err = ov_json_value_free(err);

    testrun(!ov_vocs_db_verify_entity_item(
        db, OV_VOCS_DB_DOMAIN, "domain1", out, &err));

    testrun(err);
    err = ov_json_value_free(err);

    ov_json_object_del(
        (ov_json_value *)ov_json_get(par, "/" OV_KEY_LOOPS), "loop31");

    testrun(ov_vocs_db_verify_entity_item(
        db, OV_VOCS_DB_DOMAIN, "unknown", out, &err));

    testrun(!err);

    testrun(ov_vocs_db_verify_entity_item(
        db, OV_VOCS_DB_DOMAIN, "domain1", out, &err));

    testrun(!err);

    // loop owned by project of domain1

    par = (ov_json_value *)ov_json_get(out, "/" OV_KEY_LOOPS);
    val = create_id_object("loop11");
    ov_json_object_set(par, "loop11", val);

    testrun(!ov_vocs_db_verify_entity_item(
        db, OV_VOCS_DB_DOMAIN, "unknown", out, &err));

    testrun(err);
    err = ov_json_value_free(err);

    testrun(!ov_vocs_db_verify_entity_item(
        db, OV_VOCS_DB_DOMAIN, "domain1", out, &err));

    testrun(err);
    err = ov_json_value_free(err);

    ov_json_object_del(par, "loop11");

    val = create_id_object("loop1");
    ov_json_object_set(par, "loop1", val);

    testrun(ov_vocs_db_verify_entity_item(
        db, OV_VOCS_DB_DOMAIN, "unknown", out, &err));

    testrun(!err);

    testrun(ov_vocs_db_verify_entity_item(
        db, OV_VOCS_DB_DOMAIN, "domain1", out, &err));

    testrun(!err);

    out = ov_json_value_free(out);

    testrun(NULL == ov_vocs_db_free(db));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

struct userdata {

    ov_json_value *msg;
};

/*----------------------------------------------------------------------------*/

static void dummy_process_trigger(void *userdata, ov_json_value *event) {

    struct userdata *data = (struct userdata *)userdata;
    data->msg = ov_json_value_free(data->msg);
    data->msg = event;

    return;
}

/*----------------------------------------------------------------------------*/

int test_ov_vocs_db_update_entity_item() {

    struct userdata userdata = (struct userdata){0};

    ov_event_trigger *trigger =
        ov_event_trigger_create((ov_event_trigger_config){0});

    testrun(ov_event_trigger_register_listener(
        trigger,
        "VOCS",
        (ov_event_trigger_data){
            .userdata = &userdata, .process = dummy_process_trigger}));

    ov_vocs_db *db = ov_vocs_db_create((ov_vocs_db_config){.trigger = trigger});
    testrun(db);

    testrun(create_test_db(db));

    testrun(3 == ov_dict_count(db->index.domains));
    testrun(3 == ov_dict_count(db->index.projects));
    testrun(12 == ov_dict_count(db->index.users));
    testrun(9 == ov_dict_count(db->index.roles));
    testrun(9 == ov_dict_count(db->index.loops));

    ov_json_value *out = ov_json_object();
    ov_json_value const *val = NULL;
    ov_json_value *par = NULL;
    ov_json_value *err = NULL;

    // user MUST not contain a password

    // user not existing
    testrun(!ov_vocs_db_update_entity_item(
        db, OV_VOCS_DB_USER, "unknown", out, &err));

    testrun(err);
    testrun(0 == strcmp(ov_json_string_get(ov_json_get(err, "/unknown")),
                        "ID not found."));

    err = ov_json_value_free(err);

    val = ov_json_get(
        db->data.domains, "/domain1/projects/project1/users/user11");
    testrun(1 == ov_json_object_count(val));
    testrun(ov_json_get(val, "/id"));

    testrun(ov_vocs_db_update_entity_item(
        db, OV_VOCS_DB_USER, "user11", out, &err));

    val = ov_json_get(
        db->data.domains, "/domain1/projects/project1/users/user11");
    testrun(1 == ov_json_object_count(val));
    testrun(ov_json_get(val, "/id"));

    val = ov_json_true();
    ov_json_object_set(out, "key", (ov_json_value *)val);

    testrun(ov_vocs_db_update_entity_item(
        db, OV_VOCS_DB_USER, "user11", out, &err));

    val = ov_json_get(
        db->data.domains, "/domain1/projects/project1/users/user11");
    testrun(2 == ov_json_object_count(val));
    testrun(ov_json_get(val, "/id"));
    testrun(ov_json_is_true(ov_json_get(val, "/key")));

    val = ov_json_false();
    ov_json_object_set(out, "key", (ov_json_value *)val);

    testrun(ov_vocs_db_update_entity_item(
        db, OV_VOCS_DB_USER, "user11", out, &err));

    val = ov_json_get(
        db->data.domains, "/domain1/projects/project1/users/user11");
    testrun(2 == ov_json_object_count(val));
    testrun(ov_json_get(val, "/id"));
    testrun(ov_json_is_false(ov_json_get(val, "/key")));

    val = ov_json_false();
    ov_json_object_set(out, "password", (ov_json_value *)val);

    testrun(!ov_vocs_db_update_entity_item(
        db, OV_VOCS_DB_USER, "user11", out, &err));
    err = ov_json_value_free(err);

    val = ov_json_get(
        db->data.domains, "/domain1/projects/project1/users/user11");
    testrun(2 == ov_json_object_count(val));
    testrun(ov_json_get(val, "/id"));
    testrun(ov_json_is_false(ov_json_get(val, "/key")));

    out = ov_json_value_free(out);
    out = ov_json_object();

    val = ov_json_get(
        db->data.domains, "/domain1/projects/project1/roles/role11");
    testrun(1 == ov_json_object_count(val));
    testrun(ov_json_get(val, "/id"));

    testrun(ov_vocs_db_update_entity_item(
        db, OV_VOCS_DB_ROLE, "role11", out, &err));

    val = ov_json_get(
        db->data.domains, "/domain1/projects/project1/roles/role11");
    testrun(1 == ov_json_object_count(val));
    testrun(ov_json_get(val, "/id"));

    val = ov_json_get(
        db->data.domains, "/domain1/projects/project1/loops/loop11");
    testrun(2 == ov_json_object_count(val));
    testrun(ov_json_get(val, "/id"));
    testrun(ov_json_get(val, "/roles"));

    testrun(ov_vocs_db_update_entity_item(
        db, OV_VOCS_DB_LOOP, "loop11", out, &err));

    val = ov_json_get(
        db->data.domains, "/domain1/projects/project1/loops/loop11");
    testrun(2 == ov_json_object_count(val));
    testrun(ov_json_get(val, "/id"));
    testrun(ov_json_get(val, "/roles"));

    val = ov_json_false();
    ov_json_object_set(out, "key", (ov_json_value *)val);

    testrun(ov_vocs_db_update_entity_item(
        db, OV_VOCS_DB_ROLE, "role11", out, &err));

    val = ov_json_get(
        db->data.domains, "/domain1/projects/project1/roles/role11");
    testrun(2 == ov_json_object_count(val));
    testrun(ov_json_get(val, "/id"));
    testrun(ov_json_is_false(ov_json_get(val, "/key")));

    testrun(ov_vocs_db_update_entity_item(
        db, OV_VOCS_DB_LOOP, "loop11", out, &err));

    val = ov_json_get(
        db->data.domains, "/domain1/projects/project1/loops/loop11");
    testrun(3 == ov_json_object_count(val));
    testrun(ov_json_get(val, "/id"));
    testrun(ov_json_get(val, "/roles"));
    testrun(ov_json_is_false(ov_json_get(val, "/key")));

    out = ov_json_value_free(out);
    out = ov_json_object();
    par = ov_json_object();
    ov_json_object_set(out, OV_KEY_USERS, par);
    val = create_id_object("id");
    ov_json_object_set(par, "user31", (ov_json_value *)val);

    testrun(!ov_vocs_db_update_entity_item(
        db, OV_VOCS_DB_PROJECT, "project1", out, &err));

    testrun(err);
    err = ov_json_value_free(err);

    testrun(!ov_vocs_db_update_entity_item(
        db, OV_VOCS_DB_DOMAIN, "domain1", out, &err));

    testrun(err);
    err = ov_json_value_free(err);

    testrun(!ov_vocs_db_update_entity_item(
        db, OV_VOCS_DB_DOMAIN, "domain3", out, &err));

    testrun(err);
    err = ov_json_value_free(err);

    // update project owning the user, will delete user32 and user33

    testrun(ov_vocs_db_update_entity_item(
        db, OV_VOCS_DB_PROJECT, "project3", out, &err));

    testrun(!err);

    testrun(3 == ov_dict_count(db->index.domains));
    testrun(3 == ov_dict_count(db->index.projects));
    testrun(10 == ov_dict_count(db->index.users));
    testrun(9 == ov_dict_count(db->index.roles));
    testrun(9 == ov_dict_count(db->index.loops));

    val = ov_json_get(db->data.domains, "/domain3/projects/project3/users");
    testrun(1 == ov_json_object_count(val));
    testrun(ov_json_get(val, "/user31"));

    out = ov_json_value_free(out);
    out = ov_json_object();
    par = ov_json_object();
    ov_json_object_set(out, OV_KEY_ROLES, par);
    val = create_id_object("id");
    ov_json_object_set(par, "role31", (ov_json_value *)val);

    testrun(!ov_vocs_db_update_entity_item(
        db, OV_VOCS_DB_PROJECT, "project1", out, &err));

    testrun(err);
    err = ov_json_value_free(err);

    testrun(!ov_vocs_db_update_entity_item(
        db, OV_VOCS_DB_DOMAIN, "domain1", out, &err));

    testrun(err);
    err = ov_json_value_free(err);

    testrun(!ov_vocs_db_update_entity_item(
        db, OV_VOCS_DB_DOMAIN, "domain3", out, &err));

    testrun(err);
    err = ov_json_value_free(err);

    // update project owning the role, will delete role32 and role33

    testrun(ov_vocs_db_update_entity_item(
        db, OV_VOCS_DB_PROJECT, "project3", out, &err));

    testrun(!err);

    testrun(3 == ov_dict_count(db->index.domains));
    testrun(3 == ov_dict_count(db->index.projects));
    testrun(10 == ov_dict_count(db->index.users));
    testrun(7 == ov_dict_count(db->index.roles));
    testrun(9 == ov_dict_count(db->index.loops));

    val = ov_json_get(db->data.domains, "/domain3/projects/project3/roles");
    testrun(1 == ov_json_object_count(val));
    testrun(ov_json_get(val, "/role31"));

    out = ov_json_value_free(out);
    out = ov_json_object();
    par = ov_json_object();
    ov_json_object_set(out, OV_KEY_LOOPS, par);
    val = create_id_object("id");
    ov_json_object_set(par, "loop31", (ov_json_value *)val);

    testrun(!ov_vocs_db_update_entity_item(
        db, OV_VOCS_DB_PROJECT, "project1", out, &err));

    testrun(err);
    err = ov_json_value_free(err);

    testrun(!ov_vocs_db_update_entity_item(
        db, OV_VOCS_DB_DOMAIN, "domain1", out, &err));

    testrun(err);
    err = ov_json_value_free(err);

    testrun(!ov_vocs_db_update_entity_item(
        db, OV_VOCS_DB_DOMAIN, "domain3", out, &err));

    testrun(err);
    err = ov_json_value_free(err);

    // update project owning the loop, will delete loop32 and loop33

    testrun(ov_vocs_db_update_entity_item(
        db, OV_VOCS_DB_PROJECT, "project3", out, &err));

    testrun(!err);

    testrun(3 == ov_dict_count(db->index.domains));
    testrun(3 == ov_dict_count(db->index.projects));
    testrun(10 == ov_dict_count(db->index.users));
    testrun(7 == ov_dict_count(db->index.roles));
    testrun(7 == ov_dict_count(db->index.loops));

    out = ov_json_value_free(out);

    val = ov_json_get(db->data.domains, "/domain3/projects/project3/loops");
    testrun(1 == ov_json_object_count(val));
    testrun(ov_json_get(val, "/loop31"));

    ov_json_value_free(userdata.msg);

    testrun(NULL == ov_vocs_db_free(db));
    testrun(NULL == ov_event_trigger_free(trigger));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_vocs_db_inject() {

    ov_vocs_db *db = ov_vocs_db_create((ov_vocs_db_config){0});
    testrun(db);

    testrun(create_test_db(db));

    testrun(3 == ov_dict_count(db->index.domains));
    testrun(3 == ov_dict_count(db->index.projects));
    testrun(12 == ov_dict_count(db->index.users));
    testrun(9 == ov_dict_count(db->index.roles));
    testrun(9 == ov_dict_count(db->index.loops));

    ov_json_value *data = ov_vocs_db_eject(db, OV_VOCS_DB_TYPE_AUTH);
    testrun(data);

    testrun(vocs_db_clear(db));
    testrun(0 == db->data.domains);
    testrun(0 == ov_dict_count(db->index.domains));
    testrun(0 == ov_dict_count(db->index.projects));
    testrun(0 == ov_dict_count(db->index.users));
    testrun(0 == ov_dict_count(db->index.roles));
    testrun(0 == ov_dict_count(db->index.loops));

    testrun(ov_vocs_db_inject(db, OV_VOCS_DB_TYPE_AUTH, data));
    testrun(3 == ov_dict_count(db->index.domains));
    testrun(3 == ov_dict_count(db->index.projects));
    testrun(12 == ov_dict_count(db->index.users));
    testrun(9 == ov_dict_count(db->index.roles));
    testrun(9 == ov_dict_count(db->index.loops));
    testrun(data == db->data.domains);

    testrun(NULL == ov_vocs_db_free(db));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_vocs_db_eject() {

    ov_vocs_db *db = ov_vocs_db_create((ov_vocs_db_config){0});
    testrun(db);

    testrun(create_test_db(db));

    // ov_vocs_db_dump(stdout, db);

    testrun(3 == ov_dict_count(db->index.domains));
    testrun(3 == ov_dict_count(db->index.projects));
    testrun(12 == ov_dict_count(db->index.users));
    testrun(9 == ov_dict_count(db->index.roles));
    testrun(9 == ov_dict_count(db->index.loops));

    ov_json_value *data = ov_vocs_db_eject(db, OV_VOCS_DB_TYPE_AUTH);
    testrun(data);
    testrun(data != db->data.domains);

    testrun(ov_json_get(data, "/domain1"));
    testrun(ov_json_get(data, "/domain2"));
    testrun(ov_json_get(data, "/domain3"));
    testrun(ov_json_get(data, "/domain1/projects/project1/loops/loop11"));
    testrun(ov_json_get(data, "/domain2/projects/project2/users/user21"));
    testrun(ov_json_get(data, "/domain3/projects/project3/roles/role31"));

    data = ov_json_value_free(data);

    testrun(NULL == ov_vocs_db_free(db));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_vocs_db_set_password() {

    ov_vocs_db *db = ov_vocs_db_create((ov_vocs_db_config){0});
    testrun(db);

    testrun(create_test_db(db));

    // ov_vocs_db_dump(stdout, db);

    ov_json_value *user = ov_dict_get(db->index.users, "user11");
    testrun(user);
    testrun(1 == ov_json_object_count(user));
    testrun(ov_json_get(user, "/id"));

    testrun(!ov_vocs_db_set_password(NULL, NULL, NULL));
    testrun(!ov_vocs_db_set_password(NULL, "user11", "pass"));
    testrun(!ov_vocs_db_set_password(db, NULL, "pass"));
    testrun(!ov_vocs_db_set_password(db, "user11", NULL));

    testrun(ov_vocs_db_set_password(db, "user11", "pass"));

    testrun(2 == ov_json_object_count(user));
    testrun(ov_json_get(user, "/id"));
    testrun(ov_json_get(user, "/password"));

    testrun(ov_vocs_db_authenticate(db, "user11", "pass"));

    db->config.password.params.workfactor = 2048;
    db->config.password.params.blocksize = 16;
    db->config.password.params.parallel = 32;

    testrun(ov_vocs_db_set_password(db, "user11", "pass"));
    testrun(ov_vocs_db_authenticate(db, "user11", "pass"));

    db->config.password.params.workfactor = 0;
    db->config.password.params.blocksize = 0;
    db->config.password.params.parallel = 0;

    testrun(ov_vocs_db_authenticate(db, "user11", "pass"));

    testrun(ov_vocs_db_set_password(db, "user11", "pass1"));
    testrun(!ov_vocs_db_authenticate(db, "user11", "pass"));
    testrun(ov_vocs_db_authenticate(db, "user11", "pass1"));

    // ov_vocs_db_dump(stdout, db);

    testrun(NULL == ov_vocs_db_free(db));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_vocs_db_authenticate() {

    ov_vocs_db *db = ov_vocs_db_create((ov_vocs_db_config){0});
    testrun(db);

    testrun(create_test_db(db));

    // ov_vocs_db_dump(stdout, db);

    ov_json_value *user = ov_dict_get(db->index.users, "user11");
    testrun(user);
    testrun(1 == ov_json_object_count(user));
    testrun(ov_json_get(user, "/id"));

    // no password set

    testrun(!ov_vocs_db_authenticate(db, "user11", "pass"));

    testrun(ov_vocs_db_set_password(db, "user11", "pass"));

    testrun(!ov_vocs_db_authenticate(NULL, NULL, NULL));
    testrun(!ov_vocs_db_authenticate(NULL, "user11", "pass"));
    testrun(!ov_vocs_db_authenticate(db, NULL, "pass"));
    testrun(!ov_vocs_db_authenticate(db, "user11", NULL));

    testrun(ov_vocs_db_authenticate(db, "user11", "pass"));
    testrun(!ov_vocs_db_authenticate(db, "user11", "pass1"));
    testrun(!ov_vocs_db_authenticate(db, "user11", "Pass"));
    testrun(!ov_vocs_db_authenticate(db, "user11", "pas"));
    testrun(!ov_vocs_db_authenticate(db, "user11", "PASS"));
    testrun(!ov_vocs_db_authenticate(db, "user11", "pss"));

    // ov_vocs_db_dump(stdout, db);

    testrun(NULL == ov_vocs_db_free(db));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_vocs_db_authorize() {

    ov_vocs_db *db = ov_vocs_db_create((ov_vocs_db_config){0});
    testrun(db);

    testrun(create_test_db(db));

    // ov_vocs_db_dump(stdout, db);

    testrun(ov_vocs_db_authorize(db, "user31", "role31"));

    testrun(!ov_vocs_db_authorize(NULL, "user31", "role31"));
    testrun(!ov_vocs_db_authorize(db, NULL, "role31"));
    testrun(!ov_vocs_db_authorize(db, "user31", NULL));
    testrun(!ov_vocs_db_authorize(db, "user31", "role32"));

    testrun(NULL == ov_vocs_db_free(db));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_vocs_db_get_permission() {

    ov_vocs_db *db = ov_vocs_db_create((ov_vocs_db_config){0});
    testrun(db);

    testrun(create_test_db(db));

    // ov_vocs_db_dump(stdout, db);

    testrun(OV_VOCS_NONE == ov_vocs_db_get_permission(db, "role22", "loop11"));
    testrun(OV_VOCS_RECV == ov_vocs_db_get_permission(db, "role21", "loop11"));
    testrun(OV_VOCS_RECV == ov_vocs_db_get_permission(db, "role31", "loop11"));
    testrun(OV_VOCS_SEND == ov_vocs_db_get_permission(db, "role11", "loop11"));
    testrun(OV_VOCS_SEND == ov_vocs_db_get_permission(db, "role12", "loop11"));

    testrun(OV_VOCS_NONE ==
            ov_vocs_db_get_permission(NULL, "role12", "loop11"));
    testrun(OV_VOCS_NONE == ov_vocs_db_get_permission(db, NULL, "loop11"));
    testrun(OV_VOCS_NONE == ov_vocs_db_get_permission(db, "role12", NULL));

    testrun(OV_VOCS_NONE == ov_vocs_db_get_permission(db, "role12", "unknown"));
    testrun(OV_VOCS_NONE == ov_vocs_db_get_permission(db, "unknown", "loop11"));

    testrun(NULL == ov_vocs_db_free(db));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_vocs_db_authorize_project_admin() {

    ov_vocs_db *db = ov_vocs_db_create((ov_vocs_db_config){0});
    testrun(db);

    testrun(create_test_db(db));

    // create some project admin
    ov_json_value *out = ov_json_object();
    ov_json_value *adm = ov_json_object();
    ov_json_value *usr = ov_json_object();
    ov_json_value *val = ov_json_null();
    ov_json_object_set(out, OV_KEY_ADMIN, adm);
    ov_json_object_set(adm, OV_KEY_USERS, usr);
    ov_json_object_set(usr, "user11", val);
    ov_json_object_set(usr, "user12", ov_json_null());
    ov_json_object_set(usr, "user13", ov_json_null());

    testrun(ov_vocs_db_update_entity_key(
        db, OV_VOCS_DB_PROJECT, "project1", OV_KEY_ROLES, out));

    // create some other project admins

    ov_json_object_del(usr, "user11");
    ov_json_object_del(usr, "user12");
    ov_json_object_del(usr, "user13");

    ov_json_object_set(usr, "user21", ov_json_null());
    ov_json_object_set(usr, "user22", ov_json_null());
    ov_json_object_set(usr, "user23", ov_json_null());

    testrun(ov_vocs_db_update_entity_key(
        db, OV_VOCS_DB_PROJECT, "project2", OV_KEY_ROLES, out));

    // create some domain admins

    ov_json_object_del(usr, "user21");
    ov_json_object_del(usr, "user22");
    ov_json_object_del(usr, "user23");

    ov_json_object_set(usr, "user11", ov_json_null());
    ov_json_object_set(usr, "user21", ov_json_null());
    ov_json_object_set(usr, "user31", ov_json_null());

    testrun(ov_vocs_db_update_entity_key(
        db, OV_VOCS_DB_DOMAIN, "domain1", OV_KEY_ROLES, out));

    out = ov_json_value_free(out);

    // ov_vocs_db_dump(stdout, db);

    testrun(ov_vocs_db_authorize_project_admin(db, "user11", "project1"));
    testrun(ov_vocs_db_authorize_project_admin(db, "user12", "project1"));
    testrun(ov_vocs_db_authorize_project_admin(db, "user13", "project1"));
    testrun(ov_vocs_db_authorize_project_admin(db, "user21", "project2"));
    testrun(ov_vocs_db_authorize_project_admin(db, "user22", "project2"));
    testrun(ov_vocs_db_authorize_project_admin(db, "user23", "project2"));
    testrun(ov_vocs_db_authorize_project_admin(db, "user21", "project1"));
    testrun(ov_vocs_db_authorize_project_admin(db, "user31", "project1"));
    testrun(!ov_vocs_db_authorize_project_admin(db, "user11", "project2"));
    testrun(ov_vocs_db_authorize_project_admin(db, "user21", "project1"));
    testrun(!ov_vocs_db_authorize_project_admin(db, "user22", "project1"));
    testrun(!ov_vocs_db_authorize_project_admin(db, "user23", "project1"));

    testrun(NULL == ov_vocs_db_free(db));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_vocs_db_authorize_domain_admin() {

    ov_vocs_db *db = ov_vocs_db_create((ov_vocs_db_config){0});
    testrun(db);

    testrun(create_test_db(db));

    // create some project admin
    ov_json_value *out = ov_json_object();
    ov_json_value *adm = ov_json_object();
    ov_json_value *usr = ov_json_object();
    ov_json_value *val = ov_json_null();
    ov_json_object_set(out, OV_KEY_ADMIN, adm);
    ov_json_object_set(adm, OV_KEY_USERS, usr);
    ov_json_object_set(usr, "user11", val);
    ov_json_object_set(usr, "user12", ov_json_null());
    ov_json_object_set(usr, "user13", ov_json_null());

    testrun(ov_vocs_db_update_entity_key(
        db, OV_VOCS_DB_PROJECT, "project1", OV_KEY_ROLES, out));

    // create some other project admins

    ov_json_object_del(usr, "user11");
    ov_json_object_del(usr, "user12");
    ov_json_object_del(usr, "user13");

    ov_json_object_set(usr, "user21", ov_json_null());
    ov_json_object_set(usr, "user22", ov_json_null());
    ov_json_object_set(usr, "user23", ov_json_null());

    testrun(ov_vocs_db_update_entity_key(
        db, OV_VOCS_DB_PROJECT, "project2", OV_KEY_ROLES, out));

    // create some domain admins

    ov_json_object_del(usr, "user21");
    ov_json_object_del(usr, "user22");
    ov_json_object_del(usr, "user23");

    ov_json_object_set(usr, "user11", ov_json_null());
    ov_json_object_set(usr, "user21", ov_json_null());
    ov_json_object_set(usr, "user31", ov_json_null());

    testrun(ov_vocs_db_update_entity_key(
        db, OV_VOCS_DB_DOMAIN, "domain1", OV_KEY_ROLES, out));

    out = ov_json_value_free(out);

    // ov_vocs_db_dump(stdout, db);

    testrun(ov_vocs_db_authorize_domain_admin(db, "user11", "domain1"));
    testrun(!ov_vocs_db_authorize_domain_admin(db, "user12", "domain1"));
    testrun(!ov_vocs_db_authorize_domain_admin(db, "user13", "domain1"));
    testrun(ov_vocs_db_authorize_domain_admin(db, "user21", "domain1"));
    testrun(!ov_vocs_db_authorize_domain_admin(db, "user22", "domain1"));
    testrun(!ov_vocs_db_authorize_domain_admin(db, "user23", "domain1"));
    testrun(ov_vocs_db_authorize_domain_admin(db, "user31", "domain1"));

    testrun(NULL == ov_vocs_db_free(db));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_vocs_db_get_admin_domains() {

    ov_vocs_db *db = ov_vocs_db_create((ov_vocs_db_config){0});
    testrun(db);

    testrun(create_test_db(db));

    // create some project admin
    ov_json_value *out = ov_json_object();
    ov_json_value *adm = ov_json_object();
    ov_json_value *usr = ov_json_object();
    ov_json_value *val = ov_json_null();
    ov_json_object_set(out, OV_KEY_ADMIN, adm);
    ov_json_object_set(adm, OV_KEY_USERS, usr);
    ov_json_object_set(usr, "user11", val);
    ov_json_object_set(usr, "user12", ov_json_null());
    ov_json_object_set(usr, "user13", ov_json_null());

    testrun(ov_vocs_db_update_entity_key(
        db, OV_VOCS_DB_PROJECT, "project1", OV_KEY_ROLES, out));

    // create some other project admins

    ov_json_object_set(usr, "user21", ov_json_null());
    ov_json_object_set(usr, "user22", ov_json_null());
    ov_json_object_set(usr, "user23", ov_json_null());

    testrun(ov_vocs_db_update_entity_key(
        db, OV_VOCS_DB_PROJECT, "project2", OV_KEY_ROLES, out));

    // create some domain admins

    ov_json_object_del(usr, "user11");
    ov_json_object_del(usr, "user12");
    ov_json_object_del(usr, "user13");
    ov_json_object_del(usr, "user21");
    ov_json_object_del(usr, "user22");
    ov_json_object_del(usr, "user23");

    ov_json_object_set(usr, "user11", ov_json_null());
    ov_json_object_set(usr, "user21", ov_json_null());
    ov_json_object_set(usr, "user31", ov_json_null());

    testrun(ov_vocs_db_update_entity_key(
        db, OV_VOCS_DB_DOMAIN, "domain1", OV_KEY_ROLES, out));

    testrun(ov_vocs_db_update_entity_key(
        db, OV_VOCS_DB_DOMAIN, "domain2", OV_KEY_ROLES, out));

    out = ov_json_value_free(out);

    // ov_vocs_db_dump(stdout, db);

    out = ov_vocs_db_get_admin_domains(db, "user11");
    testrun(out);
    testrun(2 == ov_json_array_count(out));
    out = ov_json_value_free(out);

    out = ov_vocs_db_get_admin_domains(db, "user22");
    testrun(out);
    testrun(0 == ov_json_array_count(out));
    out = ov_json_value_free(out);

    out = ov_vocs_db_get_admin_domains(db, "user23");
    testrun(out);
    testrun(0 == ov_json_array_count(out));
    out = ov_json_value_free(out);

    out = ov_vocs_db_get_admin_domains(db, "user21");
    testrun(out);
    testrun(2 == ov_json_array_count(out));
    out = ov_json_value_free(out);

    testrun(NULL == ov_vocs_db_free(db));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_vocs_db_get_admin_projects() {

    ov_vocs_db *db = ov_vocs_db_create((ov_vocs_db_config){0});
    testrun(db);

    testrun(create_test_db(db));

    // create some project admin
    ov_json_value *out = ov_json_object();
    ov_json_value *adm = ov_json_object();
    ov_json_value *usr = ov_json_object();
    ov_json_value *val = ov_json_null();
    ov_json_object_set(out, OV_KEY_ADMIN, adm);
    ov_json_object_set(adm, OV_KEY_USERS, usr);
    ov_json_object_set(usr, "user11", val);
    ov_json_object_set(usr, "user12", ov_json_null());
    ov_json_object_set(usr, "user13", ov_json_null());

    testrun(ov_vocs_db_update_entity_key(
        db, OV_VOCS_DB_PROJECT, "project1", OV_KEY_ROLES, out));

    // create some other project admins

    ov_json_object_set(usr, "user21", ov_json_null());
    ov_json_object_set(usr, "user22", ov_json_null());
    ov_json_object_set(usr, "user23", ov_json_null());

    testrun(ov_vocs_db_update_entity_key(
        db, OV_VOCS_DB_PROJECT, "project2", OV_KEY_ROLES, out));

    // create some domain admins

    ov_json_object_del(usr, "user11");
    ov_json_object_del(usr, "user12");
    ov_json_object_del(usr, "user13");
    ov_json_object_del(usr, "user21");
    ov_json_object_del(usr, "user22");
    ov_json_object_del(usr, "user23");

    ov_json_object_set(usr, "user11", ov_json_null());
    ov_json_object_set(usr, "user21", ov_json_null());
    ov_json_object_set(usr, "user31", ov_json_null());

    testrun(ov_vocs_db_update_entity_key(
        db, OV_VOCS_DB_DOMAIN, "domain1", OV_KEY_ROLES, out));

    testrun(ov_vocs_db_update_entity_key(
        db, OV_VOCS_DB_DOMAIN, "domain2", OV_KEY_ROLES, out));

    testrun(ov_vocs_db_update_entity_key(
        db, OV_VOCS_DB_DOMAIN, "domain3", OV_KEY_ROLES, out));

    out = ov_json_value_free(out);

    // ov_vocs_db_dump(stdout, db);

    out = ov_vocs_db_get_admin_projects(db, "user11");
    testrun(out);
    testrun(3 == ov_json_object_count(out));
    val = ov_json_object_get(out, "project1");
    testrun(val);
    testrun(3 == ov_json_object_count(val));
    testrun(ov_json_object_get(val, OV_KEY_ID));
    testrun(ov_json_object_get(val, OV_KEY_NAME));
    testrun(ov_json_object_get(val, OV_KEY_DOMAIN));
    out = ov_json_value_free(out);

    out = ov_vocs_db_get_admin_projects(db, "user22");
    testrun(out);
    testrun(1 == ov_json_object_count(out));
    out = ov_json_value_free(out);

    out = ov_vocs_db_get_admin_projects(db, "user23");
    testrun(out);
    testrun(1 == ov_json_object_count(out));
    out = ov_json_value_free(out);

    out = ov_vocs_db_get_admin_projects(db, "user12");
    testrun(out);
    testrun(2 == ov_json_object_count(out));
    out = ov_json_value_free(out);

    out = ov_vocs_db_get_admin_projects(db, "user13");
    testrun(out);
    testrun(2 == ov_json_object_count(out));
    out = ov_json_value_free(out);

    out = ov_vocs_db_get_admin_projects(db, "user21");
    testrun(out);
    testrun(3 == ov_json_object_count(out));
    out = ov_json_value_free(out);

    out = ov_vocs_db_get_admin_projects(db, "user32");
    testrun(out);
    testrun(0 == ov_json_object_count(out));
    out = ov_json_value_free(out);

    testrun(NULL == ov_vocs_db_free(db));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_vocs_db_get_user_roles() {

    ov_vocs_db *db = ov_vocs_db_create((ov_vocs_db_config){0});
    testrun(db);

    testrun(create_test_db(db));

    // ov_vocs_db_dump(stdout, db);

    ov_json_value *out = ov_vocs_db_get_user_roles(db, "user31");
    testrun(out);
    testrun(1 == ov_json_object_count(out));
    testrun(ov_json_get(out, "/role31/project"));
    testrun(ov_json_get(out, "/role31/id"));
    // ov_json_value_dump(stdout, out);
    out = ov_json_value_free(out);

    out = ov_json_object();

    ov_json_object_set(out, "user31", ov_json_null());

    testrun(ov_vocs_db_update_entity_key(
        db, OV_VOCS_DB_ROLE, "role11", OV_KEY_USERS, out));

    testrun(ov_vocs_db_update_entity_key(
        db, OV_VOCS_DB_ROLE, "role21", OV_KEY_USERS, out));

    out = ov_json_value_free(out);

    uint64_t start = ov_time_get_current_time_usecs();
    out = ov_vocs_db_get_user_roles(db, "user31");
    uint64_t stop = ov_time_get_current_time_usecs();
    fprintf(stdout, "user roles in %" PRIu64 " usec\n", stop - start);
    testrun(out);
    testrun(3 == ov_json_object_count(out));
    testrun(ov_json_get(out, "/role31/project"));
    testrun(ov_json_get(out, "/role31/id"));
    testrun(ov_json_get(out, "/role11/project"));
    testrun(ov_json_get(out, "/role11/id"));
    testrun(ov_json_get(out, "/role21/project"));
    testrun(ov_json_get(out, "/role21/id"));
    // ov_json_value_dump(stdout, out);
    out = ov_json_value_free(out);

    testrun(NULL == ov_vocs_db_free(db));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_vocs_db_get_role_loops() {

    ov_vocs_db *db = ov_vocs_db_create((ov_vocs_db_config){0});
    testrun(db);

    testrun(create_test_db(db));

    // ov_vocs_db_dump(stdout, db);

    ov_json_value *out = ov_vocs_db_get_role_loops(db, "role11");
    testrun(out);
    // ov_json_value_dump(stdout, out);
    testrun(1 == ov_json_object_count(out));
    testrun(ov_json_get(out, "/loop11/permission"));
    testrun(ov_json_get(out, "/loop11/name"));
    testrun(ov_json_get(out, "/loop11/type"));
    testrun(ov_json_get(out, "/loop11/id"));
    out = ov_json_value_free(out);

    out = ov_json_object();
    ov_json_object_set(out, "role11", ov_json_true());

    testrun(ov_vocs_db_update_entity_key(
        db, OV_VOCS_DB_LOOP, "loop21", OV_KEY_ROLES, out));

    testrun(ov_vocs_db_update_entity_key(
        db, OV_VOCS_DB_LOOP, "loop31", OV_KEY_ROLES, out));

    testrun(ov_vocs_db_update_entity_key(
        db, OV_VOCS_DB_LOOP, "loop33", OV_KEY_ROLES, out));

    out = ov_json_value_free(out);

    out = ov_vocs_db_get_role_loops(db, "role11");
    testrun(out);
    // ov_json_value_dump(stdout, out);
    testrun(4 == ov_json_object_count(out));
    testrun(ov_json_get(out, "/loop11/permission"));
    testrun(ov_json_get(out, "/loop11/name"));
    testrun(ov_json_get(out, "/loop11/type"));
    testrun(ov_json_get(out, "/loop11/id"));
    testrun(ov_json_get(out, "/loop21/permission"));
    testrun(ov_json_get(out, "/loop21/name"));
    testrun(ov_json_get(out, "/loop21/type"));
    testrun(ov_json_get(out, "/loop21/id"));
    testrun(ov_json_get(out, "/loop31/permission"));
    testrun(ov_json_get(out, "/loop31/name"));
    testrun(ov_json_get(out, "/loop31/type"));
    testrun(ov_json_get(out, "/loop31/id"));
    testrun(ov_json_get(out, "/loop33/permission"));
    testrun(ov_json_get(out, "/loop33/name"));
    testrun(ov_json_get(out, "/loop33/type"));
    testrun(ov_json_get(out, "/loop33/id"));
    out = ov_json_value_free(out);

    testrun(NULL == ov_vocs_db_free(db));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_vocs_db_get_user_role_loops() {

    ov_vocs_db *db = ov_vocs_db_create((ov_vocs_db_config){0});
    testrun(db);

    testrun(create_test_db(db));

    // ov_vocs_db_dump(stdout, db);

    ov_json_value *out = ov_vocs_db_get_user_role_loops(db, "user11", "role11");
    testrun(out);
    // ov_json_value_dump(stdout, out);
    testrun(1 == ov_json_object_count(out));
    testrun(ov_json_get(out, "/loop11/permission"));
    testrun(ov_json_get(out, "/loop11/name"));
    testrun(ov_json_get(out, "/loop11/type"));
    testrun(ov_json_get(out, "/loop11/id"));
    testrun(ov_json_get(out, "/loop11/state"));
    testrun(ov_json_get(out, "/loop11/volume"));
    out = ov_json_value_free(out);

    out = ov_json_object();
    ov_json_object_set(out, "role11", ov_json_true());

    testrun(ov_vocs_db_update_entity_key(
        db, OV_VOCS_DB_LOOP, "loop21", OV_KEY_ROLES, out));

    out = ov_json_value_free(out);

    out = ov_vocs_db_get_user_role_loops(db, "user11", "role11");
    testrun(out);
    // ov_json_value_dump(stdout, out);
    testrun(2 == ov_json_object_count(out));
    testrun(ov_json_get(out, "/loop11/permission"));
    testrun(ov_json_get(out, "/loop11/name"));
    testrun(ov_json_get(out, "/loop11/type"));
    testrun(ov_json_get(out, "/loop11/id"));
    testrun(ov_json_get(out, "/loop11/state"));
    testrun(ov_json_get(out, "/loop11/volume"));
    testrun(ov_json_get(out, "/loop21/permission"));
    testrun(ov_json_get(out, "/loop21/name"));
    testrun(ov_json_get(out, "/loop21/type"));
    testrun(ov_json_get(out, "/loop21/id"));
    testrun(ov_json_get(out, "/loop21/state"));
    testrun(ov_json_get(out, "/loop21/volume"));
    testrun(0 == strcmp(ov_vocs_permission_to_string(OV_VOCS_NONE),
                        ov_json_string_get(ov_json_get(out, "/loop21/state"))));
    testrun(50 == ov_json_number_get(ov_json_get(out, "/loop21/volume")));
    out = ov_json_value_free(out);

    testrun(
        ov_vocs_db_set_state(db, "user11", "role11", "loop11", OV_VOCS_SEND));
    testrun(
        ov_vocs_db_set_state(db, "user11", "role21", "loop11", OV_VOCS_RECV));
    testrun(ov_vocs_db_set_volume(db, "user11", "role11", "loop11", 60));
    testrun(ov_vocs_db_set_volume(db, "user11", "role21", "loop11", 70));

    out = ov_vocs_db_get_user_role_loops(db, "user11", "role11");
    testrun(out);
    // ov_json_value_dump(stdout, out);
    testrun(2 == ov_json_object_count(out));
    testrun(ov_json_get(out, "/loop11/permission"));
    testrun(ov_json_get(out, "/loop11/name"));
    testrun(ov_json_get(out, "/loop11/type"));
    testrun(ov_json_get(out, "/loop11/id"));
    testrun(ov_json_get(out, "/loop11/state"));
    testrun(ov_json_get(out, "/loop11/volume"));
    testrun(ov_json_get(out, "/loop21/permission"));
    testrun(ov_json_get(out, "/loop21/name"));
    testrun(ov_json_get(out, "/loop21/type"));
    testrun(ov_json_get(out, "/loop21/id"));
    testrun(ov_json_get(out, "/loop21/state"));
    testrun(ov_json_get(out, "/loop21/volume"));
    testrun(0 == strcmp(ov_vocs_permission_to_string(OV_VOCS_NONE),
                        ov_json_string_get(ov_json_get(out, "/loop21/state"))));
    testrun(0 == strcmp(ov_vocs_permission_to_string(OV_VOCS_SEND),
                        ov_json_string_get(ov_json_get(out, "/loop11/state"))));
    testrun(60 == ov_json_number_get(ov_json_get(out, "/loop11/volume")));
    out = ov_json_value_free(out);

    out = ov_vocs_db_get_user_role_loops(db, "user11", "role21");
    testrun(out);
    // ov_json_value_dump(stdout, out);
    testrun(1 == ov_json_object_count(out));
    testrun(ov_json_get(out, "/loop11/permission"));
    testrun(ov_json_get(out, "/loop11/name"));
    testrun(ov_json_get(out, "/loop11/type"));
    testrun(ov_json_get(out, "/loop11/id"));
    testrun(ov_json_get(out, "/loop11/state"));
    testrun(ov_json_get(out, "/loop11/volume"));
    testrun(0 == strcmp(ov_vocs_permission_to_string(OV_VOCS_RECV),
                        ov_json_string_get(ov_json_get(out, "/loop11/state"))));
    testrun(70 == ov_json_number_get(ov_json_get(out, "/loop11/volume")));
    out = ov_json_value_free(out);

    testrun(NULL == ov_vocs_db_free(db));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_vocs_db_set_state() {

    ov_vocs_db *db = ov_vocs_db_create((ov_vocs_db_config){0});
    testrun(db);

    testrun(create_test_db(db));

    testrun(!ov_vocs_db_set_state(
        NULL, "user11", "role11", "loop11", OV_VOCS_NONE));
    testrun(!ov_vocs_db_set_state(db, NULL, "role11", "loop11", OV_VOCS_NONE));
    testrun(!ov_vocs_db_set_state(db, "user11", NULL, "loop11", OV_VOCS_NONE));
    testrun(!ov_vocs_db_set_state(db, "user11", "role11", NULL, OV_VOCS_NONE));
    testrun(
        ov_vocs_db_set_state(db, "user11", "role11", "loop11", OV_VOCS_NONE));
    testrun(
        ov_vocs_db_set_state(db, "user11", "role12", "loop11", OV_VOCS_RECV));
    testrun(
        ov_vocs_db_set_state(db, "user11", "role13", "loop11", OV_VOCS_SEND));
    testrun(
        ov_vocs_db_set_state(db, "user11", "role11", "loop12", OV_VOCS_SEND));

    testrun(OV_VOCS_NONE ==
            ov_vocs_db_get_state(db, "user11", "role11", "loop11"));
    testrun(OV_VOCS_RECV ==
            ov_vocs_db_get_state(db, "user11", "role12", "loop11"));
    testrun(OV_VOCS_SEND ==
            ov_vocs_db_get_state(db, "user11", "role13", "loop11"));
    testrun(OV_VOCS_SEND ==
            ov_vocs_db_get_state(db, "user11", "role11", "loop12"));

    // ov_vocs_db_dump(stdout, db);

    testrun(NULL == ov_vocs_db_free(db));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_vocs_db_get_state() {

    ov_vocs_db *db = ov_vocs_db_create((ov_vocs_db_config){0});
    testrun(db);

    testrun(create_test_db(db));

    testrun(OV_VOCS_NONE ==
            ov_vocs_db_get_state(NULL, "user11", "role11", "loop11"));
    testrun(OV_VOCS_NONE == ov_vocs_db_get_state(db, NULL, "role11", "loop11"));
    testrun(OV_VOCS_NONE == ov_vocs_db_get_state(db, "user11", NULL, "loop11"));
    testrun(OV_VOCS_NONE == ov_vocs_db_get_state(db, "user11", "role11", NULL));

    testrun(
        ov_vocs_db_set_state(db, "user11", "role11", "loop11", OV_VOCS_NONE));
    testrun(
        ov_vocs_db_set_state(db, "user11", "role12", "loop11", OV_VOCS_RECV));
    testrun(
        ov_vocs_db_set_state(db, "user11", "role13", "loop11", OV_VOCS_SEND));
    testrun(
        ov_vocs_db_set_state(db, "user11", "role11", "loop12", OV_VOCS_SEND));

    testrun(OV_VOCS_NONE ==
            ov_vocs_db_get_state(db, "user21", "role11", "loop11"));
    testrun(OV_VOCS_NONE ==
            ov_vocs_db_get_state(db, "user31", "role11", "loop11"));
    testrun(OV_VOCS_NONE ==
            ov_vocs_db_get_state(db, "user11", "role31", "loop11"));
    testrun(OV_VOCS_NONE ==
            ov_vocs_db_get_state(db, "user11", "role11", "loop31"));
    testrun(OV_VOCS_NONE ==
            ov_vocs_db_get_state(db, "user31", "role31", "loop31"));

    testrun(OV_VOCS_NONE ==
            ov_vocs_db_get_state(db, "user11", "role11", "loop11"));
    testrun(OV_VOCS_RECV ==
            ov_vocs_db_get_state(db, "user11", "role12", "loop11"));
    testrun(OV_VOCS_SEND ==
            ov_vocs_db_get_state(db, "user11", "role13", "loop11"));
    testrun(OV_VOCS_SEND ==
            ov_vocs_db_get_state(db, "user11", "role11", "loop12"));

    // ov_vocs_db_dump(stdout, db);

    testrun(NULL == ov_vocs_db_free(db));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_vocs_db_set_volume() {

    ov_vocs_db *db = ov_vocs_db_create((ov_vocs_db_config){0});
    testrun(db);

    testrun(create_test_db(db));

    testrun(!ov_vocs_db_set_volume(NULL, "user11", "role11", "loop11", 100));
    testrun(!ov_vocs_db_set_volume(db, NULL, "role11", "loop11", 100));
    testrun(!ov_vocs_db_set_volume(db, "user11", NULL, "loop11", 100));
    testrun(!ov_vocs_db_set_volume(db, "user11", "role11", NULL, 100));
    testrun(!ov_vocs_db_set_volume(db, "user11", "role11", "loop11", 101));
    testrun(ov_vocs_db_set_volume(db, "user11", "role12", "loop11", 100));
    testrun(ov_vocs_db_set_volume(db, "user11", "role13", "loop11", 0));
    testrun(ov_vocs_db_set_volume(db, "user11", "role11", "loop12", 1));

    testrun(0 == ov_vocs_db_get_volume(db, "user11", "role11", "loop11"));
    testrun(100 == ov_vocs_db_get_volume(db, "user11", "role12", "loop11"));
    testrun(0 == ov_vocs_db_get_volume(db, "user11", "role13", "loop11"));
    testrun(1 == ov_vocs_db_get_volume(db, "user11", "role11", "loop12"));

    // ov_vocs_db_dump(stdout, db);

    testrun(NULL == ov_vocs_db_free(db));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_vocs_db_get_volume() {

    ov_vocs_db *db = ov_vocs_db_create((ov_vocs_db_config){0});
    testrun(db);

    testrun(create_test_db(db));

    testrun(0 == ov_vocs_db_get_volume(NULL, "user11", "role11", "loop11"));
    testrun(0 == ov_vocs_db_get_volume(db, NULL, "role11", "loop11"));
    testrun(0 == ov_vocs_db_get_volume(db, "user11", NULL, "loop11"));
    testrun(0 == ov_vocs_db_get_volume(db, "user11", "role11", NULL));
    testrun(0 == ov_vocs_db_get_volume(db, "user11", "role11", "loop11"));

    testrun(ov_vocs_db_set_volume(db, "user11", "role12", "loop11", 100));
    testrun(ov_vocs_db_set_volume(db, "user11", "role13", "loop11", 0));
    testrun(ov_vocs_db_set_volume(db, "user11", "role11", "loop12", 1));

    testrun(0 == ov_vocs_db_get_volume(db, "user11", "role11", "loop11"));
    testrun(100 == ov_vocs_db_get_volume(db, "user11", "role12", "loop11"));
    testrun(0 == ov_vocs_db_get_volume(db, "user11", "role13", "loop11"));
    testrun(1 == ov_vocs_db_get_volume(db, "user11", "role11", "loop12"));

    // ov_vocs_db_dump(stdout, db);

    testrun(NULL == ov_vocs_db_free(db));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_vocs_db_get_parent() {

    ov_vocs_db *db = ov_vocs_db_create((ov_vocs_db_config){0});
    testrun(db);

    testrun(create_test_db(db));

    testrun(ov_vocs_db_create_entity(
        db, OV_VOCS_DB_LOOP, "loopA", OV_VOCS_DB_SCOPE_DOMAIN, "domain1"));

    testrun(ov_vocs_db_create_entity(
        db, OV_VOCS_DB_LOOP, "loopB", OV_VOCS_DB_SCOPE_DOMAIN, "domain2"));

    testrun(ov_vocs_db_create_entity(
        db, OV_VOCS_DB_USER, "userA", OV_VOCS_DB_SCOPE_DOMAIN, "domain1"));

    testrun(ov_vocs_db_create_entity(
        db, OV_VOCS_DB_USER, "userB", OV_VOCS_DB_SCOPE_DOMAIN, "domain2"));

    testrun(ov_vocs_db_create_entity(
        db, OV_VOCS_DB_ROLE, "roleA", OV_VOCS_DB_SCOPE_DOMAIN, "domain1"));

    testrun(ov_vocs_db_create_entity(
        db, OV_VOCS_DB_ROLE, "roleB", OV_VOCS_DB_SCOPE_DOMAIN, "domain2"));

    // ov_vocs_db_dump(stdout, db);

    ov_vocs_db_parent parent = {0};

    parent = ov_vocs_db_get_parent(db, OV_VOCS_DB_USER, "user11");
    testrun(parent.scope == OV_VOCS_DB_SCOPE_PROJECT);
    testrun(0 == strcmp(parent.id, "project1"));
    testrun(ov_vocs_db_parent_clear(&parent));

    parent = ov_vocs_db_get_parent(db, OV_VOCS_DB_LOOP, "loop11");
    testrun(parent.scope == OV_VOCS_DB_SCOPE_PROJECT);
    testrun(0 == strcmp(parent.id, "project1"));
    testrun(ov_vocs_db_parent_clear(&parent));

    parent = ov_vocs_db_get_parent(db, OV_VOCS_DB_ROLE, "role11");
    testrun(parent.scope == OV_VOCS_DB_SCOPE_PROJECT);
    testrun(0 == strcmp(parent.id, "project1"));
    testrun(ov_vocs_db_parent_clear(&parent));

    parent = ov_vocs_db_get_parent(db, OV_VOCS_DB_USER, "user21");
    testrun(parent.scope == OV_VOCS_DB_SCOPE_PROJECT);
    testrun(0 == strcmp(parent.id, "project2"));
    testrun(ov_vocs_db_parent_clear(&parent));

    parent = ov_vocs_db_get_parent(db, OV_VOCS_DB_LOOP, "loop21");
    testrun(parent.scope == OV_VOCS_DB_SCOPE_PROJECT);
    testrun(0 == strcmp(parent.id, "project2"));
    testrun(ov_vocs_db_parent_clear(&parent));

    parent = ov_vocs_db_get_parent(db, OV_VOCS_DB_ROLE, "role21");
    testrun(parent.scope == OV_VOCS_DB_SCOPE_PROJECT);
    testrun(0 == strcmp(parent.id, "project2"));
    testrun(ov_vocs_db_parent_clear(&parent));

    parent = ov_vocs_db_get_parent(db, OV_VOCS_DB_PROJECT, "project1");
    testrun(parent.scope == OV_VOCS_DB_SCOPE_DOMAIN);
    testrun(0 == strcmp(parent.id, "domain1"));
    testrun(ov_vocs_db_parent_clear(&parent));

    parent = ov_vocs_db_get_parent(db, OV_VOCS_DB_PROJECT, "project2");
    testrun(parent.scope == OV_VOCS_DB_SCOPE_DOMAIN);
    testrun(0 == strcmp(parent.id, "domain2"));
    testrun(ov_vocs_db_parent_clear(&parent));

    parent = ov_vocs_db_get_parent(db, OV_VOCS_DB_DOMAIN, "domain1");
    testrun(parent.scope == OV_VOCS_DB_SCOPE_DOMAIN);
    testrun(0 == strcmp(parent.id, "domain1"));
    testrun(ov_vocs_db_parent_clear(&parent));

    parent = ov_vocs_db_get_parent(db, OV_VOCS_DB_USER, "userA");
    testrun(parent.scope == OV_VOCS_DB_SCOPE_DOMAIN);
    testrun(0 == strcmp(parent.id, "domain1"));
    testrun(ov_vocs_db_parent_clear(&parent));

    parent = ov_vocs_db_get_parent(db, OV_VOCS_DB_LOOP, "loopA");
    testrun(parent.scope == OV_VOCS_DB_SCOPE_DOMAIN);
    testrun(0 == strcmp(parent.id, "domain1"));
    testrun(ov_vocs_db_parent_clear(&parent));

    parent = ov_vocs_db_get_parent(db, OV_VOCS_DB_ROLE, "roleA");
    testrun(parent.scope == OV_VOCS_DB_SCOPE_DOMAIN);
    testrun(0 == strcmp(parent.id, "domain1"));
    testrun(ov_vocs_db_parent_clear(&parent));

    parent = ov_vocs_db_get_parent(db, OV_VOCS_DB_USER, "userB");
    testrun(parent.scope == OV_VOCS_DB_SCOPE_DOMAIN);
    testrun(0 == strcmp(parent.id, "domain2"));
    testrun(ov_vocs_db_parent_clear(&parent));

    parent = ov_vocs_db_get_parent(db, OV_VOCS_DB_LOOP, "loopB");
    testrun(parent.scope == OV_VOCS_DB_SCOPE_DOMAIN);
    testrun(0 == strcmp(parent.id, "domain2"));
    testrun(ov_vocs_db_parent_clear(&parent));

    parent = ov_vocs_db_get_parent(db, OV_VOCS_DB_ROLE, "roleB");
    testrun(parent.scope == OV_VOCS_DB_SCOPE_DOMAIN);
    testrun(0 == strcmp(parent.id, "domain2"));
    testrun(ov_vocs_db_parent_clear(&parent));

    testrun(NULL == ov_vocs_db_free(db));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int check_admin_role() {

    ov_vocs_db *db = ov_vocs_db_create((ov_vocs_db_config){0});
    testrun(db);

    testrun(create_test_db(db));

    testrun(ov_vocs_db_create_entity(
        db, OV_VOCS_DB_ROLE, "admin", OV_VOCS_DB_SCOPE_DOMAIN, "domain1"));

    testrun(ov_vocs_db_create_entity(
        db, OV_VOCS_DB_ROLE, "admin", OV_VOCS_DB_SCOPE_DOMAIN, "domain2"));

    testrun(ov_vocs_db_create_entity(
        db, OV_VOCS_DB_ROLE, "admin", OV_VOCS_DB_SCOPE_PROJECT, "project1"));

    testrun(ov_vocs_db_add_domain_admin(db, "domain1", "user11"));
    testrun(ov_vocs_db_add_domain_admin(db, "domain2", "user11"));
    testrun(ov_vocs_db_add_domain_admin(db, "domain1", "user31"));

    testrun(ov_vocs_db_authorize_domain_admin(db, "user11", "domain1"));
    testrun(ov_vocs_db_authorize_domain_admin(db, "user11", "domain2"));
    testrun(ov_vocs_db_authorize_domain_admin(db, "user31", "domain1"));

    testrun(ov_vocs_db_add_project_admin(db, "project1", "user12"));
    testrun(ov_vocs_db_add_project_admin(db, "project1", "user21"));
    testrun(ov_vocs_db_add_project_admin(db, "project2", "user21"));

    testrun(ov_vocs_db_authorize_project_admin(db, "user12", "project1"));
    testrun(ov_vocs_db_authorize_project_admin(db, "user21", "project1"));
    testrun(ov_vocs_db_authorize_project_admin(db, "user21", "project2"));
    testrun(ov_vocs_db_authorize_project_admin(db, "user11", "project1"));
    testrun(ov_vocs_db_authorize_project_admin(db, "user31", "project1"));
    testrun(ov_vocs_db_authorize_project_admin(db, "user11", "project2"));

    // ov_vocs_db_dump(stdout, db);

    testrun(NULL == ov_vocs_db_free(db));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_vocs_db_get_layout() {

    ov_vocs_db *db = ov_vocs_db_create((ov_vocs_db_config){0});
    testrun(db);

    testrun(create_test_db(db));

    // no layout set

    ov_json_value *layout = ov_vocs_db_get_layout(db, "role11");
    testrun(layout);
    testrun(ov_json_object_is_empty(layout));
    layout = ov_json_value_free(layout);

    // set some layout
    layout = ov_json_object();
    ov_json_object_set(layout, "loop11", ov_json_number(1));
    ov_json_object_set(layout, "loop21", ov_json_number(2));
    ov_json_object_set(layout, "loop31", ov_json_number(3));
    testrun(ov_vocs_db_set_layout(db, "role11", layout));
    layout = ov_json_value_free(layout);

    layout = ov_vocs_db_get_layout(db, "role11");
    testrun(layout);
    testrun(!ov_json_object_is_empty(layout));
    testrun(3 == ov_json_object_count(layout));
    testrun(1 == ov_json_number_get(ov_json_get(layout, "/loop11")));
    testrun(2 == ov_json_number_get(ov_json_get(layout, "/loop21")));
    testrun(3 == ov_json_number_get(ov_json_get(layout, "/loop31")));
    layout = ov_json_value_free(layout);

    // loop unknown

    testrun(!ov_vocs_db_get_layout(db, "unknown"));

    testrun(NULL == ov_vocs_db_free(db));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_vocs_db_set_layout() {

    ov_vocs_db *db = ov_vocs_db_create((ov_vocs_db_config){0});
    testrun(db);

    testrun(create_test_db(db));

    ov_json_value *layout = ov_json_object();
    ov_json_object_set(layout, "loop11", ov_json_number(1));
    ov_json_object_set(layout, "loop21", ov_json_number(2));
    ov_json_object_set(layout, "loop31", ov_json_number(3));
    testrun(!ov_vocs_db_set_layout(db, "unknown", layout));
    testrun(ov_vocs_db_set_layout(db, "role11", layout));

    layout = ov_json_value_free(layout);

    testrun(NULL == ov_vocs_db_free(db));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int check_update_users() {

    ov_vocs_db *db = ov_vocs_db_create((ov_vocs_db_config){0});
    testrun(db);

    testrun(create_test_db(db));

    ov_json_value *parent = ov_dict_get(db->index.projects, "project1");
    testrun(parent);

    ov_json_value *val = ov_json_object_get(parent, OV_KEY_USERS);
    testrun(val);
    testrun(3 == ov_json_object_count(val));
    testrun(ov_json_object_get(val, "user11"));
    testrun(ov_json_object_get(val, "user12"));
    testrun(ov_json_object_get(val, "user13"));

    ov_json_value *users = ov_json_object();
    ov_json_value *user1 = ov_json_object();
    ov_json_value *user2 = ov_json_object();
    ov_json_value *user3 = ov_json_object();
    testrun(ov_json_object_set(users, "user11", user1)); // existing user
    testrun(ov_json_object_set(users, "user12", user2)); // existing user
    testrun(ov_json_object_set(users, "user14", user3)); // new user

    testrun(!ov_dict_get(db->index.users, "user14"));
    testrun(ov_dict_get(db->index.users, "user11"));
    testrun(ov_dict_get(db->index.users, "user12"));
    testrun(ov_dict_get(db->index.users, "user13"));

    testrun(update_users(db, parent, users));

    testrun(val == ov_json_object_get(parent, OV_KEY_USERS));
    testrun(3 == ov_json_object_count(val));
    testrun(ov_json_object_get(val, "user11"));
    testrun(ov_json_object_get(val, "user12"));
    testrun(ov_json_object_get(val, "user14"));

    testrun(ov_dict_get(db->index.users, "user14"));
    testrun(ov_dict_get(db->index.users, "user11"));
    testrun(ov_dict_get(db->index.users, "user12"));
    testrun(!ov_dict_get(db->index.users, "user13"));
    testrun(!ov_dict_get(db->index.users, "user15"));

    users = ov_json_value_free(users);
    users = ov_json_object();
    user1 = ov_json_object();
    testrun(ov_json_object_set(users, "user15", user1)); // new user

    testrun(update_users(db, parent, users));

    testrun(val == ov_json_object_get(parent, OV_KEY_USERS));
    testrun(1 == ov_json_object_count(val));
    testrun(ov_json_object_get(val, "user15"));

    testrun(!ov_dict_get(db->index.users, "user14"));
    testrun(!ov_dict_get(db->index.users, "user11"));
    testrun(!ov_dict_get(db->index.users, "user12"));
    testrun(!ov_dict_get(db->index.users, "user13"));
    testrun(ov_dict_get(db->index.users, "user15"));
    users = ov_json_value_free(users);

    testrun(NULL == ov_vocs_db_free(db));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_vocs_db_get_entity_domain() {

    ov_vocs_db *db = ov_vocs_db_create((ov_vocs_db_config){0});
    testrun(db);

    testrun(create_test_db(db));

    ov_json_value *out =
        ov_vocs_db_get_entity_domain(db, OV_VOCS_DB_USER, "user11");
    testrun(out);
    testrun(0 ==
            strcmp("domain1",
                   ov_json_string_get(ov_json_object_get(out, OV_KEY_DOMAIN))));
    testrun(0 == strcmp("project1",
                        ov_json_string_get(
                            ov_json_object_get(out, OV_KEY_PROJECT))));
    ov_json_value_dump(stdout, out);
    out = ov_json_value_free(out);

    out = ov_vocs_db_get_entity_domain(db, OV_VOCS_DB_USER, "user1");
    testrun(out);
    testrun(0 ==
            strcmp("domain1",
                   ov_json_string_get(ov_json_object_get(out, OV_KEY_DOMAIN))));
    ov_json_value_dump(stdout, out);
    out = ov_json_value_free(out);

    out = ov_vocs_db_get_entity_domain(db, OV_VOCS_DB_USER, "user2");
    testrun(out);
    testrun(0 ==
            strcmp("domain2",
                   ov_json_string_get(ov_json_object_get(out, OV_KEY_DOMAIN))));
    ov_json_value_dump(stdout, out);
    out = ov_json_value_free(out);

    out = ov_vocs_db_get_entity_domain(db, OV_VOCS_DB_ROLE, "role12");
    testrun(out);
    testrun(0 ==
            strcmp("domain1",
                   ov_json_string_get(ov_json_object_get(out, OV_KEY_DOMAIN))));
    testrun(0 == strcmp("project1",
                        ov_json_string_get(
                            ov_json_object_get(out, OV_KEY_PROJECT))));
    ov_json_value_dump(stdout, out);
    out = ov_json_value_free(out);

    testrun(NULL == ov_vocs_db_free(db));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

const char *vocs_db_permission =
    "{"
    "\"localhost\" : "
    "{"
    "\"id\" : \"localhost\","
    "\"projects\" : "
    "{"
    "\"project1\" :"
    "{"
    "\"id\" : \"project1\","
    "\"loops\" :"
    "{"
    "\"loop1\" : "
    "{"
    "\"id\" : \"loop1\","
    "\"sip\" : "
    "{"
    "\"whitelist\": "
    "["
    "{"
    "\"caller\" : \"1\","
    "\"callee\" : \"2\""
    "},"
    "]"
    "}"
    "},"
    "\"loop2\" : "
    "{"
    "\"id\" : \"loop2\","
    "\"sip\" : "
    "{"
    "\"whitelist\": "
    "["
    "{"
    "\"caller\" : \"2\","
    "\"callee\" : \"2\""
    "},"
    "]"
    "}"
    "}"

    "}"
    "}"
    "},"
    "\"loops\" : "
    "{"
    "\"loop3\" :"
    "{"
    "\"id\" : \"loop3\","
    "\"sip\" : "
    "{"
    "\"whitelist\": "
    "["
    "{"
    "\"caller\" : \"1\","
    "\"callee\" : \"2\""
    "},"
    "]"
    "}"
    "}"
    "}"
    "}"
    "}";

/*----------------------------------------------------------------------------*/

int check_update_sip_permissions() {

    ov_json_value *val = NULL;

    struct userdata userdata = (struct userdata){0};

    ov_event_trigger *trigger =
        ov_event_trigger_create((ov_event_trigger_config){0});

    testrun(ov_event_trigger_register_listener(
        trigger,
        "VOCS",
        (ov_event_trigger_data){
            .userdata = &userdata, .process = dummy_process_trigger}));

    ov_vocs_db *db = ov_vocs_db_create((ov_vocs_db_config){.trigger = trigger});
    testrun(db);

    testrun(create_test_db(db));

    // add some SIP permission to loop

    ov_json_value *loop = ov_vocs_db_get_entity(db, OV_VOCS_DB_LOOP, "loop11");
    testrun(loop);

    ov_json_value *out = NULL;
    ov_json_value *perm1 =
        ov_json_decode("{\"callee\":\"lee\",\"caller\":\"ler\"}");
    ov_json_value *perm2 = ov_json_decode("{\"callee\":\"lee\"}");
    ov_json_value *perm3 = ov_json_decode("{\"caller\":\"ler\"}");
    ov_json_value *arr = ov_json_array();
    ov_json_array_push(arr, perm1);
    ov_json_array_push(arr, perm2);
    ov_json_array_push(arr, perm3);
    ov_json_value *whitelist = ov_json_object();
    ov_json_object_set(whitelist, OV_KEY_WHITELIST, arr);
    ov_json_object_set(loop, OV_KEY_SIP, whitelist);

    testrun(ov_vocs_db_update_entity_item(
        db, OV_VOCS_DB_LOOP, "loop11", loop, NULL));

    // check trigger
    testrun(userdata.msg);
    arr = (ov_json_value *)ov_json_get(
        userdata.msg, "/parameter/processing/loop11/permit");
    testrun(ov_json_is_array(arr));
    testrun(3 == ov_json_array_count(arr));
    loop = ov_json_value_free(loop);

    // revoke permissions
    loop = ov_vocs_db_get_entity(db, OV_VOCS_DB_LOOP, "loop11");
    testrun(loop);
    arr = (ov_json_value *)ov_json_get(loop, "/sip/whitelist");
    testrun(arr);
    out = ov_json_array_pop(arr);
    out = ov_json_value_free(out);
    out = ov_json_array_pop(arr);
    out = ov_json_value_free(out);

    testrun(ov_vocs_db_update_entity_item(
        db, OV_VOCS_DB_LOOP, "loop11", loop, NULL));

    // check trigger
    testrun(userdata.msg);
    arr = (ov_json_value *)ov_json_get(
        userdata.msg, "/parameter/processing/loop11/revoke");
    testrun(2 == ov_json_array_count(arr));
    val = ov_json_array_get(arr, 1);
    testrun(val);
    testrun(ov_json_object_get(val, OV_KEY_CALLEE));
    testrun(1 == ov_json_object_count(val));
    val = ov_json_array_get(arr, 2);
    testrun(val);
    testrun(ov_json_object_get(val, OV_KEY_CALLER));
    testrun(1 == ov_json_object_count(val));
    loop = ov_json_value_free(loop);
    userdata.msg = ov_json_value_free(userdata.msg);

    // add some SIP permission to project

    val = ov_json_decode(vocs_db_permission);
    testrun(val);
    testrun(ov_vocs_db_inject(db, OV_VOCS_DB_TYPE_AUTH, val));

    ov_json_value *project =
        ov_vocs_db_get_entity(db, OV_VOCS_DB_PROJECT, "project1");
    testrun(project);
    ov_json_value *loops = ov_json_object_get(project, OV_KEY_LOOPS);
    testrun(loops);
    loop = ov_json_object_get(loops, "loop1");
    testrun(loop);

    out = NULL;
    perm1 = ov_json_decode("{\"callee\":\"lee\",\"caller\":\"ler\"}");
    arr = ov_json_array();
    ov_json_array_push(arr, perm1);
    whitelist = ov_json_object();
    ov_json_object_set(whitelist, OV_KEY_WHITELIST, arr);
    ov_json_object_set(loop, OV_KEY_SIP, whitelist);

    testrun(ov_vocs_db_update_entity_item(
        db, OV_VOCS_DB_PROJECT, "project1", project, NULL));

    // check trigger
    testrun(userdata.msg);
    val = (ov_json_value *)ov_json_get(
        userdata.msg, "/parameter/processing/loop1/permit");
    testrun(ov_json_is_array(val));
    val = ov_json_array_get(val, 1);
    out = ov_json_object_get(val, OV_KEY_CALLEE);
    testrun(out);
    testrun(0 == strcmp("lee", ov_json_string_get(out)));
    out = ov_json_object_get(val, OV_KEY_CALLER);
    testrun(out);
    testrun(0 == strcmp("ler", ov_json_string_get(out)));
    val = (ov_json_value *)ov_json_get(
        userdata.msg, "/parameter/processing/loop2");
    testrun(val);
    testrun(ov_json_object_is_empty(val));

    val = (ov_json_value *)ov_json_get(
        userdata.msg, "/parameter/processing/loop1/revoke");
    testrun(ov_json_is_array(val));
    val = ov_json_array_get(val, 1);
    out = ov_json_object_get(val, OV_KEY_CALLEE);
    testrun(out);
    testrun(0 == strcmp("2", ov_json_string_get(out)));
    out = ov_json_object_get(val, OV_KEY_CALLER);
    testrun(out);
    testrun(0 == strcmp("1", ov_json_string_get(out)));
    project = ov_json_value_free(project);

    project = ov_vocs_db_get_entity(db, OV_VOCS_DB_PROJECT, "project1");
    testrun(project);
    val = (ov_json_value *)ov_json_get(project, "/loops/loop1/sip/whitelist");
    val = ov_json_array_get(val, 1);
    testrun(val);
    out = ov_json_object_get(val, OV_KEY_CALLEE);
    testrun(out);
    testrun(0 == strcmp("lee", ov_json_string_get(out)));
    out = ov_json_object_get(val, OV_KEY_CALLER);
    testrun(out);
    testrun(0 == strcmp("ler", ov_json_string_get(out)));
    project = ov_json_value_free(project);

    // add to project without loops
    testrun(ov_vocs_db_create_entity(db,
                                     OV_VOCS_DB_PROJECT,
                                     "project2",
                                     OV_VOCS_DB_SCOPE_DOMAIN,
                                     "localhost"));

    project = ov_vocs_db_get_entity(db, OV_VOCS_DB_PROJECT, "project2");
    testrun(project);
    loops = ov_json_object();
    ov_json_object_set(project, OV_KEY_LOOPS, loops);
    loop = ov_json_object();
    ov_json_object_set(loops, "loop23", loop);
    perm1 = ov_json_decode("{\"callee\":\"lee\",\"caller\":\"ler\"}");
    arr = ov_json_array();
    ov_json_array_push(arr, perm1);
    whitelist = ov_json_object();
    ov_json_object_set(whitelist, OV_KEY_WHITELIST, arr);
    ov_json_object_set(loop, OV_KEY_SIP, whitelist);

    testrun(ov_vocs_db_update_entity_item(
        db, OV_VOCS_DB_PROJECT, "project2", project, NULL));

    project = ov_json_value_free(project);

    // check trigger
    testrun(userdata.msg);
    val = (ov_json_value *)ov_json_get(
        userdata.msg, "/parameter/processing/loop23/permit");
    testrun(ov_json_is_array(val));
    val = ov_json_array_get(val, 1);
    out = ov_json_object_get(val, OV_KEY_CALLEE);
    testrun(out);
    testrun(0 == strcmp("lee", ov_json_string_get(out)));
    out = ov_json_object_get(val, OV_KEY_CALLER);
    testrun(out);
    testrun(0 == strcmp("ler", ov_json_string_get(out)));

    // check permission

    project = ov_vocs_db_get_entity(db, OV_VOCS_DB_PROJECT, "project2");
    testrun(project);
    val = (ov_json_value *)ov_json_get(project, "/loops/loop23/sip/whitelist");
    val = ov_json_array_get(val, 1);
    testrun(val);
    out = ov_json_object_get(val, OV_KEY_CALLEE);
    testrun(out);
    testrun(0 == strcmp("lee", ov_json_string_get(out)));
    out = ov_json_object_get(val, OV_KEY_CALLER);
    testrun(out);
    testrun(0 == strcmp("ler", ov_json_string_get(out)));
    project = ov_json_value_free(project);

    // check oldloop not contained in new loops

    project = ov_vocs_db_get_entity(db, OV_VOCS_DB_PROJECT, "project1");
    testrun(project);
    loops = ov_json_object_get(project, "loops");
    testrun(loops);
    testrun(2 == ov_json_object_count(loops));
    ov_json_object_del(loops, "loop1");
    testrun(1 == ov_json_object_count(loops));
    // change permission of loop2
    loop = ov_json_object_get(loops, "loop2");
    perm1 = ov_json_decode("{\"callee\":\"lee\",\"caller\":\"ler\"}");
    arr = ov_json_array();
    ov_json_array_push(arr, perm1);
    whitelist = ov_json_object();
    ov_json_object_set(whitelist, OV_KEY_WHITELIST, arr);
    ov_json_object_set(loop, OV_KEY_SIP, whitelist);

    testrun(ov_vocs_db_update_entity_item(
        db, OV_VOCS_DB_PROJECT, "project1", project, NULL));

    project = ov_json_value_free(project);

    // check trigger
    testrun(userdata.msg);
    // ov_log_debug("%s", ov_json_encode(userdata.msg));
    val = (ov_json_value *)ov_json_get(
        userdata.msg, "/parameter/processing/loop1/revoke");
    testrun(ov_json_is_array(val));
    val = ov_json_array_get(val, 1);
    out = ov_json_object_get(val, OV_KEY_CALLEE);
    testrun(out);
    testrun(0 == strcmp("lee", ov_json_string_get(out)));
    out = ov_json_object_get(val, OV_KEY_CALLER);
    testrun(out);
    testrun(0 == strcmp("ler", ov_json_string_get(out)));
    val = (ov_json_value *)ov_json_get(
        userdata.msg, "/parameter/processing/loop2/revoke");
    testrun(ov_json_is_array(val));
    val = ov_json_array_get(val, 1);
    out = ov_json_object_get(val, OV_KEY_CALLEE);
    testrun(out);
    testrun(0 == strcmp("2", ov_json_string_get(out)));
    out = ov_json_object_get(val, OV_KEY_CALLER);
    testrun(out);
    testrun(0 == strcmp("2", ov_json_string_get(out)));
    val = (ov_json_value *)ov_json_get(
        userdata.msg, "/parameter/processing/loop2/permit");
    testrun(ov_json_is_array(val));
    val = ov_json_array_get(val, 1);
    out = ov_json_object_get(val, OV_KEY_CALLEE);
    testrun(out);
    testrun(0 == strcmp("lee", ov_json_string_get(out)));
    out = ov_json_object_get(val, OV_KEY_CALLER);
    testrun(out);
    testrun(0 == strcmp("ler", ov_json_string_get(out)));
    project = ov_json_value_free(project);

    // add some permission at domain level

    ov_json_value *domain =
        ov_vocs_db_get_entity(db, OV_VOCS_DB_DOMAIN, "localhost");
    testrun(domain);
    loops = ov_json_object_get(domain, OV_KEY_LOOPS);
    testrun(loops);
    loop = ov_json_object_get(loops, "loop3");
    testrun(loop);

    out = NULL;
    perm1 = ov_json_decode("{\"callee\":\"lee\",\"caller\":\"ler\"}");
    arr = ov_json_array();
    ov_json_array_push(arr, perm1);
    whitelist = ov_json_object();
    ov_json_object_set(whitelist, OV_KEY_WHITELIST, arr);
    ov_json_object_set(loop, OV_KEY_SIP, whitelist);

    testrun(ov_vocs_db_update_entity_item(
        db, OV_VOCS_DB_DOMAIN, "localhost", domain, NULL));

    testrun(userdata.msg);
    val = (ov_json_value *)ov_json_get(
        userdata.msg, "/parameter/processing/loop3/revoke");
    testrun(ov_json_is_array(val));
    val = ov_json_array_get(val, 1);
    out = ov_json_object_get(val, OV_KEY_CALLEE);
    testrun(out);
    testrun(0 == strcmp("2", ov_json_string_get(out)));
    out = ov_json_object_get(val, OV_KEY_CALLER);
    testrun(out);
    testrun(0 == strcmp("1", ov_json_string_get(out)));
    val = (ov_json_value *)ov_json_get(
        userdata.msg, "/parameter/processing/loop3/permit");
    testrun(ov_json_is_array(val));
    val = ov_json_array_get(val, 1);
    out = ov_json_object_get(val, OV_KEY_CALLEE);
    testrun(out);
    testrun(0 == strcmp("lee", ov_json_string_get(out)));
    out = ov_json_object_get(val, OV_KEY_CALLER);
    testrun(out);
    testrun(0 == strcmp("ler", ov_json_string_get(out)));
    domain = ov_json_value_free(domain);
    userdata.msg = ov_json_value_free(userdata.msg);

    testrun(NULL == ov_vocs_db_free(db));
    testrun(NULL == ov_event_trigger_free(trigger));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int check_revoke_call_in_whitelist() {

    ov_json_value *val = NULL;

    ov_json_value *old_list = ov_json_decode(
        "["
        "{\"caller\":\"ler\"},"
        "{\"callee\":\"lee\"},"
        "{\"caller\":\"ler\", \"callee\":\"lee\"}"
        "]");

    ov_json_value *new_list = ov_json_decode(
        "["
        "{\"caller\":\"ler\", \"callee\":\"lee\"},"
        "{\"caller\":\"ler\", \"callee\":\"other\"}"
        "]");

    ov_json_value *out = ov_json_object();

    testrun(revoke_calls(out, old_list, new_list));
    testrun(2 == ov_json_array_count(ov_json_object_get(out, OV_KEY_REVOKE)));
    out = ov_json_value_free(out);
    old_list = ov_json_value_free(old_list);
    new_list = ov_json_value_free(new_list);

    old_list = ov_json_decode(
        "["
        "{\"caller\":\"ler\"},"
        "{\"callee\":\"lee\"},"
        "{\"caller\":\"ler\", \"callee\":\"lee\"}"
        "]");

    new_list = ov_json_decode(
        "["
        "{\"caller\":\"ler\"},"
        "{\"callee\":\"lee\"},"
        "{\"caller\":\"ler\", \"callee\":\"lee\"},"
        "{\"caller\":\"ler\", \"callee\":\"other\"}"
        "]");

    out = ov_json_object();

    testrun(revoke_calls(out, old_list, new_list));
    testrun(0 == ov_json_array_count(ov_json_object_get(out, OV_KEY_REVOKE)));
    out = ov_json_value_free(out);
    old_list = ov_json_value_free(old_list);
    new_list = ov_json_value_free(new_list);

    old_list = ov_json_decode(
        "["
        "{\"caller\":\"ler\"},"
        "{\"callee\":\"lee\"},"
        "{\"caller\":\"ler\", \"callee\":\"lee\"}"
        "]");

    new_list = ov_json_decode(
        "["
        "{\"caller\":\"ler\"},"
        "{\"caller\":\"ler\", \"callee\":\"lee\"},"
        "{\"caller\":\"ler\", \"callee\":\"other\"}"
        "]");

    out = ov_json_object();

    testrun(revoke_calls(out, old_list, new_list));
    testrun(1 == ov_json_array_count(ov_json_object_get(out, OV_KEY_REVOKE)));
    val = ov_json_array_get(ov_json_object_get(out, OV_KEY_REVOKE), 1);
    testrun(ov_json_object_get(val, OV_KEY_CALLEE));
    testrun(0 ==
            strcmp("lee",
                   ov_json_string_get(ov_json_object_get(val, OV_KEY_CALLEE))));
    out = ov_json_value_free(out);
    old_list = ov_json_value_free(old_list);
    new_list = ov_json_value_free(new_list);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CLUSTER                                                    #CLUSTER
 *
 *      ------------------------------------------------------------------------
 */

int all_tests() {

    testrun_init();

    testrun_test(test_ov_vocs_db_create);
    testrun_test(test_ov_vocs_db_cast);
    testrun_test(test_ov_vocs_db_free);

    testrun_test(test_ov_vocs_db_dump);

    testrun_test(test_ov_vocs_db_create_entity);
    testrun_test(test_ov_vocs_db_delete_entity);

    testrun_test(test_ov_vocs_db_update_entity_key);
    testrun_test(test_ov_vocs_db_delete_entity_key);

    testrun_test(test_ov_vocs_db_verify_entity_item);
    testrun_test(test_ov_vocs_db_update_entity_item);

    testrun_test(test_ov_vocs_db_inject);
    testrun_test(test_ov_vocs_db_eject);

    testrun_test(test_ov_vocs_db_set_password);
    testrun_test(test_ov_vocs_db_authenticate);
    testrun_test(test_ov_vocs_db_authorize);
    testrun_test(test_ov_vocs_db_get_permission);

    testrun_test(test_ov_vocs_db_authorize_project_admin);
    testrun_test(test_ov_vocs_db_authorize_domain_admin);
    testrun_test(test_ov_vocs_db_get_admin_domains);
    testrun_test(test_ov_vocs_db_get_admin_projects);

    testrun_test(test_ov_vocs_db_get_user_roles);
    testrun_test(test_ov_vocs_db_get_role_loops);
    testrun_test(test_ov_vocs_db_get_user_role_loops);

    testrun_test(test_ov_vocs_db_set_state);
    testrun_test(test_ov_vocs_db_get_state);
    testrun_test(test_ov_vocs_db_set_volume);
    testrun_test(test_ov_vocs_db_get_volume);

    testrun_test(test_ov_vocs_db_get_parent);

    testrun_test(check_admin_role);

    testrun_test(test_ov_vocs_db_get_layout);
    testrun_test(test_ov_vocs_db_set_layout);

    testrun_test(check_update_users);

    testrun_test(test_ov_vocs_db_get_entity_domain);

    testrun_test(check_revoke_call_in_whitelist);
    testrun_test(check_update_sip_permissions);

    return testrun_counter;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST EXECUTION                                                  #EXEC
 *
 *      ------------------------------------------------------------------------
 */

testrun_run(all_tests);
