/***
        ------------------------------------------------------------------------

        Copyright (c) 2025 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_json_test.c
        @author         Töpfer, Markus

        @date           2025-08-27


        ------------------------------------------------------------------------
*/

#include <ov_base/ov_config.h>
#include <ov_base/ov_file.h>
#include <ov_base/ov_json.h>
#include <stdlib.h>

#define CONFIG_PATH                                                            \
    OPENVOCS_ROOT                                                              \
    "/src/tools/ov_json_test/config.json"

/*---------------------------------------------------------------------------*/

int main(int argc, char **argv) {

    int retval = EXIT_FAILURE;

    const char *path = ov_config_path_from_command_line(argc, argv);
    if (!path)
        path = CONFIG_PATH;

    if (path == VERSION_REQUEST_ONLY)
        goto error;

    uint8_t *buffer = NULL;
    size_t size = 0;
    uint8_t *last_of_first = NULL;

    if (OV_FILE_SUCCESS != ov_file_read(path, &buffer, &size)) {
        ov_log_error("Failed to read file %s", path);
        goto error;
    }

    if (!ov_json_match(buffer, size, true, &last_of_first)) {
        ov_log_error("File content not JSON");
        goto error;
    }

    retval = EXIT_SUCCESS;

error:
    return retval;
}