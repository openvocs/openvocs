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
        @file           ov_io_web.c
        @author         Töpfer, Markus

        @date           2025-05-28


        ------------------------------------------------------------------------
*/
#include "../include/ov_io_web.h"
#include "../include/ov_domain.h"

#include <ov_base/ov_string.h>
#include <ov_base/ov_uri.h>
#include <ov_base/ov_linked_list.h>

#define OV_IO_WEB_MAGIC_BYTES 0x432e

#define FLAG_CLOSE_AFTER_SEND 0x0001
#define FLAG_SEND_SSL_SHUTDOWN 0x0010
#define FLAG_CLIENT_SHUTDOWN 0x0100

#define IMPL_MAX_FRAMES_FOR_JSON 255
#define IMPL_WEBSOCKET_PHRASE_LENGTH 250

#define IMPL_BUFFER_SIZE_WS_FRAME_DEFRAG 1000000 // 1MB

#define OV_SSL_MAX_BUFFER 16000 // 16kb SSL buffer max
#define OV_SSL_ERROR_STRING_BUFFER_SIZE 200

#define IMPL_CONTENT_SIZE_WS_FRAME_DEFAULT 500

/*----------------------------------------------------------------------------*/

typedef struct Websocket {

    ov_websocket_message_config config;

    ov_list *queue;

    ov_websocket_fragmentation_state last;

    uint64_t counter;

    struct {

        int code;
        char phrase[IMPL_WEBSOCKET_PHRASE_LENGTH];

    } close;

} Websocket;

/*----------------------------------------------------------------------------*/

typedef struct Connection {

    int socket;

    void *data;

    uint16_t flags;

    char *domain;

    struct {

        SSL *ssl;
        bool handshaked;

    } tls;

    Websocket *websocket;

} Connection;

/*----------------------------------------------------------------------------*/

struct ov_io_web {

    uint16_t magic_bytes;
    ov_io_web_config config;

    int socket;

    ov_dict *hosts;
    ov_dict *events;

    Connection connections[];
};

/*
 *      ------------------------------------------------------------------------
 *
 *      #CLOSE FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

static bool close_connection(ov_io_web *self, Connection *conn){

    OV_ASSERT(self);
    OV_ASSERT(conn);

    if (-1 == conn->socket){

        OV_ASSERT(NULL == conn->websocket);
        OV_ASSERT(NULL == conn->tls.ssl);

        return true;
    }

    int socket = conn->socket;
    conn->socket = -1;

    if (conn->websocket){

        // send close etc

        if (0 == conn->websocket->close.code) {
            conn->websocket->close.code = 1000;
            const char *phrase = "normal close";
            strncpy(conn->websocket->close.phrase, phrase, strlen(phrase));
        }

        if (conn->flags & FLAG_CLIENT_SHUTDOWN) {

            // do nothing

        } else {

            // send websocket shutdown

            uint8_t out[IMPL_WEBSOCKET_PHRASE_LENGTH + 5] = {0};
            ssize_t clen = strlen(conn->websocket->close.phrase);
    
            out[0] = 0x88;
            out[1] = clen + 2;
            out[2] = conn->websocket->close.code >> 8;
            out[3] = conn->websocket->close.code;
            memcpy(out + 4, conn->websocket->close.phrase, clen);
    
            ssize_t bytes = SSL_write(conn->tls.ssl, out, clen + 4);
            UNUSED(bytes);

        }

        if (conn->websocket->config.close){

            conn->websocket->config.close(
                conn->websocket->config.userdata,
                socket);
        }

        conn->websocket->queue = ov_list_free(conn->websocket->queue);
        conn->websocket = ov_data_pointer_free(conn->websocket);
    }

    if (conn->tls.ssl){

        if (conn->flags & FLAG_SEND_SSL_SHUTDOWN) SSL_shutdown(conn->tls.ssl);

        SSL_free(conn->tls.ssl);
        conn->tls.ssl = NULL;

    }

    close(socket);

    if (!ov_event_loop_unset(
        self->config.loop,
        socket,
        NULL)){

        ov_log_error("Failed to reset eventloop");
    }

    if (self->config.callbacks.close)
        self->config.callbacks.close(self->config.callbacks.userdata, socket);

    return true;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      #IO FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

static void cb_io_close(void *userdata, int socket){

    ov_io_web *self = ov_io_web_cast(userdata);
    if (!self || socket < 1) goto error;

    if (socket >= self->config.limits.sockets) goto error;

    Connection *conn = &self->connections[socket];
    close_connection(self, conn);

error:
    return;
}

/*----------------------------------------------------------------------------*/

static bool cb_io_accept(void *userdata, int listener, int connection) {

    ov_io_web *self = ov_io_web_cast(userdata);
    if (!self) goto error;

    if (connection >= self->config.limits.sockets) goto error;

    if (listener != self->socket) goto error;

    Connection *conn = &self->connections[connection];
    conn->socket = connection;

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool process_wss_control_frame(ov_io_web *self,
                                      Connection *conn,
                                      ov_websocket_frame *frame) {

    OV_ASSERT(self);
    OV_ASSERT(conn);
    OV_ASSERT(frame);

    ov_websocket_frame *response = NULL;

    switch (frame->opcode) {

        case OV_WEBSOCKET_OPCODE_PONG:

            // nothing to do on pong
            break;

        case OV_WEBSOCKET_OPCODE_PING:

            // We need to send a PONG with the same application data as received
            response = ov_websocket_frame_create(frame->config);
            if (!response) goto error;

            // set fin and OV_WEBSOCKET_OPCODE_PONG
            response->buffer->start[0] = 0x8A;

            if (frame->content.start) {

                if (!ov_websocket_frame_unmask(frame)) goto error;

                if (!ov_websocket_set_data(response,
                                           frame->content.start,
                                           frame->content.length,
                                           false))
                    goto error;

            } else {

                frame->buffer->length = 2;
            }

            if (!ov_io_send(
                    self->config.io,
                    conn->socket,
                    (ov_memory_pointer){.start = response->buffer->start,
                                        .length = response->buffer->length}))
                goto error;

            break;

        case OV_WEBSOCKET_OPCODE_CLOSE:
            // close connection over error
            goto error;
            break;

        default:
            goto error;
            break;
    }

    frame = ov_websocket_frame_free(frame);
    response = ov_websocket_frame_free(response);
    return true;
error:
    ov_websocket_frame_free(frame);
    ov_websocket_frame_free(response);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool defragmented_callback(ov_io_web *self,
                                  Connection *conn,
                                  bool text) {

    OV_ASSERT(self);
    OV_ASSERT(conn);
    OV_ASSERT(conn->websocket);
    OV_ASSERT(conn->websocket->queue);
    OV_ASSERT(conn->websocket->config.userdata);
    OV_ASSERT(conn->websocket->config.callback);

    /*
     *  This function will create some defragmented buffer containing all
     *  content of the websocket frames of the queue.
     *
     *  It will NOT check if framing is correct, e.g. if reception line is
     *  valid, which is done in @see defragmented_wss_delivery
     */

    ov_websocket_frame *frame = NULL;

    ov_buffer *buffer = ov_buffer_create(IMPL_BUFFER_SIZE_WS_FRAME_DEFRAG);
    if (!buffer) goto error;

    frame = ov_list_queue_pop(conn->websocket->queue);

    if (!frame) goto error;

    while (frame) {

        if (!ov_buffer_push(
                buffer, (void *)frame->content.start, frame->content.length))
            goto error;

        frame = ov_websocket_frame_free(frame);
        frame = ov_list_queue_pop(conn->websocket->queue);
    }

    void *userdata = conn->websocket->config.userdata;
    int socket = conn->socket;

    const char *domain = conn->domain;
    const char *uri = conn->websocket->config.uri;

    bool (*callback)(void *userdata,
                     int socket,
                     const ov_memory_pointer domain,
                     const char *uri,
                     ov_memory_pointer content,
                     bool text) = conn->websocket->config.callback;

    if (!callback(userdata,
                  socket,
                  (ov_memory_pointer){
                      .start = (uint8_t *)domain, .length = strlen(domain)},
                  uri,
                  (ov_memory_pointer){
                      .start = buffer->start, .length = buffer->length},
                  text))
        goto error;

    ov_buffer_free(buffer);
    conn->websocket->counter = 0;
    return true;
error:
    ov_buffer_free(buffer);
    ov_websocket_frame_free(frame);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool defragmented_wss_delivery(ov_io_web *self,
                                      Connection *conn,
                                      ov_websocket_frame *frame,
                                      bool text) {

    OV_ASSERT(self);
    OV_ASSERT(conn);
    OV_ASSERT(frame);

    /*
     *  This function will create / fill some queue with websocket frames
     *  and check if the reception is in line to the protocol.
     */

    ov_websocket_frame *out = NULL;

    bool callback_queue = false;

    switch (frame->state) {

        case OV_WEBSOCKET_FRAGMENTATION_NONE:

            /* fragmentation start */
            switch (conn->websocket->last) {

                case OV_WEBSOCKET_FRAGMENTATION_NONE:
                case OV_WEBSOCKET_FRAGMENTATION_LAST:
                    break;

                default:
                    goto error;
            }

            /* Non fragmented frame -> perform callback */

            void *userdata = conn->websocket->config.userdata;
            int socket = conn->socket;
            const char *domain = conn->domain;
            const char *uri = conn->websocket->config.uri;
            bool (*callback)(void *,
                             int,
                             const ov_memory_pointer,
                             const char *,
                             ov_memory_pointer,
                             bool) = conn->websocket->config.callback;

            if (!callback(userdata,
                          socket,
                          (ov_memory_pointer){.start = (uint8_t *)domain,
                                              .length = strlen(domain)},
                          uri,
                          frame->content,
                          text))
                goto error;

            ov_websocket_frame_free(frame);
            conn->websocket->counter = 0;
            conn->websocket->last = OV_WEBSOCKET_FRAGMENTATION_NONE;

            /* do not push to queue */
            return true;

            break;

        case OV_WEBSOCKET_FRAGMENTATION_START:

            /* fragmentation start */
            switch (conn->websocket->last) {

                case OV_WEBSOCKET_FRAGMENTATION_NONE:
                case OV_WEBSOCKET_FRAGMENTATION_LAST:
                    break;

                default:
                    goto error;
            }

            if (!conn->websocket->queue) {
                conn->websocket->queue = ov_linked_list_create(
                    (ov_list_config){.item.free = ov_websocket_frame_free});

                if (!conn->websocket->queue) goto error;
            }

            out = ov_list_queue_pop(conn->websocket->queue);

            if (out) {

                out = ov_websocket_frame_free(out);
                goto error;
            }

            /* push to queue */
            break;

        case OV_WEBSOCKET_FRAGMENTATION_CONTINUE:

            switch (conn->websocket->last) {

                case OV_WEBSOCKET_FRAGMENTATION_START:
                case OV_WEBSOCKET_FRAGMENTATION_CONTINUE:
                    break;

                default:
                    goto error;
            }

            /* push to queue */
            break;

        case OV_WEBSOCKET_FRAGMENTATION_LAST:

            switch (conn->websocket->last) {

                case OV_WEBSOCKET_FRAGMENTATION_START:
                case OV_WEBSOCKET_FRAGMENTATION_CONTINUE:
                    break;

                default:
                    goto error;
            }

            /* push to queue */
            callback_queue = true;
            break;

        default:
            goto error;
    }

    if (!ov_list_queue_push(conn->websocket->queue, frame)) goto error;

    conn->websocket->counter++;
    conn->websocket->last = frame->state;
    frame = NULL;

    if (0 != conn->websocket->config.max_frames) {

        if (conn->websocket->counter >= conn->websocket->config.max_frames) {

            if (!callback_queue) goto error;
        }
    }

    if (callback_queue) return defragmented_callback(self, conn, text);

    return true;
error:
    ov_websocket_frame_free(frame);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool process_wss_frame(ov_io_web *self,
                              Connection *conn,
                              ov_websocket_frame *frame) {

    OV_ASSERT(self);
    OV_ASSERT(conn);
    OV_ASSERT(frame);

    if (frame->opcode >= 0x08)
        return process_wss_control_frame(self, conn, frame);

    bool text = false;

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
            break;
    }

    if (!ov_websocket_frame_unmask(frame)) goto error;

    if (conn->websocket->config.fragmented) {

        /* unfragmented websocket callback used */

        void *userdata = conn->websocket->config.userdata;
        int socket = conn->socket;

        bool (*callback)(void *,
                         int,
                         const ov_memory_pointer,
                         const char *,
                         ov_websocket_frame *) =
            conn->websocket->config.fragmented;

        return callback(userdata,
                        socket,
                        (ov_memory_pointer){.start = (uint8_t *)conn->domain,
                                            .length = strlen(conn->domain)},
                        conn->websocket->config.uri,
                        frame);
    }

    return defragmented_wss_delivery(self, conn, frame, text);

error:
    ov_websocket_frame_free(frame);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool process_wss(ov_io_web *self, Connection *conn){

    OV_ASSERT(self);
    OV_ASSERT(conn);

    ov_websocket_frame *next_frame = NULL;

    uint8_t *next = NULL;

    ov_websocket_frame *frame = ov_websocket_frame_cast(conn->data);
    ov_websocket_parser_state state = OV_WEBSOCKET_PARSER_ERROR;

    do {

        OV_ASSERT(frame);
        OV_ASSERT(frame->buffer);

        if (frame->buffer->length == 0) {

            frame = ov_websocket_frame_free(frame);
            goto done;
        }

        state = ov_websocket_parse_frame(frame, &next);

        switch (state) {

            case OV_WEBSOCKET_PARSER_SUCCESS:

                if (!ov_websocket_frame_shift_trailing_bytes(
                        frame, next, (ov_websocket_frame **)&next_frame)) {
                    frame = ov_websocket_frame_free(frame);
                    goto error;
                }

                if (!process_wss_frame(self, conn, frame)) goto error;

                if (0 == next_frame->buffer->start[0]) {
                    next_frame = ov_websocket_frame_free(next_frame);
                    goto done;
                }

                frame = next_frame;
                break;

            case OV_WEBSOCKET_PARSER_PROGRESS:

                // not enough data received, wait for next IO cycle

                conn->data = frame;
                frame = NULL;
                goto done;
                break;

            default:
                frame = ov_websocket_frame_free(frame);
                goto error;
        }

    } while (frame);

done:
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static ov_websocket_message_config get_websocket_message_config(
    ov_io_web *self, const ov_http_message *msg) {

    OV_ASSERT(self);
    OV_ASSERT(msg);

    const ov_http_header *header_host = ov_http_header_get_unique(
        msg->header, msg->config.header.capacity, OV_HTTP_KEY_HOST);

    if (!header_host) return (ov_websocket_message_config){0};

    uint8_t *hostname = (uint8_t *)header_host->value.start;
    size_t hostname_length = header_host->value.length;

    uint8_t *colon = memchr(hostname, ':', header_host->value.length);
    if (colon) hostname_length = colon - hostname;

    size_t len_hostname_key = hostname_length + 1;
    uint8_t hostname_key[len_hostname_key];
    memset(hostname_key, 0, len_hostname_key);

    size_t len_uri_key = msg->request.uri.length + 1;
    uint8_t uri_key[len_uri_key];
    memset(uri_key, 0, len_uri_key);

    if (NULL == msg->request.uri.start) goto error;
    if (0 == msg->request.uri.start[0]) goto error;
    if (0 == msg->request.uri.length) goto error;

    if (!memcpy(hostname_key, hostname, hostname_length)) goto error;

    if (!memcpy(uri_key, msg->request.uri.start, msg->request.uri.length))
        goto error;

    ov_dict *hosts = ov_dict_get(self->hosts, (char *)hostname_key);
    if (!hosts) goto error;

    ov_websocket_message_config *c = ov_dict_get(hosts, uri_key);
    if (!c) goto error;

    return *c;

error:
    return (ov_websocket_message_config){0};
}

/*----------------------------------------------------------------------------*/

static bool upgrade_to_websocket(ov_io_web *self, Connection *conn, 
    ov_http_message *msg){

    OV_ASSERT(self);
    OV_ASSERT(conn);
    OV_ASSERT(msg);

    ov_websocket_message_config config =
            get_websocket_message_config(self, msg);
    
    if (!config.userdata) goto error;

    conn->websocket = calloc(1, sizeof(Websocket));
    if (!conn->websocket) goto error;

    conn->websocket->config = config;
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool process_https_message(ov_io_web *self, Connection *conn, 
    ov_http_message *msg){

    ov_http_message *response = NULL;

    OV_ASSERT(self);
    OV_ASSERT(conn);
    OV_ASSERT(msg);

    bool is_websocket_handshake = false;

    if (ov_websocket_process_handshake_request(
            msg, &response, &is_websocket_handshake)) {

        /* Websocket handshake with generated response */
        OV_ASSERT(response);
        OV_ASSERT(is_websocket_handshake);

        if (!upgrade_to_websocket(self, conn, msg))
            goto error;

        if (!ov_io_send(
                self->config.io,
                conn->socket,
                (ov_memory_pointer){.start = response->buffer->start,
                                    .length = response->buffer->length}))
            goto error;

        response = ov_http_message_free(response);

    } else if (is_websocket_handshake) {
        response = ov_http_message_free(response);
        goto error;

    } else {

        /* Not a websocket upgrade */
        OV_ASSERT(NULL == response);

        if (self->config.callbacks.https){
            
            return self->config.callbacks.https(
                self->config.callbacks.userdata,
                conn->socket,
                msg);

        }
    }

    msg = ov_http_message_free(msg);
    return true;
error:
    ov_http_message_free(msg);
    ov_http_message_free(response);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool process_https(ov_io_web *self, Connection *conn){

    OV_ASSERT(self);
    OV_ASSERT(conn);

    uint8_t *next = NULL;
    ov_http_parser_state state = OV_HTTP_PARSER_ERROR;

    ov_http_message *msg = ov_http_message_cast(conn->data);
    ov_http_message *next_msg = NULL;

    do {

        OV_ASSERT(msg->buffer);

        if (0 == msg->buffer->start[0]) {
            msg = ov_http_message_free(msg);
            goto done;
        }

        state = ov_http_pointer_parse_message(msg, &next);

        switch (state) {

            case OV_HTTP_PARSER_SUCCESS:

                if (!ov_http_message_shift_trailing_bytes(
                        msg, next, (ov_http_message **)&next_msg)) {
                    msg = ov_http_message_free(msg);
                    goto done;
                }

                if (!process_https_message(self, conn, msg)) goto error;

                if (!next_msg) {
                    goto done;
                }

                msg = next_msg;
                break;

            case OV_HTTP_PARSER_PROGRESS:

                // not enough data received, wait for next IO cycle
                conn->data = msg;
                msg = NULL;
                goto done;
                break;

            default:
                msg = ov_http_message_free(msg);
                goto error;
        }

    } while (msg);

done:
    return true;

error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool cb_io(void *userdata, int socket, const char *domain,
    const ov_memory_pointer data){

    Connection *conn = NULL;
    ov_http_message *http = NULL;
    ov_websocket_frame *frame = NULL;

    bool result = false;

    ov_io_web *self = ov_io_web_cast(userdata);
    if (!self) goto error;

    if (socket >= self->config.limits.sockets) goto error;

    conn = &self->connections[socket];

    if (domain) {

        if (!conn->domain){
            
            conn->domain = ov_string_dup(domain);
        
        } else {

            if (0 != ov_string_compare(conn->domain, domain)){
                ov_log_error("Domain inconsistency - closing");
                goto error;
            }

        }

    }

    if (conn->websocket) {

        if (conn->data) {

            frame = ov_websocket_frame_cast(conn->data);
            if (!frame) goto error;

        } else {

            frame = ov_websocket_frame_create(self->config.frame_config);
            if (!frame) goto error;

            conn->data = frame;

        }

        OV_ASSERT(frame);

        if (!ov_buffer_push(frame->buffer, (void*)data.start, data.length))
            goto error;

        result = process_wss(self, conn);
    
    } else {

        if (conn->data) {

            http = ov_http_message_cast(conn->data);
            if (!http) goto error;

        } else {

            http = ov_http_message_create(self->config.http_config);
            if (!http) goto error;

            conn->data = http;

        }

        OV_ASSERT(http);

        if (!ov_buffer_push(http->buffer, (void *)data.start, data.length))
            goto error;

        result = process_https(self, conn);
    }

    if (result)
        return result;

error:
    if (conn) close_connection(self, conn);
    return false;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      #GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

static bool init_socket(ov_io_web *self){

    if (!self) goto error;

    ov_io_socket_config io_config =
        (ov_io_socket_config){.socket = self->config.socket,
                              .callbacks.userdata = self,
                              .callbacks.accept = cb_io_accept,
                              .callbacks.io = cb_io,
                              .callbacks.close = cb_io_close};

    self->socket = ov_io_open_listener(self->config.io, io_config);
    if (-1 == self->socket) goto error;

    return true;
error:
    return false;
}


/*----------------------------------------------------------------------------*/

static bool init_config(ov_io_web_config *config){

    if (!config) goto error;

    if (!config->loop){
        ov_log_error("Config without eventloop.");
        goto error;
    }

    if (!config->io){
        ov_log_error("Config without io.");
        goto error;
    }   

    config->http_config = ov_http_message_config_init(config->http_config);

    if (0 == config->frame_config.buffer.default_size)
        config->frame_config.buffer.default_size = OV_UDP_PAYLOAD_OCTETS;

    if (0 == config->limits.sockets)
        config->limits.sockets = 
            ov_socket_get_max_supported_runtime_sockets(UINT32_MAX);
    
    if (0 == config->socket.host[0])
        config->socket = (ov_socket_configuration) {
            .host = "0.0.0.0",
            .port = 443,
            .type = TCP
        };

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

ov_io_web *ov_io_web_create(ov_io_web_config config){

    ov_io_web *self = NULL;

    if (!init_config(&config)) goto error;

    size_t size = sizeof(ov_io_web) + 
        config.limits.sockets * sizeof(Connection);

    self = calloc(1, size);
    if (!self) goto error;

    self->magic_bytes = OV_IO_WEB_MAGIC_BYTES;
    self->config = config;

    ov_dict_config d_config = ov_dict_string_key_config(255);
    d_config.value.data_function.free = ov_dict_free;

    self->hosts = ov_dict_create(d_config);
    self->events = ov_dict_create(d_config);

    if (!self->hosts) goto error;
    if (!self->events) goto error;

    if (!init_socket(self)) goto error;

    for (int i = 0; i < self->config.limits.sockets; i++){

        self->connections[i].socket = -1;
    }

    return self;
error:
    ov_io_web_free(self);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_io_web *ov_io_web_free(ov_io_web *self){

    if (!ov_io_web_cast(self)) return self;

    for (int i = 0; i < self->config.limits.sockets; i++){

        close_connection(self, &self->connections[i]);
    }

    self->hosts = ov_dict_free(self->hosts);
    self = ov_data_pointer_free(self);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_io_web *ov_io_web_cast(const void *data){

    if (!data) goto error;

    if (*(uint16_t *)data == OV_IO_WEB_MAGIC_BYTES) return (ov_io_web *)data;
error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

bool ov_io_web_close(ov_io_web *self, int socket){

    if (!self) goto error;
    if (socket >= self->config.limits.sockets) goto error;

    return close_connection(self, &self->connections[socket]);
error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_io_web_send(ov_io_web *self, int socket, const ov_http_message *msg){

    if (!self) goto error;
    if (!msg) goto error;

    return ov_io_send(self->config.io, 
        socket,
        (ov_memory_pointer){
            .start = msg->buffer->start,
            .length = msg->buffer->length
        });
error:
    return false;
}


/*----------------------------------------------------------------------------*/

static bool cb_wss_send_json(void *userdata, int socket, const ov_json_value *msg) {

    char *str = NULL;
    ov_websocket_frame *frame = NULL;

    ov_io_web *self = ov_io_web_cast(userdata);
    if (!self || !msg || socket < 0) goto error;

    if (socket >= self->config.limits.sockets) goto error;

    Connection *conn = &self->connections[socket];
    if (!conn->websocket) goto error;

    str = ov_json_value_to_string(msg);
    if (!str) goto error;

    ssize_t length = strlen(str);

    /* Send the string over websocket frames */

    frame = ov_websocket_frame_create((ov_websocket_frame_config){
        .buffer.default_size = OV_UDP_PAYLOAD_OCTETS});

    if (!frame) goto error;

    ov_websocket_frame_clear(frame);

    ssize_t chunk = 500;

    if (length < chunk) {

        /* Send one WS frame */

        frame->buffer->start[0] = 0x80 | OV_WEBSOCKET_OPCODE_TEXT;

        if (!ov_websocket_set_data(frame, (uint8_t *)str, length, false))
            goto error;

        if (!ov_io_send(self->config.io,
                        socket,
                        (ov_memory_pointer){.start = frame->buffer->start,
                                            .length = frame->buffer->length}))
            goto error;

        goto done;
    }

    /* send fragmented */
    size_t counter = 0;

    uint8_t *ptr = (uint8_t *)str;
    ssize_t open = length;

    /* send fragmented start */

    frame->buffer->start[0] = 0x00 | OV_WEBSOCKET_OPCODE_TEXT;

    if (!ov_websocket_set_data(frame, ptr, chunk, false)) goto error;

    if (!ov_io_send(self->config.io,
                    socket,
                    (ov_memory_pointer){.start = frame->buffer->start,
                                        .length = frame->buffer->length}))
        goto error;

    counter++;

    /* send continuation */

    open -= chunk;
    ptr += chunk;

    while (open > chunk) {

        frame->buffer->start[0] = 0x00;

        if (!ov_websocket_set_data(frame, ptr, chunk, false)) goto error;

        if (!ov_io_send(self->config.io,
                        socket,
                        (ov_memory_pointer){.start = frame->buffer->start,
                                            .length = frame->buffer->length}))
            goto error;

        open -= chunk;
        ptr += chunk;

        counter++;
    }

    /* send last */

    frame->buffer->start[0] = 0x80;

    if (!ov_websocket_set_data(frame, ptr, open, false)) goto error;

    if (!ov_io_send(self->config.io,
                    socket,
                    (ov_memory_pointer){.start = frame->buffer->start,
                                        .length = frame->buffer->length}))
        goto error;

    counter++;
    UNUSED(counter);

done:
    frame = ov_websocket_frame_free(frame);
    str = ov_data_pointer_free(str);
    return true;
error:
    frame = ov_websocket_frame_free(frame);
    ov_data_pointer_free(str);
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_io_web_send_json(ov_io_web *self, int socket, const ov_json_value *msg){

    if (!self || !msg) goto error;

    return cb_wss_send_json(self, socket, msg);
error:
    return false;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      CONFIG FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_io_web_config ov_io_web_config_from_json(const ov_json_value *input){

    ov_io_web_config config = {0};

    const ov_json_value *conf = ov_json_object_get(input, OV_KEY_WEBSERVER);
    if (!conf) conf = input;

    const char *name = ov_json_string_get(ov_json_get(conf, "/"OV_KEY_NAME));
    if (name) strncpy(config.name, name, PATH_MAX);

    config.http_config = ov_http_message_config_from_json(conf);
    config.frame_config = ov_websocket_frame_config_from_json(conf);

    config.socket = ov_socket_configuration_from_json(
        ov_json_get(conf, "/"OV_KEY_SOCKET),
        (ov_socket_configuration){0});

    config.limits.sockets = ov_json_number_get(
        ov_json_get(conf, "/"OV_KEY_LIMITS"/"OV_KEY_SOCKETS));

    return config;
}

/*----------------------------------------------------------------------------*/

bool ov_io_web_configure_websocket_callback(ov_io_web *self,
                                            const char *hostname,
                                            ov_websocket_message_config config){
    
    if (!self || !hostname) goto error;

    ov_dict *hosts = ov_dict_get(self->hosts, hostname);
    if (!hosts) {

        ov_dict_config d_config = ov_dict_string_key_config(255);
        d_config.value.data_function.free = ov_data_pointer_free;

        hosts = ov_dict_create(d_config);

        if (!ov_dict_set(self->hosts, ov_string_dup(hostname), hosts, NULL)) {
            hosts = ov_dict_free(hosts);
            goto error;
        }
    }

    // reset ?
    if (NULL == config.userdata) {

        ov_dict_del(self->hosts, hostname);
        goto done;
    }

    if (0 == config.uri[0]) strcat(config.uri, "/");

    ov_websocket_message_config *val =
        calloc(1, sizeof(ov_websocket_message_config));

    *val = config;

    size_t len = strlen(config.uri) + 2;
    char *key = calloc(1, len);
    if (!key) goto error;

    if ('/' == config.uri[0]) {
        snprintf(key, len, "%s", config.uri);
    } else {
        snprintf(key, len, "/%s", config.uri);
    }

    if (!ov_dict_set(hosts, key, val, NULL)) {

        val = ov_data_pointer_free(val);
        key = ov_data_pointer_free(key);
        goto error;
    }

    ov_log_debug(
        "configured websocket callback for domain "
        "%s URI %s",
        hostname,
        key);
done:
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool cb_wss_io_to_json(void *userdata,
                              int socket,
                              const ov_memory_pointer domain,
                              const char *uri,
                              ov_memory_pointer content,
                              bool text) {

    ov_json_value *value = NULL;
    ov_io_web *self = ov_io_web_cast(userdata);

    if (!self) return false;
    if (!text) return false;
    if (socket < 0) return false;

    value = ov_json_value_from_string((char *)content.start, content.length);
    if (!value) return false;

    char *host[domain.length + 1];
    memset(host, 0, domain.length + 1);
    strncpy((char *)host, (char *)domain.start, domain.length);

    ov_dict *callbacks = ov_dict_get(self->events, host);
    if (!callbacks) goto error;

    ov_event_io_config *event_config = ov_dict_get(callbacks, uri);

    if (!event_config || !event_config->callback.process) goto error;

    ov_event_io_config conf = *event_config;
    ov_event_parameter parameter = (ov_event_parameter){
        .send.instance = self, .send.send = cb_wss_send_json};

    memcpy(parameter.uri.name, uri, strlen(uri));
    memcpy(parameter.domain.name, domain.start, domain.length);

    return conf.callback.process(conf.userdata, socket, &parameter, value);

error:
    ov_json_value_free(value);
    return false;
}


/*----------------------------------------------------------------------------*/

bool ov_io_web_configure_uri_event_io(ov_io_web *self,
                                      const char *hostname,
                                      const ov_event_io_config config){

    if (!self || !hostname) goto error;

    ov_websocket_message_config wss_io_handler =
        (ov_websocket_message_config){.userdata = self,
                                      .max_frames = IMPL_MAX_FRAMES_FOR_JSON,
                                      .callback = cb_wss_io_to_json,
                                      .close = config.callback.close};

    if (0 != config.name[0]) strncpy(wss_io_handler.uri, config.name, PATH_MAX);

    if (!ov_io_web_configure_websocket_callback(self, hostname, wss_io_handler))
        return false;

    ov_dict *hosts = ov_dict_get(self->events, hostname);
    if (!hosts) {

        ov_dict_config d_config = ov_dict_string_key_config(255);
        d_config.value.data_function.free = ov_data_pointer_free;

        hosts = ov_dict_create(d_config);

        if (!ov_dict_set(self->events, ov_string_dup(hostname), hosts, NULL)) {
            hosts = ov_dict_free(hosts);
            goto error;
        }
    }

    OV_ASSERT(hosts);

    // reset ?
    if (NULL == config.userdata) {

        ov_dict_del(self->events, hostname);
        goto done;
    }

    char *key = ov_string_dup(config.name);
    if (!key) goto error;

    ov_event_io_config *val = calloc(1, sizeof(ov_event_io_config));
    if (!val) {
        key = ov_data_pointer_free(key);
        goto error;
    }

    *val = config;

    if (!ov_dict_set(hosts, key, val, NULL)) {
        key = ov_data_pointer_free(key);
        val = ov_data_pointer_free(val);
        goto error;
    }

    ov_log_debug(
        "configured event_handler callback for domain "
        "%s URI %s",
        hostname,
        config.name);

done:
    return true;

error:
    return false;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      DOMAIN FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

static bool cleaned_path_for_conn(ov_io_web *self,
                                  Connection *conn,
                                  const ov_http_message *msg,
                                  size_t len,
                                  char *out) {

    ov_uri *uri = NULL;

    if (!conn || !out || !msg) goto error;

    /* create PATH based on document root and uri request */

    uri = ov_uri_from_string(
        (char *)msg->request.uri.start, msg->request.uri.length);

    if (!uri || !uri->path) goto error;

    if (len < PATH_MAX) goto error;

    /* clean empty AND dot paths in uri path */

    char cleaned_path[PATH_MAX + 1] = {0};
    if (!ov_uri_path_remove_dot_segments(uri->path, cleaned_path)) goto error;

    /* clean empty paths between document root and uri path,
     *
     * (A) document root may finish with /
     * (B) we ensure to add some /
     * (C) uri may contain some initial /
     *
     * --> delete any non requried for some clean path */

    char full_path[PATH_MAX + 1] = {0};

    ov_domain *domain = ov_io_get_domain(self->config.io, conn->domain);
    if (!domain) goto error;

    ssize_t bytes = snprintf(
        full_path, PATH_MAX, "%s/%s", domain->config.path, cleaned_path);

    if (bytes < 1) goto error;

    if (!ov_uri_path_remove_dot_segments(full_path, out)) goto error;

    if ('/' == out[strlen(out) - 1]) {

        if (PATH_MAX - bytes < 12) goto error;

        strcat(out, "index.html");
    }

    uri = ov_uri_free(uri);
    return true;
error:
    uri = ov_uri_free(uri);
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_io_web_uri_file_path(ov_io_web *self,
                             int socket,
                             const ov_http_message *request,
                             size_t length,
                             char *path){

    if (!self || !request || !length || !path) goto error;

    if (socket >= self->config.limits.sockets) goto error;

    Connection *conn = &self->connections[socket];

    return cleaned_path_for_conn(self, conn, request, length, path);
error:
    return false;
}