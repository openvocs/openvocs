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
        @file           ov_mc_frontend.c
        @author         Markus Töpfer

        @date           2023-01-22


        ------------------------------------------------------------------------
*/
#include "../include/ov_mc_frontend.h"

#define OV_MC_FRONTEND_MAGIC_BYTES 0xabde

#include <ov_base/ov_config_keys.h>
#include <ov_base/ov_error_codes.h>
#include <ov_base/ov_linked_list.h>
#include <ov_base/ov_thread_lock.h>
#include <ov_core/ov_event_api.h>
#include <ov_core/ov_event_app.h>

#include <ov_ice/ov_ice_candidate.h>
#include <ov_ice_proxy/ov_ice_proxy_vocs_msg.h>

struct ov_mc_frontend {

    uint16_t magic_bytes;
    ov_mc_frontend_config config;

    struct {

        int manager;

    } socket;

    ov_event_app *app;

    struct {

        ov_thread_lock lock;
        ov_dict *data;

    } session;

    struct {

        ov_thread_lock lock;
        ov_dict *data;

    } proxy;
};

typedef struct Proxy {

    ov_id id;
    uint64_t load;

} Proxy;

/*
 *      ------------------------------------------------------------------------
 *
 *      CALLBACK FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */


/*----------------------------------------------------------------------------*/

static void cb_registry_session_drop(void *userdata, const char *session) {

    ov_mc_frontend *self = ov_mc_frontend_cast(userdata);
    if (!self || !session)
        goto error;

    // forward session drop to controlling application

    if (self->config.callback.session.dropped)
        self->config.callback.session.dropped(self->config.callback.userdata,
                                              session);

error:
    return;
}

/*----------------------------------------------------------------------------*/

struct container {

    int socket;
    ov_mc_frontend *self;
    ov_list *list;
};

/*----------------------------------------------------------------------------*/

static bool search_session_of_connection(const void *key, void *val,
                                         void *data) {

    if (!key)
        return true;

    struct container *container = (struct container *)data;
    if (container->socket == (intptr_t)val) {
        ov_list_push(container->list, (void *)key);
    }

    return true;
}

/*----------------------------------------------------------------------------*/

static bool drop_sessions(void *item, void *data) {

    ov_mc_frontend *self = ov_mc_frontend_cast(data);
    cb_registry_session_drop(self, (const char *)item);
    ov_dict_del(self->session.data, item);
    return true;
}

/*----------------------------------------------------------------------------*/

static void cb_close(void *userdata, int connection) {

    ov_mc_frontend *self = ov_mc_frontend_cast(userdata);
    UNUSED(self);

    ov_log_debug("Socket close received at %i", connection);

    struct container container =
        (struct container){.socket = connection,
                           .self = self,
                           .list = ov_linked_list_create((ov_list_config){0})};

    if (!ov_thread_lock_try_lock(&self->session.lock))
        goto error;
    ov_dict_for_each(self->session.data, &container,
                     search_session_of_connection);
    ov_list_for_each(container.list, self, drop_sessions);
    ov_thread_lock_unlock(&self->session.lock);

    if (!ov_thread_lock_try_lock(&self->proxy.lock))
        goto error;
    ov_dict_del(self->proxy.data, (void *)(intptr_t)connection);
    ov_thread_lock_unlock(&self->proxy.lock);

error:
    ov_list_free(container.list);
    return;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      APP FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

static void cb_event_register(void *userdata, const char *name, int socket,
                              const ov_json_value *input) {

    ov_json_value *out = NULL;

    ov_mc_frontend *self = ov_mc_frontend_cast(userdata);
    if (!self || !name || socket < 0 || !input)
        goto error;

    const char *uuid = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_UUID));

    if (!uuid)
        goto error;

    if (!ov_thread_lock_try_lock(&self->proxy.lock))
        goto error;

    Proxy *proxy = calloc(1, sizeof(Proxy));

    if (proxy) {
        ov_id_set(proxy->id, uuid);
        proxy->load = 0;
        ov_dict_set(self->proxy.data, (void *)(intptr_t)socket, proxy, NULL);
    }

    ov_thread_lock_unlock(&self->proxy.lock);

    return;
error:
    if (self)
        ov_event_app_close(self->app, socket);
    ov_json_value_free(out);
    return;
}

/*----------------------------------------------------------------------------*/

struct container1 {

    size_t pos;
    ov_ice_proxy_vocs_stream_forward_data *array;
};

/*----------------------------------------------------------------------------*/

static bool proxy_json_array_to_forward_data_array(void *item, void *data) {

    if (!item || !data)
        return false;

    ov_json_value *info = ov_json_value_cast(item);
    struct container1 *c = (struct container1 *)data;

    OV_ASSERT(info);
    OV_ASSERT(c);

    c->array[c->pos] = ov_ice_proxy_vocs_stream_forward_data_from_json(info);
    c->pos++;

    return true;
}

/*----------------------------------------------------------------------------*/

static void cb_event_ice_session_create_response(ov_mc_frontend *self,
                                                 int socket,
                                                 const ov_json_value *input) {

    bool drop_session = false;
    bool drop_proxy = false;
    ov_json_value *out = NULL;
    char *str = NULL;

    if (!self || !input) {
        return;
    }

    /*  Expected message format:

        {
            "event" : "ice_session_create",
            "uuid" : "<uuid of request>",
            "request" : { ... },
            "response" :
            {
                "session" : "<ice_session_uuid>",
                "type" : "offer",
                "sdp" : "<sdp_string>",
                "proxy" :
                [
                    {
                        "ssrc" : <ssrc_number>,
                        "socket" {
                            "port" : <port_number>,
                            "host" : "<host_name>",
                            "type" : "udp"
                        }
                    }
                ]
            }
        }
    */

    ov_response_state state = {0};
    if (!ov_response_state_from_message(input, &state)) {
        drop_proxy = true;
    }

    const char *ice_session_id = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_RESPONSE "/" OV_KEY_SESSION));

    const char *sdp = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_RESPONSE "/" OV_KEY_SDP));

    const char *type = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_RESPONSE "/" OV_KEY_TYPE));

    ov_json_value const *proxy =
        ov_json_get(input, "/" OV_KEY_RESPONSE "/" OV_KEY_PROXY);

    size_t array_size = ov_json_array_count(proxy);
    ov_ice_proxy_vocs_stream_forward_data array[array_size];
    memset(array, 0, sizeof(array));

    if (drop_proxy)
        goto close_proxy;

    switch (state.result.error_code) {

    case OV_ERROR_NOERROR:

        /* (1) check required parameter */

        if (!ice_session_id || !sdp || !type || !proxy) {

            state.result.error_code = OV_ERROR_CODE_COMMS_ERROR;
            state.result.message = "missing response data";

            drop_proxy = true;
            break;
        }

        /* (2) parse required parameter */

        struct container1 container =
            (struct container1){.pos = 0, .array = array};

        if (!ov_json_array_for_each((ov_json_value *)proxy, &container,
                                    proxy_json_array_to_forward_data_array)) {

            state.result.error_code = OV_ERROR_CODE_COMMS_ERROR;
            state.result.message = "failed to parse proxy data";

            drop_session = true;
            break;
        }

        /* (3) persist session data */

        if (ov_thread_lock_try_lock(&self->session.lock)) {

            ov_dict_set(self->session.data, ov_string_dup(ice_session_id),
                        (void *)(intptr_t)socket, NULL);
            ov_thread_lock_unlock(&self->session.lock);
        }

        break;

    default:
        /* perform error callback with received error */
        break;
    }

    if (drop_session) {

        out = ov_ice_proxy_vocs_msg_drop_session(ice_session_id);

        if (!ov_event_app_send(self->app, socket, out)) {

            ov_log_error("Failed to drop session %s with processing error at "
                         "%i",
                         ice_session_id, socket);
        }

        out = ov_json_value_free(out);
    }

    /* Perform callback for uuid with error received */
    self->config.callback.session.created(self->config.callback.userdata, state,
                                          ice_session_id, type, sdp, array_size,
                                          array);

    if (drop_proxy)
        goto close_proxy;

    out = ov_json_value_free(out);
    return;

close_proxy:

    str = ov_json_value_to_string(input);
    ov_log_error("Closing proxy at %i due to misformed response %s", socket,
                 str);
    str = ov_data_pointer_free(str);

    ov_event_app_close(self->app, socket);
    out = ov_json_value_free(out);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_event_ice_session_create(void *userdata, const char *name,
                                        int socket, const ov_json_value *input) {

    ov_mc_frontend *self = ov_mc_frontend_cast(userdata);
    if (!self || !name || socket < 0 || !input)
        goto error;

    if (ov_event_api_get_response(input))
        return cb_event_ice_session_create_response(self, socket, input);

    // we do not expect a non response for this event

error:
    if (self)
        ov_event_app_close(self->app, socket);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_event_ice_session_completed(void *userdata, const char *name,
                                           int socket, const ov_json_value *input) {

    ov_mc_frontend *self = ov_mc_frontend_cast(userdata);
    if (!self || !name || socket < 0 || !input)
        goto error;

    const char *session_id = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_SESSION));

    const char *state = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_STATE));

    if (!session_id || !state)
        goto error;

    ov_ice_state s = ov_ice_state_from_string(state);

    ov_log_debug("ICE session state %s %s", session_id, state);

    switch (s) {

    case OV_ICE_COMPLETED:

        self->config.callback.session.completed(self->config.callback.userdata,
                                                session_id, true);
        break;

    case OV_ICE_FAILED:

        self->config.callback.session.completed(self->config.callback.userdata,
                                                session_id, false);
        break;

    default:
        ov_log_error("Unexpected ICE session state %s state %s", session_id,
                     state);
    }

error:
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_event_ice_session_drop_response(ov_mc_frontend *self, int socket,
                                               const ov_json_value *input) {

    if (!self || !input)
        goto error;
    UNUSED(socket);

    const char *id = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_RESPONSE "/" OV_KEY_SESSION));

    if (!id)
        goto error;

    if (!ov_thread_lock_try_lock(&self->session.lock))
        goto error;
    ov_dict_del(self->session.data, id);
    ov_thread_lock_unlock(&self->session.lock);

    self->config.callback.session.dropped(self->config.callback.userdata, id);

error:
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_event_ice_session_drop(void *userdata, const char *name,
                                      int socket, const ov_json_value *input) {

    ov_mc_frontend *self = ov_mc_frontend_cast(userdata);
    if (!self || !name || socket < 0 || !input)
        goto error;

    if (ov_event_api_get_response(input))
        return cb_event_ice_session_drop_response(self, socket, input);

    const char *id = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_SESSION));

    OV_ASSERT(id);

    if (!ov_thread_lock_try_lock(&self->session.lock))
        goto error;
    ov_dict_del(self->session.data, id);
    ov_thread_lock_unlock(&self->session.lock);

    self->config.callback.session.dropped(self->config.callback.userdata, id);

error:
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_event_ice_session_update_response(ov_mc_frontend *self,
                                                 int socket,
                                                 const ov_json_value *input) {

    if (!self || !input)
        goto error;
    UNUSED(socket);

    const char *id = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_RESPONSE "/" OV_KEY_SESSION));

    OV_ASSERT(id);
    if (!id)
        goto error;

    ov_response_state state = {0};

    ov_response_state_from_message(input, &state);

    self->config.callback.session.update(self->config.callback.userdata, state,
                                         id);

error:
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_event_ice_session_update(void *userdata, const char *name,
                                        int socket, const ov_json_value *input) {

    ov_mc_frontend *self = ov_mc_frontend_cast(userdata);
    if (!self || !name || socket < 0 || !input)
        goto error;

    if (ov_event_api_get_response(input))
        return cb_event_ice_session_update_response(self, socket, input);

    // we do not expect some update event from the proxy

error:
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_event_state_response(ov_mc_frontend *self, int socket,
                                    const ov_json_value *input) {

    if (!self || !input)
        goto error;
    UNUSED(socket);

    const char *id = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_RESPONSE "/" OV_KEY_SESSION));

    OV_ASSERT(id);
    if (!id)
        goto error;

    ov_response_state state = {0};

    ov_response_state_from_message(input, &state);

    self->config.callback.session.state(self->config.callback.userdata, state,
                                        id, ov_event_api_get_response(input));

error:
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_event_state(void *userdata, const char *name, int socket,
                           const ov_json_value *input) {

    ov_mc_frontend *self = ov_mc_frontend_cast(userdata);
    if (!self || !name || socket < 0 || !input)
        goto error;

    if (ov_event_api_get_response(input))
        return cb_event_state_response(self, socket, input);

    // we do not expect some update event from the proxy

error:
    return;
}
/*----------------------------------------------------------------------------*/

static void cb_event_ice_candidate_response(ov_mc_frontend *self, int socket,
                                            const ov_json_value *input) {

    ov_response_state state = {0};

    if (!self || !input)
        goto error;
    UNUSED(socket);

    ov_json_value *src = ov_event_api_get_response(input);

    const char *session_id =
        ov_json_string_get(ov_json_get(src, "/" OV_KEY_SESSION));

    ov_json_value *par =
        ov_event_api_get_parameter(ov_event_api_get_request(input));

    ov_ice_candidate_info info = ov_ice_candidate_info_from_json(par);

    ov_response_state_from_message(input, &state);

    self->config.callback.candidate(self->config.callback.userdata, state,
                                    session_id, &info);

error:
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_event_ice_candidate(void *userdata, const char *name, int socket,
                                   const ov_json_value *input) {

    ov_response_state state = {0};

    ov_mc_frontend *self = ov_mc_frontend_cast(userdata);
    if (!self || !name || socket < 0 || !input)
        goto error;

    if (ov_event_api_get_response(input))
        return cb_event_ice_candidate_response(self, socket, input);

    ov_json_value *src = ov_event_api_get_parameter(input);

    const char *session_id =
        ov_json_string_get(ov_json_get(src, "/" OV_KEY_SESSION));

    ov_ice_candidate_info info = ov_ice_candidate_info_from_json(src);

    state.result.error_code = OV_ERROR_CODE_NOT_A_RESPONSE;
    state.result.message = OV_ERROR_DESC_NOT_A_RESPONSE;

    self->config.callback.candidate(self->config.callback.userdata, state,
                                    session_id, &info);

error:
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_event_ice_end_of_candidates_response(ov_mc_frontend *self,
                                                    int socket,
                                                    const ov_json_value *input) {

    ov_response_state state = {0};

    if (!self || !input)
        goto error;
    UNUSED(socket);

    ov_json_value *src = ov_event_api_get_response(input);

    const char *session_id =
        ov_json_string_get(ov_json_get(src, "/" OV_KEY_SESSION));

    ov_ice_candidate_info info = ov_ice_candidate_info_from_json(src);

    ov_response_state_from_message(input, &state);

    self->config.callback.end_of_candidates(self->config.callback.userdata,
                                            state, session_id, &info);

error:
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_event_ice_end_of_candidates(void *userdata, const char *name,
                                           int socket, const ov_json_value *input) {

    ov_response_state state = {0};

    ov_mc_frontend *self = ov_mc_frontend_cast(userdata);
    if (!self || !name || socket < 0 || !input)
        goto error;

    if (ov_event_api_get_response(input))
        return cb_event_ice_end_of_candidates_response(self, socket, input);

    ov_json_value *src = ov_event_api_get_parameter(input);

    const char *session_id =
        ov_json_string_get(ov_json_get(src, "/" OV_KEY_SESSION));

    ov_ice_candidate_info info = ov_ice_candidate_info_from_json(src);

    state.result.error_code = OV_ERROR_CODE_NOT_A_RESPONSE;
    state.result.message = OV_ERROR_DESC_NOT_A_RESPONSE;

    self->config.callback.end_of_candidates(self->config.callback.userdata,
                                            state, session_id, &info);

error:
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_event_talk_response(ov_mc_frontend *self, int socket,
                                   const ov_json_value *input) {

    ov_response_state state = {0};
    bool on = false;

    if (!self || !input)
        goto error;
    UNUSED(socket);

    ov_json_value *src = ov_event_api_get_response(input);

    const char *session_id =
        ov_json_string_get(ov_json_get(src, "/" OV_KEY_SESSION));

    ov_mc_loop_data data =
        ov_mc_loop_data_from_json(ov_json_get(src, "/" OV_KEY_LOOP));

    if (ov_json_is_true(ov_json_get(src, "/" OV_KEY_ON))) {
        on = true;
    } else {
        on = false;
    }

    ov_response_state_from_message(input, &state);

    self->config.callback.talk(self->config.callback.userdata, state,
                               session_id, data, on);

error:
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_event_talk(void *userdata, const char *name, int socket,
                          const ov_json_value *input) {

    ov_mc_frontend *self = ov_mc_frontend_cast(userdata);
    if (!self || !name || socket < 0 || !input)
        goto error;

    if (ov_event_api_get_response(input))
        return cb_event_talk_response(self, socket, input);

    // we dont expect a non response talk message
error:
    return;
}

/*----------------------------------------------------------------------------*/

static bool register_app_callbacks(ov_mc_frontend *self) {

    OV_ASSERT(self);

    if (!ov_event_app_register(self->app, OV_KEY_REGISTER, self,
                               cb_event_register))
        goto error;

    if (!ov_event_app_register(self->app, OV_KEY_ICE_SESSION_COMPLETED, self,
                               cb_event_ice_session_completed))
        goto error;

    if (!ov_event_app_register(self->app, OV_KEY_ICE_SESSION_CREATE, self,
                               cb_event_ice_session_create))
        goto error;

    if (!ov_event_app_register(self->app, OV_KEY_ICE_SESSION_DROP, self,
                               cb_event_ice_session_drop))
        goto error;

    if (!ov_event_app_register(self->app, OV_KEY_ICE_SESSION_UPDATE, self,
                               cb_event_ice_session_update))
        goto error;

    if (!ov_event_app_register(self->app, OV_ICE_STRING_CANDIDATE, self,
                               cb_event_ice_candidate))
        goto error;

    if (!ov_event_app_register(self->app, OV_ICE_STRING_END_OF_CANDIDATES, self,
                               cb_event_ice_end_of_candidates))
        goto error;

    if (!ov_event_app_register(self->app, OV_KEY_TALK, self, cb_event_talk))
        goto error;

    if (!ov_event_app_register(self->app, OV_KEY_ICE_SESSION_STATE, self,
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

ov_mc_frontend *ov_mc_frontend_create(ov_mc_frontend_config config) {

    ov_mc_frontend *self = NULL;

    if (!config.loop)
        goto error;

    if (0 == config.limits.request_usec)
        config.limits.request_usec = 5000000;

    if (0 == config.limits.threadlock_usec)
        config.limits.threadlock_usec = 100000;

    self = calloc(1, sizeof(ov_mc_frontend));
    if (!self)
        goto error;

    self->magic_bytes = OV_MC_FRONTEND_MAGIC_BYTES;
    self->config = config;

    ov_event_app_config app_config =
        (ov_event_app_config){
            .loop = config.loop,
            .io = config.io,
            .callbacks.userdata = self,
            .callbacks.close = cb_close};

    strncat(app_config.name, "VOCS FRONTEND", OV_HOST_NAME_MAX -1);
    strncat(app_config.password_path, config.password.path, PATH_MAX -1);

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

    if (!ov_thread_lock_init(&self->proxy.lock,
                             self->config.limits.threadlock_usec))
        goto error;
    if (!ov_thread_lock_init(&self->session.lock,
                             self->config.limits.threadlock_usec))
        goto error;

    ov_dict_config d_config = ov_dict_string_key_config(255);
    self->session.data = ov_dict_create(d_config);
    if (!self->session.data)
        goto error;

    d_config = ov_dict_intptr_key_config(255);
    d_config.value.data_function.free = ov_data_pointer_free;
    self->proxy.data = ov_dict_create(d_config);
    if (!self->proxy.data)
        goto error;

    return self;
error:
    ov_mc_frontend_free(self);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_mc_frontend *ov_mc_frontend_free(ov_mc_frontend *self) {

    if (!ov_mc_frontend_cast(self))
        return self;

    ov_thread_lock_clear(&self->proxy.lock);
    ov_thread_lock_clear(&self->session.lock);

    self->session.data = ov_dict_free(self->session.data);
    self->proxy.data = ov_dict_free(self->proxy.data);

    self->app = ov_event_app_free(self->app);
    self = ov_data_pointer_free(self);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_mc_frontend *ov_mc_frontend_cast(const void *data) {

    if (!data)
        return NULL;

    if (*(uint16_t *)data != OV_MC_FRONTEND_MAGIC_BYTES)
        return NULL;

    return (ov_mc_frontend *)data;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      CONTROL FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

struct container2 {

    int socket;
    uint64_t load;
};

/*----------------------------------------------------------------------------*/

static bool search_proxy(const void *key, void *val, void *data) {

    if (!key)
        return true;
    Proxy *proxy = (Proxy *)val;
    struct container2 *container = (struct container2 *)data;

    if (0 == container->socket) {
        container->socket = (intptr_t)key;
        container->load = proxy->load;
        return true;
    }

    if (proxy->load < container->load) {

        container->socket = (intptr_t)key;
        container->load = proxy->load;
        return true;
    }

    return true;
}

/*----------------------------------------------------------------------------*/

bool ov_mc_frontend_create_session(ov_mc_frontend *self, char const *uuid,
                                   const char *sdp) {

    ov_json_value *out = NULL;
    bool result = false;

    if (!self || !uuid || !sdp)
        goto error;

    struct container2 container = (struct container2){.socket = 0, .load = 0};

    if (!ov_thread_lock_try_lock(&self->proxy.lock))
        goto error;
    ov_dict_for_each(self->proxy.data, &container, search_proxy);
    ov_thread_lock_unlock(&self->proxy.lock);

    if (0 == container.socket) {
        ov_log_error("No proxy avaialable.");
        goto error;
    }

    out = ov_ice_proxy_vocs_msg_create_session_with_sdp(uuid, sdp);

    result = ov_event_app_send(self->app, container.socket, out);
    out = ov_json_value_free(out);

    return result;

error:
    ov_json_value_free(out);
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_mc_frontend_update_session(ov_mc_frontend *self, char const *uuid,
                                   const char *session_uuid, ov_media_type type,
                                   const char *sdp) {

    ov_json_value *out = NULL;

    if (!self || !uuid || !session_uuid || !sdp)
        goto error;

    if (!ov_thread_lock_try_lock(&self->session.lock))
        goto error;
    int socket = (intptr_t)ov_dict_get(self->session.data, session_uuid);
    ov_thread_lock_unlock(&self->session.lock);

    if (0 >= socket)
        goto error;

    out = ov_ice_proxy_vocs_msg_update_session(uuid, session_uuid, type, sdp);

    if (!ov_event_app_send(self->app, socket, out))
        goto error;

    out = ov_json_value_free(out);
    return true;
error:
    out = ov_json_value_free(out);
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_mc_frontend_drop_session(ov_mc_frontend *self, const char *uuid,
                                 const char *session_id) {

    ov_json_value *out = NULL;

    if (!self || !uuid || !session_id)
        goto error;

    if (!ov_thread_lock_try_lock(&self->session.lock))
        goto error;
    int socket = (intptr_t)ov_dict_get(self->session.data, session_id);
    ov_thread_lock_unlock(&self->session.lock);

    if (0 >= socket)
        goto done;

    out = ov_ice_proxy_vocs_msg_drop_session(session_id);
    ov_event_api_set_uuid(out, uuid);

    if (!ov_event_app_send(self->app, socket, out))
        goto error;
done:
    out = ov_json_value_free(out);
    return true;
error:
    out = ov_json_value_free(out);
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_mc_frontend_candidate(ov_mc_frontend *self, char const *uuid,
                              const char *session_id,
                              const ov_ice_candidate_info *info) {

    ov_json_value *out = NULL;

    if (!self || !uuid || !session_id || !info)
        goto error;

    if (!ov_thread_lock_try_lock(&self->session.lock))
        goto error;
    int socket = (intptr_t)ov_dict_get(self->session.data, session_id);
    ov_thread_lock_unlock(&self->session.lock);

    if (0 >= socket)
        goto error;

    out = ov_ice_proxy_vocs_msg_candidate(session_id, info);
    ov_event_api_set_uuid(out, uuid);

    if (!ov_event_app_send(self->app, socket, out))
        goto error;

    out = ov_json_value_free(out);
    return true;
error:
    out = ov_json_value_free(out);
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_mc_frontend_end_of_candidates(ov_mc_frontend *self, char const *uuid,
                                      const char *session_id) {

    ov_json_value *out = NULL;

    if (!self || !uuid || !session_id)
        goto error;

    if (!ov_thread_lock_try_lock(&self->session.lock))
        goto error;
    int socket = (intptr_t)ov_dict_get(self->session.data, session_id);
    ov_thread_lock_unlock(&self->session.lock);

    if (0 >= socket)
        goto error;

    out = ov_ice_proxy_vocs_msg_end_of_candidates(session_id);
    ov_event_api_set_uuid(out, uuid);

    if (!ov_event_app_send(self->app, socket, out))
        goto error;

    out = ov_json_value_free(out);
    return true;
error:
    out = ov_json_value_free(out);
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_mc_frontend_talk(ov_mc_frontend *self, char const *uuid,
                         const char *session_id, bool on,
                         ov_mc_loop_data data) {

    ov_json_value *out = NULL;

    if (!self || !uuid || !session_id)
        goto error;

    if (!ov_thread_lock_try_lock(&self->session.lock))
        goto error;
    int socket = (intptr_t)ov_dict_get(self->session.data, session_id);
    ov_thread_lock_unlock(&self->session.lock);

    if (0 >= socket)
        goto error;

    out = ov_ice_proxy_vocs_msg_talk(uuid, session_id, on, data);

    if (!ov_event_app_send(self->app, socket, out))
        goto error;

    out = ov_json_value_free(out);
    return true;
error:
    out = ov_json_value_free(out);
    return false;
}

/*----------------------------------------------------------------------------*/

ov_mc_frontend_config ov_mc_frontend_config_from_json(const ov_json_value *in) {

    ov_mc_frontend_config config = (ov_mc_frontend_config){0};

    const ov_json_value *data = ov_json_object_get(in, OV_KEY_FRONTEND);
    if (!data)
        data = in;

    config.socket.manager = ov_socket_configuration_from_json(
        ov_json_get(data, "/" OV_KEY_SOCKET "/" OV_KEY_MANAGER),
        (ov_socket_configuration){0});

    return config;
}

/*----------------------------------------------------------------------------*/

bool ov_mc_frontened_get_session_state(ov_mc_frontend *self, const char *uuid,
                                       const char *session_id) {

    ov_json_value *out = NULL;

    if (!self || !uuid || !session_id)
        goto error;

    if (!ov_thread_lock_try_lock(&self->session.lock))
        goto error;
    int socket = (intptr_t)ov_dict_get(self->session.data, session_id);
    ov_thread_lock_unlock(&self->session.lock);

    if (0 >= socket)
        goto error;

    out = ov_ice_proxy_vocs_msg_session_state(session_id);
    ov_event_api_set_uuid(out, uuid);

    if (!ov_event_app_send(self->app, socket, out))
        goto error;

    out = ov_json_value_free(out);
    return true;
error:
    out = ov_json_value_free(out);
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_mc_frontend_connect_cc(ov_mc_frontend *self, ov_io_socket_config config){

    if (!self) return false;
    return ov_event_app_connect_cc(self->app, config);
}