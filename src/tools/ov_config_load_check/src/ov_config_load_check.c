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
        @file           ov_config_load_check.c
        @author         Markus Töpfer

        @date           2020-03-15

        Simple tool to read and dump some config to verify the
        ov_config args to JSON functions.

        EXAMPLE RUN:

        (1) Path with valid JSON file

        ./build/test/ov_config_load_check/ov_config_load_check -c \
        build/testov_base/resources/JSON/json.ok1

        (2) Path with non JSON

        ./build/test/ov_config_load_check/ov_config_load_check -c \
        build/test/ov_base/resources/dump_hex

        (3) Print version

        ./build/test/ov_config_load_check/ov_config_load_check -v

        ------------------------------------------------------------------------
*/

#include <ov_base/ov_config.h>

int main(int argc, char *argv[]) {

  ov_json_value *value = ov_config_from_command_line(argc, argv);

  if (!value)
    goto error;

  char *string = ov_json_value_to_string(value);
  fprintf(stdout, "CONFIG DUMP\n%s", string);

  string = ov_data_pointer_free(string);
  value = ov_json_value_free(value);

  return EXIT_SUCCESS;
error:
  return EXIT_FAILURE;
}