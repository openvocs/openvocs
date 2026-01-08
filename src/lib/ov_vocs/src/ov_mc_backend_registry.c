/***
        ------------------------------------------------------------------------

        Copyright (c) 2023 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_mc_backend_registry.c
        @author         Markus Töpfer

        @date           2023-01-21


        ------------------------------------------------------------------------
*/
#include "../include/ov_mc_backend_registry.h"

#include <ov_base/ov_dict.h>
#include <ov_base/ov_id.h>
#include <ov_base/ov_socket.h>
#include <ov_base/ov_string.h>

#define OV_MC_BACKEND_REGISTRY_MAGIC_BYTES 0xbeab

/*----------------------------------------------------------------------------*/

struct ov_mc_backend_registry {

    uint16_t magic_bytes;
    ov_mc_backend_registry_config config;

    ov_dict *users;

    size_t sockets;
    ov_mc_mixer_data socket[];
};

/*----------------------------------------------------------------------------*/

static void init_socket(ov_mc_mixer_data *self) {

    self->socket = -1;
    return;
}

/*----------------------------------------------------------------------------*/

ov_mc_backend_registry *
ov_mc_backend_registry_create(ov_mc_backend_registry_config config) {

    ov_mc_backend_registry *self = NULL;

    size_t max_sockets = ov_socket_get_max_supported_runtime_sockets(0);

    size_t size =
        sizeof(ov_mc_backend_registry) + max_sockets * sizeof(ov_mc_mixer_data);

    self = calloc(1, size);
    if (!self)
        goto error;

    self->magic_bytes = OV_MC_BACKEND_REGISTRY_MAGIC_BYTES;
    self->config = config;
    self->sockets = max_sockets;

    for (size_t i = 0; i < max_sockets; i++) {
        init_socket(&self->socket[i]);
    }

    self->users = ov_dict_create(ov_dict_string_key_config(255));
    if (!self->users)
        goto error;

    return self;
error:
    ov_mc_backend_registry_free(self);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_mc_backend_registry *
ov_mc_backend_registry_free(ov_mc_backend_registry *self) {

    if (!ov_mc_backend_registry_cast(self))
        return self;

    self->users = ov_dict_free(self->users);
    self = ov_data_pointer_free(self);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_mc_backend_registry *ov_mc_backend_registry_cast(const void *data) {

    if (!data)
        return NULL;

    if (*(uint16_t *)data != OV_MC_BACKEND_REGISTRY_MAGIC_BYTES)
        return NULL;

    return (ov_mc_backend_registry *)data;
}

/*----------------------------------------------------------------------------*/

bool ov_mc_backend_registry_register_mixer(ov_mc_backend_registry *self,
                                           ov_mc_mixer_data data) {

    if (!self)
        goto error;

    if (data.socket > (int)self->sockets)
        goto error;
    if (data.socket < 1)
        goto error;

    ov_mc_mixer_data *slot = &self->socket[data.socket];
    *slot = data;

    memset(slot->user, 0, sizeof(ov_id));

    return true;

error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_mc_backend_registry_unregister_mixer(ov_mc_backend_registry *self,
                                             int socket) {

    if (!self)
        goto error;

    if (socket > (int)self->sockets)
        goto error;
    if (socket < 1)
        goto error;

    ov_mc_mixer_data *slot = &self->socket[socket];
    *slot = (ov_mc_mixer_data){0};
    slot->socket = -1;
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

ov_mc_backend_registry_count
ov_mc_backend_registry_count_mixers(ov_mc_backend_registry *self) {

    if (!self)
        goto error;

    ov_mc_backend_registry_count counter = (ov_mc_backend_registry_count){0};

    for (size_t i = 0; i < self->sockets; i++) {

        if (-1 == self->socket[i].socket)
            continue;

        counter.mixers++;

        if (0 != self->socket[i].user[0])
            counter.used++;
    }

    return counter;

error:
    return (ov_mc_backend_registry_count){0};
}

/*----------------------------------------------------------------------------*/

ov_mc_mixer_data
ov_mc_backend_registry_acquire_user(ov_mc_backend_registry *self,
                                    const char *uuid) {

    if (!self || !uuid)
        goto error;

    ov_mc_mixer_data *slot = NULL;

    for (size_t i = 0; i < self->sockets; i++) {

        if (-1 == self->socket[i].socket)
            continue;

        if (0 != self->socket[i].user[0])
            continue;

        slot = &self->socket[i];
        break;
    }

    if (!slot)
        goto error;

    strncpy(slot->user, uuid, sizeof(ov_id));

    char *key = ov_string_dup((char *)slot->user);
    intptr_t val = slot->socket;

    if (!ov_dict_set(self->users, key, (void *)val, NULL)) {
        key = ov_data_pointer_free(key);
        memset(slot->user, 0, sizeof(ov_id));
        goto error;
    }

    return *slot;

error:
    return (ov_mc_mixer_data){0};
}

/*----------------------------------------------------------------------------*/

ov_mc_mixer_data ov_mc_backend_registry_get_user(ov_mc_backend_registry *self,
                                                 const char *uuid) {

    if (!self || !uuid)
        goto error;

    intptr_t slot = (intptr_t)ov_dict_get(self->users, uuid);
    return self->socket[slot];

error:
    return (ov_mc_mixer_data){0};
}

/*----------------------------------------------------------------------------*/

ov_mc_mixer_data ov_mc_backend_registry_get_socket(ov_mc_backend_registry *self,
                                                   int socket) {

    if (!self)
        goto error;

    if (socket > (int)self->sockets)
        goto error;
    if (socket < 1)
        goto error;

    return self->socket[socket];
error:
    return (ov_mc_mixer_data){0};
}

/*----------------------------------------------------------------------------*/

bool ov_mc_backend_registry_release_user(ov_mc_backend_registry *self,
                                         const char *uuid) {

    if (!self || !uuid)
        goto error;

    intptr_t slot = (intptr_t)ov_dict_get(self->users, uuid);
    OV_ASSERT((int)slot < (int)self->sockets);
    OV_ASSERT((int)slot >= 0);

    ov_mc_mixer_data *data = &self->socket[slot];

    memset(data->user, 0, sizeof(ov_id));
    ov_dict_del(self->users, uuid);
    return true;
error:
    return false;
}
