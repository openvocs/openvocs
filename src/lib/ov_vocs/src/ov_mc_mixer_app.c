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
        @file           ov_mc_mixer_app.c
        @author         Markus Töpfer

        @date           2022-11-11


        ------------------------------------------------------------------------
*/
#include "../include/ov_mc_mixer_app.h"

#include <ov_base/ov_config_keys.h>
#include <ov_base/ov_error_codes.h>

#include <ov_core/ov_event_api.h>
#include <ov_core/ov_event_app.h>

#include <ov_base/ov_id.h>

#define OV_MC_MIXER_APP_MAGIC_BYTES 0xFEED

/*----------------------------------------------------------------------------*/

struct ov_mc_mixer_app {

    uint16_t magic_bytes;
    ov_mc_mixer_app_config config;

    ov_id uuid;

    int socket;
    ov_socket_data local;
    ov_socket_data remote;

    ov_event_app *app;
    ov_mc_mixer_core *mixer;
};

/*----------------------------------------------------------------------------*/

static void cb_close(void *userdata, int socket) {

    ov_mc_mixer_app *app = ov_mc_mixer_app_cast(userdata);
    if (!app)
        goto error;

    ov_log_error("Mixer socket closed %i", socket);
    app->socket = -1;

    ov_mc_mixer_core_release(app->mixer);

error:
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_connected(void *userdata, int socket) {

    ov_mc_mixer_app *app = ov_mc_mixer_app_cast(userdata);

    app->socket = socket;
    ov_socket_get_data(socket, &app->local, &app->remote);

    ov_log_info("Mixer connected at %i from %s:%i to %s:%i", socket,
                app->local.host, app->local.port, app->remote.host,
                app->remote.port);

    ov_json_value *out = ov_mc_mixer_msg_register(app->uuid, OV_KEY_AUDIO);
    ov_event_app_send(app->app, socket, out);
    out = ov_json_value_free(out);

    return;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      EVENT FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

static void cb_event_register(void *userdata, const char *name, int socket,
                              ov_json_value *input) {

    ov_mc_mixer_app *app = ov_mc_mixer_app_cast(userdata);
    if (!app || !name || socket < 0 || !input)
        goto error;

error:
    ov_json_value_free(input);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_event_configure(void *userdata, const char *name, int socket,
                               ov_json_value *input) {

    ov_mc_mixer_app *app = ov_mc_mixer_app_cast(userdata);
    if (!app || !name || socket < 0 || !input)
        goto error;

    ov_mc_mixer_core_config config = ov_mc_mixer_msg_configure_from_json(input);
    config.loop = app->config.loop;

    if (!ov_mc_mixer_core_reconfigure(app->mixer, config)) {
        ov_log_error("Failed to reconfigure mixer.");
        ov_event_app_close(app->app, app->socket);
    } else {
        ov_log_debug("Reconfigured mixer.");
    }

error:
    ov_json_value_free(input);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_event_acquire(void *userdata, const char *name, int socket,
                             ov_json_value *input) {

    /* acquire the mixer with some user and forward data */

    ov_json_value *out = NULL;

    ov_mc_mixer_app *app = ov_mc_mixer_app_cast(userdata);
    if (!app || !name || socket < 0 || !input)
        goto error;

    const char *user = ov_mc_mixer_msg_aquire_get_username(input);
    ov_mc_mixer_core_forward forward =
        ov_mc_mixer_msg_acquire_get_forward(input);

    /* the message must contain valid data, otherwise the system is broken */

    if (!user || !ov_mc_mixer_core_forward_data_is_valid(&forward))
        goto input_error_response;

    /* release the mixer and reset to new data */

    if (!ov_mc_mixer_core_release(app->mixer))
        goto error_response;
    if (!ov_mc_mixer_core_set_name(app->mixer, user))
        goto error_response;
    if (!ov_mc_mixer_core_set_forward(app->mixer, forward))
        goto error_response;

    ov_log_debug("mixer acquired for %s", user);

    out = ov_event_api_create_success_response(input);
    goto send_response;

error_response:

    if (!ov_mc_mixer_core_release(app->mixer)) {
        ov_log_error("Mixer release incomplete.");
    }

input_error_response:

    out = ov_event_api_create_error_response(
        input, OV_ERROR_CODE_PROCESSING_ERROR, OV_ERROR_DESC_PROCESSING_ERROR);

send_response:

    if (!ov_event_app_send(app->app, socket, out)) {

        char *str = ov_json_value_to_string(out);
        ov_log_error("Mixer failed to send response %s", str);
        str = ov_data_pointer_free(str);
    }

error:
    ov_json_value_free(out);
    ov_json_value_free(input);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_event_forward(void *userdata, const char *name, int socket,
                             ov_json_value *input) {

    /* acquire the mixer with some user and forward data */
    // char *str = ov_json_value_to_string(input);
    // ov_log_debug("CB EVENT FORWARD -1-: %s", str);

    ov_json_value *out = NULL;

    ov_mc_mixer_app *app = ov_mc_mixer_app_cast(userdata);
    if (!app || !name || socket < 0 || !input)
        goto error;

    // ov_log_debug("CB EVENT FORWARD -2-: %s", str);
    // str = ov_data_pointer_free(str);

    const char *user = ov_mc_mixer_msg_aquire_get_username(input);
    ov_mc_mixer_core_forward forward =
        ov_mc_mixer_msg_acquire_get_forward(input);

    /* the message must contain valid data, otherwise the system is broken */

    if (!user || !ov_mc_mixer_core_forward_data_is_valid(&forward))
        goto input_error_response;

    /* release the mixer and reset to new data */

    if (!ov_mc_mixer_core_release(app->mixer))
        goto error_response;
    if (!ov_mc_mixer_core_set_name(app->mixer, user))
        goto error_response;
    if (!ov_mc_mixer_core_set_forward(app->mixer, forward))
        goto error_response;

    ov_log_debug("mixer forward changed for %s", user);

    out = ov_event_api_create_success_response(input);
    goto send_response;

error_response:

    if (!ov_mc_mixer_core_release(app->mixer)) {
        ov_log_error("Mixer release incomplete.");
    }

input_error_response:

    out = ov_event_api_create_error_response(
        input, OV_ERROR_CODE_PROCESSING_ERROR, OV_ERROR_DESC_PROCESSING_ERROR);

send_response:

    if (!ov_event_app_send(app->app, socket, out)) {

        char *str = ov_json_value_to_string(out);
        ov_log_error("Mixer failed to send response %s", str);
        str = ov_data_pointer_free(str);
    }

error:
    ov_json_value_free(out);
    ov_json_value_free(input);
    return;
}
/*----------------------------------------------------------------------------*/

static void cb_event_release(void *userdata, const char *name, int socket,
                             ov_json_value *input) {

    ov_json_value *out = NULL;

    ov_mc_mixer_app *app = ov_mc_mixer_app_cast(userdata);
    if (!app || !name || socket < 0 || !input)
        goto error;

    if (!ov_mc_mixer_core_release(app->mixer)) {

        ov_log_error("Mixer release incomplete.");

        out = ov_event_api_create_error_response(
            input, OV_ERROR_CODE_PROCESSING_ERROR,
            OV_ERROR_DESC_PROCESSING_ERROR);

    } else {

        out = ov_event_api_create_success_response(input);
    }

    if (!ov_event_app_send(app->app, socket, out)) {

        char *str = ov_json_value_to_string(out);
        ov_log_error("Mixer failed to send response %s", str);
        str = ov_data_pointer_free(str);
    }

error:
    ov_json_value_free(out);
    ov_json_value_free(input);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_event_join(void *userdata, const char *name, int socket,
                          ov_json_value *input) {

    ov_json_value *out = NULL;

    ov_mc_mixer_app *app = ov_mc_mixer_app_cast(userdata);
    if (!app || !name || socket < 0 || !input)
        goto error;

    ov_mc_loop_data data = ov_mc_mixer_msg_join_from_json(input);
    const char *username = ov_mc_mixer_core_get_name(app->mixer);
    if (!username) {
        out = ov_event_api_create_error_response(
            input, OV_ERROR_CODE_PROCESSING_ERROR,
            OV_ERROR_DESC_PROCESSING_ERROR);
        goto send_response;
    }

    /* the message must contain valid data, otherwise the system is broken */

    OV_ASSERT(0 != data.socket.host[0]);
    OV_ASSERT(0 != data.name[0]);

    if (!ov_mc_mixer_core_join(app->mixer, data)) {

        out = ov_event_api_create_error_response(
            input, OV_ERROR_CODE_PROCESSING_ERROR,
            OV_ERROR_DESC_PROCESSING_ERROR);

    } else {

        out = ov_event_api_create_success_response(input);
    }

send_response:
    if (!ov_event_app_send(app->app, socket, out)) {

        char *str = ov_json_value_to_string(out);
        ov_log_error("Mixer failed to send response %s", str);
        str = ov_data_pointer_free(str);
    }

error:
    ov_json_value_free(out);
    ov_json_value_free(input);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_event_leave(void *userdata, const char *name, int socket,
                           ov_json_value *input) {

    ov_json_value *out = NULL;

    ov_mc_mixer_app *app = ov_mc_mixer_app_cast(userdata);
    if (!app || !name || socket < 0 || !input)
        goto error;

    const char *loop = ov_mc_mixer_msg_leave_from_json(input);

    /* the message must contain valid data, otherwise the system is broken */

    OV_ASSERT(loop);

    if (!ov_mc_mixer_core_leave(app->mixer, loop)) {

        out = ov_event_api_create_error_response(
            input, OV_ERROR_CODE_PROCESSING_ERROR,
            OV_ERROR_DESC_PROCESSING_ERROR);

    } else {

        out = ov_event_api_create_success_response(input);
    }

    if (!ov_event_app_send(app->app, socket, out)) {

        char *str = ov_json_value_to_string(out);
        ov_log_error("Mixer failed to send response %s", str);
        str = ov_data_pointer_free(str);
    }

error:
    ov_json_value_free(out);
    ov_json_value_free(input);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_event_volume(void *userdata, const char *name, int socket,
                            ov_json_value *input) {

    ov_json_value *out = NULL;

    ov_mc_mixer_app *app = ov_mc_mixer_app_cast(userdata);
    if (!app || !name || socket < 0 || !input)
        goto error;

    const char *loop = ov_mc_mixer_msg_volume_get_name(input);
    uint8_t vol = ov_mc_mixer_msg_volume_get_volume(input);

    if (!ov_mc_mixer_core_set_volume(app->mixer, loop, vol)) {

        out = ov_event_api_create_error_response(
            input, OV_ERROR_CODE_PROCESSING_ERROR,
            OV_ERROR_DESC_PROCESSING_ERROR);

    } else {

        out = ov_event_api_create_success_response(input);
    }

    if (!ov_event_app_send(app->app, socket, out)) {

        char *str = ov_json_value_to_string(out);
        ov_log_error("Mixer failed to send response %s", str);
        str = ov_data_pointer_free(str);
    }

error:
    ov_json_value_free(out);
    ov_json_value_free(input);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_event_shutdown(void *userdata, const char *name, int socket,
                              ov_json_value *input) {

    ov_mc_mixer_app *app = ov_mc_mixer_app_cast(userdata);
    if (!app || !name || socket < 0 || !input)
        goto error;

    if (!ov_mc_mixer_core_release(app->mixer)) {

        ov_log_error("Failed to shutdown mixer");
    }

    ov_event_loop *loop = app->config.loop;
    loop->callback.unset(loop, app->socket, NULL);
    close(app->socket);
    app->socket = -1;

    /* no response to shutdown */

error:
    ov_json_value_free(input);
    exit(EXIT_SUCCESS);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_event_state(void *userdata, const char *name, int socket,
                           ov_json_value *input) {

    ov_json_value *out = NULL;

    ov_mc_mixer_app *app = ov_mc_mixer_app_cast(userdata);
    if (!app || !name || socket < 0 || !input)
        goto error;

    ov_json_value *state = ov_mc_mixer_state(app->mixer);
    if (!state)
        goto error;

    out = ov_event_api_create_success_response(input);
    ov_json_object_set(out, OV_KEY_RESPONSE, state);

    if (!ov_event_app_send(app->app, socket, out)) {

        char *str = ov_json_value_to_string(out);
        ov_log_error("Mixer failed to send response %s", str);
        str = ov_data_pointer_free(str);
    }

error:
    ov_json_value_free(out);
    ov_json_value_free(input);
    return;
}

/*----------------------------------------------------------------------------*/

static bool register_app_callbacks(ov_mc_mixer_app *service) {

    OV_ASSERT(service);

    if (!ov_event_app_register(service->app, OV_KEY_REGISTER, service,
                               cb_event_register))
        goto error;

    if (!ov_event_app_register(service->app, OV_KEY_CONFIGURE, service,
                               cb_event_configure))
        goto error;

    if (!ov_event_app_register(service->app, OV_KEY_ACQUIRE, service,
                               cb_event_acquire))
        goto error;

    if (!ov_event_app_register(service->app, OV_KEY_FORWARD, service,
                               cb_event_forward))
        goto error;

    if (!ov_event_app_register(service->app, OV_KEY_RELEASE, service,
                               cb_event_release))
        goto error;

    if (!ov_event_app_register(service->app, OV_KEY_JOIN, service,
                               cb_event_join))
        goto error;

    if (!ov_event_app_register(service->app, OV_KEY_LEAVE, service,
                               cb_event_leave))
        goto error;

    if (!ov_event_app_register(service->app, OV_KEY_VOLUME, service,
                               cb_event_volume))
        goto error;

    if (!ov_event_app_register(service->app, OV_KEY_SHUTDOWN, service,
                               cb_event_shutdown))
        goto error;

    if (!ov_event_app_register(service->app, OV_KEY_STATE, service,
                               cb_event_state))
        goto error;

    return true;
error:
    return false;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_mc_mixer_app *ov_mc_mixer_app_create(ov_mc_mixer_app_config config) {

    ov_mc_mixer_app *app = NULL;

    if (0 == config.loop)
        goto error;
    if (0 == config.io)
        goto error;
    if (0 == config.manager.host[0])
        goto error;

    app = calloc(1, sizeof(ov_mc_mixer_app));
    if (!app)
        goto error;

    app->magic_bytes = OV_MC_MIXER_APP_MAGIC_BYTES;
    app->config = config;

    ov_event_app_config app_config = (ov_event_app_config){

        .io = config.io,
        .callbacks.userdata = app,
        .callbacks.close = cb_close,
        .callbacks.connected = cb_connected

    };

    ov_id_fill_with_uuid(app->uuid);

    app->app = ov_event_app_create(app_config);
    if (!app->app)
        goto error;

    if (!register_app_callbacks(app))
        goto error;

    ov_io_socket_config conn_config =
        (ov_io_socket_config){.auto_reconnect = true, .socket = config.manager};

    app->socket = ov_event_app_open_connection(app->app, conn_config);

    app->mixer = ov_mc_mixer_core_create((ov_mc_mixer_core_config){
        .loop = config.loop, .manager = config.manager});
    if (!app->mixer)
        goto error;

    return app;
error:
    ov_mc_mixer_app_free(app);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_mc_mixer_app *ov_mc_mixer_app_cast(const void *data) {

    if (!data)
        return NULL;

    if (*(uint16_t *)data != OV_MC_MIXER_APP_MAGIC_BYTES)
        return NULL;

    return (ov_mc_mixer_app *)data;
}

/*----------------------------------------------------------------------------*/

ov_mc_mixer_app *ov_mc_mixer_app_free(ov_mc_mixer_app *self) {

    if (!ov_mc_mixer_app_cast(self))
        return self;

    self->app = ov_event_app_free(self->app);
    self->mixer = ov_mc_mixer_core_free(self->mixer);
    self = ov_data_pointer_free(self);

    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_mc_mixer_app_config
ov_mc_mixer_app_config_from_json(const ov_json_value *c) {

    ov_mc_mixer_app_config config = (ov_mc_mixer_app_config){0};

    const ov_json_value *conf = ov_json_object_get(c, OV_KEY_APP);
    if (!conf)
        conf = c;

    config.manager = ov_socket_configuration_from_json(
        ov_json_object_get(conf, OV_KEY_RESOURCE_MANAGER),
        (ov_socket_configuration){0});

    config.limit.client_connect_sec = ov_json_number_get(
        ov_json_get(conf, "/" OV_KEY_LIMIT "/" OV_KEY_RECONNECT_INTERVAL_SECS));

    return config;
}
