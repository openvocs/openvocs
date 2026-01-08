/***
        ------------------------------------------------------------------------

        Copyright (c) 2022 German Aerospace Center DLR e.V. (GSOC)

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

#include "../include/ov_log_rotate.h"
#include "log_assert.h"

#include <stdio.h>
#include <string.h>

/*----------------------------------------------------------------------------*/

static char *get_numerated_file_name(char const *path, size_t num) {

    char *numerated_path = 0;

    if (0 == path) {
        goto error;
    }

    if (0 == num) {
        return strdup(path);
    }

    if (OV_LOG_ROTATE_MAX_FILES <= num) {
        goto error;
    }

    //                             path           .  xxx terminal \0
    size_t length_numerated_path = strlen(path) + 1 + 3 + 1;

    numerated_path = calloc(1, length_numerated_path);
    LOG_ASSERT(numerated_path);

    const ssize_t printed =
        snprintf(numerated_path, length_numerated_path, "%s.%03zu", path, num);

    LOG_ASSERT(printed <= (ssize_t)length_numerated_path);

    if (0 > printed) {
        goto error;
    }

    return numerated_path;

error:

    if (0 != numerated_path) {
        free(numerated_path);
        numerated_path = 0;
    }

    LOG_ASSERT(0 == numerated_path);

    return 0;
}

/*----------------------------------------------------------------------------*/

bool ov_log_rotate_files(char const *path, size_t max_num_files) {

    char *fname_1 = 0;
    char *fname_2 = get_numerated_file_name(path, max_num_files - 1);

    if (0 == fname_2) {
        goto error;
    }

    if (0 != remove(fname_2)) {
        // fprintf(stderr, "Could not remove %s\n", fname_2);
    }

    for (size_t num = max_num_files - 1; num >= 1; --num) {

        LOG_ASSERT(num > 0);

        fname_1 = get_numerated_file_name(path, num - 1);

        LOG_ASSERT(0 != fname_1);

        if (0 != rename(fname_1, fname_2)) {
            // fprintf(stderr, "Could not rename %s to %s\n", fname_1, fname_2);
        }

        free(fname_2);
        fname_2 = fname_1;
        fname_1 = 0;
    }

    free(fname_2);

    return true;

error:

    if (0 != fname_2) {
        free(fname_2);
    }

    if (0 != fname_1) {
        free(fname_1);
    }

    return false;
}
