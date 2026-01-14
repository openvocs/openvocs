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
        @file           ov_mixer_registry.c
        @author         Töpfer, Markus

        @date           2026-01-14


        ------------------------------------------------------------------------
*/
#include "../include/ov_mixer_registry.h"

#include <ov_base/ov_dict.h>
#include <ov_base/ov_id.h>
#include <ov_base/ov_socket.h>
#include <ov_base/ov_string.h>
#include <ov_base/ov_thread_lock.h>

struct ov_mixer_registry {

    ov_thread_lock lock;
    ov_dict *users;
    ov_dict *sockets;

};

/*----------------------------------------------------------------------------*/

ov_mixer_registry *ov_mixer_registry_create(ov_mixer_registry_config config){

    if (0 == config.limits.threadlock_timeout_usec)
        config.limits.threadlock_timeout_usec = 100000;

    ov_mixer_registry *self = calloc(1, sizeof(ov_mixer_registry));
    if (!self) goto error;

    self->users = ov_dict_create(ov_dict_string_key_config(255));

    ov_dict_config d_config = ov_dict_intptr_key_config(255);
    d_config.value.data_function.free = ov_data_pointer_free;

    self->sockets = ov_dict_create(d_config);
    if (!self->sockets) goto error;

    if (!ov_thread_lock_init(&self->lock, config.limits.threadlock_timeout_usec))
        goto error;

    return self;
error:
    ov_mixer_registry_free(self);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_mixer_registry *ov_mixer_registry_free(ov_mixer_registry *self){

    if (!self) return NULL;

    ov_thread_lock_clear(&self->lock);

    self->users = ov_dict_free(self->users);
    self->sockets = ov_dict_free(self->sockets);
    self = ov_data_pointer_free(self);
    return NULL;
}

/*----------------------------------------------------------------------------*/

bool ov_mixer_registry_register_mixer(ov_mixer_registry *self,
                                       int socket,
                                      const ov_socket_data *remote){

    if (!self || !remote) goto error;

    if (!ov_thread_lock_try_lock(&self->lock)) goto error;

    ov_mixer_data *val = calloc(1, sizeof(ov_mixer_data));
    val->socket = socket;
    val->remote = *remote;
    memset(val->user, 0, OV_HOST_NAME_MAX);

    bool result = ov_dict_set(self->sockets, (void*)(intptr_t)socket, val, NULL);

    ov_thread_lock_unlock(&self->lock);

    if (!result) val = ov_data_pointer_free(val);

    return result;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_mixer_registry_unregister_mixer(ov_mixer_registry *self,
                                        int socket){

    if (!self) goto error;

    if (!ov_thread_lock_try_lock(&self->lock)) goto error;

    ov_mixer_data *data = ov_dict_get(self->sockets, (void*)(intptr_t)socket);
    if (data){
        ov_dict_del(self->users, data->user);
    } 

    bool result = ov_dict_del(self->sockets, (void*)(intptr_t)socket);

    ov_thread_lock_unlock(&self->lock);

    return result;

error:
    return false;
}

/*----------------------------------------------------------------------------*/

struct container {

    int slot;
    int user;

};

/*----------------------------------------------------------------------------*/

static bool find_unused_mixer(const void *key, void *val, void *data){

    if (!key) return true;

    struct container *container = (struct container*) data;
    if (container->slot != 0) return true;

    ov_mixer_data *mixer = (ov_mixer_data*) val;

    if (0 == strlen(mixer->user)){

        container->slot = (intptr_t) key;

    }

    return true;
}

/*----------------------------------------------------------------------------*/

ov_mixer_data ov_mixer_registry_acquire_user(ov_mixer_registry *self,
                                               const char *uuid){

    ov_mixer_data data = {0};

    if (!self || !uuid) goto error;

    if (OV_HOST_NAME_MAX < strlen(uuid)) goto error;

    struct container container = (struct container){
        .slot = 0
    };

    if (!ov_thread_lock_try_lock(&self->lock)) goto error;

    bool result = ov_dict_for_each(
        self->sockets,
        &container,
        find_unused_mixer);

    if (0 == container.slot) goto done;

    ov_mixer_data *val = ov_dict_get(self->sockets, (void*)(intptr_t)container.slot);
    if (!val) goto done;

    strncpy(val->user, uuid, OV_HOST_NAME_MAX);

    char *key = ov_string_dup(val->user);
    result = ov_dict_set(self->users, key, 
        (void*)(intptr_t)container.slot, NULL);

    if (result)
        data = *val;
done:
    ov_thread_lock_unlock(&self->lock);
    return data;
error:  
    return data;
}

/*----------------------------------------------------------------------------*/

ov_mixer_data ov_mixer_registry_get_user(ov_mixer_registry *self,
                                               const char *uuid){

    ov_mixer_data data = {0};

    if (!self || !uuid) goto error;

    if (OV_HOST_NAME_MAX < strlen(uuid)) goto error;

    if (!ov_thread_lock_try_lock(&self->lock)) goto error;

    intptr_t socket = (intptr_t) ov_dict_get(self->users, uuid);

    ov_mixer_data *val = ov_dict_get(self->sockets, (void*)socket);

    if (val)
        data = *val;

    ov_thread_lock_unlock(&self->lock);
    return data;
error:  
    return data;
}

/*----------------------------------------------------------------------------*/

ov_mixer_data ov_mixer_registry_get_socket(ov_mixer_registry *self,
                                         int socket){

    ov_mixer_data data = {0};

    if (!self) goto error;

    if (!ov_thread_lock_try_lock(&self->lock)) goto error;

    ov_mixer_data *val = ov_dict_get(self->sockets, (void*)(intptr_t)socket);

    if (val)
        data = *val;

    ov_thread_lock_unlock(&self->lock);
    return data;
error:  
    return data;
}

/*----------------------------------------------------------------------------*/

bool ov_mixer_registry_release_user(ov_mixer_registry *self,
                                    const char *uuid){

    if (!self || !uuid) goto error;

    if (255 < strlen(uuid)) goto error;

    if (!ov_thread_lock_try_lock(&self->lock)) goto error;

    intptr_t socket = (intptr_t)ov_dict_get(self->users, uuid);
    ov_mixer_data *data = ov_dict_get(self->sockets, (void*)socket);

    if (data)
        memset(data->user, 0, 256);

    ov_dict_del(self->users, uuid);

    ov_thread_lock_unlock(&self->lock);
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool count_sockets(const void *key, void *val, void *data){

    if (!key) return true;

    struct container *container = (struct container*) data;

    ov_mixer_data *mixer = (ov_mixer_data*) val;

    container->slot++;

    if (0 != mixer->user[0])
        container->user++;

    return true;
}

/*----------------------------------------------------------------------------*/

ov_mixer_registry_count ov_mixer_registry_count_mixers(
        ov_mixer_registry *self){

    if (!self) goto error;

    struct container container = (struct container){
        .slot = 0,
        .user = 0
    };

    if (!ov_thread_lock_try_lock(&self->lock)) goto error;

    ov_dict_for_each(
        self->sockets,
        &container,
        count_sockets);

    ov_thread_lock_unlock(&self->lock);

    return (ov_mixer_registry_count){
        .mixers = container.slot,
        .used = container.user
    };

error:
    return (ov_mixer_registry_count){0};
}