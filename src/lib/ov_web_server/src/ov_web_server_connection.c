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

#include <ov_base/ov_time.h>

#define OV_WEB_SERVER_CONNECTION_MAGIC_BYTE 0xffe1

#define FLAG_CLOSE_AFTER_SEND 0x0001
#define FLAG_SEND_SSL_SHUTDOWN 0x0010
#define FLAG_CLIENT_SHUTDOWN 0x0100

/*----------------------------------------------------------------------------*/

typedef enum ConnectionType {

    CONNECTION_TYPE_HTTP_1_1 = 0,
    CONNECTION_TYPE_WEBSOCKET = 1

} ConnectionType;

/*----------------------------------------------------------------------------*/

struct ov_web_server_connection{

    uint16_t magic_byte;
    ov_web_server *server;

    int socket;

    uint16_t flags;
    uint64_t created;

    ConnectionType type;

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

static bool io_tls(int socket_fd, uint8_t events, void *data) {

    char errorstring[OV_SSL_ERROR_STRING_BUFFER_SIZE] = {0};
    int errorcode = -1, n = 0;

    char buf[OV_SSL_MAX_BUFFER] = {0};

    ov_web_server_connection *self = ov_web_server_connection_cast(data);
    
    OV_ASSERT(self);
    OV_ASSERT(self->socket == socket_fd);

    ov_web_server_config config = ov_web_server_get_config(self->server);
    ov_event_loop *loop = config.loop;

    if ((events & OV_EVENT_IO_CLOSE) || (events & OV_EVENT_IO_ERR)) {

        /* Ensure SSL_shutdown will not be triggered on client close */
        conn->flags = 0;
        conn->flags = FLAG_CLIENT_SHUTDOWN;
        close_connection(srv, srv->config.loop, &srv->conn[socket_fd]);
        return true;
    }

    if (!conn->tls.handshaked) return tls_perform_handshake(srv, conn);

    if (!(events & OV_EVENT_IO_IN)) goto process_out;

    /* We do have some new IO to process */

    /*
     *  Prepare buffer for READ
     */

    ov_buffer *buffer = NULL;
    ov_http_message *msg = NULL;
    ov_websocket_frame *frame = NULL;

    conn->flags = FLAG_SEND_SSL_SHUTDOWN;

    msg = ov_http_message_cast(conn->data);

    switch (conn->type) {

        case CONNECTION_TYPE_ERROR:
            goto error;

        case CONNECTION_TYPE_HTTP_1_1:

            if (!msg) {

                OV_ASSERT(NULL == conn->data);

                if (conn->data) goto error;

                msg = ov_http_message_create(srv->config.http_message);
                if (!msg) goto error;

                conn->data = msg;
            }

            OV_ASSERT(msg->buffer);
            buffer = msg->buffer;
            break;

        case CONNECTION_TYPE_WEBSOCKET:

            /* after upgrade success the data will be set to some empty
             * ov_http_message struct, so we check for empty msg here */

            if (msg) {

                if (0 != msg->buffer->start[0]) goto error;

                conn->data = ov_http_message_free(conn->data);
            }

            frame = ov_websocket_frame_cast(conn->data);
            if (!frame) {

                OV_ASSERT(NULL == conn->data);

                if (conn->data) goto error;

                frame = ov_websocket_frame_create(srv->config.websocket_frame);
                if (!frame) goto error;

                conn->data = frame;
            }

            OV_ASSERT(frame->buffer);
            buffer = frame->buffer;
            break;
    };

    OV_ASSERT(buffer);
    if (!buffer || !buffer->start) goto error;

    /* Perform PEEK read to adapt buffer size of required */

    size_t used = buffer->length;
    ssize_t open = buffer->capacity - used;

    ssize_t bytes = SSL_peek(conn->tls.ssl, buf, OV_SSL_MAX_BUFFER - 1);
    if (bytes <= 0) {

        n = SSL_get_error(conn->tls.ssl, bytes);

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

                ov_log_error(
                    "%s SSL_ERROR_SYSCALL"
                    "%d | %s",
                    srv->config.name,
                    errno,
                    strerror(errno));

                goto error;
                break;

            case SSL_ERROR_SSL:

                errorcode = ERR_get_error();
                ERR_error_string_n(
                    errorcode, errorstring, OV_SSL_ERROR_STRING_BUFFER_SIZE);
                ov_log_error("%s SSL_ERROR_SSL %s at socket %i",
                             srv->config.name,
                             errorstring,
                             conn->fd);

                conn->flags = 0;
                goto error;
                break;

            default:
                goto error;
                break;
        }
    }

    if (bytes > open) {

        if (!ov_buffer_extend(buffer, bytes - open + 1)) goto error;
    }

    open = buffer->capacity - used;
    uint8_t *ptr = buffer->start + used;

    /*
     *  Perform SSL read
     */

    bytes = SSL_read(conn->tls.ssl, ptr, open);

    if (bytes > 0) {

        conn->io.in.last = ov_time_get_current_time_usecs();
        conn->io.in.bytes += bytes;
        buffer->length += bytes;

        if (!process_tls_io(srv, conn)) goto error;

    } else if (bytes == 0) {

        goto error;

    } else {

        n = SSL_get_error(conn->tls.ssl, bytes);

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

                ov_log_error(
                    "%s SSL_ERROR_SYSCALL"
                    "%d | %s",
                    srv->config.name,
                    errno,
                    strerror(errno));

                goto error;
                break;

            case SSL_ERROR_SSL:

                errorcode = ERR_get_error();
                ERR_error_string_n(
                    errorcode, errorstring, OV_SSL_ERROR_STRING_BUFFER_SIZE);
                ov_log_error("%s SSL_ERROR_SSL %s at socket %i",
                             srv->config.name,
                             errorstring,
                             conn->fd);

                conn->flags = 0;
                goto error;
                break;

            default:
                goto error;
                break;
        }
    }

retry:
    /* Try to read again */
    conn->flags = 0;
    return true;

process_out:

    if (events & OV_EVENT_IO_OUT) {

        /* Try to send next data of outgoing queue,
         * in case of error close connection. */

        if (!io_tls_send(srv, conn, loop)) goto error;

        /* use next eventloop cycle to process next event
         * (outgoing priorisation)*/

        return true;
    }

error:
    if (srv && conn) close_connection(srv, srv->config.loop, conn);
    return false;
}

/*----------------------------------------------------------------------------*/

ov_web_server_connection *ov_web_server_connection_create(ov_web_server *srv, int socket){

    if (!srv) return NULL;

    ov_web_server_connection *self = calloc(1, sizeof(ov_web_server_connection));
    if (!self) goto error;

    self->magic_byte = OV_WEB_SERVER_CONNECTION_MAGIC_BYTE;
    self->server = srv;
    self->socket = socket;
    self->type = CONNECTION_TYPE_HTTP_1_1;
    self->tls.handshaked = false;
    self->created = ov_time_get_current_time_usecs();

    ov_socket_get_data(self->socket, NULL, &self->remote);

    ov_web_server_config config = ov_web_server_get_config(self->server);
    ov_event_loop *loop = config.loop;
    if (!loop) goto error;

    if (!ov_event_loop_set(loop,
        self->socket,
        OV_EVENT_IO_IN | OV_EVENT_IO_ERR | OV_EVENT_IO_CLOSE,
        self,
        io_tls)) goto error;

    return self;
error:
    ov_web_server_connection_free(self);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_web_server_connection *ov_web_server_connection_cast(const void *self){

    if (!self) goto error;

    if (*(uint16_t *)self == OV_WEB_SERVER_CONNECTION_MAGIC_BYTE)
        return (ov_web_server_connection*)self;
error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

void *ov_web_server_connection_free(void *self){

    ov_web_server_connection *conn = ov_web_server_connection_cast(self);
    if (!conn) return self;

    conn = ov_data_pointer_free(conn);
    return NULL;
}

/*----------------------------------------------------------------------------*/

bool ov_web_server_connection_set_ssl(ov_web_server_connection *self, SSL *ssl){

    if (!self) goto error;

    if (self->tls.ssl){

        SSL_free(self->tls.ssl);
    
    }

    self->tls.ssl = ssl;

    return true;
error:
    return false;
}