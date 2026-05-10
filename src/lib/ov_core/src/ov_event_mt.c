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
        @file           ov_event_mt.c
        @author         Töpfer, Markus

        @date           2026-05-07


        ------------------------------------------------------------------------
*/
#include "../include/ov_event_mt.h"
#include "../include/ov_event_api.h"

#include <ov_base/ov_thread_lock.h>
#include <ov_base/ov_thread_loop.h>
#include <ov_base/ov_string.h>
#include <ov_base/ov_dict.h>
#include <ov_base/ov_json_io_buffer.h>

#include <unistd.h>

#define OV_EVENT_MT_MAGIC_BYTES 0xefe4

/*----------------------------------------------------------------------------*/

struct ov_event_mt {

    uint16_t magic_bytes;
    ov_event_mt_config config;

    bool debug;

    ov_json_io_buffer *json_io_buffer;

    ov_thread_loop *tloop;

    struct {

        ov_thread_lock lock;
        ov_dict *events;

    } data;
};

/*----------------------------------------------------------------------------*/

typedef struct Event {

    void *userdata;
    void (*callback)(void *userdata, int socket, const ov_json_value *msg);

} Event;

/*----------------------------------------------------------------------------*/

static bool handle_in_loop(ov_thread_loop *tloop, ov_thread_message *msg){

    UNUSED(tloop);

    ov_log_error("Unexpected in loop message");
    msg = ov_thread_message_free(msg);
    return true;
}

/*----------------------------------------------------------------------------*/

static bool handle_in_thread(ov_thread_loop *tloop, ov_thread_message *msg){

    ov_event_mt *self = ov_thread_loop_get_data(tloop);
    if (!self || !msg) goto error;
        
    const char *name = ov_event_api_get_event(msg->json_message);
    if (!name) goto error;

    if (!ov_thread_lock_try_lock(&self->data.lock)) goto error;

    Event event = {0};
    Event *ev = ov_dict_get(self->data.events, name);
    
    if (ev){
        event = *ev;
    }

    ov_thread_lock_unlock(&self->data.lock);

    if (event.callback)
        event.callback(event.userdata, msg->socket, msg->json_message);

    ov_thread_message_free(msg);
    return true;
error:
    ov_thread_message_free(msg);
    return false;
}

/*----------------------------------------------------------------------------*/

static void json_io_success(void *userdata, int socket, ov_json_value *msg){

    ov_event_mt *self = ov_event_mt_cast(userdata);
    if (!self || !msg) goto error;

    ov_event_mt_push(self, socket, msg);
    return;
error:
    ov_json_value_free(msg);
    return;
}

/*----------------------------------------------------------------------------*/

static void json_io_failure(void *userdata, int socket){

    ov_event_mt *self = ov_event_mt_cast(userdata);
    if (!self) goto error;

    ov_io_close(self->config.io, socket);
error:
    return;
}

/*----------------------------------------------------------------------------*/

static bool init_config(ov_event_mt_config *config){

    if (!config || !config->loop || !config->io) goto error;

    if (0 == config->limits.message_queue_capacity)
        config->limits.message_queue_capacity = 1000;

    if (0 == config->limits.threadlock_timeout_usec)
        config->limits.threadlock_timeout_usec = 2000000;

    if (0 == config->limits.threads)
        config->limits.threads = sysconf(_SC_NPROCESSORS_ONLN);

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

ov_event_mt *ov_event_mt_create(ov_event_mt_config config){

    ov_event_mt *self = NULL;

    if (!init_config(&config)) goto error;

    self = calloc(1, sizeof(ov_event_mt));
    if (!self) goto error;

    self->magic_bytes = OV_EVENT_MT_MAGIC_BYTES;
    self->config = config;

    self->json_io_buffer = ov_json_io_buffer_create((ov_json_io_buffer_config){
        .debug = false,
        .objects_only = false,
        .callback.userdata = self,
        .callback.success = json_io_success,
        .callback.failure = json_io_failure
    });

    if (!self->json_io_buffer) goto error;

    ov_dict_config d_config = ov_dict_string_key_config(255);
    d_config.value.data_function.free= ov_data_pointer_free;

    self->data.events = ov_dict_create(d_config);
    if (!self->data.events) goto error;

    if (!ov_thread_lock_init(&self->data.lock, 
        self->config.limits.threadlock_timeout_usec)) goto error;

    self->tloop = ov_thread_loop_create(
        self->config.loop,
        (ov_thread_loop_callbacks){
            .handle_message_in_thread = handle_in_thread,
            .handle_message_in_loop = handle_in_loop
        },
        self);

    if (!self->tloop) goto error;

    if (!ov_thread_loop_reconfigure(self->tloop,
        (ov_thread_loop_config){
            .disable_to_loop_queue = false,
            .message_queue_capacity = self->config.limits.message_queue_capacity,
            .lock_timeout_usecs = self->config.limits.threadlock_timeout_usec,
            .num_threads = self->config.limits.threads
        })) goto error;

    if (!ov_thread_loop_start_threads(self->tloop)) goto error;

    return self;
error:
    ov_event_mt_free(self);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_event_mt *ov_event_mt_free(ov_event_mt *self){

    if (!ov_event_mt_cast(self)) return self;

    self->tloop = ov_thread_loop_free(self->tloop);
    self->data.events = ov_dict_free(self->data.events);
    ov_thread_lock_clear(&self->data.lock);
    self->json_io_buffer = ov_json_io_buffer_free(self->json_io_buffer);

    self = ov_data_pointer_free(self);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_event_mt *ov_event_mt_cast(const void *data){

    if (!data)
        return NULL;

    if (*(uint16_t *)data != OV_EVENT_MT_MAGIC_BYTES)
        return NULL;

    return (ov_event_mt *)data;

}

/*
 *      ------------------------------------------------------------------------
 *
 *      EVENT FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

static void websocket_io_callback(void *userdata, int socket, ov_json_value *msg){

    ov_event_mt *self = ov_event_mt_cast(userdata);
    if (!self || !msg) goto error;

    ov_event_mt_push(self, socket, msg);
    return;

error:
    ov_json_value_free(msg);
    return;
}

/*----------------------------------------------------------------------------*/

bool ov_event_mt_enable_websocket_events(ov_event_mt *self,
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

bool ov_event_mt_push(ov_event_mt *self, int socket, ov_json_value *input){

    ov_thread_message *msg = NULL;

    if (!self || !input) goto error;

    if (self->debug){
        char *str = ov_json_value_to_string(input);
        ov_log_debug("PUSHING TO THREADS %s", str);
        str = ov_data_pointer_free(str);
    }

    msg = ov_thread_message_standard_create(1, input);
    if (!msg) goto error;
    msg->socket = socket;

    if (!ov_thread_loop_send_message(self->tloop, msg, OV_RECEIVER_THREAD))
        goto error;

    return true;
error:
    ov_thread_message_free(msg);
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_event_mt_register(ov_event_mt *self, 
                          const char *name,
                          void *userdata,
                          void (*callback)(void *userdata,
                            int socket, const ov_json_value *input)){

    if (!self || !name || !userdata || !callback) goto error;

    if (!ov_thread_lock_try_lock(&self->data.lock)) goto error;

    char *key = ov_string_dup(name);
    Event *ev = calloc(1, sizeof(Event));

    bool result = false;

    if (key && ev){

        ev->userdata = userdata;
        ev->callback = callback;

        result = ov_dict_set(self->data.events, key, ev, NULL);

    } else {

        key = ov_data_pointer_free(key);
        ev = ov_data_pointer_free(ev);
    }

    if (!result){

        key = ov_data_pointer_free(key);
        ev = ov_data_pointer_free(ev);
    
    }

    ov_thread_lock_unlock(&self->data.lock);

    return result;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool io_socket(void *userdata, int connection, const char *domain, 
    const ov_memory_pointer buffer){

    UNUSED(domain);

    ov_event_mt *self = ov_event_mt_cast(userdata);
    return ov_json_io_buffer_push(self->json_io_buffer, connection, buffer);
}

/*----------------------------------------------------------------------------*/

int ov_event_mt_open_listener(ov_event_mt *self, 
    ov_io_socket_config config){

    if (!self) goto error;

    config.callbacks.userdata = self,
    config.callbacks.accept = NULL;
    config.callbacks.connected = NULL;
    config.callbacks.io = io_socket;

    return ov_io_open_listener(self->config.io, config);
error:
    return -1;
}

/*----------------------------------------------------------------------------*/

bool ov_event_mt_debug(ov_event_mt *self, bool on){

    if (!self) return false;
    self->debug = on;
    return true;
}