/***

Copyright   2018        German Aerospace Center DLR e.V.,
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

     \file               ov_codec_opus.c
     \author             Michael J. Beer, DLR/GSOC <michael.beer@dlr.de>
     \date               2018-01-08

     \ingroup            empty

     \brief              empty

 **/

#include "../include/ov_codec_opus.h"
#include <ov_base/ov_utils.h>

#include <opus.h>
/*---------------------------------------------------------------------------*/

/* Ensures same memory content on all archictectures */
static const uint8_t MAGIC_NUMBER_CHARS[] = {'o', 'p', 's', 0};

#define MAGIC_NUMBER (*(uint32_t *)&MAGIC_NUMBER_CHARS[0])

/******************************************************************************
 * PCM16 Signed CODEC struct
 ******************************************************************************/

typedef struct {

    ov_codec codec;

    uint64_t last_seq_number;
    uint32_t sample_rate_hertz;

    OpusDecoder *decoder;
    OpusEncoder *encoder;

} ov_codec_opus;

/******************************************************************************
 * PRIVATE FUNCTIONS - PROTOTYPES
 ******************************************************************************/

static ov_codec *impl_codec_create(uint32_t ssid,
                                   const ov_json_value *parameters);

static ov_codec *impl_free(ov_codec *self);

static int32_t impl_encode(ov_codec *self,
                           const uint8_t *input,
                           size_t length,
                           uint8_t *output,
                           size_t max_out_length);

static int32_t impl_decode(ov_codec *self,
                           uint64_t seq_number,
                           const uint8_t *input,
                           size_t length,
                           uint8_t *output,
                           size_t max_out_length);

static ov_json_value *impl_get_parameters(const ov_codec *self);
static uint32_t impl_get_samplerate_hertz(const ov_codec *self);

/******************************************************************************
 * PUBLIC FUNCTIONS
 ******************************************************************************/

const char *ov_codec_opus_id() { return "opus"; }

/*---------------------------------------------------------------------------*/

ov_codec_generator ov_codec_opus_install(ov_codec_factory *factory) {

    if (0 == factory) goto error;

    return ov_codec_factory_install_codec(
        factory, ov_codec_opus_id(), impl_codec_create);

error:

    return 0;
}

/******************************************************************************
 * PRIVATE FUNCTIONS
 ******************************************************************************/

static ov_codec *impl_codec_create(uint32_t ssid,
                                   const ov_json_value *parameters) {

    UNUSED(ssid);
    int error = 0;

    ov_codec_opus *opus = calloc(1, sizeof(ov_codec_opus));
    ov_codec *codec = (ov_codec *)opus;

    codec->type = MAGIC_NUMBER;

    codec->type_id = ov_codec_opus_id;

    opus->sample_rate_hertz =
        ov_codec_parameters_get_sample_rate_hertz(parameters);

    if (INT32_MAX < opus->sample_rate_hertz) {

        ov_log_error(
            "Could not create Opus codec -  sample rate out of bounds: "
            "%" PRIu32,
            opus->sample_rate_hertz);
        goto error;
    }

    ov_log_info("Using sample rate (Hz): %" PRIu32, opus->sample_rate_hertz);

    opus->decoder = opus_decoder_create(opus->sample_rate_hertz, 1, &error);

    if (OPUS_OK != error) {

        ov_log_error("Could not create Opus  decoder");
        goto error;
    }

    opus->encoder = opus_encoder_create(
        opus->sample_rate_hertz, 1, OPUS_APPLICATION_VOIP, &error);

    if (OPUS_OK != error) {

        ov_log_error("Could not create Opus  encoder");
        goto error;
    }

    codec->free = impl_free;
    codec->encode = impl_encode;
    codec->decode = impl_decode;
    codec->get_parameters = impl_get_parameters;
    codec->get_samplerate_hertz = impl_get_samplerate_hertz;

    return codec;

error:

    if (codec) {

        codec = impl_free(codec);
    }

    ov_log_error("Could not create OPUS codec: %s\n", opus_strerror(error));

    return codec;
}

/*---------------------------------------------------------------------------*/

static ov_codec *impl_free(ov_codec *self) {

    if (0 == self) return 0;

    if (MAGIC_NUMBER != self->type) {

        ov_log_error("Called on invalid codec");
        goto error;
    }

    ov_codec_opus *codec = (ov_codec_opus *)self;

    if (codec->encoder) {

        opus_encoder_destroy(codec->encoder);
        codec->encoder = 0;
    }

    if (codec->decoder) {

        opus_decoder_destroy(codec->decoder);
        codec->decoder = 0;
    }

    free(self);
    self = 0;

    return 0;

error:

    return self;
}

/*---------------------------------------------------------------------------*/

static int32_t impl_encode(ov_codec *self,
                           const uint8_t *input,
                           size_t length,
                           uint8_t *output,
                           size_t max_out_length) {

    if (0 == self) goto error;
    if (0 == input) goto error;
    if (0 == output) goto error;

    if ((0 == length) || (2 * (unsigned)INT_MAX < length)) {

        ov_log_error("input buffer length out of range");
        goto error;
    }

    if (MAGIC_NUMBER != self->type) {

        ov_log_error("Wrong codec type received");
        goto error;
    }

    unsigned num_samples = length / 2;

    if (num_samples * 2 != length) {

        ov_log_error("Expect input to have a length multiple to 2");
        goto error;
    }

    ov_codec_opus *opus_codec = (ov_codec_opus *)self;

    if (0 == opus_codec->encoder) {

        ov_log_error("Encoder was not initialized!");
        goto error;
    }

    int encoded_bytes = opus_encode(opus_codec->encoder,
                                    (opus_int16 *)input,
                                    num_samples,
                                    output,
                                    max_out_length);

    if (0 > encoded_bytes) {

        ov_log_error(
            "Could not encode frame: %s", opus_strerror(encoded_bytes));

        goto error;
    }

    return encoded_bytes;

error:

    return 0;
}

/*---------------------------------------------------------------------------*/

static int32_t impl_decode(ov_codec *self,
                           uint64_t seq_number,
                           const uint8_t *input,
                           size_t length,
                           uint8_t *output,
                           size_t max_out_length) {

    if (0 == self) goto error;
    if (0 == input) goto error;
    if (0 == output) goto error;

    if (0 == length) goto error;

    if (MAGIC_NUMBER != self->type) {

        ov_log_error("Wrong codec type received");
        goto error;
    }

    ov_codec_opus *codec = (ov_codec_opus *)self;

    if (0 == codec->decoder) {

        ov_log_error("Decoder not initialized");
        goto error;
    }

    if ((max_out_length == 0) || (max_out_length > 2 * (unsigned)INT_MAX)) {

        ov_log_error("Output buffer length out of range");
        goto error;
    }

    /*
        const int num_channels = opus_packet_get_nb_channels(input);

        if (1 != num_channels) {
            ov_log_info("Opus Codec: Received more than 1 channel in input
       (%i)", num_channels);
        }
    */

    int max_num_samples = max_out_length / 2;

    /*
        if ( (0 != codec->last_seq_number) &&
             (1 + codec->last_seq_number != seq_number)) {

            // 1 or more frames dropped - reset decoder
            int r = opus_decode(codec->decoder,
                                NULL,
                                length,
                                (opus_int16 *)output,
                                max_num_samples,
                                0);

            if (0 > r) {

                ov_log_error(
                    "Could not reset OPUS codec after frame "
                    "drops %i", r);
                goto error;
            }
        }
    */

    int samples_decoded = opus_decode(codec->decoder,
                                      input,
                                      length,
                                      (opus_int16 *)output,
                                      max_num_samples,
                                      0);

    if (0 > samples_decoded) {

        ov_log_error(
            "Could not decode frame: %s", opus_strerror(samples_decoded));

        goto error;
    }

    codec->last_seq_number = seq_number;

    return 2 * samples_decoded;

error:

    return 0;
}

/*---------------------------------------------------------------------------*/

static ov_json_value *impl_get_parameters(const ov_codec *self) {

    if (0 == self) goto error;
    if (MAGIC_NUMBER != self->type) {

        ov_log_error("Called on invalid codec");
        goto error;
    }

    ov_codec_opus *opus = (ov_codec_opus *)self;

    ov_json_value *json = ov_json_object();

    ov_codec_parameters_set_sample_rate_hertz(json, opus->sample_rate_hertz);

    return json;

error:

    return 0;
}

/*---------------------------------------------------------------------------*/

static uint32_t impl_get_samplerate_hertz(const ov_codec *self) {

    const ov_codec_opus *opus = (ov_codec_opus const *)self;

    return opus->sample_rate_hertz;
}

/*----------------------------------------------------------------------------*/
