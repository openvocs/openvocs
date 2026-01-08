/***
        ------------------------------------------------------------------------

        Copyright (c) 2023 German Aerospace Center DLR e.V. (GSOC)

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

        @author         Michael J. Beer, DLR/GSOC

        ------------------------------------------------------------------------
*/

#include "ov_rtp_stream_buffer.h"

/*----------------------------------------------------------------------------*/

#ifdef __STDC_NO_ATOMICS__
#error("Require atomics to be present")
#endif

#include "../../include/ov_utils.h"
#include <stdatomic.h>
#include <stdlib.h>

/*----------------------------------------------------------------------------*/

struct ov_rtp_stream_buffer {

  atomic_flag in_use;
  uint32_t lowest_acceptable_ssid;
  uint32_t top_acceptable_ssid;

  uint32_t blocked_ssid;

  // This defines the size of the buffer as well as our sliding window
  size_t max_num_frames;
  ov_rtp_frame **frames;

  bool stream_started;
  uint16_t sliding_window_start;

  size_t sliding_window_misses;
  size_t max_sliding_window_misses;
};

/*----------------------------------------------------------------------------*/

ov_rtp_stream_buffer *ov_rtp_stream_buffer_create(size_t max_num_frames) {

  if (ov_cond_valid(0 < max_num_frames,
                    "Buffer should be able to hold at least one frame")) {

    ov_rtp_stream_buffer *self = calloc(1, sizeof(ov_rtp_stream_buffer));

    self->max_num_frames = max_num_frames;
    self->frames = calloc(max_num_frames, sizeof(ov_rtp_frame *));
    self->sliding_window_start = 0;
    self->stream_started = false;
    self->max_sliding_window_misses = 5;

    atomic_flag_clear(&self->in_use);

    return self;

  } else {

    return 0;
  }
}

/*----------------------------------------------------------------------------*/

static bool lock(ov_rtp_stream_buffer const *self) {

  ov_rtp_stream_buffer *self_mut = (ov_rtp_stream_buffer *)self;

  if (0 != self_mut) {

    // we just assume that the buffer is actually not used THAT long
    while (atomic_flag_test_and_set(&self_mut->in_use)) {
    };

    return true;

  } else {
    return false;
  }
}

/*----------------------------------------------------------------------------*/

static void unlock(ov_rtp_stream_buffer const *self) {

  ov_rtp_stream_buffer *self_mut = (ov_rtp_stream_buffer *)self;

  if (0 != self) {
    atomic_flag_clear(&self_mut->in_use);
  }
}

/*----------------------------------------------------------------------------*/

static bool clear_frames(ov_rtp_frame **frames_array, size_t array_capacity) {

  if (0 != frames_array) {

    for (size_t i = 0; i < array_capacity; ++i) {
      frames_array[i] = ov_rtp_frame_free(frames_array[i]);
    }

    return true;
  } else {
    return false;
  }
}

/*----------------------------------------------------------------------------*/

ov_rtp_stream_buffer *ov_rtp_stream_buffer_free(ov_rtp_stream_buffer *self) {

  if (lock(self) && clear_frames(self->frames, self->max_num_frames)) {

    self->frames = ov_free(self->frames);
    return ov_free(self);

  } else {

    return self;
  }
}

/*----------------------------------------------------------------------------*/

static void reset_stream(ov_rtp_stream_buffer *self,
                         uint16_t sliding_window_start) {

  if (0 != self) {

    self->sliding_window_start = sliding_window_start;
    self->sliding_window_misses = 0;

    self->stream_started = false;

    clear_frames(self->frames, self->max_num_frames);

    ov_log_info("Stream reset - new sliding window start at %" PRIu16,
                sliding_window_start);
  }
}

/*----------------------------------------------------------------------------*/

bool ov_rtp_stream_buffer_accept(ov_rtp_stream_buffer *self,
                                 uint32_t bottom_ssid, uint32_t top_ssid) {

  if (bottom_ssid > top_ssid) {
    uint32_t temp = top_ssid;
    top_ssid = bottom_ssid;
    bottom_ssid = temp;
  }

  if (ov_cond_valid(0 < bottom_ssid, "Cannot use SSID 0") && lock(self)) {

    self->lowest_acceptable_ssid = bottom_ssid;
    self->top_acceptable_ssid = top_ssid;

    reset_stream(self, 0);

    unlock(self);

    return true;

  } else {
    return false;
  }
}

/*----------------------------------------------------------------------------*/

bool ov_rtp_stream_buffer_set_blocked_ssid(ov_rtp_stream_buffer *self,
                                           uint32_t blocked_ssid) {

  if (lock(self)) {

    self->blocked_ssid = blocked_ssid;
    unlock(self);

    return true;

  } else {

    return false;
  }
}

/*----------------------------------------------------------------------------*/

bool ov_rtp_stream_buffer_dont_accept_any(ov_rtp_stream_buffer *self) {

  if (lock(self)) {

    self->lowest_acceptable_ssid = 0;
    self->top_acceptable_ssid = 0;

    reset_stream(self, 0);

    unlock(self);
    return true;

  } else {
    return false;
  }
}

/*----------------------------------------------------------------------------*/

static size_t index_for(uint16_t sequence_number,
                        uint16_t sliding_window_start) {

  return sequence_number - sliding_window_start;
}

/*----------------------------------------------------------------------------*/

typedef enum { IR_OK, IR_DOUBLETTE, IR_OUT_OF_RANGE } insertion_result;

static insertion_result insert_into_frames_array(ov_rtp_frame *frame,
                                                 uint16_t sliding_window_start,
                                                 ov_rtp_frame **frames,
                                                 size_t capacity) {

  size_t index =
      index_for(frame->expanded.sequence_number, sliding_window_start);

  if (!ov_cond_valid(index < capacity,
                     "Frame outside sliding window - dropping")) {

    ov_log_debug("Frame outside sliding window: %" PRIu32 " Seq %" PRIu16
                 "  sliding window %" PRIu16 " to %" PRIu16,
                 frame->expanded.ssrc, frame->expanded.sequence_number,
                 sliding_window_start, sliding_window_start + capacity);

    return IR_OUT_OF_RANGE;

  } else if (!ov_cond_valid(0 == frames[index],
                            "Cannot insert frame: Doublette")) {

    return IR_DOUBLETTE;

  } else {

    frames[index] = frame;
    return IR_OK;
  }
}

/*----------------------------------------------------------------------------*/

static bool is_frame_accepted(ov_rtp_stream_buffer const *self, uint32_t ssrc) {

  OV_ASSERT(0 != self);

  ov_log_debug("Got another frame SSRC: % " PRIu32 " (blocked SSRC: %" PRIu32
               ", accpt range: %" PRIu32 " - %" PRIu32 ")",
               ssrc, self->blocked_ssid, self->lowest_acceptable_ssid,
               self->top_acceptable_ssid);

  return (ssrc != self->blocked_ssid) &&
         (self->lowest_acceptable_ssid <= ssrc) &&
         (ssrc <= self->top_acceptable_ssid);
}

/*----------------------------------------------------------------------------*/

static void treat_out_of_range_frame(ov_rtp_stream_buffer *self,
                                     uint16_t sliding_window_start) {

  if ((0 != self) &&
      (++self->sliding_window_misses > self->max_sliding_window_misses)) {

    reset_stream(self, sliding_window_start);
  }
}

/*----------------------------------------------------------------------------*/

bool ov_rtp_stream_buffer_put(ov_rtp_stream_buffer *self, ov_rtp_frame *frame) {

  if (ov_ptr_valid(frame, "Cannot put RTP frame in buffer - invalid frame") &&
      lock(self)) {

    bool ok = false;

    uint32_t ssrc = frame->expanded.ssrc;

    if (is_frame_accepted(self, ssrc)) {

      size_t capacity = self->max_num_frames;
      ov_rtp_frame **frames = self->frames;

      if (!self->stream_started) {

        OV_ASSERT(0 == frames[0]);

        ov_log_debug("Starting new stream SSRC: %" PRIu32 " - Seq: %" PRIu16,
                     frame->expanded.ssrc, frame->expanded.sequence_number);

        self->stream_started = true;
        self->sliding_window_start = frame->expanded.sequence_number;
        frames[0] = frame;
        ok = true;

      } else {

        switch (insert_into_frames_array(frame, self->sliding_window_start,
                                         frames, capacity)) {

        case IR_OK:
          ok = true;
          self->sliding_window_misses = 0;
          ov_log_debug("Accepted frame SSRC %" PRIu32 " - Seq %" PRIu16,
                       frame->expanded.ssrc, frame->expanded.sequence_number);
          break;

        case IR_DOUBLETTE:
          ov_log_debug("Got doublette SSRC %" PRIu32 " - Seq %" PRIu16,
                       frame->expanded.ssrc, frame->expanded.sequence_number);
          ok = false;
          break;

        case IR_OUT_OF_RANGE:
          treat_out_of_range_frame(self, frame->expanded.sequence_number);
          ov_log_debug("Out of sliding window: SSRC %" PRIu32 " - Seq %" PRIu16,
                       frame->expanded.ssrc, frame->expanded.sequence_number);
          break;
        };
      }

    } else {

      ov_log_error("Received wrong SSRC - expected between :%" PRIu32
                   " and %" PRIu32 " - blocked %" PRIu32 ", got %" PRIu32 "\n",
                   self->lowest_acceptable_ssid, self->top_acceptable_ssid,
                   self->blocked_ssid, frame->expanded.ssrc);
    }

    unlock(self);

    return ok;

  } else {
    return false;
  }
}

/*----------------------------------------------------------------------------*/

static ssize_t high_index_of_current_chunk(ov_rtp_frame const *const *frames,
                                           size_t capacity) {

  if ((0 != frames) && (0 != frames[0])) {

    uint16_t seq = frames[0]->expanded.sequence_number;

    size_t index = 0;
    for (size_t i = 1; (i < capacity) && (0 != frames[i]); ++i) {

      if (seq + 1 == frames[i]->expanded.sequence_number) {
        ++seq;
        ++index;
      } else {
        break;
      }
    }

    return index;

  } else {
    return -1;
  }
}

/*----------------------------------------------------------------------------*/

ov_rtp_stream_buffer_lookahead_info
ov_rtp_stream_buffer_lookahead(ov_rtp_stream_buffer const *self) {

  ov_rtp_stream_buffer_lookahead_info lai = {0};

  if (lock(self)) {

    if (0 != self->frames[0]) {

      lai.sequence_number = self->frames[0]->expanded.sequence_number;
      lai.number_of_frames_ready =
          1 +
          high_index_of_current_chunk((ov_rtp_frame const *const *)self->frames,
                                      self->max_num_frames);
    }

    unlock(self);
  }

  return lai;
}

/*----------------------------------------------------------------------------*/

static void shift_frames_to_start(ov_rtp_frame **array,
                                  size_t offset_from_start, size_t capacity) {

  if ((0 < offset_from_start) && (0 != array)) {

    for (size_t i = 0; i < capacity; ++i) {
      if (i + offset_from_start < capacity) {
        array[i] = array[i + offset_from_start];
      } else {
        array[i] = 0;
      }
    }
  }
}

/*----------------------------------------------------------------------------*/

ssize_t ov_rtp_stream_buffer_get(ov_rtp_stream_buffer *self,
                                 ov_rtp_frame **array,
                                 size_t frames_to_return) {

  ssize_t result = -1;

  if (lock(self)) {

    if (ov_ptr_valid(array, "Cannot get frames - no return array")) {

      size_t number_of_returned_frames = 0;

      for (size_t i = 0; (i < frames_to_return) && (i < self->max_num_frames) &&
                         (0 != self->frames[i]);
           ++number_of_returned_frames, ++i) {

        array[i] = self->frames[i];
        self->frames[i] = 0;
      }

      shift_frames_to_start(self->frames, number_of_returned_frames,
                            self->max_num_frames);

      /* Might and is totally fine to overflow */
      self->sliding_window_start += number_of_returned_frames;

      result = number_of_returned_frames;
    }

    unlock(self);
  }

  return result;
}

/*----------------------------------------------------------------------------*/

void ov_rtp_stream_buffer_print(ov_rtp_stream_buffer const *self, FILE *out) {

  if (ov_ptr_valid(self, "RTP stream buffer is invalid") &&
      ov_ptr_valid(out, "Cannot print RTP stream buffer - no output stream") &&
      ov_cond_valid(self->max_num_frames > 0,
                    "RTP stream buffer invalid: has capacity of 0")) {

    fprintf(stderr, "RTP Stream buffer contains:\n\n");

    for (ssize_t i = self->max_num_frames - 1; i >= 0; --i) {

      if (0 != self->frames[i]) {

        fprintf(out, "Slot %zu:   SSID: %" PRIu32 "   SEQ %" PRIu16 "\n", i,
                self->frames[i]->expanded.ssrc,
                self->frames[i]->expanded.sequence_number);
      }
    }
  }
}

/*----------------------------------------------------------------------------*/
