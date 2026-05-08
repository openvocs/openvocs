/***

Copyright   2017        German Aerospace Center DLR e.V.,
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

     \file               ov_codec_pcm16_signed.c
     \author             Michael J. Beer, DLR/GSOC <michael.beer@dlr.de>
     \date               2018-01-08

     \ingroup            empty

     \brief              empty

 **/

#include <ov_arch/ov_byteorder.h>
#include <ov_base/ov_config_keys.h>
#include <ov_base/ov_json.h>
#include <ov_base/ov_utils.h>

#include "../include/ov_codec_pcm16_signed.h"
#include <ov_base/ov_utils.h>

/*---------------------------------------------------------------------------*/

/* Ensures same memory content on all archictectures */
static const uint8_t MAGIC_NUMBER_CHARS[] = {'p', '2', 's', 0};

#define MAGIC_NUMBER (*(uint32_t *)&MAGIC_NUMBER_CHARS[0])

/******************************************************************************
 * PCM16 Signed CODEC struct
 ******************************************************************************/

struct ov_codec_pcm16_signed_struct {

    ov_codec codec;

    uint64_t last_seq_number;
};

/******************************************************************************
 * PRIVATE FUNCTIONS - PROTOTYPES
 ******************************************************************************/

static ov_codec *impl_codec_create(uint32_t ssid,
                                   const ov_json_value *parameters);

static ov_codec *impl_free(ov_codec *self);

static int32_t impl_encode_be(ov_codec *self, const uint8_t *input,
                              size_t length, uint8_t *output,
                              size_t max_out_length);

static int32_t impl_decode_be(ov_codec *self, uint64_t seq_number,
                              const uint8_t *input, size_t length,
                              uint8_t *output, size_t max_out_length);

static int32_t impl_encode_le(ov_codec *self, const uint8_t *input,
                              size_t length, uint8_t *output,
                              size_t max_out_length);

static int32_t impl_decode_le(ov_codec *self, uint64_t seq_number,
                              const uint8_t *input, size_t length,
                              uint8_t *output, size_t max_out_length);

static ov_json_value *impl_get_parameters(const ov_codec *self);

/******************************************************************************
 * PUBLIC FUNCTIONS
 ******************************************************************************/

const char *ov_codec_pcm16_signed_id() { return "pcm16_signed"; }

/*---------------------------------------------------------------------------*/

ov_codec_generator ov_codec_pcm16_signed_install(ov_codec_factory *factory) {

    if (0 == factory)
        goto error;

    return ov_codec_factory_install_codec(factory, ov_codec_pcm16_signed_id(),
                                          impl_codec_create);

error:

    return 0;
}

/******************************************************************************
 * PRIVATE FUNCTIONS
 ******************************************************************************/

static ov_codec *impl_codec_create(uint32_t ssid,
                                   const ov_json_value *parameters) {

    UNUSED(ssid);

    char const *endianness = OV_KEY_BIG_ENDIAN;

    if (0 != parameters) {

        endianness =
            ov_json_string_get(ov_json_get(parameters, "/" OV_KEY_ENDIANNESS));
    }

    ov_codec *codec = calloc(1, sizeof(struct ov_codec_pcm16_signed_struct));

    codec->type = MAGIC_NUMBER;

    codec->type_id = ov_codec_pcm16_signed_id;

    codec->free = impl_free;
    codec->get_parameters = impl_get_parameters;

    if ((0 == endianness) || (0 == strcmp(OV_KEY_BIG_ENDIAN, endianness))) {
        codec->encode = impl_encode_be;
        codec->decode = impl_decode_be;
        return codec;
    }

    if ((0 != endianness) && (0 == strcmp(OV_KEY_LITTLE_ENDIAN, endianness))) {
        codec->encode = impl_encode_le;
        codec->decode = impl_decode_le;
        return codec;
    }

    /* Error */

    if (0 != codec) {
        free(codec);
        codec = 0;
    }

    OV_ASSERT(0 == codec);

    return 0;
}

/*---------------------------------------------------------------------------*/

static ov_codec *impl_free(ov_codec *self) {

    if (0 == self)
        return 0;

    if (MAGIC_NUMBER != self->type) {

        ov_log_error("Called on invalid codec");
        goto error;
    }

    free(self);

error:

    return 0;
}

/*---------------------------------------------------------------------------*/

static int32_t impl_encode_be(ov_codec *self, const uint8_t *input,
                              size_t length, uint8_t *output,
                              size_t max_out_length) {

    if (0 == self)
        goto error;
    if (0 == input)
        goto error;
    if (0 == output)
        goto error;

    if (0 == length)
        goto error;

    if (MAGIC_NUMBER != self->type) {

        ov_log_error("Wrong codec type received");
        goto error;
    }

    if (length > max_out_length) {

        ov_log_error("Output buffer not large enough. Input is %zu, "
                     "output %zu",
                     length, max_out_length);
        goto error;
    }

    size_t length_16bit = length / 2;

    if (2 * length_16bit != length) {

        ov_log_error("Need an array of 16 bit values - "
                     "but array has odd number of bytes");
        goto error;
    }

    if (output != memcpy(output, input, length)) {

        ov_log_error("Severe error: Could not copy input to output "
                     "buffer");
        goto error;
    }

    if (!ov_byteorder_to_big_endian_16_bit((int16_t *)output, length_16bit)) {

        ov_log_error("Could not encode to big endian");
        goto error;
    }

    return length;

error:

    return 0;
}

/*---------------------------------------------------------------------------*/

static int32_t impl_decode_be(ov_codec *self, uint64_t seq_number,
                              const uint8_t *input, size_t length,
                              uint8_t *output, size_t max_out_length) {

    if (0 == self)
        goto error;
    if (0 == input)
        goto error;
    if (0 == output)
        goto error;

    if (0 == length)
        goto error;

    if (MAGIC_NUMBER != self->type) {

        ov_log_error("Wrong codec type received");
        goto error;
    }

    if (length > max_out_length) {

        ov_log_error("Output buffer not large enough. Input is %zu, "
                     "output %zu",
                     length, max_out_length);
        goto error;
    }

    size_t length_16bit = length / 2;

    if (2 * length_16bit != length) {

        ov_log_error("Need an array of 16 bit values - "
                     "but array has odd number of bytes");
        goto error;
    }

    struct ov_codec_pcm16_signed_struct *c =
        (struct ov_codec_pcm16_signed_struct *)self;

    if (1 + c->last_seq_number != seq_number) {

        ov_log_warning("Package Loss detected");
    }

    c->last_seq_number = seq_number;

    if (output != memcpy(output, input, length)) {

        ov_log_error("Severe error: Could not copy input to output "
                     "buffer");
        goto error;
    }

    if (!ov_byteorder_from_big_endian_16_bit((int16_t *)output, length_16bit)) {

        ov_log_error("Could not decode from big endian");
        goto error;
    }

    return length;

error:

    return 0;
}

/*---------------------------------------------------------------------------*/

static int32_t impl_encode_le(ov_codec *self, const uint8_t *input,
                              size_t length, uint8_t *output,
                              size_t max_out_length) {

    if (0 == self)
        goto error;
    if (0 == input)
        goto error;
    if (0 == output)
        goto error;

    if (0 == length)
        goto error;

    if (MAGIC_NUMBER != self->type) {

        ov_log_error("Wrong codec type received");
        goto error;
    }

    if (length > max_out_length) {

        ov_log_error("Output buffer not large enough. Input is %zu, "
                     "output %zu",
                     length, max_out_length);
        goto error;
    }

    size_t length_16bit = length / 2;

    if (2 * length_16bit != length) {

        ov_log_error("Need an array of 16 bit values - "
                     "but array has odd number of bytes");
        goto error;
    }

    if (output != memcpy(output, input, length)) {

        ov_log_error("Severe error: Could not copy input to output "
                     "buffer");
        goto error;
    }

    if (!ov_byteorder_to_little_endian_16_bit((int16_t *)output,
                                              length_16bit)) {

        ov_log_error("Could not encode to big endian");
        goto error;
    }

    return length;

error:

    return 0;
}

/*---------------------------------------------------------------------------*/

static int32_t impl_decode_le(ov_codec *self, uint64_t seq_number,
                              const uint8_t *input, size_t length,
                              uint8_t *output, size_t max_out_length) {

    if (0 == self)
        goto error;
    if (0 == input)
        goto error;
    if (0 == output)
        goto error;

    if (0 == length)
        goto error;

    if (MAGIC_NUMBER != self->type) {

        ov_log_error("Wrong codec type received");
        goto error;
    }

    if (length > max_out_length) {

        ov_log_error("Output buffer not large enough. Input is %zu, "
                     "output %zu",
                     length, max_out_length);
        goto error;
    }

    size_t length_16bit = length / 2;

    if (2 * length_16bit != length) {

        ov_log_error("Need an array of 16 bit values - "
                     "but array has odd number of bytes");
        goto error;
    }

    struct ov_codec_pcm16_signed_struct *c =
        (struct ov_codec_pcm16_signed_struct *)self;

    if (1 + c->last_seq_number != seq_number) {

        ov_log_warning("Package Loss detected");
    }

    c->last_seq_number = seq_number;

    if (output != memcpy(output, input, length)) {

        ov_log_error("Severe error: Could not copy input to output "
                     "buffer");
        goto error;
    }

    if (!ov_byteorder_from_little_endian_16_bit((int16_t *)output,
                                                length_16bit)) {

        ov_log_error("Could not decode from big endian");
        goto error;
    }

    return length;

error:

    return 0;
}
/*---------------------------------------------------------------------------*/

static ov_json_value *impl_get_parameters(const ov_codec *self) {

    if (0 == self)
        goto error;
    if (MAGIC_NUMBER != self->type) {

        ov_log_error("Called on invalid codec");
        goto error;
    }

    const char *endianness = 0;

    if ((impl_decode_le == self->decode) && (impl_encode_le == self->encode)) {

        endianness = "{\"" OV_KEY_ENDIANNESS "\":\"" OV_KEY_LITTLE_ENDIAN "\"}";
    }

    if ((impl_decode_be == self->decode) && (impl_encode_be == self->encode)) {

        endianness = "{\"" OV_KEY_ENDIANNESS "\":\"" OV_KEY_BIG_ENDIAN "\"}";
    }

    if (0 == endianness) {

        ov_log_error("Not a valid pcm codec");
        goto error;
    }

    return ov_json_value_from_string(endianness, strlen(endianness));

error:

    return 0;
}

/*---------------------------------------------------------------------------*/
