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
        @file           ov_webauthn.h
        @author         Töpfer, Markus

        @date           2026-09-14


        ------------------------------------------------------------------------
*/
#ifndef ov_webauthn_h
#define ov_webauthn_h

#include <ov_base/ov_json.h>
#include <ov_base/ov_event_loop.h>
#include <ov_vocs_db/ov_vocs_db.h>
#include <limits.h>

/*----------------------------------------------------------------------------*/

typedef struct ov_webauthn ov_webauthn;

/*----------------------------------------------------------------------------*/

typedef struct ov_webauthn_config {

    ov_event_loop *loop;
    ov_vocs_db *db;

} ov_webauthn_config;

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_webauthn *ov_webauthn_create(ov_webauthn_config config);
ov_webauthn *ov_webauthn_free(ov_webauthn *self);
ov_webauthn *ov_webauthn_cast(const void *data);

/*----------------------------------------------------------------------------*/

ov_webauthn_config ov_webauthn_config_from_json(const ov_json_value *input);

/*----------------------------------------------------------------------------*/

ov_json_value *ov_webauthn_create_registration_challenge(ov_webauthn *self, 
    const char *user, const char *domain);

/*----------------------------------------------------------------------------*/

bool ov_webauthn_process_registration_challenge(ov_webauthn *self, 
    const ov_json_value *data);

/*----------------------------------------------------------------------------*/

ov_json_value *ov_webauthn_create_login_challenge(ov_webauthn *self, 
    const char *user, const char *domain);

/*----------------------------------------------------------------------------*/

bool ov_webauthn_process_login_challenge(ov_webauthn *self, 
    const ov_json_value *data);

#endif /* ov_webauthn_h */
