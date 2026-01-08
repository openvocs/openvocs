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

        This file is part of the openvocs project. https://openvocs.org

        ------------------------------------------------------------------------
*//**
        @file           ov_stun_grammar.c
        @author         Markus Toepfer

        @date           2018-06-21

        @ingroup        ov_stun

        @brief          Implementation of basic STUN grammar


        ------------------------------------------------------------------------
*/
#include "../include/ov_stun_grammar.h"

bool ov_stun_grammar_offset_linear_whitespace(const char *start, size_t length,
                                              char **next) {

    if (!start || !next || length < 1)
        return false;

    char *ptr = (char *)start;

    // return true is no
    if (!isspace(ptr[0])) {
        *next = ptr;
        return true;
    }

    while ((ptr - start) < (int64_t)length) {

        if (!isspace(*ptr))
            break;

        ptr++;
    }

    if (isspace(*ptr))
        return false;

    *next = ptr;
    return true;
}

/*----------------------------------------------------------------------------*/

bool ov_stun_grammar_is_qdtext(const uint8_t *start, size_t length) {

    if (!start || length < 1)
        return false;

    uint8_t *ptr = (uint8_t *)start;

    if (!ov_stun_grammar_offset_linear_whitespace((char *)ptr, length,
                                                  (char **)&ptr))
        return false;

    // MUST be UTF8
    if (!ov_utf8_validate_sequence(ptr, length - (ptr - start)))
        return false;

    // check allowed ascii
    for (size_t i = 0; i < length - (ptr - start); i++) {

        if (ptr[i] <= 0x7F) {

            // ASCII

            if (ptr[i] > 0x7E)
                return false;

            if (ptr[i] == 0x5C)
                return false;

            if (ptr[i] == 0x22)
                return false;

            if (ptr[i] < 0x21)
                return false;

        } else {

            // offset UTF8 bytes
            if (ptr[i] & 0xF0) {
                i += 3;
            } else if (ptr[i] & 0xE0) {
                i += 2;
            } else if (ptr[i] & 0xC0) {
                i += 1;
            }
        }
    }

    return true;
}

/*----------------------------------------------------------------------------*/

bool ov_stun_grammar_is_quoted_pair(const uint8_t *start, size_t length) {

    if (!start || length != 2)
        return false;

    if (start[0] != 0x5C)
        return false;

    if (start[1] > 0x7F)
        return false;

    if (start[1] == 0x0A)
        return false;

    if (start[1] == 0x0D)
        return false;

    return true;
}

/*----------------------------------------------------------------------------*/

bool ov_stun_grammar_is_quoted_string_content(const uint8_t *start,
                                              size_t length) {

    if (!start || length < 1)
        return false;

    uint8_t *ptr = (uint8_t *)start;
    uint8_t *slash = NULL;
    int64_t open = length;

    while (open > 0) {

        slash = memchr(ptr, 0x5C, open);

        if (!slash)
            return ov_stun_grammar_is_qdtext(ptr, open);

        if (open < 2)
            return false;

        if (open - (slash - ptr) < 2)
            return false;

        if (!ov_stun_grammar_is_quoted_pair(slash, 2))
            return false;

        if (ptr != slash)
            if (!ov_stun_grammar_is_qdtext(ptr, slash - ptr))
                return false;

        ptr = slash + 2;
        open = length - (ptr - start);
    }

    return true;
}

/*----------------------------------------------------------------------------*/

bool ov_stun_grammar_is_quoted_string(const uint8_t *start, size_t length) {

    if (!start || length < 3)
        return false;

    uint8_t *ptr = (uint8_t *)start;

    if (!isspace(ptr[0]))
        return false;

    if (ptr[1] != 0x22)
        return false;

    if (ptr[length - 1] != 0x22)
        return false;

    // empty quoted string
    if (length == 3)
        return true;

    return ov_stun_grammar_is_quoted_string_content(ptr + 2, length - 3);
}