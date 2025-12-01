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
        @file           ov_turn_attr_dont_fragment.c
        @author         Markus Töpfer

        @date           2022-01-21


        ------------------------------------------------------------------------
*/
#include "../include/ov_turn_attr_dont_fragment.h"

bool ov_turn_attr_is_dont_fragment(const uint8_t *buffer, size_t length) {

  if (!buffer || length < 4)
    goto error;

  uint16_t type = ov_stun_attribute_get_type(buffer, length);
  int64_t size = ov_stun_attribute_get_length(buffer, length);

  if (type != TURN_DONT_FRAGMENT)
    goto error;

  if (size != 0)
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

size_t ov_turn_attr_dont_fragment_encoding_length() { return 4; }

/*----------------------------------------------------------------------------*/

bool ov_turn_attr_dont_fragment_encode(uint8_t *buffer, size_t length,
                                       uint8_t **next) {

  if (!buffer)
    goto error;

  size_t len = ov_turn_attr_dont_fragment_encoding_length();

  if (length < len)
    goto error;

  return ov_stun_attribute_encode(buffer, length, next, TURN_DONT_FRAGMENT,
                                  NULL, 0);
error:
  return false;
}
