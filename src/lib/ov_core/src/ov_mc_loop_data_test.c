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
        @file           ov_mc_loop_data_test.c
        @author         Markus

        @date           2022-11-10


        ------------------------------------------------------------------------
*/
#include "ov_mc_loop_data.c"
#include <ov_test/testrun.h>

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

int test_ov_mc_loop_data_from_json() {

    ov_json_value *val =
        ov_mc_loop_data_to_json((ov_mc_loop_data){.socket.host = "229.0.0.1",
                                                  .socket.port = 12345,
                                                  .socket.type = UDP,
                                                  .name = "loop1",
                                                  .volume = 50});

    testrun(val);

    ov_mc_loop_data data = ov_mc_loop_data_from_json(val);
    testrun(0 == strcmp("229.0.0.1", data.socket.host));
    testrun(12345 == data.socket.port);
    testrun(UDP == data.socket.type);
    testrun(0 == strcmp("loop1", data.name));
    testrun(50 == data.volume);

    val = ov_json_value_free(val);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_loop_data_to_json() {

    ov_json_value *val =
        ov_mc_loop_data_to_json((ov_mc_loop_data){.socket.host = "229.0.0.1",
                                                  .socket.port = 12345,
                                                  .socket.type = UDP,
                                                  .name = "loop1",
                                                  .volume = 50});

    testrun(val);
    testrun(ov_json_get(val, "/" OV_KEY_SOCKET));
    testrun(ov_json_get(val, "/" OV_KEY_NAME));
    testrun(ov_json_get(val, "/" OV_KEY_VOLUME));

    val = ov_json_value_free(val);

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
    testrun_test(test_ov_mc_loop_data_from_json);
    testrun_test(test_ov_mc_loop_data_to_json);

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
