/***

  Copyright   2018    German Aerospace Center DLR e.V.,
  German Space Operations Center (GSOC)

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

This file is part of the openvocs project. http://openvocs.org

 ***/ /**

     \file               ov_pulse_context.c
     \author             Michael J. Beer, DLR/GSOC <michael.beer@dlr.de>
     \date               2018-02-22

     \ingroup            empty

    **/
/*---------------------------------------------------------------------------*/

#include <ov_base/ov_utils.h>

#include <pulse/context.h>
#include <pulse/error.h>
#include <pulse/stream.h>
#include <pulse/thread-mainloop.h>

#include <ov_base/ov_buffer.h>
#include <ov_base/ov_cache.h>
#include <ov_base/ov_config_keys.h>
#include <ov_base/ov_event_keys.h>
#include <ov_base/ov_ringbuffer.h>
#include <ov_base/ov_thread_lock.h>
#include <ov_base/ov_utils.h>
#include <ov_log/ov_log.h>

#include "ov_pulse_context.h"

/******************************************************************************
 *                             INTERNAL CONSTANTS
 ******************************************************************************/

char const *DEFAULT_NAME = "openvocs";

/*---------------------------------------------------------------------------*/

#if OV_BYTE_ORDER == OV_LITTLE_ENDIAN

#define SAMPLE_FORMAT PA_SAMPLE_S16LE

#else

#define SAMPLE_FORMAT PA_SAMPLE_S16BE

#endif

/*---------------------------------------------------------------------------*/

#define UPDATE_AVERAGE(avg, val)                                               \
    do {                                                                       \
        avg *= 100;                                                            \
        avg += val;                                                            \
        avg /= 101;                                                            \
    } while (0)

/******************************************************************************
 *                                  TYPEDEFS
 ******************************************************************************/

typedef enum {

    PLAYBACK = 0,
    RECORD = 1,

} ov_stream_mode;

/*---------------------------------------------------------------------------*/

typedef struct {

    uint8_t *data;
    size_t capacity_bytes;
    size_t nbytes;

} Buffer;

/*---------------------------------------------------------------------------*/

typedef struct {

    pa_stream *stream;

    ov_thread_lock *lock;
    ov_ringbuffer *queue;

    Buffer *current;

} AudioIo;

/*---------------------------------------------------------------------------*/

struct ov_pulse_context_struct {

    uint32_t magic_number;

    char *playback_device;
    char *record_device;

    AudioIo playback;
    AudioIo record;

    pa_context *context;
    pa_threaded_mainloop *mainloop;

    pa_sample_spec sample_spec;
    size_t expected_buffer_size_bytes;

    size_t bytes_requested_by_pulse;

    ov_cache *buffer_cache;

    struct {

        size_t underflows;

        size_t no_data_to_write;

        double bytes_per_write_avg;
        double frames_per_write;

    } stats;
};

/*---------------------------------------------------------------------------*/

const char *MAGIC_NUMBER_STRING = "pULS";
#define MAGIC_NUMBER (*(uint32_t *)(MAGIC_NUMBER_STRING))

#define ENSURE_OV_PULSE_CONTEXT(c) OV_ASSERT(MAGIC_NUMBER == (c)->magic_number)

/******************************************************************************
 *                      PROTOTYPES FOR PRIVATE FUNCTIONS
 ******************************************************************************/

/* Wrapper around ov_buffer_free to match signature for ov_ringbuffer */
static void free_buffer(void *arg, void *buffer);

static ov_pulse_context *pulse_context_free(ov_pulse_context *context);
static void context_state_changed_cb(struct pa_context *c, void *userdata);

static void pulse_setup_stream(ov_pulse_context *context,
                               pa_stream **target_stream,
                               const char *device,
                               ov_stream_mode stream_mode);

static void stream_state_callback(pa_stream *stream, void *userdata);

static void stream_write_callback(pa_stream *stream,
                                  size_t nbytes,
                                  void *userdata);

static void stream_read_callback(pa_stream *stream,
                                 size_t nbytes,
                                 void *userdata);

static void stream_underflow_callback(pa_stream *stream, void *userdata);

/**
 * Try to collect as much as max_length_bytes from the queue queue
 * @param target_data_buffer If non-null used as data buffer to write the read
 * audio to, it must point to a mem region large enough to keep max_length_bytes
 * bytes. If null, a data buffer will be allocated.
 * @param bytes_avg If non-null, the average bytes collected will be written
 * there.
 * @param frames_avg If non-null, the average num of frames (chunks) will be
 * written there.
 * @return a Buffer containing the read bytes. If target_data_buffer was not
 * null, Buffer.data == target_data_buffer
 */
static Buffer get_next_chunk_from_queue(AudioIo *audio_io,
                                        ov_cache *buffer_cache,
                                        size_t max_length_bytes,
                                        uint8_t *target_data_buffer,
                                        double *bytes_avg,
                                        double *frames_avg);

static size_t ssmin(size_t s1, size_t s2);

static const char *get_stream_state(pa_stream *stream);

/**
 * Returns a buffer with a minimum size of min_size_bytes
 * The actual length of the buffer is set in buffer->nbytes.
 */
static Buffer *get_buffer(ov_cache *buffer_cache, size_t min_size_bytes);

static void audio_io_init(AudioIo *audio_io,
                          size_t lock_timeout_usecs,
                          size_t num_frames_to_buffer);

static void audio_io_free(AudioIo *audio_io);

static void adjust_latency(pa_buffer_attr *restrict bufattr,
                           const pa_sample_spec *restrict sample_spec,
                           uint32_t latency_usecs);

/******************************************************************************
 *                                 FUNCTIONS
 ******************************************************************************/

static void *free_buffer_void(void *vbuffer) {

    Buffer *buffer = vbuffer;

    if (0 == buffer) return 0;

    if (0 != buffer->data) {
        free(buffer->data);
    }

    free(buffer);
    buffer = 0;

    return buffer;
}

/*----------------------------------------------------------------------------*/

static Buffer *release_buffer(ov_cache *buffer_cache, Buffer *buffer) {

    if (0 == buffer) {
        goto finish;
    }

    buffer->nbytes = 0;

    if (0 == ov_cache_put(buffer_cache, buffer)) {

        buffer = 0;
        goto finish;
    }

    buffer = free_buffer_void(buffer);

finish:

    return buffer;
}

/*----------------------------------------------------------------------------*/

ov_pulse_context *ov_pulse_connect(ov_pulse_parameters parameters) {

    int errval = 0;
    ov_pulse_context *context = 0;

    const size_t usecs_per_sec = 1000 * 1000;

    char const *name = parameters.name;

    if (0 == name) name = DEFAULT_NAME;

    if (0 == parameters.sample_rate_hertz) {

        ov_log_error("Invalid (0) sample rate");
        goto error;
    }

    context = calloc(1, sizeof(struct ov_pulse_context_struct));
    context->magic_number = MAGIC_NUMBER;
    context->bytes_requested_by_pulse = 0;

    size_t frame_length_usecs = parameters.frame_length_usecs;

    if (0 == frame_length_usecs) {
        frame_length_usecs = 20000;
    }

    size_t frame_length_bytes = frame_length_usecs *
                                parameters.sample_rate_hertz / usecs_per_sec *
                                sizeof(uint16_t);

    context->expected_buffer_size_bytes = frame_length_bytes;

    if (0 != parameters.record_device) {
        context->record_device = strdup(parameters.record_device);
    }

    if (0 != parameters.playback_device) {
        context->playback_device = strdup(parameters.playback_device);
    }

    context->sample_spec = (pa_sample_spec){

        .format = SAMPLE_FORMAT,
        .rate = parameters.sample_rate_hertz,
        .channels = 1,

    };

    errval = pa_sample_spec_valid(&context->sample_spec);

    if (0 > errval) goto print_error;

    size_t usecs_to_buffer = parameters.usecs_to_buffer;

    /* Buffer for 2 secs */

    if (0 == usecs_to_buffer) {
        usecs_to_buffer = 2 * usecs_per_sec;
    }

    const size_t num_frames_to_buffer = usecs_to_buffer / frame_length_usecs;

    audio_io_init(&context->playback, usecs_per_sec, num_frames_to_buffer);
    audio_io_init(&context->record, usecs_per_sec, num_frames_to_buffer);

    context->buffer_cache = ov_cache_extend(0, 3 * num_frames_to_buffer / 2);

    context->mainloop = pa_threaded_mainloop_new();

    if (0 == context->mainloop) goto error;

    pa_mainloop_api *loop_api = pa_threaded_mainloop_get_api(context->mainloop);

    if (0 == loop_api) {

        goto print_context_error;
    }

    context->context = pa_context_new(loop_api, name);

    loop_api = 0;

    if (0 == context->context) goto print_context_error;

    errval = pa_context_connect(
        context->context, parameters.server, PA_CONTEXT_NOFLAGS, 0);

    if (0 > errval) goto print_error;

    errval = pa_threaded_mainloop_start(context->mainloop);
    if (0 > errval) goto print_error;

    pa_context_set_state_callback(
        context->context, context_state_changed_cb, context);

    return context;

print_context_error:

    errval = pa_context_errno(context->context);

print_error:

    ov_log_error("Error connecting to PA: %s", pa_strerror(errval));

error:

    if (0 != context) context = pulse_context_free(context);

    return 0;
}

/*---------------------------------------------------------------------------*/

ov_pulse_context *ov_pulse_disconnect(ov_pulse_context *context) {

    if (0 == context) goto finish;

    return pulse_context_free(context);

finish:

    return context;
}

/*---------------------------------------------------------------------------*/

bool ov_pulse_write(ov_pulse_context *context, ov_buffer const *input) {

    OV_ASSERT(context);

    ov_thread_lock *lock = context->playback.lock;

    if (lock) {

        if (!ov_thread_lock_try_lock(lock)) {
            ov_log_error("Could not lock on input queue");
            goto error;
        }
    }

    ov_ringbuffer *playback_queue = context->playback.queue;
    OV_ASSERT(playback_queue);

    Buffer *forward_buffer = get_buffer(context->buffer_cache, input->length);

    OV_ASSERT(forward_buffer);
    OV_ASSERT(forward_buffer->data);
    OV_ASSERT(forward_buffer->capacity_bytes >= input->length);

    memcpy(forward_buffer->data, input->start, input->length);
    forward_buffer->nbytes = input->length;

    if (!playback_queue->insert(playback_queue, forward_buffer)) {

        ov_log_error("Could not enqueue audio data");
        release_buffer(context->buffer_cache, forward_buffer);
    }

    if (lock) {

        if (!ov_thread_lock_unlock(lock)) {
            OV_ASSERT(!" WE ARE FUCKED!!!");
        }
    }

    if ((0 < context->bytes_requested_by_pulse) &&
        (0 != context->playback.stream)) {

        pa_threaded_mainloop_lock(context->mainloop);

        if (PA_STREAM_READY == pa_stream_get_state(context->playback.stream)) {

            stream_write_callback(context->playback.stream,
                                  context->bytes_requested_by_pulse,
                                  context);
        }

        pa_threaded_mainloop_unlock(context->mainloop);
    }

    return true;

error:

    return false;
}

/*---------------------------------------------------------------------------*/

size_t ov_pulse_read(ov_pulse_context *context,
                     uint8_t *buffer,
                     size_t nbytes) {

    OV_ASSERT(context);

    Buffer recorded = get_next_chunk_from_queue(
        &context->record, context->buffer_cache, nbytes, buffer, 0, 0);

    return recorded.nbytes;
}

/*---------------------------------------------------------------------------*/

ov_json_value *ov_pulse_get_state(ov_pulse_context *restrict context) {

    ov_json_value *pulse_json = 0;
    ov_json_value *stream_json = 0;

    if (0 == context) {
        ov_log_error("Called without context");
        goto error;
    }

    stream_json = ov_json_object();

    ov_json_object_set(stream_json,
                       OV_KEY_UNDERFLOWS,
                       ov_json_number(context->stats.underflows));

    ov_json_object_set(stream_json,
                       OV_KEY_NO_DATA,
                       ov_json_number(context->stats.no_data_to_write));

    ov_json_object_set(stream_json,
                       OV_KEY_FRAMES_PER_WRITE,
                       ov_json_number(context->stats.frames_per_write));

    ov_json_object_set(
        stream_json,
        OV_KEY_STREAM_STATE,
        ov_json_string(get_stream_state(context->playback.stream)));

    pulse_json = ov_json_object();

    ov_json_object_set(pulse_json, OV_KEY_PLAYBACK, stream_json);

    stream_json = ov_json_object();

    ov_json_object_set(
        stream_json,
        OV_KEY_STREAM_STATE,
        ov_json_string(get_stream_state(context->record.stream)));

    ov_json_object_set(pulse_json, OV_KEY_RECORD, stream_json);

    return pulse_json;

error:

    if (0 != pulse_json) {

        pulse_json = pulse_json->free(pulse_json);
    }

    if (0 != stream_json) {

        stream_json = stream_json->free(stream_json);
    }

    return 0;
}

/******************************************************************************
 *                             INTERNAL FUNCTIONS
 ******************************************************************************/

static void free_buffer(void *arg, void *buffer) {

    UNUSED(arg);

    if (0 == buffer) {
        return;
    }

    Buffer *b = buffer;

    if (0 == b) {
        return;
    }

    if (0 != b->data) {
        free(b->data);
    }

    free(b);
}

/*---------------------------------------------------------------------------*/

/*
 * Assumes that mainloop stopped etc...
 */
static ov_pulse_context *pulse_context_free(ov_pulse_context *context) {

    if (0 == context) goto finish;

    /* Free PulseAudio resources */
    if (0 != context->mainloop) pa_threaded_mainloop_lock(context->mainloop);

    pa_stream *stream = context->record.stream;
    context->record.stream = 0;

    if (0 != stream) {
        pa_stream_disconnect(stream);
        pa_stream_unref(stream);
    }

    stream = context->playback.stream;
    context->playback.stream = 0;

    if (0 != stream) {
        pa_stream_disconnect(stream);
        pa_stream_unref(stream);
    }

    if (0 != context->context) {
        pa_context_disconnect(context->context);
        pa_context_unref(context->context);
    }
    context->context = 0;

    if (0 != context->mainloop) {
        pa_threaded_mainloop_unlock(context->mainloop);
        pa_threaded_mainloop_stop(context->mainloop);
        pa_threaded_mainloop_free(context->mainloop);
    }
    context->mainloop = 0;

    audio_io_free(&context->playback);
    audio_io_free(&context->record);

    if (0 != context->buffer_cache) {

        ov_cache_free(context->buffer_cache, 0, free_buffer_void);
    }

    free(context);

    context = 0;

finish:

    return context;
}

/*---------------------------------------------------------------------------*/

static void context_state_changed_cb(struct pa_context *c, void *userdata) {

    if (0 == c) {

        ov_log_error("pa_context_state_change: Got 0 pointer");
        return;
    }

    if (0 == userdata) {

        ov_log_error("pa_context_state_change: Got 0 pointer");
        return;
    }

    ov_pulse_context *context = userdata;
    ENSURE_OV_PULSE_CONTEXT(context);

    switch (pa_context_get_state(c)) {

        case PA_CONTEXT_UNCONNECTED:
            ov_log_info("UNCONNECTED");
            break;

        case PA_CONTEXT_CONNECTING:
            ov_log_info("CONNECTING");
            break;

        case PA_CONTEXT_AUTHORIZING:
            ov_log_info("AUTHORIZING");
            break;

        case PA_CONTEXT_SETTING_NAME:
            ov_log_info("SETTING_NAME");
            break;

        case PA_CONTEXT_READY:

            ov_log_info("READY");

            pulse_setup_stream(userdata,
                               &context->playback.stream,
                               context->playback_device,
                               PLAYBACK);

            pulse_setup_stream(userdata,
                               &context->record.stream,
                               context->record_device,
                               RECORD);

            break;

        case PA_CONTEXT_FAILED:
            ov_log_error("FAILED");
            break;

        case PA_CONTEXT_TERMINATED:
            ov_log_error("TERMINATED");
            break;

        default:
            OV_ASSERT(!"NEVER TO HAPPEN!");
    }

    return;
}

/*----------------------------------------------------------------------------*/

static bool connect_playback_stream(pa_stream *stream,
                                    ov_pulse_context *context,
                                    const char *device) {

    OV_ASSERT(0 != stream);
    OV_ASSERT(0 != context);

    pa_cvolume cv;

    pa_cvolume_set(&cv, 1, PA_VOLUME_NORM);

    pa_buffer_attr bufattr = {0};

    adjust_latency(&bufattr, &context->sample_spec, 20 * 1000);

    pa_stream_set_write_callback(stream, stream_write_callback, context);

    int retval = pa_stream_connect_playback(
        stream, device, &bufattr, PA_STREAM_ADJUST_LATENCY, &cv, 0);

    if (0 > retval) {

        ov_log_error(
            "Could not connect for playback: "
            "%s",
            pa_strerror(retval));

        goto error;
    }

    return true;

error:

    return false;
}

/*----------------------------------------------------------------------------*/

static bool connect_record_stream(pa_stream *stream,
                                  ov_pulse_context *context,
                                  const char *device) {

    OV_ASSERT(0 != stream);
    OV_ASSERT(0 != context);

    pa_cvolume cv;

    pa_cvolume_set(&cv, 1, PA_VOLUME_NORM);

    pa_buffer_attr bufattr = {0};

    adjust_latency(&bufattr, &context->sample_spec, 10 * 1000);

    pa_stream_set_read_callback(stream, stream_read_callback, context);

    int retval = pa_stream_connect_record(
        stream,
        device,
        &bufattr,
        PA_STREAM_START_UNMUTED | PA_STREAM_ADJUST_LATENCY);

    if (0 > retval) {

        ov_log_error("Could not connect for record: %s", pa_strerror(retval));

        goto error;
    }

    return true;

error:

    return false;
}

/*---------------------------------------------------------------------------*/

static void pulse_setup_stream(ov_pulse_context *context,
                               pa_stream **target_stream,
                               const char *device,
                               ov_stream_mode stream_mode) {

    pa_stream *stream = 0;

    if (0 == context) {

        ov_log_error("Called with 0 pointer");
    }

    if (0 == target_stream) {

        ov_log_error("Called with 0 pointer");
        goto error;
    }

    const char *stream_name = DEFAULT_NAME;

    bool (*connect_stream)(pa_stream *, ov_pulse_context *, const char *) = 0;

    switch (stream_mode) {

        case PLAYBACK:

            stream_name = "PLAYBACK";
            connect_stream = connect_playback_stream;

            break;

        case RECORD:

            stream_name = "RECORD";
            connect_stream = connect_record_stream;

            break;

        default:

            OV_ASSERT(!"NEVER TO HAPPEN!");
    };

    OV_ASSERT(0 != connect_stream);

    stream =
        pa_stream_new(context->context, stream_name, &context->sample_spec, 0);

    if (0 == stream) {

        ov_log_error("pa_stream_new() failed: %s",
                     pa_strerror(pa_context_errno(context->context)));

        goto error;
    }

    pa_stream_set_state_callback(stream, stream_state_callback, 0);

    if (!connect_stream(stream, context, device)) {
        goto error;
    }

    pa_stream_set_underflow_callback(
        stream, stream_underflow_callback, context);

    *target_stream = stream;
    stream = 0;

error:

    if (0 != stream) {

        pa_stream_unref(stream);
    }

    stream = 0;

    return;
}

/*---------------------------------------------------------------------------*/

static void stream_state_callback(pa_stream *stream, void *userdata) {

    UNUSED(userdata);

    OV_ASSERT(stream);
    const pa_buffer_attr *a;
    char cmt[PA_CHANNEL_MAP_SNPRINT_MAX], sst[PA_SAMPLE_SPEC_SNPRINT_MAX];

    /* State changed for stream */
    switch (pa_stream_get_state(stream)) {
        case PA_STREAM_CREATING:
        case PA_STREAM_TERMINATED:
            break;

        case PA_STREAM_READY:

            if (!(a = pa_stream_get_buffer_attr(stream)))

                ov_log_error(
                    "pa_stream_get_buffer_attr() "
                    "failed: %s",
                    pa_strerror(
                        pa_context_errno(pa_stream_get_context(stream))));
            else {

                ov_log_info(
                    "Buffer metrics: "
                    "maxlength=%u, tlength=%u, "
                    "prebuf=%u, "
                    "minreq=%u",
                    a->maxlength,
                    a->tlength,
                    a->prebuf,
                    a->minreq);

                ov_log_info(
                    "Using sample spec '%s', channel "
                    "map '%s'.",
                    pa_sample_spec_snprint(
                        sst, sizeof(sst), pa_stream_get_sample_spec(stream)),
                    pa_channel_map_snprint(
                        cmt, sizeof(cmt), pa_stream_get_channel_map(stream)));

                ov_log_info(
                    "Connected to device %s (%u, "
                    "%ssuspended).",
                    pa_stream_get_device_name(stream),
                    pa_stream_get_device_index(stream),
                    pa_stream_is_suspended(stream) ? "" : "not ");
            }

            break;

        case PA_STREAM_FAILED:
        default:

            ov_log_error(
                "Stream error: %s",
                pa_strerror(pa_context_errno(pa_stream_get_context(stream))));

            OV_ASSERT(!"NEVER TO HAPPEN!!!!");
    }
}

/*---------------------------------------------------------------------------*/

static void stream_write_callback(pa_stream *stream,
                                  size_t nbytes,
                                  void *userdata) {

    size_t bytes_written = 0;

    if (0 == userdata) {

        ov_log_error("stream_write_callback: Called with 0 pointer");
        return;
    }

    ov_pulse_context *context = userdata;

    ENSURE_OV_PULSE_CONTEXT(context);

    Buffer pcm_data =
        get_next_chunk_from_queue(&context->playback,
                                  context->buffer_cache,
                                  nbytes,
                                  0,
                                  &context->stats.bytes_per_write_avg,
                                  &context->stats.frames_per_write);

    if (0 == pcm_data.data) goto no_data_yet;

    int retval = pa_stream_write(
        stream, pcm_data.data, pcm_data.nbytes, free, 0, PA_SEEK_RELATIVE);

    pcm_data.data = 0;

    if (0 > retval) {

        ov_log_error(
            "Could not write buffer to PA server: %s", pa_strerror(retval));

        bytes_written = 0;

        goto error;
    }

    bytes_written = pcm_data.nbytes;

    goto finish;

no_data_yet:

    ++context->stats.no_data_to_write;

error:
finish:

    context->bytes_requested_by_pulse = nbytes - bytes_written;

    return;
}

/*---------------------------------------------------------------------------*/

static void stream_read_callback(pa_stream *stream,
                                 size_t nbytes,
                                 void *userdata) {

    OV_ASSERT(0 != stream);
    OV_ASSERT(0 != userdata);
    OV_ASSERT(0 < nbytes);

    ov_thread_lock *lock = 0;

    /* Operational checks */
    if (0 == stream) {

        ov_log_error("Invalid stream (0 pointer)");
        goto error;
    }

    if (0 == userdata) {

        ov_log_error("Invalid stream (0 pointer)");
        goto error;
    }

    ov_pulse_context *context = userdata;
    ENSURE_OV_PULSE_CONTEXT(context);

    int retval = 0;
    const void *data = 0;

    size_t nbytes_ready = 0;
    retval = pa_stream_peek(stream, &data, &nbytes_ready);

    if (0 != retval) {

        ov_log_error("Peeking from stream failed: %s",
                     pa_strerror(pa_context_errno(context->context)));
        goto error;
    }

    lock = context->record.lock;

    if (lock) {

        if (!ov_thread_lock_try_lock(lock)) {
            ov_log_error("Could not lock on input queue");

            lock = 0;
            goto error;
        }
    }

    Buffer *buffer = get_buffer(context->buffer_cache, nbytes_ready);

    if (0 == buffer) {
        OV_ASSERT(!"WE ARE FUCKING OUT-OF-MEM");
    }

    memcpy(buffer->data, data, nbytes_ready);
    buffer->nbytes = nbytes_ready;

    ov_ringbuffer *queue = context->record.queue;
    OV_ASSERT(queue);

    if (!queue->insert(queue, buffer)) {

        ov_log_error("Could not enqueue buffer from recording");
        buffer = release_buffer(context->buffer_cache, buffer);
    }

    buffer = 0;

    if (lock) {

        if (!ov_thread_lock_unlock(lock)) {
            OV_ASSERT(!" WE ARE FUCKED!!!");
        }
        lock = 0;
    }

    /* Remove data from stream */
    retval = pa_stream_drop(stream);

    if (0 != retval) {

        ov_log_error("Peeking from stream failed: %s",
                     pa_strerror(pa_context_errno(context->context)));
        goto error;
    }

error:

    if (lock) {

        if (!ov_thread_lock_unlock(lock)) {
            OV_ASSERT(!" WE ARE FUCKED!!!");
        }
    }
}

/*---------------------------------------------------------------------------*/

static void stream_underflow_callback(pa_stream *stream, void *userdata) {

    UNUSED(stream);

    ov_pulse_context *context = userdata;
    ENSURE_OV_PULSE_CONTEXT(context);

    ++context->stats.underflows;
}

/*---------------------------------------------------------------------------*/

static Buffer get_next_chunk_from_queue(AudioIo *audio_io,
                                        ov_cache *buffer_cache,
                                        size_t nbytes_requested,
                                        uint8_t *target_data_buffer,
                                        double *bytes_avg,
                                        double *frames_avg) {

    Buffer buffer = {

        .data = target_data_buffer,
        .capacity_bytes = nbytes_requested,
        .nbytes = 0,

    };

    if (0 == audio_io) {
        goto error;
    }

    if (0 == nbytes_requested) {
        goto error;
    }

    ov_ringbuffer *queue = audio_io->queue;
    ov_thread_lock *lock = audio_io->lock;

    Buffer *current = 0;

    if (0 == buffer.data) {
        buffer.data = calloc(1, nbytes_requested);
    }

    if (0 == queue) {
        goto error;
    }

    if (lock) {

        if (!ov_thread_lock_try_lock(lock)) {
            ov_log_error("Could not lock on input queue");
            goto error;
        }
    }

    uint8_t *write_pointer = buffer.data;
    size_t num_frames_processed = 0;

    current = audio_io->current;
    audio_io->current = 0;

    while (buffer.nbytes < nbytes_requested) {

        OV_ASSERT(0 == audio_io->current);

        if (0 == current) {
            current = queue->pop(queue);
        }

        if (0 == current) {
            // No more data available
            break;
        }

        size_t available_bytes = buffer.capacity_bytes - buffer.nbytes;

        size_t bytes_to_write = ssmin(available_bytes, current->nbytes);

        memcpy(write_pointer, current->data, bytes_to_write);

        write_pointer += bytes_to_write;
        buffer.nbytes += bytes_to_write;

        ++num_frames_processed;

        if (current->nbytes > bytes_to_write) {

            size_t remaining_bytes = current->nbytes - bytes_to_write;

            /* Store remainder to use on next possible write */
            Buffer *remainder = get_buffer(buffer_cache, remaining_bytes);

            remainder->nbytes = remaining_bytes;

            memcpy(remainder->data,
                   current->data + bytes_to_write,
                   remaining_bytes);

            current = release_buffer(buffer_cache, current);

            audio_io->current = remainder;

            break;
        }

        OV_ASSERT(0 == audio_io->current);

        current = release_buffer(buffer_cache, current);
    }

    OV_ASSERT((0 == audio_io->current) || (nbytes_requested >= buffer.nbytes));

    if (buffer.nbytes < nbytes_requested) {

        /* Not enough bytes available */
        ov_log_error(
            "Buffer underflow, requested %zu bytes, only got "
            "%zu\n",
            nbytes_requested,
            buffer.nbytes);

        audio_io->current = get_buffer(buffer_cache, buffer.nbytes);
        memcpy(audio_io->current->data, buffer.data, buffer.nbytes);
        audio_io->current->nbytes = buffer.nbytes;
        buffer.nbytes = 0;

        goto error_unlock;
    }

    if (0 != bytes_avg) {
        double d_bytes_avg = *bytes_avg;
        UPDATE_AVERAGE(d_bytes_avg, buffer.nbytes);
        *bytes_avg = d_bytes_avg;
    }

    if (0 != frames_avg) {
        double d_frames_avg = *frames_avg;
        UPDATE_AVERAGE(d_frames_avg, num_frames_processed);
        *frames_avg = d_frames_avg;
    }

error_unlock:

    if (lock) {

        if (!ov_thread_lock_unlock(lock)) {
            OV_ASSERT(!" WE ARE FUCKED!!!");
        }
    }

error:

    return buffer;
}

/*---------------------------------------------------------------------------*/

static size_t ssmin(size_t s1, size_t s2) { return (s1 < s2) ? s1 : s2; }

/*---------------------------------------------------------------------------*/

static const char *get_stream_state(pa_stream *stream) {

    if (0 == stream) {
        ov_log_error("Called without context");
        goto error;
    }

    switch (pa_stream_get_state(stream)) {

        case PA_STREAM_CREATING:
            return "CREATING";

        case PA_STREAM_TERMINATED:
            return "TERMINATED";

        case PA_STREAM_READY:
            return "READY";

        case PA_STREAM_FAILED:
        default:

            return "FAILED";
    };

error:

    return "FAILED";
}

/*---------------------------------------------------------------------------*/

static Buffer *get_buffer(ov_cache *buffer_cache, size_t min_size_bytes) {

    OV_ASSERT(buffer_cache);

    Buffer *buffer = ov_cache_get(buffer_cache);

    if (0 == buffer) {
        buffer = calloc(1, sizeof(Buffer));
        buffer->capacity_bytes = 0;
        buffer->data = 0;
    }

    OV_ASSERT(buffer);

    if (min_size_bytes > buffer->capacity_bytes) {

        free(buffer->data);
        buffer->data = 0;
    }

    if (0 == buffer->data) {
        buffer->data = calloc(1, min_size_bytes);
        buffer->capacity_bytes = min_size_bytes;
    }

    buffer->nbytes = 0;

    OV_ASSERT(buffer->capacity_bytes >= min_size_bytes);
    OV_ASSERT(buffer->data);
    OV_ASSERT(0 == buffer->nbytes);

    return buffer;
}

/*---------------------------------------------------------------------------*/

static void audio_io_init(AudioIo *audio_io,
                          size_t lock_timeout_usecs,
                          size_t num_frames_to_buffer) {

    audio_io->lock = calloc(1, sizeof(ov_thread_lock));
    ov_thread_lock_init(audio_io->lock, lock_timeout_usecs);

    audio_io->queue =
        ov_ringbuffer_create(num_frames_to_buffer, free_buffer, 0);
}

/*---------------------------------------------------------------------------*/

static void audio_io_free(AudioIo *audio_io) {

    OV_ASSERT(audio_io);
    OV_ASSERT(0 == audio_io->stream);

    ov_thread_lock *lock = 0;
    Buffer *buffer = 0;
    ov_ringbuffer *queue = 0;

    lock = audio_io->lock;

    if (lock) {

        if (!ov_thread_lock_try_lock(lock)) {
            ov_log_error("Could not lock on playback queue");
        }
    }
    audio_io->lock = 0;

    buffer = audio_io->current;
    audio_io->current = 0;

    queue = audio_io->queue;
    audio_io->queue = 0;

    if (0 != buffer) {
        free_buffer(0, buffer);
    }
    buffer = 0;

    if (lock) {

        if (!ov_thread_lock_unlock(lock)) {
            OV_ASSERT(!" WE ARE FUCKED!!!");
        }

        ov_thread_lock_clear(lock);
        free(lock);
    }
    lock = 0;

    if (0 != queue) {
        queue = queue->free(queue);
    }
}

/*---------------------------------------------------------------------------*/

static void adjust_latency(pa_buffer_attr *restrict bufattr,
                           const pa_sample_spec *restrict sample_spec,
                           uint32_t latency_usecs) {

    OV_ASSERT(bufattr);
    OV_ASSERT(sample_spec);

    memset(bufattr, 0, sizeof(pa_buffer_attr));

    bufattr->maxlength = (uint32_t)-1;
    bufattr->prebuf = (uint32_t)-1;
    bufattr->tlength = pa_usec_to_bytes(latency_usecs, sample_spec);
    bufattr->fragsize = bufattr->tlength;
}

/*---------------------------------------------------------------------------*/
