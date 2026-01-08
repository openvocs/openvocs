/***
        ------------------------------------------------------------------------

        Copyright (c) 2022 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_mc_loop.c
        @author         Markus

        @date           2022-11-10


        ------------------------------------------------------------------------
*/
#include "../include/ov_mc_loop.h"
#include <ov_base/ov_mc_socket.h>

#define OV_MC_LOOP_MAGIC_BYTES 0x5005

struct ov_mc_loop {

  uint16_t magic_byte;

  ov_mc_loop_config config;

  int rtp_fhd;
  // int rtcp_fhd;
};

/*----------------------------------------------------------------------------*/

static bool io_multicast(int sfd, uint8_t events, void *userdata) {

  uint8_t buffer[OV_UDP_PAYLOAD_OCTETS] = {0};

  ov_mc_loop *loop = ov_mc_loop_cast(userdata);

  if ((events & OV_EVENT_IO_CLOSE) || (events & OV_EVENT_IO_ERR)) {
    ov_log_error("Socket close at %i loop %s|%s:%i", sfd,
                 loop->config.data.name, loop->config.data.socket.host,
                 loop->config.data.socket.port);
    goto error;
  }

  ov_socket_data in = {0};
  socklen_t in_len = 0;

  ssize_t bytes = recvfrom(sfd, buffer, OV_UDP_PAYLOAD_OCTETS, 0,
                           (struct sockaddr *)&in.sa, &in_len);

  if (-1 == bytes)
    goto done;

  in = ov_socket_data_from_sockaddr_storage(&in.sa);

  if (loop->config.callback.io)
    loop->config.callback.io(loop->config.callback.userdata, &loop->config.data,
                             buffer, bytes, &in);

done:
  return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

static int open_mc_udp_socket(char const *interface, int port) {

  ov_socket_configuration s_config =
      (ov_socket_configuration){.type = UDP, .port = port};

  strncpy(s_config.host, interface, OV_HOST_NAME_MAX);

  int s = ov_mc_socket(s_config);

  if (0 > s) {

    ov_log_error("Failed to open Multicast socket %s:%i", s_config.host,
                 s_config.port);
  }

  return s;
}

/*----------------------------------------------------------------------------*/

ov_mc_loop *ov_mc_loop_create(ov_mc_loop_config config) {

  ov_mc_loop *loop = NULL;

  if (!config.loop)
    goto error;
  if (0 == config.data.socket.host[0])
    goto error;
  if (0 == config.data.name[0])
    goto error;

  loop = calloc(1, sizeof(ov_mc_loop));
  if (!loop)
    goto error;

  loop->magic_byte = OV_MC_LOOP_MAGIC_BYTES;
  loop->config = config;
  if (0 == loop->config.data.volume)
    loop->config.data.volume = 50;

  loop->rtp_fhd =
      open_mc_udp_socket(config.data.socket.host, config.data.socket.port);
  /*
  loop->rtcp_fhd =
      open_mc_udp_socket(config.data.socket.host, config.data.socket.port +
  1);
  */

  ov_event_loop *l = config.loop;

  if (ov_event_loop_set(l,
                          loop->rtp_fhd,
                          OV_EVENT_IO_IN | OV_EVENT_IO_ERR | OV_EVENT_IO_CLOSE,
                          loop,
                          io_multicast) /* &&
        ov_event_loop_set(l,
                          loop->rtcp_fhd,
                          OV_EVENT_IO_IN | OV_EVENT_IO_ERR | OV_EVENT_IO_CLOSE,
                          loop,
                          io_multicast) */) {

    return loop;
  }

error:
  ov_mc_loop_free(loop);
  return NULL;
}

/*----------------------------------------------------------------------------*/

static void close_mc_sfh(ov_event_loop *loop, int sfh) {

  if (-1 != sfh) {
    ov_event_loop_unset(loop, sfh, 0);
    ov_mc_socket_drop_membership(sfh);
    close(sfh);
  }
}

/*----------------------------------------------------------------------------*/

ov_mc_loop *ov_mc_loop_free(ov_mc_loop *self) {

  if (!ov_mc_loop_cast(self))
    return self;

  close_mc_sfh(self->config.loop, self->rtp_fhd);
  // close_mc_sfh(self->config.loop, self->rtcp_fhd);

  self = ov_data_pointer_free(self);
  return NULL;
}

/*----------------------------------------------------------------------------*/

ov_mc_loop *ov_mc_loop_cast(const void *data) {

  if (!data)
    return NULL;

  if (*(uint16_t *)data != OV_MC_LOOP_MAGIC_BYTES)
    return NULL;

  return (ov_mc_loop *)data;
}

/*----------------------------------------------------------------------------*/

void *ov_mc_loop_free_void(void *self) {

  ov_mc_loop *loop = ov_mc_loop_cast(self);
  return ov_mc_loop_free(loop);
}

/*----------------------------------------------------------------------------*/

bool ov_mc_loop_set_volume(ov_mc_loop *self, uint8_t volume) {

  if (!self || volume > 100)
    goto error;

  self->config.data.volume = volume;
  return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

uint8_t ov_mc_loop_get_volume(ov_mc_loop *self) {

  if (!self)
    return 0;
  return self->config.data.volume;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_mc_loop_to_json(ov_mc_loop *self) {

  ov_json_value *out = NULL;
  ov_json_value *val = NULL;

  if (!self)
    goto error;

  out = ov_json_object();
  val = ov_mc_loop_data_to_json(self->config.data);
  if (!ov_json_object_set(out, OV_KEY_DATA, val))
    goto error;

  val = ov_json_number(self->rtp_fhd);
  if (!ov_json_object_set(out, OV_KEY_SOCKET, val))
    goto error;

  return out;
error:
  ov_json_value_free(val);
  ov_json_value_free(out);
  return NULL;
}
