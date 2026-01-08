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

#include "ov_notify.c"
#include "ov_notify.h"
#include <ov_base/ov_string.h>
#include <ov_test/ov_test.h>
/*----------------------------------------------------------------------------*/

static int test_ov_notify_type_to_string() {

  testrun(0 == ov_notify_type_to_string(NOTIFY_INVALID));
  testrun(0 != ov_notify_type_to_string(NOTIFY_ENTITY_LOST));
  testrun(0 != ov_notify_type_to_string(NOTIFY_NEW_CALL));
  testrun(0 != ov_notify_type_to_string(NOTIFY_INCOMING_CALL));
  testrun(0 != ov_notify_type_to_string(NOTIFY_CALL_TERMINATED));
  testrun(0 != ov_notify_type_to_string(NOTIFY_NEW_RECORDING));
  testrun(0 != ov_notify_type_to_string(NOTIFY_PLAYBACK_STOPPED));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_notify_message() {
  ov_json_value *ov_notify_message(char const *uuid, ov_notify_type notify_type,
                                   ov_notify_parameters parameters);

  ov_notify_parameters params = {0};

  testrun(0 == ov_notify_message(0, NOTIFY_INVALID, params));

  // NEW CALL

  testrun(0 == ov_notify_message(0, NOTIFY_NEW_CALL, params));

  params.call.id = "argyle";
  params.call.loop = "bguile";
  params.call.peer = "cyle";

  ov_json_value *msg = ov_notify_message(0, NOTIFY_NEW_CALL, params);
  testrun(0 != msg);
  msg = ov_json_value_free(msg);

  // CALL_TERMINATED

  memset(&params, 0, sizeof(params));

  testrun(0 == ov_notify_message(0, NOTIFY_CALL_TERMINATED, params));

  params.call.id = "argyle";

  msg = ov_notify_message(0, NOTIFY_CALL_TERMINATED, params);
  testrun(0 != msg);
  msg = ov_json_value_free(msg);

  // Incoming call

  msg = ov_notify_message(0, NOTIFY_INCOMING_CALL, params);
  testrun(0 != msg);
  msg = ov_json_value_free(msg);

  // ENTITY LOST

  testrun(0 == ov_notify_message(0, NOTIFY_ENTITY_LOST, params));

  params.entity.name = "aaron";
  params.entity.type = 0;

  testrun(0 == ov_notify_message(0, NOTIFY_ENTITY_LOST, params));

  params.entity.type = "recorder";

  msg = ov_notify_message(0, NOTIFY_ENTITY_LOST, params);
  testrun(0 != msg);
  msg = ov_json_value_free(msg);

  memset(&params, 0, sizeof(params));

  // NEW_RECORDINING

  testrun(0 == ov_notify_message(0, NOTIFY_NEW_RECORDING, params));

  params.recording.end_epoch_secs = 123456;
  testrun(0 == ov_notify_message(0, NOTIFY_NEW_RECORDING, params));

  params.recording.start_epoch_secs = 123478;
  testrun(0 == ov_notify_message(0, NOTIFY_NEW_RECORDING, params));

  params.recording.id = "hinz";
  testrun(0 == ov_notify_message(0, NOTIFY_NEW_RECORDING, params));

  params.recording.loop = "und";
  testrun(0 == ov_notify_message(0, NOTIFY_NEW_RECORDING, params));

  params.recording.uri = "kunz";
  msg = ov_notify_message(0, NOTIFY_NEW_RECORDING, params);

  testrun(0 != msg);
  msg = ov_json_value_free(msg);

  memset(&params, 0, sizeof(params));

  // RECORDING STOPPED not supported yet
  // PLAYBACK STOPPED not supported yet

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static bool call_params_equal(ov_notify_parameters p1,
                              ov_notify_parameters p2) {

  if (((0 == p1.call.id) && (0 != p2.call.id)) ||
      ((0 != p1.call.id) && (0 == p2.call.id))) {
    return false;
  } else if (((0 == p1.call.peer) && (0 != p2.call.peer)) ||
             ((0 != p1.call.peer) && (0 == p2.call.peer))) {
    return false;
  } else if (((0 == p1.call.loop) && (0 != p2.call.loop)) ||
             ((0 != p1.call.loop) && (0 == p2.call.loop))) {
    return false;
  } else if ((0 != p1.call.id) && (0 != strcmp(p1.call.id, p2.call.id))) {
    return false;
  } else if ((0 != p1.call.loop) && (0 != strcmp(p1.call.loop, p2.call.loop))) {
    return false;
  } else if ((0 != p1.call.peer) && (0 != strcmp(p1.call.peer, p2.call.peer))) {
    return false;
  } else {
    return true;
  }
}

/*----------------------------------------------------------------------------*/

static bool entity_params_equal(ov_notify_parameters p1,
                                ov_notify_parameters p2) {

  if (((0 == p1.entity.name) && (0 != p2.entity.type)) ||
      ((0 != p1.entity.name) && (0 == p2.entity.type))) {
    return false;
  } else if (((0 == p1.entity.type) && (0 != p2.entity.type)) ||
             ((0 != p1.entity.type) && (0 == p2.entity.type))) {
    return false;
  } else if ((0 != p1.entity.name) &&
             (0 != strcmp(p1.entity.name, p2.entity.name))) {
    return false;
  } else if ((0 != p1.entity.type) &&
             (0 != strcmp(p1.entity.type, p2.entity.type))) {
    return false;
  } else {
    return true;
  }
}

/*----------------------------------------------------------------------------*/

static int test_ov_notify_parse() {
  ov_notify_type ov_notify_parse(ov_json_value const *parameters,
                                 ov_notify_parameters *notify_params);

  testrun(NOTIFY_INVALID == ov_notify_parse(0, 0));

  ov_notify_parameters parsed_params = {0};

  testrun(NOTIFY_INVALID == ov_notify_parse(0, &parsed_params));

  // NEW CALL

  ov_notify_parameters params = {0};

  params.call.id = "argyle";
  params.call.loop = "bguile";
  params.call.peer = "cyle";

  ov_json_value *msg = ov_notify_message(0, NOTIFY_NEW_CALL, params);
  testrun(0 != msg);

  testrun(NOTIFY_NEW_CALL ==
          ov_notify_parse(ov_event_api_get_parameter(msg), &parsed_params));
  testrun(call_params_equal(params, parsed_params));

  msg = ov_json_value_free(msg);

  memset(&params, 0, sizeof(params));
  memset(&parsed_params, 0, sizeof(parsed_params));
  //
  // INCOMING_CALL

  memset(&params, 0, sizeof(params));
  memset(&parsed_params, 0, sizeof(parsed_params));

  testrun(0 == ov_notify_message(0, NOTIFY_INCOMING_CALL, params));

  params.call.id = "argyle";
  params.call.peer = "donny";
  params.call.loop = "hans";

  msg = ov_notify_message(0, NOTIFY_INCOMING_CALL, params);
  testrun(0 != msg);

  testrun(NOTIFY_INCOMING_CALL ==
          ov_notify_parse(ov_event_api_get_parameter(msg), &parsed_params));

  testrun(call_params_equal(params, parsed_params));

  msg = ov_json_value_free(msg);

  memset(&params, 0, sizeof(params));
  memset(&parsed_params, 0, sizeof(parsed_params));

  // CALL_TERMINATED

  memset(&params, 0, sizeof(params));
  memset(&parsed_params, 0, sizeof(parsed_params));

  testrun(0 == ov_notify_message(0, NOTIFY_CALL_TERMINATED, params));

  params.call.id = "argyle";

  msg = ov_notify_message(0, NOTIFY_CALL_TERMINATED, params);
  testrun(0 != msg);

  testrun(NOTIFY_CALL_TERMINATED ==
          ov_notify_parse(ov_event_api_get_parameter(msg), &parsed_params));

  testrun(call_params_equal(params, parsed_params));

  msg = ov_json_value_free(msg);

  memset(&params, 0, sizeof(params));
  memset(&parsed_params, 0, sizeof(parsed_params));

  // ENTITY LOST

  params.entity.name = "aaron";
  params.entity.type = "recorder";

  msg = ov_notify_message(0, NOTIFY_ENTITY_LOST, params);
  testrun(0 != msg);

  testrun(NOTIFY_ENTITY_LOST ==
          ov_notify_parse(ov_event_api_get_parameter(msg), &parsed_params));

  testrun(entity_params_equal(params, parsed_params));

  msg = ov_json_value_free(msg);

  memset(&params, 0, sizeof(params));
  memset(&parsed_params, 0, sizeof(parsed_params));

  // NEW_RECORDING

  params.recording.end_epoch_secs = 123456;
  params.recording.start_epoch_secs = 123478;
  params.recording.id = "hinz";
  params.recording.loop = "und";
  params.recording.uri = "kunz";
  msg = ov_notify_message(0, NOTIFY_NEW_RECORDING, params);

  testrun(0 != msg);

  testrun(NOTIFY_NEW_RECORDING ==
          ov_notify_parse(ov_event_api_get_parameter(msg), &parsed_params));
  msg = ov_json_value_free(msg);

  testrun(params.recording.end_epoch_secs ==
          parsed_params.recording.end_epoch_secs);

  testrun(params.recording.start_epoch_secs ==
          parsed_params.recording.start_epoch_secs);

  testrun(0 ==
          ov_string_compare(params.recording.id, parsed_params.recording.id));

  testrun(0 == ov_string_compare(params.recording.loop,
                                 parsed_params.recording.loop));

  testrun(0 ==
          ov_string_compare(params.recording.uri, parsed_params.recording.uri));

  memset(&params, 0, sizeof(params));

  ov_recording_clear(&parsed_params.recording);
  memset(&parsed_params, 0, sizeof(parsed_params));

  return testrun_log_success();
}
/*----------------------------------------------------------------------------*/

OV_TEST_RUN("ov_notify", test_ov_notify_type_to_string, test_ov_notify_message,
            test_ov_notify_parse);
