/***
        ------------------------------------------------------------------------

        Copyright (c) 2021 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_password.h
        @author         Markus Töpfer

        @date           2021-02-20


        ------------------------------------------------------------------------
*/
#ifndef ov_password_h
#define ov_password_h

#include <inttypes.h>
#include <ov_base/ov_json_value.h>

#define OV_AUTH_KEY_WORKFACTOR "workfactor"
#define OV_AUTH_KEY_BLOCKSIZE "blocksize"
#define OV_AUTH_KEY_PARALLEL "parallel"
#define OV_AUTH_KEY_HASH "hash"
#define OV_AUTH_KEY_SALT "salt"

typedef struct ov_password_hash_parameter {

  uint16_t workfactor;
  uint16_t blocksize;
  uint16_t parallel;

} ov_password_hash_parameter;

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

/**
    Create some salted password hash and return a JSON representation

    @param password     password to hash
    @param params       (optional) parameter to be used for hash generation
    @param length       (optional) length of the hash to be created

    @returns some json value, which can be used as input to
    @see ov_password_is_valid
*/
ov_json_value *ov_password_hash(const char *password,
                                ov_password_hash_parameter params,
                                size_t length);

/*----------------------------------------------------------------------------*/

/**
    Validate some password against some salted representation. The json input
    identifies the hash algorithm to use over its representation and structure.

    ov_password hashing supports PDKDS2 as well as scrypt. If some
    blocksize OR parallel is defined within the json structure scrypt will be
    used. If non of them is present PDKDS2 will be used. For PDKDF2 iteration
    count may be specified over workfactor.

    Min valid representation (forces pdkdf2 validation)

    {
        "hash":"<base64 hash>",
        "salt":"<base64 salt>",
        "workfactor":1024           <- (optional) iteration count for PDKDF2
    }

    Full representation (forces scrypt validation)

    {
        "hash":"<base64 hash>",
        "salt":"<base64 salt>"
        "workfactor":1024,          <- (optional) workfactor
        "blocksize":4,              <- (optional) blocksize
        "parallel":2,               <- (optional) parallel

    }

    @param password     password to validate
    @param input        salted json representation

    @returns true if the password genereated the hash of the representation,
    using the parameters of the representation
*/
bool ov_password_is_valid(const char *password, const ov_json_value *input);

#endif /* ov_password_h */
