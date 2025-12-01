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
        @file           ov_stun_unknown_attributes.c
        @author         Markus Toepfer

        @date           2018-10-23

        @ingroup        ov_stun

        @brief          Implementation of RFC 5389 attribute "unknown attributes"


        ------------------------------------------------------------------------
*/
#include "../include/ov_stun_unknown_attributes.h"

/*
 *      ------------------------------------------------------------------------
 *
 *                              FRAME CHECKING
 *
 *      ------------------------------------------------------------------------
 */

bool ov_stun_attribute_frame_is_unknown_attributes(const uint8_t *buffer,
                                                   size_t length) {

  if (!buffer || length < 8)
    goto error;

  uint16_t type = ov_stun_attribute_get_type(buffer, length);
  int64_t size = ov_stun_attribute_get_length(buffer, length);

  if (type != STUN_UNKNOWN_ATTRIBUTES)
    goto error;

  if (length < (size_t)size + 4)
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

ov_list *ov_stun_unknown_attributes_decode(const uint8_t *buffer,
                                           size_t length) {

  ov_list *list = NULL;

  if (!ov_stun_attribute_frame_is_unknown_attributes(buffer, length))
    goto error;

  list = ov_list_create((ov_list_config){0});
  if (!list)
    goto error;

  size_t size = ov_stun_attribute_get_length(buffer, length);

  for (size_t i = 0; i < size; i += 2) {

    if (!ov_list_push(list, (uint8_t *)buffer + 4 + i))
      goto error;
  }

  return list;
error:
  return ov_list_free(list);
}

/*----------------------------------------------------------------------------*/

uint16_t ov_stun_unknown_attributes_decode_pointer(const uint8_t *pointer) {

  if (!pointer)
    return 0;

  return ntohs(*(uint16_t *)pointer);
}

/*----------------------------------------------------------------------------*/

bool ov_stun_unknown_attributes_encode(uint8_t *buffer, size_t length,
                                       uint8_t **next, uint16_t array[],
                                       size_t array_last) {

  if (!buffer || !array || array_last < 1)
    return false;

  size_t required = array_last * 2 + 4;
  size_t pad = 0;
  pad = required % 4;
  if (pad != 0)
    pad = 4 - pad;

  required += pad;

  if (length < required)
    return false;

  uint8_t content[required - 4 - pad];
  uint8_t *ptr = content;

  for (size_t i = 0; i < array_last; i++) {

    *(uint16_t *)ptr = htons(array[i]);
    ptr += 2;
  }

  if (next)
    *next = buffer + required;

  return ov_stun_attribute_encode(buffer, length, next, STUN_UNKNOWN_ATTRIBUTES,
                                  content, required - 4 - pad);
}