
/***
        ------------------------------------------------------------------------

        Copyright (c) 2026 German Aerospace Center DLR e.V. (GSOC)

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

        @author Michael J. Beer, DLR/GSOC
        @copyright (c) 2026 German Aerospace Center DLR e.V. (GSOC)

        A format that does nothing more than encode / decode anything  written
        to / read from it with AES and forward it to its lower layer format

        Use as:

        ov_format_aes_install(0);

        ov_format *lower_layer = ...;

        ov_format_aes_params params = {
        .key = "12345678901234567890123456789012"
        };

        ov_format *coding_fmt = ov_format_as(lower_layer, "aes-256-gcm", &params);

        ov_format_payload_write_chunk(coding_fmt, chunk);

        ...

        This format write the AES data as follows:

        1. IV vector (12 octets)
        2. Actual encrypted data (...)
        3. AES Tag (16 octets)

        The tag is used by AES to ensure the integrity of the encoded data.

        Currently AES-256-GCM is implemented.

        BEWARE: If EOF is reached, the last 16 octets are the tag and not
        encrypted data any loonger.
        To implement this, this format has to read 16octets in advance.

        ------------------------------------------------------------------------
*/
#ifndef OV_FORMAT_AES_H
#define OV_FORMAT_AES_H
/*----------------------------------------------------------------------------*/

#include "ov_format_registry.h"

#define OV_FORMAT_AES_TYPE_STRING "aes"
#define OV_FORMAT_AES_KEY_LENGTH 32

typedef struct {
    char key[OV_FORMAT_AES_KEY_LENGTH];
} ov_format_aes_params;

bool ov_format_aes_install(ov_format_registry *registry);

typedef enum {
    OV_AES_DATA_VALID,
    OV_AES_DATA_INVALID,
    OV_AES_NO_DATA
} ov_aes_validity;

/**
 * Check whether decrypted data could be verified successfully.
 * BEWARE: Will only be able to verify data AFTER
 * EOF was reached.
 * Thus, read until no more data is available before calling this function.
 */
ov_aes_validity ov_format_aes_decrypted_valid(ov_format *self);
/*----------------------------------------------------------------------------*/
#endif
