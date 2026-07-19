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
        @file           ov_cluster.c
        @author         Töpfer, Markus

        @date           2026-03-11


        ------------------------------------------------------------------------
*/
#include "../include/ov_cluster.h"

#define OV_CLUSTER_MAGIC_BYTE 0x42cc

#include <ov_base/ov_mc_io_buffer.h>
#include <ov_base/ov_mc_socket.h>
#include <ov_base/ov_string.h>
#include <ov_base/ov_id.h>

/*----------------------------------------------------------------------------*/

struct ov_cluster {

    uint16_t magic_byte;
    ov_cluster_config config;

    ov_id uuid;

    ov_socket_data local;
    ov_socket_data remote;

    int socket;

    ov_socket_data sender_local;
    int sender;

    ov_mc_io_buffer *buffer;
};

/*----------------------------------------------------------------------------*/

static void cb_json_success(void *userdata, ov_socket_data remote, ov_json_value *msg){

    ov_cluster *self = ov_cluster_cast(userdata);
    if (!self || !msg) goto error;

    const char *uuid = ov_json_string_get(ov_json_object_get(msg, "cluster_id"));

    if (ov_id_match(uuid, self->uuid)){
        msg = ov_json_value_free(msg);
        goto error;
    }

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

    ov_cluster *self = ov_cluster_cast(userdata);
    if (!self) goto error;

    UNUSED(remote);

error:
    return;
}

/*----------------------------------------------------------------------------*/

static bool io_multicast(int socket, uint8_t events, void *userdata){

    uint8_t buffer[2048] = {0};

    ov_cluster *self = ov_cluster_cast(userdata);
    if (!self) goto error;

    ov_socket_data remote = {0};
    socklen_t len = sizeof(struct sockaddr_storage);

    if ( (events & OV_EVENT_IO_ERR) || (events & OV_EVENT_IO_CLOSE)) {

        ov_log_error("Multicast socket closed."); 
        goto error;
    
    }

    ssize_t bytes = recvfrom(socket, buffer, 2048, 0, 
        (struct sockaddr*) &remote.sa, &len);

    remote = ov_socket_data_from_sockaddr_storage(&remote.sa);

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

static bool init_config(ov_cluster_config *config){

    if (!config->loop) goto error;

    return true;
error:
    ov_log_error("Config init failed.");
    return false;
}

/*----------------------------------------------------------------------------*/

ov_cluster *ov_cluster_create(ov_cluster_config config){

    ov_cluster *self = NULL;

    if (!init_config(&config)) goto error;

    self = calloc(1, sizeof(ov_cluster));
    if (!self) goto error;

    self->magic_byte = OV_CLUSTER_MAGIC_BYTE;
    self->config = config;

    ov_id_fill_with_uuid(self->uuid);

    self->socket = ov_mc_socket(self->config.multicast);
    if (-1 == self->socket){

        ov_log_error("Failed to open Multicast socket %s:%i",
            config.multicast.host,
            config.multicast.port);
    
        goto error;
    
    } else {

        ov_log_info("cluster opened multicast recv %s:%i",
            config.multicast.host,
            config.multicast.port);

    }

    ov_socket_ensure_nonblocking(self->socket);
    ov_socket_get_data(self->socket, &self->local, NULL);
    self->remote = ov_socket_configuration_to_socket_data(self->config.multicast);

    ov_socket_configuration send = (ov_socket_configuration){
        .host = "0.0.0.0",
        .port = 0,
        .type = UDP
    };

    if (self->local.sa.ss_family == AF_INET6){

         send = (ov_socket_configuration){
            .host = "::",
            .port = 0,
            .type = UDP
        };

    }

    self->sender = ov_socket_create(send, false, NULL);
    if (!self->sender) goto error;

    ov_socket_get_data(self->sender, &self->sender_local, NULL);

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
    ov_cluster_free(self);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_cluster *ov_cluster_cast(const void *self) {

    if (!self)
        goto error;

    if (*(uint16_t *)self == OV_CLUSTER_MAGIC_BYTE)
        return (ov_cluster *)self;
error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_cluster *ov_cluster_free(ov_cluster *self){

    if (!ov_cluster_cast(self)) return self;

    if (-1 != self->socket){

        ov_event_loop_unset(self->config.loop, self->socket, NULL);
        close(self->socket);
        self->socket = -1;

    }

    if (-1 != self->sender){

        close(self->sender);
        self->sender = -1;

    }

    self->buffer = ov_mc_io_buffer_free(self->buffer);

    self = ov_data_pointer_free(self);
    return NULL;
}

/*----------------------------------------------------------------------------*/

bool ov_cluster_send(ov_cluster *self, const ov_json_value *msg){

    if (!self || !msg) goto error;

    ov_json_object_set((ov_json_value*) msg, "cluster_id", ov_json_string(self->uuid));

    char *str = ov_json_value_to_string(msg);
    if (!str) goto error;

    socklen_t len = sizeof(struct sockaddr_in);
    
    if (self->remote.sa.ss_family == AF_INET6)
        len = sizeof(struct sockaddr_in6);

    ssize_t bytes = sendto(self->sender, str, strlen(str), 0,
        (struct sockaddr *)& self->remote.sa, len);

    str = ov_data_pointer_free(str);
    if (bytes > 0) return true;
error:
    return false;
}