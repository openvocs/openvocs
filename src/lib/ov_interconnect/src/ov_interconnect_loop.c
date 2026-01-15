/***
        ------------------------------------------------------------------------

        Copyright (c) 2026 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_interconnect_loop.c
        @author         Töpfer, Markus

        @date           2026-01-15


        ------------------------------------------------------------------------
*/
#include "../include/ov_interconnect_loop.h"

#include <ov_base/ov_random.h>
#include <ov_base/ov_rtp_frame.h>
#include <ov_base/ov_mc_socket.h>
#include <ov_base/ov_convert.h>

/*----------------------------------------------------------------------------*/

struct ov_interconnect_loop{

    ov_interconnect_loop_config config;

    uint32_t ssrc;

    ov_socket_data local;
    int socket;

    ov_mixer_data mixer;

};

/*----------------------------------------------------------------------------*/

static bool io_from_mixer(int socket, uint8_t events, void *userdata) {

    uint8_t buffer[OV_UDP_PAYLOAD_OCTETS] = {0};
    ov_socket_data remote = {};
    socklen_t src_addr_len = sizeof(remote.sa);

    ov_interconnect_loop *self = (ov_interconnect_loop*)userdata;
    if (!self || !socket) goto error;

    if ((events & OV_EVENT_IO_CLOSE) || (events & OV_EVENT_IO_ERR)) {

        ov_log_debug("%i - closing", socket);
        return true;
    }

    if (!ov_socket_parse_sockaddr_storage(
        &remote.sa, remote.host, OV_HOST_NAME_MAX, &remote.port))
        goto error;

    ssize_t bytes = recvfrom(socket, (char *)buffer, OV_UDP_PAYLOAD_OCTETS, 0,
                             (struct sockaddr *)&remote.sa, &src_addr_len);

    if (bytes < 1)
        goto error;

    bool result =
        ov_interconnect_loop_io(self->config.base, self, buffer, bytes);

    ov_log_debug("recv %zi bytes from %s:%i", bytes, 
       remote.host,
       remote.port);

    return result;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

ov_interconnect_loop *ov_interconnect_loop_create(
    ov_interconnect_loop_config config){

    ov_interconnect_loop *self = NULL;

    if (!config.loop) goto error;
    if (0 == config.name[0]) goto error;
    if (0 == config.multicast.host[0]) goto error;
    if (0 == config.internal.host[0]) goto error;
    if (0 == config.multicast.port) goto error;

    config.multicast.type = UDP;

    self = calloc(1, sizeof(ov_interconnect_loop));
    if (!self) goto error;

    self->config = config;
    self->ssrc = ov_random_uint32(); 

    ov_socket_configuration socket = config.internal;
    socket.type = UDP;
    socket.port = 0;

    self->socket = ov_socket_create(socket, false, NULL);
    if (-1 == self->socket){
        ov_log_error("could not open socket at %s", config.internal.host);
    }

    if (!ov_socket_ensure_nonblocking(self->socket))goto error;

    if (!ov_socket_get_data(self->socket, &self->local, NULL)) goto error;

    ov_log_debug("opened LOOP receiver %s | %s:%i", 
        self->config.name, 
        self->local.host,
        self->local.port);

    if (!ov_event_loop_set(
        self->config.loop, 
        self->socket,
        OV_EVENT_IO_IN | OV_EVENT_IO_ERR | OV_EVENT_IO_CLOSE,
        self, 
        io_from_mixer)) goto error;

    return self;
error:
    ov_interconnect_loop_free(self);
    return NULL;
}

/*----------------------------------------------------------------------------*/

void *ov_interconnect_loop_free(void *data){

    if (!data) return NULL;

    ov_interconnect_loop *self = (ov_interconnect_loop*)data;

    if (-1 != self->socket){

        ov_event_loop_unset(self->config.loop, self->socket, NULL);
        close(self->socket);
        self->socket = -1;

    }

    ov_interconnect_drop_mixer(self->config.base, self->mixer.socket);
    self = ov_data_pointer_free(self);
    return NULL;
}

/*----------------------------------------------------------------------------*/

uint32_t ov_interconnect_loop_get_ssrc(const ov_interconnect_loop *self){

    if (!self) return 0;
    return self->ssrc;
}

/*----------------------------------------------------------------------------*/

const char *ov_interconnect_loop_get_name(const ov_interconnect_loop *self){

    if (!self) return NULL;
    return self->config.name;
}

/*----------------------------------------------------------------------------*/

bool ov_interconnect_loop_has_mixer(const ov_interconnect_loop *self){

    if (!self) return false;

    if (0 != self->mixer.socket)
        return true;

    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_interconnect_loop_assign_mixer(ov_interconnect_loop *self){

    if (!self) return false;

    if (self->mixer.socket > 0)
        return true;

    ov_mixer_data data = ov_interconnect_assign_mixer(
        self->config.base,
        self->config.name);

    if (0 == data.socket) goto error;

    self->mixer = data;

    ov_socket_configuration config = (ov_socket_configuration){
        .port = self->local.port,
        .type = UDP
    };

    strncpy(config.host, self->local.host, OV_HOST_NAME_MAX);

    ov_mixer_forward forward = (ov_mixer_forward){
        .socket = config,
        .ssrc = self->ssrc,
        .payload_type = 100
    };

    ov_log_debug("assgined mixer to loop %s", self->config.name);
    
    return ov_interconnect_send_aquire_mixer(
        self->config.base,
        data,
        forward);
error:
    return false;
}

/*----------------------------------------------------------------------------*/

int ov_interconnect_loop_get_mixer(const ov_interconnect_loop *self){

    if (!self) return -1;
    return self->mixer.socket;
}

/*----------------------------------------------------------------------------*/

ov_mixer_forward ov_interconnect_loop_get_forward(
        const ov_interconnect_loop *self){

    if (!self) goto error;

     ov_socket_configuration config = (ov_socket_configuration){
        .port = self->local.port,
        .type = UDP
    };
    strncpy(config.host, self->local.host, OV_HOST_NAME_MAX);

    ov_mixer_forward forward = (ov_mixer_forward){
        .socket = config,
        .ssrc = self->ssrc,
        .payload_type = 100
    };

    return forward;
error:
    return (ov_mixer_forward){0};
}

/*----------------------------------------------------------------------------*/

ov_mc_loop_data ov_interconnect_loop_get_loop_data(
    const ov_interconnect_loop *self){

    if (!self) return (ov_mc_loop_data){0};

    uint8_t volume_scale = ov_convert_from_vol_percent(50, 3);

    ov_mc_loop_data data = (ov_mc_loop_data){
        .socket = self->config.multicast,
        .volume = volume_scale
    };

    strncpy(data.name, self->config.name, OV_HOST_NAME_MAX);

    return data;
}

/*----------------------------------------------------------------------------*/

bool ov_interconnect_loop_send(
        const ov_interconnect_loop *self,
        const uint8_t *buffer, 
        size_t size){

    if (!self || !buffer || !size)
        goto error;

    struct sockaddr_storage dest = {0};

    if (!ov_socket_fill_sockaddr_storage(&dest, self->local.sa.ss_family,
                                         self->config.multicast.host,
                                         self->config.multicast.port)) {
        goto error;
    }

    socklen_t len = sizeof(struct sockaddr_in);
    if (self->local.sa.ss_family == AF_INET6)
        len = sizeof(struct sockaddr_in6);

    ssize_t out =
        sendto(self->socket, buffer, size, 0, (struct sockaddr *)&dest, len);

    ov_log_debug("send %zi bytes to %s:%i", out, 
        self->config.multicast.host,
        self->config.multicast.port);

    if (out != (ssize_t) size) goto error;

    return true;
error:
    return false;

}