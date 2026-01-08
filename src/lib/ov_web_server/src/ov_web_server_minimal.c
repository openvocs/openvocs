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
        @file           ov_web_server_minimal.c
        @author         Töpfer, Markus

        @date           2025-11-12


        ------------------------------------------------------------------------
*/
#include "../include/ov_web_server_minimal.h"

#include <ov_base/ov_file.h>
#include <ov_base/ov_string.h>

/*----------------------------------------------------------------------------*/

#define OV_WEB_SERVER_MINIMAL_MAGIC_BYTE 0x0001

#define IMPL_MAX_FRAMES_FOR_JSON 10000

/*----------------------------------------------------------------------------*/

struct ov_web_server_minimal {

    uint16_t magic_byte;
    ov_web_server_config config;

    ov_web_server *core;

    /* dedicated private registry for formats */
    ov_file_format_registry *formats;
};

/*
 *      ------------------------------------------------------------------------
 *
 *      #HTTP PROCESSING
 *
 *      ------------------------------------------------------------------------
 */

static bool process_https_status(ov_web_server_minimal *self, int socket,
                                 ov_http_message *msg) {

    if (!self || !msg)
        goto error;

    ov_log_debug("unexpected STATUS message received "
                 "at %i %i|%s - closing",
                 socket, msg->status.code, msg->status.phrase);

error:
    ov_http_message_free(msg);
    return false;
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

static bool answer_head_range(ov_web_server_minimal *self,
                              ov_file_format_desc fmt,
                              const ov_http_message *request, const char *path,
                              const ov_http_header *range, int socket) {

    OV_ASSERT(self);
    OV_ASSERT(request);
    OV_ASSERT(path);
    OV_ASSERT(range);

    ov_http_message *msg = NULL;

    uint8_t *buffer = NULL;
    size_t size = 0;

    if (request) {

        msg = ov_http_create_status_string(request->config, request->version,
                                           206, OV_HTTP_PARTIAL_CONTENT);

    } else {

        msg = ov_http_create_status_string(
            self->config.http_message,
            (ov_http_version){.major = 1, .minor = 1}, 200, OV_HTTP_OK);
    }

    if (!msg)
        goto error;

    size_t from = 0;
    size_t to = 0;
    size_t all = 0;

    if (!parse_content_range(range, &from, &to))
        goto error;

    if (OV_FILE_SUCCESS !=
        ov_file_read_partial(path, &buffer, &size, from, to, &all)) {
        ov_log_error("Failed to partial read file %s", path);
        goto error;
    }

    if (!ov_http_message_add_header_string(msg, OV_KEY_SERVER, "openvocs"))
        goto error;

    if (ov_file_encoding_is_utf8(path)) {

        if (!ov_http_message_add_content_type(msg, fmt.mime, "utf-8"))
            goto error;

    } else {

        if (!ov_http_message_add_content_type(msg, fmt.mime, NULL))
            goto error;
    }

    if (!ov_http_message_set_date(msg))
        goto error;

    if (!ov_http_message_set_content_length(msg, size))
        goto error;

    if (to == 0)
        to = all;

    if (!ov_http_message_set_content_range(msg, all, from, to))
        goto error;

    if (!ov_http_message_add_header_string(msg, "Access-Control-Allow-Origin",
                                           "*"))
        goto error;

    if (!ov_http_message_close_header(msg))
        goto error;

    if (!ov_http_message_add_body(
            msg, (ov_memory_pointer){.start = buffer, .length = size}))
        goto error;

    if (!ov_web_server_send(self->core, socket, msg->buffer))
        goto error;

    buffer = ov_data_pointer_free(buffer);
    msg = ov_http_message_free(msg);
    return true;
error:
    // format = ov_format_close(format);
    buffer = ov_data_pointer_free(buffer);
    msg = ov_http_message_free(msg);
    return false;
}

/*----------------------------------------------------------------------------*/

bool answer_head(ov_web_server_minimal *self, int socket,
                 ov_file_format_desc fmt, const ov_http_message *request) {

    // ov_format *format = NULL;
    ov_http_message *msg = NULL;

    uint8_t *buffer = NULL;
    size_t size = 0;

    if (!self || (socket < 0) || !request)
        goto error;

    /* (1) check input */

    if (1 > fmt.desc.bytes) {

        goto error;
    }

    if (0 == fmt.mime[0]) {

        goto error;
    }

    /* (3) create clean path */

    char path[PATH_MAX] = {0};

    if (!ov_web_server_uri_file_path(self->core, socket, request, PATH_MAX,
                                     path))
        goto error;

    const ov_http_header *range = ov_http_header_get(
        request->header, request->config.header.capacity, "Range");

    if (range)
        return answer_head_range(self, fmt, request, path, range, socket);

    if (OV_FILE_SUCCESS != ov_file_read(path, &buffer, &size)) {
        ov_log_error("Failed to read file %s", path);
        goto error;
    }

    if (request) {

        msg = ov_http_create_status_string(request->config, request->version,
                                           200, OV_HTTP_OK);

    } else {

        msg = ov_http_create_status_string(
            self->config.http_message,
            (ov_http_version){.major = 1, .minor = 1}, 200, OV_HTTP_OK);
    }

    if (!msg)
        goto error;

    if (!ov_http_message_add_header_string(msg, OV_KEY_SERVER, "openvocs"))
        goto error;

    if (ov_file_encoding_is_utf8(path)) {

        if (!ov_http_message_add_content_type(msg, fmt.mime, "utf-8"))
            goto error;

    } else {

        if (!ov_http_message_add_content_type(msg, fmt.mime, NULL))
            goto error;
    }

    if (!ov_http_message_set_date(msg))
        goto error;

    if (!ov_http_message_set_content_length(msg, size))
        goto error;

    if (!ov_http_message_add_header_string(msg, "Accept-Ranges", "bytes"))
        goto error;

    if (!ov_http_message_close_header(msg))
        goto error;

    if (!ov_web_server_send(self->core, socket, msg->buffer))
        goto error;

    buffer = ov_data_pointer_free(buffer);
    msg = ov_http_message_free(msg);
    return true;
error:
    // format = ov_format_close(format);
    buffer = ov_data_pointer_free(buffer);
    msg = ov_http_message_free(msg);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool process_https_head(ov_web_server_minimal *self, int socket,
                               ov_http_message *msg) {

    ov_file_format_desc format = {0};
    char path[PATH_MAX] = {0};
    ov_http_message *out = NULL;

    OV_ASSERT(self);
    OV_ASSERT(msg);

    if (!ov_web_server_uri_file_path(self->core, socket, msg, PATH_MAX, path)) {

        ov_log_error("URI request rejected |%.*s|",
                     (int)msg->request.uri.length,
                     (char *)msg->request.uri.start);

        goto error;
    }

    format = ov_file_format_get_desc(self->formats, path);

    if (-1 == format.desc.bytes) {

        /* At this state the file is either not present at disk,
         * OR the file type is not enabled within the formats,
         * which is a restriction by configuration.
         * So this is NOT some error to log in general but during debug */

        out = ov_http_status_not_found(msg);

    } else if (0 == format.mime[0]) {

        /* Mime type not enabled */

        out = ov_http_status_not_found(msg);

    } else if (!answer_head(self, socket, format, msg)) {

        /* failed to add format for send */
        out = NULL;
    }

    if (out) {

        if (!ov_web_server_send(self->core, socket, out->buffer)) {

            ov_log_error("failed to send HTTPs response at %i", socket);
        }

        /* close in any case (either answer send or failed to send) */
        goto error;
    }

    msg = ov_http_message_free(msg);
    return true;

error:
    msg = ov_http_message_free(msg);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool answer_get_range(ov_web_server_minimal *self,
                             ov_file_format_desc fmt,
                             const ov_http_message *request, const char *path,
                             const ov_http_header *range, int socket) {

    OV_ASSERT(self);
    OV_ASSERT(request);
    OV_ASSERT(path);
    OV_ASSERT(range);

    ov_http_message *msg = NULL;

    uint8_t *buffer = NULL;
    size_t size = 0;

    if (request) {

        msg = ov_http_create_status_string(request->config, request->version,
                                           206, OV_HTTP_PARTIAL_CONTENT);

    } else {

        msg = ov_http_create_status_string(
            self->config.http_message,
            (ov_http_version){.major = 1, .minor = 1}, 200, OV_HTTP_OK);
    }

    if (!msg)
        goto error;

    size_t from = 0;
    size_t to = 0;
    size_t all = 0;

    if (!parse_content_range(range, &from, &to))
        goto error;

    if (OV_FILE_SUCCESS !=
        ov_file_read_partial(path, &buffer, &size, from, to, &all)) {
        ov_log_error("Failed to partial read file %s", path);
        goto error;
    }

    if (!ov_http_message_add_header_string(msg, OV_KEY_SERVER, "openvocs"))
        goto error;

    if (ov_file_encoding_is_utf8(path)) {

        if (!ov_http_message_add_content_type(msg, fmt.mime, "utf-8"))
            goto error;

    } else {

        if (!ov_http_message_add_content_type(msg, fmt.mime, NULL))
            goto error;
    }

    if (!ov_http_message_set_date(msg))
        goto error;

    if (!ov_http_message_set_content_length(msg, size))
        goto error;

    if (to == 0)
        to = all;

    if (!ov_http_message_set_content_range(msg, all, from, to))
        goto error;

    if (!ov_http_message_add_header_string(msg, "Access-Control-Allow-Origin",
                                           "*"))
        goto error;

    if (!ov_http_message_close_header(msg))
        goto error;

    if (!ov_http_message_add_body(
            msg, (ov_memory_pointer){.start = buffer, .length = size}))
        goto error;

    if (!ov_web_server_send(self->core, socket, msg->buffer))
        goto error;

    buffer = ov_data_pointer_free(buffer);
    msg = ov_http_message_free(msg);
    return true;
error:
    // format = ov_format_close(format);
    buffer = ov_data_pointer_free(buffer);
    msg = ov_http_message_free(msg);
    return false;
}

/*----------------------------------------------------------------------------*/

bool answer_get(ov_web_server_minimal *self, int socket,
                ov_file_format_desc fmt, const ov_http_message *request) {

    // ov_format *format = NULL;
    ov_http_message *msg = NULL;

    uint8_t *buffer = NULL;
    size_t size = 0;

    if (!self || (socket < 0) || !request)
        goto error;

    /* (1) check input */

    if (1 > fmt.desc.bytes) {

        goto error;
    }

    if (0 == fmt.mime[0]) {

        goto error;
    }

    if (!ov_http_is_request(request, OV_HTTP_METHOD_GET)) {

        goto error;
    }

    /* (2) create clean path */

    char path[PATH_MAX] = {0};

    if (!ov_web_server_uri_file_path(self->core, socket, request, PATH_MAX,
                                     path)) {

        goto error;
    }

    const ov_http_header *range = ov_http_header_get(
        request->header, request->config.header.capacity, "Range");

    if (range)
        return answer_get_range(self, fmt, request, path, range, socket);

    if (OV_FILE_SUCCESS != ov_file_read(path, &buffer, &size)) {
        ov_log_error("Failed to read file %s", path);
        goto error;
    }

    if (request) {

        msg = ov_http_create_status_string(request->config, request->version,
                                           200, OV_HTTP_OK);

    } else {

        msg = ov_http_create_status_string(
            self->config.http_message,
            (ov_http_version){.major = 1, .minor = 1}, 200, OV_HTTP_OK);
    }

    if (!msg)
        goto error;

    if (!ov_http_message_add_header_string(msg, OV_KEY_SERVER, "openvocs"))
        goto error;

    if (ov_file_encoding_is_utf8(path)) {

        if (!ov_http_message_add_content_type(msg, fmt.mime, "utf-8"))
            goto error;

    } else {

        if (!ov_http_message_add_content_type(msg, fmt.mime, NULL))
            goto error;
    }

    if (!ov_http_message_set_date(msg))
        goto error;

    if (!ov_http_message_set_content_length(msg, size))
        goto error;

    if (!ov_http_message_add_header_string(msg, "Accept-Ranges", "bytes"))
        goto error;

    if (!ov_http_message_close_header(msg))
        goto error;

    if (!ov_http_message_add_body(
            msg, (ov_memory_pointer){.start = buffer, .length = size}))
        goto error;

    if (!ov_web_server_send(self->core, socket, msg->buffer))
        goto error;

    buffer = ov_data_pointer_free(buffer);
    msg = ov_http_message_free(msg);
    return true;
error:
    // format = ov_format_close(format);
    buffer = ov_data_pointer_free(buffer);
    msg = ov_http_message_free(msg);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool process_https_get(ov_web_server_minimal *self, int socket,
                              ov_http_message *msg) {

    ov_file_format_desc format = {0};
    char path[PATH_MAX] = {0};
    ov_http_message *out = NULL;

    OV_ASSERT(self);
    OV_ASSERT(msg);

    if (!ov_web_server_uri_file_path(self->core, socket, msg, PATH_MAX, path)) {

        ov_log_error("URI request rejected |%.*s|",
                     (int)msg->request.uri.length,
                     (char *)msg->request.uri.start);

        goto error;
    }

    format = ov_file_format_get_desc(self->formats, path);

    if (-1 == format.desc.bytes) {

        /* At this state the file is either not present at disk,
         * OR the file type is not enabled within the formats,
         * which is a restriction by configuration.
         * So this is NOT some error to log in general but during debug */

        out = ov_http_status_not_found(msg);

    } else if (0 == format.mime[0]) {

        /* Mime type not enabled */

        out = ov_http_status_not_found(msg);

    } else if (!answer_get(self, socket, format, msg)) {

        /* failed to add format for send */
        out = NULL;
    }

    if (out) {

        ov_web_server_send(self->core, socket, out->buffer);
        /* close in any case (either answer send or failed to send) */
        goto error;
    }

    msg = ov_http_message_free(msg);
    return true;

error:
    msg = ov_http_message_free(msg);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool process_https_request(ov_web_server_minimal *self, int socket,
                                  ov_http_message *msg) {

    if (!self || !msg)
        goto error;

    /*  (1) Check message type support */

    if (0 == strncasecmp(OV_HTTP_METHOD_GET, (char *)msg->request.method.start,
                         msg->request.method.length)) {

        return process_https_get(self, socket, msg);
    }

    if (0 == strncasecmp(OV_HTTP_METHOD_HEAD, (char *)msg->request.method.start,
                         msg->request.method.length)) {

        return process_https_head(self, socket, msg);
    }

    ov_log_error("METHOD type |%.*s| not implemented - closing",
                 (int)msg->request.method.length,
                 (char *)msg->request.method.start);

error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool cb_io_https(void *userdata, int socket, ov_http_message *msg) {

    ov_web_server_minimal *self = ov_web_server_minimal_cast(userdata);
    if (!self || !msg)
        goto error;

    if (0 != msg->status.code)
        return process_https_status(self, socket, msg);

    return process_https_request(self, socket, msg);

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static void cb_io_close(void *userdata, int connection_socket) {

    ov_web_server_minimal *self = ov_web_server_minimal_cast(userdata);
    if (!self)
        goto error;

    ov_web_server_close_connection(self->core, connection_socket);

error:
    return;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_web_server_minimal *
ov_web_server_minimal_create(ov_web_server_config config) {

    ov_web_server_minimal *self = calloc(1, sizeof(ov_web_server_minimal));
    if (!self)
        goto error;

    self->magic_byte = OV_WEB_SERVER_MINIMAL_MAGIC_BYTE;

    config.callback.userdata = self;
    config.callback.https = cb_io_https;
    config.callback.close = cb_io_close;

    self->config = config;

    self->core = ov_web_server_create(config);
    if (!self->core) {
        ov_log_error("Failed to create webserver core.");
        goto error;
    }

    if (0 != config.mime.path[0]) {

        if (!ov_file_format_register_from_json_from_path(
                &self->formats, config.mime.path, config.mime.ext)) {
            goto error;
        }
    }

    return self;
error:
    ov_web_server_minimal_free(self);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_web_server_minimal *ov_web_server_minimal_free(ov_web_server_minimal *self) {

    if (!ov_web_server_minimal_cast(self))
        goto error;

    self->core = ov_web_server_free(self->core);
    self = ov_data_pointer_free(self);
    return NULL;
error:
    return self;
}

/*----------------------------------------------------------------------------*/

ov_web_server_minimal *ov_web_server_minimal_cast(const void *self) {

    if (!self)
        goto error;

    if (*(uint16_t *)self == OV_WEB_SERVER_MINIMAL_MAGIC_BYTE)
        return (ov_web_server_minimal *)self;
error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

bool ov_web_server_minimal_close(ov_web_server_minimal *self, int socket) {

    if (!self)
        goto error;
    return ov_web_server_close_connection(self->core, socket);
error:
    return false;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      SEND FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

bool ov_web_server_minimal_send(ov_web_server_minimal *self, int socket,
                                const ov_buffer *data) {

    if (!self)
        goto error;

    return ov_web_server_send(self->core, socket, data);
error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_web_server_minimal_send_json(ov_web_server_minimal *self, int socket,
                                     ov_json_value const *const data) {

    if (!self)
        goto error;

    return ov_web_server_send_json(self->core, socket, data);
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

static bool send_json(void *userdata, int socket, const ov_json_value *out) {

    ov_web_server_minimal *srv = ov_web_server_minimal_cast(userdata);
    if (!srv)
        return false;

    return ov_web_server_minimal_send_json(srv, socket, out);
}

/*----------------------------------------------------------------------------*/

static bool cb_wss_io_to_json(void *userdata, int socket,
                              const ov_memory_pointer domain, const char *uri,
                              ov_memory_pointer content, bool text) {

    /* Callback set for some JSON over WSS IO in ov_webserver_base
     * e.g. https://openvocs.org/openvocs_client */

    ov_json_value *value = NULL;
    ov_web_server_minimal *srv = ov_web_server_minimal_cast(userdata);
    if (!srv)
        goto error;

    if (!text)
        goto error;

    if (socket < 0)
        goto error;

    ov_domain *host =
        ov_web_server_find_domain(srv->core, domain.start, domain.length);
    if (!host)
        goto error;

    ov_event_io_config *handler = ov_dict_get(host->event_handler.uri, uri);

    if (!handler || !handler->callback.process)
        goto error;

    value = ov_json_value_from_string((char *)content.start, content.length);

    if (!value)
        goto error;

    /* call processing handler for uri */

    ov_event_parameter params =
        (ov_event_parameter){.send.instance = srv, .send.send = send_json};

    memcpy(params.uri.name, uri, strlen(uri));
    memcpy(params.domain.name, domain.start, domain.length);

    if (!handler->callback.process(handler->userdata, socket, &params, value)) {

        /* Handler process will take over the JSON value,
         * so we need to NULL the value in case of error */

        value = NULL;
        goto error;
    }

    return true;
error:
    ov_json_value_free(value);
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_web_server_minimal_configure_uri_event_io(
    ov_web_server_minimal *self, const ov_memory_pointer hostname,
    const ov_event_io_config input) {

    if (!self || !hostname.start || (0 == input.name[0]))
        return false;

    ov_websocket_message_config wss_io_handler =
        (ov_websocket_message_config){.userdata = self,
                                      .max_frames = IMPL_MAX_FRAMES_FOR_JSON,
                                      .callback = cb_wss_io_to_json,
                                      .close = NULL};

    return ov_web_server_configure_uri_event_io(self->core, hostname, input,
                                                &wss_io_handler);
}