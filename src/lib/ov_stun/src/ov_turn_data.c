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
        @file           ov_turn_data.c
        @author         Markus Töpfer

        @date           2022-01-21


        ------------------------------------------------------------------------
*/
#include "../include/ov_turn_data.h"

#include "../include/ov_stun_fingerprint.h"
#include "../include/ov_turn_attr_data.h"
#include "../include/ov_turn_attr_xor_peer_address.h"

bool ov_turn_method_is_data(const uint8_t *frame, size_t length) {

  if (TURN_DATA == ov_stun_frame_get_method(frame, length))
    return true;

  return false;
}

/*----------------------------------------------------------------------------*/

bool ov_turn_create_data(uint8_t *start, size_t length, uint8_t **next,
                         const uint8_t *transaction_id,
                         const struct sockaddr_storage *address,
                         const uint8_t *buffer, size_t size, bool fingerprint) {

  if (!start || length < 20 || !transaction_id)
    goto error;

  uint8_t *ptr = start;
  size_t required = 20;

  required += ov_turn_attr_xor_peer_address_encoding_length(address);
  required += ov_turn_attr_data_encoding_length(size);

  if (fingerprint)
    required += ov_stun_fingerprint_encoding_length();

  if (length < required)
    goto error;

  // write header

  if (!memset(start, 0, required))
    goto error;

  if (!ov_stun_frame_set_indication(start, length))
    goto error;

  if (!ov_stun_frame_set_method(start, length, TURN_DATA))
    goto error;

  if (!ov_stun_frame_set_magic_cookie(start, length))
    goto error;

  if (!ov_stun_frame_set_length(start, length, required - 20))
    goto error;

  if (!ov_stun_frame_set_transaction_id(start, length, transaction_id))
    goto error;

  // write content

  ptr = start + 20;

  if (!ov_turn_attr_xor_peer_address_encode(ptr, length - (ptr - start), start,
                                            &ptr, address))
    goto error;

  if (!ov_turn_attr_data_encode(ptr, length - (ptr - start), &ptr, buffer,
                                size))
    goto error;

  if (fingerprint)
    if (!ov_stun_add_fingerprint(start, length, ptr, &ptr))
      goto error;

  if (next)
    *next = ptr;

  return true;
error:
  return false;
}