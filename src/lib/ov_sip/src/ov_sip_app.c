/***
        ------------------------------------------------------------------------

        Copyright (c) 2022 German Aerospace Center DLR e.V. (GSOC)

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

        @author         Michael J. Beer, DLR/GSOC

        ------------------------------------------------------------------------
*/

#include "../include/ov_sip_app.h"
#include "../include/ov_sip_serde.h"
#include <ov_base/ov_string.h>
#include <ov_base/ov_utils.h>
#include <ov_value/ov_serde_app.h>

/*----------------------------------------------------------------------------*/

static bool warn_if_not_null(void const *ptr, char const *msg) {

    if (0 != ptr) {
        ov_log_warning(msg);
        return false;
    } else {
        return true;
    }
}

/*----------------------------------------------------------------------------*/

#define MAGIC_BYTES 0x12aa0172

struct ov_sip_app {

    uint32_t magic_bytes;

    ov_serde *serde;
    ov_serde_app *serde_app;

    ov_hashtable *method_handlers;

    void (*response_handler)(ov_sip_message const *msg,
                             int fd,
                             void *additional);

    void *additional;
    bool are_methods_case_sensitive;

    void (*cb_closed)(int sckt, void *additional);
    void (*cb_reconnected)(int sckt, void *additional);
    void (*cb_accepted)(int sckt, void *additional);
};

/*----------------------------------------------------------------------------*/

static ov_sip_app *as_sip_app(void *ptr) {

    ov_sip_app *app = ptr;

    if (0 == app) {

        ov_log_error("Not a SIP app - 0 pointer");
        return 0;

    } else if (MAGIC_BYTES != app->magic_bytes) {

        ov_log_error("Not a SIP app - wrong magic bytes");
        return 0;

    } else {

        return app;
    }
}

/*----------------------------------------------------------------------------*/

static ov_serde_app *get_serde_app(ov_sip_app *self) {

    if (ov_ptr_valid(
            as_sip_app(self), "Cannot get serde app - not a sip app")) {

        return self->serde_app;

    } else {

        return 0;
    }
}

/*----------------------------------------------------------------------------*/

static void cb_closed_wrapper(int sckt, void *additional) {

    ov_sip_app *app = as_sip_app(additional);

    if ((0 != app) && (0 != app->cb_closed)) {
        app->cb_closed(sckt, app->additional);
    }
}

/*----------------------------------------------------------------------------*/

static void cb_reconnected_wrapper(int sckt, void *additional) {

    ov_sip_app *app = as_sip_app(additional);

    if ((0 != app) && (0 != app->cb_reconnected)) {
        app->cb_reconnected(sckt, app->additional);
    }
}

/*----------------------------------------------------------------------------*/

static void cb_accepted_wrapper(int sckt, void *additional) {

    ov_sip_app *app = as_sip_app(additional);

    if ((0 != app) && (0 != app->cb_accepted)) {
        app->cb_accepted(sckt, app->additional);
    }
}

/*----------------------------------------------------------------------------*/

static ov_serde_app *create_serde_app(ov_sip_app *app,
                                      ov_serde *serde,
                                      char const *name,
                                      ov_event_loop *loop,
                                      ov_sip_app_configuration cfg) {

    ov_serde_app_configuration scfg = {

        .log_io = cfg.log_io,

        .serde = serde,
        .reconnect_interval_secs = cfg.reconnect_interval_secs,
        .accept_to_io_timeout_secs = 3600,

        .cb_closed = cb_closed_wrapper,
        .cb_reconnected = cb_reconnected_wrapper,
        .cb_accepted = cb_accepted_wrapper,

        .additional = app,

    };

    return ov_serde_app_create(name, loop, scfg);
}

/*----------------------------------------------------------------------------*/

static bool initialize_sip_app_with_serde_app(ov_sip_app *app,
                                              ov_serde_app *sapp,
                                              ov_sip_app_configuration cfg) {

    if ((!ov_ptr_valid(app, "Invalid SIP app")) ||
        (!ov_ptr_valid(sapp, "Invalid Serde app"))) {

        return false;

    } else {

        app->magic_bytes = MAGIC_BYTES;
        app->serde_app = sapp;
        app->method_handlers = ov_hashtable_create_c_string(10);
        app->additional = cfg.additional;
        app->are_methods_case_sensitive = cfg.are_methods_case_sensitive;

        app->cb_accepted = cfg.cb_accepted;
        app->cb_closed = cfg.cb_closed;
        app->cb_reconnected = cfg.cb_reconnected;

        return true;
    }
}

/*----------------------------------------------------------------------------*/

static void (*get_handler(ov_sip_app *app, char const *method))(
    ov_sip_message const *, int, void *) {

    if (ov_ptr_valid(app, "No app given")) {
        return ov_hashtable_get(app->method_handlers, method);
    } else {
        return 0;
    }
}

/*----------------------------------------------------------------------------*/

static void (*get_response_handler(ov_sip_app *app))(ov_sip_message const *,
                                                     int,
                                                     void *) {

    if (ov_ptr_valid(app, "No app given")) {
        return app->response_handler;
    } else {
        return 0;
    }
}

/*----------------------------------------------------------------------------*/

static void *get_additional(ov_sip_app *app) {

    if (ov_ptr_valid(app, "No app given")) {
        return app->additional;
    } else {
        return 0;
    }
}

/*----------------------------------------------------------------------------*/

static void treat_sip_response(ov_sip_message const *msg,
                               int fh,
                               ov_sip_app *app) {

    ov_log_debug("Got a SIP response");

    void (*handler)(ov_sip_message const *, int, void *) =
        get_response_handler(app);

    if (0 != handler) {
        handler(msg, fh, get_additional(app));

    } else {

        ov_log_warning("No handler for SIP response");
    }
}

/*----------------------------------------------------------------------------*/

static void treat_sip_request(ov_sip_message const *msg,
                              int fh,
                              ov_sip_app *app) {

    ov_log_debug("Got another SIP request");

    char const *method = ov_sip_message_method(msg);

    void (*handler)(ov_sip_message const *, int, void *) =
        get_handler(app, method);

    if (0 != handler) {

        handler(msg, fh, get_additional(app));

    } else {

        ov_log_warning("No handler for %s", ov_string_sanitize(method));
    }
}

/*----------------------------------------------------------------------------*/

void cb_sip_message_handler(void *data, int fh, void *additional) {

    ov_sip_app *app = as_sip_app(additional);
    ov_sip_message *msg = ov_sip_message_cast(data);

    switch (ov_sip_message_type_get(msg)) {

        case OV_SIP_REQUEST:
            treat_sip_request(msg, fh, app);
            break;

        case OV_SIP_RESPONSE:
            treat_sip_response(msg, fh, app);
            break;

        case OV_SIP_INVALID:
            ov_log_error("Invalid SIP message received");
            break;
    };

    ov_sip_message_free(msg);
}

/*----------------------------------------------------------------------------*/

static bool register_sip_handler(ov_serde_app *sapp) {

    return ov_serde_app_register_handler(
        sapp, OV_SIP_SERDE_TYPE, cb_sip_message_handler);
}

/*----------------------------------------------------------------------------*/

static bool initialize_sip_app(ov_sip_app *app,
                               char const *name,
                               ov_event_loop *loop,
                               ov_sip_app_configuration cfg) {

    app->serde = ov_sip_serde();
    ov_serde_app *sapp = create_serde_app(app, app->serde, name, loop, cfg);

    if ((!initialize_sip_app_with_serde_app(app, sapp, cfg)) ||
        (!register_sip_handler(sapp))) {

        ov_serde_app_free(sapp);
        app->serde = ov_serde_free(app->serde);

        return false;

    } else {

        return true;
    }
}

/*----------------------------------------------------------------------------*/

ov_sip_app *ov_sip_app_create(char const *name,
                              ov_event_loop *loop,
                              ov_sip_app_configuration cfg) {

    ov_sip_app *app = calloc(1, sizeof(ov_sip_app));

    if (!initialize_sip_app(app, name, loop, cfg)) {

        app = ov_free(app);
    }

    return app;
}

/*----------------------------------------------------------------------------*/

ov_sip_app *ov_sip_app_free(ov_sip_app *app) {

    if (!ov_ptr_valid(app, "No SIP App given")) {

        return app;

    } else {

        app->serde_app = ov_serde_app_free(app->serde_app);
        app->serde = ov_serde_free(app->serde);
        app->method_handlers = ov_hashtable_free(app->method_handlers);

        OV_ASSERT(0 == app->serde_app);
        OV_ASSERT(0 == app->method_handlers);

        return ov_free(app);
    }
}

/*----------------------------------------------------------------------------*/

bool ov_sip_app_register_handler(ov_sip_app *self,
                                 char const *method,
                                 void (*handler)(ov_sip_message const *message,
                                                 int fh,
                                                 void *additional)) {

    self = as_sip_app(self);

    if ((!ov_ptr_valid(self, "No SIP app given")) ||
        (!ov_ptr_valid(method, "No method")) ||
        (!ov_ptr_valid(handler, "No handler"))) {

        return false;

    } else if (0 != ov_hashtable_set(self->method_handlers, method, handler)) {

        ov_log_warning(
            "Overwriting old handler for %s", ov_string_sanitize(method));
        return true;

    } else {

        return true;
    }
}

/*----------------------------------------------------------------------------*/

bool ov_sip_app_register_response_handler(
    ov_sip_app *self,
    void (*handler)(ov_sip_message const *message,
                    int socket,
                    void *additional)) {

    self = as_sip_app(self);

    if ((!ov_ptr_valid(self, "No SIP app given")) ||
        (!ov_ptr_valid(handler, "No handler"))) {

        return false;

    } else {

        warn_if_not_null(
            self->response_handler, "Overwriting response handler");
        ov_log_warning("Overwriting old response handler");
        self->response_handler = handler;
        return true;
    }
}

/*----------------------------------------------------------------------------*/

bool ov_sip_app_connect(ov_sip_app *self, ov_socket_configuration config) {

    return ov_serde_app_connect(get_serde_app(self), config);
}

/*----------------------------------------------------------------------------*/

bool ov_sip_app_open_server_socket(ov_sip_app *self,
                                   ov_socket_configuration config) {

    return ov_serde_app_open_server_socket(get_serde_app(self), config);
}

/*----------------------------------------------------------------------------*/

int ov_sip_app_close(ov_sip_app *self, int fd) {

    return ov_serde_app_close(get_serde_app(self), fd);
}

/*----------------------------------------------------------------------------*/

bool ov_sip_app_send(ov_sip_app *self, int fd, ov_sip_message *msg) {

    self = as_sip_app(self);
    if (fd < 1) return false;
    if (!msg || !self) return false;

    ov_serde_data data = {
        .data = msg,
        .data_type = OV_SIP_SERDE_TYPE,
    };

    return ov_serde_app_send(get_serde_app(self), fd, data);
}

/*----------------------------------------------------------------------------*/
