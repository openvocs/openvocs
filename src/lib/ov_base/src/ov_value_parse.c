/***
        ------------------------------------------------------------------------

        Copyright (c) 2021 German Aerospace Center DLR e.V. (GSOC)

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

        @author         Michael J. Beer, DLR/GSOC

        ------------------------------------------------------------------------
*/

#include "../include/ov_value_parse.h"
#include "../include/ov_utils.h"
#include <../include/ov_buffer.h>
#include <stdio.h>

/*----------------------------------------------------------------------------*/

static char const SEPARATOR_CHARS[] = " \r\n\t";

static char const *skip_chars(char const *in, char const *skip_chars,
                              size_t len) {

    OV_ASSERT(0 != in);
    OV_ASSERT(0 != skip_chars);

    size_t i = 0;

    for (; i < len; ++i) {

        if (0 == strchr(skip_chars, in[i]))
            break;
    }

    return in + i;
}

/*----------------------------------------------------------------------------*/

typedef ov_value *(*ParseFunc)(char const *in, size_t len,
                               char const **remainder, char const **errormsg);

/*----------------------------------------------------------------------------*/

static char const *PREMATURE_END_OF_INPUT = "Premature end of input";

static ov_value *parse_next_token(char const *in, size_t len,
                                  char const **remainder,
                                  char const **errormsg);

#define CHECK_INPUT(in, len, min_len, remainder, errormsg)                     \
    OV_ASSERT(0 != in);                                                        \
    OV_ASSERT(0 != remainder);                                                 \
    OV_ASSERT(0 != errormsg);                                                  \
    if (len < min_len) {                                                       \
        *errormsg = PREMATURE_END_OF_INPUT;                                    \
        goto error;                                                            \
    }

/*----------------------------------------------------------------------------*/

static ov_value *parse_null(char const *in, size_t len, char const **remainder,
                            char const **errormsg) {

    CHECK_INPUT(in, len, 4, remainder, errormsg);

    if (0 != memcmp(in, "null", 4))
        goto error;

    *remainder = in + 4;

    return ov_value_null();

error:

    *errormsg = "Unrecognized token";
    return 0;
}

/*----------------------------------------------------------------------------*/

static ov_value *parse_true(char const *in, size_t len, char const **remainder,
                            char const **errormsg) {

    CHECK_INPUT(in, len, 4, remainder, errormsg);

    if (0 != memcmp(in, "true", 4))
        goto error;

    *remainder = in + 4;

    return ov_value_true();

error:

    *errormsg = "Unrecognized token";
    return 0;
}

/*----------------------------------------------------------------------------*/

static ov_value *parse_false(char const *in, size_t len, char const **remainder,
                             char const **errormsg) {

    CHECK_INPUT(in, len, 5, remainder, errormsg);

    if (0 != memcmp(in, "false", 5))
        goto error;

    *remainder = in + 5;

    return ov_value_false();

error:

    *errormsg = "unrecognized token";
    return 0;
}

/*****************************************************************************
                                     String
 ****************************************************************************/

static size_t copy_string_strip_escapes(char *restrict to,
                                        char const *restrict from, size_t len) {

    OV_ASSERT(0 != to);
    OV_ASSERT(0 != from);

    bool escaped = false;
    size_t chars_in_copy = 0;

    for (size_t i = 0; i < len; ++i, ++from) {

        if ((!escaped) && ('\\' == *from)) {
            escaped = true;
            continue;
        }

        if (escaped) {
            escaped = false;
        }

        *to = *from;
        ++to;
        ++chars_in_copy;
    }

    return chars_in_copy;
}

/*----------------------------------------------------------------------------*/

static ov_value *parse_string(char const *in, size_t len,
                              char const **remainder, char const **errormsg) {

    CHECK_INPUT(in, len, 2, remainder, errormsg);

    char const *ptr = in;

    size_t string_len = 0;

    bool escaped = false;

    // Skip initial '"'
    for (size_t i = 1; len > i; ++i, ++string_len) {

        if (escaped && ('"' == ptr[i])) {
            escaped = false;
            --string_len;
            continue;
        }

        if (escaped && ('\\' == ptr[i])) {
            escaped = false;
            --string_len;
            continue;
        }

        if (escaped) {
            *errormsg = "Unknown escape sequence encountered";
            return 0;
        }

        OV_ASSERT(!escaped);

        if ('\\' == ptr[i]) {

            escaped = true;
            continue;
        }

        if ('"' != ptr[i])
            continue;

        OV_ASSERT((!escaped) && ('"' == ptr[i]));

        // Space for the terminal 0
        string_len += 1;

        // We found the terminal "
        // We use buffer instead of plain char *, because
        // buffers are cached and might not be allocated
        ov_buffer *buf = ov_buffer_create(string_len);

        OV_ASSERT((0 != buf) && (string_len <= buf->capacity));

        size_t chars_in_copy =
            copy_string_strip_escapes((char *)buf->start, in + 1, i - 1);

        buf->start[string_len - 1] = 0;

        OV_ASSERT(1 + chars_in_copy == string_len);

        ov_value *result = ov_value_string((char *)buf->start);

        buf = ov_buffer_free(buf);
        OV_ASSERT(0 == buf);

        *remainder = ptr + i + 1;
        return result;
    }

    *errormsg = PREMATURE_END_OF_INPUT;

error:

    return 0;
}

/*****************************************************************************
                                     Number
 ****************************************************************************/

static ov_value *parse_number(char const *in, size_t len,
                              char const **remainder, char const **errormsg) {

    ov_buffer *buf = 0;

    CHECK_INPUT(in, len, 2, remainder, errormsg);

    /* Find length of token */
    size_t i = 0;

    for (i = 0; i < len; ++i) {

        if (0 != strchr(SEPARATOR_CHARS, *in))
            break;
    }

    if (0 == i) {

        *errormsg = "Empty number";
        goto error;
    }

    // We need to zero-terminate the 'in' content
    // buffers are cached, thus refrain from using plain char *
    buf = ov_buffer_create(i + 1);
    memcpy(buf->start, in, i);
    buf->start[i] = 0;

    char *rem = 0;

    double number = strtod((char const *)buf->start, &rem);

    if (0 == rem) {

        *errormsg = "Not a number";
        ov_log_error("Not a number: %s", buf);
        goto error;
    }

    ov_value *result = ov_value_number(number);

    ptrdiff_t remainder_offset = rem - (char *)buf->start;

    *remainder = in + remainder_offset;

    buf = ov_buffer_free(buf);
    OV_ASSERT(0 == buf);

    return result;

error:

    buf = ov_buffer_free(buf);
    OV_ASSERT(0 == buf);

    return 0;
}

/*****************************************************************************
                                      LIST
 ****************************************************************************/

static ov_value *parse_list(char const *in, size_t len, char const **remainder,
                            char const **errormsg) {

    ov_value *list = ov_value_list(0);

    CHECK_INPUT(in, len, 2, remainder, errormsg);

    bool require_comma = false;

    // Skip initial '['
    char const *ptr = in + 1;
    size_t l = len - 1;

    while (0 < l) {

        char const *start_of_token = skip_chars(ptr, SEPARATOR_CHARS, l);

        l = len - (start_of_token - in);

        if (0 == l) {

            *errormsg = PREMATURE_END_OF_INPUT;
            goto error;
        }

        if (']' == *start_of_token) {

            ptr = start_of_token + 1;
            goto finish;
        }

        if (require_comma && (',' != *start_of_token)) {

            *errormsg = "Invalid character in list";
            goto error;
        }

        if (require_comma) {
            ++start_of_token;
            --l;
        }

        if (0 == l) {

            *errormsg = PREMATURE_END_OF_INPUT;
            goto error;
        }

        ov_value *entry = parse_next_token(start_of_token, l, &ptr, errormsg);

        if (0 != *errormsg) {
            goto error;
        }

        OV_ASSERT(0 != entry);

        if (!ov_value_list_push(list, entry)) {
            *errormsg = "Could not create list";
            goto error;
        }

        l = len - (ptr - in);

        // We parsed at least one entry - next entry must be preceded by a comma
        require_comma = true;
    };

finish:

    *remainder = ptr;
    return list;

error:

    list = ov_value_free(list);
    OV_ASSERT(0 == list);

    return 0;
}

/*****************************************************************************
                                     OBJECT
 ****************************************************************************/

typedef struct {

    ov_value *key;
    ov_value *value;

} kv_pair;

static kv_pair parse_kv_pair(char const *in, size_t len, char const **remainder,
                             char const **errormsg) {

    kv_pair pair = {0};

    size_t rem_len = len;

    CHECK_INPUT(in, len, 2, remainder, errormsg);

    char const *ptr = 0;

    pair.key = parse_next_token(in, rem_len, &ptr, errormsg);

    if (0 != *errormsg)
        goto error;

    if (0 == ov_value_get_string(pair.key)) {

        *errormsg = "First element of an key value pair must be a string";
        goto error;
    }

    OV_ASSERT(0 != ptr);

    rem_len -= ptr - in;

    ptr = skip_chars(ptr, SEPARATOR_CHARS, rem_len);

    rem_len = len - (ptr - in);

    if (0 == rem_len) {
        *errormsg = PREMATURE_END_OF_INPUT;
        goto error;
    }

    if (':' != *ptr) {

        *errormsg = "Invalid char found in key value pair";
        goto error;
    }

    ++ptr;

    --rem_len;

    ov_value *value = parse_next_token(ptr, rem_len, remainder, errormsg);

    if (0 == value) {

        goto error;
    }

    pair.value = value;

    return pair;

error:

    pair.key = ov_value_free(pair.key);
    OV_ASSERT(0 == pair.key);

    return pair;
}

/*----------------------------------------------------------------------------*/

static ov_value *parse_object(char const *in, size_t len,
                              char const **remainder, char const **errormsg) {

    ov_value *object = ov_value_object();

    CHECK_INPUT(in, len, 2, remainder, errormsg);

    bool require_comma = false;

    // Skip initial '{'
    char const *ptr = in + 1;
    size_t l = len - 1;

    while (0 < l) {

        char const *start = skip_chars(ptr, SEPARATOR_CHARS, l);

        l = len - (start - in);

        if (0 == l) {
            goto premature_end_of_input;
        }

        if ('}' == *start) {

            ptr = start + 1;
            goto finish;
        }

        if (require_comma && (',' != *start)) {

            *errormsg = "Invalid character in list";
            goto error;
        }

        if (require_comma) {
            ++start;
            --l;
        }

        if (0 == l) {
            goto premature_end_of_input;
        }

        kv_pair pair = parse_kv_pair(start, l, &ptr, errormsg);

        if (0 != *errormsg) {
            goto error;
        }

        char const *key_str = ov_value_get_string(pair.key);

        OV_ASSERT(0 != key_str);

        if (0 != ov_value_object_set(object, key_str, pair.value)) {
            *errormsg = "Could not create object";
            goto error;
        }

        key_str = 0;

        pair.key = ov_value_free(pair.key);
        pair.value = 0;

        l = len - (ptr - in);

        // We parsed at least one entry - next entry must be preceded by a comma
        require_comma = true;
    };

    // we left the loop without branching to closing_bracket_found ->
    // closing bracket is missing

premature_end_of_input:

    *errormsg = PREMATURE_END_OF_INPUT;

error:

    object = ov_value_free(object);
    OV_ASSERT(0 == object);

    return 0;

finish:

    *remainder = ptr;
    return object;
}

/*****************************************************************************
                                  Main parser
 ****************************************************************************/

static ParseFunc get_parse_func_for(char c) {

    switch (c) {

    case 'n':
        return parse_null;
    case 't':
        return parse_true;
    case 'f':
        return parse_false;
    case '"':
        return parse_string;
    case '0':
        return parse_number;
    case '1':
        return parse_number;
    case '2':
        return parse_number;
    case '3':
        return parse_number;
    case '4':
        return parse_number;
    case '5':
        return parse_number;
    case '6':
        return parse_number;
    case '7':
        return parse_number;
    case '8':
        return parse_number;
    case '9':
        return parse_number;
    case '.':
        return parse_number;
    case '-':
        return parse_number;
    case '+':
        return parse_number;
    case '[':
        return parse_list;

    case '{':
        return parse_object;

    default:
        return 0;
    };

    return 0;
}

/*----------------------------------------------------------------------------*/

static ov_value *parse_next_token(char const *in, size_t len,
                                  char const **remainder,
                                  char const **errormsg) {

    OV_ASSERT(0 != in);
    OV_ASSERT(0 != remainder);
    OV_ASSERT(0 != errormsg);

    char const *start_token = skip_chars(in, SEPARATOR_CHARS, len);

    size_t new_len = len - (start_token - in);

    if (0 == new_len) {

        *errormsg = PREMATURE_END_OF_INPUT;
        goto error;
    }

    ParseFunc func = get_parse_func_for(*start_token);

    if (0 == func) {

        *errormsg = "Invalid character encountered";

        goto error;
    }

    return func(start_token, new_len, remainder, errormsg);

error:

    return 0;
}

/*----------------------------------------------------------------------------*/

ov_value *ov_value_parse(ov_buffer const *in, char const **remainder) {

    ov_value *value = 0;
    char const *rem = 0;

    if ((0 == in) || (0 == in->start)) {

        ov_log_error("No input to parse");
        goto error;
    }

    if (0 == in->length) {
        ov_log_warning("Parsing failed: Premature end of input");
        rem = (char *)in->start;
        goto premature_end_of_input;
    }

    char const *errormsg = 0;

    value =
        parse_next_token((char const *)in->start, in->length, &rem, &errormsg);

    if ((0 != errormsg) && (PREMATURE_END_OF_INPUT != errormsg)) {

        ov_log_error("Parsing failed: %s", errormsg);
        goto error;
    }

    if (PREMATURE_END_OF_INPUT == errormsg) {

        ov_log_warning("Parsing failed: Premature end of input");
        rem = (char *)in->start;
    }

premature_end_of_input:

    if (0 != remainder) {

        *remainder = rem;
    }

    return value;

error:

    if (0 != remainder)
        *remainder = 0;

    return 0;
}

/*----------------------------------------------------------------------------*/

bool ov_value_parse_stream(char const *in, size_t in_len_bytes,
                           void (*value_consumer)(ov_value *, void *),
                           void *userdata, char const **remainder) {

    if (0 == value_consumer)
        goto error;
    if (0 == in)
        goto error;
    if (0 == in_len_bytes)
        goto error;

    ov_value *value = 0;

    ov_buffer to_parse = {
        .start = (uint8_t *)in,
        .length = in_len_bytes,
    };

    char const *rem = 0;

    value = ov_value_parse(&to_parse, &rem);

    if (0 == rem)
        goto error;

    while (0 != value) {

        value_consumer(value, userdata);

        to_parse.start = (uint8_t *)rem;
        to_parse.length = in_len_bytes - (rem - in);

        rem = 0;

        value = ov_value_parse(&to_parse, &rem);

        if (0 == rem)
            goto error;
    };

    if (0 != remainder)
        *remainder = rem;

    return true;

error:

    if (0 != remainder)
        *remainder = 0;

    return false;
}

/*----------------------------------------------------------------------------*/
