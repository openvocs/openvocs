/***
        ------------------------------------------------------------------------

        Copyright 2017 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_time.h
        @author         Markus Toepfer
        @author         Michael Beer

        @date           2017-12-10

        @ingroup        ov_base

        @brief          Definition of ov_time


        ------------------------------------------------------------------------
*/
#ifndef ov_time_h
#define ov_time_h

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>

#include <ov_log/ov_log.h>

#include "ov_json.h"

typedef enum ov_time_scope {
    TIME_SCOPE_YEAR,
    TIME_SCOPE_MONTH,
    TIME_SCOPE_DAY,
    TIME_SCOPE_HOUR,
    TIME_SCOPE_MINUTE,
    TIME_SCOPE_SECOND,
    TIME_SCOPE_MILLISECOND,
    TIME_SCOPE_MICROSECOND
} ov_time_scope_t;

/*----------------------------------------------------------------------------*/

typedef enum ov_time_unit {
    SEC = 1,
    USEC,
} ov_time_unit;

/*----------------------------------------------------------------------------*/

typedef struct {

    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;

} ov_time;

/*----------------------------------------------------------------------------*/

/**
 * clock id of the clock everyone should use.
 * If one uses a different clock, the times might not be in sync, up to the
 * point where threads end up waiting eternally etc.
 */
#define OV_CLOCK_ID CLOCK_MONOTONIC

/*----------------------------------------------------------------------------*/

/**
        Create an ISO 8601 or an ISO 8601 enhanced timestamp including
        microseconds. The result will be either NULL on error or one of:

        2017-11-20T14:57:01Z
        2017-11-20T14:57:01.123456Z

        @param micro            if true use microseconds
        @returns                Timestring buffer of 30 allocated bytes
                                or NULL. result MUST be freed.
 */
char *ov_timestamp(bool micro);

/*----------------------------------------------------------------------------*/

/**
        Create an ISO 8601 or an ISO 8601 enhanced timestamp including
        microseconds. This function will write one of:

        2017-11-20T14:57:01Z
        2017-11-20T14:57:01.123456Z

        to the pointer at buffer if the size is sufficiant.

        @param micro            if true use microseconds
        @param buffer           pointer to buffer to be filled with chars
        @param size             size of the buffer (incl. terminating zero)
        @returns                true on success, false on error
*/
bool ov_timestamp_write_to(bool micro, char *buffer, size_t size);

/*----------------------------------------------------------------------------*/

/**
        Create an time string based on the scope provided. This functions
        will create string of the following form:

        SCOPE           RESULT
        -------------------------------------------
        YEAR            2017
        MONTH           2017-11
        DAY             2017-11-20
        HOUR            2017-11-20 14
        MINUTE          2017-11-20 14:57
        SECOND          2017-11-20 14:57:01
        MILLISECOND     2017-11-20 14:57:01.123
        MICROSECOND     2017-11-20 14:57:01.123456

        @param scope    SCOPE to be used for the string
        @returns        allocated timestring or NULL (result MUST be freed)
*/
char *ov_time_string(ov_time_scope_t scope);

/*----------------------------------------------------------------------------*/

char *ov_timestamp_from_data(uint16_t year,
                             uint8_t month,
                             uint8_t day,
                             uint8_t hour,
                             uint8_t minute,
                             uint8_t second);

/*----------------------------------------------------------------------------*/

bool ov_timestamp_parse(const char *timestamp,
                        uint16_t *year,
                        uint8_t *month,
                        uint8_t *day,
                        uint8_t *hour,
                        uint8_t *minute,
                        uint8_t *second);

/*----------------------------------------------------------------------------*/

ov_time ov_timestamp_from_string(const char *timestamp);
char *ov_timestamp_to_string(ov_time time);

/*----------------------------------------------------------------------------*/

ov_time ov_timestamp_create();

/*----------------------------------------------------------------------------*/

uint64_t ov_time_to_epoch(ov_time time);

/*----------------------------------------------------------------------------*/

/**
        Write a scoped timestring to a buffer.

        @param scope    SCOPE to be used for the string
        @param buffer   pointer to buffer to be filled with chars
        @param size     size of the buffer (incl. terminating zero)
        @returns        true on success, false on error
*/
bool ov_time_write_to(ov_time_scope_t scope, char *buffer, size_t size);

/*----------------------------------------------------------------------------*/

uint64_t ov_time_get_abs_timeout_usec(uint64_t rel_timeout_usec);

/*----------------------------------------------------------------------------*/

uint64_t ov_time_get_current_time_usecs();

/*----------------------------------------------------------------------------*/

bool ov_time_to_string(char *target,
                       size_t size,
                       uint64_t usecs,
                       ov_time_unit unit);

/*----------------------------------------------------------------------------*/

#endif /* ov_time_h */
