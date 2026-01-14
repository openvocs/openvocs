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
        @file           ov_mc_interconnect_mixer.c
        @author         Töpfer, Markus

        @date           2026-01-13


        ------------------------------------------------------------------------
*/
#include "../include/ov_mc_interconnect_mixer.h"

#include <ov_core/ov_event_engine.h>
#include <ov_core/ov_event_socket.h>
#include <ov_core/ov_mixer_registry.h>

/*----------------------------------------------------------------------------*/

struct ov_mc_interconnect_mixer {

    ov_mc_interconnect_mixer_config config;

    struct {

        ov_event_engine *engine;
        ov_event_socket *socket;

    } event;

    int socket;

    ov_mixer_registry *registry;

};

/*----------------------------------------------------------------------------*/

static void cb_close(void *userdata, int socket){

    ov_mc_interconnect_mixer *self = (ov_mc_interconnect_mixer*)userdata;
    UNUSED(socket);
    UNUSED(self);
    TODO("... implement close");

    return;
}

/*----------------------------------------------------------------------------*/

static bool event_register(void *userdata, const int socket,
                    const ov_event_parameter *parameter, ov_json_value *input){

    ov_mc_interconnect_mixer *self = (ov_mc_interconnect_mixer*) userdata;
    if (!self || !parameter || !input) goto error;

    ov_socket_data remote = {0};
    if (!ov_socket_get_data(socket, NULL, &remote)) goto error;

    if (!ov_mixer_registry_register_mixer(
        self->registry, socket, &remote)) goto error;

    ov_log_debug("registered mixer %s:%i", remote.host, remote.port);

    ov_json_value *out = ov_mixer_msg_configure(self->config.mixer);
    ov_event_io_send(parameter, socket, out);
    out = ov_json_value_free(out);

    if (self->config.callbacks.mixer_available)
        self->config.callbacks.mixer_available(
            self->config.callbacks.userdata, socket);

    input = ov_json_value_free(input);
    return true;
error:
    input = ov_json_value_free(input);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool event_acquire(void *userdata, const int socket,
                    const ov_event_parameter *parameter, ov_json_value *input){

    ov_mc_interconnect_mixer *self = (ov_mc_interconnect_mixer*) userdata;
    if (!self || !parameter || !input) goto error;

    ov_json_value *res = ov_event_api_get_response(input);
    if (!res) goto error;

    if (0 == ov_event_api_get_error_code(input)){

        if (self->config.callbacks.aquire)
            self->config.callbacks.aquire(
                self->config.callbacks.userdata,
                socket,
                true);

    } else {

        if (self->config.callbacks.aquire)
            self->config.callbacks.aquire(
                self->config.callbacks.userdata,
                socket,
                false);
    }


    input = ov_json_value_free(input);
    return true;
error:
    input = ov_json_value_free(input);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool event_forward(void *userdata, const int socket,
                    const ov_event_parameter *parameter, ov_json_value *input){

    ov_mc_interconnect_mixer *self = (ov_mc_interconnect_mixer*) userdata;
    if (!self || !parameter || !input) goto error;

    ov_json_value *res = ov_event_api_get_response(input);
    if (!res) goto error;

    if (0 == ov_event_api_get_error_code(input)){

        if (self->config.callbacks.forward)
            self->config.callbacks.forward(
                self->config.callbacks.userdata,
                socket,
                true);

    } else {

        if (self->config.callbacks.forward)
            self->config.callbacks.forward(
                self->config.callbacks.userdata,
                socket,
                false);
    }


    input = ov_json_value_free(input);
    return true;
error:
    input = ov_json_value_free(input);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool event_join(void *userdata, const int socket,
                    const ov_event_parameter *parameter, ov_json_value *input){

    ov_mc_interconnect_mixer *self = (ov_mc_interconnect_mixer*) userdata;
    if (!self || !parameter || !input) goto error;

    ov_json_value *res = ov_event_api_get_response(input);
    if (!res) goto error;

    if (0 == ov_event_api_get_error_code(input)){

        if (self->config.callbacks.join)
            self->config.callbacks.join(
                self->config.callbacks.userdata,
                socket,
                true);

    } else {

        if (self->config.callbacks.join)
            self->config.callbacks.join(
                self->config.callbacks.userdata,
                socket,
                false);
    }


    input = ov_json_value_free(input);
    return true;
error:
    input = ov_json_value_free(input);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool register_events(ov_mc_interconnect_mixer *self) {

    if (!self)
        goto error;

    if (!ov_event_engine_register(self->event.engine,
                                  "register", self,
                                  event_register))
        goto error;

    if (!ov_event_engine_register(self->event.engine,
                                  "acquire", self,
                                  event_acquire))
        goto error;

    if (!ov_event_engine_register(self->event.engine,
                                  "forward", self,
                                  event_forward))
        goto error;

    if (!ov_event_engine_register(self->event.engine,
                                  "join", self,
                                  event_join))
        goto error;

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

ov_mc_interconnect_mixer *ov_mc_interconnect_mixer_create(
    ov_mc_interconnect_mixer_config config){

    ov_mc_interconnect_mixer *self = NULL;
    if (!config.loop) goto error;

    self = calloc(1, sizeof(ov_mc_interconnect_mixer));
    if (!self) goto error;

    self->config = config;

    self->event.engine = ov_event_engine_create();

    if (!self->event.engine)
        goto error;

    if (!register_events(self))
        goto error;

    self->event.socket = ov_event_socket_create(
        (ov_event_socket_config){.loop = self->config.loop,
                                 .engine = self->event.engine,
                                 .callback.userdata = self,
                                 .callback.close = cb_close,
                                 .callback.connected = NULL});

    if (!self->event.socket)
        goto error;

    self->socket = ov_event_socket_create_listener(self->event.socket,
        (ov_event_socket_server_config){
            .socket = self->config.socket
        });

    if (-1 == self->socket) goto error;

    self->registry = ov_mixer_registry_create(
        (ov_mixer_registry_config){0});

    if (!self->registry) goto error;

    return self;
error:
    ov_mc_interconnect_mixer_free(self);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_mc_interconnect_mixer *ov_mc_interconnect_mixer_free(
    ov_mc_interconnect_mixer *self){

    if (!self) return NULL;

    self->event.engine = ov_event_engine_free(self->event.engine);
    self->event.socket = ov_event_socket_free(self->event.socket);
    self->registry = ov_mixer_registry_free(self->registry);

    self = ov_data_pointer_free(self);
    return NULL;

}

/*----------------------------------------------------------------------------*/

ov_mixer_data ov_mc_interconnect_mixer_assign_mixer(
    ov_mc_interconnect_mixer *self,
    const char *name){

    ov_mixer_data data = ov_mixer_registry_acquire_user(
        self->registry, name);

    return data;
}

/*----------------------------------------------------------------------------*/

bool ov_mc_interconnect_mixer_send_aquire(
    ov_mc_interconnect_mixer *self,
    ov_mixer_data data,
    ov_mixer_forward forward){

    if (!self) goto error;

    if (0 == data.user[0]) goto error;

    ov_json_value *out = ov_mixer_msg_acquire(
        data.user,
        forward);

    bool result = ov_event_socket_send(self->event.socket,
        data.socket, out);

    out = ov_json_value_free(out);

    return result;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_mc_interconnect_mixer_send_forward(
    ov_mc_interconnect_mixer *self,
    int socket,
    const char *name, 
    ov_mixer_forward forward){

    if (!self) goto error;

    ov_json_value *out = ov_mixer_msg_forward(
        name,
        forward);

    bool result = ov_event_socket_send(self->event.socket,
        socket, out);

    out = ov_json_value_free(out);

    return result;
error:  
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_mc_interconnect_mixer_send_switch(
    ov_mc_interconnect_mixer *self,
    int socket,
    ov_mc_loop_data data){

    if (!self) return false;

    ov_json_value *out = ov_mixer_msg_join(data);

    bool result = ov_event_socket_send(self->event.socket,
        socket, out);

    out = ov_json_value_free(out);
    return result;

}