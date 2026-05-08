/***
        ------------------------------------------------------------------------

        Copyright (c) 2024 German Aerospace Center DLR e.V. (GSOC)

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
/*----------------------------------------------------------------------------*/

#include "ov_rtp_mixer.h"
#include <ov_base/ov_convert.h>
#include <ov_base/ov_dict.h>
#include <ov_base/ov_rtp_frame_buffer.h>
#include <ov_base/ov_thread_lock.h>
#include <ov_codec/ov_codec.h>
#include <ov_codec/ov_codec_factory.h>
#include <ov_codec/ov_codec_opus.h>
#include <ov_pcm16s/ov_pcm16_mod.h>
#include <ov_pcm_gen/ov_pcm_gen.h>

/*----------------------------------------------------------------------------*/

#define MAGIC_BYTES 0x2f2d2e2f

struct ov_rtp_mixer_struct {

    uint32_t magic_bytes;

    ov_rtp_frame_buffer *frame_buffer;
    ov_thread_lock frame_buffer_lock;

    struct {

        size_t frame_length_ms;
        double sample_rate_hertz;
        size_t frame_length_samples;

        size_t decoded_frame_length_samples;

        uint32_t ssid_to_cancel;

    } settings;

    struct {
        bool rtp_keepalive : 1;
    };

    struct {

        bool enabled;
        ov_buffer *noisy_frame_16bit;
        uint16_t max_amplitude;

    } comfort_noise;

    ov_dict *codecs;
};

/*----------------------------------------------------------------------------*/

static ov_rtp_mixer const *as_mixer(void const *ptr) {

    ov_rtp_mixer const *mixer = ptr;

    if ((0 != ptr) && (MAGIC_BYTES == mixer->magic_bytes)) {
        return mixer;
    } else {
        return 0;
    }
}

/*----------------------------------------------------------------------------*/

typedef struct {

    ov_codec *codec;
    time_t last_used_epoch_secs; // For garbage collection

} codec_entry;

/*----------------------------------------------------------------------------*/

static codec_entry *codec_entry_free(codec_entry *entry) {

    if (0 != entry) {
        entry->codec = ov_codec_free(entry->codec);
        return ov_free(entry);
    }

    return entry;
}

/*----------------------------------------------------------------------------*/

static ov_codec *codec_from_codec_entry(codec_entry *entry) {

    if (0 != entry) {

        entry->last_used_epoch_secs = time(0);
        return entry->codec;

    } else {

        return 0;
    }
}

/*----------------------------------------------------------------------------*/

static ov_codec *get_codec_for_ssrc(ov_rtp_mixer *self, uint32_t ssrc) {

    intptr_t key = ssrc;
    codec_entry *codec = 0;

    if (ov_ptr_valid(self, "Cannot get Codec - invalid RTP mixer pointer")) {

        codec = ov_dict_get(self->codecs, (void *)key);

        if (!codec) {

            codec = calloc(1, sizeof(codec_entry));

            codec->codec =
                ov_codec_factory_get_codec(0, ov_codec_opus_id(), key, 0);

            if (codec && (!ov_dict_set(self->codecs, (void *)key, codec, 0))) {
                codec = codec_entry_free(codec);
            }
        }
    }

    return codec_from_codec_entry(codec);
}

/*----------------------------------------------------------------------------*/

static ov_codec *get_codec_for_frame(ov_rtp_mixer *mixer,
                                     const ov_rtp_frame *frame) {

    if (ov_ptr_valid(frame,
                     "Cannot get codec for RTP frame: invalid frame pointer")) {

        return get_codec_for_ssrc(mixer, frame->expanded.ssrc);

    } else {

        return 0;
    }
}

/*----------------------------------------------------------------------------*/

static ov_buffer *create_comfort_noise_for_default_frame(ov_rtp_mixer *self) {

    ov_buffer *buf16 = 0;

    if (ov_ptr_valid(self,
                     "Cannot initialize comfort noise buffer - invalid mixer "
                     "pointer")) {

        ov_pcm_gen_config cfg = {
            .frame_length_usecs = 1000 * self->settings.frame_length_ms,
            .sample_rate_hertz = self->settings.sample_rate_hertz,
        };

        ov_pcm_gen_white_noise wnoise = {
            .max_amplitude = self->comfort_noise.max_amplitude,
        };

        ov_pcm_gen *generator = ov_pcm_gen_create(OV_WHITE_NOISE, cfg, &wnoise);
        buf16 = ov_pcm_gen_generate_frame(generator);

        generator = ov_pcm_gen_free(generator);

        OV_ASSERT(0 == generator);
        OV_ASSERT(0 != self);
    }

    return buf16;
}

/*----------------------------------------------------------------------------*/

static void *codec_entry_free_void(void *codec) {

    return (void *)codec_entry_free(codec);
}

/*----------------------------------------------------------------------------*/

ov_rtp_mixer *ov_rtp_mixer_create(ov_rtp_mixer_config cfg) {

    ov_rtp_frame_buffer_config fbconf = {
        .num_frames_to_buffer_per_stream =
            OV_OR_DEFAULT(cfg.max_num_frames_per_stream, 10),
    };

    ov_rtp_mixer *mixer = calloc(1, sizeof(ov_rtp_mixer));

    mixer->magic_bytes = MAGIC_BYTES;
    mixer->frame_buffer = ov_rtp_frame_buffer_create(fbconf);
    mixer->rtp_keepalive = true;
    mixer->comfort_noise.enabled = (0 == cfg.comfort_noise_max_amplitude);

    mixer->comfort_noise.max_amplitude =
        OV_OR_DEFAULT(cfg.comfort_noise_max_amplitude, 0);

    mixer->settings.frame_length_ms =
        OV_OR_DEFAULT(cfg.frame_length_ms, OV_DEFAULT_FRAME_LENGTH_MS);

    mixer->settings.sample_rate_hertz =
        OV_OR_DEFAULT(cfg.sample_rate_hertz, OV_DEFAULT_SAMPLERATE);

    mixer->settings.frame_length_samples = ov_convert_msecs_to_samples(
        mixer->settings.frame_length_ms, mixer->settings.sample_rate_hertz);

    mixer->settings.decoded_frame_length_samples = ov_convert_msecs_to_samples(
        mixer->settings.frame_length_ms, OV_DEFAULT_SAMPLERATE);

    mixer->settings.ssid_to_cancel = cfg.ssid_to_cancel;

    mixer->comfort_noise.noisy_frame_16bit =
        create_comfort_noise_for_default_frame(mixer);

    ov_dict_config d_config = ov_dict_intptr_key_config(255);
    d_config.value.data_function.free = codec_entry_free_void;
    mixer->codecs = ov_dict_create(d_config);

    ov_thread_lock_init(&mixer->frame_buffer_lock,
                        1000 * OV_DEFAULT_FRAME_LENGTH_MS / 2.0);

    return mixer;
}

/*----------------------------------------------------------------------------*/

ov_rtp_mixer *ov_rtp_mixer_free(ov_rtp_mixer *self) {
    if (ov_ptr_valid(as_mixer(self),
                     "Cannot free RTP mixer: Invalid pointer")) {
        self->frame_buffer = ov_rtp_frame_buffer_free(self->frame_buffer);
        ov_thread_lock_clear(&self->frame_buffer_lock);
        self->codecs = ov_dict_free(self->codecs);
        self->comfort_noise.noisy_frame_16bit =
            ov_buffer_free(self->comfort_noise.noisy_frame_16bit);

        return ov_free(self);

    } else {
        return self;
    }
}

/*----------------------------------------------------------------------------*/

static ov_rtp_frame *frame_buffer_add(ov_thread_lock *lock,
                                      ov_rtp_frame_buffer *buffer,
                                      ov_rtp_frame *frame) {
    if (ov_thread_lock_try_lock(lock)) {
        frame = ov_rtp_frame_buffer_add(buffer, frame);

        ov_thread_lock_unlock(lock);
    }
    return frame;
}

/*----------------------------------------------------------------------------*/

bool ov_rtp_mixer_add_frame(ov_rtp_mixer *self, ov_rtp_frame *frame) {
    if (ov_ptr_valid(
            self, "Cannot add RTP frame to mixing buffer - invalid pointer") &&
        ov_ptr_valid(
            frame, "Cannot add RTP frame to mixing buffer - invalid pointer")) {

        if (self->settings.ssid_to_cancel == frame->expanded.ssrc) {
            ov_rtp_frame_free(frame);
        } else {
            ov_rtp_frame_free(frame_buffer_add(&self->frame_buffer_lock,
                                               self->frame_buffer, frame));
        }

        return true;

    } else {
        return false;
    }
}

/*----------------------------------------------------------------------------*/

static ov_list *free_frame_list(ov_list *list) {
    for (ov_rtp_frame *frame = ov_list_pop(list); 0 != frame;
         frame = ov_list_pop(list)) {
        frame = ov_rtp_frame_free(frame);
    }

    return ov_list_free(list);
}

/*----------------------------------------------------------------------------*/

static bool decode_frame(ov_codec *codec, ov_rtp_frame *frame,
                         ov_buffer *target) {
    if (ov_ptr_valid(codec, "Cannot decode frame - invalid codec") &&
        ov_ptr_valid(frame,
                     "Cannot decode RTP frame - invalid RTP frame pointer") &&
        ov_ptr_valid(target,
                     "Cannot decode RTP frame - invalid target pointer")) {
        target->length = ov_codec_decode(codec, frame->expanded.sequence_number,
                                         frame->expanded.payload.data,
                                         frame->expanded.payload.length,
                                         target->start, target->capacity);

        return true;

    } else {
        return false;
    }
}

/*----------------------------------------------------------------------------*/

static ov_buffer *mix_frames(ov_rtp_mixer *self, ov_list *frames,
                             size_t decoded_frame_length_samples) {
    size_t num_frames = ov_list_count(frames);
    size_t num_mixed_frames = 0;

    size_t decoded_len_16bit = decoded_frame_length_samples * sizeof(int16_t);
    size_t decoded_len_32bit = decoded_frame_length_samples * sizeof(int32_t);

    ov_buffer *decoded_16bit = ov_buffer_create(decoded_len_16bit);
    decoded_16bit->length = decoded_len_16bit;

    ov_buffer *decoded_32bit = ov_buffer_create(decoded_len_32bit);
    decoded_32bit->length = decoded_len_32bit;

    ov_buffer *mixed_32bit = ov_buffer_create(decoded_len_32bit);
    mixed_32bit->length = decoded_len_32bit;
    memset(mixed_32bit->start, 0, mixed_32bit->length);

    for (size_t i = 0; i < num_frames; ++i) {
        ov_rtp_frame *frame = ov_list_pop(frames);

        if (decode_frame(get_codec_for_frame(self, frame), frame,
                         decoded_16bit) &&
            (ov_ptr_valid(decoded_16bit, "Could not decode RTP frame") &&
             ov_cond_valid(decoded_len_16bit == decoded_16bit->length,
                           "Decoded RTP frame has unexpected length") &&
             (ov_cond_valid(ov_pcm_16_scale_to_32(
                                decoded_frame_length_samples,
                                (int16_t *)decoded_16bit->start,
                                (int32_t *)decoded_32bit->start, 1.0, 0, 0),
                            "Could not scale decoded PCM to 32 bit")) &&
             ov_cond_valid(ov_pcm_32_add(decoded_frame_length_samples,
                                         (int32_t *)mixed_32bit->start,
                                         (int32_t *)decoded_32bit->start),
                           "Could not add decoded RTP payload"))) {
            ++num_mixed_frames;
        }

        frame = ov_rtp_frame_free(frame);
    }

    ov_buffer *pcm16 = ov_buffer_create(decoded_len_16bit);
    pcm16->length = decoded_len_16bit;

    double scale_factor = OV_OR_DEFAULT(num_mixed_frames, 1);

    if ((!ov_cond_valid(ov_pcm_32_scale(decoded_frame_length_samples,
                                        (int32_t *)mixed_32bit->start,
                                        1.0 / scale_factor),
                        "Could not scale mixed PCM")) ||
        (!ov_cond_valid(ov_pcm_32_clip_to_16(decoded_frame_length_samples,
                                             (int32_t *)mixed_32bit->start,
                                             (int16_t *)pcm16->start),
                        "Could not clip mixed PCM"))) {
        pcm16 = ov_buffer_free(pcm16);
    }

    decoded_16bit = ov_buffer_free(decoded_16bit);
    decoded_32bit = ov_buffer_free(decoded_32bit);
    mixed_32bit = ov_buffer_free(mixed_32bit);

    return pcm16;
}

/*----------------------------------------------------------------------------*/

static bool process_frames(ov_rtp_mixer *self, ov_list *frames,
                           ov_chunker *chunker_to_write_to) {
    bool ok = false;

    if (ov_ptr_valid(self, "Cannot mix frames - invalid mixer pointer") &&
        ov_ptr_valid_warn(frames, "Cannot mix frames - no frame list")) {
        ov_buffer *mixed_payload = mix_frames(
            self, frames, self->settings.decoded_frame_length_samples);

        if (0 != mixed_payload) {
            ov_chunker_add(chunker_to_write_to, mixed_payload);
            ok = true;

        } else {
            ov_log_debug("No frame to forward");
            if (0 != self->comfort_noise.noisy_frame_16bit) {
                ov_log_debug("Adding comfort noise");
                ov_chunker_add(chunker_to_write_to,
                               self->comfort_noise.noisy_frame_16bit);

            } else {
                ov_log_debug("Comfort noise not added - not configured");
            }
        }

        mixed_payload = ov_buffer_free(mixed_payload);
    }

    frames = free_frame_list(frames);

    return ok;
}

/*----------------------------------------------------------------------------*/

static ov_list *frame_buffer_get_current_frames(ov_thread_lock *lock,
                                                ov_rtp_frame_buffer *buffer) {
    ov_list *frames = 0;

    if (ov_thread_lock_try_lock(lock)) {
        frames = ov_rtp_frame_buffer_get_current_frames(buffer);

        ov_thread_lock_unlock(lock);
    }

    return frames;
}

/*----------------------------------------------------------------------------*/

bool ov_rtp_mixer_mix(ov_rtp_mixer *self, ov_chunker *chunker_to_write_to) {
    if (ov_ptr_valid(self, "Cannot mix frames - invalid mixer pointer")) {
        ov_list *frame_list = frame_buffer_get_current_frames(
            &self->frame_buffer_lock, self->frame_buffer);

        return process_frames(self, frame_list, chunker_to_write_to);

    } else {
        return false;
    }
}

/*****************************************************************************
                      Garbage collection of stale streams
 ****************************************************************************/

typedef struct {
    time_t before_epoch_secs;
    uint32_t ssids[10];
    size_t ssids_found;
} gc_args;

/*----------------------------------------------------------------------------*/

static bool collect_stale_stream_ssids(const void *key, void *value,
                                       void *data) {
    gc_args *args = data;

    if (ov_ptr_valid(args, "Cannot collect stale RTP streams - invalid args "
                           "pointer") &&
        (sizeof(args->ssids) / sizeof(args->ssids[0]) > args->ssids_found)) {
        codec_entry *entry = value;

        if ((0 != entry) &&
            (entry->last_used_epoch_secs < args->before_epoch_secs)) {
            intptr_t ssid = (intptr_t)key;
            args->ssids[args->ssids_found++] = (uint32_t)ssid;
        }

        return true;

    } else {
        return false;
    }
}

/*----------------------------------------------------------------------------*/

static void clean_codec_entries(ov_dict *codec_entries, gc_args args) {
    for (size_t i = 0; i < args.ssids_found; ++i) {
        ov_log_info("RTP mixer: Removing stale stream %" PRIu32, args.ssids[i]);
        intptr_t ssidptr = args.ssids[i];
        ov_dict_del(codec_entries, (void *)ssidptr);
    }
}

/*----------------------------------------------------------------------------*/

bool ov_rtp_mixer_garbage_collect(ov_rtp_mixer *self,
                                  uint32_t max_stream_lifetime_secs) {
    gc_args args = {
        .before_epoch_secs = time(0) - max_stream_lifetime_secs,
    };

    ov_log_info("RTP mixer: Triggered GC run");

    if (ov_ptr_valid(self,
                     "Cannot perform garbage collection - invalid RTP mixer")) {
        ov_dict_for_each(self->codecs, &args, collect_stale_stream_ssids);
        clean_codec_entries(self->codecs, args);

        return true;

    } else {
        return false;
    }
}

/*----------------------------------------------------------------------------*/

void ov_rtp_mixer_enable_caching() {
    ov_rtp_frame_enable_caching(20);
    ov_buffer_enable_caching(10);
}

/*----------------------------------------------------------------------------*/
