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
        @file           ov_rtp_frame_buffer_test.c
        @author         Michael Beer

        @date           2019-02-15

        @ingroup        ov_rtp

        @brief          Unit tests of ov_rtp_frame_buffer_test


        ------------------------------------------------------------------------
*/
#include "ov_rtp_frame_buffer.c"

#include "../../include/ov_utils.h"
#include <ov_test/ov_test.h>
#include <ov_test/testrun.h>

/*----------------------------------------------------------------------------*/

static ov_rtp_frame *create_test_frame(uint32_t ssid, uint32_t sequence_number,
                                       ov_list *frame_list) {

    uint8_t input_bytes[1024] = {0};

    size_t num_bytes = RTP_HEADER_MIN_LENGTH;

    ov_rtp_frame *frame = ov_rtp_frame_decode(input_bytes, num_bytes);

    OV_ASSERT(frame);

    if (0 != frame_list) {
        frame_list->push(frame_list, frame);
    }

    frame->expanded.ssrc = ssid;
    frame->expanded.sequence_number = sequence_number;

    return frame;
}

/*----------------------------------------------------------------------------*/

void *free_frame_list(ov_list *list) {

    if (0 == list)
        return 0;

    list->for_each(list, 0, free_rtp_frame);

    return list->free(list);
}

/*----------------------------------------------------------------------------*/

bool gather_ssids(void *arg, void const *key, void *value) {

    UNUSED(value);

    ov_list *list = arg;
    testrun(list);
    list->push(list, (void *)key);

    return true;
}

/*----------------------------------------------------------------------------*/

#define FILL_BUFFER(buffer, ...)                                               \
    fill_buffer(buffer, (uint32_t[][2]){__VA_ARGS__, {0, 0}})

bool fill_buffer(ov_rtp_frame_buffer *buffer, uint32_t const data[][2]) {

    size_t i = 0;

    while ((0 != data[i][1]) || (0 != data[i][0])) {

        ov_rtp_frame_buffer_add(buffer,
                                create_test_frame(data[i][0], data[i][1], 0));

        ++i;
    };

    return true;
}

/*----------------------------------------------------------------------------*/

/**
 * Checks whether `list` contains EXACTLY frames with ssid and sequence_number
 * @param expected array of uint32_t[2]. Last element MUST be {0, 0}
 */
#define LIST_CONTAINS(list, ...)                                               \
    list_contains(list, (uint32_t[][2]){__VA_ARGS__, {0, 0}})

bool list_contains(ov_list *list, uint32_t const expected[][2]) {

    if (0 == list)
        return false;

    size_t i = 0;

    bool found = false;

    while ((0 != expected[i][1]) || (0 != expected[i][0])) {

        found = false;

        size_t const count = list->count(list);

        for (size_t j = 1; count >= j; ++j) {

            ov_rtp_frame const *frame = list->get(list, j);
            uint32_t const ssid = frame->expanded.ssrc;
            uint32_t const sequence_number = frame->expanded.sequence_number;
            found = (ssid == expected[i][0]);
            found = (sequence_number == expected[i][1]);

            if (found) {
                ov_rtp_frame *f = list->remove(list, j);
                f = f->free(f);
                break;
            }
        }

        if (!found)
            return false;

        ++i;
    }

    found = list->count(list) == 0;

    return found;
}

/******************************************************************************
 *                                   TESTS
 ******************************************************************************/

int test_ov_rtp_frame_buffer_create() {

    ov_rtp_frame_buffer_config config = {0};

    ov_rtp_frame_buffer *buffer = ov_rtp_frame_buffer_create(config);
    testrun(0 != buffer);
    testrun(0 != buffer->free);

    buffer = buffer->free(buffer);

    buffer = ov_rtp_frame_buffer_create((ov_rtp_frame_buffer_config){
        .num_frames_to_buffer_per_stream = 3,
    });

    testrun(0 != buffer);

    buffer = buffer->free(buffer);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_rtp_frame_buffer_add() {

    ov_rtp_frame *frame = create_test_frame(1, 1, 0);

    testrun(frame == ov_rtp_frame_buffer_add(0, frame));

    ov_rtp_frame_buffer *buffer =
        ov_rtp_frame_buffer_create((ov_rtp_frame_buffer_config){
            .num_frames_to_buffer_per_stream = 1,
        });

    testrun(0 == ov_rtp_frame_buffer_add(buffer, frame));

    /* The same frame cannot be inserted twice */
    testrun(frame == ov_rtp_frame_buffer_add(buffer, frame));
    frame = 0; /* Frame contained in buffer ! */

    frame = create_test_frame(1, 1, 0);
    /* Same SSID and same sequence number cannot be inserted twice */

    ov_rtp_frame *old = ov_rtp_frame_buffer_add(buffer, frame);
    testrun(0 != old);
    old = old->free(old);

    ov_rtp_frame *frame_smaller_sequence_number = create_test_frame(2, 1, 0);
    testrun(0 ==
            ov_rtp_frame_buffer_add(buffer, frame_smaller_sequence_number));

    frame = create_test_frame(2, 2, 0);
    ov_rtp_frame const *remainder = ov_rtp_frame_buffer_add(buffer, frame);
    testrun(0 != remainder);
    testrun(remainder == frame_smaller_sequence_number);

    frame_smaller_sequence_number =
        frame_smaller_sequence_number->free(frame_smaller_sequence_number);

    buffer = buffer->free(buffer);

    /**********************************************************************
     *               buffering 2 frames per stream
     **********************************************************************/

    buffer = ov_rtp_frame_buffer_create((ov_rtp_frame_buffer_config){
        .num_frames_to_buffer_per_stream = 2,
    });

    frame = create_test_frame(1, 1, 0);
    testrun(0 == ov_rtp_frame_buffer_add(buffer, frame));

    /* the same frame cannot be inserted twice */
    testrun(frame == ov_rtp_frame_buffer_add(buffer, frame));
    frame = 0; /* Frame contained in frame buffer ! */

    frame = create_test_frame(1, 1, 0);
    /* same ssid and same sequence number cannot be inserted twice */

    testrun(frame == ov_rtp_frame_buffer_add(buffer, frame));
    frame = frame->free(frame);

    frame_smaller_sequence_number = create_test_frame(2, 1, 0);
    testrun(0 ==
            ov_rtp_frame_buffer_add(buffer, frame_smaller_sequence_number));

    frame = create_test_frame(2, 2, 0);
    testrun(0 == ov_rtp_frame_buffer_add(buffer, frame));

    frame = create_test_frame(2, 3, 0);
    remainder = ov_rtp_frame_buffer_add(buffer, frame);
    testrun(frame_smaller_sequence_number == remainder);

    frame_smaller_sequence_number =
        frame_smaller_sequence_number->free(frame_smaller_sequence_number);
    remainder = 0;

    frame = create_test_frame(2, 2, 0);
    testrun(frame == ov_rtp_frame_buffer_add(buffer, frame));
    frame = frame->free(frame);

    frame = create_test_frame(3, 1, 0);
    testrun(0 == ov_rtp_frame_buffer_add(buffer, frame));

    buffer = buffer->free(buffer);

    /**********************************************************************
     *               buffering 3 frames per stream
     **********************************************************************/

    buffer = ov_rtp_frame_buffer_create((ov_rtp_frame_buffer_config){
        .num_frames_to_buffer_per_stream = 3,
    });

    frame = create_test_frame(1, 1, 0);
    testrun(0 == ov_rtp_frame_buffer_add(buffer, frame));

    /* the same frame cannot be inserted twice */
    testrun(frame == ov_rtp_frame_buffer_add(buffer, frame));
    frame = 0; /* Frame contained in frame buffer */

    frame = create_test_frame(1, 1, 0);
    /* same ssid and same sequence number cannot be inserted twice */

    testrun(frame = ov_rtp_frame_buffer_add(buffer, frame));
    frame = frame->free(frame);

    frame_smaller_sequence_number = create_test_frame(2, 1, 0);
    testrun(0 ==
            ov_rtp_frame_buffer_add(buffer, frame_smaller_sequence_number));

    frame = create_test_frame(2, 2, 0);
    testrun(0 == ov_rtp_frame_buffer_add(buffer, frame));

    frame = create_test_frame(2, 3, 0);
    testrun(0 == ov_rtp_frame_buffer_add(buffer, frame));

    frame = create_test_frame(1, 3, 0);
    testrun(0 == ov_rtp_frame_buffer_add(buffer, frame));

    /* Same again ... */
    frame = create_test_frame(1, 3, 0);
    testrun(frame == ov_rtp_frame_buffer_add(buffer, frame));
    frame = frame->free(frame);

    frame = create_test_frame(3, 3, 0);
    testrun(0 == ov_rtp_frame_buffer_add(buffer, frame));

    frame = create_test_frame(3, 1, 0);
    testrun(0 == ov_rtp_frame_buffer_add(buffer, frame));

    frame = create_test_frame(2, 4, 0);

    /* All stages got SSID 2, now add a newer frame -> the oldest
     * (with the lowest SEQUENCE NUMBER) should be dropped */
    frame = ov_rtp_frame_buffer_add(buffer, frame);
    testrun(frame_smaller_sequence_number == frame);
    frame = frame->free(frame);

    frame_smaller_sequence_number = 0;

    frame = create_test_frame(2, 2, 0);
    testrun(frame == ov_rtp_frame_buffer_add(buffer, frame));
    frame = frame->free(frame);

    frame = create_test_frame(3, 2, 0);
    testrun(0 == ov_rtp_frame_buffer_add(buffer, frame));

    frame = create_test_frame(3, 2, 0);
    testrun(frame == ov_rtp_frame_buffer_add(buffer, frame));
    frame = frame->free(frame);

    frame = create_test_frame(3, 5, 0);
    frame = ov_rtp_frame_buffer_add(buffer, frame);
    testrun(0 != frame);
    frame = frame->free(frame);

    frame = create_test_frame(122, 2, 0);
    testrun(0 == ov_rtp_frame_buffer_add(buffer, frame));

    frame = create_test_frame(122, 17, 0);
    testrun(0 == ov_rtp_frame_buffer_add(buffer, frame));

    frame = create_test_frame(122, 1, 0);
    testrun(0 == ov_rtp_frame_buffer_add(buffer, frame));

    frame = create_test_frame(122, 14, 0);
    frame = ov_rtp_frame_buffer_add(buffer, frame);
    testrun((0 != frame) && (122 == frame->expanded.ssrc) &&
            (1 == frame->expanded.sequence_number));
    frame = frame->free(frame);

    buffer = buffer->free(buffer);

    buffer = ov_rtp_frame_buffer_create((ov_rtp_frame_buffer_config){
        .num_frames_to_buffer_per_stream = 3,
    });

    frame = create_test_frame(2, 2, 0);
    testrun(0 != frame);
    testrun(0 == ov_rtp_frame_buffer_add(buffer, frame));
    frame = create_test_frame(1, 2, 0);
    testrun(0 != frame);
    testrun(0 == ov_rtp_frame_buffer_add(buffer, frame));
    frame = create_test_frame(2, 1, 0);
    testrun(0 != frame);
    testrun(0 == ov_rtp_frame_buffer_add(buffer, frame));
    frame = create_test_frame(3, 3, 0);
    testrun(0 != frame);
    testrun(0 == ov_rtp_frame_buffer_add(buffer, frame));

    buffer = buffer->free(buffer);
    testrun(0 == buffer);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_rtp_frame_buffer_get_current_frames() {

    testrun(0 == ov_rtp_frame_buffer_get_current_frames(0));

    ov_rtp_frame_buffer *buffer =
        ov_rtp_frame_buffer_create((ov_rtp_frame_buffer_config){
            .num_frames_to_buffer_per_stream = 1,
        });

    ov_list *current = ov_rtp_frame_buffer_get_current_frames(buffer);
    testrun(0 == current);

    /* Now, fill the buffer with stuff */
    ov_rtp_frame *frame = create_test_frame(1, 2, 0);
    testrun(0 == ov_rtp_frame_buffer_add(buffer, frame));

    current = ov_rtp_frame_buffer_get_current_frames(buffer);
    testrun(1 == ov_list_count(current));

    testrun(LIST_CONTAINS(current, {1, 2}));

    current = free_frame_list(current);

    FILL_BUFFER(buffer, {1, 2}, {2, 1});

    current = ov_rtp_frame_buffer_get_current_frames(buffer);
    testrun(LIST_CONTAINS(current, {1, 2}, {2, 1}));

    current = free_frame_list(current);

    FILL_BUFFER(buffer, {1, 2}, {2, 1}, {3, 3});
    current = ov_rtp_frame_buffer_get_current_frames(buffer);
    testrun(LIST_CONTAINS(current, {1, 2}, {2, 1}, {3, 3}));

    current = free_frame_list(current);

    buffer = buffer->free(buffer);

    /**********************************************************************
                         Check with more stages
     **********************************************************************/
    buffer = ov_rtp_frame_buffer_create((ov_rtp_frame_buffer_config){
        .num_frames_to_buffer_per_stream = 3,
    });

    current = ov_rtp_frame_buffer_get_current_frames(buffer);
    testrun(0 == current);

    /* Now, fill the buffer with stuff */
    frame = create_test_frame(1, 2, 0);
    testrun(0 == ov_rtp_frame_buffer_add(buffer, frame));

    current = ov_rtp_frame_buffer_get_current_frames(buffer);

    testrun(LIST_CONTAINS(current, {1, 2}));

    current = free_frame_list(current);

    FILL_BUFFER(buffer, {1, 2}, {2, 1});

    current = ov_rtp_frame_buffer_get_current_frames(buffer);
    testrun(LIST_CONTAINS(current, {1, 2}, {2, 1}));

    current = free_frame_list(current);

    FILL_BUFFER(buffer, {1, 2}, {2, 1}, {3, 3});
    current = ov_rtp_frame_buffer_get_current_frames(buffer);
    testrun(LIST_CONTAINS(current, {1, 2}, {2, 1}, {3, 3}));

    current = free_frame_list(current);

    FILL_BUFFER(buffer, {2, 2}, {1, 2}, {2, 1}, {3, 3});
    current = ov_rtp_frame_buffer_get_current_frames(buffer);
    testrun(LIST_CONTAINS(current, {1, 2}, {2, 1}, {3, 3}));

    current = free_frame_list(current);

    FILL_BUFFER(buffer, {1, 2}, {2, 1}, {3, 3});
    current = ov_rtp_frame_buffer_get_current_frames(buffer);
    testrun(LIST_CONTAINS(current, {1, 2}, {2, 1}, {3, 3}));
    current = free_frame_list(current);

    FILL_BUFFER(buffer, {3, 1}, {1, 15}, {1, 2}, {2, 1}, {3, 3});
    current = ov_rtp_frame_buffer_get_current_frames(buffer);
    testrun(LIST_CONTAINS(current, {1, 2}, {2, 1}, {3, 1}));
    current = free_frame_list(current);

    current = ov_rtp_frame_buffer_get_current_frames(buffer);
    testrun(LIST_CONTAINS(current, {3, 3}, {2, 2}, {1, 15}));
    current = free_frame_list(current);

    /* FILL ALL 3 stages */
    FILL_BUFFER(buffer, {3, 1}, {1, 15}, {1, 2}, {2, 1}, {3, 3});
    FILL_BUFFER(buffer, {1, 12}, {2, 10});

    /* STAGE 0 */
    current = ov_rtp_frame_buffer_get_current_frames(buffer);
    testrun(LIST_CONTAINS(current, {1, 2}, {2, 1}, {3, 1}));
    current = free_frame_list(current);

    /* STAGE 1 */
    current = ov_rtp_frame_buffer_get_current_frames(buffer);
    testrun(LIST_CONTAINS(current, {2, 10}, {1, 12}, {3, 3}));
    current = free_frame_list(current);

    /* STAGE 2 */
    current = ov_rtp_frame_buffer_get_current_frames(buffer);
    testrun(LIST_CONTAINS(current, {1, 15}));

    current = free_frame_list(current);

    /* Should not be empty again */
    current = ov_rtp_frame_buffer_get_current_frames(buffer);
    testrun(0 == current);

    buffer = buffer->free(buffer);
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_impl_free() {

    ov_rtp_frame_buffer *buffer =
        ov_rtp_frame_buffer_create((ov_rtp_frame_buffer_config){
            .num_frames_to_buffer_per_stream = 1,
        });

    testrun(0 == buffer->free(buffer));

    buffer = ov_rtp_frame_buffer_create((ov_rtp_frame_buffer_config){
        .num_frames_to_buffer_per_stream = 3,
    });

    testrun(0 == buffer->free(buffer));

    buffer = ov_rtp_frame_buffer_create((ov_rtp_frame_buffer_config){
        .num_frames_to_buffer_per_stream = 1,
    });

    FILL_BUFFER(buffer, {1, 1});
    testrun(0 == buffer->free(buffer));

    buffer = ov_rtp_frame_buffer_create((ov_rtp_frame_buffer_config){
        .num_frames_to_buffer_per_stream = 1,
    });

    FILL_BUFFER(buffer, {2, 2}, {7, 15}, {1, 1});
    testrun(0 == buffer->free(buffer));

    buffer = ov_rtp_frame_buffer_create((ov_rtp_frame_buffer_config){
        .num_frames_to_buffer_per_stream = 5,
    });

    FILL_BUFFER(buffer, {1, 1});
    testrun(0 == buffer->free(buffer));

    buffer = ov_rtp_frame_buffer_create((ov_rtp_frame_buffer_config){
        .num_frames_to_buffer_per_stream = 5,
    });

    FILL_BUFFER(buffer, {2, 2}, {7, 15}, {1, 1});
    testrun(0 == buffer->free(buffer));

    buffer = ov_rtp_frame_buffer_create((ov_rtp_frame_buffer_config){
        .num_frames_to_buffer_per_stream = 5,
    });

    FILL_BUFFER(buffer, {2, 15}, {1, 11}, {6, 14}, {41, 15}, {19, 3}, {19, 1},
                {6, 11}, {6, 999}, {999, 1}, {2, 1}, {1, 4}, {6, 2}, {6, 13},
                {2, 2}, {7, 15}, {1, 1});
    testrun(0 == buffer->free(buffer));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_rtp_frame_buffer_print() {

    ov_rtp_frame_buffer *buffer =
        ov_rtp_frame_buffer_create((ov_rtp_frame_buffer_config){
            .num_frames_to_buffer_per_stream = 1,
        });

    ov_rtp_frame_buffer_print(NULL, NULL);
    ov_rtp_frame_buffer_print(NULL, buffer);
    ov_rtp_frame_buffer_print(stdout, NULL);

    ov_rtp_frame_buffer_print(stdout, buffer);

    testrun(0 == buffer->free(buffer));

    return testrun_log_success();
}

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CLUSTER                                                    #CLUSTER
 *
 *      ------------------------------------------------------------------------
 */

OV_TEST_RUN("ov_rtp_frame_buffer", test_ov_rtp_frame_buffer_create,
            test_ov_rtp_frame_buffer_add,
            test_ov_rtp_frame_buffer_get_current_frames, test_impl_free,
            test_ov_rtp_frame_buffer_print);
