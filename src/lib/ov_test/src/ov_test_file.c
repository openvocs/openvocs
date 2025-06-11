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

        @author         Michael J. Beer

        ------------------------------------------------------------------------
*/

#include "../include/ov_test_file.h"

/*----------------------------------------------------------------------------*/

#include <ftw.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>

/*----------------------------------------------------------------------------*/

#define TEST_UNUSED(x)                                                         \
    do {                                                                       \
        (void)(x);                                                             \
    } while (0)

/*----------------------------------------------------------------------------*/

int ov_test_file_tmp_write(uint8_t const *content,
                           size_t length,
                           char *file_name) {

    static char const *TEMP_PATTERN = "/tmp/openvocsXXXXXX";

    if ((0 == content) || (0 == file_name) || (0 == length)) {

        return 0;
    }

    strncpy(file_name, TEMP_PATTERN, strlen(TEMP_PATTERN) + 1);

    int fd = mkstemp(file_name);

    if (0 > fd) {
        return -1;
    }

    ssize_t bytes_written = write(fd, content, length);

    if ((bytes_written < 0) || ((size_t)bytes_written != length)) {

        close(fd);
        unlink(file_name);
        return -1;
    }

    return fd;
}

/*----------------------------------------------------------------------------*/

char *ov_test_temp_dir(char const *prefix) {

    if (0 == prefix) {

        prefix = "/tmp/tmp";
    }

    char temp_path[PATH_MAX] = {0};

    ssize_t written = snprintf(temp_path, PATH_MAX, "%s.XXXXXX", prefix);

    if ((0 > written) || (PATH_MAX <= written)) {

        return 0;
    }

    if (0 == mkdtemp(temp_path)) {

        return 0;
    }

    return strdup(temp_path);
}

/*----------------------------------------------------------------------------*/

#undef TEST_UNUSED
