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

        @author         Michael J. Beer, DLR/GSOC

        ------------------------------------------------------------------------
*/

#include "ov_analogue_events.c"
#include <ov_base/ov_string.h>
#include <ov_test/ov_test.h>

/*----------------------------------------------------------------------------*/

static int test_ov_analogue_event_type_to_string() {

  testrun(ov_string_equal(ov_analogue_event_type_to_string(132), "INVALID"));

  testrun(ov_string_equal(ov_analogue_event_type_to_string(ANALOGUE_INVALID),
                          "INVALID"));

  testrun(ov_string_equal(ov_analogue_event_type_to_string(ANALOGUE_IN), "IN"));

  testrun(
      ov_string_equal(ov_analogue_event_type_to_string(ANALOGUE_OUT), "OUT"));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static ov_analogue_event replay_from_json(char const *json_str,
                                          ov_json_value **jval) {

  size_t len = 0;

  if (0 != json_str) {
    len = strlen(json_str);
  }

  ov_json_value *json = ov_json_value_from_string(json_str, len);
  ov_analogue_event rpe = ov_analogue_event_from_json(json);

  if (0 != jval) {
    *jval = json;
  } else {
    json = ov_json_value_free(json);
  }

  return rpe;
}

/*----------------------------------------------------------------------------*/

int test_ov_analogue_event_from_json() {

  ov_analogue_event rpe = ov_analogue_event_from_json(0);
  testrun(NETWORK_TRANSPORT_TYPE_ERROR == rpe.rtp.mc_socket.type);

  rpe = replay_from_json("{}", 0);
  testrun(NETWORK_TRANSPORT_TYPE_ERROR == rpe.rtp.mc_socket.type);

  rpe = replay_from_json("{\"" OV_KEY_RTP "\" : {}}", 0);
  testrun(NETWORK_TRANSPORT_TYPE_ERROR == rpe.rtp.mc_socket.type);

  rpe = replay_from_json("{\"" OV_KEY_DESTINATION "\" : {}}", 0);
  testrun(NETWORK_TRANSPORT_TYPE_ERROR == rpe.rtp.mc_socket.type);

  rpe = replay_from_json(
      "{\"" OV_KEY_DESTINATION "\" : { \"" OV_KEY_CHANNEL "\" : 0}}", 0);
  testrun(NETWORK_TRANSPORT_TYPE_ERROR == rpe.rtp.mc_socket.type);

  rpe = replay_from_json("{\"" OV_KEY_RTP "\" : {},"
                         "\"" OV_KEY_CHANNEL "\" : 0}",
                         0);

  rpe = replay_from_json("{\"" OV_KEY_RTP "\" : { \"" OV_KEY_LOOP "\" : {}},"
                         "\"" OV_KEY_CHANNEL "\" : 0}",
                         0);

  rpe = replay_from_json("{\"" OV_KEY_RTP "\" : { \"" OV_KEY_LOOP "\" : {},"
                         "\"" OV_KEY_CODEC "\": {}},"
                         "\"" OV_KEY_CHANNEL "\" : 0}",
                         0);

  rpe = replay_from_json("{\"" OV_KEY_RTP "\" : {"
                         "\"" OV_KEY_LOOP "\" : {},"
                         "\"" OV_KEY_CODEC "\": {\"codec\":\"opus\"}},"
                         "\"" OV_KEY_CHANNEL "\" : 0}",
                         0);

  ov_json_value *jval = 0;

  // rpe = replay_from_json(
  //     "{\"rtp\" : { \"loop\" : \"abc\", \"codec\" : { \"codec\" : "
  //     "\"opus\"}}, \"destination\" : { \"channel\" : 12}}", &jval);
  rpe = replay_from_json("{\"" OV_KEY_RTP "\" : {"
                         "\"" OV_KEY_LOOP "\" : \"abc\","
                         "\"" OV_KEY_CODEC "\": {\"codec\":\"opus\"}},"
                         "\"" OV_KEY_CHANNEL "\" : 12}",
                         &jval);

  testrun(UDP == rpe.rtp.mc_socket.type);
  testrun(0 == strcmp("abc", rpe.rtp.mc_socket.host));
  testrun(OV_MC_SOCKET_PORT == rpe.rtp.mc_socket.port);
  testrun(0 != rpe.rtp.codec_config);
  testrun(0 == rpe.rtp.ssid);
  testrun(12 == rpe.channel);

  jval = ov_json_value_free(jval);

  rpe = replay_from_json("{\"" OV_KEY_RTP "\" : {"
                         "\"" OV_KEY_SSRC "\": 13,"
                         "\"" OV_KEY_LOOP "\" : \"abc\","
                         "\"" OV_KEY_CODEC "\": {\"codec\":\"opus\"}},"
                         "\"" OV_KEY_CHANNEL "\" : 12}",
                         &jval);

  testrun(UDP == rpe.rtp.mc_socket.type);
  testrun(0 == strcmp("abc", rpe.rtp.mc_socket.host));
  testrun(OV_MC_SOCKET_PORT == rpe.rtp.mc_socket.port);
  testrun(0 != rpe.rtp.codec_config);
  testrun(13 == rpe.rtp.ssid);
  testrun(12 == rpe.channel);

  jval = ov_json_value_free(jval);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static bool stop_equals(char const *jstring, ov_analogue_event_type type,
                        size_t channel) {

  ov_json_value *jval = 0;

  if (0 != jstring) {
    jval = ov_json_value_from_string(jstring, strlen(jstring));
  }

  ov_analogue_stop_event event = ov_analogue_stop_event_from_json(jval);
  jval = ov_json_value_free(jval);

  return (type == event.type) && (channel == event.channel);
}

/*----------------------------------------------------------------------------*/

static int test_ov_analogue_stop_event_from_json() {

  testrun(stop_equals(0, ANALOGUE_INVALID, 0));
  testrun(stop_equals("{}", ANALOGUE_INVALID, 0));
  testrun(stop_equals("{\"" OV_KEY_TYPE "\": {}}", ANALOGUE_INVALID, 0));
  testrun(
      stop_equals("{\"" OV_KEY_TYPE "\": \"arrrgghh\"}", ANALOGUE_INVALID, 0));
  testrun(stop_equals("{\"" OV_KEY_TYPE "\": \"in\"}", ANALOGUE_IN, 0));
  testrun(stop_equals("{\"" OV_KEY_TYPE "\": \"out\"}", ANALOGUE_OUT, 0));
  testrun(stop_equals("{\"" OV_KEY_TYPE "\": \"in\", \"" OV_KEY_CHANNEL "\": "
                      "13}",
                      ANALOGUE_IN, 13));
  testrun(stop_equals("{\"" OV_KEY_TYPE "\": \"out\", \"" OV_KEY_CHANNEL "\":"
                      " 13"
                      "}",
                      ANALOGUE_OUT, 13));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static bool list_channel_equals(char const *jstring,
                                ov_analogue_event_type type) {

  ov_json_value *jval = 0;

  if (0 != jstring) {
    jval = ov_json_value_from_string(jstring, strlen(jstring));
  }

  ov_analogue_list_channel_event event =
      ov_analogue_list_channel_event_from_json(jval);
  jval = ov_json_value_free(jval);

  return (type == event.type);
}

/*----------------------------------------------------------------------------*/

static int test_ov_analogue_list_channel_event_from_json() {

  // {
  //   "type" : "in"/"out",
  // }

  testrun(list_channel_equals(0, ANALOGUE_INVALID));
  testrun(list_channel_equals("{}", ANALOGUE_INVALID));
  testrun(list_channel_equals("{\"" OV_KEY_TYPE "\": {}}", ANALOGUE_INVALID));
  testrun(list_channel_equals("{\"" OV_KEY_TYPE "\": \"arrrgghh\"}",
                              ANALOGUE_INVALID));
  testrun(list_channel_equals("{\"" OV_KEY_TYPE "\": \"in\"}", ANALOGUE_IN));
  testrun(list_channel_equals("{\"" OV_KEY_TYPE "\": \"out\"}", ANALOGUE_OUT));
  testrun(list_channel_equals("{\"" OV_KEY_TYPE "\": \"in\", "
                              "\"" OV_KEY_CHANNEL "\": "
                              "13}",
                              ANALOGUE_IN));
  testrun(list_channel_equals("{\"" OV_KEY_TYPE "\": \"out\", "
                              "\"" OV_KEY_CHANNEL "\":"
                              " 13"
                              "}",
                              ANALOGUE_OUT));

  testrun(list_channel_equals("{\"" OV_KEY_CHANNEL "\":13}", ANALOGUE_INVALID));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

OV_TEST_RUN("ov_analogue_events", test_ov_analogue_event_type_to_string,
            test_ov_analogue_event_from_json,
            test_ov_analogue_stop_event_from_json,
            test_ov_analogue_list_channel_event_from_json);
