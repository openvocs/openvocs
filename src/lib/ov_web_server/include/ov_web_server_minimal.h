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
        @file           ov_web_server_minimal.h
        @author         Töpfer, Markus

        @date           2025-11-12


        ------------------------------------------------------------------------
*/
#ifndef ov_web_server_minimal_h
#define ov_web_server_minimal_h

#include "ov_web_server.h"

/*----------------------------------------------------------------------------*/

typedef struct ov_web_server_minimal ov_web_server_minimal;

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_web_server_minimal *ov_web_server_minimal_create(ov_web_server_config config);
ov_web_server_minimal *ov_web_server_minimal_free(ov_web_server_minimal *self);
ov_web_server_minimal *ov_web_server_minimal_cast(const void *data);

bool ov_web_server_minimal_close(ov_web_server_minimal *self, int socket);

/*
 *      ------------------------------------------------------------------------
 *
 *      SEND FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

bool ov_web_server_minimal_send(ov_web_server_minimal *self, 
                                int socket, 
                                const ov_buffer *data);

bool ov_web_server_minimal_send_json(ov_web_server_minimal *self,
                                 int socket,
                                 ov_json_value const *const data);

/*
 *      ------------------------------------------------------------------------
 *
 *      CONFIG FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

bool ov_web_server_minimal_configure_uri_event_io(
    ov_web_server_minimal *self,
    const ov_memory_pointer hostname,
    const ov_event_io_config input);


#endif /* ov_web_server_minimal_h */
