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
        @file           ov_ice_proxy_vocs_msg_test.c
        @author         Markus

        @date           2024-05-08


        ------------------------------------------------------------------------
*/
#include "ov_ice_proxy_vocs_msg.c"
#include <ov_test/testrun.h>

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

int test_ov_ice_proxy_vocs_msg_create_session() {

    ov_json_value *msg = NULL;

    msg = ov_ice_proxy_vocs_msg_create_session();
    testrun(ov_event_api_event_is(msg, OV_KEY_ICE_SESSION_CREATE));
    testrun(ov_json_get(msg, "/" OV_KEY_UUID));

    msg = ov_json_value_free(msg);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_ice_proxy_vocs_msg_session_failed() {

    ov_json_value *msg = NULL;

    testrun(!ov_ice_proxy_vocs_msg_session_failed(NULL))

        msg = ov_ice_proxy_vocs_msg_session_failed("1");
    testrun(ov_event_api_event_is(msg, OV_KEY_ICE_SESSION_FAILED));
    testrun(ov_json_get(msg, "/" OV_KEY_UUID));
    testrun(0 ==
            strcmp("1", ov_json_string_get(ov_json_get(
                            msg, "/" OV_KEY_PARAMETER "/" OV_KEY_SESSION))));

    msg = ov_json_value_free(msg);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_ice_proxy_vocs_msg_drop_session() {

    ov_json_value *msg = NULL;

    testrun(!ov_ice_proxy_vocs_msg_drop_session(NULL))

        msg = ov_ice_proxy_vocs_msg_drop_session("1");
    testrun(ov_event_api_event_is(msg, OV_KEY_ICE_SESSION_DROP));
    testrun(ov_json_get(msg, "/" OV_KEY_UUID));
    testrun(0 ==
            strcmp("1", ov_json_string_get(ov_json_get(
                            msg, "/" OV_KEY_PARAMETER "/" OV_KEY_SESSION))));

    msg = ov_json_value_free(msg);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static ov_json_value *msg_client_request() {

    ov_json_value *out = ov_event_api_message_create(
        OV_EVENT_API_MEDIA, NULL, ov_ice_proxy_vocs_API_VERSION);
    return out;
}

/*----------------------------------------------------------------------------*/

static ov_json_value *msg_session_response() {

    ov_json_value *out = ov_event_api_message_create(
        OV_KEY_ICE_SESSION_CREATE, NULL, ov_ice_proxy_vocs_API_VERSION);
    ov_json_value *par = ov_event_api_set_parameter(out);
    ov_json_value *val = ov_json_string("sdp");
    ov_json_object_set(par, OV_KEY_SDP, val);

    ov_event_api_set_type(par, ov_media_type_to_string(OV_MEDIA_OFFER));

    ov_json_value *response = ov_event_api_create_success_response(out);
    out = ov_json_value_free(out);
    ov_json_value *res = ov_event_api_get_response(response);

    val = ov_json_string("sdp");
    ov_json_object_set(res, OV_KEY_SDP, val);

    val = ov_json_string("1");
    ov_json_object_set(res, OV_KEY_SESSION, val);

    ov_event_api_set_type(res, ov_media_type_to_string(OV_MEDIA_OFFER));

    return response;
}

/*----------------------------------------------------------------------------*/

int test_ov_ice_proxy_vocs_msg_client_response_from_ice_session_create() {

    ov_json_value *msg = NULL;

    ov_json_value *client_request = msg_client_request();
    ov_json_value *session_response = msg_session_response();

    testrun(!ov_ice_proxy_vocs_msg_client_response_from_ice_session_create(
        NULL, NULL));
    testrun(!ov_ice_proxy_vocs_msg_client_response_from_ice_session_create(
        client_request, NULL));
    testrun(!ov_ice_proxy_vocs_msg_client_response_from_ice_session_create(
        NULL, session_response));

    msg = ov_ice_proxy_vocs_msg_client_response_from_ice_session_create(
        client_request, session_response);

    testrun(ov_event_api_event_is(msg, OV_EVENT_API_MEDIA));
    testrun(ov_json_get(msg, "/" OV_KEY_UUID));
    testrun(0 ==
            strcmp("1", ov_json_string_get(ov_json_get(
                            msg, "/" OV_KEY_RESPONSE "/" OV_KEY_SESSION))));
    testrun(0 == strcmp("sdp", ov_json_string_get(ov_json_get(
                                   msg, "/" OV_KEY_RESPONSE "/" OV_KEY_SDP))));
    testrun(0 == strcmp("offer",
                        ov_json_string_get(ov_json_get(msg, "/" OV_KEY_RESPONSE
                                                            "/" OV_KEY_TYPE))));

    msg = ov_json_value_free(msg);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_ice_proxy_vocs_msg_register() {

    ov_json_value *msg = NULL;

    testrun(!ov_ice_proxy_vocs_msg_register(NULL))

        msg = ov_ice_proxy_vocs_msg_register("1");
    testrun(ov_event_api_event_is(msg, OV_EVENT_API_REGISTER));
    testrun(ov_json_get(msg, "/" OV_KEY_UUID));
    testrun(0 == strcmp("1", ov_json_string_get(ov_json_get(
                                 msg, "/" OV_KEY_PARAMETER "/" OV_KEY_UUID))));

    msg = ov_json_value_free(msg);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_ice_proxy_vocs_msg_get_session_id() {

    ov_json_value *msg = ov_event_api_message_create("event", NULL, 0);
    ov_json_value *par = ov_event_api_set_parameter(msg);
    ov_json_value *val = ov_json_string("1");
    ov_json_object_set(par, OV_KEY_SESSION, val);

    const char *id = ov_ice_proxy_vocs_msg_get_session_id(msg);
    testrun(id);
    testrun(0 == strcmp(id, "1"));

    msg = ov_json_value_free(msg);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_ice_proxy_vocs_msg_get_response_session_id() {

    ov_json_value *msg = ov_event_api_message_create("event", NULL, 0);
    ov_json_value *par = ov_event_api_set_parameter(msg);
    ov_json_value *val = ov_json_string("1");
    ov_json_object_set(par, OV_KEY_SESSION, val);
    ov_json_value *res = ov_json_object();
    ov_json_object_set(msg, OV_KEY_RESPONSE, res);
    val = ov_json_string("2");
    ov_json_object_set(res, OV_KEY_SESSION, val);

    const char *id = ov_ice_proxy_vocs_msg_get_response_session_id(msg);
    testrun(id);
    testrun(0 == strcmp(id, "2"));

    msg = ov_json_value_free(msg);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_ice_proxy_vocs_msg_forward_stream() {

    ov_json_value *msg = ov_ice_proxy_vocs_msg_forward_stream(
        "1", (ov_ice_proxy_vocs_stream_forward_data){.id = 2,
                                                     .ssrc = 3,
                                                     .socket.type = UDP,
                                                     .socket.host = "127.0.0.1",
                                                     .socket.port = 12345});

    testrun(msg);
    testrun(ov_event_api_event_is(msg, OV_KEY_ICE_SESSION_FORWARD_STREAM));

    const char *id = ov_ice_proxy_vocs_msg_get_session_id(msg);
    testrun(id);
    testrun(0 == strcmp(id, "1"));

    ov_json_value *par = ov_event_api_get_parameter(msg);
    testrun(ov_json_object_get(par, OV_KEY_STREAM));

    testrun(2 == ov_json_number_get(
                     ov_json_get(par, "/" OV_KEY_STREAM "/" OV_KEY_ID)));
    testrun(3 == ov_json_number_get(
                     ov_json_get(par, "/" OV_KEY_STREAM "/" OV_KEY_SSRC)));
    testrun(12345 ==
            ov_json_number_get(ov_json_get(
                par, "/" OV_KEY_STREAM "/" OV_KEY_SOCKET "/" OV_KEY_PORT)));
    testrun(0 == strcmp("127.0.0.1",
                        ov_json_string_get(ov_json_get(par, "/" OV_KEY_STREAM
                                                            "/" OV_KEY_SOCKET
                                                            "/" OV_KEY_HOST))));

    msg = ov_json_value_free(msg);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_ice_proxy_vocs_msg_candidate() {

    ov_ice_candidate_info info = (ov_ice_candidate_info){
        .candidate = "924991635 1 udp 2122260223 129.247.221.83 37842 typ host",
        .SDPMlineIndex = 1,
        .SDPMid = 0,
        .ufrag = "username"};

    ov_json_value *msg = ov_ice_proxy_vocs_msg_candidate("1", &info);
    testrun(msg);
    testrun(ov_event_api_event_is(msg, OV_ICE_STRING_CANDIDATE));

    const char *id = ov_ice_proxy_vocs_msg_get_session_id(msg);
    testrun(id);
    testrun(0 == strcmp(id, "1"));

    testrun(ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_ICE_STRING_SDP_MID));
    testrun(ov_json_get(msg,
                        "/" OV_KEY_PARAMETER "/" OV_ICE_STRING_SDP_MLINEINDEX));
    testrun(ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_ICE_STRING_UFRAG));
    testrun(ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_ICE_STRING_CANDIDATE));

    msg = ov_json_value_free(msg);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_ice_proxy_vocs_msg_end_of_candidates() {

    ov_json_value *msg = ov_ice_proxy_vocs_msg_end_of_candidates("1");
    testrun(msg);
    testrun(ov_event_api_event_is(msg, OV_ICE_STRING_END_OF_CANDIDATES));

    const char *id = ov_ice_proxy_vocs_msg_get_session_id(msg);
    testrun(id);
    testrun(0 == strcmp(id, "1"));

    msg = ov_json_value_free(msg);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_ice_proxy_vocs_msg_media() {

    ov_json_value *msg =
        ov_ice_proxy_vocs_msg_media("1", OV_MEDIA_OFFER, "sdp");
    testrun(msg);
    testrun(ov_event_api_event_is(msg, OV_KEY_MEDIA));

    const char *id = ov_ice_proxy_vocs_msg_get_session_id(msg);
    testrun(id);
    testrun(0 == strcmp(id, "1"));

    testrun(ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_TYPE));
    testrun(ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_SDP));

    msg = ov_json_value_free(msg);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_ice_proxy_vocs_msg_session_completed() {

    ov_json_value *msg =
        ov_ice_proxy_vocs_msg_session_completed("1", OV_ICE_COMPLETED);
    testrun(msg);
    testrun(ov_event_api_event_is(msg, OV_KEY_ICE_SESSION_COMPLETED));

    const char *id = ov_ice_proxy_vocs_msg_get_session_id(msg);
    testrun(id);
    testrun(0 == strcmp(id, "1"));

    testrun(ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_STATE));

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
    testrun_test(test_ov_ice_proxy_vocs_msg_create_session);
    testrun_test(test_ov_ice_proxy_vocs_msg_session_failed);
    testrun_test(test_ov_ice_proxy_vocs_msg_drop_session);
    testrun_test(
        test_ov_ice_proxy_vocs_msg_client_response_from_ice_session_create);
    testrun_test(test_ov_ice_proxy_vocs_msg_register);
    testrun_test(test_ov_ice_proxy_vocs_msg_get_session_id);
    testrun_test(test_ov_ice_proxy_vocs_msg_get_response_session_id);
    testrun_test(test_ov_ice_proxy_vocs_msg_forward_stream);
    testrun_test(test_ov_ice_proxy_vocs_msg_candidate);
    testrun_test(test_ov_ice_proxy_vocs_msg_end_of_candidates);
    testrun_test(test_ov_ice_proxy_vocs_msg_media);
    testrun_test(test_ov_ice_proxy_vocs_msg_session_completed);

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