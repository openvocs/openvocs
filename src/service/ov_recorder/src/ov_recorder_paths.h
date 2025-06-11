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
#ifndef OV_RECORDER_PATHS_H
#define OV_RECORDER_PATHS_H
/*----------------------------------------------------------------------------*/

#include <ov_codec/ov_codec.h>
#include <ov_codec/ov_codec_factory.h>
#include <ov_format/ov_format.h>
#include <stddef.h>

#include "ov_recorder_record.h"

/*----------------------------------------------------------------------------*/

char *ov_recorder_paths_get_recording_dir(char *target,
                                          size_t target_size,
                                          char const *root_path,
                                          char const *loop);

/*----------------------------------------------------------------------------*/

char *ov_recorder_paths_get_recording_file_name(char *target,
                                                size_t target_size,
                                                char const *loop,
                                                uint64_t timestamp_epoch,
                                                const ov_id id,
                                                char const *file_extension);

char *ov_recorder_paths_get_recording_file_path(char *target,
                                                size_t target_size,
                                                char const *repository_root,
                                                char const *loop,
                                                uint64_t timestamp_epoch,
                                                const ov_id id,
                                                char const *file_extension);

char const *ov_recorder_paths_format_to_file_extension(char const *sformat);

/*----------------------------------------------------------------------------*/

typedef struct {

    ov_id id;

    char *loop;
    size_t loop_capacity;

    uint32_t timestamp_epoch;

} ov_recorder_paths_recording_info;

bool ov_recorder_paths_get_info_for_path(
    char const *file_path, ov_recorder_paths_recording_info *info);

/*----------------------------------------------------------------------------*/

char *ov_recorder_paths_recording_info_to_recording_file_name(
    char *target,
    size_t target_size,
    ov_recorder_paths_recording_info const *info,
    char const *sformat);

/*----------------------------------------------------------------------------*/

char *ov_recorder_paths_recording_info_to_recording_file_path(
    char *target,
    size_t target_size,
    char const *repository_root,
    ov_recorder_paths_recording_info const *info,
    char const *sformat);

/*----------------------------------------------------------------------------*/

ov_format *ov_recorder_paths_get_format_to_record(
    char const *root_path,
    ov_recorder_paths_recording_info const *recinfo,
    char const *format,
    void *format_opts,
    ov_codec *codec);

/*----------------------------------------------------------------------------*/

// ov_format *ov_recorder_paths_get_format_to_playback(
//     char const *root_path, ov_recorder_event_playback_start const *event);

/*----------------------------------------------------------------------------*/

#endif
