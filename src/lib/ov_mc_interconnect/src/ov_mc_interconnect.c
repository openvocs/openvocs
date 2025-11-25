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
        @file           ov_mc_interconnect.c
        @author         Markus

        @date           2023-12-13


        ------------------------------------------------------------------------
*/
#include "../include/ov_mc_interconnect.h"
#include "../include/ov_mc_interconnect_dtls_filter.h"

#include <ov_base/ov_config_keys.h>
#include <ov_base/ov_dict.h>
#include <ov_base/ov_error_codes.h>
#include <ov_base/ov_string.h>

#include <ov_core/ov_event_engine.h>
#include <ov_core/ov_event_socket.h>

#include <ov_stun/ov_stun_binding.h>
#include <ov_stun/ov_stun_frame.h>

/*----------------------------------------------------------------------------*/

#define OV_MC_INTERCONNECT_MAGIC_BYTES 0xab21

/*----------------------------------------------------------------------------*/

struct ov_mc_interconnect {

    uint16_t magic_bytes;
    ov_mc_interconnect_config config;

    ov_dtls *dtls;

    struct {

        ov_event_engine *engine;
        ov_event_socket *socket;

    } event;

    struct {

        int signaling;
        int media;

    } socket;

    struct {

        ov_dict *by_signaling_remote;
        ov_dict *by_media_remote;

    } session;

    ov_dict *loops;
    ov_dict *registered;
};

/*----------------------------------------------------------------------------*/

static void cb_close(void *userdata, int socket) {

    ov_mc_interconnect *self = ov_mc_interconnect_cast(userdata);
    if (!self) goto error;

    ov_log_debug("socket closed %i", socket);

    intptr_t key = socket;
    ov_dict_del(self->registered, (void *)key);

    if (self->config.socket.client) {

        ov_dict_clear(self->session.by_media_remote);
        ov_dict_clear(self->session.by_signaling_remote);
    }

error:
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_connected(void *userdata, int socket, bool result) {

    ov_mc_interconnect *self = ov_mc_interconnect_cast(userdata);
    if (!self) goto error;

    if (!result) goto error;

    ov_log_debug("opened connection %i to %s:%i",
                 socket,
                 self->config.socket.signaling.host,
                 self->config.socket.signaling.port);

    ov_json_value *msg = ov_mc_interconnect_msg_register(
        self->config.name, self->config.password);

    ov_event_socket_send(self->event.socket, socket, msg);
    msg = ov_json_value_free(msg);

    // client initiated connection to remote, so we need to
    // register that connection to allow access within the responses
    intptr_t key = socket;
    ov_dict_set(self->registered, (void *)key, NULL, NULL);

error:
    return;
}

/*----------------------------------------------------------------------------*/

static ov_mc_interconnect_session *get_session_by_signaling_socket(
    ov_mc_interconnect *self, int socket) {

    char buf[OV_HOST_NAME_MAX + 20] = {0};

    if (!self) goto error;

    ov_socket_data remote = (ov_socket_data){0};
    ov_socket_get_data(socket, NULL, &remote);
    snprintf(buf, OV_HOST_NAME_MAX + 20, "%s:%i", remote.host, remote.port);
    return ov_dict_get(self->session.by_signaling_remote, buf);
error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

static ov_mc_interconnect_session *get_session_by_media_remote(
    ov_mc_interconnect *self, ov_socket_data *remote) {

    char buf[OV_HOST_NAME_MAX + 20] = {0};

    if (!self || !remote) goto error;

    snprintf(buf, OV_HOST_NAME_MAX + 20, "%s:%i", remote->host, remote->port);
    return ov_dict_get(self->session.by_media_remote, buf);
error:
    return NULL;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      EVENT FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

static bool event_register_response(ov_mc_interconnect *self,
                                    const int socket,
                                    const ov_event_parameter *parameter,
                                    ov_json_value *input) {

    ov_json_value *out = NULL;

    UNUSED(parameter);
    UNUSED(self);

    uint64_t error_code = ov_event_api_get_error_code(input);
    if (0 != error_code) goto error;

    if (!ov_dict_is_set(self->registered, (void *)(intptr_t)socket)) goto error;

    const char *name = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_RESPONSE "/" OV_KEY_NAME));

    ov_log_debug("REGISTER SUCCESS at %i remote |%s|", socket, name);

    out = ov_mc_interconnect_msg_connect_media(self->config.name,
                                               OV_MC_INTERCONNECT_DEFAULT_CODEC,
                                               self->config.socket.media.host,
                                               self->config.socket.media.port);

    ov_event_io_send(parameter, socket, out);
    ov_json_value_free(out);
    ov_json_value_free(input);
    return true;
error:
    ov_json_value_free(input);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool event_register(void *userdata,
                           const int socket,
                           const ov_event_parameter *parameter,
                           ov_json_value *input) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *res = NULL;

    ov_mc_interconnect *self = ov_mc_interconnect_cast(userdata);
    if (!self || !parameter || !input) goto error;

    if (ov_event_api_get_response(input))
        return event_register_response(self, socket, parameter, input);

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

        out = ov_event_api_create_error_response(
            input, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto send_response;
    }

    ov_log_debug("REGISTER at %i remote |%s|", socket, name);

    intptr_t key = (intptr_t)socket;
    ov_dict_set(self->registered, (void *)key, NULL, NULL);

    out = ov_event_api_create_success_response(input);
    res = ov_event_api_get_response(out);
    val = ov_json_string(self->config.name);
    if (!ov_json_object_set(res, OV_KEY_NAME, val)) goto error;
    val = NULL;

send_response:

    ov_event_io_send(parameter, socket, out);
    ov_json_value_free(out);
    ov_json_value_free(input);

    return true;
error:
    ov_json_value_free(out);
    ov_json_value_free(val);
    ov_json_value_free(input);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool event_connect_media_response(ov_mc_interconnect *self,
                                         const int socket,
                                         const ov_event_parameter *parameter,
                                         ov_json_value *input) {

    ov_mc_interconnect_session *session = NULL;

    if (!self || !parameter || !input) goto error;
    UNUSED(socket);

    const char *host = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_RESPONSE "/" OV_KEY_HOST));

    const char *name = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_RESPONSE "/" OV_KEY_NAME));

    const char *finger = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_RESPONSE "/" OV_KEY_FINGERPRINT));

    uint32_t port = ov_json_number_get(
        ov_json_get(input, "/" OV_KEY_RESPONSE "/" OV_KEY_PORT));

    ov_log_debug("got remote media parameter |%s| %s:%i", name, host, port);

    // start DTLS active to remote

    ov_mc_interconnect_session_config config =
        (ov_mc_interconnect_session_config){
            .base = self,
            .loop = self->config.loop,
            .dtls = self->dtls,
            .internal = self->config.socket.internal,
            .reconnect_interval_usecs = 100000,
            .keepalive_trigger_usec =
                self->config.limits.keepalive_trigger_usec};

    ov_socket_data remote = (ov_socket_data){0};
    if (!ov_socket_get_data(socket, NULL, &remote)) goto error;

    config.remote.signaling = remote;
    config.signaling = socket;
    strncpy(config.remote.media.host, host, OV_HOST_NAME_MAX);
    config.remote.media.port = port;
    strncpy(
        config.remote.interface, name, OV_MC_INTERCONNECT_INTERFACE_NAME_MAX);

    session = ov_mc_interconnect_session_create(config);
    if (!session) goto error;

    if (!ov_mc_interconnect_session_handshake_active(session, finger))
        goto error;

    ov_json_value_free(input);
    return true;
error:
    ov_json_value_free(input);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool event_connect_media(void *userdata,
                                const int socket,
                                const ov_event_parameter *parameter,
                                ov_json_value *input) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *res = NULL;

    ov_mc_interconnect_session *session = NULL;

    ov_mc_interconnect *self = ov_mc_interconnect_cast(userdata);
    if (!self || !parameter || !input) goto error;

    if (!ov_dict_is_set(self->registered, (void *)(intptr_t)socket)) goto error;

    if (ov_event_api_get_response(input))
        return event_connect_media_response(self, socket, parameter, input);

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

    if (0 != strcmp(codec, OV_MC_INTERCONNECT_DEFAULT_CODEC)) {

        out = ov_event_api_create_error_response(
            input, OV_ERROR_CODE_CODEC_ERROR, OV_ERROR_DESC_CODEC_ERROR);

        goto send_response;
    }

    ov_mc_interconnect_session_config config =
        (ov_mc_interconnect_session_config){
            .base = self, .loop = self->config.loop, .dtls = self->dtls};

    ov_socket_data remote = (ov_socket_data){0};
    if (!ov_socket_get_data(socket, NULL, &remote)) goto error;
    config.remote.signaling = remote;
    config.signaling = socket;
    strncpy(config.remote.media.host, host, OV_HOST_NAME_MAX);
    config.remote.media.port = port;
    strncpy(
        config.remote.interface, name, OV_MC_INTERCONNECT_INTERFACE_NAME_MAX);

    session = ov_mc_interconnect_session_create(config);
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

    ov_event_io_send(parameter, socket, out);
    ov_json_value_free(out);
    ov_json_value_free(input);

    return true;
error:
    ov_json_value_free(out);
    ov_json_value_free(val);
    ov_json_value_free(input);
    return false;
}

/*----------------------------------------------------------------------------*/

struct container {

    ov_json_value *array;
    ov_mc_interconnect *self;
    ov_mc_interconnect_session *session;
};

/*----------------------------------------------------------------------------*/

static bool add_loops_response(void *value, void *data) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;

    ov_json_value *item = ov_json_value_cast(value);

    struct container *c = (struct container *)data;
    if (!item || !data) goto error;

    const char *name =
        ov_json_string_get(ov_json_object_get(item, OV_KEY_NAME));

    uint32_t ssrc = ov_json_number_get(ov_json_object_get(item, OV_KEY_SSRC));

    if (!name || !ssrc) goto error;

    ov_mc_interconnect_loop *loop = ov_dict_get(c->self->loops, name);
    if (!loop) goto done;

    if (!ov_mc_interconnect_session_add(c->session, name, ssrc)) goto done;

done:
    return true;

error:
    ov_json_value_free(val);
    ov_json_value_free(out);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool event_connect_loops_response(ov_mc_interconnect *self,
                                         const int socket,
                                         const ov_event_parameter *parameter,
                                         ov_json_value *input) {

    if (!self || !socket || !parameter || !input) goto error;

    const ov_json_value *loops =
        ov_json_get(input, "/" OV_KEY_RESPONSE "/" OV_KEY_LOOPS);
    if (!loops) goto error;

    ov_mc_interconnect_session *session =
        get_session_by_signaling_socket(self, socket);

    if (!session) goto error;

    struct container container =
        (struct container){.self = self, .session = session};

    if (!ov_json_array_for_each(
            (ov_json_value *)loops, &container, add_loops_response)) {

        goto error;
    }

    ov_json_value_free(input);
    return true;

error:
    ov_json_value_free(input);
    return false;
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

    ov_mc_interconnect_loop *loop = ov_dict_get(c->self->loops, name);
    if (!loop) goto done;

    if (!ov_mc_interconnect_session_add(c->session, name, ssrc)) goto done;

    uint32_t local_ssrc = ov_mc_interconnect_loop_get_ssrc(loop);

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

static bool event_connect_loops(void *userdata,
                                const int socket,
                                const ov_event_parameter *parameter,
                                ov_json_value *input) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *res = NULL;

    ov_mc_interconnect *self = ov_mc_interconnect_cast(userdata);
    if (!self || !parameter || !input) goto error;

    if (!ov_dict_is_set(self->registered, (void *)(intptr_t)socket)) goto error;

    if (ov_event_api_get_response(input))
        return event_connect_loops_response(self, socket, parameter, input);

    const ov_json_value *loops =
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_LOOPS);

    if (!loops) {

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_PARAMETER_ERROR,
                                                 OV_ERROR_DESC_PARAMETER_ERROR);

        goto send_response;
    }

    ov_mc_interconnect_session *session =
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

    if (!ov_json_array_for_each(
            (ov_json_value *)loops, &container, add_loops)) {

        out =
            ov_event_api_create_error_response(input,
                                               OV_ERROR_CODE_PROCESSING_ERROR,
                                               OV_ERROR_DESC_PROCESSING_ERROR);

        goto send_response;
    }

    out = ov_event_api_create_success_response(input);
    res = ov_event_api_get_response(out);
    if (!ov_json_object_set(res, OV_KEY_LOOPS, val)) goto error;
    val = NULL;

send_response:

    ov_event_io_send(parameter, socket, out);
    ov_json_value_free(out);
    ov_json_value_free(input);
    return true;
error:
    ov_json_value_free(out);
    ov_json_value_free(val);
    ov_json_value_free(input);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool register_events(ov_mc_interconnect *self) {

    if (!self) goto error;

    if (!ov_event_engine_register(
            self->event.engine, OV_MC_INTERCONNECT_REGISTER, self, event_register))
        goto error;

    if (!ov_event_engine_register(self->event.engine,
                                  OV_MC_INTERCONNECT_CONNECT_MEDIA,
                                  self,
                                  event_connect_media))
        goto error;

    if (!ov_event_engine_register(self->event.engine,
                                  OV_MC_INTERCONNECT_CONNECT_LOOPS,
                                  self,
                                  event_connect_loops))
        goto error;

    return true;
error:
    return false;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      #IO FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

static bool io_external_rtp(ov_mc_interconnect *self,
                            uint8_t *buffer,
                            size_t bytes,
                            ov_socket_data *remote) {

    ov_mc_interconnect_session *session = NULL;

    if (!self || !buffer || !bytes || !remote) goto error;

    session = get_session_by_media_remote(self, remote);
    if (!session) goto done;

    ov_mc_interconnect_session_forward_rtp_external_to_internal(
        session, buffer, bytes, remote);

done:
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool io_external_ssl(ov_mc_interconnect *self,
                            uint8_t *buffer,
                            size_t bytes,
                            ov_socket_data *remote) {

    ov_mc_interconnect_session *session = NULL;

    if (!self || !buffer || !bytes || !remote) goto error;

    session = get_session_by_media_remote(self, remote);
    if (!session) goto done;

    if (!ov_mc_interconnect_session_get_dtls(session)) {

        ov_mc_interconnect_session_handshake_passive(session, buffer, bytes);

    } else {

        ov_mc_interconnect_session_dtls_io(session, buffer, bytes);
    }

done:
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool io_stun(ov_mc_interconnect *self,
                    int socket,
                    uint8_t *buffer,
                    size_t bytes,
                    ov_socket_data *remote) {

    UNUSED(self);

    socklen_t len = sizeof(struct sockaddr_in);
    if (remote->sa.ss_family == AF_INET6) len = sizeof(struct sockaddr_in6);

    if (!ov_stun_frame_is_valid(buffer, bytes)) goto ignore;
    if (!ov_stun_frame_has_magic_cookie(buffer, bytes)) goto ignore;
    if (!ov_stun_frame_class_is_request(buffer, bytes)) goto ignore;
    if (!ov_stun_method_is_binding(buffer, bytes)) goto ignore;

    /* plain STUN request processing */

    if (!ov_stun_frame_set_success_response(buffer, bytes)) goto error;

    if (!ov_stun_xor_mapped_address_encode(
            buffer + 20, bytes - 20, buffer, NULL, &remote->sa))
        goto error;

    size_t out = 20 + ov_stun_xor_mapped_address_encoding_length(&remote->sa);

    if (!ov_stun_frame_set_length(buffer, bytes, out - 20)) goto error;

    // just to be sure, we nullify the rest of the buffer
    memset(buffer + out, 0, bytes - out);

    ssize_t send = sendto(
        socket, buffer, out, 0, (const struct sockaddr *)&remote->sa, len);

    UNUSED(send);
ignore:
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool io_external(int socket, uint8_t events, void *userdata) {

    uint8_t buffer[OV_UDP_PAYLOAD_OCTETS] = {0};
    ov_socket_data remote = {};
    socklen_t src_addr_len = sizeof(remote.sa);

    ov_mc_interconnect *self = ov_mc_interconnect_cast(userdata);
    if (!self) goto error;

    if ((events & OV_EVENT_IO_CLOSE) || (events & OV_EVENT_IO_ERR)) {

        ov_log_debug("%i - closing", socket);
        return true;
    }

    OV_ASSERT(events & OV_EVENT_IO_IN);

    ssize_t bytes = recvfrom(socket,
                             (char *)buffer,
                             OV_UDP_PAYLOAD_OCTETS,
                             0,
                             (struct sockaddr *)&remote.sa,
                             &src_addr_len);

    if (bytes < 1) goto error;

    if (!ov_socket_parse_sockaddr_storage(
            &remote.sa, remote.host, OV_HOST_NAME_MAX, &remote.port))
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

    if (buffer[0] <= 3) return io_stun(self, socket, buffer, bytes, &remote);

    if (buffer[0] <= 63 && buffer[0] >= 20) {

        return io_external_ssl(self, buffer, bytes, &remote);
    }

    if (buffer[0] <= 191 && buffer[0] >= 128) {

        return io_external_rtp(self, buffer, bytes, &remote);
    }

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

static bool check_config(ov_mc_interconnect_config *config) {

    if (!config->loop) goto error;
    if (0 == config->socket.signaling.host[0]) goto error;
    if (0 == config->socket.media.host[0]) goto error;
    if (0 == config->name[0]) goto error;
    if (0 == config->password[0]) goto error;

    long number_of_processors = sysconf(_SC_NPROCESSORS_ONLN);

    if (0 == config->limits.threads)
        config->limits.threads = number_of_processors;

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

ov_mc_interconnect *ov_mc_interconnect_create(
    ov_mc_interconnect_config config) {

    ov_mc_interconnect *self = NULL;

    if (!check_config(&config)) goto error;

    self = calloc(1, sizeof(ov_mc_interconnect));
    if (!self) goto error;

    self->magic_bytes = OV_MC_INTERCONNECT_MAGIC_BYTES;
    self->config = config;

    self->dtls = ov_dtls_create(self->config.dtls);
    if (!self->dtls) goto error;

    self->event.engine = ov_event_engine_create();
    if (!self->event.engine) goto error;
    if (!register_events(self)) goto error;

    self->event.socket = ov_event_socket_create(
        (ov_event_socket_config){.loop = self->config.loop,
                                 .engine = self->event.engine,
                                 .callback.userdata = self,
                                 .callback.close = cb_close,
                                 .callback.connected = cb_connected});

    if (!self->event.socket) goto error;

    // ov_event_socket_set_debug(self->event.socket, true);

    if (!ov_event_socket_load_ssl_config(
            self->event.socket, self->config.tls.domains))
        goto error;

    if (self->config.socket.client) {

        ov_event_socket_client_config s_c = (ov_event_socket_client_config){
            .socket = self->config.socket.signaling,
            .client_connect_trigger_usec =
                self->config.limits.client_connect_trigger_usec,
            .auto_reconnect = true};

        strncpy(s_c.ssl.domain, self->config.tls.client.domain, PATH_MAX);
        strncpy(s_c.ssl.ca.file, self->config.tls.client.ca.file, PATH_MAX);
        strncpy(s_c.ssl.ca.path, self->config.tls.client.ca.path, PATH_MAX);

        self->socket.signaling =
            ov_event_socket_create_connection(self->event.socket, s_c);

        if (-1 == self->socket.signaling) {

            ov_log_error("Failed to open connection to %s:%i",
                         self->config.socket.signaling.host,
                         self->config.socket.signaling.port);

        } else {

            ov_log_debug("opened connection to %s:%i",
                         self->config.socket.signaling.host,
                         self->config.socket.signaling.port);

            cb_connected(self, self->socket.signaling, true);
        }

    } else {

        self->socket.signaling = ov_event_socket_create_listener(
            self->event.socket,
            (ov_event_socket_server_config){.socket =
                                                self->config.socket.signaling});

        if (-1 == self->socket.signaling) {

            ov_log_error("Failed to open listener %s:%i",
                         self->config.socket.signaling.host,
                         self->config.socket.signaling.port);

            goto error;

        } else {

            ov_log_debug("opened listener %s:%i",
                         self->config.socket.signaling.host,
                         self->config.socket.signaling.port);
        }
    }

    self->socket.media =
        ov_socket_create(self->config.socket.media, false, NULL);

    if (-1 == self->socket.media) {

        ov_log_error("Failed to open media listener %s:%i",
                     self->config.socket.media.host,
                     self->config.socket.media.port);
        goto error;

    } else {

        ov_log_debug("opened media listener %s:%i",
                     self->config.socket.media.host,
                     self->config.socket.media.port);
    }

    if (!ov_event_loop_set(self->config.loop,
                           self->socket.media,
                           OV_EVENT_IO_IN | OV_EVENT_IO_ERR | OV_EVENT_IO_CLOSE,
                           self,
                           io_external))
        goto error;

    ov_dict_config d_config = ov_dict_string_key_config(255);
    d_config.value.data_function.free = ov_mc_interconnect_loop_free_void;

    self->loops = ov_dict_create(d_config);

    d_config = ov_dict_string_key_config(255);
    d_config.value.data_function.free = ov_mc_interconnect_session_free_void;

    self->session.by_media_remote = ov_dict_create(d_config);
    if (!self->session.by_media_remote) goto error;

    d_config = ov_dict_string_key_config(255);
    d_config.value.data_function.free = NULL;

    self->session.by_signaling_remote = ov_dict_create(d_config);
    if (!self->session.by_signaling_remote) goto error;

    d_config = ov_dict_intptr_key_config(255);
    d_config.value.data_function.free = NULL;

    self->registered = ov_dict_create(d_config);

    ov_mc_interconnect_dtls_filter_init();
    srtp_init();

    return self;
error:
    ov_mc_interconnect_free(self);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_mc_interconnect *ov_mc_interconnect_free(ov_mc_interconnect *self) {

    if (!ov_mc_interconnect_cast(self)) goto error;

    if (-1 != self->socket.signaling) {
        ov_event_loop_unset(self->config.loop, self->socket.signaling, NULL);
        close(self->socket.signaling);
        self->socket.signaling = -1;
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
    self->registered = ov_dict_free(self->registered);

    self->dtls = ov_dtls_free(self->dtls);
    self->event.engine = ov_event_engine_free(self->event.engine);
    self->event.socket = ov_event_socket_free(self->event.socket);

    ov_mc_interconnect_dtls_filter_deinit();
    self = ov_data_pointer_free(self);
error:
    return self;
}

/*----------------------------------------------------------------------------*/

ov_mc_interconnect *ov_mc_interconnect_cast(const void *data) {

    if (!data) return NULL;

    if (*(uint16_t *)data != OV_MC_INTERCONNECT_MAGIC_BYTES) return NULL;

    return (ov_mc_interconnect *)data;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      #SESSION FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

bool ov_mc_interconnect_register_session(ov_mc_interconnect *self,
                                         ov_id id,
                                         ov_socket_data signaling,
                                         ov_socket_data media,
                                         ov_mc_interconnect_session *session) {

    char buf[OV_HOST_NAME_MAX + 20] = {0};
    UNUSED(id);

    if (!self || !session) goto error;

    char *key = NULL;

    memset(buf, 0, OV_HOST_NAME_MAX + 20);
    snprintf(
        buf, OV_HOST_NAME_MAX + 20, "%s:%i", signaling.host, signaling.port);
    key = ov_string_dup(buf);
    ov_dict_set(self->session.by_signaling_remote, key, session, NULL);

    memset(buf, 0, OV_HOST_NAME_MAX + 20);
    snprintf(buf, OV_HOST_NAME_MAX + 20, "%s:%i", media.host, media.port);
    key = ov_string_dup(buf);
    ov_dict_set(self->session.by_media_remote, key, session, NULL);

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_mc_interconnect_unregister_session(ov_mc_interconnect *self,
                                           ov_socket_data signaling,
                                           ov_socket_data media) {

    char buf[OV_HOST_NAME_MAX + 20] = {0};
    if (!self) goto error;
    UNUSED(media);

    snprintf(
        buf, OV_HOST_NAME_MAX + 20, "%s:%i", signaling.host, signaling.port);
    
    ov_log_debug("unregister session %s", buf);

    ov_dict_del(self->session.by_signaling_remote, buf);

    /*
        snprintf(buf, OV_HOST_NAME_MAX + 20, "%s:%i", media.host, media.port);
        ov_dict_del(self->session.by_media_remote, buf);
    */
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool add_loop_definition(const void *key, void *value, void *data) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;

    if (!key) return true;
    ov_mc_interconnect_loop *loop = ov_mc_interconnect_loop_cast(value);
    ov_json_value *array = ov_json_value_cast(data);

    if (!loop || !ov_json_is_array(array)) goto error;

    out = ov_json_object();
    if (!out) goto error;

    val = ov_json_number(ov_mc_interconnect_loop_get_ssrc(loop));
    if (!ov_json_object_set(out, OV_KEY_SSRC, val)) goto error;

    val = ov_json_string(ov_mc_interconnect_loop_get_name(loop));
    if (!ov_json_object_set(out, OV_KEY_NAME, val)) goto error;

    val = NULL;

    if (!ov_json_array_push(array, out)) goto error;

    return true;
error:
    ov_json_value_free(out);
    ov_json_value_free(val);
    return false;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_mc_interconnect_get_loop_definitions(
    ov_mc_interconnect *self) {

    ov_json_value *out = ov_json_array();

    if (!self) goto error;

    if (!ov_dict_for_each(self->loops, out, add_loop_definition)) goto error;

    return out;
error:
    ov_json_value_free(out);
    return NULL;
}

/*----------------------------------------------------------------------------*/

bool ov_mc_interconnect_srtp_ready(ov_mc_interconnect *self,
                                   ov_mc_interconnect_session *session) {

    ov_json_value *out = NULL;
    ov_json_value *par = NULL;

    if (!self || !session) goto error;

    if (!self->config.socket.client) goto error;

    // connect all loops from client to server

    ov_json_value *loops = ov_mc_interconnect_get_loop_definitions(self);
    if (!loops) goto error;

    ov_log_debug("Connecting all loops from client to server.");

    out = ov_mc_interconnect_msg_connect_loops();
    par = ov_event_api_set_parameter(out);
    ov_json_object_set(par, OV_KEY_LOOPS, loops);

    ov_event_socket_send(
        self->event.socket,
        ov_mc_interconnect_session_get_signaling_socket(session),
        out);

    ov_json_value_free(out);
    return true;
error:
    ov_json_value_free(out);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool load_loop(const void *key, void *val, void *data) {

    if (!key) return true;
    if (!val || !data) goto error;

    ov_mc_interconnect *self = ov_mc_interconnect_cast(data);

    const char *name = (char *)key;
    ov_socket_configuration socket_config =
        ov_socket_configuration_from_json(val, (ov_socket_configuration){0});
    socket_config.type = UDP;

    if (!name || (0 == socket_config.host[0]) || (0 == socket_config.port))
        goto error;

    ov_mc_interconnect_loop_config config = (ov_mc_interconnect_loop_config){
        .loop = self->config.loop, .base = self};

    strncpy(config.name, name, OV_HOST_NAME_MAX);
    config.socket = socket_config;

    ov_mc_interconnect_loop *loop = ov_mc_interconnect_loop_create(config);

    if (!loop) goto error;

    char *k = strdup(name);

    if (!ov_dict_set(self->loops, k, loop, NULL)) {
        k = ov_data_pointer_free(k);
        loop = ov_mc_interconnect_loop_free(loop);
        goto error;
    }

    ov_log_debug("loaded loop %s|%s:%i", name, socket_config.host, socket_config.port);

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_mc_interconnect_load_loops(ov_mc_interconnect *self,
                                   const ov_json_value *config) {

    if (!self || !config) goto error;

    const ov_json_value *loops =
        ov_json_get(config, "/" OV_KEY_INTERCONNECT "/" OV_KEY_LOOPS);

    if (!loops) goto done;

    if (!ov_json_object_for_each((ov_json_value *)loops, self, load_loop))
        goto error;

done:
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

ov_mc_interconnect_config ov_mc_interconnect_config_from_json(
    const ov_json_value *val) {

    ov_mc_interconnect_config config = {0};
    if (!val) goto error;

    const ov_json_value *conf = ov_json_object_get(val, OV_KEY_INTERCONNECT);
    if (!conf) conf = val;

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

    if (str) strncpy(config.tls.domains, str, PATH_MAX);

    str = ov_json_string_get(
        ov_json_get(conf, "/" OV_KEY_TLS "/" OV_KEY_CLIENT "/" OV_KEY_DOMAIN));

    if (str) strncpy(config.tls.client.domain, str, PATH_MAX);

    str = ov_json_string_get(
        ov_json_get(conf, "/" OV_KEY_TLS "/" OV_KEY_CLIENT "/" OV_KEY_CA_FILE));

    if (str) strncpy(config.tls.client.ca.file, str, PATH_MAX);

    str = ov_json_string_get(
        ov_json_get(conf, "/" OV_KEY_TLS "/" OV_KEY_CLIENT "/" OV_KEY_CA_PATH));

    if (str) strncpy(config.tls.client.ca.path, str, PATH_MAX);

    str = ov_json_string_get(ov_json_get(conf, "/" OV_KEY_NAME));

    if (str) strncpy(config.name, str, OV_MC_INTERCONNECT_INTERFACE_NAME_MAX);

    str = ov_json_string_get(ov_json_get(conf, "/" OV_KEY_PASSWORD));

    if (str) strncpy(config.password, str, OV_MC_INTERCONNECT_PASSWORD_MAX);

    config.limits.threads = ov_json_number_get(
        ov_json_get(conf, "/" OV_KEY_LIMITS "/" OV_KEY_THREADS));

    config.limits.client_connect_trigger_usec = ov_json_number_get(ov_json_get(
        conf, "/" OV_KEY_LIMITS "/" OV_KEY_RECONNECT_INTERVAL_SECS));

    config.limits.keepalive_trigger_usec = ov_json_number_get(
        ov_json_get(conf, "/" OV_KEY_LIMITS "/" OV_KEY_KEEPALIVE_SEC));

    config.limits.client_connect_trigger_usec *= 1000000;
    config.limits.keepalive_trigger_usec *= 1000000;

    config.dtls = ov_dtls_config_from_json(val);

    return config;

error:
    return (ov_mc_interconnect_config){0};
}

/*----------------------------------------------------------------------------*/

int ov_mc_interconnect_get_media_socket(ov_mc_interconnect *self) {

    if (!self) return -1;

    return self->socket.media;
}

/*----------------------------------------------------------------------------*/

ov_mc_interconnect_loop *ov_mc_interconnect_get_loop(ov_mc_interconnect *self,
                                                     const char *loop_name) {

    if (!self || !loop_name) goto error;

    return ov_dict_get(self->loops, loop_name);
error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

struct container1 {

    ov_mc_interconnect *self;
    ov_mc_interconnect_loop *loop;
    uint8_t *buffer;
    size_t size;
};

/*----------------------------------------------------------------------------*/

static bool forward_multicast_to_session(const void *key,
                                         void *val,
                                         void *data) {

    if (!key) return true;
    ov_mc_interconnect_session *session = ov_mc_interconnect_session_cast(val);
    struct container1 *container = (struct container1 *)data;

    return ov_mc_interconnect_session_forward_multicast_to_external(
        session, container->loop, container->buffer, container->size);
}

/*----------------------------------------------------------------------------*/

bool ov_mc_interconnect_multicast_io(ov_mc_interconnect *self,
                                     ov_mc_interconnect_loop *loop,
                                     uint8_t *buffer,
                                     size_t size) {

    if (!self || !loop || !buffer || !size) goto error;

    struct container1 container = (struct container1){
        .self = self, .loop = loop, .buffer = buffer, .size = size};

    return ov_dict_for_each(self->session.by_media_remote,
                            &container,
                            forward_multicast_to_session);

error:
    return false;
}

/*----------------------------------------------------------------------------*/

ov_dtls *ov_mc_interconnect_get_dtls(ov_mc_interconnect *self) {

    if (!self) return NULL;
    return self->dtls;
}
