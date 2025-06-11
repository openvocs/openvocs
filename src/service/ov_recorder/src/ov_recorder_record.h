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
#ifndef OV_RECORDER_RECORD_H
#define OV_RECORDER_RECORD_H
/*----------------------------------------------------------------------------*/

#include <ov_base/ov_event_loop.h>
#include <ov_base/ov_json_value.h>
#include <ov_base/ov_result.h>
#include <ov_base/ov_thread_loop.h>
#include <ov_base/ov_vad_config.h>
#include <ov_core/ov_recorder_events.h>
#include <ov_core/ov_recording.h>

/*----------------------------------------------------------------------------*/

typedef struct ov_recorder_record ov_recorder_record;

typedef struct {

    char const *repository_root_path;

    size_t num_threads;
    uint64_t lock_timeout_usecs;

    char const *file_format;
    ov_json_value const *file_codec;

} ov_recorder_record_config;

/*----------------------------------------------------------------------------*/

ov_recorder_record *ov_recorder_record_create(ov_event_loop *loop,
                                              ov_recorder_record_config cfg);

/*****************************************************************************
                                  RECORD_START
 ****************************************************************************/

typedef struct {
    bool (*cb_recording_stopped)(ov_recording recording, void *data);
    void *data;
} ov_recorder_record_callbacks;

/*----------------------------------------------------------------------------*/

typedef struct {

    ov_id id;

    uint32_t ssid;
    ov_json_value *codec;

    char *loop;

    uint32_t timestamp_epoch;

    uint64_t num_frames_to_roll_recording_after;

    struct {
        ov_vad_config parameters;
        uint32_t frames_to_use;
        uint32_t silence_cutoff_interval_frames;
    } voice_activity;

} ov_recorder_record_start;

/*----------------------------------------------------------------------------*/

bool ov_recorder_record_start_recording(ov_recorder_record *rec,
                                        ov_recorder_record_start const *event,
                                        ov_recorder_record_callbacks callbacks,
                                        ov_result *res);

/*----------------------------------------------------------------------------*/

bool ov_recorder_record_stop_recording(ov_recorder_record *rec,
                                       ov_recorder_event_stop *event,
                                       ov_recording *recording);

/*----------------------------------------------------------------------------*/

bool ov_recorder_record_get_running_recordings(ov_recorder_record *rec,
                                               ov_json_value *target);

/*----------------------------------------------------------------------------*/

ov_recorder_record *ov_recorder_record_free(ov_recorder_record *record);

/*----------------------------------------------------------------------------*/

bool ov_recorder_record_send_to_threads(ov_recorder_record *rec,
                                        ov_thread_message *msg);

/*----------------------------------------------------------------------------*/

char *ov_recorder_record_filename(ov_recorder_record const *self,
                                  uint32_t ssrc,
                                  char *target,
                                  size_t target_capacity);

/*----------------------------------------------------------------------------*/

bool ov_recorder_record_activate_vad(ov_recorder_record *self,
                                     ov_vad_config vad_config);

/*----------------------------------------------------------------------------*/
#endif
