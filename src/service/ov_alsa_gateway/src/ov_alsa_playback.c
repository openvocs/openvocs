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

        ALSA uses `frame` to denote the data played on a single time unit.
        An ALSA frame consists of 1 sample if MONO, 2 samples if STEREO and so
        forth.

        This might likely be confused with RTP frame.

        We restrict us to mono here and use `sample` instead for an ALSA frame.

        ------------------------------------------------------------------------
*/
#include "ov_alsa_playback.h"
#include <ov_arch/ov_arch_math.h>
#include <ov_base/ov_chunker.h>
#include <ov_base/ov_constants.h>
#include <ov_base/ov_convert.h>
#include <ov_base/ov_string.h>
#include <ov_base/ov_time.h>
#include <ov_os_linux/ov_alsa.h>
#include <ov_pcm_gen/ov_pcm_gen.h>
#include <pthread.h>
#include <stdbool.h>

/*----------------------------------------------------------------------------*/

static ov_buffer *create_comfort_noise(uint64_t msecs, uint16_t max_amplitude) {

    ov_pcm_gen_config cfg = {
        .frame_length_usecs = 1000 * msecs,
        .sample_rate_hertz = OV_DEFAULT_SAMPLERATE,
    };

    ov_pcm_gen_white_noise wnoise = {
        .max_amplitude = max_amplitude,
    };

    ov_pcm_gen *generator = ov_pcm_gen_create(OV_WHITE_NOISE, cfg, &wnoise);
    ov_buffer *buf = ov_pcm_gen_generate_frame(generator);

    generator = ov_pcm_gen_free(generator);

    return buf;
}

/*----------------------------------------------------------------------------*/

#define MAGIC_BYTES 0xA090B010

struct ov_alsa_playback {

    uint32_t magic_bytes;

    uint16_t msecs_to_buffer_ahead;
    size_t alsa_buffer_size_samples;
    size_t alsa_samples_per_period;
    size_t alsa_octets_per_period;

    bool buffer_after_interrupt;

    ov_buffer *alsa_period_buffer;

    ov_buffer *comfort_noise;

    ov_alsa *alsa;

    struct {

        ov_counter periods_played;

    } counter;

    int logging_fd;
};

/*----------------------------------------------------------------------------*/

static ov_alsa_playback const *as_playback(void const *ptr) {

    ov_alsa_playback const *apt = ptr;

    if (ov_ptr_valid(apt, "Cannot cast to ALSA PLAYBACK, pointer is invalid") &&
        ov_cond_valid(MAGIC_BYTES == apt->magic_bytes,
                      "Cannot cast to ALSA PLAYBACK: Invalid magic bytes")) {

        return apt;

    } else {

        return 0;
    }
}

/*----------------------------------------------------------------------------*/

static ov_alsa_playback_play_result play_pcm(ov_alsa *alsa, uint8_t *pcm) {

    ov_result res = {0};

    ov_alsa_playback_play_result alsa_res = ALSA_REPLAY_OK;

    if (ov_ptr_valid(alsa, "Cannot play buffer: Invalid ALSA object") &&
        (!ov_alsa_play_period(alsa, (int16_t *)pcm, &res))) {

        // TODO: 2024-12-16 Check if this is required
        ov_alsa_reset(alsa);

        ov_log_error("Could not replay PCM data: %s",
                     ov_string_sanitize(ov_result_get_message(res)));

        if (OV_ERROR_AUDIO_UNDERRUN == res.error_code) {

            alsa_res = ALSA_REPLAY_INSUFFICIENT;

        } else {

            alsa_res = ALSA_REPLAY_FAILED;
        }
    }

    ov_result_clear(&res);

    return alsa_res;
}

/*----------------------------------------------------------------------------*/

static ov_alsa_playback_play_result
feed_alsa_buffer_next_period(ov_chunker *chunker, ov_alsa *alsa,
                             uint8_t *buffer, size_t octets_per_period) {

    if (ov_ptr_valid(buffer, "Cannot play period - invalid buffer")) {

        if (ov_chunker_next_chunk_raw(chunker, octets_per_period, buffer)) {

            return play_pcm(alsa, buffer) ? ALSA_REPLAY_OK : ALSA_REPLAY_FAILED;

        } else {

            return ALSA_REPLAY_INSUFFICIENT;
        }

    } else {

        return ALSA_REPLAY_FAILED;
    }
}

/*----------------------------------------------------------------------------*/

static ov_alsa_playback_play_result
feed_alsa_buffer_up_to(ov_alsa_playback *self, ov_chunker *pcm,
                       size_t num_max_periods) {

    if (ov_ptr_valid(self, "Cannot feed ALSA buffer new audio - invalid "
                           "ov_alsa_playback object")) {

        OV_ASSERT(0 != self->alsa_period_buffer);

        size_t periods_fed = 0;
        ov_alsa_playback_play_result result = ALSA_REPLAY_OK;

        for (periods_fed = 0;
             (ALSA_REPLAY_OK == result) && (periods_fed <= num_max_periods);
             periods_fed++) {

            result = feed_alsa_buffer_next_period(
                pcm, self->alsa, self->alsa_period_buffer->start,
                self->alsa_octets_per_period);
        };

        --periods_fed; // Loop is iterated over one time too much

        if (ALSA_REPLAY_FAILED == result) {

            ov_log_error("ALSA: Failed to replay");
            return result;

        } else if (0 == periods_fed) {

            // ALSA_REPLAY_INSUFFICIENT == result
            // is wrong, since the loop above circles until result BECOMES
            // ALSA_REPLAY_INSUFFICIENT !!!

            ov_log_error("ALSA: Insufficient PCM available");
            return ALSA_REPLAY_INSUFFICIENT;

        } else {

            ov_counter_increase(self->counter.periods_played,
                                (size_t)periods_fed);

            ov_log_debug("ALSA: Wrote %zu periods == %zu samples", periods_fed,
                         (size_t)(periods_fed * self->alsa_octets_per_period) /
                             OV_DEFAULT_OCTETS_PER_SAMPLE);
            return ALSA_REPLAY_OK;
        }

    } else {

        return ALSA_REPLAY_FAILED;
    }
}

/*----------------------------------------------------------------------------*/

static ov_alsa_playback_play_result
replay_if_necessary(ov_alsa_playback *self, ov_alsa *alsa, ov_chunker *pcm,
                    size_t bufsize_samples) {

    ssize_t writeable_samples = ov_alsa_get_no_available_samples(alsa);

    size_t remaining_samples_in_alsa_buffer =
        (size_t)(bufsize_samples - writeable_samples);

    if (writeable_samples > (ssize_t)bufsize_samples) {

        ov_log_error("Serious ALSA problem: ALSA buffer smaller than number of "
                     "writeable octets");
        writeable_samples = bufsize_samples;
    }

    if (0 > writeable_samples) {

        ov_log_warning("ALSA: problem with ALSA connection");
        return ALSA_REPLAY_OK;

    } else if ((!self->buffer_after_interrupt) &&
               (0 == remaining_samples_in_alsa_buffer)) {

        ov_log_warning("Probable ALSA buffer underflow encountered");

        self->buffer_after_interrupt = true;

        return ALSA_REPLAY_INSUFFICIENT;

    } else if (self->buffer_after_interrupt &&
               (ov_chunker_available_octets(pcm) <
                2 * (OV_DEFAULT_OCTETS_PER_SAMPLE *
                     OV_DEFAULT_FRAME_LENGTH_SAMPLES))) {

        ov_log_warning(
            "Stream still buffering - requiring %zu octets, got only %zu "
            "octets",
            2 * OV_DEFAULT_FRAME_LENGTH_SAMPLES * OV_DEFAULT_OCTETS_PER_SAMPLE,
            remaining_samples_in_alsa_buffer);

        return ALSA_REPLAY_INSUFFICIENT;

    } else {

        OV_ASSERT(writeable_samples >= 0);
        OV_ASSERT((size_t)writeable_samples <= bufsize_samples);

        ov_log_debug(
            "ALSA: Writing ... writeable %i samples, ALSA buffer size %zu "
            "samples",
            (int)writeable_samples, bufsize_samples);

        self->buffer_after_interrupt = false;

        size_t receiveable_periods =
            writeable_samples / self->alsa_samples_per_period;

        ov_alsa_playback_play_result res =
            feed_alsa_buffer_up_to(self, pcm, receiveable_periods);

        self->buffer_after_interrupt = (ALSA_REPLAY_INSUFFICIENT == res);

        return res;
    }
}

/*----------------------------------------------------------------------------*/

ov_alsa_playback_play_result ov_alsa_playback_play(ov_alsa_playback *self,
                                                   ov_chunker *pcm) {

    if (ov_ptr_valid(self,
                     "ALSA: Cannot replay: Invalid ov_alsa_playback pointer") &&
        ov_ptr_valid(self->alsa,
                     "ALSA: Cannot replay: Invalid ov_alsa pointer")) {

        return replay_if_necessary(self, self->alsa, pcm,
                                   self->alsa_buffer_size_samples);

    } else {

        return ALSA_REPLAY_FAILED;
    }
}

/*----------------------------------------------------------------------------*/

bool ov_alsa_playback_play_comfort_noise(ov_alsa_playback *self) {

    if (ov_ptr_valid(self,
                     "Cannot play comfort noise - invalid ALSA pointer") &&
        ov_ptr_valid(self->comfort_noise,
                     "Cannot play comfort noise - comfort noise not "
                     "initialized")) {
        return play_pcm(self->alsa, self->comfort_noise->start);
    } else {
        return false;
    }
}

/*----------------------------------------------------------------------------*/

static ov_alsa_playback *create_alsa_playback(const ov_alsa_playback_config cfg,
                                              ov_result *res) {

    ov_alsa_playback *self = calloc(1, sizeof(ov_alsa_playback));

    self->magic_bytes = MAGIC_BYTES;

    self->msecs_to_buffer_ahead = cfg.msecs_to_buffer_ahead;

    size_t samples_per_period = ov_convert_msecs_to_samples(
        OV_DEFAULT_FRAME_LENGTH_MS, OV_DEFAULT_SAMPLERATE);

    self->alsa = ov_alsa_create(cfg.alsa_device, OV_DEFAULT_SAMPLERATE,
                                &samples_per_period, PLAYBACK);

    self->alsa_samples_per_period = samples_per_period;
    self->alsa_octets_per_period = 2 * samples_per_period;

    self->alsa_buffer_size_samples =
        ov_alsa_get_buffer_size_samples(self->alsa);

    self->alsa_period_buffer = ov_buffer_create(self->alsa_octets_per_period);

    self->comfort_noise = create_comfort_noise(
        ov_convert_samples_to_msecs(samples_per_period, OV_DEFAULT_SAMPLERATE),
        cfg.comfort_noise.max_amplitude);

    self->logging_fd = cfg.logging_fd;

    if (0 == self->alsa) {

        ov_log_error("Could not create alsa wrapper");
        ov_result_set(res, OV_ERROR_INTERNAL_SERVER,
                      "Could not create alsa wrapper");
        self = ov_alsa_playback_free(self);

    } else {

        ov_log_info("ALSA output to %s", ov_string_sanitize(cfg.alsa_device));
    }

    return self;
}

/*----------------------------------------------------------------------------*/

ov_alsa_playback *ov_alsa_playback_create(const ov_alsa_playback_config cfg,
                                          ov_result *res) {

    if ((0 != cfg.mixer_element) &&
        (!ov_alsa_set_volume(
            cfg.alsa_device, cfg.mixer_element, PLAYBACK,
            OV_OR_DEFAULT(cfg.volume, OV_ALSA_PLAYBACK_DEFAULT_VOLUME)))) {

        ov_result_set(res, OV_ERROR_INTERNAL_SERVER,
                      "Cannot set ALSA volume for playback");
        return 0;

    } else {

        return create_alsa_playback(cfg, res);
    }
}

/*----------------------------------------------------------------------------*/

ov_alsa_playback *ov_alsa_playback_free(ov_alsa_playback *self) {

    if (0 != as_playback(self)) {

        self->alsa = ov_alsa_free(self->alsa);
        self->alsa_period_buffer = ov_buffer_free(self->alsa_period_buffer);
        self->comfort_noise = ov_buffer_free(self->comfort_noise);

        return ov_free(self);

    } else {

        return self;
    }
}

/*----------------------------------------------------------------------------*/

bool ov_alsa_playback_reset(ov_alsa_playback *self) {

    if (ov_ptr_valid(self, "Cannot reset playback: Invalid pointer")) {

        return ov_alsa_reset(self->alsa);

    } else {

        return false;
    }
}

/*----------------------------------------------------------------------------*/
