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
        @file           ov_webserver_minimal.c
        @author         Markus Töpfer

        @date           2021-12-15


        ------------------------------------------------------------------------
*/
#include "../include/ov_webserver_minimal.h"

#include <ov_base/ov_json.h>
#include <ov_format/ov_file_format.h>

#define OV_WEBSERVER_MINIMAL_MAGIC_BYTE 0x8f00
#define IMPL_MAX_FRAMES_FOR_JSON 10000

struct ov_webserver_minimal {

    const uint16_t magic_byte;
    ov_webserver_minimal_config config;

    ov_webserver_base *base;

    /* dedicated private registry for formats */
    ov_file_format_registry *formats;
};

/*----------------------------------------------------------------------------*/

static ov_webserver_minimal init = {

    .magic_byte = OV_WEBSERVER_MINIMAL_MAGIC_BYTE

};

/*----------------------------------------------------------------------------*/

ov_webserver_minimal *ov_webserver_minimal_cast(const void *self) {

    /* used internal as we have void* from time to time e.g. for SSL */

    if (!self) goto error;

    if (*(uint16_t *)self == OV_WEBSERVER_MINIMAL_MAGIC_BYTE)
        return (ov_webserver_minimal *)self;
error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

static bool cb_accept(void *userdata, int server, int socket) {

    ov_webserver_minimal *srv = ov_webserver_minimal_cast(userdata);

    if (!srv) goto error;

    /*
     *  Here some accept restrictions may be implemented in the future.
     *  e.g. white / blacklist of client networks.
     */

    if (srv->config.callback.accept)
        return srv->config.callback.accept(
            srv->config.callback.userdata, server, socket);

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static void cb_close(void *userdata, int socket) {

    ov_webserver_minimal *srv = ov_webserver_minimal_cast(userdata);

    if (!srv) goto error;

    if (socket < 0) goto error;

    if (srv->config.callback.close)
        srv->config.callback.close(srv->config.callback.userdata, socket);

error:
    return;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      #HTTP PROCESSING
 *
 *      ------------------------------------------------------------------------
 */

static bool process_https_status(ov_webserver_minimal *srv,
                                 int socket,
                                 ov_http_message *msg) {

    /* Processing start for http status of base callback @see cb_https */

    if (!srv || !msg) goto error;

    if (srv->config.base.debug)
        ov_log_debug(
            "%s unexpected STATUS message received "
            "at %i %i|%s - closing",
            srv->config.base.name,
            socket,
            msg->status.code,
            msg->status.phrase);

    // ERROR - close the connection, we do not expect STATUS messages yet

error:
    ov_http_message_free(msg);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool process_https_get(ov_webserver_minimal *srv,
                              int socket,
                              ov_http_message *msg) {

    ov_file_format_desc format = {0};
    char path[PATH_MAX] = {0};
    ov_http_message *out = NULL;

    OV_ASSERT(srv);
    OV_ASSERT(msg);

    if (!ov_webserver_base_uri_file_path(
            srv->base, socket, msg, PATH_MAX, path)) {

        ov_log_error("%s URI request rejected |%.*s|",
                     srv->config.base.name,
                     (int)msg->request.uri.length,
                     (char *)msg->request.uri.start);

        goto error;
    }

    format = ov_file_format_get_desc(srv->formats, path);

    if (-1 == format.desc.bytes) {

        /* At this state the file is either not present at disk,
         * OR the file type is not enabled within the formats,
         * which is a restriction by configuration.
         * So this is NOT some error to log in general but during debug */

        out = ov_http_status_not_found(msg);

    } else if (0 == format.mime[0]) {

        /* Mime type not enabled */

        out = ov_http_status_not_found(msg);

    } else if (!ov_webserver_base_answer_get(srv->base, socket, format, msg)) {

        /* failed to add format for send */
        out = NULL;
    }

    if (out) {

        if (!ov_webserver_base_send_secure(srv->base, socket, out->buffer)) {

            ov_log_error("%s failed to send HTTPs response at %i",
                         srv->config.base.name,
                         socket);
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

static bool process_https_head(ov_webserver_minimal *srv,
                               int socket,
                               ov_http_message *msg) {

    ov_file_format_desc format = {0};
    char path[PATH_MAX] = {0};
    ov_http_message *out = NULL;

    OV_ASSERT(srv);
    OV_ASSERT(msg);

    if (!ov_webserver_base_uri_file_path(
            srv->base, socket, msg, PATH_MAX, path)) {

        ov_log_error("%s URI request rejected |%.*s|",
                     srv->config.base.name,
                     (int)msg->request.uri.length,
                     (char *)msg->request.uri.start);

        goto error;
    }

    format = ov_file_format_get_desc(srv->formats, path);

    if (-1 == format.desc.bytes) {

        /* At this state the file is either not present at disk,
         * OR the file type is not enabled within the formats,
         * which is a restriction by configuration.
         * So this is NOT some error to log in general but during debug */

        out = ov_http_status_not_found(msg);

    } else if (0 == format.mime[0]) {

        /* Mime type not enabled */

        out = ov_http_status_not_found(msg);

    } else if (!ov_webserver_base_answer_head(srv->base, socket, format, msg)) {

        /* failed to add format for send */
        out = NULL;
    }

    if (out) {

        if (!ov_webserver_base_send_secure(srv->base, socket, out->buffer)) {

            ov_log_error("%s failed to send HTTPs response at %i",
                         srv->config.base.name,
                         socket);
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

static bool process_https_request(ov_webserver_minimal *srv,
                                  int socket,
                                  ov_http_message *msg) {

    if (!srv || !msg) goto error;

    /*  (1) Check message type support */

    if (0 == strncasecmp(OV_HTTP_METHOD_GET,
                         (char *)msg->request.method.start,
                         msg->request.method.length)) {

        return process_https_get(srv, socket, msg);
    }

    if (0 == strncasecmp(OV_HTTP_METHOD_HEAD,
                         (char *)msg->request.method.start,
                         msg->request.method.length)) {

        return process_https_head(srv, socket, msg);
    }

    ov_log_error("%s METHOD type |%.*s| not implemented - closing",
                 srv->config.base.name,
                 (int)msg->request.method.length,
                 (char *)msg->request.method.start);

error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool cb_https(void *userdata, int socket, ov_http_message *msg) {

    ov_webserver_minimal *srv = ov_webserver_minimal_cast(userdata);

    if (!srv || (socket < 0) || !msg) goto error;

    if (0 != msg->status.code) return process_https_status(srv, socket, msg);

    return process_https_request(srv, socket, msg);

error:
    ov_http_message_free(msg);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool send_json(void *userdata, int socket, const ov_json_value *out) {

    ov_webserver_minimal *srv = ov_webserver_minimal_cast(userdata);
    if (!srv) return false;

    return ov_webserver_base_send_json(srv->base, socket, out);
}

/*----------------------------------------------------------------------------*/

static bool cb_wss_io_to_json(void *userdata,
                              int socket,
                              const ov_memory_pointer domain,
                              const char *uri,
                              ov_memory_pointer content,
                              bool text) {

    /* Callback set for some JSON over WSS IO in ov_webserver_base
     * e.g. https://openvocs.org/openvocs_client */

    ov_json_value *value = NULL;
    ov_webserver_minimal *srv = ov_webserver_minimal_cast(userdata);
    if (!srv) goto error;

    if (!text) goto error;

    if (socket < 0) goto error;

    ov_domain *host = ov_webserver_base_find_domain(srv->base, domain);
    if (!host) goto error;

    ov_event_io_config *handler = ov_dict_get(host->event_handler.uri, uri);

    if (!handler || !handler->callback.process) goto error;

    value = ov_json_value_from_string((char *)content.start, content.length);

    if (srv->config.base.debug) {

        if (!value) {

            ov_log_error("%s wss io at %i for %.*s uri %s not JSON - closing",
                         srv->config.base.name,
                         socket,
                         (int)domain.length,
                         domain.start,
                         uri);

            goto error;

        } else {

            ov_log_debug("%s wss JSON io at %i for %.*s uri %s \n%.*s",
                         srv->config.base.name,
                         socket,
                         (int)domain.length,
                         domain.start,
                         uri,
                         content.length,
                         (char *)content.start);
        }
    }

    if (!value) goto error;

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

ov_webserver_minimal *ov_webserver_minimal_create(
    ov_webserver_minimal_config config) {

    ov_webserver_minimal *srv = NULL;

    srv = calloc(1, sizeof(ov_webserver_minimal));
    if (!srv) goto error;

    memcpy(srv, &init, sizeof(init));

    config.base.callback.userdata = srv;
    config.base.callback.accept = cb_accept;
    config.base.callback.https = cb_https;
    config.base.callback.close = cb_close;

    srv->config = config;
    srv->base = ov_webserver_base_create(config.base);
    if (!srv->base) goto error;

    if (0 != config.mime.path[0]) {

        if (!ov_file_format_register_from_json_from_path(
                &srv->formats, config.mime.path, config.mime.ext)) {
            goto error;
        }
    }

    return srv;
error:
    ov_webserver_minimal_free(srv);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_webserver_minimal *ov_webserver_minimal_free(ov_webserver_minimal *self) {

    if (!ov_webserver_minimal_cast(self)) return self;

    self->base = ov_webserver_base_free(self->base);
    ov_file_format_free_registry(&self->formats);
    self = ov_data_pointer_free(self);
    return NULL;
}

/*----------------------------------------------------------------------------*/

bool ov_webserver_minimal_close(ov_webserver_minimal *self, int socket) {

    if (!self || !self->base || (0 > socket)) return false;

    return ov_webserver_base_close(self->base, socket);
}

/*----------------------------------------------------------------------------*/

bool ov_webserver_minimal_configure_uri_event_io(
    ov_webserver_minimal *self,
    const ov_memory_pointer hostname,
    const ov_event_io_config input) {

    if (!self || !hostname.start || (0 == input.name[0])) return false;

    ov_websocket_message_config wss_io_handler =
        (ov_websocket_message_config){.userdata = self,
                                      .max_frames = IMPL_MAX_FRAMES_FOR_JSON,
                                      .callback = cb_wss_io_to_json,
                                      .close = NULL};

    return ov_webserver_base_configure_uri_event_io(
        self->base, hostname, input, &wss_io_handler);
}

/*
 *      ------------------------------------------------------------------------
 *
 *      CONFIG FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_webserver_minimal_config ov_webserver_minimal_config_from_json(
    const ov_json_value *value) {

    ov_webserver_minimal_config out = {0};

    if (!value) goto error;

    const ov_json_value *config = NULL;
    config = ov_json_object_get(value, OV_KEY_WEBSERVER);
    if (!config) config = value;

    out.base = ov_webserver_base_config_from_json(config);

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
    return (ov_webserver_minimal_config){0};
}

bool ov_webserver_minimal_send_json(ov_webserver_minimal *self,
                                    int socket,
                                    ov_json_value const *const data) {

    if (!self || !data) return false;

    return ov_webserver_base_send_json(self->base, socket, data);
}
