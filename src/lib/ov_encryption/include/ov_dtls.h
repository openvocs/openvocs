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
        @file           ov_dtls.h
        @author         Markus

        @date           2023-12-13


        ------------------------------------------------------------------------
*/
#ifndef ov_dtls_h
#define ov_dtls_h

#include "ov_hash.h"
#include <limits.h>
#include <ov_base/ov_event_loop.h>
#include <ov_base/ov_json.h>

#define OV_DTLS_SRTP_PROFILES                                                  \
    "SRTP_AES128_CM_SHA1_80:"                                                  \
    "SRTP_AES128_CM_SHA1_32"

#define OV_DTLS_PROFILE_MAX 1024

#define OV_DTLS_KEY_MAX 48  // min 32 key + 14 salt
#define OV_DTLS_SALT_MAX 14 // 112 bit salt

#define OV_DTLS_SSL_BUFFER_SIZE 16000 // max SSL buffer size
#define OV_DTLS_SSL_ERROR_STRING_SIZE 200

// Default DTLS keys and key length
#define OV_DTLS_FINGERPRINT_MAX 3 * OV_SHA512_SIZE + 10 // SPname/0 HEX

/*----------------------------------------------------------------------------*/

typedef struct ov_dtls ov_dtls;

/*----------------------------------------------------------------------------*/

typedef enum {

    OV_DTLS_ACTIVE = 1,
    OV_DTLS_PASSIVE = 2

} ov_dtls_type;

/*----------------------------------------------------------------------------*/

typedef struct {

    ov_event_loop *loop;

    /*
     *      Certificate configuration for SSL,
     *      including pathes to certificate, key and verification chains.
     */

    char cert[PATH_MAX];
    char key[PATH_MAX];

    struct {

        char file[PATH_MAX]; // path to CA verify file
        char path[PATH_MAX]; // path to CAs to use

    } ca;

    /*
     *      SRTP specific configuration
     */

    struct {

        char profile[OV_DTLS_PROFILE_MAX];

    } srtp;

    /*
     *      DTLS specific configuration
     */

    struct {

        struct {

            size_t quantity; // amount of DTLS keys used for cookies
            size_t length;   // min length of DTLS cookie

            uint64_t lifetime_usec; // lifetime of keys in usecs

        } keys;

    } dtls;

    uint64_t reconnect_interval_usec; // handshaking interval

} ov_dtls_config;

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_dtls_config ov_dtls_config_from_json(const ov_json_value *input);

ov_dtls *ov_dtls_create(ov_dtls_config config);

ov_dtls *ov_dtls_free(ov_dtls *self);

/*----------------------------------------------------------------------------*/

const char *ov_dtls_get_fingerprint(const ov_dtls *self);

/*----------------------------------------------------------------------------*/

const char *ov_dtls_type_to_string(ov_dtls_type type);

/*----------------------------------------------------------------------------*/

SSL_CTX *ov_dtls_get_ctx(ov_dtls *self);

/*----------------------------------------------------------------------------*/

const char *ov_dtls_get_srtp_profile(ov_dtls *self);

/*----------------------------------------------------------------------------*/

const char *ov_dtls_get_verify_file(ov_dtls *self);

/*----------------------------------------------------------------------------*/

const char *ov_dtls_get_verify_path(ov_dtls *self);

#endif /* ov_dtls_h */
