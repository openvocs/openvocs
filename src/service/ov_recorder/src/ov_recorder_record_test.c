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

#include "ov_recorder_record.c"
#include <ov_base/ov_dir.h>
#include <ov_base/ov_registered_cache.h>
#include <ov_base/ov_teardown.h>
#include <ov_test/ov_test.h>
#include <ov_test/ov_test_file.h>
#include <time.h>

char const *TEST_CODEC_DEF =
    "{\"" OV_KEY_CODEC
    "\":\"pcm16_signed\", "
    "\"" OV_KEY_ENDIANNESS "\":\"" OV_KEY_LITTLE_ENDIAN "\"}";

/*----------------------------------------------------------------------------*/

static void install_default_formats() {

    ov_format_wav_install(0);
    ov_format_codec_install(0);
}

/*----------------------------------------------------------------------------*/

static ov_recorder_record_start *get_start_events(size_t num) {

    size_t max_num_digits_size_t = log((double)SIZE_MAX) + 1;

    ov_recorder_record_start *e = calloc(num, sizeof(ov_recorder_record_start));

    for (size_t i = 0; i < num; ++i) {

        size_t max_name_len = 5 + max_num_digits_size_t + 1;
        ov_id_fill_with_uuid(e[i].id);
        e[i].loop = calloc(1, max_name_len);

        snprintf(e[i].loop, max_name_len, "loop-%5zu", i);

        e[i].timestamp_epoch = time(0);
        e[i].ssid = i;
        e[i].codec =
            ov_json_value_from_string(TEST_CODEC_DEF, strlen(TEST_CODEC_DEF));

        OV_ASSERT(0 != e[i].codec);
    }

    return e;
}

/*----------------------------------------------------------------------------*/

static bool recording_fits(ov_recorder_record_start start,
                           ov_recording recording) {

    return (recording.start_epoch_secs == start.timestamp_epoch) &&
           ov_id_match(start.id, recording.id) &&
           ov_string_equal(recording.loop, start.loop);
}

/*----------------------------------------------------------------------------*/

static ov_recorder_record_start *free_record_starts(
    ov_recorder_record_start *events, size_t num) {

    OV_ASSERT(0 != events);

    for (size_t i = 0; i < num; ++i) {

        ov_free(events[i].loop);
        events[i].codec = ov_json_value_free(events[i].codec);
    }

    free(events);

    return 0;
}

/*****************************************************************************
                            check_running_recordings
 ****************************************************************************/

struct id_searcher_arg {

    ov_id const *ids;
    size_t num_ids;

    bool *found;
    bool unknown_ids_seen;
    bool failure;
};

static bool id_searcher(const void *key, void *value, void *data) {

    UNUSED(value);

    struct id_searcher_arg *args = data;
    char const *key_str = key;

    ov_id id;
    if (!ov_id_set(id, key_str)) {

        testrun_log_error("not a id: %s", key_str);
        args->failure = true;
        return false;
    }

    ssize_t index_found = -1;

    for (size_t i = 0; i < args->num_ids; ++i) {

        if (ov_id_match(id, args->ids[i])) {
            index_found = i;
            break;
        }
    }

    if (0 > index_found) {

        args->unknown_ids_seen = true;
        return true;
    }

    if (args->found[index_found]) {

        testrun_log_error("UUID %s appears several times?", key_str);
        args->failure = false;
        return false;
    }

    args->found[index_found] = true;

    return true;
}

/*----------------------------------------------------------------------------*/

static bool check_running_recordings(ov_recorder_record *rec,
                                     ov_id const *ids,
                                     size_t num_ids) {

    OV_ASSERT(0 != rec);

    bool retval = false;

    ov_json_value *jval = ov_json_object();
    OV_ASSERT(0 != jval);

    if (!ov_recorder_record_get_running_recordings(rec, jval)) {

        goto error;
    }

    struct id_searcher_arg args = {0};
    args.ids = ids;
    args.num_ids = num_ids;
    args.found = calloc(num_ids, sizeof(bool));

    OV_ASSERT(0 != args.found);

    /* scan */

    ov_json_object_for_each(jval, &args, id_searcher);

    /* Collapse result */

    if (args.unknown_ids_seen) goto error;

    for (size_t i = 0; i < num_ids; ++i) {

        if (!args.found[i]) goto error;
    }

    retval = true;

error:

    /* Clear */

    jval = ov_json_value_free(jval);

    free(args.found);
    args.found = 0;

    return retval;
}

/*----------------------------------------------------------------------------*/

#define TEMP_REPO_ROOT_PREFIX "/tmp/ov_recorder_record_test"

static ov_recorder_record_config get_record_config() {

    ov_recorder_record_config cfg = {

        .repository_root_path = ov_test_temp_dir(TEMP_REPO_ROOT_PREFIX),

    };

    return cfg;
}

/*----------------------------------------------------------------------------*/

static void clear_record_config(ov_recorder_record_config cfg) {

    if (0 != cfg.repository_root_path) {

        free((char *)cfg.repository_root_path);
    }

    cfg.repository_root_path = 0;
}

/*----------------------------------------------------------------------------*/

static int test_ov_recorder_record_create() {

    ov_recorder_record_config cfg = get_record_config();
    fprintf(stderr, "Created temp dir: %s\n", cfg.repository_root_path);

    testrun(0 == ov_recorder_record_create(0, cfg));

    ov_event_loop *loop = ov_event_loop_default(ov_event_loop_config_default());
    testrun(0 != loop);

    ov_recorder_record *rec = ov_recorder_record_create(loop, cfg);
    testrun(0 != rec);

    rec = ov_recorder_record_free(rec);
    testrun(0 == rec);

    ov_dir_tree_remove(cfg.repository_root_path);

    clear_record_config(cfg);

    loop = ov_event_loop_free(loop);

    testrun(0 == loop);

    ov_format_registry_clear(0);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_data = 18;

static bool cb_dummy(ov_recording recording, void *data) {

    UNUSED(recording);
    UNUSED(data);
    return true;
}

ov_recorder_record_callbacks callbacks = {
    .cb_recording_stopped = cb_dummy,
    .data = &test_data,
};

/*----------------------------------------------------------------------------*/

static int test_ov_recorder_record_start_recording() {

    install_default_formats();

    testrun(!ov_recorder_record_start_recording(0, 0, callbacks, 0));

    ov_recorder_record_config cfg = get_record_config();
    fprintf(stderr, "Created temp dir: %s\n", cfg.repository_root_path);

    ov_event_loop *loop = ov_event_loop_default(ov_event_loop_config_default());
    testrun(0 != loop);

    ov_recorder_record *rec = ov_recorder_record_create(loop, cfg);
    testrun(0 != rec);

    testrun(!ov_recorder_record_start_recording(rec, 0, callbacks, 0));

    /* Ok, error cases done, lets go for adding some entries */

    ov_id ids[5] = {0};

    const size_t num_events = sizeof(ids) / sizeof(ids[0]);

    ov_recorder_record_start *events = get_start_events(num_events);

    for (size_t i = 0; i < num_events; ++i) {

        ov_id_set(ids[i], events[i].id);
    }

    testrun(!ov_recorder_record_start_recording(0, events + 0, callbacks, 0));

    /* Now start checking valid cases */

    testrun(ov_recorder_record_start_recording(rec, events + 0, callbacks, 0));
    testrun(check_running_recordings(rec, ids, 1));

    testrun(ov_recorder_record_start_recording(rec, events + 1, callbacks, 0));
    testrun(check_running_recordings(rec, ids, 2));

    testrun(ov_recorder_record_start_recording(rec, events + 2, callbacks, 0));
    testrun(check_running_recordings(rec, ids, 3));

    /* Clear */

    rec = ov_recorder_record_free(rec);
    testrun(0 == rec);

    ov_dir_tree_remove(cfg.repository_root_path);

    clear_record_config(cfg);

    loop = ov_event_loop_free(loop);

    testrun(0 == loop);

    ov_format_registry_clear(0);

    events = free_record_starts(events, num_events);
    testrun(0 == events);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_recorder_record_stop_recording() {

    ov_recording recording = {0};

    install_default_formats();

    testrun(!ov_recorder_record_stop_recording(0, 0, 0));

    ov_recorder_record_config cfg = get_record_config();
    fprintf(stderr, "Created temp dir: %s\n", cfg.repository_root_path);

    ov_event_loop *loop = ov_event_loop_default(ov_event_loop_config_default());
    testrun(0 != loop);

    ov_recorder_record *rec = ov_recorder_record_create(loop, cfg);
    testrun(0 != rec);

    testrun(!ov_recorder_record_stop_recording(rec, 0, 0));

    /* Ok, error cases done, lets go for adding some entries */

    ov_id ids[5] = {0};

    ov_recorder_event_stop events[sizeof(ids) / sizeof(ids[0])] = {0};

    const size_t num_events = sizeof(ids) / sizeof(ids[0]);

    ov_recorder_record_start *start = get_start_events(num_events);

    for (size_t i = 0; i < num_events; ++i) {

        ov_id_set(ids[i], start[i].id);
        ov_id_set(events[i].id, ids[i]);
    }

    testrun(ov_recorder_record_stop_recording(rec, events + 0, 0));

    testrun(check_running_recordings(rec, ids, 0));

    /* Check with one recording */
    testrun(ov_recorder_record_start_recording(rec, start + 0, callbacks, 0));

    testrun(ov_recorder_record_stop_recording(rec, events + 1, 0));

    testrun(check_running_recordings(rec, ids, 1));

    testrun(ov_recorder_record_stop_recording(rec, events + 0, &recording));
    testrun(recording_fits(start[0], recording));
    ov_recording_clear(&recording);

    testrun(check_running_recordings(rec, ids, 0));

    /* Check with many recordings */

    for (size_t i = 0; i < num_events; ++i) {

        testrun(check_running_recordings(rec, ids, i));

        testrun(
            ov_recorder_record_start_recording(rec, start + i, callbacks, 0));

        /* If i is the index of the topmost recording, then there are in fact
         * 0 .. i recordings, which summarize to a number of i + 1
         */
        size_t num_running_recordings = i + 1;

        for (size_t j = i + 1; j < num_events; ++j) {

            testrun(ov_recorder_record_stop_recording(rec, events + j, 0));
            testrun(check_running_recordings(rec, ids, num_running_recordings));
        }
    }

    for (size_t i = 0; i < num_events; ++i) {

        testrun(ov_recorder_record_stop_recording(rec, events + i, &recording));

        testrun(recording_fits(start[i], recording));
        ov_recording_clear(&recording);

        size_t index_first_running_recording = i + 1;
        size_t num_running_recordings =
            num_events - index_first_running_recording;

        testrun(check_running_recordings(
            rec, ids + index_first_running_recording, num_running_recordings));
    }

    /* Clear */

    rec = ov_recorder_record_free(rec);
    testrun(0 == rec);

    ov_dir_tree_remove(cfg.repository_root_path);

    clear_record_config(cfg);

    loop = ov_event_loop_free(loop);

    testrun(0 == loop);

    ov_format_registry_clear(0);

    start = free_record_starts(start, num_events);
    testrun(0 == start);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_recorder_record_get_running_recordings() {

    install_default_formats();

    testrun(!ov_recorder_record_get_running_recordings(0, 0));

    ov_event_loop *loop = ov_event_loop_default(ov_event_loop_config_default());

    testrun(0 != loop);

    ov_recorder_record_config cfg = get_record_config();
    fprintf(stderr, "Created temp dir: %s\n", cfg.repository_root_path);

    ov_recorder_record *rec = ov_recorder_record_create(loop, cfg);
    testrun(0 != rec);

    testrun(!ov_recorder_record_get_running_recordings(rec, 0));

    ov_json_value *jval = ov_json_object();

    testrun(ov_recorder_record_get_running_recordings(rec, jval));

    testrun(0 == ov_json_object_count(jval));

    jval = ov_json_value_free(jval);
    testrun(0 == jval);

    /* Functional tests covered by test_ov_recorder_record_start_recording */

    /* clean up */

    rec = ov_recorder_record_free(rec);
    testrun(0 == rec);

    ov_dir_tree_remove(cfg.repository_root_path);
    clear_record_config(cfg);

    loop = ov_event_loop_free(loop);

    testrun(0 == loop);

    ov_format_registry_clear(0);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_recorder_record_free() {

    install_default_formats();

    testrun(0 == ov_recorder_record_free(0));

    ov_event_loop *loop = ov_event_loop_default(ov_event_loop_config_default());
    testrun(0 != loop);

    ov_recorder_record_config cfg = get_record_config();
    fprintf(stderr, "Created temp dir: %s\n", cfg.repository_root_path);

    ov_recorder_record *rec = ov_recorder_record_create(loop, cfg);
    testrun(0 != rec);

    testrun(0 == ov_recorder_record_free(rec));

    /* Check again with some running recordings */

    rec = ov_recorder_record_create(loop, cfg);
    testrun(0 != rec);

    const size_t num_recordings = 3;

    ov_recorder_record_start *start_events = get_start_events(num_recordings);

    for (size_t i = 0; i < num_recordings; ++i) {

        testrun(ov_recorder_record_start_recording(
            rec, start_events + i, callbacks, 0));
    }

    rec = ov_recorder_record_free(rec);
    testrun(0 == rec);

    /* Clean up */

    start_events = free_record_starts(start_events, num_recordings);

    ov_dir_tree_remove(cfg.repository_root_path);

    clear_record_config(cfg);

    loop = ov_event_loop_free(loop);
    testrun(0 == loop);

    ov_format_registry_clear(0);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_recorder_record_send_to_threads() {

    install_default_formats();

    testrun(!ov_recorder_record_send_to_threads(0, 0));

    ov_event_loop *loop = ov_event_loop_default(ov_event_loop_config_default());
    testrun(0 != loop);

    ov_recorder_record_config cfg = get_record_config();
    fprintf(stderr, "Created temp dir: %s\n", cfg.repository_root_path);

    ov_recorder_record *rec = ov_recorder_record_create(loop, cfg);
    testrun(0 != rec);

    testrun(!ov_recorder_record_send_to_threads(rec, 0));

    ov_rtp_frame *frame = 0;
    ov_thread_message *msg = ov_rtp_frame_message_create(frame);
    testrun(0 != msg);

    testrun(ov_recorder_record_send_to_threads(rec, msg));

    /* Cleanup */

    rec = ov_recorder_record_free(rec);
    testrun(0 == rec);

    ov_dir_tree_remove(cfg.repository_root_path);

    clear_record_config(cfg);

    loop = ov_event_loop_free(loop);
    testrun(0 == loop);

    ov_format_registry_clear(0);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int tear_down() {

    ov_codec_factory_free(0);
    ov_teardown();

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

OV_TEST_RUN("ov_recorder_record",
            test_ov_recorder_record_create,
            test_ov_recorder_record_start_recording,
            test_ov_recorder_record_stop_recording,
            test_ov_recorder_record_get_running_recordings,
            test_ov_recorder_record_free,
            test_ov_recorder_record_send_to_threads,
            tear_down);
