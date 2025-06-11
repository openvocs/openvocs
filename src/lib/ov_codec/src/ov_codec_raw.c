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

     \file               ov_codec_raw.c
     \author             Michael J. Beer, DLR/GSOC <michael.beer@dlr.de>
     \date               2017-12-07

     \ingroup            empty

     \brief              empty

 **/

#include "../include/ov_codec_raw.h"
#include <ov_base/ov_utils.h>

/*---------------------------------------------------------------------------*/

/* Ensures same memory content on all archictectures */
static const uint8_t MAGIC_NUMBER_CHARS[] = {'r', 'a', 'w', 0};

#define MAGIC_NUMBER (*(uint32_t *)MAGIC_NUMBER_CHARS)

/******************************************************************************
 * RAW CODEC struct
 ******************************************************************************/

struct ov_codec_raw_struct {

    ov_codec codec;

    uint64_t last_seq_number;
};

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

/******************************************************************************
 * PUBLIC FUNCTIONS
 ******************************************************************************/

const char *ov_codec_raw_id() { return (char *)MAGIC_NUMBER_CHARS; }

/*---------------------------------------------------------------------------*/

ov_codec_generator ov_codec_raw_install(ov_codec_factory *factory) {

    if (0 == factory) goto error;

    return ov_codec_factory_install_codec(
        factory, ov_codec_raw_id(), impl_codec_create);

error:

    return 0;
}

/******************************************************************************
 * PRIVATE FUNCTIONS
 ******************************************************************************/

static ov_codec *impl_codec_create(uint32_t ssid,
                                   const ov_json_value *parameters) {

    UNUSED(ssid);
    UNUSED(parameters);

    ov_codec *codec = calloc(1, sizeof(struct ov_codec_raw_struct));

    *codec = (ov_codec){
        .type = MAGIC_NUMBER,
        .type_id = ov_codec_raw_id,
        .free = impl_free,
        .encode = impl_encode,
        .decode = impl_decode,
        .get_parameters = impl_get_parameters,
    };

    return codec;
}

/*---------------------------------------------------------------------------*/

static ov_codec *impl_free(ov_codec *self) {

    if (0 == self) return 0;

    if (MAGIC_NUMBER != self->type) {

        ov_log_error("Called on invalid codec");
        goto error;
    }

    free(self);

error:

    return 0;
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

    if (0 == length) goto error;

    if (MAGIC_NUMBER != self->type) {

        ov_log_error("Wrong codec type received");
        goto error;
    }

    if (length > max_out_length) {

        ov_log_error(
            "Output buffer not large enough. Input is %zu, "
            "output %zu",
            length,
            max_out_length);
        goto error;
    }

    if (output != memcpy(output, input, length)) {

        ov_log_error(
            "Severe error: Could not copy input to output "
            "buffer");
        goto error;
    }

    return length;

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

    struct ov_codec_raw_struct *c = (struct ov_codec_raw_struct *)self;

    if (1 + c->last_seq_number != seq_number) {

        ov_log_warning("Package Loss detected");
    }

    int32_t written_bytes =
        impl_encode(self, input, length, output, max_out_length);

    if (0 < written_bytes) {

        c->last_seq_number = seq_number;
    }

    return written_bytes;

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

    /* This codec is not parametrized */
    return ov_json_object();

error:

    return 0;
}

/*****************************************************************************
                            SO PLUGIN Hooks for testing
 ****************************************************************************/

char const *openvocs_plugin_codec_id() { return "test_codec"; }

/*----------------------------------------------------------------------------*/

ov_codec *openvocs_plugin_codec_create(uint32_t ssid,
                                       const ov_json_value *parameters) {

    return impl_codec_create(ssid, parameters);
}

/*----------------------------------------------------------------------------*/
