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
        @file           ov_mc_backend_sip.h
        @author         Markus Töpfer

        @date           2023-03-24


        ------------------------------------------------------------------------
*/
#ifndef ov_mc_backend_sip_h
#define ov_mc_backend_sip_h

#include "ov_mc_backend.h"
#include <ov_base/ov_event_loop.h>
#include <ov_base/ov_socket.h>
#include <ov_core/ov_io.h>
#include <ov_sip/ov_sip_permission.h>
#include <ov_vocs_db/ov_vocs_db.h>

#define OV_MC_BACKEND_SIP_DEFAULT_TIMEOUT 10000000 // 10 sec

/*----------------------------------------------------------------------------*/

typedef struct ov_mc_backend_sip ov_mc_backend_sip;

/*----------------------------------------------------------------------------*/

typedef struct ov_mc_backend_sip_config {

  ov_event_loop *loop;
  ov_vocs_db *db;
  ov_io *io;
  ov_mc_backend *backend;

  struct {

    ov_socket_configuration manager; // manager liege socket

  } socket;

  struct {

    uint64_t response_usec;

  } timeout;

  struct {

    void *userdata;

    struct {

      void (*init)(void *userdata, const char *uuid, const char *loopname,
                   const char *call_id, const char *caller, const char *callee,
                   uint8_t error_code, const char *error_desc);

      void (*new)(void *userdata, const char *loopname, const char *call_id,
                  const char *peer);

      void (*terminated)(void *userdata, const char *call_id,
                         const char *loopname);

      void (*permit)(void *userdata, const ov_sip_permission permission,
                     uint64_t error_code, const char *error_desc);

      void (*revoke)(void *userdata, const ov_sip_permission permission,
                     uint64_t error_code, const char *error_desc);

    } call;

    void (*list_calls)(void *userdata, const char *uuid,
                       const ov_json_value *calls, uint64_t error_code,
                       const char *error_desc);

    void (*list_permissions)(void *userdata, const char *uuid,
                             const ov_json_value *permissions,
                             uint64_t error_code, const char *error_desc);

    void (*get_status)(void *userdata, const char *uuid,
                       const ov_json_value *status, uint64_t error_code,
                       const char *error_desc);

    void (*connected)(void *userdata, bool status);

  } callback;

} ov_mc_backend_sip_config;

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_mc_backend_sip *ov_mc_backend_sip_create(ov_mc_backend_sip_config config);
ov_mc_backend_sip *ov_mc_backend_sip_free(ov_mc_backend_sip *self);
ov_mc_backend_sip *ov_mc_backend_sip_cast(const void *self);

/*----------------------------------------------------------------------------*/

ov_mc_backend_sip_config
ov_mc_backend_sip_config_from_json(const ov_json_value *val);

/*----------------------------------------------------------------------------*/

char *ov_mc_backend_sip_create_call(ov_mc_backend_sip *self, const char *loop,
                                    const char *destination_number,
                                    const char *from_number);

/*----------------------------------------------------------------------------*/

bool ov_mc_backend_sip_terminate_call(ov_mc_backend_sip *self,
                                      const char *call_id);

/*----------------------------------------------------------------------------*/

bool ov_mc_backend_sip_create_permission(ov_mc_backend_sip *self,
                                         ov_sip_permission permission);

/*----------------------------------------------------------------------------*/

bool ov_mc_backend_sip_terminate_permission(ov_mc_backend_sip *self,
                                            ov_sip_permission permission);

/*----------------------------------------------------------------------------*/

bool ov_mc_backend_sip_list_calls(ov_mc_backend_sip *self, const char *uuid);

/*----------------------------------------------------------------------------*/

bool ov_mc_backend_sip_list_permissions(ov_mc_backend_sip *self,
                                        const char *uuid);

/*----------------------------------------------------------------------------*/

bool ov_mc_backend_sip_get_status(ov_mc_backend_sip *self, const char *uuid);

/*----------------------------------------------------------------------------*/

bool ov_mc_backend_sip_get_connect_status(ov_mc_backend_sip *self);

/*----------------------------------------------------------------------------*/

bool ov_mc_backend_sip_configure(ov_mc_backend_sip *self);

#endif /* ov_mc_backend_sip_h */
