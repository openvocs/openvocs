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
        @file           ov_interconnect.c
        @author         Töpfer, Markus

        @date           2026-01-15


        ------------------------------------------------------------------------
*/
#include "../include/ov_interconnect.h"
#include "../include/ov_interconnect_dtls_filter.h"
#include "../include/ov_interconnect_loop.h"
#include "../include/ov_interconnect_session.h"
#include "../include/ov_interconnect_msg.h"

#include <ov_base/ov_dict.h>
#include <ov_base/ov_string.h>
#include <ov_base/ov_error_codes.h>
#include <ov_base/ov_config_keys.h>

#include <ov_core/ov_event_app.h>
#include <ov_core/ov_event_api.h>

#include <ov_stun/ov_stun_binding.h>
#include <ov_stun/ov_stun_frame.h>

/*----------------------------------------------------------------------------*/

#define OV_INTERCONNECT_MAGIC_BYTES 0xab21

/*----------------------------------------------------------------------------*/

struct ov_interconnect {

    uint16_t magic_byte;
    ov_interconnect_config config;

    ov_dtls *dtls;

    struct {

        ov_event_app *signaling;
        ov_event_app *mixer;

    } app;

    struct {

        int signaling;
        int media;
        int mixer;

    } socket;

    struct {

        ov_dict *by_signaling_remote;
        ov_dict *by_media_remote;

    } session;

    ov_dict *loops;
    ov_dict *registered;

    ov_mixer_registry *mixers;

};


/*----------------------------------------------------------------------------*/

static ov_interconnect_session *get_session_by_signaling_socket(
    ov_interconnect *self, int socket) {

    char buf[OV_HOST_NAME_MAX + 20] = {0};

    if (!self)
        goto error;

    ov_socket_data remote = (ov_socket_data){0};
    ov_socket_get_data(socket, NULL, &remote);
    snprintf(buf, OV_HOST_NAME_MAX + 20, "%s:%i", remote.host, remote.port);
    return ov_dict_get(self->session.by_signaling_remote, buf);
error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

static bool drop_session_by_signaling_socket(
    ov_interconnect *self, int socket) {

    char buf[OV_HOST_NAME_MAX + 20] = {0};

    if (!self)
        goto error;

    ov_socket_data remote = (ov_socket_data){0};
    ov_socket_get_data(socket, NULL, &remote);
    snprintf(buf, OV_HOST_NAME_MAX + 20, "%s:%i", remote.host, remote.port);
    return ov_dict_del(self->session.by_signaling_remote, buf);
error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

static ov_interconnect_session *get_session_by_media_remote(
    ov_interconnect *self, ov_socket_data *remote) {

    char buf[OV_HOST_NAME_MAX + 20] = {0};

    if (!self || !remote)
        goto error;

    snprintf(buf, OV_HOST_NAME_MAX + 20, "%s:%i", remote->host, remote->port);
    return ov_dict_get(self->session.by_media_remote, buf);
error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

static bool drop_session_by_media_remote(
    ov_interconnect *self, ov_socket_data *remote) {

    char buf[OV_HOST_NAME_MAX + 20] = {0};

    if (!self || !remote)
        goto error;

    snprintf(buf, OV_HOST_NAME_MAX + 20, "%s:%i", remote->host, remote->port);
    return ov_dict_del(self->session.by_media_remote, buf);
error:
    return NULL;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      #MIXER #EVENTS
 *
 *      ------------------------------------------------------------------------
 */


struct container2 {

    ov_interconnect *self;
    int mixer_socket;
    bool assigned;

};

/*----------------------------------------------------------------------------*/

static bool assign_mixer(const void *key, void *val, void *data){

    if (!key) return true;
    struct container2 *container = (struct container2*)data;

    if (container->assigned) return true;

    ov_interconnect_loop *loop = (ov_interconnect_loop*) val;

    if (!ov_interconnect_loop_has_mixer(loop)){

        if (ov_interconnect_loop_assign_mixer(loop)){
            container->assigned = true;
        }
    }

    return true;
}

/*----------------------------------------------------------------------------*/

static bool assign_mixer_to_loops(ov_interconnect *self, int socket){

    if (!self) goto error;

    struct container2 container = (struct container2){
        .self = self,
        .mixer_socket = socket,
        .assigned = false
    };

    return ov_dict_for_each(
        self->loops,
        &container,
        assign_mixer);

error:
    return false;
}

/*----------------------------------------------------------------------------*/

static void event_mixer_register(
    void *userdata, const char *event_name, int socket, ov_json_value *input){

    ov_interconnect *self = ov_interconnect_cast(userdata);
    if (!self || !event_name || !input) goto error;

    ov_socket_data remote = {0};
    if (!ov_socket_get_data(socket, NULL, &remote)) goto error;

    if (!ov_mixer_registry_register_mixer(
        self->mixers, socket, &remote)) goto error;

    ov_log_debug("registered mixer %s:%i", remote.host, remote.port);

    ov_json_value *out = ov_mixer_msg_configure(self->config.mixer);
    ov_event_app_send(self->app.mixer, socket, out);
    out = ov_json_value_free(out);

    assign_mixer_to_loops(self, socket);

    input = ov_json_value_free(input);
    return;
error:
    input = ov_json_value_free(input);
    return;
}

/*----------------------------------------------------------------------------*/

struct container3 {

    int socket;
    ov_interconnect_loop *loop;

};

/*----------------------------------------------------------------------------*/

static bool find_loop_by_mixer(const void *key, void *val, void *data){

    if (!key) return true;

    struct container3 *container = (struct container3*) data; 

    if (container->loop) return true;

    ov_interconnect_loop *loop = (ov_interconnect_loop*) val;
    int socket = ov_interconnect_loop_get_mixer(loop);
    
    if (socket == container->socket)
        container->loop = loop;

    return true;
}

/*----------------------------------------------------------------------------*/

static void event_mixer_acquire(
    void *userdata, const char *event_name, int socket, ov_json_value *input){

    ov_interconnect *self = ov_interconnect_cast(userdata);
    if (!self || !event_name || !input) goto error;

    ov_json_value *res = ov_event_api_get_response(input);
    if (!res) goto error;

    bool success = false;

    if (0 == ov_event_api_get_error_code(input))
        success = true;

    struct container3 container = (struct container3){
        .loop = NULL,
        .socket = socket
    };

    ov_dict_for_each(self->loops, &container, find_loop_by_mixer);

    if (success && container.loop){

        ov_json_value *out = ov_mixer_msg_join(
            ov_interconnect_loop_get_loop_data(container.loop));

        ov_event_app_send(self->app.mixer, socket, out);
        out = ov_json_value_free(out);

    } else {

        ov_log_error("Aquire not successfull - TBD");
    }

    ov_json_value_free(input);
    return;
error:
    ov_json_value_free(input);
    return;
}

/*----------------------------------------------------------------------------*/
/*
static void event_mixer_forward(
    void *userdata, const char *event_name, int socket, ov_json_value *input){

    ov_interconnect *self = ov_interconnect_cast(userdata);
    if (!self || !event_name || !input) goto error;

    ov_json_value *res = ov_event_api_get_response(input);
    if (!res) goto error;

    bool success = false;

    if (0 == ov_event_api_get_error_code(input))
        success = true;

    struct container3 container = (struct container3){
        .loop = NULL,
        .socket = socket
    };

    ov_dict_for_each(self->loops, &container, find_loop_by_mixer);

    if (success && container.loop){

        ov_json_value *out = ov_mixer_msg_join(
            ov_interconnect_loop_get_loop_data(container.loop));

        ov_event_app_send(self->app.mixer, socket, out);
        out = ov_json_value_free(out);
   
    } else {

        ov_log_error("forward not successfull - TBD");

    }

    ov_json_value_free(input);
    return;
error:
    ov_json_value_free(input);
    return;
}
*/

/*----------------------------------------------------------------------------*/

static void event_mixer_join(
    void *userdata, const char *event_name, int socket, ov_json_value *input){

    ov_interconnect *self = ov_interconnect_cast(userdata);
    if (!self || !event_name || !input) goto error;

    ov_json_value *res = ov_event_api_get_response(input);
    if (!res) goto error;

    bool success = false;

    if (0 == ov_event_api_get_error_code(input))
        success = true;

    struct container3 container = (struct container3){
        .loop = NULL,
        .socket = socket
    };

    ov_dict_for_each(self->loops, &container, find_loop_by_mixer);

    if (success && container.loop){

        ov_log_info("MIXER for loop %s - joined", 
            ov_interconnect_loop_get_name(container.loop));
   
    } else {

        ov_log_error("join not successfull - TBD");

    }

    ov_json_value_free(input);
    return;
error:
    ov_json_value_free(input);
    return;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      #SIGNALING #EVENTS
 *
 *      ------------------------------------------------------------------------
 */

static void event_register_response(
    ov_interconnect *self, int socket, ov_json_value *input){

    ov_json_value *out = NULL;

    if (!self || !input) goto error;

    uint64_t error_code = ov_event_api_get_error_code(input);
    const char *error_desc = ov_event_api_get_error_desc(input);

    if (0 != error_code) {
        ov_log_error("Register error %i|%s", error_code, error_desc);
        goto error;
    }

    const char *name = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_RESPONSE "/" OV_KEY_NAME));

    ov_log_debug("REGISTER SUCCESS at %i remote |%s|", socket, name);

    ov_interconnect_session *session = get_session_by_signaling_socket(
        self, socket);

    if (!session){

        out = ov_interconnect_msg_connect_media(
        self->config.name, 
        OV_INTERCONNECT_DEFAULT_CODEC,
        self->config.socket.media.host, 
        self->config.socket.media.port);

        bool result = ov_event_app_send(self->app.signaling, socket, out);
        
        if (result)
            ov_log_debug("SEND msg connect.");

        ov_json_value_free(out);
    }
   
    ov_json_value_free(input);

    return;
error:
    ov_json_value_free(input);
    return;
}

/*----------------------------------------------------------------------------*/

static void event_signaling_register(
    void *userdata, const char *event_name, int socket, ov_json_value *input){

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *res = NULL;

    ov_interconnect *self = ov_interconnect_cast(userdata);
    if (!self || !event_name || !input) goto error;
    
    if (ov_event_api_get_response(input)){
        event_register_response(self, socket, input);
        return;
    }

    const char *name = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_NAME));

    const char *pass = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_PASSWORD));

    if (!name || !pass) {

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_PARAMETER_ERROR,
                                                 OV_ERROR_DESC_PARAMETER_ERROR);

        goto send_response;
    }

    if (0 != strcmp(pass, self->config.password)) {

        out = ov_event_api_create_error_response(input, OV_ERROR_CODE_AUTH,
                                                 OV_ERROR_DESC_AUTH);

        goto send_response;
    }

    ov_log_debug("REGISTER at %i remote |%s|", socket, name);

    intptr_t key = (intptr_t)socket;
    ov_dict_set(self->registered, (void *)key, NULL, NULL);

    out = ov_event_api_create_success_response(input);
    res = ov_event_api_get_response(out);
    val = ov_json_string(self->config.name);
    if (!ov_json_object_set(res, OV_KEY_NAME, val))
        goto error;
    val = NULL;

send_response:

    ov_event_app_send(self->app.signaling, socket, out);
    ov_json_value_free(out);
    ov_json_value_free(input);

    return;
error:
    ov_json_value_free(out);
    ov_json_value_free(val);
    ov_json_value_free(input);
    return;
}

/*----------------------------------------------------------------------------*/

static ov_interconnect_session *session_create(
    ov_interconnect *self, 
    ov_interconnect_session_config config,
    int socket){

    char buf[OV_HOST_NAME_MAX + 20] = {0};

    ov_socket_get_data(socket, NULL, &config.remote.signaling);

    ov_interconnect_session *session = ov_interconnect_session_create(config);
    if (!session) goto error;

    char *key = NULL;

    memset(buf, 0, OV_HOST_NAME_MAX + 20);

    snprintf(buf, OV_HOST_NAME_MAX + 20, "%s:%i", 
        config.remote.signaling.host,
        config.remote.signaling.port);

    key = ov_string_dup(buf);
    ov_dict_set(self->session.by_signaling_remote, key, session, NULL);

    memset(buf, 0, OV_HOST_NAME_MAX + 20);

    snprintf(buf, OV_HOST_NAME_MAX + 20, "%s:%i", 
        config.remote.media.host, 
        config.remote.media.port);
    
    key = ov_string_dup(buf);
    ov_dict_set(self->session.by_media_remote, key, session, NULL);

    return session;
error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

static void event_connect_media_response(
    ov_interconnect *self, int socket, ov_json_value *input){

    ov_interconnect_session *session = NULL;

    if (!self || !input) goto error;
    UNUSED(socket);

    uint64_t error_code = ov_event_api_get_error_code(input);
    const char *error_desc = ov_event_api_get_error_desc(input);

    if (0 != error_code) {
        ov_log_error("MEDIA error %i|%s", error_code, error_desc);
        goto error;
    }

    const char *host = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_RESPONSE "/" OV_KEY_HOST));

    const char *name = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_RESPONSE "/" OV_KEY_NAME));

    const char *finger = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_RESPONSE "/" OV_KEY_FINGERPRINT));

    uint32_t port = ov_json_number_get(
        ov_json_get(input, "/" OV_KEY_RESPONSE "/" OV_KEY_PORT));

    if (!host || !name || !finger || 0 == port){

        ov_log_error("MEDIA response parameter missing.");
        return;
    }

    ov_log_debug("got remote media parameter |%s| %s:%i", name, host, port);

    // start DTLS active to remote

    ov_interconnect_session_config config =
        (ov_interconnect_session_config){
            .base = self,
            .loop = self->config.loop,
            .dtls = self->dtls,
            .internal = self->config.socket.internal,
            .reconnect_interval_usecs = 100000,
            .keepalive_trigger_usec =
                self->config.limits.keepalive_trigger_usec};

    ov_socket_data remote = (ov_socket_data){0};
    if (!ov_socket_get_data(socket, NULL, &remote))
        goto error;

    config.remote.signaling = remote;
    config.signaling = socket;
    strncpy(config.remote.media.host, host, OV_HOST_NAME_MAX);
    config.remote.media.port = port;
    strncpy(config.remote.interface, name,
            OV_INTERCONNECT_INTERFACE_NAME_MAX);

    session = session_create(self, config, socket);
    if (!session)
        goto error;

    if (!ov_interconnect_session_handshake_active(session, finger))
        goto error;

    ov_json_value_free(input);
    return;
error:
    ov_json_value_free(input);
    return;
}

/*----------------------------------------------------------------------------*/

static void event_signaling_connect_media(
    void *userdata, const char *event_name, int socket, ov_json_value *input){

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *res = NULL;

    ov_interconnect_session *session = NULL;

    ov_interconnect *self = ov_interconnect_cast(userdata);
    if (!self || !event_name || !input)
        goto error;

    if (ov_event_api_get_response(input)){
        event_connect_media_response(self, socket, input);
        return;
    }

    if (!ov_dict_is_set(self->registered, (void *)(intptr_t)socket)) {
        ov_log_error("GOT MEDIA - client not registered yet. - ignoring");
        goto error;
    }

    const char *name = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_NAME));

    const char *codec = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_CODEC));

    const char *host = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_HOST));

    uint64_t port = ov_json_number_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_PORT));

    if (!name || !codec || !host || !port) {

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_PARAMETER_ERROR,
                                                 OV_ERROR_DESC_PARAMETER_ERROR);

        goto send_response;
    }

    if (0 != strcmp(codec, OV_INTERCONNECT_DEFAULT_CODEC)) {

        out = ov_event_api_create_error_response(
            input, OV_ERROR_CODE_CODEC_ERROR, OV_ERROR_DESC_CODEC_ERROR);

        goto send_response;
    }

    ov_interconnect_session_config config =
        (ov_interconnect_session_config){
            .base = self,
            .loop = self->config.loop,
            .dtls = self->dtls,
            .keepalive_trigger_usec =
                self->config.limits.keepalive_trigger_usec};

    ov_socket_data remote = (ov_socket_data){0};
    if (!ov_socket_get_data(socket, NULL, &remote))goto error;

    config.remote.signaling = remote;
    config.signaling = socket;
    strncpy(config.remote.media.host, host, OV_HOST_NAME_MAX);
    config.remote.media.port = port;
    strncpy(config.remote.interface, name,
            OV_INTERCONNECT_INTERFACE_NAME_MAX);

    session = session_create(self, config, socket);
    if (!session) goto error;

    out = ov_event_api_create_success_response(input);
    res = ov_event_api_get_response(out);
    val = ov_json_number(self->config.socket.media.port);
    if (!ov_json_object_set(res, OV_KEY_PORT, val)) goto error;
    val = ov_json_string(self->config.socket.media.host);
    if (!ov_json_object_set(res, OV_KEY_HOST, val)) goto error;
    val = ov_json_string(ov_dtls_get_fingerprint(self->dtls));
    if (!ov_json_object_set(res, OV_KEY_FINGERPRINT, val)) goto error;
    val = ov_json_string(self->config.name);
    if (!ov_json_object_set(res, OV_KEY_NAME, val)) goto error;
    val = NULL;

    ov_log_debug("GOT MEDIA INVITE |%s| %s:%i ", name, host, port);

send_response:

    ov_event_app_send(self->app.signaling, socket, out);
    ov_json_value_free(out);
    ov_json_value_free(input);

    return;
error:
    ov_json_value_free(out);
    ov_json_value_free(val);
    ov_json_value_free(input);
    return;
}

/*----------------------------------------------------------------------------*/

struct container {

    ov_json_value *array;
    ov_interconnect *self;
    ov_interconnect_session *session;
};

/*----------------------------------------------------------------------------*/

static bool add_loops_response(void *value, void *data) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;

    ov_json_value *item = ov_json_value_cast(value);

    struct container *c = (struct container *)data;
    if (!item || !data)
        goto error;

    const char *name = ov_json_string_get(ov_json_object_get(item, OV_KEY_NAME));

    uint32_t ssrc = ov_json_number_get(ov_json_object_get(item, OV_KEY_SSRC));

    if (!name || !ssrc)
        goto error;

    ov_interconnect_loop *loop = ov_dict_get(c->self->loops, name);
    if (!loop) goto done;

    if (!ov_interconnect_session_add(c->session, name, ssrc)) goto done;

    ov_log_debug("adding Loop %s to session.", name);

done:
    return true;

error:
    ov_json_value_free(val);
    ov_json_value_free(out);
    return false;
}

/*----------------------------------------------------------------------------*/

static void event_connect_loops_response(
    ov_interconnect *self, int socket, ov_json_value *input){

    if (!self || !socket || !input)
        goto error;

    const ov_json_value *loops =
        ov_json_get(input, "/" OV_KEY_RESPONSE "/" OV_KEY_LOOPS);
    if (!loops)
        goto error;

    ov_interconnect_session *session = get_session_by_signaling_socket(self, socket);

    if (!session) goto error;

    struct container container =
        (struct container){.self = self, .session = session};

    if (!ov_json_array_for_each((ov_json_value *)loops, &container,
                                add_loops_response)) {

        goto error;
    }

    ov_json_value_free(input);
    return;

error:
    ov_json_value_free(input);
    return;
}

/*----------------------------------------------------------------------------*/

static bool add_loops(void *value, void *data) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;

    ov_json_value *item = ov_json_value_cast(value);

    struct container *c = (struct container *)data;
    if (!item || !data) goto error;

    const char *name =
        ov_json_string_get(ov_json_object_get(item, OV_KEY_NAME));

    uint32_t ssrc = ov_json_number_get(ov_json_object_get(item, OV_KEY_SSRC));

    if (!name || !ssrc) goto error;

    ov_log_debug("adding loop %s - %lu", name, ssrc);

    ov_interconnect_loop *loop = ov_dict_get(c->self->loops, name);
    if (!loop) goto done;

    if (!ov_interconnect_session_add(c->session, name, ssrc)) goto done;

    uint32_t local_ssrc = ov_interconnect_loop_get_ssrc(loop);

    out = ov_json_object();
    val = ov_json_string(name);

    if (!ov_json_object_set(out, OV_KEY_NAME, val)) goto error;
    val = ov_json_number(local_ssrc);

    if (!ov_json_object_set(out, OV_KEY_SSRC, val)) goto error;
    val = NULL;

    if (!ov_json_array_push(c->array, out)) goto error;

done:
    return true;

error:
    ov_json_value_free(val);
    ov_json_value_free(out);
    return false;
}

/*----------------------------------------------------------------------------*/

static void event_signaling_connect_loops(
    void *userdata, const char *name, int socket, ov_json_value *input){

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *res = NULL;

    ov_interconnect *self = ov_interconnect_cast(userdata);
    if (!self || !input || !name) goto error;

    if (ov_event_api_get_response(input)){
        event_connect_loops_response(self, socket, input);
        return;
    }

    const ov_json_value *loops =
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_LOOPS);

    if (!loops) {

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_PARAMETER_ERROR,
                                                 OV_ERROR_DESC_PARAMETER_ERROR);

        goto send_response;
    }

    ov_interconnect_session *session =
        get_session_by_signaling_socket(self, socket);

    if (!session) {

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_SESSION_UNKNOWN,
                                                 OV_ERROR_DESC_SESSION_UNKNOWN);

        goto send_response;
    }

    val = ov_json_array();

    struct container container = (struct container){0};
    container.array = val;
    container.session = session;
    container.self = self;

    if (!ov_json_array_for_each((ov_json_value *)loops, &container,
                                add_loops)) {

        out = ov_event_api_create_error_response(
            input, OV_ERROR_CODE_PROCESSING_ERROR,
            OV_ERROR_DESC_PROCESSING_ERROR);

        goto send_response;
    }

    out = ov_event_api_create_success_response(input);
    res = ov_event_api_get_response(out);
    if (!ov_json_object_set(res, OV_KEY_LOOPS, val))
        goto error;
    val = NULL;

send_response:

    ov_event_app_send(self->app.signaling, socket, out);
    ov_json_value_free(out);
    ov_json_value_free(input);
    return;
error:
    ov_json_value_free(out);
    ov_json_value_free(val);
    ov_json_value_free(input);
    return;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      #IO FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

static bool io_stun(ov_interconnect *self, int socket, uint8_t *buffer,
                    size_t bytes, ov_socket_data *remote) {

    UNUSED(self);

    socklen_t len = sizeof(struct sockaddr_in);
    if (remote->sa.ss_family == AF_INET6)
        len = sizeof(struct sockaddr_in6);

    if (!ov_stun_frame_is_valid(buffer, bytes))
        goto ignore;
    if (!ov_stun_frame_has_magic_cookie(buffer, bytes))
        goto ignore;
    if (ov_stun_frame_class_is_success_response(buffer, bytes)) {
        ov_log_debug("Received STUN response from %s:%i", remote->host,
                     remote->port);
        goto ignore;
    }
    if (!ov_stun_frame_class_is_request(buffer, bytes))
        goto ignore;
    if (!ov_stun_method_is_binding(buffer, bytes))
        goto ignore;

    /* plain STUN request processing */

    if (!ov_stun_frame_set_success_response(buffer, bytes))
        goto error;

    if (!ov_stun_xor_mapped_address_encode(buffer + 20, bytes - 20, buffer,
                                           NULL, &remote->sa))
        goto error;

    size_t out = 20 + ov_stun_xor_mapped_address_encoding_length(&remote->sa);

    if (!ov_stun_frame_set_length(buffer, bytes, out - 20))
        goto error;

    // just to be sure, we nullify the rest of the buffer
    memset(buffer + out, 0, bytes - out);

    ssize_t send = sendto(socket, buffer, out, 0,
                          (const struct sockaddr *)&remote->sa, len);

    ov_log_debug("Send STUN response to %s:%i", remote->host, remote->port);
    UNUSED(send);
ignore:
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool io_external_media_rtp(ov_interconnect *self, uint8_t *buffer,
                            size_t bytes, ov_socket_data *remote) {

    ov_interconnect_session *session = NULL;
    
    if (!self || !buffer || !bytes || !remote)
        goto error;

    session = get_session_by_media_remote(self, remote);
    if (!session) {

        ov_log_debug("Got RTP from %s:%i - no session ignoring",
            remote->host, remote->port);

        goto done;
    }

    ov_interconnect_session_media_io_external(
        session,
        buffer, 
        bytes,
        remote);

done:
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool io_external_media_ssl(ov_interconnect *self, uint8_t *buffer,
                            size_t bytes, ov_socket_data *remote) {

    ov_interconnect_session *session = NULL;

    if (!self || !buffer || !bytes || !remote)
        goto error;

    session = get_session_by_media_remote(self, remote);
    if (!session) {

        ov_log_debug("Got SSL from %s:%i - no session ignoring",
            remote->host, remote->port);

        goto done;
    }

    ov_interconnect_session_media_io_ssl_external(
        session,
        buffer, 
        bytes,
        remote);

done:
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool io_external_media(int socket, uint8_t events, void *userdata){

    uint8_t buffer[OV_UDP_PAYLOAD_OCTETS] = {0};
    ov_socket_data remote = {};
    socklen_t src_addr_len = sizeof(remote.sa);

    ov_interconnect *self = ov_interconnect_cast(userdata);
    if (!self) goto error;

    if ((events & OV_EVENT_IO_CLOSE) || (events & OV_EVENT_IO_ERR)) {

        ov_log_debug("%i - closing", socket);
        return true;
    }

    ssize_t bytes = recvfrom(socket, (char *)buffer, OV_UDP_PAYLOAD_OCTETS, 0,
                             (struct sockaddr *)&remote.sa, &src_addr_len);

    if (bytes < 1)
        goto error;

    if (!ov_socket_parse_sockaddr_storage(&remote.sa, remote.host,
                                          OV_HOST_NAME_MAX, &remote.port))
        goto error;

    /*  -----------------------------------------------------------------
     *      RFC 7983 paket forwarding
     *
     *                        BYTE 1
     *                  +----------------+
     *                  |        [0..3] -+--> forward to STUN
     *                  |                |
     *                  |      [16..19] -+--> forward to ZRTP
     *                  |                |
     *      packet -->  |      [20..63] -+--> forward to DTLS
     *                  |                |
     *                  |      [64..79] -+--> forward to TURN Channel
     *                  |                |
     *                  |    [128..191] -+--> forward to RTP/RTCP
     *                  +----------------+
     *   -----------------------------------------------------------------
     *
     *      We will get ANY non STUN non DTLS data here.
     *
     *      If some SSL header is present,
     *      process SSL
     */

    if (buffer[0] <= 3)
        return io_stun(self, socket, buffer, bytes, &remote);

    if (buffer[0] <= 63 && buffer[0] >= 20) {

        return io_external_media_ssl(self, buffer, bytes, &remote);
    }

    if (buffer[0] <= 191 && buffer[0] >= 128) {

        return io_external_media_rtp(self, buffer, bytes, &remote);
    }

    return true;
error:
    return false;
}


/*
 *      ------------------------------------------------------------------------
 *
 *      CALLBACK FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

static void cb_signaling_close(void *userdata, int socket){

    ov_interconnect *self = ov_interconnect_cast(userdata);

    ov_socket_data remote = (ov_socket_data){0};
    ov_socket_get_data(socket, NULL, &remote);

    drop_session_by_signaling_socket(self, socket);
    drop_session_by_media_remote(self, &remote);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_signaling_connected(void *userdata, int socket){

    ov_interconnect *self = ov_interconnect_cast(userdata);
    if (!self) goto error;

    ov_log_debug("opened connection %i to %s:%i", socket,
               self->config.socket.signaling.host,
               self->config.socket.signaling.port);

    ov_json_value *msg =
        ov_interconnect_msg_register(self->config.name, self->config.password);

    ov_event_app_send(self->app.signaling, socket, msg);
    msg = ov_json_value_free(msg);

    // client initiated connection to remote, so we need to
    // register that connection to allow access within the responses
    intptr_t key = socket;
    ov_dict_set(self->registered, (void *)key, NULL, NULL);
error: 
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_mixer_close(void *userdata, int socket){

    ov_interconnect *self = ov_interconnect_cast(userdata);

    UNUSED(self);
    UNUSED(socket);

    TODO("... mixer socket closed.");

    return;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

static bool open_sockets(ov_interconnect *self){

    if (!self) goto error;

    // step 1 open signaling socket

    ov_io_socket_config sig = (ov_io_socket_config){
        .auto_reconnect = true,
        .socket = self->config.socket.signaling,
        .callbacks = (ov_io_callback){
            .userdata = self,
            .accept = NULL,
            .io = NULL, // done in app
            .close = cb_signaling_close,
            .connected = cb_signaling_connected
        }
    };

    if (self->config.tls.client.domain[0] != 0)
        strncpy(sig.ssl.domain, self->config.tls.client.domain, PATH_MAX);

    if (self->config.tls.client.ca.file[0] != 0)
        strncpy(sig.ssl.ca.file, self->config.tls.client.ca.file, PATH_MAX);

    if (self->config.tls.client.ca.path[0] != 0)
        strncpy(sig.ssl.ca.path, self->config.tls.client.ca.path, PATH_MAX);

    if (self->config.socket.client){

        self->socket.signaling = ov_event_app_open_connection(
            self->app.signaling, sig);

        if (-1 != self->socket.signaling)
            cb_signaling_connected(self, self->socket.signaling);

    } else {

        self->socket.signaling = ov_event_app_open_listener(
            self->app.signaling, sig);

        if (-1 == self->socket.signaling){

            ov_log_error("Could not open signaling socket %s:%i",
                self->config.socket.signaling.host,
                self->config.socket.signaling.port);

            goto error;

        } else {

            ov_log_info("opened signaling socket %s:%i",
                self->config.socket.signaling.host,
                self->config.socket.signaling.port);
        }
    }

    // step 2 open mixer socket

    ov_io_socket_config mixer = (ov_io_socket_config){
        .auto_reconnect = true,
        .socket = self->config.socket.mixer,
        .callbacks = (ov_io_callback){
            .userdata = self,
            .accept = NULL,
            .io = NULL, // done in app
            .close = cb_mixer_close,
            .connected = NULL
        }
    };

    self->socket.mixer = ov_event_app_open_listener(
        self->app.mixer, mixer);

    if (-1 == self->socket.mixer){

        ov_log_error("Could not open mixer socket %s:%i",
            self->config.socket.mixer.host,
            self->config.socket.mixer.port);

            goto error;

    } else {

        ov_log_info("opened mixer socket %s:%i",
            self->config.socket.mixer.host,
            self->config.socket.mixer.port);
    }

    // step 3 open media socket

    self->socket.media = ov_socket_create(self->config.socket.media, false, NULL);
    ov_socket_ensure_nonblocking(self->socket.media);

    if (!ov_event_loop_set(
        self->config.loop,
        self->socket.media,
        OV_EVENT_IO_IN | OV_EVENT_IO_ERR | OV_EVENT_IO_CLOSE,
        self,
        io_external_media)) goto error;

    if (-1 == self->socket.media){

        ov_log_error("Could not open media socket %s:%i",
            self->config.socket.media.host,
            self->config.socket.media.port);

            goto error;

    } else {

        ov_log_info("opened media socket %s:%i",
            self->config.socket.media.host,
            self->config.socket.media.port);
    }

    return true;

error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool register_events(ov_interconnect *self){

    if (!self) goto error;

    // events interconnect signaling

    if (!ov_event_app_register(
        self->app.signaling,
        "register",
        self,
        event_signaling_register)) goto error;

    if (!ov_event_app_register(
        self->app.signaling,
        "connect_media",
        self,
        event_signaling_connect_media)) goto error;

    if (!ov_event_app_register(
        self->app.signaling,
        "connect_loops",
        self,
        event_signaling_connect_loops)) goto error;

    // events mixer signaling

    if (!ov_event_app_register(
        self->app.mixer,
        "register",
        self,
        event_mixer_register)) goto error;

    if (!ov_event_app_register(
        self->app.mixer,
        "acquire",
        self,
        event_mixer_acquire)) goto error;
/*
    if (!ov_event_app_register(
        self->app.mixer,
        "forward",
        self,
        event_mixer_forward)) goto error;
*/
    if (!ov_event_app_register(
        self->app.mixer,
        "join",
        self,
        event_mixer_join)) goto error;

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool check_config(ov_interconnect_config *config) {

    if (!config->loop)
        goto error;
    if (0 == config->socket.signaling.host[0])
        goto error;
    if (0 == config->socket.media.host[0])
        goto error;
    if (0 == config->socket.mixer.host[0])
        goto error;
    if (0 == config->name[0])
        goto error;
    if (0 == config->password[0])
        goto error;

    if (0 == config->limits.client_connect_trigger_usec)
        config->limits.client_connect_trigger_usec = 100000;

    if (0 == config->limits.keepalive_trigger_usec)
        config->limits.keepalive_trigger_usec = 300000000;

    config->dtls.loop = config->loop;

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

ov_interconnect *ov_interconnect_create(ov_interconnect_config config){

    ov_interconnect *self = NULL;

    if (!check_config(&config)) goto error;

    self = calloc(1, sizeof(ov_interconnect));
    if (!self) goto error;

    self->magic_byte = OV_INTERCONNECT_MAGIC_BYTES;
    self->config = config;

    self->dtls = ov_dtls_create(self->config.dtls);
    if (!self->dtls) goto error;

    self->app.signaling = ov_event_app_create(
        (ov_event_app_config){
            .io = config.io,
            .callbacks.userdata = self,
            .callbacks.close = cb_signaling_close,
            .callbacks.connected = cb_signaling_connected
        });

    self->app.mixer = ov_event_app_create(
        (ov_event_app_config){
            .io = config.io,
            .callbacks.userdata = self,
            .callbacks.close = cb_mixer_close
        });

    ov_dict_config d_config = ov_dict_string_key_config(255);
    d_config.value.data_function.free = ov_interconnect_loop_free;

    self->loops = ov_dict_create(d_config);

    d_config = ov_dict_string_key_config(255);
    d_config.value.data_function.free = NULL;

    self->session.by_media_remote = ov_dict_create(d_config);
    if (!self->session.by_media_remote) goto error;

    d_config = ov_dict_string_key_config(255);
    d_config.value.data_function.free = ov_interconnect_session_free;

    self->session.by_signaling_remote = ov_dict_create(d_config);
    if (!self->session.by_signaling_remote) goto error;

    d_config = ov_dict_intptr_key_config(255);
    d_config.value.data_function.free = NULL;

    self->registered = ov_dict_create(d_config);
    if (!self->registered) goto error;

    self->mixers = ov_mixer_registry_create((ov_mixer_registry_config){0});

    ov_interconnect_dtls_filter_init();
    srtp_init();

    if (!open_sockets(self)) goto error;

    if (!register_events(self)) goto error;

    return self;
error:
    ov_interconnect_free(self);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_interconnect *ov_interconnect_free(ov_interconnect *self) {

    if (!ov_interconnect_cast(self)) goto error;

    if (-1 != self->socket.signaling) {
        ov_event_app_close(self->app.signaling, self->socket.signaling);
    }

    if (-1 != self->socket.mixer) {
        ov_event_app_close(self->app.mixer, self->socket.mixer);
    }

    if (-1 != self->socket.media) {
        ov_event_loop_unset(self->config.loop, self->socket.media, NULL);
        close(self->socket.media);
        self->socket.media = -1;
    }

    self->loops = ov_dict_free(self->loops);
    self->session.by_signaling_remote =
        ov_dict_free(self->session.by_signaling_remote);
    self->session.by_media_remote = ov_dict_free(self->session.by_media_remote);

    self->dtls = ov_dtls_free(self->dtls);
    self->app.signaling = ov_event_app_free(self->app.signaling);
    self->app.mixer = ov_event_app_free(self->app.mixer);
    self->mixers = ov_mixer_registry_free(self->mixers);
    ov_interconnect_dtls_filter_deinit();
    self = ov_data_pointer_free(self);
error:
    return self;
}

/*----------------------------------------------------------------------------*/

ov_interconnect *ov_interconnect_cast(const void *data) {

    if (!data)
        return NULL;

    if (*(uint16_t *)data != OV_INTERCONNECT_MAGIC_BYTES)
        return NULL;

    return (ov_interconnect *)data;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      CONFIG FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_interconnect_config ov_interconnect_config_from_json(const ov_json_value *val){

    ov_interconnect_config config = {0};
    if (!val)
        goto error;

    const ov_json_value *conf = ov_json_object_get(val, OV_KEY_INTERCONNECT);
    if (!conf)
        conf = val;

    if (ov_json_is_true(
            ov_json_get(conf, "/" OV_KEY_SOCKET "/" OV_KEY_CLIENT))) {
        config.socket.client = true;
    } else {
        config.socket.client = false;
    }

    config.socket.signaling = ov_socket_configuration_from_json(
        ov_json_get(conf, "/" OV_KEY_SOCKET "/" OV_KEY_SIGNALING),
        (ov_socket_configuration){
            .type = TLS, .host = "localhost", .port = 12345});

    config.socket.mixer = ov_socket_configuration_from_json(
        ov_json_get(conf, "/" OV_KEY_SOCKET "/mixer"),
        (ov_socket_configuration){
            .type = TLS, .host = "localhost", .port = 12345});

    config.socket.media = ov_socket_configuration_from_json(
        ov_json_get(conf, "/" OV_KEY_SOCKET "/" OV_KEY_MEDIA),
        (ov_socket_configuration){
            .type = UDP, .host = "localhost", .port = 12345});

    config.socket.internal = ov_socket_configuration_from_json(
        ov_json_get(conf, "/" OV_KEY_SOCKET "/" OV_KEY_INTERNAL),
        (ov_socket_configuration){
            .type = UDP, .host = "localhost", .port = 12346});

    const char *str = ov_json_string_get(
        ov_json_get(conf, "/" OV_KEY_TLS "/" OV_KEY_DOMAINS));

    if (str)
        strncpy(config.tls.domains, str, PATH_MAX);

    str = ov_json_string_get(
        ov_json_get(conf, "/" OV_KEY_TLS "/" OV_KEY_CLIENT "/" OV_KEY_DOMAIN));

    if (str)
        strncpy(config.tls.client.domain, str, PATH_MAX);

    str = ov_json_string_get(
        ov_json_get(conf, "/" OV_KEY_TLS "/" OV_KEY_CLIENT "/" OV_KEY_CA_FILE));

    if (str)
        strncpy(config.tls.client.ca.file, str, PATH_MAX);

    str = ov_json_string_get(
        ov_json_get(conf, "/" OV_KEY_TLS "/" OV_KEY_CLIENT "/" OV_KEY_CA_PATH));

    if (str)
        strncpy(config.tls.client.ca.path, str, PATH_MAX);

    str = ov_json_string_get(ov_json_get(conf, "/" OV_KEY_NAME));

    if (str)
        strncpy(config.name, str, OV_INTERCONNECT_INTERFACE_NAME_MAX);

    str = ov_json_string_get(ov_json_get(conf, "/" OV_KEY_PASSWORD));

    if (str)
        strncpy(config.password, str, OV_INTERCONNECT_PASSWORD_MAX);

    config.limits.client_connect_trigger_usec = ov_json_number_get(ov_json_get(
        conf, "/" OV_KEY_LIMITS "/" OV_KEY_RECONNECT_INTERVAL_SECS));

    config.limits.keepalive_trigger_usec = ov_json_number_get(
        ov_json_get(conf, "/" OV_KEY_LIMITS "/" OV_KEY_KEEPALIVE_SEC));

    config.limits.client_connect_trigger_usec *= 1000000;
    config.limits.keepalive_trigger_usec *= 1000000;

    config.dtls = ov_dtls_config_from_json(val);

    config.mixer = ov_mixer_config_from_json(conf);

    return config;

error:
    return (ov_interconnect_config){0};
}

/*----------------------------------------------------------------------------*/

static bool load_loop(const void *key, void *val, void *data) {

    if (!key)
        return true;
    if (!val || !data)
        goto error;

    ov_interconnect *self = ov_interconnect_cast(data);

    const char *name = (char *)key;

    ov_socket_configuration socket_config =
        ov_socket_configuration_from_json(val, (ov_socket_configuration){0});
    
    socket_config.type = UDP;

    if (!name || (0 == socket_config.host[0]) || (0 == socket_config.port))
        goto error;

    ov_interconnect_loop_config config = 
    (ov_interconnect_loop_config){
        .loop = self->config.loop, 
        .base = self};

    strncpy(config.name, name, OV_HOST_NAME_MAX);
    config.multicast = socket_config;
    config.internal = self->config.socket.internal;

    ov_interconnect_loop *loop = ov_interconnect_loop_create(config);

    if (!loop) goto error;

    char *k = strdup(name);

    if (!ov_dict_set(self->loops, k, loop, NULL)) {
        k = ov_data_pointer_free(k);
        loop = ov_interconnect_loop_free(loop);
        goto error;
    }

    ov_log_debug("loaded loop %s|%s:%i", 
        name, 
        socket_config.host,
        socket_config.port);

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_interconnect_load_loops(ov_interconnect *self, const ov_json_value *config){

    if (!self || !config)
        goto error;

    const ov_json_value *loops =
        ov_json_get(config, "/" OV_KEY_INTERCONNECT "/" OV_KEY_LOOPS);

    if (!loops)
        goto done;

    if (!ov_json_object_for_each((ov_json_value *)loops, self, load_loop))
        goto error;

done:
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_interconnect_drop_mixer(ov_interconnect *self, int socket){

    if (!self) goto error;

    ov_event_app_close(self->app.mixer, socket);
    ov_mixer_registry_unregister_mixer(self->mixers, socket);
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

ov_mixer_data ov_interconnect_assign_mixer(ov_interconnect *self, 
    const char *name){

    return ov_mixer_registry_acquire_user(self->mixers, name);
}

/*----------------------------------------------------------------------------*/

bool ov_interconnect_send_aquire_mixer(
    ov_interconnect *self,
    ov_mixer_data data,
    ov_mixer_forward forward){

    ov_json_value *out = ov_mixer_msg_acquire(data.user, forward);
    ov_event_app_send(self->app.mixer, data.socket, out);
    out = ov_json_value_free(out);
    return true;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      IO FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

struct container1 {

    ov_interconnect *self;
    const ov_interconnect_loop *loop;
    uint8_t *buffer;
    size_t size;
};

/*----------------------------------------------------------------------------*/

static bool forward_loop_io_to_session(const void *key, void *val,
                                         void *data) {

    if (!key)
        return true;
    ov_interconnect_session *session = (ov_interconnect_session*)val;
    struct container1 *container = (struct container1 *)data;

    return ov_interconnect_session_forward_loop_io(
        session, container->loop, container->buffer, container->size);
}

/*----------------------------------------------------------------------------*/

bool ov_interconnect_loop_io(
    ov_interconnect *self,
    ov_interconnect_loop *loop,
    uint8_t *buffer,
    size_t size){

    if (!self || !loop || !buffer) goto error;

    struct container1 container = (struct container1){
        .self = self, .loop = loop, .buffer = buffer, .size = size};

    return ov_dict_for_each(self->session.by_media_remote, &container,
                            forward_loop_io_to_session);

error:
    return false;
}

/*----------------------------------------------------------------------------*/

int ov_interconnect_get_media_socket(const ov_interconnect *self){

    if (!self) return -1;

    return self->socket.media;
}

/*----------------------------------------------------------------------------*/

const ov_interconnect_loop *ov_interconnect_get_loop(
    const ov_interconnect *self, const char *name){

    if (!self || !name) goto error;

    return ov_dict_get(self->loops, name);
error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

static bool add_loop_definition(const void *key, void *value, void *data) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;

    if (!key) return true;

    ov_interconnect_loop *loop = (ov_interconnect_loop*)(value);
    ov_json_value *array = ov_json_value_cast(data);

    if (!loop || !ov_json_is_array(array))
        goto error;

    out = ov_json_object();
    if (!out)
        goto error;

    val = ov_json_number(ov_interconnect_loop_get_ssrc(loop));
    if (!ov_json_object_set(out, OV_KEY_SSRC, val))
        goto error;

    val = ov_json_string(ov_interconnect_loop_get_name(loop));
    if (!ov_json_object_set(out, OV_KEY_NAME, val))
        goto error;

    val = NULL;

    if (!ov_json_array_push(array, out))
        goto error;

    return true;
error:
    ov_json_value_free(out);
    ov_json_value_free(val);
    return false;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_interconnect_get_loop_definitions(ov_interconnect *self) {

    ov_json_value *out = ov_json_array();

    if (!self)
        goto error;

    if (!ov_dict_for_each(self->loops, out, add_loop_definition))
        goto error;

    return out;
error:
    ov_json_value_free(out);
    return NULL;
}

/*----------------------------------------------------------------------------*/

bool ov_interconnect_srtp_ready(ov_interconnect *self,
    ov_interconnect_session *session) {

    ov_json_value *out = NULL;
    ov_json_value *par = NULL;

    if (!self || !session)
        goto error;

    if (!self->config.socket.client)
        goto error;

    if (ov_interconnect_session_loops_added(session))
        goto done;

    // connect all loops from client to server

    ov_json_value *loops = ov_interconnect_get_loop_definitions(self);
    if (!loops)
        goto error;

    ov_log_debug("Connecting all loops from client to server.");

    out = ov_interconnect_msg_connect_loops();
    par = ov_event_api_set_parameter(out);
    ov_json_object_set(par, OV_KEY_LOOPS, loops);

    ov_event_app_send(
        self->app.signaling,
        ov_interconnect_session_get_signaling_socket(session), 
        out);

    ov_json_value_free(out);
    ov_interconnect_session_added_loops(session);
done:
    return true;
error:
    ov_json_value_free(out);
    return false;
}