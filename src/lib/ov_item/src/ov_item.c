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
        @file           ov_item.c
        @author         Töpfer, Markus

        @date           2025-11-28


        ------------------------------------------------------------------------
*/
#include "../include/ov_item.h"

#include <stdlib.h>
#include <string.h>

#include <ov_log/ov_log.h>
#include <ov_base/ov_thread_lock.h>
#include <ov_base/ov_data_function.h>
#include <ov_base/ov_string.h>
#include <ov_base/ov_linked_list.h>
#include <ov_base/ov_dict.h>

/*---------------------------------------------------------------------------*/

#define OV_ITEM_MAGIC_BYTES 0x17e9

/*---------------------------------------------------------------------------*/

typedef enum ov_item_literal {

    OV_ITEM_NULL   = 0,
    OV_ITEM_TRUE   = 1,
    OV_ITEM_FALSE  = 2,
    OV_ITEM_ARRAY  = 3,
    OV_ITEM_OBJECT = 4,
    OV_ITEM_STRING = 6,
    OV_ITEM_NUMBER = 7

} ov_item_literal;

/*---------------------------------------------------------------------------*/

typedef struct ov_item_config {

    ov_item_literal literal;

    union {
        double number;
        void *data;
    };

    struct {

        uint64_t thread_lock_timeout_usecs;

    } limits;

} ov_item_config; 

/*---------------------------------------------------------------------------*/

struct ov_item {

    const uint16_t magic_bytes;
    ov_item_config config;
    ov_thread_lock lock;

};

/*---------------------------------------------------------------------------*/

static ov_item root_item = (ov_item) {
    .magic_bytes = OV_ITEM_MAGIC_BYTES,
    .config = (ov_item_config){0}
};

/*---------------------------------------------------------------------------*/

bool ov_item_configure_lock_timeout_usecs(uint64_t usecs){

    root_item.config.limits.thread_lock_timeout_usecs = usecs;
    return true;
}

/*---------------------------------------------------------------------------*/

static ov_item *item_create(ov_item_config config){

    ov_item *item = calloc(1, sizeof(ov_item));
    if (!item) goto error;

    memcpy(item, &root_item, sizeof(ov_item));

    if (0 == config.limits.thread_lock_timeout_usecs)
        config.limits.thread_lock_timeout_usecs = OV_ITEM_DEFAULT_THREADLOCK_USECS;

    item->config = config;

    if (!ov_thread_lock_init(&item->lock, 
        item->config.limits.thread_lock_timeout_usecs)){

        goto error;
    }

    return item;
error:
    ov_item_free(item);
    return NULL;
}

/*---------------------------------------------------------------------------*/

ov_item *ov_item_cast(const void *self){

    if (!self) return NULL;

    if (*(uint16_t *)self == OV_ITEM_MAGIC_BYTES) 
        return (ov_item *)self;

    return NULL;
}

/*---------------------------------------------------------------------------*/

void *ov_item_free(void *self){

    ov_item *item = ov_item_cast(self);
    if (!item) return self;

    if (!ov_thread_lock_try_lock(&item->lock)){

        ov_log_error("Could not lock item.");
        goto error;
    }

    switch (item->config.literal){

        case OV_ITEM_NULL:
        case OV_ITEM_TRUE:
        case OV_ITEM_FALSE:
        case OV_ITEM_NUMBER:
            break;

        case OV_ITEM_STRING:

            if (item->config.data)
                item->config.data = ov_data_pointer_free(item->config.data);

            break;

        case OV_ITEM_ARRAY:

            if (item->config.data)
                item->config.data = ov_list_free(item->config.data);

            break;

        case OV_ITEM_OBJECT:

            if (item->config.data)
                item->config.data = ov_dict_free(item->config.data);

            break;

        default:
            break;
    }

    if (!ov_thread_lock_unlock(&item->lock)){
        ov_log_error("Failed to unlock.");
        goto error;
    }

    if (!ov_thread_lock_clear(&item->lock)){
        ov_log_error("Failed to clear lock -  data leak will occur.");
    }

    item = ov_data_pointer_free(item);
    return NULL;
error:
    return item;
}

/*---------------------------------------------------------------------------*/

bool ov_item_clear(void *self){

    ov_item *item = ov_item_cast(self);
    if (!item) return false;

    if (!ov_thread_lock_try_lock(&item->lock)){

        ov_log_error("Could not lock item.");
        goto error;
    }

    switch (item->config.literal){

        case OV_ITEM_NULL:
        case OV_ITEM_TRUE:
        case OV_ITEM_FALSE:
        case OV_ITEM_NUMBER:
            break;

        case OV_ITEM_STRING:

            if (item->config.data)
                item->config.data = ov_data_pointer_free(item->config.data);

            break;

        case OV_ITEM_ARRAY:

            if (item->config.data)
                item->config.data = ov_list_free(item->config.data);

            break;

        case OV_ITEM_OBJECT:

            if (item->config.data)
                item->config.data = ov_dict_free(item->config.data);

            break;

        default:
            break;
    }

    item->config.literal = OV_ITEM_NULL;
    item->config.number = 0;

    if (!ov_thread_lock_unlock(&item->lock)){
        ov_log_error("Failed to unlock.");
        goto error;
    }
    
    return true;
error:
    return false;
}

/*---------------------------------------------------------------------------*/

bool ov_item_dump(FILE *stream, const void *self){

    if (!stream || !self) return false;
    ov_item *item = ov_item_cast(self);
    if (!item) goto error;

    if (!ov_thread_lock_try_lock(&item->lock)){

        ov_log_error("Could not lock item.");
        goto error;
    }

    switch (item->config.literal){

        case OV_ITEM_NULL:
            
            if (!fprintf(stream, "\nnull")) goto error;
            break;

        case OV_ITEM_TRUE:

            if (!fprintf(stream, "\ntrue")) goto error;
            break;

        case OV_ITEM_FALSE:

            if (!fprintf(stream, "\nfalse")) goto error;
            break;

        case OV_ITEM_NUMBER:

            if (!fprintf(stream, "\n%f",item->config.number)) goto error;
            break;


        case OV_ITEM_STRING:

            if (item->config.data)
                if (!fprintf(stream, "\n%s",(char*) item->config.data)) goto error;

            break;

        case OV_ITEM_ARRAY:

            if (item->config.data)
                ov_list_dump(stream, item->config.data);

            break;

        case OV_ITEM_OBJECT:

            if (item->config.data)
                ov_dict_dump(stream, item->config.data);

            break;

        default:
            break;
    }

    if (!ov_thread_lock_unlock(&item->lock)){
        ov_log_error("Failed to unlock.");
        goto error;
    }

    return true;
error:
    return false;
}

/*---------------------------------------------------------------------------*/

void *ov_item_copy(void **destination, const void *self){

    ov_item *copy = NULL;

    if (!self) goto error;

    ov_item *source = ov_item_cast(self);
    if (!source) goto error;

    copy = NULL;

    if (!ov_thread_lock_try_lock(&source->lock)){

        ov_log_error("Could not lock source item.");
        goto error;
    }

    bool result = true;

    switch (source->config.literal){

        case OV_ITEM_NULL:
            copy = ov_item_null();
            break;
        case OV_ITEM_TRUE:
            copy = ov_item_true();
            break;
        case OV_ITEM_FALSE:
            copy = ov_item_false();
            break;

        case OV_ITEM_NUMBER:

            copy = ov_item_number(source->config.number);
            break;


        case OV_ITEM_STRING:

            copy = ov_item_string(source->config.data);

            break;

        case OV_ITEM_ARRAY:

            copy = ov_item_array();

            if (!ov_list_copy((void**)&copy->config.data,source->config.data)) {

                result = false;

            }

            break;

        case OV_ITEM_OBJECT:

            copy = ov_item_object();

            if (!ov_dict_copy((void**)&copy->config.data, source->config.data)) {

                result = false;
            }

            break;

        default:
            break;
    }

    if (!ov_thread_lock_unlock(&source->lock)){
        ov_log_error("Failed to unlock.");
        goto error;
    }

    if (!result) goto error;
    *destination = copy;
    return copy;
error:
    copy = ov_item_free(copy);
    return NULL;
}

/*---------------------------------------------------------------------------*/

size_t ov_item_count(ov_item *self){

    if (!self) goto error;

    uint64_t count = 0;

    if (!ov_thread_lock_try_lock(&self->lock)){

        ov_log_error("Could not lock item.");
        goto error;
    }

    switch (self->config.literal){

        case OV_ITEM_NULL:
        case OV_ITEM_TRUE:
        case OV_ITEM_FALSE:
        case OV_ITEM_NUMBER:
        case OV_ITEM_STRING:

            count = 1;
            break;

        case OV_ITEM_ARRAY:

            count = ov_list_count(self->config.data);
            break;

        case OV_ITEM_OBJECT:

            count = ov_dict_count(self->config.data);
            break;

        default:
            break;
    }

    if (!ov_thread_lock_unlock(&self->lock)){
        ov_log_error("Failed to unlock.");
        goto error;
    }

    return count;
error:
    return 0;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      #OBJECT FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_item *ov_item_object(){

    ov_item *item = item_create((ov_item_config){
        .literal = OV_ITEM_OBJECT,
        .limits.thread_lock_timeout_usecs = 
        root_item.config.limits.thread_lock_timeout_usecs
    });

    if (!item) goto error;

    ov_dict_config d_config = ov_dict_string_key_config(255);
    d_config.value.data_function.free = ov_item_free;
    d_config.value.data_function.clear = ov_item_clear;
    d_config.value.data_function.dump = ov_item_dump;
    d_config.value.data_function.copy = ov_item_copy;

    item->config.data = ov_dict_create(d_config);
    if (!item->config.data) goto error;

    return item;
error:
    ov_item_free(item);
    return NULL;
}

/*---------------------------------------------------------------------------*/

bool ov_item_is_object(ov_item *self){

    if (!self) return false;

    if (!ov_thread_lock_try_lock(&self->lock)){

        ov_log_error("Could not lock item.");
        goto error;
    }

    bool result = false;

    if (self->config.literal == OV_ITEM_OBJECT)
        result = true;

    if (!ov_thread_lock_unlock(&self->lock)){
        ov_log_error("Failed to unlock.");
        goto error;
    }

    return result;
error:
    return false;
}

/*---------------------------------------------------------------------------*/

bool ov_item_object_set(ov_item *self, const char *string, ov_item *val){

    char *key = NULL;

    if (!ov_item_is_object(self) || !string || !val) goto error;

    key = ov_string_dup(string);

    if (!ov_thread_lock_try_lock(&self->lock)){

        ov_log_error("Could not lock item.");
        goto error;
    }

    if (!ov_dict_set(self->config.data, key, val, NULL)) goto error;
    
    if (!ov_thread_lock_unlock(&self->lock)){
        ov_log_error("Failed to unlock.");
        goto error;
    }

    return true;

error:
    key = ov_data_pointer_free(key);
    return false;
}

/*---------------------------------------------------------------------------*/

ov_item *ov_item_object_get(ov_item *self, const char *string){

    ov_item *out = NULL;

    if (!ov_item_is_object(self) || !string) goto error;

    if (!ov_thread_lock_try_lock(&self->lock)){

        ov_log_error("Could not lock item.");
        goto error;
    }

    out = ov_item_cast(ov_dict_get(self->config.data, (void*) string));

    if (!ov_thread_lock_unlock(&self->lock)){
        ov_log_error("Failed to unlock.");
        goto error;
    }

    return out;
error:
    return NULL;
}

/*---------------------------------------------------------------------------*/

bool ov_item_object_for_each(ov_item *self, 
                             bool (*function)(
                                char const *key,
                                ov_item const *val,
                                void *userdata),
                             void *userdata){

    if (!ov_item_is_object(self) || !function) goto error;

    if (!ov_thread_lock_try_lock(&self->lock)){

        ov_log_error("Could not lock item.");
        goto error;
    }

    bool result = ov_dict_for_each(
        self->config.data,
        userdata,
        (void*) function);

    if (!ov_thread_lock_unlock(&self->lock)){
        ov_log_error("Failed to unlock.");
        goto error;
    }

    return result;
error:
    return false;
}

/*---------------------------------------------------------------------------*/

bool ov_item_object_delete(ov_item *self, const char *string){

    if (!ov_item_is_object(self) || !string) goto error;

    if (!ov_thread_lock_try_lock(&self->lock)){

        ov_log_error("Could not lock item.");
        goto error;
    }

    bool result = ov_dict_del(self->config.data, string);

    if (!ov_thread_lock_unlock(&self->lock)){
        ov_log_error("Failed to unlock.");
        goto error;
    }

    return result;
error:
    return false;
}

/*---------------------------------------------------------------------------*/

ov_item *ov_item_object_remove(ov_item *self, const char *string){

    if (!ov_item_is_object(self) || !string) goto error;

    ov_item *out = NULL;

    if (!ov_thread_lock_try_lock(&self->lock)){

        ov_log_error("Could not lock item.");
        goto error;
    }

    out = ov_dict_remove(self->config.data, string);

    if (!ov_thread_lock_unlock(&self->lock)){
        ov_log_error("Failed to unlock.");
        goto error;
    }

    return out;
error:
    return NULL;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      #ARRAY FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_item *ov_item_array(){

    ov_item *item = item_create((ov_item_config){
        .literal = OV_ITEM_ARRAY,
        .limits.thread_lock_timeout_usecs = 
        root_item.config.limits.thread_lock_timeout_usecs
    });

    if (!item) goto error;

    ov_list_config l_config = (ov_list_config){
        .item.free = ov_item_free,
        .item.clear = ov_item_clear,
        .item.dump = ov_item_dump,
        .item.copy = ov_item_copy
    };

    item->config.data = ov_linked_list_create(l_config);
    if (!item->config.data) goto error;

    return item;
error:
    ov_item_free(item);
    return NULL;
}

/*---------------------------------------------------------------------------*/

bool ov_item_is_array(ov_item *self){

    if (!self) return false;

    if (!ov_thread_lock_try_lock(&self->lock)){

        ov_log_error("Could not lock item.");
        goto error;
    }

    bool result = false;

    if (self->config.literal == OV_ITEM_ARRAY)
        result = true;

    if (!ov_thread_lock_unlock(&self->lock)){
        ov_log_error("Failed to unlock.");
        goto error;
    }

    return result;
error:
    return false;
}

/*---------------------------------------------------------------------------*/

ov_item *ov_item_array_get(ov_item *self, uint64_t pos){

    ov_item *out = NULL;

    if (!self) return false;

    if (!ov_thread_lock_try_lock(&self->lock)){

        ov_log_error("Could not lock item.");
        goto error;
    }

    out = ov_list_get(self->config.data, pos);

    if (!ov_thread_lock_unlock(&self->lock)){
        ov_log_error("Failed to unlock.");
        goto error;
    }

    return out;
error:
    return false;
}

/*---------------------------------------------------------------------------*/

bool ov_item_array_set(ov_item *self, uint64_t pos, ov_item *val){

    if (!self) return false;

    if (!ov_thread_lock_try_lock(&self->lock)){

        ov_log_error("Could not lock item.");
        goto error;
    }

    bool result = ov_list_set(self->config.data, pos, val, NULL);

    if (!ov_thread_lock_unlock(&self->lock)){
        ov_log_error("Failed to unlock.");
        goto error;
    }

    return result;
error:
    return false;
}

/*---------------------------------------------------------------------------*/

bool ov_item_array_push(ov_item *self, ov_item *val){

    if (!self) return false;

    if (!ov_thread_lock_try_lock(&self->lock)){

        ov_log_error("Could not lock item.");
        goto error;
    }

    bool result = ov_list_push(self->config.data, val);

    if (!ov_thread_lock_unlock(&self->lock)){
        ov_log_error("Failed to unlock.");
        goto error;
    }

    return result;
error:
    return false;
}

/*---------------------------------------------------------------------------*/

ov_item *ov_item_array_stack_pop(ov_item *self){

    ov_item *out = NULL;

    if (!self) return false;

    if (!ov_thread_lock_try_lock(&self->lock)){

        ov_log_error("Could not lock item.");
        goto error;
    }

    out = ov_list_pop(self->config.data);

    if (!ov_thread_lock_unlock(&self->lock)){
        ov_log_error("Failed to unlock.");
        goto error;
    }

    return out;
error:
    return false;
}

/*---------------------------------------------------------------------------*/

ov_item *ov_item_array_lifo(ov_item *self){

    return ov_item_array_stack_pop(self);
}

/*---------------------------------------------------------------------------*/

ov_item *ov_item_array_queue_pop(ov_item *self){

    ov_item *out = NULL;

    if (!self) return false;

    if (!ov_thread_lock_try_lock(&self->lock)){

        ov_log_error("Could not lock item.");
        goto error;
    }

    out = ov_list_remove(self->config.data, 1);

    if (!ov_thread_lock_unlock(&self->lock)){
        ov_log_error("Failed to unlock.");
        goto error;
    }

    return out;
error:
    return false;
}

/*---------------------------------------------------------------------------*/

ov_item *ov_item_array_fifo(ov_item *self){

    return ov_item_array_queue_pop(self);
}


/*
 *      ------------------------------------------------------------------------
 *
 *      #STRING FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_item *ov_item_string(const char *string){

    if (!string) return NULL;

    ov_item *item = item_create((ov_item_config){
        .literal = OV_ITEM_STRING,
        .limits.thread_lock_timeout_usecs = 
        root_item.config.limits.thread_lock_timeout_usecs
    });

    if (!item) goto error;

    item->config.data = ov_string_dup(string);

    return item;
error:
    ov_item_free(item);
    return NULL;
}

/*---------------------------------------------------------------------------*/

bool ov_item_is_string(ov_item *self){

    if (!self) return false;

    if (!ov_thread_lock_try_lock(&self->lock)){

        ov_log_error("Could not lock item.");
        goto error;
    }

    bool result = false;

    if (self->config.literal == OV_ITEM_STRING)
        result = true;

    if (!ov_thread_lock_unlock(&self->lock)){
        ov_log_error("Failed to unlock.");
        goto error;
    }

    return result;
error:
    return false;
}

/*---------------------------------------------------------------------------*/

const char *ov_item_get_string(ov_item *self){

    if (!self) goto error;
    if (!ov_item_is_string(self)) goto error;

    return (const char*) self->config.data;

error:
    return 0;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      #NUMBER FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_item *ov_item_number(double number){

    ov_item *item = item_create((ov_item_config){
        .literal = OV_ITEM_NUMBER,
        .limits.thread_lock_timeout_usecs = 
        root_item.config.limits.thread_lock_timeout_usecs
    });

    if (!item) goto error;

    item->config.number = number;

    return item;
error:
    ov_item_free(item);
    return NULL;
}

/*---------------------------------------------------------------------------*/

bool ov_item_is_number(ov_item *self){

    if (!self) return false;

    if (!ov_thread_lock_try_lock(&self->lock)){

        ov_log_error("Could not lock item.");
        goto error;
    }

    bool result = false;

    if (self->config.literal == OV_ITEM_NUMBER)
        result = true;

    if (!ov_thread_lock_unlock(&self->lock)){
        ov_log_error("Failed to unlock.");
        goto error;
    }

    return result;
error:
    return false;
}

/*---------------------------------------------------------------------------*/

double ov_item_get_number(ov_item *self){

    if (!self) goto error;
    if (!ov_item_is_number(self)) goto error;
    
    double nbr = 0;

    if (!ov_thread_lock_try_lock(&self->lock)){

        ov_log_error("Could not lock item.");
        goto error;
    }

    nbr = self->config.number;

    if (!ov_thread_lock_unlock(&self->lock)){
        ov_log_error("Failed to unlock.");
        goto error;
    }
    
    return nbr;

error:
    return 0;
}

/*---------------------------------------------------------------------------*/

int64_t ov_item_get_int(ov_item *self){

    if (!self) goto error;
    if (!ov_item_is_number(self)) goto error;
    
    int64_t nbr = 0;

    if (!ov_thread_lock_try_lock(&self->lock)){

        ov_log_error("Could not lock item.");
        goto error;
    }

    nbr = (int64_t) self->config.number;

    if (!ov_thread_lock_unlock(&self->lock)){
        ov_log_error("Failed to unlock.");
        goto error;
    }

    return nbr;

error:
    return 0;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      #LITERAL FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_item *ov_item_null(){

    ov_item *item = item_create((ov_item_config){
        .literal = OV_ITEM_NULL,
        .limits.thread_lock_timeout_usecs = 
        root_item.config.limits.thread_lock_timeout_usecs
    });

    if (!item) goto error;

    return item;
error:
    ov_item_free(item);
    return NULL;
}

/*---------------------------------------------------------------------------*/

bool ov_item_is_null(ov_item *self){

    if (!self) return false;

    if (!ov_thread_lock_try_lock(&self->lock)){

        ov_log_error("Could not lock item.");
        goto error;
    }

    bool result = false;

    if (self->config.literal == OV_ITEM_NULL)
        result = true;

    if (!ov_thread_lock_unlock(&self->lock)){
        ov_log_error("Failed to unlock.");
        goto error;
    }

    return result;
error:
    return false;
}

/*---------------------------------------------------------------------------*/

ov_item *ov_item_true(){

    ov_item *item = item_create((ov_item_config){
        .literal = OV_ITEM_TRUE,
        .limits.thread_lock_timeout_usecs = 
        root_item.config.limits.thread_lock_timeout_usecs
    });

    if (!item) goto error;

    return item;
error:
    ov_item_free(item);
    return NULL;
}

/*---------------------------------------------------------------------------*/

bool ov_item_is_true(ov_item *self){

    if (!self) return false;

    if (!ov_thread_lock_try_lock(&self->lock)){

        ov_log_error("Could not lock item.");
        goto error;
    }

    bool result = false;

    if (self->config.literal == OV_ITEM_TRUE)
        result = true;

    if (!ov_thread_lock_unlock(&self->lock)){
        ov_log_error("Failed to unlock.");
        goto error;
    }

    return result;
error:
    return false;
}

/*---------------------------------------------------------------------------*/

ov_item *ov_item_false(){

    ov_item *item = item_create((ov_item_config){
        .literal = OV_ITEM_FALSE,
        .limits.thread_lock_timeout_usecs = 
        root_item.config.limits.thread_lock_timeout_usecs
    });

    if (!item) goto error;

    return item;
error:
    ov_item_free(item);
    return NULL;
}

/*---------------------------------------------------------------------------*/

bool ov_item_is_false(ov_item *self){

    if (!self) return false;

    if (!ov_thread_lock_try_lock(&self->lock)){

        ov_log_error("Could not lock item.");
        goto error;
    }

    bool result = false;

    if (self->config.literal == OV_ITEM_FALSE)
        result = true;

    if (!ov_thread_lock_unlock(&self->lock)){
        ov_log_error("Failed to unlock.");
        goto error;
    }

    return result;
error:
    return false;
}
