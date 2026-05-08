/***
        ------------------------------------------------------------------------

        Copyright (c) 2020 German Aerospace Center DLR e.V. (GSOC)

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

        @author         Michael J. Beer

        ------------------------------------------------------------------------
*/
/*----------------------------------------------------------------------------*/

#include <ov_base/ov_utils.h>

#include <ov_base/ov_constants.h>
#include <ov_base/ov_registered_cache.h>
#include <ov_pcm16s/ov_pcm16_mod.h>

#include "../include/ov_frame_data.h"

/*----------------------------------------------------------------------------*/

static ov_registered_cache *g_cache = 0;

#define FRAME_DATA_MAGIC_BYTES 0xa34c

static ov_frame_data *as_frame_data(void *vptr) {

    if (0 == vptr)
        return 0;

    ov_frame_data *data = vptr;

    if (FRAME_DATA_MAGIC_BYTES != data->magic_bytes)
        return 0;

    return data;
}

/*----------------------------------------------------------------------------*/

static void *free_frame_data(void *vptr) {

    if (0 == vptr)
        return vptr;

    ov_frame_data *data = as_frame_data(vptr);

    if (0 == data) {
        return data;
    }

    data->pcm16s_32bit = ov_buffer_free(data->pcm16s_32bit);

    free(data);

    return 0;
}

/*----------------------------------------------------------------------------*/

void ov_frame_data_enable_caching(size_t capacity) {

    ov_registered_cache_config cfg = {

        .capacity = capacity,
        .item_free = free_frame_data,

    };

    g_cache = ov_registered_cache_extend("frame_data", cfg);
}

/*----------------------------------------------------------------------------*/

ov_frame_data *ov_frame_data_create() {

    ov_frame_data *data = ov_registered_cache_get(g_cache);

    if (0 == data) {
        data = calloc(1, sizeof(ov_frame_data));
        data->magic_bytes = FRAME_DATA_MAGIC_BYTES;
    }

    OV_ASSERT(0 == data->pcm16s_32bit);

    return data;
}

/*----------------------------------------------------------------------------*/

ov_frame_data *ov_frame_data_free(ov_frame_data *data) {

    if (0 == data) {
        return 0;
    }

    data->pcm16s_32bit = ov_buffer_free(data->pcm16s_32bit);
    OV_ASSERT(0 == data->pcm16s_32bit);

    data = ov_registered_cache_put(g_cache, data);

    if (0 != data) {
        data = free_frame_data(data);
    }

    return data;
}

/*****************************************************************************
                                    DECODING
 ****************************************************************************/

// static ov_buffer *decode_nocheck(ov_ssid_translation const *trans,
//                                  ov_rtp_frame const *frame,
//                                  const size_t buflen_max) {
//
//     OV_ASSERT(0 != trans);
//     OV_ASSERT(0 != frame);
//
//     ov_codec *codec = (ov_codec *)trans->source.codec;
//
//     if (0 == codec) {
//         goto error;
//     }
//
//     ov_buffer *decoded = ov_buffer_create(buflen_max);
//     OV_ASSERT(0 != decoded);
//
//     decoded->length = ov_codec_decode(codec,
//                                       frame->expanded.sequence_number,
//                                       frame->expanded.payload.data,
//                                       frame->expanded.payload.length,
//                                       decoded->start,
//                                       decoded->capacity);
//
//     return decoded;
//
// error:
//
//     return 0;
// }

/*----------------------------------------------------------------------------*/

// static ov_buffer *scale_decoded_pcm_nocheck(ov_buffer const *pcm,
//                                             uint16_t volume,
//                                             ov_vad_parameters *vad,
//                                             int16_t *max_amplitude) {
//
//     OV_ASSERT(0 != pcm);
//
//     ov_buffer *result = 0;
//
//     size_t const num_samples = pcm->length / 2;
//
//     OV_ASSERT(num_samples * 2 == pcm->length);
//
//     const size_t result_length = sizeof(int32_t) * num_samples;
//
//     result = ov_buffer_create(result_length);
//
//     double scale_factor = volume;
//     scale_factor /= (double)UINT16_MAX;
//
//     if (!ov_pcm_16_scale_to_32(num_samples,
//                                (int16_t *)pcm->start,
//                                (int32_t *)result->start,
//                                scale_factor,
//                                vad,
//                                max_amplitude)) {
//
//         ov_log_error("Could not scale input buffer");
//         goto error;
//     }
//
//     result->length = result_length;
//
//     return result;
//
// error:
//
//     result = ov_buffer_free(result);
//     OV_ASSERT(0 == result);
//
//     return 0;
// }

/*----------------------------------------------------------------------------*/

// static ov_frame_data *frame_data_extract_nocheck(
//     ov_rtp_frame const *frame, ov_ssid_translation_table *sources) {
//
//     ov_frame_data *data = 0;
//
//     /* we require 16 bit == 2 uint8_ts per sample */
//     const size_t buflen_max =
//         2 * OV_MAX_FRAME_LENGTH_MS * OV_MAX_SAMPLERATE_HZ / 1000;
//
//     ov_buffer *decoded = 0;
//
//     if ((0 == frame) || (0 == frame->expanded.payload.length)) {
//         ov_log_warning("No payload in frame or no frame");
//         goto error;
//     }
//
//     const ov_ssid_translation *trans =
//         ov_ssid_translation_table_find_ssid(sources, frame->expanded.ssrc,
//         0);
//
//     if (0 == trans) {
//         ov_log_warning(
//             "no source found for sid: %" PRIu32, frame->expanded.ssrc);
//
//         goto error;
//     }
//
//     uint16_t source_volume = UINT16_MAX;
//
//     if (trans->in_use.source_volume) {
//         source_volume = trans->source.volume;
//     }
//
//     decoded = decode_nocheck(trans, frame, buflen_max);
//
//     if (!ov_ssid_translation_unlock(trans)) {
//
//         OV_ASSERT(!"DEADLOCK - the translation could not be unlocked "
//                    "again!");
//     }
//
//     trans = 0;
//
//     if ((0 == decoded) || (0 == decoded->length)) {
//         ov_log_error("No payload decoded");
//         goto error;
//     }
//
//     if (0 == decoded->length) {
//
//         ov_log_error(
//             "Decoded payload empty for %" PRIu32, frame->expanded.ssrc);
//         goto error;
//     }
//
//     data = ov_frame_data_create();
//
//     data->pcm16s_32bit = scale_decoded_pcm_nocheck(
//         decoded, source_volume, &data->vad_params, &data->max_amplitude);
//
//     decoded = ov_buffer_free(decoded);
//     OV_ASSERT(0 == decoded);
//
//     data->ssid = frame->expanded.ssrc;
//     data->sequence_number = frame->expanded.sequence_number;
//     data->timestamp = frame->expanded.timestamp;
//     data->payload_type = frame->expanded.payload_type;
//     data->num_samples = data->pcm16s_32bit->length / sizeof(int32_t);
//
//     OV_ASSERT(sizeof(int32_t) * data->num_samples ==
//               data->pcm16s_32bit->length);
//
//     return data;
//
// error:
//
//     decoded = ov_buffer_free(decoded);
//     data = ov_frame_data_free(data);
//
//     return 0;
// }

/*----------------------------------------------------------------------------*/

// ov_frame_data *ov_frame_data_extract(ov_rtp_frame const *frame,
//                                      ov_ssid_translation_table *sources) {
//
//     if ((0 == frame) || (0 == frame->expanded.payload.length)) {
//
//         ov_log_error("No payload in frame or no frame");
//         goto error;
//     }
//
//     if (0 == sources) {
//
//         ov_log_error("No sources table given");
//         goto error;
//     }
//
//     return frame_data_extract_nocheck(frame, sources);
//
// error:
//
//     return false;
// }

/*****************************************************************************
                                    ENCODING
 ****************************************************************************/

// static bool adjust_to_destination_volume_nocheck(
//     size_t num_samples,
//     ov_frame_data *md,
//     ov_ssid_translation const *destination) {
//
//     OV_ASSERT(0 != md);
//     OV_ASSERT(0 != destination);
//
//     if (!destination->in_use.dest_volume) {
//
//         return true;
//     }
//
//     uint16_t vol = destination->dest.volume;
//
//     OV_ASSERT(sizeof(int32_t) * num_samples == md->pcm16s_32bit->length);
//
//     double scale_factor = vol;
//     scale_factor /= (double)UINT16_MAX;
//
//     return ov_pcm_32_scale(
//         num_samples, (int32_t *)md->pcm16s_32bit->start, scale_factor);
// }

/*----------------------------------------------------------------------------*/

static ov_rtp_frame *encode_frame(ov_frame_data const *frame_data,
                                  ov_codec *codec, bool mark) {

    if ((!ov_ptr_valid(codec, "Cannot encode frame data: No codec")) ||
        (!ov_ptr_valid(frame_data, "Cannot encode frame data: No data")) ||
        (!ov_ptr_valid(frame_data->pcm16s_32bit,
                       "Cannot encode frame data: No data"))) {
        return 0;
    }

    OV_ASSERT(0 != codec->encode);

    ov_buffer *out = 0;

    const size_t num_samples =
        frame_data->pcm16s_32bit->length / sizeof(int32_t);

    // First step: 32bit -> 16bit
    ov_buffer *reduced = ov_buffer_create(num_samples * sizeof(int16_t));
    OV_ASSERT(num_samples * sizeof(int16_t) <= reduced->capacity);

    reduced->length = num_samples * sizeof(int16_t);

    ov_pcm_32_clip_to_16(num_samples,
                         (int32_t *)frame_data->pcm16s_32bit->start,
                         (int16_t *)reduced->start);

    /* The codec will never encode to more bytes than the input, or will it ?*/
    out = ov_buffer_create(frame_data->pcm16s_32bit->length);

    int32_t encoded_bytes = ov_codec_encode(
        codec, reduced->start, reduced->length, out->start, out->capacity);

    reduced = ov_buffer_free(reduced);
    OV_ASSERT(0 == reduced);

    if (0 > encoded_bytes) {
        ov_log_error("Could not encode payload");
        goto error;
    }

    OV_ASSERT(-1 < encoded_bytes);

    ov_rtp_frame_expansion exp = {

        .version = RTP_VERSION_2,
        /* This stuff needs to come from mixed_data  in the end*/
        .payload_type = frame_data->payload_type,
        .sequence_number = frame_data->sequence_number,
        .timestamp = frame_data->timestamp,
        .ssrc = frame_data->ssid,

        .marker_bit = mark,

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

ov_rtp_frame *ov_frame_data_encode_with_codec(ov_frame_data *frame_data,
                                              ov_codec *out_codec) {

    if (0 == frame_data) {

        ov_log_error("Not frame data to encode (0 pointer)");
        return 0;
    }

    if (0 == out_codec) {

        ov_log_error("No Codec to encode audio (0 -pointer)");
        return 0;
    }

    return encode_frame(frame_data, out_codec, false);
}

/*----------------------------------------------------------------------------*/

// ov_rtp_frame *ov_frame_data_encode(ov_frame_data *frame_data,
//                                    ov_ssid_translation const *destination) {
//
//     ov_rtp_frame *encoded_frame = 0;
//
//     if (0 == frame_data) {
//
//         ov_log_error("No frame_data given");
//         goto error;
//     }
//
//     if (0 == destination) {
//
//         ov_log_error("No destination given");
//         goto error;
//     }
//
//     const size_t num_samples =
//         frame_data->pcm16s_32bit->length / sizeof(int32_t);
//
//     adjust_to_destination_volume_nocheck(num_samples, frame_data,
//     destination);
//
//     if (0 != destination->dest.payload_type) {
//         frame_data->payload_type = destination->dest.payload_type;
//     }
//
//     ov_codec *codec = destination->dest.codec;
//
//     if (0 == codec) {
//         ov_log_error("Destination %s has NO codec attached",
//         destination->id); goto error;
//     }
//
//     encoded_frame =
//         encode_frame(frame_data, codec, !destination->stream_initialized);
//
//     if (0 == encoded_frame) {
//
//         ov_log_error(
//             "Could not encode frame bound for destination %s",
//             destination->id);
//         goto error;
//     }
//
//     return encoded_frame;
//
// error:
//
//     encoded_frame = ov_rtp_frame_free(encoded_frame);
//     OV_ASSERT(0 == encoded_frame);
//
//     return 0;
// }
