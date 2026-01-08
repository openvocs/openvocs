/***
        ------------------------------------------------------------------------

        Copyright (c) 2024 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_ice_proxy_vocs.c
        @author         Markus

        @date           2024-05-08


        ------------------------------------------------------------------------
*/
#include "../include/ov_ice_proxy_vocs.h"
#include "../include/ov_ice_config_from_generic.h"
#include "../include/ov_ice_proxy_dynamic.h"
#include "../include/ov_ice_proxy_multiplexing.h"

#include <ov_core/ov_mc_loop_data.h>

#define OV_ICE_PROXY_VOCS_MAGIC_BYTES 0x1ce3

#define OV_ICE_PROXY_VOCS_DEFAULT_SDP                                          \
    "v=0\r\n"                                                                  \
    "o=- 0 0 IN IP4 0.0.0.0\r\n"                                               \
    "s=-\r\n"                                                                  \
    "t=0 0\r\n"                                                                \
    "m=audio 0 UDP/TLS/RTP/SAVPF 100\r\n"                                      \
    "a=rtpmap:100 opus/48000/2\\r\\n"                                          \
    "a=fmtp:100 "                                                              \
    "maxplaybackrate=48000;stereo=1;"                                          \
    "useinbandfec=1\r\n"

/*----------------------------------------------------------------------------*/

struct ov_ice_proxy_vocs {

    uint16_t magic_bytes;
    ov_ice_proxy_vocs_config config;

    ov_ice_proxy_generic *proxy;

    ov_dict *sessions;
};

/*----------------------------------------------------------------------------*/

typedef struct Session {

    ov_id id;
    ov_ice_proxy_vocs *proxy;

    int socket;
    ov_socket_data local;

    ov_dict *talk;

} Session;

/*----------------------------------------------------------------------------*/

static bool io_internal(int socket, uint8_t events, void *userdata) {

    uint8_t buffer[OV_UDP_PAYLOAD_OCTETS] = {0};
    ov_socket_data remote = {};
    socklen_t src_addr_len = sizeof(remote.sa);

    Session *session = (Session *)userdata;
    if (!session || !socket || !events)
        goto error;

    if (events & OV_EVENT_IO_ERR || events & OV_EVENT_IO_CLOSE ||
        !(events & OV_EVENT_IO_IN))
        goto close;

    OV_ASSERT(events & OV_EVENT_IO_IN);

    ssize_t bytes = recvfrom(socket, (char *)buffer, OV_UDP_PAYLOAD_OCTETS, 0,
                             (struct sockaddr *)&remote.sa, &src_addr_len);

    if (bytes < 1)
        goto error;

    if (!ov_socket_parse_sockaddr_storage(&remote.sa, remote.host,
                                          OV_HOST_NAME_MAX, &remote.port))
        goto error;

    ssize_t out = ov_ice_proxy_generic_stream_send(
        session->proxy->proxy, session->id, 0, buffer, bytes);

    UNUSED(out);

close:
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static void *session_free(void *self) {

    Session *session = (Session *)self;
    if (!session)
        return NULL;

    ov_ice_proxy_generic_drop_session(session->proxy->proxy, session->id);

    if (session->proxy->config.callback.session_completed)
        session->proxy->config.callback.session_completed(
            session->proxy->config.callback.userdata, session->id,
            OV_ICE_FAILED);

    if (-1 != session->socket) {

        ov_event_loop_unset(session->proxy->config.loop, session->socket, NULL);

        close(session->socket);
        session->socket = -1;
    }

    session->talk = ov_dict_free(session->talk);
    session = ov_data_pointer_free(session);
    return session;
}

/*----------------------------------------------------------------------------*/

static Session *create_session(ov_ice_proxy_vocs *proxy, const char *uuid) {

    Session *self = NULL;
    if (!proxy || !uuid)
        goto error;

    self = calloc(1, sizeof(Session));
    if (!self)
        goto error;

    self->proxy = proxy;

    ov_id_set(self->id, uuid);
    self->socket = ov_socket_create(proxy->config.socket.internal, false, NULL);
    if (-1 == self->socket)
        goto error;
    ov_socket_get_data(self->socket, &self->local, NULL);
    ov_socket_ensure_nonblocking(self->socket);

    uint8_t event = OV_EVENT_IO_IN | OV_EVENT_IO_ERR | OV_EVENT_IO_CLOSE;

    if (!ov_event_loop_set(proxy->config.loop, self->socket, event, self,
                           io_internal))
        goto error;

    ov_dict_config d_config = ov_dict_string_key_config(255);
    d_config.value.data_function.free = ov_data_pointer_free;

    self->talk = ov_dict_create(d_config);
    if (!self->talk)
        goto error;

    if (!ov_dict_set(proxy->sessions, strdup(self->id), self, NULL))
        goto error;

    return self;
error:
    session_free(self);
    return NULL;
}

/*----------------------------------------------------------------------------*/

static void session_drop(void *userdata, const char *uuid) {

    ov_ice_proxy_vocs *self = ov_ice_proxy_vocs_cast(userdata);
    if (!self || !uuid)
        goto error;

    ov_dict_del(self->sessions, uuid);

error:
    return;
}

/*----------------------------------------------------------------------------*/

static void session_state(void *userdata, const char *uuid,
                          ov_ice_proxy_generic_state state) {

    ov_ice_proxy_vocs *self = ov_ice_proxy_vocs_cast(userdata);
    if (!self || !uuid)
        goto error;

    if (self->config.callback.session_completed)
        self->config.callback.session_completed(self->config.callback.userdata,
                                                uuid, (ov_ice_state)state);

error:
    return;
}

/*----------------------------------------------------------------------------*/

struct container_talk {

    const uint8_t *ptr;
    size_t len;

    Session *session;
};

/*----------------------------------------------------------------------------*/

static bool send_to_loop(const void *key, void *val, void *data) {

    if (!key)
        return true;

    struct sockaddr_storage sa = {0};

    ov_mc_loop_data *loopdata = (ov_mc_loop_data *)val;
    struct container_talk *container = (struct container_talk *)data;

    Session *session = container->session;

    socklen_t sock_len = sizeof(struct sockaddr_in);

    if (!ov_socket_fill_sockaddr_storage(&sa, session->local.sa.ss_family,
                                         loopdata->socket.host,
                                         loopdata->socket.port))
        goto error;

    if (session->local.sa.ss_family == AF_INET6)
        sock_len = sizeof(struct sockaddr_in6);

    ssize_t bytes = sendto(session->socket, container->ptr, container->len, 0,
                           (struct sockaddr *)&sa, sock_len);
    /*
        ov_log_debug("send %zi bytes of %zi bytes to loop %s:%i",
            bytes, container->len, loopdata->socket.host,
            loopdata->socket.port);
    */
    UNUSED(bytes);

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static void stream_io(void *userdata, const char *session_id, int stream_id,
                      uint8_t *buffer, size_t size) {

    ov_ice_proxy_vocs *self = ov_ice_proxy_vocs_cast(userdata);
    if (!self || !session_id || !buffer || !size)
        goto error;
    OV_ASSERT(stream_id == 0);

    Session *session = ov_dict_get(self->sessions, session_id);
    if (!session)
        goto ignore;

    switch (buffer[1]) {

    case 200:
        // ov_log_debug("RTCP sender report received - ignoring");
        goto ignore;

    case 201:
        // ov_log_debug("RTCP receiver report received - ignoring");
        goto ignore;

    case 202:
        // ov_log_debug("RTCP SDES received - ignoring");
        goto ignore;

    case 203:
        // ov_log_debug("RTCP GOOD BYE received - ignoring");
        goto ignore;

    case 204:
        // ov_log_debug("RTCP APP DATA received - ignoring");
        goto ignore;

    default:
        break;
    }

    struct container_talk container = (struct container_talk){

        .ptr = buffer, .len = size, .session = session};

    ov_dict_for_each(session->talk, &container, send_to_loop);

ignore:
error:
    return;
}

/*----------------------------------------------------------------------------*/

bool candidates_send(void *userdata, ov_json_value *out) {

    ov_ice_proxy_vocs *self = ov_ice_proxy_vocs_cast(userdata);
    if (!self || !out)
        goto error;

    if (self->config.callback.send_candidate)
        return self->config.callback.send_candidate(
            self->config.callback.userdata, out);

error:
    ov_json_value_free(out);
    return false;
}

/*----------------------------------------------------------------------------*/

void end_of_candidates_send(void *userdata, const char *session_id) {

    ov_ice_proxy_vocs *self = ov_ice_proxy_vocs_cast(userdata);
    if (!self || !session_id)
        goto error;

    if (self->config.callback.send_end_of_candidates)
        self->config.callback.send_end_of_candidates(
            self->config.callback.userdata, session_id);

error:
    return;
}

/*----------------------------------------------------------------------------*/

ov_ice_proxy_vocs *ov_ice_proxy_vocs_create(ov_ice_proxy_vocs_config config) {

    ov_ice_proxy_vocs *self = NULL;

    if (!ov_ptr_valid(config.loop, "Cannot create ICE Proxy - no loop"))
        goto error;

    config.proxy.loop = config.loop;

    self = calloc(1, sizeof(ov_ice_proxy_vocs));
    if (!self)
        goto error;

    self->magic_bytes = OV_ICE_PROXY_VOCS_MAGIC_BYTES;

    config.proxy.callbacks.userdata = self;
    config.proxy.callbacks.session.drop = session_drop;
    config.proxy.callbacks.session.state = session_state;
    config.proxy.callbacks.stream.io = stream_io;
    config.proxy.callbacks.candidate.send = candidates_send;
    config.proxy.callbacks.candidate.end_of_candidates = end_of_candidates_send;

    self->config = config;

    if (config.multiplexing) {
        self->proxy = ov_ice_proxy_multiplexing_create(config.proxy);
    } else {

        self->proxy = ov_ice_proxy_dynamic_create(config.proxy);
    }

    if (!ov_ptr_valid(self->proxy,
                      "Cannot create ICE Proxy - Could not create Proxy"))
        goto error;

    ov_dict_config d_config = ov_dict_string_key_config(255);
    d_config.value.data_function.free = session_free;

    self->sessions = ov_dict_create(d_config);
    if (!ov_ptr_valid(self->sessions,
                      "Cannot create ICE Proxy - Could not create sessions db"))
        goto error;

    return self;
error:
    ov_ice_proxy_vocs_free(self);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_ice_proxy_vocs *ov_ice_proxy_vocs_free(ov_ice_proxy_vocs *self) {

    if (!ov_ice_proxy_vocs_cast(self))
        goto error;

    self->proxy = ov_ice_proxy_generic_free(self->proxy);
    self->sessions = ov_dict_free(self->sessions);
    self = ov_data_pointer_free(self);
error:
    return self;
}

/*----------------------------------------------------------------------------*/

ov_ice_proxy_vocs *ov_ice_proxy_vocs_cast(const void *data) {

    if (!data)
        return NULL;

    if (*(uint16_t *)data != OV_ICE_PROXY_VOCS_MAGIC_BYTES)
        return NULL;

    return (ov_ice_proxy_vocs *)data;
}

/*----------------------------------------------------------------------------*/

ov_ice_proxy_vocs_config
ov_ice_proxy_vocs_config_from_json(const ov_json_value *input) {

    ov_ice_proxy_vocs_config out = {0};

    if (ov_ptr_valid(input, "Cannot read ICE proxy vocs config - no config")) {

        const ov_json_value *config = ov_json_object_get(input, OV_KEY_PROXY);
        if (!config)
            config = input;

        out.socket.internal = ov_socket_configuration_from_json(
            ov_json_get(config, "/" OV_KEY_INTERNAL),
            (ov_socket_configuration){0});

        out.proxy = ov_ice_proxy_generic_config_from_json(config);

        if (ov_json_is_true(ov_json_get(config, "/" OV_KEY_MULTIPLEXING)))
            out.multiplexing = true;
    }

    return out;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      EXTERNAL FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

/*----------------------------------------------------------------------------*/

ov_ice_proxy_vocs_session_data
ov_ice_proxy_vocs_create_session(ov_ice_proxy_vocs *self) {

    ov_sdp_session *sdp = NULL;

    ov_ice_proxy_vocs_session_data data = {0};

    if (!self)
        goto error;

    sdp = ov_sdp_parse(OV_ICE_PROXY_VOCS_DEFAULT_SDP,
                       strlen(OV_ICE_PROXY_VOCS_DEFAULT_SDP));

    if (!sdp)
        goto error;

    const char *id = ov_ice_proxy_generic_create_session(self->proxy, sdp);
    if (!id)
        goto error;

    data.ssrc = ov_ice_proxy_generic_stream_get_ssrc(self->proxy, id, 0);
    ov_id_set(data.uuid, id);
    data.desc = sdp;

    Session *session = create_session(self, id);
    if (!session)
        goto error;

    data.proxy = session->local;

    return data;
error:
    sdp = ov_sdp_session_free(sdp);
    return (ov_ice_proxy_vocs_session_data){0};
}

/*----------------------------------------------------------------------------*/

bool ov_ice_proxy_vocs_drop_session(ov_ice_proxy_vocs *self, const char *uuid) {

    if (!self || !uuid)
        return false;
    return ov_dict_del(self->sessions, uuid);
}

/*----------------------------------------------------------------------------*/

bool ov_ice_proxy_vocs_update_answer(ov_ice_proxy_vocs *self,
                                     const char *session_id,
                                     ov_sdp_session *sdp) {

    if (!self || !session_id || !sdp)
        return false;
    return ov_ice_proxy_generic_update_session(self->proxy, session_id, sdp);
}

/*----------------------------------------------------------------------------*/

bool ov_ice_proxy_vocs_candidate_in(ov_ice_proxy_vocs *self,
                                    const char *session_id, uint32_t stream_id,
                                    const ov_ice_candidate *candidate) {

    if (!self || !session_id || !candidate)
        goto error;
    return ov_ice_proxy_generic_stream_candidate_in(self->proxy, session_id,
                                                    stream_id, candidate);
error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_ice_proxy_vocs_end_of_candidates_in(ov_ice_proxy_vocs *self,
                                            const char *session_id,
                                            uint32_t stream_id) {

    if (!self || !session_id)
        goto error;
    return ov_ice_proxy_generic_stream_end_of_candidates_in(
        self->proxy, session_id, stream_id);
error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_ice_proxy_vocs_talk(ov_ice_proxy_vocs *self, const char *session_id,
                            bool on, ov_mc_loop_data data) {

    bool result = false;

    if (!self || !session_id)
        goto error;

    Session *session = ov_dict_get(self->sessions, session_id);
    if (!session)
        goto error;

    if (!on) {

        result = ov_dict_del(session->talk, data.name);

    } else {

        ov_mc_loop_data *t_data = calloc(1, sizeof(ov_mc_loop_data));

        if (t_data) {

            *t_data = data;

            char *key = strdup(data.name);

            if (!ov_dict_set(session->talk, key, t_data, NULL)) {
                t_data = ov_data_pointer_free(t_data);
                key = ov_data_pointer_free(key);
            } else {
                result = true;
            }
        }
    }

    return result;

error:
    return false;
}
