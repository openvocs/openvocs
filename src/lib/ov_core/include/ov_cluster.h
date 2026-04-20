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
        @file           ov_cluster.h
        @author         Töpfer, Markus

        @date           2026-03-11


        ------------------------------------------------------------------------
*/
#ifndef ov_cluster_h
#define ov_cluster_h

#include <ov_base/ov_socket.h>
#include <ov_base/ov_event_loop.h>
#include <ov_base/ov_json.h>

typedef struct ov_cluster ov_cluster;

typedef struct ov_cluster_config {

    ov_event_loop *loop;
    ov_socket_configuration multicast;

    struct {

        void *userdata;
        void (*io)(void *userdata, ov_json_value *msg);

    } callback;

} ov_cluster_config;

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_cluster *ov_cluster_create(ov_cluster_config config);
ov_cluster *ov_cluster_free(ov_cluster *self);
ov_cluster *ov_cluster_cast(const void *self);

/*----------------------------------------------------------------------------*/

bool ov_cluster_send(ov_cluster *self, const ov_json_value *msg);

#endif /* ov_cluster_h */
