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
        @file           ov_event_app.c
        @author         Töpfer, Markus

        @date           2026-08-21


        ------------------------------------------------------------------------
*/
#include "../include/ov_event_app.h"
#include "../include/ov_event_api.h"
#include "../include/ov_socket_storage.h"
#include "../include/ov_client_registry.h"

#include <ov_base/ov_json_io_buffer.h>
#include <ov_base/ov_node.h>
#include <ov_base/ov_id.h>
#include <ov_base/ov_dict.h>
#include <ov_base/ov_string.h>
#include <ov_base/ov_file.h>
#include <ov_base/ov_error_codes.h>

#include <unistd.h>

/*----------------------------------------------------------------------------*/

#define OV_EVENT_APP_MAGIC_BYTES 0x42dd

/*----------------------------------------------------------------------------*/

struct ov_event_app {

    uint16_t magic_bytes;
    ov_event_app_config config;

    ov_id id;
    pid_t pid;

    ov_dict *events;
    ov_json_io_buffer *json_io_buffer;

    ov_client_registry *registry;
    ov_socket_storage *connections;

    int cc;

};

/*----------------------------------------------------------------------------*/

typedef struct ListenerEvent {

    ov_node node;

    void *userdata;
    void (*callback) (
        void *userdata, const char *name, int socket, const ov_json_value *msg);

} ListenerEvent;

/*----------------------------------------------------------------------------*/

typedef struct Event {

    ListenerEvent *listener;
    ov_dict *remote;

} Event;

/*----------------------------------------------------------------------------*/

static Event *event_create(){

    Event *event = calloc(1, sizeof(Event));
    if (!event) return NULL;

    event->remote = ov_dict_create((ov_dict_intptr_key_config(255)));
    return event;
}

/*----------------------------------------------------------------------------*/

static void *event_free(void *self){

    Event *event = (Event*) self;
    if (!event) return NULL;

    ListenerEvent *next = ov_node_pop((void**)&event->listener);

    while(next){

        next = ov_data_pointer_free(next);
        next = ov_node_pop((void**)&event->listener);
    }

    event->remote = ov_dict_free(event->remote);

    event = ov_data_pointer_free(event);
    return NULL;
}

/*----------------------------------------------------------------------------*/

static bool check_user_login(ov_event_app *self, int socket){

    ov_json_value *data = ov_socket_storage_get(self->connections, socket);
    const char *user = ov_json_string_get(ov_json_get(data, "/user"));
   
    if (!user){

        data = ov_socket_storage_get(self->connections, socket);
    
        if (!ov_json_is_true(ov_json_object_get(data, "auth"))) {
            goto error;
        }
    }

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static void broker_login(void *userdata, const char *name, int socket, 
    const ov_json_value *msg){

    ov_json_value *out = NULL;

    char buffer[2048] = {0};
    size_t size = 2048;
    unsigned char *ptr = (unsigned char *) buffer;

    ov_event_app *self = ov_event_app_cast(userdata);
    if (!self || !name || !msg) goto error;

    const char *password = ov_json_string_get(
        ov_json_get(msg, "/parameter/password"));

    if (!password){

        out = ov_event_api_create_error_response(
                msg, 
                OV_ERROR_CODE_AUTH,
                OV_ERROR_DESC_AUTH);

        goto response;
    }

    if (OV_FILE_SUCCESS != ov_file_read(self->config.password_path, &ptr, &size)){

        out = ov_event_api_create_error_response(
                msg, 
                OV_ERROR_CODE_PROCESSING_ERROR,
                OV_ERROR_DESC_PROCESSING_ERROR);

        goto response;
    }

    if (buffer[size - 1] == '\n') buffer[size - 1] = 0;

    if (0 != ov_string_compare(password, buffer)){

        out = ov_event_api_create_error_response(
                msg, 
                OV_ERROR_CODE_AUTH,
                OV_ERROR_DESC_AUTH);

        goto response;

    }

    ov_json_value *data = ov_socket_storage_get(self->connections, socket);
    if (!data) goto error;

    ov_json_object_set(data, "auth", ov_json_true());

    out = ov_event_api_create_success_response(msg);

response:
    
    char *str = ov_json_value_to_string(out);
    if (!str) goto error;

    ov_io_send(self->config.io, socket, (ov_memory_pointer){
        .start = (uint8_t*) str,
        .length = strlen(str)
    });

    str = ov_data_pointer_free(str);
error:
    out = ov_json_value_free(out);
    return;
}

/*----------------------------------------------------------------------------*/

static void broker_publish(void *userdata, const char *name, int socket, 
    const ov_json_value *msg){

    ov_json_value *out = NULL;

    ov_event_app *self = ov_event_app_cast(userdata);
    if (!self || !name || !msg) goto error;

    const char *event_name = ov_json_string_get(
        ov_json_get(msg, "/parameter/event"));

    if (!event_name){

        out = ov_event_api_create_error_response(
                msg, 
                OV_ERROR_CODE_PARAMETER_ERROR,
                OV_ERROR_DESC_PARAMETER_ERROR);

        goto response;

    }

    if (!check_user_login(self, socket)){

        out = ov_event_api_create_error_response(
                msg, 
                OV_ERROR_CODE_AUTH,
                OV_ERROR_DESC_AUTH);

        goto response;
    }

    Event *event = ov_dict_get(self->events, event_name);
    if (!event){

        event = event_create();

        if (!ov_dict_set(self->events, ov_string_dup(event_name), event, NULL))
            goto error;

    }

    if (!ov_dict_set(event->remote, (void*)(intptr_t) socket, NULL, NULL))
        goto error;

    out = ov_event_api_create_success_response(msg);

response:

    char *str = ov_json_value_to_string(out);
    if (!str) goto error;

    ov_io_send(self->config.io, socket, (ov_memory_pointer){
        .start = (uint8_t*) str,
        .length = strlen(str)
    });

    str = ov_data_pointer_free(str);
error:
    out = ov_json_value_free(out);
    return;
}

/*----------------------------------------------------------------------------*/

static void broker_subscribe(void *userdata, const char *name, int socket, 
    const ov_json_value *msg){

    ov_json_value *out = NULL;

    ov_event_app *self = ov_event_app_cast(userdata);
    if (!self || !name || !msg) goto error;

    const char *event_name = ov_json_string_get(
        ov_json_get(msg, "/parameter/event"));

    if (!event_name){

        out = ov_event_api_create_error_response(
                msg, 
                OV_ERROR_CODE_PARAMETER_ERROR,
                OV_ERROR_DESC_PARAMETER_ERROR);

        goto response;

    }

    if (!check_user_login(self, socket)){

        out = ov_event_api_create_error_response(
                msg, 
                OV_ERROR_CODE_AUTH,
                OV_ERROR_DESC_AUTH);

        goto response;
    }

    Event *event = ov_dict_get(self->events, event_name);
    if (!event){

        out = ov_event_api_create_error_response(
            msg,
            OV_ERROR_CODE_UNKNOWN_EVENT_ERROR,
            OV_ERROR_DESC_UNKNOWN_EVENT_ERROR);

        goto response;

    }

    if (!ov_dict_set(event->remote, (void*)(intptr_t) socket, NULL, NULL))
        goto error;

    out = ov_event_api_create_success_response(msg);

response:

    char *str = ov_json_value_to_string(out);
    if (!str) goto error;

    ov_io_send(self->config.io, socket, (ov_memory_pointer){
        .start = (uint8_t*) str,
        .length = strlen(str)
    });

    str = ov_data_pointer_free(str);
error:
    out = ov_json_value_free(out);
    return;
}

/*----------------------------------------------------------------------------*/

static void broker_functions(void *userdata, const char *name, int socket, 
    const ov_json_value *msg){

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;

    ov_event_app *self = ov_event_app_cast(userdata);
    if (!self || !name || !msg) goto error;

    if (!check_user_login(self, socket)){

        out = ov_event_api_create_error_response(
                    msg, 
                    OV_ERROR_CODE_AUTH,
                    OV_ERROR_DESC_AUTH);

        goto response;
    }

    val = ov_event_app_get_functions(self);
    if (!val) goto error;

    out = ov_event_api_create_success_response(msg);
    ov_json_value *par = ov_event_api_set_parameter(out);
    ov_json_object_set(par, "functions", val);

response:

    char *str = ov_json_value_to_string(out);
    if (!str) goto error;

    ov_io_send(self->config.io, socket, (ov_memory_pointer){
        .start = (uint8_t*) str,
        .length = strlen(str)
    });

    str = ov_data_pointer_free(str);
error:
    out = ov_json_value_free(out);
    return;
}

/*----------------------------------------------------------------------------*/

static void broker_forward(void *userdata, const char *name, int socket, 
    const ov_json_value *msg){

    ov_json_value *out = NULL;

    ov_event_app *self = ov_event_app_cast(userdata);
    if (!self || !name || !msg) goto error;

    if (!check_user_login(self, socket)){

        out = ov_event_api_create_error_response(
                    msg, 
                    OV_ERROR_CODE_AUTH,
                    OV_ERROR_DESC_AUTH);

        goto response;
    }

    const char *client_id = ov_json_string_get(ov_json_get(msg, "/client_id"));
    const ov_json_value *message = ov_json_get(msg, "/message");

    if (!client_id || !message){

        out = ov_event_api_create_error_response(
                msg, 
                OV_ERROR_CODE_PARAMETER_ERROR,
                OV_ERROR_DESC_PARAMETER_ERROR);

        goto response;
    }

    int client_socket = ov_client_registry_get_socket(self->registry, client_id);

    if (-1 == client_socket){

        out = ov_event_api_create_error_response(
                msg, 
                OV_ERROR_CODE_PARAMETER_ERROR,
                "client unknown");

        goto response;

    }

    char *str = ov_json_value_to_string(message);
    if (!str) goto error;

    ov_io_send(self->config.io, client_socket, (ov_memory_pointer){
        .start = (uint8_t*) str,
        .length = strlen(str)
    });

    str = ov_data_pointer_free(str);
    out = ov_event_api_create_success_response(msg);

response:

    str = ov_json_value_to_string(out);
    if (!str) goto error;

    ov_io_send(self->config.io, socket, (ov_memory_pointer){
        .start = (uint8_t*) str,
        .length = strlen(str)
    });

    str = ov_data_pointer_free(str);
error:
    out = ov_json_value_free(out);
    return;
}

/*----------------------------------------------------------------------------*/

static bool register_events(ov_event_app *self){

    if (!self) goto error;

    if (!ov_event_app_register(
        self, 
        "broker_login", 
        self, 
        broker_login))
        goto error;

    if (!ov_event_app_register(
        self, 
        "publish", 
        self, 
        broker_publish))
        goto error;

    if (!ov_event_app_register(
        self, 
        "subscribe", 
        self, 
        broker_subscribe))
        goto error;

    if (!ov_event_app_register(
        self, 
        "forward", 
        self, 
        broker_forward))
        goto error;

    if (!ov_event_app_register(
        self, 
        "functions", 
        self, 
        broker_functions))
        goto error;

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static void json_io_success(void *userdata, int socket, ov_json_value *msg){

    ov_event_app *self = ov_event_app_cast(userdata);
    if (!self || !msg) goto error;

    ov_event_app_push(self, socket, msg);
    return;
error:
    ov_json_value_free(msg);
    return;
}

/*----------------------------------------------------------------------------*/

static void json_io_failure(void *userdata, int socket){

    ov_event_app *self = ov_event_app_cast(userdata);
    if (!self) goto error;

    ov_io_close(self->config.io, socket);
error:
    return;
}

/*----------------------------------------------------------------------------*/

static bool init_config(ov_event_app_config *config){

    if (!config || !config->loop || !config->io) goto error;

    if (0 == config->password_path[0]){
        ov_log_error("Event APP without broker password.");
    }

    if (0 == config->name[0]){
        strncat(config->name, "APP", OV_HOST_NAME_MAX);
    }

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static void cb_connected_cc(void *userdata, int connection){

    ov_event_app *self = ov_event_app_cast(userdata);

    self->cc = connection;
    ov_log_info("Connection to CC established.");

    ov_json_value *out = ov_event_api_message_create("Register", NULL, 0);
    send_to_cc(self, connection, out);
    out = ov_json_value_free(out);

    return;
}

/*----------------------------------------------------------------------------*/

static bool cb_io_cc(void *userdata, int connection, const char *domain, 
    const ov_memory_pointer buffer){

    UNUSED(domain);

    ov_event_app *self = ov_event_app_cast(userdata);
    return ov_json_io_buffer_push(self->json_io_buffer, connection, buffer);
}

/*----------------------------------------------------------------------------*/

static void cb_close_cc(void *userdata, int socket){

    UNUSED(socket);

    ov_event_app *self = ov_event_app_cast(userdata);

    self->cc = -1;
    ov_log_error("Connection to CC lost.");

    if (self->config.callbacks.close)
        self->config.callbacks.close(self->config.callbacks.userdata, socket);

    return;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_event_app *ov_event_app_create(ov_event_app_config config){

    ov_event_app *self = NULL;

    if (!init_config(&config)) goto error;

    self = calloc(1, sizeof(ov_event_app));
    if (!self) goto error;

    self->magic_bytes = OV_EVENT_APP_MAGIC_BYTES;
    self->config = config;

    ov_id_fill_with_uuid(self->id);
    self->pid = getpid();

    ov_dict_config d_config = ov_dict_string_key_config(255);
    d_config.value.data_function.free = event_free;

    self->events = ov_dict_create(d_config);
    if (!self->events) goto error;

    self->json_io_buffer = ov_json_io_buffer_create((ov_json_io_buffer_config){
        .debug = false,
        .objects_only = false,
        .callback.userdata = self,
        .callback.success = json_io_success,
        .callback.failure = json_io_failure
    });

    if (!self->json_io_buffer) goto error;

    self->connections = ov_socket_storage_create();
    if (!self->connections) goto error;

    if (!register_events(self)) goto error;

    self->registry = ov_client_registry_create();

    if (0 != self->config.command_and_control.host[0]){

        self->cc = ov_io_open_connection(self->config.io,
            (ov_io_socket_config){
                .auto_reconnect = true,
                .socket = self->config.command_and_control,
                .callbacks.userdata = self,
                .callbacks.accept = NULL,
                .callbacks.io = cb_io_cc,
                .callbacks.close = cb_close_cc,
                .callbacks.connected = cb_connected_cc
            });
    }

    return self;
error:
    ov_event_app_free(self);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_event_app *ov_event_app_free(ov_event_app *self){

    if (!ov_event_app_cast(self)) return NULL;

    self->connections = ov_socket_storage_free(self->connections);
    self->json_io_buffer = ov_json_io_buffer_free(self->json_io_buffer);
    self->events = ov_dict_free(self->events);
    self->registry = ov_client_registry_free(self->registry);

    self = ov_data_pointer_free(self);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_event_app *ov_event_app_cast(const void *data){

    if (!data)
        return NULL;

    if (*(uint16_t *)data != OV_EVENT_APP_MAGIC_BYTES)
        return NULL;

    return (ov_event_app *)data;

}

/*
 *      ------------------------------------------------------------------------
 *
 *      EVENT FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

static void send_to_cc(ov_event_app *self, int socket, const ov_json_value *msg){

    ov_json_value *out = ov_json_object();
    ov_json_value *val = NULL;
    ov_json_value_copy((void**)&val, msg);
    ov_json_object_set(out, "message", val);
    ov_json_object_set(out, "socket", ov_json_number(socket));
    ov_json_object_set(out, "uuid", ov_json_string(self->id));
    ov_json_object_set(out, "pid", ov_json_number(self->pid));
    ov_json_object_set(out, "name", ov_json_string(self->config.name));

    char *timestamp = ov_timestamp(false);
    ov_json_object_set(out, "time", ov_json_string(timestamp));
    timestamp = ov_data_pointer_free(timestamp);

    if (-1 != self->cc){

        char *str = ov_json_value_to_string(out);
        ov_io_send(self->config.io, self->cc, (ov_memory_pointer){
            .start = (uint8_t*) str,
            .length = strlen(str)
        });
        ov_data_pointer_free(str);
    }

    ov_json_value_free(out);
    return;
}

/*----------------------------------------------------------------------------*/

static void websocket_io_callback(void *userdata, int socket, ov_json_value *msg){

    ov_event_app *self = ov_event_app_cast(userdata);
    if (!self || !msg) goto error;

    ov_event_app_push(self, socket, msg);
    return;

error:
    ov_json_value_free(msg);
    return;
}

/*----------------------------------------------------------------------------*/

bool ov_event_app_enable_websocket_events(ov_event_app *self,
                                       const char *domain,
                                       const char *uri){

    if (!self || !domain || !uri) return false;

    return ov_io_enable_websocket_events(self->config.io,
                                         domain,
                                         uri, 
                                         self,
                                         websocket_io_callback);
}

/*----------------------------------------------------------------------------*/

struct container1 {

    ov_event_app *self;
    int socket;
    ov_memory_pointer msg;
};

/*----------------------------------------------------------------------------*/

static bool send_to_remote(const void *key, void *val, void *data){

    if (!key) return true;
    intptr_t socket = (intptr_t) key;
    UNUSED(val);
    struct container1 *container = (struct container1*) data;

    ov_io_send(container->self->config.io,
        socket,
        container->msg);

    return true;
}

/*----------------------------------------------------------------------------*/

bool ov_event_app_push(ov_event_app *self, int socket, ov_json_value *msg){

    if (!self || !msg) goto error;

    const char *name = ov_event_api_get_event(msg);
    if (!name) goto error;

    Event *event = ov_dict_get(self->events, name);

    if (!event) {

        ov_log_error("Event %s NOT set.", name);
        goto error;

    }

    ListenerEvent *ev = event->listener;
    while(ev){

        if (ev->callback)
            ev->callback(ev->userdata, name, socket, msg);

        ev = ov_node_next(ev);
    }

    char *str = ov_json_value_to_string(msg);

    struct container1 container = (struct container1){
        .self = self,
        .socket = socket,
        .msg.start = (uint8_t*) str,
        .msg.length = strlen(str)
    };

    ov_dict_for_each(event->remote, &container, send_to_remote);

    send_to_cc(self, socket, msg);
    ov_data_pointer_free(str);
    ov_json_value_free(msg);
    return true;
error:
    ov_json_value_free(msg);
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_event_app_register(ov_event_app *self, 
                              const char *name, 
                              void *userdata,
                              void (*callback)(void *userdata, const char *name,
                                            int socket, const ov_json_value *input)){

    if (!self || !name || !userdata || !callback) goto error;

    Event *event = ov_dict_get(self->events, name);
    if (!event){

        event = event_create();
        char *key = ov_string_dup(name);

        if (!ov_dict_set(self->events, key, event, NULL)){
            
            key = ov_data_pointer_free(key);
            event = event_free(event);
            goto error;
        
        }
    }

    OV_ASSERT(event);

    ListenerEvent *le = calloc(1, sizeof(ListenerEvent));
    if (!le) goto error;

    if (!ov_node_push((void**)&event->listener, (void*)le)){
        le = ov_data_pointer_free(le);
        goto error;
    }

    le->userdata = userdata;
    le->callback = callback;

    return true;
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

static void cb_connected(void *userdata, int connection){

    ov_event_app *self = ov_event_app_cast(userdata);

    if (self->config.callbacks.connected)
        self->config.callbacks.connected(
            self->config.callbacks.userdata, connection);

    return;
}

/*----------------------------------------------------------------------------*/

static bool io_socket(void *userdata, int connection, const char *domain, 
    const ov_memory_pointer buffer){

    UNUSED(domain);

    ov_event_app *self = ov_event_app_cast(userdata);
    return ov_json_io_buffer_push(self->json_io_buffer, connection, buffer);
}

/*----------------------------------------------------------------------------*/

static bool close_remote_listener(const void *key, void *val, void *data){

    if (!key) return true;
    Event *event = (Event*) val;

    ov_dict_del(event->remote, data);

    return true;
}

/*----------------------------------------------------------------------------*/

static void close_socket(void *userdata, int socket){

    ov_event_app *self = ov_event_app_cast(userdata);

    ov_dict_for_each(self->events, (void*)(intptr_t) socket, close_remote_listener);

    if (self->config.callbacks.close)
        self->config.callbacks.close(self->config.callbacks.userdata, socket);

    return;
}

/*----------------------------------------------------------------------------*/

bool ov_event_app_close(ov_event_app *self, int socket){

    if (!self) return false;
    return ov_io_close(self->config.io, socket);
}

/*----------------------------------------------------------------------------*/

bool ov_event_app_open_cc(ov_event_app *self, 
    ov_io_socket_config config){

    if (!self) goto error;

    if (self->cc > 0)
        goto error;

    config.callbacks.userdata = self,
    config.callbacks.accept = NULL;
    config.callbacks.connected = cb_connected_cc;
    config.callbacks.io = cb_io_cc;
    config.callbacks.close = cb_close_cc;

    self->cc = ov_io_open_listener(self->config.io, config);
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

int ov_event_app_open_listener(ov_event_app *self, 
    ov_io_socket_config config){

    if (!self) goto error;

    config.callbacks.userdata = self,
    config.callbacks.accept = NULL;
    config.callbacks.connected = NULL;
    config.callbacks.io = io_socket;
    config.callbacks.close = close_socket;

    return ov_io_open_listener(self->config.io, config);
error:
    return -1;
}

/*----------------------------------------------------------------------------*/

int ov_event_app_open_connection(ov_event_app *self, 
    ov_io_socket_config config){

    if (!self) goto error;

    config.callbacks.userdata = self,
    config.callbacks.accept = NULL;
    config.callbacks.connected = cb_connected;
    config.callbacks.io = io_socket;
    config.callbacks.close = close_socket;

    return ov_io_open_connection(self->config.io, config);
error:
    return -1;
}

/*----------------------------------------------------------------------------*/

static bool add_event_name(const void *key, void *val, void *data){

    if (!key) return true;
    UNUSED(val);

    ov_json_value *out = ov_json_value_cast(data);

    ov_json_object_set(out, (const char*) key, ov_json_null());
    return true;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_event_app_get_functions(const ov_event_app *self){

    ov_json_value *out = NULL;

    if (!self) goto error;

    out = ov_json_object();

    ov_dict_for_each(self->events, out, add_event_name);

    return out;

error:
    ov_json_value_free(out);
    return NULL;
}

/*----------------------------------------------------------------------------*/

bool ov_event_app_send(ov_event_app *self, int socket, const ov_json_value *msg){

    if (!self || !msg) goto error;

    char *out = ov_json_value_to_string(msg);

    bool result = ov_io_send(self->config.io, socket, (ov_memory_pointer){
        .start = (uint8_t*) out,
        .length = strlen(out)
    });

    send_to_cc(self, socket, msg);
    out = ov_data_pointer_free(out);
    return result;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_event_app_get_socket_data(ov_event_app *self, int socket){

    if (!self) return NULL;
    return ov_socket_storage_get(self->connections, socket);
}

/*----------------------------------------------------------------------------*/

static bool add_connection_data(const void *key, void *val, void *data){

    if (!key) return true;

    ov_json_value *array = ov_json_value_cast(data);
    ov_json_value *copy = NULL;
    ov_json_value *item = ov_json_value_cast(val);

    ov_json_value_copy((void**)&copy, item);
    ov_json_array_push(array, copy);
    return true;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_event_app_get_clients(ov_event_app *self){

    if (!self) return NULL;

    ov_json_value *array = ov_json_array();
    if (!ov_socket_storage_for_each(
        self->connections, array, add_connection_data)) goto error;

    return array;
error:
    ov_json_value_free(array);
    return NULL;
}