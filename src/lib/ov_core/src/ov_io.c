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
        @file           ov_io.c
        @author         Markus Töpfer

        @date           2024-12-20


        ------------------------------------------------------------------------
*/
#include "../include/ov_io.h"
#include "../include/ov_domain.h"

#include <ov_base/ov_dict.h>
#include <ov_base/ov_linked_list.h>
#include <ov_base/ov_string.h>
#include <ov_base/ov_thread_lock.h>
#include <ov_base/ov_thread_loop.h>
#include <ov_base/ov_time.h>

#include <openssl/conf.h>
#include <openssl/err.h>
#include <openssl/ssl.h>

#define OV_IO_MAGIC_BYTES 0xf1f0

#define OV_SSL_MAX_BUFFER 16000 // 16kb SSL buffer max
#define OV_SSL_ERROR_STRING_BUFFER_SIZE 200

/*----------------------------------------------------------------------------*/

typedef enum ConnectionType {

    OV_IO_LISTENER,
    OV_IO_CONNECTION,
    OV_IO_CLIENT_CONNECTION

} ConnectionType;

/*----------------------------------------------------------------------------*/

typedef struct Connection {

    ov_io *io;
    ConnectionType type;

    uint64_t created_usec;
    uint64_t last_update_usec;

    int socket;
    int listener;

    char domain[PATH_MAX];
    ov_io_socket_config config;

    struct {

        bool handshaked;
        SSL_CTX *ctx;
        SSL *ssl;

    } tls;

    struct {

        bool (*callback)(int, uint8_t, void *);

        struct {

            ov_buffer *buffer;
            ov_list *queue;

        } out;

    } io_data;

    uint32_t timer_id;

} Connection;

/*----------------------------------------------------------------------------*/

struct ov_io {

    uint16_t magic_bytes;
    ov_io_config config;

    ov_thread_loop *tloop;

    struct {

        ssize_t default_domain;
        size_t size;
        ov_domain *array;

    } domain;

    struct {

        uint32_t reconnects;
        uint32_t timeouts;

    } timer;

    struct {

        ov_thread_lock lock;
        ov_list *list;

    } reconnects;

    
    ov_dict *connections;
};

/*----------------------------------------------------------------------------*/

static void *connection_free(void *self) {

    if (!self)
        return NULL;
    Connection *conn = (Connection *)self;

    if (conn->timer_id != OV_TIMER_INVALID) {

        ov_event_loop_timer_unset(conn->io->config.loop, conn->timer_id, NULL);
    }

    if (conn->config.callbacks.close) {

        conn->config.callbacks.close(conn->config.callbacks.userdata,
                                     conn->socket);
    }

    if (conn->tls.ssl) {
        SSL_shutdown(conn->tls.ssl);
        SSL_free(conn->tls.ssl);
        conn->tls.ssl = NULL;
    }

    if (conn->tls.ctx) {
        SSL_CTX_free(conn->tls.ctx);
        conn->tls.ctx = NULL;
    }

    if (-1 != conn->socket) {

        OV_ASSERT(conn->io);

        ov_event_loop_unset(conn->io->config.loop, conn->socket, NULL);

        close(conn->socket);
        conn->socket = -1;
    }

    if (conn->config.auto_reconnect) {

        ov_io_socket_config *conf = calloc(1, sizeof(ov_io_socket_config));

        *conf = conn->config;
        if (!ov_list_queue_push(conn->io->reconnects.list, conf)) {

            conf = ov_data_pointer_free(conf);
            ov_log_error("failed to enable auto reconnect");
        }
    }

    conn->io_data.out.queue = ov_list_free(conn->io_data.out.queue);

    conn = ov_data_pointer_free(conn);
    return NULL;
}

/*----------------------------------------------------------------------------*/

static ov_thread_message *thread_message_reconnect(){

    return ov_thread_message_standard_create(123, NULL);
}

/*----------------------------------------------------------------------------*/

static int tls_client_hello_callback(SSL *ssl, int *al, void *arg) {

    /* This function provides SNI for multiple secure domains.
     *
     * It will use the hostname provided by the SSL client hello.
     * If no hostname was used by the client, some fallback is implemented,
     * to use the standard domain of the instance.
     * If no hostname is used and no standard domain is set,
     * the function will use the first domain configured.
     *
     * If some hostname is used, but the hostname is NOT known (configured)
     * the handshake will be stopped. */

    ov_io *base = ov_io_cast(arg);
    if (!base)
        goto error;

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

    if (0 == SSL_client_hello_get0_ext(ssl, TLSEXT_NAMETYPE_host_name, &str,
                                       &str_len)) {

        /* In case the header extension for host name is NOT set,
         * the default certificate of the configured domains will be used.
         *
         * This will allow access to e.g. https://127.0.0.1:12345 with some
         * certificate response for local tests or in general IP address based
         * access to the default domain of some webserver. */

        if (-1 == base->domain.default_domain) {
            domain = &base->domain.array[0];
        } else {
            domain = &base->domain.array[base->domain.default_domain];
        }

    } else {

        /* Perform SNI */

        OV_ASSERT(str);
        OV_ASSERT(0 < str_len);

        /* min valid length without chars for hostname content */
        if (4 >= str_len)
            goto error;

        /* check if the first entry is host name */
        if (0 != str[3])
            goto error;

        size_t hostname_length = str[4];

        if (str_len < hostname_length + 5)
            goto error;

        uint8_t *hostname = (uint8_t *)str + 5;

        for (size_t i = 0; i < base->domain.size; i++) {

            if (base->domain.array[i].config.name.length != hostname_length)
                continue;

            if (0 == memcmp(hostname, base->domain.array[i].config.name.start,
                            hostname_length)) {
                domain = &base->domain.array[i];
                break;
            }
        }

        /*  If some SNI is present and the domain is not enabled.
         *  the handshake will be stopped. */

        if (!domain)
            goto error;
    }

    OV_ASSERT(domain);

    if (!SSL_set_SSL_CTX(ssl, domain->context.tls))
        goto error;

    return SSL_CLIENT_HELLO_SUCCESS;

error:
    if (al)
        *al = SSL_TLSEXT_ERR_ALERT_FATAL;
    return SSL_CLIENT_HELLO_ERROR;
}

/*----------------------------------------------------------------------------*/

static bool tls_init_context(ov_io *base, ov_domain *domain) {

    if (!base || !domain)
        goto error;

    if (!domain->context.tls) {
        ov_log_error("TLS context for |%s| not set", domain->config.name);
        goto error;
    }

    SSL_CTX_set_client_hello_cb(domain->context.tls, tls_client_hello_callback,
                                base);

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool load_domains(ov_io *self) {

    OV_ASSERT(self);

    /* Load domains */
    if (!ov_domain_load(self->config.domain.path, &self->domain.size,
                        &self->domain.array)) {
        ov_log_error("failed to load domains from %s",
                     self->config.domain.path);
        goto error;
    }

    self->domain.default_domain = -1;

    /* Init additional (specific) SSL functions in domain context */
    for (size_t i = 0; i < self->domain.size; i++) {

        if (!tls_init_context(self, &self->domain.array[i]))
            goto error;

        if (self->domain.array[i].config.is_default) {

            if (-1 != self->domain.default_domain) {
                ov_log_error("more than ONE domain configured as default");
                goto error;
            }

            self->domain.default_domain = i;
        }
    }

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

struct container1 {

    uint64_t now;
    int listener;
    ov_list *list;
};

/*----------------------------------------------------------------------------*/

static bool check_connection_timeout(const void *key, void *val, void *data) {

    if (!key)
        return true;
    Connection *conn = (Connection *)val;
    struct container1 *container = (struct container1 *)data;
    UNUSED(conn);
    UNUSED(container);
    /*
        if (container->now - conn->created <
            conn->io->config.limits.timeout_usec){

            ov_list_push(container->list, (void*) key);
        }
    */
    return true;
}

/*----------------------------------------------------------------------------*/

static bool drop_connection(void *val, void *data) {

    ov_io *self = ov_io_cast(data);
    return ov_dict_del(self->connections, val);
}

/*----------------------------------------------------------------------------*/

static bool run_timeout_check(uint32_t timer, void *data) {

    ov_io *self = ov_io_cast(data);
    OV_ASSERT(timer == self->timer.timeouts);
    self->timer.timeouts = OV_TIMER_INVALID;

    uint64_t now = ov_time_get_current_time_usecs();

    self->timer.timeouts = ov_event_loop_timer_set(
        self->config.loop, self->config.limits.timeout_usec, self,
        run_timeout_check);

    struct container1 container = (struct container1){
        .now = now, .list = ov_linked_list_create((ov_list_config){0})};

    ov_dict_for_each(self->connections, &container, check_connection_timeout);

    ov_list_for_each(container.list, self, drop_connection);

    container.list = ov_list_free(container.list);

    return true;
}

/*----------------------------------------------------------------------------*/

static bool run_reconnect(uint32_t timer, void *data) {

    ov_io *self = ov_io_cast(data);
    OV_ASSERT(timer == self->timer.reconnects);
    self->timer.reconnects = OV_TIMER_INVALID;

    ov_thread_message *msg = thread_message_reconnect();

    ov_thread_loop_send_message(self->tloop, msg, OV_RECEIVER_THREAD);

    self->timer.reconnects = ov_event_loop_timer_set(
        self->config.loop, self->config.limits.reconnect_interval_usec, self,
        run_reconnect);

    return true;
}

/*----------------------------------------------------------------------------*/

static bool init_config(ov_io_config *config) {

    if (!config || !config->loop)
        goto error;

    if (0 == config->limits.reconnect_interval_usec)
        config->limits.reconnect_interval_usec = 3000000;

    if (0 == config->limits.timeout_usec)
        config->limits.timeout_usec = 3000000;

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static int open_connection(ov_io *self, ov_io_socket_config config);

/*----------------------------------------------------------------------------*/

static bool handle_in_thread(ov_thread_loop *self, ov_thread_message *msg) {

    if (!self || !msg) goto error;
    if (msg->type != 123) goto error;

    ov_io *io = ov_thread_loop_get_data(self);
    if (!io) goto error;

    if (!ov_thread_lock_try_lock(&io->reconnects.lock)) goto error;

    ov_list *reconnects = io->reconnects.list;
    io->reconnects.list = ov_linked_list_create(
        (ov_list_config){.item.free = ov_data_pointer_free});

    ov_io_socket_config *config = ov_list_queue_pop(reconnects);

    int socket = -1;
    while (config) {

        socket = open_connection(io, *config);
        if (socket < 0){
            ov_log_error("reconnect attempt to %s:%i failed",
                         config->socket.host, config->socket.port);
        } else {
            ov_log_debug("reconnected to %s:%i", config->socket.host, config->socket.port);
        }

        config = ov_data_pointer_free(config);
        config = ov_list_queue_pop(reconnects);
    }

    reconnects = ov_list_free(reconnects);

    ov_thread_lock_unlock(&io->reconnects.lock);

error:
    msg = ov_thread_message_free(msg);
    return true;
}

/*----------------------------------------------------------------------------*/

static bool handle_in_loop(ov_thread_loop *self, ov_thread_message *msg) {
    
    if (!self || !msg) goto error;

error:
    msg = ov_thread_message_free(msg);
    return true;
}

/*----------------------------------------------------------------------------*/

static ov_thread_loop *start_connect_thread(ov_event_loop *loop,
                                            ov_io *manager) {
    return ov_thread_loop_create(
        loop,
        (ov_thread_loop_callbacks){
            .handle_message_in_thread = handle_in_thread,
            .handle_message_in_loop = handle_in_loop,
        },
        manager);
}

/*----------------------------------------------------------------------------*/

ov_io *ov_io_create(ov_io_config config) {

    ov_io *self = NULL;
    if (!init_config(&config))
        goto error;

    size_t size = sizeof(ov_io);

    self = calloc(1, size);
    if (!self)
        goto error;

    self->magic_bytes = OV_IO_MAGIC_BYTES;
    self->config = config;

    if (0 != self->config.domain.path[0]) {

        if (!load_domains(self))
            goto error;
    }

    ov_dict_config d_config = ov_dict_intptr_key_config(255);
    d_config.value.data_function.free = connection_free;

    self->connections = ov_dict_create(d_config);

    self->reconnects.list = ov_linked_list_create(
        (ov_list_config){.item.free = ov_data_pointer_free});

    self->timer.reconnects = ov_event_loop_timer_set(
        self->config.loop, self->config.limits.reconnect_interval_usec, self,
        run_reconnect);

    self->timer.timeouts = ov_event_loop_timer_set(
        self->config.loop, self->config.limits.timeout_usec, self,
        run_timeout_check);

    self->tloop = start_connect_thread(config.loop, self);
    if ((0 == self->tloop) || (!ov_thread_loop_start_threads(self->tloop))) {
        ov_log_error("Could not start connect thread");
        goto error;
    }

    if (!ov_thread_lock_init(&self->reconnects.lock, 100000)) goto error;

    /* Initialize OpenSSL */
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();

    return self;
error:
    ov_io_free(self);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_io *ov_io_free(ov_io *self) {

    if (!ov_io_cast(self))
        return self;

    ov_thread_lock_clear(&self->reconnects.lock);
    self->tloop = ov_thread_loop_free(self->tloop);

    if (OV_TIMER_INVALID != self->timer.reconnects) {
        ov_event_loop_timer_unset(self->config.loop, self->timer.reconnects,
                                  NULL);
        self->timer.reconnects = OV_TIMER_INVALID;
    }

    if (OV_TIMER_INVALID != self->timer.timeouts) {
        ov_event_loop_timer_unset(self->config.loop, self->timer.timeouts,
                                  NULL);
        self->timer.timeouts = OV_TIMER_INVALID;
    }

    self->connections = ov_dict_free(self->connections);

    self->domain.array =
        ov_domain_array_free(self->domain.size, self->domain.array);

    self->reconnects.list = ov_list_free(self->reconnects.list);

    self = ov_data_pointer_free(self);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_io *ov_io_cast(const void *data) {

    if (!data)
        return NULL;

    if (*(uint16_t *)data != OV_IO_MAGIC_BYTES)
        return NULL;

    return (ov_io *)data;
}

/*----------------------------------------------------------------------------*/

static bool search_listener(const void *key, void *val, void *data) {

    if (!key)
        return true;
    Connection *conn = (Connection *)val;
    struct container1 *container = (struct container1 *)data;

    if (conn->listener == container->listener)
        ov_list_push(container->list, (void *)key);

    return true;
}

/*----------------------------------------------------------------------------*/

static bool close_listener(ov_io *self, int socket) {

    if (!self || !socket)
        goto error;

    struct container1 container =
        (struct container1){.now = 0,
                            .listener = socket,
                            .list = ov_linked_list_create((ov_list_config){0})};

    ov_dict_for_each(self->connections, &container, search_listener);

    ov_list_for_each(container.list, self, drop_connection);

    ov_dict_del(self->connections, (void *)(intptr_t)socket);

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool stream_recv_unbuffered(ov_io *self, Connection *conn) {

    OV_ASSERT(self);
    OV_ASSERT(conn);

    size_t size = ov_socket_get_recv_buffer_size(conn->socket);
    uint8_t buffer[size];
    memset(buffer, 0, size);

    ssize_t bytes = 0;

    bytes = recv(conn->socket, buffer, size, 0);

    if (0 == bytes) {

        if (conn->type == OV_IO_CLIENT_CONNECTION) {

            if (conn->config.auto_reconnect) {

                if (ov_thread_lock_try_lock(&self->reconnects.lock)){

                    ov_io_socket_config *conf = calloc(1, sizeof(ov_io_socket_config));

                    *conf = conn->config;

                    if (!ov_list_queue_push(self->reconnects.list, conf)) {

                        conf = ov_data_pointer_free(conf);
                        ov_log_error("failed to enable auto reconnect");
                    }

                    ov_thread_lock_unlock(&self->reconnects.lock);

                    ov_log_debug("enabled reconnect to %s:%i",
                        conf->socket.host, conf->socket.port);
                } else {

                    ov_log_debug("failed to enable reconnect to %s:%i",
                        conn->config.socket.host, conn->config.socket.port);

                }
                
            }
        }

        ov_dict_del(self->connections, (void *)(intptr_t)conn->socket);
        return false;

    } else if (-1 == bytes) {

        if (errno != EAGAIN) {
            ov_dict_del(self->connections, (void *)(intptr_t)conn->socket);
            return false;

        } else {

            return true;
        }

    } else {

        conn->last_update_usec = ov_time_get_current_time_usecs();

        if (conn->config.callbacks.io) {

            return conn->config.callbacks.io(
                conn->config.callbacks.userdata, conn->socket, NULL,
                (ov_memory_pointer){.start = buffer, .length = bytes});
        }
    }

    return true;
}

/*----------------------------------------------------------------------------*/

static bool stream_send(ov_io *self, Connection *conn) {

    if (!self || !conn)
        goto error;

    ssize_t bytes = 0;

    if (conn->io_data.out.buffer) {

        bytes = send(conn->socket, conn->io_data.out.buffer->start,
                     conn->io_data.out.buffer->length, 0);
        if (bytes < 1) {
            goto done;
        } else {
            conn->io_data.out.buffer = ov_buffer_free(conn->io_data.out.buffer);
            goto done;
        }

    } else {

        ov_buffer *buffer = ov_list_queue_pop(conn->io_data.out.queue);

        if (!buffer) {

            ov_event_loop *loop = self->config.loop;

            if (!loop->callback.set(loop, conn->socket,
                                    OV_EVENT_IO_IN | OV_EVENT_IO_ERR |
                                        OV_EVENT_IO_CLOSE,
                                    self, conn->io_data.callback))
                goto error;

        } else {

            bytes = send(conn->socket, buffer->start, buffer->length, 0);
            if (bytes > 1) {
                buffer = ov_buffer_free(buffer);
                goto done;
            } else {
                conn->io_data.out.buffer = buffer;
                goto done;
            }
        }
    }

done:
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool io_stream(int socket, uint8_t events, void *data) {

    ov_io *self = ov_io_cast(data);
    if (!self)
        goto error;

    Connection *conn =
        (Connection *)ov_dict_get(self->connections, (void *)(intptr_t)socket);

    if ((events & OV_EVENT_IO_CLOSE) || (events & OV_EVENT_IO_ERR)) {

        if (conn->type == OV_IO_CLIENT_CONNECTION) {

            if (conn->config.auto_reconnect) {

                if (ov_thread_lock_try_lock(&self->reconnects.lock)){

                    ov_io_socket_config *conf = calloc(1, sizeof(ov_io_socket_config));

                    *conf = conn->config;

                    if (!ov_list_queue_push(self->reconnects.list, conf)) {

                        conf = ov_data_pointer_free(conf);
                        ov_log_error("failed to enable auto reconnect");
                    }

                    ov_thread_lock_unlock(&self->reconnects.lock);

                    ov_log_debug("enabled reconnect to %s:%i",
                        conf->socket.host, conf->socket.port);
                } else {

                    ov_log_debug("failed to enable reconnect to %s:%i",
                        conn->config.socket.host, conn->config.socket.port);

                }
            }
        }

        ov_dict_del(self->connections, (void *)(intptr_t)socket);
        goto done;
    }

    if (events & OV_EVENT_IO_OUT)
        return stream_send(self, conn);

    if (!(events & OV_EVENT_IO_IN))
        goto error;

    return stream_recv_unbuffered(self, conn);

done:
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool tls_perform_handshake(ov_io *self, Connection *conn) {

    OV_ASSERT(self);
    OV_ASSERT(conn);

    char errorstring[OV_SSL_ERROR_STRING_BUFFER_SIZE];
    int errorcode = -1;
    int n = 0, r = 0;

    r = SSL_accept(conn->tls.ssl);

    conn->last_update_usec = ov_time_get_current_time_usecs();

    if (1 == r) {

        const char *hostname =
            SSL_get_servername(conn->tls.ssl, TLSEXT_NAMETYPE_host_name);

        if (!hostname) {

            if (-1 == self->domain.default_domain) {

                if (!ov_string_copy(
                        conn->domain,
                        (char *)self->domain.array[0].config.name.start,
                        PATH_MAX))
                    goto error;

            } else {

                if (!ov_string_copy(
                        conn->domain,
                        (char *)self->domain.array[self->domain.default_domain]
                            .config.name.start,
                        PATH_MAX))
                    goto error;
            }

        } else {

            if (!ov_string_copy(conn->domain, (char *)hostname, PATH_MAX))
                goto error;
        }

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

            if (r == 0)
                break;

            goto error;
            break;

        case SSL_ERROR_SSL:

            errorcode = ERR_get_error();
            ERR_error_string_n(errorcode, errorstring,
                               OV_SSL_ERROR_STRING_BUFFER_SIZE);

            ov_log_error("SSL_ERROR_SSL %s at socket %i", errorstring,
                         conn->socket);

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

    if (conn->tls.ssl) {
        SSL_free(conn->tls.ssl);
        conn->tls.ssl = NULL;
    }

error:

    if (self && conn)
        ov_dict_del(self->connections, (void *)(intptr_t)conn->socket);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool io_stream_ssl_send(ov_io *self, Connection *conn) {

    if (!self || !conn)
        goto error;
    if (!conn->tls.ssl)
        goto error;

    ssize_t bytes = 0;

    if (conn->io_data.out.buffer) {

        bytes = SSL_write(conn->tls.ssl, conn->io_data.out.buffer->start,
                          conn->io_data.out.buffer->length);
        if (bytes < 1) {
            goto done;
        } else {
            conn->io_data.out.buffer = ov_buffer_free(conn->io_data.out.buffer);
            goto done;
        }

    } else {

        ov_buffer *buffer = ov_list_queue_pop(conn->io_data.out.queue);

        if (!buffer) {

            ov_event_loop *loop = self->config.loop;

            if (!loop->callback.set(loop, conn->socket,
                                    OV_EVENT_IO_IN | OV_EVENT_IO_ERR |
                                        OV_EVENT_IO_CLOSE,
                                    self, conn->io_data.callback))
                goto error;

        } else {

            bytes = SSL_write(conn->tls.ssl, buffer->start, buffer->length);
            if (bytes > 0) {
                buffer = ov_buffer_free(buffer);
                goto done;
            } else {
                conn->io_data.out.buffer = buffer;
                goto done;
            }
        }
    }

done:
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool io_stream_ssl(int socket, uint8_t events, void *data) {

    char errorstring[OV_SSL_ERROR_STRING_BUFFER_SIZE] = {0};
    int errorcode = -1, n = 0;
    Connection *conn = 0;

    uint8_t buffer[OV_SSL_MAX_BUFFER] = {0};

    ov_io *self = ov_io_cast(data);
    if (!self)
        goto error;

    conn =
        (Connection *)ov_dict_get(self->connections, (void *)(intptr_t)socket);
    if (!conn)
        goto error;

    conn->last_update_usec = ov_time_get_current_time_usecs();

    if ((events & OV_EVENT_IO_CLOSE) || (events & OV_EVENT_IO_ERR)) {
        ov_dict_del(self->connections, (void *)(intptr_t)socket);
        goto done;
    }

    if (!conn->tls.handshaked)
        return tls_perform_handshake(self, conn);

    if (events & OV_EVENT_IO_OUT)
        return io_stream_ssl_send(self, conn);

    if (!(events & OV_EVENT_IO_IN))
        goto error;

    ssize_t bytes = SSL_read(conn->tls.ssl, buffer, OV_SSL_MAX_BUFFER);

    if (bytes > 0) {

        if (conn->config.callbacks.io) {

            return conn->config.callbacks.io(
                conn->config.callbacks.userdata, conn->socket, conn->domain,
                (ov_memory_pointer){.start = buffer, .length = bytes});
        }

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

            ov_log_error("SSL_ERROR_SYSCALL"
                         "%d | %s",
                         errno, strerror(errno));

            goto error;
            break;

        case SSL_ERROR_SSL:

            errorcode = ERR_get_error();
            ERR_error_string_n(errorcode, errorstring,
                               OV_SSL_ERROR_STRING_BUFFER_SIZE);
            ov_log_error("SSL_ERROR_SSL %s at socket %i", errorstring,
                         conn->socket);
            goto send_no_shutdown;
            break;

        default:
            goto error;
            break;
        }
    }

    /* Try to read again */
done:
    return true;

send_no_shutdown:

    if (conn->tls.ssl) {
        SSL_free(conn->tls.ssl);
        conn->tls.ssl = NULL;
    }

error:

    if (self)
        ov_dict_del(self->connections, (void *)(intptr_t)socket);

    return false;
}

/*----------------------------------------------------------------------------*/

static Connection *accept_stream_base(
    ov_io *self, Connection *listener, int socket,
    bool (*callback)(int socket, uint8_t events, void *userdata)) {

    OV_ASSERT(self);
    OV_ASSERT(listener);
    OV_ASSERT(callback);

    int nfd = -1;

    struct sockaddr_storage sa = {0};
    socklen_t sa_len = sizeof(sa);

    nfd = accept(socket, (struct sockaddr *)&sa, &sa_len);
    if (!ov_socket_ensure_nonblocking(nfd))
        goto error;

    if (NULL != listener->config.callbacks.accept) {

        if (!listener->config.callbacks.accept(
                listener->config.callbacks.userdata, socket, nfd))
            goto error;
    }

    Connection *conn = calloc(1, sizeof(Connection));
    if (!conn)
        goto error;

    if (!ov_dict_set(self->connections, (void *)(intptr_t)nfd, conn, NULL)) {
        conn = ov_data_pointer_free(conn);
        goto error;
    }

    conn->type = OV_IO_CONNECTION;
    conn->socket = nfd;
    conn->listener = socket;
    conn->config = listener->config;
    conn->io = self;
    conn->created_usec = ov_time_get_current_time_usecs();
    conn->last_update_usec = 0;
    conn->io_data.callback = callback;
    conn->io_data.out.queue = ov_list_free(conn->io_data.out.queue);
    conn->io_data.out.queue =
        ov_linked_list_create((ov_list_config){.item.free = ov_buffer_free});

    if (!ov_event_loop_set(self->config.loop, nfd,
                           OV_EVENT_IO_IN | OV_EVENT_IO_ERR | OV_EVENT_IO_CLOSE,
                           self, callback)) {

        ov_dict_del(self->connections, (void *)(intptr_t)nfd);
        goto error;
    }

    return conn;

error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

static bool accept_stream(int socket, uint8_t events, void *data) {

    ov_io *self = ov_io_cast(data);
    if (!self)
        goto error;

    Connection *listener_conn =
        (Connection *)ov_dict_get(self->connections, (void *)(intptr_t)socket);

    if (!listener_conn)
        goto error;

    if ((events & OV_EVENT_IO_CLOSE) || (events & OV_EVENT_IO_ERR)) {
        close_listener(self, socket);
        goto done;
    }

    if (!(events & OV_EVENT_IO_IN))
        goto error;

    Connection *conn =
        accept_stream_base(self, listener_conn, socket, io_stream);

    if (!conn)
        goto error;

done:
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool accept_tls(int socket, uint8_t events, void *data) {

    SSL *ssl = NULL;

    ov_io *self = ov_io_cast(data);
    if (!self)
        goto error;

    Connection *listener_conn =
        (Connection *)ov_dict_get(self->connections, (void *)(intptr_t)socket);

    if (!listener_conn)
        goto error;

    if ((events & OV_EVENT_IO_CLOSE) || (events & OV_EVENT_IO_ERR)) {
        close_listener(self, socket);
        goto done;
    }

    if (!(events & OV_EVENT_IO_IN))
        goto error;

    Connection *conn =
        accept_stream_base(self, listener_conn, socket, io_stream_ssl);
    if (!conn)
        goto error;

    if (listener_conn->tls.ctx) {

        ssl = SSL_new(listener_conn->tls.ctx);

    } else {

        int id = 0;
        if (-1 != self->domain.default_domain)
            id = self->domain.default_domain;

        ssl = SSL_new(self->domain.array[id].context.tls);
    }

    if (!ssl)
        goto unroll;

    if (1 != SSL_set_fd(ssl, conn->socket))
        goto unroll;

    SSL_set_accept_state(ssl);

    conn->tls.handshaked = false;
    conn->tls.ssl = ssl;

done:
    return true;

unroll:
    ov_dict_del(self->connections, (void *)(intptr_t)socket);

error:
    if (ssl)
        SSL_free(ssl);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool load_certificate(SSL_CTX *ctx, ov_io_ssl_config *config) {

    if (!ctx || !config)
        goto error;

    if (SSL_CTX_use_certificate_chain_file(ctx, config->certificate.cert) !=
        1) {
        ov_log_error("%s failed to load certificate "
                     "from %s | error %d | %s",
                     config->domain, config->certificate.cert, errno,
                     strerror(errno));
        goto error;
    }

    if (SSL_CTX_use_PrivateKey_file(ctx, config->certificate.key,
                                    SSL_FILETYPE_PEM) != 1) {
        ov_log_error("%s failed to load key "
                     "from %s | error %d | %s",
                     config->domain, config->certificate.key, errno,
                     strerror(errno));
        goto error;
    }

    if (SSL_CTX_check_private_key(ctx) != 1) {
        ov_log_error("%s failure private key for\n"
                     "CERT | %s\n"
                     " KEY | %s",
                     config->domain, config->certificate.cert,
                     config->certificate.key);
        goto error;
    }

    ov_log_debug("loaded SSL certificate \n file %s\n key %s\n",
                 config->certificate.cert, config->certificate.key);

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool load_verify_locations(SSL_CTX *ctx, ov_io_ssl_config *config) {

    char errorstring[OV_SSL_ERROR_STRING_BUFFER_SIZE];
    int errorcode = -1;
    int r = 0;

    if (!ctx || !config)
        goto error;

    if (0 == config->verify_depth)
        config->verify_depth = 1;

    const char *file = NULL;
    const char *path = NULL;
    const char *client_ca = NULL;

    if (0 != config->ca.file[0])
        file = config->ca.file;
    if (0 != config->ca.path[0])
        path = config->ca.path;

    if (file || path) {

        r = SSL_CTX_load_verify_locations(ctx, file, path);

        if (r != 1) {

            errorcode = ERR_get_error();

            ERR_error_string_n(errorcode, errorstring,
                               OV_SSL_ERROR_STRING_BUFFER_SIZE);

            ov_log_error("SSL_CTX_load_verify_locations failed | %s",
                         errorstring);

            goto error;
        }

        SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);

    } else {

        ov_log_error(
            "SSL without verify - cannot verify any incoming certs - abort");

        goto error;
    }

    if (client_ca) {

        SSL_CTX_set_client_CA_list(ctx, SSL_load_client_CA_file(client_ca));

        SSL_CTX_set_verify(
            ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);

        SSL_CTX_set_verify_depth(ctx, config->verify_depth);
    }

    SSL_CTX_set_mode(ctx, SSL_MODE_AUTO_RETRY);

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static int tls_client_hello_callback_no_sni(SSL *ssl, int *al, void *arg) {

    Connection *conn = (Connection *)arg;

    const unsigned char *str = NULL;
    size_t str_len = 0;

    if (0 == SSL_client_hello_get0_ext(ssl, TLSEXT_NAMETYPE_host_name, &str,
                                       &str_len)) {

        /* In case the header extension for host name is NOT set,
         * the default certificate of the configured will be used.*/

    } else {

        /* check SNI */

        OV_ASSERT(str);
        OV_ASSERT(0 < str_len);

        /* min valid length without chars for hostname content */
        if (4 >= str_len)
            goto error;

        /* check if the first entry is host name */
        if (0 != str[3])
            goto error;

        size_t hostname_length = str[4];

        if (str_len < hostname_length + 5)
            goto error;

        uint8_t *hostname = (uint8_t *)str + 5;

        if (0 !=
            strncmp((char *)hostname, conn->config.ssl.domain, hostname_length))
            goto error;
    }

    return SSL_CLIENT_HELLO_SUCCESS;

error:
    if (al)
        *al = SSL_TLSEXT_ERR_ALERT_FATAL;
    return SSL_CLIENT_HELLO_ERROR;
}

/*----------------------------------------------------------------------------*/

static bool open_listener_ctx(ov_io *self, Connection *conn) {

    OV_ASSERT(self);
    OV_ASSERT(conn);

    SSL_CTX *ctx = NULL;

    if (TLS != conn->config.socket.type)
        return true;
    if (0 == conn->config.ssl.certificate.cert[0])
        return true;

    // dedicated ctx required

    ctx = SSL_CTX_new(TLS_server_method());
    SSL_CTX_set_min_proto_version(ctx, TLS1_2_VERSION);

    if (!load_certificate(ctx, &conn->config.ssl))
        goto error;
    if (!load_verify_locations(ctx, &conn->config.ssl))
        goto error;

    SSL_CTX_set_client_hello_cb(ctx, tls_client_hello_callback_no_sni, conn);

    conn->tls.ctx = ctx;
    return true;
error:
    if (ctx)
        SSL_CTX_free(ctx);
    return false;
}

/*----------------------------------------------------------------------------*/

int ov_io_open_listener(ov_io *self, ov_io_socket_config config) {

    if (!self)
        goto error;
    if (!config.callbacks.io)
        goto error;
    if (0 == config.socket.host[0])
        goto error;

    bool (*accept_handler)(int socket, uint8_t events, void *data) = NULL;

    switch (config.socket.type) {

    case TCP:
        accept_handler = accept_stream;
        break;
    case LOCAL:
        accept_handler = accept_stream;
        break;
    case TLS:
        accept_handler = accept_tls;
        break;
    default:
        ov_log_error("Socket type unsupported");
        goto error;
    }

    int listener = ov_socket_create(config.socket, false, NULL);
    if (!ov_socket_ensure_nonblocking(listener))
        goto error;

    Connection *conn = calloc(1, sizeof(Connection));
    if (!conn)
        goto error;

    if (!ov_dict_set(self->connections, (void *)(intptr_t)listener, conn,
                     NULL)) {
        conn = ov_data_pointer_free(conn);
        goto error;
    }

    conn->type = OV_IO_LISTENER;
    conn->socket = listener;
    conn->config = config;
    conn->io = self;
    conn->last_update_usec = ov_time_get_current_time_usecs();

    if (!open_listener_ctx(self, conn))
        goto error;

    if (!ov_event_loop_set(self->config.loop, listener,
                           OV_EVENT_IO_IN | OV_EVENT_IO_ERR | OV_EVENT_IO_CLOSE,
                           self, accept_handler))
        goto error;

    ov_log_debug("created listener %s:%i", config.socket.host,
                 config.socket.port);

    return listener;

error:
    return -1;
}

/*----------------------------------------------------------------------------*/

static void callback_connection_success(Connection *conn) {

    if (!conn)
        return;

    if (conn->config.callbacks.connected)
        conn->config.callbacks.connected(conn->config.callbacks.userdata,
                                         conn->socket);

    return;
}

/*----------------------------------------------------------------------------*/

static int tls_client_hello_cb(SSL *s, int *al, void *arg) {

    /*
     *      At some point @perform_ssl_client_handshake
     *      may stop with SSL_ERROR_WANT_CLIENT_HELLO_CB,
     *      so we add some dummy callback, to resume
     *      standard SSL operation.
     */
    if (!s)
        return SSL_CLIENT_HELLO_ERROR;

    if (al || arg) { /* ignored */
    };
    return SSL_CLIENT_HELLO_SUCCESS;
}

/*----------------------------------------------------------------------------*/

static bool tls_perform_client_handshake(Connection *conn);

/*----------------------------------------------------------------------------*/

static bool perform_client_handshake_triggered(uint32_t timer_id, void *data) {

    Connection *conn = (Connection *)data;

    OV_ASSERT(timer_id == conn->timer_id);
    conn->timer_id = OV_TIMER_INVALID;

    return tls_perform_client_handshake(conn);
}

/*----------------------------------------------------------------------------*/

static bool tls_perform_client_handshake(Connection *conn) {

    char errorstring[OV_SSL_ERROR_STRING_BUFFER_SIZE];
    int errorcode = -1;
    int n = 0, r = 0;

    OV_ASSERT(conn);

    if (!conn)
        goto error;

    conn->last_update_usec = ov_time_get_current_time_usecs();

    if (-1 == conn->socket)
        goto error;

    OV_ASSERT(conn->tls.ssl);

    ov_io *base = conn->io;
    ov_event_loop *loop = base->config.loop;

    /* Switch of the timer */

    if (conn->timer_id != OV_TIMER_INVALID) {

        loop->timer.unset(loop, conn->timer_id, NULL);
        conn->timer_id = OV_TIMER_INVALID;
    }

    r = SSL_connect(conn->tls.ssl);

    switch (r) {

    case 1:
        goto success;
        break;

    default:

        /* The TLS/SSL handshake was not successful
         * but was shut down controlled and by the
         * specifications of the TLS/SSL protocol.
         *
         * Call SSL_get_error() with the return value ret
         * to find out the reason.
         */

        n = SSL_get_error(conn->tls.ssl, r);
        switch (n) {

        case SSL_ERROR_NONE:
            /* SHOULD not ne returned in 0 */
            break;

        case SSL_ERROR_ZERO_RETURN:
            /* close */
            goto error;
            break;

        case SSL_ERROR_WANT_READ:
            /* try read again */
            goto call_again_later;
            break;

        case SSL_ERROR_WANT_WRITE:
            /* try write again */
            goto call_again_later;
            break;

        case SSL_ERROR_WANT_CONNECT:
            /* try connect again */
            goto call_again_later;
            break;

        case SSL_ERROR_WANT_X509_LOOKUP:
            /* try lookup again */
            goto call_again_later;
            break;

        case SSL_ERROR_WANT_ASYNC:
            /* try async again */
            goto call_again_later;
            break;

        case SSL_ERROR_WANT_ASYNC_JOB:
            /* try async job again */
            goto call_again_later;
            break;

        case SSL_ERROR_WANT_CLIENT_HELLO_CB:
            /* try client hello again */
            goto call_again_later;
            break;

        case SSL_ERROR_SYSCALL:
            /* nonrecoverable IO error */
            goto error;
            break;

        case SSL_ERROR_SSL:
            /* nonrecoverable SSL error */
            errorcode = ERR_get_error();
            ERR_error_string_n(errorcode, errorstring,
                               OV_SSL_ERROR_STRING_BUFFER_SIZE);

            SSL_free(conn->tls.ssl);
            conn->tls.ssl = NULL;
            goto error;
            break;

        case SSL_ERROR_WANT_ACCEPT:
            /* falltrough to accept */
            break;
        }
        break;
    }

    /*
     *      Accept the handshake
     *
     *      This stage SHOULD be reached only with SSL_ERROR_WANT_ACCEPT
     */

    if (n != SSL_ERROR_WANT_ACCEPT)
        goto error;

    // accept
    r = SSL_accept(conn->tls.ssl);

    switch (r) {

    case 1:
        /* The TLS/SSL handshake was successfully
         * completed, a TLS/SSL connection has been
         * established.
         */
        goto success;
        break;

    default:

        /* The TLS/SSL handshake was not successful
         * but was shut down controlled and by the
         * specifications of the TLS/SSL protocol.
         *
         * Call SSL_get_error() with the return value ret
         * to find out the reason.
         */

        n = SSL_get_error(conn->tls.ssl, r);
        switch (n) {

        case SSL_ERROR_NONE:
            /* SHOULD not ne returned in 0 */
            break;

        case SSL_ERROR_ZERO_RETURN:
            /* close */
            goto error;
            break;

        case SSL_ERROR_WANT_READ:
        case SSL_ERROR_WANT_WRITE:
        case SSL_ERROR_WANT_CONNECT:
        case SSL_ERROR_WANT_X509_LOOKUP:
        case SSL_ERROR_WANT_ASYNC:
        case SSL_ERROR_WANT_ASYNC_JOB:
        case SSL_ERROR_WANT_CLIENT_HELLO_CB:
        case SSL_ERROR_WANT_ACCEPT:
            goto call_again_later;
            break;

        case SSL_ERROR_SYSCALL:
            /* nonrecoverable IO error */
            goto error;
            break;

        case SSL_ERROR_SSL:
            /* nonrecoverable SSL error */
            errorcode = ERR_get_error();
            ERR_error_string_n(errorcode, errorstring,
                               OV_SSL_ERROR_STRING_BUFFER_SIZE);

            SSL_free(conn->tls.ssl);
            conn->tls.ssl = NULL;
            goto error;
            break;

        default:
            goto error;
        }
        break;
    }

    OV_ASSERT(1 == 0);

call_again_later:

    /*
     *      Handshake not successfull, but also not failed yet.
     *      Need to recall the function. This will be done
     *      via a timed callback.
     */

    conn->timer_id =
        loop->timer.set(loop, 50000, conn, perform_client_handshake_triggered);

    if (OV_TIMER_INVALID == conn->timer_id) {
        ov_log_error("Failed to reenable trigger "
                     "for SSL client handshake");
        goto error;
    }

    return true;

success:

    if (conn->config.ssl.certificate.cert[0]) {
        if (SSL_get_verify_result(conn->tls.ssl) != X509_V_OK)
            goto error;
    }
    conn->tls.handshaked = true;
    callback_connection_success(conn);
    return true;

error:
    if (conn)
        ov_dict_del(conn->io->connections, (void *)(intptr_t)conn->socket);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool io_ssl_client(int socket, uint8_t events, void *data) {

    size_t size = OV_SSL_MAX_BUFFER;
    uint8_t buffer[size];
    memset(buffer, 0, size);

    ov_io *self = ov_io_cast(data);
    if (!self)
        goto error;

    Connection *conn = ov_dict_get(self->connections, (void *)(intptr_t)socket);
    if (!conn)
        goto error;

    conn->last_update_usec = ov_time_get_current_time_usecs();

    OV_ASSERT(self);

    if ((events & OV_EVENT_IO_CLOSE) || (events & OV_EVENT_IO_ERR)) {

        if (conn->type == OV_IO_CLIENT_CONNECTION) {

            if (conn->config.auto_reconnect) {

                ov_io_socket_config *conf =
                    calloc(1, sizeof(ov_io_socket_config));

                *conf = conn->config;
                if (!ov_list_queue_push(self->reconnects.list, conf)) {

                    conf = ov_data_pointer_free(conf);
                    ov_log_error("failed to enable auto reconnect");
                    goto error;
                }
            }
        }

        ov_dict_del(self->connections, (void *)(intptr_t)socket);
        return true;
    }

    if (!conn->tls.handshaked)
        return tls_perform_client_handshake(conn);

    if ((events & OV_EVENT_IO_OUT))
        return io_stream_ssl_send(self, conn);

    if (!(events & OV_EVENT_IO_IN))
        goto error;

    ssize_t bytes = SSL_read(conn->tls.ssl, buffer, size);

    if (0 == bytes) {

        ov_dict_del(self->connections, (void *)(intptr_t)socket);
        return false;

    } else if (-1 == bytes) {

        if (errno != EAGAIN) {
            ov_dict_del(self->connections, (void *)(intptr_t)socket);
            return false;

        } else {

            return true;
        }

    } else {

        if (conn->config.callbacks.io) {

            return conn->config.callbacks.io(
                conn->config.callbacks.userdata, conn->socket, NULL,
                (ov_memory_pointer){.start = buffer, .length = bytes});
        }
    }

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool init_ssl_client(ov_io *self, Connection *conn) {

    OV_ASSERT(self);
    OV_ASSERT(conn);

    char errorstring[OV_SSL_ERROR_STRING_BUFFER_SIZE];
    int errorcode = -1;

    int r = 0;

    conn->tls.ctx = SSL_CTX_new(TLS_client_method());
    if (!conn->tls.ctx)
        goto error;

    if (conn->config.ssl.verify_depth == 0)
        conn->config.ssl.verify_depth = 1;

    const char *file = NULL;
    const char *path = NULL;

    if (0 != conn->config.ssl.ca.file[0])
        file = conn->config.ssl.ca.file;
    if (0 != conn->config.ssl.ca.path[0])
        path = conn->config.ssl.ca.path;

    if (file || path) {

        r = SSL_CTX_load_verify_locations(conn->tls.ctx, file, path);

        if (r != 1) {

            errorcode = ERR_get_error();
            ERR_error_string_n(errorcode, errorstring,
                               OV_SSL_ERROR_STRING_BUFFER_SIZE);
            ov_log_error("SSL_CTX_load_verify_locations failed "
                         "at socket %i | %s",
                         conn->socket, errorstring);
            goto error;
        }

    } else {

        ov_log_error("SSL client %i without verify "
                     "- cannot verify any incoming certs - abort",
                     conn->socket);

        goto error;
    }

    if (0 != conn->config.ssl.certificate.cert[0]) {

        if (1 != SSL_CTX_use_certificate_file(conn->tls.ctx,
                                              conn->config.ssl.certificate.cert,
                                              SSL_FILETYPE_PEM))
            goto error;

        if (1 != SSL_CTX_use_PrivateKey_file(conn->tls.ctx,
                                             conn->config.ssl.certificate.key,
                                             SSL_FILETYPE_PEM))
            goto error;

        if (1 != SSL_CTX_check_private_key(conn->tls.ctx))
            goto error;
    }

    SSL_CTX_set_mode(conn->tls.ctx, SSL_MODE_AUTO_RETRY);
    SSL_CTX_set_verify(conn->tls.ctx, SSL_VERIFY_PEER, NULL);
    SSL_CTX_set_verify_depth(conn->tls.ctx, conn->config.ssl.verify_depth);
    SSL_CTX_set_client_hello_cb(conn->tls.ctx, tls_client_hello_cb, NULL);

    /* init ssl */

    conn->tls.ssl = SSL_new(conn->tls.ctx);
    if (!conn->tls.ssl)
        goto error;
    conn->io_data.callback = io_ssl_client;

    /* create read bio */

    if (1 != SSL_set_fd(conn->tls.ssl, conn->socket)) {
        ov_log_error("SSL_set_fd failed "
                     "at socket %i | %s",
                     conn->socket, errorstring);
        goto error;
    }

    SSL_set_connect_state(conn->tls.ssl);

    if (0 != conn->config.ssl.domain[0]) {

        if (1 != SSL_set_tlsext_host_name(conn->tls.ssl,
                                          (char *)conn->config.ssl.domain)) {

            goto error;
        }
    }

    tls_perform_client_handshake(conn);

    return true;
error:
    return false;
}


/*----------------------------------------------------------------------------*/

static int open_connection(ov_io *self, ov_io_socket_config config) {

    if (!self)
        goto error;
    if (!config.callbacks.io)
        goto error;

    bool (*io)(int socket, uint8_t events, void *data) = NULL;

    switch (config.socket.type) {

    case TCP:
    case LOCAL:
        io = io_stream;
        break;
    case TLS:
        io = io_ssl_client;
        break;
    default:
        ov_log_error("Socket type unsupported.");
        goto error;
    }

    int socket = ov_socket_create(config.socket, true, NULL);
    if (-1 == socket) {

        if (config.auto_reconnect) {

            ov_io_socket_config *conf = calloc(1, sizeof(ov_io_socket_config));

            *conf = config;
            if (!ov_list_queue_push(self->reconnects.list, conf)) {

                conf = ov_data_pointer_free(conf);
                ov_log_error("failed to enable auto reconnect");
                goto error;
            }

            goto error;

        } else {

            ov_log_error("failed to create socket %s:%i %i|%s",
                         config.socket.host, config.socket.port, errno,
                         strerror(errno));
        }

        goto error;
    }

    if (!ov_socket_ensure_nonblocking(socket)) {
        close(socket);
        goto error;
    }

    Connection *conn = calloc(1, sizeof(Connection));
    if (!conn)
        goto error;

    if (!ov_dict_set(self->connections, (void *)(intptr_t)socket, conn, NULL)) {
        conn = ov_data_pointer_free(conn);
        goto error;
    }

    conn->socket = socket;
    conn->listener = -1;
    conn->type = OV_IO_CLIENT_CONNECTION;
    conn->config = config;
    conn->io = self;
    conn->io_data.callback = io;
    conn->io_data.out.buffer = NULL;
    conn->io_data.out.queue = ov_list_free(conn->io_data.out.queue);
    conn->io_data.out.queue =
        ov_linked_list_create((ov_list_config){.item.free = ov_buffer_free});

    if (TLS == config.socket.type) {

        if (!init_ssl_client(self, conn)) {
            ov_dict_del(self->connections, (void *)(intptr_t)socket);
            goto error;
        }
    }

    if (!ov_event_loop_set(self->config.loop, socket,
                           OV_EVENT_IO_IN | OV_EVENT_IO_ERR | OV_EVENT_IO_CLOSE,
                           self, conn->io_data.callback)) {

        ov_dict_del(self->connections, (void *)(intptr_t)socket);
        goto error;
    }

    if (config.callbacks.connected)
        config.callbacks.connected(config.callbacks.userdata, socket);

    return socket;

error:
    return -1;
}

/*----------------------------------------------------------------------------*/

int ov_io_open_connection(ov_io *self, ov_io_socket_config config) {

    if (!self)
        goto error;
    if (!config.callbacks.io)
        goto error;

    if (ov_thread_lock_try_lock(&self->reconnects.lock)){

        ov_io_socket_config *conf = calloc(1, sizeof(ov_io_socket_config));
        *conf = config;

        if (!ov_list_queue_push(self->reconnects.list, conf)) {

            conf = ov_data_pointer_free(conf);
            ov_log_error("failed to enable auto reconnect");
        
        }

        ov_thread_lock_unlock(&self->reconnects.lock);

        ov_log_debug("enabled reconnect to %s:%i",
                        config.socket.host, config.socket.port);
    } else {
        
        ov_log_debug("failed to enable reconnect to %s:%i",
                       config.socket.host, config.socket.port);

    }

error:
    return -1;
}

/*----------------------------------------------------------------------------*/

ov_io_config ov_io_config_from_json(const ov_json_value *input) {

    ov_io_config config = {0};

    const ov_json_value *conf = ov_json_object_get(input, OV_KEY_IO);
    if (!conf)
        conf = input;

    const char *string = ov_json_string_get(
        ov_json_get(conf, "/" OV_KEY_DOMAIN "/" OV_KEY_PATH));

    if (string)
        strncpy(config.domain.path, string, PATH_MAX);

    config.limits.reconnect_interval_usec = ov_json_number_get(
        ov_json_get(conf, "/" OV_KEY_LIMITS "/" OV_KEY_RECONNECT_USEC));

    config.limits.timeout_usec = ov_json_number_get(
        ov_json_get(conf, "/" OV_KEY_LIMITS "/" OV_KEY_TIMEOUT_USEC));

    return config;
}

/*----------------------------------------------------------------------------*/

bool ov_io_close(ov_io *self, int socket) {

    if (!self)
        goto error;
    return ov_dict_del(self->connections, (void *)(intptr_t)socket);

error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_io_send(ov_io *self, int socket, const ov_memory_pointer buffer) {

    if (!self)
        goto error;

    Connection *conn = ov_dict_get(self->connections, (void *)(intptr_t)socket);
    if (!conn)
        goto error;

    ov_event_loop *loop = self->config.loop;

    /* Ensure outgoing readiness listening */

    if (!loop->callback.set(loop, socket,
                            OV_EVENT_IO_IN | OV_EVENT_IO_ERR |
                                OV_EVENT_IO_CLOSE | OV_EVENT_IO_OUT,
                            self, conn->io_data.callback))
        goto error;

    size_t max = ov_socket_get_send_buffer_size(socket);
    if (max < buffer.length) {

        ov_buffer *temp = NULL;
        ssize_t open = buffer.length;
        uint8_t *ptr = (uint8_t *)buffer.start;
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

            if (!ov_list_queue_push(conn->io_data.out.queue, temp)) {
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

    // Push to queue

    ov_buffer *buf = ov_buffer_create(buffer.length);
    if (!buf)
        goto error;

    if (!ov_buffer_push(buf, (void *)buffer.start, buffer.length))
        goto error;

    if (!ov_list_queue_push(conn->io_data.out.queue, buf)) {
        buf = ov_buffer_free(buf);
        goto error;
    }

    if (conn->tls.ssl) {
        io_stream_ssl_send(self, conn);
    } else {
        stream_send(self, conn);
    }

    return true;

error:
    return false;
}

/*----------------------------------------------------------------------------*/

ov_domain *ov_io_get_domain(ov_io *self, const char *name) {

    OV_ASSERT(self);
    OV_ASSERT(name);

    ov_domain *domain = NULL;
    size_t len = strlen(name);

    for (size_t i = 0; i < self->domain.size; i++) {

        if (self->domain.array[i].config.name.length != len)
            continue;

        if (0 == memcmp(self->domain.array[i].config.name.start, name, len)) {
            domain = &self->domain.array[i];
            break;
        }
    }

    return domain;
}