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

#include "log_assert.h"
#include "ov_log_rotate.c"
#include "testrun.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

/*----------------------------------------------------------------------------*/

#define BIN_NUMBER_TO_CHAR(x) ((char)(30 + x))

/*----------------------------------------------------------------------------*/

static bool create_file(char const **path, size_t num, size_t id) {

    LOG_ASSERT(0 != path);
    LOG_ASSERT(0 != path[num]);

    fprintf(stderr, "Creating %s\n", path[num]);

    int fh = open(path[num], O_RDWR | O_CREAT | O_APPEND, S_IRWXU);

    LOG_ASSERT(0 < fh);

    // Convert binary number to literal number: 0 -> '0' etc
    char n = (char)(30 + id);

    if (1 != write(fh, &n, 1)) {
        close(fh);
        return false;
    }

    close(fh);
    fh = -1;

    return true;
}

/*----------------------------------------------------------------------------*/

static bool file_contains(char const **path, size_t index, size_t id) {

    LOG_ASSERT(0 != path);
    LOG_ASSERT(0 != path[index]);

    int fh = open(path[index], O_RDONLY);

    if (0 >= fh) {
        return false;
    }

    char n = BIN_NUMBER_TO_CHAR(id);

    char r = 0;
    if (1 != read(fh, &r, 1)) {
        close(fh);
        return false;
    }

    close(fh);
    fh = -1;
    return r == n;
}

/*----------------------------------------------------------------------------*/

static bool file_exists(char const *path) {

    struct stat sbuf = {0};

    if (0 != stat(path, &sbuf)) {
        return false;
    }

    return true;
}
/*----------------------------------------------------------------------------*/

static void remove_all(char const *const *paths) {

    LOG_ASSERT(0 != paths);

    for (char const *const *path = paths; 0 != *path; ++path) {

        fprintf(stderr, "Removing %s\n", *path);
        remove(*path);
    }
}

/*----------------------------------------------------------------------------*/

static int test_ov_log_rotate_files() {

    char const *files[] = {
        "/tmp/ov_log_rotate.log",     "/tmp/ov_log_rotate.log.001",
        "/tmp/ov_log_rotate.log.002", "/tmp/ov_log_rotate.log.003",
        "/tmp/ov_log_rotate.log.004", 0};

    remove_all(files);

    testrun(create_file(files, 0, 0));
    testrun(file_contains(files, 0, 0));
    testrun(!file_exists(files[1]));
    testrun(!file_exists(files[2]));
    testrun(!file_exists(files[3]));
    testrun(!file_exists(files[4]));

    testrun(!ov_log_rotate_files(0, 0));
    testrun(!ov_log_rotate_files(0, 2));

    testrun(!ov_log_rotate_files(files[0], OV_LOG_ROTATE_MAX_FILES + 1));

    testrun(ov_log_rotate_files(files[0], OV_LOG_ROTATE_MAX_FILES));
    testrun(!file_contains(files, 0, 0));
    testrun(file_contains(files, 1, 0));

    testrun(!file_exists(files[0]));
    testrun(file_exists(files[1]));
    testrun(!file_exists(files[2]));
    testrun(!file_exists(files[3]));
    testrun(!file_exists(files[4]));

    testrun(create_file(files, 0, 1));
    testrun(file_contains(files, 0, 1));

    // File 0 should be shifted to 1

    testrun(ov_log_rotate_files(files[0], 2));

    testrun(file_contains(files, 1, 1));

    testrun(!file_exists(files[0]));
    testrun(file_exists(files[1]));
    testrun(!file_exists(files[2]));
    testrun(!file_exists(files[3]));
    testrun(!file_exists(files[4]));

    // File 0 should be shifted to 1 once more.
    // No file 2 since limit of files is set to 2

    testrun(create_file(files, 0, 2));
    testrun(file_contains(files, 0, 2));

    testrun(ov_log_rotate_files(files[0], 2));

    testrun(file_contains(files, 1, 2));

    testrun(!file_exists(files[0]));
    testrun(file_exists(files[1]));
    testrun(!file_exists(files[2]));
    testrun(!file_exists(files[3]));
    testrun(!file_exists(files[4]));

    // File 0 should be shifted to 1 once more.
    // Now file 1 should be shifted to 2 since limit for files raised to 3

    testrun(create_file(files, 0, 3));
    testrun(file_contains(files, 0, 3));

    testrun(ov_log_rotate_files(files[0], 3));

    testrun(file_contains(files, 1, 3));
    testrun(file_contains(files, 2, 2));

    testrun(!file_exists(files[0]));
    testrun(file_exists(files[1]));
    testrun(file_exists(files[2]));
    testrun(!file_exists(files[3]));
    testrun(!file_exists(files[4]));

    remove_all(files);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

RUN_TESTS("ov_log_rotate", test_ov_log_rotate_files);

/*----------------------------------------------------------------------------*/
