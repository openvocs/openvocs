/***
        ------------------------------------------------------------------------

        Copyright (c) 2019 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_imf.h
        @author         Markus Toepfer

        @date           2019-07-19

        @ingroup        ov_core

        @brief          Definition of a implementation of

                        RFC 5322 "Internet Message Format"


                        @NOTE implementeation uncomple
        ------------------------------------------------------------------------
*/
#ifndef ov_imf_h
#define ov_imf_h

#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>

#include <time.h>

/**
        Write an IMF timestamp to an buffer.

        ** Sun, 06 Nov 1994 08:49:37 GMT **

        This function will write an IMF string with GMT and englisch names,
        independent of the locale settings.

        Advantage over strftime ...

                no locale issues,
                no locale changes

        @param buffer   buffer to write to
        @param size     size of the buffer
        @param next     optional pointer to next byte after write
*/
bool ov_imf_write_timestamp(char *buffer, size_t size, char **next);

/*----------------------------------------------------------------------------*/

/**
        Convert a string to a struct tm.

        ** Sun, 06 Nov 1994 08:49:37 GMT **

        This function will parse an IMF string independent of the
        locale settings.

        Required:

                GMT
                Englisch based month and day names

        Advantage over strptime ...

                no locale issues,
                no locale changes

        @param buffer   buffer to write to
        @param size     size of the buffer
        @param next     optional pointer to next byte after write
*/
struct tm ov_imf_timestamp_to_tm(const char *start, size_t size, char **next);

/*----------------------------------------------------------------------------*/

/**
        Convert a struct tm to an imf timestamp.

        @param source   source to convert (GMT)
        @param buffer   buffer to write to
        @param size     size of the buffer
        @param next     optional pointer to next byte after write
        @param zone     optional string for zone to write
*/
bool ov_imf_tm_to_timestamp(const struct tm *source,
                            char *buffer,
                            size_t size,
                            char **next,
                            const char *zone);

#endif /* ov_imf_h */
