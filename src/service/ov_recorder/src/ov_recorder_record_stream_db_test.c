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
#include "ov_recorder_record_stream_db.c"
#include <ov_base/ov_config_keys.h>
#include <ov_codec/ov_codec_factory.h>
#include <ov_format/ov_format_registry.h>
#include <ov_format/ov_format_wav.h>
#include <ov_test/ov_test.h>

/*----------------------------------------------------------------------------*/

static ov_codec *get_codec(char const *codec_def) {

    if (0 == codec_def) {

        codec_def = "{\"" OV_KEY_CODEC
                    "\":\"pcm16_signed\", "
                    "\"" OV_KEY_ENDIANNESS "\":\"" OV_KEY_LITTLE_ENDIAN "\"}";
    }

    OV_ASSERT(0 != codec_def);

    ov_json_value *jdef =
        ov_json_value_from_string(codec_def, strlen(codec_def));
    OV_ASSERT(0 != jdef);

    ov_codec_factory *factory = ov_codec_factory_create_standard();

    ov_codec *codec = ov_codec_factory_get_codec_from_json(factory, jdef, 23);

    OV_ASSERT(0 != codec);

    factory = ov_codec_factory_free(factory);

    jdef = ov_json_value_free(jdef);

    OV_ASSERT(0 == factory);
    OV_ASSERT(0 == jdef);

    return codec;
}

/*----------------------------------------------------------------------------*/

static int test_ov_recorder_record_stream_db_create() {

    testrun(0 == ov_recorder_record_stream_db_create(0));

    ov_recorder_record_stream_db *db = ov_recorder_record_stream_db_create(123);

    testrun(0 != db);

    db = ov_recorder_record_stream_db_free(db);
    testrun(0 == db);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_recorder_record_stream_db_free() {

    testrun(0 == ov_recorder_record_stream_db_free(0));

    ov_recorder_record_stream_db *db = ov_recorder_record_stream_db_create(123);

    testrun(0 != db);

    db = ov_recorder_record_stream_db_free(db);
    testrun(0 == db);

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

static int test_ov_recorder_record_stream_db_add() {

    const uint32_t MSID = 12;

    ov_id id;
    ov_id_fill_with_uuid(id);

    testrun(!ov_recorder_record_stream_db_add(0, 0, 0, 0, id, callbacks));

    ov_recorder_record_stream_db *db = ov_recorder_record_stream_db_create(10);

    testrun(0 != db);

    testrun(!ov_recorder_record_stream_db_add(db, 0, 0, 0, id, callbacks));
    testrun(!ov_recorder_record_stream_db_add(0, MSID, 0, 0, id, callbacks));
    testrun(!ov_recorder_record_stream_db_add(db, MSID, 0, 0, id, callbacks));

    ov_codec *codec = get_codec(0);
    testrun(0 != codec);

    testrun(!ov_recorder_record_stream_db_add(0, 0, codec, 0, id, callbacks));
    testrun(!ov_recorder_record_stream_db_add(db, 0, codec, 0, id, callbacks));
    testrun(
        !ov_recorder_record_stream_db_add(0, MSID, codec, 0, id, callbacks));
    testrun(
        !ov_recorder_record_stream_db_add(db, MSID, codec, 0, id, callbacks));

    char const *lname = "loopinglouie";

    testrun(!ov_recorder_record_stream_db_add(0, 0, 0, lname, id, callbacks));

    testrun(!ov_recorder_record_stream_db_add(db, 0, 0, lname, id, callbacks));
    testrun(
        !ov_recorder_record_stream_db_add(0, MSID, 0, lname, id, callbacks));
    testrun(
        !ov_recorder_record_stream_db_add(db, MSID, 0, lname, id, callbacks));

    testrun(
        !ov_recorder_record_stream_db_add(0, 0, codec, lname, id, callbacks));

    /* Finally - SUCCESS */

    testrun(
        ov_recorder_record_stream_db_add(db, 0, codec, lname, id, callbacks));

    /* Those have been consumed by add */
    codec = 0;

    codec = get_codec(0);

    testrun(0 != codec);

    /* Adding same ssid twice should fail */
    testrun(
        !ov_recorder_record_stream_db_add(db, 0, codec, lname, id, callbacks));

    testrun(ov_recorder_record_stream_db_add(
        db, MSID, codec, lname, id, callbacks));

    /*                  Check if records have been added                      */

    ov_recorder_record_stream_entry *entry =
        ov_recorder_record_stream_db_get(db, 0);
    testrun(0 != entry);

    ov_thread_lock_unlock(&entry->lock);

    entry = ov_recorder_record_stream_db_get(db, MSID);
    testrun(0 != entry);

    ov_thread_lock_unlock(&entry->lock);

    /*                              Cleanup                                   */

    db = ov_recorder_record_stream_db_free(db);

    testrun(0 == db);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_recorder_record_stream_db_get() {

    testrun(0 == ov_recorder_record_stream_db_get(0, 0));
    testrun(0 == ov_recorder_record_stream_db_get(0, 12));

    ov_recorder_record_stream_db *db = ov_recorder_record_stream_db_create(10);

    testrun(0 != db);

    ov_codec *codec_12 = get_codec(0);
    testrun(0 != codec_12);

    ov_id id12;
    ov_id_fill_with_uuid(id12);

    char const *lname = "loopinglouie2";

    testrun(ov_recorder_record_stream_db_add(
        db, 12, codec_12, lname, id12, callbacks));

    ov_recorder_record_stream_entry *entry =
        ov_recorder_record_stream_db_get(db, 12);

    testrun(0 != entry);

    testrun(codec_12 == entry->source_codec);
    testrun(ov_id_match(id12, entry->id));

    testrun(ov_thread_lock_unlock(&entry->lock));

    /* MSID 1 not registered */
    testrun(0 == ov_recorder_record_stream_db_get(db, 1));

    /*                             2nd entry                                  */

    ov_codec *codec_1 = get_codec(0);
    testrun(0 != codec_1);

    ov_id id1;
    ov_id_fill_with_uuid(id1);

    testrun(ov_recorder_record_stream_db_add(
        db, 1, codec_1, lname, id1, callbacks));

    entry = ov_recorder_record_stream_db_get(db, 1);

    testrun(0 != entry);

    testrun(codec_1 == entry->source_codec);
    testrun(ov_id_match(id1, entry->id));

    testrun(ov_thread_lock_unlock(&entry->lock));

    /* MSID 13 not registered */
    testrun(0 == ov_recorder_record_stream_db_get(db, 13));

    entry = ov_recorder_record_stream_db_get(db, 12);

    testrun(0 != entry);

    testrun(codec_12 == entry->source_codec);

    testrun(ov_thread_lock_unlock(&entry->lock));

    /*                              Cleanup                                   */

    db = ov_recorder_record_stream_db_free(db);

    testrun(0 == db);
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_recorder_record_stream_db_remove_msid() {

    testrun(!ov_recorder_record_stream_db_remove_msid(0, 0));

    ov_recorder_record_stream_db *db = ov_recorder_record_stream_db_create(10);

    testrun(0 != db);

    /* No entries added yet - thus all the entries we try to remove_msid
     * should not be within the db and hence remove_msid should return true */
    testrun(ov_recorder_record_stream_db_remove_msid(db, 0));
    testrun(ov_recorder_record_stream_db_remove_msid(db, 1));
    testrun(ov_recorder_record_stream_db_remove_msid(db, 12));

    /*                   Check with one existing entry                        */

    ov_codec *codec_12 = get_codec(0);
    testrun(0 != codec_12);

    ov_id id12;
    ov_id_fill_with_uuid(id12);

    char const *lname = "loopinglouie3";

    testrun(ov_recorder_record_stream_db_add(
        db, 12, codec_12, lname, id12, callbacks));

    ov_recorder_record_stream_entry *entry =
        ov_recorder_record_stream_db_get(db, 12);

    testrun(0 != entry);

    testrun(codec_12 == entry->source_codec);

    testrun(ov_thread_lock_unlock(&entry->lock));

    testrun(ov_recorder_record_stream_db_remove_msid(db, 0));
    testrun(ov_recorder_record_stream_db_remove_msid(db, 1));
    testrun(ov_recorder_record_stream_db_remove_msid(db, 12));

    /* Check that entry has indeed gone */

    testrun(0 == ov_recorder_record_stream_db_get(db, 0));
    testrun(0 == ov_recorder_record_stream_db_get(db, 1));
    testrun(0 == ov_recorder_record_stream_db_get(db, 12));

    /*                     Check with several entries                         */

    codec_12 = get_codec(0);
    testrun(0 != codec_12);

    testrun(ov_recorder_record_stream_db_add(
        db, 12, codec_12, lname, id12, callbacks));

    ov_codec *codec_0 = get_codec(0);
    testrun(0 != codec_0);

    ov_id id0;
    ov_id_fill_with_uuid(id0);

    testrun(ov_recorder_record_stream_db_add(
        db, 0, codec_0, lname, id0, callbacks));

    /* Check both entries are there indeed */

    entry = ov_recorder_record_stream_db_get(db, 0);

    testrun(0 != entry);

    testrun(codec_0 == entry->source_codec);

    testrun(ov_thread_lock_unlock(&entry->lock));

    entry = ov_recorder_record_stream_db_get(db, 12);

    testrun(0 != entry);

    testrun(codec_12 == entry->source_codec);

    testrun(ov_thread_lock_unlock(&entry->lock));

    /* 0 and 12 are actually present, 1 is not */

    testrun(ov_recorder_record_stream_db_remove_msid(db, 12));

    /* check that entry 12 is gone, 0 is still present */

    entry = ov_recorder_record_stream_db_get(db, 0);

    testrun(0 != entry);

    testrun(codec_0 == entry->source_codec);

    testrun(ov_thread_lock_unlock(&entry->lock));

    testrun(0 == ov_recorder_record_stream_db_get(db, 1));
    testrun(0 == ov_recorder_record_stream_db_get(db, 12));

    /* Remove entry 0 */

    testrun(ov_recorder_record_stream_db_remove_msid(db, 0));

    /* Check that entry has indeed gone */

    testrun(0 == ov_recorder_record_stream_db_get(db, 0));
    testrun(0 == ov_recorder_record_stream_db_get(db, 1));
    testrun(0 == ov_recorder_record_stream_db_get(db, 12));

    /*                              Cleanup                                   */

    db = ov_recorder_record_stream_db_free(db);

    testrun(0 == db);
    return testrun_log_success();
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_recorder_record_stream_db_remove_id() {

    testrun(!ov_recorder_record_stream_db_remove_id(0, 0));

    ov_recorder_record_stream_db *db = ov_recorder_record_stream_db_create(10);

    testrun(0 != db);

    ov_id id1;
    ov_id id12;
    ov_id id0;

    ov_id_fill_with_uuid(id1);
    ov_id_fill_with_uuid(id12);
    ov_id_fill_with_uuid(id0);

    /* No entries added yet - thus all the entries we try to remove_id
     * should not be within the db and hence remove_uuid should return true */
    testrun(ov_recorder_record_stream_db_remove_id(db, id1));
    testrun(ov_recorder_record_stream_db_remove_id(db, id12));
    testrun(ov_recorder_record_stream_db_remove_id(db, id0));

    /*                   Check with one existing entry                        */

    ov_codec *codec_12 = get_codec(0);
    testrun(0 != codec_12);

    char const *lname = "looplinglouie4";

    testrun(ov_recorder_record_stream_db_add(
        db, 12, codec_12, lname, id12, callbacks));

    ov_recorder_record_stream_entry *entry =
        ov_recorder_record_stream_db_get(db, 12);

    testrun(0 != entry);

    ov_id_set(id1, entry->id);

    testrun(codec_12 == entry->source_codec);

    testrun(ov_thread_lock_unlock(&entry->lock));

    testrun(ov_recorder_record_stream_db_remove_id(db, id1));
    testrun(ov_recorder_record_stream_db_remove_id(db, id12));
    testrun(ov_recorder_record_stream_db_remove_id(db, id0));

    /* Check that entry has indeed gone */

    testrun(0 == ov_recorder_record_stream_db_get(db, 0));
    testrun(0 == ov_recorder_record_stream_db_get(db, 1));
    testrun(0 == ov_recorder_record_stream_db_get(db, 12));

    /*                     Check with several entries                         */

    codec_12 = get_codec(0);
    testrun(0 != codec_12);

    testrun(ov_recorder_record_stream_db_add(
        db, 12, codec_12, lname, id12, callbacks));

    ov_codec *codec_0 = get_codec(0);
    testrun(0 != codec_0);

    testrun(ov_recorder_record_stream_db_add(
        db, 0, codec_0, lname, id0, callbacks));

    /* Check both entries are there indeed */

    entry = ov_recorder_record_stream_db_get(db, 0);

    testrun(0 != entry);

    testrun(codec_0 == entry->source_codec);

    testrun(ov_id_match(entry->id, id0));

    testrun(ov_thread_lock_unlock(&entry->lock));

    entry = ov_recorder_record_stream_db_get(db, 12);

    testrun(0 != entry);

    testrun(codec_12 == entry->source_codec);
    testrun(ov_id_match(entry->id, id12));

    testrun(ov_thread_lock_unlock(&entry->lock));

    /* 0 and 12 are actually present, 1 is not */

    testrun(ov_recorder_record_stream_db_remove_id(db, id12));

    /* check that entry 12 is gone, 0 is still present */

    entry = ov_recorder_record_stream_db_get(db, 0);

    testrun(0 != entry);

    testrun(codec_0 == entry->source_codec);
    testrun(ov_id_match(entry->id, id0));

    testrun(ov_thread_lock_unlock(&entry->lock));

    testrun(0 == ov_recorder_record_stream_db_get(db, 1));
    testrun(0 == ov_recorder_record_stream_db_get(db, 12));

    /* Remove entry 0 */

    testrun(ov_recorder_record_stream_db_remove_id(db, id0));

    /* Check that entry has indeed gone */

    testrun(0 == ov_recorder_record_stream_db_get(db, 0));
    testrun(0 == ov_recorder_record_stream_db_get(db, 1));
    testrun(0 == ov_recorder_record_stream_db_get(db, 12));

    /*                              Cleanup                                   */

    db = ov_recorder_record_stream_db_free(db);

    testrun(0 == db);

    return testrun_log_success();
}

/*****************************************************************************
                     tset_ov_recorder_record_stream_db_list
 ****************************************************************************/

static bool check_list_contains(ov_json_value *jlist, const ov_id **ids) {

    OV_ASSERT(0 != jlist);

    while (0 != *ids) {

        char key[sizeof(ov_id) + 1];

        snprintf(key, sizeof(key), "/%s", **ids);

        if (0 == ov_json_get(jlist, key)) return false;

        ++ids;
    }

    return true;
}

/*----------------------------------------------------------------------------*/

static int test_ov_recorder_record_stream_db_list() {

    testrun(!ov_recorder_record_stream_db_list(0, 0));

    ov_recorder_record_stream_db *db = ov_recorder_record_stream_db_create(10);

    testrun(0 != db);

    testrun(!ov_recorder_record_stream_db_list(db, 0));

    /*************************************************************************
                          Check without any db entries
     ************************************************************************/

    ov_json_value *target = ov_json_object();

    testrun(0 != target);

    testrun(ov_recorder_record_stream_db_list(db, target));

    const ov_id *no_ids[1] = {0};

    testrun(check_list_contains(target, no_ids));

    target = ov_json_value_free(target);
    testrun(0 == target);

    target = ov_json_object();

    testrun(0 != target);

    target = ov_json_value_free(target);
    testrun(0 == target);

    /*************************************************************************
                         Check again with some entries
     ************************************************************************/

    ov_id id0;
    ov_id id1;

    ov_id_fill_with_uuid(id0);
    ov_id_fill_with_uuid(id1);

    /*                   Check with one existing entry                        */

    ov_codec *codec = get_codec(0);
    testrun(0 != codec);

    char const *lname = "looplinglouie6";

    testrun(
        ov_recorder_record_stream_db_add(db, 12, codec, lname, id0, callbacks));

    target = ov_json_object();

    testrun(0 != target);

    testrun(ov_recorder_record_stream_db_list(db, target));

    const ov_id *ids[3] = {&id0};

    testrun(check_list_contains(target, ids));

    target = ov_json_value_free(target);
    testrun(0 == target);

    /* two entries */

    codec = get_codec(0);
    testrun(0 != codec);

    testrun(
        ov_recorder_record_stream_db_add(db, 1, codec, lname, id1, callbacks));

    target = ov_json_object();

    testrun(0 != target);

    testrun(ov_recorder_record_stream_db_list(db, target));

    ids[1] = &id1;
    ids[2] = 0;

    testrun(check_list_contains(target, ids));

    target = ov_json_value_free(target);
    testrun(0 == target);

    /*************************************************************************
                                    Clean up
     ************************************************************************/

    db = ov_recorder_record_stream_db_free(db);
    testrun(0 == db);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_recorder_record_stream_entry_unlock() {

    testrun(!ov_recorder_record_stream_entry_unlock(0));

    ov_recorder_record_stream_db *db = ov_recorder_record_stream_db_create(10);

    testrun(0 != db);

    /* create entry */

    ov_codec *codec_12 = get_codec(0);
    testrun(0 != codec_12);

    ov_id id;
    ov_id_fill_with_uuid(id);

    char const *lname = "looplinglouie6";

    testrun(ov_recorder_record_stream_db_add(
        db, 12, codec_12, lname, id, callbacks));

    ov_recorder_record_stream_entry *entry =
        ov_recorder_record_stream_db_get(db, 12);

    testrun(0 != entry);

    testrun(codec_12 == entry->source_codec);

    testrun(ov_recorder_record_stream_entry_unlock(entry));
    entry = 0;

    db = ov_recorder_record_stream_db_free(db);

    testrun(0 == db);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

OV_TEST_RUN("ov_recorder_record_stream_db",
            test_ov_recorder_record_stream_db_create,
            test_ov_recorder_record_stream_db_free,
            test_ov_recorder_record_stream_db_add,
            test_ov_recorder_record_stream_db_get,
            test_ov_recorder_record_stream_db_remove_id,
            test_ov_recorder_record_stream_db_remove_msid,
            test_ov_recorder_record_stream_db_list,
            test_ov_recorder_record_stream_entry_unlock);
