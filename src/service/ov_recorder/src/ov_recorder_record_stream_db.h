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

        @author Michael J. Beer, DLR/GSOC
        @copyright (c) 2021 German Aerospace Center DLR e.V. (GSOC)

        ------------------------------------------------------------------------
*/
#ifndef OV_RECORDER_RECORD_STREAM_DB_H
#define OV_RECORDER_RECORD_STREAM_DB_H
/*----------------------------------------------------------------------------*/

#include "ov_recorder_paths.h"
#include <ov_base/ov_chunker.h>
#include <ov_base/ov_string.h>
#include <ov_base/ov_thread_lock.h>
#include <ov_codec/ov_codec.h>
#include <ov_format/ov_format.h>
#include <stdbool.h>

/*----------------------------------------------------------------------------*/

#define OV_RECORDER_RECORD_STREAM_ENTRY_MAGIC_BYTES 0x1ab3ccdd

typedef struct {

    uint32_t magic_bytes;

    ov_codec *source_codec;
    ov_format *dest_format;
    ov_id id;

    char file_name[PATH_MAX + 1];

    ov_thread_lock lock;

    time_t start_epoch_secs;
    char loop[OV_STRING_DEFAULT_SIZE];

    ov_recorder_record_callbacks callbacks;

    uint64_t frames_received;
    uint64_t silent_frames_received;
    uint64_t num_frames_to_roll_after; // 0 => rolling disabled

    size_t frames_to_buffer;
    uint64_t silence_cutoff_interval_frames;

    ov_vad_config
        voice_activity; // If either of the parameters is 0, don't perform VAD

    ov_chunker *pcm;

} ov_recorder_record_stream_entry;

/*----------------------------------------------------------------------------*/

struct ov_recorder_record_stream_db;

typedef struct ov_recorder_record_stream_db ov_recorder_record_stream_db;

/*----------------------------------------------------------------------------*/

ov_recorder_record_stream_db *ov_recorder_record_stream_db_create(
    uint64_t lock_timeout_usecs);

/*----------------------------------------------------------------------------*/

ov_recorder_record_stream_db *ov_recorder_record_stream_db_free(
    ov_recorder_record_stream_db *db);

/*----------------------------------------------------------------------------*/

bool ov_recorder_record_stream_db_add(ov_recorder_record_stream_db *db,
                                      uint32_t msid,
                                      ov_codec *source_codec,
                                      char const *loop,
                                      const ov_id id,
                                      ov_recorder_record_callbacks callbacks);

/*----------------------------------------------------------------------------*/

bool ov_recorder_record_stream_db_set_frames_to_roll_after(
    ov_recorder_record_stream_db *db,
    const ov_id id,
    uint64_t frames_to_roll_after);

/*----------------------------------------------------------------------------*/

bool ov_recorder_record_stream_db_set_vad_parameters(
    ov_recorder_record_stream_db *db,
    const ov_id id,
    ov_vad_config vad_parameters);

/*----------------------------------------------------------------------------*/

bool ov_recorder_record_stream_db_set_frames_to_buffer(
    ov_recorder_record_stream_db *db, const ov_id id, uint32_t num_frames);

/*----------------------------------------------------------------------------*/

bool ov_recorder_record_stream_db_set_silence_cutoff_interval_frames(
    ov_recorder_record_stream_db *db, const ov_id id, uint32_t num_frames);

/*----------------------------------------------------------------------------*/

/**
 * @return a (locked) entry that has been added for id
 */
ov_recorder_record_stream_entry *ov_recorder_record_stream_db_get_id(
    ov_recorder_record_stream_db *db, const ov_id id);

/**
 * @return a (locked) entry that has been added for msid
 */
ov_recorder_record_stream_entry *ov_recorder_record_stream_db_get(
    ov_recorder_record_stream_db *db, uint32_t msid);

/*----------------------------------------------------------------------------*/

/**
 * @return true if the db does not contain an entry for id after call
 */
bool ov_recorder_record_stream_db_remove_id(ov_recorder_record_stream_db *db,
                                            ov_id id);

/*----------------------------------------------------------------------------*/

/**
 * @return true if the db does not contain an entry for msid after call
 */
bool ov_recorder_record_stream_db_remove_msid(ov_recorder_record_stream_db *db,
                                              uint32_t msid);

/*----------------------------------------------------------------------------*/

bool ov_recorder_record_stream_db_list(ov_recorder_record_stream_db *,
                                       ov_json_value *target);

/*----------------------------------------------------------------------------*/

bool ov_recorder_record_stream_entry_unlock(
    ov_recorder_record_stream_entry *entry);

/*----------------------------------------------------------------------------*/

#endif
