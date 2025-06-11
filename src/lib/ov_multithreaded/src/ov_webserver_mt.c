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
        @file           ov_webserver_mt.c
        @author         Töpfer, Markus

        @date           2025-06-02


        ------------------------------------------------------------------------
*/
#include "../include/ov_webserver_mt.h"

#include <ov_base/ov_thread_lock.h>
#include <ov_base/ov_thread_loop.h>
#include <ov_base/ov_file.h>
#include <ov_base/ov_string.h>

#include <ov_core/ov_io_web.h>

#include <ov_format/ov_file_format.h>

#define OV_WEBSERVER_MT_MAGIC_BYTES 0x423f
#define MSG_HTTP 0x000F

struct ov_webserver_mt {

    uint16_t magic_bytes;
    ov_webserver_mt_config config;

    ov_io_web *io;

    ov_thread_loop *tloop;

    /* dedicated private registry for formats */
    ov_file_format_registry *formats;
};

/*
 *      ------------------------------------------------------------------------
 *
 *      #threaded FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

typedef struct thread_msg {

    ov_thread_message generic;
    ov_http_message *msg;

} thread_msg;

/*----------------------------------------------------------------------------*/

static ov_thread_message *thread_msg_free(ov_thread_message *msg){

    if (!msg) return NULL;
    if (msg->type != MSG_HTTP) return msg;

    thread_msg *self = (thread_msg*) msg;
    self->msg = ov_http_message_free(self->msg);
    self->generic.json_message = ov_json_value_free(self->generic.json_message);
    self = ov_data_pointer_free(self);
    return NULL;
}

/*----------------------------------------------------------------------------*/

static thread_msg *thread_msg_create(int socket, ov_http_message *http){

    thread_msg *msg = calloc(1, sizeof(thread_msg));
    if (!msg) return NULL;

    msg->generic.magic_bytes = OV_THREAD_MESSAGE_MAGIC_BYTES;
    msg->generic.type = MSG_HTTP;
    msg->generic.socket = socket;
    msg->generic.free = thread_msg_free;    
    msg->msg = http;

    return msg;

}

/*----------------------------------------------------------------------------*/

static bool process_https_status(ov_webserver_mt *self,
                                 int socket,
                                 ov_http_message *msg) {

    if (!self || !msg) goto error;

    // WE do not expect any status messages to be received by the server. 

error:
    ov_io_web_close(self->io, socket);
    ov_http_message_free(msg);
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

static ov_http_message *answer_gethead_range(ov_webserver_mt *self, 
    const char *path,
    const ov_file_format_desc *format, 
    const ov_http_message *req,
    const ov_http_header *range,
    bool set_body){

    ov_http_message *msg = NULL;

    uint8_t *buffer = NULL;
    size_t size = 0;

    size_t from = 0;
    size_t to = 0;
    size_t all = 0;

    if (!parse_content_range(range, &from, &to)) goto error;

    if (OV_FILE_SUCCESS !=
        ov_file_read_partial(path, &buffer, &size, from, to, &all)) {
        ov_log_error("Failed to partial read file %s", path);
        goto error;
    }

    msg = ov_http_create_status_string(req->config, req->version, 
        206, OV_HTTP_PARTIAL_CONTENT);

    if (!msg) goto error;

    if (!ov_http_message_add_header_string(
            msg, OV_KEY_SERVER, self->config.name))
        goto error;

    if (ov_file_encoding_is_utf8(path)) {

        if (!ov_http_message_add_content_type(msg, format->mime, "utf-8"))
            goto error;

    } else {

        if (!ov_http_message_add_content_type(msg, format->mime, NULL)) goto error;
    }

    if (!ov_http_message_set_date(msg)) goto error;

    if (!ov_http_message_set_content_length(msg, size)) goto error;

    if (to == 0) to = all;

    if (!ov_http_message_set_content_range(msg, all, from, to)) goto error;

    if (!ov_http_message_add_header_string(msg, "Access-Control-Allow-Origin", "*"))
        goto error;

    if (!ov_http_message_close_header(msg)) goto error;

    if (set_body) {
        
        if (!ov_http_message_add_body(
            msg, (ov_memory_pointer){.start = buffer, .length = size}))
        goto error;

    }

    buffer = ov_data_pointer_free(buffer);
    return msg;

error:
    msg = ov_http_message_free(msg);
    buffer = ov_data_pointer_free(buffer);
    return NULL;
}

/*----------------------------------------------------------------------------*/

static ov_http_message *answer_gethead(ov_webserver_mt *self, 
    const char *path,
    const ov_file_format_desc *format, 
    const ov_http_message *req,
    bool set_body){

    ov_http_message *out = NULL;

    uint8_t *buffer = NULL;
    size_t size = 0;

    if (!self || !path || !format || !req) goto error;

    const ov_http_header *range = ov_http_header_get(
        req->header, req->config.header.capacity, "Range");

    if (range)
        return answer_gethead_range(self, path, format, req, range, set_body);

    out = ov_http_create_status_string(req->config, req->version, 200, OV_HTTP_OK);
    if (!out) goto error;

    if (!ov_http_message_add_header_string(
            out, OV_KEY_SERVER, self->config.name))
        goto error;

    if (ov_file_encoding_is_utf8(path)) {

        if (!ov_http_message_add_content_type(out, format->mime, "utf-8"))
            goto error;

    } else {

        if (!ov_http_message_add_content_type(out, format->mime, NULL)) goto error;
    }

    if (OV_FILE_SUCCESS != ov_file_read(path, &buffer, &size)) {
        ov_log_error("Failed to read file %s", path);
        goto error;
    }

    if (!ov_http_message_set_date(out)) goto error;

    if (!ov_http_message_set_content_length(out, size)) goto error;

    if (!ov_http_message_add_header_string(out, "Accept-Ranges", "bytes"))
        goto error;

    if (!ov_http_message_close_header(out)) goto error;

    if (set_body){

        if (!ov_http_message_add_body(
            out, (ov_memory_pointer){.start = buffer, .length = size}))
            goto error;
    }

    buffer = ov_data_pointer_free(buffer);

    return out;
error:
    ov_http_message_free(out);
    buffer = ov_data_pointer_free(buffer);
    return NULL;
}

/*----------------------------------------------------------------------------*/

static bool process_https_gethead(ov_webserver_mt *self,
                               int socket,
                               ov_http_message *msg,
                               bool set_body) {

    ov_file_format_desc format = {0};

    char path[PATH_MAX] = {0};
    ov_http_message *out = NULL;

    OV_ASSERT(self);
    OV_ASSERT(msg);

    if (!ov_io_web_uri_file_path(
            self->io, socket, msg, PATH_MAX, path)) {

        ov_log_error("%s URI request rejected |%.*s|",
                     self->config.name,
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

    } else {
    
        out = answer_gethead(self, path, &format, msg, set_body);
    
    }

    thread_msg *tmsg = thread_msg_create(socket, out);
    if (!tmsg) goto error;

    msg = NULL;

    if (!ov_thread_loop_send_message(self->tloop, ov_thread_message_cast(tmsg), OV_RECEIVER_EVENT_LOOP)){
        thread_msg_free(ov_thread_message_cast(tmsg));
        goto error;
    }

    msg = ov_http_message_free(msg);
    return true;

error:
    if (self) ov_io_web_close(self->io, socket);
    msg = ov_http_message_free(msg);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool handle_http(ov_webserver_mt *self, int socket, ov_http_message *msg){

    if (!self || !msg) goto error;

    if (0 != msg->status.code) return process_https_status(self, socket, msg);

    // (1) check message type support

    if (0 == strncasecmp(OV_HTTP_METHOD_GET,
                         (char *)msg->request.method.start,
                         msg->request.method.length)) {

        return process_https_gethead(self, socket, msg, true);
    }

    if (0 == strncasecmp(OV_HTTP_METHOD_HEAD,
                         (char *)msg->request.method.start,
                         msg->request.method.length)) {

        return process_https_gethead(self, socket, msg, false);
    }

    ov_log_error("%s METHOD type |%.*s| not implemented - closing",
                 self->config.name,
                 (int)msg->request.method.length,
                 (char *)msg->request.method.start);

    ov_io_web_close(self->io, socket);
    ov_http_message_free(msg);
    return true;
error:
    ov_http_message_free(msg);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool handle_in_thread(ov_thread_loop *tloop, ov_thread_message *tmsg){

    if (!tloop || !tmsg) goto error;

    ov_webserver_mt *self = ov_webserver_mt_cast(ov_thread_loop_get_data(tloop));
    if (!self) goto error;

    if (tmsg->type != MSG_HTTP) goto error;

    thread_msg *in = (thread_msg*) tmsg;

    ov_http_message *msg = in->msg;
    in->msg = NULL;

    if (!handle_http(self, tmsg->socket, msg)) 
        goto error;

    ov_thread_message_free(tmsg);
    return true;


error:
    ov_thread_message_free(tmsg);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool handle_in_loop(ov_thread_loop *tloop, ov_thread_message *tmsg){

    if (!tloop || !tmsg) goto error;

    ov_webserver_mt *self = ov_webserver_mt_cast(ov_thread_loop_get_data(tloop));
    if (!self) goto error;

    if (tmsg->type != MSG_HTTP) goto error;

    thread_msg *in = (thread_msg*) tmsg;

    ov_http_message *msg = in->msg;

    if (!ov_io_web_send(self->io, tmsg->socket, msg)) goto error;

    ov_thread_message_free(tmsg);
    return true;

error:
    ov_thread_message_free(tmsg);
    return false;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      #IO FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

static bool cb_https(void *userdata, int socket, ov_http_message *msg){

    thread_msg *tmsg = NULL;

    ov_webserver_mt *self = ov_webserver_mt_cast(userdata);

    if (!self || !msg) goto error;

    tmsg = thread_msg_create(socket, msg);
    if (!tmsg) goto error;

    msg = NULL;

    if (!ov_thread_loop_send_message(self->tloop, ov_thread_message_cast(tmsg), OV_RECEIVER_THREAD))
        goto error;

    return true;

error:
    ov_http_message_free(msg);
    ov_thread_message_free(ov_thread_message_cast(tmsg));
    return false;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      #GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

static bool init_config(ov_webserver_mt_config *config){

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
    
    if (0 == config->socket.host[0])
        config->socket = (ov_socket_configuration) {
            .host = "0.0.0.0",
            .port = 443,
            .type = TCP
        };

    if (0 == config->limits.sockets)
        config->limits.sockets = 
            ov_socket_get_max_supported_runtime_sockets(UINT32_MAX);

    if (0 == config->limits.thread_lock_timeout_usecs)
        config->limits.thread_lock_timeout_usecs = 1000000;

    if (0 == config->limits.message_queue_capacity)
        config->limits.message_queue_capacity = 10000;

    if (0 == config->limits.threads) {

        long number_of_processors = sysconf(_SC_NPROCESSORS_ONLN);
        config->limits.threads = number_of_processors;
    }

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

ov_webserver_mt *ov_webserver_mt_create(ov_webserver_mt_config config){

    ov_webserver_mt *self = NULL;

    if (!init_config(&config)) goto error;

    self = calloc(1, sizeof(ov_webserver_mt));
    if (!self) goto error;

    self->magic_bytes = OV_WEBSERVER_MT_MAGIC_BYTES;
    self->config = config;

    ov_io_web_config io_config = (ov_io_web_config){

        .loop = config.loop,
        .io = config.io,

        .http_config = config.http_config,
        .frame_config = config.frame_config,

        .socket = config.socket,

        .limits.sockets = config.limits.sockets,

        .callbacks.userdata = self,
        .callbacks.https = cb_https
    };

    strncpy(io_config.name, config.name, PATH_MAX);

    self->io = ov_io_web_create(io_config);
    
    if (!self->io){
        ov_log_error("Failed to create IO layer.");
        goto error;
    }

    self->tloop = ov_thread_loop_create(
        config.loop,
        (ov_thread_loop_callbacks){
            .handle_message_in_thread = handle_in_thread,
            .handle_message_in_loop = handle_in_loop
        },
        self);

    if (!self->tloop) {
        ov_log_error("Failed to create Thread loop.");
        goto error;
    }

    if (!ov_thread_loop_reconfigure(self->tloop,
        (ov_thread_loop_config){
            .disable_to_loop_queue = false,
            .message_queue_capacity = config.limits.message_queue_capacity,
            .lock_timeout_usecs = config.limits.thread_lock_timeout_usecs,
            .num_threads = config.limits.threads
        })) {

        ov_log_error("Faield to configure Thread loop.");
        goto error;
    }

    if (!ov_thread_loop_start_threads(self->tloop)) goto error;

    if (0 != config.mime.path[0]) {

        if (!ov_file_format_register_from_json_from_path(
                &self->formats, config.mime.path, config.mime.ext)) {
            goto error;
        }
    }

    return self;

error:
    ov_webserver_mt_free(self);
    return NULL;

}

/*----------------------------------------------------------------------------*/

ov_webserver_mt *ov_webserver_mt_free(ov_webserver_mt *self){

    if (!ov_webserver_mt_cast(self)) return self;

    ov_thread_loop_stop_threads(self->tloop);

    self->io = ov_io_web_free(self->io);
    self->tloop = ov_thread_loop_free(self->tloop);

    self = ov_data_pointer_free(self);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_webserver_mt *ov_webserver_mt_cast(const void *data){

    if (!data) goto error;

    if (*(uint16_t *)data == OV_WEBSERVER_MT_MAGIC_BYTES) 
        return (ov_webserver_mt *)data;

error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

bool ov_webserver_mt_close(ov_webserver_mt *self, int socket){

    if (!self) return false;

    return ov_io_web_close(self->io, socket);

}

/*----------------------------------------------------------------------------*/

bool ov_webserver_mt_configure_uri_event_io(
    ov_webserver_mt *self,
    const char *hostname,
    const ov_event_io_config handler){

    if (!self || !hostname) return false;

    return ov_io_web_configure_uri_event_io(
        self->io, hostname, handler);
}

/*----------------------------------------------------------------------------*/

bool ov_webserver_mt_send_json(ov_webserver_mt *self,
                                    int socket,
                                    ov_json_value const *const data){

    if (!self || !data) return false;

    return ov_io_web_send_json(self->io, socket, data);
}

/*----------------------------------------------------------------------------*/

ov_webserver_mt_config ov_webserver_mt_config_from_json(const ov_json_value *in){

    ov_webserver_mt_config config = {0};

    const ov_json_value *conf = ov_json_object_get(in, OV_KEY_WEBSERVER);
    if (!conf) conf = in;

    const char *name = ov_json_string_get(ov_json_get(conf, "/"OV_KEY_NAME));
    if (name) strncpy(config.name, name, PATH_MAX);

    config.http_config = ov_http_message_config_from_json(conf);
    config.frame_config = ov_websocket_frame_config_from_json(conf);

    config.socket = ov_socket_configuration_from_json(
        ov_json_get(conf, "/"OV_KEY_SOCKET),
        (ov_socket_configuration){0});

    config.limits.sockets = ov_json_number_get(
        ov_json_get(conf, "/"OV_KEY_LIMITS"/"OV_KEY_SOCKETS));

    config.limits.threads = ov_json_number_get(
        ov_json_get(conf, "/"OV_KEY_LIMITS"/"OV_KEY_THREADS));

    config.limits.thread_lock_timeout_usecs = ov_json_number_get(
        ov_json_get(conf, "/"OV_KEY_LIMITS"/"OV_KEY_THREAD_LOCK_TIMEOUT));

    config.limits.message_queue_capacity = ov_json_number_get(
        ov_json_get(conf, "/"OV_KEY_LIMITS"/"OV_KEY_MESSAGE_QUEUE_CAPACITY));

    ov_json_value *mime = ov_json_object_get(conf, OV_KEY_MIME);

    const char *str = ov_json_string_get(ov_json_object_get(mime, OV_KEY_PATH));
    if (!str) goto error;

    if (strlen(str) > PATH_MAX) goto error;

    if (!strncpy(config.mime.path, str, PATH_MAX)) goto error;

    str = ov_json_string_get(ov_json_object_get(mime, OV_KEY_EXTENSION));
    if (str) {

        if (strlen(str) > PATH_MAX) goto error;

        if (!strncpy(config.mime.ext, str, PATH_MAX)) goto error;
    }

    return config;
error:
    return (ov_webserver_mt_config){0};
}