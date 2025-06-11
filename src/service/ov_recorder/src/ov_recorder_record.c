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
#include "ov_recorder_record.h"
#include "ov_recorder_config.h"
#include "ov_recorder_constants.h"
#include "ov_recorder_record_stream_db.h"
#include <ov_backend/ov_rtp_frame_message.h>
#include <ov_base/ov_config_keys.h>
#include <ov_base/ov_constants.h>
#include <ov_base/ov_string.h>
#include <ov_base/ov_time.h>
#include <ov_base/ov_utils.h>
#include <ov_codec/ov_codec_factory.h>
#include <ov_format/ov_format_codec.h>
#include <ov_format/ov_format_wav.h>
#include <ov_pcm16s/ov_pcm16_mod.h>

/*----------------------------------------------------------------------------*/

#define THREAD_MESSAGES_PER_THREAD 20

/*----------------------------------------------------------------------------*/

/* Require dummy ssid for codec creation */
#define FAKE_SSID 0

/*----------------------------------------------------------------------------*/

#define RECORD_DATA_MAGIC_BYTES 0x2fffa12d

struct ov_recorder_record {

    uint32_t magic_bytes;

    ov_thread_loop *thread_loop;

    ov_recorder_record_config config;

    ov_recorder_record_stream_db *db;
};

/*----------------------------------------------------------------------------*/

static ov_recorder_record *as_recorder_record(void *data) {

    if (0 == data) return 0;

    ov_recorder_record *rec = data;

    if (RECORD_DATA_MAGIC_BYTES != rec->magic_bytes) return 0;

    return rec;
}

/*----------------------------------------------------------------------------*/

static ov_recorder_record_stream_db const *get_stream_db(
    ov_recorder_record const *self) {

    if (ov_ptr_valid(self, "Cannot get stream DB: No record object")) {

        return self->db;

    } else {

        return 0;
    }
}

/*----------------------------------------------------------------------------*/

static ov_json_value *initialize_file_codec(ov_json_value const *jval) {

    ov_json_value *file_codec = 0;

    ov_json_value_copy((void **)&file_codec, jval);

    if (0 == file_codec) {

        file_codec = ov_json_value_from_string(
            FILE_CODEC_DEFAULT, strlen(FILE_CODEC_DEFAULT));

        ov_log_info("No file codec specified in ov_recorder config - using %s",
                    FILE_CODEC_DEFAULT);
    }

    return file_codec;
}

/*----------------------------------------------------------------------------*/

static char const *initialize_file_format(char const *format) {

    if (0 == format) {

        ov_log_info("No file format specified in ov_recorder config - using %s",
                    FILE_FORMAT_DEFAULT);
        format = FILE_FORMAT_DEFAULT;
    }

    return format;
}

/*----------------------------------------------------------------------------*/

static char const *initialize_repository_root_path(char const *path) {

    if (0 == path) {

        ov_log_info(
            "No repository path specified in ov_recorder config - "
            "using %s",
            OV_DEFAULT_RECORDER_REPOSITORY_ROOT_PATH);
        path = OV_DEFAULT_RECORDER_REPOSITORY_ROOT_PATH;
    }

    return path;
}

/*----------------------------------------------------------------------------*/

static ov_recorder_record *recorder_record_create_nocheck(
    ov_recorder_record_config cfg) {

    ov_recorder_record *rd = calloc(1, sizeof(ov_recorder_record));
    OV_ASSERT(0 != rd);

    ov_json_value *file_codec = initialize_file_codec(cfg.file_codec);

    char const *file_format = initialize_file_format(cfg.file_format);

    char const *repository_root_path =
        initialize_repository_root_path(cfg.repository_root_path);

    rd->magic_bytes = RECORD_DATA_MAGIC_BYTES;

    rd->config.file_codec = file_codec;

    rd->config.file_format = strdup(file_format);

    rd->config.repository_root_path = strdup(repository_root_path);

    return rd;
}

/*----------------------------------------------------------------------------*/

static void log_recording_started_nocheck(
    ov_recorder_record_start const *event) {

    OV_ASSERT(0 != event);

    char *loop = OV_OR_DEFAULT(event->loop, "?");

    char timestamp[30] = {0};

    ov_time_to_string(timestamp, 30, event->timestamp_epoch, SEC);

    ov_log_info(
        "Recording started %s (Loop %s, Time %s)", loop, event->id, timestamp);
}

/*----------------------------------------------------------------------------*/

static ov_codec *codec_from_event(ov_recorder_record_start const *event) {

    ov_codec *codec = 0;

    if (0 != event) {
        codec =
            ov_codec_factory_get_codec_from_json(0, event->codec, event->ssid);

        if (0 == codec) {
            char *codec_str = ov_json_value_to_string(event->codec);
            ov_log_error("Could not create codec %s", codec_str);
            free(codec_str);
        }
    }

    return codec;
}

/*----------------------------------------------------------------------------*/

bool ov_recorder_record_start_recording(ov_recorder_record *rec,
                                        ov_recorder_record_start const *event,
                                        ov_recorder_record_callbacks callbacks,
                                        ov_result *res) {

    ov_codec *source_codec = codec_from_event(event);

    if (ov_ptr_valid_result(
            rec, res, OV_ERROR_INTERNAL_SERVER, "No record object") &&
        ov_ptr_valid_result(source_codec,
                            res,
                            OV_ERROR_INTERNAL_SERVER,
                            "Invalid/Unknown stream codec") &&
        ov_cond_valid_result(ov_recorder_record_stream_db_add(rec->db,
                                                              event->ssid,
                                                              source_codec,
                                                              event->loop,
                                                              event->id,
                                                              callbacks),
                             res,
                             OV_ERROR_INTERNAL_SERVER,
                             "Could not register recording") &&
        ov_cond_valid(
            ov_recorder_record_stream_db_set_frames_to_roll_after(
                rec->db, event->id, event->num_frames_to_roll_recording_after),
            "Could not set roll recording") &&
        ov_cond_valid(ov_recorder_record_stream_db_set_vad_parameters(
                          rec->db, event->id, event->voice_activity.parameters),
                      "Could not set voice activity detection parameters") &&
        ov_cond_valid(ov_recorder_record_stream_db_set_frames_to_buffer(
                          rec->db, event->id, 3),
                      "Could not set frames to buffer") &&
        ov_cond_valid(
            ov_recorder_record_stream_db_set_silence_cutoff_interval_frames(
                rec->db,
                event->id,
                OV_OR_DEFAULT(
                    event->voice_activity.silence_cutoff_interval_frames, 1)),
            "Could not set silence cutoff interval")) {

        log_recording_started_nocheck(event);
        return true;

    } else {

        source_codec = ov_codec_free(source_codec);
        return false;
    }
}

/*----------------------------------------------------------------------------*/

static bool get_recording_info(ov_recorder_record_stream_entry *db_entry,
                               ov_recording *recording) {

    return (0 == recording) || (ov_ptr_valid(db_entry,
                                             "Cannot stop recording - unknown "
                                             "recording") &&
                                ov_recording_set(recording,
                                                 db_entry->id,
                                                 db_entry->loop,
                                                 db_entry->file_name,
                                                 db_entry->start_epoch_secs,
                                                 time(0)));
}

/*----------------------------------------------------------------------------*/

static bool notify_recording_stopped(ov_recorder_record_callbacks callbacks,
                                     ov_recording *recording) {

    if (0 == callbacks.cb_recording_stopped) {

        return true;

    } else if (ov_ptr_valid(recording,
                            "Cannot notify of stopped recording - no "
                            "recording")) {

        return callbacks.cb_recording_stopped(*recording, callbacks.data);

    } else {

        return false;
    }
}

/*----------------------------------------------------------------------------*/

static bool close_entry_out_format(ov_recorder_record_stream_entry *entry) {

    if (0 != entry) {
        entry->dest_format = ov_format_close(entry->dest_format);
        entry->frames_received = 0;
        entry->silent_frames_received = 0;
        ov_buffer_free(ov_chunker_remainder(entry->pcm));
    }

    return true;
}

/*----------------------------------------------------------------------------*/

static bool stop_recording(ov_recorder_record_stream_entry *entry,
                           ov_recording *recording) {

    return ov_ptr_valid(entry,
                        "Cannot stop recording - recording entry is a 0 "
                        "pointer") &&
           get_recording_info(entry, recording) &&
           close_entry_out_format(entry) &&
           notify_recording_stopped(entry->callbacks, recording);
}

/*----------------------------------------------------------------------------*/

bool ov_recorder_record_stop_recording(ov_recorder_record *rec,
                                       ov_recorder_event_stop *event,
                                       ov_recording *recording) {

    if (ov_ptr_valid(rec, "Got 0 pointer") &&
        ov_ptr_valid(event, "No record start event received (0 pointer)")) {

        ov_recorder_record_stream_entry *db_entry =
            ov_recorder_record_stream_db_get_id(rec->db, event->id);

        if (0 == db_entry) {

            ov_log_warning("Cannot remove recording %s - unknown recording",
                           ov_string_sanitize(event->id));

            return true;

        } else {

            return stop_recording(db_entry, recording) &&
                   ov_recorder_record_stream_entry_unlock(db_entry) &&
                   ov_cond_valid(ov_recorder_record_stream_db_remove_id(
                                     rec->db, event->id),
                                 "Could not remove stream entry from database");
        }

    } else {

        return false;
    }
}

/*****************************************************************************
                               New recording
 ****************************************************************************/

static ov_codec *create_file_codec(ov_recorder_record const *rec) {

    ov_codec *codec = 0;

    if (0 != rec) {

        codec = ov_codec_factory_get_codec_from_json(
            0, rec->config.file_codec, FAKE_SSID);

        if (0 == codec) {
            char *codec_str = ov_json_value_to_string(rec->config.file_codec);
            ov_log_error("Could not create codec %s", codec_str);
            free(codec_str);
        }
    }

    return codec;
}

/*----------------------------------------------------------------------------*/

static ov_format *format_with_file_codec(ov_recorder_record const *rec,
                                         ov_recorder_paths_recording_info info,
                                         ov_codec *codec) {

    if (ov_ptr_valid(rec, "Cannot create format: No valid record object")) {

        return ov_recorder_paths_get_format_to_record(
            rec->config.repository_root_path,
            &info,
            rec->config.file_format,
            0,
            codec);

    } else {

        return 0;
    }
}

/*----------------------------------------------------------------------------*/

static ov_format *create_out_format(ov_recorder_record const *rec,
                                    ov_recorder_record_start const *event,
                                    ov_result *res) {

    ov_codec *codec = create_file_codec(rec);

    ov_recorder_paths_recording_info info = {

        .loop = event->loop,
        .loop_capacity = 0,
        .timestamp_epoch = event->timestamp_epoch,
    };

    ov_id_set(info.id, event->id);

    ov_format *format = format_with_file_codec(rec, info, codec);

    if (ov_ptr_valid_result(codec,
                            res,
                            OV_ERROR_INTERNAL_SERVER,
                            "Invalid/Unknown file codec") &&
        (0 != format)) {

        return format;

    } else {

        codec = ov_codec_free(codec);
        return 0;
    }
}

/*----------------------------------------------------------------------------*/

static char *file_name_for(char *target,
                           size_t target_capacity,
                           ov_recorder_record_start const *event,
                           char const *sformat) {

    if (0 != event) {

        ov_recorder_paths_recording_info info = {

            .loop = event->loop,
            .loop_capacity = 0,
            .timestamp_epoch = event->timestamp_epoch,
        };

        ov_id_set(info.id, event->id);

        return ov_recorder_paths_recording_info_to_recording_file_path(
            target,
            target_capacity,
            0,
            &info,
            ov_recorder_paths_format_to_file_extension(sformat));

    } else {

        return 0;
    }
}

/*----------------------------------------------------------------------------*/

static bool restart_recording(ov_recorder_record *rec,
                              ov_recorder_record_stream_entry *entry) {

    if (ov_ptr_valid(entry, "Cannot start recording - no recording entry") &&
        ov_cond_valid(0 == entry->dest_format,
                      "Cannot start recording - recording already in "
                      "progress")) {

        // only required for file name generation - some fields
        // are unused and hence left blank...
        ov_recorder_record_start event = {.ssid = 1,
                                          .codec = 0,
                                          .loop = entry->loop,
                                          .timestamp_epoch = time(0)

        };

        ov_string_copy(event.id, entry->id, sizeof(event.id));

        entry->start_epoch_secs = event.timestamp_epoch;

        ov_format *out_format = create_out_format(rec, &event, 0);
        if (0 != file_name_for(entry->file_name,
                               sizeof(entry->file_name),
                               &event,
                               rec->config.file_format)) {

            entry->dest_format = out_format;
            return true;

        } else {

            out_format = ov_format_close(out_format);
            return false;
        }

    } else {

        return false;
    }
}

/*----------------------------------------------------------------------------*/

static bool roll_recording_if_required(ov_recorder_record_stream_entry *entry) {

    if (ov_ptr_valid(entry,
                     "Cannot handle rolling the recording - no record db "
                     "entry")) {

        if ((0 != entry->num_frames_to_roll_after) &&
            (0 != entry->dest_format) &&
            (entry->num_frames_to_roll_after < ++entry->frames_received)) {

            ov_recording recording = {0};
            return stop_recording(entry, &recording) &&
                   ov_recording_clear(&recording);
        }

        return true;

    } else {

        return false;
    }
}

/*----------------------------------------------------------------------------*/

static size_t buffer_frame_data(ov_recorder_record_stream_entry *entry,
                                ov_rtp_frame const *frame) {

    if ((0 == frame) || (0 == entry) || (0 == entry->source_codec)) {

        ov_log_error("Received invalid argument (0 pointer)");
        return 0;

    } else {

        uint8_t pcm_buffer[OV_MAX_FRAME_LENGTH_BYTES];

        const size_t pcm_buffer_length = sizeof(pcm_buffer);

        int32_t num_bytes = ov_codec_decode(entry->source_codec,
                                            frame->expanded.sequence_number,
                                            frame->expanded.payload.data,
                                            frame->expanded.payload.length,
                                            pcm_buffer,
                                            pcm_buffer_length);

        if (0 > num_bytes) {

            ov_log_error("Could not decode frame payload with codec %s",
                         entry->source_codec->type_id);

            return 0;

        } else {

            ov_buffer pcm = {
                .magic_byte = OV_BUFFER_MAGIC_BYTE,
                .capacity = OV_MAX_FRAME_LENGTH_BYTES,
                .length = (size_t)num_bytes,
                .start = pcm_buffer,
            };

            if (ov_chunker_add(entry->pcm, &pcm)) {
                return num_bytes;
            } else {
                return 0;
            }
        };
    }
}

/*----------------------------------------------------------------------------*/

static bool detect_voice_activity(uint8_t const *pcm,
                                  size_t num_octets,
                                  ov_vad_config parameters) {

    // fprintf(stderr,
    //         "VAD params: zero-x: %f    power_density: %f\n",
    //         parameters.zero_crossings_rate_threshold_hertz,
    //         parameters.powerlevel_density_threshold_db);
    //
    //  if ((0 == parameters.powerlevel_density_threshold_db) ||
    //      (0 == parameters.zero_crossings_rate_threshold_hertz)) {

    //     // fprintf(stderr, "VAD inactive\n");
    //     return true;

    // } else {

    //     ov_vad_parameters vad_params = {0};

    //     bool voice_detected =
    //         ov_pcm_16_get_vad_parameters(
    //             num_octets / OV_DEFAULT_OCTETS_PER_SAMPLE,
    //             (int16_t const *)pcm,
    //             &vad_params) &&
    //         ov_pcm_vad_detected(OV_DEFAULT_SAMPLERATE, vad_params,
    //         parameters);

    //     if (voice_detected) {

    //         fprintf(stderr, "Voice detected\n)");
    //     } else {

    //         fprintf(stderr, "!!! NO  Voice detected!!!\n");
    //     }

    //     return voice_detected;
    // }

    ov_vad_parameters vad_params = {0};

    return (0 == parameters.powerlevel_density_threshold_db) ||
           (0 == parameters.zero_crossings_rate_threshold_hertz) ||
           (ov_pcm_16_get_vad_parameters(
                num_octets / OV_DEFAULT_OCTETS_PER_SAMPLE,
                (int16_t const *)pcm,
                &vad_params) &&
            ov_pcm_vad_detected(OV_DEFAULT_SAMPLERATE, vad_params, parameters));
}

/*----------------------------------------------------------------------------*/

static bool is_recording(ov_recorder_record_stream_entry *entry) {

    return (0 != entry) && (0 != entry->dest_format);
}

/*----------------------------------------------------------------------------*/

static ov_buffer *get_next_chunk(ov_recorder_record_stream_entry *entry,
                                 size_t chunk_size,
                                 size_t min_num_octets_buffered) {

    // Decode frame
    // Add PCM to entry->pcm chunker
    if (ov_ptr_valid(
            entry, "Cannot buffer incoming frame - no stream db entry")) {

        // Look ahead whether enought data is there NO -> return 0
        uint8_t const *lookahead =
            ov_chunker_next_chunk_preview(entry->pcm, min_num_octets_buffered);

        if (0 == lookahead) {

            // fprintf(stderr, "Not enough PCM to start recording\n");

            entry->silent_frames_received = 0;
            ov_log_warning("Not enough PCM to start recording");
            return 0;

        } else {

            // Perform VAD if configured NO VA detected
            bool voice_detected = detect_voice_activity(
                lookahead, min_num_octets_buffered, entry->voice_activity);

            // Remove chunk_size PCMs
            ov_buffer *pcm_to_write =
                ov_chunker_next_chunk(entry->pcm, chunk_size);

            // fprintf(
            //     stderr, "Got another %zu octets ...\n",
            //     pcm_to_write->length);

            if (!voice_detected) {

                if (is_recording(entry)) {

                    ++entry->silent_frames_received;

                    // fprintf(stderr,
                    //         "No voice detected, counter %" PRIu64
                    //         " - limit "
                    //         "%" PRIu64 "\n",
                    //         entry->silent_frames_received,
                    //         entry->silence_cutoff_interval_frames);
                    ov_log_debug("No voice detected, counter %" PRIu64
                                 " - limit "
                                 "%" PRIu64 "\n",
                                 entry->silent_frames_received,
                                 entry->silence_cutoff_interval_frames);

                    if (entry->silent_frames_received >
                        entry->silence_cutoff_interval_frames) {

                        // fprintf(stderr,
                        //         "Silence counter surpassed %" PRIu64
                        //         ": %" PRIu64 "\n",
                        //         entry->silence_cutoff_interval_frames,
                        //         entry->silent_frames_received);

                        ov_log_debug(
                            "VOICE lost - Stop recording: "
                            "Silence counter surpassed %" PRIu64 ": %" PRIu64
                            "\n",
                            entry->silence_cutoff_interval_frames,
                            entry->silent_frames_received);

                        pcm_to_write = ov_buffer_free(pcm_to_write);
                    }

                } else {

                    pcm_to_write = ov_buffer_free(pcm_to_write);
                }

            } else {

                // fprintf(stderr, "VOICE detected\n");
                ov_log_info("VOICE detected");
                entry->silent_frames_received = 0;
            }

            return pcm_to_write;
        }

    } else {

        return 0;
    }
}

/*----------------------------------------------------------------------------*/

static bool record_with_entry(ov_recorder_record *rec,
                              ov_recorder_record_stream_entry *entry,
                              ov_rtp_frame const *frame) {

    bool success_p = false;

    if (ov_ptr_valid(entry, "Cannot record for record DB entry: No entry")) {

        size_t chunk_size = buffer_frame_data(entry, frame);

        if (0 == chunk_size) {

            ov_log_error("Failed to decode/add RTP frame payload to buffer");
            chunk_size =
                OV_DEFAULT_FRAME_LENGTH_SAMPLES * OV_DEFAULT_OCTETS_PER_SAMPLE;
        }

        // Put into chunker and get next 20ms chunk - only get something if
        // voice was detected -> no pcm, no voice -> no data
        // Will remove data worth of one frame if chunker "full", but
        // no voice detected to act as FIFO
        ov_buffer *pcm = get_next_chunk(
            entry, chunk_size, entry->frames_to_buffer * chunk_size);

        if (0 != pcm) {

            if (0 == entry->dest_format) {
                // New stream
                // fprintf(stderr, "Starting new stream\n");
                restart_recording(rec, entry);
            }

            success_p = roll_recording_if_required(entry) &&
                        ov_cond_valid(-1 < ov_format_payload_write_chunk(
                                               entry->dest_format, pcm),
                                      "Could not write to format");

            pcm = ov_buffer_free(pcm);

        } else if (0 != entry->dest_format) {

            // fprintf(stderr, "Stop recording - VOICE lost\n");
            ov_log_info("Silence detected -> Stop recording");

            ov_recording recording = {0};
            success_p = stop_recording(entry, &recording) &&
                        ov_recording_clear(&recording);

            OV_ASSERT(0 == entry->dest_format);

        } else {

            // fprintf(stderr, "Not recording - waiting for VOICE\n");
            ov_log_debug("Not recording - waiting for voice detection");
        }
    }

    return success_p;
}

/*----------------------------------------------------------------------------*/

static bool handle_rtp_frame_message_nocheck(ov_recorder_record *rec,
                                             ov_rtp_frame_message *msg) {
    OV_ASSERT(0 != rec);
    OV_ASSERT(0 != msg);

    bool success_p = false;

    ov_rtp_frame *frame = msg->frame;

    if (0 == frame) {

        ov_log_error("RTP frame message does not contain a frame");
        return false;
    }

    ov_recorder_record_stream_entry *entry =
        ov_recorder_record_stream_db_get(rec->db, frame->expanded.ssrc);

    if (0 == entry) {

        ov_log_error("No recording for MSID %" PRIu32
                     " running, discaring frame",
                     frame->expanded.ssrc);

        return false;

    } else {

        success_p = record_with_entry(rec, entry, frame);

        if (!ov_recorder_record_stream_entry_unlock(entry)) {

            OV_ASSERT(!"MUST NEVER HAPPEN");
        }

        return success_p;
    }
}

/*----------------------------------------------------------------------------*/

static bool handle_message_in_thread(ov_thread_loop *thread_loop,
                                     ov_thread_message *msg) {
    bool successful_p = false;

    ov_recorder_record *rec =
        as_recorder_record(ov_thread_loop_get_data(thread_loop));

    if (ov_ptr_valid(rec,
                     "Called on wrong thread loop object (does not contain a "
                     "ov_recorder_record)") &&
        ov_ptr_valid(msg, "No message received (0 pointer)")) {

        switch (msg->type) {

            case OV_RTP_FRAME_MESSAGE_TYPE:

                successful_p = handle_rtp_frame_message_nocheck(
                    rec, (ov_rtp_frame_message *)msg);

                break;

            default:

                OV_ASSERT(!"MUST NEVER HAPPEN");
        };
    }

    msg = ov_thread_message_free(msg);

    OV_ASSERT(0 == msg);

    return successful_p;
}

/*----------------------------------------------------------------------------*/

static bool handle_message_in_loop(ov_thread_loop *thread_loop,
                                   ov_thread_message *msg) {
    UNUSED(thread_loop);
    UNUSED(msg);

    TODO("IMPLEMENT");
    return false;
}

/*----------------------------------------------------------------------------*/

ov_recorder_record *ov_recorder_record_create(ov_event_loop *loop,
                                              ov_recorder_record_config cfg) {
    ov_recorder_record *rd = 0;

    if (0 == cfg.num_threads) {

        cfg.num_threads = OV_DEFAULT_NUM_THREADS;
    }

    if (0 == cfg.lock_timeout_usecs) {

        cfg.lock_timeout_usecs = OV_DEFAULT_LOCK_TIMEOUT_USECS;
    }

    rd = recorder_record_create_nocheck(cfg);

    if (0 == rd) goto error;

    ov_thread_loop_callbacks callbacks = {
        .handle_message_in_loop = handle_message_in_loop,
        .handle_message_in_thread = handle_message_in_thread,

    };

    rd->thread_loop = ov_thread_loop_create(loop, callbacks, rd);

    if (0 == rd->thread_loop) {

        ov_log_error("Could not create ov_recorder_record");
        goto error;
    }

    rd->db = ov_recorder_record_stream_db_create(cfg.lock_timeout_usecs);

    if (0 == rd->db) {

        ov_log_error("Could not create record stream database");
        goto error;
    }

    ov_rtp_frame_message_enable_caching(cfg.num_threads *
                                        THREAD_MESSAGES_PER_THREAD);

    ov_thread_loop_config tl_config = {

        .disable_to_loop_queue = false,
        .message_queue_capacity = cfg.num_threads * THREAD_MESSAGES_PER_THREAD,
        .lock_timeout_usecs = cfg.lock_timeout_usecs,
        .num_threads = cfg.num_threads,

    };

    if (!ov_thread_loop_reconfigure(rd->thread_loop, tl_config)) {

        ov_log_warning("Could not reconfigure thread pool");
    }

    if (!ov_thread_loop_start_threads(rd->thread_loop)) {

        ov_log_error("Could not start threads");
        goto error;
    }

    return rd;

error:

    rd = ov_recorder_record_free(rd);

    OV_ASSERT(0 == rd);

    return 0;
}

/*****************************************************************************
                             get running recordings
 ****************************************************************************/

bool ov_recorder_record_get_running_recordings(ov_recorder_record *rec,
                                               ov_json_value *target) {
    if (0 == rec) {

        ov_log_error("Got 0 pointer");
        goto error;
    }

    return ov_recorder_record_stream_db_list(rec->db, target);

error:

    return false;
}

/*----------------------------------------------------------------------------*/

ov_recorder_record *ov_recorder_record_free(ov_recorder_record *record) {
    if (0 == record) goto error;

    if (0 != record->thread_loop) {

        record->thread_loop = ov_thread_loop_free(record->thread_loop);
    }

    if (0 != record->db) {

        record->db = ov_recorder_record_stream_db_free(record->db);
    }

    ov_recorder_record_config *cfg = &record->config;

    if (0 != cfg->file_codec) {

        cfg->file_codec = ov_json_value_free((ov_json_value *)cfg->file_codec);
    }

    cfg->file_format = ov_free((char *)cfg->file_format);
    cfg->repository_root_path = ov_free((char *)cfg->repository_root_path);

    OV_ASSERT(0 == record->thread_loop);
    OV_ASSERT(0 == record->db);

    OV_ASSERT(0 == record->config.file_codec);
    OV_ASSERT(0 == record->config.file_codec);
    OV_ASSERT(0 == record->config.repository_root_path);

    free(record);
    record = 0;

error:

    return record;
}

/*----------------------------------------------------------------------------*/

bool ov_recorder_record_send_to_threads(ov_recorder_record *rec,
                                        ov_thread_message *msg) {
    if ((0 == rec) || (0 == msg)) {

        ov_log_error("0 pointer");
        goto error;
    }

    return ov_thread_loop_send_message(
        rec->thread_loop, msg, OV_RECEIVER_THREAD);

error:

    return false;
}

/*----------------------------------------------------------------------------*/

char *ov_recorder_record_filename(ov_recorder_record const *self,
                                  uint32_t ssrc,
                                  char *target,
                                  size_t target_capacity) {
    ov_recorder_record_stream_entry *stream = ov_recorder_record_stream_db_get(
        (ov_recorder_record_stream_db *)get_stream_db(self), ssrc);

    if (ov_ptr_valid(
            target, "Cannot get file name for recording - no target buffer") &&
        ov_ptr_valid(stream,
                     "Cannot get file name for recording - no recording "
                     "found")) {

        ov_string_copy(target, stream->file_name, target_capacity);

    } else {

        target = 0;
    }

    ov_recorder_record_stream_entry_unlock(stream);

    return target;
}

/*----------------------------------------------------------------------------*/
