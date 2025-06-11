/***
        ------------------------------------------------------------------------

        Copyright (c) 2021 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_vocs_connection_test.c
        @author         Markus Töpfer

        @date           2021-10-09


        ------------------------------------------------------------------------
*/
#include "ov_vocs_connection.c"
#include <ov_test/testrun.h>

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

int test_ov_vocs_connection_clear() {

    ov_vocs_connection conn = {.socket = 1,
                               .local.host = "127.0.0.1",
                               .local.port = 1234,
                               .remote.host = "127.0.0.1",
                               .remote.port = 2345,
                               .data = ov_json_object()};

    testrun(!ov_vocs_connection_clear(NULL));
    testrun(ov_vocs_connection_clear(&conn));
    testrun(-1 == conn.socket);
    testrun(0 == conn.local.port);
    testrun(0 == conn.remote.port);
    testrun(NULL == conn.data);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_vocs_connection_is_authenticated() {

    ov_vocs_connection conn = {0};
    testrun(!ov_vocs_connection_is_authenticated(&conn));
    testrun(!ov_vocs_connection_is_authenticated(NULL));

    testrun(ov_vocs_connection_set_user(&conn, "user2"));
    testrun(ov_vocs_connection_is_authenticated(&conn));

    testrun(ov_vocs_connection_clear(&conn));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_vocs_connection_is_authorized() {

    ov_vocs_connection conn = {0};
    testrun(!ov_vocs_connection_is_authorized(&conn));
    testrun(!ov_vocs_connection_is_authorized(NULL));

    testrun(ov_vocs_connection_set_role(&conn, "role1"));
    testrun(ov_vocs_connection_is_authorized(&conn));

    testrun(ov_vocs_connection_clear(&conn));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_vocs_connection_get_user() {

    ov_vocs_connection conn = {0};

    testrun(NULL == ov_vocs_connection_get_user(&conn));
    testrun(ov_vocs_connection_set_user(&conn, "user2"));
    const char *out = ov_vocs_connection_get_user(&conn);
    testrun(out);
    testrun(0 == strcmp(out, "user2"));

    testrun(ov_vocs_connection_clear(&conn));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_vocs_connection_set_user() {

    ov_vocs_connection conn = {0};

    testrun(!ov_vocs_connection_set_user(NULL, NULL));
    testrun(!ov_vocs_connection_set_user(&conn, NULL));
    testrun(!ov_vocs_connection_set_user(NULL, "user2"));

    testrun(ov_vocs_connection_set_user(&conn, "user2"));
    const char *out = ov_vocs_connection_get_user(&conn);
    testrun(out);
    testrun(0 == strcmp(out, "user2"));

    testrun(ov_vocs_connection_clear(&conn));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_vocs_connection_get_role() {

    ov_vocs_connection conn = {0};

    testrun(NULL == ov_vocs_connection_get_role(&conn));
    testrun(ov_vocs_connection_set_role(&conn, "user2"));
    const char *out = ov_vocs_connection_get_role(&conn);
    testrun(out);
    testrun(0 == strcmp(out, "user2"));

    testrun(ov_vocs_connection_clear(&conn));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_vocs_connection_set_role() {

    ov_vocs_connection conn = {0};

    testrun(!ov_vocs_connection_set_role(NULL, NULL));
    testrun(!ov_vocs_connection_set_role(&conn, NULL));
    testrun(!ov_vocs_connection_set_role(NULL, "user2"));

    testrun(ov_vocs_connection_set_role(&conn, "user2"));
    const char *out = ov_vocs_connection_get_role(&conn);
    testrun(out);
    testrun(0 == strcmp(out, "user2"));

    testrun(ov_vocs_connection_clear(&conn));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_vocs_connection_get_session() {

    ov_vocs_connection conn = {0};

    testrun(NULL == ov_vocs_connection_get_session(&conn));
    testrun(ov_vocs_connection_set_session(&conn, "user2"));
    const char *out = ov_vocs_connection_get_session(&conn);
    testrun(out);
    testrun(0 == strcmp(out, "user2"));

    testrun(ov_vocs_connection_clear(&conn));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_vocs_connection_set_session() {

    ov_vocs_connection conn = {0};

    testrun(!ov_vocs_connection_set_session(NULL, NULL));
    testrun(!ov_vocs_connection_set_session(&conn, NULL));
    testrun(!ov_vocs_connection_set_session(NULL, "user2"));

    testrun(ov_vocs_connection_set_session(&conn, "user2"));
    const char *out = ov_vocs_connection_get_session(&conn);
    testrun(out);
    testrun(0 == strcmp(out, "user2"));

    testrun(ov_vocs_connection_clear(&conn));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_vocs_connection_get_client() {

    ov_vocs_connection conn = {0};

    testrun(NULL == ov_vocs_connection_get_client(&conn));
    testrun(ov_vocs_connection_set_client(&conn, "user2"));
    const char *out = ov_vocs_connection_get_client(&conn);
    testrun(out);
    testrun(0 == strcmp(out, "user2"));

    testrun(ov_vocs_connection_clear(&conn));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_vocs_connection_set_client() {

    ov_vocs_connection conn = {0};

    testrun(!ov_vocs_connection_set_client(NULL, NULL));
    testrun(!ov_vocs_connection_set_client(&conn, NULL));
    testrun(!ov_vocs_connection_set_client(NULL, "user2"));

    testrun(ov_vocs_connection_set_client(&conn, "user2"));
    const char *out = ov_vocs_connection_get_client(&conn);
    testrun(out);
    testrun(0 == strcmp(out, "user2"));

    testrun(ov_vocs_connection_clear(&conn));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_vocs_connection_get_loop_state() {

    ov_vocs_connection conn = {0};

    testrun(OV_VOCS_NONE == ov_vocs_connection_get_loop_state(&conn, "loop1"));
    testrun(ov_vocs_connection_set_loop_state(&conn, "loop1", OV_VOCS_RECV));
    testrun(ov_vocs_connection_set_loop_state(&conn, "loop2", OV_VOCS_SEND));
    testrun(ov_vocs_connection_set_loop_state(&conn, "loop3", OV_VOCS_RECV));
    testrun(ov_vocs_connection_set_loop_state(&conn, "loop4", OV_VOCS_NONE));
    testrun(OV_VOCS_RECV == ov_vocs_connection_get_loop_state(&conn, "loop1"));
    testrun(OV_VOCS_SEND == ov_vocs_connection_get_loop_state(&conn, "loop2"));
    testrun(OV_VOCS_RECV == ov_vocs_connection_get_loop_state(&conn, "loop3"));
    testrun(OV_VOCS_NONE == ov_vocs_connection_get_loop_state(&conn, "loop4"));

    testrun(ov_vocs_connection_clear(&conn));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_vocs_connection_set_loop_state() {

    ov_vocs_connection conn = {0};

    testrun(!ov_vocs_connection_set_loop_state(NULL, NULL, OV_VOCS_NONE));
    testrun(!ov_vocs_connection_set_loop_state(&conn, NULL, OV_VOCS_NONE));
    testrun(!ov_vocs_connection_set_loop_state(NULL, "loop1", OV_VOCS_NONE));

    testrun(ov_vocs_connection_set_loop_state(&conn, "loop1", OV_VOCS_NONE));
    testrun(ov_vocs_connection_set_loop_state(&conn, "loop2", OV_VOCS_RECV));
    testrun(ov_vocs_connection_set_loop_state(&conn, "loop3", OV_VOCS_SEND));

    testrun(OV_VOCS_NONE == ov_vocs_connection_get_loop_state(&conn, "loop1"));
    testrun(OV_VOCS_SEND == ov_vocs_connection_get_loop_state(&conn, "loop3"));
    testrun(OV_VOCS_RECV == ov_vocs_connection_get_loop_state(&conn, "loop2"));

    testrun(ov_vocs_connection_clear(&conn));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_vocs_connection_get_loop_volume() {

    ov_vocs_connection conn = {0};

    testrun(0 == ov_vocs_connection_get_loop_volume(&conn, "loop1"));
    testrun(ov_vocs_connection_set_loop_volume(&conn, "loop1", 1));
    testrun(ov_vocs_connection_set_loop_volume(&conn, "loop2", 2));
    testrun(ov_vocs_connection_set_loop_volume(&conn, "loop3", 3));
    testrun(ov_vocs_connection_set_loop_volume(&conn, "loop4", 4));
    testrun(1 == ov_vocs_connection_get_loop_volume(&conn, "loop1"));
    testrun(2 == ov_vocs_connection_get_loop_volume(&conn, "loop2"));
    testrun(3 == ov_vocs_connection_get_loop_volume(&conn, "loop3"));
    testrun(4 == ov_vocs_connection_get_loop_volume(&conn, "loop4"));

    testrun(ov_vocs_connection_clear(&conn));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_vocs_connection_set_loop_volume() {

    ov_vocs_connection conn = {0};

    testrun(!ov_vocs_connection_set_loop_volume(NULL, NULL, 0));
    testrun(!ov_vocs_connection_set_loop_volume(&conn, NULL, 0));
    testrun(!ov_vocs_connection_set_loop_volume(NULL, "loop1", 0));
    testrun(!ov_vocs_connection_set_loop_volume(&conn, "loop1", 101));

    testrun(ov_vocs_connection_set_loop_volume(&conn, "loop1", 0));
    testrun(ov_vocs_connection_set_loop_volume(&conn, "loop2", 1));
    testrun(ov_vocs_connection_set_loop_volume(&conn, "loop3", 99));
    testrun(ov_vocs_connection_set_loop_volume(&conn, "loop4", 100));

    testrun(0 == ov_vocs_connection_get_loop_volume(&conn, "loop1"));
    testrun(1 == ov_vocs_connection_get_loop_volume(&conn, "loop2"));
    testrun(99 == ov_vocs_connection_get_loop_volume(&conn, "loop3"));
    testrun(100 == ov_vocs_connection_get_loop_volume(&conn, "loop4"));

    testrun(ov_vocs_connection_clear(&conn));
    return testrun_log_success();
}

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CLUSTER                                                    #CLUSTER
 *
 *      ------------------------------------------------------------------------
 */

int all_tests() {

    testrun_init();
    testrun_test(test_ov_vocs_connection_clear);

    testrun_test(test_ov_vocs_connection_is_authenticated);
    testrun_test(test_ov_vocs_connection_is_authorized);

    testrun_test(test_ov_vocs_connection_get_user);
    testrun_test(test_ov_vocs_connection_set_user);

    testrun_test(test_ov_vocs_connection_get_role);
    testrun_test(test_ov_vocs_connection_set_role);

    testrun_test(test_ov_vocs_connection_get_session);
    testrun_test(test_ov_vocs_connection_set_session);

    testrun_test(test_ov_vocs_connection_get_client);
    testrun_test(test_ov_vocs_connection_set_client);

    testrun_test(test_ov_vocs_connection_get_loop_state);
    testrun_test(test_ov_vocs_connection_set_loop_state);

    testrun_test(test_ov_vocs_connection_get_loop_volume);
    testrun_test(test_ov_vocs_connection_set_loop_volume);

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
