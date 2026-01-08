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
        @file           ov_mc_backend_sip.c
        @author         Markus Töpfer

        @date           2023-03-24


        ------------------------------------------------------------------------
*/
#include "../include/ov_mc_backend_sip.h"
#include "../include/ov_mc_backend_sip_registry.h"
#include "../include/ov_mc_sip_msg.h"

#include <ov_base/ov_dict.h>
#include <ov_base/ov_error_codes.h>
#include <ov_base/ov_string.h>
#include <ov_base/ov_time.h>

#include <ov_core/ov_event_api.h>
#include <ov_core/ov_event_app.h>
#include <ov_core/ov_event_async.h>

#define OV_MC_BACKEND_SIP_MAGIC_BYTES 0xd425

/*----------------------------------------------------------------------------*/

struct ov_mc_backend_sip {

    uint16_t magic_bytes;
    ov_mc_backend_sip_config config;

    struct {

        int manager;

    } socket;

    struct {

        ov_dict *mixer;

    } resources;

    ov_event_app *app;
    ov_event_async_store *async;
    ov_mc_backend_sip_registry *registry;
};

static void cb_mixer_released(void *userdata, const char *uuid,
                              const char *user_uuid, uint64_t error_code,
                              const char *error_desc);

/*----------------------------------------------------------------------------*/

static bool drop_mixer(const void *key, void *val, void *data) {

    if (!key)
        return true;
    UNUSED(val);
    ov_mc_backend_sip *self = ov_mc_backend_sip_cast(data);

    ov_id id = {0};
    ov_id_fill_with_uuid(id);

    ov_mc_backend_release_mixer(self->config.backend, id, key, self,
                                cb_mixer_released);

    return true;
}

/*----------------------------------------------------------------------------*/

static void asign_sip_mixer(ov_mc_backend_sip *self, int socket,
                            const char *name) {

    ov_dict *res = ov_dict_get(self->resources.mixer, (void *)(intptr_t)socket);

    if (!res) {

        res = ov_dict_create(ov_dict_string_key_config(255));
        ov_dict_set(self->resources.mixer, (void *)(intptr_t)socket, res, NULL);
    }

    char *key = ov_string_dup(name);
    ov_dict_set(res, key, NULL, NULL);

    return;
}

/*----------------------------------------------------------------------------*/

static void drop_sip_mixer_assignment(ov_mc_backend_sip *self, int socket,
                                      const char *name) {

    ov_dict *res = ov_dict_get(self->resources.mixer, (void *)(intptr_t)socket);

    if (res)
        ov_dict_del(res, name);

    return;
}

/*----------------------------------------------------------------------------*/

static void drop_mixer_resources(ov_mc_backend_sip *self, int socket) {

    ov_dict *res = ov_dict_get(self->resources.mixer, (void *)(intptr_t)socket);
    if (!res)
        goto done;

    ov_dict_for_each(res, self, drop_mixer);

done:
    return;
}

/*----------------------------------------------------------------------------*/

static bool cb_accept(void *userdata, int listener, int connection) {

    // accept any connection

    UNUSED(userdata);
    UNUSED(listener);
    UNUSED(connection);
    return true;
}

/*----------------------------------------------------------------------------*/

static void cb_close(void *userdata, int connection) {

    ov_mc_backend_sip *self = ov_mc_backend_sip_cast(userdata);
    UNUSED(self);

    ov_log_debug("Socket close received at %i", connection);

    drop_mixer_resources(self, connection);
    ov_mc_backend_sip_registry_unregister_proxy(self->registry, connection);

    if (self->config.callback.connected)
        self->config.callback.connected(self->config.callback.userdata, false);

    return;
}

/*----------------------------------------------------------------------------*/

static void cb_async_timedout(void *userdata, ov_event_async_data data) {

    ov_mc_backend_sip *self = ov_mc_backend_sip_cast(userdata);
    if (!self)
        goto error;

    char *str = ov_json_value_to_string(data.value);
    ov_log_error("async timeout %i | %s", data.socket, str);
    str = ov_data_pointer_free(str);

    ov_json_value *out = ov_event_api_create_error_response(
        data.value, OV_ERROR_TIMEOUT, OV_ERROR_DESC_TIMEOUT);
    ov_event_app_send(self->app, data.socket, out);
    out = ov_json_value_free(out);

error:
    ov_event_async_data_clear(&data);
    return;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      APP FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

static void cb_event_get_multicast(void *userdata, const char *name, int socket,
                                   ov_json_value *input) {

    ov_json_value *out = NULL;

    ov_mc_backend_sip *self = ov_mc_backend_sip_cast(userdata);
    if (!self || !name || socket < 0 || !input)
        goto error;

    const char *loop = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_LOOP));

    if (!loop) {

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_PARAMETER_ERROR,
                                                 OV_ERROR_DESC_PARAMETER_ERROR);

        goto response;
    }

    ov_socket_configuration config =
        ov_vocs_db_get_multicast_group(self->config.db, loop);

    if (0 == config.host[0]) {

        out = ov_event_api_create_error_response(
            input, OV_ERROR_CODE_UNKNOWN_LOOP, OV_ERROR_DESC_UNKNOWN_LOOP);

        goto response;
    }

    out = ov_mc_sip_msg_get_multicast_response(input, config);

    if (0 == out) {

        out = ov_event_api_create_error_response(
            input, OV_ERROR_INTERNAL_SERVER, OV_ERROR_DESC_INTERNAL_SERVER);
        goto error;
    }

response:

    ov_event_app_send(self->app, socket, out);

error:
    out = ov_json_value_free(out);
    input = ov_json_value_free(input);
    return;
}

/*----------------------------------------------------------------------------*/

struct container_permission {

    ov_mc_backend_sip *backend;
    int socket;
    const char *loop;
};

/*----------------------------------------------------------------------------*/

static bool send_permission(void *item, void *data) {

    ov_json_value *out = NULL;

    if (!item || !data)
        return false;

    struct container_permission *container =
        (struct container_permission *)data;

    ov_sip_permission permission = {0};

    permission.loop = container->loop;
    permission.caller =
        ov_json_string_get(ov_json_get(item, "/" OV_KEY_CALLER));
    permission.callee =
        ov_json_string_get(ov_json_get(item, "/" OV_KEY_CALLEE));

    const char *time_from_string =
        ov_json_string_get(ov_json_get(item, "/" OV_KEY_VALID_FROM));
    const char *time_to_string =
        ov_json_string_get(ov_json_get(item, "/" OV_KEY_VALID_UNTIL));

    if (time_from_string) {
        ov_time time_from = ov_timestamp_from_string(time_from_string);
        permission.from_epoch = ov_time_to_epoch(time_from);
    }

    if (time_to_string) {
        ov_time time_to = ov_timestamp_from_string(time_to_string);
        permission.until_epoch = ov_time_to_epoch(time_to);
    }

    out = ov_mc_sip_msg_permit(permission);
    ov_event_app_send(container->backend->app, container->socket, out);
    out = ov_json_value_free(out);

    return true;
}

/*----------------------------------------------------------------------------*/

static bool configure_permission(void *item, void *data) {

    if (!item || !data)
        goto error;

    struct container_permission *container =
        (struct container_permission *)data;
    container->loop = ov_json_string_get(ov_json_get(item, "/" OV_KEY_LOOP));

    ov_json_value *whitelist =
        (ov_json_value *)ov_json_get(item, "/" OV_KEY_WHITELIST);
    return ov_json_array_for_each(whitelist, container, send_permission);

error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool configure_sip_gateway(ov_mc_backend_sip *self, int socket) {

    ov_json_value *config = NULL;

    if (!self || !socket)
        goto error;

    config = ov_vocs_db_get_sip(self->config.db);
    if (!config)
        goto error;

    ov_json_value_dump(stdout, config);

    struct container_permission container =
        (struct container_permission){.backend = self, .socket = socket};

    if (!ov_json_array_for_each(config, &container, configure_permission))
        goto error;

    ov_json_value_free(config);
    return true;
error:
    ov_json_value_free(config);
    return false;
}

/*----------------------------------------------------------------------------*/

static void cb_event_register(void *userdata, const char *name, int socket,
                              ov_json_value *input) {

    ov_mc_backend_sip *self = ov_mc_backend_sip_cast(userdata);
    if (!self || !name || socket < 0 || !input)
        goto error;

    const char *uuid = ov_json_string_get(ov_json_get(input, "/" OV_KEY_UUID));

    if (!ov_mc_backend_sip_registry_register_proxy(self->registry, socket,
                                                   uuid)) {

        ov_log_error("Failed to register SIP gateway");
        goto error;
    }

    ov_log_debug("registered sip gateway at socket %i", socket);

    configure_sip_gateway(self, socket);

    if (self->config.callback.connected)
        self->config.callback.connected(self->config.callback.userdata, true);

error:
    ov_json_value_free(input);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_mixer_released(void *userdata, const char *uuid,
                              const char *user_uuid, uint64_t error_code,
                              const char *error_desc) {

    ov_json_value *out = NULL;
    ov_json_value *par = NULL;
    ov_json_value *val = NULL;
    ov_event_async_data adata = {0};

    ov_mc_backend_sip *self = ov_mc_backend_sip_cast(userdata);
    if (!self)
        goto error;

    adata = ov_event_async_unset(self->async, uuid);

    // no async call set, ignore released message
    if (!adata.value)
        goto error;

    switch (error_code) {

    case OV_ERROR_NO_ERROR:

        out = ov_event_api_create_success_response(adata.value);

        par = ov_event_api_get_response(out);
        val = ov_json_string(user_uuid);
        if (!ov_json_object_set(par, OV_KEY_USER, val))
            goto error;
        val = NULL;

        break;

    default:

        out = ov_event_api_create_error_response(adata.value, error_code,
                                                 error_desc);

        par = ov_event_api_get_response(out);
        val = ov_json_string(user_uuid);
        if (!ov_json_object_set(par, OV_KEY_USER, val))
            goto error;
        val = NULL;
    }

    ov_event_app_send(self->app, adata.socket, out);

error:
    ov_event_async_data_clear(&adata);
    ov_json_value_free(out);
    ov_json_value_free(val);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_mixer_aquired(void *userdata, const char *uuid,
                             const char *user_uuid, uint64_t error_code,
                             const char *error_desc) {

    ov_json_value *out = NULL;
    ov_json_value *par = NULL;
    ov_json_value *val = NULL;
    ov_event_async_data adata = {0};

    ov_mc_backend_sip *self = ov_mc_backend_sip_cast(userdata);
    if (!self)
        goto error;

    adata = ov_event_async_unset(self->async, uuid);

    // reverse timeout acquire
    if (!adata.value) {

        if (OV_ERROR_NO_ERROR == error_code) {

            ov_mc_backend_release_mixer(self->config.backend, uuid, user_uuid,
                                        self, cb_mixer_released);

        } else {

            // ignore timeout and error in acquisition
            goto error;
        }
    }

    switch (error_code) {

    case OV_ERROR_NO_ERROR:

        out = ov_event_api_create_success_response(adata.value);

        par = ov_event_api_get_response(out);
        val = ov_json_string(user_uuid);
        if (!ov_json_object_set(par, OV_KEY_USER, val))
            goto error;
        val = NULL;

        break;

    default:

        out = ov_event_api_create_error_response(adata.value, error_code,
                                                 error_desc);

        par = ov_event_api_get_response(out);
        val = ov_json_string(user_uuid);
        if (!ov_json_object_set(par, OV_KEY_USER, val))
            goto error;
        val = NULL;
    }

    ov_event_app_send(self->app, adata.socket, out);

error:
    ov_event_async_data_clear(&adata);
    ov_json_value_free(out);
    ov_json_value_free(val);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_event_acquire(void *userdata, const char *name, int socket,
                             ov_json_value *input) {

    ov_json_value *out = NULL;

    ov_mc_backend_sip *self = ov_mc_backend_sip_cast(userdata);
    if (!self || !name || socket < 0 || !input)
        goto error;

    ov_mc_mixer_core_forward forward = ov_mc_sip_msg_get_loop_socket(input);

    const char *uuid = ov_event_api_get_uuid(input);
    const char *user = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_USER));

    if (!uuid || !user)
        goto error;

    // set some default forward if forward is not set
    if (0 == forward.socket.host[0]) {

        forward = (ov_mc_mixer_core_forward){.socket.host = "127.0.0.1",
                                             .socket.port = 12345,
                                             .socket.type = UDP,
                                             .payload_type = 100,
                                             .ssrc = 12345

        };
    }

    if (!ov_mc_backend_acquire_mixer(self->config.backend, uuid, user, forward,
                                     self, cb_mixer_aquired)) {

        out = ov_event_api_create_error_response(
            input, OV_ERROR_CODE_PROCESSING_ERROR,
            OV_ERROR_DESC_PROCESSING_ERROR);

        ov_event_app_send(self->app, socket, out);
        out = ov_json_value_free(out);
        goto error;
    }

    asign_sip_mixer(self, socket, user);

    if (!ov_event_async_set(
            self->async, uuid,
            (ov_event_async_data){.socket = socket,
                                  .value = input,
                                  .timedout.userdata = self,
                                  .timedout.callback = cb_async_timedout},
            self->config.timeout.response_usec))
        goto error;

    input = NULL;
error:
    ov_json_value_free(input);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_event_release(void *userdata, const char *name, int socket,
                             ov_json_value *input) {

    ov_json_value *out = NULL;

    ov_mc_backend_sip *self = ov_mc_backend_sip_cast(userdata);
    if (!self || !name || socket < 0 || !input)
        goto error;

    const char *uuid = ov_event_api_get_uuid(input);
    const char *user = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_USER));

    if (!uuid || !user)
        goto error;

    if (!ov_mc_backend_release_mixer(self->config.backend, uuid, user, self,
                                     cb_mixer_released)) {

        out = ov_event_api_create_error_response(
            input, OV_ERROR_CODE_PROCESSING_ERROR,
            OV_ERROR_DESC_PROCESSING_ERROR);

        ov_event_app_send(self->app, socket, out);
        out = ov_json_value_free(out);
        goto error;
    }

    drop_sip_mixer_assignment(self, socket, user);

    if (!ov_event_async_set(
            self->async, uuid,
            (ov_event_async_data){.socket = socket,
                                  .value = input,
                                  .timedout.userdata = self,
                                  .timedout.callback = cb_async_timedout},
            self->config.timeout.response_usec))
        goto error;

    input = NULL;

error:
    ov_json_value_free(input);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_mixer_forwarded(void *userdata, const char *uuid,
                               const char *user_uuid, uint64_t error_code,
                               const char *error_desc) {

    ov_json_value *out = NULL;
    ov_json_value *par = NULL;
    ov_json_value *val = NULL;
    ov_event_async_data adata = {0};

    ov_mc_backend_sip *self = ov_mc_backend_sip_cast(userdata);
    if (!self)
        goto error;

    adata = ov_event_async_unset(self->async, uuid);

    // reverse timeout acquire
    if (!adata.value) {

        if (OV_ERROR_NO_ERROR == error_code) {

            ov_mc_backend_release_mixer(self->config.backend, uuid, user_uuid,
                                        self, cb_mixer_released);

        } else {

            // ignore timeout and error in acquisition
            goto error;
        }
    }

    switch (error_code) {

    case OV_ERROR_NO_ERROR:

        out = ov_event_api_create_success_response(adata.value);

        par = ov_event_api_get_response(out);
        val = ov_json_string(user_uuid);
        if (!ov_json_object_set(par, OV_KEY_USER, val))
            goto error;
        val = NULL;

        break;

    default:

        out = ov_event_api_create_error_response(adata.value, error_code,
                                                 error_desc);

        par = ov_event_api_get_response(out);
        val = ov_json_string(user_uuid);
        if (!ov_json_object_set(par, OV_KEY_USER, val))
            goto error;
        val = NULL;
    }

    ov_event_app_send(self->app, adata.socket, out);

error:
    ov_event_async_data_clear(&adata);
    ov_json_value_free(out);
    ov_json_value_free(val);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_loop_joined(void *userdata, const char *uuid,
                           const char *user_uuid, const char *loop_name,
                           uint64_t error_code, const char *error_desc) {

    ov_mc_backend_sip *self = ov_mc_backend_sip_cast(userdata);

    ov_log_debug("%s joined %s - err %" PRIu64 " %s", user_uuid, loop_name,
                 error_code, error_desc);

    UNUSED(uuid);
    UNUSED(self);

    return;
}

/*----------------------------------------------------------------------------*/

static void cb_event_set_singlecast(void *userdata, const char *name,
                                    int socket, ov_json_value *input) {

    ov_json_value *out = NULL;

    ov_mc_backend_sip *self = ov_mc_backend_sip_cast(userdata);
    if (!self || !name || socket < 0 || !input)
        goto error;

    const char *uuid = ov_event_api_get_uuid(input);
    const char *user = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_USER));
    const char *loop = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_LOOP));

    if (!uuid || !user || !loop)
        goto error;

    ov_socket_configuration socket_config =
        ov_vocs_db_get_multicast_group(self->config.db, loop);

    if (0 == socket_config.host[0]) {
        ov_log_error("Could not get multicast group");
        goto error;
    }

    ov_mc_mixer_core_forward forward = ov_mc_sip_msg_get_loop_socket(input);

    ov_mc_loop_data data = {0};
    data.socket = socket_config;
    strncpy(data.name, loop, OV_MC_LOOP_NAME_MAX);
    data.volume = 50;

    if (!ov_mc_mixer_core_forward_data_is_valid(&forward)) {

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_PARAMETER_ERROR,
                                                 OV_ERROR_DESC_PARAMETER_ERROR);

        ov_event_app_send(self->app, socket, out);
        out = ov_json_value_free(out);
        goto error;
    }

    if (!ov_mc_backend_set_mixer_forward(self->config.backend, uuid, user,
                                         forward, self, cb_mixer_forwarded)) {

        out = ov_event_api_create_error_response(
            input, OV_ERROR_CODE_PROCESSING_ERROR,
            OV_ERROR_DESC_PROCESSING_ERROR);

        ov_event_app_send(self->app, socket, out);
        out = ov_json_value_free(out);
        goto error;
    }

    ov_id uuid_new = {0};
    ov_id_fill_with_uuid(uuid_new);

    if (!ov_mc_backend_join_loop(self->config.backend, uuid_new, user, data,
                                 self, cb_loop_joined)) {

        ov_log_error("Failed to send join loop %s for %s", data.name, user);
    }

    if (!ov_event_async_set(
            self->async, uuid,
            (ov_event_async_data){.socket = socket,
                                  .value = input,
                                  .timedout.userdata = self,
                                  .timedout.callback = cb_async_timedout},
            self->config.timeout.response_usec))
        goto error;

    input = NULL;

error:
    ov_json_value_free(input);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_event_notify(void *userdata, const char *name, int socket,
                            ov_json_value *input) {

    /*

        {
            "parameter": {
                "loop": "LOOP_NAME",
                "id": "3212@SpK1XxthgPFYMTACUoF",
                "peer": "3212",
                "type": "new_call"
            },
            "uuid": "40ac2020-6b69-4ee9-9f61-db0b6020a57c",
            "event": "notify"
        }

        {
            "parameter": {
                "id": "3212@SpK1XxthgPFYMTACUoF",
                "type": "call_terminated"
            },
            "uuid": "e912c815-bcb4-429d-b784-542a6329c743",
            "event": "notify"
        }
    */

    ov_mc_backend_sip *self = ov_mc_backend_sip_cast(userdata);
    if (!self || !name || socket < 0 || !input)
        goto error;

    const char *type = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_TYPE));

    const char *loop = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_LOOP));

    const char *id = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_ID));

    const char *peer = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_PEER));

    if (!type || !id)
        goto error;

    if (0 == ov_string_compare(type, "new_call")) {

        if (!loop || !peer)
            goto error;

        if (!ov_mc_backend_sip_registry_register_call(self->registry, socket,
                                                      id, loop, peer))
            goto error;

        if (self->config.callback.call.new)
            self->config.callback.call.new(self->config.callback.userdata, loop,
                                           id, peer);

    } else if (0 == ov_string_compare(type, "call_terminated")) {

        char *loop = ov_string_dup(
            ov_mc_backend_sip_registry_get_call_loop(self->registry, id));

        if (!ov_mc_backend_sip_registry_unregister_call(self->registry, id))
            goto error;

        if (self->config.callback.call.terminated)
            self->config.callback.call.terminated(
                self->config.callback.userdata, id, loop);

        loop = ov_data_pointer_free(loop);

    } else {

        // ignore unknown
        ov_log_error("Unregistered notify %s", type);
        goto error;
    }

error:
    ov_json_value_free(input);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_event_call_response(ov_mc_backend_sip *self, int socket,
                                   ov_json_value *input) {

    if (!self || !input || socket < 0)
        goto error;

    /*
    {
        "uuid":"4b71bff8-d200-11ed-b638-0456e524616b",
        "request":{
            "parameter":{
                "loop":"loop17",
                "caller":"InternalUserToMap",
                "callee":"4132"
            },
            "uuid":"4b71bff8-d200-11ed-b638-0456e524616b",
            "protocol":1,
            "event":"call",
            "type":"unicast"},
        "response":{
            "id": "*43@6cAyaMQp7HfOxhnO6XA"
        },
        "event":"call"
    }
    */

    uint64_t code = 0;
    const char *desc = NULL;

    const char *call_id = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_RESPONSE "/" OV_KEY_ID));
    const char *uuid = ov_event_api_get_uuid(input);

    const char *caller = ov_json_string_get(ov_json_get(
        input, "/" OV_KEY_REQUEST "/" OV_KEY_PARAMETER "/" OV_KEY_CALLER));
    const char *callee = ov_json_string_get(ov_json_get(
        input, "/" OV_KEY_REQUEST "/" OV_KEY_PARAMETER "/" OV_KEY_CALLEE));
    const char *loop = ov_json_string_get(ov_json_get(
        input, "/" OV_KEY_REQUEST "/" OV_KEY_PARAMETER "/" OV_KEY_LOOP));

    ov_event_api_get_error_parameter(input, &code, &desc);

    if (0 == code) {

        ov_mc_backend_sip_registry_register_call(self->registry, socket,
                                                 call_id, loop, callee);
    }

    if (self->config.callback.call.init)
        self->config.callback.call.init(self->config.callback.userdata, uuid,
                                        loop, call_id, caller, callee, code,
                                        desc);

error:
    ov_json_value_free(input);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_event_call(void *userdata, const char *name, int socket,
                          ov_json_value *input) {

    ov_mc_backend_sip *self = ov_mc_backend_sip_cast(userdata);
    if (!self || !name || socket < 0 || !input)
        goto error;

    if (ov_event_api_get_response(input)) {
        cb_event_call_response(self, socket, input);
        return;
    }

    // we do not expect a non response here

error:
    ov_json_value_free(input);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_event_permit_response(ov_mc_backend_sip *self, int socket,
                                     ov_json_value *input) {

    if (!self || !input || socket < 0)
        goto error;

    uint64_t error_code = 0;
    const char *error_desc = NULL;

    bool ok = true;

    ov_event_api_get_error_parameter(input, &error_code, &error_desc);

    ov_json_value *req = ov_event_api_get_request(input);
    ov_json_value *par = ov_event_api_get_parameter(req);

    ov_sip_permission permission = ov_sip_permission_from_json(par, &ok);

    if (self->config.callback.call.permit)
        self->config.callback.call.permit(self->config.callback.userdata,
                                          permission, error_code, error_desc);

error:
    ov_json_value_free(input);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_event_permit(void *userdata, const char *name, int socket,
                            ov_json_value *input) {

    ov_mc_backend_sip *self = ov_mc_backend_sip_cast(userdata);
    if (!self || !name || socket < 0 || !input)
        goto error;

    if (ov_event_api_get_response(input)) {
        cb_event_permit_response(self, socket, input);
        return;
    }

    // we do not expect a non response here

error:
    ov_json_value_free(input);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_event_revoke_response(ov_mc_backend_sip *self, int socket,
                                     ov_json_value *input) {

    if (!self || !input || socket < 0)
        goto error;

    uint64_t error_code = 0;
    const char *error_desc = NULL;

    bool ok = true;

    ov_event_api_get_error_parameter(input, &error_code, &error_desc);

    ov_json_value *req = ov_event_api_get_request(input);
    ov_json_value *par = ov_event_api_get_parameter(req);

    ov_sip_permission permission = ov_sip_permission_from_json(par, &ok);

    if (self->config.callback.call.revoke)
        self->config.callback.call.revoke(self->config.callback.userdata,
                                          permission, error_code, error_desc);

error:
    ov_json_value_free(input);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_event_revoke(void *userdata, const char *name, int socket,
                            ov_json_value *input) {

    ov_mc_backend_sip *self = ov_mc_backend_sip_cast(userdata);
    if (!self || !name || socket < 0 || !input)
        goto error;

    if (ov_event_api_get_response(input)) {
        cb_event_revoke_response(self, socket, input);
        return;
    }

    // we do not expect a non response here

error:
    ov_json_value_free(input);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_event_hangup_response(ov_mc_backend_sip *self, int socket,
                                     ov_json_value *input) {

    if (!self || !input || socket < 0)
        goto error;

    ov_json_value *req = ov_event_api_get_request(input);
    ov_json_value *par = ov_event_api_get_parameter(req);
    const char *call_id = ov_json_string_get(ov_json_get(par, "/" OV_KEY_CALL));
    const char *loopname =
        ov_mc_backend_sip_registry_get_call_loop(self->registry, call_id);

    if (self->config.callback.call.terminated)
        self->config.callback.call.terminated(self->config.callback.userdata,
                                              call_id, loopname);

error:
    ov_json_value_free(input);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_event_hangup(void *userdata, const char *name, int socket,
                            ov_json_value *input) {

    ov_mc_backend_sip *self = ov_mc_backend_sip_cast(userdata);
    if (!self || !name || socket < 0 || !input)
        goto error;

    if (ov_event_api_get_response(input)) {
        cb_event_hangup_response(self, socket, input);
        return;
    }

    // we do not expect a non response here

error:
    ov_json_value_free(input);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_event_list_calls_response(ov_mc_backend_sip *self, int socket,
                                         ov_json_value *input) {

    if (!self || !input || socket < 0)
        goto error;

    uint64_t error_code = 0;
    const char *error_desc = NULL;

    ov_event_api_get_error_parameter(input, &error_code, &error_desc);

    const char *uuid = ov_event_api_get_uuid(input);
    const ov_json_value *calls =
        ov_json_get(input, "/" OV_KEY_RESPONSE "/" OV_KEY_CALLS);

    if (self->config.callback.list_calls)
        self->config.callback.list_calls(self->config.callback.userdata, uuid,
                                         calls, error_code, error_desc);

error:
    ov_json_value_free(input);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_event_list_calls(void *userdata, const char *name, int socket,
                                ov_json_value *input) {

    ov_mc_backend_sip *self = ov_mc_backend_sip_cast(userdata);
    if (!self || !name || socket < 0 || !input)
        goto error;

    if (ov_event_api_get_response(input)) {
        cb_event_list_calls_response(self, socket, input);
        return;
    }

    // we do not expect a non response here

error:
    ov_json_value_free(input);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_event_list_permissions_response(ov_mc_backend_sip *self,
                                               int socket,
                                               ov_json_value *input) {

    if (!self || !input || socket < 0)
        goto error;

    uint64_t error_code = 0;
    const char *error_desc = NULL;

    ov_event_api_get_error_parameter(input, &error_code, &error_desc);

    const char *uuid = ov_event_api_get_uuid(input);
    const ov_json_value *perm =
        ov_json_get(input, "/" OV_KEY_RESPONSE "/" OV_KEY_PERMISSIONS);

    if (self->config.callback.list_permissions)
        self->config.callback.list_permissions(
            self->config.callback.userdata, uuid, perm, error_code, error_desc);

error:
    ov_json_value_free(input);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_event_list_permissions(void *userdata, const char *name,
                                      int socket, ov_json_value *input) {

    ov_mc_backend_sip *self = ov_mc_backend_sip_cast(userdata);
    if (!self || !name || socket < 0 || !input)
        goto error;

    if (ov_event_api_get_response(input)) {
        cb_event_list_permissions_response(self, socket, input);
        return;
    }

    // we do not expect a non response here

error:
    ov_json_value_free(input);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_event_get_status_response(ov_mc_backend_sip *self, int socket,
                                         ov_json_value *input) {

    if (!self || !input || socket < 0)
        goto error;

    uint64_t error_code = 0;
    const char *error_desc = NULL;

    ov_event_api_get_error_parameter(input, &error_code, &error_desc);

    const char *uuid = ov_event_api_get_uuid(input);
    const ov_json_value *status =
        ov_json_get(input, "/" OV_KEY_RESPONSE "/" OV_KEY_STATUS);

    if (self->config.callback.get_status)
        self->config.callback.get_status(self->config.callback.userdata, uuid,
                                         status, error_code, error_desc);

error:
    ov_json_value_free(input);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_event_get_status(void *userdata, const char *name, int socket,
                                ov_json_value *input) {

    ov_mc_backend_sip *self = ov_mc_backend_sip_cast(userdata);
    if (!self || !name || socket < 0 || !input)
        goto error;

    if (ov_event_api_get_response(input)) {
        cb_event_get_status_response(self, socket, input);
        return;
    }

    // we do not expect a non response here

error:
    ov_json_value_free(input);
    return;
}

/*----------------------------------------------------------------------------*/

static bool register_app_callbacks(ov_mc_backend_sip *self) {

    if (!ov_event_app_register(self->app, OV_KEY_REGISTER, self,
                               cb_event_register))
        goto error;

    if (!ov_event_app_register(self->app, OV_SIP_EVENT_GET_MULTICAST, self,
                               cb_event_get_multicast))
        goto error;

    if (!ov_event_app_register(self->app, OV_SIP_EVENT_SET_SINGLECAST, self,
                               cb_event_set_singlecast))
        goto error;

    if (!ov_event_app_register(self->app, OV_KEY_ACQUIRE, self,
                               cb_event_acquire))
        goto error;

    if (!ov_event_app_register(self->app, OV_KEY_RELEASE, self,
                               cb_event_release))
        goto error;

    if (!ov_event_app_register(self->app, OV_KEY_NOTIFY, self, cb_event_notify))
        goto error;

    if (!ov_event_app_register(self->app, OV_KEY_CALL, self, cb_event_call))
        goto error;

    if (!ov_event_app_register(self->app, OV_KEY_PERMIT, self, cb_event_permit))
        goto error;

    if (!ov_event_app_register(self->app, OV_KEY_REVOKE, self, cb_event_revoke))
        goto error;

    if (!ov_event_app_register(self->app, OV_KEY_HANGUP, self, cb_event_hangup))
        goto error;

    if (!ov_event_app_register(self->app, OV_KEY_LIST_CALLS, self,
                               cb_event_list_calls))
        goto error;

    if (!ov_event_app_register(self->app, OV_KEY_LIST_PERMISSIONS, self,
                               cb_event_list_permissions))
        goto error;

    if (!ov_event_app_register(self->app, OV_KEY_GET_STATUS, self,
                               cb_event_get_status))
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

ov_mc_backend_sip *ov_mc_backend_sip_create(ov_mc_backend_sip_config config) {

    ov_mc_backend_sip *self = NULL;

    if (!config.loop)
        goto error;
    if (!config.db)
        goto error;
    if (!config.backend)
        goto error;

    if (0 == config.timeout.response_usec)
        config.timeout.response_usec = OV_MC_BACKEND_SIP_DEFAULT_TIMEOUT;

    self = calloc(1, sizeof(ov_mc_backend_sip));
    if (!self)
        goto error;

    self->magic_bytes = OV_MC_BACKEND_SIP_MAGIC_BYTES;
    self->config = config;

    ov_event_app_config app_config = (ov_event_app_config){

        .io = config.io,
        .callbacks.userdata = self,
        .callbacks.accept = cb_accept,
        .callbacks.close = cb_close};

    self->app = ov_event_app_create(app_config);
    if (!self->app)
        goto error;

    self->socket.manager = ov_event_app_open_listener(
        self->app,
        (ov_io_socket_config){.socket = self->config.socket.manager});

    if (-1 == self->socket.manager) {

        ov_log_error("Failed to open manager socket %s:%i",
                     self->config.socket.manager.host,
                     self->config.socket.manager.port);

        goto error;
    }

    if (!register_app_callbacks(self))
        goto error;

    self->async = ov_event_async_store_create(
        (ov_event_async_store_config){.loop = config.loop});

    if (!self->async)
        goto error;

    self->registry = ov_mc_backend_sip_registry_create(
        (ov_mc_backend_sip_registry_config){0});

    if (!self->registry)
        goto error;

    ov_dict_config d_config = ov_dict_intptr_key_config(255);
    d_config.value.data_function.free = ov_dict_free;

    self->resources.mixer = ov_dict_create(d_config);
    if (!self->resources.mixer)
        goto error;

    return self;
error:
    ov_mc_backend_sip_free(self);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_mc_backend_sip *ov_mc_backend_sip_free(ov_mc_backend_sip *self) {

    if (!ov_mc_backend_sip_cast(self))
        return self;

    self->app = ov_event_app_free(self->app);
    self->async = ov_event_async_store_free(self->async);
    self->registry = ov_mc_backend_sip_registry_free(self->registry);
    self->resources.mixer = ov_dict_free(self->resources.mixer);
    self = ov_data_pointer_free(self);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_mc_backend_sip *ov_mc_backend_sip_cast(const void *data) {

    if (!data)
        return NULL;

    if (*(uint16_t *)data != OV_MC_BACKEND_SIP_MAGIC_BYTES)
        return NULL;

    return (ov_mc_backend_sip *)data;
}

/*----------------------------------------------------------------------------*/

ov_mc_backend_sip_config
ov_mc_backend_sip_config_from_json(const ov_json_value *val) {

    ov_mc_backend_sip_config config = (ov_mc_backend_sip_config){0};

    const ov_json_value *data = ov_json_object_get(val, OV_KEY_SIP);
    if (!data)
        data = val;

    config.socket.manager = ov_socket_configuration_from_json(
        ov_json_get(data, "/" OV_KEY_SOCKET "/" OV_KEY_MANAGER),
        (ov_socket_configuration){0});

    config.timeout.response_usec = ov_json_number_get(
        ov_json_get(data, "/" OV_KEY_TIMEOUT "/" OV_KEY_RESPONSE_TIMEOUT));

    return config;
}

/*----------------------------------------------------------------------------*/

char *ov_mc_backend_sip_create_call(ov_mc_backend_sip *self, const char *loop,
                                    const char *destination_number,
                                    const char *from_number) {

    ov_json_value *out = NULL;
    char *uuid = NULL;

    if (!self || !loop || !destination_number)
        goto error;

    int gw = ov_mc_backend_sip_registry_get_proxy_socket(self->registry);
    if (0 >= gw)
        goto error;

    out = ov_mc_sip_msg_create_call(loop, destination_number, from_number);
    if (!out)
        goto error;

    uuid = ov_string_dup(ov_event_api_get_uuid(out));

    if (!ov_event_app_send(self->app, gw, out))
        goto error;

    out = ov_json_value_free(out);
    return uuid;

error:
    out = ov_json_value_free(out);
    return NULL;
}

/*----------------------------------------------------------------------------*/

bool ov_mc_backend_sip_terminate_call(ov_mc_backend_sip *self,
                                      const char *call_id) {

    ov_json_value *out = NULL;

    if (!self || !call_id)
        goto error;

    int gw = ov_mc_backend_sip_registry_get_call_proxy(self->registry, call_id);
    if (0 >= gw)
        goto error;

    out = ov_mc_sip_msg_terminate_call(call_id);
    if (!out)
        goto error;

    if (!ov_event_app_send(self->app, gw, out))
        goto error;

    out = ov_json_value_free(out);
    return true;

error:
    out = ov_json_value_free(out);
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_mc_backend_sip_create_permission(ov_mc_backend_sip *self,
                                         ov_sip_permission permission) {

    ov_json_value *out = NULL;

    if (!self)
        goto error;

    int gw = ov_mc_backend_sip_registry_get_proxy_socket(self->registry);
    if (0 >= gw)
        goto error;

    out = ov_mc_sip_msg_permit(permission);
    if (!out)
        goto error;

    if (!ov_event_app_send(self->app, gw, out))
        goto error;

    out = ov_json_value_free(out);
    return true;

error:
    out = ov_json_value_free(out);
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_mc_backend_sip_terminate_permission(ov_mc_backend_sip *self,
                                            ov_sip_permission permission) {

    ov_json_value *out = NULL;

    if (!self)
        goto error;

    int gw = ov_mc_backend_sip_registry_get_proxy_socket(self->registry);
    if (0 >= gw)
        goto error;

    out = ov_mc_sip_msg_revoke(permission);
    if (!out)
        goto error;

    if (!ov_event_app_send(self->app, gw, out))
        goto error;

    out = ov_json_value_free(out);
    return true;

error:
    out = ov_json_value_free(out);
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_mc_backend_sip_list_calls(ov_mc_backend_sip *self, const char *uuid) {

    ov_json_value *out = NULL;

    if (!self)
        goto error;

    int gw = ov_mc_backend_sip_registry_get_proxy_socket(self->registry);
    if (0 >= gw)
        goto error;

    out = ov_mc_sip_msg_list_calls(uuid);
    if (!out)
        goto error;

    if (!ov_event_app_send(self->app, gw, out))
        goto error;

    out = ov_json_value_free(out);
    return true;

error:
    out = ov_json_value_free(out);
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_mc_backend_sip_list_permissions(ov_mc_backend_sip *self,
                                        const char *uuid) {

    ov_json_value *out = NULL;

    if (!self)
        goto error;

    int gw = ov_mc_backend_sip_registry_get_proxy_socket(self->registry);
    if (0 >= gw)
        goto error;

    out = ov_mc_sip_msg_list_permissions(uuid);
    if (!out)
        goto error;

    if (!ov_event_app_send(self->app, gw, out))
        goto error;

    out = ov_json_value_free(out);
    return true;

error:
    out = ov_json_value_free(out);
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_mc_backend_sip_get_status(ov_mc_backend_sip *self, const char *uuid) {

    ov_json_value *out = NULL;

    if (!self)
        goto error;

    int gw = ov_mc_backend_sip_registry_get_proxy_socket(self->registry);
    if (0 >= gw)
        goto error;

    out = ov_mc_sip_msg_get_status(uuid);
    if (!out)
        goto error;

    if (!ov_event_app_send(self->app, gw, out))
        goto error;

    out = ov_json_value_free(out);
    return true;

error:
    out = ov_json_value_free(out);
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_mc_backend_sip_get_connect_status(ov_mc_backend_sip *self) {

    if (!self)
        goto error;

    int gw = ov_mc_backend_sip_registry_get_proxy_socket(self->registry);

    if (gw > 0)
        return true;

error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_mc_backend_sip_configure(ov_mc_backend_sip *self) {

    int socket = ov_mc_backend_sip_registry_get_proxy_socket(self->registry);

    if (-1 != socket)
        configure_sip_gateway(self, socket);

    return true;
}