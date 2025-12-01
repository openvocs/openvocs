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

        @author Michael J. Beer, DLR/GSOC
        @copyright (c) 2023 German Aerospace Center DLR e.V. (GSOC)

        BEWARE: Due to the implemenation of a sliding window,
        there might be a glitch once every 2^16-1 frames (when
        the sequence number overflows ...)

        ------------------------------------------------------------------------
*/
#ifndef OV_STREAM_BUFFER_H
#define OV_STREAM_BUFFER_H
/*----------------------------------------------------------------------------*/

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>

#include "ov_rtp_frame.h"

/*----------------------------------------------------------------------------*/

struct ov_rtp_stream_buffer;
typedef struct ov_rtp_stream_buffer ov_rtp_stream_buffer;

/*----------------------------------------------------------------------------*/

ov_rtp_stream_buffer *ov_rtp_stream_buffer_create(size_t max_num_frames);

ov_rtp_stream_buffer *ov_rtp_stream_buffer_free(ov_rtp_stream_buffer *self);

/*----------------------------------------------------------------------------*/

bool ov_rtp_stream_buffer_accept(ov_rtp_stream_buffer *self,
                                 uint32_t bottom_ssid, uint32_t top_ssid);

/*----------------------------------------------------------------------------*/

/**
 * Set specific SSRC to block.
 * Only one SSRC can be blocked at the same time - if you call
 *
 * ov_rtp_stream_buffer_set_blocked_ssid(rtpbuf, 12); // No more frames with
 * SSRC 12 will be accepted ov_rtp_stream_buffer_set_blocked_ssid(rtpbuf, 13);
 * // SSRC 12 will be accepted again. SSRC 13 will not be accepted any more
 *
 * Does not affect the SSRC range to accept.
 */
bool ov_rtp_stream_buffer_set_blocked_ssid(ov_rtp_stream_buffer *self,
                                           uint32_t blocked_ssid);

/*----------------------------------------------------------------------------*/

bool ov_rtp_stream_buffer_dont_accept_any(ov_rtp_stream_buffer *self);

/*----------------------------------------------------------------------------*/

bool ov_rtp_stream_buffer_put(ov_rtp_stream_buffer *self, ov_rtp_frame *frame);

/*----------------------------------------------------------------------------*/

typedef struct {
  size_t number_of_frames_ready;
  uint16_t sequence_number;
} ov_rtp_stream_buffer_lookahead_info;

ov_rtp_stream_buffer_lookahead_info
ov_rtp_stream_buffer_lookahead(ov_rtp_stream_buffer const *self);

/**
 * Returns number of frames actually put into array
 */
ssize_t ov_rtp_stream_buffer_get(ov_rtp_stream_buffer *self,
                                 ov_rtp_frame **array, size_t frames_to_return);

/*----------------------------------------------------------------------------*/

void ov_rtp_stream_buffer_print(ov_rtp_stream_buffer const *self, FILE *out);

/*----------------------------------------------------------------------------*/
#endif
