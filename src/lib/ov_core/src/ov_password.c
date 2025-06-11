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
        @file           ov_password.c
        @author         Markus Töpfer

        @date           2021-02-20


        ------------------------------------------------------------------------
*/
#include "../include/ov_password.h"

#include <ov_base/ov_base64.h>
#include <ov_base/ov_data_function.h>
#include <ov_base/ov_json.h>
#include <ov_base/ov_memory_pointer.h>

#include <openssl/evp.h>
#include <openssl/kdf.h>
#include <openssl/rand.h>

#include <unistd.h>

#define IMPL_WORKFACTOR_DEFAULT 1024
#define IMPL_BLOCKSIZE_DEFAULT 8
#define IMPL_PARALLEL_DEFAULT 16
#define IMPL_LENGTH_DEFAULT 64

bool ov_password_hash_scrypt(uint8_t *out,
                             size_t *len,
                             const char *pass,
                             const char *salt,
                             ov_password_hash_parameter params) {

    EVP_PKEY_CTX *pctx = NULL;

    if (!out || !len || !pass || !salt) goto error;

    /* defaults (use the one of openssl example */

    if (0 == params.workfactor) params.workfactor = IMPL_WORKFACTOR_DEFAULT;

    if (0 == params.blocksize) params.blocksize = IMPL_BLOCKSIZE_DEFAULT;

    if (0 == params.parallel) params.parallel = IMPL_PARALLEL_DEFAULT;

    pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_SCRYPT, NULL);

    if (1 != EVP_PKEY_derive_init(pctx)) goto error;

    if (1 != EVP_PKEY_CTX_set1_pbe_pass(pctx, pass, strlen(pass))) goto error;

    if (1 != EVP_PKEY_CTX_set1_scrypt_salt(pctx, (uint8_t *)salt, strlen(salt)))
        goto error;

    if (1 != EVP_PKEY_CTX_set_scrypt_N(pctx, params.workfactor)) goto error;

    if (1 != EVP_PKEY_CTX_set_scrypt_r(pctx, params.blocksize)) goto error;

    if (1 != EVP_PKEY_CTX_set_scrypt_p(pctx, params.parallel)) goto error;

    if (1 != EVP_PKEY_derive(pctx, out, len)) goto error;

    EVP_PKEY_CTX_free(pctx);
    return out;
error:
    if (pctx) EVP_PKEY_CTX_free(pctx);
    return NULL;
}

/*----------------------------------------------------------------------------*/

bool ov_password_hash_pdkdf2(uint8_t *out,
                             size_t *len,
                             const char *pass,
                             const char *salt,
                             ov_password_hash_parameter params) {

    if (!out || !len || !pass || !salt) goto error;

    int length = *len;

    if (0 == params.workfactor) params.workfactor = IMPL_WORKFACTOR_DEFAULT;

    if (1 != PKCS5_PBKDF2_HMAC(pass,
                               strlen(pass),
                               (uint8_t *)salt,
                               strlen(salt),
                               (int)params.workfactor,
                               EVP_sha3_512(),
                               length,
                               out))
        goto error;

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

ov_password_hash_parameter ov_password_params_from_json(
    const ov_json_value *input) {

    ov_password_hash_parameter params = {0};

    if (!input) goto error;

    params.workfactor = (uint16_t)ov_json_number_get(
        ov_json_object_get(input, OV_AUTH_KEY_WORKFACTOR));

    params.blocksize = (uint16_t)ov_json_number_get(
        ov_json_object_get(input, OV_AUTH_KEY_BLOCKSIZE));

    params.parallel = (uint16_t)ov_json_number_get(
        ov_json_object_get(input, OV_AUTH_KEY_PARALLEL));

    return params;

error:
    return (ov_password_hash_parameter){0};
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_password_params_to_json(ov_password_hash_parameter params) {

    ov_json_value *out = ov_json_object();
    ov_json_value *val = NULL;

    val = ov_json_number(params.workfactor);
    if (!ov_json_object_set(out, OV_AUTH_KEY_WORKFACTOR, val)) goto error;

    val = ov_json_number(params.blocksize);
    if (!ov_json_object_set(out, OV_AUTH_KEY_BLOCKSIZE, val)) goto error;

    val = ov_json_number(params.parallel);
    if (!ov_json_object_set(out, OV_AUTH_KEY_PARALLEL, val)) goto error;

    return out;

error:
    ov_json_value_free(val);
    ov_json_value_free(out);
    return NULL;
}

/*----------------------------------------------------------------------------*/

bool ov_password_is_valid(const char *password, const ov_json_value *input) {

    uint8_t *hash_byte_string = NULL;
    size_t hash_byte_length = 0;

    uint8_t *salt_byte_string = NULL;
    size_t salt_byte_length = 0;

    if (!password || !input) return false;

    /* parse the expected hash and salt to be used */
    const char *hash =
        ov_json_string_get(ov_json_object_get(input, OV_AUTH_KEY_HASH));
    const char *salt =
        ov_json_string_get(ov_json_object_get(input, OV_AUTH_KEY_SALT));
    if (!hash || !salt) return false;

    /* parse the params to be used */
    ov_password_hash_parameter params = ov_password_params_from_json(input);

    size_t hash_length = strlen(hash);
    size_t salt_length = strlen(salt);

    uint8_t hash_buffer[hash_length];
    memset(hash_buffer, 0, hash_length);

    /* NOTE we cannot use goto error up to here,
     * as we use dynamic length stack buffers
     *
     * The hash functions returns some byte string,
     * which cannot be transported over JSON due to the JSON character encoding,
     * but some base64 encoded bytestring may be transported over JSON
     * as base64 is basically ASCII,
     *
     * So we will use the convention HASH and SALT MUST be base64 encoded for
     * JSON storage
     *
     * NOTE base64 decoding will add some processing delay due to the decoding
     * process.
     */

    if (!ov_base64_decode(
            (uint8_t *)hash, hash_length, &hash_byte_string, &hash_byte_length))
        goto error;

    if (!ov_base64_decode(
            (uint8_t *)salt, salt_length, &salt_byte_string, &salt_byte_length))
        goto error;

    size_t length = hash_length;

    /* NOTE we may use some explicit or implicit definition of the
     * hash function to use.
     *
     * Implementation with implicit definition by convention:
     *
     *  - if some blocksize of some parallel is used within
     *    the parameter defintion of the JSON value, we use scrypt
     *    otherwise we use pdkf2
     *
     *  example:
     *
     *  Config input 1 (min valid) -> will use pkdf2 hashing
     *
     *  {
     *      "hash" : "<some base64 encoded byte string>",
     *      "salt" : "<some base64 encoded byte string>"
     *  }
     *
     *  Config input 2 -> will use pkdf2 hashing with 1024 iterations
     *
     *  {
     *      "hash" : "<some base64 encoded byte string>",
     *      "salt" : "<some base64 encoded byte string>",
     *      "workfactor" : 1024
     *  }
     *
     *  Config input 3 (min scrypt) -> will use script hashing with defaults
     *
     *  {
     *      "hash" : "<some base64 encoded byte string>",
     *      "salt" : "<some base64 encoded byte string>",
     *      "parallel" : 1
     *  }
     *
     *  Config input 4 (scrypt) -> will use script hashing with inputs
     *
     *  {
     *      "hash" : "<some base64 encoded byte string>",
     *      "salt" : "<some base64 encoded byte string>",
     *      "workfactor" : 1024,
     *      "blocksize" : 8
     *      "parallel" : 16
     *  }
     */

    if ((0 != params.blocksize) || (0 != params.parallel)) {

        if (!ov_password_hash_scrypt(hash_buffer,
                                     &length,
                                     password,
                                     (char *)salt_byte_string,
                                     params))
            goto error;

    } else if (!ov_password_hash_pdkdf2(hash_buffer,
                                        &length,
                                        password,
                                        (char *)salt_byte_string,
                                        params)) {

        goto error;
    }

    /* We now have some byte stream encoded result in hash_buffer, as well as
     * the byte stream encoded expected hash in hash_expect */

    if (0 != memcmp(hash_byte_string, hash_buffer, hash_byte_length))
        goto error;

    hash_byte_string = ov_data_pointer_free(hash_byte_string);
    salt_byte_string = ov_data_pointer_free(salt_byte_string);
    return true;
error:
    hash_byte_string = ov_data_pointer_free(hash_byte_string);
    salt_byte_string = ov_data_pointer_free(salt_byte_string);
    return false;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_password_hash(const char *password,
                                ov_password_hash_parameter params,
                                size_t length) {

    if (0 == length) length = IMPL_LENGTH_DEFAULT;

    size_t len = length + 1;

    /* We follow the recommendation to use the same length of
     * salt and hash */

    uint8_t hash[len];
    memset(hash, 0, len);

    uint8_t salt[len];
    memset(salt, 0, len);

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;

    if (!password) goto error;

    /* Create salt (using openssl CSPRNG)*/

    if (1 != RAND_bytes(salt, length)) goto error;

    /* Create hash */

    if ((0 != params.blocksize) || (0 != params.parallel)) {

        if (!ov_password_hash_scrypt(
                hash, &length, password, (char *)salt, params))
            goto error;

    } else if (!ov_password_hash_pdkdf2(
                   hash, &length, password, (char *)salt, params)) {

        goto error;
    }

    /* Create params and remove unrequired entries */

    out = ov_password_params_to_json(params);

    if (0 == params.workfactor) ov_json_object_del(out, OV_AUTH_KEY_WORKFACTOR);

    if (0 == params.blocksize) ov_json_object_del(out, OV_AUTH_KEY_BLOCKSIZE);

    if (0 == params.parallel) ov_json_object_del(out, OV_AUTH_KEY_PARALLEL);

    /* add base64 salt and hash */

    uint8_t *b64 = NULL;
    size_t b64_len = 0;

    if (!ov_base64_encode(hash, length, &b64, &b64_len)) goto error;

    val = ov_json_string((char *)b64);
    b64 = ov_data_pointer_free(b64);
    b64_len = 0;

    if (!ov_json_object_set(out, OV_AUTH_KEY_HASH, val)) goto error;

    if (!ov_base64_encode(salt, length, &b64, &b64_len)) goto error;

    val = ov_json_string((char *)b64);
    b64 = ov_data_pointer_free(b64);
    b64_len = 0;

    if (!ov_json_object_set(out, OV_AUTH_KEY_SALT, val)) goto error;

    return out;

error:
    ov_json_value_free(out);
    ov_json_value_free(val);
    return NULL;
}
