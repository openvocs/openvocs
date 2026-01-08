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
        @file           ov_web_server_connection.c
        @author         Töpfer, Markus

        @date           2025-11-06


        ------------------------------------------------------------------------
*/
#include "../include/ov_web_server_connection.h"

#include <ov_base/ov_linked_list.h>
#include <ov_base/ov_time.h>
#include <ov_base/ov_uri.h>

#include <ov_core/ov_event_io.h>

#include <openssl/conf.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/ssl.h>

#define OV_WEB_SERVER_CONNECTION_MAGIC_BYTE 0xffe1

#define FLAG_CLOSE_AFTER_SEND 0x0001
#define FLAG_SEND_SSL_SHUTDOWN 0x0010
#define FLAG_CLIENT_SHUTDOWN 0x0100

#define IMPL_WEBSOCKET_PHRASE_LENGTH 250
#define IMPL_BUFFER_SIZE_WS_FRAME_DEFRAG 1000000 // 1MB

#define OV_SSL_MAX_BUFFER 16000 // 16kb SSL buffer max
#define OV_SSL_ERROR_STRING_BUFFER_SIZE 200

/*----------------------------------------------------------------------------*/

typedef enum ConnectionType {

    CONNECTION_TYPE_HTTP_1_1 = 0,
    CONNECTION_TYPE_WEBSOCKET = 1

} ConnectionType;

/*----------------------------------------------------------------------------*/

struct ov_web_server_connection {

    uint16_t magic_byte;
    ov_web_server *server;

    int socket;

    uint16_t flags;
    uint64_t created;

    ConnectionType type;

    ov_domain *domain;

    void *data;

    ov_socket_data remote;

    struct {

        ov_websocket_message_config config;
        ov_list *queue;

        ov_websocket_fragmentation_state last;

        uint64_t counter;

        struct {

            int code;
            char phrase[IMPL_WEBSOCKET_PHRASE_LENGTH];

            uint64_t wait_for_response;

            bool send;
            bool recv;

        } close;

    } websocket;

    struct {

        SSL *ssl;
        bool handshaked;

    } tls;

    struct {

        /*  Outgoing data buffering.
         *
         *  When some format is set, the format defines the actual data
         *  to be send. (PRIO 1)
         *
         *  buffer will contain any leftovers to be send (PRIO 2)
         *  queue will contain next data packages to be send (PRIO 3)
         *
         *  NOTE always use bool push_outgoing(Connection *conn, void* data)
         *  to add data to the outgoing queue, to enable outgoing event
         *  readiness listening for the socket!
         */
        struct {

            ov_buffer *buffer;
            ov_list *queue;

            uint64_t bytes; // counter outgoing bytes
            uint64_t last;  // time of last seen outgoing io

            uint64_t size; // send buffer size outgoing

        } out;

        struct {

            uint64_t bytes; // counter incoming bytes
            uint64_t last;  // time of last seen incoming io

        } in;

    } io;
};

/*----------------------------------------------------------------------------*/

static bool tls_send_buffer(ov_web_server_connection *self,
                            const ov_buffer *buffer);

/*----------------------------------------------------------------------------*/

static bool defragmented_callback(ov_web_server_connection *self, bool text) {

    OV_ASSERT(self);
    OV_ASSERT(self->websocket.queue);
    OV_ASSERT(self->websocket.config.userdata);
    OV_ASSERT(self->websocket.config.callback);

    /*
     *  This function will create some defragmented buffer containing all
     *  content of the websocket frames of the queue.
     *
     *  It will NOT check if framing is correct, e.g. if reception line is
     *  valid, which is done in @see defragmented_wss_delivery
     */

    ov_websocket_frame *frame = NULL;

    ov_buffer *buffer = ov_buffer_create(IMPL_BUFFER_SIZE_WS_FRAME_DEFRAG);
    if (!buffer)
        goto error;

    frame = ov_list_queue_pop(self->websocket.queue);

    if (!frame)
        goto error;

    while (frame) {

        if (!ov_buffer_push(buffer, (void *)frame->content.start,
                            frame->content.length))
            goto error;

        frame = ov_websocket_frame_free(frame);
        frame = ov_list_queue_pop(self->websocket.queue);
    }

    if (!self->websocket.config.callback(
            self->websocket.config.userdata, self->socket,
            (ov_memory_pointer){self->domain->config.name.start,
                                self->domain->config.name.length},
            self->websocket.config.uri,
            (ov_memory_pointer){.start = buffer->start,
                                .length = buffer->length},
            text))
        goto error;

    ov_buffer_free(buffer);
    self->websocket.counter = 0;
    return true;
error:
    ov_buffer_free(buffer);
    ov_websocket_frame_free(frame);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool defragmented_wss_delivery(ov_web_server_connection *self,
                                      ov_websocket_frame *frame, bool text) {

    OV_ASSERT(self);
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
        switch (self->websocket.last) {

        case OV_WEBSOCKET_FRAGMENTATION_NONE:
        case OV_WEBSOCKET_FRAGMENTATION_LAST:
            break;

        default:
            goto error;
        }

        /* Non fragmented frame -> perform callback */

        if (!self->websocket.config.callback(
                self->websocket.config.userdata, self->socket,
                (ov_memory_pointer){self->domain->config.name.start,
                                    self->domain->config.name.length},
                self->websocket.config.uri, frame->content, text)) {

            self->websocket.close.code = 1002;
            const char *phrase = "content not accepted";
            strncpy(self->websocket.close.phrase, phrase, strlen(phrase));
            goto error;
        }

        ov_websocket_frame_free(frame);
        self->websocket.counter = 0;
        self->websocket.last = OV_WEBSOCKET_FRAGMENTATION_NONE;

        /* do not push to queue */
        return true;

        break;

    case OV_WEBSOCKET_FRAGMENTATION_START:

        /* fragmentation start */
        switch (self->websocket.last) {

        case OV_WEBSOCKET_FRAGMENTATION_NONE:
        case OV_WEBSOCKET_FRAGMENTATION_LAST:
            break;

        default:
            goto error;
        }

        if (!self->websocket.queue) {
            self->websocket.queue = ov_linked_list_create(
                (ov_list_config){.item.free = ov_websocket_frame_free});

            if (!self->websocket.queue) {
                goto error;
            }
        }

        out = ov_list_queue_pop(self->websocket.queue);

        if (out) {

            out = ov_websocket_frame_free(out);
            goto error;
        }

        /* push to queue */
        break;

    case OV_WEBSOCKET_FRAGMENTATION_CONTINUE:

        switch (self->websocket.last) {

        case OV_WEBSOCKET_FRAGMENTATION_START:
        case OV_WEBSOCKET_FRAGMENTATION_CONTINUE:
            break;

        default:
            goto error;
        }

        /* push to queue */
        break;

    case OV_WEBSOCKET_FRAGMENTATION_LAST:

        switch (self->websocket.last) {

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

    if (!ov_list_queue_push(self->websocket.queue, frame)) {
        goto error;
    }

    self->websocket.counter++;
    self->websocket.last = frame->state;
    frame = NULL;

    if (0 != self->websocket.config.max_frames) {

        if (self->websocket.counter >= self->websocket.config.max_frames) {

            if (!callback_queue) {

                self->websocket.close.code = 1002;
                const char *phrase = "max frames reached";
                strncpy(self->websocket.close.phrase, phrase, strlen(phrase));
                goto error;
            }
        }
    }

    if (callback_queue)
        return defragmented_callback(self, text);

    return true;
error:
    if (self && (0 == self->websocket.close.code)) {
        self->websocket.close.code = 1002;
        const char *phrase = "websocket protocol error";
        strncpy(self->websocket.close.phrase, phrase, strlen(phrase));
    }
    ov_websocket_frame_free(frame);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool process_wss_control_frame(ov_web_server_connection *self,
                                      ov_websocket_frame *frame) {

    OV_ASSERT(self);
    OV_ASSERT(frame);

    ov_websocket_frame *response = NULL;

    switch (frame->opcode) {

    case OV_WEBSOCKET_OPCODE_PONG:

        // nothing to do on pong
        break;

    case OV_WEBSOCKET_OPCODE_PING:

        // We need to send a PONG with the same application data as received
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

            frame->buffer->length = 2;
        }

        /*
         *  Try to send direct reponse or schedule response in
         *  send queue.
         */

        if (!tls_send_buffer(self, response->buffer))
            goto error;

        break;

    case OV_WEBSOCKET_OPCODE_CLOSE:

        self->websocket.close.recv = true;

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

static bool process_wss_frame(ov_web_server_connection *self,
                              ov_websocket_frame *frame) {

    if (!self || !frame)
        goto error;

    if (frame->opcode >= 0x08)
        return process_wss_control_frame(self, frame);

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

    if (!ov_websocket_frame_unmask(frame)) {
        goto error;
    }

    if (self->websocket.config.fragmented) {

        /* unfragmented websocket callback used */

        return self->websocket.config.fragmented(
            self->websocket.config.userdata, self->socket,
            (ov_memory_pointer){self->domain->config.name.start,
                                self->domain->config.name.length},
            self->websocket.config.uri, frame);
    }

    return defragmented_wss_delivery(self, frame, text);

error:
    ov_websocket_frame_free(frame);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool process_wss(ov_web_server_connection *self) {

    OV_ASSERT(self);

    ov_websocket_frame *frame = ov_websocket_frame_cast(self->data);
    if (!frame)
        goto error;

    uint8_t *next = NULL;
    ov_websocket_parser_state state = OV_WEBSOCKET_PARSER_ERROR;

    while (frame) {

        OV_ASSERT(frame->buffer);

        if (frame->buffer->length == 0)
            goto done;

        state = ov_websocket_parse_frame(frame, &next);

        switch (state) {

        case OV_WEBSOCKET_PARSER_SUCCESS:

            self->data = NULL;

            if (!ov_websocket_frame_shift_trailing_bytes(
                    frame, next, (ov_websocket_frame **)&self->data)) {
                frame = ov_websocket_frame_free(frame);
                goto error;
            }

            if (!process_wss_frame(self, frame))
                goto error;

            frame = self->data;
            break;

        case OV_WEBSOCKET_PARSER_PROGRESS:

            // not enough data received, wait for next IO cycle
            goto done;
            break;

        default:
            self->websocket.close.code = 1002;
            const char *phrase = "not a websocket frame";
            strncpy(self->websocket.close.phrase, phrase, strlen(phrase));
            goto error;
        }
    }

done:
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool check_websocket_enabled(ov_web_server_connection *self,
                                    const ov_http_message *msg,
                                    const uint8_t *hostname,
                                    size_t hostname_length) {

    if (!self || !msg || !hostname || (0 == hostname_length))
        return false;

    size_t len = msg->request.uri.length + 1;
    uint8_t key[len];
    memset(key, 0, len);

    ov_domain *domain = self->domain;
    if (!domain)
        goto error;

    /* URI callbacks are stored in a dict with /0 terminated strings,
     * so we MUST ensure to check with /0 terminated uri */

    if (NULL == msg->request.uri.start)
        goto error;

    if (0 == msg->request.uri.start[0])
        goto error;

    if (0 == msg->request.uri.length)
        goto error;

    if (!memcpy(key, msg->request.uri.start, msg->request.uri.length))
        goto error;

    ov_websocket_message_config *config =
        (ov_websocket_message_config *)ov_dict_get(domain->websocket.uri, key);

    if (!config)
        config = &domain->websocket.config;

    self->websocket.config = *config;

    return true;
error:
    if (self)
        self->websocket.config = (ov_websocket_message_config){0};

    return false;
}

/*----------------------------------------------------------------------------*/

static bool process_https_message(ov_web_server_connection *self,
                                  ov_http_message *msg) {

    ov_http_message *response = NULL;

    if (!self || !msg)
        goto error;

    ov_web_server_config config = ov_web_server_get_config(self->server);

    bool is_websocket_handshake = false;

    /*  Check request domain is the same domain name as used for SSL */

    const ov_http_header *header_host = ov_http_header_get_unique(
        msg->header, msg->config.header.capacity, OV_HTTP_KEY_HOST);

    if (!header_host)
        goto error;

    uint8_t *hostname = (uint8_t *)header_host->value.start;
    size_t hostname_length = header_host->value.length;

    uint8_t *colon = memchr(hostname, ':', header_host->value.length);
    if (colon)
        hostname_length = colon - hostname;

    if (!self->domain)
        goto error;

    if (ov_websocket_process_handshake_request(msg, &response,
                                               &is_websocket_handshake)) {

        /* Websocket handshake with generated response */
        OV_ASSERT(response);
        OV_ASSERT(is_websocket_handshake);

        if (!check_websocket_enabled(self, msg, hostname, hostname_length)) {
            response = ov_http_message_free(response);
            goto error;
        }

        self->type = CONNECTION_TYPE_WEBSOCKET;
        if (!tls_send_buffer(self, response->buffer))
            goto error;

        response = ov_http_message_free(response);

    } else if (is_websocket_handshake) {

        /* We will not send any response, but close the socket */
        response = ov_http_message_free(response);
        goto error;

    } else {

        /* Not a websocket upgrade */
        OV_ASSERT(NULL == response);

        if (config.callback.https) {
            return config.callback.https(config.callback.userdata, self->socket,
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

static bool process_https(ov_web_server_connection *self) {

    OV_ASSERT(self);

    ov_http_message *msg = ov_http_message_cast(self->data);
    if (!msg)
        goto error;

    uint8_t *next = NULL;
    ov_http_parser_state state = OV_HTTP_PARSER_ERROR;

    while (msg) {

        OV_ASSERT(msg->buffer);

        if (0 == msg->buffer->start[0])
            goto done;

        state = ov_http_pointer_parse_message(msg, &next);

        switch (state) {

        case OV_HTTP_PARSER_SUCCESS:

            self->data = NULL;

            if (!ov_http_message_shift_trailing_bytes(
                    msg, next, (ov_http_message **)&self->data)) {
                msg = ov_http_message_free(msg);
                goto error;
            }

            if (!process_https_message(self, msg))
                goto error;

            msg = self->data;
            break;

        case OV_HTTP_PARSER_PROGRESS:

            // not enough data received, wait for next IO cycle
            goto done;
            break;

        default:
            goto error;
        }
    }

done:
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool process_tls_io(ov_web_server_connection *self) {

    if (!self)
        goto error;

    /* At this point we read something to some buffer,
     * but are unaware of the type */

    switch (self->type) {

    case CONNECTION_TYPE_HTTP_1_1:
        return process_https(self);
        break;

    case CONNECTION_TYPE_WEBSOCKET:
        return process_wss(self);
        break;
    };

    // fall through into error

error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool io_tls(int socket_fd, uint8_t events, void *data);

/*----------------------------------------------------------------------------*/

static bool tls_perform_handshake(ov_web_server_connection *self) {

    OV_ASSERT(self);
    OV_ASSERT(self->tls.ssl);

    char errorstring[OV_SSL_ERROR_STRING_BUFFER_SIZE];
    int errorcode = -1;
    int n = 0, r = 0;

    if (!self)
        goto error;

    /* Send some error SHUTDOWN SSL and send to remote */

    self->flags = FLAG_SEND_SSL_SHUTDOWN;

    r = SSL_accept(self->tls.ssl);

    if (1 == r) {

        const char *hostname =
            SSL_get_servername(self->tls.ssl, TLSEXT_NAMETYPE_host_name);

        if (!hostname) {

            self->domain = ov_web_server_get_default_domain(self->server);

        } else {

            /* Set domain pointer for connection */
            self->domain = ov_web_server_find_domain(
                self->server, (uint8_t *)hostname, strlen(hostname));
        }

        if (!self->domain)
            goto error;

        self->tls.handshaked = true;

    } else {

        n = SSL_get_error(self->tls.ssl, r);

        switch (n) {

        case SSL_ERROR_NONE:
            /* no error */
            break;
        case SSL_ERROR_WANT_READ:
            /* return to loop to reread */
            break;
        case SSL_ERROR_WANT_WRITE:
            /* return to loop to rewrite */
            break;
        case SSL_ERROR_WANT_CONNECT:
            /* return to loop to reconnect */
            break;
        case SSL_ERROR_WANT_ACCEPT:
            /* return to loop to reaccept */
            break;
        case SSL_ERROR_WANT_X509_LOOKUP:
            /* return to loop for loopup */
            break;

        case SSL_ERROR_ZERO_RETURN:
            // connection close
            goto error;
            break;

        case SSL_ERROR_SYSCALL:

            if (r == 0)
                break;
            /*
            ov_log_error(
            "SSL_ERROR_SYSCALL"
            "%d | %s",
            errno, strerror(errno));
            */
            goto error;
            break;

        case SSL_ERROR_SSL:

            errorcode = ERR_get_error();
            ERR_error_string_n(errorcode, errorstring,
                               OV_SSL_ERROR_STRING_BUFFER_SIZE);

            /* in case of some SSL_ERROR we cannot send the shutdown */
            goto send_no_shutdown;
            break;

        default:
            goto error;
            break;
        }
    }

    return true;

send_no_shutdown:

    self->flags = 0;

error:

    ov_web_server_close_connection(self->server, self->socket);

    return false;
}

/*----------------------------------------------------------------------------*/

static bool tls_send_buffer(ov_web_server_connection *self,
                            const ov_buffer *buffer) {

    ov_buffer *copy = NULL;

    if (!self)
        goto error;

    self->flags = FLAG_SEND_SSL_SHUTDOWN;

    if (!self->tls.handshaked)
        goto error;

    ov_web_server_config config = ov_web_server_get_config(self->server);
    ov_event_loop *loop = config.loop;

    /* Ensure outgoing readiness listening */

    if (!loop->callback.set(loop, self->socket,
                            OV_EVENT_IO_IN | OV_EVENT_IO_ERR |
                                OV_EVENT_IO_CLOSE | OV_EVENT_IO_OUT,
                            self, io_tls))
        goto error;

    /*
     *  Try to send the data, or push the data to the outgoing buffer if
     *  a direct send fails.
     */

    size_t max = ov_socket_get_send_buffer_size(self->socket);
    if (max < buffer->length) {

        /*
         *  We MAY try to increase the send buffer size here, but this MAY
         *  have unwanted and unintendet side effects, so we split the input
         *  buffer instead and use the internal queue to transmit the message!
         *
         *  Doing so will leave the option to manipulate the actual send buffer
         *  of some socket to the application layer using this implementation.
         *
         *  ... OR we MAY try partial sends with ssl as alternative
         */

        /* Split buffer in chunks of max */

        ov_buffer *temp = NULL;
        ssize_t open = buffer->length;
        uint8_t *ptr = buffer->start;
        size_t len = 0;

        while (open > 0) {

            temp = ov_buffer_create(max);
            if (!temp)
                goto error;

            if (open > (ssize_t)max) {
                len = max;
            } else {
                len = (size_t)open;
            }

            if (!ov_buffer_set(temp, ptr, len)) {
                temp = ov_buffer_free(temp);
                goto error;
            }

            if (!ov_list_queue_push(self->io.out.queue, temp)) {
                temp = ov_buffer_free(temp);
                goto error;
            }

            temp = NULL;
            open = open - max;

            if (open > 0)
                ptr = ptr + max;
        }

        /* Return here to not increase io counters, and let processing be done
         * in next eventloop run */
        return true;
    }

    /*  SSL non blocking will fail if SSL_ERROR_WANT_WRITE is called with
     *  some other pointer as input, so we need to ensure to use the same
     *  pointer, when some write failed.
     *
     *  Therefore we need to copy the input and set the copy buffer to either
     *  conn->io.out.buffer or push it to the queue in case of error. */

    copy = ov_buffer_create(buffer->length);
    if (!ov_buffer_set(copy, (void *)buffer->start, buffer->length))
        goto error;

    if (self->io.out.buffer) {

        /* this pointer MUST be used in the next SSL_WRITE!
         * so we queue the new data to be send */

        if (!ov_list_queue_push(self->io.out.queue, copy))
            goto error;

        copy = NULL;

        /* Return here to not increase io counters */
        return true;
    }

    ssize_t bytes = SSL_write(self->tls.ssl, copy->start, copy->length);

    if (0 == bytes)
        goto error;

    /* Send all ? */

    if (bytes == (ssize_t)buffer->length) {
        copy = ov_buffer_free(copy);
        goto done;
    }

    int r = SSL_get_error(self->tls.ssl, bytes);

    switch (r) {

    case SSL_ERROR_NONE:
    case SSL_ERROR_WANT_READ:
    case SSL_ERROR_WANT_WRITE:
        /* retry */
        break;

    case SSL_ERROR_WANT_CONNECT:
    case SSL_ERROR_WANT_ACCEPT:
    case SSL_ERROR_WANT_X509_LOOKUP:
        goto error;
        break;

    case SSL_ERROR_WANT_ASYNC:
    case SSL_ERROR_WANT_ASYNC_JOB:
        goto error;
        break;

    case SSL_ERROR_WANT_CLIENT_HELLO_CB:
        goto error;
        break;

    case SSL_ERROR_SYSCALL:

        /* close here again ? */
        if (bytes == 0)
            break;
        if (errno == EAGAIN)
            break;

        goto error;

    case SSL_ERROR_SSL:
        self->flags = 0;
        goto error;
    }

    /* Send again ? */

    if ((-1 == bytes) || ((bytes < 0) && (EAGAIN == errno))) {

        OV_ASSERT(NULL == self->io.out.buffer);
        self->io.out.buffer = copy;
        copy = NULL;

        /* Return here to not increase io counters */
        return true;
    }

    /* Sent just a part of the buffer
     * (will not be reached without partial SSL send),
     * anyway we check if we get here,
     * and close the connection / process if so */

    OV_ASSERT(1 == 0);
    goto error;

done:
    self->io.out.last = ov_time_get_current_time_usecs();
    self->io.out.bytes += bytes;
    return true;
error:
    if (self)
        ov_web_server_close_connection(self->server, self->socket);

    ov_buffer_free(copy);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool io_tls_send(ov_web_server_connection *self) {

    ssize_t bytes = 0;

    ov_web_server_config config = ov_web_server_get_config(self->server);
    ov_event_loop *loop = config.loop;

    if (-1 == self->socket || !self->tls.handshaked)
        goto error;

    /* (1) Try to send any leftovers outgoing */

    if (self->io.out.buffer) {

        /* We have an active outgoing buffer,
         * and send the next data of this buffer */

        bytes = SSL_write(self->tls.ssl, self->io.out.buffer->start,
                          self->io.out.buffer->length);

        if (bytes == (ssize_t)self->io.out.buffer->length) {

            self->io.out.buffer = ov_buffer_free(self->io.out.buffer);
            self->io.out.bytes += bytes;
            self->io.out.last = ov_time_get_current_time_usecs();

            goto done;
        }

        if (0 == bytes)
            goto error;

        int r = SSL_get_error(self->tls.ssl, bytes);

        switch (r) {

        case SSL_ERROR_NONE:
        case SSL_ERROR_WANT_READ:
        case SSL_ERROR_WANT_WRITE:
            /* retry */
            break;

        case SSL_ERROR_WANT_CONNECT:
        case SSL_ERROR_WANT_ACCEPT:
        case SSL_ERROR_WANT_X509_LOOKUP:
            OV_ASSERT(1 == 0);
            goto error;
            break;

        case SSL_ERROR_WANT_ASYNC:
        case SSL_ERROR_WANT_ASYNC_JOB:
            goto error;
            break;

        case SSL_ERROR_WANT_CLIENT_HELLO_CB:
            goto error;
            break;

        case SSL_ERROR_SYSCALL:

            /* close here again ? */
            if (bytes == 0)
                break;

            if (errno == EAGAIN)
                break;

            goto error;

        case SSL_ERROR_SSL:

            self->flags = 0;
            goto error;
        }

        /* Try again */
        goto done;
    }

    OV_ASSERT(NULL == self->io.out.buffer);

    /* (2) Try to get next message of outgoing queue */

    void *out = ov_list_queue_pop(self->io.out.queue);

    if (NULL == out) {

        /*  Nothing more in queue,
         *  nothing leftover to be send,
         *  -> disable outgoing readiness listening */

        if (self->flags & FLAG_CLOSE_AFTER_SEND) {
            ov_web_server_close_connection(self->server, self->socket);
            goto done;
        }

        if (!loop->callback.set(loop, self->socket,
                                OV_EVENT_IO_CLOSE | OV_EVENT_IO_IN |
                                    OV_EVENT_IO_ERR,
                                self, io_tls)) {

            goto error;
        }

        goto done;
    }

    /* (3) Some data to send to process
     *
     *  NOTE all expected outgoing message of the queue MUST
     *  be enabled explicit at this point.
     */

    ov_http_message *msg = ov_http_message_cast(out);
    if (msg) {

        if (!tls_send_buffer(self, msg->buffer)) {
            msg = ov_http_message_free(msg);
            goto error;
        }

        msg = ov_http_message_free(msg);
        goto done;
    }

    ov_buffer *buf = ov_buffer_cast(out);
    if (buf) {

        if (!tls_send_buffer(self, buf)) {
            buf = ov_buffer_free(buf);
            goto error;
        }

        buf = ov_buffer_free(buf);
        goto done;
    }

    ov_websocket_frame *frame = ov_websocket_frame_cast(out);
    if (frame) {

        if (!tls_send_buffer(self, frame->buffer)) {
            frame = ov_websocket_frame_free(frame);
            goto error;
        }

        frame = ov_websocket_frame_free(frame);
        goto done;
    }

    OV_ASSERT(1 == 0);

done:
    return true;
error:
    if (self)
        ov_web_server_close_connection(self->server, self->socket);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool io_tls(int socket_fd, uint8_t events, void *data) {

    int n = 0;

    char buf[OV_SSL_MAX_BUFFER] = {0};

    ov_web_server_connection *self = ov_web_server_connection_cast(data);

    OV_ASSERT(self);
    OV_ASSERT(self->socket == socket_fd);

    ov_web_server_config config = ov_web_server_get_config(self->server);

    if ((events & OV_EVENT_IO_CLOSE) || (events & OV_EVENT_IO_ERR)) {

        /* Ensure SSL_shutdown will not be triggered on client close */
        self->flags = 0;
        self->flags = FLAG_CLIENT_SHUTDOWN;
        ov_web_server_close_connection(self->server, self->socket);
        return true;
    }

    if (!self->tls.handshaked)
        return tls_perform_handshake(self);

    if (!(events & OV_EVENT_IO_IN))
        goto process_out;

    /* We do have some new IO to process */

    /*
     *  Prepare buffer for READ
     */

    ov_buffer *buffer = NULL;
    ov_http_message *msg = NULL;
    ov_websocket_frame *frame = NULL;

    self->flags = FLAG_SEND_SSL_SHUTDOWN;

    msg = ov_http_message_cast(self->data);

    switch (self->type) {

    case CONNECTION_TYPE_HTTP_1_1:

        if (!msg) {

            OV_ASSERT(NULL == self->data);

            if (self->data)
                goto error;

            msg = ov_http_message_create(config.http_message);
            if (!msg)
                goto error;

            self->data = msg;
        }

        OV_ASSERT(msg->buffer);
        buffer = msg->buffer;
        break;

    case CONNECTION_TYPE_WEBSOCKET:

        /* after upgrade success the data will be set to some empty
         * ov_http_message struct, so we check for empty msg here */

        if (msg) {

            if (0 != msg->buffer->start[0])
                goto error;

            self->data = ov_http_message_free(self->data);
        }

        frame = ov_websocket_frame_cast(self->data);
        if (!frame) {

            OV_ASSERT(NULL == self->data);

            if (self->data)
                goto error;

            frame = ov_websocket_frame_create(config.websocket_frame);
            if (!frame)
                goto error;

            self->data = frame;
        }

        OV_ASSERT(frame->buffer);
        buffer = frame->buffer;
        break;
    };

    OV_ASSERT(buffer);
    if (!buffer || !buffer->start)
        goto error;

    /* Perform PEEK read to adapt buffer size of required */

    size_t used = buffer->length;
    ssize_t open = buffer->capacity - used;

    ssize_t bytes = SSL_peek(self->tls.ssl, buf, OV_SSL_MAX_BUFFER - 1);
    if (bytes <= 0) {

        n = SSL_get_error(self->tls.ssl, bytes);

        switch (n) {
        case SSL_ERROR_NONE:
        case SSL_ERROR_WANT_READ:
        case SSL_ERROR_WANT_WRITE:
        case SSL_ERROR_WANT_CONNECT:
        case SSL_ERROR_WANT_ACCEPT:
        case SSL_ERROR_WANT_X509_LOOKUP:
            goto retry;
            break;

        case SSL_ERROR_ZERO_RETURN:
            // connection close
            goto error;
            break;

        case SSL_ERROR_SYSCALL:

            goto error;
            break;

        case SSL_ERROR_SSL:

            self->flags = 0;
            goto error;
            break;

        default:
            goto error;
            break;
        }
    }

    if (bytes > open) {

        if (!ov_buffer_extend(buffer, bytes - open + 1))
            goto error;
    }

    open = buffer->capacity - used;
    uint8_t *ptr = buffer->start + used;

    /*
     *  Perform SSL read
     */

    bytes = SSL_read(self->tls.ssl, ptr, open);

    if (bytes > 0) {

        self->io.in.last = ov_time_get_current_time_usecs();
        self->io.in.bytes += bytes;
        buffer->length += bytes;

        if (!process_tls_io(self))
            goto error;

    } else if (bytes == 0) {

        goto error;

    } else {

        n = SSL_get_error(self->tls.ssl, bytes);

        switch (n) {
        case SSL_ERROR_NONE:
        case SSL_ERROR_WANT_READ:
        case SSL_ERROR_WANT_WRITE:
        case SSL_ERROR_WANT_CONNECT:
        case SSL_ERROR_WANT_ACCEPT:
        case SSL_ERROR_WANT_X509_LOOKUP:
            break;

        case SSL_ERROR_ZERO_RETURN:
            // connection close
            goto error;
            break;

        case SSL_ERROR_SYSCALL:

            goto error;
            break;

        case SSL_ERROR_SSL:
            self->flags = 0;
            goto error;
            break;

        default:
            goto error;
            break;
        }
    }

retry:
    /* Try to read again */
    self->flags = 0;
    return true;

process_out:

    if (events & OV_EVENT_IO_OUT) {

        /* Try to send next data of outgoing queue,
         * in case of error close connection. */

        if (!io_tls_send(self))
            goto error;

        /* use next eventloop cycle to process next event
         * (outgoing priorisation)*/

        return true;
    }

error:
    if (self)
        ov_web_server_close_connection(self->server, self->socket);
    return false;
}

/*----------------------------------------------------------------------------*/

ov_web_server_connection *ov_web_server_connection_create(ov_web_server *srv,
                                                          int socket) {

    if (!srv)
        return NULL;

    ov_web_server_connection *self =
        calloc(1, sizeof(ov_web_server_connection));
    if (!self)
        goto error;

    self->magic_byte = OV_WEB_SERVER_CONNECTION_MAGIC_BYTE;
    self->server = srv;
    self->socket = socket;
    self->type = CONNECTION_TYPE_HTTP_1_1;
    self->tls.handshaked = false;
    self->created = ov_time_get_current_time_usecs();

    ov_socket_get_data(self->socket, NULL, &self->remote);

    ov_web_server_config config = ov_web_server_get_config(self->server);
    ov_event_loop *loop = config.loop;
    if (!loop)
        goto error;

    if (!ov_event_loop_set(loop, self->socket,
                           OV_EVENT_IO_IN | OV_EVENT_IO_ERR | OV_EVENT_IO_CLOSE,
                           self, io_tls))
        goto error;

    return self;
error:
    ov_web_server_connection_free(self);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_web_server_connection *ov_web_server_connection_cast(const void *self) {

    if (!self)
        goto error;

    if (*(uint16_t *)self == OV_WEB_SERVER_CONNECTION_MAGIC_BYTE)
        return (ov_web_server_connection *)self;
error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

static bool close_socket_in_event_handler(const void *key, void *item,
                                          void *data) {

    if (!key)
        return true;

    ov_event_io_config *handler = (ov_event_io_config *)item;

    if (!handler || !data)
        return false;

    int socket = *(int *)data;

    if (handler->callback.close)
        handler->callback.close(handler->userdata, socket);

    return true;
}

/*----------------------------------------------------------------------------*/

void *ov_web_server_connection_free(void *userdata) {

    ov_web_server_connection *self = ov_web_server_connection_cast(userdata);
    if (!self)
        return userdata;

    int socket = self->socket;

    if (self->flags & FLAG_CLIENT_SHUTDOWN)
        goto close_without_websocket_shutdown;

    ov_web_server_config config = ov_web_server_get_config(self->server);
    ov_event_loop *loop = config.loop;

    /* Websocket shutdown implementation.
     *
     * We send some Websocket shutdown frame and before we TLS shutdown the
     * socket anyway, so that the client may receive the websocket shutdown
     * for some protocol conform closure of the websocket layer.
     */

    if (self->tls.ssl && (self->type == CONNECTION_TYPE_WEBSOCKET) &&
        (!self->websocket.close.send)) {

        /* By default we set 1000 "close" as message to the client */
        if (0 == self->websocket.close.code) {
            self->websocket.close.code = 1000;
            const char *phrase = "normal close";
            strncpy(self->websocket.close.phrase, phrase, strlen(phrase));
        }

        // send close frame

        uint8_t out[IMPL_WEBSOCKET_PHRASE_LENGTH + 5] = {0};
        ssize_t clen = strlen(self->websocket.close.phrase);

        out[0] = 0x88;
        out[1] = clen + 2;
        out[2] = self->websocket.close.code >> 8;
        out[3] = self->websocket.close.code;
        memcpy(out + 4, self->websocket.close.phrase, clen);

        ssize_t bytes = SSL_write(self->tls.ssl, out, clen + 4);

        if (bytes != clen + 4)
            goto close_without_websocket_shutdown;

        self->websocket.close.send = true;
    }

close_without_websocket_shutdown:

    loop->callback.unset(loop, self->socket, NULL);

    if (self->tls.ssl) {

        if (self->flags & FLAG_SEND_SSL_SHUTDOWN)
            SSL_shutdown(self->tls.ssl);

        SSL_free(self->tls.ssl);
        self->tls.ssl = NULL;
    }

    close(socket);

    self->socket = -1;

    ConnectionType type = self->type;

    if ((type == CONNECTION_TYPE_WEBSOCKET) && self->websocket.config.close) {

        self->websocket.config.close(self->websocket.config.userdata, socket);
    }

    if (self->domain && self->domain->event_handler.uri) {

        ov_dict_for_each(self->domain->event_handler.uri, &socket,
                         close_socket_in_event_handler);
    }

    // drop all outgoing
    self->io.out.buffer = ov_buffer_free(self->io.out.buffer);

    // drop all format sending
    /*
        conn->io.out.fmt.format = ov_format_close(conn->io.out.fmt.format);
        conn->io.out.fmt.msg = ov_http_message_free(conn->io.out.fmt.msg);
        conn->io.out.size = 0;
        conn->io.out.fmt.start = false;
    */
    void *data = ov_list_queue_pop(self->io.out.queue);
    while (data) {

        data = ov_http_message_free(data);
        data = ov_websocket_frame_free(data);
        data = ov_buffer_free(data);

        if (data) {
            TODO(" ... add cleanup for queue data!");
        }

        OV_ASSERT(!data);
        data = ov_list_queue_pop(self->io.out.queue);
    };

    // ensure to free all allocations
    self->data = ov_http_message_free(self->data);
    self->data = ov_websocket_frame_free(self->data);
    self->data = ov_buffer_free(self->data);

    if (config.callback.close)
        config.callback.close(config.callback.userdata, socket);

    self = ov_data_pointer_free(self);
    return NULL;
}

/*----------------------------------------------------------------------------*/

bool ov_web_server_connection_set_ssl(ov_web_server_connection *self,
                                      SSL *ssl) {

    if (!self)
        goto error;

    if (self->tls.ssl) {

        SSL_free(self->tls.ssl);
    }

    self->tls.ssl = ssl;

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_web_server_connection_send(ov_web_server_connection *self,
                                   const ov_buffer *data) {

    if (!self || !data)
        goto error;

    return tls_send_buffer(self, data);
error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_web_server_connection_uri_file_path(ov_web_server_connection *self,
                                            const ov_http_message *msg,
                                            size_t len, char *path) {

    ov_uri *uri = NULL;

    if (!self || !msg || !path)
        goto error;

    /* create PATH based on document root and uri request */

    uri = ov_uri_from_string((char *)msg->request.uri.start,
                             msg->request.uri.length);

    if (!uri || !uri->path)
        goto error;

    if (len < PATH_MAX)
        goto error;

    /* clean empty AND dot paths in uri path */

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

    ssize_t bytes = snprintf(full_path, PATH_MAX, "%s/%s",
                             self->domain->config.path, cleaned_path);

    if (bytes < 1)
        goto error;

    if (!ov_uri_path_remove_dot_segments(full_path, path))
        goto error;

    if ('/' == path[strlen(path) - 1]) {

        if (PATH_MAX - bytes < 12)
            goto error;

        strcat(path, "index.html");
    }

    uri = ov_uri_free(uri);
    return true;
error:
    uri = ov_uri_free(uri);
    return false;
}

/*----------------------------------------------------------------------------*/

ov_domain *ov_web_server_connection_get_domain(ov_web_server_connection *self) {

    if (!self)
        return NULL;
    return self->domain;
}