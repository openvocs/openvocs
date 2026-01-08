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
        @file           ov_mc_interconnect_loop.c
        @author         Markus Töpfer

        @date           2023-12-11


        ------------------------------------------------------------------------
*/
#include "../include/ov_mc_interconnect_loop.h"

#include <ov_base/ov_mc_socket.h>
#include <ov_base/ov_random.h>
#include <ov_base/ov_rtp_frame.h>
#include <ov_base/ov_socket.h>

#define OV_MC_INTERCONNECT_LOOP_MAGIC_BYTES 0x1006

/*----------------------------------------------------------------------------*/

struct ov_mc_interconnect_loop {

  uint16_t magic_bytes;
  ov_mc_interconnect_loop_config config;

  uint32_t ssrc;
  int socket;
  int sender;

  ov_socket_data local;
};

/*----------------------------------------------------------------------------*/

static bool io_multicast(int socket, uint8_t events, void *userdata) {

  uint8_t buffer[OV_UDP_PAYLOAD_OCTETS] = {0};
  ov_socket_data remote = {};
  socklen_t src_addr_len = sizeof(remote.sa);

  ov_rtp_frame *frame = NULL;

  ov_mc_interconnect_loop *self = ov_mc_interconnect_loop_cast(userdata);
  if (!self || !socket)
    goto error;

  if ((events & OV_EVENT_IO_CLOSE) || (events & OV_EVENT_IO_ERR)) {

    ov_log_debug("%i - closing", socket);
    return true;
  }

  ssize_t bytes = recvfrom(socket, (char *)buffer, OV_UDP_PAYLOAD_OCTETS, 0,
                           (struct sockaddr *)&remote.sa, &src_addr_len);

  if (bytes < 1)
    goto error;

  if (!ov_socket_parse_sockaddr_storage(&remote.sa, remote.host,
                                        OV_HOST_NAME_MAX, &remote.port))
    goto error;

  frame = ov_rtp_frame_decode(buffer, bytes);
  if (!frame)
    goto error;

  if (frame->expanded.ssrc == self->ssrc)
    goto error;
  /*
      ov_log_debug("--> recv %zi bytes at %s %"PRIu32,
          bytes, self->config.name, frame->expanded.ssrc);
  */

  bool result = ov_mc_interconnect_multicast_io(self->config.base, self, buffer,
                                         bytes);

  frame = ov_rtp_frame_free(frame);
  return result;
error:
  frame = ov_rtp_frame_free(frame);
  return false;
}

/*----------------------------------------------------------------------------*/

ov_mc_interconnect_loop *
ov_mc_interconnect_loop_create(ov_mc_interconnect_loop_config config) {

  ov_mc_interconnect_loop *self = NULL;

  if (!config.loop)
    goto error;
  if (0 == config.name[0])
    goto error;
  if (0 == config.socket.host[0])
    goto error;
  if (0 == config.socket.port)
    goto error;

  self = calloc(1, sizeof(ov_mc_interconnect_loop));
  if (!self)
    goto error;

  self->magic_bytes = OV_MC_INTERCONNECT_LOOP_MAGIC_BYTES;
  self->config = config;
  self->ssrc = ov_random_uint32();

  self->socket = ov_mc_socket(config.socket);
  if (!ov_socket_get_data(self->socket, &self->local, NULL))
    goto error;
  if (-1 == self->socket)
    goto error;

  ov_socket_configuration socket_config =
      (ov_socket_configuration){.type = UDP, .host = "0.0.0.0"};

  self->sender = ov_socket_open_server(&socket_config);

  ov_log_debug("opened LOOP %s | %s:%i", self->config.name, self->local.host,
               self->local.port);

  if (!ov_event_loop_set(self->config.loop, self->socket,
                         OV_EVENT_IO_IN | OV_EVENT_IO_ERR | OV_EVENT_IO_CLOSE,
                         self, io_multicast))
    goto error;

  return self;
error:
  ov_mc_interconnect_loop_free(self);
  return NULL;
}

/*----------------------------------------------------------------------------*/

ov_mc_interconnect_loop *
ov_mc_interconnect_loop_free(ov_mc_interconnect_loop *self) {

  if (!ov_mc_interconnect_loop_cast(self))
    goto error;

  if (-1 != self->socket) {
    ov_mc_socket_drop_membership(self->socket);
    ov_event_loop_unset(self->config.loop, self->socket, NULL);
    close(self->socket);
    self->socket = -1;
  }

  if (-1 != self->sender) {
    ov_event_loop_unset(self->config.loop, self->sender, NULL);
    close(self->sender);
    self->sender = -1;
  }

  self = ov_data_pointer_free(self);
error:
  return self;
}

/*----------------------------------------------------------------------------*/

void *ov_mc_interconnect_loop_free_void(void *self) {

  return ov_mc_interconnect_loop_cast(self);
}

/*----------------------------------------------------------------------------*/

ov_mc_interconnect_loop *ov_mc_interconnect_loop_cast(const void *data) {

  if (!data)
    return NULL;

  if (*(uint16_t *)data != OV_MC_INTERCONNECT_LOOP_MAGIC_BYTES)
    return NULL;

  return (ov_mc_interconnect_loop *)data;
}

/*----------------------------------------------------------------------------*/

uint32_t ov_mc_interconnect_loop_get_ssrc(const ov_mc_interconnect_loop *self) {

  if (!self)
    return 0;
  return self->ssrc;
}

/*----------------------------------------------------------------------------*/

const char *
ov_mc_interconnect_loop_get_name(const ov_mc_interconnect_loop *self) {

  if (!self)
    return NULL;
  return self->config.name;
}

/*----------------------------------------------------------------------------*/

bool ov_mc_interconnect_loop_send(ov_mc_interconnect_loop *self,
                                  uint8_t *buffer, size_t bytes) {

  if (!self || !buffer || !bytes)
    goto error;

  struct sockaddr_storage dest = {0};

  if (!ov_socket_fill_sockaddr_storage(&dest, self->local.sa.ss_family,
                                       self->config.socket.host,
                                       self->config.socket.port)) {
    goto error;
  }

  socklen_t len = sizeof(struct sockaddr_in);
  if (dest.ss_family == AF_INET6)
    len = sizeof(struct sockaddr_in6);

  ssize_t out =
      sendto(self->sender, buffer, bytes, 0, (struct sockaddr *)&dest, len);

  UNUSED(out);
  return true;
error:
  return false;
}
