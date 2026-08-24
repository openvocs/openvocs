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
        @file           ov_event_cc.c
        @author         Töpfer, Markus

        @date           2026-08-24


        ------------------------------------------------------------------------
*/
#include "../include/ov_event_cc.h"

#include "../include/ov_event_api.h"

#include <ov_base/ov_json_io_buffer.h>
#include <ov_base/ov_id.h>
#include <ov_base/ov_dict.h>
#include <ov_base/ov_string.h>

/*----------------------------------------------------------------------------*/

#define OV_EVENT_CC_MAGIC_BYTES 0xccee

/*----------------------------------------------------------------------------*/

struct ov_event_cc {

    uint16_t magic_bytes;
    ov_event_cc_config config;

    ov_id id;
    pid_t pid;

    int socket;

    ov_json_io_buffer *buffer;

    ov_dict *events;
};

typedef struct Event {

    void (*callback)(ov_event_cc *self, int socket, const ov_json_value *msg);

} Event;


/*
 *      ------------------------------------------------------------------------
 *
 *      SOCKET FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

static void cb_io_close(void *userdata, int connection){

    ov_event_cc *self = ov_event_cc_cast(userdata);

    ov_log_error("CC connection closed");
    UNUSED(connection);
    self->socket = -1;

    return;
}

/*----------------------------------------------------------------------------*/

static void cb_io_connected(void *userdata, int connection){

    ov_event_cc *self = ov_event_cc_cast(userdata);

    ov_log_error("CC connection opened %i", connection);
    self->socket = connection;

    ov_json_value *msg = ov_event_api_message_create("register", NULL, 0);
    ov_json_value *par = ov_event_api_set_parameter(msg);
    ov_json_object_set(par, "name", ov_json_string(self->config.name));
    ov_json_object_set(par, "uuid", ov_json_string(self->id));
    ov_json_object_set(par, "pid", ov_json_number(self->pid));

    char *str = ov_json_value_to_string(msg);
    ov_io_send(self->config.io, self->socket, (ov_memory_pointer){
        .start = (uint8_t*) str,
        .length = strlen(str)
    });

    ov_log_debug("send %s", str);

    str = ov_data_pointer_free(str);
    msg = ov_json_value_free(msg);

    return;
}

/*----------------------------------------------------------------------------*/

static bool cb_io(void *userdata, int connection, const char *domain, 
    const ov_memory_pointer buffer){

    UNUSED(domain);

    ov_event_cc *self = ov_event_cc_cast(userdata);
    return ov_json_io_buffer_push(self->buffer, connection, buffer);
}

/*----------------------------------------------------------------------------*/

static void json_io_success(void *userdata, int socket, ov_json_value *msg){

    ov_event_cc *self = ov_event_cc_cast(userdata);
    if (!self || !msg) goto error;

    const char *name = ov_event_api_get_event(msg);
    Event *event = ov_dict_get(self->events, name);
    if (!event) goto error;

    event->callback(self, socket, msg);

    ov_json_value_free(msg);
    return;
error:
    ov_json_value_free(msg);
    return;
}

/*----------------------------------------------------------------------------*/

static void json_io_failure(void *userdata, int socket){

    ov_event_cc *self = ov_event_cc_cast(userdata);
    if (!self) goto error;

    ov_io_close(self->config.io, socket);
error:
    return;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      EVENT FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

static void event_register(
    ov_event_cc *self, int socket, const ov_json_value *msg){

    if (!self || !msg) goto error;

    char *str = ov_json_value_to_string(msg);
    ov_log_info("CC register at %i | %s", socket, str);
    str = ov_data_pointer_free(str);

error:
    return;
}


/*----------------------------------------------------------------------------*/

static bool register_event(ov_event_cc *self, const char *name, 
    void (*callback)(ov_event_cc *self, int socket, const ov_json_value *msg)){

    if (!self || !name || !callback) goto error;

    Event *event = calloc(1, sizeof(event));
    if (!event) goto error;

    event->callback = callback;

    return ov_dict_set(self->events, ov_string_dup(name), event, NULL);
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool register_events(ov_event_cc *self){

    if (!self) goto error;

    if (!register_event(self,
        "register", 
        event_register)) goto error;

    return true;
error:
    return false;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

static bool init_config(ov_event_cc_config *config){

    if (!config || !config->loop || !config->io)
        goto error;

    if (0 == config->name[0])
        strncat(config->name, "APP", OV_HOST_NAME_MAX -1);

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

ov_event_cc *ov_event_cc_create(ov_event_cc_config config){

    ov_event_cc *self = NULL;
    if (!init_config(&config)) goto error;

    self = calloc(1, sizeof(ov_event_cc));
    if (!self) goto error;

    self->magic_bytes = OV_EVENT_CC_MAGIC_BYTES;
    self->config = config;

    self->socket = -1;
    ov_id_fill_with_uuid(self->id);
    self->pid = getpid();

    ov_dict_config d_config = ov_dict_string_key_config(255);
    d_config.value.data_function.free = ov_data_pointer_free;

    self->buffer = ov_json_io_buffer_create((ov_json_io_buffer_config){
        .debug = false,
        .objects_only = false,
        .callback.userdata = self,
        .callback.success = json_io_success,
        .callback.failure = json_io_failure
    });

    if (!self->buffer) goto error;

    self->events = ov_dict_create(d_config);
    if (!self->events) goto error;

    if (!register_events(self)) goto error;

    return self;
error:
    ov_event_cc_free(self);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_event_cc *ov_event_cc_free(ov_event_cc *self){

    if (!ov_event_cc_cast(self)) return self;

    if (-1 != self->socket) ov_io_close(self->config.io, self->socket);
    self->buffer = ov_json_io_buffer_free(self->buffer);
    self->events = ov_dict_free(self->events);
    self = ov_data_pointer_free(self);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_event_cc *ov_event_cc_cast(const void *self){

    if (!self)
        goto error;

    if (*(uint16_t *)self == OV_EVENT_CC_MAGIC_BYTES)
        return (ov_event_cc *)self;

error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

bool ov_event_cc_connect(ov_event_cc *self, ov_socket_configuration socket){

    if (!self) goto error;

    if (-1 != self->socket){
        ov_io_close(self->config.io, self->socket);
        self->socket = -1;
    }

    ov_io_socket_config config = (ov_io_socket_config){
        .auto_reconnect = true,
        .socket = socket,
        .callbacks.userdata = self,
        .callbacks.connected = cb_io_connected,
        .callbacks.io = cb_io,
        .callbacks.close = cb_io_close
    };

    ov_io_open_connection(self->config.io, config);

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_event_cc_log(ov_event_cc *self, int socket, const ov_json_value *msg){

    if (!self || !msg) goto error;

    if (-1 == self->socket) goto error;

    ov_json_value *out = ov_event_api_message_create("cc_log", NULL, 0);
    ov_json_value *par = ov_event_api_set_parameter(out);
    ov_json_value *val = NULL;
    ov_json_value_copy((void**)&val, msg);
    ov_json_object_set(par, "message", val);
    ov_json_object_set(par, "name", ov_json_string(self->config.name));
    ov_json_object_set(par, "uuid", ov_json_string(self->id));
    ov_json_object_set(par, "pid", ov_json_number(self->pid));
    ov_json_object_set(par, "socket", ov_json_number(socket));

    char *str = ov_json_value_to_string(out);
    ov_io_send(self->config.io, self->socket, (ov_memory_pointer){
        .start = (uint8_t*)str,
        .length = strlen(str)
    });
    str = ov_data_pointer_free(str);
    out = ov_json_value_free(out);
    return true;
error:
    return false;
}