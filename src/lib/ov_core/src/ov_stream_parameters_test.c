/***
        ------------------------------------------------------------------------

        Copyright (c) 2020 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_stream_parameters_test.c
        @author         Michael J. Beer

        @date           2020-02-07

        ------------------------------------------------------------------------
*/
#include "ov_stream_parameters.c"
#include <ov_test/ov_test.h>

int test_ov_stream_parameters_to_json() {

  /* Pretty much a dummy tes to check for error handling.
  Actual functionality is checked in ...from_json test */
  ov_stream_parameters sparams = {0};
  testrun(!ov_stream_parameters_to_json(sparams, 0));
  ov_json_value *recpt = ov_json_object();
  testrun(ov_stream_parameters_to_json(sparams, recpt));

  recpt = recpt->free(recpt);
  testrun(0 == recpt);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_stream_parameters_from_json() {

  testrun(!ov_stream_parameters_from_json(0, 0));

  ov_stream_parameters sparams = {0};

  testrun(!ov_stream_parameters_from_json(0, &sparams));

  ov_json_value *jv = ov_json_object();

  char const *dummy_codec = "Eat me";

  ov_stream_parameters ref_params = {
      .codec_parameters = ov_json_string(dummy_codec),
  };

  testrun(ov_stream_parameters_to_json(ref_params, jv));

  testrun(ov_stream_parameters_from_json(jv, &sparams));

  char const *read_dummy_codec = ov_json_string_get(sparams.codec_parameters);

  testrun(0 != read_dummy_codec);
  testrun(0 == strcmp(dummy_codec, read_dummy_codec));

  jv = jv->free(jv);
  testrun(0 == jv);

  ov_json_value *helper = (ov_json_value *)ref_params.codec_parameters;
  ref_params.codec_parameters = 0;

  testrun(0 == helper->free(helper));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

OV_TEST_RUN("ov_stream_parameters", test_ov_stream_parameters_to_json,
            test_ov_stream_parameters_from_json);

/*----------------------------------------------------------------------------*/
