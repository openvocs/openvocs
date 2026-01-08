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
        @file           ov_stun_attr_password_algorithms.c
        @author         Markus Töpfer

        @date           2022-01-22


        ------------------------------------------------------------------------
*/
#include "../include/ov_stun_attr_password_algorithms.h"

bool ov_stun_attr_is_password_algorithms(const uint8_t *buffer, size_t length) {

  if (!buffer || length < 8)
    goto error;

  uint16_t type = ov_stun_attribute_get_type(buffer, length);
  // int64_t size = ov_stun_attribute_get_length(buffer, length);

  if (type != STUN_ATTR_PASSWORD_ALGORITHMS)
    goto error;

  return true;

error:
  return false;
}

/*----------------------------------------------------------------------------*/

size_t ov_stun_attr_encode_password_algorithms_rfc8489_length() {
  return 4 + 8;
}

/*----------------------------------------------------------------------------*/

bool ov_stun_attr_encode_password_algorithms_rfc8489(uint8_t *buffer,
                                                     size_t length,
                                                     uint8_t **next) {

  /* we set sha256 as preference and MD5 as fallback */

  uint8_t buf[8] = {0};
  buf[0] = 0x00;
  buf[1] = 0x02;
  buf[2] = 0x00;
  buf[3] = 0x00;
  buf[4] = 0x00;
  buf[5] = 0x01;
  buf[6] = 0x00;
  buf[7] = 0x00;

  return ov_stun_attribute_encode(buffer, length, next,
                                  STUN_ATTR_PASSWORD_ALGORITHMS, buf, 8);
}