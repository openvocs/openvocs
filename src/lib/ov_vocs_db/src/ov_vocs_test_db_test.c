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
        @file           ov_vocs_test_db_test.c
        @author         Markus Töpfer

        @date           2022-11-24


        ------------------------------------------------------------------------
*/
#include "ov_vocs_test_db.c"
#include <ov_test/testrun.h>

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

int test_ov_vocs_test_db_create() {

    ov_vocs_db *db = ov_vocs_test_db_create();
    testrun(db);

    /*
        ov_json_value *out = ov_vocs_db_eject(db, OV_VOCS_DB_TYPE_AUTH);
        char *str = ov_json_value_to_string(out);
        fprintf(stdout, "%s\n", str);
        str = ov_data_pointer_free(str);
    */
    testrun(ov_vocs_db_authenticate(db, "user1", "user1"));
    testrun(ov_vocs_db_authenticate(db, "user2", "user2"));
    testrun(ov_vocs_db_authenticate(db, "user3", "user3"));

    testrun(ov_vocs_db_authorize(db, "user1", "role1"));
    testrun(ov_vocs_db_authorize(db, "user1", "role2"));
    testrun(ov_vocs_db_authorize(db, "user1", "role3"));

    testrun(ov_vocs_db_authorize(db, "user2", "role1"));
    testrun(ov_vocs_db_authorize(db, "user2", "role2"));

    testrun(ov_vocs_db_authorize(db, "user3", "role1"));
    testrun(ov_vocs_db_authorize(db, "user3", "role3"));

    testrun(!ov_vocs_db_authorize(db, "user2", "role3"));
    testrun(!ov_vocs_db_authorize(db, "user3", "role2"));

    testrun(OV_VOCS_SEND == ov_vocs_db_get_permission(db, "role1", "loop1"));
    testrun(OV_VOCS_SEND == ov_vocs_db_get_permission(db, "role2", "loop1"));
    testrun(OV_VOCS_SEND == ov_vocs_db_get_permission(db, "role3", "loop1"));

    testrun(OV_VOCS_SEND == ov_vocs_db_get_permission(db, "role1", "loop2"));
    testrun(OV_VOCS_SEND == ov_vocs_db_get_permission(db, "role2", "loop2"));
    testrun(OV_VOCS_RECV == ov_vocs_db_get_permission(db, "role3", "loop2"));

    testrun(OV_VOCS_SEND == ov_vocs_db_get_permission(db, "role1", "loop3"));
    testrun(OV_VOCS_RECV == ov_vocs_db_get_permission(db, "role2", "loop3"));
    testrun(OV_VOCS_SEND == ov_vocs_db_get_permission(db, "role3", "loop3"));

    testrun(OV_VOCS_SEND == ov_vocs_db_get_permission(db, "role1", "loop3"));
    testrun(OV_VOCS_RECV == ov_vocs_db_get_permission(db, "role2", "loop3"));
    testrun(OV_VOCS_SEND == ov_vocs_db_get_permission(db, "role3", "loop3"));

    testrun(OV_VOCS_SEND == ov_vocs_db_get_permission(db, "role1", "loop4"));
    testrun(OV_VOCS_SEND == ov_vocs_db_get_permission(db, "role2", "loop4"));
    testrun(OV_VOCS_NONE == ov_vocs_db_get_permission(db, "role3", "loop4"));

    testrun(OV_VOCS_NONE == ov_vocs_db_get_permission(db, "role1", "loop5"));
    testrun(OV_VOCS_SEND == ov_vocs_db_get_permission(db, "role2", "loop5"));
    testrun(OV_VOCS_SEND == ov_vocs_db_get_permission(db, "role3", "loop5"));

    testrun(NULL == ov_vocs_db_free(db));
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
    testrun_test(test_ov_vocs_test_db_create);

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
