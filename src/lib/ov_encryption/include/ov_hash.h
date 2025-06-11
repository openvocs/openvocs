/***
        ------------------------------------------------------------------------

        Copyright (c) 2019 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_hash.h
        @author         Markus Toepfer

        @date           2019-05-01

        @ingroup        ov_hash

        @brief          Definition of


        ------------------------------------------------------------------------
*/
#ifndef ov_hash_h
#define ov_hash_h

#include <inttypes.h>
#include <openssl/evp.h>
#include <openssl/md5.h>
#include <openssl/sha.h>
#include <stdbool.h>

typedef enum {

    OV_HASH_UNSPEC = 0,
    OV_HASH_SHA1,
    OV_HASH_SHA256,
    OV_HASH_SHA512,
    OV_HASH_MD5

} ov_hash_function;

#define OV_SHA1_SIZE SHA_DIGEST_LENGTH
#define OV_SHA256_SIZE SHA256_DIGEST_LENGTH
#define OV_SHA512_SIZE SHA512_DIGEST_LENGTH
#define OV_MD5_SIZE MD5_DIGEST_LENGTH

/*----------------------------------------------------------------------------*/

/**
        Return the string represenation of the function. (lowercase)
*/
const char *ov_hash_function_to_string(ov_hash_function func);

/*----------------------------------------------------------------------------*/

/**
        Return the string represenation of the function. (lowercase)
*/
const char *ov_hash_function_to_RFC8122_string(ov_hash_function func);

/*----------------------------------------------------------------------------*/

/**
        Parse the hash function from a string (caseignore)
*/
ov_hash_function ov_hash_function_from_string(const char *string,
                                              size_t length);

/*----------------------------------------------------------------------------*/

/**
        Return the SSL specific type for the abstraction.
*/
const EVP_MD *ov_hash_function_to_EVP(ov_hash_function type);

/*----------------------------------------------------------------------------*/

/**
        HASH some array of strings using the provided hash function.

        This function operated in FILL / CREATE mode. If *result points to a
        buffer this buffer will be filled up to *length. If *length is not big
        enough for the used hash function ov_hash will fail.

        If *result is NULL it will be allocated and *length will contain
        the allocated size.

        @param func     MUST be a known (enabled) ov_hash_function
        @param array    pointer to array of /0 terminated strings
        @param items    items (elements) of the array
        @param result   pointer to result buffer
        @param length   length of result buffer (will be set to used length)
*/
bool ov_hash(ov_hash_function func,
             const char *array[],
             size_t items,
             uint8_t **result,
             size_t *length);

/*----------------------------------------------------------------------------*/

/**
        HASH a string with some HASH function. FILL / CREATE mode @see ov_hash

        This function will use up to src_len bytes
        of the buffer provided at source

        @param func     MUST be a known (enabled) ov_hash_function
        @param source   pointer to source buffer
        @param src_len  size of the source buffer to use
        @param result   pointer to reusult buffer
        @param length   length of result buffer (will be set to used length)

*/
bool ov_hash_string(ov_hash_function func,
                    const uint8_t *source,
                    size_t src_len,
                    uint8_t **result,
                    size_t *length);

#endif /* ov_hash_h */
