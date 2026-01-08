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
        @file           ov_stun_fingerprint.c
        @author         Markus Toepfer

        @date           2018-10-22

        @ingroup        ov_stun

        @brief          Implementation of RFC 5389 attribute "fingerprint"


        ------------------------------------------------------------------------
*/
#include "../include/ov_stun_fingerprint.h"
#include <ov_base/ov_crc32.h>

/*      ------------------------------------------------------------------------
 *
 *                              FRAME CHECKING
 *
 *      ------------------------------------------------------------------------
 */

bool ov_stun_attribute_frame_is_fingerprint(const uint8_t *buffer,
                                            size_t length) {

  if (!buffer || length < 8)
    goto error;

  uint16_t type = ov_stun_attribute_get_type(buffer, length);
  int64_t size = ov_stun_attribute_get_length(buffer, length);

  if (type != STUN_FINGERPRINT)
    goto error;

  if (size != 4)
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

size_t ov_stun_fingerprint_encoding_length() { return 8; }

/*----------------------------------------------------------------------------*/

bool ov_stun_add_fingerprint(uint8_t *head, size_t length, uint8_t *start,
                             uint8_t **next) {

  if (!head || !start || !length)
    goto error;

  if (length < (size_t)(start - head) + 8)
    goto error;

  size_t len = (start - head) + 8;

  if (len < 28)
    goto error;

  if (length < len)
    goto error;

  // not starting at multiple of 32 bit
  if (((start - head) % 4) != 0)
    goto error;

  if (!ov_stun_attribute_set_type(start, 4, STUN_FINGERPRINT))
    goto error;

  if (!ov_stun_attribute_set_length(start, 4, 4))
    goto error;

  // set length including fingerprint, excluding the header
  if (!ov_stun_frame_set_length(head, length, len - 20))
    goto error;

  // compute fingerprint

  uint32_t crc = ov_crc32_zlib(0, head, (start - head));
  *(uint32_t *)(start + 4) = htonl(crc ^ 0x5354554e);

  if (next)
    *next = start + 8;

  return true;

error:
  return false;
}

/*----------------------------------------------------------------------------*/

bool ov_stun_check_fingerprint(uint8_t *head, size_t length, uint8_t *attr[],
                               size_t attr_size, bool must_be_set) {

  if (!head || !attr || length < 20 || attr_size < 1)
    goto error;

  uint32_t crc = 0;
  size_t len = 0;
  uint8_t *finger = NULL;

  for (size_t i = 0; i < attr_size; i++) {

    if (finger)
      attr[i] = NULL;

    if (ov_stun_attribute_frame_is_fingerprint(attr[i],
                                               length - (attr[i] - head))) {
      finger = attr[i];
    }
  }

  if (!finger) {

    if (must_be_set)
      goto error;

    // nothing to check
    return true;
  }

  // ignore original length
  len = (finger - head) - 12; // + 8 - 20

  if (!ov_stun_frame_set_length(head, length, len))
    goto error;

  crc = ov_crc32_zlib(0, head, (finger - head));
  crc = crc ^ 0x5354554e;

  if (crc != htonl(*(uint32_t *)(finger + 4)))
    goto error;

  // do not reset original length
  return true;
error:
  return false;
}
