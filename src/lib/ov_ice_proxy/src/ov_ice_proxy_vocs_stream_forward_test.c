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
        @file           ov_ice_proxy_vocs_stream_forward_test.c
        @author         Markus

        @date           2024-05-08


        ------------------------------------------------------------------------
*/
#include "ov_ice_proxy_vocs_stream_forward.c"
#include <ov_test/testrun.h>

static ov_json_value *resmgr_example_response() {

  char *str = "{"
              "\"uuid\": \"afd4ede6-e0a1-11eb-90f4-533d5ed30e37\","
              "\"request\": {"
              "\"parameter\": {"
              "\"media\": {"
              "\"port\": 21001,"
              "\"host\": \"127.0.0.1\","
              "\"type\": \"UDP\""
              "},"
              "\"msid\": 23,"
              "\"stream_parameters\": {"
              "\"codec\": {"
              "\"law\": \"ulaw\","
              "\"sample_rate_hz\": 48000,"
              "\"codec\": \"opus\""
              "}"
              "},"
              "\"name\": \"USER_NAME\""
              "},"
              "\"uuid\": \"afd4ede6-e0a1-11eb-90f4-533d5ed30e37\","
              "\"protocol\": 1,"
              "\"event\": \"acquire_user\","
              "\"type\": \"unicast\""
              "},"
              "\"response\": {"
              "\"media\": {"
              "\"port\": 38737,"
              "\"host\": \"10.61.11.225\","
              "\"type\": \"UDP\""
              "},"
              "\"msid\": 32000"
              "},"
              "\"event\": \"acquire_user\""
              "}";

  return ov_json_value_from_string(str, strlen(str));
}
/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

int test_ov_ice_proxy_vocs_stream_forward_data_from_resmgr_response() {

  ov_json_value *in = resmgr_example_response();
  testrun(in);

  ov_ice_proxy_vocs_stream_forward_data data =
      ov_ice_proxy_vocs_stream_forward_data_from_resmgr_response(in);

  testrun(1 == data.id);
  testrun(32000 == data.ssrc);
  testrun(38737 == data.socket.port);
  testrun(UDP == data.socket.type);
  testrun(0 == strcmp(data.socket.host, "10.61.11.225"));

  ov_json_value *val = ov_event_api_get_response(in);
  data = (ov_ice_proxy_vocs_stream_forward_data){0};
  data = ov_ice_proxy_vocs_stream_forward_data_from_resmgr_response(val);

  testrun(1 == data.id);
  testrun(32000 == data.ssrc);
  testrun(38737 == data.socket.port);
  testrun(UDP == data.socket.type);
  testrun(0 == strcmp(data.socket.host, "10.61.11.225"));

  in = ov_data_pointer_free(in);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_ice_proxy_vocs_stream_forward_data_to_json() {

  ov_json_value *out = ov_ice_proxy_vocs_stream_forward_data_to_json(
      (ov_ice_proxy_vocs_stream_forward_data){0});

  testrun(!out);

  /* set min valid input */
  out = ov_ice_proxy_vocs_stream_forward_data_to_json(
      (ov_ice_proxy_vocs_stream_forward_data){
          .socket.host = "127.0.0.1", .socket.port = 1234, .socket.type = UDP});

  testrun(out);

  testrun(3 == ov_json_object_count(out));
  testrun(0 == ov_json_number_get(ov_json_object_get(out, OV_KEY_ID)));
  testrun(0 == ov_json_number_get(ov_json_object_get(out, OV_KEY_SSRC)));
  testrun(1234 == ov_json_number_get(
                      ov_json_get(out, "/" OV_KEY_SOCKET "/" OV_KEY_PORT)));
  testrun(0 == strcasecmp("UDP", ov_json_string_get(ov_json_get(
                                     out, "/" OV_KEY_SOCKET "/" OV_KEY_TYPE))));
  testrun(0 == strcasecmp("127.0.0.1",
                          ov_json_string_get(ov_json_get(
                              out, "/" OV_KEY_SOCKET "/" OV_KEY_HOST))));
  out = ov_json_value_free(out);

  out = ov_ice_proxy_vocs_stream_forward_data_to_json(
      (ov_ice_proxy_vocs_stream_forward_data){.id = 123,
                                              .ssrc = 456,
                                              .socket.host = "127.0.0.1",
                                              .socket.port = 1234,
                                              .socket.type = UDP});

  testrun(out);

  testrun(3 == ov_json_object_count(out));
  testrun(123 == ov_json_number_get(ov_json_object_get(out, OV_KEY_ID)));
  testrun(456 == ov_json_number_get(ov_json_object_get(out, OV_KEY_SSRC)));
  testrun(1234 == ov_json_number_get(
                      ov_json_get(out, "/" OV_KEY_SOCKET "/" OV_KEY_PORT)));
  testrun(0 == strcasecmp("UDP", ov_json_string_get(ov_json_get(
                                     out, "/" OV_KEY_SOCKET "/" OV_KEY_TYPE))));
  testrun(0 == strcasecmp("127.0.0.1",
                          ov_json_string_get(ov_json_get(
                              out, "/" OV_KEY_SOCKET "/" OV_KEY_HOST))));
  out = ov_json_value_free(out);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_ice_proxy_vocs_stream_forward_data_from_json() {

  ov_json_value *in = ov_ice_proxy_vocs_stream_forward_data_to_json(
      (ov_ice_proxy_vocs_stream_forward_data){.id = 123,
                                              .ssrc = 456,
                                              .socket.host = "127.0.0.1",
                                              .socket.port = 1234,
                                              .socket.type = UDP});

  testrun(in);

  ov_ice_proxy_vocs_stream_forward_data out =
      ov_ice_proxy_vocs_stream_forward_data_from_json(in);

  in = ov_json_value_free(in);
  testrun(out.id == 123);
  testrun(out.ssrc == 456);
  testrun(out.socket.type == UDP);
  testrun(out.socket.port == 1234);
  testrun(0 == strcmp("127.0.0.1", out.socket.host));

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
  testrun_test(test_ov_ice_proxy_vocs_stream_forward_data_from_resmgr_response);
  testrun_test(test_ov_ice_proxy_vocs_stream_forward_data_to_json);
  testrun_test(test_ov_ice_proxy_vocs_stream_forward_data_from_json);

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