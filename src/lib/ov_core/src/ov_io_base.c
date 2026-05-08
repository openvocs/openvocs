/***
        ------------------------------------------------------------------------

        Copyright (c) 2021 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_io_base.c
        @author         Markus Töpfer
        @author         Michael Beer

        @date           2021-01-30


        ------------------------------------------------------------------------
*/
#include "../include/ov_io_base.h"
#include "../include/ov_domain.h"

#include <ov_base/ov_utils.h>

#include <openssl/conf.h>
#include <openssl/err.h>
#include <openssl/ssl.h>

#include <ov_arch/ov_arch_math.h>
#include <ov_base/ov_config_keys.h>
#include <ov_base/ov_linked_list.h>
#include <ov_base/ov_thread_lock.h>
#include <ov_base/ov_time.h>

#define OV_SSL_MAX_BUFFER 16000 // 16kb SSL buffer max
#define OV_SSL_ERROR_STRING_BUFFER_SIZE 200

#define FLAG_CLOSE_AFTER_SEND 0x0001
#define FLAG_SEND_SSL_SHUTDOWN 0x0010

/*----------------------------------------------------------------------------*/

typedef enum SocketType {

    CONNECTION_UNUSED = 0,
    CONNECTION,
    CLIENT,
    LISTENER

} SocketType;

/*----------------------------------------------------------------------------*/

typedef struct Connection {

    ov_io_base *base;
    ov_thread_lock lock;

    bool use_socket_buffer;

    SocketType type;

    int fd;

    struct {

        int fd;
        ov_io_base_listener_config config;

    } listener;

    ov_io_base_connection_config config;
    void (*connected)(void *userdata, int socket, bool result);

    ov_domain *domain;

    ov_io_base_statistics stats;

    uint16_t flags;

    struct {

        SSL_CTX *ctx;
        SSL *ssl;
        bool handshaked;

    } tls;

    struct {

        ov_buffer *buffer;
        ov_list *queue;

    } out;

    struct {

        int recv_buffer_size;
        ssize_t offset;

    } in;

    uint32_t timer_id;

} Connection;

/*----------------------------------------------------------------------------*/

struct ov_io_base {

    uint16_t magic_byte;
    ov_io_base_config config;

    struct {

        uint32_t io_timeout_check;
        uint64_t timeout_usecs;

    } timer;

    struct {

        ssize_t standard;
        size_t size;
        ov_domain *array;

    } domain;

    ov_list *reconnects;
    uint64_t last_reconnect_run;

    // private connections max of allocated connections
    uint32_t connections_max;
    struct Connection conn[];
};

/*----------------------------------------------------------------------------*/

static ov_io_base *ov_io_base_cast(const void *self) {

    /* used internal as we have void* from time to time e.g. for SSL */

    if (!self)
        goto error;

    if (*(uint16_t *)self == OV_IO_BASE_MAGIC_BYTE)
        return (ov_io_base *)self;
error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

static void close_connection(Connection *conn) {

    if (!conn)
        return;

    OV_ASSERT(conn->base);
    OV_ASSERT(conn->base->config.loop);

    int socket = conn->fd;

    if (-1 == socket)
        return;

    size_t i = 0;

    for (i = 0; i < 100; i++) {

        if (ov_thread_lock_try_lock(&conn->lock))
            break;
    }

    OV_ASSERT(i < 100);

    ov_io_base *base = conn->base;
    ov_event_loop *loop = base->config.loop;

    if (conn->config.auto_reconnect) {

        ov_io_base_connection_config *conf =
            calloc(1, sizeof(ov_io_base_connection_config));

        if (conf) {

            *conf = conn->config;

            if (!ov_list_queue_push(base->reconnects, conf)) {

                conf = ov_data_pointer_free(conf);
                ov_log_error("%s failed to enable auto reconnect for %i",
                             conn->base->config.name, conn->fd);
            }

        } else {

            ov_log_error("%s failed to enable auto reconnect for %i",
                         conn->base->config.name, conn->fd);
        }
    }

    if (conn->type == LISTENER) {

        /* close all clients */

        for (size_t i = 0; i < base->connections_max; i++) {

            if (-1 == base->conn[i].fd)
                continue;

            if (conn->fd != base->conn[i].listener.fd)
                continue;

            close_connection(&base->conn[i]);
        }
    }

    if (!loop->callback.unset(loop, conn->fd, NULL))
        ov_log_error("%s failed to unset connection IO "
                     "callback at %i",
                     base->config.name, conn->fd);

    if (conn->tls.ssl) {

        if (conn->flags & FLAG_SEND_SSL_SHUTDOWN)
            SSL_shutdown(conn->tls.ssl);

        SSL_free(conn->tls.ssl);
        conn->tls.ssl = NULL;
    }

    if (conn->tls.ctx) {

        SSL_CTX_free(conn->tls.ctx);
        conn->tls.ctx = NULL;
    }

    close(socket);

    if (OV_TIMER_INVALID != conn->timer_id) {
        loop->callback.unset(loop, conn->timer_id, NULL);
        conn->timer_id = OV_TIMER_INVALID;
    }

    if (conn->config.connection.callback.close)
        conn->config.connection.callback.close(
            conn->config.connection.callback.userdata, socket);

    /* reset data */

    conn->listener.config = (ov_io_base_listener_config){0};
    conn->config = (ov_io_base_connection_config){0};

    conn->type = CONNECTION_UNUSED;

    conn->fd = -1;
    conn->listener.fd = -1;

    conn->flags = 0;

    conn->tls.ssl = NULL;
    conn->tls.handshaked = false;

    conn->in.recv_buffer_size = 0;
    conn->in.offset = 0;
    conn->stats = (ov_io_base_statistics){0};
    conn->out.buffer = ov_buffer_free(conn->out.buffer);
    ov_list_clear(conn->out.queue);

    ov_thread_lock_unlock(&conn->lock);

    return;
}

/*----------------------------------------------------------------------------*/

static void callback_connection_success(Connection *conn) {

    if (!conn)
        return;

    if (conn->config.connected)
        conn->config.connected(conn->config.connection.callback.userdata,
                               conn->fd, true);

    if (conn->config.auto_reconnect) {

        ov_log_info("%s auto reconnect with socket %i", conn->base->config.name,
                    conn->fd);
    }

    return;
}

/*----------------------------------------------------------------------------*/

static void callback_connection_error(Connection *conn) {

    if (!conn)
        return;

    if (!conn->config.auto_reconnect)
        if (conn->config.connected)
            conn->config.connected(conn->config.connection.callback.userdata,
                                   conn->fd, false);

    // close connection will autoreconnect in case of error due to close
    return;
}

/*----------------------------------------------------------------------------*/

static bool check_io_timeout(uint32_t timer, void *data) {

    ov_io_base *base = ov_io_base_cast(data);
    OV_ASSERT(base);
    OV_ASSERT(timer == base->timer.io_timeout_check);

    ov_event_loop *loop = base->config.loop;

    uint64_t now = ov_time_get_current_time_usecs();

    base->timer.io_timeout_check = loop->timer.set(
        loop, base->timer.timeout_usecs, base, check_io_timeout);

    if (OV_TIMER_INVALID == base->timer.io_timeout_check) {
        ov_log_error("%s failed to reset IO timeout check", base->config.name);
        goto error;
    }

    for (size_t i = 0; i < base->connections_max; i++) {

        if (-1 == base->conn[i].fd)
            continue;

        switch (base->conn[i].type) {

        case CONNECTION_UNUSED:
        case LISTENER:
        case CLIENT:

            /* Do not check idle for non inbound connection sockets */

            continue;
            break;

        case CONNECTION:

            /* ACCEPT without ANY IO ? */

            if ((now - base->conn[i].stats.created_usec) <
                base->config.timer.accept_to_io_timeout_usec)
                continue;

            if (0 == base->conn[i].stats.recv.last_usec) {

                ov_log_error("%s conn %i remote %s:%i accepted "
                             "but no IO during timeout of %" PRIu64 " ("
                             "usec) "
                             "- "
                             "closing",
                             base->config.name, base->conn[i].fd,
                             base->conn[i].stats.remote.host,
                             base->conn[i].stats.remote.port,
                             base->config.timer.accept_to_io_timeout_usec);

                close_connection(&base->conn[i]);
            }

            /* IO timeout without ANY IO ? */

            if (0 != base->config.timer.io_timeout_usec) {

                if ((now - base->conn[i].stats.recv.last_usec) <
                    base->config.timer.io_timeout_usec)
                    continue;

                ov_log_error("%s conn %i remote %s:%i IO timeout "
                             "- closing",
                             base->config.name, base->conn[i].fd,
                             base->conn[i].stats.remote.host,
                             base->conn[i].stats.remote.port);

                close_connection(&base->conn[i]);
            }

            break;
        }
    }

    /* auto reconnect */

    if (now >= (base->last_reconnect_run +
                base->config.timer.reconnect_interval_usec)) {

        ov_list *reconnects = base->reconnects;

        base->reconnects = ov_linked_list_create(
            (ov_list_config){.item.free = ov_data_pointer_free});

        ov_io_base_connection_config *conf = ov_list_queue_pop(reconnects);

        int socket = -1;
        while (conf) {

            /* We just call the connection create,
             * with same config as initial config.
             *
             * The result is unimportated as auto reconnect will
             * reschedule itself in case of error */

            socket = ov_io_base_create_connection(base, *conf);
            if (socket < 0)
                ov_log_error("reconnect attempt to %s:%i failed",
                             conf->connection.socket.host,
                             conf->connection.socket.port);

            conf = ov_data_pointer_free(conf);
            conf = ov_list_queue_pop(reconnects);
        }

        reconnects = ov_list_free(reconnects);

        /* actually we do not need to have a microsecond
         * resolution here, so we store the now we already have
         * (without rerunning timestamp) */

        base->last_reconnect_run = now;
    }

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_io_base_debug(ov_io_base *self, bool on) {

    if (!self)
        return false;

    self->config.debug = on;

    return true;
}

/*----------------------------------------------------------------------------*/

ov_io_base *ov_io_base_create(ov_io_base_config config) {

    ov_io_base *base = NULL;

    if (!config.loop) {
        ov_log_error("IO without eventloop instance unsupported.");
        goto error;
    }

    if (0 == config.name[0])
        strncpy(config.name, "base", OV_IO_BASE_NAME_MAX);

    if (0 == config.limit.max_sockets)
        config.limit.max_sockets = UINT32_MAX;

    config.limit.thread_lock_timeout_usec =
        OV_OR_DEFAULT(config.limit.thread_lock_timeout_usec,
                      OV_IO_BASE_DEFAULT_THREADLOCK_TIMEOUT_USEC);

    config.timer.accept_to_io_timeout_usec =
        OV_OR_DEFAULT(config.timer.accept_to_io_timeout_usec,
                      OV_IO_BASE_DEFAULT_ACCEPT_TIMEOUT_USEC);

    config.timer.reconnect_interval_usec =
        OV_OR_DEFAULT(config.timer.reconnect_interval_usec,
                      OV_IO_BASE_DEFAULT_ACCEPT_TIMEOUT_USEC);

    config.limit.max_sockets =
        ov_socket_get_max_supported_runtime_sockets(config.limit.max_sockets);

    uint64_t size = config.limit.max_sockets * sizeof(struct Connection) +
                    sizeof(ov_io_base);

    base = calloc(1, size);
    if (!base)
        goto error;

    base->magic_byte = OV_IO_BASE_MAGIC_BYTE;

    base->connections_max = config.limit.max_sockets;
    base->config = config;
    base->domain.standard = -1;

    ov_event_loop *loop = base->config.loop;

    // This timer is used both for io timeouts and reconnects.
    // Hence, its timeout must be the min of both!
    base->timer.timeout_usecs =
        OV_MIN(base->config.timer.accept_to_io_timeout_usec,
               config.timer.reconnect_interval_usec);

    base->timer.io_timeout_check = loop->timer.set(
        loop, base->timer.timeout_usecs, base, check_io_timeout);

    if (OV_TIMER_INVALID == base->timer.io_timeout_check)
        goto error;

    /* init connections */

    for (uint32_t i = 0; i < base->connections_max; i++) {
        base->conn[i].type = CONNECTION_UNUSED;

        // See header for weird semantics
        base->conn[i].use_socket_buffer = !config.dont_buffer;
        base->conn[i].base = base;
        base->conn[i].fd = -1;
        base->conn[i].out.queue = ov_linked_list_create(
            (ov_list_config){.item.free = ov_buffer_free});

        base->conn[i].timer_id = OV_TIMER_INVALID;
        base->last_reconnect_run = 0;

        if (!ov_thread_lock_init(&base->conn[i].lock,
                                 config.limit.thread_lock_timeout_usec))
            goto error;
    }

    base->reconnects = ov_linked_list_create(
        (ov_list_config){.item.free = ov_data_pointer_free});

    if (0 != config.cache.capacity)
        ov_linked_list_enable_caching(config.cache.capacity);

    ov_log_debug("IO base allocated %" PRIu64 " bytes supporting %" PRIu32 " so"
                 "cke"
                 "ts",
                 size, config.limit.max_sockets);

    return base;
error:
    ov_io_base_free(base);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_io_base *ov_io_base_free(ov_io_base *base) {

    if (!base)
        goto error;

    ov_event_loop *loop = base->config.loop;
    OV_ASSERT(loop);
    OV_ASSERT(loop->callback.unset);
    OV_ASSERT(loop->timer.unset);

    if (OV_TIMER_INVALID == base->timer.io_timeout_check) {
        loop->timer.unset(loop, base->timer.io_timeout_check, NULL);
        base->timer.io_timeout_check = OV_TIMER_INVALID;
    }

    for (size_t i = 0; i < base->connections_max; i++) {

        close_connection(&base->conn[i]);
        base->conn[i].out.queue = ov_list_free(base->conn[i].out.queue);
        ov_thread_lock_clear(&base->conn[i].lock);
    }

    base->domain.array =
        ov_domain_array_free(base->domain.size, base->domain.array);

    base->reconnects = ov_list_free(base->reconnects);

    base = ov_data_pointer_free(base);
    return NULL;

error:
    return base;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      #SSL FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

static ov_domain *find_domain(const ov_io_base *base, const uint8_t *name,
                              size_t len) {

    OV_ASSERT(base);
    OV_ASSERT(name);

    ov_domain *domain = NULL;

    for (size_t i = 0; i < base->domain.size; i++) {

        if (base->domain.array[i].config.name.length != len)
            continue;

        if (0 == memcmp(base->domain.array[i].config.name.start, name, len)) {
            domain = &base->domain.array[i];
            break;
        }
    }

    return domain;
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

    ov_io_base *base = ov_io_base_cast(arg);
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

        if (-1 == base->domain.standard) {
            domain = &base->domain.array[0];
        } else {
            domain = &base->domain.array[base->domain.standard];
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

static bool tls_init_context(ov_io_base *base, ov_domain *domain) {

    if (!base || !domain)
        goto error;

    if (!domain->context.tls) {
        ov_log_error("%s TLS context for |%s| not set", base->config.name,
                     domain->config.name);
        goto error;
    }

    SSL_CTX_set_client_hello_cb(domain->context.tls, tls_client_hello_callback,
                                base);

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_io_base_load_ssl_config(ov_io_base *base, const char *path) {

    if (!base || !path)
        return false;

    if (base->domain.array) {
        ov_log_error("%s cannot load SSL from %s - SSL already configured",
                     base->config.name, path);

        goto error;
    }

    if (!ov_domain_load(path, &base->domain.size, &base->domain.array)) {

        ov_log_error("%s failed to load domains from %s", base->config.name,
                     path);

        goto error;
    }

    /* Init additional (specific) SSL functions in domain context */

    base->domain.standard = -1;
    for (size_t i = 0; i < base->domain.size; i++) {

        if (!tls_init_context(base, &base->domain.array[i]))
            goto error;

        if (base->domain.array[i].config.is_default) {

            if (-1 != base->domain.standard) {
                ov_log_error("%s more than ONE domain configured as default");
                goto error;
            }

            base->domain.standard = i;
        }
    }

    return true;

error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool tls_perform_handshake(ov_io_base *base, Connection *conn) {

    OV_ASSERT(base);
    OV_ASSERT(conn);
    OV_ASSERT(conn->tls.ssl);

    char errorstring[OV_SSL_ERROR_STRING_BUFFER_SIZE];
    int errorcode = -1;
    int n = 0, r = 0;

    if (!base || !conn || !conn->tls.ssl)
        goto error;

    conn->stats.recv.last_usec = ov_time_get_current_time_usecs();

    /* Send some error SHUTDOWN SSL and send to remote */

    conn->flags = FLAG_SEND_SSL_SHUTDOWN;

    r = SSL_accept(conn->tls.ssl);

    if (1 == r) {

        const char *hostname =
            SSL_get_servername(conn->tls.ssl, TLSEXT_NAMETYPE_host_name);

        if (!hostname) {

            if (-1 == base->domain.standard) {
                conn->domain = &base->domain.array[0];
            } else {
                conn->domain = &base->domain.array[base->domain.standard];
            }

        } else {

            conn->domain =
                find_domain(base, (uint8_t *)hostname, strlen(hostname));
        }

        if (!conn->domain)
            goto error;

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

            ov_log_error("%s SSL_ERROR_SSL %s at socket %i", base->config.name,
                         errorstring, conn->fd);

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
    close_connection(conn);
    return false;
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

    if (-1 == conn->fd)
        goto error;

    conn->stats.recv.last_usec = ov_time_get_current_time_usecs();

    OV_ASSERT(conn->base);
    OV_ASSERT(conn->base->config.loop);
    OV_ASSERT(conn->tls.ssl);

    ov_io_base *base = conn->base;
    ov_event_loop *loop = base->config.loop;

    /* Send some error SHUTDOWN SSL and send to remote */
    conn->flags = FLAG_SEND_SSL_SHUTDOWN;

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
            conn->flags = 0;
            errorcode = ERR_get_error();
            ERR_error_string_n(errorcode, errorstring,
                               OV_SSL_ERROR_STRING_BUFFER_SIZE);
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
            conn->flags = 0;
            errorcode = ERR_get_error();
            ERR_error_string_n(errorcode, errorstring,
                               OV_SSL_ERROR_STRING_BUFFER_SIZE);
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
        loop->timer.set(loop, conn->config.client_connect_trigger_usec, conn,
                        perform_client_handshake_triggered);

    if (OV_TIMER_INVALID == conn->timer_id) {
        ov_log_error("Failed to reenable trigger "
                     "for SSL client handshake");
        goto error;
    }

    return true;

success:

    conn->tls.handshaked = true;
    callback_connection_success(conn);
    return true;

error:

    callback_connection_error(conn);
    close_connection(conn);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool init_ssl_client(Connection *conn) {

    int r = 0;

    char errorstring[OV_SSL_ERROR_STRING_BUFFER_SIZE];
    int errorcode = -1;

    OV_ASSERT(conn);

    if (conn->tls.ctx)
        goto error;

    if (conn->tls.ssl)
        goto error;

    /* init context */

    conn->tls.ctx = SSL_CTX_new(TLS_client_method());
    if (!conn->tls.ctx)
        goto cleanup;

    SSL_CTX_set_verify(conn->tls.ctx, SSL_VERIFY_PEER, NULL);

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
                         conn->fd, errorstring);
            goto cleanup;
        }

    } else {

        ov_log_error("%s SSL client %i without verify "
                     "- cannot verify any incoming certs - abort",
                     conn->base->config.name, conn->fd);

        goto cleanup;
    }

    /* init ssl */

    conn->tls.ssl = SSL_new(conn->tls.ctx);
    if (!conn->tls.ssl)
        goto cleanup;

    /* create read bio */

    if (1 != SSL_set_fd(conn->tls.ssl, conn->fd)) {
        ov_log_error("SSL_set_fd failed "
                     "at socket %i | %s",
                     conn->fd, errorstring);
        goto cleanup;
    }

    SSL_set_connect_state(conn->tls.ssl);
    SSL_CTX_set_client_hello_cb(conn->tls.ctx, tls_client_hello_cb, NULL);

    if (0 != conn->config.ssl.domain[0]) {

        if (1 != SSL_set_tlsext_host_name(conn->tls.ssl,
                                          (char *)conn->config.ssl.domain)) {

            ov_log_error("SSL set hostname failed "
                         "at socket %i | %s",
                         conn->fd, conn->config.ssl.domain);

            goto cleanup;
        }
    }

    tls_perform_client_handshake(conn);

    return true;

cleanup:

    if (conn->tls.ssl)
        SSL_free(conn->tls.ssl);

    if (conn->tls.ctx)
        SSL_CTX_free(conn->tls.ctx);

    conn->tls.ssl = NULL;
    conn->tls.ctx = NULL;

error:
    return false;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      SOCKET FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

bool ov_io_base_close(ov_io_base *base, int socket) {

    if (!base || (0 >= socket))
        return false;

    if (-1 != base->conn[socket].fd) {

        /* ensure to send SSL shutdown to remote (if conn is SSL) */
        base->conn[socket].flags = FLAG_SEND_SSL_SHUTDOWN;

        /* ensure no auto reconnects for external close  sockets */
        base->conn[socket].config.auto_reconnect = false;
        close_connection(&base->conn[socket]);
    }

    return true;
}

/*----------------------------------------------------------------------------*/

ov_io_base_statistics ov_io_base_get_statistics(ov_io_base *base, int socket) {

    if (!base || (0 > socket))
        return (ov_io_base_statistics){0};

    Connection *conn = &base->conn[socket];
    /* copy statistics */
    return conn->stats;
}

/*----------------------------------------------------------------------------*/

static bool io_send(ov_io_base *base, Connection *conn, ov_event_loop *loop,
                    bool (*send_out)(Connection *conn, ov_buffer **buffer),
                    bool (*callback)(int, uint8_t, void *)) {

    OV_ASSERT(base);
    OV_ASSERT(conn);
    OV_ASSERT(loop);
    OV_ASSERT(send_out);
    OV_ASSERT(callback);

    if (conn->out.buffer) {

        /* (1) Try to send any leftovers outgoing.
         *
         * This will either free or keep conn->out.buffer,
         * depending of successfull send or returned EAGAIN */

        if (!send_out(conn, &conn->out.buffer))
            goto error;

    } else {

        /* (2) Try to send next queued */

        ov_buffer *buffer = ov_buffer_cast(ov_list_queue_pop(conn->out.queue));

        if (NULL == buffer) {

            /*  Nothing more in queue,
             *  nothing leftover to be send,
             *  -> disable outgoing readiness listening */

            if (!loop->callback.set(loop, conn->fd,
                                    OV_EVENT_IO_CLOSE | OV_EVENT_IO_IN |
                                        OV_EVENT_IO_ERR,
                                    base, callback)) {

                ov_log_error("%s failed to reset events at %i",
                             base->config.name, conn->fd);

                goto error;
            }

        } else {

            bool r = send_out(conn, &buffer);

            if (!r) {

                /* close happened, free buffer popped */
                buffer = ov_buffer_free(buffer);
                goto error;

            } else {

                /* Buffer will be either freed or keeped,
                 * depending of successfull send or returned EAGAIN,
                 * so if the buffer was not send, we set it to be resend in
                 * next IO cylce */

                if (buffer)
                    conn->out.buffer = buffer;
            }
        }
    }

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool ssl_send(Connection *conn, ov_buffer **input) {

    char errorstring[OV_SSL_ERROR_STRING_BUFFER_SIZE];
    int errorcode = -1;

    OV_ASSERT(conn);
    OV_ASSERT(input);
    OV_ASSERT(*input);

    ov_buffer *buffer = *input;
    ov_io_base *base = conn->base;

    ssize_t bytes = SSL_write(conn->tls.ssl, buffer->start, buffer->length);

    if (0 == bytes)
        goto error;

    if (-1 == bytes) {

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
        case SSL_ERROR_WANT_ASYNC:
        case SSL_ERROR_WANT_ASYNC_JOB:
        case SSL_ERROR_WANT_CLIENT_HELLO_CB:
            ov_log_error("%s SSL return %i? Should never happen here",
                         base->config.name, r);
            OV_ASSERT(1 == 0);
            goto error;
            break;

        case SSL_ERROR_SYSCALL:

            /* close here again ? */
            if (bytes == 0)
                break;

            if (errno == EAGAIN)
                break;

            ov_log_error("%s SSL_ERROR_SYSCALL"
                         "%d | %s",
                         base->config.name, errno, strerror(errno));

            goto error;

        case SSL_ERROR_SSL:

            errorcode = ERR_get_error();
            ERR_error_string_n(errorcode, errorstring,
                               OV_SSL_ERROR_STRING_BUFFER_SIZE);

            ov_log_error("%s SSL_ERROR_SSL %s at socket %i", base->config.name,
                         errorstring, conn->fd);

            conn->flags = 0;
            goto error;
        }

        /* Try again */
        goto done;
    }

    OV_ASSERT(bytes == (ssize_t)buffer->length);

    conn->stats.send.last_usec = ov_time_get_current_time_usecs();
    conn->stats.send.bytes += bytes;

    buffer = ov_buffer_free(buffer);
    *input = NULL;

    if (conn->base->config.debug)
        ov_log_debug("%s send %zi bytes at %i to %s:%i",
                     conn->base->config.name, bytes, conn->fd,
                     conn->stats.remote.host, conn->stats.remote.port);

done:
    return true;
error:
    close_connection(conn);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool ssl_recv(Connection *conn) {

    char errorstring[OV_SSL_ERROR_STRING_BUFFER_SIZE] = {0};
    int errorcode = -1, n = 0;

    OV_ASSERT(conn);
    OV_ASSERT(0 < conn->in.recv_buffer_size);

    ov_io_base *base = conn->base;

    /* During connection init the buffer size was set for the maximum
     * amount of data of the network queue, so we use this information,
     * to transfer the data to the STACK and pass that pointer to the
     * processing function of userdata */

    uint8_t buffer[conn->in.recv_buffer_size];
    memset(buffer, 0, conn->in.recv_buffer_size);

    ssize_t bytes = -1;

    if (conn->in.offset) {

        /* some data we already sent to processing */

        bytes = SSL_read(conn->tls.ssl, buffer, conn->in.offset);
        ;

        if (0 == bytes) {
            close_connection(conn);
            goto error;
        }

        if (-1 == bytes) {

            n = SSL_get_error(conn->tls.ssl, bytes);
            switch (n) {

            case SSL_ERROR_NONE:
            case SSL_ERROR_WANT_READ:
            case SSL_ERROR_WANT_WRITE:
            case SSL_ERROR_WANT_CONNECT:
            case SSL_ERROR_WANT_ACCEPT:
            case SSL_ERROR_WANT_X509_LOOKUP:
                break;

            default:
                close_connection(conn);
                goto error;
            }

            goto done;
        }

        OV_ASSERT(bytes == conn->in.offset);
        memset(buffer, 0, conn->in.recv_buffer_size);
    }

    bytes = SSL_peek(conn->tls.ssl, buffer, conn->in.recv_buffer_size);

    if (0 == bytes) {

        close_connection(conn);
        goto error;
    }

    if (-1 == bytes) {

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

            ov_log_error("%s SSL_ERROR_SYSCALL"
                         "%d | %s",
                         base->config.name, errno, strerror(errno));

            goto error;
            break;

        case SSL_ERROR_SSL:

            errorcode = ERR_get_error();
            ERR_error_string_n(errorcode, errorstring,
                               OV_SSL_ERROR_STRING_BUFFER_SIZE);
            ov_log_error("%s SSL_ERROR_SSL %s at socket %i", base->config.name,
                         errorstring, conn->fd);

            conn->flags = 0;
            goto error;
            break;

        default:
            goto error;
            break;
        }

        goto done;
    }

    /* Try to perform callback,
     * which MAY block due to (optional) thread lock of userdata */

    if (conn->config.connection.callback.io(
            conn->config.connection.callback.userdata, conn->fd,
            (ov_memory_pointer){.start = buffer, .length = bytes})) {

        if (conn->base->config.debug)
            ov_log_debug("%s processed %zi data from %i",
                         conn->base->config.name, bytes, conn->fd);

        /* data processed, actually read the amount of bytes consumed */
        ssize_t clear = SSL_read(conn->tls.ssl, buffer, bytes);

        if (0 == clear) {
            close_connection(conn);
            goto error;
        }

        if (-1 == clear) {

            n = SSL_get_error(conn->tls.ssl, bytes);
            switch (n) {

            case SSL_ERROR_NONE:
            case SSL_ERROR_WANT_READ:
            case SSL_ERROR_WANT_WRITE:
            case SSL_ERROR_WANT_CONNECT:
            case SSL_ERROR_WANT_ACCEPT:
            case SSL_ERROR_WANT_X509_LOOKUP:
                break;

            default:
                close_connection(conn);
                goto error;
            }

            conn->in.offset = bytes;
        }

        OV_ASSERT(clear == bytes);

        if (clear < bytes) {
            conn->in.offset = bytes - clear;
        } else {
            conn->in.offset = 0;
        }

        conn->stats.recv.last_usec = ov_time_get_current_time_usecs();
        conn->stats.recv.bytes += bytes;

    } else {

        ov_log_debug("%s processing of %zi data from %i - delayed",
                     conn->base->config.name, bytes, conn->fd);

        /* callback is set, but was not executed, so we keep the
         * data in the network queue and let the eventloop retrigger.
         * NOTE This will only work with level triggered event loops */

        if (conn->base->config.debug)
            ov_log_debug("%s leave data in network queue due to "
                         "processing failure at %i "
                         "- level triggered event loop required",
                         conn->base->config.name, conn->fd);
    }

done:
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool io_ssl_client(int socket, uint8_t events, void *data) {

    ov_event_loop *loop = NULL;
    Connection *conn = NULL;
    ov_io_base *base = ov_io_base_cast(data);

    OV_ASSERT(base);
    OV_ASSERT(0 < socket);

    if (!base)
        goto error;

    conn = &base->conn[socket];
    OV_ASSERT(conn->fd == socket);
    OV_ASSERT(base->config.loop);
    loop = base->config.loop;

    if ((events & OV_EVENT_IO_CLOSE) || (events & OV_EVENT_IO_ERR)) {

        if (base->config.debug)
            ov_log_debug("%s ssl close at %i remote %s:%i", base->config.name,
                         conn->fd, conn->stats.remote.host,
                         conn->stats.remote.port);

        conn->flags = 0;
        close_connection(conn);
        return true;
    }

    if (!conn->tls.handshaked)
        return tls_perform_client_handshake(conn);

    if (events & OV_EVENT_IO_IN) {

        return ssl_recv(conn);

    } else if (events & OV_EVENT_IO_OUT) {

        if (!io_send(base, conn, loop, ssl_send, io_ssl_client))
            goto error;

    } else {

        ov_log_error("%s unexpected event IO", base->config.name);
        OV_ASSERT(1 == 0);
        goto error;
    }

error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool io_ssl(int socket, uint8_t events, void *data) {

    ov_event_loop *loop = NULL;
    Connection *conn = NULL;
    ov_io_base *base = ov_io_base_cast(data);

    OV_ASSERT(base);
    OV_ASSERT(0 < socket);

    if (!base)
        goto error;

    conn = &base->conn[socket];
    OV_ASSERT(conn->fd == socket);
    OV_ASSERT(base->config.loop);
    loop = base->config.loop;

    if ((events & OV_EVENT_IO_CLOSE) || (events & OV_EVENT_IO_ERR)) {

        if (base->config.debug)
            ov_log_debug("%s ssl close at %i remote %s:%i", base->config.name,
                         conn->fd, conn->stats.remote.host,
                         conn->stats.remote.port);

        conn->flags = 0;
        close_connection(conn);
        return true;
    }

    if (!conn->tls.handshaked)
        return tls_perform_handshake(base, conn);

    if (events & OV_EVENT_IO_IN) {

        return ssl_recv(conn);

    } else if (events & OV_EVENT_IO_OUT) {

        if (!io_send(base, conn, loop, ssl_send, io_ssl))
            goto error;

    } else {

        ov_log_error("%s unexpected event IO", base->config.name);
        OV_ASSERT(1 == 0);
        goto error;
    }

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool stream_send(Connection *conn, ov_buffer **input) {

    OV_ASSERT(conn);
    OV_ASSERT(input);
    OV_ASSERT(*input);

    ov_buffer *buffer = *input;

    ssize_t bytes = send(conn->fd, buffer->start, buffer->length, 0);

    if (0 == bytes)
        goto error;

    if (-1 == bytes) {

        if (errno != EAGAIN)
            goto error;

        goto done;
    }

    OV_ASSERT(bytes == (ssize_t)buffer->length);

    conn->stats.send.last_usec = ov_time_get_current_time_usecs();
    conn->stats.send.bytes += bytes;

    buffer = ov_buffer_free(buffer);
    *input = NULL;

    if (conn->base->config.debug)
        ov_log_debug("%s send %zi bytes at %i to %s:%i",
                     conn->base->config.name, bytes, conn->fd,
                     conn->stats.remote.host, conn->stats.remote.port);

done:
    return true;
error:
    close_connection(conn);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool stream_recv_buffered(Connection *conn) {

    OV_ASSERT(conn);
    OV_ASSERT(0 < conn->in.recv_buffer_size);

    /* During connection init the buffer size was set for the maximum
     * amount of data of the network queue, so we use this information,
     * to transfer the data to the STACK and pass that pointer to the
     * processing function of userdata */

    uint8_t buffer[conn->in.recv_buffer_size];
    memset(buffer, 0, conn->in.recv_buffer_size);

    ssize_t bytes = 0;

    if (conn->in.offset) {

        /* some data we already sent to processing */

        bytes = recv(conn->fd, buffer, conn->in.offset, 0);

        if (0 == bytes) {
            close_connection(conn);
            goto error;
        }

        if (-1 == bytes) {

            if (errno != EAGAIN) {
                close_connection(conn);
                goto error;
            }

            goto done;
        }

        // OV_ASSERT(bytes == conn->in.offset);
        memset(buffer, 0, conn->in.recv_buffer_size);
    }

    bytes = recv(conn->fd, buffer, conn->in.recv_buffer_size, MSG_PEEK);

    if (0 == bytes) {
        close_connection(conn);
        goto error;
    }

    if (-1 == bytes) {

        if (errno != EAGAIN) {
            close_connection(conn);
            goto error;
        }

        goto done;
    }

    /* Try to perform callback,
     * which MAY block due to (optional) thread lock of userdata */

    if (conn->config.connection.callback.io(
            conn->config.connection.callback.userdata, conn->fd,
            (ov_memory_pointer){.start = buffer, .length = bytes})) {

        if (conn->base->config.debug)
            ov_log_debug("%s processed %zi data from %i",
                         conn->base->config.name, bytes, conn->fd);

        /* data processed, actually read the amount of bytes consumed */
        ssize_t clear = recv(conn->fd, buffer, bytes, 0);

        if (0 == clear) {
            close_connection(conn);
            goto error;
        }

        if (-1 == clear) {
            conn->in.offset = bytes;
            goto done;
        }

        OV_ASSERT(clear == bytes);

        if (clear < bytes) {
            conn->in.offset = bytes - clear;
        } else {
            conn->in.offset = 0;
        }

        conn->stats.recv.last_usec = ov_time_get_current_time_usecs();
        conn->stats.recv.bytes += bytes;

    } else {

        ov_log_debug("%s processing of %zi data from %i - delayed",
                     conn->base->config.name, bytes, conn->fd);

        /* callback is set, but was not executed, so we keep the
         * data in the network queue and let the eventloop retrigger.
         * NOTE This will only work with level triggered event loops */

        if (conn->base->config.debug)
            ov_log_debug("%s leave data in network queue due to "
                         "processing failure at %i "
                         "- level triggered event loop required",
                         conn->base->config.name, conn->fd);
    }

done:
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool stream_recv_unbuffered(Connection *conn) {

    OV_ASSERT(conn);
    OV_ASSERT(0 < conn->in.recv_buffer_size);

    /* During connection init the buffer size was set for the maximum
     * amount of data of the network queue, so we use this information,
     * to transfer the data to the STACK and pass that pointer to the
     * processing function of userdata */

    uint8_t buffer[conn->in.recv_buffer_size];
    memset(buffer, 0, conn->in.recv_buffer_size);

    ssize_t bytes = 0;

    bytes = recv(conn->fd, buffer, conn->in.recv_buffer_size, 0);

    if (0 == bytes) {

        close_connection(conn);
        return false;

    } else if (-1 == bytes) {

        if (errno != EAGAIN) {

            close_connection(conn);
            return false;

        } else {

            return true;
        }

    } else {

        conn->stats.recv.last_usec = ov_time_get_current_time_usecs();
        conn->stats.recv.bytes += bytes;

        /* Try to perform callback,
         * which MAY block due to (optional) thread lock of userdata */

        if (conn->config.connection.callback.io(
                conn->config.connection.callback.userdata, conn->fd,
                (ov_memory_pointer){.start = buffer, .length = bytes}) &&
            conn->base->config.debug) {
            ov_log_debug("%s processed %zi data from %i",
                         conn->base->config.name, bytes, conn->fd);
        }

        return true;
    }
}

/*----------------------------------------------------------------------------*/

static bool stream_recv(Connection *conn) {

    OV_ASSERT(conn);

    if (conn->use_socket_buffer) {
        return stream_recv_buffered(conn);
    } else {
        return stream_recv_unbuffered(conn);
    }
}

/*----------------------------------------------------------------------------*/

static bool io_stream(int socket, uint8_t events, void *data) {

    ov_event_loop *loop = NULL;
    Connection *conn = NULL;
    ov_io_base *base = ov_io_base_cast(data);

    OV_ASSERT(base);
    OV_ASSERT(0 < socket);

    if (!base)
        goto error;

    conn = &base->conn[socket];
    OV_ASSERT(conn->fd == socket);
    OV_ASSERT(base->config.loop);
    loop = base->config.loop;

    if ((events & OV_EVENT_IO_CLOSE) || (events & OV_EVENT_IO_ERR)) {

        if (base->config.debug)
            ov_log_debug("%s stream close at %i remote %s:%i",
                         base->config.name, conn->fd, conn->stats.remote.host,
                         conn->stats.remote.port);

        close_connection(conn);
        return true;
    }

    if (events & OV_EVENT_IO_IN) {

        return stream_recv(conn);

    } else if (events & OV_EVENT_IO_OUT) {

        if (!io_send(base, conn, loop, stream_send, io_stream))
            goto error;

    } else {

        ov_log_error("%s unexpected event IO", base->config.name);
        OV_ASSERT(1 == 0);
        goto error;
    }

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static Connection *init_connection(ov_io_base *base, Connection *listener,
                                   int socket) {

    OV_ASSERT(socket >= 0);
    OV_ASSERT(listener);
    OV_ASSERT(base);

    int nfd = -1;

    struct sockaddr_storage sa = {0};
    socklen_t sa_len = sizeof(sa);

    nfd = accept(socket, (struct sockaddr *)&sa, &sa_len);

    if (nfd < 0) {

        ov_log_error("%s failed to accept at socket %i", base->config.name,
                     socket);

        goto error;
    }

    if (nfd >= (int)base->config.limit.max_sockets) {

        ov_log_error("%s fd %i max_sockets %" PRIu32, base->config.name, socket,
                     base->config.limit.max_sockets);

        close(nfd);
        goto error;
    }

    if (!ov_socket_ensure_nonblocking(nfd))
        goto error;

    if (NULL != listener->listener.config.callback.accept) {

        if (!listener->listener.config.callback.accept(
                listener->listener.config.callback.userdata, socket, nfd))
            goto error;
    }

    /*
     *      Create the connection
     */

    OV_ASSERT(-1 == base->conn[nfd].fd);

    if (-1 != base->conn[nfd].fd)
        goto error;

    Connection *conn = &base->conn[nfd];

    conn->type = CONNECTION;
    conn->base = base;
    conn->fd = nfd;
    conn->listener.fd = socket;

    conn->stats.created_usec = ov_time_get_current_time_usecs();
    conn->stats.remote = ov_socket_data_from_sockaddr_storage(&sa);
    conn->stats.type = listener->listener.config.socket.type;

    /* set configured callbacks */

    conn->config.connection.callback.userdata =
        listener->listener.config.callback.userdata;
    conn->config.connection.callback.io = listener->listener.config.callback.io;
    conn->config.connection.callback.close =
        listener->listener.config.callback.close;

    switch (conn->stats.type) {

    case TCP:
    case LOCAL:

        /* use of actual MAX network queue buffer of the OS */

        conn->in.recv_buffer_size = ov_socket_get_recv_buffer_size(conn->fd);
        if (-1 == conn->in.recv_buffer_size)
            goto error;

        break;

    case TLS:
    case DTLS:

        /* use of MAY buffer for SSL in case SSL is queuing or delaying
         * something */

        conn->in.recv_buffer_size = OV_SSL_MAX_BUFFER;
        break;

    default:
        goto error;
    }

    return conn;
error:
    close(nfd);
    return NULL;
}

/*----------------------------------------------------------------------------*/

static Connection *accept_stream_base(int socket, uint8_t events,
                                      void *userdata) {

    OV_ASSERT(userdata);

    Connection *listener = (Connection *)userdata;
    if (!listener)
        goto error;

    OV_ASSERT(listener->base);
    OV_ASSERT(listener->base->config.loop);

    ov_io_base *base = listener->base;

    if ((events & OV_EVENT_IO_CLOSE) || (events & OV_EVENT_IO_ERR)) {
        ov_io_base_close(base, socket);
        goto error;
    }

    // accept MUST have some incoming IO
    if (!(events & OV_EVENT_IO_IN))
        goto error;

    return init_connection(base, listener, socket);
error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

static void log_accepted(ov_io_base *base, int socket) {

    OV_ASSERT(base);
    OV_ASSERT(socket >= 0);

    ov_socket_data local = {0};
    ov_socket_data remote = {0};

    if (!ov_socket_get_data(socket, &local, &remote)) {
        ov_log_error("%s failed to parse data at accepted socket "
                     "%i",
                     base->config.name, socket);
        goto error;
    }

    ov_log_debug("%s accepted connection socket %i at %s:%i from %s:%i",
                 base->config.name, socket, local.host, local.port, remote.host,
                 remote.port);

error:
    return;
}

/*----------------------------------------------------------------------------*/

static bool accept_stream(int socket, uint8_t events, void *userdata) {

    Connection *conn = accept_stream_base(socket, events, userdata);
    if (!conn)
        goto error;

    ov_io_base *base = conn->base;
    ov_event_loop *loop = base->config.loop;

    if (!loop->callback.set(loop, conn->fd,
                            OV_EVENT_IO_IN | OV_EVENT_IO_ERR |
                                OV_EVENT_IO_CLOSE,
                            base, io_stream))
        goto error;

    if (base->config.debug)
        log_accepted(base, conn->fd);

    return true;

error:

    if (conn)
        close_connection(conn);

    return false;
}

/*----------------------------------------------------------------------------*/

static bool accept_tls(int socket, uint8_t events, void *userdata) {

    SSL *ssl = NULL;

    Connection *conn = accept_stream_base(socket, events, userdata);
    if (!conn)
        goto error;

    ov_io_base *base = conn->base;
    ov_event_loop *loop = base->config.loop;

    /*
     *      Create SSL for the first domain,
     *      MAY change during SSL setup.
     */

    int id = 0;
    if (-1 != base->domain.standard)
        id = base->domain.standard;

    ssl = SSL_new(base->domain.array[id].context.tls);
    if (!ssl)
        goto error;

    if (1 != SSL_set_fd(ssl, conn->fd))
        goto error;

    SSL_set_accept_state(ssl);

    if (!loop->callback.set(
            loop, conn->fd,
            OV_EVENT_IO_IN | OV_EVENT_IO_ERR | OV_EVENT_IO_CLOSE, base, io_ssl))
        goto error;

    if (base->config.debug)
        log_accepted(base, conn->fd);

    conn->tls.handshaked = false;
    conn->tls.ssl = ssl;
    conn->stats.type = TLS;

    return true;

error:

    if (ssl)
        SSL_free(ssl);

    if (conn)
        close_connection(conn);

    return false;
}

/*----------------------------------------------------------------------------*/

int ov_io_base_create_listener(ov_io_base *base,
                               ov_io_base_listener_config config) {

    if (!base)
        goto error;

    if (!config.callback.io) {
        ov_log_error("Listener without IO callback unsupported.");
        goto error;
    }

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
        ov_log_error("Socket type unsupported. %i", config.socket.type);
        goto error;
    }

    int socket = ov_socket_create(config.socket, false, NULL);
    if (-1 == socket) {
        ov_log_error("failed to create socket %s:%i %i|%s", config.socket.host,
                     config.socket.port, errno, strerror(errno));
    }

    if (!ov_socket_ensure_nonblocking(socket))
        goto error;

    ov_event_loop *loop = base->config.loop;

    Connection *conn = &base->conn[socket];
    if (-1 != conn->fd)
        goto error;

    conn->type = LISTENER;
    conn->base = base;
    conn->fd = socket;
    conn->listener.fd = -1;
    conn->listener.config = config;

    if (!loop->callback.set(
            loop, socket, OV_EVENT_IO_IN | OV_EVENT_IO_ERR | OV_EVENT_IO_CLOSE,
            conn, accept_handler))
        goto error;

    ov_log_debug("%s created listener %s:%i", base->config.name,
                 config.socket.host, config.socket.port);

    return socket;

error:
    return -1;
}

/*----------------------------------------------------------------------------*/

int ov_io_base_create_connection(ov_io_base *base,
                                 ov_io_base_connection_config config) {

    if (!base)
        goto error;

    if (!config.connection.callback.io) {
        ov_log_error("Connection without IO callback unsupported.");
        goto error;
    }

    bool (*io)(int socket, uint8_t events, void *data) = NULL;

    if (0 == config.client_connect_trigger_usec)
        config.client_connect_trigger_usec = 3000000;

    switch (config.connection.socket.type) {

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

    int socket = ov_socket_create(config.connection.socket, true, NULL);
    if (-1 == socket) {

        if (config.auto_reconnect) {

            ov_io_base_connection_config *conf =
                calloc(1, sizeof(ov_io_base_connection_config));

            *conf = config;
            if (!ov_list_queue_push(base->reconnects, conf)) {

                conf = ov_data_pointer_free(conf);
                ov_log_error("failed to enable auto reconnect");
            }

        } else {

            ov_log_error("failed to create socket %s:%i %i|%s",
                         config.connection.socket.host,
                         config.connection.socket.port, errno, strerror(errno));
        }

        goto error;
    }

    if (!ov_socket_ensure_nonblocking(socket)) {
        close(socket);
        goto error;
    }

    ov_event_loop *loop = base->config.loop;

    /* init connection */
    Connection *conn = &base->conn[socket];
    if (conn->fd != -1) {
        close(socket);
        goto error;
    }

    /* From here use cleanup in case of error, to unset everything at conn */

    conn->fd = socket;
    conn->listener.fd = -1;
    conn->config = config;
    conn->type = CLIENT;
    conn->stats.type = config.connection.socket.type;
    conn->stats.created_usec = ov_time_get_current_time_usecs();

    if (TLS == config.connection.socket.type) {

        if (!init_ssl_client(conn))
            goto cleanup;
    }

    if (!loop->callback.set(
            loop, socket, OV_EVENT_IO_IN | OV_EVENT_IO_ERR | OV_EVENT_IO_CLOSE,
            base, io))
        goto cleanup;

    /* set recv buffer size once socket was created */

    switch (config.connection.socket.type) {

    case TCP:
    case LOCAL:

        /* use of actual MAX network queue buffer of the OS */

        conn->in.recv_buffer_size = ov_socket_get_recv_buffer_size(conn->fd);
        if (-1 == conn->in.recv_buffer_size)
            goto cleanup;

        callback_connection_success(conn);
        break;

    case TLS:

        /* use of MAY buffer for SSL in case SSL is queuing or delaying
         * something */

        conn->in.recv_buffer_size = OV_SSL_MAX_BUFFER;
        break;

    default:
        goto cleanup;
    }

    ov_log_debug("%s created connection socket %i to %s:%i", base->config.name,
                 socket, config.connection.socket.host,
                 config.connection.socket.port);

    return socket;

cleanup:
    callback_connection_error(conn);
    OV_ASSERT(conn);
    close_connection(conn);
error:
    return -1;
}

/*----------------------------------------------------------------------------*/

bool ov_io_base_send(ov_io_base *base, int socket,
                     const ov_memory_pointer buffer) {

    bool result = false;

    if (!base || (0 > socket) || !buffer.start || (0 == buffer.length))
        goto error;

    if (socket > (int)base->connections_max)
        goto error;

    Connection *conn = &base->conn[socket];

    if (conn->fd == -1)
        goto error;

    if ((conn->type != CONNECTION) && (conn->type != CLIENT))
        goto error;

    OV_ASSERT(conn->fd == socket);

    /* SSL_write require some retry with the same pointer,
     * so some copy is required for SSL functionality at all.
     *
     * To hand back to some thread as fast as possible,
     * we do not even try to send here,
     * but schedule the connection socket for OUTGOING
     * event listening and let actual transmission be done,
     * whenever convinient for the event loop. */

    bool (*send_out)(int, uint8_t, void *) = NULL;
    size_t last = ov_list_count(conn->out.queue);

    ov_buffer *copy = NULL;

    switch (conn->stats.type) {

    case TCP:
    case LOCAL:

        send_out = io_stream;
        break;

    case TLS:

        send_out = io_ssl;

        if (!conn->tls.handshaked)
            goto error;

        break;

    default:

        /* should never be reached */
        OV_ASSERT(1 == 0);
        goto error;
        break;
    }

    size_t max = ov_socket_get_send_buffer_size(conn->fd);

    if (0 != base->config.limit.max_send)
        if (base->config.limit.max_send < max)
            max = base->config.limit.max_send;

    if (!ov_thread_lock_try_lock(&conn->lock))
        goto error;

    if (max < buffer.length) {

        /* Split buffer in chunks of max */

        ssize_t open = buffer.length;
        const uint8_t *ptr = buffer.start;
        size_t len = 0;

        while (open > 0) {

            copy = ov_buffer_create(max);
            if (!copy)
                goto cleanup;

            if (open > (ssize_t)max) {
                len = max;
            } else {
                len = (size_t)open;
            }

            if (!ov_buffer_set(copy, ptr, len)) {
                copy = ov_buffer_free(copy);
                goto cleanup;
            }

            if (!ov_list_queue_push(conn->out.queue, copy)) {
                copy = ov_buffer_free(copy);
                goto cleanup;
            }

            copy = NULL;
            open = open - max;

            if (open > 0)
                ptr = ptr + max;
        }

    } else {

        copy = ov_buffer_create(buffer.length);

        if (!ov_buffer_set(copy, buffer.start, buffer.length)) {
            copy = ov_buffer_free(copy);
            goto cleanup;
        }

        if (!ov_list_queue_push(conn->out.queue, copy)) {
            copy = ov_buffer_free(copy);
            goto cleanup;
        }
    }

    ov_event_loop *loop = conn->base->config.loop;

    result = loop->callback.set(loop, conn->fd,
                                OV_EVENT_IO_CLOSE | OV_EVENT_IO_IN |
                                    OV_EVENT_IO_ERR | OV_EVENT_IO_OUT,
                                conn->base, send_out);

    // force outgoing sending readiness
    send_out(conn->fd, OV_EVENT_IO_OUT, conn->base);

cleanup:

    if (false == result) {

        /* undo all added on error */

        size_t items = ov_list_count(conn->out.queue);
        items -= last;

        for (size_t i = 1; i <= items; i++) {

            if (!ov_list_delete(conn->out.queue, i)) {

                OV_ASSERT(1 == 0);
                ov_thread_lock_unlock(&conn->lock);
                close_connection(conn);
                break;
            }
        }
    }

    if (!ov_thread_lock_unlock(&conn->lock)) {
        OV_ASSERT(1 == 0);
    }

    return result;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

ov_io_base_config ov_io_base_config_from_json(const ov_json_value *input) {

    ov_io_base_config out = {0};

    const ov_json_value *config = ov_json_object_get(input, OV_IO_BASE_KEY);

    if (!config)
        config = input;

    if (ov_json_is_true(ov_json_object_get(config, OV_KEY_DEBUG)))
        out.debug = true;

    const char *name =
        ov_json_string_get(ov_json_object_get(config, OV_KEY_NAME));
    if (name)
        strncpy(out.name, name, OV_IO_BASE_NAME_MAX);

    ov_json_value *limit = ov_json_object_get(config, OV_KEY_LIMIT);
    if (limit) {
        out.limit.max_sockets =
            ov_json_number_get(ov_json_object_get(limit, OV_KEY_SOCKETS));
        out.limit.max_send =
            ov_json_number_get(ov_json_object_get(limit, OV_KEY_SEND));
    }

    limit = ov_json_object_get(config, OV_KEY_TIMER);
    if (limit) {
        out.timer.io_timeout_usec =
            ov_json_number_get(ov_json_object_get(limit, OV_KEY_TIMEOUT_USEC));
        out.timer.accept_to_io_timeout_usec = ov_json_number_get(
            ov_json_object_get(limit, OV_KEY_ACCEPT_TIMEOUT_USEC));
        out.timer.reconnect_interval_usec = ov_json_number_get(
            ov_json_object_get(limit, OV_KEY_RECONNECT_USEC));
    }

    out.cache.capacity = ov_json_number_get(ov_json_object_get(
        ov_json_object_get(config, OV_KEY_CACHE), OV_KEY_CAPACITY));

    return out;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_io_base_config_to_json(const ov_io_base_config config) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;

    out = ov_json_object();

    if (config.debug) {
        val = ov_json_true();
    } else {
        val = ov_json_false();
    }

    if (!ov_json_object_set(out, OV_KEY_DEBUG, val))
        goto error;

    val = ov_json_object();
    if (!ov_json_object_set(out, OV_KEY_LIMIT, val))
        goto error;

    ov_json_value *tmp = val;
    val = NULL;

    val = ov_json_number(config.limit.max_sockets);
    if (!ov_json_object_set(tmp, OV_KEY_SOCKETS, val))
        goto error;

    val = ov_json_number(config.limit.max_send);
    if (!ov_json_object_set(tmp, OV_KEY_SEND, val))
        goto error;

    val = ov_json_object();
    if (!ov_json_object_set(out, OV_KEY_TIMER, val))
        goto error;

    tmp = val;
    val = NULL;

    val = ov_json_number(config.timer.io_timeout_usec);
    if (!ov_json_object_set(tmp, OV_KEY_TIMEOUT_USEC, val))
        goto error;

    val = ov_json_number(config.timer.accept_to_io_timeout_usec);
    if (!ov_json_object_set(tmp, OV_KEY_ACCEPT_TIMEOUT_USEC, val))
        goto error;

    val = ov_json_number(config.timer.reconnect_interval_usec);
    if (!ov_json_object_set(tmp, OV_KEY_RECONNECT_USEC, val))
        goto error;

    val = ov_json_object();
    if (!ov_json_object_set(out, OV_KEY_CACHE, val))
        goto error;

    tmp = val;
    val = ov_json_number(config.cache.capacity);
    if (!ov_json_object_set(tmp, OV_KEY_CAPACITY, val))
        goto error;

    return out;
error:
    ov_json_value_free(out);
    ov_json_value_free(val);
    return NULL;
}
