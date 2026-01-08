/***
        ------------------------------------------------------------------------

        Copyright 2018 German Aerospace Center DLR e.V. (GSOC)

        Licensed under the Apache License, Version 2.0 (the "License");
        you may not use this file except in compliance with the License.
        You may obtain a copy of the License at

                http://www.apache.org/licenses/LICENSE-2.0

        Unless required by applicable law or agreed to in writing, software
        distributed under the License is distributed on an "AS IS" BASIS,
        WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
        See the License for the specific language governing permissions and
        limitations under the License.

        This file is part of the openvocs project. http://openvocs.org

        ------------------------------------------------------------------------
*//**
        @file           ov_convert.h
        @author         Markus Toepfer
        @author         Michael Beer

        @date           2018-03-06

        @ingroup        ov_base

        @brief          Definition of basic convertions.


        ------------------------------------------------------------------------
*/
#ifndef ov_convert_h
#define ov_convert_h

#include <ctype.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ov_log/ov_log.h>

/*----------------------------------------------------------------------------*/

/**
 * Give number of samples within a particular number of milli seconds.
 * Requires a samplerate obviously.
 */
#define ov_convert_msecs_to_samples(msecs, samplerate_hz)                      \
  ((msecs * samplerate_hz) / 1000)

/**
 * Give time span in msecs formed by a number of samples.
 * Requires a samplerate obviously.
 * Returns 0 in case of the samplerate being 0.
 */
#define ov_convert_samples_to_msecs(samples, samplerate_hz)                    \
  (0 == samplerate_hz ? 0 : ((samples * 1000) / samplerate_hz))

/*
 *      ------------------------------------------------------------------------
 *
 *      STRING TO ...
 *
 *      In general:
 *      If the string up to size fits with the type required in the output
 *      number, the conversion will be done and written to the pointer at
 *      number.
 *
 *      ------------------------------------------------------------------------
 */

bool ov_convert_string_to_uint(const char *string, uint64_t size,
                               uint64_t *number, uint64_t max);

bool ov_convert_string_to_uint64(const char *string, uint64_t size,
                                 uint64_t *number);

bool ov_convert_string_to_uint32(const char *string, uint64_t size,
                                 uint32_t *number);

bool ov_convert_string_to_uint16(const char *string, uint64_t size,
                                 uint16_t *number);

bool ov_convert_string_to_int64(const char *string, uint64_t size,
                                int64_t *number);

bool ov_convert_string_to_double(const char *string, uint64_t size,
                                 double *number);

/*
 *      ------------------------------------------------------------------------
 *
 *      DOUBLE TO ...
 *
 *      In general two input modes are supported:
 *
 *      (1)     a preallocated string of size
 *              The resulting string will be copied to the preallocated string
 *              and the size will be set to the actual used length.
 *
 *      (2)     a string pointer to NULL
 *              The resulting string will be allocated and the size wil be
 *              set to the actual used length.
 *
 *      ------------------------------------------------------------------------
 */

bool ov_convert_double_to_string(double number, char **string, size_t *size);

/*
 *      ------------------------------------------------------------------------
 *
 *      INT TO ...
 *
 *      In general two input modes are supported:
 *
 *      (1)     a preallocated string of size
 *              The resulting string will be copied to the preallocated string
 *              and the size will be set to the actual used length.
 *
 *      (2)     a string pointer to NULL
 *              The resulting string will be allocated and the size wil be
 *              set to the actual used length.
 *      ------------------------------------------------------------------------
 */

bool ov_convert_int64_to_string(int64_t number, char **string, size_t *size);

bool ov_convert_uint64_to_string(uint64_t number, char **string, size_t *size);

bool ov_convert_uint32_to_string(uint32_t number, char **string, size_t *size);

/*
 *      ------------------------------------------------------------------------
 *
 *      Additional convertions
 *
 *      ------------------------------------------------------------------------
 */

/**
        Convert the first (upto 20 byte) of the input string to an UINT64
        value.
*/
bool ov_convert_hex_string_to_uint64(const char *string, size_t size,
                                     uint64_t *number);

/*
 *      ------------------------------------------------------------------------
 *
 *      HEX to BINARY and BACK
 *
 *      In general two input modes are supported:
 *
 *      (1)     a preallocated out of size
 *              The resulting string will be copied to the preallocated out
 *              and the size will be set to the actual used length.
 *
 *      (2)     a out pointer to NULL
 *              The resulting string will be allocated and the size wil be
 *              set to the actual used length.
 *
 *      ------------------------------------------------------------------------
 */

bool ov_convert_binary_to_hex(const uint8_t *binary, size_t binary_length,
                              uint8_t **out, size_t *out_length);

bool ov_convert_hex_to_binary(const uint8_t *hex, size_t hex_length,
                              uint8_t **out, size_t *out_length);

double ov_convert_from_db(double db);

uint8_t ov_convert_from_vol_percent(uint8_t percent,
                                    uint8_t db_drop_per_decade);
uint8_t ov_convert_to_vol_percent(uint8_t multiplier,
                                  uint8_t db_drop_per_decade);

#endif /* ov_convert_h */
