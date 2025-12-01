/***
        ------------------------------------------------------------------------

        Copyright (c) 2018 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_stun_binding.c
        @author         Markus Toepfer

        @date           2018-10-23

        @ingroup        ov_stun

        @brief          Implementation of RFC 5389 method "binding"


        ------------------------------------------------------------------------
*/
#include "../include/ov_stun_binding.h"

/*      ------------------------------------------------------------------------
 *
 *                              METHOD
 *
 *      ------------------------------------------------------------------------
 */

/*----------------------------------------------------------------------------*/

bool ov_stun_binding_class_support(const uint8_t *frame, size_t length) {

  if (!frame || length < 20)
    goto error;

  if (ov_stun_frame_class_is_request(frame, length) ||
      ov_stun_frame_class_is_indication(frame, length) ||
      ov_stun_frame_class_is_success_response(frame, length) ||
      ov_stun_frame_class_is_error_response(frame, length))
    return true;

error:
  return false;
}

/*----------------------------------------------------------------------------*/

bool ov_stun_method_is_binding(const uint8_t *frame, size_t length) {

  if (STUN_BINDING == ov_stun_frame_get_method(frame, length))
    return true;

  return false;
}

/*      ------------------------------------------------------------------------
 *
 *                              REQUEST CREATION
 *
 *      ------------------------------------------------------------------------
 */

bool ov_stun_generate_binding_request_plain(uint8_t *start, size_t length,
                                            uint8_t **next,
                                            const uint8_t *transaction_id,
                                            const uint8_t *software,
                                            size_t software_length,
                                            bool fingerprint) {

  return ov_stun_generate_binding_request(
      start, length, next, transaction_id, software, software_length, NULL, 0,
      NULL, 0, NULL, 0, NULL, 0, false, fingerprint);
}

/*----------------------------------------------------------------------------*/

bool ov_stun_generate_binding_request_short_term(
    uint8_t *start, size_t length, uint8_t **next,
    const uint8_t *transaction_id, const uint8_t *software,
    size_t software_length, const uint8_t *username, size_t username_length,
    const uint8_t *key, size_t key_length, bool fingerprint) {

  if (!ov_stun_username_validate(username, username_length) || !key ||
      key_length < 1)
    return false;

  return ov_stun_generate_binding_request(
      start, length, next, transaction_id, software, software_length, username,
      username_length, NULL, 0, NULL, 0, key, key_length, true, fingerprint);
}

/*----------------------------------------------------------------------------*/

bool ov_stun_generate_binding_request_long_term(
    uint8_t *start, size_t length, uint8_t **next,
    const uint8_t *transaction_id, const uint8_t *software,
    size_t software_length, const uint8_t *username, size_t username_length,
    const uint8_t *realm, size_t realm_length, const uint8_t *nonce,
    size_t nonce_length, const uint8_t *key, size_t key_length,
    bool fingerprint) {

  if (!ov_stun_username_validate(username, username_length) ||
      !ov_stun_realm_validate(realm, realm_length) ||
      !ov_stun_nonce_validate(nonce, nonce_length) || !key || key_length < 1)
    return false;

  return ov_stun_generate_binding_request(
      start, length, next, transaction_id, software, software_length, username,
      username_length, realm, realm_length, nonce, nonce_length, key,
      key_length, true, fingerprint);
}

/*----------------------------------------------------------------------------*/

bool ov_stun_generate_binding_request(
    uint8_t *start, size_t length, uint8_t **next,
    const uint8_t *transaction_id, const uint8_t *software,
    size_t software_length, const uint8_t *username, size_t username_length,
    const uint8_t *realm, size_t realm_length, const uint8_t *nonce,
    size_t nonce_length, const uint8_t *key, size_t key_len, bool integrity,
    bool fingerprint) {

  if (!start || length < 20 || !transaction_id)
    goto error;

  uint8_t *ptr = start;
  size_t required = 20;
  size_t len = 0;

  // check size and input

  if (software) {

    if (!ov_stun_software_validate(software, software_length))
      goto error;

    len = ov_stun_software_encoding_length(software, software_length);
    if (len == 0)
      goto error;

    required += len;
  }

  if (integrity) {

    if (!key || key_len < 1)
      goto error;

    required += ov_stun_message_integrity_encoding_length();
  }

  if (fingerprint)
    required += ov_stun_fingerprint_encoding_length();

  if (username) {

    len = ov_stun_username_encoding_length(username, username_length);
    if (len == 0)
      goto error;

    required += len;
  }

  if (realm) {

    len = ov_stun_realm_encoding_length(realm, realm_length);
    if (len == 0)
      goto error;

    required += len;
  }

  if (nonce) {

    len = ov_stun_nonce_encoding_length(nonce, nonce_length);
    if (len == 0)
      goto error;

    required += len;
  }

  if (length < required)
    goto error;

  // write header

  if (!memset(start, 0, required))
    goto error;

  if (!ov_stun_frame_set_request(start, length))
    goto error;

  if (!ov_stun_frame_set_method(start, length, STUN_BINDING))
    goto error;

  if (!ov_stun_frame_set_magic_cookie(start, length))
    goto error;

  if (!ov_stun_frame_set_length(start, length, required - 20))
    goto error;

  if (!ov_stun_frame_set_transaction_id(start, length, transaction_id))
    goto error;

  // write content

  ptr = start + 20;

  if (username)
    if (!ov_stun_username_encode(ptr, length - (ptr - start), &ptr, username,
                                 username_length))
      goto error;

  if (realm)
    if (!ov_stun_realm_encode(ptr, length - (ptr - start), &ptr, realm,
                              realm_length))
      goto error;

  if (nonce)
    if (!ov_stun_nonce_encode(ptr, length - (ptr - start), &ptr, nonce,
                              nonce_length))
      goto error;

  if (software)
    if (!ov_stun_software_encode(ptr, length - (ptr - start), &ptr, software,
                                 software_length))
      goto error;

  if (integrity)
    if (!ov_stun_add_message_integrity(start, length, ptr, &ptr, key, key_len))
      goto error;

  if (fingerprint)
    if (!ov_stun_add_fingerprint(start, length, ptr, &ptr))
      goto error;

  if (next)
    *next = ptr;
  return true;
error:
  if (next)
    *next = NULL;
  return false;
}
