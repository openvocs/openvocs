/***
        ------------------------------------------------------------------------

        Copyright (c) 2021 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_event_broadcast.c
        @author         Markus Töpfer

        @date           2021-01-23


        ------------------------------------------------------------------------
*/
#include "../include/ov_event_broadcast.h"

#define OV_EVENT_BROADCAST_MAGIC_BYTE 0xbbbb

/* Based on the testoutput for ov_event_broadcast_send with 100K interested
 * sockets and runtime of > 300 micro seconds the default timeout is set to
 * 500 micro seconds.
 * NOTE this is some hardware dependend and use case dependend setting */
#define IMPL_THREADLOCK_TIMEOUT_USEC 500 * 1000 // 500 micro seconds

#include <ov_base/ov_utils.h>

#include <ov_base/ov_convert.h>
#include <ov_base/ov_socket.h>
#include <ov_base/ov_thread_lock.h>

struct connection {

    uint8_t type;
    ov_event_parameter_send send;
};

/*----------------------------------------------------------------------------*/

struct ov_event_broadcast {

    const uint16_t magic_byte;
    ov_event_broadcast_config config;

    /* Threadlock to be used for send */

    ov_thread_lock lock;

    /* We store the last (biggest known socket),
     * to limit for loops over connections */

    int last;

    /*  We need some store to flag a socket as included or not,
     *  so we use the a rather small array type (byte) and the socket itself
     *  as the key (pos based access) */

    struct connection connections[];
};

/*----------------------------------------------------------------------------*/

static ov_event_broadcast init = {

    .magic_byte = OV_EVENT_BROADCAST_MAGIC_BYTE};

/*----------------------------------------------------------------------------*/

static ov_event_broadcast *broadcast_cast(const void *self) {

    /* NOT external, as it is not required external */

    if (!self) goto error;

    if (*(uint16_t *)self == OV_EVENT_BROADCAST_MAGIC_BYTE)
        return (ov_event_broadcast *)self;
error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

static void cb_close(void *userdata, int socket) {

    ov_event_broadcast *bcast = broadcast_cast(userdata);
    if (!bcast || socket < 0) goto error;

    ov_event_broadcast_set(bcast, socket, OV_BROADCAST_UNSET);

error:
    return;
};

/*----------------------------------------------------------------------------*/

static bool cb_process(void *userdata,
                       const int socket,
                       const ov_event_parameter *params,
                       ov_json_value *input) {

    ov_event_broadcast *bcast = broadcast_cast(userdata);
    if (!bcast || !input || socket < 0) goto error;

    if (!ov_event_broadcast_set(bcast, socket, OV_BROADCAST)) goto error;

    bool result =
        ov_event_broadcast_send_params(bcast, params, input, OV_BROADCAST);

    input = ov_json_value_free(input);
    return result;
error:
    ov_json_value_free(input);
    return false;
}

/*----------------------------------------------------------------------------*/

ov_event_broadcast *ov_event_broadcast_create(
    ov_event_broadcast_config config) {

    if (0 == config.lock_timeout_usec)
        config.lock_timeout_usec = IMPL_THREADLOCK_TIMEOUT_USEC;

    ov_event_broadcast *bcast = NULL;

    if (0 == config.max_sockets)
        config.max_sockets = ov_socket_get_max_supported_runtime_sockets(0);

    /* TODO maybe limit the max_size to some reasonable value,
     * e.g. 1 million sockets or so,
     * otherwise this may eat all memory and become to slow at some point TBD */

    uint64_t size = sizeof(ov_event_broadcast) +
                    (config.max_sockets * sizeof(struct connection));

    if (size >= INT_MAX) goto error;

    bcast = calloc(1, (size_t)size);
    if (!bcast) {
        bcast = ov_data_pointer_free(bcast);
        goto error;
    }

    if (!memcpy(bcast, &init, sizeof(ov_event_broadcast))) {
        bcast = ov_data_pointer_free(bcast);
        goto error;
    }

    bcast->config = config;

    for (uint32_t i = 0; i < config.max_sockets; i++) {
        bcast->connections[i].type = OV_BROADCAST_UNSET;
    }

    bcast->last = 0;

    if (!ov_thread_lock_init(&bcast->lock, config.lock_timeout_usec))
        goto error;

    // ov_log_debug("ov_event_broadcast allocated %zu bytes", size);
    return bcast;

error:
    ov_event_broadcast_free(bcast);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_event_broadcast *ov_event_broadcast_free(ov_event_broadcast *self) {

    if (!broadcast_cast(self)) return self;

    int i = 0;
    int max = 100;

    for (i = 0; i < max; i++) {

        if (ov_thread_lock_try_lock(&self->lock)) break;
    }

    if (i == max) {
        OV_ASSERT(1 == 0);
        return self;
    }

    if (!ov_thread_lock_unlock(&self->lock)) {
        OV_ASSERT(1 == 0);
    }

    if (!ov_thread_lock_clear(&self->lock))
        ov_log_error("Failed to clear thread lock");

    self = ov_data_pointer_free(self);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_event_io_config ov_event_broadcast_configure_uri_event_io(
    ov_event_broadcast *self, const char *name) {

    ov_event_io_config config = {0};

    if (!self || !name) goto error;

    /* Check max name length fit to ov_event_config max */

    size_t length = strlen(name);
    if (length >= PATH_MAX) {
        ov_log_error("name input to long max name length %zu", PATH_MAX);
        goto error;
    }

    if (!strncat(config.name, name, PATH_MAX - 1)) goto error;

    config.callback.close = cb_close;
    config.callback.process = cb_process;
    config.userdata = self;

    return config;

error:
    return (ov_event_io_config){0};
}

/*----------------------------------------------------------------------------*/

bool ov_event_broadcast_set(ov_event_broadcast *self,
                            int socket,
                            uint8_t type) {

    if (!self || (socket < 0)) return false;

    if ((uint32_t)socket > self->config.max_sockets) return false;

    if (!ov_thread_lock_try_lock(&self->lock)) return false;

    self->connections[socket].type = type;

    switch (type) {

        case OV_BROADCAST_UNSET:

            /* In case of unset and last,
             * move last pointer to last set entry */

            if (self->last == socket) {

                for (int i = socket; i > 0; i--) {

                    if (self->connections[i].type == OV_BROADCAST_UNSET)
                        continue;

                    self->last = i;
                    goto done;
                }

                self->last = 0;
            }

            break;

        default:

            /* In case we set a broadcast to a socket > last,
             * the id of last need to advance to socket */

            if (self->last < socket) self->last = socket;

            break;
    }

done:

    if (!ov_thread_lock_unlock(&self->lock)) {

        ov_log_critical(
            "Unlocking failed"
            " - service restart required.");

        OV_ASSERT(1 == 0);
    }

    return true;
}

/*----------------------------------------------------------------------------*/

bool ov_event_broadcast_set_send(ov_event_broadcast *self,
                                 int socket,
                                 uint8_t type,
                                 ov_event_parameter_send send) {

    if (!self || (socket < 0)) return false;

    if ((uint32_t)socket > self->config.max_sockets) return false;

    if (!ov_thread_lock_try_lock(&self->lock)) return false;

    self->connections[socket].type = type;
    self->connections[socket].send = send;

    switch (type) {

        case OV_BROADCAST_UNSET:

            /* In case of unset and last,
             * move last pointer to last set entry */

            if (self->last == socket) {

                for (int i = socket; i > 0; i--) {

                    if (self->connections[i].type == OV_BROADCAST_UNSET)
                        continue;

                    self->last = i;
                    goto done;
                }

                self->last = 0;
            }

            break;

        default:

            /* In case we set a broadcast to a socket > last,
             * the id of last need to advance to socket */

            if (self->last < socket) self->last = socket;

            break;
    }

done:

    if (!ov_thread_lock_unlock(&self->lock)) {

        ov_log_critical(
            "Unlocking failed"
            " - service restart required.");

        OV_ASSERT(1 == 0);
    }

    return true;
}

/*----------------------------------------------------------------------------*/

uint8_t ov_event_broadcast_get(ov_event_broadcast *self, int socket) {

    if (!self || (socket < 0)) return 0;

    if ((uint32_t)socket > self->config.max_sockets) return 0;

    return self->connections[socket].type;
}

/*----------------------------------------------------------------------------*/

ov_list *ov_event_broadcast_get_sockets(ov_event_broadcast *self,
                                        uint8_t type) {

    ov_list *list = NULL;

    if (!self) goto error;

    if (OV_BROADCAST_UNSET == type) goto error;

    list = ov_list_create((ov_list_config){0});

    if (!ov_thread_lock_try_lock(&self->lock)) return false;

    bool result = true;

    for (intptr_t i = 0; i <= self->last; i++) {

        if (0x00 != (type & self->connections[i].type)) {

            if (!ov_list_push(list, (void *)i)) {
                result = false;
                break;
            }
        }
    }

    if (!ov_thread_lock_unlock(&self->lock)) {

        ov_log_critical(
            "Unlocking failed"
            " - service restart required.");

        OV_ASSERT(1 == 0);
    }

    if (result) return list;

error:
    list = ov_list_free(list);
    return NULL;
}
/*----------------------------------------------------------------------------*/

bool ov_event_broadcast_send(ov_event_broadcast *self,
                             const ov_json_value *input,
                             uint8_t type) {

    if (!self || !input) goto error;

    if (OV_BROADCAST_UNSET == type) goto error;

    if (!ov_thread_lock_try_lock(&self->lock)) return false;

    int result = true;

    for (int i = 0; i <= self->last; i++) {

        if (0x00 != (type & self->connections[i].type)) {

            ov_event_parameter params =
                (ov_event_parameter){.send = self->connections[i].send};

            if (!ov_event_io_send(&params, i, input)) {
                result = false;
                break;
            }
        }
    }

    if (!ov_thread_lock_unlock(&self->lock)) {

        ov_log_critical(
            "Unlocking failed"
            " - service restart required.");

        OV_ASSERT(1 == 0);
    }

    return result;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_event_broadcast_send_params(ov_event_broadcast *self,
                                    const ov_event_parameter *params,
                                    const ov_json_value *input,
                                    uint8_t type) {

    if (!self || !params || !input) goto error;

    if (!params->send.instance || !params->send.send) goto error;

    if (OV_BROADCAST_UNSET == type) goto error;

    if (!ov_thread_lock_try_lock(&self->lock)) return false;

    int result = true;

    for (int i = 0; i <= self->last; i++) {

        if (0x00 != (type & self->connections[i].type)) {

            if (!ov_event_io_send(params, i, input)) {
                result = false;
                break;
            }
        }
    }

    if (!ov_thread_lock_unlock(&self->lock)) {

        ov_log_critical(
            "Unlocking failed"
            " - service restart required.");

        OV_ASSERT(1 == 0);
    }

    return result;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_event_broadcast_is_empty(ov_event_broadcast *self) {

    bool result = false;

    if (!self) goto error;

    if (!ov_thread_lock_try_lock(&self->lock)) goto error;

    result = true;

    if (OV_BROADCAST_UNSET != self->connections[self->last].type) {

        result = false;

    } else {

        result = true;
    }

    if (!ov_thread_lock_unlock(&self->lock)) {

        ov_log_critical(
            "Unlocking failed"
            " - service restart required.");

        OV_ASSERT(1 == 0);
    }

error:
    return result;
}

/*----------------------------------------------------------------------------*/

int64_t ov_event_broadcast_count(ov_event_broadcast *self, uint8_t type) {

    if (!self) goto error;

    int64_t count = 0;

    if (!ov_thread_lock_try_lock(&self->lock)) goto error;

    for (int i = 0; i <= self->last; i++) {

        if (0x00 != (type & self->connections[i].type)) {
            count++;
        }
    }

    if (!ov_thread_lock_unlock(&self->lock)) {

        ov_log_critical(
            "Unlocking failed"
            " - service restart required.");

        OV_ASSERT(1 == 0);
    }

    return count;
error:
    return -1;
}

/*----------------------------------------------------------------------------*/

static bool add_socket_state(ov_json_value *socket, uint8_t state) {

    ov_json_value *val = NULL;

    OV_ASSERT(socket);
    if (!socket) return false;

    if (0 == state) return true;

    if (state & OV_BROADCAST) {
        val = ov_json_true();
        if (!ov_json_object_set(socket, OV_BROADCAST_KEY_BROADCAST, val))
            goto error;
    }

    if (state & OV_USER_BROADCAST) {
        val = ov_json_true();
        if (!ov_json_object_set(socket, OV_BROADCAST_KEY_USER_BROADCAST, val))
            goto error;
    }

    if (state & OV_ROLE_BROADCAST) {
        val = ov_json_true();
        if (!ov_json_object_set(socket, OV_BROADCAST_KEY_ROLE_BROADCAST, val))
            goto error;
    }

    if (state & OV_LOOP_BROADCAST) {
        val = ov_json_true();
        if (!ov_json_object_set(socket, OV_BROADCAST_KEY_LOOP_BROADCAST, val))
            goto error;
    }

    if (state & OV_LOOP_SENDER_BROADCAST) {
        val = ov_json_true();
        if (!ov_json_object_set(
                socket, OV_BROADCAST_KEY_LOOP_SENDER_BROADCAST, val))
            goto error;
    }

    if (state & OV_PROJECT_BROADCAST) {
        val = ov_json_true();
        if (!ov_json_object_set(
                socket, OV_BROADCAST_KEY_PROJECT_BROADCAST, val))
            goto error;
    }

    if (state & OV_DOMAIN_BROADCAST) {
        val = ov_json_true();
        if (!ov_json_object_set(socket, OV_BROADCAST_KEY_DOMAIN_BROADCAST, val))
            goto error;
    }

    if (state & OV_SYSTEM_BROADCAST) {
        val = ov_json_true();
        if (!ov_json_object_set(socket, OV_BROADCAST_KEY_SYSTEM_BROADCAST, val))
            goto error;
    }

    return true;
error:
    ov_json_value_free(val);
    return false;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_event_broadcast_state(ov_event_broadcast *self) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;

    char key[30] = {0};
    char *ptr = key;
    size_t len = 0;

    if (!self) goto error;

    if (!ov_thread_lock_try_lock(&self->lock)) goto error;

    out = ov_json_object();

    bool result = true;

    for (int i = 0; i <= self->last; i++) {

        if (0 == self->connections[i].type) continue;

        len = 30;
        memset(key, 0, len);

        if (!ov_convert_int64_to_string(i, &ptr, &len)) {
            result = false;
            break;
        }

        val = ov_json_object();
        if (!ov_json_object_set(out, key, val)) {
            val = ov_json_value_free(val);
            result = false;
            break;
        }

        if (!add_socket_state(val, self->connections[i].type)) {
            result = false;
            break;
        }
    }

    if (!ov_thread_lock_unlock(&self->lock)) {

        ov_log_critical(
            "Unlocking failed"
            " - service restart required.");

        OV_ASSERT(1 == 0);
    }

    OV_ASSERT(result);
    if (!result) goto error;

    return out;
error:
    ov_json_value_free(val);
    ov_json_value_free(out);
    return NULL;
}
