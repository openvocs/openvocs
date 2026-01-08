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
        @file           ov_mc_frontend_registry.c
        @author         Markus Töpfer

        @date           2023-01-23


        ------------------------------------------------------------------------
*/
#include "../include/ov_mc_frontend_registry.h"

#include <inttypes.h>

#include <ov_base/ov_dict.h>
#include <ov_base/ov_id.h>
#include <ov_base/ov_socket.h>

#define OV_MC_FRONTEND_REGISTRY_MAGIC_BYTES 0xf5e5

typedef struct IceSession {

    int socket;
    ov_id uuid;
    ov_mc_frontend_registry *registry;

} IceSession;

/*----------------------------------------------------------------------------*/

typedef struct IceProxyConnection {

    int socket;
    ov_id uuid;

    uint64_t load;
    ov_dict *sessions;

} IceProxyConnection;

/*----------------------------------------------------------------------------*/

struct ov_mc_frontend_registry {

    uint16_t magic_bytes;
    ov_mc_frontend_registry_config config;

    ov_dict *sessions;

    size_t sockets;
    IceProxyConnection socket[];
};

/*
 *      ------------------------------------------------------------------------
 *
 *      SESSION FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

static void *ice_session_free(void *self) {

    IceSession *session = (IceSession *)self;
    if (!session)
        return NULL;

    session->registry->config.callback.session.drop(
        session->registry->config.callback.userdata, session->uuid);

    session = ov_data_pointer_free(session);
    return NULL;
}

/*----------------------------------------------------------------------------*/

static IceSession *ice_session_create(ov_mc_frontend_registry *self, int socket,
                                      const char *uuid) {

    if (!self || !uuid)
        goto error;

    IceSession *session = calloc(1, sizeof(IceSession));
    if (!session)
        goto error;

    session->socket = socket;
    session->registry = self;
    strncpy(session->uuid, uuid, sizeof(ov_id));

    return session;

error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

static void init_socket(IceProxyConnection *self) {

    self->socket = -1;
    return;
}

/*----------------------------------------------------------------------------*/

ov_mc_frontend_registry *
ov_mc_frontend_registry_create(ov_mc_frontend_registry_config config) {

    ov_mc_frontend_registry *self = NULL;

    size_t max_sockets = ov_socket_get_max_supported_runtime_sockets(0);

    size_t size = sizeof(ov_mc_frontend_registry) +
                  max_sockets * sizeof(IceProxyConnection);

    self = calloc(1, size);
    if (!self)
        goto error;

    self->magic_bytes = OV_MC_FRONTEND_REGISTRY_MAGIC_BYTES;
    self->config = config;
    self->sockets = max_sockets;

    for (size_t i = 0; i < max_sockets; i++) {
        init_socket(&self->socket[i]);
    }

    self->sessions = ov_dict_create(ov_dict_string_key_config(255));
    if (!self->sessions)
        goto error;

    return self;
error:
    ov_mc_frontend_registry_free(self);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_mc_frontend_registry *
ov_mc_frontend_registry_free(ov_mc_frontend_registry *self) {

    if (!ov_mc_frontend_registry_cast(self))
        return self;

    for (size_t i = 0; i < self->sockets; i++) {

        self->socket[i].sessions = ov_dict_free(self->socket[i].sessions);
    }

    self->sessions = ov_dict_free(self->sessions);
    self = ov_data_pointer_free(self);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_mc_frontend_registry *ov_mc_frontend_registry_cast(const void *data) {

    if (!data)
        return NULL;

    if (*(uint16_t *)data != OV_MC_FRONTEND_REGISTRY_MAGIC_BYTES)
        return NULL;

    return (ov_mc_frontend_registry *)data;
}

/*----------------------------------------------------------------------------*/

bool ov_mc_frontend_registry_register_proxy(ov_mc_frontend_registry *self,
                                            int socket, const char *uuid) {

    if (!self || !uuid)
        goto error;

    if ((size_t)socket > self->sockets)
        goto error;
    if (socket < 1)
        goto error;

    IceProxyConnection *connection = &self->socket[socket];

    if (connection->sessions)
        goto error;

    ov_dict_config d_config = ov_dict_string_key_config(255);
    d_config.value.data_function.free = ice_session_free;

    connection->sessions = ov_dict_create(d_config);
    if (!connection->sessions)
        goto error;

    connection->socket = socket;
    strncpy(connection->uuid, uuid, sizeof(ov_id));

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_mc_frontend_registry_unregister_proxy(ov_mc_frontend_registry *self,
                                              int socket) {

    if (!self)
        goto error;

    if ((size_t)socket > self->sockets)
        goto error;
    if (socket < 1)
        goto error;

    IceProxyConnection *connection = &self->socket[socket];
    connection->sessions = ov_dict_free(connection->sessions);
    connection->socket = -1;
    memset(connection->uuid, 0, sizeof(ov_id));

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

int ov_mc_frontend_registry_get_proxy_socket(ov_mc_frontend_registry *self) {

    if (!self)
        goto error;

    uint64_t load = UINT64_MAX;
    int socket = -1;

    for (size_t i = 0; i < self->sockets; i++) {

        if (-1 == self->socket[i].socket)
            continue;

        if (self->socket[i].load < load) {
            load = self->socket[i].load;
            socket = i;
        }
    }

    return socket;

error:
    return -1;
}

/*----------------------------------------------------------------------------*/

bool ov_mc_frontend_registry_register_session(ov_mc_frontend_registry *self,
                                              int socket,
                                              const char *session_uuid) {

    if (!self || !session_uuid)
        goto error;

    if ((size_t)socket > self->sockets)
        goto error;
    if (socket < 1)
        goto error;

    IceProxyConnection *connection = &self->socket[socket];
    if (connection->socket != socket)
        goto error;
    OV_ASSERT(connection->sessions);

    IceSession *session = ice_session_create(self, socket, session_uuid);
    if (!session)
        goto error;

    // store session in connection dict

    char *key = strdup(session_uuid);
    if (!ov_dict_set(connection->sessions, key, session, NULL)) {
        key = ov_data_pointer_free(key);
        session = ov_data_pointer_free(session);
        goto error;
    }

    // store connection in sessions dict

    key = strdup(session_uuid);
    intptr_t val = socket;

    if (!ov_dict_set(self->sessions, key, (void *)val, NULL)) {
        ov_dict_del(connection->sessions, key);
        key = ov_data_pointer_free(key);
        goto error;
    }

    connection->load++;
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_mc_frontend_registry_unregister_session(ov_mc_frontend_registry *self,
                                                const char *session_uuid) {

    if (!self || !session_uuid)
        goto error;

    intptr_t conn = (intptr_t)ov_dict_get(self->sessions, session_uuid);
    IceProxyConnection *connection = &self->socket[conn];

    IceSession *sess = ov_dict_remove(connection->sessions, session_uuid);
    sess = ov_data_pointer_free(sess);
    ov_dict_del(self->sessions, session_uuid);

    if (connection->load > 0)
        connection->load--;

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

int ov_mc_frontend_registry_get_session_socket(ov_mc_frontend_registry *self,
                                               const char *session_uuid) {

    if (!self || !session_uuid)
        goto error;

    intptr_t conn = (intptr_t)ov_dict_get(self->sessions, session_uuid);
    return conn;
error:
    return -1;
}
