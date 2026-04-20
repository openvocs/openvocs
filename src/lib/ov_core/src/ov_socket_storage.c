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
        @file           ov_socket_storage.c
        @author         Töpfer, Markus

        @date           2026-04-08


        ------------------------------------------------------------------------
*/
#include "../include/ov_socket_storage.h"

#include <ov_base/ov_dict.h>
#include <ov_base/ov_convert.h>

struct ov_socket_storage {

    ov_dict *sockets;

};


/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_socket_storage *ov_socket_storage_create(){

    ov_socket_storage *self = calloc(1, sizeof(ov_socket_storage));
    if (!self) goto error;

    ov_dict_config d_config = ov_dict_intptr_key_config(255);
    d_config.value.data_function.free = ov_json_value_free;

    self->sockets = ov_dict_create(d_config);
    if (!self->sockets) goto error;

    return self;
error:
    ov_socket_storage_free(self);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_socket_storage *ov_socket_storage_free(ov_socket_storage *self){

    if (!self) return NULL;

    self->sockets = ov_dict_free(self->sockets);
    self = ov_data_pointer_free(self);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_socket_storage_get(ov_socket_storage *self, int socket){

    if (!self) return NULL;

    ov_json_value *item = ov_dict_get(self->sockets, (void*)(intptr_t)socket);
    if (!item){
        item = ov_json_object();
        ov_dict_set(self->sockets, (void*)(intptr_t)socket, item, NULL);
    }

    return item;
}

/*----------------------------------------------------------------------------*/

bool ov_socket_storage_drop(ov_socket_storage *self, int socket){

    if (!self) return false;
    return ov_dict_del(self->sockets, (void*)(intptr_t) socket);
}

/*----------------------------------------------------------------------------*/

bool ov_socket_storage_for_each(ov_socket_storage *self, void *userdata, 
        bool (*function)(const void *key, void *val, void *data)){

    if (!self) return false;

    return ov_dict_for_each(self->sockets, userdata, function);
}

/*----------------------------------------------------------------------------*/

static bool add_data_to_out(const void *key, void *val, void *data) {

    char *k = NULL;
    size_t l = 0;

    if (!key)
        return true;

    ov_json_value *out = ov_json_value_cast(data);
    ov_json_value *self = ov_json_value_cast(val);

    ov_json_value *copy = NULL;
    if (!ov_json_value_copy((void **)&copy, self))
        goto error;

    intptr_t p = (intptr_t)key;

    if (!ov_convert_int64_to_string((int64_t)p, &k, &l))
        goto error;
    if (!ov_json_object_set(out, k, copy))
        goto error;

    return true;
error:
    copy = ov_json_value_free(copy);
    k = ov_data_pointer_free(k);
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_socket_storage_for_each_set_data(ov_socket_storage *self,
                                      ov_json_value *out) {

    if (!self)
        goto error;

    bool result = ov_dict_for_each(self->sockets, out, add_data_to_out);
    return result;
error:
    return false;
}
