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
        @file           ov_vocs_msg_test.c
        @author         Markus Töpfer

        @date           2022-11-25


        ------------------------------------------------------------------------
*/
#include "ov_vocs_msg.c"
#include <ov_test/testrun.h>

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

int test_ov_vocs_msg_logout() {

    ov_json_value *msg = ov_vocs_msg_logout();
    testrun(msg);
    testrun(ov_event_api_event_is(msg, OV_EVENT_API_LOGOUT));
    testrun(ov_event_api_get_uuid(msg));

    char *str = ov_json_value_to_string(msg);
    fprintf(stdout, "%s\n", str);
    str = ov_data_pointer_free(str);

    msg = ov_json_value_free(msg);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_vocs_msg_login() {

    ov_json_value *msg = ov_vocs_msg_login(NULL, NULL, NULL);
    testrun(msg);
    testrun(ov_event_api_event_is(msg, OV_EVENT_API_LOGIN));
    testrun(ov_event_api_get_uuid(msg));
    testrun(!ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_USER));
    testrun(!ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_PASSWORD));
    testrun(!ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_CLIENT));
    msg = ov_json_value_free(msg);

    msg = ov_vocs_msg_login("user", NULL, NULL);
    testrun(msg);
    testrun(ov_event_api_event_is(msg, OV_EVENT_API_LOGIN));
    testrun(ov_event_api_get_uuid(msg));
    testrun(ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_USER));
    testrun(!ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_PASSWORD));
    testrun(!ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_CLIENT));
    testrun(0 == strcmp("user",
                        ov_json_string_get(ov_json_get(msg, "/" OV_KEY_PARAMETER
                                                            "/" OV_KEY_USER))));
    msg = ov_json_value_free(msg);

    msg = ov_vocs_msg_login("user", "pass", NULL);
    testrun(msg);
    testrun(ov_event_api_event_is(msg, OV_EVENT_API_LOGIN));
    testrun(ov_event_api_get_uuid(msg));
    testrun(ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_USER));
    testrun(ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_PASSWORD));
    testrun(!ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_CLIENT));
    testrun(0 == strcmp("user",
                        ov_json_string_get(ov_json_get(msg, "/" OV_KEY_PARAMETER
                                                            "/" OV_KEY_USER))));
    testrun(0 == strcmp("pass",
                        ov_json_string_get(ov_json_get(
                            msg, "/" OV_KEY_PARAMETER "/" OV_KEY_PASSWORD))));
    msg = ov_json_value_free(msg);

    msg = ov_vocs_msg_login("user", "pass", "client");
    testrun(msg);
    testrun(ov_event_api_event_is(msg, OV_EVENT_API_LOGIN));
    testrun(ov_event_api_get_uuid(msg));
    testrun(ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_USER));
    testrun(ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_PASSWORD));
    testrun(ov_json_get(msg, "/" OV_KEY_CLIENT));
    testrun(0 == strcmp("user",
                        ov_json_string_get(ov_json_get(msg, "/" OV_KEY_PARAMETER
                                                            "/" OV_KEY_USER))));
    testrun(0 == strcmp("pass",
                        ov_json_string_get(ov_json_get(
                            msg, "/" OV_KEY_PARAMETER "/" OV_KEY_PASSWORD))));
    testrun(0 == strcmp("client", ov_json_string_get(
                                      ov_json_get(msg, "/" OV_KEY_CLIENT))));

    char *str = ov_json_value_to_string(msg);
    fprintf(stdout, "%s\n", str);
    str = ov_data_pointer_free(str);

    msg = ov_json_value_free(msg);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_vocs_msg_media() {

    ov_json_value *msg = ov_vocs_msg_media(OV_MEDIA_ERROR, "sdp");
    testrun(!msg);

    msg = ov_vocs_msg_media(OV_MEDIA_REQUEST, "sdp");
    testrun(msg);
    testrun(ov_event_api_event_is(msg, OV_EVENT_API_MEDIA));
    testrun(ov_event_api_get_uuid(msg));
    testrun(ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_TYPE));
    testrun(ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_SDP));
    testrun(0 == strcmp("request",
                        ov_json_string_get(ov_json_get(msg, "/" OV_KEY_PARAMETER
                                                            "/" OV_KEY_TYPE))));
    testrun(0 == strcmp("sdp", ov_json_string_get(ov_json_get(
                                   msg, "/" OV_KEY_PARAMETER "/" OV_KEY_SDP))));

    char *str = ov_json_value_to_string(msg);
    fprintf(stdout, "%s\n", str);
    str = ov_data_pointer_free(str);

    msg = ov_json_value_free(msg);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_vocs_msg_candidate() {

    ov_json_value *msg = ov_vocs_msg_candidate((ov_ice_candidate_info){0});
    testrun(!msg);

    msg = ov_vocs_msg_candidate(
        (ov_ice_candidate_info){.candidate = "candidate", .ufrag = "ufrag"});
    testrun(msg);
    testrun(ov_event_api_event_is(msg, OV_ICE_STRING_CANDIDATE));
    testrun(ov_event_api_get_uuid(msg));
    testrun(ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_CANDIDATE));
    testrun(ov_json_get(msg,
                        "/" OV_KEY_PARAMETER "/" OV_ICE_STRING_SDP_MLINEINDEX));
    testrun(ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_ICE_STRING_SDP_MID));
    testrun(ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_ICE_STRING_UFRAG));
    msg = ov_json_value_free(msg);

    msg = ov_vocs_msg_candidate((ov_ice_candidate_info){.ufrag = "ufrag"});
    testrun(msg);
    testrun(ov_event_api_event_is(msg, OV_ICE_STRING_CANDIDATE));
    testrun(ov_event_api_get_uuid(msg));
    testrun(!ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_CANDIDATE));
    testrun(ov_json_get(msg,
                        "/" OV_KEY_PARAMETER "/" OV_ICE_STRING_SDP_MLINEINDEX));
    testrun(ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_ICE_STRING_SDP_MID));
    testrun(ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_ICE_STRING_UFRAG));

    char *str = ov_json_value_to_string(msg);
    fprintf(stdout, "%s\n", str);
    str = ov_data_pointer_free(str);

    msg = ov_json_value_free(msg);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_vocs_msg_authorise() {

    ov_json_value *msg = ov_vocs_msg_authorise(NULL);
    testrun(msg);

    testrun(ov_event_api_event_is(msg, OV_EVENT_API_AUTHORISE));
    testrun(ov_event_api_get_uuid(msg));
    testrun(!ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_ROLE));
    msg = ov_json_value_free(msg);

    msg = ov_vocs_msg_authorise("role");
    testrun(msg);
    testrun(ov_event_api_event_is(msg, OV_EVENT_API_AUTHORISE));
    testrun(ov_event_api_get_uuid(msg));
    testrun(ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_ROLE));

    char *str = ov_json_value_to_string(msg);
    fprintf(stdout, "%s\n", str);
    str = ov_data_pointer_free(str);

    msg = ov_json_value_free(msg);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_vocs_msg_get() {

    ov_json_value *msg = ov_vocs_msg_get(NULL);
    testrun(msg);

    testrun(ov_event_api_event_is(msg, OV_EVENT_API_GET));
    testrun(ov_event_api_get_uuid(msg));
    testrun(!ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_TYPE));
    msg = ov_json_value_free(msg);

    msg = ov_vocs_msg_get("role");
    testrun(msg);
    testrun(ov_event_api_event_is(msg, OV_EVENT_API_GET));
    testrun(ov_event_api_get_uuid(msg));
    testrun(ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_TYPE));

    char *str = ov_json_value_to_string(msg);
    fprintf(stdout, "%s\n", str);
    str = ov_data_pointer_free(str);

    msg = ov_json_value_free(msg);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_vocs_msg_client_user_roles() {

    ov_json_value *msg = ov_vocs_msg_client_user_roles();
    testrun(msg);

    testrun(ov_event_api_event_is(msg, OV_EVENT_API_USER_ROLES));
    testrun(ov_event_api_get_uuid(msg));

    char *str = ov_json_value_to_string(msg);
    fprintf(stdout, "%s\n", str);
    str = ov_data_pointer_free(str);

    msg = ov_json_value_free(msg);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_vocs_msg_client_role_loops() {

    ov_json_value *msg = ov_vocs_msg_client_role_loops();
    testrun(msg);

    testrun(ov_event_api_event_is(msg, OV_EVENT_API_ROLE_LOOPS));
    testrun(ov_event_api_get_uuid(msg));

    char *str = ov_json_value_to_string(msg);
    fprintf(stdout, "%s\n", str);
    str = ov_data_pointer_free(str);

    msg = ov_json_value_free(msg);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_vocs_msg_switch_loop_state() {

    ov_json_value *msg = ov_vocs_msg_switch_loop_state(NULL, OV_VOCS_NONE);
    testrun(msg);

    testrun(ov_event_api_event_is(msg, OV_EVENT_API_SWITCH_LOOP_STATE));
    testrun(ov_event_api_get_uuid(msg));
    testrun(ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_STATE));
    testrun(!ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_LOOP));
    msg = ov_json_value_free(msg);

    msg = ov_vocs_msg_switch_loop_state("loop", OV_VOCS_NONE);
    testrun(msg);
    testrun(ov_event_api_event_is(msg, OV_EVENT_API_SWITCH_LOOP_STATE));
    testrun(ov_event_api_get_uuid(msg));
    testrun(ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_STATE));
    testrun(ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_LOOP));
    msg = ov_json_value_free(msg);

    msg = ov_vocs_msg_switch_loop_state("loop", OV_VOCS_RECV);
    testrun(msg);
    testrun(ov_event_api_event_is(msg, OV_EVENT_API_SWITCH_LOOP_STATE));
    testrun(ov_event_api_get_uuid(msg));
    testrun(ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_STATE));
    testrun(ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_LOOP));
    msg = ov_json_value_free(msg);

    msg = ov_vocs_msg_switch_loop_state("loop", OV_VOCS_SEND);
    testrun(msg);
    testrun(ov_event_api_event_is(msg, OV_EVENT_API_SWITCH_LOOP_STATE));
    testrun(ov_event_api_get_uuid(msg));
    testrun(ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_STATE));
    testrun(ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_LOOP));

    char *str = ov_json_value_to_string(msg);
    fprintf(stdout, "%s\n", str);
    str = ov_data_pointer_free(str);

    msg = ov_json_value_free(msg);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_vocs_msg_switch_loop_volume() {

    ov_json_value *msg = ov_vocs_msg_switch_loop_volume(NULL, 0);
    testrun(msg);

    testrun(ov_event_api_event_is(msg, OV_EVENT_API_SWITCH_LOOP_VOLUME));
    testrun(ov_event_api_get_uuid(msg));
    testrun(ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_VOLUME));
    testrun(!ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_LOOP));
    msg = ov_json_value_free(msg);

    msg = ov_vocs_msg_switch_loop_volume("loop", 1);
    testrun(msg);
    testrun(ov_event_api_event_is(msg, OV_EVENT_API_SWITCH_LOOP_VOLUME));
    testrun(ov_event_api_get_uuid(msg));
    testrun(ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_VOLUME));
    testrun(ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_LOOP));
    msg = ov_json_value_free(msg);

    msg = ov_vocs_msg_switch_loop_volume("loop", 100);
    testrun(msg);
    testrun(ov_event_api_event_is(msg, OV_EVENT_API_SWITCH_LOOP_VOLUME));
    testrun(ov_event_api_get_uuid(msg));
    testrun(ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_VOLUME));
    testrun(ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_LOOP));
    msg = ov_json_value_free(msg);

    msg = ov_vocs_msg_switch_loop_volume("loop", 255);
    testrun(msg);
    testrun(ov_event_api_event_is(msg, OV_EVENT_API_SWITCH_LOOP_VOLUME));
    testrun(ov_event_api_get_uuid(msg));
    testrun(ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_VOLUME));
    testrun(ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_LOOP));

    char *str = ov_json_value_to_string(msg);
    fprintf(stdout, "%s\n", str);
    str = ov_data_pointer_free(str);

    msg = ov_json_value_free(msg);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_vocs_msg_talking() {

    ov_json_value *msg = ov_vocs_msg_talking(NULL, true);
    testrun(msg);

    testrun(ov_event_api_event_is(msg, OV_EVENT_API_TALKING));
    testrun(ov_event_api_get_uuid(msg));
    testrun(ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_STATE));
    testrun(!ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_LOOP));
    msg = ov_json_value_free(msg);

    msg = ov_vocs_msg_talking("loop", true);
    testrun(msg);
    testrun(ov_event_api_event_is(msg, OV_EVENT_API_TALKING));
    testrun(ov_event_api_get_uuid(msg));
    testrun(ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_STATE));
    testrun(ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_LOOP));
    msg = ov_json_value_free(msg);

    msg = ov_vocs_msg_talking("loop", false);
    testrun(msg);
    testrun(ov_event_api_event_is(msg, OV_EVENT_API_TALKING));
    testrun(ov_event_api_get_uuid(msg));
    testrun(ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_STATE));
    testrun(ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_LOOP));

    char *str = ov_json_value_to_string(msg);
    fprintf(stdout, "%s\n", str);
    str = ov_data_pointer_free(str);

    msg = ov_json_value_free(msg);

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
    testrun_test(test_ov_vocs_msg_logout);
    testrun_test(test_ov_vocs_msg_login);
    testrun_test(test_ov_vocs_msg_media);
    testrun_test(test_ov_vocs_msg_candidate);
    testrun_test(test_ov_vocs_msg_authorise);
    testrun_test(test_ov_vocs_msg_get);
    testrun_test(test_ov_vocs_msg_client_user_roles);
    testrun_test(test_ov_vocs_msg_client_role_loops);
    testrun_test(test_ov_vocs_msg_switch_loop_state);
    testrun_test(test_ov_vocs_msg_switch_loop_volume);
    testrun_test(test_ov_vocs_msg_talking);

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
