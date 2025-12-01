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
        @file           ov_stun_attribute.c
        @author         Markus Toepfer

        @date           2018-10-18

        @ingroup        ov_stun

        @brief          Implementation of of RFC 5389 attribute related functions.


        ------------------------------------------------------------------------
*/
#include "../include/ov_stun_attribute.h"

/*
 *      ------------------------------------------------------------------------
 *
 *                              FRAME CHECKING
 *
 *      ------------------------------------------------------------------------
 */

bool ov_stun_attribute_frame_is_valid(const uint8_t *buffer, size_t length) {

  if (!buffer || length < 4)
    goto error;

  int64_t len = ov_stun_attribute_get_length(buffer, length);
  if (len < 0)
    goto error;

  size_t pad = 0;
  pad = len % 4;
  if (pad != 0)
    pad = 4 - pad;

  len += 4 + pad;

  if (length < (size_t)len)
    goto error;

  return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

bool ov_stun_attribute_frame_content(const uint8_t *buffer, size_t length,
                                     uint8_t **content_start,
                                     size_t *content_length) {

  if (!buffer || !content_start || !content_length)
    goto error;

  if (!ov_stun_attribute_frame_is_valid(buffer, length))
    goto error;

  int64_t len = ov_stun_attribute_get_length(buffer, length);

  if (len < 0) {

    goto error;

  } else if (len == 0) {

    *content_start = NULL;

  } else {

    *content_start = (uint8_t *)buffer + 4;
  }

  *content_length = (size_t)len;
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

uint16_t ov_stun_attribute_get_type(const uint8_t *buffer, size_t length) {

  if (!buffer || length < 2)
    return 0;

  return ntohs(*(uint16_t *)(buffer));
}

/*----------------------------------------------------------------------------*/

bool ov_stun_attribute_set_type(uint8_t *buffer, size_t length, uint16_t type) {

  if (!buffer || length < 2)
    return false;

  *(uint16_t *)(buffer) = htons(type);
  return true;
}

/*----------------------------------------------------------------------------*/

int64_t ov_stun_attribute_get_length(const uint8_t *buffer, size_t length) {

  if (!buffer || length < 4)
    return -1;

  // use of shift based calculation (network byte order)
  return ntohs(*(uint16_t *)(buffer + 2));
}

/*----------------------------------------------------------------------------*/

bool ov_stun_attribute_set_length(uint8_t *buffer, size_t buffer_length,
                                  uint16_t length) {

  if (!buffer || buffer_length < 4)
    return false;

  *(uint16_t *)(buffer + 2) = htons(length);
  return true;
}

/*----------------------------------------------------------------------------*/

bool ov_stun_attribute_encode(uint8_t *buffer, size_t length, uint8_t **next,
                              uint16_t attribute_type,
                              const uint8_t *content_start,
                              size_t content_length) {

  if (!buffer)
    goto error;

  size_t pad = 0;
  pad = content_length % 4;
  if (pad != 0)
    pad = 4 - pad;

  if (length < (4 + content_length + pad))
    goto error;

  // set attribute
  if (!ov_stun_attribute_set_type(buffer, length, attribute_type))
    goto error;

  if (!ov_stun_attribute_set_length(buffer, length, content_length))
    goto error;

  if (content_start) {

    if (!memcpy(buffer + 4, content_start, content_length))
      goto error;

    if (!memset(buffer + 4 + content_length, 0, pad))
      goto error;
  }

  if (next)
    *next = buffer + 4 + content_length + pad;

  return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

bool ov_stun_attribute_decode(const uint8_t *buffer, size_t length,
                              uint16_t *type, uint8_t **content) {

  if (!buffer || length < 4 || !type || !content)
    goto error;

  int64_t len = ov_stun_attribute_get_length(buffer, length);
  if (len < 0)
    goto error;

  *type = ov_stun_attribute_get_type(buffer, length);

  if (len > 0) {

    *content = (uint8_t *)strndup((char *)buffer + 4, len);

  } else {

    *content = NULL;
  }

  return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

uint8_t *ov_stun_attributes_get_type(uint8_t *attr[], size_t attr_size,
                                     uint16_t type) {

  if (!attr || attr_size < 1)
    goto error;

  for (size_t i = 0; i < attr_size; i++) {

    if (attr[i] == NULL)
      break;

    if (type == ov_stun_attribute_get_type(attr[i], 4))
      return attr[i];
  }

error:
  return NULL;
}

/*----------------------------------------------------------------------------*/

bool ov_stun_attribute_frame_copy(const uint8_t *orig, uint8_t *dest,
                                  size_t dest_size, uint8_t **next) {

  if (!orig || !dest)
    goto error;

  size_t length = ov_stun_attribute_get_length(orig, 4);
  if (dest_size < length + 4)
    goto error;

  if (!memcpy(dest, orig, length + 4))
    goto error;

  if (next)
    *next = dest + length + 4;

  return true;
error:
  return false;
}