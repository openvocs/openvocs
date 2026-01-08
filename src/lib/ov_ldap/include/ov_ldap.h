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
        @file           ov_ldap.h
        @author         Markus Töpfer

        @date           2022-03-06


        ------------------------------------------------------------------------
*/
#ifndef ov_ldap_h
#define ov_ldap_h

#include <inttypes.h>

#include <ov_base/ov_thread_loop.h>

/*---------------------------------------------------------------------------*/

#define OV_KEY_LDAP "ldap"
#define OV_KEY_USER_DN_TREE "user_dn_tree"

/*---------------------------------------------------------------------------*/

#define OV_LDAP_MAGIC_BYTE 0x7da6
#define OV_LDAP_DEFAULT_NETWORK_TIMEOUT_USEC 3000 * 1000
#define OV_LDAP_DEFAULT_THREAD_QUEUE_CAPACITY 1000
#define OV_LDAP_DEFAULT_THREAD_LOCK_TIMEOUT_USEC 1000 * 1000
#define OV_LDAP_DEFAULT_THREADS 4

#define OV_LDAP_USER_DN_TREE 1000

/*---------------------------------------------------------------------------*/

typedef struct ov_ldap ov_ldap;

/*----------------------------------------------------------------------------*/

typedef enum ov_ldap_auth_result {

    OV_LDAP_AUTH_REJECTED = 0,
    OV_LDAP_AUTH_GRANTED = 1

} ov_ldap_auth_result;

/*----------------------------------------------------------------------------*/

typedef struct ov_ldap_auth_callback {

    void *userdata;
    void (*callback)(void *userdata, const char *uuid,
                     ov_ldap_auth_result result);

} ov_ldap_auth_callback;

/*---------------------------------------------------------------------------*/

typedef struct ov_ldap_config {

    ov_event_loop *loop;

    char host[OV_HOST_NAME_MAX];
    char user_dn_tree[OV_LDAP_USER_DN_TREE];

    struct {

        uint64_t network_timeout_usec;

    } timeout;

    ov_thread_loop_config threads;

} ov_ldap_config;

/*---------------------------------------------------------------------------*/

struct ov_ldap {

    uint16_t magic_byte;
    uint16_t type;

    ov_ldap_config config;

    /* ---------------------------------------------------------------------- */

    ov_ldap *(*free)(ov_ldap *self);

    /* ---------------------------------------------------------------------- */

    bool (*reconfigure)(ov_ldap *self, ov_ldap_config config);

    /* ---------------------------------------------------------------------- */

    struct {

        bool (*password)(ov_ldap *self, const char *user, const char *password,
                         const char *uuid, ov_ldap_auth_callback callback);

    } authenticate;
};

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_ldap *ov_ldap_create(ov_ldap_config config);
ov_ldap *ov_ldap_free(ov_ldap *self);
ov_ldap *ov_ldap_cast(const void *self);

/*----------------------------------------------------------------------------*/

bool ov_ldap_reconfigure(ov_ldap *self, ov_ldap_config config);

/*----------------------------------------------------------------------------*/

bool ov_ldap_authenticate_password(ov_ldap *self, const char *user,
                                   const char *password, const char *uuid,
                                   ov_ldap_auth_callback callback);

/*----------------------------------------------------------------------------*/

ov_ldap_config ov_ldap_config_from_json(const ov_json_value *val);

#endif /* ov_ldap_h */
