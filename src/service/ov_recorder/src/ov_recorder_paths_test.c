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

        ------------------------------------------------------------------------
*/

#include "ov_recorder_paths.c"

#include "ov_base/ov_utils.h"
#include "ov_recorder_constants.h"
#include "ov_recorder_paths.h"
#include <ov_base/ov_config_keys.h>
#include <ov_base/ov_dir.h>
#include <ov_codec/ov_codec_factory.h>
#include <ov_format/ov_format_codec.h>
#include <ov_format/ov_format_wav.h>
#include <ov_test/ov_test.h>
#include <ov_test/ov_test_file.h>

/*----------------------------------------------------------------------------*/

#define TEST_ROOT_PATH "/tmp/idafeld"
#define TEST_LOOP "ratatoskr"

#define TEST_CODEC                                                             \
    "{\"" OV_KEY_CODEC "\":\"opus\",\"" OV_KEY_SAMPLE_RATE_HERTZ "\":48000}"

#define TEST_FILE_EXTENSION "gefion"

#define TEST_FORMAT "wav"

#define FAKE_SSID 0

/*----------------------------------------------------------------------------*/

static ov_recorder_paths_recording_info get_recording_info() {

    ov_recorder_paths_recording_info info = {

        .loop = strdup(TEST_LOOP),
        .timestamp_epoch = 332211,

    };

    ov_id_fill_with_uuid(info.id);

    return info;
}

/*----------------------------------------------------------------------------*/

int test_ov_recorder_paths_get_recording_dir() {

    testrun(0 == ov_recorder_paths_get_recording_dir(0, 0, 0, 0));

    char target[PATH_MAX] = {0};

    testrun(0 == ov_recorder_paths_get_recording_dir(target, 0, 0, 0));

    testrun(0 == ov_recorder_paths_get_recording_dir(0, sizeof(target), 0, 0));

    testrun(0 ==
            ov_recorder_paths_get_recording_dir(target, sizeof(target), 0, 0));

    testrun(0 == ov_recorder_paths_get_recording_dir(0, 0, TEST_ROOT_PATH, 0));

    testrun(0 ==
            ov_recorder_paths_get_recording_dir(target, 0, TEST_ROOT_PATH, 0));

    testrun(0 == ov_recorder_paths_get_recording_dir(
                     target, sizeof(target), TEST_ROOT_PATH, 0));

    testrun(0 == ov_recorder_paths_get_recording_dir(0, 0, 0, TEST_LOOP));

    testrun(0 == ov_recorder_paths_get_recording_dir(target, 0, 0, TEST_LOOP));

    testrun(0 == ov_recorder_paths_get_recording_dir(
                     0, sizeof(target), 0, TEST_LOOP));

    testrun(0 == ov_recorder_paths_get_recording_dir(
                     0, 0, TEST_ROOT_PATH, TEST_LOOP));

    testrun(0 == ov_recorder_paths_get_recording_dir(
                     target, 0, TEST_ROOT_PATH, TEST_LOOP));

    testrun(0 == ov_recorder_paths_get_recording_dir(
                     0, sizeof(target), TEST_ROOT_PATH, TEST_LOOP));

    testrun(target == ov_recorder_paths_get_recording_dir(
                          target, sizeof(target), TEST_ROOT_PATH, TEST_LOOP));

    testrun(0 != strlen(target));

    /* Check 'canary' at end of target */
    testrun(0 == target[sizeof(target) - 1]);

    memset(target, 0, sizeof(target));

    /* result should contain root path, user and loop and some separators,
     * that is if there is too less mem to contain root path, user and loop,
     * get_recording_dir should fail */
    const size_t too_few_bytes = strlen(TEST_ROOT_PATH) + strlen(TEST_LOOP) - 1;

    testrun(0 == ov_recorder_paths_get_recording_dir(
                     target, too_few_bytes, TEST_ROOT_PATH, TEST_LOOP));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_recorder_get_recording_file_name() {

    ov_id id = {0};
    testrun(ov_id_fill_with_uuid(id));

    testrun(0 == ov_recorder_paths_get_recording_file_name(0, 0, 0, 0, 0, 0));

    /* The number of combinations is way too high to check, thus we check only
     * certain cases
     */

    char target[PATH_MAX] = {0};

    testrun(0 ==
            ov_recorder_paths_get_recording_file_name(target, 0, 0, 0, 0, 0));

    testrun(0 == ov_recorder_paths_get_recording_file_name(
                     0, sizeof(target), 0, 0, 0, 0));

    testrun(0 == ov_recorder_paths_get_recording_file_name(
                     target, sizeof(target), 0, 0, 0, 0));

    testrun(0 == ov_recorder_paths_get_recording_file_name(
                     target, sizeof(target), 0, 0, 0, 0));

    testrun(0 == ov_recorder_paths_get_recording_file_name(
                     target, sizeof(target), TEST_LOOP, 0, 0, 0));

    testrun(
        target ==
        ov_recorder_paths_get_recording_file_name(
            target, sizeof(target), TEST_LOOP, 1432, id, TEST_FILE_EXTENSION));

    testrun(0 != strlen(target));

    fprintf(stderr, "File name is: %s\n", target);

    /* Check 'canary' at end of target */
    testrun(0 == target[sizeof(target) - 1]);

    memset(target, 0, sizeof(target));

    const size_t too_few_bytes =
        strlen(TEST_LOOP) + 4 + sizeof(id) + strlen(TEST_FILE_EXTENSION) - 1;

    testrun(
        0 ==
        ov_recorder_paths_get_recording_file_name(
            target, too_few_bytes, TEST_LOOP, 1234, id, TEST_FILE_EXTENSION));

    testrun(0 == target[sizeof(target) - 1]);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_recorder_get_recording_file_path() {

    ov_id id = {0};
    testrun(ov_id_fill_with_uuid(id));

    testrun(0 ==
            ov_recorder_paths_get_recording_file_path(0, 0, 0, 0, 0, 0, 0));

    /* The number of combinations is way too high to check, thus we check only
     * certain cases
     */

    char target[PATH_MAX] = {0};

    testrun(0 == ov_recorder_paths_get_recording_file_path(
                     target, 0, 0, 0, 0, 0, 0));

    testrun(0 == ov_recorder_paths_get_recording_file_path(
                     0, sizeof(target), 0, 0, 0, 0, 0));

    testrun(0 == ov_recorder_paths_get_recording_file_path(
                     target, sizeof(target), 0, 0, 0, 0, 0));

    testrun(0 == ov_recorder_paths_get_recording_file_path(
                     target, sizeof(target), 0, 0, 0, 0, 0));

    testrun(0 == ov_recorder_paths_get_recording_file_path(
                     target, sizeof(target), 0, TEST_LOOP, 0, 0, 0));

    testrun(target ==
            ov_recorder_paths_get_recording_file_path(target,
                                                      sizeof(target),
                                                      0,
                                                      TEST_LOOP,
                                                      1432,
                                                      id,
                                                      TEST_FILE_EXTENSION));

    testrun(0 != strlen(target));

    /* Check 'canary' at end of target */
    testrun(0 == target[sizeof(target) - 1]);

    memset(target, 0, sizeof(target));

    const size_t too_few_bytes =
        strlen(TEST_LOOP) + 4 + sizeof(id) + strlen(TEST_FILE_EXTENSION) - 1;

    testrun(0 ==
            ov_recorder_paths_get_recording_file_path(target,
                                                      too_few_bytes,
                                                      0,
                                                      TEST_LOOP,
                                                      1234,
                                                      id,
                                                      TEST_FILE_EXTENSION));

    testrun(0 == target[sizeof(target) - 1]);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_recorder_paths_recording_info_to_recording_file_name() {

    char *ov_recorder_paths_recording_info_to_recording_file_name(
        char *target,
        size_t target_size,
        ov_recorder_paths_recording_info const *info,
        char const *sformat);

    testrun(0 == ov_recorder_paths_recording_info_to_recording_file_name(
                     0, 0, 0, 0));

    ov_id id = {0};
    testrun(ov_id_fill_with_uuid(id));

    /* The number of combinations is way too high to check, thus we check only
     * certain cases
     */

    char target[PATH_MAX] = {0};

    testrun(0 == ov_recorder_paths_recording_info_to_recording_file_name(
                     target, 0, 0, 0));

    testrun(0 == ov_recorder_paths_recording_info_to_recording_file_name(
                     0, sizeof(target), 0, 0));

    testrun(0 == ov_recorder_paths_recording_info_to_recording_file_name(
                     target, sizeof(target), 0, 0));

    ov_recorder_paths_recording_info info = {0};

    testrun(0 == ov_recorder_paths_recording_info_to_recording_file_name(
                     0, 0, &info, 0));

    testrun(0 == ov_recorder_paths_recording_info_to_recording_file_name(
                     target, 0, &info, 0));

    testrun(0 == ov_recorder_paths_recording_info_to_recording_file_name(
                     0, sizeof(target), &info, 0));

    testrun(0 == ov_recorder_paths_recording_info_to_recording_file_name(
                     target, sizeof(target), &info, 0));

    testrun(0 == ov_recorder_paths_recording_info_to_recording_file_name(
                     0, 0, 0, TEST_FILE_EXTENSION));

    testrun(0 == ov_recorder_paths_recording_info_to_recording_file_name(
                     target, 0, 0, TEST_FILE_EXTENSION));

    testrun(0 == ov_recorder_paths_recording_info_to_recording_file_name(
                     0, sizeof(target), 0, TEST_FILE_EXTENSION));

    testrun(0 == ov_recorder_paths_recording_info_to_recording_file_name(
                     target, sizeof(target), 0, TEST_FILE_EXTENSION));

    testrun(0 == ov_recorder_paths_recording_info_to_recording_file_name(
                     0, 0, &info, TEST_FILE_EXTENSION));

    testrun(0 == ov_recorder_paths_recording_info_to_recording_file_name(
                     target, 0, &info, TEST_FILE_EXTENSION));

    testrun(0 == ov_recorder_paths_recording_info_to_recording_file_name(
                     0, sizeof(target), &info, TEST_FILE_EXTENSION));

    testrun(0 == ov_recorder_paths_recording_info_to_recording_file_name(
                     target, sizeof(target), &info, TEST_FILE_EXTENSION));

    testrun(0 == ov_recorder_paths_get_recording_file_name(
                     target, sizeof(target), TEST_LOOP, 0, 0, 0));

    info = (ov_recorder_paths_recording_info){
        .loop = TEST_LOOP,
        .timestamp_epoch = 1432,
    };

    ov_id_set(info.id, id);
    testrun(target == ov_recorder_paths_recording_info_to_recording_file_name(
                          target, sizeof(target), &info, TEST_FILE_EXTENSION));

    testrun(0 != strlen(target));

    /* Check 'canary' at end of target */
    testrun(0 == target[sizeof(target) - 1]);

    memset(target, 0, sizeof(target));

    const size_t too_few_bytes =
        strlen(TEST_LOOP) + 4 + sizeof(id) + strlen(TEST_FILE_EXTENSION) - 1;

    testrun(0 == ov_recorder_paths_recording_info_to_recording_file_name(
                     target, too_few_bytes, &info, TEST_FILE_EXTENSION));

    testrun(0 == target[sizeof(target) - 1]);

    return testrun_log_success();

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_recorder_paths_recording_info_to_recording_file_path() {

    testrun(0 == ov_recorder_paths_recording_info_to_recording_file_path(
                     0, 0, 0, 0, 0));

    ov_id id = {0};
    testrun(ov_id_fill_with_uuid(id));

    /* The number of combinations is way too high to check, thus we check only
     * certain cases
     */

    char target[PATH_MAX] = {0};

    testrun(0 == ov_recorder_paths_recording_info_to_recording_file_path(
                     target, 0, 0, 0, 0));

    testrun(0 == ov_recorder_paths_recording_info_to_recording_file_path(
                     0, sizeof(target), 0, 0, 0));

    testrun(0 == ov_recorder_paths_recording_info_to_recording_file_path(
                     target, sizeof(target), 0, 0, 0));

    ov_recorder_paths_recording_info info = {0};

    testrun(0 == ov_recorder_paths_recording_info_to_recording_file_path(
                     0, 0, 0, &info, 0));

    testrun(0 == ov_recorder_paths_recording_info_to_recording_file_path(
                     target, 0, 0, &info, 0));

    testrun(0 == ov_recorder_paths_recording_info_to_recording_file_path(
                     0, sizeof(target), 0, &info, 0));

    testrun(0 == ov_recorder_paths_recording_info_to_recording_file_path(
                     target, sizeof(target), 0, &info, 0));

    testrun(0 == ov_recorder_paths_recording_info_to_recording_file_path(
                     0, 0, 0, 0, TEST_FILE_EXTENSION));

    testrun(0 == ov_recorder_paths_recording_info_to_recording_file_path(
                     target, 0, 0, 0, TEST_FILE_EXTENSION));

    testrun(0 == ov_recorder_paths_recording_info_to_recording_file_path(
                     0, sizeof(target), 0, 0, TEST_FILE_EXTENSION));

    testrun(0 == ov_recorder_paths_recording_info_to_recording_file_path(
                     target, sizeof(target), 0, 0, TEST_FILE_EXTENSION));

    testrun(0 == ov_recorder_paths_recording_info_to_recording_file_path(
                     0, 0, 0, &info, TEST_FILE_EXTENSION));

    testrun(0 == ov_recorder_paths_recording_info_to_recording_file_path(
                     target, 0, 0, &info, TEST_FILE_EXTENSION));

    testrun(0 == ov_recorder_paths_recording_info_to_recording_file_path(
                     0, sizeof(target), 0, &info, TEST_FILE_EXTENSION));

    testrun(0 == ov_recorder_paths_recording_info_to_recording_file_path(
                     target, sizeof(target), 0, &info, TEST_FILE_EXTENSION));

    info = (ov_recorder_paths_recording_info){
        .loop = TEST_LOOP,
        .timestamp_epoch = 1432,
    };

    ov_id_set(info.id, id);
    testrun(target ==
            ov_recorder_paths_recording_info_to_recording_file_path(
                target, sizeof(target), 0, &info, TEST_FILE_EXTENSION));

    fprintf(stderr, "File name is: %s\n", target);

    testrun(0 != strlen(target));

    /* Check 'canary' at end of target */
    testrun(0 == target[sizeof(target) - 1]);

    memset(target, 0, sizeof(target));

    testrun(
        target ==
        ov_recorder_paths_recording_info_to_recording_file_path(
            target, sizeof(target), "/tartar/", &info, TEST_FILE_EXTENSION));

    fprintf(stderr, "File name is: %s\n", target);

    testrun(0 != strlen(target));

    /* Check 'canary' at end of target */
    testrun(0 == target[sizeof(target) - 1]);

    memset(target, 0, sizeof(target));
    const size_t too_few_bytes =
        strlen(TEST_LOOP) + 4 + sizeof(id) + strlen(TEST_FILE_EXTENSION) - 1;

    testrun(0 == ov_recorder_paths_recording_info_to_recording_file_path(
                     target, too_few_bytes, 0, &info, TEST_FILE_EXTENSION));

    testrun(0 == target[sizeof(target) - 1]);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_recorder_paths_get_format_to_record() {

    ov_format_wav_install(0);
    ov_format_codec_install(0);

    ov_json_value *jcodec =
        ov_json_value_from_string(TEST_CODEC, strlen(TEST_CODEC) + 1);
    testrun(0 != jcodec);

    ov_codec *codec =
        ov_codec_factory_get_codec_from_json(0, jcodec, FAKE_SSID);

    testrun(0 != codec);

    jcodec = ov_json_value_free(jcodec);
    testrun(0 == jcodec);

    ov_recorder_paths_recording_info info = get_recording_info();

    testrun(0 == ov_recorder_paths_get_format_to_record(0, 0, 0, 0, 0));
    testrun(0 ==
            ov_recorder_paths_get_format_to_record(TEST_ROOT_PATH, 0, 0, 0, 0));

    testrun(0 == ov_recorder_paths_get_format_to_record(0, &info, 0, 0, 0));

    testrun(0 == ov_recorder_paths_get_format_to_record(
                     TEST_ROOT_PATH, &info, 0, 0, 0));

    testrun(0 ==
            ov_recorder_paths_get_format_to_record(0, 0, TEST_FORMAT, 0, 0));

    testrun(0 == ov_recorder_paths_get_format_to_record(
                     TEST_ROOT_PATH, 0, TEST_FORMAT, 0, 0));

    testrun(0 == ov_recorder_paths_get_format_to_record(0, 0, 0, 0, codec));

    testrun(0 == ov_recorder_paths_get_format_to_record(
                     TEST_ROOT_PATH, 0, 0, 0, codec));

    testrun(0 == ov_recorder_paths_get_format_to_record(0, &info, 0, 0, codec));

    testrun(0 == ov_recorder_paths_get_format_to_record(
                     TEST_ROOT_PATH, &info, 0, 0, codec));

    testrun(0 == ov_recorder_paths_get_format_to_record(
                     0, 0, TEST_FORMAT, 0, codec));

    testrun(0 == ov_recorder_paths_get_format_to_record(
                     TEST_ROOT_PATH, 0, TEST_FORMAT, 0, codec));

    testrun(0 == ov_recorder_paths_get_format_to_record(
                     0, &info, TEST_FORMAT, 0, codec));

    ov_format *fmt = ov_recorder_paths_get_format_to_record(
        TEST_ROOT_PATH, &info, TEST_FORMAT, 0, codec);

    testrun(0 != fmt);

    /* The opus codec only allows certain input buffer lengths,
     * for 48k 120 frames is the smallest permitted length,
     * and as one frame is 2 bytes -> 240 bytes input buffer required */
    uint8_t opus_input[240] = {0};

    ov_buffer buf = {
        .start = opus_input,
        .length = sizeof(opus_input),
    };

    ssize_t written = ov_format_payload_write_chunk(fmt, &buf);

    testrun(0 < written);
    /* Opus should not write more bytes than it received */
    testrun(buf.length >= (size_t)written);

    fmt = ov_format_close(fmt);
    testrun(0 == fmt);

    /* Remove test file */
    char path[PATH_MAX] = {0};

    testrun(path == ov_recorder_paths_get_recording_dir(
                        path, sizeof(path), TEST_ROOT_PATH, info.loop));

    char file_name[PATH_MAX] = {0};

    testrun(file_name ==
            ov_recorder_paths_get_recording_file_name(file_name,
                                                      sizeof(file_name),
                                                      info.loop,
                                                      info.timestamp_epoch,
                                                      info.id,
                                                      TEST_FORMAT));

    testrun(path == strcat(path, "/"));
    testrun(path == strcat(path, file_name));

    fprintf(stderr, "temp file to remove: %s\n", path);

    unlink(path);

    /* Free remaining resources */

    ov_free(info.loop);

    ov_format_registry_clear(0);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_recorder_paths_get_info_for_path() {

    testrun(!ov_recorder_paths_get_info_for_path(0, 0));

    ov_id id;
    ov_id_fill_with_uuid(id);

    char recording[PATH_MAX] = {0};

    ov_recorder_paths_get_recording_file_name(
        recording, sizeof(recording), "ratatoskr", 1492, id, ".wav");

    testrun(!ov_recorder_paths_get_info_for_path(recording, 0));

    ov_recorder_paths_recording_info info = {0};

    testrun(!ov_recorder_paths_get_info_for_path(0, &info));

    /* Target memory for info members */
    char loop[225] = {0};

    /*                  Succeed - path with directories                       */

    testrun(ov_recorder_paths_get_info_for_path(recording, &info));

    testrun(0 == info.loop);
    testrun(!ov_id_valid(info.id));

    testrun(1492 == info.timestamp_epoch);

    info.loop = loop;
    info.loop_capacity = sizeof(loop);

    testrun(ov_recorder_paths_get_info_for_path(recording, &info));

    testrun(0 != info.loop);
    testrun(!ov_id_valid(info.id));

    testrun(0 == strcmp(info.loop, "ratatoskr"));
    testrun(1492 == info.timestamp_epoch);

    memset(loop, 0, sizeof(loop));

    info.loop = 0;
    ov_id_set(info.id, id);

    testrun(ov_recorder_paths_get_info_for_path(recording, &info));

    testrun(0 == info.loop);
    testrun(ov_id_valid(info.id));

    testrun(1492 == info.timestamp_epoch);

    ov_id parsed_id;

    testrun(ov_id_match(id, parsed_id));

    info.loop = loop;
    ov_id_clear(info.id);

    testrun(ov_recorder_paths_get_info_for_path(recording, &info));

    testrun(0 != info.loop);
    testrun(ov_id_valid(info.id));

    testrun(1492 == info.timestamp_epoch);
    testrun(0 == strcmp(info.loop, "ratatoskr"));

    testrun(ov_id_match(id, info.id));

    ov_id_clear(info.id);

    /*                     Success - query all fields                         */

    testrun(ov_recorder_paths_get_info_for_path(recording, &info));

    testrun(0 != info.loop);
    testrun(0 == strcmp(info.loop, "ratatoskr"));
    testrun(1492 == info.timestamp_epoch);

    testrun(ov_id_match(id, info.id));
    ov_id_clear(info.id);

    /*       Succeed - also with bare filename without directories            */

    char *filename = basename(recording);
    testrun(0 != filename);

    testrun(ov_recorder_paths_get_info_for_path(filename, &info));

    testrun(0 != info.loop);
    testrun(0 == strcmp(info.loop, "ratatoskr"));
    testrun(1492 == info.timestamp_epoch);

    testrun(ov_id_match(id, info.id));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int tear_down() {

    ov_codec_factory_free(0);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

OV_TEST_RUN("ov_recorder_paths",
            test_ov_recorder_paths_get_recording_dir,
            test_ov_recorder_get_recording_file_name,
            test_ov_recorder_get_recording_file_path,
            test_ov_recorder_paths_recording_info_to_recording_file_name,
            test_ov_recorder_paths_recording_info_to_recording_file_path,
            test_ov_recorder_paths_get_format_to_record,
            tear_down);

/*----------------------------------------------------------------------------*/
