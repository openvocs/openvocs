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
        @file           ov_mc_interconnect_session.h
        @author         Markus Töpfer

        @date           2023-12-13


        ------------------------------------------------------------------------
*/
#ifndef ov_mc_interconnect_session_h
#define ov_mc_interconnect_session_h

#include <ov_base/ov_socket.h>

typedef struct ov_mc_interconnect_session ov_mc_interconnect_session;

/*----------------------------------------------------------------------------*/

#include "ov_mc_interconnect.h"

/*----------------------------------------------------------------------------*/

typedef struct {

    ov_mc_interconnect *base;
    ov_event_loop *loop;
    ov_dtls *dtls;

    int signaling;

    ov_socket_configuration internal;

    struct {

        char interface[OV_MC_INTERCONNECT_INTERFACE_NAME_MAX];
        ov_socket_data signaling;
        ov_socket_data media;

    } remote;

    uint64_t reconnect_interval_usecs;
    uint64_t keepalive_trigger_usec;

} ov_mc_interconnect_session_config;

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_mc_interconnect_session *
ov_mc_interconnect_session_create(ov_mc_interconnect_session_config config);

/*----------------------------------------------------------------------------*/

ov_mc_interconnect_session *
ov_mc_interconnect_session_free(ov_mc_interconnect_session *self);

/*----------------------------------------------------------------------------*/

void *ov_mc_interconnect_session_free_void(void *self);

/*----------------------------------------------------------------------------*/

ov_mc_interconnect_session *ov_mc_interconnect_session_cast(const void *data);

/*----------------------------------------------------------------------------*/

int ov_mc_interconnect_session_send(ov_mc_interconnect_session *self,
                                    const uint8_t *buffer, size_t size);

/*----------------------------------------------------------------------------*/

bool ov_mc_interconnect_session_handshake_active(
    ov_mc_interconnect_session *self, const char *fingerprint);

/*----------------------------------------------------------------------------*/

bool ov_mc_interconnect_session_handshake_passive(
    ov_mc_interconnect_session *self, const uint8_t *buffer, size_t size);

/*----------------------------------------------------------------------------*/

int ov_mc_interconnect_session_get_signaling_socket(
    ov_mc_interconnect_session *self);

/*----------------------------------------------------------------------------*/

ov_dtls *ov_mc_interconnect_session_get_dtls(ov_mc_interconnect_session *self);

/*----------------------------------------------------------------------------*/

bool ov_mc_interconnect_session_dtls_io(ov_mc_interconnect_session *self,
                                        const uint8_t *buffer, size_t size);

/*----------------------------------------------------------------------------*/

bool ov_mc_interconnect_session_add(ov_mc_interconnect_session *self,
                                    const char *loop, uint32_t ssrc);

/*----------------------------------------------------------------------------*/

bool ov_mc_interconnect_session_remove(ov_mc_interconnect_session *self,
                                       const char *loop);

/*----------------------------------------------------------------------------*/

bool ov_mc_interconnect_session_forward_rtp_external_to_internal(
    ov_mc_interconnect_session *self, uint8_t *buffer, size_t size,
    const ov_socket_data *remote);

/*----------------------------------------------------------------------------*/

bool ov_mc_interconnect_session_forward_loop_io_to_external(
    ov_mc_interconnect_session *self, ov_mc_interconnect_loop *loop,
    uint8_t *buffer, size_t size);

#endif /* ov_mc_interconnect_session_h */
