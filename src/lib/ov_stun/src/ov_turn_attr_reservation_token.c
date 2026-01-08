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
        @file           ov_turn_attr_reservation_token.c
        @author         Markus Töpfer

        @date           2022-01-21


        ------------------------------------------------------------------------
*/
#include "../include/ov_turn_attr_reservation_token.h"

bool ov_turn_attr_is_reservaton_token(const uint8_t *buffer, size_t length) {

  if (!buffer || length < 12)
    goto error;

  uint16_t type = ov_stun_attribute_get_type(buffer, length);
  int64_t size = ov_stun_attribute_get_length(buffer, length);

  if (type != TURN_RESERVATION_TOKEN)
    goto error;

  if (size != 8)
    goto error;

  return true;

error:
  return false;
}

/*
 *      ------------------------------------------------------------------------
 *
 *                        SERIALIZATION / DESERIALIZATION
 *
 *      ------------------------------------------------------------------------
 */

size_t ov_turn_attr_reservaton_token_encoding_length() { return 12; }

/*----------------------------------------------------------------------------*/

bool ov_turn_attr_reservaton_token_encode(uint8_t *buffer, size_t length,
                                          uint8_t **next,
                                          const uint8_t *reservaton_token,
                                          size_t size) {

  if (!buffer || !reservaton_token || (size != 8))
    goto error;

  size_t len = ov_turn_attr_reservaton_token_encoding_length();

  if (length < len)
    goto error;

  return ov_stun_attribute_encode(buffer, length, next, TURN_RESERVATION_TOKEN,
                                  reservaton_token, size);
error:
  return false;
}

/*----------------------------------------------------------------------------*/

bool ov_turn_attr_reservaton_token_decode(const uint8_t *buffer, size_t length,
                                          const uint8_t **reservaton_token,
                                          size_t *size) {

  if (!buffer || length < 8 || !reservaton_token || !size)
    goto error;

  if (!ov_turn_attr_is_reservaton_token(buffer, length))
    goto error;

  int64_t len = ov_stun_attribute_get_length(buffer, length);
  if (len <= 0)
    goto error;

  *size = (size_t)len;
  *reservaton_token = (uint8_t *)buffer + 4;

  return true;
error:
  return false;
}
