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

        An `ov_rtp_buffering_io` manages *one* socket to receive / send
        RTP.

        The IO is done synchronously, i.e. output is just written to the socket
        immediately.

        Input is automatically parsed to `ov_rtp_frame` s, and sorted
        according to SSRC ID and sequence number.

        However, input is not forwarded further automatically, but must be
        requested using `ov_rtp_buffering_io_next_frame_list` , which will return
        a list of frames, namely for each SSRC ID frames were received for,
        the oldest frame will be in the list.

        It is meant to be used in conjunction with e.g. a timer that
        triggers `ov_rtp_buffering_io_next_frame_list` being called and the
        RTP frames therein being processed.

        ------------------------------------------------------------------------
*/
#ifndef OV_MIXER_IO_H
#define OV_MIXER_IO_H

#include "ov_event_loop.h"
#include "ov_rtp_frame_buffer.h"

#include "ov_rtp_frame.h"

/*----------------------------------------------------------------------------*/

struct ov_rtp_buffering_io;

typedef struct ov_rtp_buffering_io ov_rtp_buffering_io;

/*----------------------------------------------------------------------------*/

typedef struct {

  ov_event_loop *loop;

  size_t num_frames_to_buffer_per_stream;

} ov_rtp_buffering_io_config;

/*----------------------------------------------------------------------------*/

/**
 * Creates a new rtp_io instance.
 * @param config event loop is required
 */
ov_rtp_buffering_io *
ov_rtp_buffering_io_create(ov_rtp_buffering_io_config config);

/**
 * Frees an rtp_io instance.
 * Underlying resources passed in on creation (like the event loop) are
 * *not* released/freed!
 */
ov_rtp_buffering_io *
ov_rtp_buffering_io_free(ov_rtp_buffering_io *rtp_buffering_io);

/**
 * register socket with an ov_rtp_buffering_io instance.
 */
bool ov_rtp_buffering_io_register_socket(ov_rtp_buffering_io *io, int fd);

/**
 * Unregister the socket.
 * @return true if at the end of the call, no socket is registered - no matter
 * whether it was before
 */
bool ov_rtp_buffering_io_unregister_socket(ov_rtp_buffering_io *io);

ov_list *ov_rtp_buffering_io_next_frame_list(ov_rtp_buffering_io *io);

/*----------------------------------------------------------------------------*/
#endif
