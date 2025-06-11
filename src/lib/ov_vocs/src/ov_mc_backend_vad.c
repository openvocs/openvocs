/***
        ------------------------------------------------------------------------

        Copyright (c) 2025 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_mc_backend_vad.c
        @author         Markus Töpfer

        @date           2025-01-11


        ------------------------------------------------------------------------
*/
#include "../include/ov_mc_backend_vad.h"

#define OV_MC_BACKEND_VAD_MAGIC_BYTES 0xfad1

#include <ov_core/ov_event_app.h>

/*---------------------------------------------------------------------------*/

struct ov_mc_backend_vad {

    uint16_t magic_bytes;
    ov_mc_backend_vad_config config;

    int socket;

    ov_event_app *app;

    ov_dict *connections;
};

/*----------------------------------------------------------------------------*/

typedef struct Connection {

    int socket;

} Connection;

/*----------------------------------------------------------------------------*/

static void cb_socket_close(void *userdata, int socket) {

    ov_mc_backend_vad *self = ov_mc_backend_vad_cast(userdata);

    ov_log_error("Lost vad at %i", socket);

    ov_dict_del(self->connections, (void *)(intptr_t)socket);

    return;
}

/*---------------------------------------------------------------------------*/

static void cb_register(void *userdata,
                        const char *name,
                        int socket,
                        ov_json_value *input) {

    ov_mc_backend_vad *self = ov_mc_backend_vad_cast(userdata);
    if (!self || !name || !socket || !input) goto error;

    Connection *conn = calloc(1, sizeof(Connection));
    if (!conn) goto error;

    conn->socket = socket;

    ov_dict_set(self->connections, (void *)(intptr_t)socket, conn, NULL);

    ov_json_value *out = ov_event_api_create_success_response(input);
    ov_event_app_send(self->app, socket, out);
    out = ov_json_value_free(out);

error:
    ov_json_value_free(input);
    return;
}

/*---------------------------------------------------------------------------*/

static void cb_loops(void *userdata,
                     const char *name,
                     int socket,
                     ov_json_value *input) {

    ov_mc_backend_vad *self = ov_mc_backend_vad_cast(userdata);
    if (!self || !name || !socket || !input) goto error;

    ov_json_value *loops = ov_vocs_db_get_all_loops(self->config.db);
    if (!loops) goto error;

    ov_json_value *out = ov_event_api_create_success_response(input);
    ov_json_value *res = ov_event_api_get_response(out);
    ov_json_object_set(res, OV_KEY_LOOPS, loops);

    ov_event_app_send(self->app, socket, out);
    out = ov_json_value_free(out);

error:
    ov_json_value_free(input);
    return;
}

/*---------------------------------------------------------------------------*/

static void cb_vad(void *userdata,
                   const char *name,
                   int socket,
                   ov_json_value *input) {

    bool vad_on = false;

    ov_mc_backend_vad *self = ov_mc_backend_vad_cast(userdata);
    if (!self || !name || !socket || !input) goto error;

    const char *loop = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_LOOP));

    const ov_json_value *on =
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_ON);

    if (ov_json_is_true(on)) vad_on = true;

    if (self->config.callbacks.vad)
        self->config.callbacks.vad(
            self->config.callbacks.userdata, loop, vad_on);

error:
    ov_json_value_free(input);
    return;
}

/*---------------------------------------------------------------------------*/

static bool register_callbacks(ov_mc_backend_vad *self) {

    if (!self) goto error;

    if (!ov_event_app_register(self->app, OV_KEY_REGISTER, self, cb_register))
        goto error;

    if (!ov_event_app_register(self->app, OV_KEY_LOOPS, self, cb_loops))
        goto error;

    if (!ov_event_app_register(self->app, OV_KEY_VAD, self, cb_vad)) goto error;

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool init_config(ov_mc_backend_vad_config *config) {

    if (!config->loop) goto error;
    if (!config->io) goto error;

    if (0 == config->socket.host[0]) goto error;

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

ov_mc_backend_vad *ov_mc_backend_vad_create(ov_mc_backend_vad_config config) {

    ov_mc_backend_vad *self = NULL;

    if (!init_config(&config)) goto error;

    self = calloc(1, sizeof(ov_mc_backend_vad));
    if (!self) goto error;

    self->magic_bytes = OV_MC_BACKEND_VAD_MAGIC_BYTES;
    self->config = config;

    self->app = ov_event_app_create(
        (ov_event_app_config){.io = config.io,
                              .callbacks.userdata = self,
                              .callbacks.close = cb_socket_close});

    if (!self->app) goto error;

    self->socket = ov_event_app_open_listener(
        self->app, (ov_io_socket_config){.socket = config.socket});

    if (-1 == self->socket) {

        ov_log_error("Failed to open socket %s:%i",
                     config.socket.host,
                     config.socket.port);

        goto error;
    }

    if (!register_callbacks(self)) goto error;

    ov_dict_config d_config = ov_dict_intptr_key_config(255);
    d_config.value.data_function.free = ov_data_pointer_free;

    self->connections = ov_dict_create(d_config);
    if (!self->connections) goto error;

    return self;
error:
    ov_mc_backend_vad_free(self);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_mc_backend_vad *ov_mc_backend_vad_free(ov_mc_backend_vad *self) {

    if (!ov_mc_backend_vad_cast(self)) return self;

    self->app = ov_event_app_free(self->app);
    self->connections = ov_dict_free(self->connections);

    self = ov_data_pointer_free(self);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_mc_backend_vad *ov_mc_backend_vad_cast(const void *data) {

    if (!data) return NULL;

    if (*(uint16_t *)data != OV_MC_BACKEND_VAD_MAGIC_BYTES) return NULL;

    return (ov_mc_backend_vad *)data;
}

/*----------------------------------------------------------------------------*/

ov_mc_backend_vad_config ov_mc_backend_vad_config_from_json(
    const ov_json_value *in) {

    ov_mc_backend_vad_config config = {0};
    const ov_json_value *conf = ov_json_object_get(in, OV_KEY_VAD);
    if (!conf) conf = in;

    config.socket = ov_socket_configuration_from_json(
        ov_json_get(conf, "/" OV_KEY_SOCKET), (ov_socket_configuration){0});

    return config;
}
