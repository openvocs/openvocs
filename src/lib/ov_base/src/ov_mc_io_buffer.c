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
        @file           ov_mc_io_buffer.c
        @author         Töpfer, Markus

        @date           2026-03-10


        ------------------------------------------------------------------------
*/
#include "../include/ov_mc_io_buffer.h"

#include "../include/ov_buffer.h"
#include "../include/ov_dict.h"
#include "../include/ov_string.h"

#include "../include/ov_utils.h"

#define OV_MC_IO_BUFFER_MAGIC_BYTE 0x7532

/*----------------------------------------------------------------------------*/

struct ov_mc_io_buffer {

    uint16_t magic_byte;
    ov_mc_io_buffer_config config;
    ov_dict *dict;
};

/*----------------------------------------------------------------------------*/

ov_mc_io_buffer *ov_mc_io_buffer_cast(const void *data) {

    if (!data)
        return NULL;

    if (*(uint16_t *)data == OV_MC_IO_BUFFER_MAGIC_BYTE)
        return (ov_mc_io_buffer *)data;

    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_mc_io_buffer *ov_mc_io_buffer_create(ov_mc_io_buffer_config config) {

    if (!config.callback.success)
        return NULL;

    ov_mc_io_buffer *self = calloc(1, sizeof(ov_mc_io_buffer));
    if (!self)
        goto error;

    self->magic_byte = OV_MC_IO_BUFFER_MAGIC_BYTE;

    ov_dict_config d_config = ov_dict_string_key_config(255);
    d_config.value.data_function = ov_buffer_data_functions();

    self->dict = ov_dict_create(d_config);
    self->config = config;

    if (!self->dict)
        goto error;

    return self;
error:
    return ov_mc_io_buffer_free(self);
};

/*----------------------------------------------------------------------------*/

ov_mc_io_buffer *ov_mc_io_buffer_free(ov_mc_io_buffer *self) {

    if (!self)
        return NULL;

    self->dict = ov_dict_free(self->dict);
    free(self);
    return NULL;
}

/*----------------------------------------------------------------------------*/

bool ov_mc_io_buffer_drop(ov_mc_io_buffer *self, ov_socket_data remote) {

    if (!self || !self->dict)
        return false;

    char buffer[1024] = {0};
    snprintf(buffer, 1024, "%s:%i", remote.host, remote.port);

    return ov_dict_del(self->dict, (void *) &buffer);
}

/*----------------------------------------------------------------------------*/

bool ov_mc_io_buffer_push(ov_mc_io_buffer *self, ov_socket_data remote,
                            const ov_memory_pointer input) {

    char key[1024] = {0};
    snprintf(key, 1024, "%s:%i", remote.host, remote.port);

    if (!self || !self->dict || !input.start || (input.length == 0))
        goto error;

    ov_buffer *buffer = ov_buffer_cast(ov_dict_get(self->dict, (void *)key));

    if (!buffer) {

        buffer = ov_buffer_create(input.length);

        if (!ov_dict_set(self->dict, ov_string_dup(key), buffer, NULL)) {
            buffer = ov_buffer_free(buffer);
            goto error;
        }
    }

    if (!ov_buffer_push(buffer, (uint8_t *)input.start, input.length)) {
        ov_log_error("Failed to push to buffer.");
        goto error;
    }

    uint8_t *start = buffer->start;
    size_t open = buffer->length;
    uint8_t *last = NULL;

    uint8_t *ptr = NULL;
    size_t len = 0;

    ov_json_value *value = NULL;

    while (open > 0) {

        ptr = start;
        len = open;

        if (!ov_json_clear_whitespace(&ptr, &len))
            goto mismatch;

        if (self->config.objects_only && (ptr[0] != '{'))
            goto mismatch;

        /* try to match incomplete first */

        if (!ov_json_match(ptr, len, true, &last)) {

            goto mismatch;

        } else if (NULL == last) {

            /* wait for more input */
            OV_ASSERT(1 == len);
            break;

        } else {

            /* buffer is matching to JSON up to last,
             * try to parse some value */

            value = ov_json_value_from_string((char *)ptr, (last - ptr) + 1);

            if (value) {

                self->config.callback.success(self->config.callback.userdata,
                                              remote, value);

                /* check if dropped over callback */
                if (buffer != ov_dict_get(self->dict, (void *)&key))
                    goto error;

                /* shift buffer */

                if (!ov_buffer_shift(buffer, last + 1))
                    goto error;

                start = buffer->start;
                open = buffer->length;

            } else {

                /* advance parsing to next */
                open -= (last - start) + 1;
                start = last + 1;
            }
        }
    }

    return true;

mismatch:

    if (self->config.debug)
        ov_log_debug("Input not matching JSON at %i", socket);

error:

    if (self && self->config.callback.failure)
        self->config.callback.failure(self->config.callback.userdata, remote);

    ov_mc_io_buffer_drop(self, remote);
    return false;
}
