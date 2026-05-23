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
        @file           ov_timed.c
        @author         Töpfer, Markus

        @date           2026-05-23


        ------------------------------------------------------------------------
*/
#include "../include/ov_timed.h"
#include "../include/ov_dict.h"
#include "../include/ov_string.h"

struct ov_timed {

    ov_timed_config config;

    ov_dict *dict;

};

/*----------------------------------------------------------------------------*/

typedef struct Callback {

    ov_timed *timed;
    ov_id id;
    uint32_t timer;
    void *userdata;
    void (*callback)(void *userdata, const char *uuid);

} Callback;

/*----------------------------------------------------------------------------*/

static void *callback_free(void *data){

    if (!data) return NULL;

    Callback *cb = (Callback*) data;
    
    if (0 != cb->timer)
        ov_event_loop_timer_unset(cb->timed->config.loop, cb->timer, NULL);

    data = ov_data_pointer_free(data);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_timed *ov_timed_create(ov_timed_config config){

    ov_timed *self = NULL;
    if (!config.loop) goto error;

    self = calloc(1, sizeof(ov_timed));
    if (!self) goto error;

    ov_dict_config d_config = ov_dict_string_key_config(255);
    d_config.value.data_function.free = callback_free;

    self->dict = ov_dict_create(d_config);
    if (!self->dict) goto error;

    return self;
error:
    ov_timed_free(self);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_timed *ov_timed_free(ov_timed *self){

    if (!self) return self;

    self->dict = ov_dict_free(self->dict);
    self = ov_data_pointer_free(self);
    return NULL;
}

/*----------------------------------------------------------------------------*/

static bool cb_timed(uint32_t timer, void *userdata){

    UNUSED(timer);

    Callback *cb = (Callback*) userdata;

    if (cb->callback)
        cb->callback(cb->userdata, cb->id);

    ov_dict_del(cb->timed->dict, cb->id);
    return true;
}

/*----------------------------------------------------------------------------*/

bool ov_timed_add(
    ov_timed *self, 
    ov_time time, 
    const ov_id id,
    void *userdata, 
    void (*callback)(void *userdata,const char *uuid)){

    if (!self || !id) goto error;

    ov_time now = ov_timestamp_create();

    if (0 == time.year)
        time.year = now.year;

    if (0 == time.month)
        time.month = now.month;

    if (0 == time.day)
        time.day = now.day;

    if (0 == time.hour)
        time.hour = now.hour;

    if (0 == time.minute)
        time.minute = now.minute;

    if (0 == time.second)
        time.second = now.second;

    uint64_t epoch_now = ov_time_to_epoch(now);
    uint64_t epoch_new = ov_time_to_epoch(time);

    uint64_t rel_timeout_usecs = epoch_new - epoch_now;

    Callback *cb = calloc(1, sizeof(Callback));
    if (!cb) goto error;
    cb->userdata = userdata;
    cb->callback = callback;
    cb->timed = self;
    ov_id_set(cb->id, id);

    if (!ov_dict_set(self->dict, ov_string_dup(id), cb, NULL)) goto error;
    
    cb->timer = ov_event_loop_timer_set(self->config.loop,
        rel_timeout_usecs,
        cb,
        cb_timed);

    if (0 == cb->timer) goto error;

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_timed_del(
    ov_timed *self,
    const ov_id id){

    if (!self) return false;
    return ov_dict_del(self->dict, id);
}