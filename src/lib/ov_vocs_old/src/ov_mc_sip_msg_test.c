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
        @file           ov_mc_sip_msg_test.c
        @author         Markus Töpfer

        @date           2023-03-24


        ------------------------------------------------------------------------
*/
#include "ov_base/ov_socket.h"
#include "ov_mc_sip_msg.c"
#include <ov_test/testrun.h>

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

int test_ov_mc_sip_msg_register() {

    ov_json_value *msg = ov_mc_sip_msg_register(NULL);

    testrun(ov_event_api_event_is(msg, OV_KEY_REGISTER));
    msg = ov_json_value_free(msg);

    msg = ov_mc_sip_msg_register("1-2-3");
    testrun(ov_event_api_event_is(msg, OV_KEY_REGISTER));
    testrun(0 == strcmp("1-2-3", ov_event_api_get_uuid(msg)));
    msg = ov_json_value_free(msg);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_sip_msg_get_multicast() {

    ov_json_value *msg = ov_mc_sip_msg_get_multicast(NULL, NULL);
    testrun(!msg);

    msg = ov_mc_sip_msg_get_multicast(NULL, "loop1");

    testrun(ov_event_api_event_is(msg, OV_SIP_EVENT_GET_MULTICAST));
    testrun(0 == strcmp("loop1",
                        ov_json_string_get(ov_json_get(
                            msg, "/" OV_KEY_PARAMETER "/" OV_KEY_LOOP))));
    msg = ov_json_value_free(msg);

    msg = ov_mc_sip_msg_get_multicast("1-2-3", "loop1");
    testrun(ov_event_api_event_is(msg, OV_SIP_EVENT_GET_MULTICAST));
    testrun(0 == strcmp("1-2-3", ov_event_api_get_uuid(msg)));
    testrun(0 == strcmp("loop1",
                        ov_json_string_get(ov_json_get(
                            msg, "/" OV_KEY_PARAMETER "/" OV_KEY_LOOP))));
    msg = ov_json_value_free(msg);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_sip_msg_get_multicast_response() {

    ov_socket_configuration ref_socket = {0};

    testrun(0 == ov_mc_sip_msg_get_multicast_response(0, ref_socket));

    ov_json_value *invalid_request = ov_json_object();
    testrun(0 ==
            ov_mc_sip_msg_get_multicast_response(invalid_request, ref_socket));
    invalid_request = ov_json_value_free(invalid_request);

    ov_json_value *request = ov_mc_sip_msg_get_multicast(0, "abcd");
    testrun(0 == ov_mc_sip_msg_get_multicast_response(request, ref_socket));

    strncpy(ref_socket.host, "anton.hofreiter", sizeof(ref_socket.host));
    ref_socket.type = UDP;

    ov_json_value *response =
        ov_mc_sip_msg_get_multicast_response(request, ref_socket);
    request = ov_json_value_free(request);

    testrun(0 != response);

    ov_socket_configuration socket_from_response = {0};

    testrun(OV_ERROR_NOERROR ==
            ov_mc_sip_msg_get_response(response, &socket_from_response)
                .result.error_code);

    testrun(ref_socket.port == socket_from_response.port);
    testrun(ref_socket.type == socket_from_response.type);
    testrun(ov_string_equal(ref_socket.host, socket_from_response.host));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_sip_msg_get_response() {

    testrun(OV_ERROR_CODE_PROCESSING_ERROR ==
            ov_mc_sip_msg_get_response(0, 0).result.error_code);

    ov_socket_configuration loop_socket = {0};

    testrun(OV_ERROR_CODE_PROCESSING_ERROR ==
            ov_mc_sip_msg_get_response(0, &loop_socket).result.error_code);

    ov_id id;
    ov_id_fill_with_uuid(id);

    // Not a response message
    ov_json_value *msg = ov_mc_sip_msg_get_multicast(id, "loop1");
    testrun(0 != msg);

    testrun(OV_ERROR_CODE_PROCESSING_ERROR ==
            ov_mc_sip_msg_get_response(msg, &loop_socket).result.error_code);

    ov_json_value *response = ov_event_api_create_success_response(msg);
    testrun(0 != response);

    msg = ov_json_value_free(msg);

    // response, but without proper response section
    testrun(OV_ERROR_CODE_PROCESSING_ERROR ==
            ov_mc_sip_msg_get_response(msg, &loop_socket).result.error_code);

    ov_json_value *response_params = ov_event_api_get_response(response);

    ov_socket_configuration ref_socket = {
        .host = "antrax", .port = 17283, .type = TCP};

    testrun(ov_socket_configuration_to_json(ref_socket, &response_params));

    ov_response_state state =
        ov_mc_sip_msg_get_response(response, &loop_socket);

    testrun(OV_ERROR_NOERROR == state.result.error_code);

    testrun(0 == ov_string_compare(loop_socket.host, "antrax"));
    testrun(ref_socket.port == loop_socket.port);
    testrun(ref_socket.type == loop_socket.type);

    ov_json_object_set(response_params, OV_KEY_PORT, ov_json_number(4132));

    state = ov_mc_sip_msg_get_response(response, &loop_socket);

    testrun(OV_ERROR_NOERROR == state.result.error_code);

    testrun(0 == ov_string_compare(loop_socket.host, "antrax"));
    testrun(4132 == loop_socket.port);

    response = ov_json_value_free(response);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_sip_msg_set_sc() {

    ov_json_value *msg =
        ov_mc_sip_msg_set_sc(NULL, NULL, NULL, (ov_mc_mixer_core_forward){0});
    testrun(!msg);

    msg = ov_mc_sip_msg_set_sc(NULL,
                               "user",
                               "loop1",
                               (ov_mc_mixer_core_forward){.socket.host = "127."
                                                                         "0.0."
                                                                         "1",
                                                          .socket.type = UDP,
                                                          .socket.port = 1234,
                                                          .payload_type = 7,
                                                          .ssrc = 12345

                               });

    testrun(ov_event_api_event_is(msg, OV_SIP_EVENT_SET_SINGLECAST));
    ov_mc_mixer_core_forward socket = ov_mc_sip_msg_get_loop_socket(msg);
    testrun(0 == strcmp(socket.socket.host, "127.0.0.1"));
    testrun(UDP == socket.socket.type);
    testrun(1234 == socket.socket.port);
    testrun(7 == socket.payload_type);
    testrun(12345 == socket.ssrc);

    // char const *loop =
    // ov_json_string_get(ov_json_get(ov_event_api_get_parameter(msg), "/"
    // OV_KEY_LOOP))
    msg = ov_json_value_free(msg);

    msg = ov_mc_sip_msg_set_sc("1-2-3",
                               "user",
                               "loop",
                               (ov_mc_mixer_core_forward){.socket.host = "192."
                                                                         "168."
                                                                         "0.1",
                                                          .socket.type = TCP,
                                                          .socket.port = 33333,
                                                          .payload_type = 255

                               });
    testrun(0 == strcmp("1-2-3", ov_event_api_get_uuid(msg)));
    testrun(ov_event_api_event_is(msg, OV_SIP_EVENT_SET_SINGLECAST));
    socket = ov_mc_sip_msg_get_loop_socket(msg);
    testrun(0 == strcmp(socket.socket.host, "192.168.0.1"));
    testrun(TCP == socket.socket.type);
    testrun(33333 == socket.socket.port);
    testrun(255 == socket.payload_type);
    testrun(0 == socket.ssrc);
    msg = ov_json_value_free(msg);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_sip_msg_get_loop_socket() {

    ov_json_value *msg =
        ov_mc_sip_msg_set_sc(NULL,
                             "user",
                             "loop",
                             (ov_mc_mixer_core_forward){.socket.host = "127.0."
                                                                       "0.1",
                                                        .socket.type = UDP,
                                                        .socket.port = 1234,
                                                        .payload_type = 7,
                                                        .ssrc = 12345

                             });
    testrun(ov_event_api_event_is(msg, OV_SIP_EVENT_SET_SINGLECAST));
    ov_mc_mixer_core_forward socket = ov_mc_sip_msg_get_loop_socket(msg);
    testrun(0 == strcmp(socket.socket.host, "127.0.0.1"));
    testrun(UDP == socket.socket.type);
    testrun(1234 == socket.socket.port);
    testrun(7 == socket.payload_type);
    testrun(12345 == socket.ssrc);
    msg = ov_json_value_free(msg);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_sip_msg_acquire() {

    ov_json_value *msg = ov_mc_sip_msg_acquire(NULL, "user");
    // fprintf(stdout, "%s", ov_json_value_to_string(msg));
    testrun(ov_event_api_event_is(msg, OV_KEY_ACQUIRE));
    msg = ov_json_value_free(msg);

    msg = ov_mc_sip_msg_acquire("1-2-3", "a-b-c");
    testrun(ov_event_api_event_is(msg, OV_KEY_ACQUIRE));
    testrun(0 == strcmp("1-2-3", ov_event_api_get_uuid(msg)));
    msg = ov_json_value_free(msg);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_sip_msg_release() {

    ov_json_value *msg = ov_mc_sip_msg_release(NULL, NULL);
    testrun(!msg);

    msg = ov_mc_sip_msg_release(NULL, "user1");
    // fprintf(stdout, "%s", ov_json_value_to_string(msg));

    testrun(ov_event_api_event_is(msg, OV_KEY_RELEASE));
    testrun(0 == strcmp("user1",
                        ov_json_string_get(ov_json_get(
                            msg, "/" OV_KEY_PARAMETER "/" OV_KEY_USER))));
    msg = ov_json_value_free(msg);

    msg = ov_mc_sip_msg_release("1-2-3", "user1");
    testrun(ov_event_api_event_is(msg, OV_KEY_RELEASE));
    testrun(0 == strcmp("1-2-3", ov_event_api_get_uuid(msg)));
    testrun(0 == strcmp("user1",
                        ov_json_string_get(ov_json_get(
                            msg, "/" OV_KEY_PARAMETER "/" OV_KEY_USER))));
    msg = ov_json_value_free(msg);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_sip_msg_create_call() {

    ov_json_value *msg = ov_mc_sip_msg_create_call(NULL, NULL, NULL);
    testrun(!msg);

    msg = ov_mc_sip_msg_create_call("loop", "destination", NULL);
    // fprintf(stdout, "%s", ov_json_value_to_string(msg));

    testrun(ov_event_api_event_is(msg, OV_SIP_EVENT_CALL));
    testrun(0 == strcmp("loop",
                        ov_json_string_get(ov_json_get(
                            msg, "/" OV_KEY_PARAMETER "/" OV_KEY_LOOP))));
    testrun(0 == strcmp("destination",
                        ov_json_string_get(ov_json_get(
                            msg, "/" OV_KEY_PARAMETER "/" OV_KEY_CALLEE))));
    testrun(ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_CALLER));
    msg = ov_json_value_free(msg);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_sip_msg_terminate_call() {

    ov_json_value *msg = ov_mc_sip_msg_terminate_call(NULL);
    testrun(!msg);

    msg = ov_mc_sip_msg_terminate_call("id");

    testrun(ov_event_api_event_is(msg, OV_KEY_HANGUP));
    testrun(0 == strcmp("id",
                        ov_json_string_get(ov_json_get(
                            msg, "/" OV_KEY_PARAMETER "/" OV_KEY_CALL))));
    msg = ov_json_value_free(msg);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_sip_msg_permit() {

    ov_json_value *msg = ov_mc_sip_msg_permit((ov_sip_permission){0});
    testrun(!msg);

    msg = ov_mc_sip_msg_permit((ov_sip_permission){
        .caller = "caller", .loop = "loop", .from_epoch = 1, .until_epoch = 2});

    testrun(ov_event_api_event_is(msg, OV_KEY_PERMIT));
    testrun(ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_CALLER));
    testrun(ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_LOOP));
    testrun(ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_VALID_FROM));
    testrun(ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_VALID_UNTIL));
    msg = ov_json_value_free(msg);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_sip_msg_revoke() {

    ov_json_value *msg = ov_mc_sip_msg_revoke((ov_sip_permission){0});
    testrun(!msg);

    msg = ov_mc_sip_msg_revoke((ov_sip_permission){
        .caller = "caller", .loop = "loop", .from_epoch = 1, .until_epoch = 2});

    testrun(ov_event_api_event_is(msg, OV_KEY_REVOKE));
    testrun(ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_CALLER));
    testrun(ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_LOOP));
    testrun(ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_VALID_FROM));
    testrun(ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_VALID_UNTIL));
    msg = ov_json_value_free(msg);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_sip_msg_list_calls() {

    ov_json_value *msg = ov_mc_sip_msg_list_calls(NULL);
    testrun(msg);
    testrun(ov_event_api_event_is(msg, OV_KEY_LIST_CALLS));
    msg = ov_json_value_free(msg);

    msg = ov_mc_sip_msg_list_calls("id");

    testrun(ov_event_api_event_is(msg, OV_KEY_LIST_CALLS));
    testrun(0 == strcmp("id", ov_event_api_get_uuid(msg)));
    msg = ov_json_value_free(msg);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_sip_msg_list_permissions() {

    ov_json_value *msg = ov_mc_sip_msg_list_permissions(NULL);
    testrun(msg);
    testrun(ov_event_api_event_is(msg, OV_KEY_LIST_PERMISSIONS));
    msg = ov_json_value_free(msg);

    msg = ov_mc_sip_msg_list_permissions("id");

    testrun(ov_event_api_event_is(msg, OV_KEY_LIST_PERMISSIONS));
    testrun(0 == strcmp("id", ov_event_api_get_uuid(msg)));
    msg = ov_json_value_free(msg);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_sip_msg_get_status() {

    ov_json_value *msg = ov_mc_sip_msg_get_status(NULL);
    testrun(msg);
    testrun(ov_event_api_event_is(msg, OV_KEY_GET_STATUS));
    msg = ov_json_value_free(msg);

    msg = ov_mc_sip_msg_get_status("id");

    testrun(ov_event_api_event_is(msg, OV_KEY_GET_STATUS));
    testrun(0 == strcmp("id", ov_event_api_get_uuid(msg)));
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
    testrun_test(test_ov_mc_sip_msg_register);
    testrun_test(test_ov_mc_sip_msg_get_multicast);
    testrun_test(test_ov_mc_sip_msg_get_multicast_response);

    testrun_test(test_ov_mc_sip_msg_get_response);
    testrun_test(test_ov_mc_sip_msg_set_sc);
    testrun_test(test_ov_mc_sip_msg_get_loop_socket);

    testrun_test(test_ov_mc_sip_msg_acquire);
    testrun_test(test_ov_mc_sip_msg_release);

    testrun_test(test_ov_mc_sip_msg_create_call);
    testrun_test(test_ov_mc_sip_msg_terminate_call);

    testrun_test(test_ov_mc_sip_msg_permit);
    testrun_test(test_ov_mc_sip_msg_revoke);

    testrun_test(test_ov_mc_sip_msg_list_calls);
    testrun_test(test_ov_mc_sip_msg_list_permissions);
    testrun_test(test_ov_mc_sip_msg_get_status);

    return testrun_counter;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST EXECUTION #EXEC
 *
 *      ------------------------------------------------------------------------
 */

testrun_run(all_tests);
