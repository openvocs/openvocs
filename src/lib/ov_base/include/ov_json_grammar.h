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
        @file           ov_json_grammar.h
        @author         Markus Toepfer

        @date           2018-12-03

        @ingroup        ov_json_grammar

        @brief          Definition of standard JSON grammer for parsing


        ------------------------------------------------------------------------
*/
#ifndef ov_json_grammar_h
#define ov_json_grammar_h

#include <inttypes.h>
#include <stdbool.h>
#include <string.h>

#include "ov_utf8.h"

/*
 *      ------------------------------------------------------------------------
 *
 *      JSON STRINGS
 *
 *      ------------------------------------------------------------------------
 */

/*
        Check if a byte is a valid JSON whitespace byte.
*/
bool ov_json_is_whitespace(uint8_t byte);

/*----------------------------------------------------------------------------*/

/**
        Unshift all whitespace at buffer.
        @param buffer   pointer to buffer move by whitespace
        @param length   MUST contain length of open buffer size
                        and will we reduced by the amount of unshifted
                        whitespace bytes.
*/
bool ov_json_clear_whitespace(uint8_t **buffer, size_t *length);

/*----------------------------------------------------------------------------*/

/**
        Valid content is a JSON string.

        @param buffer   buffer to validate
        @parem size     size of the buffer
        @param quotes   if true the buffer MUST start and end with a quote
*/
bool ov_json_validate_string(const uint8_t *buffer, size_t size, bool quotes);

/*
 *      ------------------------------------------------------------------------
 *
 *      STRING MATCHING
 *
 *      ------------------------------------------------------------------------
 */

/**
        Match a JSON string within a buffer.

                        0                   1
        bytes           0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6
                       +-------+---------------+----------
        buffer          \n \t  \" s t r i n g \"
                      ------------------------------------

        start (init)    ^
        start (success) __________^
        start (failure) ^
        end   (init)    NULL
        end   (success) ____________________^
        end   (failure) NULL

        @param start    pointer to buffer start, will be set to start
                        (first byte between quotes)
        @param end      pointer to be set to end
                        (last byte between quotes)
        @param size     size of the buffer open at start
*/
bool ov_json_match_string(uint8_t **start, uint8_t **end, size_t size);

/*---------------------------------------------------------------------------*/

/**
        Match a JSON array within a buffer.

        @NOTE will match first success
        (closing brackets matches opening brackets)

                          0                   1
        bytes           0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6
                       +-------+---------------+----------
        buffer          \n \t   [ .., .., ..  ]
                       -----------------------------------
        start (init)    ^
        start (success) __________^
        start (failure) ^
        end   (init)    NULL
        end   (success) ___________________^
        end   (failure) NULL

        @param start    pointer to buffer start, will be set to start
                        (first byte between brackets)
        @param end      pointer to be set to end
                        (last byte between brackets)
        @param size     size of the buffer open at start
*/
bool ov_json_match_array(uint8_t **start, uint8_t **end, size_t size);

/*---------------------------------------------------------------------------*/

/**
        Match a JSON object within a buffer.

        @NOTE will match first success
        (closing brackets matches opening brackets)

                          0                   1
        bytes           0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6
                       +-------+---------------+----------
        buffer          \n \t   { .., .., ..  }
                       -----------------------------------
        start (init)    ^
        start (success) __________^
        start (failure) ^
        end   (init)    NULL
        end   (success) ___________________^
        end   (failure) NULL

        @param start    pointer to buffer start, will be set to start
                        (first byte between brackets)
        @param end      pointer to be set to end
                        (last byte between brackets)
        @param size     size of the buffer open at start
*/
bool ov_json_match_object(uint8_t **start, uint8_t **end, size_t size);

/*---------------------------------------------------------------------------*/

/**
        Match for valid JSON content within a buffer.

        @param start            start of the buffer
        @param size             size of the buffer open at start
        @param incomplete       if true, return success, even if the JSON
   parsing is unfinished, but valid up to size

                                @NOTE Incomplete will only work in strict
   parsing for objects {} and arrays [], for numbers, strings and literals only
   variable length matching is supported.
        @param last_of_first    Will point to the last byte of the first
   successfully parsed JSON value. This pointer is helpfull if the matching
   function is used to match some buffers including a JSON value, followed by a
   partial JSON value e.g. {"one":true}{"secondhalf":[
   If the buffer contains only the beginning of a valid json string, but no
   entire one, there is no first fully parseable element, last_of_first is set
   to 0
*/
bool ov_json_match(const uint8_t *start,
                   size_t size,
                   bool incomplete,
                   uint8_t **last_of_first);

#endif /* ov_json_grammar_h */
