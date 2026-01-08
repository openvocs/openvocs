/***
        ------------------------------------------------------------------------

        Copyright (c) 2019 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_rtp_frame_test.c
        @author         Michael Beer

        @date           2019-02-15

        @ingroup        ov_rtp

        @brief          Unit tests of ov_rtp_frame


        ------------------------------------------------------------------------
*/
#include "ov_rtp_frame.c"
#include <ov_test/ov_test.h>

#include "../../include/ov_utils.h"

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

#define FRAME_HAS(frame, ...)                                                  \
    frames_equal((ov_rtp_frame_expansion){__VA_ARGS__}, frame)

bool frames_equal(const ov_rtp_frame_expansion ref, const ov_rtp_frame *frame) {

    const ov_rtp_frame_expansion *exp = &frame->expanded;

    if (ref.version != exp->version)
        return false;
    if (ref.payload.length != exp->payload.length)
        return false;
    if (ref.padding_bit != exp->padding_bit)
        return false;
    if (ref.padding.length != exp->padding.length)
        return false;
    if (ref.marker_bit != exp->marker_bit)
        return false;
    if (ref.extension_bit != exp->extension_bit)
        return false;
    if (ref.extension.type != exp->extension.type)
        return false;
    if (ref.extension.length != exp->extension.length)
        return false;
    if (ref.payload_type != exp->payload_type)
        return false;
    if (ref.sequence_number != exp->sequence_number)
        return false;
    if (ref.timestamp != exp->timestamp)
        return false;
    if (ref.ssrc != exp->ssrc)
        return false;
    if (ref.csrc_count != exp->csrc_count)
        return false;

    if (0 != ref.csrc_count) {

        OV_ASSERT(ref.csrc_ids);
        OV_ASSERT(exp->csrc_ids);

        if (0 != memcmp(ref.csrc_ids, exp->csrc_ids,
                        ref.csrc_count * sizeof(uint32_t))) {

            return false;
        }
    }

    if (0 != ref.extension.length) {

        bool ref_extension_present = 0 != ref.extension.data;
        bool exp_extension_present = 0 != exp->extension.data;

        if (ref_extension_present != exp_extension_present)
            return false;

        if (ref_extension_present) {

            if (0 != memcmp(ref.extension.data, exp->extension.data,
                            ref.extension.length)) {

                return false;
            }
        }
    }

    if (0 != ref.payload.length) {

        OV_ASSERT(ref.payload.data);
        OV_ASSERT(exp->payload.data);

        if (0 !=
            memcmp(ref.payload.data, exp->payload.data, ref.payload.length)) {

            return false;
        }
    }

    if (0 != ref.padding.length) {

        OV_ASSERT(ref.padding.data);
        OV_ASSERT(exp->padding.data);

        if (0 !=
            memcmp(ref.padding.data, exp->padding.data, ref.padding.length)) {

            return false;
        }
    }

    return true;
}

/*---------------------------------------------------------------------------*/

uint8_t *datadup(const uint8_t *src, size_t length) {

    uint8_t *dest = malloc(length * sizeof(uint8_t));
    return memcpy(dest, src, length);
}

/******************************************************************************
 *                                   TESTS
 ******************************************************************************/

int test_ov_rtp_frame_enable_caching() {

    /* With default cache size */
    ov_rtp_frame_enable_caching(0);

    ov_rtp_frame_expansion data = {};

    /* Encode 2 frames with empty caches -> Should be different pointers */
    ov_rtp_frame *frame_1 = ov_rtp_frame_encode(&data);

    testrun(0 != frame_1);
    testrun(frame_1->expanded.version == RTP_VERSION_INVALID);

    ov_rtp_frame *frame_2 = ov_rtp_frame_encode(&data);

    testrun(0 != frame_2);
    testrun(frame_2->expanded.version == RTP_VERSION_INVALID);

    testrun(frame_1 != frame_2);

    /* Now free frame -> fill cache */
    testrun(0 == frame_1->free(frame_1));

    /* And encode again -> Should get pointer of frame_1 */
    ov_rtp_frame *frame_cached = ov_rtp_frame_encode(&data);

    testrun(0 != frame_cached);
    testrun(frame_cached->expanded.version == RTP_VERSION_INVALID);

    testrun(frame_cached == frame_1);
    frame_1 = 0;

    frame_cached = frame_cached->free(frame_cached);
    testrun(0 == frame_cached);

    frame_2 = frame_2->free(frame_2);
    testrun(0 == frame_2);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_rtp_frame_encode() {

#define ASSERT_BYTES_EQUAL(exp, frame, len)                                    \
    testrun((len == frame->bytes.length) &&                                    \
            0 == memcmp(exp, frame->bytes.data, len))

    uint8_t expected_bytes[1024] = {};

    testrun(0 == ov_rtp_frame_encode(0));

    ov_rtp_frame_expansion data = {};

    ov_rtp_frame *frame = ov_rtp_frame_encode(&data);

    testrun(frame);

    ov_rtp_frame_expansion *exp = &frame->expanded;

    testrun(exp->version == RTP_VERSION_INVALID);
    testrun(exp->payload.data == 0);
    testrun(exp->payload.length == 0);
    testrun(exp->padding.data == 0);
    testrun(exp->padding.length == 0);
    testrun(exp->extension.data == 0);
    testrun(exp->extension.length == 0);
    testrun(exp->csrc_ids == 0);
    testrun(exp->payload_type == OV_RTP_PAYLOAD_TYPE_DEFAULT);
    testrun(exp->sequence_number == 0);
    testrun(exp->timestamp == 0);
    testrun(exp->ssrc == 0);
    testrun(exp->csrc_count == 0);

    testrun(frame->bytes.data);
    testrun(RTP_HEADER_MIN_LENGTH == frame->bytes.length);

    ASSERT_BYTES_EQUAL(expected_bytes, frame, RTP_HEADER_MIN_LENGTH);

    frame = frame->free(frame);
    testrun(0 == frame);

    /*----------------------------*/
    /* Check encoding of 1st byte */

    /* ENCODE VERSION CORRECTLY */
    data.version = RTP_VERSION_2;

    frame = ov_rtp_frame_encode(&data);
    testrun(frame);

    expected_bytes[0] = RTP_VERSION_2 << 6;

    ASSERT_BYTES_EQUAL(expected_bytes, frame, RTP_HEADER_MIN_LENGTH);

    /* ENCODE PADDING BIT WITHOUT PADDING SHOULD SUCCEED */
    data.padding_bit = true;

    frame = frame->free(frame);
    testrun(0 == frame);

    frame = ov_rtp_frame_encode(&data);
    testrun(frame);

    expected_bytes[0] |= 0x20;

    expected_bytes[RTP_HEADER_MIN_LENGTH] = 1;
    ASSERT_BYTES_EQUAL(expected_bytes, frame, RTP_HEADER_MIN_LENGTH + 1);

    memset(expected_bytes, 0, 1024);
    expected_bytes[0] |= 0x20;

    data.padding_bit = false;
    data.version = 0;

    frame = frame->free(frame);
    testrun(0 == frame);

    frame = ov_rtp_frame_encode(&data);
    testrun(frame);
    expected_bytes[0] = 0;

    ASSERT_BYTES_EQUAL(expected_bytes, frame, RTP_HEADER_MIN_LENGTH);

    /* ENCODE EXTENSION BIT */
    data.extension_bit = true;

    frame = frame->free(frame);
    testrun(0 == frame);

    frame = ov_rtp_frame_encode(&data);
    testrun(frame);

    expected_bytes[0] |= 0x10;
    ASSERT_BYTES_EQUAL(expected_bytes, frame, RTP_HEADER_MIN_LENGTH + 4);

    data.extension.type = 0x15aa;

    frame = frame->free(frame);
    testrun(0 == frame);

    frame = ov_rtp_frame_encode(&data);
    testrun(frame);

    expected_bytes[RTP_HEADER_MIN_LENGTH] = 0x15;
    expected_bytes[RTP_HEADER_MIN_LENGTH + 1] = 0xaa;
    ASSERT_BYTES_EQUAL(expected_bytes, frame, RTP_HEADER_MIN_LENGTH + 4);

    data.version = RTP_VERSION_2;

    frame = frame->free(frame);
    testrun(0 == frame);

    frame = ov_rtp_frame_encode(&data);
    testrun(frame);

    expected_bytes[0] |= RTP_VERSION_2 << 6;

    ASSERT_BYTES_EQUAL(expected_bytes, frame, RTP_HEADER_MIN_LENGTH + 4);

    data.padding_bit = false;

    frame = frame->free(frame);
    testrun(0 == frame);

    frame = ov_rtp_frame_encode(&data);
    testrun(frame);

    expected_bytes[0] &= ~0x20;
    ASSERT_BYTES_EQUAL(expected_bytes, frame, RTP_HEADER_MIN_LENGTH + 4);

    /* ENCODE CSRCS */
    uint32_t csrcs[] = {3, 4, 7, 11, 0x192837ff};
    uint8_t csrcs_encoded[] = {0,    0,    0,    0x03, 0,    0,   0,
                               0x04, 0,    0,    0,    0x07, 0,   0,
                               0,    0x0b, 0x19, 0x28, 0x37, 0xff};

    data.csrc_ids = csrcs;

    /* CSRC count too high */
    data.csrc_count = 16;

    frame = frame->free(frame);
    testrun(0 == frame);

    frame = ov_rtp_frame_encode(&data);
    testrun(0 == frame);

    data.csrc_count = 5;

    frame = ov_rtp_frame_encode(&data);
    testrun(frame);

    expected_bytes[0] |= 5;
    memcpy(expected_bytes + 12, csrcs_encoded, 5 * sizeof(uint32_t));
    expected_bytes[12 + +5 * sizeof(uint32_t)] = 0x15;
    expected_bytes[12 + +5 * sizeof(uint32_t) + 1] = 0xaa;

    ASSERT_BYTES_EQUAL(expected_bytes, frame,
                       RTP_HEADER_MIN_LENGTH + 5 * sizeof(uint32_t) +
                           sizeof(uint32_t));

    data.extension_bit = false;
    expected_bytes[0] &= ~0x10;
    memset(frame->bytes.data, 0, frame->bytes.length);

    frame = frame->free(frame);
    testrun(0 == frame);

    frame = ov_rtp_frame_encode(&data);
    testrun(frame);

    ASSERT_BYTES_EQUAL(expected_bytes, frame,
                       RTP_HEADER_MIN_LENGTH + 5 * sizeof(uint32_t));

    memset(frame->bytes.data, 0, frame->bytes.length);

    data.version = RTP_VERSION_1;
    expected_bytes[0] &= ~(RTP_VERSION_2 << 6);
    expected_bytes[0] |= (RTP_VERSION_1 << 6);
    expected_bytes[0] |= 0x20;
    data.padding_bit = true;
    expected_bytes[RTP_HEADER_MIN_LENGTH + 5 * sizeof(uint32_t)] = 0x01;

    frame = frame->free(frame);
    testrun(0 == frame);

    frame = ov_rtp_frame_encode(&data);
    testrun(frame);

    ASSERT_BYTES_EQUAL(expected_bytes, frame,
                       RTP_HEADER_MIN_LENGTH + 5 * sizeof(uint32_t) + 1);

    /*--------------------*/
    /* Now check 2nd byte */

    /* Continue with old data to check for
     * cross-dependencies between 1st and 2nd byte */

    /* ENCODE MARKER BIT */
    data.marker_bit = true;
    expected_bytes[1] |= 1 << 7;

    frame = frame->free(frame);
    testrun(0 == frame);

    frame = ov_rtp_frame_encode(&data);

    testrun(frame);

    ASSERT_BYTES_EQUAL(expected_bytes, frame,
                       RTP_HEADER_MIN_LENGTH + 5 * sizeof(uint32_t) + 1);

    /* ENCODE PAYLOAD TYPE CORRECTLY */
    data.payload_type = 0x7f;
    expected_bytes[1] |= 0x7f;

    frame = frame->free(frame);
    testrun(0 == frame);

    frame = ov_rtp_frame_encode(&data);
    testrun(frame);
    ASSERT_BYTES_EQUAL(expected_bytes, frame,
                       RTP_HEADER_MIN_LENGTH + 5 * sizeof(uint32_t) + 1);

    data.marker_bit = false;
    expected_bytes[1] &= ~(1 << 7);

    frame = frame->free(frame);
    testrun(0 == frame);

    frame = ov_rtp_frame_encode(&data);
    testrun(frame);
    ASSERT_BYTES_EQUAL(expected_bytes, frame,
                       RTP_HEADER_MIN_LENGTH + 5 * sizeof(uint32_t) + 1);

    /* Alter some data in byte 0 to ensure no cross-relations */

    data.csrc_count = 0;
    expected_bytes[0] &= ~0x0f;
    expected_bytes[RTP_HEADER_MIN_LENGTH] = 0x01;

    frame = frame->free(frame);
    testrun(0 == frame);

    frame = ov_rtp_frame_encode(&data);
    testrun(frame);
    ASSERT_BYTES_EQUAL(expected_bytes, frame, RTP_HEADER_MIN_LENGTH + 1);

    data.csrc_count = 5;
    expected_bytes[0] |= 0x05;

    /* Check Sequence number (byte 2 & 3) */
    data.sequence_number = 0x1234;
    expected_bytes[2] = 0x12;
    expected_bytes[3] = 0x34;
    expected_bytes[RTP_HEADER_MIN_LENGTH] = 0x00;
    expected_bytes[RTP_HEADER_MIN_LENGTH + 5 * sizeof(uint32_t)] = 0x01;

    frame = frame->free(frame);
    testrun(0 == frame);

    frame = ov_rtp_frame_encode(&data);
    testrun(frame);
    ASSERT_BYTES_EQUAL(expected_bytes, frame,
                       RTP_HEADER_MIN_LENGTH + 5 * sizeof(uint32_t) + 1);

    data.csrc_count = 2;
    expected_bytes[0] &= ~0x0f;
    expected_bytes[0] |= 0x02;
    expected_bytes[RTP_HEADER_MIN_LENGTH + 2 * sizeof(uint32_t)] = 0x01;

    frame = frame->free(frame);
    testrun(0 == frame);

    frame = ov_rtp_frame_encode(&data);
    testrun(frame);
    ASSERT_BYTES_EQUAL(expected_bytes, frame,
                       RTP_HEADER_MIN_LENGTH + 2 * sizeof(uint32_t) + 1);

    /* Check timestamp */
    data.timestamp = 0xa2b3c4d5;
    expected_bytes[4] = 0xa2;
    expected_bytes[5] = 0xb3;
    expected_bytes[6] = 0xc4;
    expected_bytes[7] = 0xd5;

    frame = frame->free(frame);
    testrun(0 == frame);

    frame = ov_rtp_frame_encode(&data);
    testrun(frame);
    ASSERT_BYTES_EQUAL(expected_bytes, frame,
                       RTP_HEADER_MIN_LENGTH + 2 * sizeof(uint32_t) + 1);

    data.extension_bit = false;
    expected_bytes[0] &= ~0x10;

    frame = frame->free(frame);
    testrun(0 == frame);

    frame = ov_rtp_frame_encode(&data);
    testrun(frame);
    ASSERT_BYTES_EQUAL(expected_bytes, frame,
                       RTP_HEADER_MIN_LENGTH + 2 * sizeof(uint32_t) + 1);

    /*Check synchronisation Source ID */
    data.ssrc = 0xd1c2b3a4;
    expected_bytes[8] = 0xd1;
    expected_bytes[9] = 0xc2;
    expected_bytes[10] = 0xb3;
    expected_bytes[11] = 0xa4;

    frame = frame->free(frame);
    testrun(0 == frame);

    frame = ov_rtp_frame_encode(&data);
    testrun(frame);
    ASSERT_BYTES_EQUAL(expected_bytes, frame,
                       RTP_HEADER_MIN_LENGTH + 2 * sizeof(uint32_t) + 1);

    data.marker_bit = false;
    expected_bytes[1] &= ~0x80;

    frame = frame->free(frame);
    testrun(0 == frame);

    frame = ov_rtp_frame_encode(&data);
    testrun(frame);
    ASSERT_BYTES_EQUAL(expected_bytes, frame,
                       RTP_HEADER_MIN_LENGTH + 2 * sizeof(uint32_t) + 1);

    data.csrc_count = 5;
    expected_bytes[0] &= ~0x0f;
    expected_bytes[0] |= 0x05;
    expected_bytes[RTP_HEADER_MIN_LENGTH + 2 * sizeof(uint32_t)] = 0x00;
    expected_bytes[RTP_HEADER_MIN_LENGTH + 5 * sizeof(uint32_t)] = 0x01;

    frame = frame->free(frame);
    testrun(0 == frame);

    frame = ov_rtp_frame_encode(&data);
    testrun(frame);
    ASSERT_BYTES_EQUAL(expected_bytes, frame,
                       RTP_HEADER_MIN_LENGTH +
                           data.csrc_count * sizeof(uint32_t) + 1);

    /* Check payload */

    data.padding_bit = false;
    expected_bytes[0] &= ~0x20;

    size_t expected_index = RTP_HEADER_MIN_LENGTH;
    expected_index += data.csrc_count * sizeof(uint32_t);

    uint8_t payload[] = {0xf1, 0xe2, 0xd3, 0xc4, 0xb5, 0xa6};
    const size_t payload_length = 6;
    data.payload.data = payload;
    data.payload.length = payload_length;
    memcpy(expected_bytes + expected_index, payload, payload_length);
    expected_index += payload_length;

    frame = frame->free(frame);
    testrun(0 == frame);

    frame = ov_rtp_frame_encode(&data);
    testrun(frame);
    ASSERT_BYTES_EQUAL(expected_bytes, frame, expected_index);

    /* Check padding */

    data.padding.data = (uint8_t[]){0x10, 0x02, 0x30};
    data.padding.length = 3;
    data.padding_bit = true;

    uint8_t *p = expected_bytes + expected_index;

    expected_bytes[0] |= 0x20;
    memcpy(p, data.padding.data, data.padding.length);
    p += data.padding.length;

    *p = 1 + data.padding.length;

    expected_index += 1 + data.padding.length;

    frame = frame->free(frame);
    testrun(0 == frame);

    frame = ov_rtp_frame_encode(&data);
    testrun(frame);
    ASSERT_BYTES_EQUAL(expected_bytes, frame, expected_index);

    /* Alter some stuff in the lower bytes to check for cross-dependencies
     */
    data.payload_type = 0x71;
    expected_bytes[1] &= ~0x7f;
    expected_bytes[1] |= 0x71;

    frame = frame->free(frame);
    testrun(0 == frame);

    frame = ov_rtp_frame_encode(&data);
    testrun(frame);
    ASSERT_BYTES_EQUAL(expected_bytes, frame, expected_index);

    /*-------------------------------------------------------------------*/
    /* Check Extension - without allocated extension data */

    testrun(0 == data.extension.allocated_bytes);
    data.extension.data =
        (uint8_t[]){0x43, 0x8f, 0x85, 0x86, 0x01, 0x1a, 0xf7, 0xdd};

    data.extension.length = 8;
    data.extension.type = 0x0102;

    data.extension_bit = true;

    expected_bytes[0] |= 0x10;

    expected_index = RTP_HEADER_MIN_LENGTH;
    expected_index += data.csrc_count * sizeof(uint32_t);

    p = expected_bytes;
    p += expected_index;

    *p = 0x01;
    *(++p) = 0x02;
    *(++p) = 0x00;
    *(++p) = 0x02; /* Length of extension */
    ++p;

    memcpy(p, data.extension.data, data.extension.length);

    p += data.extension.length;
    expected_index += sizeof(uint32_t) + data.extension.length;
    memcpy(p, data.payload.data, data.payload.length);
    p += data.payload.length;
    expected_index += data.payload.length;

    memcpy(p, data.padding.data, data.padding.length);

    p += data.padding.length;
    *p = 1 + data.padding.length;

    expected_index += 1 + data.padding.length;

    frame = frame->free(frame);
    testrun(0 == frame);

    frame = ov_rtp_frame_encode(&data);
    testrun(frame);

    testrun(0 == frame->expanded.extension.allocated_bytes);
    testrun(data.extension.length == frame->expanded.extension.length);
    ASSERT_BYTES_EQUAL(expected_bytes, frame, expected_index);

    /*-------------------------------------------------------------------*/
    /* Check Extension - with allocated extension data  in input frame*/

    uint8_t extension_data[] = {0x43, 0x8f, 0x85, 0x86, 0x01, 0x1a, 0xf7, 0xdd};

    testrun(0 == data.extension.allocated_bytes);

    /* The allocated buffer should not be altered */
    data.extension.allocated_bytes = sizeof(extension_data) + 7;
    data.extension.data = calloc(1, sizeof(extension_data) + 7);
    memcpy(data.extension.data, extension_data, sizeof(extension_data));

    data.extension.length = 8;
    data.extension.type = 0x0102;

    data.extension_bit = true;

    expected_bytes[0] |= 0x10;

    expected_index = RTP_HEADER_MIN_LENGTH;
    expected_index += data.csrc_count * sizeof(uint32_t);

    p = expected_bytes;
    p += expected_index;

    *p = 0x01;
    *(++p) = 0x02;
    *(++p) = 0x00;
    *(++p) = 0x02; /* Length of extension */
    ++p;

    memcpy(p, data.extension.data, data.extension.length);

    p += data.extension.length;
    expected_index += sizeof(uint32_t) + data.extension.length;
    memcpy(p, data.payload.data, data.payload.length);
    p += data.payload.length;
    expected_index += data.payload.length;

    memcpy(p, data.padding.data, data.padding.length);

    p += data.padding.length;
    *p = 1 + data.padding.length;

    expected_index += 1 + data.padding.length;

    frame = frame->free(frame);
    testrun(0 == frame);

    frame = ov_rtp_frame_encode(&data);
    testrun(frame);

    testrun(0 == frame->expanded.extension.allocated_bytes);

    testrun(sizeof(extension_data) + 7 == data.extension.allocated_bytes);

    ASSERT_BYTES_EQUAL(expected_bytes, frame, expected_index);

    free(data.extension.data);
    data.extension.allocated_bytes = 0;

    /*-------------------------------------------------------------------*/
    /* Check Extension - with allocated extension data in target frame */

    testrun(0 == data.extension.allocated_bytes);
    testrun(0 == frame->expanded.extension.allocated_bytes);

    /* The allocated buffer in target_frame should be freed */
    frame->expanded.extension.allocated_bytes = sizeof(extension_data) + 7;
    frame->expanded.extension.data = calloc(1, sizeof(extension_data) + 7);
    memcpy(frame->expanded.extension.data, extension_data,
           sizeof(extension_data));

    data.extension.data = extension_data;
    data.extension.length = 8;
    data.extension.type = 0x0102;

    data.extension_bit = true;

    expected_bytes[0] |= 0x10;

    expected_index = RTP_HEADER_MIN_LENGTH;
    expected_index += data.csrc_count * sizeof(uint32_t);

    p = expected_bytes;
    p += expected_index;

    *p = 0x01;
    *(++p) = 0x02;
    *(++p) = 0x00;
    *(++p) = 0x02; /* Length of extension */
    ++p;

    memcpy(p, data.extension.data, data.extension.length);

    p += data.extension.length;
    expected_index += sizeof(uint32_t) + data.extension.length;
    memcpy(p, data.payload.data, data.payload.length);
    p += data.payload.length;
    expected_index += data.payload.length;

    memcpy(p, data.padding.data, data.padding.length);

    p += data.padding.length;
    *p = 1 + data.padding.length;

    expected_index += 1 + data.padding.length;

    frame = frame->free(frame);
    testrun(0 == frame);

    frame = ov_rtp_frame_encode(&data);
    testrun(frame);

    testrun(0 == frame->expanded.extension.allocated_bytes);
    testrun(0 == data.extension.allocated_bytes);
    ASSERT_BYTES_EQUAL(expected_bytes, frame, expected_index);

    frame = frame->free(frame);
    testrun(0 == frame);

    return testrun_log_success();

#undef ASSERT_BYTES_EQUAL
}

/*---------------------------------------------------------------------------*/

int test_ov_rtp_frame_decode() {

    uint8_t input_bytes[1024] = {0};

    testrun(0 == ov_rtp_frame_decode(0, 0));

    size_t num_bytes = RTP_HEADER_MIN_LENGTH;

    ov_rtp_frame *frame = ov_rtp_frame_decode(input_bytes, num_bytes);

    testrun(0 != frame);

    testrun(FRAME_HAS(frame));

    frame = frame->free(frame);

    /*--------------------------------------------------------------------*/
    /* Check decoding of 1st byte */

    /* DECODE VERSION CORRECTLY */
    input_bytes[0] = RTP_VERSION_2 << 6;

    frame = ov_rtp_frame_decode(input_bytes, num_bytes);

    testrun(0 != frame);

    testrun(FRAME_HAS(frame, .version = RTP_VERSION_2));
    testrun(frame->bytes.length == num_bytes);

    frame = frame->free(frame);

    /* DECODE PADDING BIT CORRECTLY  - Should fail because of missing
     * payload */
    input_bytes[0] |= 0x20;

    frame = ov_rtp_frame_decode(input_bytes, num_bytes);
    testrun(0 == frame);

    input_bytes[0] &= ~0xc0;

    frame = ov_rtp_frame_decode(input_bytes, num_bytes);
    testrun(0 == frame);

    /* Actual padding checked later */

    /* DECODE EXTENSION BIT */
    input_bytes[0] &= ~0x20;
    input_bytes[0] |= 0x10;

    frame = ov_rtp_frame_decode(input_bytes, num_bytes);
    testrun(0 == frame);

    input_bytes[0] &= ~0x10;

    uint32_t extension[] = {0x11223344, 0x55667788, 0x99aabbcc};

    ov_rtp_frame_expansion ref = {
        .version = 0,
        .extension_bit = true,
        .extension.type = 0x14,
    };

    ov_rtp_frame *encoded = ov_rtp_frame_encode(&ref);
    testrun(0 != encoded);

    frame = ov_rtp_frame_decode(encoded->bytes.data, encoded->bytes.length);

    testrun(0 != frame);

    testrun(frames_equal(ref, frame));
    testrun(0 == frame->expanded.extension.allocated_bytes);

    ref.extension.length = 3 * sizeof(uint32_t);
    ref.extension.data = (uint8_t *)&extension;

    encoded = encoded->free(encoded);
    testrun(0 == encoded);

    frame = frame->free(frame);

    encoded = ov_rtp_frame_encode(&ref);
    testrun(encoded);

    frame = ov_rtp_frame_decode(encoded->bytes.data, encoded->bytes.length);

    testrun(0 != frame);

    testrun(0 == frame->expanded.extension.allocated_bytes);
    testrun(frames_equal(ref, frame));

    ref = (ov_rtp_frame_expansion){
        .version = RTP_VERSION_2,
        .padding_bit = true,
        .extension_bit = true,
        .extension.type = 0x14,
    };

    encoded = encoded->free(encoded);
    testrun(0 == encoded);

    frame = frame->free(frame);

    encoded = ov_rtp_frame_encode(&ref);
    testrun(encoded);

    frame = ov_rtp_frame_decode(encoded->bytes.data, encoded->bytes.length);

    testrun(0 != frame);

    testrun(0 == frame->expanded.extension.allocated_bytes);
    testrun(frames_equal(ref, frame));

    ref.padding_bit = false;

    frame = frame->free(frame);
    testrun(0 == frame);

    encoded = encoded->free(encoded);
    testrun(0 == encoded);

    encoded = ov_rtp_frame_encode(&ref);
    testrun(encoded);

    frame = ov_rtp_frame_decode(encoded->bytes.data, encoded->bytes.length);

    testrun(0 != frame);
    testrun(frames_equal(ref, frame));

    frame = frame->free(frame);
    testrun(0 == frame);

    encoded = encoded->free(encoded);
    testrun(0 == encoded);

    /*--------------------------------------------------------------------*/
    /* ENCODE CSRCS */
    uint32_t csrcs[] = {3, 4, 7, 11, 0x192837ff};

    ref = (ov_rtp_frame_expansion){
        .version = RTP_VERSION_2,
        .csrc_count = 5,
        .csrc_ids = csrcs,
    };

    encoded = ov_rtp_frame_encode(&ref);
    testrun(encoded);

    /* Cut away the csrcs - should fail */
    encoded->bytes.length = RTP_HEADER_MIN_LENGTH;
    frame = ov_rtp_frame_decode(encoded->bytes.data, encoded->bytes.length);
    testrun(0 == frame);

    /* With CSRCs */
    encoded = encoded->free(encoded);
    testrun(0 == encoded);

    encoded = ov_rtp_frame_encode(&ref);
    testrun(encoded);

    frame = ov_rtp_frame_decode(encoded->bytes.data, encoded->bytes.length);
    testrun(0 != frame);
    testrun(frames_equal(ref, frame));

    frame = frame->free(frame);
    testrun(0 == frame);

    encoded = encoded->free(encoded);
    testrun(0 == encoded);

    /* extension */

    /*Header only */
    ref.extension.length = 0;
    ref.extension.type = 0x0102;
    ref.extension_bit = true;

    encoded = ov_rtp_frame_encode(&ref);
    testrun(encoded);

    frame = ov_rtp_frame_decode(encoded->bytes.data, encoded->bytes.length);

    testrun(0 != frame);
    testrun(frames_equal(ref, frame));

    frame = frame->free(frame);
    testrun(0 == frame);

    encoded = encoded->free(encoded);
    testrun(0 == encoded);

    /* Header & Additional Extension data */
    ref.extension.data =
        (uint8_t[]){0x43, 0x8f, 0x85, 0x86, 0x01, 0x1a, 0xf7, 0xdd};

    ref.extension.length = 8;

    encoded = ov_rtp_frame_encode(&ref);

    testrun(encoded);
    frame = ov_rtp_frame_decode(encoded->bytes.data, encoded->bytes.length);

    testrun(0 != frame);
    testrun(frames_equal(ref, frame));

    frame = frame->free(frame);
    testrun(0 == frame);

    encoded = encoded->free(encoded);
    testrun(0 == encoded);

    ref.version = RTP_VERSION_1;

    encoded = ov_rtp_frame_encode(&ref);
    testrun(0 != encoded);

    frame = ov_rtp_frame_decode(encoded->bytes.data, encoded->bytes.length);
    testrun(0 != frame);
    testrun(frames_equal(ref, frame));

    frame = frame->free(frame);
    testrun(0 == frame);

    encoded = encoded->free(encoded);
    testrun(0 == encoded);

    ref.version = RTP_VERSION_2;

    /*--------------------------------------------------------------------*/
    /* Now check 2nd byte */

    ref.marker_bit = true;

    encoded = ov_rtp_frame_encode(&ref);

    frame = ov_rtp_frame_decode(encoded->bytes.data, encoded->bytes.length);

    testrun(0 != frame);
    testrun(frames_equal(ref, frame));

    frame = frame->free(frame);
    testrun(0 == frame);

    encoded = encoded->free(encoded);
    testrun(0 == encoded);

    ref.payload_type = 0x7f;

    encoded = ov_rtp_frame_encode(&ref);

    frame = ov_rtp_frame_decode(encoded->bytes.data, encoded->bytes.length);

    testrun(0 != frame);
    testrun(frames_equal(ref, frame));

    frame = frame->free(frame);
    testrun(0 == frame);

    encoded = encoded->free(encoded);
    testrun(0 == encoded);

    ref.payload_type = 0x78;

    encoded = ov_rtp_frame_encode(&ref);
    testrun(0 != encoded);

    frame = ov_rtp_frame_decode(encoded->bytes.data, encoded->bytes.length);

    testrun(0 != frame);
    testrun(frames_equal(ref, frame));

    frame = frame->free(frame);
    testrun(0 == frame);

    encoded = encoded->free(encoded);
    testrun(0 == encoded);

    ref.marker_bit = false;

    encoded = ov_rtp_frame_encode(&ref);

    testrun(0 != encoded);

    frame = ov_rtp_frame_decode(encoded->bytes.data, encoded->bytes.length);
    testrun(0 != frame);
    testrun(frames_equal(ref, frame));

    frame = frame->free(frame);
    testrun(0 == frame);

    encoded = encoded->free(encoded);
    testrun(0 == encoded);

    ref.version = RTP_VERSION_2;

    encoded = ov_rtp_frame_encode(&ref);

    testrun(0 != encoded);

    frame = ov_rtp_frame_decode(encoded->bytes.data, encoded->bytes.length);
    testrun(0 != frame);
    testrun(frames_equal(ref, frame));

    frame = frame->free(frame);
    testrun(0 == frame);

    encoded = encoded->free(encoded);
    testrun(0 == encoded);

    /* Check sequence number */

    ref.sequence_number = 0x1001;

    encoded = ov_rtp_frame_encode(&ref);

    testrun(0 != encoded);

    frame = ov_rtp_frame_decode(encoded->bytes.data, encoded->bytes.length);
    testrun(0 != frame);
    testrun(frames_equal(ref, frame));

    frame = frame->free(frame);
    testrun(0 == frame);

    encoded = encoded->free(encoded);
    testrun(0 == encoded);

    /* Check timestamp */

    ref.timestamp = 0xf1e77ea;

    encoded = ov_rtp_frame_encode(&ref);
    testrun(0 != encoded);

    frame = ov_rtp_frame_decode(encoded->bytes.data, encoded->bytes.length);
    testrun(0 != frame);
    testrun(frames_equal(ref, frame));

    frame = frame->free(frame);
    testrun(0 == frame);

    encoded = encoded->free(encoded);
    testrun(0 == encoded);

    /* Check SSRC */

    ref.ssrc = 0x98fe76dc;

    encoded = ov_rtp_frame_encode(&ref);
    testrun(0 != encoded);

    frame = ov_rtp_frame_decode(encoded->bytes.data, encoded->bytes.length);

    testrun(0 != frame);
    testrun(frames_equal(ref, frame));

    frame = frame->free(frame);
    testrun(0 == frame);

    encoded = encoded->free(encoded);
    testrun(0 == encoded);

    /*--------------------------------------------------------------------*/
    /* Check payload */
    uint8_t payload[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                         0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10};

    ref.payload.length = 9;
    ref.payload.data = payload;

    encoded = ov_rtp_frame_encode(&ref);

    testrun(0 != encoded);

    frame = ov_rtp_frame_decode(encoded->bytes.data, encoded->bytes.length);
    testrun(0 != frame);
    testrun(frames_equal(ref, frame));

    frame = frame->free(frame);
    testrun(0 == frame);

    encoded = encoded->free(encoded);
    testrun(0 == encoded);

    ref.padding_bit = true;
    ref.padding.data = (uint8_t[]){0xf1, 0xf2, 0xf3};
    ref.padding.length = 3;

    encoded = ov_rtp_frame_encode(&ref);
    testrun(0 != encoded);

    frame = ov_rtp_frame_decode(encoded->bytes.data, encoded->bytes.length);
    testrun(0 != frame);
    testrun(frames_equal(ref, frame));

    frame = frame->free(frame);
    testrun(0 == frame);

    encoded = encoded->free(encoded);
    testrun(0 == encoded);

    /*------------------------------------------------------------------------*/
    /* Check some random configurations */
    /* With caching */

    ov_rtp_frame_enable_caching(15);

    ref.csrc_count = 2;

    encoded = ov_rtp_frame_encode(&ref);
    testrun(0 != encoded);

    frame = ov_rtp_frame_decode(encoded->bytes.data, encoded->bytes.length);
    testrun(0 != frame);
    testrun(frames_equal(ref, frame));

    frame = frame->free(frame);
    testrun(0 == frame);

    encoded = encoded->free(encoded);
    testrun(0 == encoded);

    ref.csrc_count = 0;

    encoded = ov_rtp_frame_encode(&ref);
    testrun(0 != encoded);

    frame = ov_rtp_frame_decode(encoded->bytes.data, encoded->bytes.length);
    testrun(0 != frame);
    testrun(frames_equal(ref, frame));

    frame = frame->free(frame);
    testrun(0 == frame);

    encoded = encoded->free(encoded);
    testrun(0 == encoded);

    ref.csrc_count = 5;
    ref.extension_bit = true;

    encoded = ov_rtp_frame_encode(&ref);
    testrun(0 != encoded);

    frame = ov_rtp_frame_decode(encoded->bytes.data, encoded->bytes.length);
    testrun(0 != frame);
    testrun(frames_equal(ref, frame));

    frame = frame->free(frame);
    testrun(0 == frame);

    encoded = encoded->free(encoded);
    testrun(0 == encoded);

    ref.padding.length = 0;

    encoded = ov_rtp_frame_encode(&ref);
    testrun(0 != encoded);

    frame = ov_rtp_frame_decode(encoded->bytes.data, encoded->bytes.length);
    testrun(0 != frame);
    testrun(frames_equal(ref, frame));

    frame = frame->free(frame);
    testrun(0 == frame);

    encoded = encoded->free(encoded);
    testrun(0 == encoded);

    /*--------------------------------------------------------------------*/
    /* Check with allocated extension data */

    ref.padding.length = 0;

    encoded = ov_rtp_frame_encode(&ref);
    testrun(0 != encoded);

    frame = ov_rtp_frame_decode(encoded->bytes.data, encoded->bytes.length);
    testrun(0 != frame);
    testrun(frames_equal(ref, frame));

    encoded = encoded->free(encoded);
    testrun(0 == encoded);

    frame = frame->free(frame);
    testrun(0 == frame);

    return testrun_log_success();
}

/*---------------------------------------------------------------------------*/

static int test_ov_rtp_frame_free() {

    testrun(0 == ov_rtp_frame_free(0));

    uint8_t input_bytes[1024] = {0};
    size_t num_bytes = RTP_HEADER_MIN_LENGTH;

    ov_rtp_frame *frame = ov_rtp_frame_decode(input_bytes, num_bytes);
    testrun(0 != frame);

    frame = ov_rtp_frame_free(frame);

    testrun(0 == frame);

    return testrun_log_success();
}

/*****************************************************************************
                               ov_rtp_frame_dump
 ****************************************************************************/

bool assert_dump_equals(const ov_rtp_frame_expansion *exp, const char *ref) {

    bool retval = false;

    char *buffer = 0;
    size_t buflen = 0;

    FILE *s = open_memstream(&buffer, &buflen);

    if (!s)
        goto error;

    if (!ov_rtp_frame_dump(exp, s)) {
        goto error;
    }

    if (0 == s) {

        fprintf(stderr, "%s\n\n", strerror(errno));
        exit(1);
    }

    fflush(s);

    if (strlen(ref) != buflen)
        goto error;

    size_t i;
    for (i = 0; i < buflen + 1; i++) {

        if (ref[i] != buffer[i])
            goto error;
    }

    retval = true;

error:

    if (s)
        fclose(s);
    s = 0;

    if (buffer)
        free(buffer);
    buffer = 0;

    return retval;
}

/*----------------------------------------------------------------------------*/

int test_ov_rtp_frame_dump() {

/* Macro required in order to retain correct code line */
#define ASSERT_DUMP_EQUALS(e, c) testrun(assert_dump_equals(e, c))

    testrun(!ov_rtp_frame_dump(0, stderr));

    ov_rtp_frame_expansion exp = {0};

    testrun(!ov_rtp_frame_dump(&exp, 0));

    exp.version = RTP_VERSION_INVALID;
    exp.payload_type = OV_RTP_PAYLOAD_TYPE_DEFAULT;

    ASSERT_DUMP_EQUALS(&exp, "\n"
                             "RTP Frame dump\n"
                             "\n"
                             "RTP Version             0\n"
                             "RTP Padding bit     false\n"
                             "RTP Extension bit   false\n"
                             "RTP Marker bit      false\n"
                             "RTP Payload type        0\n"
                             "RTP Sequence number     0\n"
                             "RTP Timestamp (SR Units)                      0\n"
                             "RTP SSRC ID                                   0\n"
                             "RTP CSRC IDs                                  0\n"
                             "\n"
                             "RTP Payload   NONE\n"
                             "RTP Padding   NONE\n"
                             "RTP Extension NONE\n"
                             "\n");

    exp.version = RTP_VERSION_2;

    ASSERT_DUMP_EQUALS(&exp, "\n"
                             "RTP Frame dump\n"
                             "\n"
                             "RTP Version             2\n"
                             "RTP Padding bit     false\n"
                             "RTP Extension bit   false\n"
                             "RTP Marker bit      false\n"
                             "RTP Payload type        0\n"
                             "RTP Sequence number     0\n"
                             "RTP Timestamp (SR Units)                      0\n"
                             "RTP SSRC ID                                   0\n"
                             "RTP CSRC IDs                                  0\n"
                             "\n"
                             "RTP Payload   NONE\n"
                             "RTP Padding   NONE\n"
                             "RTP Extension NONE\n"
                             "\n");

    exp.padding_bit = true;

    ASSERT_DUMP_EQUALS(&exp, "\n"
                             "RTP Frame dump\n"
                             "\n"
                             "RTP Version             2\n"
                             "RTP Padding bit      true\n"
                             "RTP Extension bit   false\n"
                             "RTP Marker bit      false\n"
                             "RTP Payload type        0\n"
                             "RTP Sequence number     0\n"
                             "RTP Timestamp (SR Units)                      0\n"
                             "RTP SSRC ID                                   0\n"
                             "RTP CSRC IDs                                  0\n"
                             "\n"
                             "RTP Payload   NONE\n"
                             "RTP Padding Length            1\n"
                             "\n\n"
                             "RTP Extension NONE\n"
                             "\n");

    exp.padding_bit = false;
    exp.version = 0;

    ASSERT_DUMP_EQUALS(&exp, "\n"
                             "RTP Frame dump\n"
                             "\n"
                             "RTP Version             0\n"
                             "RTP Padding bit     false\n"
                             "RTP Extension bit   false\n"
                             "RTP Marker bit      false\n"
                             "RTP Payload type        0\n"
                             "RTP Sequence number     0\n"
                             "RTP Timestamp (SR Units)                      0\n"
                             "RTP SSRC ID                                   0\n"
                             "RTP CSRC IDs                                  0\n"
                             "\n"
                             "RTP Payload   NONE\n"
                             "RTP Padding   NONE\n"
                             "RTP Extension NONE\n"
                             "\n");

    exp.extension_bit = true;

    ASSERT_DUMP_EQUALS(&exp, "\n"
                             "RTP Frame dump\n"
                             "\n"
                             "RTP Version             0\n"
                             "RTP Padding bit     false\n"
                             "RTP Extension bit    true\n"
                             "RTP Marker bit      false\n"
                             "RTP Payload type        0\n"
                             "RTP Sequence number     0\n"
                             "RTP Timestamp (SR Units)                      0\n"
                             "RTP SSRC ID                                   0\n"
                             "RTP CSRC IDs                                  0\n"
                             "\n"
                             "RTP Payload   NONE\n"
                             "RTP Padding   NONE\n"
                             "RTP Extension type            0\n"
                             "RTP Extension Length          0\n"
                             "\n");

    exp.extension.type = 0x15aa;

    ASSERT_DUMP_EQUALS(&exp, "\n"
                             "RTP Frame dump\n"
                             "\n"
                             "RTP Version             0\n"
                             "RTP Padding bit     false\n"
                             "RTP Extension bit    true\n"
                             "RTP Marker bit      false\n"
                             "RTP Payload type        0\n"
                             "RTP Sequence number     0\n"
                             "RTP Timestamp (SR Units)                      0\n"
                             "RTP SSRC ID                                   0\n"
                             "RTP CSRC IDs                                  0\n"
                             "\n"
                             "RTP Payload   NONE\n"
                             "RTP Padding   NONE\n"
                             "RTP Extension type         5546\n"
                             "RTP Extension Length          0\n"
                             "\n");

    exp.version = RTP_VERSION_2;

    ASSERT_DUMP_EQUALS(&exp, "\n"
                             "RTP Frame dump\n"
                             "\n"
                             "RTP Version             2\n"
                             "RTP Padding bit     false\n"
                             "RTP Extension bit    true\n"
                             "RTP Marker bit      false\n"
                             "RTP Payload type        0\n"
                             "RTP Sequence number     0\n"
                             "RTP Timestamp (SR Units)                      0\n"
                             "RTP SSRC ID                                   0\n"
                             "RTP CSRC IDs                                  0\n"
                             "\n"
                             "RTP Payload   NONE\n"
                             "RTP Padding   NONE\n"
                             "RTP Extension type         5546\n"
                             "RTP Extension Length          0\n"
                             "\n");

    /* ENCODE CSRCS */
    uint32_t csrcs[] = {3, 4, 7, 11, 0x192837ff};

    exp.csrc_ids = csrcs;
    exp.csrc_count = 5;

    ASSERT_DUMP_EQUALS(&exp, "\n"
                             "RTP Frame dump\n"
                             "\n"
                             "RTP Version             2\n"
                             "RTP Padding bit     false\n"
                             "RTP Extension bit    true\n"
                             "RTP Marker bit      false\n"
                             "RTP Payload type        0\n"
                             "RTP Sequence number     0\n"
                             "RTP Timestamp (SR Units)                      0\n"
                             "RTP SSRC ID                                   0\n"
                             "RTP CSRC IDs                                  5\n"
                             "     CSRC ID                                  3\n"
                             "     CSRC ID                                  4\n"
                             "     CSRC ID                                  7\n"
                             "     CSRC ID                                 11\n"
                             "     CSRC ID                          422066175\n"
                             "\n"
                             "RTP Payload   NONE\n"
                             "RTP Padding   NONE\n"
                             "RTP Extension type         5546\n"
                             "RTP Extension Length          0\n"
                             "\n");

    exp.extension_bit = false;
    exp.version = RTP_VERSION_1;

    ASSERT_DUMP_EQUALS(&exp, "\n"
                             "RTP Frame dump\n"
                             "\n"
                             "RTP Version             1\n"
                             "RTP Padding bit     false\n"
                             "RTP Extension bit   false\n"
                             "RTP Marker bit      false\n"
                             "RTP Payload type        0\n"
                             "RTP Sequence number     0\n"
                             "RTP Timestamp (SR Units)                      0\n"
                             "RTP SSRC ID                                   0\n"
                             "RTP CSRC IDs                                  5\n"
                             "     CSRC ID                                  3\n"
                             "     CSRC ID                                  4\n"
                             "     CSRC ID                                  7\n"
                             "     CSRC ID                                 11\n"
                             "     CSRC ID                          422066175\n"
                             "\n"
                             "RTP Payload   NONE\n"
                             "RTP Padding   NONE\n"
                             "RTP Extension NONE\n"
                             "\n");

    exp.padding_bit = true;

    ASSERT_DUMP_EQUALS(&exp, "\n"
                             "RTP Frame dump\n"
                             "\n"
                             "RTP Version             1\n"
                             "RTP Padding bit      true\n"
                             "RTP Extension bit   false\n"
                             "RTP Marker bit      false\n"
                             "RTP Payload type        0\n"
                             "RTP Sequence number     0\n"
                             "RTP Timestamp (SR Units)                      0\n"
                             "RTP SSRC ID                                   0\n"
                             "RTP CSRC IDs                                  5\n"
                             "     CSRC ID                                  3\n"
                             "     CSRC ID                                  4\n"
                             "     CSRC ID                                  7\n"
                             "     CSRC ID                                 11\n"
                             "     CSRC ID                          422066175\n"
                             "\n"
                             "RTP Payload   NONE\n"
                             "RTP Padding Length            1\n"
                             "\n"
                             "\n"
                             "RTP Extension NONE\n"
                             "\n");

    exp.marker_bit = true;

    ASSERT_DUMP_EQUALS(&exp, "\n"
                             "RTP Frame dump\n"
                             "\n"
                             "RTP Version             1\n"
                             "RTP Padding bit      true\n"
                             "RTP Extension bit   false\n"
                             "RTP Marker bit       true\n"
                             "RTP Payload type        0\n"
                             "RTP Sequence number     0\n"
                             "RTP Timestamp (SR Units)                      0\n"
                             "RTP SSRC ID                                   0\n"
                             "RTP CSRC IDs                                  5\n"
                             "     CSRC ID                                  3\n"
                             "     CSRC ID                                  4\n"
                             "     CSRC ID                                  7\n"
                             "     CSRC ID                                 11\n"
                             "     CSRC ID                          422066175\n"
                             "\n"
                             "RTP Payload   NONE\n"
                             "RTP Padding Length            1\n"
                             "\n"
                             "\n"
                             "RTP Extension NONE\n"
                             "\n");

    exp.payload_type = 0x7f;

    ASSERT_DUMP_EQUALS(&exp, "\n"
                             "RTP Frame dump\n"
                             "\n"
                             "RTP Version             1\n"
                             "RTP Padding bit      true\n"
                             "RTP Extension bit   false\n"
                             "RTP Marker bit       true\n"
                             "RTP Payload type      127\n"
                             "RTP Sequence number     0\n"
                             "RTP Timestamp (SR Units)                      0\n"
                             "RTP SSRC ID                                   0\n"
                             "RTP CSRC IDs                                  5\n"
                             "     CSRC ID                                  3\n"
                             "     CSRC ID                                  4\n"
                             "     CSRC ID                                  7\n"
                             "     CSRC ID                                 11\n"
                             "     CSRC ID                          422066175\n"
                             "\n"
                             "RTP Payload   NONE\n"
                             "RTP Padding Length            1\n"
                             "\n"
                             "\n"
                             "RTP Extension NONE\n"
                             "\n");

    exp.marker_bit = false;

    ASSERT_DUMP_EQUALS(&exp, "\n"
                             "RTP Frame dump\n"
                             "\n"
                             "RTP Version             1\n"
                             "RTP Padding bit      true\n"
                             "RTP Extension bit   false\n"
                             "RTP Marker bit      false\n"
                             "RTP Payload type      127\n"
                             "RTP Sequence number     0\n"
                             "RTP Timestamp (SR Units)                      0\n"
                             "RTP SSRC ID                                   0\n"
                             "RTP CSRC IDs                                  5\n"
                             "     CSRC ID                                  3\n"
                             "     CSRC ID                                  4\n"
                             "     CSRC ID                                  7\n"
                             "     CSRC ID                                 11\n"
                             "     CSRC ID                          422066175\n"
                             "\n"
                             "RTP Payload   NONE\n"
                             "RTP Padding Length            1\n"
                             "\n"
                             "\n"
                             "RTP Extension NONE\n"
                             "\n");

    exp.csrc_count = 0;

    ASSERT_DUMP_EQUALS(&exp, "\n"
                             "RTP Frame dump\n"
                             "\n"
                             "RTP Version             1\n"
                             "RTP Padding bit      true\n"
                             "RTP Extension bit   false\n"
                             "RTP Marker bit      false\n"
                             "RTP Payload type      127\n"
                             "RTP Sequence number     0\n"
                             "RTP Timestamp (SR Units)                      0\n"
                             "RTP SSRC ID                                   0\n"
                             "RTP CSRC IDs                                  0\n"
                             "\n"
                             "RTP Payload   NONE\n"
                             "RTP Padding Length            1\n"
                             "\n"
                             "\n"
                             "RTP Extension NONE\n"
                             "\n");

    exp.csrc_count = 5;

    ASSERT_DUMP_EQUALS(&exp, "\n"
                             "RTP Frame dump\n"
                             "\n"
                             "RTP Version             1\n"
                             "RTP Padding bit      true\n"
                             "RTP Extension bit   false\n"
                             "RTP Marker bit      false\n"
                             "RTP Payload type      127\n"
                             "RTP Sequence number     0\n"
                             "RTP Timestamp (SR Units)                      0\n"
                             "RTP SSRC ID                                   0\n"
                             "RTP CSRC IDs                                  5\n"
                             "     CSRC ID                                  3\n"
                             "     CSRC ID                                  4\n"
                             "     CSRC ID                                  7\n"
                             "     CSRC ID                                 11\n"
                             "     CSRC ID                          422066175\n"
                             "\n"
                             "RTP Payload   NONE\n"
                             "RTP Padding Length            1\n"
                             "\n"
                             "\n"
                             "RTP Extension NONE\n"
                             "\n");

    exp.sequence_number = 0x1234;

    ASSERT_DUMP_EQUALS(&exp, "\n"
                             "RTP Frame dump\n"
                             "\n"
                             "RTP Version             1\n"
                             "RTP Padding bit      true\n"
                             "RTP Extension bit   false\n"
                             "RTP Marker bit      false\n"
                             "RTP Payload type      127\n"
                             "RTP Sequence number  4660\n"
                             "RTP Timestamp (SR Units)                      0\n"
                             "RTP SSRC ID                                   0\n"
                             "RTP CSRC IDs                                  5\n"
                             "     CSRC ID                                  3\n"
                             "     CSRC ID                                  4\n"
                             "     CSRC ID                                  7\n"
                             "     CSRC ID                                 11\n"
                             "     CSRC ID                          422066175\n"
                             "\n"
                             "RTP Payload   NONE\n"
                             "RTP Padding Length            1\n"
                             "\n"
                             "\n"
                             "RTP Extension NONE\n"
                             "\n");

    exp.csrc_count = 2;

    ASSERT_DUMP_EQUALS(&exp, "\n"
                             "RTP Frame dump\n"
                             "\n"
                             "RTP Version             1\n"
                             "RTP Padding bit      true\n"
                             "RTP Extension bit   false\n"
                             "RTP Marker bit      false\n"
                             "RTP Payload type      127\n"
                             "RTP Sequence number  4660\n"
                             "RTP Timestamp (SR Units)                      0\n"
                             "RTP SSRC ID                                   0\n"
                             "RTP CSRC IDs                                  2\n"
                             "     CSRC ID                                  3\n"
                             "     CSRC ID                                  4\n"
                             "\n"
                             "RTP Payload   NONE\n"
                             "RTP Padding Length            1\n"
                             "\n"
                             "\n"
                             "RTP Extension NONE\n"
                             "\n");

    exp.timestamp = 0xa2b3c4d5;

    ASSERT_DUMP_EQUALS(&exp, "\n"
                             "RTP Frame dump\n"
                             "\n"
                             "RTP Version             1\n"
                             "RTP Padding bit      true\n"
                             "RTP Extension bit   false\n"
                             "RTP Marker bit      false\n"
                             "RTP Payload type      127\n"
                             "RTP Sequence number  4660\n"
                             "RTP Timestamp (SR Units)             2729690325\n"
                             "RTP SSRC ID                                   0\n"
                             "RTP CSRC IDs                                  2\n"
                             "     CSRC ID                                  3\n"
                             "     CSRC ID                                  4\n"
                             "\n"
                             "RTP Payload   NONE\n"
                             "RTP Padding Length            1\n"
                             "\n"
                             "\n"
                             "RTP Extension NONE\n"
                             "\n");

    exp.ssrc = 0xd1c2b3a4;

    ASSERT_DUMP_EQUALS(&exp, "\n"
                             "RTP Frame dump\n"
                             "\n"
                             "RTP Version             1\n"
                             "RTP Padding bit      true\n"
                             "RTP Extension bit   false\n"
                             "RTP Marker bit      false\n"
                             "RTP Payload type      127\n"
                             "RTP Sequence number  4660\n"
                             "RTP Timestamp (SR Units)             2729690325\n"
                             "RTP SSRC ID                          3519198116\n"
                             "RTP CSRC IDs                                  2\n"
                             "     CSRC ID                                  3\n"
                             "     CSRC ID                                  4\n"
                             "\n"
                             "RTP Payload   NONE\n"
                             "RTP Padding Length            1\n"
                             "\n"
                             "\n"
                             "RTP Extension NONE\n"
                             "\n");

    exp.marker_bit = true;

    ASSERT_DUMP_EQUALS(&exp, "\n"
                             "RTP Frame dump\n"
                             "\n"
                             "RTP Version             1\n"
                             "RTP Padding bit      true\n"
                             "RTP Extension bit   false\n"
                             "RTP Marker bit       true\n"
                             "RTP Payload type      127\n"
                             "RTP Sequence number  4660\n"
                             "RTP Timestamp (SR Units)             2729690325\n"
                             "RTP SSRC ID                          3519198116\n"
                             "RTP CSRC IDs                                  2\n"
                             "     CSRC ID                                  3\n"
                             "     CSRC ID                                  4\n"
                             "\n"
                             "RTP Payload   NONE\n"
                             "RTP Padding Length            1\n"
                             "\n"
                             "\n"
                             "RTP Extension NONE\n"
                             "\n");

    exp.csrc_count = 5;

    ASSERT_DUMP_EQUALS(&exp, "\n"
                             "RTP Frame dump\n"
                             "\n"
                             "RTP Version             1\n"
                             "RTP Padding bit      true\n"
                             "RTP Extension bit   false\n"
                             "RTP Marker bit       true\n"
                             "RTP Payload type      127\n"
                             "RTP Sequence number  4660\n"
                             "RTP Timestamp (SR Units)             2729690325\n"
                             "RTP SSRC ID                          3519198116\n"
                             "RTP CSRC IDs                                  5\n"
                             "     CSRC ID                                  3\n"
                             "     CSRC ID                                  4\n"
                             "     CSRC ID                                  7\n"
                             "     CSRC ID                                 11\n"
                             "     CSRC ID                          422066175\n"
                             "\n"
                             "RTP Payload   NONE\n"
                             "RTP Padding Length            1\n"
                             "\n"
                             "\n"
                             "RTP Extension NONE\n"
                             "\n");

    exp.padding_bit = false;

    ASSERT_DUMP_EQUALS(&exp, "\n"
                             "RTP Frame dump\n"
                             "\n"
                             "RTP Version             1\n"
                             "RTP Padding bit     false\n"
                             "RTP Extension bit   false\n"
                             "RTP Marker bit       true\n"
                             "RTP Payload type      127\n"
                             "RTP Sequence number  4660\n"
                             "RTP Timestamp (SR Units)             2729690325\n"
                             "RTP SSRC ID                          3519198116\n"
                             "RTP CSRC IDs                                  5\n"
                             "     CSRC ID                                  3\n"
                             "     CSRC ID                                  4\n"
                             "     CSRC ID                                  7\n"
                             "     CSRC ID                                 11\n"
                             "     CSRC ID                          422066175\n"
                             "\n"
                             "RTP Payload   NONE\n"
                             "RTP Padding   NONE\n"
                             "RTP Extension NONE\n"
                             "\n");

    uint8_t payload[] = {0xf1, 0xe2, 0xd3, 0xc4, 0xb5, 0xa6};
    const size_t payload_length = 6;
    exp.payload.data = payload;
    exp.payload.length = payload_length;

    ASSERT_DUMP_EQUALS(&exp, "\n"
                             "RTP Frame dump\n"
                             "\n"
                             "RTP Version             1\n"
                             "RTP Padding bit     false\n"
                             "RTP Extension bit   false\n"
                             "RTP Marker bit       true\n"
                             "RTP Payload type      127\n"
                             "RTP Sequence number  4660\n"
                             "RTP Timestamp (SR Units)             2729690325\n"
                             "RTP SSRC ID                          3519198116\n"
                             "RTP CSRC IDs                                  5\n"
                             "     CSRC ID                                  3\n"
                             "     CSRC ID                                  4\n"
                             "     CSRC ID                                  7\n"
                             "     CSRC ID                                 11\n"
                             "     CSRC ID                          422066175\n"
                             "\n"
                             "RTP Payload Length            6\n"
                             " f1 e2 d3 c4 b5 a6\n"
                             "\n"
                             "RTP Padding   NONE\n"
                             "RTP Extension NONE\n"
                             "\n");

    exp.padding.data = (uint8_t[]){0x10, 0x02, 0x30};
    exp.padding.length = 3;
    exp.padding_bit = true;

    ASSERT_DUMP_EQUALS(&exp, "\n"
                             "RTP Frame dump\n"
                             "\n"
                             "RTP Version             1\n"
                             "RTP Padding bit      true\n"
                             "RTP Extension bit   false\n"
                             "RTP Marker bit       true\n"
                             "RTP Payload type      127\n"
                             "RTP Sequence number  4660\n"
                             "RTP Timestamp (SR Units)             2729690325\n"
                             "RTP SSRC ID                          3519198116\n"
                             "RTP CSRC IDs                                  5\n"
                             "     CSRC ID                                  3\n"
                             "     CSRC ID                                  4\n"
                             "     CSRC ID                                  7\n"
                             "     CSRC ID                                 11\n"
                             "     CSRC ID                          422066175\n"
                             "\n"
                             "RTP Payload Length            6\n"
                             " f1 e2 d3 c4 b5 a6\n"
                             "\n"
                             "RTP Padding Length            4\n"
                             " 10 02 30\n"
                             "\n"
                             "RTP Extension NONE\n"
                             "\n");

    exp.payload_type = 0x71;

    ASSERT_DUMP_EQUALS(&exp, "\n"
                             "RTP Frame dump\n"
                             "\n"
                             "RTP Version             1\n"
                             "RTP Padding bit      true\n"
                             "RTP Extension bit   false\n"
                             "RTP Marker bit       true\n"
                             "RTP Payload type      113\n"
                             "RTP Sequence number  4660\n"
                             "RTP Timestamp (SR Units)             2729690325\n"
                             "RTP SSRC ID                          3519198116\n"
                             "RTP CSRC IDs                                  5\n"
                             "     CSRC ID                                  3\n"
                             "     CSRC ID                                  4\n"
                             "     CSRC ID                                  7\n"
                             "     CSRC ID                                 11\n"
                             "     CSRC ID                          422066175\n"
                             "\n"
                             "RTP Payload Length            6\n"
                             " f1 e2 d3 c4 b5 a6\n"
                             "\n"
                             "RTP Padding Length            4\n"
                             " 10 02 30\n"
                             "\n"
                             "RTP Extension NONE\n"
                             "\n");

    /* Check Extension */
    exp.extension.data =
        (uint8_t[]){0x43, 0x8f, 0x85, 0x86, 0x01, 0x1a, 0xf7, 0xdd};

    exp.extension.length = 8;
    exp.extension.type = 0x0102;

    exp.extension_bit = true;

    ASSERT_DUMP_EQUALS(&exp, "\n"
                             "RTP Frame dump\n"
                             "\n"
                             "RTP Version             1\n"
                             "RTP Padding bit      true\n"
                             "RTP Extension bit    true\n"
                             "RTP Marker bit       true\n"
                             "RTP Payload type      113\n"
                             "RTP Sequence number  4660\n"
                             "RTP Timestamp (SR Units)             2729690325\n"
                             "RTP SSRC ID                          3519198116\n"
                             "RTP CSRC IDs                                  5\n"
                             "     CSRC ID                                  3\n"
                             "     CSRC ID                                  4\n"
                             "     CSRC ID                                  7\n"
                             "     CSRC ID                                 11\n"
                             "     CSRC ID                          422066175\n"
                             "\n"
                             "RTP Payload Length            6\n"
                             " f1 e2 d3 c4 b5 a6\n"
                             "\n"
                             "RTP Padding Length            4\n"
                             " 10 02 30\n"
                             "\n"
                             "RTP Extension type          258\n"
                             "RTP Extension Length          8\n"
                             " 43 8f 85 86 01 1a f7 dd\n");

    return testrun_log_success();

#undef ASSERT_DUMP_EQUALS
}

/******************************************************************************
 *
 *  Additional functions
 *
 ******************************************************************************/

int test_rtp_frame_create() {

    ov_rtp_frame *frame = rtp_frame_create();

    testrun(frame);

    testrun(frame->bytes.length == 0);
    testrun(frame->bytes.data == 0);
    testrun(frame->expanded.version == RTP_VERSION_2);

    testrun(frame->expanded.padding_bit == false);
    testrun(frame->expanded.extension_bit == false);
    testrun(frame->expanded.marker_bit == false);

    testrun(frame->expanded.payload_type == 0);
    testrun(frame->expanded.sequence_number == 0);
    testrun(frame->expanded.timestamp == 0);

    testrun(frame->expanded.ssrc == 0);

    testrun(frame->expanded.csrc_count == 0);
    testrun(frame->expanded.csrc_ids == 0);
    testrun(frame->expanded.payload.length == 0);
    testrun(frame->expanded.payload.data == 0);
    testrun(frame->expanded.padding.length == 0);
    testrun(frame->expanded.padding.data == 0);

    testrun(0 == frame->expanded.extension.allocated_bytes);

    testrun(frame->free == impl_rtp_frame_free);
    testrun(frame->copy == impl_rtp_frame_copy);

    frame->free(frame);

    return testrun_log_success();
}

/*---------------------------------------------------------------------------*/

int test_impl_rtp_frame_free() {

    ov_rtp_frame *frame = rtp_frame_create();
    ov_rtp_frame_expansion *expanded = 0;

    testrun(0 == impl_rtp_frame_free(0));

    // empty
    testrun(0 == impl_rtp_frame_free(frame));

    frame = rtp_frame_create();
    expanded = (ov_rtp_frame_expansion *)frame;

    frame->bytes.data = calloc(1, 27);
    expanded->payload.data = frame->bytes.data + 2;
    expanded->padding.data = frame->bytes.data + 10;
    expanded->extension.data = frame->bytes.data + 7;

    expanded->version = RTP_VERSION_1;
    expanded->padding_bit = true;
    expanded->extension_bit = true;
    expanded->marker_bit = true;
    expanded->csrc_count = 1;
    expanded->payload_type = 10;
    expanded->sequence_number = 123;
    expanded->timestamp = 1234;
    expanded->ssrc = 12;
    expanded->csrc_ids = (uint32_t *)(frame->bytes.data + 2);

    testrun(0 == impl_rtp_frame_free(frame));

    /*-------------------------------------------------------------------*/
    /* With allocated extension header */

    frame = rtp_frame_create();

    expanded = (ov_rtp_frame_expansion *)frame;

    frame->bytes.data = calloc(1, 27);
    expanded->payload.data = frame->bytes.data + 2;
    expanded->padding.data = frame->bytes.data + 10;

    const size_t NUM_BYTES = 4 * 5;
    expanded->extension.data = calloc(1, NUM_BYTES);
    expanded->extension.allocated_bytes = NUM_BYTES;

    expanded->version = RTP_VERSION_1;
    expanded->padding_bit = true;
    expanded->extension_bit = true;
    expanded->marker_bit = true;
    expanded->csrc_count = 1;
    expanded->payload_type = 10;
    expanded->sequence_number = 123;
    expanded->timestamp = 1234;
    expanded->ssrc = 12;
    expanded->csrc_ids = (uint32_t *)(frame->bytes.data + 2);

    testrun(0 == impl_rtp_frame_free(frame));

    return testrun_log_success();
}

/*---------------------------------------------------------------------------*/

int test_impl_rtp_frame_copy() {

    ov_rtp_frame_expansion *expanded = 0;
    ov_rtp_frame *frame = rtp_frame_create();
    ov_rtp_frame *copy = NULL;

    copy = impl_rtp_frame_copy(frame);
    testrun(!copy);

    expanded = (ov_rtp_frame_expansion *)frame;

    frame->bytes.allocated_bytes = 412;
    frame->bytes.length = 412;
    frame->bytes.data = calloc(1, frame->bytes.allocated_bytes);
    expanded->payload.data = frame->bytes.data + 2;
    expanded->padding.data = frame->bytes.data + 9;

    expanded->version = RTP_VERSION_1;
    expanded->padding_bit = true;
    expanded->extension_bit = true;
    expanded->marker_bit = true;
    expanded->csrc_count = 17;
    expanded->payload_type = 10;
    expanded->sequence_number = 123;
    expanded->timestamp = 1234;
    expanded->ssrc = 12;

    copy = impl_rtp_frame_copy(frame);
    testrun(copy);

    TODO("make some data copy tests");

    testrun(0 == impl_rtp_frame_free(frame));
    testrun(0 == impl_rtp_frame_free(copy));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int tear_down() {

    ov_registered_cache_free_all();
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

OV_TEST_RUN("ov_rtp_frame", test_ov_rtp_frame_enable_caching,
            test_ov_rtp_frame_encode, test_ov_rtp_frame_decode,
            test_ov_rtp_frame_free, test_ov_rtp_frame_dump,
            test_rtp_frame_create, test_impl_rtp_frame_free,
            test_impl_rtp_frame_copy, tear_down);
