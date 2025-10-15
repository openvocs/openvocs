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
        @file           ov_vocs.c
        @author         Töpfer, Markus

        @date           2025-02-03


        ------------------------------------------------------------------------
*/
#include "../include/ov_vocs.h"
#include "../include/ov_vocs_management.h"

#define OV_VOCS_MAGIC_BYTES 0x67ef

#include <ov_base/ov_utils.h>

#include <ov_base/ov_dict.h>
#include <ov_base/ov_event_keys.h>
#include <ov_base/ov_json_value.h>

#include <ov_core/ov_broadcast_registry.h>
#include <ov_core/ov_event_api.h>
#include <ov_core/ov_event_async.h>
#include <ov_core/ov_event_session.h>
#include <ov_core/ov_socket_json.h>

#include "../include/ov_mc_sip_msg.h"
#include "../include/ov_vocs_connection.h"
#include "../include/ov_vocs_events.h"
#include "../include/ov_vocs_loop.h"
#include <ov_base/ov_error_codes.h>

#include <ov_vocs_db/ov_vocs_db_app.h>

#define ov_vocs_MAGIC_BYTES 0x13db

#define IMPL_TIMEOUT_RESPONSE_USEC 5000 * 1000
#define IMPL_TIMEOUT_RECONNECT_USEC 1000 * 1000

struct ov_vocs {

    uint16_t magic_bytes;
    ov_vocs_config config;

    bool debug;

    ov_mc_backend *backend;
    ov_mc_frontend *frontend;
    ov_mc_backend_sip *sip;
    ov_mc_backend_sip_static *sip_static;
    ov_vocs_recorder *recorder;
    ov_mc_backend_vad *vad;

    ov_ldap *ldap;

    ov_event_async_store *async;
    ov_broadcast_registry *broadcasts;
    ov_event_session *user_sessions;

    ov_dict *sessions; // sessions spanning over ice proxy and resmgr
    ov_dict *loops;    // loops aquired (name dict)
    ov_dict *io;       // event functions (event io)

    ov_socket_json *connections;

    ov_callback_registry *callbacks;

    ov_vocs_management *management;
};

/*
 *      ------------------------------------------------------------------------
 *
 *      #BACKEND FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

static void cb_backend_mixer_lost(void *userdata, const char *uuid);

static void cb_backend_mixer_acquired(void *userdata,
                                      const char *uuid,
                                      const char *session_id,
                                      uint64_t error_code,
                                      const char *error_desc);

static void cb_backend_mixer_released(void *userdata,
                                      const char *uuid,
                                      const char *session_id,
                                      uint64_t error_code,
                                      const char *error_desc);

static void cb_backend_mixer_join(void *userdata,
                                  const char *uuid,
                                  const char *session_id,
                                  const char *loopname,
                                  uint64_t error_code,
                                  const char *error_desc);

static void cb_backend_mixer_leave(void *userdata,
                                   const char *uuid,
                                   const char *session_id,
                                   const char *loopname,
                                   uint64_t error_code,
                                   const char *error_desc);

static void cb_backend_mixer_volume(void *userdata,
                                    const char *uuid,
                                    const char *session_id,
                                    const char *loopname,
                                    uint8_t volume,
                                    uint64_t error_code,
                                    const char *error_desc);

static void cb_backend_mixer_state(void *userdata,
                                   const char *uuid,
                                   const ov_json_value *state);

/*
 *      ------------------------------------------------------------------------
 *
 *      #SEND EVENTS
 *
 *      ------------------------------------------------------------------------
 */

static bool env_send(ov_vocs *vocs, int socket, const ov_json_value *input) {

    OV_ASSERT(vocs);
    OV_ASSERT(vocs->config.env.send);
    OV_ASSERT(vocs->config.env.userdata);

    bool result =
        vocs->config.env.send(vocs->config.env.userdata, socket, input);

    return result;
}

/*----------------------------------------------------------------------------*/

static bool send_socket(void *self, int socket, const ov_json_value *input) {

    ov_vocs *vocs = ov_vocs_cast(self);
    if (!vocs || !input) return false;

    return env_send(vocs, socket, input);
}

/*----------------------------------------------------------------------------*/

static bool send_error_response(ov_vocs *vocs,
                                const ov_json_value *input,
                                int socket,
                                uint64_t code,
                                const char *desc) {

    bool result = false;

    if (!vocs || !input) goto error;

    if (!desc) desc = OV_ERROR_DESC;

    ov_json_value *out = ov_event_api_create_error_response(input, code, desc);
    ov_event_api_set_type(out, OV_KEY_UNICAST);

    result = env_send(vocs, socket, out);

    out = ov_json_value_free(out);
    return result;

error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool send_success_response(ov_vocs *vocs,
                                  const ov_json_value *input,
                                  int socket,
                                  ov_json_value **response) {

    bool result = false;

    if (!vocs || !input) goto error;

    ov_json_value *out = ov_event_api_create_success_response(input);
    ov_event_api_set_type(out, OV_KEY_UNICAST);

    if (response && *response) {

        if (ov_json_object_set(out, OV_KEY_RESPONSE, *response))
            *response = NULL;
    }

    result = env_send(vocs, socket, out);

    out = ov_json_value_free(out);
    return result;

error:
    return false;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      #BROADCAST FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

static bool send_switch_loop_broadcast(ov_vocs *vocs,
                                       int socket,
                                       const char *loop,
                                       ov_vocs_permission current) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *par = NULL;
    ov_json_value *data = NULL;

    if (!vocs || !loop) goto error;

    data = ov_socket_json_get(vocs->connections, socket);
    const char *state = ov_vocs_permission_to_string(current);
    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));
    const char *role = ov_json_string_get(ov_json_get(data, "/" OV_KEY_ROLE));
    const char *client =
        ov_json_string_get(ov_json_get(data, "/" OV_KEY_CLIENT));

    ov_vocs_loop *l = ov_dict_get(vocs->loops, loop);
    ov_json_value *participants = ov_vocs_loop_get_participants(l);

    out = ov_event_api_message_create(OV_EVENT_API_SWITCH_LOOP_STATE, NULL, 0);

    if (!ov_event_api_set_type(out, OV_BROADCAST_KEY_LOOP_BROADCAST))
        goto error;

    par = ov_event_api_set_parameter(out);
    if (!ov_json_object_set(par, OV_KEY_PARTICIPANTS, participants)) goto error;

    val = ov_json_string(loop);
    if (!ov_json_object_set(par, OV_KEY_LOOP, val)) goto error;

    if (user) {

        val = ov_json_string(user);
        if (!ov_json_object_set(par, OV_KEY_USER, val)) goto error;
    }

    if (role) {

        val = ov_json_string(role);
        if (!ov_json_object_set(par, OV_KEY_ROLE, val)) goto error;
    }

    if (state) {

        val = ov_json_string(state);
        if (!ov_json_object_set(par, OV_KEY_STATE, val)) goto error;
    }

    if (client) {

        val = ov_json_string(client);
        if (!ov_json_object_set(par, OV_KEY_CLIENT, val)) goto error;
    }

    val = NULL;

    ov_event_parameter parameter =
        (ov_event_parameter){.send.instance = vocs, .send.send = send_socket};

    if (!ov_broadcast_registry_send(
            vocs->broadcasts, loop, &parameter, out, OV_LOOP_BROADCAST))
        goto error;

    data = ov_json_value_free(data);
    out = ov_json_value_free(out);

    return true;

error:
    ov_json_value_free(out);
    ov_json_value_free(val);
    ov_json_value_free(data);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool send_switch_loop_user_broadcast(ov_vocs *vocs,
                                            int socket,
                                            const char *loop,
                                            ov_vocs_permission current) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *par = NULL;
    ov_json_value *data = NULL;

    if (!vocs || !loop) goto error;

    data = ov_socket_json_get(vocs->connections, socket);
    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));
    const char *role = ov_json_string_get(ov_json_get(data, "/" OV_KEY_ROLE));
    const char *client =
        ov_json_string_get(ov_json_get(data, "/" OV_KEY_CLIENT));
    const char *state = ov_vocs_permission_to_string(current);

    ov_vocs_loop *l = ov_dict_get(vocs->loops, loop);
    ov_json_value *participants = ov_vocs_loop_get_participants(l);

    out = ov_event_api_message_create(OV_EVENT_API_SWITCH_LOOP_STATE, NULL, 0);

    if (!ov_event_api_set_type(out, OV_BROADCAST_KEY_USER_BROADCAST))
        goto error;

    par = ov_event_api_set_parameter(out);
    if (!ov_json_object_set(par, OV_KEY_PARTICIPANTS, participants)) goto error;

    val = ov_json_string(loop);
    if (!ov_json_object_set(par, OV_KEY_LOOP, val)) goto error;

    if (user) {

        val = ov_json_string(user);
        if (!ov_json_object_set(par, OV_KEY_USER, val)) goto error;
    }

    if (role) {

        val = ov_json_string(role);
        if (!ov_json_object_set(par, OV_KEY_ROLE, val)) goto error;
    }

    if (state) {

        val = ov_json_string(state);
        if (!ov_json_object_set(par, OV_KEY_STATE, val)) goto error;
    }

    if (client) {

        val = ov_json_string(client);
        if (!ov_json_object_set(par, OV_KEY_CLIENT, val)) goto error;
    }

    val = NULL;

    ov_event_parameter parameter =
        (ov_event_parameter){.send.instance = vocs, .send.send = send_socket};

    if (!ov_broadcast_registry_send(
            vocs->broadcasts, user, &parameter, out, OV_USER_BROADCAST))
        goto error;

    out = ov_json_value_free(out);
    data = ov_json_value_free(data);

    return true;

error:
    ov_json_value_free(out);
    ov_json_value_free(val);
    data = ov_json_value_free(data);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool send_switch_volume_user_broadcast(ov_vocs *vocs,
                                              int socket,
                                              const char *loop,
                                              uint8_t volume) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *par = NULL;
    ov_json_value *data = NULL;

    if (!vocs || !loop) goto error;

    data = ov_socket_json_get(vocs->connections, socket);
    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));
    const char *role = ov_json_string_get(ov_json_get(data, "/" OV_KEY_ROLE));
    const char *client =
        ov_json_string_get(ov_json_get(data, "/" OV_KEY_CLIENT));

    if (!user) goto error;

    out = ov_event_api_message_create(OV_EVENT_API_SWITCH_LOOP_VOLUME, NULL, 0);

    if (!ov_event_api_set_type(out, OV_BROADCAST_KEY_USER_BROADCAST))
        goto error;

    par = ov_event_api_set_parameter(out);

    val = ov_json_string(loop);
    if (!ov_json_object_set(par, OV_KEY_LOOP, val)) goto error;

    val = ov_json_number(volume);
    if (!ov_json_object_set(par, OV_KEY_VOLUME, val)) goto error;

    if (user) {

        val = ov_json_string(user);
        if (!ov_json_object_set(par, OV_KEY_USER, val)) goto error;
    }

    if (role) {

        val = ov_json_string(role);
        if (!ov_json_object_set(par, OV_KEY_ROLE, val)) goto error;
    }

    if (client) {

        val = ov_json_string(client);
        if (!ov_json_object_set(par, OV_KEY_CLIENT, val)) goto error;
    }

    val = NULL;

    ov_event_parameter parameter =
        (ov_event_parameter){.send.instance = vocs, .send.send = send_socket};

    if (!ov_broadcast_registry_send(
            vocs->broadcasts, user, &parameter, out, OV_USER_BROADCAST))
        goto error;

    out = ov_json_value_free(out);
    data = ov_json_value_free(data);
    return true;
error:
    ov_json_value_free(out);
    ov_json_value_free(val);
    ov_json_value_free(data);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool send_talking_loop_broadcast(ov_vocs *vocs,
                                        int socket,
                                        const ov_event_parameter *params,
                                        const char *loop,
                                        const ov_json_value *state,
                                        const char *client) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *par = NULL;
    ov_json_value *data = NULL;

    UNUSED(params);

    if (!vocs || !loop) goto error;

    data = ov_socket_json_get(vocs->connections, socket);
    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));
    const char *role = ov_json_string_get(ov_json_get(data, "/" OV_KEY_ROLE));

    out = ov_event_api_message_create(OV_EVENT_API_TALKING, NULL, 0);

    if (!ov_event_api_set_type(out, OV_BROADCAST_KEY_LOOP_BROADCAST))
        goto error;

    par = ov_event_api_set_parameter(out);

    val = ov_json_string(loop);
    if (!ov_json_object_set(par, OV_KEY_LOOP, val)) goto error;

    if (user) {

        val = ov_json_string(user);
        if (!ov_json_object_set(par, OV_KEY_USER, val)) goto error;
    }

    if (role) {

        val = ov_json_string(role);
        if (!ov_json_object_set(par, OV_KEY_ROLE, val)) goto error;
    }

    if (state) {

        val = NULL;
        if (!ov_json_value_copy((void **)&val, state)) goto error;

        if (!ov_json_object_set(par, OV_KEY_STATE, val)) goto error;
    }

    if (client) {

        val = ov_json_string(client);
        if (!ov_json_object_set(par, OV_KEY_CLIENT, val)) goto error;
    }

    val = NULL;

    ov_event_parameter parameter =
        (ov_event_parameter){.send.instance = vocs, .send.send = send_socket};

    if (!ov_broadcast_registry_send(
            vocs->broadcasts, loop, &parameter, out, OV_LOOP_BROADCAST)) {

        goto error;
    }

    out = ov_json_value_free(out);
    data = ov_json_value_free(data);
    return true;
error:
    ov_json_value_free(out);
    ov_json_value_free(val);
    ov_json_value_free(data);
    return false;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      #DROP EVENTS
 *
 *      ------------------------------------------------------------------------
 */

struct container_drop {

    ov_vocs *vocs;
    int socket;
    ov_json_value *data;
};

/*----------------------------------------------------------------------------*/

static bool close_participation(const void *key, void *val, void *data) {

    if (!key) return true;

    struct container_drop *c = (struct container_drop *)data;
    ov_vocs_loop *l = ov_vocs_loop_cast(val);

    const ov_json_value *loops = ov_json_get(data, "/" OV_KEY_LOOPS);

    ov_vocs_permission current = ov_vocs_permission_from_string(
        ov_json_string_get(ov_json_object_get(loops, (char *)key)));

    ov_vocs_loop_drop_participant(l, c->socket);

    switch (current) {

        case OV_VOCS_NONE:
            return true;

        default:
            if (!send_switch_loop_broadcast(
                    c->vocs, c->socket, key, OV_VOCS_NONE)) {
                ov_log_error("failed to send switch loop broadcast %s", key);
            }
    }

    return true;
}

/*----------------------------------------------------------------------------*/

static bool drop_connection(ov_vocs *vocs,
                            int socket,
                            bool frontend,
                            bool backend) {

    ov_json_value *data = NULL;

    if (!vocs) goto error;

    data = ov_socket_json_get(vocs->connections, socket);
    const char *session =
        ov_json_string_get(ov_json_get(data, "/" OV_KEY_SESSION));

    if (NULL != session) {

        ov_id uuid = {0};
        ov_id_fill_with_uuid(uuid);

        if (frontend &&
            !ov_mc_frontend_drop_session(vocs->frontend, uuid, session)) {

            ov_log_error("failed to close %s in ICE proxy", session);
        }

        if (backend &&
            !ov_mc_backend_release_mixer(vocs->backend,
                                         uuid,
                                         session,
                                         vocs,
                                         cb_backend_mixer_released)) {

            ov_log_error("failed to close %s in resmgr", session);
        }

        ov_dict_del(vocs->sessions, session);
    }

    ov_event_async_drop(vocs->async, socket);
    ov_broadcast_registry_unset(vocs->broadcasts, socket);

    struct container_drop container = {
        .vocs = vocs, .socket = socket, .data = data};

    ov_dict_for_each(vocs->loops, &container, close_participation);

    ov_socket_json_drop(vocs->connections, socket);

    vocs->config.env.close(vocs->config.env.userdata, socket);
    data = ov_json_value_free(data);
    return true;
error:
    data = ov_json_value_free(data);
    return false;
}

/*----------------------------------------------------------------------------*/

static void cb_socket_close(void *userdata, int socket) {

    ov_vocs *vocs = ov_vocs_cast(userdata);
    if (!vocs || socket < 0) goto error;

    ov_log_debug("Client socket close at %i", socket);
    ov_json_value *data = ov_socket_json_get(vocs->connections, socket);

    if (data) {
        drop_connection(vocs, socket, true, true);
        data = ov_json_value_free(data);
    }

error:
    return;
}

/*----------------------------------------------------------------------------*/

static void async_timedout(void *userdata, ov_event_async_data data) {

    ov_vocs *self = ov_vocs_cast(userdata);
    if (!self) goto error;

    char *str = ov_json_value_to_string(data.value);
    ov_log_error("Async timeout - dropping %i | %s", data.socket, str);
    str = ov_data_pointer_free(str);

    ov_json_value *out = ov_event_api_create_error_response(
        data.value, OV_ERROR_TIMEOUT, OV_ERROR_DESC_TIMEOUT);
    env_send(self, data.socket, out);
    out = ov_json_value_free(out);

    drop_connection(self, data.socket, true, true);

error:
    ov_event_async_data_clear(&data);
    return;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      #BACKEND EVENTS
 *
 *      ------------------------------------------------------------------------
 */

static void cb_backend_mixer_lost(void *userdata, const char *uuid) {

    ov_vocs *vocs = ov_vocs_cast(userdata);
    if (!vocs || !uuid) goto error;

    intptr_t socket = (intptr_t)ov_dict_get(vocs->sessions, uuid);

    ov_log_error("mixer lost %s", uuid);

    drop_connection(vocs, socket, true, false);

error:
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_backend_mixer_acquired(void *userdata,
                                      const char *uuid,
                                      const char *session_id,
                                      uint64_t error_code,
                                      const char *error_desc) {

    ov_json_value *data = NULL;
    ov_json_value *orig = NULL;

    ov_id new_uuid = {0};

    ov_vocs *vocs = ov_vocs_cast(userdata);
    if (!vocs || !uuid || !session_id) goto error;

    intptr_t socket = (intptr_t)ov_dict_get(vocs->sessions, session_id);
    if (0 == socket) goto drop_mixer_acquisition;

    data = ov_socket_json_get(vocs->connections, socket);

    ov_event_async_data adata = ov_event_async_unset(vocs->async, uuid);
    orig = adata.value;

    switch (error_code) {

        case OV_ERROR_NOERROR:
            break;

        default:

            ov_log_error("acquire mixer failed %i|%s for %s",
                         error_code,
                         error_desc,
                         session_id);

            drop_connection(vocs, socket, true, false);
            goto error;
    }

    ov_json_object_set(data, OV_KEY_MEDIA_READY, ov_json_true());

    if (ov_json_is_true(ov_json_get(data, "/" OV_KEY_ICE))) {

        /* ICE completed already returned, send media ready */

        ov_json_value *out =
            ov_event_api_message_create(OV_KEY_MEDIA_READY, NULL, 0);

        ov_event_api_set_type(out, OV_KEY_UNICAST);

        if (!ov_event_api_set_parameter(out)) goto error;

        env_send(vocs, socket, out);
        out = ov_json_value_free(out);
    }

    ov_socket_json_set(vocs->connections, socket, &data);
    orig = ov_json_value_free(orig);
    return;

drop_mixer_acquisition:

    ov_id_fill_with_uuid(new_uuid);
    ov_mc_backend_release_mixer(
        vocs->backend, new_uuid, session_id, vocs, cb_backend_mixer_released);

error:
    data = ov_json_value_free(data);
    orig = ov_json_value_free(orig);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_backend_mixer_released(void *userdata,
                                      const char *uuid,
                                      const char *session_id,
                                      uint64_t error_code,
                                      const char *error_desc) {

    ov_vocs *vocs = ov_vocs_cast(userdata);
    if (!vocs || !uuid || !session_id) goto error;

    intptr_t socket = (intptr_t)ov_dict_get(vocs->sessions, session_id);
    if (0 == socket) goto error;

    switch (error_code) {

        case OV_ERROR_NOERROR:
            break;

        default:

            ov_log_error("release mixer failed %i|%s for %s",
                         error_code,
                         error_desc,
                         session_id);

            goto error;
    }

    drop_connection(vocs, socket, true, false);
error:
    return;
}

/*----------------------------------------------------------------------------*/

static bool perform_switch_loop_request(ov_vocs *vocs,
                                        const char *uuid,
                                        const char *sess,
                                        const char *user,
                                        const char *role,
                                        const char *loop,
                                        ov_vocs_permission current,
                                        ov_vocs_permission request) {

    if (!vocs || !uuid || !sess || !loop) goto error;

    ov_socket_configuration multicast =
        ov_vocs_db_get_multicast_group(vocs->config.db, loop);

    if (0 == multicast.host[0]) {
        ov_log_error("no multicast group configured for loop %s", loop);
        goto error;
    }
    ov_mc_loop_data data = {0};
    data.socket = multicast;
    strncpy(data.name, loop, OV_MC_LOOP_NAME_MAX);
    data.volume = ov_vocs_db_get_volume(vocs->config.db, user, role, loop);

    switch (current) {

        case OV_VOCS_NONE:

            switch (request) {

                case OV_VOCS_NONE:
                    break;

                case OV_VOCS_RECV:
                case OV_VOCS_SEND:

                    ov_vocs_recorder_join_loop(
                        vocs->recorder, user, role, loop);

                    return ov_mc_backend_join_loop(vocs->backend,
                                                   uuid,
                                                   sess,
                                                   data,
                                                   vocs,
                                                   cb_backend_mixer_join);

                    break;
            }

            break;

        case OV_VOCS_RECV:

            switch (request) {

                case OV_VOCS_NONE:

                    ov_vocs_recorder_leave_loop(
                        vocs->recorder, user, role, loop);

                    return ov_mc_backend_leave_loop(vocs->backend,
                                                    uuid,
                                                    sess,
                                                    loop,
                                                    vocs,
                                                    cb_backend_mixer_leave);

                    break;

                case OV_VOCS_RECV:
                    break;

                case OV_VOCS_SEND:

                    ov_vocs_recorder_talk_on_loop(
                        vocs->recorder, user, role, loop);

                    return ov_mc_frontend_talk(
                        vocs->frontend, uuid, sess, true, data);

                    break;
            }

            break;

        case OV_VOCS_SEND:

            switch (request) {

                case OV_VOCS_NONE:
                case OV_VOCS_RECV:

                    ov_vocs_recorder_talk_off_loop(
                        vocs->recorder, user, role, loop);

                    return ov_mc_frontend_talk(
                        vocs->frontend, uuid, sess, false, data);

                    break;

                case OV_VOCS_SEND:
                    break;
            }

            break;
    }

error:
    return false;
}

/*----------------------------------------------------------------------------*/

/*
static ov_participation_state permission_to_participation_state(
    ov_vocs_permission permission) {

    switch (permission) {

        case OV_VOCS_RECV:
            return OV_PARTICIPATION_STATE_RECV;

        case OV_VOCS_SEND:
            return OV_PARTICIPATION_STATE_SEND;

        default:
            return OV_PARTICIPATION_STATE_NONE;
    };
}
*/

/*----------------------------------------------------------------------------*/

static bool set_loop_state_in_data(ov_json_value *data,
                                   const char *loop,
                                   ov_vocs_permission permission) {

    ov_json_value *loops = ov_json_object_get(data, OV_KEY_LOOPS);
    if (!loops) {
        loops = ov_json_object();
        ov_json_object_set(data, OV_KEY_LOOPS, loops);
    }

    ov_json_object_set(
        loops, loop, ov_json_string(ov_vocs_permission_to_string(permission)));
    return true;
}

/*----------------------------------------------------------------------------*/

static void cb_backend_mixer_join(void *userdata,
                                  const char *uuid,
                                  const char *session_id,
                                  const char *loopname,
                                  uint64_t error_code,
                                  const char *error_desc) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *orig = NULL;
    ov_json_value *data = NULL;

    ov_vocs *vocs = ov_vocs_cast(userdata);
    if (!vocs || !loopname || !uuid || !session_id) goto error;

    ov_event_async_data adata = ov_event_async_unset(vocs->async, uuid);
    orig = adata.value;

    if (!orig) goto switch_off_loop;

    intptr_t socket = (intptr_t)ov_dict_get(vocs->sessions, session_id);
    data = ov_socket_json_get(vocs->connections, socket);

    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));
    const char *role = ov_json_string_get(ov_json_get(data, "/" OV_KEY_ROLE));
    const char *client =
        ov_json_string_get(ov_json_get(data, "/" OV_KEY_CLIENT));

    switch (error_code) {

        case OV_ERROR_NOERROR:
            break;

        default:

            send_error_response(vocs, orig, socket, error_code, error_desc);

            goto error;
            break;
    }

    const char *state = ov_json_string_get(
        ov_json_get(orig, "/" OV_KEY_PARAMETER "/" OV_KEY_STATE));

    ov_vocs_permission request = ov_vocs_permission_from_string(state);
    ov_vocs_permission current = OV_VOCS_RECV;

    if (!set_loop_state_in_data(data, loopname, current)) goto drop;
    if (!ov_vocs_db_set_state(vocs->config.db, user, role, loopname, current))
        goto drop;

    ov_vocs_loop *loop = ov_dict_get(vocs->loops, loopname);
    if (!loop) {

        loop = ov_vocs_loop_create(loopname);
        char *name = ov_string_dup(loopname);

        if (!ov_dict_set(vocs->loops, name, loop, NULL)) {
            loop = ov_vocs_loop_free(loop);
            name = ov_data_pointer_free(name);
            goto drop;
        }
    }

    if (!ov_vocs_loop_add_participant(loop, socket, client, user, role))
        goto drop;

    // ensure loop broadcast is registered
    if (!ov_broadcast_registry_set(
            vocs->broadcasts, loopname, socket, OV_LOOP_BROADCAST))
        goto drop;

    send_switch_loop_broadcast(vocs, socket, loopname, current);

    if (current != request) {

        OV_ASSERT(request == OV_VOCS_SEND);

        if (!ov_event_async_set(
                vocs->async,
                uuid,
                (ov_event_async_data){.socket = socket,
                                      .value = orig,
                                      .timedout.userdata = vocs,
                                      .timedout.callback = async_timedout},
                vocs->config.timeout.response_usec)) {

            ov_log_error("failed to reset async");
            OV_ASSERT(1 == 0);
            goto drop;
        }

        orig = NULL;

        if (!perform_switch_loop_request(vocs,
                                         uuid,
                                         session_id,
                                         user,
                                         role,
                                         loopname,
                                         current,
                                         request)) {

            ov_log_error("Failed to perform talk switch %s session %s|%s|%s",
                         loopname,
                         session_id,
                         user,
                         role);

            goto drop;
        }

    } else {

        out = ov_json_object();

        ov_json_value *participants = ov_vocs_loop_get_participants(loop);
        if (!ov_json_object_set(out, OV_KEY_PARTICIPANTS, participants))
            goto drop;

        val = ov_json_string(ov_vocs_permission_to_string(current));
        if (!ov_json_object_set(out, OV_KEY_STATE, val)) goto drop;

        val = ov_json_string(loopname);
        if (!ov_json_object_set(out, OV_KEY_LOOP, val)) goto drop;

        val = NULL;

        send_switch_loop_user_broadcast(vocs, socket, loopname, current);

        if (!send_success_response(vocs, orig, socket, &out)) goto drop;
    }

    ov_socket_json_set(vocs->connections, socket, &data);
    orig = ov_json_value_free(orig);
    out = ov_json_value_free(out);

    return;

switch_off_loop:
    ov_mc_backend_leave_loop(vocs->backend,
                             uuid,
                             session_id,
                             loopname,
                             vocs,
                             cb_backend_mixer_leave);
    goto error;

drop:
    drop_connection(vocs, socket, true, true);

error:
    out = ov_json_value_free(out);
    val = ov_json_value_free(val);
    orig = ov_json_value_free(orig);
    data = ov_json_value_free(data);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_backend_mixer_leave(void *userdata,
                                   const char *uuid,
                                   const char *session_id,
                                   const char *loopname,
                                   uint64_t error_code,
                                   const char *error_desc) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *orig = NULL;
    ov_json_value *data = NULL;

    ov_vocs *vocs = ov_vocs_cast(userdata);
    if (!vocs || !loopname || !uuid || !session_id) goto error;

    ov_event_async_data adata = ov_event_async_unset(vocs->async, uuid);
    orig = adata.value;

    intptr_t socket = (intptr_t)ov_dict_get(vocs->sessions, session_id);
    if (0 == socket) goto error;

    data = ov_socket_json_get(vocs->connections, socket);
    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));
    const char *role = ov_json_string_get(ov_json_get(data, "/" OV_KEY_ROLE));

    if (!orig) goto drop;

    switch (error_code) {

        case OV_ERROR_NOERROR:
            break;

        default:

            send_error_response(vocs, orig, socket, error_code, error_desc);

            goto error;
            break;
    }

    if (!set_loop_state_in_data(data, loopname, OV_VOCS_NONE)) goto drop;
    if (!ov_vocs_db_set_state(
            vocs->config.db, user, role, loopname, OV_VOCS_NONE))
        goto drop;

    if (!ov_broadcast_registry_set(
            vocs->broadcasts, loopname, socket, OV_BROADCAST_UNSET))
        goto drop;

    ov_vocs_loop *loop = ov_dict_get(vocs->loops, loopname);
    OV_ASSERT(loop);

    ov_vocs_loop_drop_participant(loop, socket);

    send_switch_loop_broadcast(vocs, socket, loopname, OV_VOCS_NONE);

    out = ov_json_object();

    ov_json_value *participants = ov_vocs_loop_get_participants(loop);
    if (!ov_json_object_set(out, OV_KEY_PARTICIPANTS, participants)) goto drop;

    val = ov_json_string(ov_vocs_permission_to_string(OV_VOCS_NONE));
    if (!ov_json_object_set(out, OV_KEY_STATE, val)) goto drop;

    val = ov_json_string(loopname);
    if (!ov_json_object_set(out, OV_KEY_LOOP, val)) goto drop;

    val = NULL;

    send_switch_loop_user_broadcast(vocs, socket, loopname, OV_VOCS_NONE);

    if (!send_success_response(vocs, orig, socket, &out)) goto drop;

    if (!ov_socket_json_set(vocs->connections, socket, &data)) goto error;

    ov_json_value_free(out);
    ov_json_value_free(orig);
    return;

drop:
    drop_connection(vocs, socket, true, true);
error:
    ov_json_value_free(val);
    ov_json_value_free(out);
    orig = ov_json_value_free(orig);
    data = ov_json_value_free(data);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_backend_mixer_volume(void *userdata,
                                    const char *uuid,
                                    const char *session_id,
                                    const char *loopname,
                                    uint8_t volume,
                                    uint64_t error_code,
                                    const char *error_desc) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *orig = NULL;
    ov_json_value *data = NULL;

    ov_vocs *vocs = ov_vocs_cast(userdata);
    if (!vocs || !loopname || !uuid || !session_id) goto error;

    ov_event_async_data adata = ov_event_async_unset(vocs->async, uuid);
    orig = adata.value;

    volume = ov_convert_to_vol_percent(volume, 3);

    intptr_t socket = (intptr_t)ov_dict_get(vocs->sessions, session_id);
    if (0 == socket) goto error;

    data = ov_socket_json_get(vocs->connections, socket);
    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));
    const char *role = ov_json_string_get(ov_json_get(data, "/" OV_KEY_ROLE));

    if (!orig) goto drop;

    switch (error_code) {

        case OV_ERROR_NOERROR:
            break;

        default:

            send_error_response(vocs, orig, socket, error_code, error_desc);

            goto error;
            break;
    }

    ov_vocs_db_set_volume(vocs->config.db, user, role, loopname, volume);

    out = ov_json_object();
    val = ov_json_number(volume);

    if (!ov_json_object_set(out, OV_KEY_VOLUME, val)) goto drop;

    val = ov_json_string(loopname);
    if (!ov_json_object_set(out, OV_KEY_LOOP, val)) goto drop;

    send_success_response(vocs, orig, socket, &out);

    out = ov_json_value_free(out);
    orig = ov_json_value_free(orig);
    data = ov_json_value_free(data);
    return;

drop:
    drop_connection(vocs, socket, true, true);
error:
    ov_json_value_free(val);
    ov_json_value_free(out);
    orig = ov_json_value_free(orig);
    data = ov_json_value_free(data);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_backend_mixer_state(void *userdata,
                                   const char *uuid,
                                   const ov_json_value *state) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *orig = NULL;

    ov_vocs *vocs = ov_vocs_cast(userdata);
    if (!vocs || !state) goto error;

    ov_event_async_data adata = ov_event_async_unset(vocs->async, uuid);
    orig = adata.value;

    if (!orig) goto error;

    if (!ov_json_value_copy((void **)&val, state)) goto error;

    if (!ov_json_object_set(
            ov_event_api_get_response(orig), OV_KEY_BACKEND, val))
        goto error;

    env_send(vocs, adata.socket, orig);

error:
    ov_json_value_free(val);
    ov_json_value_free(out);
    orig = ov_json_value_free(orig);
    return;
}

/*----------------------------------------------------------------------------*/

static bool module_load_backend(ov_vocs *self) {

    OV_ASSERT(self);

    self->config.module.backend.io = self->config.io;
    self->config.module.backend.loop = self->config.loop;
    self->config.module.backend.callback.userdata = self;
    self->config.module.backend.callback.mixer.lost = cb_backend_mixer_lost;

    self->backend = ov_mc_backend_create(self->config.module.backend);
    if (!self->backend) return false;

    return true;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      #CLIENT EVENTS
 *
 *      ------------------------------------------------------------------------
 */

static bool cb_client_process(void *userdata,
                              const int socket,
                              const ov_event_parameter *params,
                              ov_json_value *input) {

    bool (*function)(ov_vocs *vocs,
                     int socket,
                     const ov_event_parameter *params,
                     ov_json_value *input) = NULL;

    ov_vocs *self = ov_vocs_cast(userdata);
    if (!self || (0 > socket) || !input) {
        goto error;
    }

    const char *event = ov_event_api_get_event(input);

    function = ov_dict_get(self->io, event);

    if (function) {

        /* Execute function and let function context decide for socket close */

        return function(self, socket, params, input);
    }

    ov_log_debug("Websocket IO %s at %i event %s unsupported\n",
                 params->uri.name,
                 socket,
                 event);

    /* Close connection socket with false as return value */

error:
    ov_json_value_free(input);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool client_logout(ov_vocs *vocs,
                          int socket,
                          const ov_event_parameter *params,
                          ov_json_value *input) {

    ov_json_value *data = NULL;

    bool result = false;
    UNUSED(params);

    if (!vocs || !input || socket < 0) goto error;

    data = ov_socket_json_get(vocs->connections, socket);
    const char *session =
        ov_json_string_get(ov_json_get(data, "/" OV_KEY_SESSION));
    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));
    const char *client =
        ov_json_string_get(ov_json_get(data, "/" OV_KEY_CLIENT));

    send_success_response(vocs, input, socket, NULL);

    ov_log_info("client logout %i|%s user %s", socket, session, user);

    ov_event_session_delete(vocs->user_sessions, client);

    /* Close connection socket if drop is not successfull */
    result = drop_connection(vocs, socket, true, true);
    data = ov_json_value_free(data);

error:
    ov_json_value_free(input);
    ov_json_value_free(data);
    return result;
}

/*----------------------------------------------------------------------------*/

static void cb_ldap_authentication(void *userdata,
                                   const char *uuid,
                                   ov_ldap_auth_result result) {

    ov_json_value *out = NULL;
    ov_json_value *data = NULL;

    ov_event_async_data adata = {0};

    ov_vocs *vocs = ov_vocs_cast(userdata);
    OV_ASSERT(vocs);
    if (!vocs) goto error;

    adata = ov_event_async_unset(vocs->async, uuid);
    if (!adata.value) goto error;

    data = ov_socket_json_get(vocs->connections, adata.socket);

    const char *client_id =
        ov_json_string_get(ov_json_object_get(adata.value, OV_KEY_CLIENT));

    const char *user = ov_json_string_get(
        ov_json_get(adata.value, "/" OV_KEY_PARAMETER "/" OV_KEY_USER));

    const char *session_id = NULL;

    switch (result) {

        case OV_LDAP_AUTH_REJECTED:

            ov_log_error(
                "LDAP AUTHENTICATE failed at %i | %s", adata.socket, user);

            send_error_response(vocs,
                                adata.value,
                                adata.socket,
                                OV_ERROR_CODE_AUTH,
                                OV_ERROR_DESC_AUTH);

            drop_connection(vocs, adata.socket, true, true);
            goto error;
            break;

        case OV_LDAP_AUTH_GRANTED:
            ov_log_info(
                "LDAP AUTHENTICATE granted at %i | %s", adata.socket, user);
            break;
    }

    session_id = ov_event_session_init(vocs->user_sessions, client_id, user);

    /* successful authenticated */

    if (!ov_broadcast_registry_set(
            vocs->broadcasts, user, adata.socket, OV_USER_BROADCAST)) {
        goto error;
    }

    if (!ov_broadcast_registry_set(vocs->broadcasts,
                                   OV_BROADCAST_KEY_SYSTEM_BROADCAST,
                                   adata.socket,
                                   OV_SYSTEM_BROADCAST)) {
        goto error;
    }

    ov_json_object_set(data, OV_KEY_CLIENT, ov_json_string(client_id));
    ov_json_object_set(data, OV_KEY_USER, ov_json_string(user));

    ov_socket_json_set(vocs->connections, adata.socket, &data);

    out = ov_json_object();
    if (!ov_vocs_json_set_id(out, user)) goto error;
    if (!ov_vocs_json_set_session_id(out, session_id)) goto error;

    result = send_success_response(vocs, adata.value, adata.socket, &out);
    if (result) {
        ov_log_info("VOCS AUTHENTICATE LDAP at %i | %s", adata.socket, user);
    } else {
        ov_log_error(
            "VOCS AUTHENTICATE LDAP failed at %i | %s", adata.socket, user);
    }

error:
    ov_json_value_free(out);
    ov_event_async_data_clear(&adata);
    ov_json_value_free(data);
    return;
}

/*----------------------------------------------------------------------------*/

static bool client_login(ov_vocs *vocs,
                         int socket,
                         const ov_event_parameter *params,
                         ov_json_value *input) {

    ov_json_value *data = NULL;
    ov_json_value *out = NULL;

    bool result = false;

    UNUSED(params);

    if (!vocs || !input || socket < 0) goto error;

    data = ov_socket_json_get(vocs->connections, socket);
    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));

    if (user) {

        send_error_response(vocs,
                            input,
                            socket,
                            OV_ERROR_CODE_ALREADY_AUTHENTICATED,
                            OV_ERROR_DESC_ALREADY_AUTHENTICATED);

        goto error;
    }

    const char *session_id = NULL;
    const char *pass = NULL;

    const char *uuid = ov_event_api_get_uuid(input);
    if (!uuid) goto error;

    const char *client_id =
        ov_json_string_get(ov_json_object_get(input, OV_KEY_CLIENT));

    const char *session_user =
        ov_event_session_get_user(vocs->user_sessions, client_id);

    user = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_USER));
    pass = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_PASSWORD));

    if (session_user && user && pass) {

        if (ov_event_session_verify(
                vocs->user_sessions, client_id, user, pass)) {

            ov_log_debug("relogin session for client %s", client_id);

            goto login_session_user;
        }
    }

    if (!user || !pass) {

        send_error_response(vocs,
                            input,
                            socket,
                            OV_ERROR_CODE_PARAMETER_ERROR,
                            OV_ERROR_DESC_PARAMETER_ERROR);

        goto error;
    }

    if (vocs->config.ldap.enable) {

        ov_log_debug("Requesting LDAP authentication for user %s", user);

        if (!ov_event_async_set(
                vocs->async,
                uuid,
                (ov_event_async_data){.socket = socket,
                                      .value = input,
                                      .timedout.userdata = vocs,
                                      .timedout.callback = async_timedout},
                vocs->config.timeout.response_usec)) {

            char *str = ov_json_value_to_string(input);
            ov_log_error(
                "failed to set async msg - closing %i | %s", socket, str);
            str = ov_data_pointer_free(str);

            goto error;
        }

        return ov_ldap_authenticate_password(
            vocs->ldap,
            user,
            pass,
            uuid,
            (ov_ldap_auth_callback){
                .userdata = vocs, .callback = cb_ldap_authentication});
    }

    if (!ov_vocs_db_authenticate(vocs->config.db, user, pass)) {

        send_error_response(
            vocs, input, socket, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        ov_log_error("VOCS AUTHENTICATE local failed at %i | %s", socket, user);
        goto error;
    }

login_session_user:

    session_id = ov_event_session_init(vocs->user_sessions, client_id, user);

    /* successful authenticated */

    if (!ov_broadcast_registry_set(
            vocs->broadcasts, user, socket, OV_USER_BROADCAST)) {
        goto error;
    }

    if (!ov_broadcast_registry_set(vocs->broadcasts,
                                   OV_BROADCAST_KEY_SYSTEM_BROADCAST,
                                   socket,
                                   OV_SYSTEM_BROADCAST)) {
        goto error;
    }

    ov_json_object_set(data, OV_KEY_CLIENT, ov_json_string(client_id));
    ov_json_object_set(data, OV_KEY_USER, ov_json_string(user));

    ov_socket_json_set(vocs->connections, socket, &data);

    out = ov_json_object();
    if (!ov_vocs_json_set_id(out, user)) goto error;
    if (!ov_vocs_json_set_session_id(out, session_id)) goto error;

    result = send_success_response(vocs, input, socket, &out);
    if (result) {
        ov_log_info("VOCS AUTHENTICATE local at %i | %s", socket, user);
    } else {
        ov_log_error("VOCS AUTHENTICATE local failed at %i | %s", socket, user);
    }

    /* close socket if authentication messaging failed */
error:
    ov_json_value_free(out);
    ov_json_value_free(input);
    ov_json_value_free(data);
    return result;
}

/*----------------------------------------------------------------------------*/

static bool update_client_login(ov_vocs *vocs,
                                int socket,
                                const ov_event_parameter *params,
                                ov_json_value *input) {

    bool result = false;
    ov_json_value *out = NULL;

    UNUSED(params);

    if (!vocs || !input || socket < 0) goto error;

    const char *client_id =
        ov_json_string_get(ov_json_object_get(input, OV_KEY_CLIENT));

    const char *session_id = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_SESSION));

    const char *user_id = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_USER));

    if (!client_id || !session_id || !user_id) {

        send_error_response(vocs,
                            input,
                            socket,
                            OV_ERROR_CODE_PARAMETER_ERROR,
                            OV_ERROR_DESC_PARAMETER_ERROR);

        goto error;
    }

    if (!ov_event_session_update(
            vocs->user_sessions, client_id, user_id, session_id)) {

        send_error_response(vocs,
                            input,
                            socket,
                            OV_ERROR_CODE_PROCESSING_ERROR,
                            OV_ERROR_DESC_PROCESSING_ERROR);

        goto error;
    }

    out = ov_json_object();
    if (!ov_vocs_json_set_id(
            out, ov_event_session_get_user(vocs->user_sessions, client_id)))
        goto error;

    if (!ov_vocs_json_set_session_id(out, session_id)) goto error;

    result = send_success_response(vocs, input, socket, &out);
    return true;

    /* close socket if authentication messaging failed */
error:
    ov_json_value_free(out);
    ov_json_value_free(input);
    return result;
}

/*----------------------------------------------------------------------------*/

static bool client_media(ov_vocs *vocs,
                         int socket,
                         const ov_event_parameter *params,
                         ov_json_value *input) {

    UNUSED(params);
    ov_json_value *data = NULL;

    if (!vocs || !input || socket < 0) goto error;

    data = ov_socket_json_get(vocs->connections, socket);
    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));
    const char *session =
        ov_json_string_get(ov_json_get(data, "/" OV_KEY_SESSION));

    if (!user) goto error;

    const char *uuid = ov_event_api_get_uuid(input);
    const char *sdp = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_SDP));

    ov_media_type type = ov_media_type_from_string(ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_TYPE)));

    if (!uuid) {

        send_error_response(vocs,
                            input,
                            socket,
                            OV_ERROR_CODE_PARAMETER_ERROR,
                            OV_ERROR_DESC_PARAMETER_ERROR);

        goto error;
    }

    switch (type) {

        case OV_MEDIA_REQUEST:

            if (!ov_mc_frontend_create_session(
                    vocs->frontend, uuid, OV_VOCS_DEFAULT_SDP)) {

                send_error_response(vocs,
                                    input,
                                    socket,
                                    OV_ERROR_CODE_PROCESSING_ERROR,
                                    OV_ERROR_DESC_PROCESSING_ERROR);

                goto error;
            }

            break;

        case OV_MEDIA_OFFER:
        case OV_MEDIA_ANSWER:

            if (!sdp) {

                send_error_response(vocs,
                                    input,
                                    socket,
                                    OV_ERROR_CODE_PARAMETER_ERROR,
                                    OV_ERROR_DESC_PARAMETER_ERROR);

                goto error;
            }

            if (!session) {

                send_error_response(vocs,
                                    input,
                                    socket,
                                    OV_ERROR_CODE_SESSION_UNKNOWN,
                                    OV_ERROR_DESC_SESSION_UNKNOWN);

                goto error;
            }

            if (!ov_mc_frontend_update_session(
                    vocs->frontend, uuid, session, type, sdp)) {

                send_error_response(vocs,
                                    input,
                                    socket,
                                    OV_ERROR_CODE_PROCESSING_ERROR,
                                    OV_ERROR_DESC_PROCESSING_ERROR);

                goto error;
            }

            break;

        default:

            send_error_response(vocs,
                                input,
                                socket,
                                OV_ERROR_CODE_PARAMETER_ERROR,
                                OV_ERROR_DESC_PARAMETER_ERROR);

            goto error;
    }

    if (!ov_event_async_set(
            vocs->async,
            uuid,
            (ov_event_async_data){.socket = socket,
                                  .value = input,
                                  .timedout.userdata = vocs,
                                  .timedout.callback = async_timedout},
            vocs->config.timeout.response_usec)) {

        char *str = ov_json_value_to_string(input);
        ov_log_error("failed to set async msg - closing %i | %s", socket, str);
        str = ov_data_pointer_free(str);

        goto error;
    }

    input = NULL;
    data = ov_json_value_free(data);
    return true;
error:
    /* let socket close in case of media setup errors */
    ov_json_value_free(input);
    ov_json_value_free(data);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool client_candidate(ov_vocs *vocs,
                             int socket,
                             const ov_event_parameter *params,
                             ov_json_value *input) {

    UNUSED(params);
    ov_json_value *data = NULL;

    if (!vocs || !input || socket < 0) goto error;

    data = ov_socket_json_get(vocs->connections, socket);
    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));
    const char *session =
        ov_json_string_get(ov_json_get(data, "/" OV_KEY_SESSION));

    if (!user) goto error;

    const char *uuid = ov_event_api_get_uuid(input);
    ov_json_value *par = ov_event_api_get_parameter(input);
    ov_ice_candidate_info info = ov_ice_candidate_info_from_json(par);

    if (!uuid || !info.candidate || !info.ufrag) {

        send_error_response(vocs,
                            input,
                            socket,
                            OV_ERROR_CODE_PARAMETER_ERROR,
                            OV_ERROR_DESC_PARAMETER_ERROR);

        goto error;
    }

    if (NULL == session) {

        send_error_response(vocs,
                            input,
                            socket,
                            OV_ERROR_CODE_SESSION_UNKNOWN,
                            OV_ERROR_DESC_SESSION_UNKNOWN);

        goto error;
    }

    if (!ov_mc_frontend_candidate(vocs->frontend, uuid, session, &info)) {

        send_error_response(vocs,
                            input,
                            socket,
                            OV_ERROR_CODE_PROCESSING_ERROR,
                            OV_ERROR_DESC_PROCESSING_ERROR);

        goto error;
    }

    if (!ov_event_async_set(
            vocs->async,
            uuid,
            (ov_event_async_data){.socket = socket,
                                  .value = input,
                                  .timedout.userdata = vocs,
                                  .timedout.callback = async_timedout},
            vocs->config.timeout.response_usec)) {

        char *str = ov_json_value_to_string(input);
        ov_log_error("failed to set async msg - closing %i | %s", socket, str);
        str = ov_data_pointer_free(str);

        goto error;
    }

    data = ov_json_value_free(data);
    return true;

error:
    /* let socket close in case of media setup errors */
    ov_json_value_free(input);
    ov_json_value_free(data);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool client_end_of_candidates(ov_vocs *vocs,
                                     int socket,
                                     const ov_event_parameter *params,
                                     ov_json_value *input) {

    UNUSED(params);
    ov_json_value *data = NULL;

    if (!vocs || !input || socket < 0) goto error;

    data = ov_socket_json_get(vocs->connections, socket);
    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));
    const char *session =
        ov_json_string_get(ov_json_get(data, "/" OV_KEY_SESSION));

    if (!user) goto error;

    const char *uuid = ov_event_api_get_uuid(input);

    if (!uuid) {

        send_error_response(vocs,
                            input,
                            socket,
                            OV_ERROR_CODE_PARAMETER_ERROR,
                            OV_ERROR_DESC_PARAMETER_ERROR);

        goto error;
    }

    if (NULL == session) {

        send_error_response(vocs,
                            input,
                            socket,
                            OV_ERROR_CODE_SESSION_UNKNOWN,
                            OV_ERROR_DESC_SESSION_UNKNOWN);

        goto error;
    }

    if (!ov_mc_frontend_end_of_candidates(vocs->frontend, uuid, session)) {

        send_error_response(vocs,
                            input,
                            socket,
                            OV_ERROR_CODE_PROCESSING_ERROR,
                            OV_ERROR_DESC_PROCESSING_ERROR);

        goto error;
    }

    if (!ov_event_async_set(
            vocs->async,
            uuid,
            (ov_event_async_data){.socket = socket,
                                  .value = input,
                                  .timedout.userdata = vocs,
                                  .timedout.callback = async_timedout},
            vocs->config.timeout.response_usec)) {

        char *str = ov_json_value_to_string(input);
        ov_log_error("failed to set async msg - closing %i | %s", socket, str);
        str = ov_data_pointer_free(str);

        goto error;
    }

    data = ov_json_value_free(data);
    return true;

error:
    /* let socket close in case of media setup errors */
    ov_json_value_free(input);
    ov_json_value_free(data);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool client_authorize(ov_vocs *vocs,
                             int socket,
                             const ov_event_parameter *params,
                             ov_json_value *input) {

    ov_json_value *data = NULL;
    ov_json_value *out = NULL;
    bool result = false;

    UNUSED(params);

    if (!vocs || !input || socket < 0) goto error;

    data = ov_socket_json_get(vocs->connections, socket);
    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));
    const char *role = ov_json_string_get(ov_json_get(data, "/" OV_KEY_ROLE));

    if (!user) goto error;

    if (role) {

        send_error_response(vocs,
                            input,
                            socket,
                            OV_ERROR_CODE_NOT_IMPLEMENTED,
                            "current status: "
                            "changing a role MUST be done using logout/login.");

        goto error;
    }

    role = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_ROLE));

    if (!role) {

        send_error_response(vocs,
                            input,
                            socket,
                            OV_ERROR_CODE_PARAMETER_ERROR,
                            OV_ERROR_DESC_PARAMETER_ERROR);

        goto error;
    }

    /* process authorization request */

    if (!ov_vocs_db_authorize(vocs->config.db, user, role)) {

        send_error_response(
            vocs, input, socket, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto error;
    }

    ov_json_object_set(data, OV_KEY_ROLE, ov_json_string(role));

    if (!ov_broadcast_registry_set(
            vocs->broadcasts, role, socket, OV_ROLE_BROADCAST)) {
        goto error;
    }

    out = ov_json_object();
    if (!ov_vocs_json_set_id(out, role)) goto error;

    result = send_success_response(vocs, input, socket, &out);
    if (result) {
        ov_log_info("VOCS AUTHORIZE at %i | %s | %s", socket, user, role);
    } else {
        ov_log_error("VOCS AUTHORIZE failed at %i | %s", socket, user);
    }

    ov_socket_json_set(vocs->connections, socket, &data);

    /* close socket if authorization messaging failed */
error:
    ov_json_value_free(out);
    ov_json_value_free(input);
    ov_json_value_free(data);
    return result;
}

/*----------------------------------------------------------------------------*/

static bool client_get(ov_vocs *vocs,
                       int socket,
                       const ov_event_parameter *params,
                       ov_json_value *input) {

    bool result = false;
    ov_json_value *val = NULL;
    ov_json_value *out = NULL;
    ov_json_value *data = NULL;

    UNUSED(params);

    if (!vocs || !input || socket < 0) goto error;

    data = ov_socket_json_get(vocs->connections, socket);
    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));

    if (!user) {

        send_error_response(
            vocs, input, socket, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto error;
    }

    ov_json_value *parameter = ov_event_api_get_parameter(input);
    const char *type = ov_event_api_get_type(parameter);

    if (!parameter || !type) {

        send_error_response(vocs,
                            input,
                            socket,
                            OV_ERROR_CODE_PARAMETER_ERROR,
                            OV_ERROR_DESC_PARAMETER_ERROR);

        goto error;
    }

    if (0 == strcmp(type, OV_KEY_USER)) {

        val = ov_vocs_db_get_entity(vocs->config.db, OV_VOCS_DB_USER, user);
        out = ov_json_object();
        if (!ov_json_object_set(out, OV_KEY_RESULT, val)) goto error;

        val = ov_json_string(type);
        if (!ov_json_object_set(out, OV_KEY_TYPE, val)) goto error;

        val = ov_vocs_db_get_entity_domain(
            vocs->config.db, OV_VOCS_DB_USER, user);
        if (val) ov_json_object_set(out, OV_KEY_DOMAIN, val);

        val = NULL;
        result = send_success_response(vocs, input, socket, &out);

    } else {

        send_error_response(vocs,
                            input,
                            socket,
                            OV_ERROR_CODE_NOT_IMPLEMENTED,
                            "only GET user implemented yet.");

        TODO("... to be implemented");
    }

error:
    ov_json_value_free(out);
    ov_json_value_free(val);
    ov_json_value_free(input);
    ov_json_value_free(data);
    return result;
}

/*----------------------------------------------------------------------------*/

static bool client_user_roles(ov_vocs *vocs,
                              int socket,
                              const ov_event_parameter *params,
                              ov_json_value *input) {

    bool result = false;
    ov_json_value *out = NULL;
    ov_json_value *val = NULL;

    ov_json_value *data = NULL;

    UNUSED(params);

    if (!vocs || !input || socket < 0) goto error;

    data = ov_socket_json_get(vocs->connections, socket);
    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));

    if (!user) goto error;

    val = ov_vocs_db_get_user_roles(vocs->config.db, user);

    if (!val) {

        send_error_response(
            vocs, input, socket, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto error;
    }

    out = ov_json_object();
    if (!ov_json_object_set(out, OV_KEY_ROLES, val)) goto error;

    val = NULL;

    result = send_success_response(vocs, input, socket, &out);

error:
    ov_json_value_free(val);
    ov_json_value_free(out);
    ov_json_value_free(input);
    ov_json_value_free(data);
    return result;
}

/*----------------------------------------------------------------------------*/

struct container_loop_broadcasts {

    ov_vocs *vocs;
    int socket;

    const char *project;
    const char *user;
    const char *role;
};

/*----------------------------------------------------------------------------*/

static bool add_loop_broadcast_and_state(const void *key,
                                         void *val,
                                         void *data) {

    if (!key) return true;

    UNUSED(val);

    char *name = (char *)key;
    struct container_loop_broadcasts *c =
        (struct container_loop_broadcasts *)data;

    return ov_broadcast_registry_set(
        c->vocs->broadcasts, name, c->socket, OV_LOOP_BROADCAST);
}

/*----------------------------------------------------------------------------*/

static bool client_role_loops(ov_vocs *vocs,
                              int socket,
                              const ov_event_parameter *params,
                              ov_json_value *input) {

    bool result = false;
    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    UNUSED(params);

    ov_json_value *data = NULL;

    UNUSED(params);

    if (!vocs || !input || socket < 0) goto error;

    data = ov_socket_json_get(vocs->connections, socket);
    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));
    const char *role = ov_json_string_get(ov_json_get(data, "/" OV_KEY_ROLE));

    if (!user || !role) {

        send_error_response(
            vocs, input, socket, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto error;
    }

    val = ov_vocs_db_get_user_role_loops(vocs->config.db, user, role);

    if (!val) {

        send_error_response(
            vocs, input, socket, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto error;
    }

    struct container_loop_broadcasts c = (struct container_loop_broadcasts){
        .vocs = vocs, .socket = socket, .user = user, .role = role};

    if (!ov_json_object_for_each(val, &c, add_loop_broadcast_and_state))
        goto error;

    out = ov_json_object();
    if (!ov_json_object_set(out, OV_KEY_LOOPS, val)) goto error;

    val = NULL;

    result = send_success_response(vocs, input, socket, &out);

error:
    ov_json_value_free(val);
    ov_json_value_free(out);
    ov_json_value_free(input);
    ov_json_value_free(data);
    return result;
}

/*----------------------------------------------------------------------------*/

static bool client_event_get_recording(ov_vocs *vocs,
                                       int socket,
                                       const ov_event_parameter *params,
                                       ov_json_value *input) {

    ov_json_value *val = NULL;
    ov_json_value *data = NULL;

    UNUSED(params);

    if (!vocs || !input || socket < 0) goto error;

    data = ov_socket_json_get(vocs->connections, socket);
    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));
    const char *role = ov_json_string_get(ov_json_get(data, "/" OV_KEY_ROLE));

    if (!user || !role) {

        send_error_response(
            vocs, input, socket, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto error;
    }

    ov_db_recordings_get_params db_params = {0};

    db_params.loop = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_LOOP));
    db_params.user = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_USER));
    db_params.from_epoch_secs = ov_json_number_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_FROM));
    db_params.until_epoch_secs = ov_json_number_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_TO));

    val = ov_vocs_recorder_get_recording(vocs->recorder, db_params);
    if (!val) {

        send_error_response(vocs,
                            input,
                            socket,
                            OV_ERROR_CODE_PROCESSING_ERROR,
                            OV_ERROR_DESC_PROCESSING_ERROR);

    } else if (OV_DB_RECORDINGS_RESULT_TOO_BIG == val) {

        send_error_response(vocs,
                            input,
                            socket,
                            OV_ERROR_CODE_PROCESSING_ERROR,
                            "Search returned too many results - please confine "
                            "your search parameters");

    } else {
        send_success_response(vocs, input, socket, &val);
    }

error:
    ov_json_value_free(val);
    ov_json_value_free(input);
    ov_json_value_free(data);
    return true;
}

/*----------------------------------------------------------------------------*/

static bool client_event_sip_status(ov_vocs *vocs,
                                    int socket,
                                    const ov_event_parameter *params,
                                    ov_json_value *input) {

    ov_json_value *val = NULL;
    ov_json_value *out = NULL;
    ov_json_value *data = NULL;

    UNUSED(params);

    if (!vocs || !input || socket < 0) goto error;

    data = ov_socket_json_get(vocs->connections, socket);
    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));

    if (!user) {

        send_error_response(
            vocs, input, socket, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto error;
    }

    bool status = ov_mc_backend_sip_get_connect_status(vocs->sip);

    out = ov_json_object();

    if (status) {
        val = ov_json_true();
    } else {
        val = ov_json_false();
    }

    ov_json_object_set(out, OV_KEY_CONNECTED, val);

    send_success_response(vocs, input, socket, &out);

error:
    ov_json_value_free(out);
    ov_json_value_free(input);
    ov_json_value_free(data);
    return true;
}

/*----------------------------------------------------------------------------*/

static bool client_switch_loop_state(ov_vocs *vocs,
                                     int socket,
                                     const ov_event_parameter *params,
                                     ov_json_value *input) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;

    ov_json_value *data = NULL;

    UNUSED(params);

    if (!vocs || !input || socket < 0) goto error;

    data = ov_socket_json_get(vocs->connections, socket);
    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));
    const char *sess =
        ov_json_string_get(ov_json_get(data, "/" OV_KEY_SESSION));
    const char *role = ov_json_string_get(ov_json_get(data, "/" OV_KEY_ROLE));

    if (!user || !role) {

        send_error_response(
            vocs, input, socket, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto error;
    }

    if (NULL == sess) {

        send_error_response(vocs,
                            input,
                            socket,
                            OV_ERROR_CODE_SESSION_UNKNOWN,
                            OV_ERROR_DESC_SESSION_UNKNOWN);

        goto error;
    }

    const char *uuid = ov_event_api_get_uuid(input);
    const char *loop = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_LOOP));
    const char *state = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_STATE));

    if (!uuid || !loop || !state) {

        send_error_response(vocs,
                            input,
                            socket,
                            OV_ERROR_CODE_PARAMETER_ERROR,
                            OV_ERROR_DESC_PARAMETER_ERROR);

        goto error;
    }

    if (!(ov_json_is_true(ov_json_get(data, "/" OV_KEY_ICE)) &&
          (ov_json_is_true(ov_json_get(data, "/" OV_KEY_MEDIA_READY)))))
        goto error;

    ov_vocs_permission requested = ov_vocs_permission_from_string(state);
    ov_vocs_permission permission =
        ov_vocs_db_get_permission(vocs->config.db, role, loop);

    if (!ov_vocs_permission_granted(permission, requested)) {

        send_error_response(vocs,
                            input,
                            socket,
                            OV_ERROR_CODE_AUTH_PERMISSION,
                            OV_ERROR_DESC_AUTH_PERMISSION);

        goto error;
    }

    const ov_json_value *loops = ov_json_get(data, "/" OV_KEY_LOOPS);

    ov_vocs_permission current = ov_vocs_permission_from_string(
        ov_json_string_get(ov_json_object_get(loops, loop)));

    if (current == requested) {

        ov_vocs_loop *l = ov_dict_get(vocs->loops, loop);

        if (!l) {

            l = ov_vocs_loop_create(loop);
            char *name = ov_string_dup(loop);

            if (!ov_dict_set(vocs->loops, name, l, NULL)) {
                l = ov_vocs_loop_free(l);
                name = ov_data_pointer_free(name);
                goto error;
            }
        }

        ov_json_value *participants = ov_vocs_loop_get_participants(l);

        out = ov_json_object();

        if (!ov_json_object_set(out, OV_KEY_PARTICIPANTS, participants))
            goto error;

        val = ov_json_string(ov_vocs_permission_to_string(current));
        if (!ov_json_object_set(out, OV_KEY_STATE, val)) goto error;

        val = ov_json_string(loop);
        if (!ov_json_object_set(out, OV_KEY_LOOP, val)) goto error;

        val = NULL;

        send_success_response(vocs, input, socket, &out);

        out = ov_json_value_free(out);
        input = ov_json_value_free(input);

        goto done;
    }

    if (!perform_switch_loop_request(
            vocs, uuid, sess, user, role, loop, current, requested)) {

        send_error_response(vocs,
                            input,
                            socket,
                            OV_ERROR_CODE_PROCESSING_ERROR,
                            OV_ERROR_DESC_PROCESSING_ERROR);

        goto error;
    }

    /* we requested either some loop aquisition or switch loop */

    if (!ov_event_async_set(
            vocs->async,
            uuid,
            (ov_event_async_data){.socket = socket,
                                  .value = input,
                                  .timedout.userdata = vocs,
                                  .timedout.callback = async_timedout},
            vocs->config.timeout.response_usec)) {

        char *str = ov_json_value_to_string(input);
        ov_log_error("failed to set async msg - closing %i | %s", socket, str);
        str = ov_data_pointer_free(str);

        goto error;
    }

done:
    data = ov_json_value_free(data);
    return true;

error:
    /* let socket close in case of switch media errors */
    ov_json_value_free(input);
    ov_json_value_free(data);
    out = ov_json_value_free(out);
    val = ov_json_value_free(val);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool client_switch_loop_volume(ov_vocs *vocs,
                                      int socket,
                                      const ov_event_parameter *params,
                                      ov_json_value *input) {

    UNUSED(params);
    ov_json_value *data = NULL;

    if (!vocs || !input || socket < 0) goto error;

    data = ov_socket_json_get(vocs->connections, socket);

    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));
    const char *role = ov_json_string_get(ov_json_get(data, "/" OV_KEY_ROLE));
    const char *sess =
        ov_json_string_get(ov_json_get(data, "/" OV_KEY_SESSION));

    if (!user || !role) {

        send_error_response(
            vocs, input, socket, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto error;
    }

    const char *uuid = ov_event_api_get_uuid(input);
    const char *loop = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_LOOP));
    uint64_t percent = ov_json_number_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_VOLUME));

    if (!uuid || !loop || (percent > 100)) {

        send_error_response(vocs,
                            input,
                            socket,
                            OV_ERROR_CODE_PARAMETER_ERROR,
                            OV_ERROR_DESC_PARAMETER_ERROR);

        goto error;
    }

    if (NULL == sess) {

        send_error_response(vocs,
                            input,
                            socket,
                            OV_ERROR_CODE_SESSION_UNKNOWN,
                            OV_ERROR_DESC_SESSION_UNKNOWN);

        goto error;
    }

    if (!ov_mc_backend_set_loop_volume(vocs->backend,
                                       uuid,
                                       sess,
                                       loop,
                                       percent,
                                       vocs,
                                       cb_backend_mixer_volume)) {

        send_error_response(vocs,
                            input,
                            socket,
                            OV_ERROR_CODE_PROCESSING_ERROR,
                            OV_ERROR_DESC_PROCESSING_ERROR);

        goto error;
    }

    if (!ov_event_async_set(
            vocs->async,
            uuid,
            (ov_event_async_data){.socket = socket,
                                  .value = input,
                                  .timedout.userdata = vocs,
                                  .timedout.callback = async_timedout},
            vocs->config.timeout.response_usec)) {

        char *str = ov_json_value_to_string(input);
        ov_log_error("failed to set async msg - closing %i | %s", socket, str);
        str = ov_data_pointer_free(str);

        goto error;
    }

    send_switch_volume_user_broadcast(vocs, socket, loop, percent);

    ov_json_value_free(data);
    return true;
error:
    /* let socket close in case of switch media errors */
    ov_json_value_free(input);
    ov_json_value_free(data);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool client_talking(ov_vocs *vocs,
                           int socket,
                           const ov_event_parameter *params,
                           ov_json_value *input) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *data = NULL;

    UNUSED(params);

    if (!vocs || !input || socket < 0) goto error;

    data = ov_socket_json_get(vocs->connections, socket);

    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));
    const char *role = ov_json_string_get(ov_json_get(data, "/" OV_KEY_ROLE));

    if (!user || !role) {

        send_error_response(
            vocs, input, socket, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto error;
    }

    const char *uuid = ov_event_api_get_uuid(input);
    const char *loop = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_LOOP));
    ov_json_value const *state =
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_STATE);

    bool off = ov_json_is_false(state);

    const char *client_id =
        ov_json_string_get(ov_json_get(input, "/" OV_KEY_CLIENT));

    ov_vocs_recorder_ptt(vocs->recorder, user, role, loop, off);

    if (!uuid || !loop || !state) {

        send_error_response(vocs,
                            input,
                            socket,
                            OV_ERROR_CODE_PARAMETER_ERROR,
                            OV_ERROR_DESC_PARAMETER_ERROR);

        goto error;
    }

    ov_vocs_permission permission =
        ov_vocs_db_get_permission(vocs->config.db, role, loop);

    if (permission != OV_VOCS_SEND) {

        send_error_response(vocs,
                            input,
                            socket,
                            OV_ERROR_CODE_AUTH_PERMISSION,
                            OV_ERROR_DESC_AUTH_PERMISSION);

        goto error;
    }

    out = ov_json_object();
    val = ov_json_string(user);
    if (!ov_json_object_set(out, OV_KEY_USER, val)) goto error;

    val = ov_json_string(role);
    if (!ov_json_object_set(out, OV_KEY_ROLE, val)) goto error;

    val = ov_json_string(loop);
    if (!ov_json_object_set(out, OV_KEY_LOOP, val)) goto error;

    val = NULL;
    if (!ov_json_value_copy((void **)&val, state)) goto error;

    if (!ov_json_object_set(out, OV_KEY_STATE, val)) goto error;

    val = NULL;

    send_success_response(vocs, input, socket, &out);

    if (!send_talking_loop_broadcast(
            vocs, socket, params, loop, state, client_id)) {

        ov_log_error(
            "Failed to send talking broadcast at %s from %s", loop, user);
    }

error:
    /* Do not close socket in case of broadcasting errors */
    ov_json_value_free(val);
    ov_json_value_free(out);
    ov_json_value_free(input);
    ov_json_value_free(data);
    return true;
}

/*----------------------------------------------------------------------------*/

static bool client_calling(ov_vocs *vocs,
                           int socket,
                           const ov_event_parameter *params,
                           ov_json_value *input) {

    UNUSED(params);

    ov_json_value *data = NULL;

    char *uuid_request = NULL;

    if (!vocs || !input || socket < 0) goto error;

    data = ov_socket_json_get(vocs->connections, socket);

    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));
    const char *role = ov_json_string_get(ov_json_get(data, "/" OV_KEY_ROLE));

    if (!user || !role) {

        send_error_response(
            vocs, input, socket, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto error;
    }

    const char *uuid = ov_event_api_get_uuid(input);
    const char *loop = ov_json_string_get(
        (ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_LOOP)));
    const char *dest = ov_json_string_get(
        (ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_DESTINATION)));
    const char *from = ov_json_string_get(
        (ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_FROM)));

    if (!uuid || !loop || !dest) {

        send_error_response(vocs,
                            input,
                            socket,
                            OV_ERROR_CODE_PARAMETER_ERROR,
                            OV_ERROR_DESC_PARAMETER_ERROR);

        goto error;
    }

    if (!ov_vocs_db_sip_allow_callout(vocs->config.db, loop, role)) {

        send_error_response(
            vocs, input, socket, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto error;
    }

    uuid_request = ov_mc_backend_sip_create_call(vocs->sip, loop, dest, from);

    if (!uuid_request) {

        send_error_response(vocs,
                            input,
                            socket,
                            OV_ERROR_CODE_PROCESSING_ERROR,
                            OV_ERROR_DESC_PROCESSING_ERROR);
        goto error;
    }

    if (!ov_event_async_set(
            vocs->async,
            uuid_request,
            (ov_event_async_data){.socket = socket,
                                  .value = input,
                                  .timedout.userdata = vocs,
                                  .timedout.callback = async_timedout},
            vocs->config.timeout.response_usec)) {

        ov_log_error("failed to reset async");
        OV_ASSERT(1 == 0);
        goto error;
    }

    input = NULL;

error:
    /* Do not close socket in case of call errors */
    ov_json_value_free(input);
    ov_json_value_free(data);
    uuid_request = ov_data_pointer_free(uuid_request);
    return true;
}

/*----------------------------------------------------------------------------*/

static bool client_hangup(ov_vocs *vocs,
                          int socket,
                          const ov_event_parameter *params,
                          ov_json_value *input) {

    UNUSED(params);

    ov_json_value *data = NULL;

    if (!vocs || !input || socket < 0) goto error;

    data = ov_socket_json_get(vocs->connections, socket);

    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));
    const char *role = ov_json_string_get(ov_json_get(data, "/" OV_KEY_ROLE));

    if (!user || !role) {

        send_error_response(
            vocs, input, socket, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto error;
    }

    const char *uuid = ov_event_api_get_uuid(input);
    const char *call_id = ov_json_string_get(
        (ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_CALL)));
    const char *loop = ov_json_string_get(
        (ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_LOOP)));

    if (!uuid || !call_id) {

        send_error_response(vocs,
                            input,
                            socket,
                            OV_ERROR_CODE_PARAMETER_ERROR,
                            OV_ERROR_DESC_PARAMETER_ERROR);

        goto error;
    }

    if (!ov_vocs_db_sip_allow_callend(vocs->config.db, loop, role)) {

        send_error_response(
            vocs, input, socket, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto error;
    }

    if (ov_mc_backend_sip_terminate_call(vocs->sip, call_id)) {

        send_success_response(vocs, input, socket, NULL);

    } else {

        send_error_response(vocs,
                            input,
                            socket,
                            OV_ERROR_CODE_PROCESSING_ERROR,
                            OV_ERROR_DESC_PROCESSING_ERROR);
    }

error:
    /* Do not close socket in case of call errors */
    ov_json_value_free(input);
    ov_json_value_free(data);
    return true;
}

/*----------------------------------------------------------------------------*/

static bool client_permit_call(ov_vocs *vocs,
                               int socket,
                               const ov_event_parameter *params,
                               ov_json_value *input) {

    UNUSED(params);

    ov_json_value *data = NULL;

    if (!vocs || !input || socket < 0) goto error;

    data = ov_socket_json_get(vocs->connections, socket);

    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));
    const char *role = ov_json_string_get(ov_json_get(data, "/" OV_KEY_ROLE));

    if (!user || !role) {

        send_error_response(
            vocs, input, socket, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto error;
    }

    bool ok = true;

    const char *uuid = ov_event_api_get_uuid(input);
    ov_sip_permission permission = ov_sip_permission_from_json(
        ov_json_get(input, "/" OV_KEY_PARAMETER), &ok);

    if (!uuid || !ok) {

        send_error_response(vocs,
                            input,
                            socket,
                            OV_ERROR_CODE_PARAMETER_ERROR,
                            OV_ERROR_DESC_PARAMETER_ERROR);

        goto error;
    }

    if (ov_mc_backend_sip_create_permission(vocs->sip, permission)) {

        send_success_response(vocs, input, socket, NULL);

    } else {

        send_error_response(vocs,
                            input,
                            socket,
                            OV_ERROR_CODE_PROCESSING_ERROR,
                            OV_ERROR_DESC_PROCESSING_ERROR);
    }

error:
    /* Do not close socket in case of call errors */
    ov_json_value_free(input);
    ov_json_value_free(data);
    return true;
}

/*----------------------------------------------------------------------------*/

static bool client_revoke_call(ov_vocs *vocs,
                               int socket,
                               const ov_event_parameter *params,
                               ov_json_value *input) {

    UNUSED(params);

    ov_json_value *data = NULL;

    if (!vocs || !input || socket < 0) goto error;

    data = ov_socket_json_get(vocs->connections, socket);

    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));
    const char *role = ov_json_string_get(ov_json_get(data, "/" OV_KEY_ROLE));

    if (!user || !role) {

        send_error_response(
            vocs, input, socket, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto error;
    }

    bool ok = true;

    const char *uuid = ov_event_api_get_uuid(input);
    ov_sip_permission permission = ov_sip_permission_from_json(
        ov_json_get(input, "/" OV_KEY_PARAMETER), &ok);

    if (!uuid || !ok) {

        send_error_response(vocs,
                            input,
                            socket,
                            OV_ERROR_CODE_PARAMETER_ERROR,
                            OV_ERROR_DESC_PARAMETER_ERROR);

        goto error;
    }

    if (ov_mc_backend_sip_terminate_permission(vocs->sip, permission)) {

        send_success_response(vocs, input, socket, NULL);

    } else {

        send_error_response(vocs,
                            input,
                            socket,
                            OV_ERROR_CODE_PROCESSING_ERROR,
                            OV_ERROR_DESC_PROCESSING_ERROR);
    }

error:
    /* Do not close socket in case of call errors */
    ov_json_value_free(input);
    ov_json_value_free(data);
    return true;
}

/*----------------------------------------------------------------------------*/

static bool client_call_function(ov_vocs *vocs,
                                 int socket,
                                 const ov_event_parameter *params,
                                 ov_json_value *input,
                                 bool (*function)(ov_mc_backend_sip *sip,
                                                  const char *uuid)) {

    UNUSED(params);

    ov_json_value *data = NULL;

    if (!vocs || !input || socket < 0) goto error;

    data = ov_socket_json_get(vocs->connections, socket);

    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));
    const char *role = ov_json_string_get(ov_json_get(data, "/" OV_KEY_ROLE));
    const char *uuid = ov_event_api_get_uuid(input);

    if (!user || !role || !uuid) {

        send_error_response(
            vocs, input, socket, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto error;
    }

    if (!ov_event_async_set(
            vocs->async,
            uuid,
            (ov_event_async_data){.socket = socket,
                                  .value = input,
                                  .timedout.userdata = vocs,
                                  .timedout.callback = async_timedout},
            vocs->config.timeout.response_usec)) {

        goto error;
    }

    input = NULL;

    if (!function(vocs->sip, uuid)) {

        ov_event_async_data adata = ov_event_async_unset(vocs->async, uuid);

        send_error_response(vocs,
                            adata.value,
                            adata.socket,
                            OV_ERROR_CODE_PROCESSING_ERROR,
                            OV_ERROR_DESC_PROCESSING_ERROR);

        adata.value = ov_json_value_free(adata.value);

        goto error;
    }

error:
    /* Do not close socket in case of call errors */
    ov_json_value_free(input);
    ov_json_value_free(data);
    return true;
}

/*----------------------------------------------------------------------------*/

static bool client_list_calls(ov_vocs *vocs,
                              int socket,
                              const ov_event_parameter *params,
                              ov_json_value *input) {

    return client_call_function(
        vocs, socket, params, input, ov_mc_backend_sip_list_calls);
}

/*----------------------------------------------------------------------------*/

static bool client_list_call_permissions(ov_vocs *vocs,
                                         int socket,
                                         const ov_event_parameter *params,
                                         ov_json_value *input) {

    return client_call_function(
        vocs, socket, params, input, ov_mc_backend_sip_list_permissions);
}

/*----------------------------------------------------------------------------*/

static bool client_list_sip_status(ov_vocs *vocs,
                                   int socket,
                                   const ov_event_parameter *params,
                                   ov_json_value *input) {

    return client_call_function(
        vocs, socket, params, input, ov_mc_backend_sip_get_status);
}

/*----------------------------------------------------------------------------*/

static bool client_event_set_keyset_layout(ov_vocs *vocs,
                                           int socket,
                                           const ov_event_parameter *params,
                                           ov_json_value *input) {

    /* checking input:

       {
           "event" : "set_keyset_layout",
           "parameter" :
           {
                "domain" : "<domainname>",
                "name"   : "<name>",
                "layout"   : {}
           }
       }

   */

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *data = NULL;

    if (!vocs || !socket || !params || !input) goto error;

    data = ov_socket_json_get(vocs->connections, socket);

    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));

    if (!user) goto error;

    const char *domain = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_DOMAIN));

    const char *name = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_NAME));

    const ov_json_value *layout =
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_LAYOUT);

    if (!domain || !name || !layout) {

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_PARAMETER_ERROR,
                                                 OV_ERROR_DESC_PARAMETER_ERROR);

        goto response;
    }

    if (!ov_vocs_db_authorize_domain_admin(vocs->config.db, user, domain)) {

        out = ov_event_api_create_error_response(
            input, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto response;
    }

    if (!ov_vocs_db_set_keyset_layout(vocs->config.db, domain, name, layout)) {

        out =
            ov_event_api_create_error_response(input,
                                               OV_ERROR_CODE_PROCESSING_ERROR,
                                               OV_ERROR_DESC_PROCESSING_ERROR);

        goto response;
    }

    out = ov_event_api_create_success_response(input);

response:

    ov_event_io_send(params, socket, out);
    out = ov_json_value_free(out);
    input = ov_json_value_free(input);
    data = ov_json_value_free(data);
    return true;
error:
    val = ov_json_value_free(val);
    out = ov_json_value_free(out);
    input = ov_json_value_free(input);
    data = ov_json_value_free(data);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool client_event_get_keyset_layout(ov_vocs *vocs,
                                           int socket,
                                           const ov_event_parameter *params,
                                           ov_json_value *input) {

    /* checking input:

       {
           "event" : "get_keyset_layout",
           "parameter" :
           {
                "domain"   : "<domain_id>",
                "layout".  : "<layout_name>"
           }
       }

   */

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *data = NULL;

    if (!vocs || !socket || !params || !input) goto error;

    data = ov_socket_json_get(vocs->connections, socket);

    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));

    if (!user) goto error;

    const char *domain = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_DOMAIN));

    const char *layout = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_LAYOUT));

    if (!domain || !layout) {

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_PARAMETER_ERROR,
                                                 OV_ERROR_DESC_PARAMETER_ERROR);

        goto response;
    }

    /* anyone can get the layout of some keyset */

    val = ov_vocs_db_get_keyset_layout(vocs->config.db, domain, layout);
    if (!val) {

        out =
            ov_event_api_create_error_response(input,
                                               OV_ERROR_CODE_PROCESSING_ERROR,
                                               OV_ERROR_DESC_PROCESSING_ERROR);

        goto response;
    }

    out = ov_event_api_create_success_response(input);
    ov_json_value *res = ov_event_api_get_response(out);
    ov_json_object_set(res, OV_KEY_LAYOUT, val);

response:

    ov_event_io_send(params, socket, out);
    out = ov_json_value_free(out);
    input = ov_json_value_free(input);
    data = ov_json_value_free(data);
    return true;
error:
    val = ov_json_value_free(val);
    out = ov_json_value_free(out);
    data = ov_json_value_free(data);
    input = ov_json_value_free(input);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool client_event_set_user_data(ov_vocs *vocs,
                                       int socket,
                                       const ov_event_parameter *params,
                                       ov_json_value *input) {

    /* checking input:

       {
           "event" : "set_user_data",
           "parameter" :
           {
                // data to set
           }
       }

   */

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *data = NULL;

    if (!vocs || !socket || !params || !input) goto error;

    data = ov_socket_json_get(vocs->connections, socket);
    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));

    if (!user) {

        out = ov_event_api_create_error_response(
            input, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto response;
    }

    ov_json_value *fdata = ov_event_api_get_parameter(input);
    if (!fdata) {

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_PARAMETER_ERROR,
                                                 OV_ERROR_DESC_PARAMETER_ERROR);

        goto response;
    }

    if (!ov_vocs_db_set_user_data(vocs->config.db, user, fdata)) {

        out =
            ov_event_api_create_error_response(input,
                                               OV_ERROR_CODE_PROCESSING_ERROR,
                                               OV_ERROR_DESC_PROCESSING_ERROR);

        goto response;
    }

    out = ov_event_api_create_success_response(input);
    ov_json_value *res = ov_event_api_get_response(out);
    val = NULL;
    ov_json_value_copy((void **)&val, fdata);
    ov_json_object_set(res, OV_KEY_DATA, val);

    ov_event_parameter parameter =
        (ov_event_parameter){.send.instance = vocs, .send.send = send_socket};

    if (!ov_broadcast_registry_send(
            vocs->broadcasts, user, &parameter, out, OV_USER_BROADCAST))
        goto error;

response:

    ov_event_io_send(params, socket, out);
    out = ov_json_value_free(out);
    input = ov_json_value_free(input);
    data = ov_json_value_free(data);
    return true;
error:
    val = ov_json_value_free(val);
    out = ov_json_value_free(out);
    data = ov_json_value_free(data);
    input = ov_json_value_free(input);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool client_event_get_user_data(ov_vocs *vocs,
                                       int socket,
                                       const ov_event_parameter *params,
                                       ov_json_value *input) {

    /* checking input:

       {
           "event" : "get_user_data",
           "parameter" :
           {
           }
       }

   */

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *data = NULL;

    if (!vocs || !socket || !params || !input) goto error;

    data = ov_socket_json_get(vocs->connections, socket);

    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));

    if (!user) {

        out = ov_event_api_create_error_response(
            input, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto response;
    }

    ov_json_value *fdata = ov_vocs_db_get_user_data(vocs->config.db, user);

    if (!fdata) {

        out =
            ov_event_api_create_error_response(input,
                                               OV_ERROR_CODE_PROCESSING_ERROR,
                                               OV_ERROR_DESC_PROCESSING_ERROR);

        goto response;
    }

    out = ov_event_api_create_success_response(input);
    ov_json_value *res = ov_event_api_get_response(out);
    ov_json_object_set(res, OV_KEY_DATA, fdata);

response:

    ov_event_io_send(params, socket, out);
    out = ov_json_value_free(out);
    input = ov_json_value_free(input);
    data = ov_json_value_free(data);
    return true;
error:
    val = ov_json_value_free(val);
    out = ov_json_value_free(out);
    data = ov_json_value_free(data);
    input = ov_json_value_free(input);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool client_event_register(ov_vocs *vocs,
                                  int socket,
                                  const ov_event_parameter *params,
                                  ov_json_value *input) {

    /* checking input:

       {
           "event" : "register",
           "parameter" :
           {
           }
       }

   */

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;

    if (!vocs || !socket || !params || !input) goto error;

    if (!ov_broadcast_registry_set(vocs->broadcasts,
                                   OV_BROADCAST_KEY_SYSTEM_BROADCAST,
                                   socket,
                                   OV_SYSTEM_BROADCAST)) {
        goto error;
    }

    out = ov_event_api_create_success_response(input);

    ov_event_io_send(params, socket, out);
    out = ov_json_value_free(out);
    input = ov_json_value_free(input);
    return true;
error:
    val = ov_json_value_free(val);
    out = ov_json_value_free(out);
    input = ov_json_value_free(input);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool set_function(ov_dict *dict,
                         const char *name,
                         bool(func)(ov_vocs *vocs,
                                    int socket,
                                    const ov_event_parameter *params,
                                    ov_json_value *input)) {

    if (!dict || !name || !func) goto error;

    char *key = strdup(name);
    if (ov_dict_set(dict, key, func, NULL)) return true;

    key = ov_data_pointer_free(key);

error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool enable_websocket_function(ov_vocs *vocs) {

    if (!vocs) goto error;

    if (!set_function(vocs->io, OV_EVENT_API_LOGOUT, client_logout)) goto error;

    if (!set_function(vocs->io, OV_EVENT_API_LOGIN, client_login)) goto error;

    if (!set_function(vocs->io, OV_EVENT_API_AUTHENTICATE, client_login))
        goto error;

    if (!set_function(vocs->io, OV_EVENT_API_UPDATE_LOGIN, update_client_login))
        goto error;

    if (!set_function(vocs->io, OV_EVENT_API_MEDIA, client_media)) goto error;

    if (!set_function(vocs->io, OV_ICE_STRING_CANDIDATE, client_candidate))
        goto error;

    if (!set_function(vocs->io,
                      OV_ICE_STRING_END_OF_CANDIDATES,
                      client_end_of_candidates))
        goto error;

    if (!set_function(vocs->io, OV_EVENT_API_AUTHORISE, client_authorize))
        goto error;

    if (!set_function(vocs->io, OV_EVENT_API_AUTHORIZE, client_authorize))
        goto error;

    if (!set_function(vocs->io, OV_EVENT_API_GET, client_get)) goto error;

    if (!set_function(vocs->io, OV_EVENT_API_USER_ROLES, client_user_roles))
        goto error;

    if (!set_function(vocs->io, OV_EVENT_API_ROLE_LOOPS, client_role_loops))
        goto error;

    if (!set_function(
            vocs->io, OV_EVENT_API_SWITCH_LOOP_STATE, client_switch_loop_state))
        goto error;

    if (!set_function(vocs->io,
                      OV_EVENT_API_SWITCH_LOOP_VOLUME,
                      client_switch_loop_volume))
        goto error;

    if (!set_function(vocs->io, OV_EVENT_API_TALKING, client_talking))
        goto error;

    if (!set_function(vocs->io, OV_KEY_CALL, client_calling)) goto error;

    if (!set_function(vocs->io, OV_KEY_HANGUP, client_hangup)) goto error;

    if (!set_function(vocs->io, OV_KEY_PERMIT_CALL, client_permit_call))
        goto error;

    if (!set_function(vocs->io, OV_KEY_REVOKE_CALL, client_revoke_call))
        goto error;

    if (!set_function(vocs->io, OV_KEY_LIST_CALLS, client_list_calls))
        goto error;

    if (!set_function(vocs->io,
                      OV_KEY_LIST_CALL_PERMISSIONS,
                      client_list_call_permissions))
        goto error;

    if (!set_function(vocs->io, OV_KEY_LIST_SIP_STATUS, client_list_sip_status))
        goto error;

    if (!set_function(vocs->io,
                      OV_VOCS_DB_SET_KEYSET_LAYOUT,
                      client_event_set_keyset_layout))
        goto error;

    if (!set_function(vocs->io,
                      OV_VOCS_DB_GET_KEYSET_LAYOUT,
                      client_event_get_keyset_layout))
        goto error;

    if (!set_function(
            vocs->io, OV_VOCS_DB_SET_USER_DATA, client_event_set_user_data))
        goto error;

    if (!set_function(
            vocs->io, OV_VOCS_DB_GET_USER_DATA, client_event_get_user_data))
        goto error;

    if (!set_function(
            vocs->io, OV_KEY_GET_RECORDING, client_event_get_recording))
        goto error;

    if (!set_function(vocs->io, OV_KEY_SIP, client_event_sip_status))
        goto error;

    if (!set_function(vocs->io, OV_KEY_REGISTER, client_event_register))
        goto error;

    return true;
error:
    return false;
}
   
/*
 *      ------------------------------------------------------------------------
 *
 *      #FRONTEND EVENTS
 *
 *      ------------------------------------------------------------------------
 */

static void cb_frontend_session_dropped(void *userdata,
                                        const char *session_id) {

    ov_vocs *vocs = ov_vocs_cast(userdata);
    if (!vocs || !session_id) goto error;

    intptr_t socket = (intptr_t)ov_dict_get(vocs->sessions, session_id);
    drop_connection(vocs, socket, false, true);

error:
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_frontend_session_created(
    void *userdata,
    const ov_response_state event,
    const char *session_id,
    const char *type,
    const char *sdp,
    size_t array_size,
    const ov_ice_proxy_vocs_stream_forward_data *array) {

    int socket = 0;
    ov_json_value *data = NULL;
    ov_json_value *orig = NULL;
    ov_json_value *out = NULL;
    ov_json_value *val = NULL;

    ov_id new_uuid = {0};

    ov_vocs *vocs = ov_vocs_cast(userdata);
    if (!vocs) goto error;

    OV_ASSERT(event.id);

    ov_event_async_data adata = ov_event_async_unset(vocs->async, event.id);
    orig = adata.value;
    socket = adata.socket;

    if (!orig) {

        /* original request may be timed out,
         * we release the session, if some session was created */

        if (vocs->debug)
            ov_log_debug(
                "ignoring ice_session_create %s "
                "with error %" PRIu64 " msg %s",
                session_id,
                event.result.error_code,
                event.result.message);

        if (OV_ERROR_NOERROR == event.result.error_code) goto drop_ice_session;

        /* ignore input */
        goto error;
    }

    /* orig request available */

    switch (event.result.error_code) {

        case OV_ERROR_NOERROR:
            break;

        default:

            send_error_response(vocs,
                                orig,
                                socket,
                                event.result.error_code,
                                event.result.message);

            /* let client react to error */
            goto error;
            break;
    }

    OV_ASSERT(1 == array_size);
    OV_ASSERT(array);

    data = ov_socket_json_get(vocs->connections, socket);

    char *key = strdup(session_id);
    if (!key) goto error;

    if (!ov_dict_set(vocs->sessions, key, (void *)(intptr_t)socket, NULL)) {
        key = ov_data_pointer_free(key);
        goto drop_ice_session;
    }

    if (!ov_json_object_set(data, OV_KEY_SESSION, ov_json_string(session_id)))
        goto drop_ice_session;

    if (!ov_socket_json_set(vocs->connections, socket, &data))
        goto drop_ice_session;

    /* (1) send aquire_mixer */

    ov_mc_mixer_core_forward forward = (ov_mc_mixer_core_forward){
        .socket = array[0].socket, .ssrc = array[0].ssrc, .payload_type = 100};

    if (!ov_mc_backend_acquire_mixer(vocs->backend,
                                     event.id,
                                     session_id,
                                     forward,
                                     vocs,
                                     cb_backend_mixer_acquired)) {

        ov_log_error("failed to request aquire_mixer %s", session_id);
        goto drop_session;
    }

    /* (2) send media offer to client */

    out = ov_json_object();

    val = ov_json_string(type);
    if (!ov_json_object_set(out, OV_KEY_TYPE, val)) goto drop_session;

    val = ov_json_string(sdp);
    if (!ov_json_object_set(out, OV_KEY_SDP, val)) goto drop_session;

    if (!send_success_response(vocs, orig, socket, &out)) {

        out = ov_json_value_free(out);
        goto drop_session;
    }

    orig = ov_json_value_free(orig);
    data = ov_json_value_free(data);
    return;

drop_session:

    out = ov_json_value_free(out);
    val = ov_json_value_free(val);

    // fall into drop_ice_session

drop_ice_session:

    ov_id_fill_with_uuid(new_uuid);
    ov_mc_frontend_drop_session(vocs->frontend, new_uuid, session_id);

    // fall into error

error:
    data = ov_json_value_free(data);
    orig = ov_json_value_free(orig);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_frontend_session_completed(void *userdata,
                                          const char *session_id,
                                          bool success) {

    ov_json_value *data = NULL;

    ov_vocs *vocs = ov_vocs_cast(userdata);
    if (!vocs) goto error;

    intptr_t socket = (intptr_t)ov_dict_get(vocs->sessions, session_id);
    data = ov_socket_json_get(vocs->connections, socket);

    if (success && ov_json_is_true(ov_json_get(data, "/" OV_KEY_ICE))) {

        data = ov_json_value_free(data);
        return;
    }

    ov_json_object_set(data, OV_KEY_ICE, ov_json_true());

    if (!success) {

        ov_log_error("ICE session failed %s", session_id);
        goto drop_connection;

    } else if (ov_json_is_true(ov_json_get(data, "/" OV_KEY_MEDIA_READY))) {

        ov_json_value *out =
            ov_event_api_message_create(OV_KEY_MEDIA_READY, NULL, 0);
        ov_event_api_set_type(out, OV_KEY_UNICAST);

        if (!ov_event_api_set_parameter(out)) goto error;

        env_send(vocs, socket, out);
        out = ov_json_value_free(out);
    }

    ov_socket_json_set(vocs->connections, socket, &data);
    return;

drop_connection:
    drop_connection(vocs, socket, false, true);
error:
    ov_json_value_free(data);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_frontend_session_update(void *userdata,
                                       const ov_response_state event,
                                       const char *session_id) {

    ov_json_value *orig = NULL;
    ov_json_value *out = NULL;

    ov_vocs *vocs = ov_vocs_cast(userdata);
    if (!vocs) goto error;

    OV_ASSERT(session_id);

    if (!session_id) goto error;

    ov_event_async_data data = ov_event_async_unset(vocs->async, event.id);
    orig = data.value;

    switch (event.result.error_code) {

        case OV_ERROR_NOERROR:

            if (orig) {

                out = ov_event_api_create_success_response(orig);

            } else {

                out = ov_event_api_message_create(OV_KEY_MEDIA, NULL, 0);
            }

            break;

        case OV_ERROR_CODE_NOT_A_RESPONSE:

            out = ov_event_api_message_create(OV_KEY_MEDIA, NULL, 0);
            break;

        default:

            if (orig) {

                out = ov_event_api_create_error_response(
                    orig, event.result.error_code, event.result.message);

            } else {

                /* We should only receive some error reponse for requests,
                 * which may be timed out already - ignore */

                goto error;
            }

            break;
    }

    OV_ASSERT(out);
    ov_event_api_set_type(out, OV_KEY_UNICAST);
    env_send(vocs, data.socket, out);

error:
    out = ov_json_value_free(out);
    orig = ov_json_value_free(orig);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_frontend_session_state(void *userdata,
                                      const ov_response_state event,
                                      const char *session_id,
                                      const ov_json_value *state) {

    ov_json_value *orig = NULL;
    ov_json_value *out = NULL;
    ov_json_value *val = NULL;

    ov_vocs *vocs = ov_vocs_cast(userdata);
    if (!vocs) goto error;

    OV_ASSERT(session_id);

    if (!session_id || !state) goto error;

    ov_event_async_data data = ov_event_async_unset(vocs->async, event.id);
    orig = data.value;

    // withdraw timedout state messages
    if (!orig) goto error;

    const char *uuid = ov_event_api_get_uuid(orig);
    if (!uuid || !session_id) goto error;

    switch (event.result.error_code) {

        case OV_ERROR_NOERROR:

            break;

        default:

            out = ov_event_api_create_error_response(
                orig, event.result.error_code, event.result.message);

            env_send(vocs, data.socket, out);
            goto error;
            break;
    }

    if (!ov_json_value_copy((void **)&val, state)) goto error;

    out = ov_event_api_create_success_response(orig);

    if (!ov_event_api_set_uuid(out, uuid)) goto error;

    if (!ov_json_object_set(
            ov_event_api_get_response(out), OV_KEY_FRONTEND, val))
        goto error;

    if (!ov_event_async_set(
            vocs->async,
            uuid,
            (ov_event_async_data){.socket = data.socket,
                                  .value = out,
                                  .timedout.userdata = vocs,
                                  .timedout.callback = async_timedout},
            vocs->config.timeout.response_usec)) {

        char *str = ov_json_value_to_string(out);
        ov_log_error("failed to set async msg - closing %i | %s", socket, str);
        str = ov_data_pointer_free(str);

        goto error;
    }

    out = NULL;

    if (!ov_mc_backend_get_session_state(
            vocs->backend, uuid, session_id, vocs, cb_backend_mixer_state))
        goto error;

error:
    out = ov_json_value_free(out);
    orig = ov_json_value_free(orig);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_frontend_candidate(void *userdata,
                                  const ov_response_state event,
                                  const char *session_id,
                                  const ov_ice_candidate_info *info) {

    ov_json_value *orig = NULL;
    ov_json_value *out = NULL;
    ov_json_value *val = NULL;

    ov_vocs *vocs = ov_vocs_cast(userdata);
    if (!vocs) goto error;

    intptr_t socket = (intptr_t)ov_dict_get(vocs->sessions, session_id);

    ov_event_async_data data = ov_event_async_unset(vocs->async, event.id);
    orig = data.value;

    val = ov_ice_candidate_info_to_json(*info);
    if (!val) goto error;

    switch (event.result.error_code) {

        case OV_ERROR_NOERROR:

            if (orig) {

                out = ov_event_api_create_success_response(orig);
                if (!ov_json_object_set(out, OV_KEY_RESPONSE, val)) goto error;

                val = NULL;

            } else {

                out = ov_event_api_message_create(
                    OV_ICE_STRING_CANDIDATE, NULL, 0);
                if (!ov_json_object_set(out, OV_KEY_PARAMETER, val)) goto error;

                val = NULL;
            }

            break;

        case OV_ERROR_CODE_NOT_A_RESPONSE:

            out = ov_event_api_message_create(OV_ICE_STRING_CANDIDATE, NULL, 0);
            if (!ov_json_object_set(out, OV_KEY_PARAMETER, val)) goto error;

            val = NULL;
            break;

        default:

            if (orig) {

                out = ov_event_api_create_error_response(
                    orig, event.result.error_code, event.result.message);

            } else {

                /* We should only receive some error reponse for requests,
                 * which may be timed out already - ignore */

                goto error;
            }

            break;
    }

    OV_ASSERT(out);
    ov_event_api_set_type(out, OV_KEY_UNICAST);
    env_send(vocs, socket, out);

error:
    out = ov_json_value_free(out);
    val = ov_json_value_free(val);
    orig = ov_json_value_free(orig);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_frontend_end_of_candidates(void *userdata,
                                          const ov_response_state event,
                                          const char *session_id,
                                          const ov_ice_candidate_info *info) {

    ov_json_value *orig = NULL;
    ov_json_value *out = NULL;
    ov_json_value *val = NULL;

    UNUSED(session_id);

    ov_vocs *vocs = ov_vocs_cast(userdata);
    if (!vocs) goto error;

    intptr_t socket = (intptr_t)ov_dict_get(vocs->sessions, session_id);

    ov_event_async_data data = ov_event_async_unset(vocs->async, event.id);
    orig = data.value;

    switch (event.result.error_code) {

        case OV_ERROR_NOERROR:

            if (orig) {

                out = ov_event_api_create_success_response(orig);

            } else {

                out = ov_event_api_message_create(
                    OV_ICE_STRING_END_OF_CANDIDATES, NULL, 0);

                val = ov_ice_candidate_info_to_json(*info);
                if (!val) goto error;

                if (!ov_json_object_set(out, OV_KEY_PARAMETER, val)) goto error;

                val = NULL;
            }

            break;

        case OV_ERROR_CODE_NOT_A_RESPONSE:

            out = ov_event_api_message_create(
                OV_ICE_STRING_END_OF_CANDIDATES, NULL, 0);

            val = ov_ice_candidate_info_to_json(*info);
            if (!val) goto error;

            if (!ov_json_object_set(out, OV_KEY_PARAMETER, val)) goto error;

            val = NULL;

            break;

        default:

            if (orig) {

                out = ov_event_api_create_error_response(
                    orig, event.result.error_code, event.result.message);

            } else {

                /* We should only receive some error reponse for requests,
                 * which may be timed out already - ignore */

                goto error;
            }

            break;
    }

    OV_ASSERT(out);
    ov_event_api_set_type(out, OV_KEY_UNICAST);
    env_send(vocs, socket, out);

error:
    out = ov_json_value_free(out);
    val = ov_json_value_free(val);
    orig = ov_json_value_free(orig);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_frontend_talk(void *userdata,
                             const ov_response_state event,
                             const char *session_id,
                             const ov_mc_loop_data ldata,
                             bool on) {

    ov_json_value *data = NULL;
    ov_json_value *orig = NULL;
    ov_json_value *out = NULL;
    ov_json_value *val = NULL;

    UNUSED(session_id);

    ov_vocs *vocs = ov_vocs_cast(userdata);
    if (!vocs) goto error;

    ov_event_async_data adata = ov_event_async_unset(vocs->async, event.id);
    orig = adata.value;

    if (!orig) goto drop;

    const char *loop = ldata.name;
    const char *state = ov_json_string_get(
        ov_json_get(orig, "/" OV_KEY_PARAMETER "/" OV_KEY_STATE));

    ov_vocs_permission current = OV_VOCS_SEND;
    ov_vocs_permission requested = ov_vocs_permission_from_string(state);

    switch (event.result.error_code) {

        case OV_ERROR_NOERROR:

            break;

        default:

            send_error_response(vocs,
                                orig,
                                adata.socket,
                                event.result.error_code,
                                event.result.message);

            goto error;
    }

    if (on) {

        current = OV_VOCS_SEND;

    } else {

        current = OV_VOCS_RECV;
    }

    data = ov_socket_json_get(vocs->connections, adata.socket);
    set_loop_state_in_data(data, loop, current);
    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));
    const char *role = ov_json_string_get(ov_json_get(data, "/" OV_KEY_ROLE));

    if (!ov_vocs_db_set_state(vocs->config.db, user, role, loop, current))
        goto drop;

    ov_socket_json_set(vocs->connections, adata.socket, &data);

    switch (requested) {

        case OV_VOCS_SEND:
        case OV_VOCS_RECV:
            break;

        case OV_VOCS_NONE:
            goto switch_off_loop;
            break;
    }

    // we are at the final state for the switch command

    ov_vocs_loop *l = ov_dict_get(vocs->loops, loop);
    OV_ASSERT(l);

    ov_json_value *participants = ov_vocs_loop_get_participants(l);

    out = ov_json_object();
    if (!ov_json_object_set(out, OV_KEY_PARTICIPANTS, participants)) goto drop;

    val = ov_json_string(ov_vocs_permission_to_string(current));
    if (!ov_json_object_set(out, OV_KEY_STATE, val)) goto drop;

    val = ov_json_string(loop);
    if (!ov_json_object_set(out, OV_KEY_LOOP, val)) goto drop;

    val = NULL;

    if (!send_switch_loop_user_broadcast(vocs, adata.socket, loop, current)) {

        ov_log_error(
            "failed to send switch loop user "
            "broadcast %s",
            loop);
    }

    if (!send_success_response(vocs, orig, adata.socket, &out)) {
        out = ov_json_value_free(out);
        goto drop;
    }

    orig = ov_json_value_free(orig);
    ov_json_value_free(out);

    data = ov_json_value_free(data);
    return;

switch_off_loop:

    // reset async event

    if (!ov_event_async_set(
            vocs->async,
            event.id,
            (ov_event_async_data){.socket = adata.socket,
                                  .value = orig,
                                  .timedout.userdata = vocs,
                                  .timedout.callback = async_timedout},
            vocs->config.timeout.response_usec)) {

        char *str = ov_json_value_to_string(orig);
        ov_log_error("failed to set async msg - closing %i | %s", socket, str);
        str = ov_data_pointer_free(str);

        goto drop;
    }

    orig = NULL;

    if (!ov_mc_backend_leave_loop(vocs->backend,
                                  event.id,
                                  session_id,
                                  loop,
                                  vocs,
                                  cb_backend_mixer_leave))
        goto drop;

    data = ov_json_value_free(data);
    return;

drop:
    drop_connection(vocs, adata.socket, true, true);

error:
    orig = ov_json_value_free(orig);
    data = ov_json_value_free(data);
    ov_json_value_free(val);
    ov_json_value_free(out);
    return;
}

/*----------------------------------------------------------------------------*/

static bool module_load_frontend(ov_vocs *self) {

    OV_ASSERT(self);

    self->config.module.frontend.io = self->config.io;

    self->config.module.frontend.loop = self->config.loop;
    self->config.module.frontend.callback.userdata = self;
    self->config.module.frontend.callback.session.dropped =
        cb_frontend_session_dropped;
    self->config.module.frontend.callback.session.created =
        cb_frontend_session_created;
    self->config.module.frontend.callback.session.completed =
        cb_frontend_session_completed;
    self->config.module.frontend.callback.session.update =
        cb_frontend_session_update;
    self->config.module.frontend.callback.session.state =
        cb_frontend_session_state;
    self->config.module.frontend.callback.candidate = cb_frontend_candidate;
    self->config.module.frontend.callback.end_of_candidates =
        cb_frontend_end_of_candidates;
    self->config.module.frontend.callback.talk = cb_frontend_talk;

    self->frontend = ov_mc_frontend_create(self->config.module.frontend);
    if (!self->frontend) return false;

    return true;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      #SIP FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

static void cb_call_init(void *userdata,
                         const char *uuid,
                         const char *loopname,
                         const char *call_id,
                         const char *caller,
                         const char *callee,
                         uint8_t error_code,
                         const char *error_desc) {

    ov_event_async_data adata = {0};

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *par = NULL;

    ov_vocs *self = ov_vocs_cast(userdata);
    if (!self || !uuid) goto error;

    adata = ov_event_async_unset(self->async, uuid);
    if (!adata.value) goto response_loop;

    switch (error_code) {

        case OV_ERROR_NOERROR:

            out = ov_event_api_create_success_response(adata.value);
            par = ov_event_api_get_response(out);
            break;

        default:

            out = ov_event_api_create_error_response(
                adata.value, error_code, error_desc);

            par = ov_event_api_get_response(out);
    }

    if (loopname) {

        val = ov_json_string(loopname);
        if (!ov_json_object_set(par, OV_KEY_LOOP, val)) goto error;
    }

    val = NULL;

    if (call_id) {

        val = ov_json_string(call_id);
        if (!ov_json_object_set(par, OV_KEY_CALL_ID, val)) goto error;
    }

    val = NULL;

    if (caller) {

        val = ov_json_string(caller);
        if (!ov_json_object_set(par, OV_KEY_CALLER, val)) goto error;
    }

    val = NULL;

    if (callee) {

        val = ov_json_string(callee);
        if (!ov_json_object_set(par, OV_KEY_CALLEE, val)) goto error;
    }

    val = NULL;

    env_send(self, adata.socket, out);

    adata.value = ov_json_value_free(adata.value);
    ov_json_value_free(out);
    return;

response_loop:

    out = ov_mc_sip_msg_create_call(loopname, callee, caller);
    par = ov_event_api_get_parameter(out);

    switch (error_code) {

        case OV_ERROR_NOERROR:
            break;

        default:

            ov_event_api_add_error(out, error_code, error_desc);
    }

    if (call_id) {

        val = ov_json_string(call_id);
        if (!ov_json_object_set(par, OV_KEY_CALL_ID, val)) goto error;
    }

    val = NULL;

    if (!ov_event_api_set_type(out, OV_BROADCAST_KEY_LOOP_BROADCAST))
        goto error;

    ov_event_parameter parameter =
        (ov_event_parameter){.send.instance = self, .send.send = send_socket};

    if (!ov_broadcast_registry_send(
            self->broadcasts, loopname, &parameter, out, OV_LOOP_BROADCAST)) {

        goto error;
    }

    ov_json_value_free(out);
    return;

error:
    ov_json_value_free(out);
    ov_json_value_free(val);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_sip_new(void *userdata,
                       const char *loopname,
                       const char *call_id,
                       const char *peer) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *par = NULL;

    ov_vocs *self = ov_vocs_cast(userdata);
    if (!self || !loopname || !call_id || !peer) goto error;

    out = ov_event_api_message_create(OV_KEY_CALL, NULL, 0);

    if (!ov_event_api_set_type(out, OV_BROADCAST_KEY_LOOP_BROADCAST))
        goto error;

    par = ov_event_api_set_parameter(out);

    val = ov_json_string(loopname);
    if (!ov_json_object_set(par, OV_KEY_LOOP, val)) goto error;

    val = ov_json_string(call_id);
    if (!ov_json_object_set(par, OV_KEY_CALL_ID, val)) goto error;

    val = ov_json_string(peer);
    if (!ov_json_object_set(par, OV_KEY_PEER, val)) goto error;

    val = NULL;

    ov_event_parameter parameter =
        (ov_event_parameter){.send.instance = self, .send.send = send_socket};

    if (!ov_broadcast_registry_send(
            self->broadcasts, loopname, &parameter, out, OV_LOOP_BROADCAST)) {

        goto error;
    }

error:
    val = ov_json_value_free(val);
    out = ov_json_value_free(out);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_sip_terminated(void *userdata,
                              const char *call_id,
                              const char *loopname) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *par = NULL;

    ov_vocs *self = ov_vocs_cast(userdata);
    if (!self || !loopname || !call_id) goto error;

    out = ov_event_api_message_create(OV_KEY_HANGUP, NULL, 0);

    if (!ov_event_api_set_type(out, OV_BROADCAST_KEY_LOOP_BROADCAST))
        goto error;

    par = ov_event_api_set_parameter(out);

    val = ov_json_string(loopname);
    if (!ov_json_object_set(par, OV_KEY_LOOP, val)) goto error;

    val = ov_json_string(call_id);
    if (!ov_json_object_set(par, OV_KEY_CALL_ID, val)) goto error;

    val = NULL;

    ov_event_parameter parameter =
        (ov_event_parameter){.send.instance = self, .send.send = send_socket};

    if (!ov_broadcast_registry_send(
            self->broadcasts, loopname, &parameter, out, OV_LOOP_BROADCAST)) {

        goto error;
    }

error:
    val = ov_json_value_free(val);
    out = ov_json_value_free(out);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_sip_permit(void *userdata,
                          const ov_sip_permission permission,
                          uint64_t error_code,
                          const char *error_desc) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;

    ov_vocs *self = ov_vocs_cast(userdata);
    if (!self) goto error;

    if (NULL == permission.loop) goto error;

    out = ov_event_api_message_create(OV_KEY_PERMIT, NULL, 0);

    if (!ov_event_api_set_type(out, OV_BROADCAST_KEY_LOOP_BROADCAST))
        goto error;

    val = ov_sip_permission_to_json(permission);
    if (!ov_json_object_set(out, OV_KEY_PARAMETER, val)) goto error;

    val = NULL;

    if (0 != error_code) {

        val = ov_event_api_create_error_code(error_code, error_desc);
        if (!ov_json_object_set(out, OV_KEY_ERROR, val)) goto error;

        ov_vocs_db_remove_permission(self->config.db, permission);
        ov_vocs_db_app_send_broadcast(self->config.db_app, out);


        val = NULL;
    }

    ov_event_parameter parameter =
        (ov_event_parameter){.send.instance = self, .send.send = send_socket};

    if (!ov_broadcast_registry_send(self->broadcasts,
                                    permission.loop,
                                    &parameter,
                                    out,
                                    OV_LOOP_BROADCAST)) {

        goto error;
    }

error:
    val = ov_json_value_free(val);
    out = ov_json_value_free(out);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_sip_revoke(void *userdata,
                          const ov_sip_permission permission,
                          uint64_t error_code,
                          const char *error_desc) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;

    ov_vocs *self = ov_vocs_cast(userdata);
    if (!self) goto error;

    if (NULL == permission.loop) goto error;

    out = ov_event_api_message_create(OV_KEY_REVOKE, NULL, 0);

    if (!ov_event_api_set_type(out, OV_BROADCAST_KEY_LOOP_BROADCAST))
        goto error;

    val = ov_sip_permission_to_json(permission);
    if (!ov_json_object_set(out, OV_KEY_PARAMETER, val)) goto error;

    val = NULL;

    if (0 != error_code) {

        val = ov_event_api_create_error_code(error_code, error_desc);
        if (!ov_json_object_set(out, OV_KEY_ERROR, val)) goto error;

        ov_vocs_db_app_send_broadcast(self->config.db_app, out);

        val = NULL;
    }

    ov_event_parameter parameter =
        (ov_event_parameter){.send.instance = self, .send.send = send_socket};

    if (!ov_broadcast_registry_send(self->broadcasts,
                                    permission.loop,
                                    &parameter,
                                    out,
                                    OV_LOOP_BROADCAST)) {

        goto error;
    }

error:
    val = ov_json_value_free(val);
    out = ov_json_value_free(out);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_sip_list_calls(void *userdata,
                              const char *uuid,
                              const ov_json_value *calls,
                              uint64_t error_code,
                              const char *error_desc) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *par = NULL;

    ov_event_async_data adata = {0};

    ov_vocs *self = ov_vocs_cast(userdata);
    if (!self) goto error;

    adata = ov_event_async_unset(self->async, uuid);
    if (!adata.value) goto error;

    switch (error_code) {

        case OV_ERROR_NOERROR:

            out = ov_event_api_create_success_response(adata.value);
            par = ov_event_api_get_response(out);
            val = NULL;
            ov_json_value_copy((void **)&val, calls);
            if (!ov_json_object_set(par, OV_KEY_CALLS, val)) goto error;

            val = NULL;

            break;

        default:

            out = ov_event_api_create_error_response(
                adata.value, error_code, error_desc);
    }

    env_send(self, adata.socket, out);

error:
    out = ov_json_value_free(out);
    val = ov_json_value_free(val);
    adata.value = ov_json_value_free(adata.value);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_sip_list_permissions(void *userdata,
                                    const char *uuid,
                                    const ov_json_value *permissions,
                                    uint64_t error_code,
                                    const char *error_desc) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *par = NULL;

    ov_event_async_data adata = {0};

    ov_vocs *self = ov_vocs_cast(userdata);
    if (!self) goto error;

    adata = ov_event_async_unset(self->async, uuid);
    if (!adata.value) goto error;

    switch (error_code) {

        case OV_ERROR_NOERROR:

            out = ov_event_api_create_success_response(adata.value);
            par = ov_event_api_get_response(out);
            val = NULL;
            ov_json_value_copy((void **)&val, permissions);
            if (!ov_json_object_set(par, OV_KEY_PERMISSIONS, val)) goto error;

            val = NULL;

            break;

        default:

            out = ov_event_api_create_error_response(
                adata.value, error_code, error_desc);
    }

    env_send(self, adata.socket, out);

error:
    out = ov_json_value_free(out);
    val = ov_json_value_free(val);
    adata.value = ov_json_value_free(adata.value);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_sip_get_status(void *userdata,
                              const char *uuid,
                              const ov_json_value *status,
                              uint64_t error_code,
                              const char *error_desc) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *par = NULL;

    ov_event_async_data adata = {0};

    ov_vocs *self = ov_vocs_cast(userdata);
    if (!self) goto error;

    adata = ov_event_async_unset(self->async, uuid);
    if (!adata.value) goto error;

    switch (error_code) {

        case OV_ERROR_NOERROR:

            out = ov_event_api_create_success_response(adata.value);
            par = ov_event_api_get_response(out);
            val = NULL;
            ov_json_value_copy((void **)&val, status);
            if (!ov_json_object_set(par, OV_KEY_STATUS, val)) goto error;

            val = NULL;

            break;

        default:

            out = ov_event_api_create_error_response(
                adata.value, error_code, error_desc);
    }

    env_send(self, adata.socket, out);

error:
    out = ov_json_value_free(out);
    val = ov_json_value_free(val);
    adata.value = ov_json_value_free(adata.value);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_sip_connected(void *userdata, bool status) {

    ov_json_value *out = NULL;
    ov_json_value *par = NULL;
    ov_json_value *val = NULL;

    ov_vocs *self = ov_vocs_cast(userdata);
    if (!self) goto error;

    out = ov_event_api_message_create(OV_KEY_SIP, NULL, 0);
    par = ov_event_api_set_parameter(out);

    if (status) {
        val = ov_json_true();
    } else {
        val = ov_json_false();
    }

    ov_json_object_set(par, OV_KEY_CONNECTED, val);

    ov_event_parameter parameter =
        (ov_event_parameter){.send.instance = self, .send.send = send_socket};

    if (!ov_broadcast_registry_send(self->broadcasts,
                                    OV_BROADCAST_KEY_SYSTEM_BROADCAST,
                                    &parameter,
                                    out,
                                    OV_SYSTEM_BROADCAST)) {

        goto error;
    }
error:
    ov_json_value_free(out);
    return;
}

/*----------------------------------------------------------------------------*/

static bool module_load_sip(ov_vocs *self) {

    OV_ASSERT(self);

    self->config.module.sip.io = self->config.io;
    self->config.module.sip.backend = self->backend;
    self->config.module.sip.loop = self->config.loop;
    self->config.module.sip.db = self->config.db;

    self->config.module.sip.callback.userdata = self;
    self->config.module.sip.callback.call.init = cb_call_init;
    self->config.module.sip.callback.call.new = cb_sip_new;
    self->config.module.sip.callback.call.terminated = cb_sip_terminated;
    self->config.module.sip.callback.call.permit = cb_sip_permit;
    self->config.module.sip.callback.call.revoke = cb_sip_revoke;
    self->config.module.sip.callback.list_calls = cb_sip_list_calls;
    self->config.module.sip.callback.list_permissions = cb_sip_list_permissions;
    self->config.module.sip.callback.get_status = cb_sip_get_status;
    self->config.module.sip.callback.connected = cb_sip_connected;

    self->sip = ov_mc_backend_sip_create(self->config.module.sip);
    if (!self->sip) return false;

    return true;
}

/*----------------------------------------------------------------------------*/

static bool module_load_sip_static(ov_vocs *self) {

    OV_ASSERT(self);

    self->config.module.sip_static.io = self->config.io;
    self->config.module.sip_static.backend = self->backend;
    self->config.module.sip_static.loop = self->config.loop;
    self->config.module.sip_static.db = self->config.db;

    self->sip_static =
        ov_mc_backend_sip_static_create(self->config.module.sip_static);
    if (!self->sip_static) return false;

    return true;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      #RECORDER FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

static void cb_start_record(void *userdata, const char *uuid, ov_result error){

    ov_vocs *self = ov_vocs_cast(userdata);
    if (!self || !uuid) goto error;

    ov_event_async_data adata = ov_event_async_unset(self->async, uuid);

    if (!adata.value) goto error;

    switch (error.error_code) {

        case OV_ERROR_NOERROR:

            send_success_response(self, adata.value, adata.socket, NULL);
            break;

        default:
            send_error_response(self,
                            adata.value, adata.socket,
                            error.error_code,
                            error.message);
    }

    adata.value = ov_json_value_free(adata.value);

error:

    return;
}

/*----------------------------------------------------------------------------*/

static void cb_stop_record(void *userdata, const char *uuid, ov_result error){

    ov_vocs *self = ov_vocs_cast(userdata);
    if (!self || !uuid) goto error;

    ov_event_async_data adata = ov_event_async_unset(self->async, uuid);

    if (!adata.value) goto error;

    switch (error.error_code) {

        case OV_ERROR_NOERROR:

            send_success_response(self, adata.value, adata.socket, NULL);
            break;
            
        default:
            send_error_response(self,
                            adata.value, adata.socket,
                            error.error_code,
                            error.message);
    }

    adata.value = ov_json_value_free(adata.value);

error:
    return;
}

/*----------------------------------------------------------------------------*/

static bool module_load_recorder(ov_vocs *self) {

    OV_ASSERT(self);

    self->config.module.recorder.loop = self->config.loop;
    self->config.module.recorder.vocs_db = self->config.db;

    self->config.module.recorder.timeout.response_usec = self->config.timeout.response_usec;

    self->config.module.recorder.callbacks.userdata =self;
    self->config.module.recorder.callbacks.start_record = cb_start_record;
    self->config.module.recorder.callbacks.stop_record = cb_stop_record;

    self->recorder = ov_vocs_recorder_create(self->config.module.recorder);
    if (!self->recorder) return false;

    return true;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      #VAD FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

/*----------------------------------------------------------------------------*/

static void io_vad(void *userdata, const char *loop, bool on) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *par = NULL;

    ov_vocs *self = ov_vocs_cast(userdata);

    out = ov_event_api_message_create(OV_KEY_VAD, NULL, 0);

    if (!ov_event_api_set_type(out, OV_BROADCAST_KEY_LOOP_BROADCAST))
        goto error;

    par = ov_event_api_set_parameter(out);

    if (!ov_json_object_set(par, OV_KEY_LOOP, ov_json_string(loop))) goto error;

    if (on) {
        val = ov_json_true();
    } else {
        val = ov_json_false();
    }

    if (!ov_json_object_set(par, OV_KEY_ON, val)) goto error;

    val = NULL;

    ov_event_parameter parameter =
        (ov_event_parameter){.send.instance = self, .send.send = send_socket};

    if (!ov_broadcast_registry_send(
            self->broadcasts, loop, &parameter, out, OV_LOOP_BROADCAST))
        goto error;

    out = ov_json_value_free(out);
    return;
error:
    ov_json_value_free(out);
    ov_json_value_free(val);
    return;
}

/*----------------------------------------------------------------------------*/

static bool module_load_vad(ov_vocs *self) {

    OV_ASSERT(self);

    self->config.module.vad.loop = self->config.loop;
    self->config.module.vad.db = self->config.db;
    self->config.module.vad.io = self->config.io;

    self->config.module.vad.callbacks.userdata = self;
    self->config.module.vad.callbacks.vad = io_vad;

    self->vad = ov_mc_backend_vad_create(self->config.module.vad);
    if (!self->vad) return false;

    return true;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      #LDAP FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

static bool module_load_ldap(ov_vocs *self) {

    OV_ASSERT(self);

    self->config.ldap.config.loop = self->config.loop;
    self->ldap = ov_ldap_create(self->config.ldap.config);
    if (!self->ldap) goto error;

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

struct container_updata_sip {

    const char *loop;
    ov_vocs *vocs;
};

/*----------------------------------------------------------------------------*/

static bool revoke_sip_call(void *item, void *data) {

    struct container_updata_sip *container =
        (struct container_updata_sip *)data;

    ov_sip_permission permission = (ov_sip_permission){
        .caller = ov_json_string_get(ov_json_object_get(item, OV_KEY_CALLER)),
        .callee = ov_json_string_get(ov_json_object_get(item, OV_KEY_CALLEE)),
        .loop = container->loop,
        .from_epoch =
            ov_json_number_get(ov_json_object_get(item, OV_KEY_VALID_FROM)),
        .until_epoch =
            ov_json_number_get(ov_json_object_get(item, OV_KEY_VALID_UNTIL))};

    return ov_mc_backend_sip_terminate_permission(
        container->vocs->sip, permission);
}

/*----------------------------------------------------------------------------*/

static bool permit_sip_call(void *item, void *data) {

    struct container_updata_sip *container =
        (struct container_updata_sip *)data;

    ov_sip_permission permission = (ov_sip_permission){
        .caller = ov_json_string_get(ov_json_object_get(item, OV_KEY_CALLER)),
        .callee = ov_json_string_get(ov_json_object_get(item, OV_KEY_CALLEE)),
        .loop = container->loop,
        .from_epoch =
            ov_json_number_get(ov_json_object_get(item, OV_KEY_VALID_FROM)),
        .until_epoch =
            ov_json_number_get(ov_json_object_get(item, OV_KEY_VALID_UNTIL))};

    return ov_mc_backend_sip_create_permission(
        container->vocs->sip, permission);
}

/*----------------------------------------------------------------------------*/

static bool update_sip_backend(const void *key, void *val, void *data) {

    if (!key) return true;

    const char *loop = (const char *)key;

    ov_json_value *revoke = ov_json_object_get(val, OV_KEY_REVOKE);
    ov_json_value *permit = ov_json_object_get(val, OV_KEY_PERMIT);

    struct container_updata_sip container =
        (struct container_updata_sip){.loop = loop, .vocs = ov_vocs_cast(data)};

    ov_json_array_for_each(revoke, &container, revoke_sip_call);
    ov_json_array_for_each(permit, &container, permit_sip_call);

    return true;
}

/*----------------------------------------------------------------------------*/

static void process_trigger(void *userdata, ov_json_value *input) {

    ov_vocs *vocs = ov_vocs_cast(userdata);
    if (!vocs || !input) goto error;

    const char *event = ov_event_api_get_event(input);
    if (!event) goto error;

    if (0 == ov_string_compare(event, OV_VOCS_DB_UPDATE_DB)) {

        ov_json_value *proc = (ov_json_value *)ov_json_get(
            input, "/" OV_KEY_PARAMETER "/" OV_KEY_PROCESSING);
        ov_json_object_for_each(proc, vocs, update_sip_backend);
    }

    if (0 == ov_string_compare(event, OV_VOCS_DB_KEY_LDAP_UPDATE)) {

        ov_vocs_db_app_send_broadcast(vocs->config.db_app, input);
    }

    input = ov_json_value_free(input);
error:
    input = ov_json_value_free(input);
}

/*----------------------------------------------------------------------------*/

ov_vocs *ov_vocs_create(ov_vocs_config config) {

    ov_vocs *vocs = NULL;

    if (!config.loop) goto error;

    if (!config.db) goto error;

    if (!config.env.userdata) goto error;
    if (!config.env.close) goto error;
    if (!config.env.send) goto error;

    if (0 == config.sessions.path[0]) {
        strncpy(config.sessions.path, OV_VOCS_DEFAULT_SESSIONS_PATH, PATH_MAX);
    }

    if (0 == config.timeout.response_usec)
        config.timeout.response_usec = IMPL_TIMEOUT_RESPONSE_USEC;

    if (0 == config.timeout.reconnect_interval_usec)
        config.timeout.reconnect_interval_usec = IMPL_TIMEOUT_RECONNECT_USEC;

    vocs = calloc(1, sizeof(ov_vocs));
    if (!vocs) goto error;

    vocs->magic_bytes = OV_VOCS_MAGIC_BYTES;
    vocs->config = config;

    vocs->connections =
        ov_socket_json_create((ov_socket_json_config){.loop = config.loop});
    if (!vocs->connections) goto error;

    vocs->async = ov_event_async_store_create(
        (ov_event_async_store_config){.loop = config.loop});

    if (!vocs->async) goto error;

    vocs->broadcasts = ov_broadcast_registry_create((ov_event_broadcast_config){
        .max_sockets = ov_socket_get_max_supported_runtime_sockets(0)});

    if (!vocs->broadcasts) goto error;

    ov_event_session_config event_config =
        (ov_event_session_config){.loop = config.loop};

    strncpy(event_config.path, config.sessions.path, PATH_MAX);

    vocs->user_sessions = ov_event_session_create(event_config);
    if (!vocs->user_sessions) goto error;

    vocs->sessions = ov_dict_create(ov_dict_string_key_config(255));
    if (!vocs->sessions) goto error;

    ov_dict_config d_config = ov_dict_string_key_config(255);
    d_config.value.data_function.free = ov_vocs_loop_free_void;
    vocs->loops = ov_dict_create(d_config);
    if (!vocs->loops) goto error;

    vocs->io = ov_dict_create(ov_dict_string_key_config(255));
    if (!vocs->io) goto error;

    if (!module_load_backend(vocs)) {
        ov_log_error("Failed to enable Backend");
        goto error;
    }

    if (!enable_websocket_function(vocs)) {
        ov_log_error("Failed to enable websocket.");
        goto error;
    }

    if (config.ldap.enable && !module_load_ldap(vocs)) {
        ov_log_error("Failed to enable LDAP");
        goto error;
    }

    if (!module_load_frontend(vocs)) {
        ov_log_error("Failed to enable Frontend");
        goto error;
    }

    if (!module_load_sip(vocs)) {
        ov_log_error("Failed to enable SIP");
        goto error;
    }

    if (!module_load_sip_static(vocs)) {
        ov_log_error("Failed to enable SIP static");
        goto error;
    }

    if (!module_load_recorder(vocs)) {
        ov_log_error("Failed to enable Recorder");
        goto error;
    }

    if (!module_load_vad(vocs)) {
        ov_log_error("Failed to enable VAD");
        goto error;
    }

    if (config.trigger)
        ov_event_trigger_register_listener(
            config.trigger,
            "VOCS",
            (ov_event_trigger_data){
                .userdata = vocs, .process = process_trigger});

    ov_vocs_management_config mgmt_config = (ov_vocs_management_config){
        .loop = vocs->config.loop,
        .vocs = vocs,
        .db = vocs->config.db,
        .ldap = vocs->ldap,
        .env = vocs->config.env,
        .timeout.response_usec = vocs->config.timeout.response_usec
    };

    strncpy(mgmt_config.sessions.path,vocs->config.sessions.path,PATH_MAX);

    vocs->management = ov_vocs_management_create(mgmt_config);
    if (!vocs->management) goto error;

    vocs->callbacks = ov_callback_registry_create((ov_callback_registry_config){
        .loop = vocs->config.loop,
        .timeout_usec = vocs->config.timeout.response_usec
    });

    if (!vocs->callbacks) goto error;

    return vocs;
error:
    ov_vocs_free(vocs);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_vocs *ov_vocs_cast(const void *self) {

    if (!self) goto error;

    if (*(uint16_t *)self == OV_VOCS_MAGIC_BYTES) return (ov_vocs *)self;
error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

void *ov_vocs_free(void *self) {

    ov_vocs *vocs = ov_vocs_cast(self);
    if (!vocs) return self;

    vocs->backend = ov_mc_backend_free(vocs->backend);
    vocs->frontend = ov_mc_frontend_free(vocs->frontend);
    vocs->sip = ov_mc_backend_sip_free(vocs->sip);
    vocs->sip_static = ov_mc_backend_sip_static_free(vocs->sip_static);
    vocs->ldap = ov_ldap_free(vocs->ldap);
    vocs->recorder = ov_vocs_recorder_free(vocs->recorder);
    vocs->vad = ov_mc_backend_vad_free(vocs->vad);

    vocs->user_sessions = ov_event_session_free(vocs->user_sessions);

    vocs->async = ov_event_async_store_free(vocs->async);
    vocs->sessions = ov_dict_free(vocs->sessions);
    vocs->loops = ov_dict_free(vocs->loops);
    vocs->io = ov_dict_free(vocs->io);
    vocs->broadcasts = ov_broadcast_registry_free(vocs->broadcasts);
    vocs->connections = ov_socket_json_free(vocs->connections);
    vocs->management = ov_vocs_management_free(vocs->management);
    vocs->callbacks = ov_callback_registry_free(vocs->callbacks);

    self = ov_data_pointer_free(self);
    return NULL;
}

/*----------------------------------------------------------------------------*/
ov_event_io_config ov_vocs_event_io_uri_config(ov_vocs *self) {

    return (ov_event_io_config){.name = OV_KEY_VOCS,
                                .userdata = self,
                                .callback.close = cb_socket_close,
                                .callback.process = cb_client_process};
}

/*----------------------------------------------------------------------------*/

ov_event_io_config ov_vocs_mgmt_io_uri_config(ov_vocs *self) {

    return ov_vocs_management_io_uri_config(self->management);
}

/*----------------------------------------------------------------------------*/

ov_vocs_config ov_vocs_config_from_json(const ov_json_value *val) {

    ov_vocs_config out = {0};

    if (!val) goto error;

    const ov_json_value *config = ov_json_object_get(val, OV_KEY_VOCS);
    if (!config) config = val;

    out.module.backend = ov_mc_backend_config_from_json(config);
    out.module.frontend = ov_mc_frontend_config_from_json(config);
    out.module.sip = ov_mc_backend_sip_config_from_json(config);
    out.module.sip_static = ov_mc_backend_sip_static_config_from_json(config);
    out.module.recorder = ov_vocs_recorder_config_from_json(config);
    out.module.vad = ov_mc_backend_vad_config_from_json(config);

    out.timeout.response_usec = ov_json_number_get(
        ov_json_get(config, "/" OV_KEY_TIMEOUT "/" OV_KEY_RESPONSE));

    out.timeout.reconnect_interval_usec = ov_json_number_get(
        ov_json_get(config, "/" OV_KEY_TIMEOUT "/" OV_KEY_RECONNECT_TIMEOUT));

    if (ov_json_is_true(ov_json_get(val, "/" OV_KEY_LDAP "/" OV_KEY_ENABLED)))
        out.ldap.enable = true;

    out.ldap.config = ov_ldap_config_from_json(val);

    const char *session_path = ov_json_string_get(
        ov_json_get(config, "/" OV_KEY_SESSION "/" OV_KEY_PATH));

    if (session_path) strncpy(out.sessions.path, session_path, PATH_MAX);

    return out;
error:
    return (ov_vocs_config){0};
}

/*
 *      ------------------------------------------------------------------------
 *
 *      MANAGEMENT FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_mc_backend_registry_count ov_vocs_count_mixers(ov_vocs *self){

    return ov_mc_backend_state_mixers(self->backend);
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_vocs_get_connection_state(ov_vocs *self){

    ov_json_value *out = NULL;

    if (!self) goto error;

    out = ov_json_object();

    if (!ov_socket_json_for_each_set_data(self->connections, out)) goto error;

    return out;
error:
    ov_json_value_free(out);
    return NULL;
}

/*----------------------------------------------------------------------------*/

static void cb_mixer_state_mgmt(void *userdata, const char *uuid, const ov_json_value *input){

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *res = NULL;

    ov_vocs *vocs = ov_vocs_cast(userdata);
    if (!vocs || !uuid || !input) goto error;

    ov_callback cb = ov_callback_registry_get(vocs->callbacks, uuid);

    if (!cb.function) goto error;

    const char *session = ov_json_string_get(ov_json_get(input, "/"OV_KEY_PARAMETER"/"OV_KEY_SESSION));
    if (!session) goto error;

    intptr_t socket = (intptr_t)ov_dict_get(vocs->sessions, session);

    ov_json_value *data = ov_socket_json_get(vocs->connections, socket);

    const char *user = ov_json_string_get(ov_json_get(data, "/"OV_KEY_USER));
    const char *role = ov_json_string_get(ov_json_get(data, "/"OV_KEY_ROLE));

    out = ov_event_api_message_create("get_mixer_state", uuid, 0);
    res = ov_json_object();
    ov_json_object_set(out, OV_KEY_RESPONSE, res);

    ov_json_object_set(res, OV_KEY_SESSION, ov_json_string(session));

    if (role)
        ov_json_object_set(res, OV_KEY_ROLE, ov_json_string(role));

    if (user)
        ov_json_object_set(res, OV_KEY_USER, ov_json_string(user));

    ov_json_value *mixer = ov_event_api_get_response(input);
    val = NULL;

    ov_json_value_copy((void**)&val, mixer);

    ov_json_object_set(res, OV_KEY_MIXER, val);

    data = ov_json_value_free(data);

    void (*function)(void *userdata, int socket, ov_json_value *input) = cb.function;

    function(cb.userdata, cb.socket, out);

error:
    return;
}

/*----------------------------------------------------------------------------*/

struct mixer_data_mgmt {

    ov_vocs *vocs;
    const char *user;
    const char *uuid;

};

/*----------------------------------------------------------------------------*/

static bool call_mixer_state_mgmt(const void *key, void *val, void *data){

    if (!key) return true;

    struct mixer_data_mgmt *container = (struct mixer_data_mgmt*) data;

    const char *user = ov_json_string_get(ov_json_get(
        ov_json_value_cast(val), "/"OV_KEY_USER));

    if (!user) return true;

    if (container->user){

        if (0 != ov_string_compare(user, container->user))
            return true;

    }

    const char *session = ov_json_string_get(ov_json_get(
        ov_json_value_cast(val), "/"OV_KEY_SESSION));

    if (!session) return true;

    if (!ov_mc_backend_get_session_state(container->vocs->backend,
        container->uuid, session, container->vocs, cb_mixer_state_mgmt)) {
        goto error;
    
    }

    return true;

error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_vocs_get_mixer_state(ov_vocs *self,
    const char *uuid, const char *user, 
    void *userdata, int socket, void (*function)(void*, int, ov_json_value*)){

    if (!self || !function || !userdata) goto error;

    ov_callback cb = (ov_callback){
        .userdata = userdata,
        .function = function,
        .socket = socket
    };

    if (!ov_callback_registry_register(self->callbacks, uuid, cb, self->config.timeout.response_usec))
        goto error;

    struct mixer_data_mgmt container = (struct mixer_data_mgmt){
        .vocs = self,
        .user = user,
        .uuid = uuid
    };

    if (!ov_socket_json_for_each(self->connections, &container, 
        call_mixer_state_mgmt)){
        goto error;
    }

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_vocs_start_recording(ov_vocs *self,
    const char *uuid, const char *loop,
    void *userdata, int socket, void (*callback)(
                                        void *userdata,
                                        int socket,
                                        const char *uuid,
                                        const char *loop,
                                        ov_result result)){

    if (!self || !uuid || !loop || !userdata || !callback) goto error;

    return ov_vocs_recorder_start_loop_recording(
        self->recorder, uuid, loop, userdata, socket, callback);

error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_vocs_stop_recording(ov_vocs *self,
    const char *uuid, const char *loop,
    void *userdata, int socket, void (*callback)(
                                        void *userdata,
                                        int socket,
                                        const char *uuid,
                                        const char *loop,
                                        ov_result result)){

    if (!self || !uuid || !loop || !userdata || !callback) goto error;

    return ov_vocs_recorder_stop_loop_recording(
        self->recorder, uuid, loop, userdata, socket, callback);

error:
    return false;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_vocs_get_recorded_loops(ov_vocs *self){

    if (!self) return NULL;

    return ov_vocs_recorder_get_recorded_loops(self->recorder);
}

/*----------------------------------------------------------------------------*/

bool ov_vocs_send_broadcast(ov_vocs *self, int socket, ov_json_value *input){

    if (!self || !input) goto error;

    if (!ov_event_api_set_type(input, OV_BROADCAST_KEY_SYSTEM_BROADCAST))
        goto error;

    ov_event_parameter parameter =
        (ov_event_parameter){.send.instance = self, .send.send = send_socket};

    if (!ov_broadcast_registry_send(
            self->broadcasts, OV_BROADCAST_KEY_SYSTEM_BROADCAST, &parameter, input, 
            OV_SYSTEM_BROADCAST)) {

        send_error_response(self,
                            input,
                            socket,
                            OV_ERROR_CODE_PROCESSING_ERROR,
                            OV_ERROR_DESC_PROCESSING_ERROR);
    } else {

        send_success_response(self, input, socket, NULL);
        
    }

    return true;
error:
    return false;
}