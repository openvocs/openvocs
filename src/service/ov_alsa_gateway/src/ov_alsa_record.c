/***
        ------------------------------------------------------------------------

        Copyright (c) 2023 German Aerospace Center DLR e.V. (GSOC)

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

#include "ov_alsa_record.h"
#include <netdb.h>
#include <ov_arch/ov_arch_math.h>
#include <ov_base/ov_chunker.h>
#include <ov_base/ov_convert.h>
#include <ov_base/ov_counter.h>
#include <ov_base/ov_rtp_app.h>
#include <ov_base/ov_string.h>
#include <ov_base/ov_time.h>
#include <ov_codec/ov_codec_factory.h>
#include <ov_os_linux/ov_alsa.h>
#include <ov_pcm16s/ov_pcm16_mod.h>
#include <ov_pcm_gen/ov_pcm_gen.h>

/*----------------------------------------------------------------------------*/

struct ov_alsa_record_struct {

    uint32_t magic_bytes;

    struct rtp_stream {

        uint32_t ssrc;

        int sd;

        struct {
            struct sockaddr_storage addr;
            socklen_t len;
        } dest;

        ov_codec *codec;
        uint64_t frame_length_ms;
        uint64_t frame_length_samples;

        uint16_t sequence_number;
        uint32_t timestamp;
        uint8_t encoded_buffer[OV_MAX_FRAME_LENGTH_BYTES];

    } rtp;

    char *alsa_device;
    ov_alsa *record;

    uint64_t samplerate_hz;
    uint16_t comfort_noise_amplitude_max;
    double volume;

    struct {
        pthread_t tid;
        bool keep_running;
        bool is_running;
    } thread;

    ov_chunker *sample_buffer;

    struct {

        ov_counter rtp_sent;

    } counter;
};

#define MAGIC_BYTES 0xa32161bc

/*----------------------------------------------------------------------------*/

static ov_alsa_record const *as_record(void const *ptr) {

    ov_alsa_record const *record = ptr;

    if (ov_ptr_valid(ptr, "Invalid ALSA record struct - 0 pointer") &&
        ov_cond_valid(record->magic_bytes,
                      "Invalid ALSA record struct - magic bytes don't match")) {

        return record;

    } else {
        return 0;
    }
}

/*----------------------------------------------------------------------------*/

static ov_alsa_record *as_record_mut(void *ptr) {
    return (ov_alsa_record *)as_record(ptr);
}

/*----------------------------------------------------------------------------*/

static bool init_alsa_record(ov_alsa_record *self, ov_alsa_record_config cfg) {

    ov_codec *codec = ov_codec_factory_get_codec_from_json(
        0, cfg.rtp_stream.codec_config, cfg.rtp_stream.ssid);

    ov_chunker *chunker = ov_chunker_create();

    if (ov_ptr_valid(self,
                     "Cannot initialize alsa record object - 0 pointer") &&
        ov_ptr_valid(codec,
                     "Cannot create alsa record - could not create codec") &&
        ov_ptr_valid(chunker,
                     "Cannot create alsa record - could not create chunker") &&
        ov_socket_fill_sockaddr_storage(&self->rtp.dest.addr, IPV4,
                                        cfg.rtp_stream.target.host,
                                        cfg.rtp_stream.target.port)) {

        self->rtp.ssrc = cfg.rtp_stream.ssid;
        self->rtp.dest.len = sizeof(struct sockaddr_in);
        self->rtp.frame_length_ms = cfg.rtp_stream.frame_length_ms;

        self->rtp.codec = codec;

        self->sample_buffer = chunker;

        self->samplerate_hz =
            OV_OR_DEFAULT(cfg.samplerate_hz, OV_DEFAULT_SAMPLERATE);

        self->volume = OV_OR_DEFAULT(cfg.volume, 1.0);

        self->alsa_device = ov_string_dup(cfg.alsa_device);

        self->comfort_noise_amplitude_max = cfg.comfort_noise.max_amplitude;

        self->rtp.frame_length_samples = ov_convert_msecs_to_samples(
            self->rtp.frame_length_ms, self->samplerate_hz);

        size_t samples_period = (size_t)self->rtp.frame_length_samples;

        self->record = ov_alsa_create(cfg.alsa_device, OV_DEFAULT_SAMPLERATE,
                                      &samples_period, CAPTURE);

        return (0 != self->record);

    } else {

        codec = ov_codec_free(codec);
        chunker = ov_chunker_free(chunker);

        return false;
    }
}

/*----------------------------------------------------------------------------*/

static bool send_as_rtp(struct rtp_stream *rtp, uint8_t const *pcm,
                        size_t num_octets) {

    int32_t octets_encoded = -1;

    if (ov_ptr_valid(rtp, "Cannot send rtp - invalid ov_alsa_record object") &&
        ov_ptr_valid(pcm, "Cannot send rtp - no data to send")) {

        octets_encoded =
            ov_codec_encode(rtp->codec, pcm, num_octets, rtp->encoded_buffer,
                            sizeof(rtp->encoded_buffer));
    }

    if (ov_cond_valid(octets_encoded >= 0, "Could not encode PCM")) {

        ov_rtp_frame_expansion rtp_exp = {

            .version = RTP_VERSION_2,
            .ssrc = rtp->ssrc,
            .sequence_number = rtp->sequence_number,
            .timestamp = rtp->timestamp,
            .payload.data = rtp->encoded_buffer,
            .payload.length = (size_t)octets_encoded,

        };

        ov_rtp_frame *frame = ov_rtp_frame_encode(&rtp_exp);

        ssize_t sent_bytes = -1;

        if (ov_ptr_valid(frame, "Could not encode RTP frame")) {

            ov_log_debug("Sending frame SSRC: %" PRIu32 "  - SEQ %" PRIu16,
                         frame->expanded.ssrc, frame->expanded.sequence_number);

            sent_bytes =
                sendto(rtp->sd, frame->bytes.data, frame->bytes.length, 0,
                       (struct sockaddr *)&rtp->dest.addr, rtp->dest.len);

            ++rtp->sequence_number;
            rtp->timestamp += num_octets / OV_DEFAULT_OCTETS_PER_SAMPLE;
        }

        frame = ov_rtp_frame_free(frame);

        if (!(0 < sent_bytes)) {
            ov_log_error("Could not send RTP frame");
            return false;
        } else {
            return true;
        }

    } else {

        return false;
    }
}

/*----------------------------------------------------------------------------*/

static void *cb_alsa_record(void *arg) {

    ov_alsa_record *self = as_record_mut(arg);

    if (ov_ptr_valid(self, "Cannot record - invalid ALSA record object") &&
        ov_cond_valid(!self->thread.is_running,
                      "Cannot record - record thread already running")) {

        self->thread.is_running = true;

        size_t octets_for_one_frame = 2 * self->rtp.frame_length_samples;

        ov_chunker *chunker = self->sample_buffer;

        ov_buffer *pcm = ov_buffer_create(
            OV_MAX(2 * ov_alsa_get_samples_per_period(self->record),
                   octets_for_one_frame));

        // First frame will not get sufficient samples from card - does not
        // matter, just send comfort noise instead for the first 20ms
        while (self->thread.keep_running) {

            pcm->length = 2 * ov_alsa_capture_period(self->record,
                                                     (int16_t *)pcm->start, 0);

            ov_chunker_add(chunker, pcm);

            if (ov_chunker_next_chunk_raw(chunker, octets_for_one_frame,
                                          pcm->start) &&
                send_as_rtp(&self->rtp, pcm->start, octets_for_one_frame)) {

                ov_counter_increase(self->counter.rtp_sent, 1);
            }

            pcm->length = 0;
        };

        pcm = ov_free(pcm);
    }

    self->thread.is_running = false;

    return 0;
}

/*----------------------------------------------------------------------------*/

static ov_alsa_record *create_alsa_record(const ov_alsa_record_config cfg,
                                          int sd_for_sending, ov_result *res) {

    ov_alsa_record *self = calloc(1, sizeof(ov_alsa_record));
    self->magic_bytes = MAGIC_BYTES;
    self->thread.keep_running = true;
    self->rtp.sd = sd_for_sending;

    if (!ov_cond_valid(-1 < sd_for_sending,
                       "Cannot create ov_alsa_record - invalid socket "
                       "descriptor")) {

        ov_result_set(res, OV_ERROR_BAD_ARG, "Invalid socket descriptor");
        self = ov_alsa_record_free(self);

    } else if (!init_alsa_record(self, cfg)) {

        self = ov_alsa_record_free(self);
        ov_result_set(res, OV_ERROR_BAD_ARG, "Invalid configuration");

    } else if (0 !=
               pthread_create(&self->thread.tid, 0, cb_alsa_record, self)) {

        ov_log_error("Could not create ALSA record thread");
        ov_result_set(res, OV_ERROR_INTERNAL_SERVER,
                      "Could not create ALSA record thread");
        self = ov_alsa_record_free(self);

    } else {
        ov_log_info("Start recording from %s",
                    ov_string_sanitize(cfg.alsa_device));
    }

    return self;
}

/*----------------------------------------------------------------------------*/

ov_alsa_record *ov_alsa_record_create(const ov_alsa_record_config cfg,
                                      int sd_for_sending, ov_result *res) {

    if ((0 != cfg.mixer_element) &&
        (!ov_alsa_set_volume(
            cfg.alsa_device, cfg.mixer_element, CAPTURE,
            OV_OR_DEFAULT(cfg.volume, OV_ALSA_RECORD_DEFAULT_VOLUME)))) {

        ov_result_set(res, OV_ERROR_INTERNAL_SERVER,
                      "Cannot set ALSA volume for capture");

        return 0;

    } else {

        return create_alsa_record(cfg, sd_for_sending, res);
    }
}

/*----------------------------------------------------------------------------*/

ov_alsa_record *ov_alsa_record_free(ov_alsa_record *self) {

    if (0 != self) {

        self->thread.keep_running = false;

        void *retval = 0;

        if (self->thread.is_running &&
            ov_cond_valid(0 == pthread_join(self->thread.tid, &retval),
                          "Could not stop ALSA record thread")) {

            ov_log_info("Stop recording from %s",
                        ov_string_sanitize(self->alsa_device));

            self->alsa_device = ov_free(self->alsa_device);
            self->rtp.codec = ov_codec_free(self->rtp.codec);
            self->record = ov_alsa_free(self->record);
            self->sample_buffer = ov_chunker_free(self->sample_buffer);

            self = ov_free(self);
        }

    } else {

        ov_log_info("Stop recording for unused device");
    }

    return self;
}

/*----------------------------------------------------------------------------*/

static bool counter_to_json(ov_json_value *target, char const *key,
                            ov_counter counter) {

    ov_json_value *jcounter = ov_counter_to_json(counter);

    if (ov_json_object_set(target, key, jcounter)) {
        return true;
    } else {

        jcounter = ov_json_value_free(jcounter);
        return false;
    }
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_alsa_record_get_counters(ov_alsa_record *self) {

    ov_json_value *jval = ov_json_object();
    ov_alsa_counters counters = {0};

    if ((!ov_ptr_valid(self,
                       "Cannot turn ALSA record counters to JSON: Invalid ALSA "
                       "record")) ||
        (!ov_alsa_get_counters(self->record, &counters)) ||
        (!counter_to_json(jval, "rtp_frames_sent", self->counter.rtp_sent)) ||
        (!counter_to_json(jval, "alsa_underruns", counters.underruns))) {

        jval = ov_json_value_free(jval);
    }

    return jval;
}

/*----------------------------------------------------------------------------*/
