/***
        ------------------------------------------------------------------------

        Copyright 2018 German Aerospace Center DLR e.V. (GSOC)

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

        ------------------------------------------------------------------------
*//**
        @file           ov_base64.c
        @author         Markus Toepfer

        @date           2018-04-12

        @ingroup        ov_basics

        @brief          Implementation of base64 Encoding of RFC4648.


        ------------------------------------------------------------------------
*/
#include "../../include/ov_base64.h"

/*----------------------------------------------------------------------------*/

static uint8_t base64Alphabet[64] = {
    0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B,
    0x4C, 0x4D, 0x4E, 0x4F, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56,
    0x57, 0x58, 0x59, 0x5A, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
    0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F, 0x70, 0x71, 0x72,
    0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x30, 0x31, 0x32,
    0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x2B, 0x2F};

/*----------------------------------------------------------------------------*/

static uint8_t base64urlAlphabet[64] = {
    0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B,
    0x4C, 0x4D, 0x4E, 0x4F, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56,
    0x57, 0x58, 0x59, 0x5A, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
    0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F, 0x70, 0x71, 0x72,
    0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x30, 0x31, 0x32,
    0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x2D, 0x5F};

/*----------------------------------------------------------------------------*/

static uint8_t padding = 0x3D;

/*----------------------------------------------------------------------------*/

/**
    Check the position within the alphabet of the input value
    @param byte     value to check for position within the alphabet
    @param alphabet alphabet to check
*/
static int ov_base64_alphabetPosition(uint8_t byte, uint8_t alphabet[]) {

    if (!byte || !alphabet) goto error;

    int i;
    for (i = 0; i < 64; i++) {
        if (alphabet[i] == byte) return i;
    }

error:
    return -1;
}

/*
 *      ------------------------------------------------------------------------
 *
 *                             ENCODING
 *
 *      ------------------------------------------------------------------------
 *
 */

static bool ov_base64_encode_with_alphabet(const uint8_t *source,
                                           size_t source_length,
                                           uint8_t **dest,
                                           size_t *dest_length,
                                           uint8_t alphabet[]) {

    if (!source || source_length < 1 || !dest || !dest_length) return false;

    uint8_t *result = NULL;

    size_t length = source_length / 3 * 4;
    int fillbits = source_length % 3;

    if (fillbits > 0) {
        length += 4;
        fillbits = 3 - fillbits;
    }

    if (*dest) {

        if (*dest_length < length) return false;

    } else {

        *dest = calloc(length + 1, sizeof(uint8_t));
        if (!*dest) return false;
    }

    result = *dest;
    *dest_length = length;

    size_t i = 0;
    size_t k = 0;

    while (i + 3 < source_length) {

        result[k] = alphabet[(0x3F & (source[i] >> 2))];
        result[k + 1] =
            alphabet[(0x3F & (source[i] << 4 | source[i + 1] >> 4))];
        result[k + 2] =
            alphabet[(0x3F & (source[i + 1] << 2 | source[i + 2] >> 6))];
        result[k + 3] = alphabet[(0x3F & (source[i + 2]))];
        k += 4;
        i += 3;
    }

    switch (fillbits) {

        case 0:
            result[k] = alphabet[(0x3F & (source[i] >> 2))];
            result[k + 1] =
                alphabet[(0x3F & (source[i] << 4 | source[i + 1] >> 4))];
            result[k + 2] =
                alphabet[(0x3F & (source[i + 1] << 2 | source[i + 2] >> 6))];
            result[k + 3] = alphabet[(0x3F & (source[i + 2]))];
            break;
        case 1:
            result[k] = alphabet[(0x3F & (source[i] >> 2))];
            result[k + 1] =
                alphabet[(0x3F & (source[i] << 4 | source[i + 1] >> 4))];
            result[k + 2] = alphabet[(0x3C & (source[i + 1] << 2))];
            result[k + 3] = padding;
            break;
        case 2:
            result[k] = alphabet[(0x3F & (source[i] >> 2))];
            result[k + 1] = alphabet[(0x3F & (source[i] << 4))];
            result[k + 2] = padding;
            result[k + 3] = padding;
            break;
    }

    return true;
}

/*----------------------------------------------------------------------------*/

bool ov_base64_encode(const uint8_t *buffer,
                      size_t length,
                      uint8_t **result,
                      size_t *result_length) {

    return ov_base64_encode_with_alphabet(
        buffer, length, result, result_length, base64Alphabet);
}

/*----------------------------------------------------------------------------*/

bool ov_base64_url_encode(const uint8_t *buffer,
                          size_t length,
                          uint8_t **result,
                          size_t *result_length) {

    return ov_base64_encode_with_alphabet(
        buffer, length, result, result_length, base64urlAlphabet);
}

/*
 *      ------------------------------------------------------------------------
 *
 *                             DECODING
 *
 *      ------------------------------------------------------------------------
 *
 */

static bool ov_base64_decode_with_alphabet(const uint8_t *source,
                                           size_t source_length,
                                           uint8_t **dest,
                                           size_t *dest_length,
                                           uint8_t alphabet[]) {

    if (!source || source_length < 1 || !dest || !dest_length) return false;

    if (source_length % 4) return false;

    bool created = false;
    uint8_t *result = NULL;

    size_t length = source_length / 4 * 3;
    int fillbits = 0;

    if (source[source_length - 1] == padding) fillbits++;
    if (source[source_length - 2] == padding) fillbits++;

    length = length - fillbits;

    if (*dest) {

        if (*dest_length < length) return false;

    } else {

        *dest = calloc(length + 1, sizeof(uint8_t));
        if (!*dest) return false;
        created = true;
    }

    result = *dest;
    *dest_length = length;

    int pos0 = 0;
    int pos1 = 0;
    int pos2 = 0;
    int pos3 = 0;

    size_t i = 0;
    size_t k = 0;

    while (k + 4 < source_length) {

        pos0 = ov_base64_alphabetPosition(source[k], alphabet);
        if (pos0 < 0) goto error;

        pos1 = ov_base64_alphabetPosition(source[k + 1], alphabet);
        if (pos1 < 0) goto error;

        pos2 = ov_base64_alphabetPosition(source[k + 2], alphabet);
        if (pos2 < 0) goto error;

        pos3 = ov_base64_alphabetPosition(source[k + 3], alphabet);
        if (pos3 < 0) goto error;

        // xxAA AAAA xxAA BBBB xxBB BBCC xxCC CCCC

        result[i] = pos0 << 2 | pos1 >> 4;
        result[i + 1] = pos1 << 4 | pos2 >> 2;
        result[i + 2] = pos2 << 6 | pos3;
        i += 3;
        k += 4;
    }

    switch (fillbits) {
        case 0:
            pos0 = ov_base64_alphabetPosition(source[k], alphabet);
            if (pos0 < 0) goto error;

            pos1 = ov_base64_alphabetPosition(source[k + 1], alphabet);
            if (pos1 < 0) goto error;

            pos2 = ov_base64_alphabetPosition(source[k + 2], alphabet);
            if (pos2 < 0) goto error;

            pos3 = ov_base64_alphabetPosition(source[k + 3], alphabet);
            if (pos3 < 0) goto error;

            result[i] = pos0 << 2 | pos1 >> 4;
            result[i + 1] = pos1 << 4 | pos2 >> 2;
            result[i + 2] = pos2 << 6 | pos3;
            break;

        case 1:
            pos0 = ov_base64_alphabetPosition(source[k], alphabet);
            if (pos0 < 0) goto error;

            pos1 = ov_base64_alphabetPosition(source[k + 1], alphabet);
            if (pos1 < 0) goto error;

            pos2 = ov_base64_alphabetPosition(source[k + 2], alphabet);
            if (pos2 < 0) goto error;

            pos3 = 0;
            result[i] = pos0 << 2 | pos1 >> 4;
            result[i + 1] = pos1 << 4 | pos2 >> 2;
            break;
        case 2:
            pos0 = ov_base64_alphabetPosition(source[k], alphabet);
            if (pos0 < 0) goto error;

            pos1 = ov_base64_alphabetPosition(source[k + 1], alphabet);
            if (pos1 < 0) goto error;

            pos2 = 0;
            pos3 = 0;
            result[i] = pos0 << 2 | pos1 >> 4;
            break;
    }

    return true;

error:
    if (dest) {
        if (created) {
            free(*dest);
            *dest = NULL;
        }
    }

    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_base64_decode(const uint8_t *buffer,
                      size_t length,
                      uint8_t **result,
                      size_t *result_length) {

    return ov_base64_decode_with_alphabet(
        buffer, length, result, result_length, base64Alphabet);
}

/*----------------------------------------------------------------------------*/

bool ov_base64_url_decode(const uint8_t *buffer,
                          size_t length,
                          uint8_t **result,
                          size_t *result_length) {

    return ov_base64_decode_with_alphabet(
        buffer, length, result, result_length, base64urlAlphabet);
}
