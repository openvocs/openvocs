/***
        ------------------------------------------------------------------------

        Copyright (c) 2022 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_mc_mixer_app.h
        @author         Markus Töpfer

        @date           2022-11-11

        openvocs Multicast Mixer application

        ------------------------------------------------------------------------
*/
#ifndef ov_mc_mixer_app_h
#define ov_mc_mixer_app_h

#include <ov_base/ov_event_loop.h>
#include <ov_base/ov_socket.h>

#include <ov_core/ov_io.h>

#include "ov_mc_mixer_core.h"
#include "ov_mc_mixer_msg.h"

/*----------------------------------------------------------------------------*/

typedef struct ov_mc_mixer_app ov_mc_mixer_app;

/*----------------------------------------------------------------------------*/

typedef struct ov_mc_mixer_app_config {

  ov_event_loop *loop;
  ov_io *io;
  ov_socket_configuration manager; // manager liege socket

  struct {

    uint64_t client_connect_sec;

  } limit;

} ov_mc_mixer_app_config;

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_mc_mixer_app *ov_mc_mixer_app_create(ov_mc_mixer_app_config config);
ov_mc_mixer_app *ov_mc_mixer_app_free(ov_mc_mixer_app *self);
ov_mc_mixer_app *ov_mc_mixer_app_cast(const void *self);

ov_mc_mixer_app_config ov_mc_mixer_app_config_from_json(const ov_json_value *c);

#endif /* ov_mc_mixer_app_h */
