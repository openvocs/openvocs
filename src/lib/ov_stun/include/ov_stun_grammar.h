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
        @file           ov_stun_grammar.h
        @author         Markus Toepfer

        @date           2018-06-21

        @ingroup        ov_stun

        @brief          Definition of some required grammar used for STUN,
                        which is implemented inline.

        ------------------------------------------------------------------------
*/
#ifndef ov_stun_grammar_h
#define ov_stun_grammar_h

#include <ctype.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ov_base/ov_utf8.h>

/**
        Offset a pointer with linear whitespace.
        LWS  =  [*WSP CRLF] 1*WSP ; linear whitespace

        @param start    start of the buffer
        @param length   length of the buffer
        @param next     pointer to first non linear whitespace
        @returns        true if next was set
        @returns        true is no linear whitespace to offset
        @returns        false if buffer is whitespace only
*/
bool ov_stun_grammar_offset_linear_whitespace(const char *start,
                                              size_t length,
                                              char **next);

/*----------------------------------------------------------------------------*/

/**
        Check if a buffer fit the grammar of RFC 3261 (SIP) for
        qd text.

        qdtext         =  LWS / %x21 / %x23-5B / %x5D-7E
                        / UTF8-NONASCII
*/
bool ov_stun_grammar_is_qdtext(const uint8_t *start, size_t length);

/*----------------------------------------------------------------------------*/

/**
        Check if a buffer fit the grammar of RFC 3261 (SIP) for
        quoted pair

        quoted-pair  =  "\" (%x00-09 / %x0B-0C
                / %x0E-7F)

*/
bool ov_stun_grammar_is_quoted_pair(const uint8_t *start, size_t length);

/*----------------------------------------------------------------------------*/

/**
        Check if a buffer contains only qdtext or quoted pairs.

        *(qdtext / quoted-pair )
*/
bool ov_stun_grammar_is_quoted_string_content(const uint8_t *start,
                                              size_t length);

/*----------------------------------------------------------------------------*/

/**
        Check if a buffer fit the grammar of RFC 3261 (SIP) for
        qd text.

        quoted-string  =  SWS DQUOTE *(qdtext / quoted-pair ) DQUOTE
*/
bool ov_stun_grammar_is_quoted_string(const uint8_t *start, size_t length);

#endif /* ov_stun_grammar_h */
