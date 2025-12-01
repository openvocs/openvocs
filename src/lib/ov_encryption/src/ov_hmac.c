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
        @file           ov_hmac.c
        @author         Markus Toepfer

        @date           2019-03-08

        @ingroup        ov_hmac

        @brief          Implementation of


        ------------------------------------------------------------------------
*/
#include "../include/ov_hmac.h"

#include <openssl/hmac.h>
#include <string.h>

/*----------------------------------------------------------------------------*/

bool ov_hmac(ov_hash_function type, const uint8_t *buffer, size_t size,
             const void *key, size_t key_len, uint8_t *result,
             size_t *result_len) {

  if (!buffer || !key || !result || !result_len)
    goto error;

  const EVP_MD *hash_func = ov_hash_function_to_EVP(type);
  if (!hash_func)
    goto error;

  if (!HMAC(hash_func, key, key_len, buffer, size, result,
            (unsigned int *)result_len))
    goto error;

  return true;

error:
  return false;
}
