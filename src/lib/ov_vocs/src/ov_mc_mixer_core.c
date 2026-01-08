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
        @file           ov_mc_mixer_core.c
        @author         Michael Beer
        @author         Markus Töpfer

        @date           2023-01-21


        ------------------------------------------------------------------------
*/
#include "../include/ov_mc_mixer_core.h"

#define OV_MC_MIXER_CORE_MAGIC_BYTES 0xabcd

#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

#include <ov_base/ov_dict.h>
#include <ov_base/ov_linked_list.h>
#include <ov_base/ov_rtcp.h>
#include <ov_base/ov_string.h>

#include <ov_backend/ov_frame_data.h>
#include <ov_backend/ov_frame_data_list.h>
#include <ov_base/ov_rtp_frame_buffer.h>

#include <ov_codec/ov_codec.h>
#include <ov_codec/ov_codec_factory.h>
#include <ov_codec/ov_codec_opus.h>

#include <ov_pcm16s/ov_pcm16_mod.h>
#include <ov_pcm_gen/ov_pcm_gen.h>

/*----------------------------------------------------------------------------*/

struct ov_mc_mixer_core {

  uint16_t magic_bytes;

  ov_mc_mixer_core_config config;
  ov_mc_mixer_core_forward forward;

  int socket;
  ov_socket_data local;

  char *name;

  ov_dict *loops;

  ov_rtp_frame_buffer *frame_buffer;

  uint32_t mix_timer;

  struct {

    ov_codec_factory *factory;
    ov_dict *codecs;
    size_t gc_cycle;

  } codec;

  struct output_t {

    uint16_t sequence_number;
    uint32_t timestamp;
    bool mark;

    uint8_t payload_type;
    uint32_t ssid;

  } output;

  ov_buffer *comfort_noise_32bit;

  uint64_t marker_counter;
};

/*----------------------------------------------------------------------------*/

static ov_list *ov_mc_mixer_core_frame_processing_list_free(ov_list *list) {

  if (!list)
    goto error;

  for (ov_rtp_frame *frame = ov_list_pop(list); 0 != frame;
       frame = ov_list_pop(list)) {

    frame = ov_rtp_frame_free(frame);
  }

  return ov_list_free(list);
error:
  return list;
}

/*----------------------------------------------------------------------------*/

static void process_rtcp(ov_mc_mixer_core *mixer, const uint8_t *input,
                         size_t length) {

  ov_log_debug("Got RTCP: SSRC of mixer is %" PRIu32, mixer->forward.ssrc);

  if (ov_ptr_valid(mixer, "Cannot process RTCP: Invalid mixer object") &&
      (OV_DEFAULT_SSID == mixer->forward.ssrc)) {

    ov_log_debug("Got RTCP - SSRC to cancel not set yet");

    // we will receive a compount RTCP package here

    const uint8_t *buffer = input;
    size_t open = length;

    ov_rtcp_message *msg = ov_rtcp_message_decode(&buffer, &open);

    while (msg) {

      if (OV_RTCP_SOURCE_DESC == ov_rtcp_message_type(msg)) {

        uint32_t ssrc = ov_rtcp_message_sdes_ssrc(msg, 0);
        mixer->forward.ssrc = ssrc;

        ov_log_debug("From RTCP: Set SSRC to cancel to %" PRIu32,
                     mixer->forward.ssrc);
      }

      msg = ov_rtcp_message_free(msg);
      msg = ov_rtcp_message_decode(&buffer, &open);
    }
  }
}

/*----------------------------------------------------------------------------*/

static void cb_io_multicast(void *userdata, const ov_mc_loop_data *data,
                            const uint8_t *buffer, size_t bytes,
                            const ov_socket_data *remote) {

  ov_rtp_frame *frame = NULL;

  ov_mc_mixer_core *mixer = ov_mc_mixer_core_cast(userdata);
  if (!mixer || !data || !buffer || 0 == bytes || !remote)
    return;

  switch (buffer[1]) {

  case 200:
  case 201:
  case 202:
  case 203:
  case 204:
    process_rtcp(mixer, buffer, bytes);
    return;
  default:
    break;
  }

  frame = ov_rtp_frame_decode(buffer, bytes);

  if (0 != frame) {

    /* kind of a hack to transport volume internaly within the RTP stream
     * set the volume as the payload type */

    frame->bytes.data[1] &= 0x80;
    frame->bytes.data[1] |= data->volume;
    frame->expanded.payload_type = data->volume;

    /* Echo cancelation == ignore all incoming frames with the SSRC
     * of the forward destination configured. */

    if (frame->expanded.ssrc == mixer->forward.ssrc) {
      /*
                  ov_log_debug("Ignoring (Dropping) frame for SSRC %"
         PRIu32, frame->expanded.ssrc);
      */
    } else {

      frame = ov_rtp_frame_buffer_add(mixer->frame_buffer, frame);
    }
  }

  frame = ov_rtp_frame_free(frame);
}

/*----------------------------------------------------------------------------*/

typedef struct {

  ov_codec *codec;

  struct {
    bool voice_detected : 1;
  };

  time_t last_used_epoch_secs; // For garbage collection

} RtpStream;

/*----------------------------------------------------------------------------*/

static RtpStream *rtp_stream_free(RtpStream *entry) {

  if (0 != entry) {
    entry->codec = ov_codec_free(entry->codec);
    return ov_free(entry);
  }

  return entry;
}

/*----------------------------------------------------------------------------*/

static RtpStream *rtp_stream_reset_gc_timer(RtpStream *stream,
                                            uint64_t now_epoch_secs) {

  if (0 != stream) {

    stream->last_used_epoch_secs = now_epoch_secs;
  }

  return stream;
}

/*----------------------------------------------------------------------------*/

static RtpStream *rtp_stream_for_ssrc(ov_mc_mixer_core *self, uint32_t ssrc) {

  intptr_t key = ssrc;
  RtpStream *stream = 0;

  if (ov_ptr_valid(self, "Cannot get Stream Info - invalid mixer pointer")) {

    stream = ov_dict_get(self->codec.codecs, (void *)key);

    if (0 == stream) {

      stream = calloc(1, sizeof(RtpStream));

      stream->codec = ov_codec_factory_get_codec(self->codec.factory,
                                                 ov_codec_opus_id(), key, 0);

      if ((0 != stream) &&
          (!ov_dict_set(self->codec.codecs, (void *)key, stream, 0))) {
        stream = rtp_stream_free(stream);
      }
    }
  }

  return rtp_stream_reset_gc_timer(stream, time(0));
}

/*----------------------------------------------------------------------------*/

static RtpStream *get_rtp_stream(ov_mc_mixer_core *mixer,
                                 const ov_rtp_frame *frame) {

  OV_ASSERT(mixer);
  OV_ASSERT(frame);

  return rtp_stream_for_ssrc(mixer, frame->expanded.ssrc);
}

/*----------------------------------------------------------------------------*/

static ov_codec *get_destination_codec(ov_mc_mixer_core *mixer) {

  OV_ASSERT(mixer);

  RtpStream *stream = rtp_stream_for_ssrc(mixer, mixer->output.ssid);

  if (0 != stream) {

    return stream->codec;

  } else {

    ov_log_error("Could not get destination codec");
    return 0;
  }
}

/*----------------------------------------------------------------------------*/

static ov_buffer *decode(ov_mc_mixer_core *mixer, ov_rtp_frame const *frame,
                         ov_codec *codec, const size_t buflen_max) {

  OV_ASSERT(mixer);
  OV_ASSERT(frame);

  if (!codec)
    goto error;

  ov_buffer *decoded = ov_buffer_create(buflen_max);
  OV_ASSERT(NULL != decoded);

  decoded->length = ov_codec_decode(
      codec, frame->expanded.sequence_number, frame->expanded.payload.data,
      frame->expanded.payload.length, decoded->start, decoded->capacity);

  return decoded;
error:
  return NULL;
}

/*----------------------------------------------------------------------------*/

static ov_buffer *scale_decoded_pcm_nocheck(ov_buffer const *pcm,
                                            double scale_factor) {

  OV_ASSERT(0 != pcm);

  ov_buffer *result = 0;

  size_t const num_samples = pcm->length / 2;

  OV_ASSERT(num_samples * 2 == pcm->length);

  const size_t result_length = sizeof(int32_t) * num_samples;

  result = ov_buffer_create(result_length);

  if (!ov_pcm_16_scale_to_32_bare(num_samples, (int16_t *)pcm->start,
                                  (int32_t *)result->start, scale_factor)) {

    ov_log_error("Scaling of incoming PCM failed");

    goto error;
  }

  result->length = result_length;

  return result;

error:

  result = ov_buffer_free(result);
  OV_ASSERT(0 == result);

  return 0;
}

/*----------------------------------------------------------------------------*/

static ov_buffer *fade_decoded_pcm_nocheck(ov_buffer const *pcm,
                                           double scale_factor_start,
                                           double scale_factor_end) {

  OV_ASSERT(0 != pcm);

  ov_buffer *result = 0;

  size_t const num_samples = pcm->length / 2;

  OV_ASSERT(num_samples * 2 == pcm->length);

  const size_t result_length = sizeof(int32_t) * num_samples;

  result = ov_buffer_create(result_length);

  if (!ov_pcm_16_fade_to_32(num_samples, (int16_t *)pcm->start,
                            (int32_t *)result->start, scale_factor_start,
                            scale_factor_end)) {

    ov_log_error("Scaling of incoming PCM failed");

    goto error;
  }

  result->length = result_length;

  return result;

error:

  result = ov_buffer_free(result);
  OV_ASSERT(0 == result);

  return 0;
}

/*----------------------------------------------------------------------------*/

static ov_frame_data *frame_data_from_pcm(ov_buffer const *decoded,
                                          ov_rtp_frame_expansion const *frame,
                                          double volume) {

  ov_frame_data *data = 0;

  if ((0 != decoded) && (0 != frame)) {

    data = ov_frame_data_create();

    data->pcm16s_32bit = scale_decoded_pcm_nocheck(decoded, volume);

    data->ssid = frame->ssrc;
    data->sequence_number = frame->sequence_number;
    data->timestamp = frame->timestamp;
    data->payload_type = frame->payload_type;
    data->num_samples = data->pcm16s_32bit->length / sizeof(int32_t);

    OV_ASSERT(sizeof(int32_t) * data->num_samples ==
              data->pcm16s_32bit->length);
  }

  return data;
}

/*----------------------------------------------------------------------------*/

static ov_frame_data *
frame_data_from_pcm_with_vad(ov_buffer const *decoded,
                             ov_rtp_frame_expansion const *frame,
                             ov_vad_config vad_config, bool drop_no_va,
                             double volume, RtpStream *rtp_stream) {

  ov_frame_data *data = 0;
  ov_vad_parameters vad_params = {0};
  int16_t max_amplitude = 0;

  if ((0 != decoded) && (0 != frame) && (0 != rtp_stream) &&
      ov_cond_valid(ov_pcm_16_get_audio_params(decoded->length / 2,
                                               (int16_t *)decoded->start,
                                               &vad_params, &max_amplitude),
                    "Extracting audio parameters for VAD/normalisation "
                    "failed")) {

    bool voice_detected =
        ov_pcm_vad_detected(OV_DEFAULT_SAMPLERATE, vad_params, vad_config);

    bool voice_was_detected = rtp_stream->voice_detected;

    if (voice_detected) {

      double final_volume = volume;
      final_volume *= INT16_MAX;
      final_volume /= (double)max_amplitude;

      data = ov_frame_data_create();

      if (!voice_was_detected) {
        // Fade in
        data->pcm16s_32bit =
            fade_decoded_pcm_nocheck(decoded, volume, final_volume);
      } else {
        data->pcm16s_32bit = scale_decoded_pcm_nocheck(decoded, final_volume);
      }

    } else if (!drop_no_va) {

      // No VA detected, but don't drop

      data = ov_frame_data_create();

      if (voice_was_detected) {

        double start_volume = volume;
        start_volume *= INT16_MAX;
        start_volume /= (double)max_amplitude;

        // fade out
        data->pcm16s_32bit =
            fade_decoded_pcm_nocheck(decoded, start_volume, volume);
      } else {

        data->pcm16s_32bit = scale_decoded_pcm_nocheck(decoded, volume);
      }
    }

    if (0 != data) {

      data->ssid = frame->ssrc;
      data->sequence_number = frame->sequence_number;
      data->timestamp = frame->timestamp;
      data->payload_type = frame->payload_type;
      data->num_samples = data->pcm16s_32bit->length / sizeof(int32_t);

      data->max_amplitude = max_amplitude;

      OV_ASSERT(sizeof(int32_t) * data->num_samples ==
                data->pcm16s_32bit->length);
    }

    rtp_stream->voice_detected = voice_detected;
  }

  return data;
}

/*----------------------------------------------------------------------------*/

static ov_frame_data *frame_data_extract_nocheck(ov_mc_mixer_core *mixer,
                                                 ov_rtp_frame *const frame) {

  ov_frame_data *data = 0;
  ov_buffer *decoded = 0;

  RtpStream *rtp_stream = get_rtp_stream(mixer, frame);

  if (ov_ptr_valid(rtp_stream, "Cannot decode frame - no Stream info found") &&
      ov_ptr_valid(frame, "Cannot decode frame - no frame") &&
      // Volume 0 -> just drop frame
      (0 != frame->expanded.payload.length)) {

    /* we require 16 bit == 2 uint8_ts per sample */
    const size_t buflen_max =
        2 * OV_MAX_FRAME_LENGTH_MS * OV_MAX_SAMPLERATE_HZ / 1000;

    /* Scale volume to percent */

    double scale_factor = frame->expanded.payload_type;
    scale_factor /= 100.0;

    decoded = decode(mixer, frame, rtp_stream->codec, buflen_max);

    if ((0 != decoded) && (0 != decoded->length)) {

      if (mixer->config.incoming_vad) {

        ov_log_debug("VAD active - normalizing");
        data = frame_data_from_pcm_with_vad(
            decoded, &frame->expanded, mixer->config.vad,
            mixer->config.drop_no_va, scale_factor, rtp_stream);

      } else {

        ov_log_debug("VAD inactive - no normalization");
        data = frame_data_from_pcm(decoded, &frame->expanded, scale_factor);
      }
    }
  }

  decoded = ov_buffer_free(decoded);
  OV_ASSERT(0 == decoded);

  return data;
}

/*----------------------------------------------------------------------------*/

static void extract_frame_data(ov_mc_mixer_core *self, ov_frame_data_list *list,
                               ov_list *frames) {

  ov_rtp_frame *frame = 0;

  size_t num_frames = 0;
  size_t max_num_frames = UINT16_MAX;

  // size_t discarded_frames = 0;
  // size_t seen_frames = 0;

  void *iter = frames->iter(frames);

  while ((0 != iter) && (num_frames <= max_num_frames)) {

    void *vframe = 0;

    iter = frames->next(frames, iter, &vframe);

    frame = vframe;
    vframe = 0;

    if ((0 == frame) || (0 == frame->expanded.payload.length)) {
      continue;
    }

    ov_frame_data *data = frame_data_extract_nocheck(self, frame);

    if (0 == data) {
      continue;
    }

    //++seen_frames;

    ov_frame_data *old = 0;

    if (0 != data) {
      old = ov_frame_data_list_push_data(list, data);
      ++num_frames;
    }

    old = ov_frame_data_free(old);
    OV_ASSERT(0 == old);
  };

  return;
}

/*----------------------------------------------------------------------------*/

static void frame_length_from_frame_list(ov_frame_data_list const *list,
                                         size_t *num_bytes,
                                         size_t *num_samples) {

  size_t bytes = 0;
  size_t samples = 0;

  // get length of one frame
  for (size_t i = 0; list->capacity > i; ++i) {

    if (0 == list->frames[i]) {
      continue;
    }

    bytes = list->frames[i]->pcm16s_32bit->length;

    if (0 != bytes) {
      samples = list->frames[i]->num_samples;
      break;
    }
  }

  OV_ASSERT(sizeof(int32_t) * samples == bytes);

  if (0 != num_samples) {
    *num_samples = samples;
  }

  if (0 != num_bytes) {
    *num_bytes = bytes;
  }
}

/*----------------------------------------------------------------------------*/

/**
 * Input: list of pcm16s frames
 * Output: buffer containing pcm16s wrapped in pcm32s - the mixed pcm
 */
static ov_buffer *mix_nocheck(ov_frame_data_list const *list,
                              size_t length_bytes, size_t num_samples) {

  OV_ASSERT(0 != list);
  OV_ASSERT(0 != list->capacity);

  ov_buffer *result = 0;

  if (0 == length_bytes) {
    goto error;
  }

  result = ov_buffer_create(length_bytes);

  OV_ASSERT(0 != result);
  OV_ASSERT(length_bytes <= result->capacity);

  memset(result->start, 0, result->capacity);
  result->length = length_bytes;

  for (size_t i = 0; list->capacity > i; ++i) {

    ov_frame_data *frame = list->frames[i];

    if (0 == frame)
      continue;

    OV_ASSERT(0 != list->frames[i]);
    OV_ASSERT(0 != list->frames[i]->pcm16s_32bit);
    OV_ASSERT(0 != list->frames[i]->pcm16s_32bit->start);

    if (length_bytes != frame->pcm16s_32bit->length) {

      continue;
    }

    OV_ASSERT(length_bytes == list->frames[i]->pcm16s_32bit->length);

    if (!ov_pcm_32_add(num_samples, (int32_t *)result->start,
                       (int32_t *)list->frames[i]->pcm16s_32bit->start)) {

      goto error;
    }
  }

  return result;

error:

  if (0 != result) {

    result = ov_buffer_free(result);
  }

  OV_ASSERT(0 == result);

  return 0;
}

/*----------------------------------------------------------------------------*/

static ov_buffer *mix_frames_nocheck(ov_mc_mixer_core *self, size_t num_frames,
                                     ov_list *frames_list, size_t *num_samples,
                                     ov_frame_data_list **used_frames) {

  if ((!ov_ptr_valid(self, "Internal error: No mixer processing object")) ||
      (!ov_ptr_valid(used_frames,
                     "Internal error: No pointer to store used frames")) ||
      (0 == num_samples) || (0 == frames_list) || (0 == num_frames)) {
    goto error;
  }

  ov_frame_data_list *list = ov_frame_data_list_create(num_frames);
  *used_frames = list;

  extract_frame_data(self, list, frames_list);

  frames_list = ov_mc_mixer_core_frame_processing_list_free(frames_list);

  size_t len_bytes = 0;

  frame_length_from_frame_list(list, &len_bytes, num_samples);

  return mix_nocheck(list, len_bytes, *num_samples);

error:

  frames_list = ov_mc_mixer_core_frame_processing_list_free(frames_list);

  return 0;
}

/*----------------------------------------------------------------------------*/

static ov_buffer *get_comfort_noise(ov_mc_mixer_core *self,
                                    size_t *num_samples) {

  OV_ASSERT(0 != self);
  OV_ASSERT(0 != num_samples);
  OV_ASSERT(0 != self->comfort_noise_32bit);

  if (!self->config.rtp_keepalive) {
    return 0;
  }

  ov_buffer *comfort_noise = ov_buffer_copy(0, self->comfort_noise_32bit);
  OV_ASSERT(0 != comfort_noise);

  *num_samples = comfort_noise->length / sizeof(int32_t);

  return comfort_noise;
}

/*----------------------------------------------------------------------------*/

static ov_rtp_frame *encode_frame_nocheck(ov_frame_data const *frame_data,
                                          ov_codec *codec) {

  OV_ASSERT(0 != codec);
  OV_ASSERT(0 != codec->encode);

  ov_buffer *out = 0;

  if (0 == frame_data->pcm16s_32bit) {
    goto error;
  }

  const size_t num_samples = frame_data->pcm16s_32bit->length / sizeof(int32_t);

  // First step: 32bit -> 16bit
  ov_buffer *reduced = ov_buffer_create(num_samples * sizeof(int16_t));
  OV_ASSERT(num_samples * sizeof(int16_t) <= reduced->capacity);

  reduced->length = num_samples * sizeof(int16_t);

  ov_pcm_32_clip_to_16(num_samples, (int32_t *)frame_data->pcm16s_32bit->start,
                       (int16_t *)reduced->start);

  /* The codec will never encode to more bytes than the input, or will it ?*/
  out = ov_buffer_create(frame_data->pcm16s_32bit->length);

  int32_t encoded_bytes = ov_codec_encode(
      codec, reduced->start, reduced->length, out->start, out->capacity);

  reduced = ov_buffer_free(reduced);
  OV_ASSERT(0 == reduced);

  if (0 > encoded_bytes) {
    goto error;
  }

  OV_ASSERT(-1 < encoded_bytes);

  ov_rtp_frame_expansion exp = {

      .version = RTP_VERSION_2,
      /* This stuff needs to come from mixed_data  in the end*/
      .payload_type = frame_data->payload_type,
      .marker_bit = frame_data->marked,
      .sequence_number = frame_data->sequence_number,
      .timestamp = frame_data->timestamp,
      .ssrc = frame_data->ssid,

      .payload.length = (size_t)encoded_bytes,
      .payload.data = out->start,

  };

  ov_rtp_frame *frame = ov_rtp_frame_encode(&exp);

  out = ov_buffer_free(out);
  OV_ASSERT(0 == out);

  return frame;

error:

  out = ov_buffer_free(out);

  OV_ASSERT(0 == out);

  return 0;
}

/*----------------------------------------------------------------------------*/

static bool forward_mixed_frame_to_destination(ov_mc_mixer_core *mixer,
                                               ov_rtp_frame *out) {

  if (!mixer || !out)
    goto error;

  if (-1 == mixer->socket)
    goto error;

  struct sockaddr_storage dest = {0};

  if (!ov_socket_fill_sockaddr_storage(&dest, mixer->local.sa.ss_family,
                                       mixer->forward.socket.host,
                                       mixer->forward.socket.port))
    goto error;

  /* Set correct payload type */

  // set payload type
  out->bytes.data[1] =
      (out->bytes.data[1] & 0x80) | (0x7f & mixer->forward.payload_type);

  // set marker bit

  mixer->marker_counter++;
  if (0 == (mixer->marker_counter % 100))
    out->bytes.data[1] = (out->bytes.data[1] | 0x80);

  socklen_t len = sizeof(struct sockaddr_in);
  if (dest.ss_family == AF_INET6)
    len = sizeof(struct sockaddr_in6);

  ssize_t bytes = sendto(mixer->socket, out->bytes.data, out->bytes.length, 0,
                         (struct sockaddr *)&dest, len);
  /*
      ov_log_debug("send %zi bytes to %s:%i", bytes,
              mixer->forward.socket.host, mixer->forward.socket.port);
  */

  if (-1 == bytes)
    goto error;

  return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

static void
forward_mixed_frame_to_encoder(ov_mc_mixer_core *mixer,
                               ov_frame_data mixed_data,
                               ov_frame_data_list *original_frames) {

  ov_rtp_frame *encoded_frame = NULL;
  UNUSED(original_frames);

  if (!mixer)
    goto error;

  ov_buffer *mixed_signal = mixed_data.pcm16s_32bit;
  size_t num_samples = mixed_data.num_samples;

  OV_ASSERT(0 != mixed_signal);
  OV_ASSERT(num_samples * sizeof(int32_t) == mixed_signal->length);

  ov_codec *codec = get_destination_codec(mixer);

  encoded_frame = encode_frame_nocheck(&mixed_data, codec);

  if (!forward_mixed_frame_to_destination(mixer, encoded_frame))
    goto error;
error:
  encoded_frame = ov_rtp_frame_free(encoded_frame);
  return;
}

/*----------------------------------------------------------------------------*/

static bool process_frames(ov_mc_mixer_core *mixer, ov_list *frames) {

  bool result = false;
  ov_buffer *mixed_payload = NULL;
  ov_frame_data_list *used_frames = NULL;

  if (!mixer || !frames)
    goto finish;

  size_t num_frames = ov_list_count(frames);

  size_t num_samples = 0;

  mixed_payload =
      mix_frames_nocheck(mixer, num_frames, frames, &num_samples, &used_frames);

  frames = 0;

  if (0 == mixed_payload) {
    mixed_payload = get_comfort_noise(mixer, &num_samples);
  }

  if (0 == mixed_payload) {
    result = true;
    goto finish;
  }

  /*-----------------------------------------------------------------------*/

  ov_frame_data mixed_data = {

      .pcm16s_32bit = mixed_payload,
      .marked = mixer->output.mark,
      .sequence_number = mixer->output.sequence_number,
      .timestamp = mixer->output.timestamp,
      .payload_type = mixer->output.payload_type,
      .ssid = mixer->output.ssid,
      .num_samples = num_samples,

  };

  mixer->output.sequence_number += 1;
  mixer->output.timestamp += num_samples;

  forward_mixed_frame_to_encoder(mixer, mixed_data, used_frames);

  mixed_data.pcm16s_32bit = 0;

  mixer->output.mark = false;
  result = true;

finish:

  mixed_payload = ov_buffer_free(mixed_payload);
  frames = ov_mc_mixer_core_frame_processing_list_free(frames);
  used_frames = ov_frame_data_list_free(used_frames);

  OV_ASSERT(0 == mixed_payload);
  OV_ASSERT(0 == used_frames);
  OV_ASSERT(0 == frames);

  return result;
}

/*----------------------------------------------------------------------------*/

static bool codec_gc_run(ov_mc_mixer_core *self,
                         uint32_t max_stream_lifetime_secs);

static bool run_gc_if_required(ov_mc_mixer_core *self) {

  const size_t max_codec_lifetime_secs = 300;

  // A timer cycle lasts 20ms
  const size_t timer_calls_in_single_gc_cycle =
      max_codec_lifetime_secs * 1000 / 20;

  if (self->codec.gc_cycle++ > timer_calls_in_single_gc_cycle) {

    self->codec.gc_cycle = 0;
    return codec_gc_run(self, 300);
  }

  return true;
}

/*----------------------------------------------------------------------------*/

static bool cb_mix(uint32_t id, void *data) {

  ov_list *frame_list = 0;

  ov_mc_mixer_core *mixer = ov_mc_mixer_core_cast(data);

  OV_ASSERT(mixer->mix_timer == id);
  mixer->mix_timer = OV_TIMER_INVALID;

  run_gc_if_required(mixer);

  mixer->mix_timer =
      ov_event_loop_timer_set(mixer->config.loop, 20000, mixer, cb_mix);

  frame_list = ov_rtp_frame_buffer_get_current_frames(mixer->frame_buffer);

  if (0 == frame_list) {
    frame_list = ov_linked_list_create((ov_list_config){0});
  }

  if (process_frames(mixer, frame_list)) {
    frame_list = NULL;
  }

  frame_list = ov_mc_mixer_core_frame_processing_list_free(frame_list);

  return true;
}

/*----------------------------------------------------------------------------*/

static ov_buffer *
create_comfort_noise_for_default_frame(ov_mc_mixer_core *self) {

  OV_ASSERT(0 != self);

  ov_pcm_gen_config cfg = {
      .frame_length_usecs = 1000 * OV_DEFAULT_FRAME_LENGTH_MS,
      .sample_rate_hertz = OV_DEFAULT_SAMPLERATE,
  };

  ov_pcm_gen_white_noise wnoise = {
      .max_amplitude = self->config.comfort_noise_max_amplitude,
  };

  ov_pcm_gen *generator = ov_pcm_gen_create(OV_WHITE_NOISE, cfg, &wnoise);
  ov_buffer *buf = ov_pcm_gen_generate_frame(generator);

  generator = ov_pcm_gen_free(generator);

  size_t num_samples = buf->length / sizeof(int16_t);

  ov_buffer *buf32 = ov_buffer_create(num_samples * sizeof(int32_t));
  OV_ASSERT(0 != buf32);

  // The buffer now contains PCM16s, we need to convert into
  // PCM16s wrapped in 32bits per sample

  ov_pcm_16_scale_to_32(num_samples, (int16_t const *)buf->start,
                        (int32_t *)buf32->start, 1.0, 0, 0);

  buf = ov_buffer_free(buf);

  buf32->length = num_samples * sizeof(int32_t);

  OV_ASSERT(0 == generator);
  OV_ASSERT(0 == buf);

  return buf32;
}

/*----------------------------------------------------------------------------*/

static uint16_t get_max_amplitude(double level_db) {

  // Remember: Weird engineers decided it would be great to have
  // 10 dB in power equal 20 dB in levels...
  double factor = ov_utils_db_to_factor(level_db / 2.0);

  uint16_t level = (uint16_t)(INT16_MAX * factor);
  return level;
}

/*----------------------------------------------------------------------------*/

static ov_mc_mixer_core_config
set_config_defaults(ov_mc_mixer_core_config config) {

  ov_mc_mixer_core_config out = (ov_mc_mixer_core_config){

      .vad.powerlevel_density_threshold_db =
          OV_DEFAULT_POWERLEVEL_DENSITY_THRESHOLD_DB,
      .vad.zero_crossings_rate_threshold_hertz =
          OV_DEFAULT_ZERO_CROSSINGS_RATE_THRESHOLD_HZ,
      .incoming_vad = false,
      .samplerate_hz = 48000,
      .comfort_noise_max_amplitude = OV_DEFAULT_COMFORT_NOISE_LEVEL_DB,
      //.normalize_input = true,
      .rtp_keepalive = true,
      .max_num_frames_to_mix = 0,
      .normalize_mixing_result_by_square_root = false};

  if (0 != config.vad.powerlevel_density_threshold_db)
    out.vad.powerlevel_density_threshold_db =
        config.vad.powerlevel_density_threshold_db;

  if (0 != config.vad.zero_crossings_rate_threshold_hertz)
    out.vad.zero_crossings_rate_threshold_hertz =
        config.vad.zero_crossings_rate_threshold_hertz;

  if (0 != config.samplerate_hz)
    out.samplerate_hz = config.samplerate_hz;

  if (0 != config.comfort_noise_max_amplitude)
    out.comfort_noise_max_amplitude = config.comfort_noise_max_amplitude;

  if (0 != config.max_num_frames_to_mix)
    out.max_num_frames_to_mix = config.max_num_frames_to_mix;

  out.incoming_vad = config.incoming_vad;
  // out.normalize_input = config.normalize_input;
  out.rtp_keepalive = config.rtp_keepalive;
  out.normalize_mixing_result_by_square_root =
      config.normalize_mixing_result_by_square_root;

  out.comfort_noise_max_amplitude =
      get_max_amplitude(out.comfort_noise_max_amplitude);

  return out;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

static void *rtp_stream_free_void(void *rtp_stream) {

  return (void *)rtp_stream_free(rtp_stream);
}

/*----------------------------------------------------------------------------*/

ov_mc_mixer_core *ov_mc_mixer_core_create(ov_mc_mixer_core_config config) {

  ov_mc_mixer_core *mixer = NULL;

  if (0 == config.loop)
    goto error;

  ov_event_loop *loop = config.loop;
  config = set_config_defaults(config);
  config.loop = loop;

  if (0 == config.limit.frame_buffer_max)
    config.limit.frame_buffer_max = 10;

  mixer = calloc(1, sizeof(ov_mc_mixer_core));
  if (!mixer)
    goto error;

  mixer->magic_bytes = OV_MC_MIXER_CORE_MAGIC_BYTES;
  mixer->config = config;
  mixer->output.mark = true;

  ov_dict_config d_config = ov_dict_string_key_config(255);
  d_config.value.data_function.free = ov_mc_loop_free_void;

  mixer->loops = ov_dict_create(d_config);
  if (!mixer->loops)
    goto error;

  mixer->frame_buffer = ov_rtp_frame_buffer_create((ov_rtp_frame_buffer_config){
      .num_frames_to_buffer_per_stream = config.limit.frame_buffer_max});

  if (!mixer->frame_buffer)
    goto error;

  mixer->mix_timer = ov_event_loop_timer_set(config.loop, 20000, mixer, cb_mix);

  mixer->codec.factory = ov_codec_factory_create_standard();
  if (!mixer->codec.factory)
    goto error;

  d_config = ov_dict_intptr_key_config(255);
  d_config.value.data_function.free = rtp_stream_free_void;
  mixer->codec.codecs = ov_dict_create(d_config);
  if (!mixer->codec.codecs)
    goto error;

  mixer->comfort_noise_32bit = create_comfort_noise_for_default_frame(mixer);

  mixer->socket = -1;

  return mixer;

error:
  ov_mc_mixer_core_free(mixer);
  return NULL;
}

/*----------------------------------------------------------------------------*/

bool ov_mc_mixer_core_reconfigure(ov_mc_mixer_core *self,
                                  ov_mc_mixer_core_config config) {

  if (!self || !config.loop)
    goto error;

  if (self->mix_timer != OV_TIMER_INVALID) {
    ov_event_loop_timer_unset(self->config.loop, self->mix_timer, NULL);
  }

  ov_event_loop *loop = config.loop;
  config = set_config_defaults(config);
  config.loop = loop;
  self->config = config;

  self->comfort_noise_32bit = ov_buffer_free(self->comfort_noise_32bit);
  self->comfort_noise_32bit = create_comfort_noise_for_default_frame(self);

  self->mix_timer = ov_event_loop_timer_set(config.loop, 20000, self, cb_mix);

  return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

ov_mc_mixer_core *ov_mc_mixer_core_cast(const void *data) {

  if (!data)
    return NULL;

  if (*(uint16_t *)data != OV_MC_MIXER_CORE_MAGIC_BYTES)
    return NULL;

  return (ov_mc_mixer_core *)data;
}

/*----------------------------------------------------------------------------*/

ov_mc_mixer_core *ov_mc_mixer_core_free(ov_mc_mixer_core *self) {

  if (!ov_mc_mixer_core_cast(self))
    return self;

  if (self->frame_buffer)
    self->frame_buffer = self->frame_buffer->free(self->frame_buffer);

  self->name = ov_data_pointer_free(self->name);
  self->loops = ov_dict_free(self->loops);
  self->codec.factory = ov_codec_factory_free(self->codec.factory);
  self->codec.codecs = ov_dict_free(self->codec.codecs);
  self->comfort_noise_32bit = ov_buffer_free(self->comfort_noise_32bit);

  if (self->frame_buffer) {
    self->frame_buffer = self->frame_buffer->free(self->frame_buffer);
  }

  self = ov_data_pointer_free(self);
  return NULL;
}

/*----------------------------------------------------------------------------*/

bool ov_mc_mixer_core_set_name(ov_mc_mixer_core *self, const char *name) {

  if (!self || !name)
    goto error;

  self->name = ov_data_pointer_free(self->name);
  self->name = ov_string_dup(name);

  return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

const char *ov_mc_mixer_core_get_name(const ov_mc_mixer_core *self) {

  if (!self)
    return NULL;
  return self->name;
}

/*----------------------------------------------------------------------------*/

bool ov_mc_mixer_core_set_volume(ov_mc_mixer_core *self, const char *name,
                                 uint8_t vol) {

  if (!self || !name)
    goto error;

  ov_mc_loop *l = ov_dict_get(self->loops, name);
  if (!l)
    goto error;

  if (vol > 100)
    vol = 100;

  return ov_mc_loop_set_volume(l, vol);
error:
  return false;
}

/*----------------------------------------------------------------------------*/

uint8_t ov_mc_mixer_core_get_volume(const ov_mc_mixer_core *self,
                                    const char *name) {

  if (!self || !name)
    goto error;

  ov_mc_loop *l = ov_dict_get(self->loops, name);
  if (!l)
    goto error;

  return ov_mc_loop_get_volume(l);

error:
  return 0;
}

/*----------------------------------------------------------------------------*/

bool ov_mc_mixer_core_join(ov_mc_mixer_core *self, ov_mc_loop_data loop) {

  if (!self || (0 == loop.socket.host[0]) || (0 == loop.socket.port) ||
      (0 == loop.name[0]))
    goto error;

  ov_mc_loop_config config = (ov_mc_loop_config){

      .loop = self->config.loop,
      .data = loop,
      .callback.userdata = self,
      .callback.io = cb_io_multicast};

  ov_mc_loop *l = ov_mc_loop_create(config);
  if (!l)
    goto error;

  char *key = ov_string_dup(loop.name);
  if (!ov_dict_set(self->loops, key, l, NULL)) {

    key = ov_data_pointer_free(key);
    l = ov_mc_loop_free(l);
    goto error;
  }

  ov_log_info("Mixer %s joins loop %s at %s:%i", self->name, loop.name,
              loop.socket.host, loop.socket.port);

  return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

bool ov_mc_mixer_core_leave(ov_mc_mixer_core *self, const char *name) {

  if (!self || !name)
    goto error;

  if (!ov_dict_del(self->loops, name))
    goto error;

  ov_log_info("Mixer %s left loop %s", self->name, name);

  return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

bool ov_mc_mixer_core_release(ov_mc_mixer_core *self) {

  if (!self)
    goto error;

  ov_log_info("Mixer release %s", self->name);

  if (!ov_dict_clear(self->loops))
    goto error;

  if (!ov_dict_clear(self->codec.codecs))
    goto error;

  self->name = ov_data_pointer_free(self->name);
  self->forward = (ov_mc_mixer_core_forward){0};

  ov_list *frames = ov_rtp_frame_buffer_get_current_frames(self->frame_buffer);
  frames = ov_mc_mixer_core_frame_processing_list_free(frames);

  return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

bool ov_mc_mixer_core_forward_data_is_valid(
    const ov_mc_mixer_core_forward *data) {

  if (!data || (0 == data->ssrc) || (0 == data->payload_type) ||
      (0 == data->socket.port) || (UDP != data->socket.type) ||
      (0 == data->socket.host[0])) {

    ov_log_error("Invalid forward data.");
    return false;
  }

  return true;
}

/*----------------------------------------------------------------------------*/

static bool cb_io_forwarding_socket(int socket, uint8_t events, void *data) {

  ov_mc_mixer_core *self = ov_mc_mixer_core_cast(data);
  if (!self)
    goto error;

  if ((events & OV_EVENT_IO_CLOSE) || (events & OV_EVENT_IO_ERR)) {

    ov_log_info("Forwarding socket %i closed.", socket);
    return true;

  } else if (events & OV_EVENT_IO_IN) {

    // ignore any incoming IO at forwarding socket
  }

  return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

bool ov_mc_mixer_core_set_forward(ov_mc_mixer_core *self,
                                  ov_mc_mixer_core_forward data) {

  if (!self)
    return false;

  /* Forward data should be correct */
  if (!ov_mc_mixer_core_forward_data_is_valid(&data))
    return false;

  ov_event_loop *loop = self->config.loop;
  if (!loop)
    goto error;

  self->forward = data;
  self->output.payload_type = data.payload_type;
  self->output.ssid = data.ssrc;

  if (-1 != self->socket) {
    loop->callback.unset(loop, self->socket, NULL);
    close(self->socket);
    self->socket = -1;
  }

  ov_socket_configuration socket_config =
      (ov_socket_configuration){.host = "0.0.0.0", .port = 0, .type = UDP};

  self->socket = ov_socket_create(socket_config, false, NULL);
  if (-1 == self->socket)
    goto error;

  if (!ov_socket_ensure_nonblocking(self->socket))
    goto error;

  if (!ov_socket_get_data(self->socket, &self->local, NULL))
    goto error;

  if (!loop->callback.set(loop, self->socket,
                          OV_EVENT_IO_IN | OV_EVENT_IO_ERR | OV_EVENT_IO_CLOSE,
                          self, cb_io_forwarding_socket))
    goto error;

  return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

ov_mc_mixer_core_forward ov_mc_mixer_core_get_forward(ov_mc_mixer_core *self) {

  if (!self)
    return (ov_mc_mixer_core_forward){0};
  return self->forward;
}

/*----------------------------------------------------------------------------*/

static bool add_loop_data(const void *key, void *val, void *data) {

  if (!key)
    return true;
  ov_mc_loop *loop = (ov_mc_loop *)val;
  ov_json_value *store = ov_json_value_cast(data);
  if (!loop || !store)
    goto error;

  val = ov_mc_loop_to_json(loop);
  if (!ov_json_object_set(store, (const char *)key, val))
    goto error;

  return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_mc_mixer_state(ov_mc_mixer_core *self) {

  ov_json_value *out = NULL;
  ov_json_value *val = NULL;

  if (!self)
    goto error;

  pid_t pid = getpid();

  out = ov_json_object();

  val = ov_json_number(pid);
  if (!ov_json_object_set(out, "pid", val))
    goto error;

  val = ov_json_object();
  if (!ov_json_object_set(out, OV_KEY_FORWARD, val))
    goto error;

  ov_json_value *temp = val;

  val = ov_json_number(self->forward.ssrc);
  if (!ov_json_object_set(temp, OV_KEY_SSRC, val))
    goto error;

  val = ov_json_number(self->forward.socket.port);
  if (!ov_json_object_set(temp, OV_KEY_PORT, val))
    goto error;

  val = ov_json_string(self->forward.socket.host);
  if (!ov_json_object_set(temp, OV_KEY_HOST, val))
    goto error;

  val = ov_json_object();
  if (!ov_json_object_set(out, OV_KEY_LOOPS, val))
    goto error;

  if (!ov_dict_for_each(self->loops, val, add_loop_data))
    goto error;

  val = ov_json_object();
  if (!ov_json_object_set(out, OV_KEY_OUTPUT, val))
    goto error;

  temp = val;

  val = ov_json_number(self->output.sequence_number);
  if (!ov_json_object_set(temp, OV_KEY_SEQUENCE_NUMBER, val))
    goto error;

  val = ov_json_number(self->output.timestamp);
  if (!ov_json_object_set(temp, OV_KEY_TIMESTAMP, val))
    goto error;

  val = ov_json_number(self->output.payload_type);
  if (!ov_json_object_set(temp, OV_KEY_PAYLOAD_TYPE, val))
    goto error;

  val = ov_json_number(self->output.ssid);
  if (!ov_json_object_set(temp, OV_KEY_SSRC, val))
    goto error;

  return out;
error:
  ov_json_value_free(val);
  ov_json_value_free(out);
  return NULL;
}

/*****************************************************************************
                      Garbage collector for Codec database
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

    RtpStream *entry = value;

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

static void clean_stream_entries(ov_dict *codec_entries, gc_args args) {

  for (size_t i = 0; i < args.ssids_found; ++i) {

    ov_log_info("ALSA RTP mixer: Removing stale stream %" PRIu32,
                args.ssids[i]);
    intptr_t ssidptr = args.ssids[i];
    ov_dict_del(codec_entries, (void *)ssidptr);
  }
}

/*----------------------------------------------------------------------------*/

static bool codec_gc_run(ov_mc_mixer_core *self,
                         uint32_t max_stream_lifetime_secs) {

  ov_log_debug("Running the codec garbage collector...");

  gc_args args = {
      .before_epoch_secs = time(0) - max_stream_lifetime_secs,
  };

  ov_log_info("ALSA RTP mixer: Triggered GC run");

  if (ov_ptr_valid(self, "Cannot perform garbage collection - invalid ALSA RTP "
                         "mixer")) {

    ov_dict_for_each(self->codec.codecs, &args, collect_stale_stream_ssids);
    clean_stream_entries(self->codec.codecs, args);

    return true;

  } else {

    return false;
  }
}
