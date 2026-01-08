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

/*----------------------------------------------------------------------------*/

#include "../../include/ov_utils.h"

#include "../../include/ov_constants.h"
#include "../../include/ov_linked_list.h"

#include "../../include/ov_rtp_buffering_io.h"

/*----------------------------------------------------------------------------*/

struct ov_rtp_buffering_io {

  uint32_t magic_bytes;

  ov_event_loop *loop;

  ov_rtp_frame_buffer *frame_buffer;

  int fd;
};

/*----------------------------------------------------------------------------*/

#define MAGIC_BYTES 0xbbeeee01

/*----------------------------------------------------------------------------*/

static ov_rtp_buffering_io *as_rtp_io(void *vptr) {

  if (0 == vptr)
    return 0;

  ov_rtp_buffering_io *rtp_io = vptr;

  if (MAGIC_BYTES != rtp_io->magic_bytes)
    return 0;

  return rtp_io;
}

/*----------------------------------------------------------------------------*/

ov_rtp_buffering_io *
ov_rtp_buffering_io_create(ov_rtp_buffering_io_config config) {

  ov_rtp_buffering_io *io = 0;

  if (0 == config.loop) {

    ov_log_error("No event loop (0 pointer)");
    goto error;
  }

  io = calloc(1, sizeof(ov_rtp_buffering_io));

  io->magic_bytes = MAGIC_BYTES;

  OV_ASSERT(0 != io);

  size_t num_frames_per_stream = config.num_frames_to_buffer_per_stream;
  if (0 == num_frames_per_stream) {

    num_frames_per_stream = 2;
  }

  ov_rtp_frame_buffer_config fb_cfg = {
      .num_frames_to_buffer_per_stream = num_frames_per_stream,

  };

  io->frame_buffer = ov_rtp_frame_buffer_create(fb_cfg);

  if (0 == io->frame_buffer) {

    ov_log_error("Could not create frame buffer");
    goto error;
  }

  io->loop = config.loop;

  return io;

error:

  if (0 != io) {

    free(io);
    io = 0;
  }

  return 0;
}

/*----------------------------------------------------------------------------*/

ov_rtp_buffering_io *ov_rtp_buffering_io_free(ov_rtp_buffering_io *io) {

  if (0 == io) {
    return io;
  }

  if ((0 < io->fd) && (!ov_rtp_buffering_io_unregister_socket(io))) {

    ov_log_error("Socket was not unregistered properly!");
    goto error;
  }

  if (0 != io->frame_buffer) {

    io->frame_buffer = io->frame_buffer->free(io->frame_buffer);
  }

  OV_ASSERT(0 == io->frame_buffer);

  free(io);

  io = 0;

error:

  return io;
}

/*----------------------------------------------------------------------------*/

static bool cb_io_udp_media(int fd, uint8_t events, void *userdata) {

  UNUSED(events);

  uint8_t buffer[OV_UDP_PAYLOAD_OCTETS];

  ov_rtp_buffering_io *io = as_rtp_io(userdata);

  if (0 == io) {

    ov_log_error("Invalid userdata - expected rtp_buffering_io");
    goto error;
  }

  ov_log_info("Received UDP data");

  ssize_t in = read(fd, buffer, sizeof(buffer));

  // read again
  if (in < 0) {
    return true;
  }

  ov_rtp_frame *decoded = ov_rtp_frame_decode(buffer, (size_t)in);

  if (0 == decoded) {

    ov_log_error("could not decode RTP frame");
    goto error;
  }

  decoded = ov_rtp_frame_buffer_add(io->frame_buffer, decoded);

  if (0 != decoded) {

    decoded = decoded->free(decoded);
    ov_log_error("Could not forward RTP frame");
  }

  OV_ASSERT(0 == decoded);

  return true;

error:

  return false;
}

/*----------------------------------------------------------------------------*/
bool ov_rtp_buffering_io_register_socket(ov_rtp_buffering_io *io, int fd) {

  if (0 >= fd) {

    ov_log_error("Invalid file descriptor");
    goto error;
  }

  if ((0 == io) || (0 == io->loop)) {

    ov_log_error("No or incomplete rtp_buffering_io given");
    goto error;
  }

  if (0 < io->fd) {

    ov_log_error("Already registered a socket");
    goto error;
  }

  OV_ASSERT(0 != io);
  OV_ASSERT(0 != io->loop);

  if (!io->loop->callback.set(
          io->loop, fd, OV_EVENT_IO_IN | OV_EVENT_IO_ERR | OV_EVENT_IO_CLOSE,
          io, cb_io_udp_media)) {

    ov_log_error("Could not regsiter socket with loop");
    goto error;
  }

  io->fd = fd;

  return true;

error:

  return false;
}

/*----------------------------------------------------------------------------*/

bool ov_rtp_buffering_io_unregister_socket(ov_rtp_buffering_io *io) {

  if (0 == io) {

    ov_log_error("Require mixer rtp io, got 0 pointer");
    goto error;
  }

  if (0 > io->fd) {

    return true;
  }

  if (0 == io->loop) {

    ov_log_error("No or incomplete rtp_buffering_io given");
    goto error;
  }

  OV_ASSERT(0 != io);
  OV_ASSERT(0 != io->loop);

  if (!io->loop->callback.unset(io->loop, io->fd, 0)) {

    ov_log_error("Could not unregsiter socket with loop");
    goto error;
  }

  io->fd = -1;

  return true;

error:

  return false;
}

/*----------------------------------------------------------------------------*/

ov_list *ov_rtp_buffering_io_next_frame_list(ov_rtp_buffering_io *io) {

  if (0 == io) {

    goto error;
  }

  ov_list *list = ov_rtp_frame_buffer_get_current_frames(io->frame_buffer);

  if (0 == list) {

    ov_log_info("No frames ready");

    list = ov_linked_list_create((ov_list_config){0});
  }

  return list;

error:

  return 0;
}

/*----------------------------------------------------------------------------*/
