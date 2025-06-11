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
        @file           ov_source_file.c
        @author         Markus Toepfer

        @date           2020-02-10

        @brief          Simple tool to generate

                        Some new module files:

                                include/ov_*.h
                                src/ov_*.c
                                src/ov_*_test.c

                        Or some new module projects, using @see ov_source_file

        ------------------------------------------------------------------------
*/

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <ov_base/ov_source_file.h>

int main(int argc, char *argv[]) {

    size_t size = 1000;
    char buffer[size];
    memset(buffer, 0, size);

    char path[PATH_MAX];
    memset(path, 0, PATH_MAX);

    ov_source_file_config config = {0};

    if (!ov_source_file_read_user_input(
            argc, argv, &config, OV_SOURCE_FILE_NAME)) {
        fprintf(stderr, "Failed to read user input\n");
        goto error;
    }

    if (!config.copyright.author) {

        if (!ov_source_file_get_git_author(buffer, size)) goto error;

        config.copyright.author = buffer;
    }

    if (!config.project.path) {

        if (!getcwd(path, PATH_MAX)) goto error;

        config.project.path = path;
    }

    /* ... override openvocs specific settings */

    config.project.url = "https://openvocs.org";
    config.copyright.owner = "German Aerospace Center DLR e.V. (GSOC)";
    config.copyright.note =
        "This file is part of the openvocs project. "
        "https://openvocs.org";

    if (!config.project.create) {

        if (!ov_source_file_create_source_files(&config)) goto error;
    } else {

        if (!ov_source_file_create_project(&config)) goto error;
    }

    return EXIT_SUCCESS;
error:
    return EXIT_FAILURE;
}
