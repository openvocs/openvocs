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
        @file           ov_json_grammar.c
        @author         Markus Toepfer

        @date           2018-12-03

        @ingroup        ov_json_grammar

        @brief          Implementation of ov_json_grammar.


        ------------------------------------------------------------------------
*/
#include "../../include/ov_json_grammar.h"
#include <errno.h>
#include <stdio.h>

struct json_counter {

    uint64_t objects;
    uint64_t arrays;
};

static bool match_value(uint8_t *ptr, size_t size, struct json_counter *counter,
                        uint8_t **next);

static bool match_number(uint8_t *ptr, size_t size, uint8_t **next);

static bool match_literal(uint8_t *ptr, size_t size, uint8_t **next);

static bool match_array(uint8_t *start, size_t size,
                        struct json_counter *counter, uint8_t **next);

static bool match_object(uint8_t *start, size_t size,
                         struct json_counter *counter, uint8_t **next);

static bool match_string(uint8_t *ptr, size_t size, uint8_t **next);

/*----------------------------------------------------------------------------*/

bool ov_json_validate_string(const uint8_t *buffer, size_t size, bool quotes) {

    /*
     *      Validate a JSON string in terms of:
     *
     *                      0                   1
     *      bytes           0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6
     *                     +-+-----------------------------+-+
     *      text            \"s o m e   v a l i d   u t f 8 \"
     *                     +-+-----------------------------+-+
     *      quotes (true)   ^                               ^
     *                     +-+-----------------------------+-+
     *      quotes (false)    ^                           ^
     *
     *
     *      JSON escapes the following characters " \ / b f n r t u
     *
     *      \" |0x5C|0x22| or |\|"| or u\0022
     *      \/ |0x5C|0x2F| or |\|/| or u\002F
     *      \b |0x5C|0x62| or |\|b| or u\0062
     *      \f |0x5C|0x66| or |\|f| or u\0066
     *      \n |0x5C|0x6E| or |\|n| or u\006E
     *      \r |0x5C|0x72| or |\|r| or u\0072
     *      \t |0x5C|0x74| or |\|t| or u\0074
     *
     *      \uXXXX |0x5C|0x75|
     *
     */

    if (!buffer || size < 1)
        return false;

    if (!ov_utf8_validate_sequence(buffer, size))
        return false;

    size_t check = 0;

    if (quotes) {

        if (size < 2)
            return false;

        if (buffer[0] != 0x22)
            return false;

        if (buffer[size - 1] != 0x22)
            return false;

        check++;
        size--;
    }

    while (check < size) {

        // check for escape sign backslash \ (0x5C)
        if (buffer[check] == 0x5C) {

            // check sign following backslash
            if (check == size - 1)
                return false;

            switch (buffer[check + 1]) {
            case 0x22: // "    quotation mark  U+0022
            case 0x5C: // \    reverse solidus U+005C
            case 0x2F: // /    solidus         U+002F
            case 0x62: // b    backspace       U+0008
            case 0x66: // f    form feed       U+000C
            case 0x6E: // n    line feed       U+000A
            case 0x72: // r    carriage return U+000D
            case 0x74: // t    tab             U+0009
                // one byte escape, skip checking next
                if (check + 2 > size)
                    return false;
                check += 2;
                break;
            case 0x75: // uXXXX                U+XXXX
                if (check + 6 > size)
                    return false;
                // 4 byte unicode escape,
                // skip checking \uXXXX sequence
                check += 6;
                break;
            default:
                return false;
            }

        } else if (buffer[check] < 0x1F) {

            // unescaped control character
            return false;

        } else if (buffer[check] == 0x22) {

            // unescaped string
            return false;

        } else {
            check++;
        }
    }

    return true;
}

/*----------------------------------------------------------------------------*/

bool ov_json_match_string(uint8_t **start, uint8_t **end, size_t size) {

    /*
     *      Match a JSON string within a buffer.
     *
     *                      0                   1
     *      bytes           0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6
     *                     +-------+---------------+----------
     *      buffer          \n \t  \" s t r i n g \"
     *                    ------------------------------------
     *
     *      start (init)    ^
     *      start (success) __________^
     *      start (failure) ^
     *      end   (init)    NULL
     *      end   (success) ____________________^
     *      end   (failure) NULL
     */

    if (!start || !end || size < 2)
        return false;

    if (!*start)
        return false;

    char *in = (char *)*start;
    char *ptr = (char *)*start;

    while (ov_json_is_whitespace(ptr[0])) {

        if (size < 1)
            goto error;

        ptr++;
        size--;
    }

    if (size < 2)
        goto error;

    // need to start with
    if (ptr[0] != 0x22)
        goto error;

    size--;
    ptr++;

    *start = (uint8_t *)ptr;

    while (size > 1) {

        if (ptr[0] == 0x22) {

            if (ptr[-1] != 0x5c)
                break;
        }

        size--;
        ptr++;
    }

    if (ptr[0] != 0x22)
        goto error;

    *end = (uint8_t *)(ptr - 1);

    /* check for at least ONE byte (char) between start and end */
    if ((*end - *start) < 0)
        goto error;

    if (ov_json_validate_string(*start, ((*end - *start) + 1), false))
        return true;

    /* run into error and reset *start to input on non valid content */
error:
    *start = (uint8_t *)in;
    *end = NULL;
    return false;
}

/*---------------------------------------------------------------------------*/

bool ov_json_is_whitespace(uint8_t byte) {

    switch (byte) {
    case 0x20:
    case 0x09:
    case 0x0A:
    case 0x0D:
        return true;
    }
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_json_clear_whitespace(uint8_t **buffer, size_t *length) {

    if (!buffer || !*buffer || !length)
        return false;

    uint8_t *ptr = *buffer;
    int64_t len = *length;

    if (len < 1)
        return true;

    while (ov_json_is_whitespace((uint8_t)ptr[0])) {

        ptr++;
        len--;

        if (len == 0)
            break;
    }

    *buffer = ptr;
    *length = len;
    return true;
}

/*----------------------------------------------------------------------------*/

bool ov_json_match_array(uint8_t **start, uint8_t **end, size_t size) {

    /*
     *      Match a JSON array within a buffer.
     *
     *                        0                   1
     *      bytes           0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6
     *                     +-------+---------------+----------
     *      buffer          \n \t   [ .., .., ..  ]
     *                     -----------------------------------
     *      start (init)    ^
     *      start (success) __________^
     *      start (failure) ^
     *      end   (init)    NULL
     *      end   (success) ___________________^
     *      end   (failure) NULL
     */

    if (!start || !end || size < 2)
        return false;

    if (!*start)
        return false;

    char *in = (char *)*start;
    char *ptr = (char *)*start;

    size_t child_open = 0;

    // point to first non whitespace char
    while (ov_json_is_whitespace(ptr[0])) {

        if (size < 1)
            goto error;

        ptr++;
        size--;
    }

    if (ptr[0] != '[')
        goto error;

    ptr++;
    size--;

    *start = (uint8_t *)ptr;

    while (size > 1) {

        switch (ptr[0]) {

        case '[':
            child_open++;
            break;
        case ']':

            if (child_open == 0)
                goto done;

            child_open--;
            break;
        }

        size--;
        ptr++;
    }

done:
    if (ptr[0] != ']')
        goto error;

    if (child_open != 0)
        goto error;

    *end = (uint8_t *)ptr - 1;
    return true;

    /* run into error and reset *start to input on non valid content */
error:
    *start = (uint8_t *)in;
    *end = NULL;
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_json_match_object(uint8_t **start, uint8_t **end, size_t size) {

    /*
     *      Match a JSON object within a buffer.
     *
     *                        0                   1
     *      bytes           0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6
     *                     +-------+---------------+----------
     *      buffer          \n \t   { .., .., ..  }
     *                     -----------------------------------
     *      start (init)    ^
     *      start (success) __________^
     *      start (failure) ^
     *      end   (init)    NULL
     *      end   (success) ___________________^
     *      end   (failure) NULL
     */

    if (!start || !end || size < 2)
        return false;

    if (!*start)
        return false;

    char *in = (char *)*start;
    char *ptr = (char *)*start;

    uint64_t child_open = 0;

    // point to first non whitespace char
    while (ov_json_is_whitespace(ptr[0])) {

        if (size < 1)
            goto error;

        ptr++;
        size--;
    }

    if (ptr[0] != '{')
        goto error;

    ptr++;
    size--;

    *start = (uint8_t *)ptr;

    while (size > 1) {

        switch (ptr[0]) {

        case '{':
            child_open++;
            break;
        case '}':

            if (child_open == 0)
                goto done;

            child_open--;
            break;
        }

        size--;
        ptr++;
    }

done:
    if (ptr[0] != '}')
        goto error;

    if (child_open != 0)
        goto error;

    *end = (uint8_t *)ptr - 1;
    return true;

    /* run into error and reset *start to input on non valid content */
error:
    *start = (uint8_t *)in;
    *end = NULL;
    return false;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      MATCHING ONLY
 *
 *      ------------------------------------------------------------------------
 */

/*---------------------------------------------------------------------------*/

bool match_string(uint8_t *ptr, size_t size, uint8_t **next) {

    if (!ptr || size < 1 || !next)
        goto error;

    if (ptr[0] != '"')
        goto error;

    size--;

    if (0 == size) {
        *next = ptr + 1;
        return true;
    }

    uint8_t *nxt = ptr + 1;
    // try to find counter double quote
    while (size > 1) {

        if (nxt[0] == 0x22) {

            if (nxt[-1] != 0x5c)
                break;
        }

        size--;
        nxt++;
    }

    // not enough data received to validate string,
    // but parsed up to nxt
    *next = nxt + 1;

    if (nxt[0] == 0x22) {

        if (!ov_json_validate_string(ptr, (nxt - ptr + 1), true))
            goto error;
    }

    return true;
error:
    return false;
}

/*---------------------------------------------------------------------------*/

bool match_object(uint8_t *start, size_t size, struct json_counter *counter,
                  uint8_t **next) {

    if (!start || size < 1 || !counter || !next)
        goto error;

    if (start[0] != '{')
        goto error;

    counter->objects++;

    if (size == 1)
        goto done;

    int64_t length = size - 1;
    *next = start + 1;

    // check for empty object
    while (ov_json_is_whitespace(*next[0])) {

        *next = *next + 1;
        length--;

        if (length < 1)
            goto done;
    }

    if (length < 1)
        goto done;

    if (*next[0] == '}') {
        counter->objects--;
        *next = *next + 1;
        goto done;
    }

    while (length > 0) {

        // allow whitespace before the key

        while (ov_json_is_whitespace(*next[0])) {
            *next = *next + 1;
            length--;
            if (length == 0)
                goto done;
        }

        if (!match_string(*next, length, next))
            goto error;

        length = size - (*next - start);
        if (length < 1)
            break;

        // allow whitespace between key string and separator

        while (ov_json_is_whitespace(*next[0])) {
            *next = *next + 1;
            length--;
            if (length == 0)
                goto done;
        }

        if (length < 1)
            goto done;

        // match the separator

        if (*next[0] != ':')
            goto error;

        *next = *next + 1;
        length--;

        if (length < 1)
            goto done;

        // match the value

        if (!match_value(*next, length, counter, next))
            goto error;

        if (!next || !*next)
            goto error;

        length = size - (*next - start);
        if (length < 1)
            break;

        // allow whitespace between value and separator

        while (ov_json_is_whitespace(*next[0])) {
            *next = *next + 1;
            length--;
            if (length == 0)
                goto done;
        }

        switch (*next[0]) {

        case ',':
            *next = *next + 1;
            length--;
            break;

        case '}':
            *next = *next + 1;
            counter->objects--;
            goto done;
            break;

        case '{':
            if (1 == length)
                goto done;
            break;

        case '[':
            if ((1 == length) && (counter->objects > 0))
                goto done;
            break;

        default:
            goto error;
        }
    }

done:
    return true;
error:
    return false;
}

/*---------------------------------------------------------------------------*/

bool match_array(uint8_t *start, size_t size, struct json_counter *counter,
                 uint8_t **next) {

    if (!start || size < 1 || !counter || !next)
        goto error;

    int64_t length = size;

    if (start[0] != '[')
        goto error;

    counter->arrays++;

    if (size == 1)
        goto done;

    length--;
    *next = start + 1;

    // check for empty array
    while (ov_json_is_whitespace(*next[0])) {
        *next = *next + 1;
        length--;
        if (length < 1)
            goto done;
    }

    if (length < 1)
        goto done;

    if (*next[0] == ']') {
        *next = *next + 1;
        counter->arrays--;
        goto done;
    }

    while (length > 0) {

        if (!match_value(*next, length, counter, next))
            goto error;

        if (!next || !*next)
            goto error;

        length = size - (*next - start);
        if (length < 1)
            break;

        while (ov_json_is_whitespace(*next[0])) {
            *next = *next + 1;
        }

        length = size - (*next - start);
        if (length < 1)
            break;

        switch (*next[0]) {

        case ',':
            *next = *next + 1;
            length--;
            break;

        case ']':
            *next = *next + 1;
            counter->arrays--;
            goto done;
            break;

        case '[':
            if (1 == length)
                goto done;
            break;

        case '{':
            if ((1 == length) && (counter->arrays > 0))
                goto done;
            break;

        default:
            goto error;
        }
    }

done:
    return true;
error:
    return false;
}

/*---------------------------------------------------------------------------*/

bool match_literal(uint8_t *ptr, size_t size, uint8_t **next) {

    if (!ptr || size < 1 || !next)
        goto error;

    switch (ptr[0]) {

    case 'n':

        if (size > 4)
            size = 4;

        if (0 != strncmp((char *)ptr, "null", size))
            goto error;

        break;

    case 't':

        if (size > 4)
            size = 4;

        if (0 != strncmp((char *)ptr, "true", size))
            goto error;

        break;

    case 'f':

        if (size > 5)
            size = 5;

        if (0 != strncmp((char *)ptr, "false", size))
            goto error;

        break;

    default:
        goto error;
    }

    *next = ptr + size;
    return true;
error:
    return false;
}

/*---------------------------------------------------------------------------*/

bool match_number(uint8_t *ptr, size_t size, uint8_t **next) {

    if (!ptr || size < 1 || !next)
        goto error;

    /* CHECK start */
    switch (ptr[0]) {

    case '-':
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
        break;

    default:
        goto error;
    }

    errno = 0;
    double number = strtod((char *)ptr, (char **)next);
    if (errno != 0)
        goto error;

    if (number == 0) { /* unused value */
    }

    if ((*next - ptr) > (ssize_t)size)
        *next = ptr + size;

    return true;
error:
    return false;
}

/*---------------------------------------------------------------------------*/

bool match_value(uint8_t *ptr, size_t size, struct json_counter *counter,
                 uint8_t **next) {

    if (!ptr || size < 1 || !counter || !next)
        goto error;

    int64_t length = (int64_t)size;

    *next = ptr;

    // clear intro whitespace
    while (ov_json_is_whitespace(*next[0])) {

        *next = *next + 1;
        length--;
    }

    if (length < 1)
        goto done;

    switch (*next[0]) {

    case '{':

        if (!match_object(*next, length, counter, next))
            goto error;
        break;

    case '[':

        if (!match_array(*next, length, counter, next))
            goto error;
        break;

    case '"':

        if (!match_string(*next, length, next))
            goto error;
        break;

    case 'n':
    case 't':
    case 'f':

        if (!match_literal(*next, length, next))
            goto error;
        break;

    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    case '0':
    case '-':

        if (!match_number(*next, length, next))
            goto error;
        break;

    default:
        // some unallowed character parsed
        goto error;
    }

done:
    return true;
error:
    return false;
}

/*---------------------------------------------------------------------------*/

bool ov_json_match(const uint8_t *start, size_t size, bool incomplete,
                   uint8_t **last_of_first) {

    if (!start || size < 1)
        goto error;

    uint8_t *last = NULL;

    if (last_of_first)
        *last_of_first = NULL;

    uint8_t *ptr = NULL;
    uint8_t *next = (uint8_t *)start;
    int64_t length = size;

    struct json_counter counter = {0};

    while (length > 0) {

        ptr = next;

        if (!match_value(ptr, length, &counter, &next))
            goto error;

        length = size - (next - start);

        if (!last && (ptr != next))
            last = next - 1;

        if (ptr == next)
            break;

        ov_json_clear_whitespace(&next, (size_t *)&length);
    }

    if (last_of_first)
        *last_of_first = last;

    if (!incomplete) {

        if (0 != length)
            goto error;

        if ((0 != counter.objects) || (0 != counter.arrays)) {

            if (last && last_of_first)
                if (last == next - 1)
                    *last_of_first = NULL;

            goto error;
        }
    }

    return true;
error:
    return false;
}
