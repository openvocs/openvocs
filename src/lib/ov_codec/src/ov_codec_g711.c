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

     \file               ov_codec_g711.c
     \author             Michael J. Beer, DLR/GSOC <michael.beer@dlr.de>
     \date               2018-06-05

     \ingroup            empty

     \brief              empty

 **/

#include "../include/ov_codec_g711.h"
#include <ov_base/ov_json.h>
#include <ov_base/ov_utils.h>

/*---------------------------------------------------------------------------*/

/* Ensures same memory content on all archictectures */
static const uint8_t MAGIC_NUMBER_CHARS[] = {'7', '1', '1', 0};

#define MAGIC_NUMBER (*(uint32_t *)&MAGIC_NUMBER_CHARS[0])

#define CONFIG_KEY_G711_LAW "law"
#define CONFIG_KEY_G711_ALAW "alaw"
#define CONFIG_KEY_G711_ULAW "ulaw"

/******************************************************************************
 * PCM16 Signed CODEC struct
 ******************************************************************************/

struct codec_g711_struct {

    ov_codec codec;

    uint64_t last_seq_number;
    uint8_t (*compress)(int16_t sample);
    int16_t (*expand)(uint8_t sample);
};

typedef struct codec_g711_struct codec_g711;

/*---------------------------------------------------------------------------*/

typedef enum { ULAW, ALAW, INVALID } Law;

/******************************************************************************
 *                             PRIVATE PROTOTYPES
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

static ov_json_value *impl_get_parameters(const ov_codec *);

static uint32_t impl_get_samplerate_hertz(const ov_codec *);

static Law law_from_parameters(const ov_json_value *parameters);

static int16_t alaw_expand(const uint8_t x);
static uint8_t alaw_compress(const int16_t x);
static uint8_t alaw_get_exponent(const uint16_t x);

static int16_t ulaw_expand(uint8_t x);
static uint8_t ulaw_compress(int16_t x);

/******************************************************************************
 *                              PUBLIC FUNCTIONS
 ******************************************************************************/

const char *ov_codec_g711_id() { return "G.711"; }

/*---------------------------------------------------------------------------*/

ov_codec_generator ov_codec_g711_install(ov_codec_factory *factory) {

    if (0 == factory) goto error;

    return ov_codec_factory_install_codec(
        factory, ov_codec_g711_id(), impl_codec_create);

error:

    return 0;
}

/******************************************************************************
 *                             PRIVATE FUNCTIONS
 ******************************************************************************/

static int8_t impl_rtp_payload_type_alaw(ov_codec const *codec) {
    UNUSED(codec);
    return 8;
}

/*----------------------------------------------------------------------------*/

static int8_t impl_rtp_payload_type_ulaw(ov_codec const *codec) {
    UNUSED(codec);
    return 0;
}

/*----------------------------------------------------------------------------*/

static ov_codec *impl_codec_create(uint32_t ssid,
                                   const ov_json_value *parameters) {

    UNUSED(ssid);

    struct codec_g711_struct *g711 = 0;

    Law law = law_from_parameters(parameters);

    if (INVALID == law) goto error;

    g711 = calloc(1, sizeof(struct codec_g711_struct));

    ov_codec *public = &g711->codec;

    *public = (ov_codec){

        .type = MAGIC_NUMBER,
        .type_id = ov_codec_g711_id,
        .free = impl_free,
        .encode = impl_encode,
        .decode = impl_decode,
        .get_parameters = impl_get_parameters,
        .get_samplerate_hertz = impl_get_samplerate_hertz,
    };

    switch (law) {

        case ULAW:
            g711->expand = ulaw_expand;
            g711->compress = ulaw_compress;
            g711->codec.rtp_payload_type = impl_rtp_payload_type_ulaw;
            break;

        case ALAW:
            g711->expand = alaw_expand;
            g711->compress = alaw_compress;
            g711->codec.rtp_payload_type = impl_rtp_payload_type_alaw;
            break;

        default:

            goto error;
    };

    return (ov_codec *)g711;

error:

    if (0 != g711) impl_free((ov_codec *)g711);

    return 0;
}

/*---------------------------------------------------------------------------*/

static ov_codec *impl_free(ov_codec *self) {

    if (0 == self) goto error;

    free(self);

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

    codec_g711 *g711 = (codec_g711 *)self;

    if (0 == g711->compress) {

        ov_log_error("Encoder was no initialized!");
        goto error;
    }

    size_t samples_to_write = num_samples;
    if (samples_to_write > max_out_length) {
        ov_log_error("encode buffer too small");
        goto error;
    }

    int16_t *in = (int16_t *)input;

    for (size_t i = 0; i < samples_to_write; ++i) {

        output[i] = g711->compress(in[i]);
    }

    return samples_to_write;

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

    UNUSED(seq_number);

    if (0 == self) goto error;
    if (0 == input) goto error;
    if (0 == output) goto error;

    if (0 == length) goto error;

    if (MAGIC_NUMBER != self->type) {

        ov_log_error("Wrong codec type received");
        goto error;
    }

    codec_g711 *g711 = (codec_g711 *)self;

    if (0 == g711->expand) {

        ov_log_error("Codec misconfigured");
        goto error;
    }

    if ((max_out_length == 0) || (max_out_length > 2 * (unsigned)INT_MAX)) {

        ov_log_error("Decode buffer length out of range");
        goto error;
    }

    size_t samples_to_decode = max_out_length / 2;

    if (samples_to_decode * 2 != max_out_length) {

        ov_log_error("Decode buffer length no multiple of 2");
        goto error;
    }

    if (samples_to_decode > length) {
        samples_to_decode = length;
    }

    if (samples_to_decode < length) {
        ov_log_error("Decode buffer too small");
        goto error;
    }

    int16_t *out_16 = (int16_t *)output;

    for (size_t i = 0; i < samples_to_decode; ++i) {
        out_16[i] = g711->expand(input[i]);
    }

    return samples_to_decode * 2;

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

    codec_g711 *g711 = (codec_g711 *)self;

    const char *law = 0;

    if (alaw_expand == g711->expand) {
        law = CONFIG_KEY_G711_ALAW;
    }

    if (ulaw_expand == g711->expand) {
        law = CONFIG_KEY_G711_ULAW;
    }

    if (0 == law) {
        ov_log_error("Codec misconfigured: neither A nor U law");
        goto error;
    }

    ov_json_value *json = ov_json_object();

    ov_json_object_set(json, CONFIG_KEY_G711_LAW, ov_json_string(law));

    return json;

error:

    return 0;
}

/*---------------------------------------------------------------------------*/

static uint32_t impl_get_samplerate_hertz(const ov_codec *self) {

    UNUSED(self);

    /* The standard defines G.711 to operate at 8000 Hz */
    return 8000;
}

/*----------------------------------------------------------------------------*/

static Law law_from_parameters(const ov_json_value *parameters) {

    if (0 == parameters) return ULAW;

    char const *law_string =
        ov_json_string_get(ov_json_get(parameters, "/" CONFIG_KEY_G711_LAW));

    if (0 == law_string) return ULAW;

    Law law = INVALID;

    if (0 == strcmp(law_string, CONFIG_KEY_G711_ULAW))
        law = ULAW;
    else if (0 == strcmp(law_string, CONFIG_KEY_G711_ALAW))
        law = ALAW;

    law_string = 0;

    return law;
}

/******************************************************************************
 *                                CODING STUFF
 ******************************************************************************/

#define HIGH_ORDER_BIT ((uint8_t)0x80)

#define EVEN_BITS ((uint8_t)0x01 | 0x04 | 0x10 | 0x40)

#define MANTISSA_MASK ((uint8_t)0x0f)
#define EXPONENT_MASK ((uint8_t)0x70)

/*---------------------------------------------------------------------------*
 *                           Byte-order independent
 *---------------------------------------------------------------------------*/

#define ODD_BITS ((uint8_t)~EVEN_BITS & 0x7F)

/*---------------------------------------------------------------------------*/

#define INVERT_EVEN_BITS(x) ((x) ^ EVEN_BITS)
#define REMOVE_HIGH_ORDER_BIT(x) ((x) & ~HIGH_ORDER_BIT)

#define A_LAW_SIGN(x)                                                          \
    (((x & HIGH_ORDER_BIT) == HIGH_ORDER_BIT) ? (int8_t)1 : (int8_t) - 1)

/*----------------------------------------------------------------------------*
 *                                   A-Law
 *----------------------------------------------------------------------------*/

static int16_t alaw_expand(uint8_t x) {

    int8_t s = A_LAW_SIGN(x);

    uint8_t intermediate = INVERT_EVEN_BITS(x);

    uint16_t mantissa = intermediate & MANTISSA_MASK;
    uint16_t exponent = intermediate & EXPONENT_MASK;
    exponent = exponent >> 4;

    if (0 < exponent) mantissa += 0x10;

    if (0 == exponent) exponent = 1;

    uint16_t result_abs = mantissa << exponent;

    result_abs += 1 << (exponent - 1);

    OV_ASSERT(result_abs < 0x8000);

    int16_t res = (int16_t)result_abs;

    return s * res;
}

/*---------------------------------------------------------------------------*/

static uint8_t alaw_compress(int16_t x) {

    if (x > 4032) return INVERT_EVEN_BITS(0xff);
    if (x < -4032) return INVERT_EVEN_BITS(0x7f);

    int s = 1;

    if (0 > x) {
        x = -x;
        s = -1;
    }

    OV_ASSERT(-1 < x);

    uint16_t abs = (uint16_t)x;

    OV_ASSERT(0x1000 > abs);

    uint8_t exp = alaw_get_exponent(abs);
    uint8_t shift = exp;

    if (0 == exp) shift = 1;

    OV_ASSERT(1 << 4 > exp);
    OV_ASSERT(8 > shift);

    abs = abs >> shift;
    abs &= 0x0f;

    uint8_t compressed = (uint8_t)abs;
    compressed |= exp << 4;

    if (0 < s) compressed |= HIGH_ORDER_BIT;

    return INVERT_EVEN_BITS(compressed);
}

/*---------------------------------------------------------------------------*/

static uint8_t alaw_get_exponent(uint16_t x) {

    OV_ASSERT(0x1000 > x);
    x &= 0x7fff; /* Ensure MSB is 0 */

    x = x >> 5;

    uint8_t exp = 0;

    while (0 < x) {
        ++exp;
        x = x >> 1;
    };

    return exp;
}

/*----------------------------------------------------------------------------*
  u-Law
 *----------------------------------------------------------------------------*/

#define U_LAW_SIGN(x)                                                          \
    (((x & HIGH_ORDER_BIT) == HIGH_ORDER_BIT) ? (int8_t) - 1 : (int8_t)1)

/*---------------------------------------------------------------------------*/

static int16_t ulaw_expand(uint8_t x) {

    x ^= 0xff;

    int8_t s = U_LAW_SIGN(x);

    uint16_t exponent = x & EXPONENT_MASK;
    exponent = exponent >> 4;

    uint16_t mantissa = x & MANTISSA_MASK;
    mantissa = mantissa << 1;
    mantissa |= 0x21;

    mantissa = mantissa << exponent;

    int16_t result = mantissa;
    result -= 0x21;

    return s * result;
}

/*---------------------------------------------------------------------------*/

static uint8_t ulaw_compress(int16_t x) {

    if (8158 < x) return 0x80;
    if (-8158 > x) return 0x00;

    int8_t s = 1;

    if (0 > x) {
        s = -1;
        x = -x;
    }

    uint16_t u = x;

    /* 8159 == '1111111011111' == 0x1fdf */
    OV_ASSERT(8159 == 0x1fdf);
    OV_ASSERT(8159 > u);

    u += 0x21;

    /* This ensures that the value of u is actually at most 14 bits wide: */
    OV_ASSERT(0x1fdf + 0x21 == 0x2000); /* ---> */
    OV_ASSERT(0x2000 > u);              /* (1) */

    OV_ASSERT(0x40 << 7 == 0x2000); /* from (1) and this ----> */
    OV_ASSERT(0x40 << 7 > u);

    uint8_t exp = 0;

    while (0x40 <= u) {
        ++exp;
        u = u >> 1;
    }

    OV_ASSERT(0x40 >= u);

    /*     limit u satisfied     max of u      upper limit of exp */
    OV_ASSERT(0x2000 == (0x40 << 7));
    OV_ASSERT(7 >= exp);

    /* We got something like '001a bcdx' which encodes to '0000 abcd' */
    u = u >> 1;
    u &= 0x0f;

    /* We got '0000 abcd', We need to add the exponent bits: '0EXP abcd' */
    uint8_t result = u | (exp << 4);

    /* Set the SIGN bit if negative */
    if (0 > s) result |= HIGH_ORDER_BIT;

    /* And invert */
    return result ^ 0xff;
}

/*---------------------------------------------------------------------------*/
