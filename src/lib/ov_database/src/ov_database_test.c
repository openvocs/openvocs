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

        @author         Michael J. Beer, DLR/GSOC

        ------------------------------------------------------------------------
*/

#include "ov_database.c"
#include <ov_base/ov_config_keys.h>
#include <ov_test/ov_test.h>

/*----------------------------------------------------------------------------*/

static bool dbi_equals(ov_database_info const i1, ov_database_info const i2) {

    return (i1.port == i2.port) && (ov_string_equal(i1.dbname, i2.dbname) &&
                                    ov_string_equal(i1.host, i2.host) &&
                                    ov_string_equal(i1.user, i2.user) &&
                                    ov_string_equal(i1.password, i2.password) &&
                                    ov_string_equal(i1.type, i2.type));
}

/*----------------------------------------------------------------------------*/

static ov_json_value *json_from_string(char const *str) {

    return ov_json_value_from_string(str, ov_string_len(str));
}

/*----------------------------------------------------------------------------*/

static int test_ov_database_info_from_json() {

    ov_database_info dbi = {0};

    testrun(dbi_equals(ov_database_info_from_json(0), dbi));

    ov_json_value *jval =
        json_from_string("{\"" OV_KEY_HOST "\":\"krambambuli\"}");

    testrun(!dbi_equals(ov_database_info_from_json(jval), dbi));

    dbi.host = "krambambuli";
    testrun(dbi_equals(ov_database_info_from_json(jval), dbi));

    jval = ov_json_value_free(jval);

    jval = json_from_string("{\"" OV_KEY_HOST
                            "\":\"krambambuli\","
                            "\"" OV_KEY_PORT "\":2144}");

    testrun(!dbi_equals(ov_database_info_from_json(jval), dbi));

    dbi.port = 2144;
    testrun(dbi_equals(ov_database_info_from_json(jval), dbi));

    jval = ov_json_value_free(jval);

    jval = json_from_string("{\"" OV_KEY_HOST
                            "\":\"krambambuli\","
                            "\"" OV_KEY_PORT "\":2144, \"" OV_KEY_USER
                            "\":\"arbol\","
                            "\"" OV_KEY_PASSWORD "\":\"braga\", \"" OV_KEY_DB
                            "\":\"db2\","
                            "\"" OV_KEY_TYPE "\":\"baalburga\"}");

    testrun(!dbi_equals(ov_database_info_from_json(jval), dbi));

    dbi.user = "arbol";
    dbi.password = "braga";
    dbi.dbname = "db2";
    dbi.type = "baalburga";

    testrun(dbi_equals(ov_database_info_from_json(jval), dbi));

    jval = ov_json_value_free(jval);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_database_info_to_json() {

    ov_json_value *jval = ov_database_info_to_json((ov_database_info){0});
    testrun(0 != jval);

    testrun(
        dbi_equals((ov_database_info){0}, ov_database_info_from_json(jval)));

    jval = ov_json_value_free(jval);

    ov_database_info dbi = {.host = "1.2.3.1",
                            .port = 3214,
                            .dbname = "baldurs tor",
                            .type = "rechen",
                            .user = "wasser",
                            .password = "laib"};

    jval = ov_database_info_to_json(dbi);

    ov_database_info dbi2 = ov_database_info_from_json(jval);

    testrun(dbi_equals(dbi, dbi2));

    jval = ov_json_value_free(jval);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

#ifdef OV_TEST_USE_POSTGRES

static ov_database_info database_info(char const *dbname) {

    return (ov_database_info){

        .type = OV_DB_POSTGRES,
        .host = "127.0.0.1",
        .port = 5432,
        .dbname = OV_OR_DEFAULT(dbname, "test123"),
        .user = "openvocs",
        .password = "secret",

    };
}

#else

static ov_database_info database_info(char const *fname) {

    return (ov_database_info){

        .type = OV_DB_SQLITE,
        .dbname = OV_OR_DEFAULT(fname, OV_DB_SQLITE_MEMORY),

    };
}

#endif

/*----------------------------------------------------------------------------*/

static bool closedummy(ov_database *self) {

    UNUSED(self);
    return true;
}

/*----------------------------------------------------------------------------*/

static ov_database *testconnector(ov_database_info info) {

    UNUSED(info);
    ov_database *db = calloc(1, sizeof(ov_database));

    db->close = closedummy;

    return db;
}

/*----------------------------------------------------------------------------*/

static int test_ov_database_register_connector() {

    ov_database_info info = {
        .type = "testc",
    };

    testrun(0 == ov_database_connect(info));

    testrun(!ov_database_connector_register(0, 0));

    testrun(!ov_database_connector_register("testc", 0));
    testrun(0 == ov_database_connect(info));

    testrun(!ov_database_connector_register(0, testconnector));
    testrun(0 == ov_database_connect(info));

    testrun(ov_database_connector_register("testc", testconnector));
    ov_database *db = ov_database_connect(info);

    testrun(0 != db);

    db = ov_database_close(db);
    testrun(0 == db);

    db = ov_database_connect_singleton(info);
    ov_database *db2 = ov_database_connect_singleton(info);

    testrun(db == db2);
    testrun(0 != db);

    db = ov_database_close(db);
    db2 = ov_database_close(db2);

    testrun(0 == db);
    testrun(db == db2);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_database_connect() {

    ov_database_info info = database_info(0);

    ov_database *db = ov_database_connect(info);

    if (0 != db) {
        fprintf(stderr, "Connected\n");
    } else {
        fprintf(stderr, "Could not connect\n");
    }

    db = ov_database_close(db);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_database_connect_singleton() {

    ov_database *ov_database_connect_singleton(ov_database_info info);

    ov_database_info info = database_info(0);

    ov_database *db = ov_database_connect_singleton(info);
    ov_database *db2 = ov_database_connect_singleton(info);

    testrun(0 != db);
    testrun(0 != db2);

    testrun(db == db2);

    db = ov_database_close(db);
    db2 = ov_database_close(db2);

    testrun(0 == db);
    testrun(0 == db2);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_database_query() {

    ov_database_info info = database_info(0);

    ov_database *db = ov_database_connect(info);

    if (0 != db) {
        fprintf(stderr, "Connected\n");
    } else {
        fprintf(stderr, "Could not connect\n");
    }

    ov_result table_res = ov_database_query(db,
                                            "CREATE TABLE IF NOT EXISTS "
                                            "events (name VARCHAR(150), "
                                            "event VARCHAR(150));",
                                            0);

    if (OV_ERROR_NOERROR == table_res.error_code) {

        fprintf(stderr, "Ok\n");

    } else {

        fprintf(stderr,
                "Failure table: %" PRIu64 " %s\n",
                table_res.error_code,
                table_res.message);
    }

    testrun(OV_ERROR_NOERROR == table_res.error_code);

    ov_result_clear(&table_res);

    ov_result entry_inserted = ov_database_query(db,
                                                 "INSERT INTO events "
                                                 " (name, event) VALUES "
                                                 "('konrad', 'toblerone');",
                                                 0);

    if (OV_ERROR_NOERROR == entry_inserted.error_code) {

        fprintf(stderr, "Entry inserted: Ok\n");

    } else {

        fprintf(stderr,
                "Failure inserting entry: %" PRIu64 " %s\n",
                entry_inserted.error_code,
                entry_inserted.message);
    }

    testrun(OV_ERROR_NOERROR == entry_inserted.error_code);
    ov_result_clear(&entry_inserted);

    ov_json_value *jresult = 0;

    ov_result select_res =
        ov_database_query(db, "SELECT name, event FROM events", &jresult);

    if (OV_ERROR_NOERROR == select_res.error_code) {

        fprintf(stderr, "Select: Ok\n");

    } else {

        fprintf(stderr,
                "Failure selecting: %" PRIu64 " %s\n",
                select_res.error_code,
                select_res.message);
    }

    testrun(0 != jresult);
    testrun(OV_ERROR_NOERROR == select_res.error_code);

    ov_result_clear(&select_res);

    ov_json_value_dump(stderr, jresult);

    ov_json_value_free(jresult);

    db = ov_database_close(db);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int teardown() {

    ov_teardown();
    return 1;
}

/*----------------------------------------------------------------------------*/

OV_TEST_RUN("ov_database",
            test_ov_database_info_from_json,
            test_ov_database_info_to_json,
            test_ov_database_register_connector,
            test_ov_database_connect,
            test_ov_database_connect_singleton,
            test_ov_database_query,
            teardown);
