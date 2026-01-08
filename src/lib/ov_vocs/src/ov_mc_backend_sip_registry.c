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
        @file           ov_mc_backend_sip_registry.c
        @author         Markus Töpfer

        @date           2023-03-31


        ------------------------------------------------------------------------
*/
#include "../include/ov_mc_backend_sip_registry.h"

#include <ov_base/ov_dict.h>
#include <ov_base/ov_string.h>

#include <ov_base/ov_id.h>

#define OV_MC_BACKEND_SIP_REGISTRY_MAGIC_BYTES 0xb23b

/*----------------------------------------------------------------------------*/

struct ov_mc_backend_sip_registry {

    uint16_t magic_bytes;
    ov_mc_backend_sip_registry_config config;

    ov_dict *proxys;
    ov_dict *calls;
};

/*----------------------------------------------------------------------------*/

typedef struct Proxy {

    int socket;
    ov_id uuid;

} Proxy;

/*----------------------------------------------------------------------------*/

static void *free_proxy(void *self) {

    if (!self)
        return NULL;

    struct Proxy *proxy = (struct Proxy *)self;
    proxy->socket = 0;

    proxy = ov_data_pointer_free(proxy);
    return NULL;
}

/*----------------------------------------------------------------------------*/

typedef struct Call {

    int socket;
    char *call_id;
    char *loop_id;
    char *peer_id;

} Call;

/*----------------------------------------------------------------------------*/

static void *free_call(void *self) {

    if (!self)
        return NULL;

    Call *call = (Call *)self;

    call->socket = 0;
    call->call_id = ov_data_pointer_free(call->call_id);
    call->loop_id = ov_data_pointer_free(call->loop_id);
    call->peer_id = ov_data_pointer_free(call->peer_id);
    call = ov_data_pointer_free(call);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_mc_backend_sip_registry *
ov_mc_backend_sip_registry_create(ov_mc_backend_sip_registry_config config) {

    ov_mc_backend_sip_registry *self = NULL;

    self = calloc(1, sizeof(ov_mc_backend_sip_registry));
    if (!self)
        goto error;

    self->magic_bytes = OV_MC_BACKEND_SIP_REGISTRY_MAGIC_BYTES;
    self->config = config;

    ov_dict_config dconfig = ov_dict_intptr_key_config(255);
    dconfig.value.data_function.free = free_proxy;

    self->proxys = ov_dict_create(dconfig);
    if (!self->proxys)
        goto error;

    dconfig = ov_dict_string_key_config(255);
    dconfig.value.data_function.free = free_call;

    self->calls = ov_dict_create(dconfig);
    if (!self->calls)
        goto error;

    return self;
error:
    ov_mc_backend_sip_registry_free(self);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_mc_backend_sip_registry *
ov_mc_backend_sip_registry_free(ov_mc_backend_sip_registry *self) {

    if (!ov_mc_backend_sip_registry_cast(self))
        return self;

    self->proxys = ov_dict_free(self->proxys);
    self->calls = ov_dict_free(self->calls);
    self = ov_data_pointer_free(self);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_mc_backend_sip_registry *ov_mc_backend_sip_registry_cast(const void *data) {

    if (!data)
        return NULL;

    if (*(uint16_t *)data != OV_MC_BACKEND_SIP_REGISTRY_MAGIC_BYTES)
        return NULL;

    return (ov_mc_backend_sip_registry *)data;
}

/*----------------------------------------------------------------------------*/

bool ov_mc_backend_sip_registry_register_proxy(ov_mc_backend_sip_registry *self,
                                               int socket, const char *uuid) {

    if (!self || socket < 1)
        goto error;

    Proxy *proxy = calloc(1, sizeof(Proxy));
    if (!proxy)
        goto error;

    if (uuid)
        strncpy(proxy->uuid, uuid, sizeof(ov_id));
    proxy->socket = socket;

    intptr_t key = socket;
    if (!ov_dict_set(self->proxys, (void *)key, proxy, NULL)) {
        proxy = free_proxy(proxy);
        goto error;
    }

    return true;

error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_mc_backend_sip_registry_unregister_proxy(
    ov_mc_backend_sip_registry *self, int socket) {

    if (!self || socket < 1)
        goto error;

    intptr_t key = socket;
    return ov_dict_del(self->proxys, (void *)key);

error:
    return false;
}

/*----------------------------------------------------------------------------*/

struct container {

    int socket;
};

/*----------------------------------------------------------------------------*/

static bool get_proxy_socket(const void *key, void *val, void *data) {

    if (!key)
        return true;

    Proxy *proxy = (Proxy *)val;
    struct container *c = (struct container *)data;

    if (c->socket < 1)
        c->socket = proxy->socket;

    return true;
}

/*----------------------------------------------------------------------------*/

int ov_mc_backend_sip_registry_get_proxy_socket(
    ov_mc_backend_sip_registry *self) {

    if (!self)
        goto error;

    struct container c = {0};

    if (!ov_dict_for_each(self->proxys, &c, get_proxy_socket))
        goto error;

    return c.socket;

error:
    return -1;
}

/*----------------------------------------------------------------------------*/

bool ov_mc_backend_sip_registry_register_call(ov_mc_backend_sip_registry *self,
                                              int socket, const char *call_id,
                                              const char *loop_id,
                                              const char *peer_id) {

    if (!self || !call_id)
        goto error;

    Call *call = calloc(1, sizeof(Call));
    if (!call)
        goto error;

    call->socket = socket;
    call->call_id = ov_string_dup(call_id);
    call->peer_id = ov_string_dup(peer_id);
    call->loop_id = ov_string_dup(loop_id);

    char *key = ov_string_dup(call_id);

    if (!ov_dict_set(self->calls, key, call, NULL)) {
        call = free_call(call);
        key = ov_data_pointer_free(key);
        goto error;
    }

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_mc_backend_sip_registry_unregister_call(
    ov_mc_backend_sip_registry *self, const char *call_id) {

    if (!self || !call_id)
        goto error;

    return ov_dict_del(self->calls, call_id);
error:
    return false;
}

/*----------------------------------------------------------------------------*/

int ov_mc_backend_sip_registry_get_call_proxy(ov_mc_backend_sip_registry *self,
                                              const char *call_id) {

    if (!self || !call_id)
        goto error;

    Call *call = ov_dict_get(self->calls, call_id);
    if (!call)
        goto error;

    return call->socket;
error:
    return -1;
}

/*----------------------------------------------------------------------------*/

const char *
ov_mc_backend_sip_registry_get_call_loop(ov_mc_backend_sip_registry *self,
                                         const char *call_id) {

    if (!self || !call_id)
        goto error;

    Call *call = ov_dict_get(self->calls, call_id);
    if (!call)
        goto error;

    return call->loop_id;
error:
    return NULL;
}
