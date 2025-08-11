/***
        ------------------------------------------------------------------------

        Copyright (c) 2020 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_webserver_base.c
        @author         Markus Töpfer

        @date           2020-12-07


        ------------------------------------------------------------------------
*/
#include "../include/ov_webserver_base.h"
#include "../include/ov_domain.h"
#include "../include/ov_http_pointer.h"
#include "../include/ov_websocket_pointer.h"

#include <ov_stun/ov_stun_attribute.h>
#include <ov_stun/ov_stun_binding.h>
#include <ov_stun/ov_stun_frame.h>

#include <openssl/conf.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/ssl.h>

#include <ov_base/ov_utils.h>
#include <strings.h>
#include <sys/resource.h>
#include <sys/time.h>

#include <ov_base/ov_buffer.h>
#include <ov_base/ov_constants.h>
#include <ov_base/ov_dump.h>
#include <ov_base/ov_file.h>
#include <ov_base/ov_linked_list.h>
#include <ov_base/ov_string.h>
#include <ov_base/ov_uri.h>
#include <ov_base/ov_utils.h>

#include <signal.h>

/*
 *      ------------------------------------------------------------------------
 *
 *      #INIT
 *
 *      ------------------------------------------------------------------------
 */

#define IMPL_TYPE 0x0001
#define IMPL_DEFAULT_WEB_SOCKETS 4
#define IMPL_MAX_STUN_ATTRIBUTES 5
#define IMPL_DEFAULT_DOMAINS 10
#define IMPL_MAX_FRAMES_FOR_JSON 10000
#define IMPL_WEBSOCKET_PHRASE_LENGTH 250

#define OV_SSL_MAX_BUFFER 16000 // 16kb SSL buffer max
#define OV_SSL_ERROR_STRING_BUFFER_SIZE 200

#define IMPL_DEFAULT_ACCEPT_TO_IO_USEC 1000 * 1000      // 1 second
#define IMPL_WEBSOCKET_CLOSE_RESPONSE_DELAY 1000 * 1000 // 1 second
#define IMPL_BUFFER_SIZE_WS_FRAME_DEFRAG 1000000        // 1MB

/* We use some rather small buffer size for websocket frames
 * to reduce delay for sending due to "large" socket writes,
 * we callback anyway as long as some data is available to
 * send, so a smaller size will increase multiplexing of
 * different IO sockets by default. */
#define IMPL_CONTENT_SIZE_WS_FRAME_DEFAULT 500

typedef struct ov_webserver_base ov_webserver_base;

#define FLAG_CLOSE_AFTER_SEND 0x0001
#define FLAG_SEND_SSL_SHUTDOWN 0x0010
#define FLAG_CLIENT_SHUTDOWN 0x0100

typedef enum ConnectionType {

    CONNECTION_TYPE_ERROR = 0,
    CONNECTION_TYPE_WEBSOCKET = 1,
    CONNECTION_TYPE_HTTP_1_1 = 2

} ConnectionType;

/*----------------------------------------------------------------------------*/

const char *connection_type_string(ConnectionType type) {

    switch (type) {

        case CONNECTION_TYPE_WEBSOCKET:
            return "websocket";

        case CONNECTION_TYPE_HTTP_1_1:
            return "http";

        default:
            break;
    }

    return "unknown";
}

/*----------------------------------------------------------------------------*/

typedef struct Connection {

    int fd;
    int listener;

    uint16_t flags;
    uint64_t created;

    ov_domain *domain;

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

        /*  We need to persist the callback here to set and unset
         *  outgoing reediness listening.
         *
         *  The eventloop interface allows to change events & callback,
         *  but not event listening for some callback */

        bool (*callback)(int, uint8_t, void *);

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
            /*
                        struct {

                            uint64_t chunks;
                            bool start;
                            ov_format *format;
                            ov_http_message *msg;

                        } fmt;
            */
            uint64_t bytes; // counter outgoing bytes
            uint64_t last;  // time of last seen outgoing io

            uint64_t size; // send buffer size outgoing

        } out;

        struct {

            uint64_t bytes; // counter incoming bytes
            uint64_t last;  // time of last seen incoming io

        } in;

    } io;

    ov_webserver_base *srv;

} Connection;

/*----------------------------------------------------------------------------*/

struct ov_webserver_base {

    const uint16_t magic_byte;
    ov_webserver_base_config config;

    // private connections max of allocated connections
    uint32_t connections_max;

    struct {

        int https;
        int http;

        int stun[OV_WEBSERVER_BASE_MAX_STUN_LISTENER];

    } sockets;

    struct {

        uint32_t io_timeout_check;

    } timer;

    struct {

        ssize_t default_domain;
        size_t size;
        ov_domain *array;

    } domain;

    struct Connection *conn;
};

/*----------------------------------------------------------------------------*/

static ov_webserver_base init = {

    .magic_byte = OV_WEBSERVER_BASE_MAGIC_BYTE

};

/*----------------------------------------------------------------------------*/

static ov_webserver_base *ov_webserver_base_cast(const void *self) {

    /* used internal as we have void* from time to time e.g. for SSL */

    if (!self) goto error;

    if (*(uint16_t *)self == OV_WEBSERVER_BASE_MAGIC_BYTE)
        return (ov_webserver_base *)self;
error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

static const uint8_t EVENTS =
    OV_EVENT_IO_IN | OV_EVENT_IO_ERR | OV_EVENT_IO_CLOSE;

/*
 *      ------------------------------------------------------------------------
 *
 *      #TLS
 *
 *      ------------------------------------------------------------------------
 */

static int tls_client_hello_callback(SSL *ssl, int *al, void *arg) {

    ov_webserver_base *srv = ov_webserver_base_cast(arg);
    if (!srv) goto error;

    /*
            Example from dump of browser request to https://openvocs.test:12345

             0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F  10 20
                17       13 o  p  e  n  v  o  c  s  .  t  e  s  t
             00 10 00 00 0d 6f 70 65 6e 76 6f 63 73 2e 74 65 73 74
             ^  ^     ^  ^  ^
             |  |     |  |  content ( == openvocs.test )
             |  |     |  length ( 8 bit == 13 )
             |  |     name type ( 0 == hostname )
             |  length ( 16 bit == 17 )
             type ( 8 bit SNI == 0 )

             0                   1                   2                   3
             0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
            +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
            |     TYPE      |         List Length             |   NAME-TYPE |
            +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
            | ContentLength |      Content (variable length 0 .. 255)       |
            +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
            | next NAME-TYPE| ContentLength |         next Content          |
            +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

            NOTE SSL_client_hello_get0_ext seems to deliver the length in
            little endian on Intel (macOS) (which is host byte order) TBC
            RFC 8446 states the byte order is network byte order.

            For our use case this may be important if more than one Servername
            is included, but we do not support this yet, so we do not check
            the whole buffer, but the first element of the Servernamelist.

            Here the length is given in ONE byte at start[4]
    */

    const unsigned char *str = NULL;
    size_t str_len = 0;

    ov_domain *domain = NULL;

    if (0 == SSL_client_hello_get0_ext(
                 ssl, TLSEXT_NAMETYPE_host_name, &str, &str_len)) {

        /* In case the header extension for host name is NOT set,
         * the default certificate of the configured domains will be used.
         *
         * This will allow access to e.g. https://127.0.0.1:12345 with some
         * certificate response for local tests or in general IP address based
         * access to the default domain of some webserver. */

        if (-1 == srv->domain.default_domain) {
            domain = &srv->domain.array[0];
        } else {
            domain = &srv->domain.array[srv->domain.default_domain];
        }

    } else {

        /* Perform SNI */

        OV_ASSERT(str);
        OV_ASSERT(0 < str_len);

        /* min valid length without chars for hostname content */
        if (4 >= str_len) goto error;

        /* check if the first entry is host name */
        if (0 != str[3]) goto error;

        size_t hostname_length = str[4];

        if (str_len < hostname_length + 5) goto error;

        uint8_t *hostname = (uint8_t *)str + 5;

        for (size_t i = 0; i < srv->domain.size; i++) {

            if (srv->domain.array[i].config.name.length != hostname_length)
                continue;

            if (0 == memcmp(hostname,
                            srv->domain.array[i].config.name.start,
                            hostname_length)) {
                domain = &srv->domain.array[i];
                break;
            }
        }

        /*  If some SNI is present and the domain is not enabled.
         *  the handshake will be stopped. */

        if (!domain) goto error;
    }

    OV_ASSERT(domain);

    if (!SSL_set_SSL_CTX(ssl, domain->context.tls)) {
        goto error;
    }

    return SSL_CLIENT_HELLO_SUCCESS;

error:
    if (al) *al = SSL_TLSEXT_ERR_ALERT_FATAL;
    return SSL_CLIENT_HELLO_ERROR;
}

/*----------------------------------------------------------------------------*/

static bool tls_init_context(ov_webserver_base *srv, ov_domain *domain) {

    if (!srv || !domain) goto error;

    if (!domain->context.tls) {
        ov_log_error("%s TLS context for domain |%s| not set",
                     srv->config.name,
                     domain->config.name);
        goto error;
    }

    SSL_CTX_set_client_hello_cb(
        domain->context.tls, tls_client_hello_callback, srv);

    return true;
error:
    return false;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      #CONNNECTION
 *
 *      ------------------------------------------------------------------------
 */

static bool close_socket_in_event_handler(const void *key,
                                          void *item,
                                          void *data) {

    if (!key) return true;

    ov_event_io_config *handler = (ov_event_io_config *)item;

    if (!handler || !data) return false;

    int socket = *(int *)data;

    if (handler->callback.close)
        handler->callback.close(handler->userdata, socket);

    return true;
}

/*----------------------------------------------------------------------------*/

static void close_connection(ov_webserver_base *srv,
                             ov_event_loop *loop,
                             struct Connection *conn) {

    OV_ASSERT(srv);
    OV_ASSERT(loop);
    OV_ASSERT(conn);

    int socket = conn->fd;

    if (-1 == socket) return;

    if (conn->flags & FLAG_CLIENT_SHUTDOWN)
        goto close_without_websocket_shutdown;

    /* Websocket shutdown implementation.
     *
     * We send some Websocket shutdown frame and before we TLS shutdown the
     * socket anyway, so that the client may receive the websocket shutdown
     * for some protocol conform closure of the websocket layer.
     */

    if (conn->tls.ssl && (conn->type == CONNECTION_TYPE_WEBSOCKET) &&
        (!conn->websocket.close.send)) {

        /* By default we set 1000 "close" as message to the client */
        if (0 == conn->websocket.close.code) {
            conn->websocket.close.code = 1000;
            const char *phrase = "normal close";
            strncpy(conn->websocket.close.phrase, phrase, strlen(phrase));
        }

        // send close frame

        uint8_t out[IMPL_WEBSOCKET_PHRASE_LENGTH + 5] = {0};
        ssize_t clen = strlen(conn->websocket.close.phrase);

        out[0] = 0x88;
        out[1] = clen + 2;
        out[2] = conn->websocket.close.code >> 8;
        out[3] = conn->websocket.close.code;
        memcpy(out + 4, conn->websocket.close.phrase, clen);

        ssize_t bytes = SSL_write(conn->tls.ssl, out, clen + 4);

        if (bytes != clen + 4) goto close_without_websocket_shutdown;

        conn->websocket.close.send = true;
    }

close_without_websocket_shutdown:

    if (!loop->callback.unset(loop, conn->fd, NULL))
        ov_log_error(
            "%s failed to unset connection IO "
            "callback at %i",
            srv->config.name,
            conn->fd);

    if (conn->tls.ssl) {

        if (conn->flags & FLAG_SEND_SSL_SHUTDOWN) SSL_shutdown(conn->tls.ssl);

        SSL_free(conn->tls.ssl);
        conn->tls.ssl = NULL;
    }

    close(socket);

    conn->fd = -1;
    conn->listener = -1;

    ConnectionType type = conn->type;

    if ((type == CONNECTION_TYPE_WEBSOCKET) && conn->websocket.config.close) {

        conn->websocket.config.close(conn->websocket.config.userdata, socket);
    }

    if (conn->domain && conn->domain->event_handler.uri) {

        ov_dict_for_each(conn->domain->event_handler.uri,
                         &socket,
                         close_socket_in_event_handler);
    }

    conn->websocket.config = (ov_websocket_message_config){0};
    conn->websocket.queue = ov_list_free(conn->websocket.queue);
    conn->websocket.last = OV_WEBSOCKET_FRAGMENTATION_NONE;
    conn->websocket.counter = 0;
    conn->type = CONNECTION_TYPE_ERROR;
    conn->websocket.close.send = false;
    conn->websocket.close.recv = false;
    conn->websocket.close.wait_for_response = 0;
    memset(conn->websocket.close.phrase, 0, IMPL_WEBSOCKET_PHRASE_LENGTH);
    conn->websocket.close.code = 0;

    // drop all outgoing
    conn->io.out.buffer = ov_buffer_free(conn->io.out.buffer);

    // drop all format sending
    /*
        conn->io.out.fmt.format = ov_format_close(conn->io.out.fmt.format);
        conn->io.out.fmt.msg = ov_http_message_free(conn->io.out.fmt.msg);
        conn->io.out.size = 0;
        conn->io.out.fmt.start = false;
    */
    void *data = ov_list_queue_pop(conn->io.out.queue);
    while (data) {

        data = ov_http_message_free(data);
        data = ov_websocket_frame_free(data);
        data = ov_buffer_free(data);

        if (data) {
            TODO(" ... add cleanup for queue data!");
        }

        OV_ASSERT(!data);
        data = ov_list_queue_pop(conn->io.out.queue);
    };

    // ensure to free all allocations
    conn->data = ov_http_message_free(conn->data);
    conn->data = ov_websocket_frame_free(conn->data);
    conn->data = ov_buffer_free(conn->data);

    // ov_log_debug("%s closed socket %i ", srv->config.name, socket);

    if (srv->config.callback.close)
        srv->config.callback.close(srv->config.callback.userdata, socket);

    return;
}

/*----------------------------------------------------------------------------*/

static bool close_tcp_listener(ov_webserver_base *srv, int socket_fd) {

    if (!srv) goto error;

    ov_event_loop *loop = srv->config.loop;

    if (!loop->callback.unset(loop, socket_fd, NULL))
        ov_log_error(
            "%s failed to unset accept "
            "callback",
            srv->config.name);

    if (srv->sockets.http == socket_fd) srv->sockets.http = -1;

    if (srv->sockets.https == socket_fd) srv->sockets.https = -1;

    for (size_t i = 0; i < srv->connections_max; i++) {

        if (srv->conn[i].listener != socket_fd) continue;

        close_connection(srv, loop, &srv->conn[i]);
    }
    return true;
error:
    return false;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      #IO
 *
 *      ------------------------------------------------------------------------
 */

static bool push_outgoing(Connection *conn, void *data) {

    if (!conn || !data) goto error;

    ov_webserver_base *srv = conn->srv;
    OV_ASSERT(srv);
    ov_event_loop *loop = srv->config.loop;

    OV_ASSERT(loop);
    OV_ASSERT(loop->callback.set);
    OV_ASSERT(conn->io.callback);
    OV_ASSERT(conn->io.out.queue);

    /* Push data to outgoing queue */
    if (!ov_list_queue_push(conn->io.out.queue, data)) goto error;

    /* Enable outgoing readiness listening */
    if (!loop->callback.set(loop,
                            conn->fd,
                            OV_EVENT_IO_IN | OV_EVENT_IO_ERR |
                                OV_EVENT_IO_CLOSE | OV_EVENT_IO_OUT,
                            srv,
                            conn->io.callback))
        goto error;

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool create_redirect_reply(ov_webserver_base *srv,
                                  Connection *conn,
                                  ov_http_message *msg) {

    ov_http_message *response = NULL;

    OV_ASSERT(srv);
    OV_ASSERT(conn);
    OV_ASSERT(msg);

    /* STATE msg is unplugged from conn and parsed as valid HTTP message. */
    OV_ASSERT(conn->data != msg);

    /* We don't expect a STATUS message here, so close if message is status */
    if (0 != msg->status.code) goto error;

    size_t index = 0;

    const ov_http_header *host = ov_http_header_get_next(
        msg->header, msg->config.header.capacity, &index, OV_HTTP_KEY_HOST);

    if (!host) goto error;

    // more than one host header?
    if (ov_http_header_get_next(
            msg->header, msg->config.header.capacity, &index, OV_HTTP_KEY_HOST))
        goto error;

    /* Focus of this function is a redirect to the SSL instance,
     * so we take the HOST Header field and respond with location set within
     * this field, and do NOT evaluate the request.uri, as this SHOULD be
     * done, when the actuall request will be checked.
     *
     * This functions only use case is to redirect non secure traffic to the
     * secure port
     */

    response = ov_http_create_status_string(
        msg->config, msg->version, 301, OV_HTTP_MOVED_PERMANENTLY);

    if (!response) goto error;

    uint8_t *colon = memchr(host->value.start, ':', host->value.length);

    char location[PATH_MAX] = {0};
    ssize_t bytes = 0;

    if (colon) {
        bytes = snprintf(location,
                         PATH_MAX,
                         "https://%.*s:%i",
                         (int)(colon - host->value.start),
                         host->value.start,
                         srv->config.http.secure.port);

    } else {
        bytes = snprintf(location,
                         PATH_MAX,
                         "https://%.*s:%i",
                         (int)host->value.length,
                         host->value.start,
                         srv->config.http.secure.port);
    }

    if (bytes == -1) goto error;

    if (bytes == PATH_MAX) goto error;

    if (!ov_uri_string_is_valid(location, strlen(location))) goto error;

    if (!ov_http_message_add_header_string(
            response, OV_HTTP_KEY_LOCATION, location))
        goto error;

    if (!ov_http_message_close_header(response)) goto error;

    /*
     *  We try to sent the message inline to avoid any additional
     *  processing in case of success.
     */

    bytes =
        send(conn->fd, response->buffer->start, response->buffer->length, 0);

    if ((bytes < 0) && (EAGAIN != errno)) {
        close_connection(srv, srv->config.loop, conn);
        goto error;
    }

    if ((-1 == bytes) || ((bytes < 0) && (EAGAIN == errno))) {

        /* Direct send not successfull, push to outgoing queue */

        conn->flags = FLAG_CLOSE_AFTER_SEND;
        if (!push_outgoing(conn, response)) goto error;

        response = NULL;

    } else if (bytes < (ssize_t)response->buffer->length) {

        conn->io.out.bytes += bytes;

        /* Just some bytes of the message are sent */

        if (!conn->io.out.buffer) {

            conn->io.out.buffer = response->buffer;
            response->buffer = NULL;

            /* This is a realloc shift! */
            if (!ov_buffer_shift(
                    conn->io.out.buffer, conn->io.out.buffer->start + bytes))
                goto error;

        } else {

            /* This is a copy push! */
            if (!ov_buffer_push(conn->io.out.buffer,
                                response->buffer->start + bytes,
                                response->buffer->length - bytes))
                goto error;
        }

    } else if (bytes > (ssize_t)response->buffer->length) {

        /* More bytes sent as contained? */
        conn->io.out.bytes += bytes;
        goto error;

    } else {

        /* Whole message sent */
        conn->io.out.bytes += bytes;
    }

    if (srv->config.debug)
        ov_log_debug("%s redirect at %i to %s (%i bytes sent)",
                     srv->config.name,
                     conn->fd,
                     location,
                     bytes);

    /* At this point we have either sent the response,
     * or queued the response outgoing at the connection. */

    response = ov_http_message_free(response);
    msg = ov_http_message_free(msg);
    return true;
error:
    msg = ov_http_message_free(msg);
    response = ov_http_message_free(response);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool process_tcp_redirect(ov_webserver_base *srv, Connection *conn) {

    OV_ASSERT(srv);
    OV_ASSERT(conn);

    if (!srv || !conn) goto error;

    OV_ASSERT(conn->data);

    uint8_t *next = NULL;
    ov_http_parser_state state = OV_HTTP_PARSER_ERROR;

    ov_http_message *msg = ov_http_message_cast(conn->data);

    OV_ASSERT(msg);
    if (!msg) goto error;

    while (msg) {

        if (msg->buffer->start[0] == 0) goto done;

        state = ov_http_pointer_parse_message(msg, &next);

        switch (state) {

            case OV_HTTP_PARSER_SUCCESS:

                conn->data = NULL;

                if (!ov_http_message_shift_trailing_bytes(
                        msg, next, (ov_http_message **)&conn->data)) {
                    msg = ov_http_message_free(msg);
                    goto error;
                }

                if (!create_redirect_reply(srv, conn, msg)) goto error;

                msg = conn->data;
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

static bool tls_send_buffer(ov_webserver_base *srv,
                            Connection *conn,
                            ov_buffer const *const buffer) {
    OV_ASSERT(srv);
    OV_ASSERT(conn);
    OV_ASSERT(buffer);

    ov_buffer *copy = NULL;

    char errorstring[OV_SSL_ERROR_STRING_BUFFER_SIZE];
    int errorcode = -1;

    if (!srv || !conn || !buffer) goto error;

    conn->flags = FLAG_SEND_SSL_SHUTDOWN;

    if (!conn->tls.handshaked) goto error;

    ov_event_loop *loop = srv->config.loop;
    OV_ASSERT(loop);

    /* Ensure outgoing readiness listening */

    if (!loop->callback.set(loop,
                            conn->fd,
                            OV_EVENT_IO_IN | OV_EVENT_IO_ERR |
                                OV_EVENT_IO_CLOSE | OV_EVENT_IO_OUT,
                            srv,
                            conn->io.callback))
        goto error;

    /*
     *  Try to send the data, or push the data to the outgoing buffer if
     *  a direct send fails.
     */

    size_t max = ov_socket_get_send_buffer_size(conn->fd);
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
            if (!temp) goto error;

            if (open > (ssize_t)max) {
                len = max;
            } else {
                len = (size_t)open;
            }

            if (!ov_buffer_set(temp, ptr, len)) {
                temp = ov_buffer_free(temp);
                goto error;
            }

            if (!ov_list_queue_push(conn->io.out.queue, temp)) {
                temp = ov_buffer_free(temp);
                goto error;
            }

            temp = NULL;
            open = open - max;

            if (open > 0) ptr = ptr + max;
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
    if (!ov_buffer_set(copy, (void *)buffer->start, buffer->length)) goto error;

    if (conn->io.out.buffer) {

        /* this pointer MUST be used in the next SSL_WRITE!
         * so we queue the new data to be send */

        if (!ov_list_queue_push(conn->io.out.queue, copy)) goto error;

        copy = NULL;

        /* Return here to not increase io counters */
        return true;
    }

    ssize_t bytes = SSL_write(conn->tls.ssl, copy->start, copy->length);

    if (0 == bytes) goto error;

    /* Send all ? */

    if (bytes == (ssize_t)buffer->length) {
        copy = ov_buffer_free(copy);
        goto done;
    }

    int r = SSL_get_error(conn->tls.ssl, bytes);

    switch (r) {

        case SSL_ERROR_NONE:
        case SSL_ERROR_WANT_READ:
        case SSL_ERROR_WANT_WRITE:
            /* retry */
            break;

        case SSL_ERROR_WANT_CONNECT:
        case SSL_ERROR_WANT_ACCEPT:
        case SSL_ERROR_WANT_X509_LOOKUP:
            ov_log_error(
                "%s SSL want connect/accept/lookup after handshaked? "
                "Should never happen here",
                srv->config.name);
            OV_ASSERT(1 == 0);
            goto error;
            break;

        case SSL_ERROR_WANT_ASYNC:
        case SSL_ERROR_WANT_ASYNC_JOB:
            ov_log_error("%s SSL want async? Should never happen here",
                         srv->config.name);
            OV_ASSERT(1 == 0);
            goto error;
            break;

        case SSL_ERROR_WANT_CLIENT_HELLO_CB:
            ov_log_error("%s SSL want client CB? Should never happen here",
                         srv->config.name);
            OV_ASSERT(1 == 0);
            goto error;
            break;

        case SSL_ERROR_SYSCALL:

            /* close here again ? */
            if (bytes == 0) break;

            ov_log_error(
                "%s SSL_ERROR_SYSCALL"
                "%d | %s",
                srv->config.name,
                errno,
                strerror(errno));

            if (errno == EAGAIN) break;

            goto error;

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
    }

    /* Send again ? */

    if ((-1 == bytes) || ((bytes < 0) && (EAGAIN == errno))) {

        OV_ASSERT(NULL == conn->io.out.buffer);
        conn->io.out.buffer = copy;
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
    conn->io.out.last = ov_time_get_current_time_usecs();
    conn->io.out.bytes += bytes;
    return true;
error:
    if (srv && conn) close_connection(srv, srv->config.loop, conn);

    ov_buffer_free(copy);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool io_tls_send(ov_webserver_base *srv,
                        Connection *conn,
                        ov_event_loop *loop) {

    char errorstring[OV_SSL_ERROR_STRING_BUFFER_SIZE];
    int errorcode = -1;

    OV_ASSERT(srv);
    OV_ASSERT(conn);
    OV_ASSERT(loop);

    ssize_t bytes = 0;

    if (-1 == conn->fd || !conn->tls.handshaked) goto error;

    /* (1) Try to send any leftovers outgoing */

    if (conn->io.out.buffer) {

        /* We have an active outgoing buffer,
         * and send the next data of this buffer */

        bytes = SSL_write(conn->tls.ssl,
                          conn->io.out.buffer->start,
                          conn->io.out.buffer->length);

        if (bytes == (ssize_t)conn->io.out.buffer->length) {

            if (srv->config.debug)
                ov_log_debug("%s send %zu at %i %i bytes",
                             srv->config.name,
                             bytes,
                             conn->fd,
                             (int)conn->io.out.buffer->length);

            conn->io.out.buffer = ov_buffer_free(conn->io.out.buffer);
            conn->io.out.bytes += bytes;
            conn->io.out.last = ov_time_get_current_time_usecs();

            goto done;
        }

        if (0 == bytes) goto error;

        int r = SSL_get_error(conn->tls.ssl, bytes);

        switch (r) {

            case SSL_ERROR_NONE:
            case SSL_ERROR_WANT_READ:
            case SSL_ERROR_WANT_WRITE:
                /* retry */
                break;

            case SSL_ERROR_WANT_CONNECT:
            case SSL_ERROR_WANT_ACCEPT:
            case SSL_ERROR_WANT_X509_LOOKUP:
                ov_log_error(
                    "%s SSL want connect/accept/lookup after "
                    "handshaked? "
                    "Should never happen here",
                    srv->config.name);
                OV_ASSERT(1 == 0);
                goto error;
                break;

            case SSL_ERROR_WANT_ASYNC:
            case SSL_ERROR_WANT_ASYNC_JOB:
                ov_log_error("%s SSL want async? Should never happen here",
                             srv->config.name);
                OV_ASSERT(1 == 0);
                goto error;
                break;

            case SSL_ERROR_WANT_CLIENT_HELLO_CB:
                ov_log_error("%s SSL want client CB? Should never happen here",
                             srv->config.name);
                OV_ASSERT(1 == 0);
                goto error;
                break;

            case SSL_ERROR_SYSCALL:

                /* close here again ? */
                if (bytes == 0) break;

                if (errno == EAGAIN) break;

                ov_log_error(
                    "%s SSL_ERROR_SYSCALL"
                    "%d | %s",
                    srv->config.name,
                    errno,
                    strerror(errno));

                goto error;

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
        }

        /* Try again */
        goto done;
    }

    OV_ASSERT(NULL == conn->io.out.buffer);

    /* (2) Try to get next message of outgoing queue */

    void *out = ov_list_queue_pop(conn->io.out.queue);

    if (NULL == out) {

        /*  Nothing more in queue,
         *  nothing leftover to be send,
         *  -> disable outgoing readiness listening */

        if (conn->flags & FLAG_CLOSE_AFTER_SEND) {
            close_connection(srv, loop, conn);
            goto done;
        }

        if (!loop->callback.set(
                loop,
                conn->fd,
                OV_EVENT_IO_CLOSE | OV_EVENT_IO_IN | OV_EVENT_IO_ERR,
                srv,
                conn->io.callback)) {

            ov_log_error(
                "%s failed to reset events at %i", srv->config.name, conn->fd);

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

        if (!tls_send_buffer(srv, conn, msg->buffer)) {
            msg = ov_http_message_free(msg);
            goto error;
        }

        msg = ov_http_message_free(msg);
        goto done;
    }

    ov_buffer *buf = ov_buffer_cast(out);
    if (buf) {

        if (!tls_send_buffer(srv, conn, buf)) {
            buf = ov_buffer_free(buf);
            goto error;
        }

        buf = ov_buffer_free(buf);
        goto done;
    }

    ov_websocket_frame *frame = ov_websocket_frame_cast(out);
    if (frame) {

        if (!tls_send_buffer(srv, conn, frame->buffer)) {
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
    if (srv && conn) close_connection(srv, srv->config.loop, conn);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool cleaned_path_for_conn(Connection *conn,
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

    ssize_t bytes = snprintf(
        full_path, PATH_MAX, "%s/%s", conn->domain->config.path, cleaned_path);

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

/*
static int calculate_send_buffer_size(int socket) {

    if (socket < 0)
        goto error;
*/
/*
 *  This calculation will fill the send_buffers of the OS
 *  and let the OS return to the eventloop, when the send_buffer is
 *  ready again. May be changed to limit the send_buffer to the
 *  network buffer sizes.
 */
/*
    int send_buffer = ov_socket_get_send_buffer_size(socket);
    if (send_buffer < 1)
        goto error;

    struct sockaddr_storage sa = {0};

    if (!ov_socket_get_sockaddr_storage(socket, &sa, NULL, NULL))
        goto error;

    switch (sa.ss_family) {

        case AF_INET:
            send_buffer -= 60; // max overhead with all options IP4
            break;

        case AF_INET6:
            send_buffer -= 40;
            TODO(" ... check for IPv6 header chains.");
            break;

        default:
            goto error;
    }

    send_buffer -= 36; // max SSL overhead
    send_buffer -= 24; // TCP header

    return send_buffer;

error:
    return -1;
}

*/

/*----------------------------------------------------------------------------*/

static bool io_tcp_send(ov_webserver_base *srv,
                        Connection *conn,
                        ov_event_loop *loop) {

    OV_ASSERT(srv);
    OV_ASSERT(conn);
    OV_ASSERT(loop);

    ssize_t bytes = 0;

    if (-1 == conn->fd) goto error;

    /* (1) Try to send any leftovers outgoing */

    if (conn->io.out.buffer) {

        /* We have an active outgoing buffer,
         * and send the next data of this buffer */

        bytes = send(conn->fd,
                     conn->io.out.buffer->start,
                     conn->io.out.buffer->length,
                     0);

        if ((bytes < 0) && (EAGAIN != errno)) {
            close_connection(srv, srv->config.loop, conn);
            goto error;
        }

        /* Send again ? */
        if ((-1 == bytes) || ((bytes < 0) && (EAGAIN == errno))) return true;

        if (bytes == (ssize_t)conn->io.out.buffer->length) {

            conn->io.out.buffer = ov_buffer_free(conn->io.out.buffer);

        } else {

            /* We just send part of the buffer */
            if (!ov_buffer_shift(
                    conn->io.out.buffer, conn->io.out.buffer->start + bytes))
                goto error;
        }

        conn->io.out.bytes += bytes;
        conn->io.out.last = ov_time_get_current_time_usecs();

        goto done;
    }

    OV_ASSERT(NULL == conn->io.out.buffer);

    /* (2) Try to get next message of outgoing queue */

    void *out = ov_list_queue_pop(conn->io.out.queue);

    if (NULL == out) {

        /*  Nothing more in queue,
         *  nothing leftover to be send,
         *  -> disable outgoing readiness listening */

        if (conn->flags & FLAG_CLOSE_AFTER_SEND) {
            close_connection(srv, loop, conn);
            goto done;
        }

        if (!loop->callback.set(
                loop,
                conn->fd,
                OV_EVENT_IO_CLOSE | OV_EVENT_IO_IN | OV_EVENT_IO_ERR,
                srv,
                conn->io.callback)) {

            ov_log_error(
                "%s failed to reset events at %i", srv->config.name, conn->fd);

            goto error;
        }

        goto done;
    }

    /* (3) Some data to send to process
     *
     *  NOTE all expected outgoing message of the queue MUST
     *  be enabled explicit at this point.
     */

    if (ov_http_message_cast(out)) {

        ov_http_message *msg = ov_http_message_cast(out);

        bytes = send(conn->fd, msg->buffer->start, msg->buffer->length, 0);

        if ((bytes < 0) && (EAGAIN != errno)) {
            close_connection(srv, srv->config.loop, conn);
            goto error;
        }

        /* Send again ? */
        if ((-1 == bytes) || ((bytes < 0) && (EAGAIN == errno))) {

            /*  The msg is out of the queue and MUST be freed to avoid
             *  MEMLEAKS.
             *
             *  The buffer is NOT send, but the next buffer to send,
             *  so we set the io.out.buffer to the buffer of the message
             *  (content of the HTTP message) and free the msg structure.
             */

            conn->io.out.buffer = msg->buffer;
            msg->buffer = NULL;

            /* Will recache the msg struct without some buffer if
             * caching is enabled,
             * the send buffer is moved to io.out.buffer and will be
             * used in the next eventloop cycle
             */

            msg = ov_http_message_free(msg);
            return true;
        }

        conn->io.out.bytes += bytes;

        /* We send something from the msg */

        if (bytes != (ssize_t)msg->buffer->length) {

            /* We send something of the buffer,
             * but not the whole message! */

            conn->io.out.buffer = msg->buffer;

            msg->buffer = NULL;
            msg = ov_http_message_free(msg);

            /* Offset the bytes we actually sent */
            if (!ov_buffer_shift(
                    conn->io.out.buffer, conn->io.out.buffer->start + bytes))
                goto error;

            goto done;
        }

        OV_ASSERT(bytes == (ssize_t)msg->buffer->length);

        /* The whole message has been sent */
        msg = ov_http_message_free(msg);

    } else if (ov_buffer_cast(out)) {

        /* We do have some unspecific buffer to sent */

        ov_buffer *buffer = ov_buffer_cast(out);

        bytes = send(conn->fd, buffer->start, buffer->length, 0);

        if ((bytes < 0) && (EAGAIN != errno)) {
            close_connection(srv, srv->config.loop, conn);
            goto error;
        }

        /* Send again ? */
        if ((-1 == bytes) || ((bytes < 0) && (EAGAIN == errno))) {

            /* Nothing was sent */
            conn->io.out.buffer = buffer;
            return true;
        }

        if (bytes != (ssize_t)buffer->length) {

            /* We send something of the buffer,
             * but not the whole buffer! */

            conn->io.out.buffer = buffer;

            /* Offset the bytes we actually sent */
            if (!ov_buffer_shift(
                    conn->io.out.buffer, conn->io.out.buffer->start + bytes))
                goto error;

            goto done;
        }

        OV_ASSERT(bytes == (ssize_t)buffer->length);

        /* The whole buffer has been sent */
        buffer = ov_buffer_free(buffer);

    } else {

        ov_log_error("%s unspecific data in send queue at %i",
                     srv->config.name,
                     conn->fd);

        /* SHOULD NEVER HAPPEN */
        OV_ASSERT(1 == 0);
        goto error;
    }

done:
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool io_tcp_redirect(int socket_fd, uint8_t events, void *data) {

    ov_webserver_base *srv = ov_webserver_base_cast(data);
    if (!srv) goto error;

    /* Work on pointer NOT copy! */
    Connection *conn = &srv->conn[socket_fd];
    OV_ASSERT(conn->fd == socket_fd);

    ov_event_loop *loop = srv->config.loop;

    if ((events & OV_EVENT_IO_CLOSE) || (events & OV_EVENT_IO_ERR)) {

        if (srv->config.debug)
            ov_log_debug("%s TCP close at %i remote %s:%i",
                         srv->config.name,
                         conn->fd,
                         conn->remote.host,
                         conn->remote.port);

        goto error;
    }

    if (!(events & OV_EVENT_IO_IN)) goto process_out;

    /*
     *  This function is basically a convinience function if a client sends
     *  some http:// instead of https:// so the only reply we support
     *  is a redirect to the https port for GET requests.
     *
     *  Nonetheless we try to support the redirect with full buffering
     *  until a valid / invalid request is received.
     */

    if (NULL == conn->data)
        conn->data = ov_http_message_create(srv->config.http_message);

    ov_http_message *msg = ov_http_message_cast(conn->data);
    if (!msg || !msg->buffer) goto error;

    if (!ov_http_message_ensure_open_capacity(
            msg, srv->config.http_message.buffer.default_size))
        goto error;

    uint8_t *buf = msg->buffer->start + msg->buffer->length;
    size_t open = msg->buffer->capacity - msg->buffer->length;

    ssize_t in = recv(socket_fd, buf, open, 0);

    if (srv->config.debug)
        ov_log_debug("%s IO TCP redirect at %i | %i bytes",
                     srv->config.name,
                     socket_fd,
                     in);

    if (in == 0) {
        goto error;
    }

    // read again or try to process outgoing
    if ((-1 == in) || ((in < 0) && (EAGAIN == errno))) {

        if (events & OV_EVENT_IO_OUT) goto process_out;

        return true;
    }

    if (in < 0) {
        goto error;
    }

    msg->buffer->length += in;

    conn->io.in.last = ov_time_get_current_time_usecs();
    conn->io.in.bytes += in;
    if (!process_tcp_redirect(srv, conn)) goto error;

    // somthing to be send put to queue ?
    if (conn->flags & FLAG_CLOSE_AFTER_SEND) return true;

    // all data was send, connection can be closed
    close_connection(srv, srv->config.loop, &srv->conn[socket_fd]);
    return true;

process_out:

    if (events & OV_EVENT_IO_OUT) {

        /* Try to send next data of outgoing queue,
         * in case of error close connection. */

        if (!io_tcp_send(srv, conn, loop)) goto error;

        /* use next eventloop cycle to process next event
         * (outgoing priorisation)*/

        return true;
    }

error:
    if (srv) close_connection(srv, srv->config.loop, &srv->conn[socket_fd]);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool io_tcp_accept_redirect(int socket_fd, uint8_t events, void *data) {

    int nfd = -1;
    Connection *conn = NULL;
    ov_event_loop *loop = NULL;

    ov_webserver_base *srv = ov_webserver_base_cast(data);
    if (!srv) goto error;

    loop = srv->config.loop;
    if (!loop) goto error;

    if ((events & OV_EVENT_IO_CLOSE) || (events & OV_EVENT_IO_ERR))
        return close_tcp_listener(srv, socket_fd);

    // accept MUST have some incoming IO
    if (!(events & OV_EVENT_IO_IN)) goto error;

    struct sockaddr_storage sa = {0};
    socklen_t sa_len = sizeof(sa);

    nfd = accept(socket_fd, (struct sockaddr *)&sa, &sa_len);

    if (nfd < 0) {

        ov_log_error(
            "%s failed to accept at socket %i", srv->config.name, socket_fd);
        goto error;
    }

    if (nfd >= (int)srv->config.limit.max_sockets) {

        ov_log_error("%s fd %i max_sockets %" PRIu32,
                     srv->config.name,
                     socket_fd,
                     srv->config.limit.max_sockets);

        close(nfd);
        goto error;
    }

    if (!ov_socket_ensure_nonblocking(nfd)) goto error;

    if (NULL != srv->config.callback.accept) {

        if (!srv->config.callback.accept(
                srv->config.callback.userdata, socket_fd, nfd))
            goto error;
    }

    /*
     *      Create the connection
     */

    OV_ASSERT(-1 == srv->conn[nfd].fd);

    if (-1 != srv->conn[nfd].fd) goto error;

    conn = &srv->conn[nfd];
    conn->srv = srv;
    conn->type = CONNECTION_TYPE_HTTP_1_1;

    conn->fd = nfd;
    conn->listener = socket_fd;
    conn->created = ov_time_get_current_time_usecs();
    conn->io.callback = io_tcp_redirect;
    conn->remote = ov_socket_data_from_sockaddr_storage(&sa);

    if (!loop->callback.set(loop, nfd, EVENTS, srv, conn->io.callback))
        goto error;

    if (srv->config.debug) {

        ov_socket_data local = {0};
        ov_socket_data remote = {0};

        if (!ov_socket_get_data(nfd, &local, &remote)) {
            ov_log_error(
                "%s failed to parse data at accepted fd "
                "%i",
                srv->config.name,
                nfd);
            goto error;
        }

        ov_log_debug("%s accepted connection socket %i at %s:%i from %s:%i",
                     srv->config.name,
                     nfd,
                     local.host,
                     local.port,
                     remote.host,
                     remote.port);
    }

    return true;
error:
    if (nfd > -1) close(nfd);
    if (conn) close_connection(srv, loop, conn);

    return false;
}

/*----------------------------------------------------------------------------*/

static ov_domain *find_domain(const ov_webserver_base *srv,
                              const uint8_t *name,
                              size_t len) {

    OV_ASSERT(srv);
    OV_ASSERT(name);

    ov_domain *domain = NULL;

    for (size_t i = 0; i < srv->domain.size; i++) {

        if (srv->domain.array[i].config.name.length != len) continue;

        if (0 == memcmp(srv->domain.array[i].config.name.start, name, len)) {
            domain = &srv->domain.array[i];
            break;
        }
    }

    return domain;
}

/*----------------------------------------------------------------------------*/

static bool tls_perform_handshake(ov_webserver_base *srv, Connection *conn) {

    OV_ASSERT(srv);
    OV_ASSERT(conn);
    OV_ASSERT(conn->tls.ssl);

    char errorstring[OV_SSL_ERROR_STRING_BUFFER_SIZE];
    int errorcode = -1;
    int n = 0, r = 0;

    if (!srv || !conn || !conn->tls.ssl) goto error;

    /* Send some error SHUTDOWN SSL and send to remote */

    conn->flags = FLAG_SEND_SSL_SHUTDOWN;

    r = SSL_accept(conn->tls.ssl);

    if (1 == r) {

        const char *hostname =
            SSL_get_servername(conn->tls.ssl, TLSEXT_NAMETYPE_host_name);

        if (!hostname) {

            if (-1 == srv->domain.default_domain) {
                conn->domain = &srv->domain.array[0];
            } else {
                conn->domain = &srv->domain.array[srv->domain.default_domain];
            }

        } else {

            /* Set domain pointer for connection */
            conn->domain =
                find_domain(srv, (uint8_t *)hostname, strlen(hostname));
        }

        if (!conn->domain) goto error;

        /*
                ov_log_debug("%s TLS connect at %i for %.*s from remote %s:%i",
                             srv->config.name,
                             conn->fd,
                             conn->domain->config.name.length,
                             conn->domain->config.name.start,
                             conn->remote.host,
                             conn->remote.port);
        */
        conn->tls.handshaked = true;

    } else {

        n = SSL_get_error(conn->tls.ssl, r);

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

                if (r == 0) break;
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
                ERR_error_string_n(
                    errorcode, errorstring, OV_SSL_ERROR_STRING_BUFFER_SIZE);

                ov_log_error("%s SSL_ERROR_SSL %s at socket %i",
                             srv->config.name,
                             errorstring,
                             conn->fd);

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

    conn->flags = 0;

error:

    if (srv && conn) close_connection(srv, srv->config.loop, conn);

    return false;
}

/*----------------------------------------------------------------------------*/

static bool cb_wss_send_json(void *self, int socket, const ov_json_value *msg) {

    char *str = NULL;
    ov_websocket_frame *frame = NULL;

    ov_webserver_base *srv = ov_webserver_base_cast(self);
    if (!srv || !msg || socket < 0) goto error;

    if ((uint32_t)socket > srv->connections_max) goto error;

    Connection *conn = &srv->conn[socket];

    if (conn->fd == -1) {

        ov_log_error(
            "%s connection for %i gone - abort", srv->config.name, socket);
        goto error;
    }

    if (conn->type != CONNECTION_TYPE_WEBSOCKET) {

        ov_log_error("%s connection for %i not websocket - abort",
                     srv->config.name,
                     socket);

        goto error;
    }

    str = ov_json_value_to_string(msg);
    if (!str) goto error;

    ssize_t length = strlen(str);

    /* Send the string over websocket frames */

    frame = ov_websocket_frame_create(srv->config.websocket_frame);

    if (!frame) goto error;

    ov_websocket_frame_clear(frame);

    ssize_t chunk = srv->config.limit.max_content_bytes_per_websocket_frame;

    if (length < chunk) {

        /* Send one WS frame */

        frame->buffer->start[0] = 0x80 | OV_WEBSOCKET_OPCODE_TEXT;

        if (!ov_websocket_set_data(frame, (uint8_t *)str, length, false)) {

            ov_log_error("%s failed to set websocket data for %i",
                         srv->config.name,
                         socket);

            goto error;
        }

        if (!tls_send_buffer(srv, conn, frame->buffer)) {

            ov_log_error("%s failed to send websocket data at %i",
                         srv->config.name,
                         socket);

            goto error;
        }

        if (srv->config.debug)
            ov_log_debug("%s send WSS JSON at %i", srv->config.name, socket);

        goto done;
    }

    /* send fragmented */
    size_t counter = 0;

    uint8_t *ptr = (uint8_t *)str;
    ssize_t open = length;

    /* send fragmented start */

    frame->buffer->start[0] = 0x00 | OV_WEBSOCKET_OPCODE_TEXT;

    if (!ov_websocket_set_data(frame, ptr, chunk, false)) {

        ov_log_error(
            "%s failed to set websocket data for %i", srv->config.name, socket);

        goto error;
    }

    if (!tls_send_buffer(srv, conn, frame->buffer)) {

        ov_log_error(
            "%s failed to send websocket data at %i", srv->config.name, socket);

        goto error;
    }

    counter++;

    /* send continuation */

    open -= chunk;
    ptr += chunk;

    while (open > chunk) {

        frame->buffer->start[0] = 0x00;

        if (!ov_websocket_set_data(frame, ptr, chunk, false)) {

            ov_log_error("%s failed to set websocket data for %i",
                         srv->config.name,
                         socket);

            goto error;
        }

        if (!tls_send_buffer(srv, conn, frame->buffer)) {

            ov_log_error("%s failed to send websocket data at %i",
                         srv->config.name,
                         socket);

            goto error;
        }

        open -= chunk;
        ptr += chunk;

        counter++;
    }

    /* send last */

    frame->buffer->start[0] = 0x80;

    if (!ov_websocket_set_data(frame, ptr, open, false)) {

        ov_log_error(
            "%s failed to set websocket data for %i", srv->config.name, socket);

        goto error;
    }

    if (!tls_send_buffer(srv, conn, frame->buffer)) {

        ov_log_error(
            "%s failed to send websocket data at %i", srv->config.name, socket);

        goto error;
    }

    counter++;

    if (srv->config.debug)
        ov_log_debug("%s send WSS JSON at %i over %zu frames",
                     srv->config.name,
                     socket,
                     counter);

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

bool ov_webserver_base_send_json(ov_webserver_base *self,
                                 int socket,
                                 ov_json_value const *const data) {

    return cb_wss_send_json(self, socket, data);
}

/*----------------------------------------------------------------------------*/

static bool cb_wss_io_to_json(void *userdata,
                              int socket,
                              const ov_memory_pointer domain,
                              const char *uri,
                              ov_memory_pointer content,
                              bool text) {

    ov_json_value *value = NULL;
    ov_webserver_base *srv = ov_webserver_base_cast(userdata);

    if (!srv) goto error;

    if (!text) goto error;

    if (socket < 0) goto error;

    Connection *conn = &srv->conn[socket];
    if (!conn->domain) goto error;

    ov_event_io_config *event_config =
        ov_dict_get(conn->domain->event_handler.uri, uri);

    if (!event_config || !event_config->callback.process) goto error;

    value = ov_json_value_from_string((char *)content.start, content.length);

    if (!value) {

        if (srv->config.debug) {
            ov_log_error("%s wss io at %i for %.*s uri %s not JSON - closing",
                         srv->config.name,
                         socket,
                         (int)domain.length,
                         domain.start,
                         uri);
        }

        conn->websocket.close.code = 1003;
        const char *phrase = "non JSON input";
        strncpy(conn->websocket.close.phrase, phrase, strlen(phrase));
        goto error;
    }

    /*
        ov_log_debug("%s wss JSON io at %i for %.*s uri %s | %.*s",
                     srv->config.name,
                     socket,
                     (int)domain.length,
                     domain.start,
                     uri,
                     (int)content.length,
                     (char *)content.start);
    */

    ov_event_parameter parameter = (ov_event_parameter){

        .send.instance = srv, .send.send = cb_wss_send_json};

    memcpy(parameter.uri.name, uri, strlen(uri));
    memcpy(parameter.domain.name, domain.start, domain.length);

    return event_config->callback.process(
        event_config->userdata, socket, &parameter, value);

error:
    ov_json_value_free(value);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool check_websocket_enabled(ov_webserver_base *srv,
                                    Connection *conn,
                                    const ov_http_message *msg,
                                    const uint8_t *hostname,
                                    size_t hostname_length) {

    if (!srv || !msg || !conn || !hostname || (0 == hostname_length))
        return false;

    size_t len = msg->request.uri.length + 1;
    uint8_t key[len];
    memset(key, 0, len);

    ov_domain *domain = conn->domain;
    if (!domain) goto error;

    /* URI callbacks are stored in a dict with /0 terminated strings,
     * so we MUST ensure to check with /0 terminated uri */

    if (NULL == msg->request.uri.start) goto error;

    if (0 == msg->request.uri.start[0]) goto error;

    if (0 == msg->request.uri.length) goto error;

    if (!memcpy(key, msg->request.uri.start, msg->request.uri.length))
        goto error;

    ov_websocket_message_config *config =
        (ov_websocket_message_config *)ov_dict_get(domain->websocket.uri, key);

    if (!config) config = &domain->websocket.config;

    if (NULL == config->userdata) {

        if (srv->config.debug)
            ov_log_debug(
                "%s websocket request | "
                "- websocket not enabled for host %.*s uri %s\n",
                srv->config.name,
                (int)hostname_length,
                (char *)hostname,
                key);

        goto error;
    }

    conn->websocket.config = *config;

    return true;
error:
    if (conn) conn->websocket.config = (ov_websocket_message_config){0};

    return false;
}

/*----------------------------------------------------------------------------*/

static bool process_https_message(ov_webserver_base *srv,
                                  Connection *conn,
                                  ov_http_message *msg) {

    ov_http_message *response = NULL;

    if (!srv || !conn || !msg) goto error;

    bool is_websocket_handshake = false;

    /*  Check request domain is the same domain name as used for SSL */

    const ov_http_header *header_host = ov_http_header_get_unique(
        msg->header, msg->config.header.capacity, OV_HTTP_KEY_HOST);

    if (!header_host) goto error;

    uint8_t *hostname = (uint8_t *)header_host->value.start;
    size_t hostname_length = header_host->value.length;

    uint8_t *colon = memchr(hostname, ':', header_host->value.length);
    if (colon) hostname_length = colon - hostname;

    if (srv->config.debug) {

        ov_log_debug("%s io HTTPs at %i from %s:%i |\n %s \n",
                     srv->config.name,
                     conn->fd,
                     conn->remote.host,
                     conn->remote.port,
                     (char *)msg->buffer->start);

    } else if (0 < msg->status.code) {

        ov_log_debug("%s HTTPs at %i %.*s from %s:%i status %i",
                     srv->config.name,
                     conn->fd,
                     hostname_length,
                     hostname,
                     conn->remote.host,
                     conn->remote.port,
                     msg->status.code);
    }

    if (!conn->domain) goto error;

    /*
        if ((hostname_length != conn->domain->config.name.length) ||
            (0 !=
             memcmp(hostname, conn->domain->config.name.start,
       hostname_length))) {

            ov_log_error(
                "%s HTTPS consistency ERROR | "
                "using conn domain |%.*s| and request host |%.*s| - "
                "failure\n",
                srv->config.name,
                (int)conn->domain->config.name.length,
                conn->domain->config.name.start,
                (int)hostname_length,
                hostname);

            goto error;
        }
    */

    if (ov_websocket_process_handshake_request(
            msg, &response, &is_websocket_handshake)) {

        /* Websocket handshake with generated response */
        OV_ASSERT(response);
        OV_ASSERT(is_websocket_handshake);

        if (!check_websocket_enabled(
                srv, conn, msg, hostname, hostname_length)) {
            response = ov_http_message_free(response);
            goto error;
        }

        /*
                ov_log_debug("%s wss upgrade at %i domain %.*s uri %.*s",
                             srv->config.name,
                             conn->fd,
                             (int)conn->domain->config.name.length,
                             conn->domain->config.name.start,
                             (int)msg->request.uri.length,
                             msg->request.uri.start);
        */

        conn->type = CONNECTION_TYPE_WEBSOCKET;
        if (!tls_send_buffer(srv, conn, response->buffer)) goto error;

        response = ov_http_message_free(response);

    } else if (is_websocket_handshake) {

        /* We will not send any response, but close the socket */

        ov_log_error("%s websocket upgrade identified, processing failed",
                     srv->config.name);

        response = ov_http_message_free(response);
        goto error;

    } else {

        /* Not a websocket upgrade */
        OV_ASSERT(NULL == response);

        if (srv->config.callback.https) {
            return srv->config.callback.https(
                srv->config.callback.userdata, conn->fd, msg);

        } else if (srv->config.debug) {

            ov_log_debug("%s no HTTPS io callback set, ignoring input",
                         srv->config.name);
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

static bool process_wss_control_frame(ov_webserver_base *srv,
                                      Connection *conn,
                                      ov_websocket_frame *frame) {

    OV_ASSERT(srv);
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

            /*
             *  Try to send direct reponse or schedule response in
             *  send queue.
             */

            if (!tls_send_buffer(srv, conn, response->buffer)) goto error;

            break;

        case OV_WEBSOCKET_OPCODE_CLOSE:

            if (srv->config.debug)
                ov_log_debug("%s io wss at %i from %s:%i | close received\n",
                             srv->config.name,
                             conn->fd,
                             conn->remote.host,
                             conn->remote.port);

            conn->websocket.close.recv = true;

            // close connection over error
            goto error;
            break;

        default:

            if (srv->config.debug)
                ov_log_debug(
                    "%s io wss at %i from %s:%i | "
                    "unknown control received - closing\n",
                    srv->config.name,
                    conn->fd,
                    conn->remote.host,
                    conn->remote.port);

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

static bool defragmented_callback(ov_webserver_base *srv,
                                  Connection *conn,
                                  bool text) {

    OV_ASSERT(srv);
    OV_ASSERT(conn);
    OV_ASSERT(conn->websocket.queue);
    OV_ASSERT(conn->websocket.config.userdata);
    OV_ASSERT(conn->websocket.config.callback);

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

    frame = ov_list_queue_pop(conn->websocket.queue);

    if (!frame) goto error;

    while (frame) {

        if (!ov_buffer_push(
                buffer, (void *)frame->content.start, frame->content.length))
            goto error;

        frame = ov_websocket_frame_free(frame);
        frame = ov_list_queue_pop(conn->websocket.queue);
    }

    if (!conn->websocket.config.callback(
            conn->websocket.config.userdata,
            conn->fd,
            (ov_memory_pointer){conn->domain->config.name.start,
                                conn->domain->config.name.length},
            conn->websocket.config.uri,
            (ov_memory_pointer){
                .start = buffer->start, .length = buffer->length},
            text))
        goto error;

    ov_buffer_free(buffer);
    conn->websocket.counter = 0;
    return true;
error:
    ov_buffer_free(buffer);
    ov_websocket_frame_free(frame);
    return false;
}
/*----------------------------------------------------------------------------*/

static bool defragmented_wss_delivery(ov_webserver_base *srv,
                                      Connection *conn,
                                      ov_websocket_frame *frame,
                                      bool text) {

    OV_ASSERT(srv);
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
            switch (conn->websocket.last) {

                case OV_WEBSOCKET_FRAGMENTATION_NONE:
                case OV_WEBSOCKET_FRAGMENTATION_LAST:
                    break;

                default:

                    ov_log_error("%s websocket fragmentation mismatch at %i",
                                 srv->config.name,
                                 conn->fd);

                    goto error;
            }

            /* Non fragmented frame -> perform callback */

            if (!conn->websocket.config.callback(
                    conn->websocket.config.userdata,
                    conn->fd,
                    (ov_memory_pointer){conn->domain->config.name.start,
                                        conn->domain->config.name.length},
                    conn->websocket.config.uri,
                    frame->content,
                    text)) {

                conn->websocket.close.code = 1002;
                const char *phrase = "content not accepted";
                strncpy(conn->websocket.close.phrase, phrase, strlen(phrase));
                goto error;
            }

            ov_websocket_frame_free(frame);
            conn->websocket.counter = 0;
            conn->websocket.last = OV_WEBSOCKET_FRAGMENTATION_NONE;

            /* do not push to queue */
            return true;

            break;

        case OV_WEBSOCKET_FRAGMENTATION_START:

            /* fragmentation start */
            switch (conn->websocket.last) {

                case OV_WEBSOCKET_FRAGMENTATION_NONE:
                case OV_WEBSOCKET_FRAGMENTATION_LAST:
                    break;

                default:

                    ov_log_error("%s websocket fragmentation mismatch at %i",
                                 srv->config.name,
                                 conn->fd);

                    goto error;
            }

            if (!conn->websocket.queue) {
                conn->websocket.queue = ov_linked_list_create(
                    (ov_list_config){.item.free = ov_websocket_frame_free});

                if (!conn->websocket.queue) {

                    ov_log_error(
                        "%s websocket fragmentation failed to create "
                        " frame queue at %i",
                        srv->config.name,
                        conn->fd);

                    goto error;
                }
            }

            out = ov_list_queue_pop(conn->websocket.queue);

            if (out) {

                out = ov_websocket_frame_free(out);

                ov_log_error(
                    "%s websocket fragmentation failed - "
                    " start with non empty queue at %i",
                    srv->config.name,
                    conn->fd);

                goto error;
            }

            /* push to queue */
            break;

        case OV_WEBSOCKET_FRAGMENTATION_CONTINUE:

            switch (conn->websocket.last) {

                case OV_WEBSOCKET_FRAGMENTATION_START:
                case OV_WEBSOCKET_FRAGMENTATION_CONTINUE:
                    break;

                default:

                    ov_log_error("%s websocket fragmentation mismatch at %i",
                                 srv->config.name,
                                 conn->fd);

                    goto error;
            }

            /* push to queue */
            break;

        case OV_WEBSOCKET_FRAGMENTATION_LAST:

            switch (conn->websocket.last) {

                case OV_WEBSOCKET_FRAGMENTATION_START:
                case OV_WEBSOCKET_FRAGMENTATION_CONTINUE:
                    break;

                default:

                    ov_log_error("%s websocket fragmentation mismatch at %i",
                                 srv->config.name,
                                 conn->fd);

                    goto error;
            }

            /* push to queue */
            callback_queue = true;
            break;

        default:

            ov_log_error("%s websocket fragmentation mismatch at %i",
                         srv->config.name,
                         conn->fd);

            goto error;
    }

    if (!ov_list_queue_push(conn->websocket.queue, frame)) {

        ov_log_error(
            "%s websocket fragmentation failed "
            " to buffer frame at %i",
            srv->config.name,
            conn->fd);

        goto error;
    }

    conn->websocket.counter++;
    conn->websocket.last = frame->state;
    frame = NULL;

    if (0 != conn->websocket.config.max_frames) {

        if (conn->websocket.counter >= conn->websocket.config.max_frames) {

            if (!callback_queue) {

                ov_log_error(
                    "%s websocket fragmentation max frames reached "
                    "%zu at %i - closing",
                    srv->config.name,
                    conn->websocket.counter,
                    conn->fd);

                conn->websocket.close.code = 1002;
                const char *phrase = "max frames reached";
                strncpy(conn->websocket.close.phrase, phrase, strlen(phrase));
                goto error;
            }
        }
    }

    if (callback_queue) return defragmented_callback(srv, conn, text);

    return true;
error:
    if (conn && (0 == conn->websocket.close.code)) {
        conn->websocket.close.code = 1002;
        const char *phrase = "websocket protocol error";
        strncpy(conn->websocket.close.phrase, phrase, strlen(phrase));
    }
    ov_websocket_frame_free(frame);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool process_wss_frame(ov_webserver_base *srv,
                              Connection *conn,
                              ov_websocket_frame *frame) {

    if (!srv || !conn || !frame) goto error;

    if (srv->config.debug)
        ov_log_debug("%s io wss at %i from %s:%i\n",
                     srv->config.name,
                     conn->fd,
                     conn->remote.host,
                     conn->remote.port);

    if (frame->opcode >= 0x08)
        return process_wss_control_frame(srv, conn, frame);

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

            if (srv->config.debug)
                ov_log_debug(
                    "%s io wss at %i from %s:%i | "
                    "opcode %#x unsupported - closing\n",
                    srv->config.name,
                    conn->fd,
                    conn->remote.host,
                    conn->remote.port,
                    frame->opcode);

            goto error;
            break;
    }

    OV_ASSERT(conn->websocket.config.userdata);

    if (!ov_websocket_frame_unmask(frame)) {

        ov_log_error(
            "%s io wss at %i from %s:%i  "
            "failed to unmask - closing\n",
            srv->config.name,
            conn->fd,
            conn->remote.host,
            conn->remote.port);

        goto error;
    }

    if (conn->websocket.config.fragmented) {

        /* unfragmented websocket callback used */

        return conn->websocket.config.fragmented(
            conn->websocket.config.userdata,
            conn->fd,
            (ov_memory_pointer){conn->domain->config.name.start,
                                conn->domain->config.name.length},
            conn->websocket.config.uri,
            frame);
    }

    return defragmented_wss_delivery(srv, conn, frame, text);

error:
    ov_websocket_frame_free(frame);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool process_wss(ov_webserver_base *srv, Connection *conn) {

    OV_ASSERT(srv);
    OV_ASSERT(conn);

    ov_websocket_frame *frame = ov_websocket_frame_cast(conn->data);
    if (!frame) goto error;

    uint8_t *next = NULL;
    ov_websocket_parser_state state = OV_WEBSOCKET_PARSER_ERROR;

    while (frame) {

        OV_ASSERT(frame->buffer);

        if (frame->buffer->length == 0) goto done;

        state = ov_websocket_parse_frame(frame, &next);

        switch (state) {

            case OV_WEBSOCKET_PARSER_SUCCESS:

                conn->data = NULL;

                if (!ov_websocket_frame_shift_trailing_bytes(
                        frame, next, (ov_websocket_frame **)&conn->data)) {
                    frame = ov_websocket_frame_free(frame);
                    goto error;
                }

                if (!process_wss_frame(srv, conn, frame)) goto error;

                frame = conn->data;
                break;

            case OV_WEBSOCKET_PARSER_PROGRESS:

                // not enough data received, wait for next IO cycle
                goto done;
                break;

            default:
                conn->websocket.close.code = 1002;
                const char *phrase = "not a websocket frame";
                strncpy(conn->websocket.close.phrase, phrase, strlen(phrase));
                goto error;
        }
    }

done:
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool process_https(ov_webserver_base *srv, Connection *conn) {

    OV_ASSERT(srv);
    OV_ASSERT(conn);

    ov_http_message *msg = ov_http_message_cast(conn->data);
    if (!msg) goto error;

    uint8_t *next = NULL;
    ov_http_parser_state state = OV_HTTP_PARSER_ERROR;

    while (msg) {

        OV_ASSERT(msg->buffer);

        if (0 == msg->buffer->start[0]) goto done;

        state = ov_http_pointer_parse_message(msg, &next);

        switch (state) {

            case OV_HTTP_PARSER_SUCCESS:

                conn->data = NULL;

                if (!ov_http_message_shift_trailing_bytes(
                        msg, next, (ov_http_message **)&conn->data)) {
                    msg = ov_http_message_free(msg);
                    goto error;
                }

                if (!process_https_message(srv, conn, msg)) goto error;

                msg = conn->data;
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

static bool process_tls_io(ov_webserver_base *srv, Connection *conn) {

    OV_ASSERT(srv);
    OV_ASSERT(conn);

    if (!srv || !conn) goto error;

    /* At this point we read something to some buffer,
     * but are unaware of the type */

    switch (conn->type) {

        case CONNECTION_TYPE_ERROR:
            goto error;

        case CONNECTION_TYPE_HTTP_1_1:
            return process_https(srv, conn);
            break;

        case CONNECTION_TYPE_WEBSOCKET:
            return process_wss(srv, conn);
            break;
    };

    // fall through into error

error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool io_tls(int socket_fd, uint8_t events, void *data) {

    char errorstring[OV_SSL_ERROR_STRING_BUFFER_SIZE] = {0};
    int errorcode = -1, n = 0;

    char buf[OV_SSL_MAX_BUFFER] = {0};

    ov_webserver_base *srv = ov_webserver_base_cast(data);
    if (!srv) goto error;

    /* Work on pointer NOT copy! */
    Connection *conn = &srv->conn[socket_fd];
    OV_ASSERT(conn->fd == socket_fd);

    ov_event_loop *loop = srv->config.loop;

    if ((events & OV_EVENT_IO_CLOSE) || (events & OV_EVENT_IO_ERR)) {

        /* Ensure SSL_shutdown will not be triggered on client close */
        conn->flags = 0;
        /*
                ov_log_debug("%s TLS close at %i from remote %s:%i",
                             srv->config.name,
                             conn->fd,
                             conn->remote.host,
                             conn->remote.port);
        */
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

static bool io_tcp_accept_secure(int socket_fd, uint8_t events, void *data) {

    int nfd = -1;
    SSL *ssl = NULL;

    Connection *conn = NULL;
    ov_event_loop *loop = NULL;

    ov_webserver_base *srv = ov_webserver_base_cast(data);
    if (!srv) goto error;

    loop = srv->config.loop;
    if (!loop) goto error;

    if ((events & OV_EVENT_IO_CLOSE) || (events & OV_EVENT_IO_ERR))
        return close_tcp_listener(srv, socket_fd);

    // accept MUST have some incoming IO
    if (!(events & OV_EVENT_IO_IN)) goto error;

    struct sockaddr_storage sa = {0};
    socklen_t sa_len = sizeof(sa);

    nfd = accept(socket_fd, (struct sockaddr *)&sa, &sa_len);

    if (nfd < 0) {

        ov_log_error(
            "%s failed to accept at socket %i", srv->config.name, socket_fd);
        goto error;
    }

    if (nfd >= (int)srv->config.limit.max_sockets) {

        ov_log_error("%s fd %i max_sockets %" PRIu32,
                     srv->config.name,
                     socket_fd,
                     srv->config.limit.max_sockets);

        close(nfd);
        goto error;
    }

    if (!ov_socket_ensure_nonblocking(nfd)) goto error;

    if (NULL != srv->config.callback.accept) {

        if (!srv->config.callback.accept(
                srv->config.callback.userdata, socket_fd, nfd))
            goto error;
    }

    /*
     *      Create the connection
     */

    OV_ASSERT(-1 == srv->conn[nfd].fd);

    conn = &srv->conn[nfd];
    if (-1 != conn->fd) goto error;

    /*
     *      Create SSL for the first domain,
     *      MAY change during SSL setup over @see tls_servername_callback
     */

    ssl = SSL_new(srv->domain.array[0].context.tls);
    if (!ssl) goto error;

    if (1 != SSL_set_fd(ssl, nfd)) goto error;

    SSL_set_accept_state(ssl);

    conn->srv = srv;
    conn->fd = nfd;
    conn->type = CONNECTION_TYPE_HTTP_1_1;
    conn->listener = socket_fd;
    conn->io.callback = io_tls;
    conn->created = ov_time_get_current_time_usecs();
    conn->tls.handshaked = false;
    conn->remote = ov_socket_data_from_sockaddr_storage(&sa);

    if (!loop->callback.set(loop, nfd, EVENTS, srv, conn->io.callback))
        goto error;

    if (srv->config.debug) {

        ov_socket_data local = {0};
        ov_socket_data remote = {0};

        if (!ov_socket_get_data(nfd, &local, &remote)) {
            ov_log_error(
                "%s failed to parse data at accepted fd "
                "%i",
                srv->config.name,
                nfd);
            goto error;
        }

        ov_log_debug(
            "%s accepted secure connection socket %i at %s:%i from "
            "%s:%i",
            srv->config.name,
            nfd,
            local.host,
            local.port,
            remote.host,
            remote.port);
    }

    conn->tls.ssl = ssl;
    return true;
error:
    if (ssl) SSL_free(ssl);
    if (nfd > -1) close(nfd);
    if (conn) close_connection(srv, loop, conn);

    return false;
}

/*----------------------------------------------------------------------------*/

static bool io_stun(int socket_fd, uint8_t events, void *data) {

    /*
     *  We only support unauthenticated binding requests
     *  to support ICE.
     *
     *  We perform a minimum of required operations and copy
     *  to create the answer, but ensure the protocol is correct.
     */

    size_t size = OV_UDP_PAYLOAD_OCTETS;

    uint8_t buffer[size];
    memset(buffer, 0, size);

    uint8_t *attr[IMPL_MAX_STUN_ATTRIBUTES] = {0};

    ov_webserver_base *srv = ov_webserver_base_cast(data);
    if (!srv) goto error;

    if (!(events & OV_EVENT_IO_IN)) goto done;

    struct sockaddr_storage sa = {0};
    socklen_t sa_len = sizeof(sa);

    ssize_t in =
        recvfrom(socket_fd, buffer, size, 0, (struct sockaddr *)&sa, &sa_len);

    // read again
    if (in < 0) goto done;

    // ignore any non stun io
    if (!ov_stun_frame_is_valid(buffer, in)) goto done;

    // ignore any non binding
    if (!ov_stun_method_is_binding(buffer, in)) goto done;

    // only requests supported
    if (!ov_stun_frame_class_is_request(buffer, in)) goto done;

    // check we received no more attributes as supported
    if (!ov_stun_frame_slice(buffer, in, attr, IMPL_MAX_STUN_ATTRIBUTES))
        goto error;

    uint16_t type = 0;

    // we only support a fingerprint as attribute
    for (size_t i = 0; i < IMPL_MAX_STUN_ATTRIBUTES; i++) {

        if (attr[i] == NULL) break;

        type = ov_stun_attribute_get_type(attr[i], 4);

        switch (type) {

            case STUN_SOFTWARE:
            case STUN_FINGERPRINT:
                break;

            default:
                goto done;
        }
    }

    // fingerprint is optinal
    if (!ov_stun_check_fingerprint(
            buffer, in, attr, IMPL_MAX_STUN_ATTRIBUTES, false))
        goto done;

    /*
     *  We do use the read buffer to create the response.
     *  This will limit memory operations to the minimum amount.
     *
     *  This means, we need to change the class to response,
     *  and keep the magic_cookie, method and transaction id.
     *
     *  We will copy the xor mapped address as first attribute after
     *  the header, and reset the length attribute within the header.
     *
     *  This is a zero copy implementaion within the read buffer.
     */

    if (!ov_stun_frame_set_success_response(buffer, in)) goto error;

    if (!ov_stun_xor_mapped_address_encode(
            buffer + 20, size - 20, buffer, NULL, &sa))
        goto done;

    size_t out = 20 + ov_stun_xor_mapped_address_encoding_length(&sa);
    OV_ASSERT(out < size);

    if (!ov_stun_frame_set_length(buffer, size, out - 20)) goto error;

    // just to be sure, we nullify the rest of the buffer
    memset(buffer + out, 0, size - out);

    ssize_t send = sendto(socket_fd,
                          buffer,
                          out,
                          0,
                          (const struct sockaddr *)&sa,
                          sizeof(struct sockaddr_storage));

    /*
     *  We do ignore if we actually send the reponse or not,
     *  (in case of non debuging)
     *  as the STUN protocol is not reliable anyway.
     */

    if (srv->config.debug) {

        ov_socket_data data = ov_socket_data_from_sockaddr_storage(&sa);

        ov_log_debug(
            "%s STUN request at %i "
            "sent response %s:%i with %zu bytes",
            srv->config.name,
            socket_fd,
            data.host,
            data.port,
            send);
    }

done:
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool add_stun_listener(ov_webserver_base *srv,
                              ov_socket_configuration config,
                              int number) {

    if (!srv) goto error;

    ov_event_loop *loop = srv->config.loop;
    if (!loop || !loop->callback.set) goto error;

    int socket = ov_socket_create(config, false, NULL);

    errno = 0;
    if (-1 == socket) {
        ov_log_error("%s failed to create socket %s:%i %i|%s",
                     srv->config.name,
                     config.host,
                     config.port,
                     errno,
                     strerror(errno));
    }

    if (!ov_socket_ensure_nonblocking(socket)) {
        close(socket);
        goto error;
    }

    if (!loop->callback.set(loop, socket, EVENTS, srv, io_stun)) {
        close(socket);
        goto error;
    }

    srv->sockets.stun[number] = socket;

    ov_log_info("%s created STUN listener %s:%i",
                srv->config.name,
                config.host,
                config.port);

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static int create_socket(ov_webserver_base *srv,
                         ov_socket_configuration config,
                         bool (*callback)(int socket,
                                          uint8_t events,
                                          void *data)) {

    int socket = -1;
    if (!srv || !callback) goto error;

    ov_event_loop *loop = srv->config.loop;
    if (!loop || !loop->callback.set) goto error;

    socket = ov_socket_create(config, false, NULL);
    if (-1 == socket) {
        ov_log_error("%s failed to create socket %s:%i %i|%s",
                     srv->config.name,
                     config.host,
                     config.port,
                     errno,
                     strerror(errno));
    }

    if (!ov_socket_ensure_nonblocking(socket)) goto error;

    if (!loop->callback.set(loop, socket, EVENTS, srv, callback)) goto error;

    ov_log_info("%s created HTTP(s) listener %i %s:%i",
                srv->config.name,
                socket,
                config.host,
                config.port);

    return socket;

error:
    if (socket != -1) close(socket);
    return -1;
}

/*----------------------------------------------------------------------------*/

static bool check_io_timeout(uint32_t timer, void *data) {

    ov_webserver_base *srv = ov_webserver_base_cast(data);
    OV_ASSERT(srv);
    OV_ASSERT(timer == srv->timer.io_timeout_check);

    ov_event_loop *loop = srv->config.loop;
    srv->timer.io_timeout_check = OV_TIMER_INVALID;

    uint64_t now = ov_time_get_current_time_usecs();

    for (size_t i = 0; i < srv->connections_max; i++) {

        if (-1 == srv->conn[i].fd) continue;

        /* ACCEPT without ANY IO ? */

        if ((now - srv->conn[i].created) <
            srv->config.timer.accept_to_io_timeout_usec)
            continue;

        if (0 != srv->conn[i].websocket.close.wait_for_response) {

            if (now < srv->conn[i].websocket.close.wait_for_response) {

                srv->conn[i].websocket.close.send = true;
                srv->conn[i].websocket.close.recv = true;
                srv->conn[i].websocket.close.wait_for_response = 0;

                close_connection(srv, loop, &srv->conn[i]);
            }

            continue;
        }

        if (0 == srv->conn[i].io.in.last) {

            ov_log_error(
                "%s conn %i remote %s:%i accepted "
                "but no IO during timeout of %" PRIu64
                " (usec) - "
                "closing",
                srv->config.name,
                srv->conn[i].fd,
                srv->conn[i].remote.host,
                srv->conn[i].remote.port,
                srv->config.timer.accept_to_io_timeout_usec);

            close_connection(srv, loop, &srv->conn[i]);
            continue;
        }

        /* IO timeout without ANY IO ? */

        if (0 != srv->config.timer.io_timeout_usec) {

            if ((now - srv->conn[i].io.in.last) <
                srv->config.timer.io_timeout_usec)
                continue;

            ov_log_error(
                "%s conn %i remote %s:%i IO timeout "
                "- closing",
                srv->config.name,
                srv->conn[i].fd,
                srv->conn[i].remote.host,
                srv->conn[i].remote.port);

            close_connection(srv, loop, &srv->conn[i]);
            continue;
        }
    }

    srv->timer.io_timeout_check =
        loop->timer.set(loop,
                        srv->config.timer.accept_to_io_timeout_usec,
                        srv,
                        check_io_timeout);

    if (OV_TIMER_INVALID == srv->timer.io_timeout_check) {
        ov_log_error("%s failed to reset IO timeout check", srv->config.name);

        goto error;
    }

    return true;
error:
    return false;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      #PUBLIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_webserver_base *ov_webserver_base_create(ov_webserver_base_config config) {

    ov_webserver_base *srv = NULL;

    if (!config.loop) {
        ov_log_error("Webserver without eventloop instance unsupported.");
        goto error;
    }

    if (0 == config.name[0])
        strncpy(config.name, OV_KEY_WEBSERVER, OV_WEBSERVER_BASE_NAME_MAX);

    if (0 == config.limit.max_sockets) config.limit.max_sockets = UINT32_MAX;

    if (0 == config.limit.max_content_bytes_per_websocket_frame)
        config.limit.max_content_bytes_per_websocket_frame =
            IMPL_CONTENT_SIZE_WS_FRAME_DEFAULT;

    if (0 == config.domain_config_path[0])
        strncpy(config.domain_config_path,
                OV_WEBSERVER_BASE_DOMAIN_CONFIG_PATH_DEFAULT,
                PATH_MAX);

    if (0 == config.timer.accept_to_io_timeout_usec)
        config.timer.accept_to_io_timeout_usec = IMPL_DEFAULT_ACCEPT_TO_IO_USEC;

    config.limit.max_sockets =
        ov_socket_get_max_supported_runtime_sockets(config.limit.max_sockets);

    srv = calloc(1, sizeof(ov_webserver_base));
    if (!srv) goto error;

    memcpy(srv, &init, sizeof(ov_webserver_base));

    srv->connections_max = config.limit.max_sockets;

    srv->conn = calloc(1, srv->connections_max * sizeof(struct Connection));
    if (!srv->conn) goto error;

    for (uint32_t i = 0; i < srv->connections_max; i++) {
        srv->conn[i].fd = -1;
        srv->conn[i].websocket.last = OV_WEBSOCKET_FRAGMENTATION_NONE;
        srv->conn[i].websocket.counter = 0;
        srv->conn[i].type = CONNECTION_TYPE_ERROR;
        srv->conn[i].io.out.queue = ov_linked_list_create((ov_list_config){0});
        srv->conn[i].srv = srv;

        srv->conn[i].websocket.close.send = false;
        srv->conn[i].websocket.close.recv = false;
        srv->conn[i].websocket.close.wait_for_response = 0;
        memset(srv->conn[i].websocket.close.phrase,
               0,
               IMPL_WEBSOCKET_PHRASE_LENGTH);
        srv->conn[i].websocket.close.code = 0;
    }

    bool default_sockets = false;

    config.http.redirect.type = TCP;
    config.http.secure.type = TCP;

    if (0 == config.http.secure.host[0]) default_sockets = true;

    srv->config = config;

    /* Start the sockets for web communications */

    srv->sockets.http = -1;
    srv->sockets.https = -1;

    if (default_sockets) {

        if (!config.ip4_only) {

            /* Here we try to open IPv6 sockets for the webserver */

            srv->sockets.http =
                create_socket(srv,
                              (ov_socket_configuration){
                                  .type = TCP, .host = "::", .port = 80},
                              io_tcp_accept_redirect);

            srv->sockets.https =
                create_socket(srv,
                              (ov_socket_configuration){
                                  .type = TCP, .host = "::", .port = 443},
                              io_tcp_accept_secure);
        }

        /* Here we try to open IPv4 sockets for the webserver */

        if (-1 != srv->sockets.http)
            srv->sockets.http =
                create_socket(srv,
                              (ov_socket_configuration){
                                  .type = TCP, .host = "0.0.0.0", .port = 443},
                              io_tcp_accept_redirect);

        if (-1 != srv->sockets.https)
            srv->sockets.https =
                create_socket(srv,
                              (ov_socket_configuration){
                                  .type = TCP, .host = "0.0.0.0", .port = 443},
                              io_tcp_accept_secure);

    } else if (0 == config.http.secure.host[0]) {

        goto error;

    } else {

        if (0 != config.http.redirect.host[0])
            srv->sockets.http = create_socket(
                srv, config.http.redirect, io_tcp_accept_redirect);

        srv->sockets.https =
            create_socket(srv, config.http.secure, io_tcp_accept_secure);
    }

    if (-1 == srv->sockets.https) {

        ov_log_error(
            "%s failed to open listener socket - unrecoverable "
            "failure",
            srv->config.name);

        goto error;

    } else {

        ov_socket_data local = {0};
        if (!ov_socket_get_data(srv->sockets.https, &local, NULL)) goto error;

        ov_log_info(
            "%s open https at %s:%i", srv->config.name, local.host, local.port);
    }

    /* Adapt default buffer sizes to system buffer (if not set external) */

    int send_buf_size = ov_socket_get_send_buffer_size(srv->sockets.https);
    int recv_buf_size = ov_socket_get_recv_buffer_size(srv->sockets.https);

    int default_buf_size = send_buf_size;
    if (recv_buf_size > send_buf_size) default_buf_size = recv_buf_size;

    if (0 == config.http_message.buffer.default_size)
        config.http_message.buffer.default_size = default_buf_size;

    if (0 == config.websocket_frame.buffer.default_size)
        config.http_message.buffer.default_size = default_buf_size;

    srv->config.http_message = ov_http_message_config_init(config.http_message);

    for (size_t i = 0; i < OV_WEBSERVER_BASE_MAX_STUN_LISTENER; i++) {

        srv->sockets.stun[i] = -1;

        if (0 == config.stun.socket[i].host[0]) continue;

        config.stun.socket[i].type = UDP;

        if (!add_stun_listener(srv, config.stun.socket[i], i)) {

            ov_log_error("%s failed to create socket %s:%i %i|%s",
                         config.name,
                         config.stun.socket[i].host,
                         config.stun.socket[i].port,
                         errno,
                         strerror(errno));
        }
    }

    /* Load domains */
    if (!ov_domain_load(srv->config.domain_config_path,
                        &srv->domain.size,
                        &srv->domain.array)) {
        ov_log_error("%s failed to load domains from %s",
                     srv->config.name,
                     srv->config.domain_config_path);
        goto error;
    }

    srv->domain.default_domain = -1;

    /* Init additional (specific) SSL functions in domain context */
    for (size_t i = 0; i < srv->domain.size; i++) {

        if (!tls_init_context(srv, &srv->domain.array[i])) goto error;

        if (srv->domain.array[i].config.is_default) {

            if (-1 != srv->domain.default_domain) {
                ov_log_error("%s more than ONE domain configured as default");
                goto error;
            }

            srv->domain.default_domain = i;
        }
    }

    ov_event_loop *loop = srv->config.loop;
    srv->timer.io_timeout_check =
        loop->timer.set(loop,
                        srv->config.timer.accept_to_io_timeout_usec,
                        srv,
                        check_io_timeout);

    if (OV_TIMER_INVALID == srv->timer.io_timeout_check) goto error;

    /*
        uint64_t size = 0;
        size = srv->connections_max * sizeof(struct Connection) +
               sizeof(ov_webserver_base);


        ov_log_debug("Webserver allocated %" PRIu64 " bytes supporting %" PRIu32
                     " sockets",
                     size,
                     config.limit.max_sockets);
        */

    /* Ignore sigpipe messages */
    signal(SIGPIPE, SIG_IGN);

    return ov_webserver_base_cast(srv);

error:
    ov_webserver_base_free((ov_webserver_base *)srv);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_webserver_base_config ov_webserver_base_config_from_json(
    const ov_json_value *value) {

    ov_webserver_base_config out = (ov_webserver_base_config){0};

    if (!value) goto error;

    const ov_json_value *config = NULL;
    ov_json_value *val = NULL;
    ov_json_value *item = NULL;
    const char *str = NULL;

    config = ov_json_object_get(value, OV_KEY_WEBSERVER);
    if (!config) config = value;

    val = ov_json_object_get(config, OV_KEY_NAME);
    if (val) {

        str = ov_json_string_get(val);

        if (!str || strlen(str) >= OV_WEBSERVER_BASE_NAME_MAX) {
            ov_log_error("webserver name not string or to long, max size %i",
                         OV_WEBSERVER_BASE_NAME_MAX);

            goto error;
        }

        strncpy(out.name, str, OV_WEBSERVER_BASE_NAME_MAX);
    }

    if (ov_json_is_true(ov_json_object_get(config, OV_KEY_DEBUG)))
        out.debug = true;

    ov_json_value *sockets = ov_json_object_get(config, OV_KEY_SOCKETS);
    ov_json_value *stun = ov_json_object_get(sockets, OV_KEY_STUN);
    ov_json_value *http = ov_json_object_get(sockets, OV_KEY_HTTP);
    ov_json_value *https = ov_json_object_get(sockets, OV_KEY_HTTPS);

    if (http) {

        out.http.redirect = ov_socket_configuration_from_json(
            http, (ov_socket_configuration){0});
    }

    if (https) {

        out.http.secure = ov_socket_configuration_from_json(
            https, (ov_socket_configuration){0});
    }

    if (stun) {

        for (size_t i = 1; i <= ov_json_array_count(stun); i++) {

            if (i > OV_WEBSERVER_BASE_MAX_STUN_LISTENER) {
                ov_log_error("ignoring socket entry from %zu OOB", i);
                break;
            }

            item = ov_json_array_get(stun, i);
            if (!item) goto error;

            out.stun.socket[i - 1] = ov_socket_configuration_from_json(
                item, (ov_socket_configuration){0});
        }
    }

    if (ov_json_is_true(ov_json_object_get(config, OV_KEY_IP4_ONLY)))
        out.ip4_only = true;

    out.http_message = ov_http_message_config_from_json(config);
    out.websocket_frame = ov_websocket_frame_config_from_json(config);

    const char *domain_config_path =
        ov_json_string_get(ov_json_object_get(config, OV_KEY_DOMAINS));

    if (domain_config_path)
        if (!strncpy(out.domain_config_path, domain_config_path, PATH_MAX))
            goto error;

    ov_json_value *timer = ov_json_object_get(config, OV_KEY_TIMER);
    out.timer.io_timeout_usec =
        ov_json_number_get(ov_json_object_get(timer, OV_KEY_IO));
    out.timer.accept_to_io_timeout_usec =
        ov_json_number_get(ov_json_object_get(timer, OV_KEY_ACCEPT));

    out.limit.max_sockets = ov_json_number_get(
        ov_json_get(config, "/" OV_KEY_LIMITS "/" OV_KEY_SOCKETS));

    out.limit.max_content_bytes_per_websocket_frame = ov_json_number_get(
        ov_json_get(config, "/" OV_KEY_LIMITS "/" OV_WEBSOCKET_KEY));

    return out;
error:
    return (ov_webserver_base_config){0};
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_webserver_base_config_to_json(
    ov_webserver_base_config config) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;

    out = ov_json_object();
    if (!out) goto error;

    if (0 != config.name[0]) {
        val = ov_json_string(config.name);
        if (!ov_json_object_set(out, OV_KEY_NAME, val)) goto error;
    }

    if (config.debug) {
        val = ov_json_true();

        if (!ov_json_object_set(out, OV_KEY_DEBUG, val)) goto error;
    }

    val = ov_json_object();

    if (!ov_json_object_set(out, OV_KEY_SOCKETS, val)) goto error;

    ov_json_value *sockets = val;
    val = NULL;

    if (ov_socket_configuration_to_json(config.http.secure, &val))
        if (!ov_json_object_set(sockets, OV_KEY_HTTPS, val)) goto error;

    val = NULL;

    if (ov_socket_configuration_to_json(config.http.redirect, &val))
        if (!ov_json_object_set(sockets, OV_KEY_HTTP, val)) goto error;

    val = NULL;
    val = ov_json_array();

    if (!ov_json_object_set(sockets, OV_KEY_STUN, val)) goto error;

    ov_json_value *stun = val;
    val = NULL;

    for (size_t i = 0; i < OV_WEBSERVER_BASE_MAX_STUN_LISTENER; i++) {

        if (0 == config.stun.socket[i].host[0]) continue;

        val = NULL;
        if (!ov_socket_configuration_to_json(config.stun.socket[i], &val))
            continue;

        if (!ov_json_array_push(stun, val)) goto error;
    }

    if (config.ip4_only) {
        val = ov_json_true();
        if (!ov_json_object_set(out, OV_KEY_IP4_ONLY, val)) goto error;
    }

    val = ov_http_message_config_to_json(config.http_message);
    if (val && !ov_json_object_set(out, OV_KEY_HTTP_MESSAGE, val)) goto error;

    val = ov_websocket_frame_config_to_json(config.websocket_frame);
    if (val && !ov_json_object_set(out, OV_KEY_WEBSOCKET, val)) goto error;

    if (0 != config.domain_config_path[0]) {
        val = ov_json_string(config.domain_config_path);

        if (!ov_json_object_set(out, OV_KEY_DOMAINS, val)) goto error;
    }

    ov_json_value *timer = NULL;
    val = ov_json_object();
    if (!ov_json_object_set(out, OV_KEY_TIMER, val)) goto error;

    timer = val;
    val = NULL;

    val = ov_json_number(config.timer.io_timeout_usec);
    if (!ov_json_object_set(timer, OV_KEY_IO, val)) goto error;

    val = ov_json_number(config.timer.accept_to_io_timeout_usec);
    if (!ov_json_object_set(timer, OV_KEY_ACCEPT, val)) goto error;

    val = ov_json_object();
    if (!ov_json_object_set(out, OV_KEY_LIMITS, val)) goto error;

    ov_json_value *limits = val;
    val = NULL;

    val = ov_json_number(config.limit.max_sockets);
    if (!ov_json_object_set(limits, OV_KEY_SOCKETS, val)) goto error;

    val = ov_json_number(config.limit.max_content_bytes_per_websocket_frame);
    if (!ov_json_object_set(limits, OV_KEY_WEBSOCKET, val)) goto error;

    return out;
error:
    ov_json_value_free(val);
    ov_json_value_free(out);
    return NULL;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      #PRIVATE FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

static void close_socket(ov_webserver_base *srv,
                         ov_event_loop *loop,
                         int *socket) {

    OV_ASSERT(srv);
    OV_ASSERT(loop);
    OV_ASSERT(socket);

    int sock = *socket;
    if (-1 == sock) return;

    *socket = -1;

    if (!loop->callback.unset(loop, sock, NULL))
        ov_log_error(
            "%s failed to unset IO callback at %i", srv->config.name, sock);

    close(sock);
    return;
}

/*----------------------------------------------------------------------------*/

ov_webserver_base *ov_webserver_base_free(ov_webserver_base *srv) {

    if (!srv) goto error;

    ov_event_loop *loop = srv->config.loop;
    OV_ASSERT(loop);
    OV_ASSERT(loop->callback.unset);
    OV_ASSERT(loop->timer.unset);

    if (OV_TIMER_INVALID == srv->timer.io_timeout_check) {
        loop->timer.unset(loop, srv->timer.io_timeout_check, NULL);
        srv->timer.io_timeout_check = OV_TIMER_INVALID;
    }

    // close all sockets

    for (size_t i = 0; i < OV_WEBSERVER_BASE_MAX_STUN_LISTENER; i++)
        close_socket(srv, loop, &srv->sockets.stun[i]);

    for (size_t i = 0; i < srv->connections_max; i++) {
        close_connection(srv, loop, &srv->conn[i]);

        srv->conn[i].data = ov_http_message_free(srv->conn[i].data);
        srv->conn[i].data = ov_websocket_frame_free(srv->conn[i].data);
        srv->conn[i].data = ov_buffer_free(srv->conn[i].data);

        if (srv->conn[i].tls.ssl) {
            SSL_free(srv->conn[i].tls.ssl);
            srv->conn[i].tls.ssl = NULL;
        }

        srv->conn[i].io.out.buffer = ov_buffer_free(srv->conn[i].io.out.buffer);

        void *data = ov_list_queue_pop(srv->conn[i].io.out.queue);
        while (data) {
            data = ov_buffer_free(data);
            data = ov_http_message_free(data);
            data = ov_websocket_frame_free(data);
            OV_ASSERT(0 == data);
            data = ov_list_queue_pop(srv->conn[i].io.out.queue);
        }

        srv->conn[i].io.out.queue = ov_list_free(srv->conn[i].io.out.queue);
    }

    close_socket(srv, loop, &srv->sockets.http);
    close_socket(srv, loop, &srv->sockets.https);

    srv->domain.array =
        ov_domain_array_free(srv->domain.size, srv->domain.array);

    srv->conn = ov_data_pointer_free(srv->conn);
    free(srv);
    return NULL;
error:
    return srv;
}

/*----------------------------------------------------------------------------*/

bool ov_webserver_base_configure_websocket_callback(
    ov_webserver_base *srv,
    const ov_memory_pointer hostname,
    ov_websocket_message_config config) {

    char *key = NULL;
    ov_websocket_message_config *val = NULL;

    if (!srv || !hostname.start || (hostname.length == 0)) goto error;

    ov_domain *domain = find_domain(srv, hostname.start, hostname.length);

    if (!domain) {

        ov_log_error(
            "%s cannot configure websocket callback for domain "
            "%.*s - domain not enabled",
            srv->config.name,
            (int)hostname.length,
            hostname.start);

        goto error;
    }

    /* Check reset */

    if (NULL == config.userdata) {

        domain->websocket.config = (ov_websocket_message_config){0};
        domain->websocket.uri = ov_dict_free(domain->websocket.uri);

        ov_log_debug(
            "%s all websocket callbacks for domain "
            "%.*s - unset",
            srv->config.name,
            (int)hostname.length,
            hostname.start);

        goto done;
    }

    if ((NULL == config.callback) && (NULL == config.fragmented)) {

        if (0 != config.uri[0]) {

            ov_dict_del(domain->websocket.uri, config.uri);

            ov_log_debug(
                "%s websocket callback for domain "
                "%.*s uri %s - unset",
                srv->config.name,
                (int)hostname.length,
                hostname.start,
                config.uri);

        } else {

            domain->websocket.config = (ov_websocket_message_config){0};

            ov_log_debug(
                "%s websocket callback for domain "
                "%.*s - unset",
                srv->config.name,
                (int)hostname.length,
                hostname.start);
        }

        goto done;
    }

    if (domain->websocket.config.userdata) {

        if (domain->websocket.config.userdata != config.userdata) {

            ov_log_error(
                "%s cannot configure websocket callback for domain "
                "%.*s - userdata already set to different data",
                srv->config.name,
                (int)hostname.length,
                hostname.start);

            goto error;
        }
    }

    if (0 == config.uri[0]) {

        strcat(config.uri, "/");

        domain->websocket.config = config;

        ov_log_debug(
            "%s configured websocket callback for domain "
            "%.*s",
            srv->config.name,
            (int)hostname.length,
            hostname.start);

        goto done;
    }

    /* add uri based callback */

    if (!domain->websocket.uri) {

        ov_dict_config conf = ov_dict_string_key_config(255);
        conf.value.data_function.free = ov_data_pointer_free;

        domain->websocket.uri = ov_dict_create(conf);
        if (!domain->websocket.uri) goto error;
    }

    /* Ensure we add a key of form /uri including the root slash
     * within the dict (to support e.g. GET /uri 1.1.) */

    size_t len = strlen(config.uri) + 2;
    key = calloc(1, len);
    if (!key) goto error;

    if ('/' == config.uri[0]) {
        snprintf(key, len, "%s", config.uri);
    } else {
        snprintf(key, len, "/%s", config.uri);
    }

    val = calloc(1, sizeof(ov_websocket_message_config));
    if (!val) goto error;

    *val = config;

    if (!ov_dict_set(domain->websocket.uri, key, val, NULL)) {

        ov_log_error(
            "%s configure websocket callback for domain "
            "%.*s URI %s - failed",
            srv->config.name,
            (int)hostname.length,
            hostname.start,
            config.uri);

        goto error;
    }

done:
    ov_log_debug(
        "%s configured websocket callback for domain "
        "%.*s URI %s",
        srv->config.name,
        (int)hostname.length,
        hostname.start,
        config.uri);

    return true;
error:
    ov_data_pointer_free(key);
    ov_data_pointer_free(val);
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_webserver_base_configure_uri_event_io(
    ov_webserver_base *srv,
    const ov_memory_pointer hostname,
    const ov_event_io_config config,
    const ov_websocket_message_config *wss_io_handler) {

    char *key = NULL;
    ov_event_io_config *val = NULL;

    if (!srv || !hostname.start || (hostname.length == 0)) goto error;

    if (NULL != config.userdata && (0 == config.name[0])) {
        ov_log_error(
            "%s cannot configure event_handler callback for domain "
            "%.*s - domain without uri",
            srv->config.name,
            (int)hostname.length,
            hostname.start);

        goto error;
    }

    ov_domain *domain = find_domain(srv, hostname.start, hostname.length);

    if (!domain) {

        ov_log_error(
            "%s cannot configure event_handler callback for domain "
            "%.*s - domain not enabled",
            srv->config.name,
            (int)hostname.length,
            hostname.start);

        goto error;
    }

    /*
     *  NOTE We do NOT support overriding or deletion of some existing
     *  URI callback yet, as this will require to lock the handler config,
     *  which may be already used within some thread.
     */

    if (domain->event_handler.uri) {

        if (ov_dict_get(domain->event_handler.uri, config.name)) {

            ov_log_error(
                "%s cannot override or delete existing event_handler "
                "callback for domain %.*s uri %s",
                srv->config.name,
                (int)hostname.length,
                hostname.start,
                config.name);

            goto error;
        }
    }

    ov_websocket_message_config msg_config =
        (ov_websocket_message_config){.userdata = srv,
                                      .max_frames = IMPL_MAX_FRAMES_FOR_JSON,
                                      .callback = cb_wss_io_to_json,
                                      .close = config.callback.close};

    if (wss_io_handler) msg_config = *wss_io_handler;

    if (0 != config.name[0]) strncat(msg_config.uri, config.name, PATH_MAX - 1);

    // perform WS config reset on no userdata
    if (!config.userdata) msg_config = (ov_websocket_message_config){0};

    // perform WS URI config reset on no processing
    if (0 == config.callback.process) msg_config.callback = NULL;

    if (!ov_webserver_base_configure_websocket_callback(
            srv, hostname, msg_config)) {

        ov_log_error("%s failed to enable WSS to JSON at %.*s uri %s",
                     srv->config.name,
                     (int)hostname.length,
                     hostname.start,
                     config.name);

        goto error;
    }

    /* Check for reset */

    if (NULL == config.userdata) {

        domain->event_handler.uri = ov_dict_free(domain->event_handler.uri);
        goto done;
    }

    if (NULL == config.callback.process) {

        ov_dict_del(domain->event_handler.uri, config.name);

        ov_log_debug(
            "%s websocket callback for domain "
            "%.*s uri %s - unset",
            srv->config.name,
            (int)hostname.length,
            hostname.start,
            config.name);

        goto done;
    }

    /* add uri based callback */

    if (!domain->event_handler.uri) {

        ov_dict_config conf = ov_dict_string_key_config(255);
        conf.value.data_function.free = ov_data_pointer_free;

        domain->event_handler.uri = ov_dict_create(conf);
        if (!domain->event_handler.uri) goto error;

    } else {

        ov_event_io_config *check =
            ov_dict_get(domain->event_handler.uri, config.name);

        if (check) {

            if (check->userdata != config.userdata) goto error;
        }
    }

    key = strdup(config.name);
    if (!key) goto error;

    val = calloc(1, sizeof(ov_event_io_config));
    if (!val) goto error;

    *val = config;

    if (!ov_dict_set(domain->event_handler.uri, key, val, NULL)) {

        ov_log_error(
            "%s configure event_handler callback for domain "
            "%.*s URI %s - failed",
            srv->config.name,
            (int)hostname.length,
            hostname.start,
            config.name);

        goto error;
    }

done:
    ov_log_debug(
        "%s configured event_handler callback for domain "
        "%.*s URI %s",
        srv->config.name,
        (int)hostname.length,
        hostname.start,
        config.name);
    return true;
error:
    ov_data_pointer_free(key);
    ov_data_pointer_free(val);
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_webserver_base_send_secure(ov_webserver_base *srv,
                                   int socket,
                                   ov_buffer const *const buffer) {

    if (!srv || (socket < 0) || !buffer) goto error;

    Connection *conn = NULL;

    for (size_t i = 0; i < srv->connections_max; i++) {

        if (srv->conn[i].fd == socket) {
            conn = &srv->conn[i];
            break;
        }
    }

    if (!conn) {

        if (srv->config.debug)
            ov_log_debug(
                "%s send request for socket %i - "
                "socket not connected | abort",
                srv->config.name,
                socket);

        goto error;
    }

    if (!conn->tls.handshaked || !conn->tls.ssl) {

        if (srv->config.debug)
            ov_log_debug(
                "%s send request for socket %i - "
                "socket state is not SSL connected | abort",
                srv->config.name,
                socket);

        goto error;
    }

    /* Data and connection accepted */

    return tls_send_buffer(srv, conn, buffer);

error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool parse_content_range(const ov_http_header *range,
                                size_t *from,
                                size_t *to) {

    OV_ASSERT(range);
    OV_ASSERT(from);
    OV_ASSERT(to);

    long n1 = 0;
    long n2 = 0;

    if (!ov_string_startswith((const char *)range->value.start, "bytes="))
        goto error;

    char *end_ptr = NULL;

    char *ptr = memchr(range->value.start, '=', range->value.length);
    if (!ptr) goto error;

    ptr++;

    n1 = strtol(ptr, &end_ptr, 10);

    ptr = end_ptr;
    ptr++;

    n2 = strtol(ptr, &end_ptr, 10);

    *from = n1;
    *to = n2;

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool answer_get_range(ov_webserver_base *srv,
                             ov_file_format_desc fmt,
                             const ov_http_message *request,
                             const char *path,
                             const ov_http_header *range,
                             Connection *conn) {

    OV_ASSERT(srv);
    OV_ASSERT(request);
    OV_ASSERT(path);
    OV_ASSERT(range);
    OV_ASSERT(conn);

    ov_http_message *msg = NULL;

    ov_log_debug("%s", (char *)request->buffer->start);

    uint8_t *buffer = NULL;
    size_t size = 0;

    if (request) {

        msg = ov_http_create_status_string(
            request->config, request->version, 206, OV_HTTP_PARTIAL_CONTENT);

    } else {

        msg = ov_http_create_status_string(
            srv->config.http_message,
            (ov_http_version){.major = 1, .minor = 1},
            200,
            OV_HTTP_OK);
    }

    if (!msg) goto error;

    size_t from = 0;
    size_t to = 0;
    size_t all = 0;

    if (!parse_content_range(range, &from, &to)) goto error;

    if (OV_FILE_SUCCESS !=
        ov_file_read_partial(path, &buffer, &size, from, to, &all)) {
        ov_log_error("Failed to partial read file %s", path);
        goto error;
    }

    if (!ov_http_message_add_header_string(
            msg, OV_KEY_SERVER, srv->config.name))
        goto error;

    if (ov_file_encoding_is_utf8(path)) {

        if (!ov_http_message_add_content_type(msg, fmt.mime, "utf-8"))
            goto error;

    } else {

        if (!ov_http_message_add_content_type(msg, fmt.mime, NULL)) goto error;
    }

    if (!ov_http_message_set_date(msg)) goto error;

    if (!ov_http_message_set_content_length(msg, size)) goto error;

    if (to == 0) to = all;

    if (!ov_http_message_set_content_range(msg, all, from, to)) goto error;

    if (!ov_http_message_add_header_string(
            msg, "Access-Control-Allow-Origin", "*"))
        goto error;

    if (!ov_http_message_close_header(msg)) goto error;

    if (!ov_http_message_add_body(
            msg, (ov_memory_pointer){.start = buffer, .length = size}))
        goto error;

    if (!push_outgoing(conn, msg)) goto error;

    buffer = ov_data_pointer_free(buffer);
    return true;
error:
    // format = ov_format_close(format);
    buffer = ov_data_pointer_free(buffer);
    msg = ov_http_message_free(msg);
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_webserver_base_answer_get(ov_webserver_base *srv,
                                  int socket,
                                  ov_file_format_desc fmt,
                                  const ov_http_message *request) {

    // ov_format *format = NULL;
    ov_http_message *msg = NULL;

    uint8_t *buffer = NULL;
    size_t size = 0;

    if (!srv || (socket < 0) || !request) goto error;

    /* (1) check input */

    if (1 > fmt.desc.bytes) {

        ov_log_error(
            "%s answer get without bytes - unsupported", srv->config.name);

        goto error;
    }

    if (0 == fmt.mime[0]) {

        ov_log_error(
            "%s answer get without mime type - unsupported", srv->config.name);

        goto error;
    }

    if (!ov_http_is_request(request, OV_HTTP_METHOD_GET)) {

        ov_log_error("%s answer get without GET request - unsupported",
                     srv->config.name);

        goto error;
    }

    /* (2) find open connection */

    Connection *conn = NULL;

    for (size_t i = 0; i < srv->connections_max; i++) {

        if (srv->conn[i].fd == socket) {
            conn = &srv->conn[i];
            break;
        }
    }

    if (!conn) {

        if (srv->config.debug)
            ov_log_debug(
                "%s answer get for socket %i - "
                "socket not connected | abort",
                srv->config.name,
                socket);

        goto error;
    }

    if (!conn->tls.handshaked || !conn->tls.ssl) {

        if (srv->config.debug)
            ov_log_debug(
                "%s answer get for socket %i - "
                "socket state is not SSL connected | abort",
                srv->config.name,
                socket);

        goto error;
    }

    if (CONNECTION_TYPE_HTTP_1_1 != conn->type) {

        if (srv->config.debug)
            ov_log_debug(
                "%s answer get for socket %i - "
                "socket type not HTTP socket | abort",
                srv->config.name,
                socket);

        goto error;
    }

    /* (3) create clean path */

    char path[PATH_MAX] = {0};

    if (!cleaned_path_for_conn(conn, request, PATH_MAX, path)) {

        ov_log_error(
            "%s failed to create clean path at %i", srv->config.name, socket);

        goto error;
    }

    const ov_http_header *range = ov_http_header_get(
        request->header, request->config.header.capacity, "Range");

    if (range) return answer_get_range(srv, fmt, request, path, range, conn);

    if (OV_FILE_SUCCESS != ov_file_read(path, &buffer, &size)) {
        ov_log_error("Failed to read file %s", path);
        goto error;
    }

    if (request) {

        msg = ov_http_create_status_string(
            request->config, request->version, 200, OV_HTTP_OK);

    } else {

        msg = ov_http_create_status_string(
            srv->config.http_message,
            (ov_http_version){.major = 1, .minor = 1},
            200,
            OV_HTTP_OK);
    }

    if (!msg) goto error;

    if (!ov_http_message_add_header_string(
            msg, OV_KEY_SERVER, srv->config.name))
        goto error;

    if (ov_file_encoding_is_utf8(path)) {

        if (!ov_http_message_add_content_type(msg, fmt.mime, "utf-8"))
            goto error;

    } else {

        if (!ov_http_message_add_content_type(msg, fmt.mime, NULL)) goto error;
    }

    if (!ov_http_message_set_date(msg)) goto error;

    if (!ov_http_message_set_content_length(msg, size)) goto error;

    if (!ov_http_message_add_header_string(msg, "Accept-Ranges", "bytes"))
        goto error;

    if (!ov_http_message_close_header(msg)) goto error;

    if (!ov_http_message_add_body(
            msg, (ov_memory_pointer){.start = buffer, .length = size}))
        goto error;

    if (!push_outgoing(conn, msg)) goto error;

    buffer = ov_data_pointer_free(buffer);
    return true;
error:
    // format = ov_format_close(format);
    buffer = ov_data_pointer_free(buffer);
    msg = ov_http_message_free(msg);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool answer_head_range(ov_webserver_base *srv,
                              ov_file_format_desc fmt,
                              const ov_http_message *request,
                              const char *path,
                              const ov_http_header *range,
                              Connection *conn) {

    OV_ASSERT(srv);
    OV_ASSERT(request);
    OV_ASSERT(path);
    OV_ASSERT(range);
    OV_ASSERT(conn);

    ov_http_message *msg = NULL;

    uint8_t *buffer = NULL;
    size_t size = 0;

    if (request) {

        msg = ov_http_create_status_string(
            request->config, request->version, 206, OV_HTTP_PARTIAL_CONTENT);

    } else {

        msg = ov_http_create_status_string(
            srv->config.http_message,
            (ov_http_version){.major = 1, .minor = 1},
            200,
            OV_HTTP_OK);
    }

    if (!msg) goto error;

    size_t from = 0;
    size_t to = 0;
    size_t all = 0;

    if (!parse_content_range(range, &from, &to)) goto error;

    if (OV_FILE_SUCCESS !=
        ov_file_read_partial(path, &buffer, &size, from, to, &all)) {
        ov_log_error("Failed to partial read file %s", path);
        goto error;
    }

    if (!ov_http_message_add_header_string(
            msg, OV_KEY_SERVER, srv->config.name))
        goto error;

    if (ov_file_encoding_is_utf8(path)) {

        if (!ov_http_message_add_content_type(msg, fmt.mime, "utf-8"))
            goto error;

    } else {

        if (!ov_http_message_add_content_type(msg, fmt.mime, NULL)) goto error;
    }

    if (!ov_http_message_set_date(msg)) goto error;

    if (!ov_http_message_set_content_length(msg, size)) goto error;

    if (to == 0) to = all;

    if (!ov_http_message_set_content_range(msg, all, from, to)) goto error;

    if (!ov_http_message_add_header_string(
            msg, "Access-Control-Allow-Origin", "*"))
        goto error;

    if (!ov_http_message_close_header(msg)) goto error;

    if (!ov_http_message_add_body(
            msg, (ov_memory_pointer){.start = buffer, .length = size}))
        goto error;

    if (!push_outgoing(conn, msg)) goto error;

    buffer = ov_data_pointer_free(buffer);
    return true;
error:
    // format = ov_format_close(format);
    buffer = ov_data_pointer_free(buffer);
    msg = ov_http_message_free(msg);
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_webserver_base_answer_head(ov_webserver_base *srv,
                                   int socket,
                                   ov_file_format_desc fmt,
                                   const ov_http_message *request) {

    // ov_format *format = NULL;
    ov_http_message *msg = NULL;

    uint8_t *buffer = NULL;
    size_t size = 0;

    if (!srv || (socket < 0) || !request) goto error;

    /* (1) check input */

    if (1 > fmt.desc.bytes) {

        ov_log_error(
            "%s answer get without bytes - unsupported", srv->config.name);

        goto error;
    }

    if (0 == fmt.mime[0]) {

        ov_log_error(
            "%s answer get without mime type - unsupported", srv->config.name);

        goto error;
    }

    /* (2) find open connection */

    Connection *conn = NULL;

    for (size_t i = 0; i < srv->connections_max; i++) {

        if (srv->conn[i].fd == socket) {
            conn = &srv->conn[i];
            break;
        }
    }

    if (!conn) {

        if (srv->config.debug)
            ov_log_debug(
                "%s answer get for socket %i - "
                "socket not connected | abort",
                srv->config.name,
                socket);

        goto error;
    }

    if (!conn->tls.handshaked || !conn->tls.ssl) {

        if (srv->config.debug)
            ov_log_debug(
                "%s answer get for socket %i - "
                "socket state is not SSL connected | abort",
                srv->config.name,
                socket);

        goto error;
    }

    if (CONNECTION_TYPE_HTTP_1_1 != conn->type) {

        if (srv->config.debug)
            ov_log_debug(
                "%s answer get for socket %i - "
                "socket type not HTTP socket | abort",
                srv->config.name,
                socket);

        goto error;
    }

    /* (3) create clean path */

    char path[PATH_MAX] = {0};

    if (!cleaned_path_for_conn(conn, request, PATH_MAX, path)) {

        ov_log_error(
            "%s failed to create clean path at %i", srv->config.name, socket);

        goto error;
    }

    const ov_http_header *range = ov_http_header_get(
        request->header, request->config.header.capacity, "Range");

    if (range) return answer_head_range(srv, fmt, request, path, range, conn);

    if (OV_FILE_SUCCESS != ov_file_read(path, &buffer, &size)) {
        ov_log_error("Failed to read file %s", path);
        goto error;
    }

    if (request) {

        msg = ov_http_create_status_string(
            request->config, request->version, 200, OV_HTTP_OK);

    } else {

        msg = ov_http_create_status_string(
            srv->config.http_message,
            (ov_http_version){.major = 1, .minor = 1},
            200,
            OV_HTTP_OK);
    }

    if (!msg) goto error;

    if (!ov_http_message_add_header_string(
            msg, OV_KEY_SERVER, srv->config.name))
        goto error;

    if (ov_file_encoding_is_utf8(path)) {

        if (!ov_http_message_add_content_type(msg, fmt.mime, "utf-8"))
            goto error;

    } else {

        if (!ov_http_message_add_content_type(msg, fmt.mime, NULL)) goto error;
    }

    if (!ov_http_message_set_date(msg)) goto error;

    if (!ov_http_message_set_content_length(msg, size)) goto error;

    if (!ov_http_message_add_header_string(msg, "Accept-Ranges", "bytes"))
        goto error;

    if (!ov_http_message_close_header(msg)) goto error;

    if (!push_outgoing(conn, msg)) goto error;

    buffer = ov_data_pointer_free(buffer);
    return true;
error:
    // format = ov_format_close(format);
    buffer = ov_data_pointer_free(buffer);
    msg = ov_http_message_free(msg);
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_webserver_base_close(ov_webserver_base *srv, int socket) {

    if (!srv || 0 > socket) goto error;

    if (socket > (int)srv->connections_max) goto error;

    Connection *conn = &srv->conn[socket];

    if (-1 == conn->fd) goto error;

    OV_ASSERT(conn->fd == socket);

    if (conn->fd != socket) goto error;

    conn->flags = FLAG_SEND_SSL_SHUTDOWN;
    close_connection(srv, srv->config.loop, conn);
    return true;

error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_webserver_base_uri_file_path(ov_webserver_base *srv,
                                     int socket,
                                     const ov_http_message *msg,
                                     size_t length,
                                     char *path) {

    if (!srv || !msg || !path) goto error;

    /* (1) find open connection */

    Connection *conn = NULL;

    for (size_t i = 0; i < srv->connections_max; i++) {

        if (srv->conn[i].fd == socket) {
            conn = &srv->conn[i];
            break;
        }
    }

    if (!conn || !conn->domain) goto error;

    return cleaned_path_for_conn(conn, msg, length, path);

error:
    return false;
}

/*----------------------------------------------------------------------------*/

ov_domain *ov_webserver_base_find_domain(const ov_webserver_base *srv,
                                         const ov_memory_pointer hostname) {

    if (!srv || !hostname.start) return NULL;

    return find_domain(srv, hostname.start, hostname.length);
}
