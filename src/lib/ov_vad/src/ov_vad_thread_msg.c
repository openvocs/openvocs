/***
        ------------------------------------------------------------------------

        Copyright (c) 2025 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_vad_thread_msg.c
        @author         Markus Töpfer

        @date           2025-01-16


        ------------------------------------------------------------------------
*/
#include "../include/ov_vad_thread_msg.h"

#include <ov_base/ov_string.h>

/*---------------------------------------------------------------------------*/

static ov_thread_message *thread_msg_free(ov_thread_message *msg) {

    if (msg->type != OV_VAD_THREAD_MSG_TYPE) return msg;

    ov_vad_thread_msg *m = (ov_vad_thread_msg *)msg;

    m->buffer = ov_buffer_free(m->buffer);
    m = ov_data_pointer_free(m);
    return NULL;
}

/*---------------------------------------------------------------------------*/

ov_vad_thread_msg *ov_vad_thread_msg_create() {

    ov_vad_thread_msg *self = NULL;

    self = calloc(1, sizeof(ov_vad_thread_msg));
    if (!self) goto error;

    self->public.magic_bytes = OV_THREAD_MESSAGE_MAGIC_BYTES;
    self->public.type = OV_VAD_THREAD_MSG_TYPE;
    self->public.free = thread_msg_free;

    self->buffer = ov_buffer_create(OV_UDP_PAYLOAD_OCTETS);

    return self;
error:
    ov_thread_message_free(ov_thread_message_cast(self));
    return NULL;
}