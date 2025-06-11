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
#include "ov_recorder_record_stream_db.h"
#include <ov_base/ov_dict.h>
#include <ov_base/ov_string.h>
#include <ov_base/ov_utils.h>

/*----------------------------------------------------------------------------*/

struct ov_recorder_record_stream_db {

    ov_thread_lock lock;
    ov_dict *dict;

    uint64_t timeout_usecs;
};

static ov_recorder_record_stream_entry *as_entry(void *void_entry) {

    if (0 == void_entry) return 0;

    ov_recorder_record_stream_entry *entry = void_entry;

    if (OV_RECORDER_RECORD_STREAM_ENTRY_MAGIC_BYTES != entry->magic_bytes) {

        return 0;
    }

    return entry;
}

/*----------------------------------------------------------------------------*/

bool entry_clear(void *void_entry) {

    ov_recorder_record_stream_entry *entry = as_entry(void_entry);

    if (0 == void_entry) return false;

    entry->pcm = ov_chunker_free(entry->pcm);

    if (0 != entry->dest_format) {

        entry->dest_format = ov_format_close(entry->dest_format);
    }

    if (0 != entry->source_codec) {

        entry->source_codec = entry->source_codec->free(entry->source_codec);
    }

    OV_ASSERT(0 == entry->dest_format);
    OV_ASSERT(0 == entry->source_codec);

    return ov_thread_lock_clear(&entry->lock);
}

/*----------------------------------------------------------------------------*/

void *entry_free(void *void_entry) {

    ov_recorder_record_stream_entry *entry = as_entry(void_entry);

    if (0 == entry) return void_entry;

    if (!entry_clear(entry)) {
        return entry;
    }

    free(entry);

    entry = 0;

    return entry;
}

/*----------------------------------------------------------------------------*/

static bool entry_unlock_nocheck(ov_recorder_record_stream_entry *entry) {

    OV_ASSERT(0 != entry);

    return ov_thread_lock_unlock(&entry->lock);
}

/*----------------------------------------------------------------------------*/

static ov_recorder_record_stream_db *recorder_record_stream_db_free_nolock(
    ov_recorder_record_stream_db *db) {

    if (0 == db) return db;

    if (0 != db->dict) {

        db->dict = ov_dict_free(db->dict);
    }

    ov_thread_lock_clear(&db->lock);

    OV_ASSERT(0 == db->dict);

    free(db);

    db = 0;

    return db;
}

/*----------------------------------------------------------------------------*/

ov_recorder_record_stream_db *ov_recorder_record_stream_db_create(
    uint64_t timeout_usecs) {

    ov_recorder_record_stream_db *db =
        calloc(1, sizeof(ov_recorder_record_stream_db));

    db->timeout_usecs = timeout_usecs;

    if (!ov_thread_lock_init(&db->lock, timeout_usecs)) {

        ov_log_error("Could not initialize record stream db lock");
        goto error;
    }

    ov_dict_config dict_cfg = ov_dict_intptr_key_config(20);

    dict_cfg.value.data_function = (ov_data_function){
        .clear = entry_clear,
        .free = entry_free,
    };

    db->dict = ov_dict_create(dict_cfg);

    if (0 == db->dict) {

        ov_log_error("Could not initialize record stream db dictionary");
        goto error;
    }

    return db;

error:

    db = recorder_record_stream_db_free_nolock(db);

    OV_ASSERT(0 == db);

    return 0;
}

/*----------------------------------------------------------------------------*/

ov_recorder_record_stream_db *ov_recorder_record_stream_db_free(
    ov_recorder_record_stream_db *db) {

    if (0 == db) goto error;

    if (!ov_thread_lock_try_lock(&db->lock)) {

        ov_log_error("Could not lock stream record db");
        goto error;
    }

    db = recorder_record_stream_db_free_nolock(db);

error:

    return db;
}

/*----------------------------------------------------------------------------*/

static ov_dict *get_locked_dict(ov_recorder_record_stream_db *db) {

    if (0 == db) return 0;

    if (!ov_thread_lock_try_lock(&db->lock)) {

        ov_log_error("Could not lock record stream database");
        return 0;
    }

    return db->dict;
}

/*----------------------------------------------------------------------------*/

bool ov_recorder_record_stream_db_add(ov_recorder_record_stream_db *db,
                                      uint32_t msid,
                                      ov_codec *source_codec,
                                      char const *loop,
                                      const ov_id id,
                                      ov_recorder_record_callbacks callbacks) {

    ov_recorder_record_stream_entry *new_entry = 0;
    ov_recorder_record_stream_entry *entry = 0;

    bool result = false;

    ov_dict *dict = get_locked_dict(db);

    if (ov_ptr_valid(
            dict, "Cannot register recording - could not lock database") &&
        ov_ptr_valid(loop, "Cannot register recording - no loop name given") &&
        ov_ptr_valid(
            source_codec, "Cannot register recording - no codec given")) {

        OV_ASSERT(0 != db);

        /* dict now locked */

        uintptr_t msid_ptr = msid;

        entry = ov_dict_get(dict, (void *)msid_ptr);

        if (0 != entry) {

            ov_log_error("Already recording for %" PRIu32, msid);
            goto error;
        }

        OV_ASSERT(0 == entry);

        new_entry = calloc(1, sizeof(ov_recorder_record_stream_entry));

        new_entry->pcm = ov_chunker_create();

        new_entry->magic_bytes = OV_RECORDER_RECORD_STREAM_ENTRY_MAGIC_BYTES;
        new_entry->source_codec = source_codec;

        new_entry->start_epoch_secs = time(0);
        new_entry->callbacks.data = callbacks.data;
        new_entry->callbacks.cb_recording_stopped =
            callbacks.cb_recording_stopped;

        ov_id_set(new_entry->id, id);
        ov_string_copy(new_entry->loop, loop, sizeof(new_entry->loop));

        new_entry->frames_to_buffer = 1;

        ov_thread_lock_init(&new_entry->lock, db->timeout_usecs);

        ov_recorder_record_stream_entry *overwritten_entry = 0;

        /* entry in db is either 0 or locked */

        if (!ov_dict_set(dict,
                         (void *)msid_ptr,
                         new_entry,
                         (void **)&overwritten_entry)) {

            ov_log_error("Could not insert db entry");
            goto error;
        }

        new_entry = 0;

        OV_ASSERT(0 == overwritten_entry);

        result = true;

    } else {

        result = false;
    }

error:

    if (0 != dict) {

        ov_thread_lock_unlock(&db->lock);
        dict = 0;
    }

    new_entry = entry_free(new_entry);

    OV_ASSERT(0 == dict);
    OV_ASSERT(0 == new_entry);

    return result;
}

/*****************************************************************************
                    ov_recorder_record_stream_db_get_id
 ****************************************************************************/

typedef struct find_id_data {

    ov_id id;
    uint32_t msid;

    bool found;

} find_id_data;

static bool find_id(const void *key, void *value, void *data) {

    OV_ASSERT(0 != data);

    uintptr_t msid_ptr = (uintptr_t)key;

    ov_recorder_record_stream_entry *entry = as_entry(value);

    if (0 == entry) return true;

    if (!ov_thread_lock_try_lock(&entry->lock)) {

        ov_log_error("Could not lock on entry");
        return false;
    }

    bool continue_search = true;

    find_id_data *sd = data;

    if (ov_id_match(entry->id, sd->id)) {

        sd->found = true;
        sd->msid = (uint32_t)msid_ptr;

        continue_search = false;
    }

    if (!ov_thread_lock_unlock(&entry->lock)) {

        OV_ASSERT(!"MUST NEVER HAPPEN");
    }

    return continue_search;
}

/*----------------------------------------------------------------------------*/

static bool get_msid_for_id(ov_dict *db_dict, const ov_id id, uint32_t *msid) {

    find_id_data sd = {
        .found = false,
    };

    if (ov_ptr_valid(db_dict, "Cannot resolve ID to MSID - no database") &&
        ov_ptr_valid(
            msid, "Cannot resolve ID to MSID - no target MSID variable")) {

        ov_id_set(sd.id, id);

        ov_dict_for_each(db_dict, &sd, find_id);

        *msid = sd.msid;
    }

    return sd.found;
}

/*----------------------------------------------------------------------------*/

bool ov_recorder_record_stream_db_set_frames_to_roll_after(
    ov_recorder_record_stream_db *db,
    const ov_id id,
    uint64_t frames_to_roll_after) {

    ov_recorder_record_stream_entry *entry =
        ov_recorder_record_stream_db_get_id(db, id);

    if (0 != entry) {

        entry->num_frames_to_roll_after = frames_to_roll_after;
        return ov_recorder_record_stream_entry_unlock(entry);

    } else {

        return false;
    }
}

/*----------------------------------------------------------------------------*/

bool ov_recorder_record_stream_db_set_vad_parameters(
    ov_recorder_record_stream_db *db,
    const ov_id id,
    ov_vad_config vad_parameters) {

    ov_recorder_record_stream_entry *entry =
        ov_recorder_record_stream_db_get_id(db, id);

    if (0 != entry) {

        entry->voice_activity.powerlevel_density_threshold_db =
            vad_parameters.powerlevel_density_threshold_db;
        entry->voice_activity.zero_crossings_rate_threshold_hertz =
            vad_parameters.zero_crossings_rate_threshold_hertz;

        return ov_recorder_record_stream_entry_unlock(entry);

    } else {

        return false;
    }
}

/*----------------------------------------------------------------------------*/

bool ov_recorder_record_stream_db_set_frames_to_buffer(
    ov_recorder_record_stream_db *db, const ov_id id, uint32_t num_frames) {

    ov_recorder_record_stream_entry *entry =
        ov_recorder_record_stream_db_get_id(db, id);

    if (0 != entry) {

        entry->frames_to_buffer = OV_OR_DEFAULT(num_frames, 1);
        return ov_recorder_record_stream_entry_unlock(entry);

    } else {

        return false;
    }
}

/*----------------------------------------------------------------------------*/

bool ov_recorder_record_stream_db_set_silence_cutoff_interval_frames(
    ov_recorder_record_stream_db *db, const ov_id id, uint32_t num_frames) {

    ov_recorder_record_stream_entry *entry =
        ov_recorder_record_stream_db_get_id(db, id);

    if (0 != entry) {

        entry->silence_cutoff_interval_frames = OV_OR_DEFAULT(num_frames, 10);
        return ov_recorder_record_stream_entry_unlock(entry);

    } else {

        return false;
    }
}

/*----------------------------------------------------------------------------*/

ov_recorder_record_stream_entry *ov_recorder_record_stream_db_get_id(
    ov_recorder_record_stream_db *db, const ov_id id) {

    uint32_t msid = 0;

    ov_dict *dict = get_locked_dict(db);

    bool rec_found = get_msid_for_id(dict, id, &msid);

    if (!ov_thread_lock_unlock(&db->lock)) {

        OV_ASSERT(!"SHOULD NEVER HAPPEN");
    }

    if (rec_found) {

        return ov_recorder_record_stream_db_get(db, msid);

    } else {

        ov_log_warning("Recording %s does not exist", ov_string_sanitize(id));
        return 0;
    }
}

/*----------------------------------------------------------------------------*/

ov_recorder_record_stream_entry *ov_recorder_record_stream_db_get(
    ov_recorder_record_stream_db *db, uint32_t msid) {

    ov_recorder_record_stream_entry *entry = 0;

    ov_dict *dict = get_locked_dict(db);

    if (0 == dict) {

        goto error;
    }

    OV_ASSERT(0 != db);

    /* dict now locked */

    uintptr_t msid_ptr = msid;

    entry = ov_dict_get(dict, (void *)msid_ptr);

    if ((0 != entry) && (!ov_thread_lock_try_lock(&entry->lock))) {

        ov_log_error("Could not lock entry for %" PRIu32, msid);
        entry = 0;
    }

error:

    if (0 != dict) {

        ov_thread_lock_unlock(&db->lock);
    }

    return entry;
}

/*----------------------------------------------------------------------------*/

bool ov_recorder_record_stream_db_remove_id(ov_recorder_record_stream_db *db,
                                            ov_id id) {

    ov_dict *dict = get_locked_dict(db);

    if (0 == dict) {

        goto error;
    }

    OV_ASSERT(0 != db);

    find_id_data sd = {
        .found = false,
    };

    ov_id_set(sd.id, id);

    ov_dict_for_each(dict, &sd, find_id);

    if (!ov_thread_lock_unlock(&db->lock)) {

        OV_ASSERT(!"SHOULD NEVER HAPPEN");
    }

    if (!sd.found) {

        ov_log_error("Recording %s does not exist", ov_string_sanitize(id));
        goto removed;
    }

    return ov_recorder_record_stream_db_remove_msid(db, sd.msid);

error:

    return false;

removed:

    return true;
}

/*----------------------------------------------------------------------------*/

bool ov_recorder_record_stream_db_remove_msid(ov_recorder_record_stream_db *db,
                                              uint32_t msid) {

    ov_recorder_record_stream_entry *entry = 0;

    ov_dict *dict = get_locked_dict(db);

    bool result = false;

    if (0 == dict) {

        goto error;
    }

    OV_ASSERT(0 != db);

    /* dict now locked */

    uintptr_t msid_ptr = msid;

    /* We need to lock the entry before removing it ... */
    entry = ov_dict_get(dict, (const void *)msid_ptr);

    if (0 == entry) {

        result = true;
        goto finish;
    }

    if ((0 != entry) && (!ov_thread_lock_try_lock(&entry->lock))) {

        ov_log_error("Could not lock entry for %" PRIu32, msid);
        entry = 0;
    }

    /* entry has been locked by us -> nobody locked it
     * dict is locked -> nobody will be able to lock it
     * -> we can safely unlock and still remove it safely
     */

    entry_unlock_nocheck(entry);

    result = ov_dict_del(dict, (const void *)msid_ptr);

    entry = 0;

error:
finish:

    if (0 != dict) {

        ov_thread_lock_unlock(&db->lock);
    }

    if (0 != entry) {

        entry_unlock_nocheck(entry);
    }

    return result;
}

/*****************************************************************************
                             record_stream_db_list
 ****************************************************************************/

static bool entry_to_json(const void *key, void *value, void *data) {

    ov_json_value *target = data;

    ov_recorder_record_stream_entry *entry = as_entry(value);

    if (0 == entry) return true;

    if (!ov_thread_lock_try_lock(&entry->lock)) {

        ov_log_error("Could not lock on entry");
        return false;
    }

    OV_ASSERT(0 != entry);

    uintptr_t msid = (uintptr_t)key;

    ov_json_object_set(target, entry->id, ov_json_number((double)msid));

    ov_thread_lock_unlock(&entry->lock);

    return true;
}

/*----------------------------------------------------------------------------*/

bool ov_recorder_record_stream_db_list(ov_recorder_record_stream_db *db,
                                       ov_json_value *target) {

    bool retval = false;

    ov_dict *dict = get_locked_dict(db);

    if (0 == dict) {

        ov_log_error("Could not get locked dict");
        goto error;
    }

    if (0 == target) {

        ov_log_error("No target json object given");
        goto error;
    }

    retval = ov_dict_for_each(dict, target, entry_to_json);

error:

    if (0 != dict) {

        OV_ASSERT(0 != db);
        ov_thread_lock_unlock(&db->lock);
    }

    return retval;
}

/*----------------------------------------------------------------------------*/

bool ov_recorder_record_stream_entry_unlock(
    ov_recorder_record_stream_entry *entry) {

    if (0 == entry) return 0;

    return entry_unlock_nocheck(entry);
}
