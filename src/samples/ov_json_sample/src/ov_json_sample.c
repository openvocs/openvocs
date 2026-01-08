/***
        ------------------------------------------------------------------------

        Copyright (c) 2021 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_recorder.c
        @author         Michael Beer

        ------------------------------------------------------------------------
*/

#include <math.h>
#include <ov_base/ov_json.h>
#include <ov_base/ov_utils.h>

/*----------------------------------------------------------------------------*/

int main(int argc, char **argv) {

  UNUSED(argc);
  UNUSED(argv);

  /*

  */

  char const *json_string = "{"
                            "   \"l1key1\" : {"
                            "      \"l2key1\" : 2"
                            "    },"
                            "    \"l1key2\" : \"ratatoskr\""
                            "}";

  // Parse JSON from string
  ov_json_value *main_value =
      ov_json_value_from_string(json_string, strlen(json_string));

  // get a json value wrapping the double 2.0d
  ov_json_value const *json_number = ov_json_get(main_value, "/l1key1/l2key1");

  // get the actual double value
  double number = ov_json_number_get(json_number);

  // Comparing double values with exact value is not recommended ...
  OV_ASSERT(fabs(number - 2.0) < 0.001);

  printf("Got %f\n", number);

  // JSON in C erzeugen

  ov_json_value *object = ov_json_object();

  ov_json_object_set(object, "killed_by", ov_json_string("hoedur"));
  ov_json_object_set(object, "using", ov_json_string("mistle arrow"));
  ov_json_object_set(object, "times", ov_json_number(1));

  // Add object as value to our main json
  ov_json_object_set(main_value, "baldr", object);

  // `object` was consumed by main_value
  object = 0;

  // turn JSON into string

  char *new_json_string = ov_json_value_to_string(main_value);
  OV_ASSERT(0 != new_json_string);

  printf("Our json now looks like: %s\n", new_json_string);
  free(new_json_string);

  // Get rid of json value
  main_value = ov_json_value_free(main_value);
  OV_ASSERT(0 == main_value);

  return EXIT_SUCCESS;
}
