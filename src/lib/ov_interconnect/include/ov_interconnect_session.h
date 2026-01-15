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
        @file           ov_interconnect_session.h
        @author         Töpfer, Markus

        @date           2026-01-15


        ------------------------------------------------------------------------
*/
#ifndef ov_interconnect_session_h
#define ov_interconnect_session_h

#include <ov_base/ov_socket.h>

typedef struct ov_interconnect_session ov_interconnect_session;

/*----------------------------------------------------------------------------*/

#include "ov_interconnect.h"

/*----------------------------------------------------------------------------*/

typedef struct {

    ov_interconnect *base;
    ov_event_loop *loop;
    ov_dtls *dtls;

    int signaling;

    ov_socket_configuration internal;

    struct {

        char interface[OV_INTERCONNECT_INTERFACE_NAME_MAX];
        ov_socket_data signaling;
        ov_socket_data media;

    } remote;

    uint64_t reconnect_interval_usecs;
    uint64_t keepalive_trigger_usec;

} ov_interconnect_session_config;

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_interconnect_session *ov_interconnect_session_create(
    ov_interconnect_session_config config);

void *ov_interconnect_session_free(void *self);

/*
 *      ------------------------------------------------------------------------
 *
 *      IO FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

int ov_interconnect_session_send(ov_interconnect_session *self,
                                const uint8_t *buffer, size_t size);

/*----------------------------------------------------------------------------*/

bool ov_interconnect_session_media_io_external(
    ov_interconnect_session *self,
    uint8_t *buffer,
    size_t bytes,
    const ov_socket_data *remote);

/*----------------------------------------------------------------------------*/

bool ov_interconnect_session_media_io_ssl_external(
    ov_interconnect_session *self,
    uint8_t *buffer,
    size_t bytes,
    const ov_socket_data *remote);

/*----------------------------------------------------------------------------*/

bool ov_interconnect_session_handshake_active(
    ov_interconnect_session *self,
    const char *fingerprint);

/*----------------------------------------------------------------------------*/

bool ov_interconnect_session_forward_loop_io(
    ov_interconnect_session *self,
    const ov_interconnect_loop *loop,
    uint8_t *buffer, 
    size_t size);

/*
 *      ------------------------------------------------------------------------
 *
 *      DEFINITION FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

bool ov_interconnect_session_add(
    ov_interconnect_session *self,
    const char *name, 
    uint32_t ssrc_remote);

int ov_interconnect_session_get_signaling_socket(
    const ov_interconnect_session *self);

bool ov_interconnect_session_loops_added(
    const ov_interconnect_session *self);

#endif /* ov_interconnect_session_h */
