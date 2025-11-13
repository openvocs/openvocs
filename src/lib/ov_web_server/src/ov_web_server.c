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
        @file           ov_web_server.c
        @author         Töpfer, Markus

        @date           2025-11-06


        ------------------------------------------------------------------------
*/
#include "../include/ov_web_server.h"
#include "../include/ov_web_server_connection.h"

#define IMPL_MAX_FRAMES_FOR_JSON 10000

#include <ov_base/ov_dict.h>
#include <ov_core/ov_domain.h>

#include <signal.h>

#include <openssl/conf.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/ssl.h>

/*----------------------------------------------------------------------------*/

#define OV_WEB_SERVER_MAGIC_BYTE 0xefe3

/*----------------------------------------------------------------------------*/

struct ov_web_server {

    uint16_t magic_byte;
    ov_web_server_config config;

    int socket;

    struct {

        ssize_t default_domain;
        size_t size;
        ov_domain *array;

    } domain;

    ov_dict *connections;

};

/*
 *      ------------------------------------------------------------------------
 *
 *      #TLS
 *
 *      ------------------------------------------------------------------------
 */

static int tls_client_hello_callback(SSL *ssl, int *al, void *arg) {

    ov_web_server *srv = ov_web_server_cast(arg);
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

static bool tls_init_context(ov_web_server *srv, ov_domain *domain) {

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


/*----------------------------------------------------------------------------*/

static bool init_config(ov_web_server_config *config){

    if (!config || !config->loop) goto error;

    if (0 == config->name[0])
        strncpy(config->name, "OV_WEBSERVER", PATH_MAX);

    if (0 == config->domain_config_path[0]){
        ov_log_error("No domain config path given - abort");
        goto error;
    }

    if (0 == config->limits.max_content_bytes_per_websocket_frame)
        config->limits.max_content_bytes_per_websocket_frame = 1000000000;

    if (0 == config->http_message.buffer.default_size)
        config->http_message.buffer.default_size = 1400;

    if (0 == config->websocket_frame.buffer.default_size)
        config->http_message.buffer.default_size = 1400;

    config->http_message = ov_http_message_config_init(config->http_message);

    if (0 == config->socket.host[0]){

        ov_log_debug("No socket given configuring to 0.0.0.0:443");

        config->socket = (ov_socket_configuration){
            .type = TLS, 
            .host = "0.0.0.0",
            .port = 443};

    } else {

        config->socket.type = TLS;
    }

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool close_tcp_listener(ov_web_server *self){

    ov_log_notice("Closing HTTPS at %s", self->config.name);
    
    ov_dict_clear(self->connections);
    self->socket = -1;

    return true;
}

/*----------------------------------------------------------------------------*/

static bool io_accept_secure(int socket, uint8_t events, void *data){

    int nfd = -1;
    SSL *ssl = NULL;
    ov_web_server_connection *conn = NULL;

    ov_web_server *self = ov_web_server_cast(data);
    if (!self) goto error;

    if ((events & OV_EVENT_IO_CLOSE) || (events & OV_EVENT_IO_ERR))
        return close_tcp_listener(self);

    // accept MUST have some incoming IO
    if (!(events & OV_EVENT_IO_IN)) goto error;

    struct sockaddr_storage sa = {0};
    socklen_t sa_len = sizeof(sa);

    nfd = accept(socket, (struct sockaddr *)&sa, &sa_len);

    if (nfd < 0) {

        ov_log_error(
            "%s failed to accept at socket %i", self->config.name, socket);
        goto error;
    }

    if (!ov_socket_ensure_nonblocking(nfd)) goto error;

    /*
     *      Create the connection
     */

    conn = ov_web_server_connection_create(self, nfd);
    if (!conn){

        close(nfd);
        goto error;

    }

    if (!ov_dict_set(self->connections, (void*)(intptr_t)nfd, conn, NULL)){

        conn = ov_web_server_connection_free(conn);
        goto error;

    }

    /*
     *      Create SSL for the first domain,
     *      MAY change during SSL setup over @see tls_servername_callback
     */

    ssl = SSL_new(self->domain.array[0].context.tls);
    if (!ssl) {

        ov_dict_del(self->connections, (void*)(intptr_t)nfd);
        goto error;
    }

    if (1 != SSL_set_fd(ssl, nfd)) {

        ov_dict_del(self->connections, (void*)(intptr_t)nfd);
        SSL_free(ssl);
        goto error;
    }

    if (!ov_web_server_connection_set_ssl(conn, ssl)) {

        ov_dict_del(self->connections, (void*)(intptr_t)nfd);
        SSL_free(ssl);
        goto error;
    }

    if (self->config.debug) {

        ov_socket_data local = {0};
        ov_socket_data remote = {0};

        ov_socket_get_data(nfd, &local, &remote);

        ov_log_debug(
            "%s accepted secure connection socket %i at %s:%i from "
            "%s:%i",
            self->config.name,
            nfd,
            local.host,
            local.port,
            remote.host,
            remote.port);
    }

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool open_socket(ov_web_server *self){

    if (!self) goto error;

    self->socket = ov_socket_create(self->config.socket, false, NULL);
    if (-1 == self->socket) goto error;
    
    if (!ov_socket_ensure_nonblocking(self->socket)) goto error;

    if (!ov_event_loop_set(self->config.loop,
        self->socket,
        OV_EVENT_IO_IN | OV_EVENT_IO_ERR | OV_EVENT_IO_CLOSE,
        self,
        io_accept_secure)) goto error;

    ov_log_info("opened HTTPs socket %s:%i", 
        self->config.socket.host,
        self->config.socket.port);

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

ov_web_server *ov_web_server_create(ov_web_server_config config){

    ov_web_server *self = NULL;

    if (!init_config(&config)) goto error;

    self = calloc(1, sizeof(ov_web_server));
    if (!self) goto error;

    self->magic_byte = OV_WEB_SERVER_MAGIC_BYTE;
    self->config = config;

    ov_dict_config d_config = ov_dict_intptr_key_config(255);
    d_config.value.data_function.free = ov_web_server_connection_free;

    self->connections = ov_dict_create(d_config);
    if (!self->connections) goto error;

    /* Load domains */
    if (!ov_domain_load(self->config.domain_config_path,
                        &self->domain.size,
                        &self->domain.array)) {
        ov_log_error("%s failed to load domains from %s",
                     self->config.name,
                     self->config.domain_config_path);
        goto error;
    }

    self->domain.default_domain = -1;

     /* Init additional (specific) SSL functions in domain context */
    for (size_t i = 0; i < self->domain.size; i++) {

        if (!tls_init_context(self, &self->domain.array[i])) goto error;

        if (self->domain.array[i].config.is_default) {

            if (-1 != self->domain.default_domain) {

                ov_log_error("%s more than ONE domain configured as default",
                    self->config.domain_config_path);

                goto error;
            }

            self->domain.default_domain = i;
        }
    }

    if (!open_socket(self)){

        ov_log_error("Failed to open socket %s:%i - abort",
            self->config.socket.host,
            self->config.socket.port);

        goto error;
    }

    /* Ignore sigpipe messages */
    signal(SIGPIPE, SIG_IGN);

    return self;

error:
    ov_web_server_free(self);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_web_server *ov_web_server_free(ov_web_server *self){

    if (!ov_web_server_cast(self)) return self;

    self = ov_data_pointer_free(self);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_web_server *ov_web_server_cast(const void *self){

    if (!self) goto error;

    if (*(uint16_t *)self == OV_WEB_SERVER_MAGIC_BYTE)
        return (ov_web_server*)self;
error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_web_server_config ov_web_server_get_config(ov_web_server *self){

    if (!self) return (ov_web_server_config){0};
    return self->config;
    
}

/*
 *      ------------------------------------------------------------------------
 *
 *      #Internal FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_domain *ov_web_server_find_domain(ov_web_server *srv,
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

ov_domain *ov_web_server_get_default_domain(ov_web_server *self){

    if (!self) goto error;

    if (-1 == self->domain.default_domain) {
        return &self->domain.array[0];
    } else {
        return &self->domain.array[self->domain.default_domain];
    }

error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

bool ov_web_server_close_connection(ov_web_server *self, int socket){

    return ov_dict_del(self->connections, (void*) (intptr_t) socket);
}

/*----------------------------------------------------------------------------*/

bool ov_web_server_send(ov_web_server *self, int socket, const ov_buffer *data){

    if (!self || !data) goto error;

    ov_web_server_connection *conn = ov_dict_get(self->connections, (void*)(intptr_t) socket);
    if (!conn) goto error;

    return ov_web_server_connection_send(conn, data);
error:
    return false;
}


/*----------------------------------------------------------------------------*/

static bool cb_wss_send_json(void *userdata, int socket, const ov_json_value *msg) {

    char *str = NULL;
    ov_websocket_frame *frame = NULL;

    ov_web_server *self = ov_web_server_cast(userdata);
    if (!self || !msg || socket < 0) goto error;

    ov_web_server_connection *conn = ov_dict_get(self->connections, (void*)(intptr_t)socket);
    if (!conn) goto error;

    str = ov_json_value_to_string(msg);
    if (!str) goto error;

    ssize_t length = strlen(str);

    /* Send the string over websocket frames */

    frame = ov_websocket_frame_create(self->config.websocket_frame);

    if (!frame) goto error;

    ov_websocket_frame_clear(frame);

    ssize_t chunk = self->config.limits.max_content_bytes_per_websocket_frame;

    if (length < chunk) {

        /* Send one WS frame */

        frame->buffer->start[0] = 0x80 | OV_WEBSOCKET_OPCODE_TEXT;

        if (!ov_websocket_set_data(frame, (uint8_t *)str, length, false)) {
            goto error;
        }

        if (!ov_web_server_connection_send(conn,frame->buffer)) {
            goto error;
        }

        goto done;
    }

    /* send fragmented */
    size_t counter = 0;

    uint8_t *ptr = (uint8_t *)str;
    ssize_t open = length;

    /* send fragmented start */

    frame->buffer->start[0] = 0x00 | OV_WEBSOCKET_OPCODE_TEXT;

    if (!ov_websocket_set_data(frame, ptr, chunk, false)) {
        goto error;
    }

    if (!ov_web_server_connection_send(conn,frame->buffer)) {
        goto error;
    }

    counter++;

    /* send continuation */

    open -= chunk;
    ptr += chunk;

    while (open > chunk) {

        frame->buffer->start[0] = 0x00;

        if (!ov_websocket_set_data(frame, ptr, chunk, false)) {
            goto error;
        }

        if (!ov_web_server_connection_send(conn,frame->buffer)) {
            goto error;
        }

        open -= chunk;
        ptr += chunk;

        counter++;
    }

    /* send last */

    frame->buffer->start[0] = 0x80;

    if (!ov_websocket_set_data(frame, ptr, open, false)) {
        goto error;
    }

    if (!ov_web_server_connection_send(conn,frame->buffer)) {
        goto error;
    }

    counter++;

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

bool ov_web_server_send_json(ov_web_server *self,
                                 int socket,
                                 ov_json_value const *const data){

    return cb_wss_send_json(self, socket, data);
}

/*----------------------------------------------------------------------------*/

bool ov_web_server_uri_file_path(ov_web_server *srv,
                                     int socket,
                                     const ov_http_message *msg,
                                     size_t length,
                                     char *path) {

    if (!srv || !msg || !path) goto error;

    /* (1) find open connection */

    ov_web_server_connection *conn = ov_dict_get(srv->connections, (void*)(intptr_t)socket);

    return ov_web_server_connection_uri_file_path(conn, msg, length, path);

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

bool ov_web_server_configure_websocket_callback(
    ov_web_server *self,
    const ov_memory_pointer hostname,
    ov_websocket_message_config config){

    char *key = NULL;
    ov_websocket_message_config *val = NULL;

    if (!self || !hostname.start || (hostname.length == 0)) goto error;

    ov_domain *domain = ov_web_server_find_domain(self, hostname.start, hostname.length);

    if (!domain) {
        goto error;
    }

    /* Check reset */

    if (NULL == config.userdata) {

        domain->websocket.config = (ov_websocket_message_config){0};
        domain->websocket.uri = ov_dict_free(domain->websocket.uri);
        goto done;
    }

    if ((NULL == config.callback) && (NULL == config.fragmented)) {

        if (0 != config.uri[0]) {

            ov_dict_del(domain->websocket.uri, config.uri);

        } else {

            domain->websocket.config = (ov_websocket_message_config){0};
        }

        goto done;
    }

    if (domain->websocket.config.userdata) {

        if (domain->websocket.config.userdata != config.userdata) {
            goto error;
        }
    }

    if (0 == config.uri[0]) {

        domain->websocket.config = config;

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

        goto error;
    }

done:
    ov_log_debug(
        "configured websocket callback for domain "
        "%.*s URI %s",
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

static bool cb_wss_io_to_json(void *userdata,
                              int socket,
                              const ov_memory_pointer domain,
                              const char *uri,
                              ov_memory_pointer content,
                              bool text) {

    ov_json_value *value = NULL;
    ov_web_server *srv = ov_web_server_cast(userdata);

    if (!srv) goto error;

    if (!text) goto error;

    if (socket < 0) goto error;

    ov_web_server_connection *conn = ov_dict_get(srv->connections, (void*)(intptr_t)socket);
    if (!conn) goto error;

    ov_domain *conn_domain = ov_web_server_connection_get_domain(conn);
    if (!conn_domain) goto error;

    ov_event_io_config *event_config =
        ov_dict_get(conn_domain->event_handler.uri, uri);

    if (!event_config || !event_config->callback.process) goto error;

    value = ov_json_value_from_string((char *)content.start, content.length);

    if (!value)
        goto error;

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

bool ov_web_server_configure_uri_event_io(
    ov_web_server *self,
    const ov_memory_pointer hostname,
    const ov_event_io_config config,
    const ov_websocket_message_config *wss_io){

    char *key = NULL;
    ov_event_io_config *val = NULL;

    if (!self || !hostname.start || (hostname.length == 0)) goto error;

    if (NULL != config.userdata && (0 == config.name[0])) {

        goto error;
    }

    ov_domain *domain = ov_web_server_find_domain(self, hostname.start, hostname.length);

    if (!domain) {
        goto error;
    }

    /*
     *  NOTE We do NOT support overriding or deletion of some existing
     *  URI callback yet, as this will require to lock the handler config,
     *  which may be already used within some thread.
     */

    if (domain->event_handler.uri) {

        if (ov_dict_get(domain->event_handler.uri, config.name)) {
            goto error;
        }
    }

    ov_websocket_message_config msg_config =
        (ov_websocket_message_config){.userdata = self,
                                      .max_frames = IMPL_MAX_FRAMES_FOR_JSON,
                                      .callback = cb_wss_io_to_json,
                                      .close = config.callback.close};

    if (wss_io) msg_config = *wss_io;

    if (0 != config.name[0]) strncat(msg_config.uri, config.name, PATH_MAX - 1);

    // perform WS config reset on no userdata
    if (!config.userdata) msg_config = (ov_websocket_message_config){0};

    // perform WS URI config reset on no processing
    if (0 == config.callback.process) msg_config.callback = NULL;

    if (!ov_web_server_configure_websocket_callback(
            self, hostname, msg_config)) {
        goto error;
    }

    /* Check for reset */

    if (NULL == config.userdata) {

        domain->event_handler.uri = ov_dict_free(domain->event_handler.uri);
        goto done;
    }

    if (NULL == config.callback.process) {

        ov_dict_del(domain->event_handler.uri, config.name);

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
        goto error;
    }

done:
    ov_log_debug(
        "configured event_handler callback for domain "
        "%.*s URI %s",
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

ov_web_server_config ov_web_server_config_from_json(
    const ov_json_value *value){

    ov_web_server_config out = (ov_web_server_config){0};

    if (!value) goto error;

    const ov_json_value *config = NULL;

    config = ov_json_object_get(value, OV_KEY_WEBSERVER);
    if (!config) config = value;

    if (ov_json_is_true(ov_json_object_get(config, OV_KEY_DEBUG)))
        out.debug = true;

    ov_json_value *sockets = ov_json_object_get(config, OV_KEY_SOCKETS);
    ov_json_value *https = ov_json_object_get(sockets, OV_KEY_HTTPS);

    if (https) {

        out.socket = ov_socket_configuration_from_json(
            https, (ov_socket_configuration){0});
    }

    out.http_message = ov_http_message_config_from_json(config);
    out.websocket_frame = ov_websocket_frame_config_from_json(config);

    const char *domain_config_path =
        ov_json_string_get(ov_json_object_get(config, OV_KEY_DOMAINS));

    if (domain_config_path)
        if (!strncpy(out.domain_config_path, domain_config_path, PATH_MAX))
            goto error;

    out.limits.max_content_bytes_per_websocket_frame = ov_json_number_get(
        ov_json_get(config, "/" OV_KEY_LIMITS "/" OV_WEBSOCKET_KEY));

    ov_json_value *mime = ov_json_object_get(config, OV_KEY_MIME);

    const char *str = ov_json_string_get(ov_json_object_get(mime, OV_KEY_PATH));
    if (!str) goto error;

    if (strlen(str) > PATH_MAX) goto error;

    if (!strncpy(out.mime.path, str, PATH_MAX)) goto error;

    str = ov_json_string_get(ov_json_object_get(mime, OV_KEY_EXTENSION));
    if (str) {

        if (strlen(str) > PATH_MAX) goto error;

        if (!strncpy(out.mime.ext, str, PATH_MAX)) goto error;
    }

    return out;
error:
    return (ov_web_server_config){0};
}