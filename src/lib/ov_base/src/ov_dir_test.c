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
        @file           ov_dir_test.c
        @author         Markus Töpfer

        @date           2021-02-12


        ------------------------------------------------------------------------
*/
#include "ov_dir.c"
#include <ov_test/ov_test.h>
#include <ov_test/ov_test_file.h>

#include "../include/ov_file.h"
#include <limits.h>
#include <ov_base/ov_random.h>
#include <ov_base/ov_string.h>
#include <unistd.h>

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

int test_ov_dir_tree_remove() {

    char *path = ov_test_get_resource_path("save");
    fprintf(stdout, "using path %s", path);

    char *sub1 = ov_test_get_resource_path("save/1");
    char *sub2 = ov_test_get_resource_path("save/2");
    char *sub3 = ov_test_get_resource_path("save/3");

    char *sub1_1 = ov_test_get_resource_path("save/1/1");
    char *sub1_2 = ov_test_get_resource_path("save/1/2");
    char *sub1_3 = ov_test_get_resource_path("save/1/3");

    char file[10][PATH_MAX] = {0};

    snprintf(file[0], PATH_MAX, "%s/%s", path, "file1");
    snprintf(file[1], PATH_MAX, "%s/%s", path, "file2");
    snprintf(file[2], PATH_MAX, "%s/%s", path, "file3");
    snprintf(file[3], PATH_MAX, "%s/%s", sub1, "file1");
    snprintf(file[4], PATH_MAX, "%s/%s", sub2, "file1");
    snprintf(file[5], PATH_MAX, "%s/%s", sub3, "file1");
    snprintf(file[6], PATH_MAX, "%s/%s", sub1_1, "file1");
    snprintf(file[7], PATH_MAX, "%s/%s", sub1_2, "file2");
    snprintf(file[8], PATH_MAX, "%s/%s", sub1_3, "file3.1");
    snprintf(file[9], PATH_MAX, "%s/%s", sub1_3, "file3.2");

    testrun(access(path, F_OK) != 0);
    testrun(!ov_dir_tree_remove(NULL));

    // no existing
    testrun(ov_dir_tree_remove(path));

    // top level only
    testrun(0 == mkdir(path, 0740));
    testrun(access(path, F_OK) == 0);
    testrun(ov_dir_tree_remove(path));
    testrun(access(path, F_OK) != 0);

    // top level with files
    testrun(0 == mkdir(path, 0740));
    testrun(OV_FILE_SUCCESS == ov_file_write(file[0], (uint8_t *)"0", 1, "w"));
    testrun(OV_FILE_SUCCESS == ov_file_write(file[1], (uint8_t *)"1", 1, "w"));
    testrun(OV_FILE_SUCCESS == ov_file_write(file[2], (uint8_t *)"2", 1, "w"));
    testrun(access(path, F_OK) == 0);
    testrun(access(file[0], F_OK) == 0);
    testrun(access(file[1], F_OK) == 0);
    testrun(access(file[2], F_OK) == 0);
    testrun(ov_dir_tree_remove(path));
    testrun(access(path, F_OK) != 0);
    testrun(access(file[0], F_OK) != 0);
    testrun(access(file[1], F_OK) != 0);
    testrun(access(file[2], F_OK) != 0);

    // top level with subfolders and files
    testrun(0 == mkdir(path, 0740));
    testrun(0 == mkdir(sub1, 0740));
    testrun(0 == mkdir(sub2, 0740));
    testrun(0 == mkdir(sub3, 0740));
    testrun(access(path, F_OK) == 0);
    testrun(access(sub1, F_OK) == 0);
    testrun(access(sub2, F_OK) == 0);
    testrun(access(sub3, F_OK) == 0);
    testrun(OV_FILE_SUCCESS == ov_file_write(file[0], (uint8_t *)"0", 1, "w"));
    testrun(OV_FILE_SUCCESS == ov_file_write(file[1], (uint8_t *)"1", 1, "w"));
    testrun(OV_FILE_SUCCESS == ov_file_write(file[2], (uint8_t *)"2", 1, "w"));
    testrun(access(file[0], F_OK) == 0);
    testrun(access(file[1], F_OK) == 0);
    testrun(access(file[2], F_OK) == 0);
    testrun(ov_dir_tree_remove(path));
    testrun(access(path, F_OK) != 0);
    testrun(access(sub1, F_OK) != 0);
    testrun(access(sub2, F_OK) != 0);
    testrun(access(sub3, F_OK) != 0);
    testrun(access(file[0], F_OK) != 0);
    testrun(access(file[1], F_OK) != 0);
    testrun(access(file[2], F_OK) != 0);

    // top level with subfolders and files in subfolder
    testrun(0 == mkdir(path, 0740));
    testrun(0 == mkdir(sub1, 0740));
    testrun(0 == mkdir(sub2, 0740));
    testrun(0 == mkdir(sub3, 0740));
    testrun(access(path, F_OK) == 0);
    testrun(access(sub1, F_OK) == 0);
    testrun(access(sub2, F_OK) == 0);
    testrun(access(sub3, F_OK) == 0);
    testrun(OV_FILE_SUCCESS == ov_file_write(file[0], (uint8_t *)"0", 1, "w"));
    testrun(OV_FILE_SUCCESS == ov_file_write(file[1], (uint8_t *)"1", 1, "w"));
    testrun(OV_FILE_SUCCESS == ov_file_write(file[2], (uint8_t *)"2", 1, "w"));
    testrun(OV_FILE_SUCCESS == ov_file_write(file[3], (uint8_t *)"x", 1, "w"));
    testrun(OV_FILE_SUCCESS == ov_file_write(file[4], (uint8_t *)"x", 1, "w"));
    testrun(OV_FILE_SUCCESS == ov_file_write(file[5], (uint8_t *)"x", 1, "w"));
    testrun(access(file[0], F_OK) == 0);
    testrun(access(file[1], F_OK) == 0);
    testrun(access(file[2], F_OK) == 0);
    testrun(access(file[3], F_OK) == 0);
    testrun(access(file[4], F_OK) == 0);
    testrun(access(file[5], F_OK) == 0);
    testrun(ov_dir_tree_remove(path));
    testrun(access(path, F_OK) != 0);
    testrun(access(sub1, F_OK) != 0);
    testrun(access(sub2, F_OK) != 0);
    testrun(access(sub3, F_OK) != 0);
    testrun(access(file[0], F_OK) != 0);
    testrun(access(file[1], F_OK) != 0);
    testrun(access(file[2], F_OK) != 0);
    testrun(access(file[3], F_OK) != 0);
    testrun(access(file[4], F_OK) != 0);
    testrun(access(file[5], F_OK) != 0);

    // top level with subfolders with subfolder and files
    testrun(0 == mkdir(path, 0740));
    testrun(0 == mkdir(sub1, 0740));
    testrun(0 == mkdir(sub2, 0740));
    testrun(0 == mkdir(sub3, 0740));
    testrun(0 == mkdir(sub1_1, 0740));
    testrun(0 == mkdir(sub1_2, 0740));
    testrun(0 == mkdir(sub1_3, 0740));
    testrun(access(path, F_OK) == 0);
    testrun(access(sub1, F_OK) == 0);
    testrun(access(sub2, F_OK) == 0);
    testrun(access(sub3, F_OK) == 0);
    testrun(access(sub1_1, F_OK) == 0);
    testrun(access(sub1_2, F_OK) == 0);
    testrun(access(sub1_3, F_OK) == 0);
    testrun(OV_FILE_SUCCESS == ov_file_write(file[0], (uint8_t *)"0", 1, "w"));
    testrun(OV_FILE_SUCCESS == ov_file_write(file[1], (uint8_t *)"1", 1, "w"));
    testrun(OV_FILE_SUCCESS == ov_file_write(file[2], (uint8_t *)"2", 1, "w"));
    testrun(OV_FILE_SUCCESS == ov_file_write(file[3], (uint8_t *)"x", 1, "w"));
    testrun(OV_FILE_SUCCESS == ov_file_write(file[4], (uint8_t *)"x", 1, "w"));
    testrun(OV_FILE_SUCCESS == ov_file_write(file[5], (uint8_t *)"x", 1, "w"));
    testrun(OV_FILE_SUCCESS == ov_file_write(file[6], (uint8_t *)"x", 1, "w"));
    testrun(OV_FILE_SUCCESS == ov_file_write(file[7], (uint8_t *)"x", 1, "w"));
    testrun(OV_FILE_SUCCESS == ov_file_write(file[8], (uint8_t *)"x", 1, "w"));
    testrun(OV_FILE_SUCCESS == ov_file_write(file[9], (uint8_t *)"x", 1, "w"));
    testrun(access(file[0], F_OK) == 0);
    testrun(access(file[1], F_OK) == 0);
    testrun(access(file[2], F_OK) == 0);
    testrun(access(file[3], F_OK) == 0);
    testrun(access(file[4], F_OK) == 0);
    testrun(access(file[5], F_OK) == 0);
    testrun(access(file[6], F_OK) == 0);
    testrun(access(file[7], F_OK) == 0);
    testrun(access(file[8], F_OK) == 0);
    testrun(access(file[9], F_OK) == 0);
    testrun(ov_dir_tree_remove(path));
    testrun(access(path, F_OK) != 0);
    testrun(access(sub1, F_OK) != 0);
    testrun(access(sub2, F_OK) != 0);
    testrun(access(sub3, F_OK) != 0);
    testrun(access(sub1_1, F_OK) != 0);
    testrun(access(sub1_2, F_OK) != 0);
    testrun(access(sub1_3, F_OK) != 0);
    testrun(access(file[0], F_OK) != 0);
    testrun(access(file[1], F_OK) != 0);
    testrun(access(file[2], F_OK) != 0);
    testrun(access(file[3], F_OK) != 0);
    testrun(access(file[4], F_OK) != 0);
    testrun(access(file[5], F_OK) != 0);
    testrun(access(file[6], F_OK) != 0);
    testrun(access(file[7], F_OK) != 0);
    testrun(access(file[8], F_OK) != 0);
    testrun(access(file[9], F_OK) != 0);

    path = ov_data_pointer_free(path);
    sub1 = ov_data_pointer_free(sub1);
    sub2 = ov_data_pointer_free(sub2);
    sub3 = ov_data_pointer_free(sub3);
    sub1_1 = ov_data_pointer_free(sub1_1);
    sub1_2 = ov_data_pointer_free(sub1_2);
    sub1_3 = ov_data_pointer_free(sub1_3);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static bool collapse_to_path(char const **dirs,
                             char *target_path,
                             size_t const target_length) {

    memset(target_path, 0, target_length);

    size_t remaining_bytes = target_length;

    while (0 != *dirs) {

        if (remaining_bytes < 1) {

            return false;
        }

        if (target_path != strcat(target_path, "/")) {

            return false;
        }

        remaining_bytes += 1;

        const size_t len_dir = strlen(*dirs);

        if (remaining_bytes < len_dir) {

            return false;
        }

        if (target_path != strcat(target_path, *dirs)) {

            return false;
        }

        remaining_bytes -= len_dir;

        ++dirs;
    }

    return true;
}

/*----------------------------------------------------------------------------*/

static bool path_exists(char const *path) {

    struct stat buf = {0};

    if (0 != stat(path, &buf)) {

        return false;
    }

    return S_ISDIR(buf.st_mode);
}

/*----------------------------------------------------------------------------*/

static char **dirs_copy(char **dirs, size_t dirs_len) {

    const size_t len = sizeof(char *) * dirs_len;
    char **copy = calloc(1, len);
    memcpy(copy, dirs, len);

    return copy;
}

/*----------------------------------------------------------------------------*/

static bool switch_to_temp_dir(char *old_cwd) {

    char *tempdir = ov_test_temp_dir(0);

    bool ok = (0 != tempdir) && (0 != getcwd(old_cwd, PATH_MAX)) &&
              (0 == chdir(tempdir));

    if (!ok) {
        fprintf(stderr, "Could not change cwd\n");
        rmdir(tempdir);
    }

    char cwd[PATH_MAX + 1] = {0};
    getcwd(cwd, sizeof(cwd));

    fprintf(stderr,
            "Temp dir: %s  - Current working dir: %s\n",
            ov_string_sanitize(tempdir),
            ov_string_sanitize(cwd));

    ov_free(tempdir);
    return ok;
}

/*----------------------------------------------------------------------------*/

static bool drop_temp_dir_and_restore(char const *old_cwd) {

    char cwd[PATH_MAX + 1] = {0};

    return (0 != old_cwd) && (0 != getcwd(cwd, sizeof(cwd))) &&
           (0 == chdir(old_cwd)) && (0 == rmdir(cwd));
}

/*----------------------------------------------------------------------------*/

static int test_ov_dir_tree_create() {

    char cwd[PATH_MAX] = {0};
    testrun(switch_to_temp_dir(cwd));

    char path[PATH_MAX] = {0};

    testrun(!ov_dir_tree_create(0));

    /* Special case: path exists, but is no dir */

    int tmp_fd = ov_test_file_tmp_write((const uint8_t *)"", 1, path);
    testrun(0 < tmp_fd);
    close(tmp_fd);

    testrun(!ov_dir_tree_create(path));

    unlink(path);

    /* sequence of dirs, last entry must be 0 */
    char *dirs[5] = {0};
    const size_t dirs_len = sizeof(dirs) / sizeof(dirs[0]);

    /* This dir definitely exists */
    dirs[0] = strdup("tmp");

    collapse_to_path((char const **)dirs, path, sizeof(path));

    testrun(path_exists(path));
    testrun(ov_dir_tree_create(path));
    testrun(path_exists(path));
    testrun(ov_dir_tree_create(path));
    testrun(path_exists(path));

    for (size_t i = 1; i < dirs_len - 1; ++i) {

        testrun(ov_random_string(&dirs[i], 2 + i, 0));
    }

    collapse_to_path((char const **)dirs, path, sizeof(path));

    testrun(!path_exists(path));

    fprintf(stderr, "Creating %s\n", path);

    testrun(ov_dir_tree_create(path));
    testrun(path_exists(path));

    char **abs_dirs = dirs_copy(dirs, dirs_len);

    for (size_t i = dirs_len - 1; i > 0; --i) {

        collapse_to_path((char const **)abs_dirs, path, sizeof(path));

        fprintf(stderr, "Removing %s\n", path);

        rmdir(path);

        abs_dirs[i] = 0;
    }

    free(abs_dirs);
    abs_dirs = 0;

    /* Special cases */

    /* 1. relative path */

    collapse_to_path((char const **)dirs, path, sizeof(path));
    char const *rel_path = path + 1;

    testrun(!path_exists(rel_path));

    fprintf(stderr, "Creating %s\n", rel_path);

    testrun(ov_dir_tree_create(rel_path));
    testrun(path_exists(rel_path));
    // check recreate with same path and success
    testrun(ov_dir_tree_create(rel_path));
    testrun(path_exists(rel_path));

    char **rel_dirs = dirs_copy(dirs, dirs_len);

    /* Remove rel dirs & free dir strings */
    for (ssize_t i = dirs_len - 2; i >= 0; --i) {

        collapse_to_path((char const **)rel_dirs, path, sizeof(path));
        rel_path = path + 1;

        char cwd[PATH_MAX] = {0};
        getcwd(cwd, sizeof(cwd));
        fprintf(stderr,
                "(cwd: %s) Removing %s\n",
                ov_string_sanitize(cwd),
                rel_path);

        testrun(0 == rmdir(rel_path));
        rel_dirs[i] = 0;
    }

    free(rel_dirs);
    rel_dirs = 0;

    /* 2. relative path: single tier */

    testrun(!path_exists(dirs[0]));

    fprintf(stderr, "Creating %s\n", dirs[0]);

    testrun(ov_dir_tree_create(dirs[0]));
    testrun(path_exists(dirs[0]));

    fprintf(stderr, "Removing %s\n", dirs[0]);
    testrun(0 == rmdir(dirs[0]));

    for (ssize_t i = dirs_len - 2; i >= 0; --i) {

        free(dirs[i]);
        dirs[i] = 0;
    }

    const char *check_path = "/tmp/some19dis2nfsdnölq3rdadj42rj";

    memset(path, 0, PATH_MAX);
    strcat(path, check_path);
    strcat(path, "///////////////");

    testrun(ov_dir_tree_remove(check_path));
    testrun(!path_exists(check_path));
    testrun(ov_dir_tree_create(path));
    testrun(path_exists(check_path));
    testrun(ov_dir_tree_remove(check_path));

    memset(path, 0, PATH_MAX);
    strcat(path, check_path);
    strcat(path, "//something////////x///y//");

    char expect[PATH_MAX] = {0};
    snprintf(expect, PATH_MAX, "%s/something/x/y", check_path);

    testrun(ov_dir_tree_remove(expect));
    testrun(!path_exists(expect));
    testrun(ov_dir_tree_create(path));
    testrun(path_exists(expect));
    testrun(ov_dir_tree_remove(expect));

    // Restore original cwd
    drop_temp_dir_and_restore(cwd);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_dir_access_to_path() {

    char *path = ov_test_get_resource_path("dir");
    char *file = ov_test_get_resource_path("file");

    testrun(ov_dir_tree_remove(path));

    testrun(OV_FILE_SUCCESS == ov_file_write(file, (uint8_t *)"1", 1, "w"));
    testrun(0 == access(file, F_OK));

    testrun(!ov_dir_access_to_path(NULL));
    testrun(!ov_dir_access_to_path(path));

    testrun(0 == mkdir(path, 0740));
    testrun(ov_dir_access_to_path(path));

    // file is not dir
    testrun(0 == access(file, F_OK));
    testrun(!ov_dir_access_to_path(file));

    // TBD access rights tests

    unlink(file);

    testrun(ov_dir_tree_remove(path));

    file = ov_data_pointer_free(file);
    path = ov_data_pointer_free(path);
    return testrun_log_success();
}

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST EXECUTION                                                  #EXEC
 *
 *      ------------------------------------------------------------------------
 */

OV_TEST_RUN("ov_dir",
            test_ov_dir_tree_remove,
            test_ov_dir_tree_create,
            test_ov_dir_access_to_path);
