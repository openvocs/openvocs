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
        @file           ov_config_test.c
        @author         Markus Töpfer
        @author         Michael Beer

        @date           2020-03-11


        ------------------------------------------------------------------------
*/

#include "ov_config.h"
#ifndef OV_TEST_RESOURCE_DIR
#error "Must provide -D OV_TEST_RESOURCE_DIR=value while compiling this file."
#endif

#include "../../include/ov_utils.h"
#include "sys/stat.h"
#include <ov_test/ov_test.h>

#include <ov_arch/ov_getopt.h>

#include "ov_config.c"

/******************************************************************************
 *                                  HELPERS
 ******************************************************************************/

bool create_dummy_config_file(char const *filename) {

    ov_json_value *dummy_config = ov_json_object();

    struct stat statbuf = {0};
    if (0 == stat(filename, &statbuf)) {
        goto error;
    }

    if (0 == dummy_config) {
        goto error;
    }

    if (!ov_json_write_file(filename, dummy_config)) {
        goto error;
    }

    dummy_config = ov_json_value_free(dummy_config);

    return true;

error:

    dummy_config = ov_json_value_free(dummy_config);

    return false;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

int test_ov_config_default_config_file_for() {

    testrun(0 == ov_config_default_config_file_for(0));

    char const *file = ov_config_default_config_file_for("me_test");

    testrun(0 != file);

    const size_t len = strlen(file);
    testrun(0 < len);

    /* File ends in ".json" - Rest cannot be assumed */
    testrun('n' == file[len - 1]);
    testrun('o' == file[len - 2]);
    testrun('s' == file[len - 3]);
    testrun('j' == file[len - 4]);
    testrun('.' == file[len - 5]);

    file = 0;

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_config_load() {

    const char *path = OV_TEST_RESOURCE_DIR "JSON/json.ok1";

    testrun(!ov_config_load(NULL));

    ov_json_value *value = ov_config_load(path);
    testrun(value);

    testrun(NULL == ov_json_value_free(value));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_config_path_from_command_line() {

    char *path = OV_TEST_RESOURCE_DIR "JSON/json.ok1";

    char *argv[] = {"filename", "-v", "-y", "-x"};
    ov_reset_getopt();
    testrun(NULL == ov_config_path_from_command_line(1, argv));

    ov_reset_getopt();
    testrun(VERSION_REQUEST_ONLY == ov_config_path_from_command_line(2, argv));
    ov_reset_getopt();
    testrun(VERSION_REQUEST_ONLY == ov_config_path_from_command_line(3, argv));
    ov_reset_getopt();
    testrun(VERSION_REQUEST_ONLY == ov_config_path_from_command_line(4, argv));

    argv[1] = "-c";
    argv[2] = path;

    ov_reset_getopt();
    const char *p = ov_config_path_from_command_line(3, argv);
    testrun(p);
    fprintf(stderr, "%s\n", p);

    testrun(NULL == ov_config_path_from_command_line(4, argv));
    /* expect log invalid option -x */

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_config_from_command_line() {

    /* Only patholocial cases are checked here,
     * see ov_config_load_test for remainder of tests */

    /* Critical string with special meaning - must NOT be loaded */

    testrun(create_dummy_config_file(VERSION_REQUEST_ONLY));

    char *argv[] = {"filename", "-v"};

    optind = 0;
    optarg = 0;

    /* this sequence of commands grants removal of test file */

    ov_json_value *loaded = ov_config_from_command_line(2, argv);

    if (0 != unlink(VERSION_REQUEST_ONLY)) {
        testrun_log_error("Could not remove file %s", VERSION_REQUEST_ONLY);
    }

    testrun(0 == loaded);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static ov_json_value *json_from_str(char const *str) {
    return ov_json_value_from_string(str, strlen(str));
}

/*----------------------------------------------------------------------------*/

static int test_ov_config_double_or_default() {

    testrun(3 == ov_config_double_or_default(0, "/o", 3));

    ov_json_value *jval = json_from_str("{\"o\": 1, \"p\": -1, \"q\":1.12}");

    testrun(1.0 == ov_config_double_or_default(jval, "o", 3));
    testrun(-1.0 == ov_config_double_or_default(jval, "p", 3));
    testrun(1.12 == ov_config_double_or_default(jval, "q", 3));
    testrun(3 == ov_config_double_or_default(jval, "r", 3));

    jval = ov_json_value_free(jval);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_config_u32_or_default() {

    testrun(3 == ov_config_u32_or_default(0, "/o", 3));

    ov_json_value *jval = json_from_str("{\"o\": 1, \"p\": -1, \"q\":1.12}");

    testrun(1 == ov_config_u32_or_default(jval, "o", 3));
    testrun(3 == ov_config_u32_or_default(jval, "p", 3));
    testrun(3 == ov_config_u32_or_default(jval, "q", 3));
    testrun(3 == ov_config_u32_or_default(jval, "r", 3));

    jval = ov_json_value_free(jval);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_config_u64_or_default() {

    testrun(3 == ov_config_u64_or_default(0, "/o", 3));

    ov_json_value *jval = json_from_str("{\"o\": 1, \"p\": -1, \"q\":1.12}");

    testrun(1 == ov_config_u32_or_default(jval, "o", 3));
    testrun(3 == ov_config_u32_or_default(jval, "p", 3));
    testrun(3 == ov_config_u32_or_default(jval, "q", 3));
    testrun(3 == ov_config_u32_or_default(jval, "r", 3));

    jval = ov_json_value_free(jval);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_config_bool_or_default() {

    testrun(false == ov_config_bool_or_default(0, 0, false));
    testrun(true == ov_config_bool_or_default(0, 0, true));

    testrun(false == ov_config_bool_or_default(
                         0, "tralladi trallada - nicht da", false));
    testrun(true ==
            ov_config_bool_or_default(0, "tralladi trallada - nicht da", true));

    ov_json_value *jval = ov_json_object();
    testrun(0 != jval);

    testrun(false == ov_config_bool_or_default(jval, 0, false));
    testrun(true == ov_config_bool_or_default(jval, 0, true));

    testrun(false == ov_config_bool_or_default(
                         jval, "tralladi trallada - nicht da", false));
    testrun(true == ov_config_bool_or_default(
                        jval, "tralladi trallada - nicht da", true));

    testrun(ov_json_object_set(jval, "wahr", ov_json_true()));
    testrun(ov_json_object_set(jval, "falsch", ov_json_false()));
    testrun(ov_json_object_set(jval, "muell", ov_json_string("muellme")));

    testrun(false == ov_config_bool_or_default(jval, 0, false));
    testrun(true == ov_config_bool_or_default(jval, 0, true));

    testrun(false == ov_config_bool_or_default(
                         jval, "tralladi trallada - nicht da", false));
    testrun(true == ov_config_bool_or_default(
                        jval, "tralladi trallada - nicht da", true));

    testrun(true == ov_config_bool_or_default(jval, "wahr", false));
    testrun(true == ov_config_bool_or_default(jval, "wahr", true));

    testrun(false == ov_config_bool_or_default(jval, "falsch", false));
    testrun(false == ov_config_bool_or_default(jval, "falsch", true));

    testrun(false == ov_config_bool_or_default(jval, "muell", false));
    testrun(true == ov_config_bool_or_default(jval, "muell", true));

    jval = ov_json_value_free(jval);
    testrun(0 == jval);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

OV_TEST_RUN("ov_config", test_ov_config_default_config_file_for,
            test_ov_config_load, test_ov_config_path_from_command_line,
            test_ov_config_from_command_line, test_ov_config_double_or_default,
            test_ov_config_u32_or_default, test_ov_config_u64_or_default,
            test_ov_config_bool_or_default);
