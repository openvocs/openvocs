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
        @file           ov_domain_config_verify.c
        @author         Markus Töpfer

        @date           2021-11-04


        ------------------------------------------------------------------------
*/
#include <ov_base/ov_config.h>
#include <ov_core/ov_domain.h>

int main(int argc, char **argv) {

  ov_json_value *config = NULL;

  const char *path = ov_config_path_from_command_line(argc, argv);
  if (!path) {

    fprintf(stdout, "NO path given with -c %s\n", path);
    goto error;
  }

  config = ov_config_load(path);
  if (!config) {

    fprintf(stdout, "Failed to load JSON from %s\n", path);
    goto error;
  }

  char *str = ov_json_value_to_string(config);
  fprintf(stdout, "Checking config %s\n", str);
  str = ov_data_pointer_free(str);

  ov_domain_config domain_config = ov_domain_config_from_json(config);

  if (!ov_domain_config_verify(&domain_config)) {

    fprintf(stdout, "NOK domain config invalid at %s\n", path);
    goto error;

  } else {

    fprintf(stdout, "OK domain config valid at %s\n", path);
  }

  config = ov_json_value_free(config);
  return EXIT_SUCCESS;
error:
  config = ov_json_value_free(config);
  return EXIT_FAILURE;
}
