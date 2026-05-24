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
        @file           ov_webserver_io.c
        @author         Töpfer, Markus

        @date           2026-01-31


        ------------------------------------------------------------------------
*/
#include "../include/ov_webserver_io.h"

#include <ov_base/ov_dump.h>
#include <ov_base/ov_file.h>
#include <ov_base/ov_json_io_buffer.h>
#include <ov_base/ov_linked_list.h>
#include <ov_base/ov_string.h>
#include <ov_base/ov_uri.h>
#include <ov_base/ov_utils.h>

struct ov_webserver_io {

    uint16_t magic_bytes;
    ov_webserver_io_config config;

    bool debug;

    int socket;

    ov_dict *connections;

    ov_dict *domains;
    ov_dict *events;

    ov_json_io_buffer *json_io_buffer;
};

/*----------------------------------------------------------------------------*/

typedef enum ConnectionType {

    HTTP = 0,
    WEBSOCKET = 1

} ConnectionType;

/*----------------------------------------------------------------------------*/

typedef struct Connection {

    ov_webserver_io *server;

    ov_socket_data remote;

    int socket;
    char *domain;
    char *uri;

    ov_buffer *buffer;

    ConnectionType type;

    struct {

        ov_list *queue;
        uint64_t counter;
        ov_websocket_fragmentation_state last;

    } websocket;

} Connection;

/*----------------------------------------------------------------------------*/

typedef struct Callback {

    void *userdata;
    void *callback;

} Callback;

/*----------------------------------------------------------------------------*/

static void *connection_free(void *connection) {

    Connection *conn = (Connection *)connection;
    if (!conn)
        return NULL;

    if (conn->server) {

        if (conn->server->config.callbacks.close)
            conn->server->config.callbacks.close(
                conn->server->config.callbacks.userdata, conn->socket);
    }

    if (-1 != conn->socket) {

        close(conn->socket);

        ov_event_loop_unset(conn->server->config.loop, conn->socket, NULL);

        conn->socket = -1;
    }

    conn->websocket.queue = ov_list_free(conn->websocket.queue);
    conn->buffer = ov_buffer_free(conn->buffer);
    conn->domain = ov_data_pointer_free(conn->domain);
    conn->uri = ov_data_pointer_free(conn->uri);
    conn = ov_data_pointer_free(conn);
    return NULL;
}

/*----------------------------------------------------------------------------*/

static bool cb_io_accept(void *userdata, int listener, int socket) {

    ov_webserver_io *self = (ov_webserver_io *)userdata;
    if (!self)
        goto error;

    if (self->debug)
        ov_log_debug("accepted socket %i at %i", socket, listener);

    Connection *conn = calloc(1, sizeof(Connection));

    conn->socket = socket;
    conn->server = self;
    conn->type = HTTP;
    conn->buffer = ov_buffer_create(2048);

    if (!conn->buffer)
        goto error;

    if (!ov_socket_get_data(socket, NULL, &conn->remote)) {
        conn = ov_data_pointer_free(conn);
        goto error;
    }

    if (!ov_dict_set(self->connections, (void *)(intptr_t)socket, conn, NULL)) {
        conn = connection_free(conn);
        goto error;
    }

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool process_wss_control_frame(Connection *conn,
                                      ov_websocket_frame *frame) {

    OV_ASSERT(conn);
    OV_ASSERT(frame);

    ov_websocket_frame *response = NULL;

    switch (frame->opcode) {

    case OV_WEBSOCKET_OPCODE_PONG:
        break;

    case OV_WEBSOCKET_OPCODE_PING:

        response = ov_websocket_frame_create(frame->config);

        if (!response)
            goto error;

        // set fin and OV_WEBSOCKET_OPCODE_PONG
        response->buffer->start[0] = 0x8A;

        if (frame->content.start) {

            if (!ov_websocket_frame_unmask(frame))
                goto error;

            if (!ov_websocket_set_data(response, frame->content.start,
                                       frame->content.length, false))
                goto error;

        } else {

            response->buffer->length = 2;
        }

        if (!ov_io_send(
                conn->server->config.io, conn->socket,
                (ov_memory_pointer){.start = response->buffer->start,
                                    .length = response->buffer->length}))
            goto error;

        break;

    case OV_WEBSOCKET_OPCODE_CLOSE:

        ov_log_debug("received websocket close from %s:%i", conn->remote.host,
                     conn->remote.port);

        goto error;

    default:
        goto error;
    }

    frame = ov_websocket_frame_free(frame);
    response = ov_websocket_frame_free(response);
    return true;
error:
    frame = ov_websocket_frame_free(frame);
    response = ov_websocket_frame_free(response);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool defragmented_callback(Connection *conn) {

    OV_ASSERT(conn);
    OV_ASSERT(conn->websocket.queue);

    ov_websocket_frame *frame = NULL;
    ov_buffer *buffer = ov_buffer_create(2048);
    if (!buffer)
        goto error;

    frame = ov_list_queue_pop(conn->websocket.queue);
    if (!frame)
        goto error;

    while (frame) {

        if (!ov_buffer_push(buffer, (void *)frame->content.start,
                            frame->content.length)) {
            frame = ov_websocket_frame_free(frame);
            goto error;
        }

        frame = ov_websocket_frame_free(frame);
        frame = ov_list_queue_pop(conn->websocket.queue);
    }

    // we expect only JSON websocket frames
    if (!ov_json_io_buffer_push(conn->server->json_io_buffer, conn->socket,
                                (ov_memory_pointer){.start = buffer->start,
                                                    .length = buffer->length}))
        goto error;

    buffer = ov_buffer_free(buffer);
    conn->websocket.counter = 0;
    return true;
error:
    ov_buffer_free(buffer);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool process_non_fragmented_frame(Connection *conn,
                                         ov_websocket_frame *frame) {

    OV_ASSERT(conn);
    OV_ASSERT(frame);

    // we expect only JSON websocket frames
    if (!ov_json_io_buffer_push(
            conn->server->json_io_buffer, conn->socket,
            (ov_memory_pointer){.start = frame->content.start,
                                .length = frame->content.length}))
        goto error;

    frame = ov_websocket_frame_free(frame);
    return true;
error:
    frame = ov_websocket_frame_free(frame);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool defragmented_wss_delivery(Connection *conn,
                                      ov_websocket_frame *frame) {

    OV_ASSERT(conn);
    OV_ASSERT(frame);

    bool callback_queue = false;

    ov_websocket_frame *out = NULL;

    switch (frame->state) {

    case OV_WEBSOCKET_FRAGMENTATION_NONE:

        switch (conn->websocket.last) {

        case OV_WEBSOCKET_FRAGMENTATION_NONE:
        case OV_WEBSOCKET_FRAGMENTATION_LAST:
            break;

        default:
            goto error;
        }

        // non fragmented frame
        return process_non_fragmented_frame(conn, frame);

    case OV_WEBSOCKET_FRAGMENTATION_START:

        switch (conn->websocket.last) {

        case OV_WEBSOCKET_FRAGMENTATION_NONE:
        case OV_WEBSOCKET_FRAGMENTATION_LAST:
            break;

        default:
            goto error;
        }

        if (!conn->websocket.queue)
            conn->websocket.queue = ov_linked_list_create(
                (ov_list_config){.item.free = ov_websocket_frame_free});

        // at fragmentation start the queue should be empty

        out = ov_list_queue_pop(conn->websocket.queue);
        if (out) {

            out = ov_websocket_frame_free(out);
            goto error;
        }

        // push to queue
        break;

    case OV_WEBSOCKET_FRAGMENTATION_CONTINUE:

        switch (conn->websocket.last) {

        case OV_WEBSOCKET_FRAGMENTATION_START:
        case OV_WEBSOCKET_FRAGMENTATION_CONTINUE:
            break;

        default:
            goto error;
        }

        // push to queue
        break;

    case OV_WEBSOCKET_FRAGMENTATION_LAST:

        switch (conn->websocket.last) {

        case OV_WEBSOCKET_FRAGMENTATION_START:
        case OV_WEBSOCKET_FRAGMENTATION_CONTINUE:
            break;

        default:
            goto error;
        }

        callback_queue = true;
        // push to queue
        break;

    default:
        // fragmentation mismatch
        goto error;
    }

    if (!ov_list_queue_push(conn->websocket.queue, frame))
        goto error;

    conn->websocket.counter++;
    conn->websocket.last = frame->state;
    frame = NULL;

    if (callback_queue)
        return defragmented_callback(conn);

    return true;
error:
    ov_websocket_frame_free(frame);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool process_websocket(Connection *conn, ov_websocket_frame *frame) {

    OV_ASSERT(conn);
    OV_ASSERT(frame);

    bool result = false;
    bool text = false;

    if (frame->opcode >= 0x08) {

        result = process_wss_control_frame(conn, frame);
        goto done;
    }

    switch (frame->opcode) {

    case OV_WEBSOCKET_OPCODE_CONTINUATION:
        break;
    case OV_WEBSOCKET_OPCODE_TEXT:
        text = true;
        break;
    case OV_WEBSOCKET_OPCODE_BINARY:
        text = false;
        break;
    default:
        goto error;
    }

    if (!ov_websocket_frame_unmask(frame))
        goto error;

    UNUSED(text);
    return defragmented_wss_delivery(conn, frame);

done:
    if (!result)
        goto error;

    ov_websocket_frame_free(frame);
    return true;
error:
    frame = ov_websocket_frame_free(frame);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool io_connection_websocket(Connection *conn) {

    OV_ASSERT(conn);

    ov_websocket_parser_state state = OV_WEBSOCKET_PARSER_ERROR;

    bool all_done = false;
    bool result = false;

    while (!all_done) {

        if (!conn->buffer) {
            conn->buffer = ov_buffer_create(2048);
            goto done;
        }

        ov_websocket_frame *msg = ov_websocket_frame_pop(
            &conn->buffer, &conn->server->config.frame, &state);
        switch (state) {

        case OV_WEBSOCKET_PARSER_SUCCESS:

            OV_ASSERT(msg);
            result = process_websocket(conn, msg);
            break;

        case OV_WEBSOCKET_PARSER_PROGRESS:

            if (msg)
                msg = ov_websocket_frame_free(msg);
            goto done;

        default:
            goto error;
        }

        if (!result)
            goto error;
    }

done:
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool cleaned_path_for_connection(Connection *conn,
                                        const ov_http_message *msg, size_t len,
                                        char *out) {

    ov_uri *uri = NULL;
    if (len < PATH_MAX)
        goto error;

    uri = ov_uri_from_string((char *)msg->request.uri.start,
                             msg->request.uri.length);

    if (!uri || !uri->path)
        goto error;

    const char *domain_path = ov_dict_get(conn->server->domains, conn->domain);

    if (!domain_path) {

        ov_log_error("Access to domain %s denied "
                     "- no domain root path.",
                     conn->domain);

        goto error;
    }

    char cleaned_path[PATH_MAX + 1] = {0};
    if (!ov_uri_path_remove_dot_segments(uri->path, cleaned_path))
        goto error;

    /* clean empty paths between document root and uri path,
     *
     * (A) document root may finish with /
     * (B) we ensure to add some /
     * (C) uri may contain some initial /
     *
     * --> delete any non requried for some clean path */

    char full_path[PATH_MAX + 1] = {0};

    ssize_t bytes =
        snprintf(full_path, PATH_MAX, "%s/%s", domain_path, cleaned_path);

    if (bytes < 1)
        goto error;

    if (!ov_uri_path_remove_dot_segments(full_path, out))
        goto error;

    uri = ov_uri_free(uri);
    return true;
error:
    uri = ov_uri_free(uri);
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_webserver_io_clean_path(ov_webserver_io *self, int socket,
                                const ov_http_message *msg, char *path_out,
                                size_t path_out_len) {

    if (!self)
        return false;

    Connection *conn = ov_dict_get(self->connections, (void *)(intptr_t)socket);
    return cleaned_path_for_connection(conn, msg, path_out_len, path_out);
}

/*----------------------------------------------------------------------------*/

static bool enable_domain(const void *key, void *val, void *data) {

    if (!key)
        return true;

    const char *name = (char *)key;
    const char *path = ov_json_string_get(val);

    ov_webserver_io *self = (ov_webserver_io *)data;

    if (!self->domains) {

        ov_dict_config d_config = ov_dict_string_key_config(255);
        d_config.value.data_function.free = ov_data_pointer_free;

        self->domains = ov_dict_create(d_config);
        if (!self->domains)
            goto error;
    }

    return ov_dict_set(self->domains, ov_string_dup(name), ov_string_dup(path),
                       NULL);
error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_webserver_io_enable_domains(ov_webserver_io *self,
                                    const ov_json_value *config) {

    if (!self || !config)
        return false;

    ov_json_value *domains =
        (ov_json_value *)ov_json_get(config, "/webserver/domains");

    return ov_json_object_for_each(domains, self, enable_domain);
}

/*----------------------------------------------------------------------------*/

static bool process_http_message(Connection *conn, const ov_http_message *msg) {

    OV_ASSERT(conn);
    OV_ASSERT(msg);

    const ov_http_header *header_host = ov_http_header_get_unique(
        msg->header, msg->config.header.capacity, OV_HTTP_KEY_HOST);

    if (!header_host)
        goto error;

    if (conn->server->debug)
        ov_log_debug("RECV %.*s", (int)msg->buffer->length,
                     (char *)msg->buffer->start);

    uint8_t *hostname = (uint8_t *)header_host->value.start;
    size_t hostname_length = header_host->value.length;

    uint8_t *colon = memchr(hostname, ':', header_host->value.length);
    if (colon)
        hostname_length = colon - hostname;

    if (0 != strncmp(conn->domain, (char *)hostname, hostname_length)) {

        ov_log_error("HTTPs TLS consistency error,"
                     " using domain %s and hostname %.*s at %s:%i - ignoring",
                     conn->domain, (int)hostname_length, (char *)hostname,
                     conn->remote.host, conn->remote.port);
    }

    if (!conn->server->config.callbacks.io)
        goto error;

    return conn->server->config.callbacks.io(
        conn->server->config.callbacks.userdata, conn->socket, msg);

error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool process_http(Connection *conn, ov_http_message *msg) {

    OV_ASSERT(conn);
    OV_ASSERT(msg);

    ov_http_message *out = NULL;
    bool is_handshake = false;

    if (ov_websocket_process_handshake_request(msg, &out, &is_handshake)) {

        OV_ASSERT(out);
        OV_ASSERT(is_handshake);

        conn->type = WEBSOCKET;

        char uri[PATH_MAX] = {0};

        snprintf(uri, PATH_MAX, "%.*s", (int)msg->request.uri.length,
                 msg->request.uri.start);

        conn->uri = ov_string_dup(uri);

        if (!ov_io_send(conn->server->config.io, conn->socket,
                        (ov_memory_pointer){.start = out->buffer->start,
                                            .length = out->buffer->length})) {

            if (conn->server->debug)
                ov_log_debug("Failed to send handshake response to %s:%i",
                             conn->remote.host, conn->remote.port);

            out = ov_http_message_free(out);
            goto error;

        } else {

            out = ov_http_message_free(out);
            goto done;
        }
        goto done;
    }

    if (is_handshake)
        goto error;

    bool result = process_http_message(conn, msg);
    if (!result)
        goto error;

done:
    ov_http_message_free(msg);
    return true;
error:
    ov_http_message_free(msg);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool io_connection_http(Connection *conn) {

    OV_ASSERT(conn);

    ov_http_parser_state state = OV_HTTP_PARSER_ERROR;

    bool all_done = false;
    bool result = false;

    while (!all_done) {

        if (!conn->buffer) {
            conn->buffer = ov_buffer_create(2048);
            goto done;
        }

        ov_http_message *msg = ov_http_message_pop(
            &conn->buffer, &conn->server->config.http, &state);

        switch (state) {

        case OV_HTTP_PARSER_SUCCESS:

            if (msg) {
                result = process_http(conn, msg);
            } else {
                goto done;
            }
            break;

        case OV_HTTP_PARSER_PROGRESS:

            if (msg)
                msg = ov_http_message_free(msg);
            goto done;

        default:
            goto error;
        }

        if (!result)
            goto error;
    }

done:
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool io_connection(ov_webserver_io *self, Connection *conn,
                          const char *domain, const ov_memory_pointer data) {

    int socket = 0;

    if (!conn || !domain)
        goto error;

    socket = conn->socket;

    // 1st check domain input (same domain like in first contact)

    if (!conn->domain) {

        conn->domain = ov_string_dup(domain);

    } else {

        if (0 != ov_string_compare(conn->domain, domain)) {

            ov_log_error("Connection %i switched from domain %s to domain %s",
                         conn->socket, conn->domain, domain);

            goto error;
        }
    }

    if (conn->buffer == NULL)
        conn->buffer = ov_buffer_create(2048);

    if (!ov_buffer_push(conn->buffer, (uint8_t *)data.start, data.length))
        goto error;

    bool result = false;

    switch (conn->type) {

    case HTTP:
        result = io_connection_http(conn);
        break;
    case WEBSOCKET:
        result = io_connection_websocket(conn);
        break;
    }

    if (!result)
        goto error;

    return true;
error:
    if (self)
        ov_io_close(self->config.io, socket);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool cb_io(void *userdata, int connection, const char *domain,
                  const ov_memory_pointer data) {

    ov_webserver_io *self = (ov_webserver_io *)userdata;
    if (!self || connection < 1 || !domain)
        goto error;

    Connection *conn =
        ov_dict_get(self->connections, (void *)(intptr_t)connection);

    if (!conn)
        goto error;

    return io_connection(self, conn, domain, data);
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static void cb_close(void *userdata, int connection) {

    ov_webserver_io *self = (ov_webserver_io *)userdata;

    if (!self || connection < 1)
        goto error;

    ov_log_debug("closing socket %i", connection);

    ov_dict_del(self->connections, (void *)(intptr_t)connection);
    ov_json_io_buffer_drop(self->json_io_buffer, connection);

error:
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_json_success(void *userdata, int socket, ov_json_value *val) {

    if (!userdata || !val)
        goto error;

    ov_webserver_io *self = (ov_webserver_io *)userdata;

    Connection *conn = ov_dict_get(self->connections, (void *)(intptr_t)socket);

    if (!self || !conn)
        goto error;

    ov_dict *uris = ov_dict_get(self->events, conn->domain);
    if (!uris) {
        ov_log_error("failed to get uris for domain %s", conn->domain);
        goto error;
    }

    Callback *cb = ov_dict_get(uris, conn->uri);
    if (!cb) {
        ov_log_error("failed to get cb for domain %s%s", conn->domain,
                     conn->uri);
        goto error;
    }

    void (*function)(void *, int, ov_json_value *) = cb->callback;

    function(cb->userdata, socket, val);
    return;
error:
    ov_json_value_free(val);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_json_failure(void *userdata, int socket) {

    if (!userdata)
        goto error;

    ov_webserver_io *self = (ov_webserver_io *)userdata;

    ov_log_error("JSON IO failure at %i - closing", socket);
    ov_io_close(self->config.io, socket);

error:
    return;
}

/*----------------------------------------------------------------------------*/

static bool init_config(ov_webserver_io_config *config) {

    if (!config || !config->loop || !config->io)
        goto error;

    if (0 == config->socket.host[0]) {

        config->socket = (ov_socket_configuration){
            .host = "0.0.0.0", .type = TLS, .port = 443};
    }

    // ensure TLS is set
    config->socket.type = TLS;

    if (0 == config->name[0])
        strncpy(config->name, "openvocs", 9);

    config->http = ov_http_message_config_init(config->http);

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

ov_webserver_io *ov_webserver_io_create(ov_webserver_io_config config) {

    ov_webserver_io *self = NULL;

    if (!init_config(&config))
        goto error;

    self = calloc(1, sizeof(ov_webserver_io));
    if (!self)
        goto error;

    self->config = config;

    self->socket = ov_io_open_listener(
        self->config.io, (ov_io_socket_config){.socket = self->config.socket,
                                               .callbacks.userdata = self,
                                               .callbacks.accept = cb_io_accept,
                                               .callbacks.io = cb_io,
                                               .callbacks.close = cb_close});

    if (-1 == self->socket) {

        ov_log_error("Failed to open socket %s:%i - abort.",
                     self->config.socket.host, self->config.socket.port);

        goto error;
    }

    ov_dict_config d_config = ov_dict_intptr_key_config(255);
    d_config.value.data_function.free = connection_free;

    self->connections = ov_dict_create(d_config);
    if (!self->connections)
        goto error;

    self->json_io_buffer = ov_json_io_buffer_create(
        (ov_json_io_buffer_config){.callback.userdata = self,
                                   .callback.success = cb_json_success,
                                   .callback.failure = cb_json_failure});
    if (!self->json_io_buffer)
        goto error;

    d_config = ov_dict_string_key_config(255);
    d_config.value.data_function.free = ov_dict_free;
    self->events = ov_dict_create(d_config);

    return self;
error:
    ov_webserver_io_free(self);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_webserver_io *ov_webserver_io_free(ov_webserver_io *self) {

    if (!self)
        return self;

    self->json_io_buffer = ov_json_io_buffer_free(self->json_io_buffer);
    self->connections = ov_dict_free(self->connections);
    self->domains = ov_dict_free(self->domains);
    self->events = ov_dict_free(self->events);
    self = ov_data_pointer_free(self);
    return NULL;
}

/*----------------------------------------------------------------------------*/

bool ov_webserver_io_set_debug(ov_webserver_io *self, bool on) {

    if (!self)
        goto error;

    self->debug = on;
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

ov_webserver_io_config
ov_webserver_io_config_from_json(const ov_json_value *input) {

    ov_webserver_io_config config = {0};
    if (!input)
        goto error;

    const ov_json_value *item = ov_json_object_get(input, "webserver");
    if (!item)
        item = input;

    const char *name = ov_json_string_get(ov_json_object_get(item, "name"));
    if (name)
        strncpy(config.name, name, PATH_MAX);

    config.socket = ov_socket_configuration_from_json(
        ov_json_object_get(item, "socket"), (ov_socket_configuration){0});

    ov_json_value *http = ov_json_object_get(item, "http");
    if (http) {

        config.http.header.capacity =
            ov_json_number_get(ov_json_object_get(http, "capacity"));

        config.http.header.max_bytes_method_name =
            ov_json_number_get(ov_json_object_get(http, "max_method_name"));

        config.http.header.max_bytes_line =
            ov_json_number_get(ov_json_object_get(http, "max_byte_line"));

        config.http.buffer.default_size =
            ov_json_number_get(ov_json_object_get(http, "buffer_size"));

        config.http.buffer.max_bytes_recache =
            ov_json_number_get(ov_json_object_get(http, "buffer_size_recache"));

        config.http.transfer.max =
            ov_json_number_get(ov_json_object_get(http, "max_transfer"));

        config.http.chunk.max_bytes =
            ov_json_number_get(ov_json_object_get(http, "max_chunk_bytes"));
    }

    ov_json_value *frame = ov_json_object_get(item, "frame");
    if (frame) {

        config.frame.buffer.default_size =
            ov_json_number_get(ov_json_object_get(http, "buffer_size"));

        config.frame.buffer.max_bytes_recache =
            ov_json_number_get(ov_json_object_get(http, "buffer_size_recache"));
    }

    return config;
error:
    return (ov_webserver_io_config){0};
}

/*----------------------------------------------------------------------------*/

bool ov_webserver_io_event_callback(ov_webserver_io *self, const char *domain,
                                    const char *uri, void *userdata,
                                    void (*callback)(void *userdata, int socket,
                                                     ov_json_value *msg)) {

    Callback *val = NULL;
    char *key = NULL;

    if (!self || !domain || !uri)
        goto error;

    ov_dict *uris = ov_dict_get(self->events, domain);

    if (!uris) {

        ov_dict_config d_config = ov_dict_string_key_config(255);
        d_config.value.data_function.free = ov_data_pointer_free;

        uris = ov_dict_create(d_config);
        ov_dict_set(self->events, ov_string_dup(domain), uris, NULL);
    }

    if (!userdata) {

        return ov_dict_del(uris, uri);
    }

    key = ov_string_dup(uri);
    if (!key)
        goto error;

    val = calloc(1, sizeof(Callback));
    if (!val)
        goto error;

    val->userdata = userdata;
    val->callback = callback;

    if (!ov_dict_set(uris, key, val, NULL))
        goto error;

    ov_log_info("Enabled JSON callback at domain |%s|%s|", domain, uri);

    return true;
error:
    ov_data_pointer_free(val);
    ov_data_pointer_free(key);
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_webserver_io_close(ov_webserver_io *self, int socket) {

    if (!self)
        return false;

    return ov_dict_del(self->connections, (void *)(intptr_t)socket);
}

/*
 *      ------------------------------------------------------------------------
 *
 *      SEND FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

bool ov_webserver_io_send_json(ov_webserver_io *self, int socket,
                               const ov_json_value *msg) {

    char *string = NULL;
    ov_websocket_frame *frame = NULL;

    if (!self || !msg)
        goto error;

    Connection *conn = ov_dict_get(self->connections, (void *)(intptr_t)socket);
    if (!conn)
        goto error;

    string = ov_json_value_to_string(msg);
    if (!string)
        goto error;

    ssize_t length = strlen(string);

    frame = ov_websocket_frame_create(self->config.frame);
    if (!frame)
        goto error;

    ov_websocket_frame_clear(frame);

    ssize_t chunk = 1000;

    if (length < chunk) {

        frame->buffer->start[0] = 0x80 | OV_WEBSOCKET_OPCODE_TEXT;

        if (!ov_websocket_set_data(frame, (uint8_t *)string, length, false))
            goto error;

        if (!ov_io_send(self->config.io, conn->socket,
                        (ov_memory_pointer){.start = frame->buffer->start,
                                            .length = frame->buffer->length}))
            goto error;

        goto done;
    }

    // send in chunks

    uint8_t *ptr = (uint8_t *)string;
    ssize_t open = length;

    frame->buffer->start[0] = 0x00 | OV_WEBSOCKET_OPCODE_TEXT;

    if (!ov_websocket_set_data(frame, ptr, chunk, false))
        goto error;

    if (!ov_io_send(self->config.io, conn->socket,
                    (ov_memory_pointer){.start = frame->buffer->start,
                                        .length = frame->buffer->length}))
        goto error;

    open -= chunk;
    ptr += chunk;

    while (open > chunk) {

        frame->buffer->start[0] = 0x00;

        if (!ov_websocket_set_data(frame, ptr, chunk, false))
            goto error;

        if (!ov_io_send(self->config.io, conn->socket,
                        (ov_memory_pointer){.start = frame->buffer->start,
                                            .length = frame->buffer->length}))
            goto error;

        open -= chunk;
        ptr += chunk;
    }

    frame->buffer->start[0] = 0x80;

    if (!ov_websocket_set_data(frame, ptr, open, false))
        goto error;

    if (!ov_io_send(self->config.io, conn->socket,
                    (ov_memory_pointer){.start = frame->buffer->start,
                                        .length = frame->buffer->length}))
        goto error;

done:
    ov_data_pointer_free(string);
    ov_websocket_frame_free(frame);
    return true;
error:
    ov_data_pointer_free(string);
    ov_websocket_frame_free(frame);
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_webserver_io_send_http(ov_webserver_io *self, int socket,
                               const ov_http_message *msg) {

    if (!self || !msg)
        goto error;

    Connection *conn = ov_dict_get(self->connections, (void *)(intptr_t)socket);

    if (!conn)
        goto error;

    if (!ov_io_send(self->config.io, conn->socket,
                    (ov_memory_pointer){.start = msg->buffer->start,
                                        .length = msg->buffer->length}))
        goto error;

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_webserver_io_send(ov_webserver_io *self, int socket, const char *buffer,
                          size_t size) {

    if (!self || !buffer)
        goto error;

    Connection *conn = ov_dict_get(self->connections, (void *)(intptr_t)socket);

    if (!conn)
        goto error;

    if (!ov_io_send(
            self->config.io, conn->socket,
            (ov_memory_pointer){.start = (uint8_t *)buffer, .length = size}))
        goto error;

    return true;
error:
    return false;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      GETTER FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_event_loop *ov_webserver_io_get_eventloop(const ov_webserver_io *self) {

    if (!self)
        return NULL;

    return self->config.loop;
}
