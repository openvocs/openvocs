/***
        ------------------------------------------------------------------------

        Copyright (c) 2022 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_vocs_loop.c
        @author         Markus Töpfer

        @date           2022-11-08


        ------------------------------------------------------------------------
*/
#include "../include/ov_vocs_loop.h"

#include <ov_base/ov_config_keys.h>
#include <ov_base/ov_dict.h>
#include <ov_base/ov_string.h>

#define OV_VOCS_LOOP_MAGIC_BYTES 0x5005

typedef struct Participant {

    char *client;
    char *user;
    char *role;

} Participant;

/*----------------------------------------------------------------------------*/

struct ov_vocs_loop {

    uint16_t magic_bytes;
    char *name;

    ov_dict *participants;
};

/*----------------------------------------------------------------------------*/

void *participant_free(void *self) {

    Participant *p = (Participant *)self;
    if (!p)
        return NULL;

    p->client = ov_data_pointer_free(p->client);
    p->user = ov_data_pointer_free(p->user);
    p->role = ov_data_pointer_free(p->role);
    p = ov_data_pointer_free(p);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_vocs_loop *ov_vocs_loop_create(const char *name) {

    ov_vocs_loop *self = NULL;
    if (!name)
        goto error;

    self = calloc(1, sizeof(ov_vocs_loop));
    if (!self)
        goto error;

    self->magic_bytes = OV_VOCS_LOOP_MAGIC_BYTES;

    ov_dict_config d_config = ov_dict_intptr_key_config(255);
    d_config.value.data_function.free = participant_free;
    self->participants = ov_dict_create(d_config);
    self->name = strdup(name);

    return self;
error:
    ov_vocs_loop_free(self);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_vocs_loop *ov_vocs_loop_free(ov_vocs_loop *self) {

    if (!self)
        return NULL;

    self->participants = ov_dict_free(self->participants);
    self->name = ov_data_pointer_free(self->name);
    self = ov_data_pointer_free(self);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_vocs_loop *ov_vocs_loop_cast(const void *self) {

    if (!self)
        goto error;

    if (*(uint16_t *)self == OV_VOCS_LOOP_MAGIC_BYTES)
        return (ov_vocs_loop *)self;
error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

void *ov_vocs_loop_free_void(void *self) {

    ov_vocs_loop *v = ov_vocs_loop_cast(self);
    return ov_vocs_loop_free(v);
}

/*----------------------------------------------------------------------------*/

int64_t ov_vocs_loop_get_participants_count(ov_vocs_loop *self) {

    if (!self)
        goto error;

    return ov_dict_count(self->participants);
error:
    return -1;
}

/*----------------------------------------------------------------------------*/

static bool add_participant(const void *key, void *value, void *data) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;

    if (!key)
        return true;
    ov_json_value *store = ov_json_value_cast(data);
    Participant *p = (Participant *)value;

    out = ov_json_object();
    val = ov_json_string(p->client);
    if (!ov_json_object_set(out, OV_KEY_CLIENT, val))
        goto error;
    val = ov_json_string(p->user);
    if (!ov_json_object_set(out, OV_KEY_USER, val))
        goto error;
    val = ov_json_string(p->role);
    if (!ov_json_object_set(out, OV_KEY_ROLE, val))
        goto error;

    if (!ov_json_array_push(store, out))
        goto error;
    return true;

error:
    ov_json_value_free(val);
    ov_json_value_free(out);
    return false;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_vocs_loop_get_participants(ov_vocs_loop *self) {

    ov_json_value *out = NULL;
    if (!self)
        goto error;

    out = ov_json_array();

    if (!ov_dict_for_each(self->participants, out, add_participant))
        goto error;

    return out;
error:
    ov_json_value_free(out);
    return NULL;
}

/*----------------------------------------------------------------------------*/

bool ov_vocs_loop_add_participant(ov_vocs_loop *self, int socket,
                                  const char *client, const char *user,
                                  const char *role) {

    Participant *p = NULL;

    if (0 >= socket || !self || !user || !role)
        goto error;

    p = calloc(1, sizeof(Participant));
    if (!p)
        goto error;

    if (client)
        p->client = ov_string_dup(client);
    if (user)
        p->user = ov_string_dup(user);
    if (role)
        p->role = ov_string_dup(role);

    intptr_t key = socket;

    if (!ov_dict_set(self->participants, (void *)key, p, NULL))
        goto error;

    return true;
error:
    participant_free(p);
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_vocs_loop_drop_participant(ov_vocs_loop *self, int socket) {

    if (!self || !self->participants)
        goto error;

    intptr_t key = socket;

    return ov_dict_del(self->participants, (void *)key);
error:
    return false;
}
