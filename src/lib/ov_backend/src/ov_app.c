/***
        ------------------------------------------------------------------------

        Copyright (c) 2019 German Aerospace Center DLR e.V. (GSOC)

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
        @author         Markus Toepfer
        @author         Michael Beer

        @date           2020-04-08

        ------------------------------------------------------------------------
*/

#include "../include/ov_app.h"

#include <libgen.h> /* for basename(3) (POSIX) */
#include <unistd.h>

#include <ov_base/ov_utils.h>

#include <ov_base/ov_buffer.h>
#include <ov_base/ov_cache.h>
#include <ov_base/ov_config.h>
#include <ov_base/ov_config_keys.h>
#include <ov_base/ov_constants.h>
#include <ov_base/ov_dict.h>
#include <ov_base/ov_file.h>
#include <ov_base/ov_linked_list.h>
#include <ov_base/ov_node.h>
#include <ov_base/ov_socket.h>

#include <ov_base/ov_id.h>
#include <ov_base/ov_reconnect_manager.h>
#include <ov_base/ov_utils.h>

#define IMPL_APP_TYPE 0x0001

#define IMPL_LISTENER_MAGIC_BYTES 0xAAAA
#define IMPL_CONNECTION_MAGIC_BYTES 0xCCCC

#define IMPL_DICT_SLOTS 255
#define IMPL_READ_BUFFER_SIZE 2048
#define IMPL_UUID_CAPACITY 40
#define IMPL_5TUPLE_MAX (2 * OV_HOST_NAME_MAX) + 20

#define CONNECTION_KEY_LENGTH OV_HOST_NAME_MAX + 10

/*----------------------------------------------------------------------------*/

typedef struct DefaultApp DefaultApp;

/*----------------------------------------------------------------------------*/

struct socket_node {

    ov_node node;
    intptr_t socket;
};

/*----------------------------------------------------------------------------*/

typedef struct {

    uint32_t magic_bytes;
    DefaultApp *app;
    ov_app_socket_config scfg;

} ReconnectData;

/*----------------------------------------------------------------------------*/

typedef struct {

    bool in_use;
    ReconnectData rd;

} ReconnectDataEntry;

/*----------------------------------------------------------------------------*/

ov_app *ov_app_cast(const void *self) {

    if (!self) goto error;

    if (*(uint16_t *)self == OV_APP_MAGIC_BYTE) return (ov_app *)self;
error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

struct DefaultApp {

    ov_app public; // public function interface and config

    ov_dict *uuids;   // UUID dict
    ov_dict *sockets; // sockets dict - guaranteed that it only contains TCP

    ov_reconnect_manager *reconnect;
    /* We need to track the reconnect data in use
     * in order to free them at program termination.
     * Technically, this would be unneccessary since
     * on process termination, all mem is freed anyhow.
     * But it spoils reports of tools like valgrind
     * which in turns makes it hard to track down
     * 'real' memleaks.
     */
    ReconnectDataEntry *reconnect_data;

    // Socket pair to trigger parse_again if there is more data ready after
    // successful parse on a connection
    // Transmits the socket number
    // Callback looks for the socket number in self->sockets and triggers
    // Parsing of the available data
    struct {
        int in;
        int out;
    } trigger_parse_again;

    ov_parser parser_buffer_switch;
};

/*----------------------------------------------------------------------------*/

/* Manage TCP server sockets */
typedef struct {

    uint16_t magic_bytes;

    DefaultApp *app; // backpointer to app

    ov_app_socket_config config; // config applied during open

    int socket; // IO socket

    ov_dict *connections; // dict for DTLS connections

} AppListener;

/*----------------------------------------------------------------------------*/

/* Manage TCP client sockets */
typedef struct {

    uint16_t magic_bytes;

    ov_app_socket_config config; // app socket config

    DefaultApp *app; // backpointer to app

    bool server; // true for UDP / DTLS listener
    bool child;
    bool uuid_skip;

    char uuid[IMPL_UUID_CAPACITY]; // dedicated UUID
    int socket;                    // IO socket
    int master_socket;             // listener master

    ov_socket_data local;  // parsed local socket data
    ov_socket_data remote; // parsed remote socket data

    ov_parser_data data; // dedicated ov_parser_data
    ov_parser *parser;   // dedicated parser instance

    uint64_t last_io_in; // last io incoming seen

    /*
     *      Callback pointer slots for connection
     *      setup over SSL
     */
    void (*success)(int socket, void *self);
    void (*failure)(int socket, void *failure);

} AppConnection;

/*----------------------------------------------------------------------------*/

#define AS_IMPL_APP(x)                                                         \
    (((ov_app_cast(x) != 0) && (IMPL_APP_TYPE == ((ov_app *)x)->type))         \
         ? (DefaultApp *)(x)                                                   \
         : 0);

/*----------------------------------------------------------------------------*/

static ov_app *impl_app_free(ov_app *self);

static bool impl_app_socket_open(ov_app *self,
                                 ov_app_socket_config config,
                                 void (*success_callback)(int socket,
                                                          void *self),
                                 void (*failure_callback)(int socket,
                                                          void *self));

static bool impl_app_socket_add(ov_app *self,
                                ov_app_socket_config config,
                                int socket);

static bool impl_app_socket_close(ov_app *self, int socket);

/*----------------------------------------------------------------------------*/

static bool socket_is_contained(ov_app *self, int socket) {

    intptr_t id = socket;
    DefaultApp *app = AS_IMPL_APP(self);

    if (!app || socket < 0) return false;

    return ov_dict_is_set(app->sockets, (void *)id);
}

/*----------------------------------------------------------------------------*/

static bool impl_app_connection_close(ov_app *self, const char *uuid);
static int impl_app_connection_get_socket(ov_app *self, const char *uuid);
static bool impl_app_connection_close_all(ov_app *self);

static bool impl_app_send(ov_app *self,
                          int socket,
                          const ov_socket_data *remote,
                          void *data);

/*----------------------------------------------------------------------------*/

static void *app_socket_data_free(void *);

static ov_dict_config app_socket_data_dict_config(size_t slots) {

    ov_dict_config config = ov_dict_intptr_key_config(slots);
    config.value.data_function.free = app_socket_data_free;
    return config;
}

static ov_dict_config app_uuids_dict_config(size_t slots) {

    ov_dict_config config = ov_dict_string_key_config(slots);
    return config;
}

static void *app_connection_free(void *self);

/*----------------------------------------------------------------------------*/

static bool trigger_parse_again(AppConnection *connection) {

    if (0 == connection) {
        goto error;
    }

    if (!connection->parser->buffer.is_enabled(connection->parser)) {
        return true;
    }

    if (!connection->parser->buffer.has_data(connection->parser)) {
        return true;
    }

    DefaultApp *app = connection->app;

    if (0 >= app->trigger_parse_again.in) {
        ov_log_error("Trigger in socket not initialized");
        goto error;
    }

    int csock = connection->socket;

    ssize_t octets = write(app->trigger_parse_again.in, &csock, sizeof(csock));

    if (sizeof(csock) != octets) {
        ov_log_error("Could not write to trigger socket: %s", strerror(errno));
        goto error;
    }

    return true;

error:

    return false;
}

/*----------------------------------------------------------------------------*/

static bool app_connection_use_parser(AppConnection *connection);

static bool cb_parse_again(int trigger_fd, uint8_t events, void *data) {

    UNUSED(events);

    DefaultApp *app = AS_IMPL_APP(data);

    if (0 == app) {
        goto error;
    }

    if (0 != (events & OV_EVENT_IO_CLOSE)) {
        ov_log_error("Trigger socket closed");
        return false;
    }

    if (0 != (events & OV_EVENT_IO_ERR)) {
        ov_log_error("Trigger socket got error");
        return false;
    }

    OV_ASSERT(0 == (events & OV_EVENT_IO_CLOSE));
    OV_ASSERT(0 == (events & OV_EVENT_IO_ERR));

    if (0 == (events & OV_EVENT_IO_IN)) {
        ov_log_warning("Trigger socket not ready to be read");
        return true;
    }

    int fd = -1;

    ssize_t bytes = read(trigger_fd, &fd, sizeof(fd));

    if (sizeof(fd) != bytes) {
        ov_log_error("Unexpected number of bytes from trigger fd");
        goto error;
    }

    intptr_t fd_intptr = fd;

    AppConnection *conn = ov_dict_get(app->sockets, (void *)fd_intptr);

    if (0 == conn) {
        ov_log_warning("No connection found for socket %i", fd);
        goto error;
    }

    // ensure we do have no new/old data, once we call
    ov_parser_data_clear(&conn->data);
    return app_connection_use_parser(conn);

error:

    return false;
}

/*----------------------------------------------------------------------------*/

static bool setup_trigger_parse_again(DefaultApp *app) {

    if (0 == app) {
        goto error;
    }

    ov_event_loop *loop = app->public.config.loop;

    if (0 == app->public.config.loop) {
        goto error;
    }

    int spair[2] = {0};

    if (0 != socketpair(AF_UNIX, SOCK_STREAM, 0, spair)) {
        ov_log_error("Could not create trigger sockets");
        goto error;
    }

    /* TBD verify SOCK_CLOEXEC is not required, or
     * may be implemented using e.g. fcntl */

    if (!ov_socket_ensure_nonblocking(spair[0]) ||
        !ov_socket_ensure_nonblocking(spair[1]))
        goto error;

    app->trigger_parse_again.in = spair[0];
    app->trigger_parse_again.out = spair[1];

    const uint8_t EVENTS = OV_EVENT_IO_IN | OV_EVENT_IO_ERR | OV_EVENT_IO_CLOSE;

    if (!loop->callback.set(
            loop, app->trigger_parse_again.out, EVENTS, app, cb_parse_again)) {
        ov_log_error("could not register trigger_parse_again callback");
        goto error;
    }

    return true;

error:

    return false;
}

/*----------------------------------------------------------------------------*/

static bool close_trigger_parse_again(DefaultApp *app) {

    if (0 == app) {
        return false;
    }

    close(app->trigger_parse_again.in);
    close(app->trigger_parse_again.out);

    app->trigger_parse_again.in = -1;
    app->trigger_parse_again.out = -1;

    return true;
}

/*****************************************************************************
                                 DEFAULT Parser
 ****************************************************************************/

static ov_parser *parser_free(ov_parser *self) {

    if (self) free(self);
    return NULL;
}

/*---------------------------------------------------------------------------*/

static ov_parser_state parser_parse_magic(ov_parser *self,
                                          ov_parser_data *const data) {

    if (!self || !data || !ov_buffer_cast(data->in.data)) {
        return OV_PARSER_ERROR;
    }

    data->out.data = data->in.data;
    data->out.free = data->in.free;

    data->in.data = NULL;
    data->in.free = NULL;
    return true;
}

/*---------------------------------------------------------------------------*/

static bool parser_buffer_is_enabled(const ov_parser *self) {

    UNUSED(self);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool init_parser_buffer_switch(ov_parser *parser) {

    if (!ov_parser_set_head(parser, 0x0815)) return false;

    parser->free = parser_free;
    parser->decode = parser_parse_magic;
    parser->encode = parser_parse_magic;
    parser->buffer.is_enabled = parser_buffer_is_enabled;

    return true;
}

/*****************************************************************************
                                 INITIALIZE APP
 ****************************************************************************/

static bool app_init(DefaultApp *app, ov_app_config config) {

    if (!app) goto error;

    if (!config.loop) goto error;

    if (0 == config.cache.capacity)
        config.cache.capacity = OV_APP_DEFAULT_CACHE;

    /*
     *      Structure inititalization,
     *      ensure we have a name set for log messages
     */

    app->public.config = config;

    if (!app->public.config.name[0])
        strcat(app->public.config.name, OV_APP_DEFAULT_NAME);

    app->public.magic_byte = OV_APP_MAGIC_BYTE;
    app->public.type = IMPL_APP_TYPE;

    /*
     *      Definition of implementation functions.
     *
     */

    app->public.free = impl_app_free;

    app->public.socket.open = impl_app_socket_open;
    app->public.socket.close = impl_app_socket_close;
    app->public.socket.add = impl_app_socket_add;

    app->public.connection.close = impl_app_connection_close;
    app->public.connection.close_all = impl_app_connection_close_all;
    app->public.connection.get_socket = impl_app_connection_get_socket,

    app->public.send = impl_app_send;

    /*
     *      Dict creation for connection management.
     *
     */

    app->sockets = ov_dict_create(app_socket_data_dict_config(IMPL_DICT_SLOTS));

    app->uuids = ov_dict_create(app_uuids_dict_config(IMPL_DICT_SLOTS));

    /*
     *      Check correct initializaton.
     *
     */

    if (!app->sockets || !app->uuids) goto error;

    app->reconnect = 0;

    if (0 < config.reconnect.max_connections) {

        app->reconnect =
            ov_reconnect_manager_create(config.loop,
                                        config.reconnect.interval_secs,
                                        config.reconnect.max_connections);
        app->reconnect_data = calloc(
            config.reconnect.max_connections, sizeof(ReconnectDataEntry));
    }

    init_parser_buffer_switch(&app->parser_buffer_switch);

    if (!setup_trigger_parse_again(app)) {
        goto error;
    }

    return true;
error:
    if (app) impl_app_free((ov_app *)app);

    return false;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      CALLBACKS
 *
 *      ------------------------------------------------------------------------
 */

static bool cb_io_accept(int socket_fd, uint8_t events, void *data);

static bool cb_io_stream(int socket, uint8_t events, void *app);

static bool cb_io_raw(AppConnection *connection,
                      const uint8_t *buffer,
                      size_t size,
                      const ov_socket_data *remote);

/*
 *      ------------------------------------------------------------------------
 *
 *      HELPER
 *
 *      ------------------------------------------------------------------------
 */

static bool open_client_connection(ov_app *self,
                                   ov_app_socket_config config,
                                   void (*cb_success)(int socket, void *self),
                                   void (*cb_failure)(int socket, void *self));

static AppConnection *app_connection_cast(const void *data);

static AppConnection *app_connection_create(DefaultApp *app,
                                            ov_app_socket_config config,
                                            int socket);

static AppListener *app_listener_cast(const void *data);

static AppListener *app_listener_create(DefaultApp *app,
                                        ov_app_socket_config config,
                                        int socket);

static void *app_listener_free(void *self);

static bool default_send_buffer(ov_app *app,
                                int socket,
                                const ov_buffer *buffer);

/*
 *      ------------------------------------------------------------------------
 *
 *      PUBLIC IMPLEMENTATIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_app *ov_app_create(ov_app_config config) {

    DefaultApp *app = calloc(1, sizeof(DefaultApp));
    if (!app) goto error;

    if (app_init(app, config)) return (ov_app *)app;

    free(app);
error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

void *ov_app_free(void *self) {

    ov_app *app = ov_app_cast(self);
    if (!app || !app->free) goto error;

    return app->free(app);
error:
    return self;
}

/*----------------------------------------------------------------------------*/

bool ov_app_open_socket(ov_app *self,
                        ov_app_socket_config config,
                        void (*success)(int socket, void *self),
                        void (*failure)(int socket, void *self)) {

    if (!ov_app_cast(self) || !self->socket.open) return false;

    return (self->socket.open(self, config, success, failure));
}

/*----------------------------------------------------------------------------*/

bool ov_app_close_socket(ov_app *self, int socket) {

    if (!ov_app_cast(self) || !self->socket.close || socket < 0) return false;

    return (self->socket.close(self, socket));
}

/*----------------------------------------------------------------------------*/

static void *app_socket_data_free(void *data) {

    if (!data) return NULL;

    if (app_listener_cast(data)) return app_listener_free(data);

    if (app_connection_cast(data)) return app_connection_free(data);

    return data;
}

/*----------------------------------------------------------------------------*/

static AppConnection *app_connection_cast(const void *self) {

    if (!self) goto error;

    if (*(uint16_t *)self == IMPL_CONNECTION_MAGIC_BYTES)
        return (AppConnection *)self;
error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

static AppConnection *app_connection_create(DefaultApp *app,
                                            ov_app_socket_config config,
                                            int socket) {

    char *key = NULL;

    AppConnection *connection = calloc(1, sizeof(AppConnection));

    if (!connection) goto error;

    intptr_t id = socket;

    if (!app) goto error;

    *connection = (AppConnection){

        .magic_bytes = IMPL_CONNECTION_MAGIC_BYTES,
        .app = app,
        .config = config,
        .socket = socket

    };

    if (socket > -1)
        if (!ov_socket_get_data(
                socket, &connection->local, &connection->remote))
            goto error;

    if (!ov_id_fill_with_uuid(connection->uuid)) goto error;

    if (config.parser.create) {

        connection->parser = config.parser.create(config.parser.config);
        if (!connection->parser) goto error;
    }

    if (socket > -1) {

        if (TCP != config.socket_config.type) {
            goto error;
        }

        if (!ov_dict_set(app->sockets, (void *)id, connection, NULL))
            goto error;

        key = strdup(connection->uuid);
        if (!key) goto error;

        if (!ov_dict_set(app->uuids, key, (void *)id, NULL)) {
            ov_dict_del(app->sockets, (void *)id);
            goto error;
        }
    }

    OV_ASSERT(0 != connection);

    return connection;

error:

    key = ov_data_pointer_free(key);

    app_connection_free(connection);

    return NULL;
}

/*----------------------------------------------------------------------------*/

static void *app_connection_free(void *data) {

    AppConnection *connection = app_connection_cast(data);
    if (!connection) return data;

    DefaultApp *app = AS_IMPL_APP(connection->app);
    if (!app) {
        ov_log_critical("No app in connection data, dataloss");
        return NULL;
    }

    ov_event_loop *loop = app->public.config.loop;

    if (!connection->uuid_skip) ov_dict_del(app->uuids, connection->uuid);

    connection->parser = ov_parser_free(connection->parser);
    ov_parser_data_clear(&connection->data);

    if (connection->socket >= 0) {

        if (!connection->child) {

            loop->callback.unset(loop, connection->socket, NULL);
            close(connection->socket);
        }
    }

    if (connection->config.callback.close)
        connection->config.callback.close((ov_app *)connection->app,
                                          connection->socket,
                                          connection->uuid,
                                          connection->config.callback.userdata);

    memset(connection, 0, sizeof(*connection));

    free(connection);
    return NULL;
}

/*****************************************************************************
                                  APP Listener
 ****************************************************************************/

AppListener *app_listener_cast(const void *self) {

    if (!self) goto error;

    if (*(uint16_t *)self == IMPL_LISTENER_MAGIC_BYTES)
        return (AppListener *)self;
error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

AppListener *app_listener_create(DefaultApp *app,
                                 ov_app_socket_config config,
                                 int socket) {

    intptr_t id = socket;

    AppListener *listener = calloc(1, sizeof(AppListener));
    if (!listener) goto error;

    *listener = (AppListener){

        .magic_bytes = IMPL_LISTENER_MAGIC_BYTES,
        .app = app,
        .config = config,
        .socket = socket,
    };

    if (TCP != config.socket_config.type) {
        goto error;
    }

    if (!ov_dict_set(app->sockets, (void *)id, listener, NULL)) goto error;

    return listener;
error:
    app_listener_free(listener);
    return NULL;
}

/*----------------------------------------------------------------------------*/

struct container1 {

    ov_list *list;
    int socket;
};

/*----------------------------------------------------------------------------*/

static bool search_connections(const void *key, void *value, void *data) {

    if (!key) return true;

    struct container1 *container = data;
    if (!container || !container->list || container->socket < 0) return false;

    AppConnection *connection = app_connection_cast(value);
    if (!connection) return true;

    if (!connection->child)
        if (connection->master_socket == container->socket)
            if (!ov_list_push(container->list, (void *)key)) return false;

    return true;
}

/*----------------------------------------------------------------------------*/

void *app_listener_free(void *data) {

    intptr_t id = 0;

    ov_list *list = NULL;

    AppListener *listener = app_listener_cast(data);
    if (!listener) goto error;
    if (!listener->app) goto error;

    DefaultApp *app = listener->app;
    ov_event_loop *loop = app->public.config.loop;

    listener->connections = ov_dict_free(listener->connections);

    // delete all accepted sockets
    list = ov_list_create((ov_list_config){0});
    if (!list) {
        ov_log_error(
            "%s listener %i failed to create search "
            "list",
            app->public.config.name,
            listener->socket);
        goto done;
    }

    struct container1 container = {

        .socket = listener->socket, .list = list};

    if (!ov_dict_for_each(app->sockets, &container, search_connections)) {
        ov_log_error(
            "%s listener %i failed to search "
            "connections",
            app->public.config.name,
            listener->socket);
        goto done;
    }

    void *next = list->iter(list);
    void *key = NULL;

    while (next) {

        next = list->next(list, next, &key);
        if (!key) continue;

        id = (intptr_t)key;

        if (!ov_dict_del(app->sockets, (void *)id)) {
            ov_log_error(
                "%s listener %i failed to delete "
                "conn %i",
                app->public.config.name,
                listener->socket,
                socket);
        } else {

            ov_log_debug(
                "%s listener %i delete "
                "conn %i",
                app->public.config.name,
                listener->socket,
                socket);
        }
    }

done:

    if (listener->socket >= 0) {

        loop->callback.unset(loop, listener->socket, NULL);
        close(listener->socket);
    }

    if (listener->config.callback.close)
        listener->config.callback.close((ov_app *)listener->app,
                                        listener->socket,
                                        NULL,
                                        listener->config.callback.userdata);

    free(listener);
    ov_list_free(list);
    return NULL;
error:
    ov_list_free(list);
    return data;
}

/*----------------------------------------------------------------------------*/

static bool delete_socket(const void *key, void *item, void *data) {

    if (!key) return true;

    ov_dict *dict = ov_dict_cast(data);

    if (!item || !dict) return false;

    intptr_t id = (intptr_t)item;

    AppConnection *conn = app_connection_cast(ov_dict_get(dict, (void *)id));
    if (!conn) return false;

    conn->uuid_skip = true;

    return ov_dict_del(dict, (void *)id);
}

/*----------------------------------------------------------------------------*/

ov_app *impl_app_free(ov_app *self) {

    DefaultApp *app = AS_IMPL_APP(self);
    if (!app) return self;

    close_trigger_parse_again(app);

    if (!ov_dict_for_each(app->uuids, app->sockets, delete_socket)) {
        ov_log_error("Failed to delete connections.");
    }

    ov_dict_free(app->sockets);
    ov_dict_free(app->uuids);

    if (0 != app->reconnect) {

        app->reconnect = ov_reconnect_manager_free(app->reconnect);
        free(app->reconnect_data);
    }

    OV_ASSERT(0 == app->reconnect);

    free(app);
    return NULL;
}

/*----------------------------------------------------------------------------*/

bool impl_app_socket_open(ov_app *self,
                          ov_app_socket_config config,
                          void (*cb_success)(int socket, void *self),
                          void (*cb_failure)(int socket, void *self)) {

    int socket = 0;

    ov_event_loop *loop = NULL;

    DefaultApp *app = AS_IMPL_APP(self) if (!app) goto error;

    if (config.as_client)
        return open_client_connection(self, config, cb_success, cb_failure);

    loop = app->public.config.loop;
    if (!loop || !loop->callback.set) goto error;

    socket = ov_socket_create(config.socket_config, false, NULL);
    if (socket < 0) {
        ov_log_error("%s failed to open socket %s:%i | %s",
                     app->public.config.name,
                     config.socket_config.host,
                     config.socket_config.port,
                     ov_socket_transport_to_string(config.socket_config.type));
        goto error;
    }

    if (!self->socket.add || !self->socket.add(self, config, socket))
        goto error;

    if (cb_success) cb_success(socket, self);
    return true;

error:

    if (socket > 0) {

        if (!loop || !loop->callback.unset) {

            loop->callback.unset(loop, socket, NULL);
        }

        close(socket);
    }

    // raise error callback
    if (cb_failure) cb_failure(-1, self);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool fill_connection_key(char *key,
                                size_t size,
                                const ov_socket_data *remote) {

    if (!key || !remote) return false;

    int bytes = snprintf(key, size, "%s%i", remote->host, remote->port);
    if (bytes <= 0) return false;
    if (bytes >= CONNECTION_KEY_LENGTH) return false;
    return true;
}

/******************************************************************************
 *                            impl_app_socket_add
 ******************************************************************************/

static bool socket_config_valid(const ov_app_socket_config cfg, int fd) {

    OV_ASSERT(-1 < fd);

    int opt = 0;
    socklen_t optlen = sizeof(opt);

    if (0 != getsockopt(fd, SOL_SOCKET, SO_TYPE, &opt, &optlen)) {
        ov_log_error("Could not get type of fd %i", fd);
        return false;
    }

    if (TCP != cfg.socket_config.type) {
        return false;
    }

    return true;
}

/*----------------------------------------------------------------------------*/

static const uint8_t EVENTS =
    OV_EVENT_IO_IN | OV_EVENT_IO_ERR | OV_EVENT_IO_CLOSE;

static bool tcp_add_nocheck(DefaultApp *app,
                            ov_event_loop *loop,
                            ov_app_socket_config config,
                            int fd) {

    OV_ASSERT(0 != app);
    OV_ASSERT(0 != loop);
    OV_ASSERT(-1 < fd);

    if (config.as_client) {

        if (!app_connection_create(app, config, fd)) {

            goto error;
        }

        if (!loop->callback.set(loop, fd, EVENTS, app, cb_io_stream))
            goto error;

    } else {

        if (!app_listener_create(app, config, fd)) {

            goto error;
        }

        if (!loop->callback.set(loop, fd, EVENTS, app, cb_io_accept))
            goto error;
    }

    return true;

error:

    return false;
}

/*----------------------------------------------------------------------------*/

bool impl_app_socket_add(ov_app *self, ov_app_socket_config config, int fd) {

    bool succeeded = false;

    intptr_t id = fd;

    DefaultApp *app = AS_IMPL_APP(self);

    if (!app || fd < 0) goto error;

    // check if already added
    if (ov_dict_get(app->sockets, (void *)id)) goto done;

    if (!socket_config_valid(config, fd)) goto error;

    OV_ASSERT(TCP == config.socket_config.type);

    ov_event_loop *loop = app->public.config.loop;

    if (!loop || !loop->callback.set) goto error;

    /*
     *      Open a server socket,
     *      and set connection (accept) listener.
     *
     */

    // read config with opened port
    if (!ov_socket_get_config(fd, &config.socket_config, NULL, NULL)) {
        ov_log_error(
            "%s failed to read back config "
            "from socket %i - closing",
            app->public.config.name,
            fd);
        goto error;
    }

    if (!ov_socket_ensure_nonblocking(fd)) goto error;

    // reset correct input type
    config.socket_config.type = TCP;

    succeeded = tcp_add_nocheck(app, loop, config, fd);

    if (succeeded) {
        ov_log_info("%s added socket %i %s at %s:%" PRIu16 " | %s ",
                    app->public.config.name,
                    fd,
                    config.as_client ? "client" : "server",
                    config.socket_config.host,
                    config.socket_config.port,
                    ov_socket_transport_to_string(config.socket_config.type));
    }

    return succeeded;

done:

    return true;

error:

    if (app) ov_dict_del(app->sockets, (void *)id);

    return false;
}

/*----------------------------------------------------------------------------*/

static bool open_client_connection(ov_app *self,
                                   ov_app_socket_config config,
                                   void (*cb_success)(int socket, void *self),
                                   void (*cb_failure)(int socket, void *self)) {

    AppConnection *connection = NULL;

    intptr_t id = 0;

    DefaultApp *app =
        AS_IMPL_APP(self) if (!app || !config.as_client) goto error;

    ov_event_loop *loop = app->public.config.loop;

    if (!loop || !loop->callback.set) goto error;

    int socket = -1;

    if (TCP != config.socket_config.type) {
        ov_log_error("Only TCP supported");
        goto error;
    }

    /*
     *      Open a client socket and set connection IO listener.
     *
     */

    socket = ov_socket_create(config.socket_config, true, NULL);
    if (socket < 0) {

        ov_log_error("%s failed to open client socket to %s:%i",
                     app->public.config.name,
                     config.socket_config.host,
                     config.socket_config.port);
        goto error;
    }

    if (!ov_socket_ensure_nonblocking(socket)) goto error;

    id = socket;

    /*
     *      Set IO callbacks,
     *      based on the socket configuration.
     *
     */

    uint8_t events = OV_EVENT_IO_IN | OV_EVENT_IO_ERR | OV_EVENT_IO_CLOSE;

    connection = app_connection_create(app, config, socket);
    if (!connection) goto error;

    connection->success = cb_success;
    connection->failure = cb_failure;

    if (!loop->callback.set(loop, socket, events, app, cb_io_stream)) {

        ov_log_error(
            "%s failed to add STREAM "
            "IO at %s",
            app->public.config.name,
            config.socket_config.host);

        goto error;
    }

    ov_log_info(
        "%s opened client connection "
        "with UUID %s at socket %i from %s:%i to %s:%i | "
        "%s",
        app->public.config.name,
        connection->uuid,
        socket,
        connection->local.host,
        connection->local.port,
        connection->remote.host,
        connection->remote.port,
        ov_socket_transport_to_string(connection->config.socket_config.type));

    /*
     *      Perform direct async callback for,
     *      synchron opened sockets.
     */

    if (cb_success) {

        switch (config.socket_config.type) {

            case LOCAL:
            case TCP:
            case UDP:
                cb_success(socket, self);
                break;
            default:
                break;
        }
    }

    return true;
error:
    // raise error callback
    if (app) {

        if (connection) ov_dict_del(app->uuids, connection->uuid);

        ov_dict_del(app->sockets, (void *)id);
    }
    if (cb_failure) cb_failure(-1, self);
    return false;
}

/*----------------------------------------------------------------------------*/

bool impl_app_socket_close(ov_app *self, int socket) {

    intptr_t id = socket;
    DefaultApp *app = AS_IMPL_APP(self);
    if (!app || socket < 0) goto error;

    ov_log_debug("%s closing socket %i", app->public.config.name, socket);

    return ov_dict_del(app->sockets, (void *)id);

error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool impl_app_connection_close(ov_app *self, const char *uuid) {

    DefaultApp *app = AS_IMPL_APP(self);
    if (!app || !uuid) return false;

    intptr_t id = (intptr_t)ov_dict_get(app->uuids, uuid);

    ov_log_debug("%s connection close %s at socket %i",
                 app->public.config.name,
                 uuid,
                 id);

    return impl_app_socket_close(self, id);
}

/*----------------------------------------------------------------------------*/

int impl_app_connection_get_socket(ov_app *self, const char *uuid) {

    DefaultApp *app = AS_IMPL_APP(self);
    if (!app || !uuid) return false;

    intptr_t id = (intptr_t)ov_dict_get(app->uuids, uuid);
    return id;
}

/*----------------------------------------------------------------------------*/

static bool close_connections(void *key, void *data) {

    return impl_app_socket_close(ov_app_cast(data), (intptr_t)key);
}

/*----------------------------------------------------------------------------*/

static bool gather_client_socket_keys(const void *key, void *item, void *data) {

    if (!key) return true;

    AppConnection *conn = app_connection_cast(item);
    if (!conn) return true;
    if (conn->server) return true;

    return ov_list_push(data, (void *)key);
}

/*----------------------------------------------------------------------------*/

bool impl_app_connection_close_all(ov_app *self) {

    DefaultApp *app = AS_IMPL_APP(self);
    if (!app) return false;

    ov_list *keys = NULL;

    keys = ov_list_create((ov_list_config){0});

    if (!ov_dict_for_each(app->sockets, keys, gather_client_socket_keys))
        goto error;

    if (!ov_list_for_each(keys, app, close_connections)) goto error;

    keys = ov_list_free(keys);

    return true;
error:
    ov_list_free(keys);
    return false;
}

/*----------------------------------------------------------------------------*/

static AppConnection *connection_from_socket_data(
    void *data, ov_socket_data const *remote) {

    AppConnection *connection = app_connection_cast(data);

    if (0 != connection) {
        return connection;
    }

    AppListener *listener = app_listener_cast(data);

    if (0 == listener) {
        ov_log_error("callback without listener or connection");
        goto error;
    }

    char key[CONNECTION_KEY_LENGTH] = {0};
    if (!fill_connection_key(key, CONNECTION_KEY_LENGTH, remote)) goto error;

    return ov_dict_get(listener->connections, key);

error:

    return 0;
}

/*----------------------------------------------------------------------------*/

static ov_parser *app_get_send_parser(DefaultApp *app,
                                      int socket,
                                      const ov_socket_data *remote) {

    intptr_t id = socket;

    if (!app || socket < 1) goto error;

    void *socket_data = ov_dict_get(app->sockets, (void *)id);

    if (!socket_data) {

        ov_log_debug(
            "%s has no data for socket %i", app->public.config.name, socket);
        goto error;
    }

    AppConnection *connection =
        connection_from_socket_data(socket_data, remote);

    if (!connection) {

        ov_log_debug(
            "%s has no socket %i which is able to "
            "send",
            app->public.config.name,
            socket);
        goto error;
    }

    return connection->parser;

error:
    return 0;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      CALLBACKS
 *
 *      ------------------------------------------------------------------------
 */

static bool cb_io_accept(int socket_fd, uint8_t events, void *data) {

    intptr_t id = socket_fd;

    int nfd = 0;

    DefaultApp *app = AS_IMPL_APP(data);
    if (!app) goto error;

    ov_event_loop *loop = app->public.config.loop;
    if (!loop) goto error;

    if (socket_fd < 1) goto error;

    if ((events & OV_EVENT_IO_CLOSE) || (events & OV_EVENT_IO_ERR)) {

        if (!loop->callback.unset(loop, socket_fd, NULL)) {
            ov_log_error(
                "%s failed to unset accept "
                "callback",
                app->public.config.name);
            goto error;
        }

        return impl_app_socket_close((ov_app *)app, socket_fd);
    }

    // accept MUST have some incoming IO
    if (!(events & OV_EVENT_IO_IN)) goto error;

    ov_socket_data local;
    memset(&local, 0, sizeof(local));
    ov_socket_data remote;
    memset(&remote, 0, sizeof(remote));

    nfd = accept(socket_fd, NULL, NULL);

    if (nfd < 0) {

        ov_log_error("%s failed to accept at socket %i",
                     app->public.config.name,
                     socket_fd);
        goto error;
    }

    if (!ov_socket_ensure_nonblocking(nfd)) goto error;

    if (!ov_socket_get_data(nfd, &local, &remote)) {
        ov_log_error(
            "%s failed to parse data at accepted fd "
            "%i",
            app->public.config.name,
            nfd);
        goto error;
    }

    if (local.host[0] != 0) {

        ov_log_debug(
            "%s accepted at socket fd %i | "
            "LOCAL %s:%i REMOTE %s:%i | "
            "new connection fd %i",
            app->public.config.name,
            socket_fd,
            local.host,
            local.port,
            remote.host,
            remote.port,
            nfd);

    } else {

        ov_log_debug(
            "%s accepted at socket fd %i | "
            "new connection fd %i",
            app->public.config.name,
            socket_fd,
            nfd);
    }

    AppListener *listener =
        app_listener_cast(ov_dict_get(app->sockets, (void *)id));

    if (!listener) goto error;

    AppConnection *connection =
        app_connection_create(app, listener->config, nfd);

    if (!connection) {
        ov_log_error(
            "%s failed to create connection "
            "at accepted socket %i",
            app->public.config.name,
            nfd);
        goto error;
    }

    connection->master_socket = socket_fd;

    if (!loop->callback.set(
            loop,
            nfd,
            OV_EVENT_IO_ERR | OV_EVENT_IO_IN | OV_EVENT_IO_CLOSE,
            app,
            cb_io_stream)) {

        ov_log_critical(
            "%s failed to set io_callback at %i", app->public.config.name, nfd);

        if (!impl_app_socket_close((ov_app *)app, nfd)) {
            ov_log_critical("%s failed to cleanup at %i", nfd);
            goto error;
        }

        nfd = -1;
        goto error;
    }

    if ((0 != listener->config.callback.accepted) &&
        (!listener->config.callback.accepted(
            &app->public,
            socket_fd,
            nfd,
            listener->config.callback.userdata))) {
        ov_log_error("open callback signalled to close socket");
        if (!impl_app_socket_close((ov_app *)app, nfd)) {
            ov_log_critical("%s failed to cleanup at %i", nfd);
            goto error;
        }

        nfd = -1;
    }

    return true;

error:
    if (nfd > -1) close(nfd);
    return false;
}

/*----------------------------------------------------------------------------*/

bool cb_io_stream(int socket, uint8_t events, void *self) {

    intptr_t id = socket;
    size_t size = IMPL_READ_BUFFER_SIZE;
    uint8_t buffer[IMPL_READ_BUFFER_SIZE];
    memset(buffer, 0, size);

    DefaultApp *app = AS_IMPL_APP(self);

    OV_ASSERT(0 != app);

    if (!app) goto error;

    ov_log_debug("%s io stream at %i | event %i",
                 app->public.config.name,
                 socket,
                 events);

    if (events & OV_EVENT_IO_ERR) {

        ov_log_debug("events contains OV_EVENT_IO_ERR");
        goto error;
    }

    if (!(events & OV_EVENT_IO_IN)) {
        ov_log_debug("events does not contain OV_EVENT_IO_IN");
        goto error;
    }

    if (OV_EVENT_IO_CLOSE & events) {

        ov_log_debug("events contains OV_EVENT_IO_CLOSE");
        goto error;
    }

    ssize_t in = recv(socket, (char *)buffer, size, 0);

    if (in == 0) {
        ov_log_debug("Did not get ANY data");
        goto error;
    }

    // read again
    if ((-1 == in) || ((in < 0) && (EAGAIN == errno))) {
        return true;
    }

    if (in < 0) {
        ov_log_debug(
            "recv returned %i - errno is %i (%s)", in, errno, strerror(errno));
        goto error;
    }

    /* we need to detect an EOF */

    AppConnection *connection =
        app_connection_cast(ov_dict_get(app->sockets, (void *)id));

    if (!connection) {

        ov_log_critical("%s IO stream without connection at socket %i",
                        app->public.config.name,
                        socket);

        goto error;
    }

    return cb_io_raw(connection, buffer, in, &connection->remote);

error:

    impl_app_socket_close((ov_app *)app, socket);
    close(socket);
    return false;
}

/*----------------------------------------------------------------------------*/

bool cb_io_dgram(int socket, uint8_t events, void *self) {

    intptr_t id = socket;

    size_t size = OV_UDP_PAYLOAD_OCTETS;
    uint8_t buffer[size];
    memset(buffer, 0, size);

    DefaultApp *app = AS_IMPL_APP(self);
    if (!app || socket < 0) goto error;

    ov_log_debug(
        "%s io dgram at %i events %i", app->public.config.name, socket, events);

    AppConnection *connection =
        app_connection_cast(ov_dict_get(app->sockets, (void *)id));

    if (!connection) goto error;

    if (events & OV_EVENT_IO_ERR || events & OV_EVENT_IO_CLOSE ||
        !(events & OV_EVENT_IO_IN))
        goto close;

    struct sockaddr_storage sa = {0};
    socklen_t sa_len = sizeof(sa);

    ssize_t in =
        recvfrom(socket, buffer, size, 0, (struct sockaddr *)&sa, &sa_len);

    // read again
    if (in < 0) return true;

    ov_socket_data remote = ov_socket_data_from_sockaddr_storage(&sa);
    return cb_io_raw(connection, buffer, in, &remote);
close:

    ov_log_debug("%s IO UDP close %i", app->public.config.name, socket);

    if (connection->config.callback.close)
        connection->config.callback.close(
            (ov_app *)app, socket, NULL, connection->config.callback.userdata);

    return impl_app_socket_close((ov_app *)app, socket);
error:
    ov_log_critical("ov_app io_dgram error");
    return false;
}

/*----------------------------------------------------------------------------*/

static bool send_parser_answer(DefaultApp *app,
                               AppConnection *connection,
                               const ov_socket_data *remote) {

    if (!app || !connection || !remote) goto error;

    ov_log_debug(
        "%s | %s connection IO parser %s answer "
        "at %s:%i from %s:%i",
        app->public.config.name,
        connection->uuid,
        connection->parser->name,
        connection->config.socket_config.host,
        connection->config.socket_config.port,
        remote->host,
        remote->port);

    if ((!ov_buffer_cast(connection->data.out.data)) ||
        (connection->data.out.free != ov_buffer_free)) {
        ov_log_error(
            "%s | %s answer without ov_buffer data "
            "content or ov_buffer_free function %s "
            "at %s:%i from %s:%i",
            app->public.config.name,
            connection->uuid,
            connection->parser->name,
            connection->config.socket_config.host,
            connection->config.socket_config.port,
            remote->host,
            remote->port);
        goto error;
    }

    if (!default_send_buffer(
            ov_app_cast(app), connection->socket, connection->data.out.data)) {

        ov_log_error(
            "%s | %s failed to send answer %s "
            "at %s:%i from %s:%i",
            app->public.config.name,
            connection->uuid,
            connection->parser->name,
            connection->config.socket_config.host,
            connection->config.socket_config.port,
            remote->host,
            remote->port);
        goto error;
    }

    if (connection->data.out.free)
        connection->data.out.free(connection->data.out.data);
    connection->data.out.data = NULL;

    return true;
error:
    if (connection) {
        if (connection->data.out.free)
            connection->data.out.free(connection->data.out.data);
        connection->data.out.data = NULL;
    }
    return false;
}

/*****************************************************************************
                                 APP CONNECTION
 ****************************************************************************/

static bool app_connection_get_app_and_loop(AppConnection *connection,
                                            DefaultApp **app,
                                            ov_event_loop **loop) {

    if (0 == connection) {
        ov_log_error("No connection (0 pointer)");
        goto error;
    }

    DefaultApp *app_local = connection->app;

    if (!app_local) {
        ov_log_error("No app associated with connection");
        goto error;
    }

    ov_event_loop *loop_local = app_local->public.config.loop;

    if (!loop_local) {
        ov_log_error("No loop associated with app assoc. with connection");
        goto error;
    }

    *app = app_local;
    *loop = loop_local;

    return true;

error:

    return false;
}

/*----------------------------------------------------------------------------*/

static bool app_connection_process_parsed_data(AppConnection *connection) {

    DefaultApp *app = 0;
    ov_event_loop *loop = 0;

    if (!app_connection_get_app_and_loop(connection, &app, &loop)) {
        goto error;
    }

    OV_ASSERT(0 != connection->config.callback.io);

    bool ok =
        connection->config.callback.io(&app->public,
                                       connection->socket,
                                       connection->uuid,
                                       &connection->remote,
                                       (void **)&connection->data.out.data);

    if ((0 != connection->data.out.data) && (0 != connection->data.out.free)) {

        connection->data.out.data =
            connection->data.out.free(connection->data.out.data);
    }

    if (!ok) {
        goto error;
    }

    if (!trigger_parse_again(connection)) {
        goto error;
    }

    return true;

error:

    return false;
}

/*----------------------------------------------------------------------------*/

#define PARSE_LOG_DEBUG(connection, msg)                                       \
    do {                                                                       \
        ov_log_debug(                                                          \
            "%s | %s connection (%s:%i from %s:%i): Parser %s "                \
            "error: %s",                                                       \
            app->public.config.name,                                           \
            connection->uuid,                                                  \
            connection->config.socket_config.host,                             \
            connection->config.socket_config.port,                             \
            connection->remote.host,                                           \
            connection->remote.port,                                           \
            connection->parser->name,                                          \
            msg);                                                              \
    } while (0)

/*----------------------------------------------------------------------------*/

static bool app_connection_parse_nocheck(DefaultApp *app,
                                         ov_event_loop *loop,
                                         AppConnection *connection) {

    OV_ASSERT(0 != app);
    OV_ASSERT(0 != loop);

    ov_buffer *buf = 0;
    char *str = 0;

    switch (ov_parser_decode(connection->parser, &connection->data)) {

        case OV_PARSER_ERROR:

            PARSE_LOG_DEBUG(connection, "error");
            goto error;

        case OV_PARSER_DONE:

            ov_parser_data_clear_in(&connection->data);
            ov_parser_data_clear_out(&connection->data);

            PARSE_LOG_DEBUG(connection, "done");
            break;

        case OV_PARSER_CLOSE:

            ov_parser_data_clear_in(&connection->data);
            ov_parser_data_clear_out(&connection->data);

            PARSE_LOG_DEBUG(connection, "close");
            goto error;

        case OV_PARSER_MISMATCH:

            PARSE_LOG_DEBUG(connection, "mismatch");
            goto error;

        case OV_PARSER_PROGRESS:

            PARSE_LOG_DEBUG(connection, "JSON incomplete: ");

            buf = connection->data.in.data;
            if (0 != buf) {
                str = calloc(1, buf->length + 1);

                memcpy(str, buf->start, buf->length);
                str[buf->length] = 0;
                ov_log_error("JSON incomplete: '%s'", str);

                free(str);
                str = 0;
            }

            break;

        case OV_PARSER_SUCCESS:

            PARSE_LOG_DEBUG(connection, "success");

            if (!app_connection_process_parsed_data(connection)) {
                goto error;
            }

            break;

        case OV_PARSER_ANSWER:

            if (!send_parser_answer(app, connection, &connection->remote))
                goto error;
            break;

        case OV_PARSER_ANSWER_CLOSE:

            send_parser_answer(app, connection, &connection->remote);
            goto error;
    }

    return true;

error:

    return false;
}

/*----------------------------------------------------------------------------*/

bool app_connection_use_parser(AppConnection *connection) {

    DefaultApp *app = 0;
    ov_event_loop *loop = 0;

    if (!app_connection_get_app_and_loop(connection, &app, &loop)) {
        goto error;
    }

    OV_ASSERT(0 != app);
    OV_ASSERT(0 != loop);

    return app_connection_parse_nocheck(app, loop, connection);

error:

    return false;
}

/*----------------------------------------------------------------------------*/

bool cb_io_raw(AppConnection *connection,
               const uint8_t *buffer,
               size_t size,
               const ov_socket_data *remote) {

    bool result = false;
    ov_buffer *buf = 0;

    DefaultApp *app = 0;
    ov_event_loop *loop = 0;

    if (!app_connection_get_app_and_loop(connection, &app, &loop)) {
        goto error;
    }

    OV_ASSERT(0 != app);
    OV_ASSERT(0 != loop);

    if (!buffer) {
        ov_log_error("buffer is a 0 pointer");
        goto error;
    }

    if (size < 1) {
        ov_log_error("buffer has 0 length");
        goto error;
    }

    connection->last_io_in = ov_time_get_current_time_usecs();

    ov_log_debug("%s io raw at %i %s:%i from %s:%i | %.*s",
                 app->public.config.name,
                 connection->socket,
                 connection->config.socket_config.host,
                 connection->config.socket_config.port,
                 remote->host,
                 remote->port,
                 size,
                 buffer);

    buf = ov_buffer_create(size);
    memcpy(buf->start, buffer, size);
    buf->length = size;

    if (0 != connection->parser) {

        connection->remote = *remote;
        ov_parser_data_clear(&connection->data);
        connection->data.in.data = buf;

        if (!app_connection_use_parser(connection)) {
            ov_log_error("Could not parse %.s", buffer);
            goto close;
        }

    } else if (0 != connection->config.callback.io) {

        ov_buffer **pointer_to_buffer = &buf;

        bool success =
            connection->config.callback.io((ov_app *)app,
                                           connection->socket,
                                           connection->uuid,
                                           remote,
                                           (void **)pointer_to_buffer);

        buf = ov_buffer_free(*pointer_to_buffer);

        if (!success) goto close;
    }

    ov_log_debug(
        "%s | %s connection IO done "
        "at %i | %s:%i from %s:%i",
        app->public.config.name,
        connection->uuid,
        connection->socket,
        connection->config.socket_config.host,
        connection->config.socket_config.port,
        remote->host,
        remote->port);

    buf = ov_buffer_free(buf);
    OV_ASSERT(0 == buf);

    return true;

close:

    buf = ov_buffer_free(buf);

    OV_ASSERT(0 == buf);

    result = false;

error:

    buf = ov_buffer_free(buf);

    if (connection && connection->app)
        impl_app_socket_close((ov_app *)connection->app, connection->socket);

    OV_ASSERT(0 == buf);

    return result;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      SEND
 *
 *      ------------------------------------------------------------------------
 */

/*---------------------------------------------------------------------------*/

static bool app_send_tcp(ov_app *app, const ov_buffer *buffer, int socket) {

    if (!buffer) goto error;

    size_t total = 0;
    ssize_t bytes = 0;

    ov_log_debug("%s going to send at %i | %s | %.*s",
                 app->config.name,
                 socket,
                 "TCP",
                 (int)buffer->length,
                 (char *)buffer->start);

    while (total != buffer->length) {

        bytes = send(
            socket, (char *)buffer->start + total, buffer->length - total, 0);

        if (bytes == -1) {

            if (errno != EAGAIN) goto error;

        } else {

            total += bytes;
        }
    }

    return true;
error:
    return false;
}

/*---------------------------------------------------------------------------*/

bool default_send_buffer(ov_app *app, int socket, const ov_buffer *buffer) {

    if (!buffer) goto error;

    OV_ASSERT(!ov_socket_is_dgram(socket));

    return app_send_tcp(app, buffer, socket);

error:
    return false;
}

/*---------------------------------------------------------------------------*/

static bool default_send_queue(ov_app *app, int socket, const ov_list *queue) {

    if (!queue) goto error;

    OV_ASSERT(!ov_socket_is_dgram(socket));

    for (ov_buffer *buffer = ov_list_queue_pop((ov_list *)queue); 0 != buffer;
         buffer = ov_list_queue_pop((ov_list *)queue)) {

        if (!app_send_tcp(app, buffer, socket)) goto error;

        buffer = ov_buffer_free(buffer);
    }

    return true;
error:
    return false;
}

/*---------------------------------------------------------------------------*/

static bool app_send_parser(ov_app *app,
                            ov_parser *parser,
                            int socket,
                            const ov_socket_data *remote,
                            void *data) {

    bool result = false;
    ov_parser_data parser_data;
    memset(&parser_data, 0, sizeof(parser_data));
    if (!app || !data || !parser || socket <= 0) goto error;

    /* For TCP, remote is not required and overwhelmingly
     * complex to supply. However, the error logs refer to
     * the remote pointer. Therefore fill it with something
     * that can be dereferenced
     */
    ov_socket_data dummy;
    memset(&dummy, 0, sizeof(dummy));
    if (0 == remote) {
        remote = &dummy;
    }

    parser_data.in.data = data;

    if (OV_PARSER_SUCCESS != ov_parser_encode(parser, &parser_data)) {
        ov_log_error(
            "%s failed to encode data at %i for "
            "%s:%i",
            app->config.name,
            socket,
            remote->host,
            remote->port);
        goto error;
    }

    ov_buffer *buffer = ov_buffer_cast(parser_data.out.data);
    ov_list *queue = ov_list_cast(parser_data.out.data);

    if (buffer) {

        result = default_send_buffer(app, socket, buffer);

    } else if (queue) {

        result = default_send_queue(app, socket, queue);

    } else {

        ov_log_error(
            "%s failed to send outgoing "
            "at %i to %s:%i no valid buffer or "
            "queue",
            app->config.name,
            socket,
            remote->host,
            remote->port);
        goto error;
    }

error:
    ov_parser_data_clear(&parser_data);
    return result;
}

/*---------------------------------------------------------------------------*/

bool impl_app_send(ov_app *app,
                   int socket,
                   const ov_socket_data *remote,
                   void *data) {

    DefaultApp *dapp = AS_IMPL_APP(app);

    if ((0 == dapp) || (socket <= 0) || (0 == data)) goto error;

    if (!socket_is_contained(app, socket)) {
        goto error;
    }

    ov_parser *parser = app_get_send_parser(dapp, socket, remote);

    if (0 == parser) {

        if (!ov_buffer_cast(data)) {
            ov_log_error(
                "%s send input not ov_buffer "
                "AND app params without parser, "
                "cannot "
                "send.",
                app->config.name);
            goto error;
        }

        parser = &dapp->parser_buffer_switch;
    }

    return app_send_parser(app, parser, socket, remote, data);

error:
    return false;
}

/*---------------------------------------------------------------------------*/

bool ov_app_send(ov_app *app,
                 int socket,
                 const ov_socket_data *remote,
                 void *data) {

    if (!app || !app->send) goto error;

    return app->send(app, socket, remote, data);
error:
    return false;
}

/*-------------------------------------------------------------------------*/

bool ov_app_stop(ov_app *app) {

    if (ov_ptr_valid(app, "No app to stop")) {
        return ov_event_loop_stop(app->config.loop);
    } else {
        return false;
    }
}

/*----------------------------------------------------------------------------*/

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/*----------------------------------------------------------------------------*/

const char *UNKNOWN_ARGUMENT_PRESENT =
    "Unknown argument "
    "present";

const char *ov_app_parse_command_line(int argc,
                                      char *const argv[],
                                      ov_app_parameters *restrict params) {

    NOT_IN_RELEASE("Function needs to be stripped");

    if ((0 == argc) || (0 == argv)) {

        return "Invalid argument - no args or nullpointer";
    }

    char *optstring = "c:v";

    int c = 0;

    const char *retval = 0;

    while (-1 != (c = getopt(argc, argv, optstring))) {

        switch (c) {

            case 'c':

                params->config_file = optarg;
                break;

            case 'v':

                params->version_request = true;
                break;

            default:
                /* Unknown arguments are ignored since
                 * they could be custom args for the
                 * particular app
                 */
                retval = UNKNOWN_ARGUMENT_PRESENT;
        };
    };

    if (0 == params->config_file) {

        char *app_name = basename(argv[0]);

        params->config_file = ov_config_default_config_file_for(app_name);

        app_name = 0;
    }

    OV_ASSERT(0 != params->config_file);

    return retval;
};

/******************************************************************************
 *                                 Reconnect
 ******************************************************************************/

static bool connect_socket_without_reconnect_unsafe(DefaultApp *app,
                                                    ov_app_socket_config cfg) {

    OV_ASSERT(0 != app);

    ov_socket_error err = {0};

    int fd = ov_socket_create(cfg.socket_config, true, &err);

    if (0 > fd) {

        ov_log_error(
            "Could not create client socket. "
            "errno: %s "
            "gai_error: "
            "%s",
            strerror(err.err),
            strerror(err.gai));

        goto error;
    }

    return app->public.socket.add((ov_app *)app, cfg, fd);

error:

    return false;
}

/*----------------------------------------------------------------------------*/

const uint32_t reconnect_data_magic_bytes;

static ReconnectData *get_unused_reconnect_data(
    size_t no, ReconnectDataEntry *rd_entries) {

    for (size_t i = 0; i < no; ++i) {

        if (!rd_entries[i].in_use) {
            rd_entries[i].in_use = true;

            return &rd_entries[i].rd;
        }
    }

    return 0;
}

/*----------------------------------------------------------------------------*/

static bool cb_io_stream_wrapper(int fd, uint8_t events, void *userdata) {

    if (0 == userdata) {
        ov_log_error(
            "Called with invalid argument (0 "
            "pointer)");
        goto error;
    }

    ReconnectData *rd = userdata;

    if (reconnect_data_magic_bytes != rd->magic_bytes) {

        ov_log_error(
            "Called with invalid argument (magic "
            "bytes don't "
            "match");
        goto error;
    }

    OV_ASSERT(0 != rd->app);

    if (0 == rd->app) {

        ov_log_error(
            "Serious internal error - "
            "ReconnectData without "
            "app");
        goto error;
    }

    return cb_io_stream(fd, events, rd->app);

error:

    return false;
}

/*----------------------------------------------------------------------------*/

static bool cb_reconnected(int fd, void *userdata) {

    if (0 == userdata) {

        ov_log_error(
            "Called with invalid argument (0 "
            "pointer)");
        goto error;
    }

    ReconnectData *rd = userdata;

    if (reconnect_data_magic_bytes != rd->magic_bytes) {

        ov_log_error(
            "Called with invalid argument (magic "
            "bytes don't "
            "match");
        goto error;
    }

    if (0 == rd->app) {

        ov_log_error(
            "Called with invalid argument: app "
            "missing");
        goto error;
    }

    DefaultApp *app = AS_IMPL_APP(rd->app);

    if (0 == app) {

        ov_log_error("Called with wrong app");
        goto error;
    }

    bool success = app_connection_create(app, rd->scfg, fd);

    if (!success) {

        ov_log_error("Could not create connection");
        goto error;
    }

    success = rd->scfg.callback.reconnected(
        &rd->app->public, fd, rd->scfg.callback.userdata);

    if (!success) {

        ov_log_error("reconnect callback failed");
        goto error;
    }

    return true;

error:

    if (0 < fd) {
        close(fd);
    }

    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_app_connect(ov_app *self, ov_app_socket_config config) {

    /* Currently, reconnects are only supported with
     * TCP sockets, client of course, thus
     * forward any other valid connect request to
     * app->socket.add
     */

    DefaultApp *app = AS_IMPL_APP(self);

    if (0 == app) {
        ov_log_error("Wrong app given");
        return false;

    } else if (!config.as_client) {

        return self->socket.open(self, config, 0, 0);

    } else if (0 == config.callback.reconnected) {

        return connect_socket_without_reconnect_unsafe(app, config);

    } else if (TCP != config.socket_config.type) {

        ov_log_error("reconnects only supported with TCP");
        return false;

    } else if (0 == app->reconnect) {

        ov_log_error("App does not support reconnects");
        return false;

    } else {

        ReconnectData *rd = get_unused_reconnect_data(
            self->config.reconnect.max_connections, app->reconnect_data);

        if (0 == rd) {

            ov_log_error("Number of supported reconnect connections exhausted");
            return false;

        } else {

            rd->magic_bytes = reconnect_data_magic_bytes;
            rd->app = app;
            rd->scfg = config;

            uint8_t events =
                OV_EVENT_IO_IN | OV_EVENT_IO_ERR | OV_EVENT_IO_CLOSE;

            ov_reconnect_callbacks callbacks = {
                .io = cb_io_stream_wrapper,
                .reconnected = cb_reconnected,
            };

            return ov_reconnect_manager_connect(
                app->reconnect, config.socket_config, events, callbacks, rd);
        }
    }
}

/*----------------------------------------------------------------------------*/
