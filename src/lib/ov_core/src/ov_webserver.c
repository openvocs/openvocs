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
        @file           ov_webserver.c
        @author         Töpfer, Markus

        @date           2026-01-31


        ------------------------------------------------------------------------
*/
#include "../include/ov_webserver.h"
#include "../include/ov_webserver_io.h"
#include "../include/ov_mimetype.h"

#include <ov_base/ov_dump.h>
#include <ov_base/ov_file.h>
#include <ov_base/ov_string.h>
#include <ov_base/ov_uri.h>
#include <ov_base/ov_utils.h>

/*----------------------------------------------------------------------------*/

typedef struct Webserver {

    ov_webserver public;

    ov_webserver_io *io;
    bool debug;

} Webserver;

/*----------------------------------------------------------------------------*/

#define AS_WEBSERVER(x)                                                         \
    (((ov_webserver_cast(x) != 0) &&                                            \
      (0x01 == ((ov_webserver *)x)->type))                                      \
         ? (Webserver *)(x)                                                     \
         : 0)

/*
 *      ------------------------------------------------------------------------
 *
 *      PROCESSING FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

static void cb_close(void *userdata, int socket){

    Webserver *self = AS_WEBSERVER(userdata);  
    if (!self) return;

    if (self->public.config.callbacks.close)
        self->public.config.callbacks.close(
            self->public.config.callbacks.userdata,
            socket);

    return;
}

/*----------------------------------------------------------------------------*/

static bool parse_content_range(const ov_http_header *range, size_t *from,
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
    if (!ptr)
        goto error;

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

static bool answer_range(Webserver *self, int socket, const char *path,
                         const ov_http_header *range, const ov_http_message *msg,
                         bool add_body) {

    ov_http_message *response = NULL;

    uint8_t *buffer = NULL;
    size_t size = 0;

    OV_ASSERT(self);
    OV_ASSERT(msg);
    OV_ASSERT(path);
    OV_ASSERT(range);

    size_t from = 0;
    size_t to = 0;
    size_t all = 0;

    if (!parse_content_range(range, &from, &to))
        goto error;

    if (OV_FILE_SUCCESS !=
        ov_file_read_partial(path, &buffer, &size, from, to, &all)) {
        ov_log_error("failed to partial read file %s", path);
        goto error;
    }

    response = ov_http_create_status_string(msg->config, msg->version, 206,
                                             OV_HTTP_PARTIAL_CONTENT);

    if (!ov_http_message_add_header_string(response, "server",
                                            self->public.config.name))
        goto error;

    if (!ov_http_message_set_date(response))
        goto error;

    if (!ov_http_message_set_content_length(response, size))
        goto error;

    if (to == 0)
        to = all;

    if (!ov_http_message_set_content_range(response, all, from, to))
        goto error;

    if (!ov_http_message_add_header_string(response,
                                            "Access-Control-Allow-Origin", "*"))
        goto error;

    if (!ov_http_message_close_header(response))
        goto error;

    if (add_body){

        if (!ov_http_message_add_body(
            response, (ov_memory_pointer){.start = buffer, .length = size}))
            goto error;
    }

    if (!ov_webserver_io_send_http(self->io, socket, response))
        goto error;

    response = ov_http_message_free(response);
    buffer = ov_data_pointer_free(buffer);
    return true;
error:
    response = ov_http_message_free(response);
    buffer = ov_data_pointer_free(buffer);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool process_get(Webserver *self, int socket, const ov_http_message *msg){

    char path[PATH_MAX] = {0};

    ov_http_message *response = NULL;

    uint8_t *buffer = NULL;
    size_t size = 0;

    OV_ASSERT(self);
    OV_ASSERT(msg);

    if (!ov_webserver_io_clean_path(self->io, socket, msg, path, PATH_MAX))
        goto error;

    size_t path_len = strlen(path);

    if (path[path_len-1] == '/')
        strcat(path, "index.html");

    const ov_http_header *range =
        ov_http_header_get(msg->header, msg->config.header.capacity, "Range");

    if (range)
        return answer_range(self, socket, path, range, msg, true);

    if (OV_FILE_SUCCESS != ov_file_read(path, &buffer, &size)) {
        ov_log_error("failed to read file %s", path);
        goto error;
    }

    const char *ext = NULL;
    char *ptr = path + strlen(path);

    while (ptr[0] != '.') {
        ptr--;
        if (ptr == path)
            break;
    }

    ext = ptr + 1;

    const char *mimetype = ov_mimetype_from_file_extension(ext, strlen(ext));

    response = ov_http_create_status_string(
        self->public.config.http, (ov_http_version){.major = 1, .minor = 1},
        200, OV_HTTP_OK);

    if (!ov_http_message_add_header_string(response, 
        "server", 
        self->public.config.name))
        goto error;

    if (!ov_http_message_set_date(response))
        goto error;

    if (!ov_http_message_set_content_length(response, size))
        goto error;

    if (mimetype) {

        if (!ov_http_message_add_content_type(response, mimetype, NULL))
            goto error;

    } else {

        if (!ov_http_message_add_content_type(response, "text/plain", NULL))
            goto error;
    }

    if (!ov_http_message_add_header_string(response, "Accept-Ranges", "bytes"))
        goto error;

    if (!ov_http_message_close_header(response))
        goto error;

    if (!ov_http_message_add_body(
            response, (ov_memory_pointer){.start = buffer, .length = size}))
        goto error;

    if (!ov_webserver_io_send_http(self->io, socket, response))
        goto error;

    if (self->debug)
        ov_log_debug("SEND %.*s", (int)response->buffer->length,
                      (char *)response->buffer->start);

    response = ov_http_message_free(response);
    buffer = ov_data_pointer_free(buffer);
    return true;
error:
    response = ov_http_message_free(response);
    buffer = ov_data_pointer_free(buffer);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool process_head(Webserver *self, int socket, const ov_http_message *msg){

    char path[PATH_MAX] = {0};

    ov_http_message *response = NULL;

    uint8_t *buffer = NULL;
    size_t size = 0;

    OV_ASSERT(self);
    OV_ASSERT(msg);

    if (!ov_webserver_io_clean_path(self->io, socket, msg, path, PATH_MAX))
        goto error;

    const ov_http_header *range =
        ov_http_header_get(msg->header, msg->config.header.capacity, "Range");

    if (range)
        return answer_range(self, socket, path, range, msg, false);

    if (OV_FILE_SUCCESS != ov_file_read(path, &buffer, &size)) {
        ov_log_error("failed to read file %s", path);
        goto error;
    }

    const char *ext = NULL;
    char *ptr = path + strlen(path);

    while (ptr[0] != '.') {
        ptr--;
        if (ptr == path)
            break;
    }

    ext = ptr + 1;

    const char *mimetype = ov_mimetype_from_file_extension(ext, strlen(ext));

    response = ov_http_create_status_string(
        self->public.config.http, (ov_http_version){.major = 1, .minor = 1},
        200, OV_HTTP_OK);

    if (!ov_http_message_add_header_string(response, 
        "server", 
        self->public.config.name))
        goto error;

    if (!ov_http_message_set_date(response))
        goto error;

    if (!ov_http_message_set_content_length(response, size))
        goto error;

    if (mimetype) {

        if (!ov_http_message_add_content_type(response, mimetype, NULL))
            goto error;

    } else {

        if (!ov_http_message_add_content_type(response, "text/plain", NULL))
            goto error;
    }

    if (!ov_http_message_add_header_string(response, "Accept-Ranges", "bytes"))
        goto error;

    if (!ov_http_message_close_header(response))
        goto error;

    if (!ov_webserver_io_send_http(self->io, socket, response))
        goto error;

    if (self->debug)
        ov_log_debug("SEND %.*s", (int)response->buffer->length,
                      (char *)response->buffer->start);

    response = ov_http_message_free(response);
    buffer = ov_data_pointer_free(buffer);
    return true;
error:
    response = ov_http_message_free(response);
    buffer = ov_data_pointer_free(buffer);
    return false;
    
}

/*----------------------------------------------------------------------------*/

static bool cb_io(void *userdata, int socket, const ov_http_message *msg){

    Webserver *self = AS_WEBSERVER(userdata);  
    if (!self || !msg) return false;

    if (ov_http_is_request(msg, OV_HTTP_METHOD_GET))
        return process_get(self, socket, msg);

    if (ov_http_is_request(msg, OV_HTTP_METHOD_HEAD))
        return process_head(self, socket, msg);

    ov_log_debug("METHOD not implemented.");
    return true;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      IMPL FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

static ov_webserver *impl_free(ov_webserver *input){

    Webserver *self = AS_WEBSERVER(input);  
    if (!self) return input;

    self->io = ov_webserver_io_free(self->io);
    self = ov_data_pointer_free(self);
    return NULL;
}

/*----------------------------------------------------------------------------*/

static bool impl_debug(ov_webserver *input, bool on){

    Webserver *self = AS_WEBSERVER(input);
    if (!self) return false;

    self->debug = on;
    ov_webserver_io_set_debug(self->io, on);
    return true;
}

/*----------------------------------------------------------------------------*/

static bool impl_enable_domains(ov_webserver *input, const ov_json_value* config){

    Webserver *self = AS_WEBSERVER(input);
    if (!self) return false;

    return ov_webserver_io_enable_domains(self->io, config);
}

/*----------------------------------------------------------------------------*/

static bool impl_enable_events(ov_webserver *input, 
    const char *domain,
    const char *uri,
    void *userdata,
    void (*callback)(void *userdata, 
                    int socket, 
                    ov_json_value *msg)){

    Webserver *self = AS_WEBSERVER(input);
    if (!self) return false;

    return ov_webserver_io_event_callback(self->io, 
        domain, 
        uri,
        userdata,
        callback);
}

/*----------------------------------------------------------------------------*/

static bool impl_close(ov_webserver *input, int socket){

    Webserver *self = AS_WEBSERVER(input);
    if (!self) return false;

    return ov_webserver_io_close(self->io, socket);
}

/*----------------------------------------------------------------------------*/

static bool impl_send_json(ov_webserver *input, 
    int socket, const ov_json_value *msg){

    Webserver *self = AS_WEBSERVER(input);
    if (!self) return false;

    return ov_webserver_io_send_json(self->io, socket, msg);
}

/*----------------------------------------------------------------------------*/

static bool impl_send_http(ov_webserver *input, 
    int socket, const ov_http_message *msg){

    Webserver *self = AS_WEBSERVER(input);
    if (!self) return false;

    return ov_webserver_io_send_http(self->io, socket, msg);
}

/*----------------------------------------------------------------------------*/

static bool impl_send_plain(ov_webserver *input, 
    int socket, const char *buffer, size_t size){

    Webserver *self = AS_WEBSERVER(input);
    if (!self) return false;

    return ov_webserver_io_send(self->io, socket, buffer, size);
}

/*----------------------------------------------------------------------------*/

static bool init_config(ov_webserver_config *config){

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

/*----------------------------------------------------------------------------*/

ov_webserver *ov_webserver_create(ov_webserver_config config){

    Webserver *self = NULL;
    
    if (!init_config(&config)) goto error;

    self = calloc(1, sizeof(Webserver));
    if (!self) goto error;

    self->public.magic_bytes = OV_WEBSERVER_MAGIC_BYTES;
    self->public.type = 0x01;

    self->public.free = impl_free;
    self->public.debug = impl_debug;
    self->public.enable_domains = impl_enable_domains;
    self->public.enable_events = impl_enable_events;
    self->public.close = impl_close;
    self->public.send.json = impl_send_json;
    self->public.send.http = impl_send_http;
    self->public.send.plain = impl_send_plain;

    ov_webserver_io_config io_config = (ov_webserver_io_config){
        .loop = config.loop,
        .io = config.io,
        .socket = config.socket,
        .http = config.http,
        .frame = config.frame,
        .callbacks.userdata = self,
        .callbacks.close = cb_close,
        .callbacks.io = cb_io
    };

    strncpy(io_config.name, config.name, PATH_MAX);

    self->io = ov_webserver_io_create(io_config);
    if (!self->io) goto error;

    return ov_webserver_cast(self);
error:
    ov_webserver_free(ov_webserver_cast(self));
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_webserver *ov_webserver_free(ov_webserver *self){

    if (!self) return NULL;
    return self->free(self);
}

/*----------------------------------------------------------------------------*/

ov_webserver *ov_webserver_cast(const void *data){

    if (!data)
        return NULL;

    if (*(uint16_t *)data != OV_WEBSERVER_MAGIC_BYTES)
        return NULL;

    return (ov_webserver *)data;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      CONFIGURATION FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_webserver_config ov_webserver_config_from_json(
        const ov_json_value *input){
    
    ov_webserver_config config = {0};

    if (!input) goto error;

    const ov_json_value *item = ov_json_object_get(input, "webserver");
    if (!item) item = input;

    const char *name = ov_json_string_get(ov_json_object_get(item, "name"));
    if (name)
        strncpy(config.name, name, PATH_MAX);

    config.socket =
        ov_socket_configuration_from_json(ov_json_object_get(item, "socket"),
            (ov_socket_configuration){0});

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

        config.http.buffer.max_bytes_recache = ov_json_number_get(
            ov_json_object_get(http, "buffer_size_recache"));

        config.http.transfer.max =
            ov_json_number_get(ov_json_object_get(http, "max_transfer"));

        config.http.chunk.max_bytes =
            ov_json_number_get(ov_json_object_get(http, "max_chunk_bytes"));
    }

    ov_json_value *frame = ov_json_object_get(item, "frame");
    if (frame) {

        config.frame.buffer.default_size =
            ov_json_number_get(ov_json_object_get(http, "buffer_size"));

        config.frame.buffer.max_bytes_recache = ov_json_number_get(
            ov_json_object_get(http, "buffer_size_recache"));
    }

    return config;
error:
    return (ov_webserver_config){0};
}

/*----------------------------------------------------------------------------*/

bool ov_webserver_set_debug(ov_webserver *self, bool on){

    if (!self) return false;
    return self->debug(self, on);
}

/*----------------------------------------------------------------------------*/

bool ov_webserver_enable_domains(
        ov_webserver *self,
        const ov_json_value *config){

    if (!self || !config) return false;

    return self->enable_domains(self, config);
}

/*----------------------------------------------------------------------------*/

bool ov_webserver_enable_event_callback(
        ov_webserver *self,
        const char *domain, 
        const char *uri,
        void *userdata,
        void (*callback)(void *userdata, int socket, ov_json_value *msg)){

    if (!self) return false;

    return self->enable_events(self, domain, uri, userdata, callback);
}

/*----------------------------------------------------------------------------*/

bool ov_webserver_close(ov_webserver *self, int socket){

    if (!self) return false;

    return self->close(self, socket);
}

/*----------------------------------------------------------------------------*/

bool ov_webserver_register_close(ov_webserver *self, void *userdata,
    void (*callback)(void *userdata, int socket)){

    if (!self) return false;

    self->config.callbacks.userdata = userdata;
    self->config.callbacks.close = callback;
    return true;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      SEND FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

bool ov_webserver_send_json(
        ov_webserver *self, int socket, const ov_json_value *msg){

    if (!self) return false;
    return self->send.json(self, socket, msg);
}

/*----------------------------------------------------------------------------*/

bool ov_webserver_send_http(
        ov_webserver *self, int socket, const ov_http_message *msg){

    if (!self) return false;
    return self->send.http(self, socket, msg);
}

/*----------------------------------------------------------------------------*/

bool ov_webserver_send(
        ov_webserver *self, int socket, const char *buffer, size_t size){

    if (!self) return false;
    return self->send.plain(self, socket, buffer, size);
}