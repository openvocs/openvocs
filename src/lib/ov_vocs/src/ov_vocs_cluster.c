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
        @file           ov_vocs_cluster.c
        @author         Töpfer, Markus

        @date           2026-03-10


        ------------------------------------------------------------------------
*/
#include "../include/ov_vocs_cluster.h"

#define OV_VOCS_CLUSTER_MAGIC_BYTE 0x4242

#include <ov_base/ov_mc_io_buffer.h>
#include <ov_base/ov_mc_socket.h>

/*----------------------------------------------------------------------------*/

struct ov_vocs_cluster {

    uint16_t magic_byte;
    ov_vocs_cluster_config config;

    int socket;

    ov_mc_io_buffer *buffer;
};

/*----------------------------------------------------------------------------*/

static void cb_json_success(void *userdata, ov_socket_data remote, ov_json_value *msg){

    ov_vocs_cluster *self = ov_vocs_cluster_cast(userdata);
    if (!self || !msg) goto error;

    UNUSED(remote);

    if (self->config.callback.io){

        self->config.callback.io(
            self->config.callback.userdata,
            msg);
    } else {

        msg = ov_json_value_free(msg);
    }

error:
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_json_failure(void *userdata, ov_socket_data remote){

    ov_vocs_cluster *self = ov_vocs_cluster_cast(userdata);
    if (!self) goto error;

    UNUSED(remote);

error:
    return;
}

/*----------------------------------------------------------------------------*/

static bool io_multicast(int socket, uint8_t events, void *userdata){

    uint8_t buffer[2048] = {0};

    ov_vocs_cluster *self = ov_vocs_cluster_cast(userdata);
    if (!self) goto error;

    ov_socket_data remote = {0};
    if (!ov_socket_get_data(socket, NULL, &remote)) goto error;

    if ( (events & OV_EVENT_IO_ERR) || (events & OV_EVENT_IO_CLOSE)) {

        ov_log_error("Multicast socket closed."); 
        goto error;
    
    }

    ssize_t bytes = recv(socket, buffer, 2048, 0);

    if (bytes < 0) goto error;

    ov_mc_io_buffer_push(self->buffer, remote, (ov_memory_pointer){
        .start = buffer, 
        .length = bytes
    });

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool init_config(ov_vocs_cluster_config *config){

    if (!config->loop) goto error;

    return true;
error:
    ov_log_error("Config init failed.");
    return false;
}

/*----------------------------------------------------------------------------*/

ov_vocs_cluster *ov_vocs_cluster_create(ov_vocs_cluster_config config){

    ov_vocs_cluster *self = NULL;

    if (!init_config(&config)) goto error;

    self = calloc(1, sizeof(ov_vocs_cluster));
    if (!self) goto error;

    self->magic_byte = OV_VOCS_CLUSTER_MAGIC_BYTE;
    self->config = config;

    self->socket = ov_mc_socket(self->config.multicast);
    if (-1 == self->socket){

        ov_log_error("Failed to open Multicast socket %s:%i",
            config.multicast.host,
            config.multicast.port);
    
        goto error;
    }

    ov_socket_ensure_nonblocking(self->socket);

    ov_event_loop_set(
        self->config.loop,
        self->socket,
        OV_EVENT_IO_IN | OV_EVENT_IO_ERR | OV_EVENT_IO_CLOSE,
        self,
        io_multicast);

    self->buffer = ov_mc_io_buffer_create((ov_mc_io_buffer_config){
        .debug = false,
        .objects_only = false,
        .callback.userdata = self,
        .callback.success = cb_json_success,
        .callback.failure = cb_json_failure
    });

    if (!self->buffer) goto error;

    return self;
error:
    ov_vocs_cluster_free(self);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_vocs_cluster *ov_vocs_cluster_cast(const void *self) {

    if (!self)
        goto error;

    if (*(uint16_t *)self == OV_VOCS_CLUSTER_MAGIC_BYTE)
        return (ov_vocs_cluster *)self;
error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_vocs_cluster *ov_vocs_cluster_free(ov_vocs_cluster *self){

    if (!ov_vocs_cluster_cast(self)) return self;

    if (-1 != self->socket){

        ov_event_loop_unset(self->config.loop, self->socket, NULL);
        close(self->socket);
        self->socket = -1;

    }

    self = ov_data_pointer_free(self);
    return NULL;
}

/*----------------------------------------------------------------------------*/

bool ov_vocs_cluster_send(ov_vocs_cluster *self, const ov_json_value *msg){

    if (!self || !msg) goto error;

    char *str = ov_json_value_to_string(msg);
    if (!str) goto error;

    ssize_t bytes = send(self->socket, str, strlen(str),0);

    str = ov_data_pointer_free(str);
    if (bytes > 0) return true;
error:
    return false;
}