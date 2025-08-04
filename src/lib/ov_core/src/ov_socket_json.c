/***
        ------------------------------------------------------------------------

        Copyright (c) 2024 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_socket_json.c
        @author         Markus

        @date           2024-10-11


        ------------------------------------------------------------------------
*/
#include "../include/ov_socket_json.h"

#define OV_SOCKET_JSON_MAGIC_BYTES 0x5234

#include <ov_base/ov_convert.h>
#include <ov_base/ov_dict.h>
#include <ov_base/ov_thread_lock.h>

/*----------------------------------------------------------------------------*/

struct ov_socket_json {

    uint16_t magic_bytes;
    ov_socket_json_config config;

    ov_thread_lock lock;

    ov_dict *data;
};

/*----------------------------------------------------------------------------*/

ov_socket_json *ov_socket_json_create(ov_socket_json_config config) {

    ov_socket_json *self = NULL;

    if (!config.loop) goto error;

    if (0 == config.limits.threadlock_timeout_usec)
        config.limits.threadlock_timeout_usec = 100000;

    self = calloc(1, sizeof(ov_socket_json));
    if (!self) goto error;

    self->magic_bytes = OV_SOCKET_JSON_MAGIC_BYTES;
    self->config = config;

    ov_dict_config d_config = ov_dict_intptr_key_config(255);
    d_config.value.data_function.free = ov_json_value_free;

    self->data = ov_dict_create(d_config);
    if (!ov_thread_lock_init(
            &self->lock, config.limits.threadlock_timeout_usec))
        goto error;

    return self;
error:
    ov_socket_json_free(self);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_socket_json *ov_socket_json_cast(const void *self) {

    if (!self) goto error;

    if (*(uint16_t *)self == OV_SOCKET_JSON_MAGIC_BYTES)
        return (ov_socket_json *)self;

error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_socket_json *ov_socket_json_free(ov_socket_json *self) {

    if (!ov_socket_json_cast(self)) return NULL;

    ov_thread_lock_clear(&self->lock);

    self->data = ov_dict_free(self->data);
    self = ov_data_pointer_free(self);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_socket_json_get(ov_socket_json *self, int socket) {

    ov_json_value *out = NULL;

    if (!self) goto error;
    if (!ov_thread_lock_try_lock(&self->lock)) goto error;

    ov_json_value *data = ov_dict_get(self->data, (void *)(intptr_t)socket);

    if (!data) {

        out = ov_json_object();

    } else {

        ov_json_value_copy((void **)&out, data);
    }

    ov_thread_lock_unlock(&self->lock);
    return out;
error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

bool ov_socket_json_set(ov_socket_json *self,
                        int socket,
                        ov_json_value **value) {

    if (!self || !socket || !value) goto error;
    if (!ov_thread_lock_try_lock(&self->lock)) goto error;

    ov_json_value *data = *value;

    intptr_t key = socket;
    bool result = ov_dict_set(self->data, (void *)key, data, NULL);

    if (result) *value = NULL;

    ov_thread_lock_unlock(&self->lock);
    return result;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_socket_json_drop(ov_socket_json *self, int socket) {

    if (!self) goto error;
    if (!ov_thread_lock_try_lock(&self->lock)) goto error;

    bool result = ov_dict_del(self->data, (void *)(intptr_t)socket);
    ov_thread_lock_unlock(&self->lock);

    return result;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool add_data_to_out(const void *key, void *val, void *data) {

    char *k = NULL;
    size_t l = 0;

    if (!key) return true;

    ov_json_value *out = ov_json_value_cast(data);
    ov_json_value *self = ov_json_value_cast(val);

    ov_json_value *copy = NULL;
    if (!ov_json_value_copy((void **)&copy, self)) goto error;

    intptr_t p = (intptr_t)key;

    if (!ov_convert_int64_to_string((int64_t)p, &k, &l)) goto error;
    if (!ov_json_object_set(out, k, copy)) goto error;

    return true;
error:
    copy = ov_json_value_free(copy);
    k = ov_data_pointer_free(k);
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_socket_json_for_each_set_data(ov_socket_json *self,
                                      ov_json_value *out) {

    if (!self) goto error;
    if (!ov_thread_lock_try_lock(&self->lock)) goto error;

    bool result = ov_dict_for_each(self->data, out, add_data_to_out);

    ov_thread_lock_unlock(&self->lock);

    return result;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_socket_json_for_each(ov_socket_json *self, 
        void *data,
        bool (*function)(const void *key, void *val, void *data)){

    if (!self || !function) goto error;

    if (!ov_thread_lock_try_lock(&self->lock)) goto error;

    bool result = ov_dict_for_each(self->data, data, function);

    ov_thread_lock_unlock(&self->lock);

    return result;
error:
    return false;
}