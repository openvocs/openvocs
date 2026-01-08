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
        @file           ov_json_io_buffer.h
        @author         Markus Töpfer

        @date           2021-03-29

        This function implementes some buffering of JSON IO messages.


        ------------------------------------------------------------------------
*/
#ifndef ov_json_io_buffer_h
#define ov_json_io_buffer_h

#include "ov_json.h"
#include "ov_memory_pointer.h"

/*----------------------------------------------------------------------------*/

typedef struct ov_json_io_buffer ov_json_io_buffer;

/*----------------------------------------------------------------------------*/

typedef struct ov_json_io_buffer_config {

    bool debug;
    bool objects_only;

    struct {

        void *userdata;
        void (*success)(void *userdata, int socket, ov_json_value *value);
        void (*failure)(void *userdata, int socket);

    } callback;

} ov_json_io_buffer_config;

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_json_io_buffer *ov_json_io_buffer_create(ov_json_io_buffer_config config);
ov_json_io_buffer *ov_json_io_buffer_free(ov_json_io_buffer *self);
ov_json_io_buffer *ov_json_io_buffer_cast(const void *self);

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
bool ov_json_io_buffer_push(ov_json_io_buffer *self, int socket,
                            const ov_memory_pointer content);

/*----------------------------------------------------------------------------*/

/**
    Drop all content of the io buffer at socket.

    e.g. to be used on socket close, to drop all uncomplete received content.

    @param self     instance pointer
    @param socket   socket id
*/
bool ov_json_io_buffer_drop(ov_json_io_buffer *self, int socket);

#endif /* ov_json_io_buffer_h */
