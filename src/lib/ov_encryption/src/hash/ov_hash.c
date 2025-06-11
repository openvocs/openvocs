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
        @file           ov_hash.c
        @author         Markus Toepfer

        @date           2019-05-01

        @ingroup        ov_hash

        @brief          Implementation of


        ------------------------------------------------------------------------
*/
#include "ov_hash.h"

#include <string.h>
#include <strings.h>

/*----------------------------------------------------------------------------*/

const char *ov_hash_function_to_string(ov_hash_function func) {

    switch (func) {

        case OV_HASH_SHA1:
            return "sha1";
            break;
        case OV_HASH_SHA256:
            return "sha256";
            break;
        case OV_HASH_SHA512:
            return "sha512";
            break;
        case OV_HASH_MD5:
            return "md5";
            break;
        case OV_HASH_UNSPEC:
            return "unspec";
            break;
    }

    return NULL;
}

/*----------------------------------------------------------------------------*/

const char *ov_hash_function_to_RFC8122_string(ov_hash_function func) {

    /*
                    "sha-1" / "sha-224" / "sha-256" /
                    "sha-384" / "sha-512" /
                    "md5" / "md2" / token

    */
    switch (func) {

        case OV_HASH_SHA1:
            return "sha-1";
            break;
        case OV_HASH_SHA256:
            return "sha-256";
            break;
        case OV_HASH_SHA512:
            return "sha-512";
            break;
        case OV_HASH_MD5:
            return "md5";
            break;

        default:
            break;
    }

    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_hash_function ov_hash_function_from_string(const char *string,
                                              size_t length) {

    if (!string) goto error;

    switch (length) {

        case 3:

            if (0 == strncasecmp(string, "md5", length)) return OV_HASH_MD5;

            break;
        case 4:

            if (0 == strncasecmp(string, "sha1", length)) return OV_HASH_SHA1;

            break;

        case 5:
            if (0 == strncasecmp(string, "sha-1", length)) return OV_HASH_SHA1;

            break;

        case 6:

            if (0 == strncasecmp(string, "sha256", length))
                return OV_HASH_SHA256;

            if (0 == strncasecmp(string, "sha512", length))
                return OV_HASH_SHA512;

            break;

        case 7:

            if (0 == strncasecmp(string, "sha-256", length))
                return OV_HASH_SHA256;

            if (0 == strncasecmp(string, "sha-512", length))
                return OV_HASH_SHA512;

        default:
            break;
    }

error:
    return OV_HASH_UNSPEC;
}

/*----------------------------------------------------------------------------*/

const EVP_MD *ov_hash_function_to_EVP(ov_hash_function type) {

    switch (type) {

        case OV_HASH_SHA1:
            return EVP_sha1();
            break;
        case OV_HASH_SHA256:
            return EVP_sha256();
            break;
        case OV_HASH_SHA512:
            return EVP_sha512();
            break;
        case OV_HASH_MD5:
            return EVP_md5();
            break;
        default:
            break;
    }

    return NULL;
}

/*----------------------------------------------------------------------------*/

bool ov_hash(ov_hash_function func,
             const char *array[],
             size_t array_items,
             uint8_t **result,
             size_t *result_length) {

    EVP_MD_CTX *ctx = NULL;
    bool created = false;

    if (!array || array_items < 1 || !result || !result_length) return false;

    unsigned char md_value[EVP_MAX_MD_SIZE];
    unsigned int md_len = 0;
    memset(md_value, 0, EVP_MAX_MD_SIZE);

    const EVP_MD *name = ov_hash_function_to_EVP(func);
    if (!name) goto error;

    ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, name, NULL);
    for (size_t i = 0; i < array_items; i++) {
        if (0 == array[i]) goto error;
        EVP_DigestUpdate(ctx, array[i], strlen(array[i]));
    }
    EVP_DigestFinal_ex(ctx, md_value, &md_len);
    EVP_MD_CTX_free(ctx);
    ctx = NULL;

    if (!*result) {

        *result = calloc(md_len + 1, sizeof(char));
        if (!*result) goto error;
        created = true;

        *result_length = md_len;
    }

    if (*result_length < md_len) goto error;
    if (!memcpy(*result, md_value, md_len)) goto error;
    *result_length = (size_t)md_len;
    return true;

error:
    if (ctx) EVP_MD_CTX_free(ctx);
    if (created) {
        free(*result);
        *result = NULL;
    }
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_hash_string(ov_hash_function func,
                    const uint8_t *source,
                    size_t source_length,
                    uint8_t **result,
                    size_t *result_length) {

    if (!source || source_length < 1 || !result || !result_length) return false;

    char string[source_length + 1];
    memset(string, 0, source_length + 1);
    if (!memcpy(string, source, source_length)) return false;

    const char *array[1] = {0};
    array[0] = string;

    return ov_hash(func, array, 1, result, result_length);
}