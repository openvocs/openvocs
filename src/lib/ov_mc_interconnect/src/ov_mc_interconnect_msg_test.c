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
        @file           ov_mc_interconnect_msg_test.c
        @author         Markus Töpfer

        @date           2023-12-11


        ------------------------------------------------------------------------
*/
#include "ov_mc_interconnect_msg.c"
#include <ov_test/testrun.h>

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

int test_ov_mc_interconnect_msg_connect_loops() {

    ov_json_value *out = ov_mc_interconnect_msg_connect_loops();
    testrun(out);
    testrun(ov_event_api_event_is(out, OV_MC_INTERCONNECT_CONNECT_LOOPS));
    out = ov_json_value_free(out);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_interconnect_msg_connect_media() {

    ov_json_value *out =
        ov_mc_interconnect_msg_connect_media("name", "codec", "host", 1);
    testrun(out);
    testrun(ov_event_api_event_is(out, OV_MC_INTERCONNECT_CONNECT_MEDIA));
    testrun(0 == strcmp("name",
                        ov_json_string_get(ov_json_get(out, "/" OV_KEY_PARAMETER
                                                            "/" OV_KEY_NAME))));
    testrun(0 ==
            strcmp("codec", ov_json_string_get(ov_json_get(
                                out, "/" OV_KEY_PARAMETER "/" OV_KEY_CODEC))));
    testrun(0 == strcmp("host",
                        ov_json_string_get(ov_json_get(out, "/" OV_KEY_PARAMETER
                                                            "/" OV_KEY_HOST))));
    testrun(1 == ov_json_number_get(
                     ov_json_get(out, "/" OV_KEY_PARAMETER "/" OV_KEY_PORT)));
    out = ov_json_value_free(out);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_interconnect_msg_register() {

    ov_json_value *out = ov_mc_interconnect_msg_register("name", "pass");
    testrun(out);
    testrun(ov_event_api_event_is(out, OV_MC_INTERCONNECT_REGISTER));
    testrun(0 == strcmp("name",
                        ov_json_string_get(ov_json_get(out, "/" OV_KEY_PARAMETER
                                                            "/" OV_KEY_NAME))));
    testrun(0 == strcmp("pass",
                        ov_json_string_get(ov_json_get(
                            out, "/" OV_KEY_PARAMETER "/" OV_KEY_PASSWORD))));
    out = ov_json_value_free(out);

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
    testrun_test(test_ov_mc_interconnect_msg_connect_loops);
    testrun_test(test_ov_mc_interconnect_msg_connect_media);
    testrun_test(test_ov_mc_interconnect_msg_register);

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
