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
        @author         Michael Beer

        ------------------------------------------------------------------------
*/
#include "ov_rtp_stream_buffer.c"

#include "../../include/ov_utils.h"
#include <ov_test/ov_test.h>
#include <ov_test/testrun.h>

/*****************************************************************************
                                    HELPERS
 ****************************************************************************/

static ov_rtp_frame *make_frame(uint32_t ssrc, uint32_t seq,
                                uint32_t timestamp) {

    ov_rtp_frame_expansion ref = {
        .version = 2,
        .ssrc = ssrc,
        .sequence_number = seq,
        .timestamp = timestamp,
    };

    return ov_rtp_frame_encode(&ref);
}

/*----------------------------------------------------------------------------*/

int test_ov_rtp_stream_buffer_create() {

    testrun(0 == ov_rtp_stream_buffer_create(0));

    ov_rtp_stream_buffer *sb = ov_rtp_stream_buffer_create(1);
    testrun(0 != sb);

    sb = ov_rtp_stream_buffer_free(sb);
    testrun(0 == sb);

    sb = ov_rtp_stream_buffer_create(10);
    testrun(0 != sb);

    sb = ov_rtp_stream_buffer_free(sb);
    testrun(0 == sb);

    sb = ov_rtp_stream_buffer_create(1);
    testrun(0 != sb);

    sb = ov_rtp_stream_buffer_free(sb);
    testrun(0 == sb);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_rtp_stream_buffer_free() {

    testrun(0 == ov_rtp_stream_buffer_free(0));

    ov_rtp_stream_buffer *sb = ov_rtp_stream_buffer_create(1);
    testrun(0 != sb);

    sb = ov_rtp_stream_buffer_free(sb);
    testrun(0 == sb);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_rtp_stream_buffer_accept() {

    bool ov_rtp_stream_buffer_accept(ov_rtp_stream_buffer * self,
                                     uint32_t lower, uint32_t upper);

    testrun(!ov_rtp_stream_buffer_accept(0, 0, 0));

    ov_rtp_stream_buffer *buffer = ov_rtp_stream_buffer_create(5);

    testrun(!ov_rtp_stream_buffer_accept(buffer, 0, 0));
    testrun(!ov_rtp_stream_buffer_accept(buffer, 0, 1));

    // Accept effectively only 1
    testrun(ov_rtp_stream_buffer_accept(buffer, 1, 1));

    ov_rtp_frame *frame = make_frame(1, 1, 1);

    testrun(ov_rtp_stream_buffer_put(buffer, frame));

    testrun(!ov_rtp_stream_buffer_accept(buffer, 0, 0));
    testrun(!ov_rtp_stream_buffer_accept(buffer, 0, 1));

    testrun(!ov_rtp_stream_buffer_put(buffer, frame));
    testrun(ov_rtp_stream_buffer_accept(buffer, 1, 1));

    frame = make_frame(1, 1, 1);
    testrun(ov_rtp_stream_buffer_put(buffer, frame));

    frame = make_frame(2, 1, 1);
    testrun(!ov_rtp_stream_buffer_put(buffer, frame));

    frame = ov_rtp_frame_free(frame);
    testrun(0 == frame);

    buffer = ov_rtp_stream_buffer_free(buffer);
    testrun(0 == buffer);

    buffer = ov_rtp_stream_buffer_create(5);

    testrun(ov_rtp_stream_buffer_accept(buffer, 2, 10));

    frame = make_frame(1, 1, 1);
    testrun(!ov_rtp_stream_buffer_put(buffer, frame));
    frame = ov_rtp_frame_free(frame);

    frame = make_frame(11, 11, 11);
    testrun(!ov_rtp_stream_buffer_put(buffer, frame));
    frame = ov_rtp_frame_free(frame);

    frame = make_frame(3, 3, 3);
    testrun(ov_rtp_stream_buffer_put(buffer, frame));

    // Frame outside of sliding window (3 - 8)
    frame = make_frame(10, 10, 10);
    testrun(!ov_rtp_stream_buffer_put(buffer, frame));
    frame = ov_rtp_frame_free(frame);

    frame = make_frame(8, 7, 7);
    testrun(ov_rtp_stream_buffer_put(buffer, frame));

    frame = make_frame(5, 5, 5);
    testrun(ov_rtp_stream_buffer_put(buffer, frame));

    buffer = ov_rtp_stream_buffer_free(buffer);
    testrun(0 == buffer);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_rtp_stream_buffer_put() {

    ov_rtp_frame *frame = make_frame(1, 10, 320);

    testrun(!ov_rtp_stream_buffer_put(0, 0));
    testrun(!ov_rtp_stream_buffer_put(0, frame));

    ov_rtp_stream_buffer *buffer = ov_rtp_stream_buffer_create(5);
    testrun(ov_rtp_stream_buffer_accept(buffer, 1, 1));

    testrun(!ov_rtp_stream_buffer_put(buffer, 0));

    ov_rtp_stream_buffer_lookahead_info lai =
        ov_rtp_stream_buffer_lookahead(buffer);
    testrun(0 == lai.number_of_frames_ready);

    testrun(ov_rtp_stream_buffer_put(buffer, frame));
    frame = 0;
    lai = ov_rtp_stream_buffer_lookahead(buffer);
    testrun(1 == lai.number_of_frames_ready);
    testrun(10 == lai.sequence_number);

    // buffer after next operation:
    // slot 4: 0
    // slot 3: 0
    // slot 2: 0
    // slot 1: 13
    // slot 0: 10    <= Ready
    testrun(ov_rtp_stream_buffer_put(buffer, make_frame(1, 13, 320)));
    lai = ov_rtp_stream_buffer_lookahead(buffer);
    testrun(1 == lai.number_of_frames_ready);
    testrun(10 == lai.sequence_number);

    // Try to insert a frame outside of the sliding window ...

    frame = make_frame(1, 15, 320);
    testrun(!ov_rtp_stream_buffer_put(buffer, frame));
    frame = ov_rtp_frame_free(frame);

    frame = make_frame(1, 9, 320);
    testrun(!ov_rtp_stream_buffer_put(buffer, frame));
    frame = ov_rtp_frame_free(frame);

    // buffer after next operation:
    // slot 4: 0
    // slot 3: 0
    // slot 2: 34
    // slot 1: 13
    // slot 0: 10    <= Ready
    testrun(ov_rtp_stream_buffer_put(buffer, make_frame(1, 14, 330)));
    lai = ov_rtp_stream_buffer_lookahead(buffer);
    testrun(1 == lai.number_of_frames_ready);
    testrun(10 == lai.sequence_number);

    // buffer after next operation:
    // slot 4: 0
    // slot 3: 14
    // slot 2: 13
    // slot 1: 11   <= Ready
    // slot 0: 10
    testrun(ov_rtp_stream_buffer_put(buffer, make_frame(1, 11, 110)));
    lai = ov_rtp_stream_buffer_lookahead(buffer);
    testrun(2 == lai.number_of_frames_ready);
    testrun(10 == lai.sequence_number);

    // buffer after next op:
    // slot 4: 0
    // slot 3: 14
    // slot 2: 13
    // slot 1: 11   <= Ready
    // slot 0: 10
    //  Same sequence number will not be inserted twice
    frame = make_frame(1, 11, 110);
    testrun(!ov_rtp_stream_buffer_put(buffer, frame));
    lai = ov_rtp_stream_buffer_lookahead(buffer);
    testrun(2 == lai.number_of_frames_ready);
    testrun(10 == lai.sequence_number);

    frame = ov_rtp_frame_free(frame);

    // buffer after next op:
    // slot 4: 14 <= Ready
    // slot 3: 13
    // slot 2: 12
    // slot 1: 11
    // slot 0: 10
    testrun(ov_rtp_stream_buffer_put(buffer, make_frame(1, 12, 200)));
    ov_rtp_stream_buffer_print(buffer, stderr);
    lai = ov_rtp_stream_buffer_lookahead(buffer);
    testrun(5 == lai.number_of_frames_ready);
    testrun(10 == lai.sequence_number);

    // buffer:
    // slot 4: 14 <= Ready
    // slot 3: 13
    // slot 2: 12
    // slot 1: 11
    // slot 0: 10
    //
    // Sliding window is 5 frames.
    // 8 wont be inserted since it is outside of the sliding window
    frame = make_frame(1, 8, 80);
    testrun(!ov_rtp_stream_buffer_put(buffer, frame));
    frame = ov_rtp_frame_free(frame);
    ov_rtp_stream_buffer_print(buffer, stderr);
    lai = ov_rtp_stream_buffer_lookahead(buffer);
    testrun(5 == lai.number_of_frames_ready);
    testrun(10 == lai.sequence_number);

    // slot 4: 14 <= Ready
    // slot 3: 13
    // slot 2: 12
    // slot 1: 11
    // slot 0: 10
    //
    // Sliding window is 5 frames.
    // 19 wont be inserted since it is outside of the sliding window
    frame = make_frame(1, 19, 80);
    testrun(!ov_rtp_stream_buffer_put(buffer, frame));
    frame = ov_rtp_frame_free(frame);
    ov_rtp_stream_buffer_print(buffer, stderr);
    lai = ov_rtp_stream_buffer_lookahead(buffer);
    testrun(5 == lai.number_of_frames_ready);
    testrun(10 == lai.sequence_number);

    // Test reset of sliding window after sufficient misses

    frame = make_frame(1, 19, 80);

    // We already have 2 failed insertions
    for (size_t i = 2; i <= buffer->max_sliding_window_misses; ++i) {
        testrun(!ov_rtp_stream_buffer_put(buffer, frame));
    }

    // Next insert should succeed and sliding window start reset to 19

    // slot 4: 0
    // slot 3: 0
    // slot 2: 0
    // slot 1: 0
    // slot 0: 19 <= Ready
    //
    // Sliding window is 5 frames.
    testrun(ov_rtp_stream_buffer_put(buffer, frame));

    buffer = ov_rtp_stream_buffer_free(buffer);
    testrun(0 == buffer);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static void clear_array(ov_rtp_frame **array, size_t capacity) {

    if (0 != array) {
        for (size_t i = 0; i < capacity; ++i) {
            array[i] = ov_rtp_frame_free(array[i]);
        }
    }
}

/*----------------------------------------------------------------------------*/

static bool frame_equals(ov_rtp_frame const *frame, uint16_t seq_expected) {

    return (0 != frame) && (seq_expected == frame->expanded.sequence_number);
}

/*----------------------------------------------------------------------------*/

static bool cannot_insert_frame(ov_rtp_stream_buffer *buffer, uint32_t ssrc,
                                uint32_t seq, uint32_t timestamp) {

    ov_rtp_frame *frame = make_frame(ssrc, seq, timestamp);

    bool ok = ov_rtp_stream_buffer_put(buffer, frame);

    frame = ov_rtp_frame_free(frame);

    return !ok;
}

/*----------------------------------------------------------------------------*/

static int test_ov_rtp_stream_buffer_get() {

    testrun(0 > ov_rtp_stream_buffer_get(0, 0, 0));

    ov_rtp_stream_buffer *buffer = ov_rtp_stream_buffer_create(5);
    testrun(0 != buffer);

    testrun(ov_rtp_stream_buffer_accept(buffer, 1, 11));

    testrun(0 > ov_rtp_stream_buffer_get(buffer, 0, 0));
    testrun(0 > ov_rtp_stream_buffer_get(0, 0, 0));
    testrun(0 > ov_rtp_stream_buffer_get(buffer, 0, 0));

    ov_rtp_frame *array[5] = {0};

    testrun(0 > ov_rtp_stream_buffer_get(0, array, 0));
    testrun(0 == ov_rtp_stream_buffer_get(buffer, array, 0));
    testrun(0 > ov_rtp_stream_buffer_get(0, 0, 2));
    testrun(0 > ov_rtp_stream_buffer_get(buffer, 0, 2));
    testrun(0 > ov_rtp_stream_buffer_get(0, 0, 2));
    testrun(0 > ov_rtp_stream_buffer_get(buffer, 0, 2));
    testrun(0 > ov_rtp_stream_buffer_get(0, array, 2));

    testrun(0 == ov_rtp_stream_buffer_get(buffer, array, 2));

    // Buffer:
    // Slot 4:   0
    // SLot 3:   4
    // SLot 2:   3
    // SLot 1:   2
    // SLot 0:   1
    testrun(ov_rtp_stream_buffer_put(buffer, make_frame(1, 1, 10)));
    testrun(ov_rtp_stream_buffer_put(buffer, make_frame(1, 2, 20)));
    testrun(ov_rtp_stream_buffer_put(buffer, make_frame(1, 3, 30)));
    testrun(ov_rtp_stream_buffer_put(buffer, make_frame(1, 4, 40)));

    testrun(4 == ov_rtp_stream_buffer_get(buffer, array, 5));

    testrun(0 == array[4]);
    testrun(frame_equals(array[3], 4));
    testrun(frame_equals(array[2], 3));
    testrun(frame_equals(array[1], 2));
    testrun(frame_equals(array[0], 1));

    clear_array(array, 5);

    // Sliding window now starts at 5...
    testrun(cannot_insert_frame(buffer, 1, 1, 10));
    testrun(cannot_insert_frame(buffer, 1, 2, 10));
    testrun(cannot_insert_frame(buffer, 1, 3, 10));
    testrun(cannot_insert_frame(buffer, 1, 4, 10));

    // Buffer:
    // Slot 4:   0
    // SLot 3:   8
    // SLot 2:   7
    // SLot 1:   6
    // SLot 0:   5
    testrun(ov_rtp_stream_buffer_put(buffer, make_frame(1, 5, 10)));
    testrun(ov_rtp_stream_buffer_put(buffer, make_frame(1, 8, 20)));
    testrun(ov_rtp_stream_buffer_put(buffer, make_frame(11, 7, 30)));
    testrun(ov_rtp_stream_buffer_put(buffer, make_frame(10, 6, 40)));

    testrun(2 == ov_rtp_stream_buffer_get(buffer, array, 2));

    testrun(0 == array[4]);
    testrun(0 == array[3]);
    testrun(0 == array[2]);
    testrun(frame_equals(array[1], 6));
    testrun(frame_equals(array[0], 5));

    clear_array(array, 5);

    testrun(2 == ov_rtp_stream_buffer_get(buffer, array, 2));

    testrun(0 == array[4]);
    testrun(0 == array[3]);
    testrun(0 == array[2]);
    testrun(frame_equals(array[1], 8));
    testrun(frame_equals(array[0], 7));

    clear_array(array, 5);

    buffer = ov_rtp_stream_buffer_free(buffer);
    testrun(0 == buffer);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_rtp_stream_buffer_print() {

    ov_rtp_stream_buffer_print(0, 0);

    ov_rtp_stream_buffer *buffer = ov_rtp_stream_buffer_create(5);
    testrun(ov_rtp_stream_buffer_accept(buffer, 1, 11));

    ov_rtp_stream_buffer_print(buffer, 0);
    ov_rtp_stream_buffer_print(0, stderr);
    ov_rtp_stream_buffer_print(buffer, stderr);

    // Sliding window is [19 - 23]
    testrun(ov_rtp_stream_buffer_put(buffer, make_frame(1, 19, 10000)));
    ov_rtp_stream_buffer_print(buffer, stderr);

    testrun(ov_rtp_stream_buffer_put(buffer, make_frame(1, 23, 10000)));
    ov_rtp_stream_buffer_print(buffer, stderr);

    testrun(cannot_insert_frame(buffer, 1, 35, 10000));
    ov_rtp_stream_buffer_print(buffer, stderr);
    testrun(cannot_insert_frame(buffer, 1, 25, 10000));
    ov_rtp_stream_buffer_print(buffer, stderr);
    testrun(cannot_insert_frame(buffer, 1, 24, 10000));
    ov_rtp_stream_buffer_print(buffer, stderr);

    testrun(ov_rtp_stream_buffer_put(buffer, make_frame(1, 20, 10000)));
    ov_rtp_stream_buffer_print(buffer, stderr);

    testrun(ov_rtp_stream_buffer_put(buffer, make_frame(1, 22, 10000)));
    ov_rtp_stream_buffer_print(buffer, stderr);

    testrun(ov_rtp_stream_buffer_put(buffer, make_frame(1, 21, 10000)));
    ov_rtp_stream_buffer_print(buffer, stderr);

    buffer = ov_rtp_stream_buffer_free(buffer);
    testrun(0 == buffer);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

OV_TEST_RUN("ov_rtp_stream_buffer", test_ov_rtp_stream_buffer_create,
            test_ov_rtp_stream_buffer_free, test_ov_rtp_stream_buffer_accept,
            test_ov_rtp_stream_buffer_put, test_ov_rtp_stream_buffer_get,
            test_ov_rtp_stream_buffer_print);
