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
        @file           ov_mc_interconnect.h
        @author         Markus Töpfer

        @date           2023-12-13

        Interconnect implementation of different ov_* backends. 

        ------------------------------------------------------------------------
*/
#ifndef ov_mc_interconnect_h
#define ov_mc_interconnect_h

#include <ov_base/ov_event_loop.h>
#include <ov_base/ov_id.h>
#include <ov_base/ov_socket.h>

#include <ov_core/ov_mixer_registry.h>
#include <ov_core/ov_mixer_msg.h>

#include <ov_encryption/ov_dtls.h>

#include <srtp2/srtp.h>

/*----------------------------------------------------------------------------*/

#define OV_MC_INTERCONNECT_DEFAULT_CODEC "opus/48000/2"
#define OV_MC_INTERCONNECT_INTERFACE_NAME_MAX 255
#define OV_MC_INTERCONNECT_PASSWORD_MAX 255

/*----------------------------------------------------------------------------*/

typedef struct ov_mc_interconnect ov_mc_interconnect;

/*----------------------------------------------------------------------------*/

#include "ov_mc_interconnect_loop.h"
#include "ov_mc_interconnect_msg.h"
#include "ov_mc_interconnect_session.h"

/*----------------------------------------------------------------------------*/

typedef struct ov_mc_interconnect_config {

    ov_event_loop *loop;

    char name[OV_MC_INTERCONNECT_INTERFACE_NAME_MAX];
    char password[OV_MC_INTERCONNECT_PASSWORD_MAX];

    struct {

        bool client;
        ov_socket_configuration signaling;
        ov_socket_configuration media;
        ov_socket_configuration internal;
        ov_socket_configuration mixer;

    } socket;

    struct {

        char domains[PATH_MAX]; // domains to configure in listener

        struct {

            char domain[PATH_MAX]; // hostname to use in handshake
            struct {

                char file[PATH_MAX]; // path to CA verify file
                char path[PATH_MAX]; // path to CAs to use

            } ca;

        } client;

    } tls;

    struct {

        uint8_t threads;
        uint64_t client_connect_trigger_usec;
        uint64_t keepalive_trigger_usec;

    } limits;

    ov_dtls_config dtls;

    ov_mixer_config mixer;

} ov_mc_interconnect_config;

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_mc_interconnect *ov_mc_interconnect_create(ov_mc_interconnect_config config);

/*----------------------------------------------------------------------------*/

ov_mc_interconnect *ov_mc_interconnect_free(ov_mc_interconnect *self);
ov_mc_interconnect *ov_mc_interconnect_cast(const void *data);

/*----------------------------------------------------------------------------*/

ov_mc_interconnect_config
ov_mc_interconnect_config_from_json(const ov_json_value *val);

/*----------------------------------------------------------------------------*/

bool ov_mc_interconnect_load_loops(ov_mc_interconnect *self,
                                   const ov_json_value *val);

/*----------------------------------------------------------------------------*/

bool ov_mc_interconnect_loop_io(ov_mc_interconnect *self,
                                     ov_mc_interconnect_loop *loop,
                                     uint8_t *buffer, size_t bytes);

/*
 *      ------------------------------------------------------------------------
 *
 *      SESSION FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

bool ov_mc_interconnect_register_session(ov_mc_interconnect *self, ov_id id,
                                         ov_socket_data signaling,
                                         ov_socket_data media,
                                         ov_mc_interconnect_session *session);

/*----------------------------------------------------------------------------*/

bool ov_mc_interconnect_unregister_session(ov_mc_interconnect *self,
                                           ov_socket_data signaling,
                                           ov_socket_data media);

/*----------------------------------------------------------------------------*/

bool ov_mc_interconnect_srtp_ready(ov_mc_interconnect *self,
                                   ov_mc_interconnect_session *session);

/*----------------------------------------------------------------------------*/

int ov_mc_interconnect_get_media_socket(ov_mc_interconnect *self);

/*----------------------------------------------------------------------------*/

ov_mc_interconnect_loop *ov_mc_interconnect_get_loop(ov_mc_interconnect *self,
                                                     const char *loop_name);

/*----------------------------------------------------------------------------*/

ov_dtls *ov_mc_interconnect_get_dtls(ov_mc_interconnect *self);

/*----------------------------------------------------------------------------*/

ov_mixer_data ov_mc_interconnect_assign_mixer(ov_mc_interconnect *self,
    const char *name);

/*----------------------------------------------------------------------------*/

bool ov_mc_interconnect_send_aquire_mixer(ov_mc_interconnect *self,
    ov_mixer_data data,
    ov_mixer_forward forward);

#endif /* ov_mc_interconnect_h */
