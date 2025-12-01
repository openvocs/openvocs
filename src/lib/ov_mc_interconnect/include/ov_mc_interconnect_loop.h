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
        @file           ov_mc_interconnect_loop.h
        @author         Markus Töpfer

        @date           2023-12-11


        ------------------------------------------------------------------------
*/
#ifndef ov_mc_interconnect_loop_h
#define ov_mc_interconnect_loop_h

#include <ov_base/ov_event_loop.h>
#include <ov_base/ov_socket.h>

/*----------------------------------------------------------------------------*/

typedef struct ov_mc_interconnect_loop ov_mc_interconnect_loop;

/*----------------------------------------------------------------------------*/

#include "ov_mc_interconnect.h"

/*----------------------------------------------------------------------------*/

typedef struct ov_mc_interconnect_loop_config {

  ov_event_loop *loop;
  ov_mc_interconnect *base;
  char name[OV_HOST_NAME_MAX];
  ov_socket_configuration socket;

} ov_mc_interconnect_loop_config;

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_mc_interconnect_loop *
ov_mc_interconnect_loop_create(ov_mc_interconnect_loop_config config);

/*----------------------------------------------------------------------------*/

ov_mc_interconnect_loop *
ov_mc_interconnect_loop_free(ov_mc_interconnect_loop *self);

void *ov_mc_interconnect_loop_free_void(void *self);

/*----------------------------------------------------------------------------*/

ov_mc_interconnect_loop *ov_mc_interconnect_loop_cast(const void *data);

/*----------------------------------------------------------------------------*/

uint32_t ov_mc_interconnect_loop_get_ssrc(const ov_mc_interconnect_loop *self);

/*----------------------------------------------------------------------------*/

const char *
ov_mc_interconnect_loop_get_name(const ov_mc_interconnect_loop *self);

/*----------------------------------------------------------------------------*/

bool ov_mc_interconnect_loop_send(ov_mc_interconnect_loop *self,
                                  uint8_t *buffer, size_t bytes);

#endif /* ov_mc_interconnect_loop_h */
