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
        @file           ov_vad_thread_msg.h
        @author         Markus Töpfer

        @date           2025-01-16


        ------------------------------------------------------------------------
*/
#ifndef ov_vad_thread_msg_h
#define ov_vad_thread_msg_h

#include <ov_base/ov_buffer.h>
#include <ov_base/ov_thread_message.h>

/*----------------------------------------------------------------------------*/

#define OV_VAD_THREAD_MSG_TYPE 100

/*----------------------------------------------------------------------------*/

typedef struct ov_vad_thread_msg {

    ov_thread_message public;
    ov_buffer *buffer;
    int socket;

} ov_vad_thread_msg;

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_vad_thread_msg *ov_vad_thread_msg_create();

#endif /* ov_vad_thread_msg_h */
