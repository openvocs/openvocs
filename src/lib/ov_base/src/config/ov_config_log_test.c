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

        @author         Michael J. Beer, DLR/GSOC
        @date           2021-09-14

        ------------------------------------------------------------------------
*/

#include "ov_config_log.c"

#include <ov_base/ov_dir.h>

#include "../../include/ov_dir.h"
#include "../../include/ov_utils.h"
#include <ov_base/ov_file.h>
#include <ov_test/ov_test.h>

/*----------------------------------------------------------------------------*/

static bool output_empty(ov_log_output out) {

    if (0 != out.filehandle) {
        return false;
    }

    if (0 != out.format) {
        return false;
    }

    if (out.use.systemd) {
        return false;
    }

    return true;
}

/*----------------------------------------------------------------------------*/

static void lets_log_something(ov_log_level level) {

    ov_log_ng(level, __FILE__, __FUNCTION__, __LINE__, "LOGGED SOMETHIN");
}

/*----------------------------------------------------------------------------*/

#define OUR_TEMP_DIR "/tmp/ov_config_log"

/*****************************************************************************
                                     Files
 ****************************************************************************/

static bool file_object_empty(FILE *f) {

    long old_pos = ftell(f);
    OV_ASSERT(0 <= old_pos);

    int retval = fseek(f, 0, SEEK_END);
    OV_ASSERT(0 <= retval);

    bool is_empty = 0 == ftell(f);

    retval = fseek(f, old_pos, SEEK_SET);
    OV_ASSERT(0 <= retval);

    return is_empty;
}

/*----------------------------------------------------------------------------*/

static bool file_empty(char *path) {

    FILE *f = fopen(path, "r");

    if (0 == f) return true;

    bool empty = file_object_empty(f);

    fclose(f);

    return empty;
}

/*----------------------------------------------------------------------------*/

static bool file_object_clear(FILE *f) {
    int retval = fseek(f, 0, SEEK_SET);
    OV_ASSERT(0 <= retval);
    return 0 == ftruncate(fileno(f), 0);
}

/*----------------------------------------------------------------------------*/

static bool file_clear(char const *path) {

    FILE *f = fopen(path, "w+");
    if (0 == f) return false;
    bool clear = file_object_clear(f);
    fclose(f);

    return clear;
}

/*****************************************************************************
                                     STDOUT
 ****************************************************************************/

#define STDOUT_FILE OUR_TEMP_DIR "/stdout"

static bool stdout_empty() { return file_object_empty(stdout); }

/*----------------------------------------------------------------------------*/

static bool stdout_clear() { return file_object_clear(stdout); }

/*----------------------------------------------------------------------------*/

static bool stdout_redirect() {
    return stdout == freopen(STDOUT_FILE, "w+", stdout);
}

/*****************************************************************************
                                     STDERR
 ****************************************************************************/

#define STDERR_FILE OUR_TEMP_DIR "/stderr"

static bool stderr_empty() { return file_object_empty(stderr); }

/*----------------------------------------------------------------------------*/

static bool stderr_clear() { return file_object_clear(stderr); }

/*----------------------------------------------------------------------------*/

static bool stderr_redirect() {
    return stderr == freopen(STDERR_FILE, "w+", stderr);
}

/*----------------------------------------------------------------------------*/

static ov_json_value *json_from_string(char const *str) {
    OV_ASSERT(0 != str);
    return ov_json_value_from_string(str, strlen(str));
}

/*****************************************************************************
                                     Tests
 ****************************************************************************/

static int init() {

    ov_dir_tree_remove(OUR_TEMP_DIR);
    ov_dir_tree_create(OUR_TEMP_DIR);
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static ov_log_output log_output_from_string(char const *str) {

    ov_json_value *log_json = json_from_string(str);

    TEST_ASSERT(0 != log_json);

    ov_log_output out = ov_log_output_from_json(log_json);

    log_json = ov_json_value_free(log_json);
    TEST_ASSERT(0 == log_json);

    return out;
}

/*----------------------------------------------------------------------------*/

static int test_ov_log_output_from_json() {

    ov_log_output out = ov_log_output_from_json(0);
    testrun(output_empty(out));

    out = log_output_from_string(
        "{"
        "\"" OV_KEY_FORMAT "\" : \"" OV_KEY_JSON
        "\","
        "\"" OV_KEY_LEVEL
        "\" : \"info\","
        "\"" OV_KEY_SYSTEMD
        "\" : true"
        "}");

    testrun(!output_empty(out));

    testrun(out.use.systemd);
    testrun(0 == out.filehandle);
    testrun(!out.log_rotation.use);
    testrun(0 == out.format);

    // Log level is not part of output def. - it is not deserialized here
    // therefore

#define LOG_FILE_NAME OUR_TEMP_DIR "/output_from_string"

    memset(&out, 0, sizeof(out));

    out = log_output_from_string(
        "{"
        "\"" OV_KEY_FILE "\" : \"" LOG_FILE_NAME
        "\","
        "\"" OV_KEY_FORMAT "\" : \"" OV_KEY_JSON
        "\","
        "\"" OV_KEY_LEVEL
        "\" : \"info\","
        "\"" OV_KEY_SYSTEMD
        "\" : false"
        "}");

    testrun(!output_empty(out));

    testrun(!out.use.systemd);
    testrun(0 != out.filehandle);
    testrun(!out.log_rotation.use);
    testrun(OV_LOG_JSON == out.format);

    close(out.filehandle);

    memset(&out, 0, sizeof(out));

    remove(LOG_FILE_NAME);

    // Check extended file config format - no log rotation

    out = log_output_from_string(
        "{"
        "\"" OV_KEY_FILE
        "\" : {"
        "\"" OV_KEY_FILE "\":\"" LOG_FILE_NAME
        ".r\""
        "},"
        "\"" OV_KEY_FORMAT "\" : \"" OV_KEY_JSON
        "\","
        "\"" OV_KEY_LEVEL
        "\" : \"info\","
        "\"" OV_KEY_SYSTEMD
        "\" : false"
        "}");

    testrun(!out.use.systemd);
    testrun(0 != out.filehandle);
    testrun(!out.log_rotation.use);
    testrun(0 == out.log_rotation.path);

    testrun(OV_LOG_JSON == out.format);

    close(out.filehandle);
    memset(&out, 0, sizeof(out));

    remove(LOG_FILE_NAME ".r");

    //
    // Check extended file config format with log rotation

    out = log_output_from_string(
        "{"
        "\"" OV_KEY_FILE
        "\" : {"
        "\"" OV_KEY_FILE "\":\"" LOG_FILE_NAME
        ".s\","
        "\"" OV_KEY_ROTATE_AFTER_MESSAGES
        "\": 7,"
        "\"" OV_KEY_KEEP_FILES
        "\" : 3"
        "},"
        "\"" OV_KEY_FORMAT "\" : \"" OV_KEY_JSON
        "\","
        "\"" OV_KEY_LEVEL
        "\" : \"info\","
        "\"" OV_KEY_SYSTEMD
        "\" : false"
        "}");

    testrun(!out.use.systemd);
    testrun(0 != out.filehandle);
    testrun(out.log_rotation.use);
    testrun(7 == out.log_rotation.messages_per_file);
    testrun(3 == out.log_rotation.max_num_files);
    testrun(0 != out.log_rotation.path);
    testrun(0 == strcmp(LOG_FILE_NAME ".s", out.log_rotation.path));
    testrun(OV_LOG_JSON == out.format);

    free(out.log_rotation.path);
    close(out.filehandle);
    memset(&out, 0, sizeof(out));

    remove(LOG_FILE_NAME ".s");

    out = log_output_from_string(
        "{"
        "\"" OV_KEY_FILE
        "\" : {"
        "\"" OV_KEY_FILE "\":\"" LOG_FILE_NAME
        ".t\","
        "\"" OV_KEY_ROTATE_AFTER_MESSAGES
        "\": 7,"
        "\"" OV_KEY_KEEP_FILES
        "\" : 3"
        "},"
        "\"" OV_KEY_FORMAT "\" : \"" OV_KEY_JSON
        "\","
        "\"" OV_KEY_LEVEL
        "\" : \"info\","
        "\"" OV_KEY_SYSTEMD
        "\" : false"
        "}");

    testrun(!out.use.systemd);
    testrun(0 != out.filehandle);
    testrun(out.log_rotation.use);
    testrun(7 == out.log_rotation.messages_per_file);
    testrun(3 == out.log_rotation.max_num_files);
    testrun(0 != out.log_rotation.path);
    testrun(OV_LOG_JSON == out.format);
    testrun(0 == strcmp(LOG_FILE_NAME ".t", out.log_rotation.path));

    free(out.log_rotation.path);
    close(out.filehandle);
    memset(&out, 0, sizeof(out));

    remove(LOG_FILE_NAME ".t");

#undef LOG_FILE_NAME

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_config_log_from_json() {

    testrun(stderr_redirect());
    testrun(stdout_redirect());
    testrun(stderr_empty());
    testrun(stdout_empty());

    // Should log stdout, up to level info
    ov_log_error("ERROR");
    ov_log_warning("WARNING");
    ov_log_info("INFO");
    ov_log_debug("DEBUG");

    testrun(!stdout_empty());
    testrun(stderr_empty());
    testrun(stdout_clear());
    testrun(stderr_clear());

    ov_log_init();

    // Should log to stdout up to level info - JSON format
    ov_log_error("ERROR");
    ov_log_warning("WARNING");
    ov_log_info("INFO");
    ov_log_debug("DEBUG");

    testrun(!stdout_empty());
    testrun(stderr_empty());
    testrun(stdout_clear());
    testrun(stderr_clear());

    ov_json_value *log_cfg = json_from_string(
        "{"
        "\"" OV_KEY_FORMAT "\" : \"" OV_KEY_JSON
        "\","
        "\"" OV_KEY_LEVEL
        "\" : \"info\","
        "\"" OV_KEY_SYSTEMD
        "\" : \"false\""
        "}");

    testrun(0 != log_cfg);

    testrun(ov_config_log_from_json(log_cfg));
    log_cfg = ov_json_value_free(log_cfg);

    testrun(0 == log_cfg);

    // Nowhere
    ov_log_error("ERROR");
    ov_log_warning("WARNING");
    ov_log_info("INFO");
    ov_log_debug("DEBUG");

    testrun(stdout_empty());
    testrun(stderr_empty());

    // Install log handler for another module -
    // Since we are in module 'ov_log_test.c', we should still log nowhere

#define AAA_FILE_NAME OUR_TEMP_DIR "/aaa"

    log_cfg = json_from_string(
        "{"
        "\"" OV_KEY_LEVEL
        "\" : \"info\","
        "\"" OV_KEY_SYSTEMD
        "\" : \"false\","
        "\"" OV_KEY_MODULES
        "\" : {"
        "\"aaa\" : {"
        "\"" OV_KEY_FILE "\" : \"" AAA_FILE_NAME
        "\","
        "\"" OV_KEY_LEVEL
        "\" : \"info\""
        "}"
        "}"
        "}");

    testrun(0 != log_cfg);

    testrun(ov_config_log_from_json(log_cfg));
    log_cfg = ov_json_value_free(log_cfg);

    testrun(0 == log_cfg);

    ov_log_error("ERROR");

    testrun(stdout_empty());
    testrun(stderr_empty());
    testrun(file_empty(AAA_FILE_NAME));

    // Install log file for another module and
    // another log file for our own module
    // SHould log to 'our' file but not the other one

#define OV_LOG_CONFIGURE_TEST_FILE_NAME OUR_TEMP_DIR "/ov_config_log_test_c"

    log_cfg = json_from_string(
        "{"
        "\"" OV_KEY_LEVEL
        "\" : \"info\","
        "\"" OV_KEY_SYSTEMD
        "\" : \"false\","
        "\"" OV_KEY_MODULES
        "\" : {"
        "\"aaa\" : {"
        "\"" OV_KEY_FILE "\" : \"" AAA_FILE_NAME
        "\","
        "\"" OV_KEY_LEVEL
        "\" : \"info\""
        "},"
        "\"ov_config_log_test.c\" : {"
        "\"" OV_KEY_FILE "\" : \"" OV_LOG_CONFIGURE_TEST_FILE_NAME
        "\","
        "\"" OV_KEY_LEVEL
        "\" : \"info\""
        "}"
        "}"
        "}");

    testrun(0 != log_cfg);

    testrun(ov_config_log_from_json(log_cfg));
    log_cfg = ov_json_value_free(log_cfg);

    testrun(0 == log_cfg);

    ov_log_error("ERROR");

    testrun(stdout_empty());
    testrun(stderr_empty());
    testrun(file_empty(AAA_FILE_NAME));
    testrun(!file_empty(OV_LOG_CONFIGURE_TEST_FILE_NAME));

    testrun(file_clear(OV_LOG_CONFIGURE_TEST_FILE_NAME));

    //
    // Install log file for another module and
    // another log file for our own module
    // Install handler for another function
    // SHould log to 'our' file but not the other one

#define LETS_LOG_SOMETHING_FILE_NAME OUR_TEMP_DIR "/lets_log_something"

    log_cfg = json_from_string(
        "{"
        "\"" OV_KEY_LEVEL
        "\" : \"info\","
        "\"" OV_KEY_SYSTEMD
        "\" : \"false\","
        "\"" OV_KEY_MODULES
        "\" : {"
        "\"aaa\" : {"
        "\"" OV_KEY_FILE "\" : \"" AAA_FILE_NAME
        "\","
        "\"" OV_KEY_LEVEL
        "\" : \"info\""
        "},"
        "\"ov_config_log_test.c\" : {"
        "\"" OV_KEY_FILE "\" : \"" OV_LOG_CONFIGURE_TEST_FILE_NAME
        "\","
        "\"" OV_KEY_LEVEL
        "\" : \"warning\","
        "\"" OV_KEY_SYSTEMD
        "\" : true,"
        "\"" OV_KEY_FUNCTIONS
        "\" : {"
        "\"lets_log_something\" : {"
        "\"" OV_KEY_FILE
        "\" : "
        "\"" LETS_LOG_SOMETHING_FILE_NAME
        "\","
        "\"" OV_KEY_LEVEL
        "\" : \"warning\","
        "\"" OV_KEY_SYSTEMD
        "\" : true"
        "}"
        "}"
        "}"
        "}"
        "}");

    testrun(0 != log_cfg);

    testrun(ov_config_log_from_json(log_cfg));
    log_cfg = ov_json_value_free(log_cfg);

    testrun(0 == log_cfg);

    ov_log_error("ERROR");

    testrun(stdout_empty());
    testrun(stderr_empty());
    testrun(file_empty(AAA_FILE_NAME));
    testrun(file_empty(LETS_LOG_SOMETHING_FILE_NAME));
    testrun(!file_empty(OV_LOG_CONFIGURE_TEST_FILE_NAME));

    testrun(file_clear(OV_LOG_CONFIGURE_TEST_FILE_NAME));

    // Now log from out of 'lets_log_something' -> Should be logged into
    // its own log file

    lets_log_something(OV_LOG_ERR);

    testrun(stdout_empty());
    testrun(stderr_empty());
    testrun(file_empty(AAA_FILE_NAME));
    testrun(!file_empty(LETS_LOG_SOMETHING_FILE_NAME));
    testrun(file_empty(OV_LOG_CONFIGURE_TEST_FILE_NAME));

    testrun(file_clear(LETS_LOG_SOMETHING_FILE_NAME));

    // Since we set the logging level to `warning` for lets_log_something,
    // there should nothing be logged for lets_log_something

    lets_log_something(OV_LOG_INFO);

    testrun(stdout_empty());
    testrun(stderr_empty());
    testrun(file_empty(AAA_FILE_NAME));
    testrun(file_empty(LETS_LOG_SOMETHING_FILE_NAME));
    testrun(file_empty(OV_LOG_CONFIGURE_TEST_FILE_NAME));

    /*************************************************************************
                                  Log rotation
     ************************************************************************/

#define LETS_LOG_SOMETHING_FILE_NAME OUR_TEMP_DIR "/lets_log_something"

    log_cfg = json_from_string(
        "{"
        "\"" OV_KEY_LEVEL
        "\" : \"info\","
        "\"" OV_KEY_SYSTEMD
        "\" : \"false\","
        "\"" OV_KEY_MODULES
        "\" : {"
        "\"aaa\" : {"
        "\"" OV_KEY_FILE "\" : \"" AAA_FILE_NAME
        "\","
        "\"" OV_KEY_LEVEL
        "\" : \"info\""
        "},"
        "\"ov_config_log_test.c\" : {"
        "\"" OV_KEY_FILE "\" : \"" OV_LOG_CONFIGURE_TEST_FILE_NAME
        "\","
        "\"" OV_KEY_LEVEL
        "\" : \"warning\","
        "\"" OV_KEY_SYSTEMD
        "\" : true,"
        "\"" OV_KEY_FUNCTIONS
        "\" : {"
        "\"lets_log_something\" : {"
        "\"" OV_KEY_FILE
        "\" : "
        "\"" LETS_LOG_SOMETHING_FILE_NAME
        "\","
        "\"" OV_KEY_ROTATE_AFTER_MESSAGES
        "\": 3,"
        "\"" OV_KEY_KEEP_FILES
        "\" : 3,"
        "\"" OV_KEY_LEVEL
        "\" : \"warning\","
        "\"" OV_KEY_SYSTEMD
        "\" : true"
        "}"
        "}"
        "}"
        "}"
        "}");

    testrun(0 != log_cfg);

    testrun(ov_config_log_from_json(log_cfg));
    log_cfg = ov_json_value_free(log_cfg);

    testrun(0 == log_cfg);

    ov_log_error("ERROR");

    testrun(stdout_empty());
    testrun(stderr_empty());
    testrun(file_empty(AAA_FILE_NAME));
    testrun(file_empty(LETS_LOG_SOMETHING_FILE_NAME));
    testrun(!file_empty(OV_LOG_CONFIGURE_TEST_FILE_NAME));

    testrun(file_clear(OV_LOG_CONFIGURE_TEST_FILE_NAME));

    // Now log from out of 'lets_log_something' -> Should be logged into
    // its own log file

    lets_log_something(OV_LOG_ERR);

    testrun(stdout_empty());
    testrun(stderr_empty());
    testrun(file_empty(AAA_FILE_NAME));
    testrun(!file_empty(LETS_LOG_SOMETHING_FILE_NAME));
    testrun(file_empty(OV_LOG_CONFIGURE_TEST_FILE_NAME));

    testrun(file_clear(LETS_LOG_SOMETHING_FILE_NAME));

    // Since we set the logging level to `warning` for lets_log_something,
    // there should nothing be logged for lets_log_something

    lets_log_something(OV_LOG_INFO);

    testrun(stdout_empty());
    testrun(stderr_empty());
    testrun(file_empty(AAA_FILE_NAME));
    testrun(file_empty(LETS_LOG_SOMETHING_FILE_NAME));
    testrun(file_empty(LETS_LOG_SOMETHING_FILE_NAME ".001"));
    testrun(file_empty(LETS_LOG_SOMETHING_FILE_NAME ".002"));

    testrun(file_empty(OV_LOG_CONFIGURE_TEST_FILE_NAME));

    // Check log rotates, we set max messages to 3
    lets_log_something(OV_LOG_ERR);

    testrun(!file_empty(LETS_LOG_SOMETHING_FILE_NAME));
    testrun(file_empty(LETS_LOG_SOMETHING_FILE_NAME ".001"));
    testrun(file_empty(LETS_LOG_SOMETHING_FILE_NAME ".002"));

    lets_log_something(OV_LOG_ERR);
    lets_log_something(OV_LOG_ERR);
    lets_log_something(OV_LOG_ERR);

    testrun(!file_empty(LETS_LOG_SOMETHING_FILE_NAME));
    testrun(!file_empty(LETS_LOG_SOMETHING_FILE_NAME ".001"));
    testrun(file_empty(LETS_LOG_SOMETHING_FILE_NAME ".002"));

    lets_log_something(OV_LOG_ERR);
    lets_log_something(OV_LOG_ERR);
    lets_log_something(OV_LOG_ERR);

    testrun(!file_empty(LETS_LOG_SOMETHING_FILE_NAME));
    testrun(!file_empty(LETS_LOG_SOMETHING_FILE_NAME ".001"));
    testrun(!file_empty(LETS_LOG_SOMETHING_FILE_NAME ".002"));

    /*************************************************************************
                              Log to existing file
     ************************************************************************/

    ov_log_close();

#define EXISTING_LOG_FILE OUR_TEMP_DIR "/we_exist"

    char const test_content[] =
        "denn weit und breit sieht sie ueber die "
        "welten all\n";

    log_cfg = json_from_string(
        "{"
        "\"" OV_KEY_LEVEL
        "\" : \"info\","
        "\"" OV_KEY_SYSTEMD
        "\" : \"false\","
        "\"" OV_KEY_FILE "\" : \"" EXISTING_LOG_FILE
        "\""
        "}");

    testrun(0 != log_cfg);
    testrun(ov_config_log_from_json(log_cfg));

    ov_log_error(test_content);

    ov_log_close();

    // Ensure we actually created the file
    ssize_t file_size = ov_file_read_check_get_bytes(EXISTING_LOG_FILE);

    testrun(0 < file_size);

    // Now reconfigure with same file again...

    testrun(ov_config_log_from_json(log_cfg));
    log_cfg = ov_json_value_free(log_cfg);

    testrun(0 == log_cfg);

    ov_log_error(test_content);
    ov_log_warning(test_content);

    testrun(stdout_empty());
    testrun(stderr_empty());

    // Ensure our log file grew

    ssize_t new_file_size = ov_file_read_check_get_bytes(EXISTING_LOG_FILE);

    testrun(0 < new_file_size);
    testrun((size_t)file_size < (size_t)new_file_size);

    /*************************************************************************
                                   Tear down
     ************************************************************************/

    ov_log_close();

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int tear_down() {

    ov_dir_tree_remove(OUR_TEMP_DIR);
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

OV_TEST_RUN("ov_config_log",
            init,
            test_ov_log_output_from_json,
            test_ov_config_log_from_json,
            tear_down);
