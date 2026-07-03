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
        @file           ov_sip_io_buffer.h
        @author         Töpfer, Markus

        @date           2026-06-30


        ------------------------------------------------------------------------
*/
#ifndef ov_sip_io_buffer_h
#define ov_sip_io_buffer_h

#include "ov_sip_pointer.h"
#include <ov_base/ov_memory_pointer.h>

/*----------------------------------------------------------------------------*/

typedef struct ov_sip_io_buffer ov_sip_io_buffer;

/*----------------------------------------------------------------------------*/

typedef struct ov_sip_io_buffer_config {

    struct {

        void *userdata;
        void (*success)(void *userdata, int socket, ov_sip_message *value);
        void (*failure)(void *userdata, int socket);

    } callback;

} ov_sip_io_buffer_config;

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_sip_io_buffer *ov_sip_io_buffer_create(ov_sip_io_buffer_config config);
ov_sip_io_buffer *ov_sip_io_buffer_free(ov_sip_io_buffer *self);
ov_sip_io_buffer *ov_sip_io_buffer_cast(const void *self);

/*----------------------------------------------------------------------------*/

/**
    Push some new IO content to the io buffer for some socket
    (or any other int id).

    @param self     instance pointer
    @param socket   socket id
    @param content  new content received at socket

    @returns true if the content within the IO buffer is valid JSON
    @returns false if the content within the IO buffer is not valid JSON
    in case of non valid content, the buffer will be emptied at id socket.
*/
bool ov_sip_io_buffer_push(ov_sip_io_buffer *self, int socket,
                            const ov_memory_pointer content);

/*----------------------------------------------------------------------------*/

/**
    Drop all content of the io buffer at socket.

    e.g. to be used on socket close, to drop all uncomplete received content.

    @param self     instance pointer
    @param socket   socket id
*/
bool ov_sip_io_buffer_drop(ov_sip_io_buffer *self, int socket);


#endif /* ov_sip_io_buffer_h */
