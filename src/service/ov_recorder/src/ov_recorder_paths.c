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

        Current repository structure is:

        REPO_ROOT/USER/LOOP/USER_LOOP_TIMESTAMP_UUID.extension

        ------------------------------------------------------------------------
*/

#include "ov_recorder_paths.h"
#include "ov_base/ov_string.h"
#include "ov_recorder_constants.h"
#include <ftw.h>
#include <libgen.h>
#include <limits.h>
#include <ov_base/ov_dir.h>
#include <ov_base/ov_file.h>
#include <ov_base/ov_utils.h>
#include <ov_format/ov_format.h>
#include <ov_format/ov_format_codec.h>
#include <ov_format/ov_format_ogg.h>
#include <ov_format/ov_format_ogg_opus.h>

/*----------------------------------------------------------------------------*/

#define FAKE_SSID 0

/*----------------------------------------------------------------------------*/

char *ov_recorder_paths_get_recording_dir(char *target,
                                          size_t target_size,
                                          char const *root_path,
                                          char const *loop) {

    if ((!ov_ptr_valid(target, "Internal parameter missing")) ||
        (!ov_cond_valid(0 < target_size, "Internal parameter missing")) ||
        (!ov_ptr_valid(root_path, "Internal parameters missing")) ||
        (!ov_ptr_valid(loop, "No loop given for recording"))) {

        return 0;

    } else {

        ssize_t bytes_required =
            snprintf(target, target_size, "%s/%s", root_path, loop);

        if (ov_cond_valid(0 < bytes_required,
                          "recording directory would be an empty string") &&
            ov_cond_valid(target_size >= (size_t)bytes_required,
                          "Recording directory is too long") &&
            ov_cond_valid(0 == target[bytes_required],
                          "recording directory could not created - invalid "
                          "string produced")) {

            return target;

        } else {

            return 0;
        }
    }
}

/*----------------------------------------------------------------------------*/

static bool epoch_to_str(time_t epoch, char *target, size_t target_capacity) {

    struct tm epoch_tm = {0};

    return ov_ptr_valid(target, "Cannot create timestamp - no target buffer") &&
           ov_cond_valid(0 != gmtime_r(&epoch, &epoch_tm),
                         "Cannot create timestamp - could not create tm "
                         "struct") &&
           ov_cond_valid(
               0 != strftime(target, target_capacity, "%Y%m%dT%T", &epoch_tm),
               "Cannot create timestamp - could not format time info");
}

/*----------------------------------------------------------------------------*/

char *ov_recorder_paths_get_recording_file_name(char *target,
                                                size_t target_size,
                                                char const *loop,
                                                uint64_t timestamp_epoch,
                                                const ov_id id,
                                                char const *file_extension) {

    char timestamp[200] = {0};

    if ((0 == target) || (0 == target_size) || (0 == loop) ||
        (0 == file_extension) || (!ov_id_valid(id)) ||
        (!epoch_to_str(timestamp_epoch, timestamp, sizeof(timestamp)))) {

        return 0;
    } else {

        ssize_t bytes_required = snprintf(target,
                                          target_size,
                                          "%s_%s_%s.%s",
                                          loop,
                                          timestamp,
                                          id,
                                          file_extension);

        if ((1 > bytes_required) ||
            (target_size < (size_t)bytes_required + 1) ||
            /* 0 byte not included in bytes_written, see snprintf(3) */
            (0 != target[bytes_required])) {

            return 0;

        } else {

            return target;
        }
    }
}

/*----------------------------------------------------------------------------*/

static ssize_t prepend_with_path(char *target,
                                 size_t target_size,
                                 char const *path) {

    size_t path_len = ov_string_len(path);

    if (0 == path_len) {

        return 0;

    } else if (!ov_ptr_valid(target, "Cannot prepend path: No target")) {

        return -1;

    } else {

        int written_bytes = 0;

        if ('/' == path[path_len - 1]) {

            written_bytes = snprintf(target, target_size, "%s", path);

        } else {

            written_bytes = snprintf(target, target_size, "%s/", path);
        }

        if ((written_bytes > -1) && (target_size >= (size_t)written_bytes)) {

            return written_bytes;

        } else {

            return -1;
        }
    }
}

/*----------------------------------------------------------------------------*/

char *ov_recorder_paths_get_recording_file_path(char *target,
                                                size_t target_size,
                                                char const *repository_root,
                                                char const *loop,
                                                uint64_t timestamp_epoch,
                                                const ov_id id,
                                                char const *file_extension) {

    if (ov_ptr_valid(target, "Cannot get recording file path - no target") &&
        ov_ptr_valid(loop, "Cannot get recording file path - no loop given")) {

        ssize_t root_len =
            prepend_with_path(target, target_size, repository_root);

        ssize_t dir_len = 0;

        if (0 <= root_len) {

            dir_len = prepend_with_path(
                target + root_len, target_size - root_len, loop);
        }

        if ((0 <= root_len) && (0 <= dir_len) &&
            (target + root_len + dir_len ==
             ov_recorder_paths_get_recording_file_name(
                 target + root_len + dir_len,
                 target_size - root_len - dir_len,
                 loop,
                 timestamp_epoch,
                 id,
                 file_extension))) {
            return target;

        } else {

            return 0;
        }

    } else {

        return 0;
    }
}

/*----------------------------------------------------------------------------*/

char const *ov_recorder_paths_format_to_file_extension(char const *sformat) {

    if ((0 != sformat) &&
        ov_string_equal(sformat, OV_FORMAT_OGG_OPUS_TYPE_STRING)) {

        return "opus";

    } else {

        return sformat;
    }
}

/*----------------------------------------------------------------------------*/

bool ov_recorder_paths_get_info_for_path(
    char const *file_path, ov_recorder_paths_recording_info *info) {

    char path_copy[PATH_MAX] = {0};

    if ((0 == file_path) || (0 == info)) {

        ov_log_error(
            "Require file path and info struct to write to, got 0 pointer");
        return false;
        ;
    }

    // That is the canonical file name format:
    // LOOP_TIMESTAMP_ID.extension

    strncpy(path_copy, file_path, sizeof(path_copy));
    path_copy[PATH_MAX - 1] = 0;

    char *file_name = basename(path_copy);

    if (0 == file_name) return false;
    ;

    /* Extract loop */

    char *end_of_loop = strchr(file_name, '_');

    if (0 == end_of_loop) return false;

    if ((0 != info->loop) && (0 < info->loop_capacity)) {

        *end_of_loop = 0;
        strncpy(info->loop, file_name, info->loop_capacity);
        info->loop[info->loop_capacity - 1] = 0;
    }

    char *start_of_ts = end_of_loop + 1;

    char *end_of_ts = 0;

    long timestamp_epoch = strtol(start_of_ts, &end_of_ts, 10);

    if (0 == end_of_ts) return false;
    if (start_of_ts == end_of_ts) return false;
    if ('_' != *end_of_ts) return false;

    if ((int64_t)UINT32_MAX < (int64_t)timestamp_epoch) return false;
    ;

    if (0 > timestamp_epoch) return false;
    ;

    info->timestamp_epoch = (uint32_t)timestamp_epoch;

    char *start_of_id = end_of_ts + 1;

    char *end_of_id = strchr(start_of_id, '.');

    if (0 == end_of_id) {
        return false;
    } else {

        *end_of_id = 0;
        return ov_id_set(info->id, start_of_id);
    }
}

/*----------------------------------------------------------------------------*/

char *ov_recorder_paths_recording_info_to_recording_file_name(
    char *target,
    size_t target_size,
    ov_recorder_paths_recording_info const *info,
    char const *extension) {

    if (ov_ptr_valid(info,
                     "Cannot get recording file name for recording info - "
                     "no "
                     "recording info")) {

        return ov_recorder_paths_get_recording_file_name(target,
                                                         target_size,
                                                         info->loop,
                                                         info->timestamp_epoch,
                                                         info->id,
                                                         extension);
    } else {

        return false;
    }
}

/*----------------------------------------------------------------------------*/

char *ov_recorder_paths_recording_info_to_recording_file_path(
    char *target,
    size_t target_size,
    char const *repository_root,
    ov_recorder_paths_recording_info const *info,
    char const *extension) {

    if (ov_ptr_valid(info,
                     "Cannot get recording file name for recording info - "
                     "no recording info")) {

        return ov_recorder_paths_get_recording_file_path(target,
                                                         target_size,
                                                         repository_root,
                                                         info->loop,
                                                         info->timestamp_epoch,
                                                         info->id,
                                                         extension);

    } else {

        return false;
    }
}

/*----------------------------------------------------------------------------*/

ov_format *ov_recorder_paths_get_format_to_record(
    char const *root_path,
    ov_recorder_paths_recording_info const *recinfo,
    char const *sformat,
    void *format_opts,
    ov_codec *codec) {

    char *file_name = 0;
    ov_format *format = 0;
    char file_path[PATH_MAX] = {0};

    if ((!ov_ptr_valid(root_path, "No root path given (0 pointer)")) ||
        (!ov_ptr_valid(recinfo, "No recording info given (0 pointer)")) ||
        (!ov_ptr_valid(recinfo->loop, "No loop given (0 pointer)")) ||
        (!ov_ptr_valid(sformat, "No format given (0 pointer)")) ||
        (!ov_ptr_valid(codec, "No codec given (0 pointer)")) ||
        (!ov_cond_valid(
            0 != ov_recorder_paths_get_recording_dir(
                     file_path, sizeof(file_path), root_path, recinfo->loop),
            "Could not get recording directory"))) {

        goto error;
    }

    size_t len = strnlen(file_path, PATH_MAX);

    size_t remaining_chars = PATH_MAX - len;

    if (0 == remaining_chars) {

        ov_log_error("File name too long");
        goto error;
    }

    if (!ov_dir_tree_create(file_path)) {

        ov_log_error("Could not create directory '%s'", file_path);
        goto error;
    }

    file_path[len] = '/';

    --remaining_chars;

    if (0 == remaining_chars) {

        ov_log_error("File name too long");
        goto error;
    }

    char *end_of_string = file_path + len + 1;

    OV_ASSERT(0 == *end_of_string);

    file_name = ov_recorder_paths_recording_info_to_recording_file_name(
        end_of_string,
        remaining_chars,
        recinfo,
        ov_recorder_paths_format_to_file_extension(sformat));

    if (0 != file_path[PATH_MAX - 1]) {

        ov_log_error("File path too long -truncated: '%s'", file_path);
        goto error;
    }

    ov_format *file_format = ov_format_open(file_path, OV_WRITE);

    if (ov_string_equal(sformat, OV_FORMAT_OGG_OPUS_TYPE_STRING)) {

        // Unfortunately, for ogg/opus, we need to manually wrap our
        // file into an 'ogg' format first

        ov_format *ogg =
            ov_format_as(file_format, OV_FORMAT_OGG_TYPE_STRING, 0, 0);

        file_format = OV_OR_DEFAULT(ogg, ov_format_close(file_format));

        ogg = 0;
    }

    if (0 == file_format) {

        goto error;
    }

    format = ov_format_as(file_format, sformat, format_opts, 0);

    if (0 == format) {

        format = file_format;
        goto error;
    }

    file_format = 0;

    ov_format *codec_format = ov_format_as(format, "codec", codec, 0);

    if (0 == codec_format) {

        goto error;
    }

    return codec_format;

error:

    if (0 != format) {

        format = ov_format_close(format);
        unlink(file_name);
    }

    OV_ASSERT(0 == format);

    return 0;
}

/*****************************************************************************
                                    PLAYBACK
 ****************************************************************************/

// static const int FOUND = 1;
//
// static char id_found_path[PATH_MAX];
// static char const *id_string_to_search = 0;
//
// static int search_for_id(char const *fpath,
//                          struct stat const *sb,
//                          int typeflag,
//                          struct FTW *ftwbuf) {
//
//     UNUSED(sb);
//
//     if (FTW_F != typeflag) return 0;
//
//     // USER_LOOP_TIMESTAMP_I--D.extension
//     //                     ^  ^
//     //                     |--|
//     //                  This is the region we are interested in
//
//     // Lets make it easy - just search for any occurence of the UUID
//     string.
//     // Of course, this could lead to false-positives, although it is
//     highly
//     // unlikely
//
//     if (0 != strstr(fpath + ftwbuf->base, id_string_to_search)) {
//
//         strncpy(id_found_path, fpath, sizeof(id_found_path));
//         return FOUND;
//     }
//
//     return 0;
// }
//
// /*----------------------------------------------------------------------------*/
//
// static char *get_file_for_recording_nocheck(char const *root_path,
//                                            char const *id_string) {
//
//    id_string_to_search = id_string;
//
//    if (FOUND ==
//        nftw(root_path, search_for_id, 20, FTW_PHYS | FTW_MOUNT |
//        FTW_DEPTH))
//        {
//
//        id_found_path[PATH_MAX - 1] = 0;
//        return strdup(id_found_path);
//    }
//
//    return 0;
//}
//
///*----------------------------------------------------------------------------*/
//
// static ov_codec *get_file_codec_nocheck(char const *path) {
//
//     UNUSED(path);
//
//     OV_ASSERT(0 != path);
//
//     ov_json_value *jcodec = ov_json_value_from_string(
//         FILE_CODEC_DEFAULT, strlen(FILE_CODEC_DEFAULT));
//
//     OV_ASSERT(0 != jcodec);
//
//     ov_codec *file_codec =
//         ov_codec_factory_get_codec_from_json(0, jcodec, FAKE_SSID);
//
//     jcodec = ov_json_value_free(jcodec);
//
//     if (0 == file_codec) {
//
//         char *codec_str = ov_json_value_to_string(jcodec);
//         ov_log_error("Could not create codec %s", codec_str);
//         free(codec_str);
//     }
//
//     return file_codec;
// }
//
// /*----------------------------------------------------------------------------*/
//
// static char const *get_file_format_str_nocheck(char const *path) {
//
//     UNUSED(path);
//
//     return "wav";
// }
//
// /*----------------------------------------------------------------------------*/
//
// static ov_format *get_file_format_nocheck(char const *path, ov_codec
// *codec)
// {
//
//     OV_ASSERT(0 != codec);
//
//     ov_format *raw_fmt = 0;
//     ov_format *fmt_fmt = 0;
//     ov_format *codec_fmt = 0;
//
//     char const *fmt_string = get_file_format_str_nocheck(path);
//
//     if (0 == fmt_string) goto error;
//
//     raw_fmt = ov_format_open(path, OV_READ);
//
//     if (0 == raw_fmt) goto error;
//
//     fmt_fmt = ov_format_as(raw_fmt, fmt_string, 0, 0);
//
//     if (0 == fmt_fmt) goto error;
//
//     raw_fmt = 0;
//
//     codec_fmt = ov_format_as(fmt_fmt, OV_FORMAT_CODEC_TYPE_STRING, codec,
//     0);
//
//     if (0 == codec_fmt) goto error;
//
//     fmt_fmt = 0;
//
//     return codec_fmt;
//
// error:
//
//     if (0 != raw_fmt) {
//
//         raw_fmt = ov_format_close(raw_fmt);
//     }
//
//     if (0 != fmt_fmt) {
//
//         fmt_fmt = ov_format_close(fmt_fmt);
//     }
//
//     if (0 != codec_fmt) {
//
//         codec_fmt = ov_format_close(codec_fmt);
//     }
//
//     OV_ASSERT(0 == raw_fmt);
//     OV_ASSERT(0 == fmt_fmt);
//     OV_ASSERT(0 == codec_fmt);
//
//     return 0;
// }
//
// /*----------------------------------------------------------------------------*/
//
// ov_format *ov_recorder_paths_get_format_to_playback(
//    char const *root_path, ov_recorder_event_playback_start const *event)
//    {
//
//    char *file_path = 0;
//    ov_codec *codec = 0;
//    ov_format *fmt = 0;
//
//    if (0 == root_path) {
//        ov_log_error("No root path given");
//        goto error;
//    }
//
//    if (0 == event) {
//        ov_log_error("No playback start event");
//        goto error;
//    }
//
//    char const *recording_id = event->recording.id;
//    file_path = get_file_for_recording_nocheck(root_path, recording_id);
//
//    if (0 == file_path) {
//
//        ov_log_error("Recording %s not found", recording_id);
//        goto error;
//    }
//
//    codec = get_file_codec_nocheck(file_path);
//
//    if (0 == codec) {
//        ov_log_error("Could not get codec for recording %s (file %s)",
//                     recording_id,
//                     file_path);
//
//        goto error;
//    }
//
//    ov_log_info("Using %s for playback of %s (file %s)",
//                ov_codec_type_id(codec),
//                recording_id,
//                file_path);
//
//    fmt = get_file_format_nocheck(file_path, codec);
//
//    if (0 == fmt) {
//
//        ov_log_error(
//            "Could not open recording %s (file %s)", recording_id,
//            file_path);
//        goto error;
//    }
//
//    file_path = ov_free(file_path);
//
//    return fmt;
//
// error:
//
//    file_path = ov_free(file_path);
//    codec = ov_codec_free(codec);
//
//    OV_ASSERT(0 == file_path);
//    OV_ASSERT(0 == codec);
//    OV_ASSERT(0 == fmt);
//
//    return 0;
//}

/*----------------------------------------------------------------------------*/
