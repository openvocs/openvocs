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

#include <ov_base/ov_utils.h>

#include <ov_base/ov_constants.h>
#include <ov_base/ov_event_keys.h>
#include <ov_base/ov_json.h>
#include <ov_base/ov_result.h>
#include <ov_base/ov_rtp_frame.h>
#include <ov_base/ov_utils.h>

#include <ov_core/ov_event_api.h>

#include <ov_backend/ov_rtp_frame_message.h>
#include <ov_backend/ov_signaling_app.h>
#include <ov_core/ov_recorder_events.h>

#include <ov_format/ov_format.h>
#include <ov_format/ov_format_codec.h>
#include <ov_format/ov_format_wav.h>

#include "ov_recorder_app.h"
#include "ov_recorder_record.h"
#include <ov_base/ov_id.h>
#include <ov_base/ov_mc_socket.h>
#include <ov_base/ov_rtp_app.h>
#include <ov_base/ov_string.h>
#include <ov_codec/ov_codec.h>
#include <ov_core/ov_notify.h>
#include <ov_core/ov_recording.h>
#include <ov_format/ov_format_ogg.h>
#include <ov_format/ov_format_ogg_opus.h>
#include <wchar.h>

/*****************************************************************************
                                    InStream
 ****************************************************************************/

uint32_t IN_STREAM_MAGIC_BYTES = 0x312abcb2;

#define OV_ROLL_RECORDING_AFTER_SECS_DEFAULT 5

typedef struct {

    uint32_t magic_bytes;

    ov_recorder_app *rec_app;
    ov_rtp_app *rtp;

    // Set ONCE during initialization
    uint32_t internal_ssrc;
    ov_recorder_record *record;

    ov_id id;

    ov_recorder_record_start record_start_event;

} InStream;

/*----------------------------------------------------------------------------*/

static InStream *as_in_stream(void *self) {

    InStream *in = self;

    if (ov_ptr_valid(self, "Invalid InStream: 0 pointer") &&
        ov_cond_valid(IN_STREAM_MAGIC_BYTES == in->magic_bytes,
                      "Invalid InStream: Magic Bytes mismatch")) {
        return in;

    } else {
        return 0;
    }
}

/*----------------------------------------------------------------------------*/

static bool in_stream_stop(InStream *self) {

    if (0 != self) {

        self->rtp = ov_rtp_app_free(self->rtp);

        self->record_start_event.codec =
            ov_json_value_free(self->record_start_event.codec);

        self->record_start_event.loop = ov_free(self->record_start_event.loop);

        return true;
    } else {
        return false;
    }
}

/*----------------------------------------------------------------------------*/

static bool in_stream_clear(InStream *self) {

    if (in_stream_stop(self)) {

        self->record = 0;
        self->internal_ssrc = 0;

        return true;

    } else {

        return false;
    }
}

/*----------------------------------------------------------------------------*/

static InStream *in_stream_for_id(InStream *streams,
                                  size_t num_streams,
                                  char const *id) {

    if ((0 != streams) && (0 != id)) {

        for (size_t i = 0; i < num_streams; ++i) {

            if (ov_id_match(id, streams[i].id)) {

                return streams + i;
            }
        }
    }

    return 0;
}

/*****************************************************************************
                                ov_recorder_app
 ****************************************************************************/

static const uint32_t magic_bytes = 0x2314aabc;

struct ov_recorder_app_struct {

    uint32_t magic_bytes;

    ov_recorder_record *record;

    // ov_recorder_playback *playback;

    // Handling the signalling
    ov_app *app;
    int resmgr_fd;

    struct {

        char *repository_root_path;
        size_t num_threads;
        uint64_t lock_timeout_usecs;

        size_t max_num_in_streams;

    } config;

    InStream *in_streams;

    ov_json_value *record_codec;

    ov_vad_config default_vad_parameters;

    uint64_t silence_cutoff_interval_secs;

} ov_recorder_app_struct;

/*----------------------------------------------------------------------------*/

static ov_recorder_app const *as_recorder_app(void const *data) {

    if (0 == data) return 0;

    ov_recorder_app const *rd = data;

    if (magic_bytes != rd->magic_bytes) return 0;

    return rd;
}

/*----------------------------------------------------------------------------*/

static ov_recorder_app *as_recorder_app_mut(void *data) {

    return (ov_recorder_app *)as_recorder_app(data);
}

/*----------------------------------------------------------------------------*/

static ov_app const *get_signaling_app(ov_recorder_app const *self) {

    if (0 != as_recorder_app(self)) {
        return self->app;
    } else {
        return 0;
    }
}

/*----------------------------------------------------------------------------*/

static ov_app *get_signaling_app_mut(ov_recorder_app *self) {
    return (ov_app *)get_signaling_app(self);
}

/*----------------------------------------------------------------------------*/

static int get_resmgr_fd(ov_recorder_app const *self) {

    if (ov_ptr_valid(self, "Cannot get FD for ResMgr: No recorder app")) {
        return self->resmgr_fd;
    } else {
        return -1;
    }
}

/*----------------------------------------------------------------------------*/

static ov_json_value const *get_record_codec(ov_recorder_app const *self) {

    if (ov_ptr_valid(
            self, "Cannot get codec for recording - invalid record app")) {

        return self->record_codec;

    } else {

        return 0;
    }
}

/*----------------------------------------------------------------------------*/

static ov_recorder_record const *get_record(ov_recorder_app const *self) {

    if (ov_ptr_valid(self, "Cannot get record object: No recorder app")) {
        return self->record;
    } else {
        return 0;
    }
}

/*----------------------------------------------------------------------------*/

static ov_recorder_record *get_record_mut(ov_recorder_app *self) {

    return (ov_recorder_record *)get_record(self);
}

/*----------------------------------------------------------------------------*/

static InStream *get_unused_in_stream_from_array(InStream *in_streams,
                                                 size_t num_in_streams) {

    if (ov_ptr_valid(in_streams, "Cannot get unused InStream - no streams")) {

        for (size_t i = 0; i < num_in_streams; ++i) {

            if (0 == in_streams[i].rtp) {

                return in_streams + i;
            }
        }
    }

    return 0;
}

/*----------------------------------------------------------------------------*/

static InStream *get_unused_in_stream(ov_recorder_app *self) {

    if (ov_ptr_valid(self, "Cannot get unused InStream - no recorder app")) {
        return get_unused_in_stream_from_array(
            self->in_streams, self->config.max_num_in_streams);
    } else {
        return 0;
    }
}

/*****************************************************************************
                             ov_recorder_app_create
 ****************************************************************************/

static void install_default_formats() {

    ov_format_wav_install(0);
    ov_format_ogg_install(0);
    ov_format_ogg_opus_install(0);
    ov_format_codec_install(0);
}

/*----------------------------------------------------------------------------*/

static ov_app *create_app(ov_recorder_app *self,
                          char const *app_name,
                          ov_event_loop *loop,
                          uint64_t reconnect_interval_secs) {

    ov_app_config app_config = {

        .loop = loop,

        .reconnect.interval_secs = reconnect_interval_secs,
        .reconnect.max_connections = 1,

    };

    ov_string_copy(app_config.name, app_name, sizeof(app_config.name));

    ov_app *app = ov_signaling_app_create(app_config);

    if (ov_signaling_app_set_userdata(app, self)) {
        return app;
    } else {
        return ov_app_free(app);
    }
}

/*----------------------------------------------------------------------------*/

static bool initialize_in_streams(InStream *streams,
                                  size_t capacity,
                                  ov_recorder_app *rec_app,
                                  ov_recorder_record *record) {

    if (ov_ptr_valid(streams, "Cannot initialize streams - no streams")) {

        for (size_t i = 0; i < capacity; ++i) {

            streams[i].magic_bytes = IN_STREAM_MAGIC_BYTES;
            streams[i].internal_ssrc = 1 + i;
            streams[i].record = record;

            streams[i].rec_app = rec_app;
        }

        return true;

    } else {
        return false;
    }
}

/*----------------------------------------------------------------------------*/

static bool initialize_recorder_app(ov_recorder_app *self,
                                    char const *app_name,
                                    ov_event_loop *loop,
                                    ov_recorder_config cfg) {
    fprintf(stderr,
            "recorder app: VAD parameters: x-cross: %f    power: %f\n",
            cfg.defaults.vad.zero_crossings_rate_threshold_hertz,
            cfg.defaults.vad.powerlevel_density_threshold_db);

    ov_app *sapp =
        create_app(self, app_name, loop, cfg.reconnect_interval_secs);

    if ((0 == as_recorder_app(self))) {

        sapp = ov_app_free(sapp);
        return false;

    } else {

        self->app = sapp;
        self->resmgr_fd = -1;
        self->record_codec = ov_json_value_from_string(
            OV_INTERNAL_CODEC, strlen(OV_INTERNAL_CODEC));
        self->default_vad_parameters = cfg.defaults.vad;
        self->silence_cutoff_interval_secs =
            cfg.defaults.silence_cutoff_interval_secs;
        return true;
    }
}

/*----------------------------------------------------------------------------*/

static bool set_repository_path(ov_recorder_app *self, char const *path) {

    path = OV_OR_DEFAULT(path, OV_RECORDER_DEFAULT_REPOSITORY_ROOT_PATH);

    if (ov_ptr_valid(self, "Cannot set repository path - no recorder app")) {
        self->config.repository_root_path = strdup(path);
        return true;
    } else {
        return false;
    }
}

/*----------------------------------------------------------------------------*/

static bool set_threading_parameters(ov_recorder_app *self,
                                     ov_recorder_config cfg) {

    if (ov_ptr_valid(
            self, "Cannot set threading parameters - no recorder app")) {

        self->config.lock_timeout_usecs = cfg.lock_timeout_usecs;
        self->config.num_threads = cfg.num_threads;
        return true;

    } else {

        return false;
    }
}

/*----------------------------------------------------------------------------*/

static bool set_in_stream_parameters(ov_recorder_app *self,
                                     ov_recorder_config cfg) {

    if (ov_ptr_valid(
            self, "Cannot set input stream parameters - no recorder app")) {

        self->config.max_num_in_streams =
            OV_OR_DEFAULT(cfg.max_parallel_recordings,
                          OV_RECORDER_DEFAULT_MAX_PARALLEL_RECORDINGS);

        return true;

    } else {

        return false;
    }
}

/*----------------------------------------------------------------------------*/

static bool create_in_streams_array(ov_recorder_app *self) {

    if (ov_ptr_valid(
            self, "Cannot create input stream array - no recorder app")) {

        self->in_streams =
            calloc(self->config.max_num_in_streams, sizeof(InStream));
        return true;

    } else {

        return false;
    }
}

/*----------------------------------------------------------------------------*/

static bool setup_record_object(ov_recorder_app *self,
                                ov_event_loop *loop,
                                ov_recorder_config cfg) {

    if (ov_ptr_valid(self, "Cannot setup record object - 0 pointer")) {

        ov_recorder_record_config record_config = {
            .file_format = cfg.defaults.file_format,
            .file_codec = cfg.defaults.codec,
            .repository_root_path = cfg.repository_root_path,
            .num_threads = cfg.num_threads,
            .lock_timeout_usecs = cfg.lock_timeout_usecs,
        };

        self->record = ov_recorder_record_create(loop, record_config);

        return 0 != self->record;

    } else {

        return false;
    }
}

/*----------------------------------------------------------------------------*/

static bool register_callbacks(ov_recorder_app *self);

static bool trigger_connect_to_resmgr(ov_recorder_app *self,
                                      ov_socket_configuration cfg);

/*----------------------------------------------------------------------------*/

ov_recorder_app *ov_recorder_app_create(char const *app_name,
                                        ov_recorder_config cfg,
                                        ov_event_loop *loop) {

    install_default_formats();

    ov_recorder_app *rapp = calloc(1, sizeof(ov_recorder_app));
    rapp->magic_bytes = magic_bytes;

    if ((!initialize_recorder_app(rapp, app_name, loop, cfg)) ||
        (!set_repository_path(rapp, cfg.repository_root_path)) ||
        (!set_threading_parameters(rapp, cfg)) ||
        (!set_in_stream_parameters(rapp, cfg)) ||
        (!create_in_streams_array(rapp)) ||
        (!setup_record_object(rapp, loop, cfg)) ||
        (!initialize_in_streams(rapp->in_streams,
                                rapp->config.max_num_in_streams,
                                rapp,
                                get_record_mut(rapp))) ||
        (!register_callbacks(rapp)) ||
        (!trigger_connect_to_resmgr(rapp, cfg.liege_socket))) {
        rapp = ov_recorder_app_free(rapp);
    }

    return rapp;
}

/*----------------------------------------------------------------------------*/

static bool clear_in_streams(ov_recorder_app *self) {

    if (0 != self) {

        for (size_t i = 0; i < self->config.max_num_in_streams; ++i) {
            in_stream_clear(self->in_streams + i);
        }

        return true;

    } else {

        return true;
    }
}

/*----------------------------------------------------------------------------*/

static bool send_to_resmgr(ov_recorder_app *self, ov_json_value const *msg) {

    int fd = get_resmgr_fd(self);
    ov_app *app = get_signaling_app_mut(self);

    if ((!ov_ptr_valid(
            msg, "Not going to send a request - no message to send")) ||
        (!ov_cond_valid(
            fd > -1, "Not going to send a request - Not connected"))) {
        return false;
    } else {

        return ov_app_send(app, fd, 0, (ov_json_value *)msg);
    }
}

/*----------------------------------------------------------------------------*/

static bool notify_liege_recording_stopped(ov_recorder_app *self,
                                           ov_recording recording) {

    ov_notify_parameters params = {
        .recording = recording,
    };

    ov_json_value *msg = ov_notify_message(0, NOTIFY_NEW_RECORDING, params);
    bool ok = send_to_resmgr(self, msg);
    ov_json_value_free(msg);
    return ok;
}

/*----------------------------------------------------------------------------*/

static bool reset(ov_recorder_app *self) {

    ov_recorder_event_stop stop = {0};

    if (0 != self) {

        bool ok = true;

        for (size_t i = 0; ok && (i < self->config.max_num_in_streams); ++i) {

            InStream *stream = self->in_streams + i;
            ov_id_set(stop.id, stream->id);

            ov_recording recording = {0};

            ok = in_stream_stop(stream) && ov_recorder_record_stop_recording(
                                               self->record, &stop, &recording);

            if (!ok) {

                ov_log_error("Could not stop Stream %zu (%s)", i, stream->id);
            }

            ov_recording_clear(&recording);
        }

        return ok;

    } else {

        return true;
    }
}

/*----------------------------------------------------------------------------*/

ov_recorder_app *ov_recorder_app_free(ov_recorder_app *self) {

    if (ov_ptr_valid(
            as_recorder_app(self), "Cannot free recorder app - 0 pointer")) {

        self->record = ov_recorder_record_free(self->record);

        self->app = ov_app_free(self->app);

        clear_in_streams(self);

        self->in_streams = ov_free(self->in_streams);

        self->config.repository_root_path =
            ov_free(self->config.repository_root_path);

        self->record_codec = ov_json_value_free(self->record_codec);

        return ov_free(self);

    } else {
        return self;
    }
}

/*****************************************************************************
                        Signalling connection callbacks
 ****************************************************************************/

static ov_json_value *register_msg_create(char const *uuid) {

    ov_json_value *msg = ov_event_api_message_create(OV_KEY_REGISTER, uuid, 0);
    ov_json_object_set(ov_event_api_set_parameter(msg),
                       OV_KEY_TYPE,
                       ov_json_string("recorder"));
    ov_json_object_set(
        ov_event_api_set_parameter(msg), OV_KEY_UUID, ov_json_string(uuid));
    return msg;
}

/*----------------------------------------------------------------------------*/

static bool connected_cb(ov_app *app, int fd, void *userdata) {

    UNUSED(app);

    ov_id id = {0};
    ov_id_fill_with_uuid(id);

    ov_recorder_app *rapp = as_recorder_app_mut(userdata);

    if (0 != rapp) {

        rapp->resmgr_fd = fd;

        ov_json_value *register_msg = register_msg_create(id);
        bool ok = send_to_resmgr(rapp, register_msg);
        register_msg = ov_json_value_free(register_msg);

        return ok;

    } else {

        return false;
    }
}

/*----------------------------------------------------------------------------*/

static void closed_cb(ov_app *app, int fd, char const *uuid, void *userdata) {

    UNUSED(app);
    UNUSED(uuid);

    ov_recorder_app *rapp = as_recorder_app_mut(userdata);

    if (0 != rapp) {

        OV_ASSERT(rapp->resmgr_fd == fd);
        rapp->resmgr_fd = -1;
    }

    reset(rapp);
}

/*----------------------------------------------------------------------------*/

static bool trigger_connect_to_resmgr(ov_recorder_app *self,
                                      ov_socket_configuration cfg) {

    ov_app_socket_config scfg = {

        .socket_config = cfg,
        .as_client = true,

        .callback.reconnected = connected_cb,
        .callback.close = closed_cb,
        .callback.userdata = self,

    };

    return ov_signaling_app_connect(get_signaling_app_mut(self), scfg);
}

/*****************************************************************************
                                  record_start
 ****************************************************************************/

static bool set_codec_for_record_start(ov_recorder_record_start *event,
                                       ov_json_value const *codec) {

    if (0 != event) {

        event->codec = ov_json_value_copy((void **)&event->codec, codec);
        return true;

    } else {

        return false;
    }
}

/*----------------------------------------------------------------------------*/

static bool set_vad_for_record_start(ov_recorder_record_start *event,
                                     size_t frames_to_use,
                                     uint64_t silence_cutoff_interval_secs,
                                     double zero_crossings_rate_threshold_hertz,
                                     double powerlevel_density_threshold_db) {

    fprintf(stderr,
            "VAD parameters: x-cross: %f    power: %f     frames: %zu\n",
            zero_crossings_rate_threshold_hertz,
            powerlevel_density_threshold_db,
            frames_to_use);

    if (0 != event) {

        event->voice_activity.frames_to_use = frames_to_use;
        event->voice_activity.silence_cutoff_interval_frames =
            silence_cutoff_interval_secs * 1000 / OV_DEFAULT_FRAME_LENGTH_MS;

        event->voice_activity.parameters.powerlevel_density_threshold_db =
            powerlevel_density_threshold_db;

        event->voice_activity.parameters.zero_crossings_rate_threshold_hertz =
            zero_crossings_rate_threshold_hertz;

        return true;

    } else {

        return false;
    }
}
/*----------------------------------------------------------------------------*/

static bool set_uuid_for_record_start(ov_recorder_record_start *event,
                                      ov_id const id) {

    if (0 != event) {

        return ov_id_set(event->id, id);

    } else {

        return false;
    }
}

/*----------------------------------------------------------------------------*/

static bool set_ssrc_from_in_stream(ov_recorder_record_start *event,
                                    InStream const *in_stream) {

    if ((0 != event) && (0 != in_stream)) {
        event->ssid = in_stream->internal_ssrc;
        return true;
    } else {
        return false;
    }
}

/*----------------------------------------------------------------------------*/

static bool set_loop_from_start_event(
    ov_recorder_record_start *event,
    ov_recorder_event_start const *start_event) {

    if ((0 != event) && (0 != start_event)) {
        event->loop = ov_string_dup(start_event->loop);
        return true;
    } else {
        return false;
    }
}

/*----------------------------------------------------------------------------*/

static bool set_uuid_to(InStream *in_stream, ov_id id) {

    if (0 != in_stream) {

        ov_string_copy(in_stream->id, id, sizeof(in_stream->id));
        return true;

    } else {
        return false;
    }
}

/*----------------------------------------------------------------------------*/

static bool set_rtp_app_to(InStream *in_stream, ov_rtp_app *app) {

    if (0 != in_stream) {

        in_stream->rtp = app;
        return true;

    } else {

        return false;
    }
}

/*----------------------------------------------------------------------------*/

static bool set_rolling_after_secs_from_start_event(
    InStream *in_stream, ov_recorder_event_start const *start_event) {

    size_t rtp_frames_per_sec = 1000 / OV_DEFAULT_FRAME_LENGTH_MS;

    if (ov_ptr_valid(
            in_stream, "Cannot set roll recording after secs - no in stream") &&
        ov_ptr_valid(start_event,
                     "Cannot set roll recording after secs - no start event")) {

        in_stream->record_start_event.num_frames_to_roll_recording_after =
            rtp_frames_per_sec * start_event->roll_after_secs;

        ov_log_info(
            "Set roll after secs to %zu (%" PRIu16 ")",
            in_stream->record_start_event.num_frames_to_roll_recording_after,
            start_event->roll_after_secs);

        return true;

    } else {

        return false;
    }
}

/*----------------------------------------------------------------------------*/

static bool cb_recording_stopped(ov_recording recording, void *data) {

    return notify_liege_recording_stopped(data, recording);
}

/*----------------------------------------------------------------------------*/

static bool record_start_now(InStream *in, ov_result *res) {

    if (ov_ptr_valid(in, "Cannot start recording - no in stream")) {

        ov_recorder_record_callbacks callbacks = {

            .cb_recording_stopped = cb_recording_stopped,
            .data = in->rec_app,

        };

        in->record_start_event.timestamp_epoch = time(0);

        return ov_cond_valid(
            ov_recorder_record_start_recording(
                in->record, &in->record_start_event, callbacks, res),
            "Could not start recording");

    } else {

        return false;
    }
}

/*----------------------------------------------------------------------------*/

static bool io_rtp_handler(ov_rtp_frame *rtp_frame, void *userdata) {

    InStream *in_stream = as_in_stream(userdata);

    if (ov_ptr_valid(in_stream, "Cannot accept RTP - no InStream object") &&
        ov_ptr_valid(rtp_frame, "Cannot forward RTP - no frame")) {

        // To allow record instance to identify the frame properly...
        rtp_frame->expanded.ssrc = in_stream->internal_ssrc;

        ov_thread_message *msg = ov_rtp_frame_message_create(rtp_frame);
        rtp_frame = 0;

        if (!ov_recorder_record_send_to_threads(in_stream->record, msg)) {

            ov_log_error("Could not forward frame message");
            msg = ov_thread_message_free(msg);
        }
    }

    rtp_frame = ov_rtp_frame_free(rtp_frame);

    return true;
}

/*----------------------------------------------------------------------------*/

static bool start_recording(ov_event_loop *loop,
                            InStream *in_stream,
                            ov_recorder_event_start const *start_event,
                            ov_id id,
                            ov_json_value const *codec,
                            ov_result *res) {

    if (0 != start_event) {

        ov_socket_configuration sconf = {
            .port = start_event->mc_port,
            .type = UDP,
        };

        ov_string_copy(sconf.host, start_event->mc_ip, sizeof(sconf.host));

        ov_rtp_app_config cfg = {

            .multicast = true,
            .rtp_handler = io_rtp_handler,
            .rtp_handler_userdata = in_stream,
            .rtp_socket = sconf,

        };

        ov_rtp_app *rtp_app = ov_rtp_app_create(loop, cfg);

        if (ov_cond_valid_result(0 != in_stream,
                                 res,
                                 OV_ERROR_NO_RESOURCE,
                                 "Cannot start recording: Maximum number of "
                                 "parallel recordings reached") &&
            set_codec_for_record_start(&in_stream->record_start_event, codec) &&
            set_vad_for_record_start(
                &in_stream->record_start_event,
                3,
                in_stream->rec_app->silence_cutoff_interval_secs,
                in_stream->rec_app->default_vad_parameters
                    .zero_crossings_rate_threshold_hertz,
                in_stream->rec_app->default_vad_parameters
                    .powerlevel_density_threshold_db) &&

            set_uuid_for_record_start(&in_stream->record_start_event, id) &&
            set_ssrc_from_in_stream(
                &in_stream->record_start_event, in_stream) &&
            set_loop_from_start_event(
                &in_stream->record_start_event, start_event) &&
            set_uuid_to(in_stream, id) && set_rtp_app_to(in_stream, rtp_app) &&
            set_rolling_after_secs_from_start_event(in_stream, start_event)) {

            if (record_start_now(in_stream, res)) {

                return true;

            } else {

                in_stream_stop(in_stream);
                return false;
            }

        } else {

            rtp_app = ov_rtp_app_free(rtp_app);
            return false;
        }

    } else {

        return false;
    }
}

/*----------------------------------------------------------------------------*/

static ov_json_value *create_record_start_success_response(
    ov_json_value const *orig_message, InStream const *in_stream) {

    ov_json_value *response = 0;

    if (ov_ptr_valid(
            in_stream, "Cannot create response: Invalid InStream object")) {

        ov_recorder_response_start start = {.filename = "Unknown yet"};
        ov_id_set(start.id, in_stream->id);

        response = ov_event_api_create_success_response(orig_message);

        if (!ov_recorder_response_start_to_json(
                ov_event_api_get_response(response), &start)) {

            response = ov_json_value_free(response);
        }
    }

    return response;
}

/*----------------------------------------------------------------------------*/

// static bool set_default_vad_parameters(ov_recorder_event_start *start,
// uint32_t frames_to_use,
//         double zero_crossings, double power_density) {
//
//     if(0 != start) {
//
//         if(0 == start->
//
//         return true;
//
//     } else {
//
//         return false;
//
//     }
//
// }

/*----------------------------------------------------------------------------*/

static ov_json_value *cb_record_start(ov_app *app,
                                      const char *event,
                                      const ov_json_value *orig_message,
                                      int socket,
                                      const ov_socket_data *remote) {

    UNUSED(event);
    UNUSED(socket);
    UNUSED(remote);

    ov_json_value *response = 0;
    ov_result res = {0};

    ov_json_value *params = ov_event_api_get_parameter(orig_message);

    ov_recorder_event_start start = {0};

    ov_recorder_app *rapp =
        as_recorder_app_mut(ov_signaling_app_get_userdata(app));

    InStream *in_stream_to_use = get_unused_in_stream(rapp);

    ov_id recording_id;
    ov_id_fill_with_uuid(recording_id);

    if (ov_cond_valid_result(0 != app,
                             &res,
                             OV_ERROR_INTERNAL_SERVER,
                             "Cannot start recording: No recorder app") &&
        ov_cond_valid_result(ov_recorder_event_start_from_json(params, &start),
                             &res,
                             OV_ERROR_INTERNAL_SERVER,
                             "Cannot read " OV_EVENT_START_RECORD " JSON") &&
        // set_default_vad_parameters(&start, 20, 0, 0) &&
        start_recording(app->config.loop,
                        in_stream_to_use,
                        &start,
                        recording_id,
                        get_record_codec(rapp),
                        &res)) {

        response = create_record_start_success_response(
            orig_message, in_stream_to_use);
    }

    if ((0 == response) && (0 != orig_message)) {

        response = ov_event_api_create_error_response(
            orig_message, res.error_code, ov_result_get_message(res));
    }

    ov_result_clear(&res);

    response = OV_OR_DEFAULT(response, (ov_json_value *)FAILURE_CLOSE_SOCKET);

    ov_recorder_event_start_clear(&start);

    return response;
}

/*----------------------------------------------------------------------------*/

static ov_json_value *cb_record_stop(ov_app *app,
                                     const char *event,
                                     const ov_json_value *orig_message,
                                     int socket,
                                     const ov_socket_data *remote) {

    UNUSED(event);
    UNUSED(socket);
    UNUSED(remote);

    ov_json_value *parameter = ov_event_api_get_parameter(orig_message);

    ov_json_value *response = 0;

    ov_recorder_event_stop stop = {0};
    ov_recording recording = {0};

    ov_recorder_app *rapp =
        as_recorder_app_mut(ov_signaling_app_get_userdata(app));

    if (ov_ptr_valid(rapp, "Cannot stop recording - invalid recorder app") &&
        ov_cond_valid(ov_recorder_event_stop_from_json(parameter, &stop),
                      "Cannot stop recording - message invalid") &&
        in_stream_stop(in_stream_for_id(
            rapp->in_streams, rapp->config.max_num_in_streams, stop.id)) &&
        ov_recorder_record_stop_recording(rapp->record, &stop, &recording)) {

        response = ov_event_api_create_success_response(orig_message);
    }

    ov_recorder_event_stop_clear(&stop);
    ov_recording_clear(&recording);

    if ((0 == response) && (0 != orig_message)) {
        response =
            ov_event_api_create_error_response(orig_message,
                                               OV_ERROR_INTERNAL_SERVER,
                                               OV_ERROR_DESC_INTERNAL_SERVER);
    }

    if (0 == response) {
        response = (ov_json_value *)FAILURE_CLOSE_SOCKET;
    }

    return response;
}

/*----------------------------------------------------------------------------*/

static ov_json_value *cb_list_running_recordings(ov_app *app,
                                                 const char *name,
                                                 const ov_json_value *message,
                                                 int fd,
                                                 const ov_socket_data *remote) {

    UNUSED(name);
    UNUSED(fd);
    UNUSED(remote);

    ov_recorder_app *rapp =
        as_recorder_app_mut(ov_signaling_app_get_userdata(app));

    ov_json_value *response = ov_event_api_create_success_response(message);

    if (ov_ptr_valid(rapp,
                     "Cannot list recordings - internal error (no recorder "
                     "app)") &&
        ov_recorder_record_get_running_recordings(
            rapp->record, ov_event_api_get_response(response))) {

        return response;

    } else {

        response = ov_json_value_free(response);

        return ov_event_api_create_error_response(message,
                                                  OV_ERROR_INTERNAL_SERVER,
                                                  "Cannot list recordings "
                                                  "- no "
                                                  "recorder app");
    }
}

/*----------------------------------------------------------------------------*/

static ov_json_value *cb_shutdown(ov_app *app,
                                  const char *name,
                                  const ov_json_value *message,
                                  int fd,
                                  const ov_socket_data *remote) {

    UNUSED(name);
    UNUSED(fd);
    UNUSED(remote);

    if (ov_app_stop(app)) {

        return ov_event_api_create_success_response(message);

    } else {

        return ov_event_api_create_error_response(
            message, OV_ERROR_INTERNAL_SERVER, "Could not stop app");
    }
}

/*----------------------------------------------------------------------------*/

static bool register_callbacks(ov_recorder_app *self) {

    ov_app *app = get_signaling_app_mut(self);

    return ov_signaling_app_register_command(
               app, OV_EVENT_START_RECORD, 0, cb_record_start) &&
           ov_signaling_app_register_command(
               app, OV_EVENT_STOP_RECORD, 0, cb_record_stop) &&
           ov_signaling_app_register_command(app,
                                             OV_EVENT_LIST_RUNNING_RECORDINGS,
                                             0,
                                             cb_list_running_recordings) &&
           ov_signaling_app_register_command(
               app, OV_KEY_SHUTDOWN, 0, cb_shutdown);

    return true;
}

/*----------------------------------------------------------------------------*/
