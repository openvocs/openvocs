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

#include <ov_base/ov_utils.h>

#include <ov_test/ov_test.h>

#include <ov_base/ov_linked_list.h>
#include <ov_base/ov_socket.h>

#include <ov_codec/ov_codec_raw.h>

#include "ov_frame_data.c"
#include <inttypes.h>
#include <ov_base/ov_constants.h>
#include <ov_codec/ov_codec_pcm16_signed.h>

/*----------------------------------------------------------------------------*/

static int test_ov_frame_data_enable_caching() {

    // Get clean
    ov_registered_cache_free_all();

    // No real caching probably - however, we should be able to create & free
    ov_frame_data_enable_caching(0);

    ov_frame_data *data = ov_frame_data_create();

    testrun(0 != data);

    data = ov_frame_data_free(data);
    testrun(0 == data);

    // Empty cache if there
    for (size_t i = 0; i < 10; ++i) {
        data = ov_frame_data_create();
        free_frame_data(data);
    }

    // Real caching
    ov_frame_data_enable_caching(3);

    data = ov_frame_data_create();
    testrun(0 != data);

    ov_frame_data *cached = data;

    data = ov_frame_data_free(data);
    testrun(0 == data);

    // should have been cached
    data = ov_frame_data_create();
    testrun(data == cached);
    cached = 0;

    data = ov_frame_data_free(data);

    // Dealing with more datas than cache buckets available

    const size_t CACHE_SIZE = 20;

    for (size_t i = 0; i < CACHE_SIZE; ++i) {
        data = ov_frame_data_create();
        testrun(0 != data);

        data->ssid = 1 + i;

        testrun(0 == data->pcm16s_32bit);

        data = ov_frame_data_free(data);
        testrun(0 == data);
    }

    // Put non-empty datas into cache

    data = ov_frame_data_create();
    testrun(0 != data);

    testrun(0 == data->pcm16s_32bit);

    data->pcm16s_32bit = ov_buffer_create(12);

    data = ov_frame_data_free(data);
    testrun(0 == data);

    ov_frame_data **datas = calloc(CACHE_SIZE + 1, sizeof(ov_frame_data *));

    // Check that there are really only empty frames in cache
    // We need to buffer all the lists, otherwise they are put back into the
    // cache and we will not see all cached lists
    for (size_t i = 0; i < CACHE_SIZE + 1; ++i) {

        data = ov_frame_data_create();

        testrun(0 == data->pcm16s_32bit);
        datas[i] = data;

        data = 0;
    }

    // and free the lists
    for (size_t i = 0; i < CACHE_SIZE + 1; ++i) {

        datas[i] = ov_frame_data_free(datas[i]);
        testrun(0 == datas[i]);
    }

    free(datas);
    datas = 0;

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_frame_data_create() {

    ov_frame_data *data = ov_frame_data_create();

    testrun(0 != data);
    testrun(0 == data->pcm16s_32bit);
    testrun(FRAME_DATA_MAGIC_BYTES == data->magic_bytes);

    data = ov_frame_data_free(data);
    testrun(0 == data);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_frame_data_free() {

    testrun(0 == ov_frame_data_free(0));

    ov_frame_data *data = ov_frame_data_create();
    testrun(0 != data);

    data->ssid = 12;
    data->pcm16s_32bit = ov_buffer_create(8);
    testrun(0 != data->pcm16s_32bit);

    data = ov_frame_data_free(data);

    testrun(0 == data);

    return testrun_log_success();
}

/*****************************************************************************
                                    ENCODING
 ****************************************************************************/

static int32_t test_payload[] = {1, -1, 2, -2, 3, -3, 4, -4};

static void fill_frame_data(ov_frame_data *data) {

    _Static_assert(sizeof(test_payload) % 4 == 0,
                   "Test payload needs to be a multiple of 4");
    OV_ASSERT(0 != data);

    data->payload_type = 18;
    data->sequence_number = 3812;
    data->ssid = 0x12131415;
    data->timestamp = 0x91827364;

    data->pcm16s_32bit = ov_buffer_create(sizeof(test_payload));

    int32_t *data_ptr = (int32_t *)data->pcm16s_32bit->start;

    for (size_t i = 0; i < sizeof(test_payload) / 4; ++i) {
        data_ptr[i] = test_payload[i];
    }

    data->pcm16s_32bit->length = sizeof(test_payload);
}

/*----------------------------------------------------------------------------*/

static ov_codec *get_test_codec(uint32_t ssid) {

    return ov_codec_factory_get_codec(0, ov_codec_pcm16_signed_id(), ssid, 0);
}

/*----------------------------------------------------------------------------*/

#define ENSURE(cond, message)                                                  \
    if (!(cond)) {                                                             \
        testrun_log_error(message);                                            \
        return false;                                                          \
    }

static bool rtp_frame_equals_data(ov_rtp_frame const *frame,
                                  ov_frame_data const *data,
                                  ov_codec *codec) {

    if (0 == frame) {

        return 0 == data;
    }

    OV_ASSERT(0 != frame);

    if (0 == data) {

        testrun_log_error("data is 0, while frame is not");
        return false;
    }

    if (0 == codec) {
        testrun_log_error("No codec given");
        return false;
    }

    ov_rtp_frame_expansion const *exp = &frame->expanded;

    OV_ASSERT(0 != exp);

    OV_ASSERT(0 != codec);

    ov_buffer *decoded = ov_buffer_create(data->pcm16s_32bit->length);
    OV_ASSERT(0 != decoded);

    int32_t bytes_decoded = ov_codec_decode(codec,
                                            exp->sequence_number,
                                            exp->payload.data,
                                            exp->payload.length,
                                            decoded->start,
                                            decoded->capacity);

    ENSURE(0 < bytes_decoded, "Decoding failed");
    ENSURE((ssize_t)data->pcm16s_32bit->length == 2 * bytes_decoded,
           "Payload lengths do not match");

    int32_t *data_ptr = (int32_t *)data->pcm16s_32bit->start;
    int16_t *decoded_ptr = (int16_t *)decoded->start;

    for (size_t i = 0; i < (size_t)(bytes_decoded / 2); ++i) {

        fprintf(stderr,
                "%" PRIi16 "   %" PRIi16 "   %" PRIi32 "\n",
                decoded_ptr[i],
                decoded_ptr[i],
                data_ptr[i]);
        ENSURE(data_ptr[i] == decoded_ptr[i], "Payloads do not match");
    }

    decoded = ov_buffer_free(decoded);
    OV_ASSERT(0 == decoded);

    ENSURE(exp->sequence_number == data->sequence_number, "SEQ do not equal");
    ENSURE(exp->timestamp == data->timestamp, "Timestamps do not equal");

    ENSURE(
        exp->payload_type == data->payload_type, "Payload types do not equal");

    ENSURE(exp->ssrc == data->ssid, "SSIDs do not equal");

    return true;
}

/*----------------------------------------------------------------------------*/

static int test_ov_frame_data_encode_with_codec() {

    testrun(0 == ov_frame_data_encode_with_codec(0, 0));

    ov_frame_data *data = ov_frame_data_create();
    testrun(0 != data);

    fill_frame_data(data);

    /*-----------------------------------------------------------------------*/

    testrun(0 == ov_frame_data_encode_with_codec(data, 0));

    /*------------------------------------------------------------------------*/

    ov_codec *codec = get_test_codec(data->ssid);
    testrun(0 != codec);

    testrun(0 == ov_frame_data_encode_with_codec(0, codec));

    ov_rtp_frame *frame = ov_frame_data_encode_with_codec(data, codec);
    testrun(0 != frame);

    ov_rtp_frame *decoded =
        ov_rtp_frame_decode(frame->bytes.data, frame->bytes.length);

    testrun(0 != decoded);

    testrun(rtp_frame_equals_data(frame, data, codec));

    decoded = ov_rtp_frame_free(decoded);
    testrun(0 == decoded);

    frame = ov_rtp_frame_free(frame);
    testrun(0 == frame);

    codec = ov_codec_free(codec);
    testrun(0 == codec);

    data = ov_frame_data_free(data);
    testrun(0 == data);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int tear_down() {

    ov_codec_factory_free(0);
    ov_registered_cache_free_all();
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

OV_TEST_RUN("ov_frame_data",
            test_ov_frame_data_enable_caching,
            test_ov_frame_data_create,
            test_ov_frame_data_free,
            test_ov_frame_data_encode_with_codec,
            tear_down)

/*----------------------------------------------------------------------------*/
