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
        @file           ov_stun_alternate_server.c
        @author         Markus Toepfer

        @date           2018-10-23

        @ingroup        ov_stun

        @brief          Implementation of RFC 5389 attribute "alternate server"


        ------------------------------------------------------------------------
*/
#include "../include/ov_stun_alternate_server.h"

/*
 *      ------------------------------------------------------------------------
 *
 *                              FRAME CHECKING
 *
 *      ------------------------------------------------------------------------
 */

bool ov_stun_attribute_frame_is_alternate_server(const uint8_t *buffer,
                                                 size_t length) {

  if (!buffer || length < 12)
    goto error;

  uint16_t type = ov_stun_attribute_get_type(buffer, length);
  int64_t size = ov_stun_attribute_get_length(buffer, length);

  if (type != STUN_ALTERNATE_SERVER)
    goto error;

  // check family + size
  switch (*(uint8_t *)(buffer + 5)) {

  case 0x01:

    if (size != 8)
      goto error;
    break;

  case 0x02:

    if (size != 20)
      goto error;
    break;

  default:
    goto error;
  }

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

/*----------------------------------------------------------------------------*/

size_t
ov_stun_alternate_server_encoding_length(const struct sockaddr_storage *sa) {

  if (!sa)
    goto error;

  // size including attribute header

  if (sa->ss_family == AF_INET)
    return 12;

  if (sa->ss_family == AF_INET6)
    return 24;

error:
  return 0;
}

/*----------------------------------------------------------------------------*/

bool ov_stun_alternate_server_encode(uint8_t *buffer, size_t length,
                                     uint8_t **next,
                                     const struct sockaddr_storage *sa) {

  if (!buffer || !sa || length < 12)
    goto error;

  struct sockaddr_in *sock4 = NULL;
  struct sockaddr_in6 *sock6 = NULL;

  if (!ov_stun_attribute_set_type(buffer, length, STUN_ALTERNATE_SERVER))
    goto error;

  uint8_t *content = buffer + 4;

  switch (sa->ss_family) {

  case AF_INET:

    if (!ov_stun_attribute_set_length(buffer, length, 8))
      goto error;

    sock4 = (struct sockaddr_in *)sa;

    content[0] = 0;
    content[1] = 1;

    if (!memcpy(content + 2, &sock4->sin_port, 2))
      goto error;

    if (!memcpy(content + 4, &sock4->sin_addr, sizeof(struct in_addr)))
      goto error;

    if (next)
      *next = content + 8;

    break;

  case AF_INET6:

    if (length < 24)
      goto error;

    if (!ov_stun_attribute_set_length(buffer, length, 20))
      goto error;

    sock6 = (struct sockaddr_in6 *)sa;

    content[0] = 0;
    content[1] = 1;

    if (!memcpy(content + 2, &sock6->sin6_port, 2))
      goto error;

    if (!memcpy(content + 4, &sock6->sin6_addr, sizeof(struct in6_addr)))
      goto error;

    if (next)
      *next = content + 20;

    break;

  default:
    goto error;
  }

  return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

bool ov_stun_alternate_server_decode(const uint8_t *buffer, size_t length,
                                     struct sockaddr_storage **address) {

  bool created = false;
  if (!buffer || length < 12 || !address)
    goto error;

  if (!ov_stun_attribute_frame_is_alternate_server(buffer, length))
    goto error;

  if (!*address) {

    *address = calloc(1, sizeof(struct sockaddr_storage));
    if (!*address)
      goto error;

    created = true;
  }

  struct sockaddr_storage *sa = *address;
  struct sockaddr_in *sock4 = NULL;
  struct sockaddr_in6 *sock6 = NULL;

  uint8_t *content = (uint8_t *)buffer + 4;

  switch (content[1]) {

  case 0x01:
    // IPv4
    sock4 = (struct sockaddr_in *)sa;
    sock4->sin_family = AF_INET;

    if (!memcpy(&sock4->sin_port, content + 2, 2))
      goto error;

    if (!memcpy(&sock4->sin_addr, content + 4, sizeof(struct in_addr)))
      goto error;

    break;
  case 0x02:
    // IPv6
    sock6 = (struct sockaddr_in6 *)sa;
    sock6->sin6_family = AF_INET6;

    if (!memcpy(&sock6->sin6_port, content + 2, 2))
      goto error;

    if (!memcpy(&sock6->sin6_addr, content + 4, sizeof(struct in6_addr)))
      goto error;

    break;
  default:
    goto error;
  }

  return true;

error:
  if (address) {
    if (created) {
      free(*address);
      *address = NULL;
    }
  }
  return false;
}