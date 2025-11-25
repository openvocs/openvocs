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
        @file           ov_vad_app.c
        @author         Markus Töpfer

        @date           2025-01-10


        ------------------------------------------------------------------------
*/
#include "../include/ov_vad_app.h"
#include "../include/ov_vad_app_msg.h"

#define OV_VAD_APP_MAGIC_BYTES 0xf345

#include <ov_base/ov_config_keys.h>
#include <ov_base/ov_vad_config.h>
#include <ov_core/ov_event_api.h>
#include <ov_core/ov_event_app.h>

/*---------------------------------------------------------------------------*/

struct ov_vad_app {

    uint16_t magic_bytes;
    ov_vad_app_config config;

    int socket;

    ov_vad_core *vad;
    ov_event_app *app;
};

/*---------------------------------------------------------------------------*/

static void cb_socket_close(void *userdata, int socket) {

    ov_vad_app *self = ov_vad_app_cast(userdata);
    self->socket = 0;
    ov_log_error("VAD manager socket closed at %i", socket);
    return;
}

/*---------------------------------------------------------------------------*/

static void cb_socket_connected(void *userdata, int socket) {

    ov_vad_app *self = ov_vad_app_cast(userdata);
    self->socket = socket;
    ov_log_info("VAD manager socket connected at %i", socket);

    ov_json_value *out = ov_vad_app_msg_register();
    ov_event_app_send(self->app, socket, out);
    out = ov_json_value_free(out);

    return;
}

/*---------------------------------------------------------------------------*/

static void cb_register_response(ov_vad_app *self,
                                 int socket,
                                 ov_json_value *input) {

    if (!self || !socket || !input) goto error;

    ov_json_value *out = ov_vad_app_msg_loops();
    ov_event_app_send(self->app, socket, out);
    out = ov_json_value_free(out);

    ov_json_value *par = ov_event_api_get_response(input);
    ov_vad_config conf = ov_vad_config_from_json(par);

    ov_vad_core_set_vad(self->vad, conf);

error:
    ov_json_value_free(input);
    return;
}

/*---------------------------------------------------------------------------*/

static void cb_register(void *userdata,
                        const char *name,
                        int socket,
                        ov_json_value *input) {

    ov_vad_app *self = ov_vad_app_cast(userdata);
    if (!self || !name || !socket || !input) goto error;

    if (ov_event_api_get_response(input))
        return cb_register_response(self, socket, input);

    // WE DO NOT EXPECT A NON RESPONSE FOR REGISTER

error:
    ov_json_value_free(input);
    return;
}

/*---------------------------------------------------------------------------*/

static bool add_loop_vad(const void *key, void *val, void *data) {

    if (!key) return true;
    ov_vad_app *self = ov_vad_app_cast(data);

    const char *id = ov_json_string_get(ov_json_object_get(val, OV_KEY_ID));

    ov_socket_configuration socket = ov_socket_configuration_from_json(
        ov_json_object_get(val, OV_KEY_MULTICAST),
        (ov_socket_configuration){0});

    socket.type = UDP;

    ov_vad_core_add_loop(self->vad, id, socket);

    return true;
}

/*---------------------------------------------------------------------------*/

static void cb_loops(void *userdata,
                     const char *name,
                     int socket,
                     ov_json_value *input) {

    ov_vad_app *self = ov_vad_app_cast(userdata);
    if (!self || !name || !socket || !input) goto error;

    ov_json_value *res = ov_event_api_get_response(input);
    if (!res) goto error;

    ov_json_value *loops = ov_json_object_get(res, OV_KEY_LOOPS);

    ov_json_object_for_each(loops, self, add_loop_vad);

error:
    ov_json_value_free(input);
    return;
}

/*---------------------------------------------------------------------------*/

static bool register_callbacks(ov_vad_app *self) {

    if (!self) goto error;

    if (!ov_event_app_register(self->app, OV_KEY_REGISTER, self, cb_register))
        goto error;

    if (!ov_event_app_register(self->app, OV_KEY_LOOPS, self, cb_loops))
        goto error;

    return true;
error:
    return false;
}

/*---------------------------------------------------------------------------*/

static void io_vad(void *userdata, const char *loop, bool on) {

    ov_vad_app *self = ov_vad_app_cast(userdata);

    ov_json_value *val = NULL;

    ov_json_value *out = ov_event_api_message_create(OV_KEY_VAD, NULL, 0);
    ov_json_value *par = ov_event_api_set_parameter(out);

    ov_json_object_set(par, OV_KEY_LOOP, ov_json_string(loop));

    if (on) {
        val = ov_json_true();
    } else {
        val = ov_json_false();
    }

    ov_json_object_set(par, OV_KEY_ON, val);
    ov_event_app_send(self->app, self->socket, out);
    out = ov_json_value_free(out);
    return;
}

/*---------------------------------------------------------------------------*/

ov_vad_app *ov_vad_app_create(ov_vad_app_config config) {

    ov_vad_app *self = NULL;

    if (!config.loop) goto error;
    if (!config.io) goto error;

    self = calloc(1, sizeof(ov_vad_app));
    if (!self) goto error;

    self->magic_bytes = OV_VAD_APP_MAGIC_BYTES;
    self->config = config;

    config.core.callbacks.userdata = self;
    config.core.callbacks.vad = io_vad;
    config.core.loop = config.loop;

    self->vad = ov_vad_core_create(config.core);
    self->app = ov_event_app_create(
        (ov_event_app_config){.io = config.io,
                              .callbacks.userdata = self,
                              .callbacks.close = cb_socket_close,
                              .callbacks.connected = cb_socket_connected});

    if (!self->app) goto error;

    ov_event_app_open_connection(
        self->app,
        (ov_io_socket_config){
            .auto_reconnect = true, .socket = config.manager});

    if (!register_callbacks(self)) goto error;

    return self;
error:
    ov_vad_app_free(self);
    return NULL;
}

/*---------------------------------------------------------------------------*/

ov_vad_app *ov_vad_app_free(ov_vad_app *self) {

    if (!ov_vad_app_cast(self)) return self;

    self->vad = ov_vad_core_free(self->vad);
    self->app = ov_event_app_free(self->app);

    self = ov_data_pointer_free(self);
    return NULL;
}

/*---------------------------------------------------------------------------*/

ov_vad_app *ov_vad_app_cast(const void *data) {

    if (!data) return NULL;

    if (*(uint16_t *)data != OV_VAD_APP_MAGIC_BYTES) return NULL;

    return (ov_vad_app *)data;
}

/*---------------------------------------------------------------------------*/

ov_vad_app_config ov_vad_app_config_from_json(const ov_json_value *v) {

    ov_vad_app_config config = {0};
    const ov_json_value *conf = ov_json_object_get(v, OV_KEY_VAD);
    if (!conf) conf = v;

    config.manager = ov_socket_configuration_from_json(
        ov_json_get(conf, "/" OV_KEY_SOCKET), (ov_socket_configuration){0});

    config.core = ov_vad_core_config_from_json(conf);

    return config;
}