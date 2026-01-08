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
        @file           ov_endian.c
        @author         Markus Toepfer

        @date           2018-11-21

        @ingroup        ov_basics

        @brief          Implementation of some endian write and read functions.


        ------------------------------------------------------------------------
*/
#include "../../include/ov_endian.h"

bool ov_endian_write_uint16_be(uint8_t *buffer, size_t buffer_size,
                               uint8_t **next, uint16_t number) {

  if (!buffer || buffer_size < 2)
    goto error;

  buffer[1] = number;
  buffer[0] = number >> 8;

  if (next)
    *next = buffer + 2;

  return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

bool ov_endian_write_uint32_be(uint8_t *buffer, size_t buffer_size,
                               uint8_t **next, uint32_t number) {

  if (!buffer || buffer_size < 4)
    goto error;

  buffer[3] = number;
  buffer[2] = number >> 8;
  buffer[1] = number >> 16;
  buffer[0] = number >> 24;

  if (next)
    *next = buffer + 4;

  return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

bool ov_endian_write_uint64_be(uint8_t *buffer, size_t buffer_size,
                               uint8_t **next, uint64_t number) {

  if (!buffer || buffer_size < 8)
    goto error;

  buffer[7] = number;
  buffer[6] = number >> 8;
  buffer[5] = number >> 16;
  buffer[4] = number >> 24;
  buffer[3] = number >> 32;
  buffer[2] = number >> 40;
  buffer[1] = number >> 48;
  buffer[0] = number >> 56;

  if (next)
    *next = buffer + 8;

  return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

bool ov_endian_read_uint16_be(const uint8_t *buffer, size_t buffer_size,
                              uint16_t *number) {

  if (!buffer || !number || buffer_size < 2)
    goto error;

  *number = ((uint16_t)buffer[1] << 0) | ((uint16_t)buffer[0] << 8);

  return true;
error:
  if (number)
    *number = 0;
  return false;
}

/*----------------------------------------------------------------------------*/

bool ov_endian_read_uint32_be(const uint8_t *buffer, size_t buffer_size,
                              uint32_t *number) {

  if (!buffer || !number || buffer_size < 4)
    goto error;

  *number = ((uint32_t)buffer[3] << 0) | ((uint32_t)buffer[2] << 8) |
            ((uint32_t)buffer[1] << 16) | ((uint32_t)buffer[0] << 24);

  return true;
error:
  if (number)
    *number = 0;
  return false;
}

/*----------------------------------------------------------------------------*/

bool ov_endian_read_uint64_be(const uint8_t *buffer, size_t buffer_size,
                              uint64_t *number) {

  if (!buffer || !number || buffer_size < 8)
    goto error;

  *number = ((uint64_t)buffer[7] << 0) | ((uint64_t)buffer[6] << 8) |
            ((uint64_t)buffer[5] << 16) | ((uint64_t)buffer[4] << 24) |
            ((uint64_t)buffer[3] << 32) | ((uint64_t)buffer[2] << 40) |
            ((uint64_t)buffer[1] << 48) | ((uint64_t)buffer[0] << 56);

  return true;
error:
  if (number)
    *number = 0;
  return false;
}

/*----------------------------------------------------------------------------*/
