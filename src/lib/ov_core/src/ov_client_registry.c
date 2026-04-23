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
        @file           ov_client_registry.c
        @author         Töpfer, Markus

        @date           2026-04-21


        ------------------------------------------------------------------------
*/
#include "../include/ov_client_registry.h"

#include <ov_base/ov_dict.h>
#include <ov_base/ov_string.h>
#include <ov_base/ov_linked_list.h>


struct ov_client_registry {

    ov_dict *data;
};

struct data {

    int socket;
    ov_json_value *data;
};

/*----------------------------------------------------------------------------*/

void *data_free(void *data){

    struct data *d = (struct data*) data;
    d->data = ov_json_value_free(d->data);
    d = ov_data_pointer_free(d);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_client_registry *ov_client_registry_create(){

    ov_client_registry *self = calloc(1, sizeof(ov_client_registry));
    if (!self) goto error;

    ov_dict_config d_config = ov_dict_string_key_config(255);
    d_config.value.data_function.free = data_free;

    self->data = ov_dict_create(d_config);
    return self;

error:
    ov_client_registry_free(self);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_client_registry *ov_client_registry_free(ov_client_registry *self){

    if (!self) return NULL;

    self->data = ov_dict_free(self->data);
    self = ov_data_pointer_free(self);
    return NULL;
}

/*----------------------------------------------------------------------------*/

bool ov_client_registry_register(ov_client_registry *self, 
    const char *uuid, int socket){

    struct data *data = NULL;
    char *key = NULL;

    if (!self || !uuid) goto error;

    key = ov_string_dup(uuid);
    data = calloc(1, sizeof(struct data));
    if (!key || !data) goto error;

    data->socket = socket;

    if (!ov_dict_set(self->data, key, data, NULL)) goto error;
    return true;
error:
    key = ov_data_pointer_free(key);
    data = data_free(data);
    return false;
}

/*----------------------------------------------------------------------------*/

struct container {

    int socket;
    ov_list *list;
};

/*----------------------------------------------------------------------------*/

static bool drop_socket(void *item, void *data){

    ov_dict *dict = ov_dict_cast(data);
    if (!dict) return false;

    return ov_dict_del(dict, item);
}

/*----------------------------------------------------------------------------*/

static bool search_socket(const void *key, void *val, void *data){

    if (!key) return true;
    struct data *d = (struct data*) val;
    struct container *c = (struct container*) data;

    if (d->socket == c->socket)
        ov_list_push(c->list, (void*)key);

    return true;
}

/*----------------------------------------------------------------------------*/

bool ov_client_registry_unregister(ov_client_registry *self, int socket){

    if (!self) return false;

    struct container container = (struct container){
        .socket = socket,
        .list = ov_linked_list_create((ov_list_config){0})
    };

    if (!ov_dict_for_each(self->data, &container, search_socket))
        goto error;

    if (!ov_list_for_each(container.list, self->data, drop_socket))
        goto error;

    container.list = ov_list_free(container.list);
    return true;
error:
    container.list = ov_list_free(container.list);
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_client_registry_set_data(ov_client_registry *self, const char *uuid, 
    ov_json_value *data){

    if (!self || !uuid || !data) goto error;

    struct data *d = ov_dict_get(self->data, uuid);
    if (!d) goto error;

    d->data = data;
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_client_registry_get_data(ov_client_registry *self, 
    const char *uuid){

    if (!self || !uuid) goto error;

    struct data *d = ov_dict_get(self->data, uuid);
    if (!d) goto error;

    if (!d->data) d->data = ov_json_object();

    return d->data;
error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

int ov_client_registry_get_socket(ov_client_registry *self, const char *uuid){

    if (!self || !uuid) goto error;

    struct data *d = ov_dict_get(self->data, uuid);
    if (!d) goto error;

    return d->socket;
error:
    return -1;
}