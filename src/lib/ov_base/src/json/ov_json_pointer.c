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
        @file           ov_json_pointer.c
        @author         Markus Toepfer

        @date           2018-12-05

        @ingroup        ov_json_pointer

        @brief          Implementation of RFC 6901
                        "JavaScript Object Notation (JSON) Pointer"

        ------------------------------------------------------------------------
*/
#include "../../include/ov_json_pointer.h"
#include "../../include/ov_utf8.h"

#include <ov_log/ov_log.h>

/*----------------------------------------------------------------------------*/

static ov_json_value *json_object_get_with_length(ov_json_value *object,
                                                  const char *key,
                                                  size_t length) {

    if (!object || !key || length < 1) return NULL;

    char buffer[length + 1];
    memset(buffer, 0, length + 1);

    if (!strncat(buffer, key, length)) goto error;

    return ov_json_object_get(object, buffer);
error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

static bool json_pointer_parse_token(const char *start,
                                     size_t size,
                                     char **token,
                                     size_t *length) {

    if (!start || size < 1 || !token || !length) return false;

    if (start[0] != '/') goto error;

    if (size == 1) {
        *token = (char *)start;
        return true;
    }

    *token = (char *)start + 1;

    size_t len = 0;

    while (len < (size - 1)) {

        switch ((*token + len)[0]) {

            case '/':
            case '\0':
                goto done;
                break;

            default:
                len++;
        }
    }

done:
    *length = len;
    return true;

error:
    *token = NULL;
    *length = 0;
    return false;
}

/*----------------------------------------------------------------------------*/

static bool json_pointer_replace_special_encoding(char *result,
                                                  size_t size,
                                                  char *source,
                                                  char *to_replace,
                                                  char *replacement) {

    if (!result || !source || !to_replace || !replacement || size < 1)
        return false;

    size_t length_replace = strlen(to_replace);
    size_t length_replacement = strlen(replacement);

    uint64_t i = 0;
    uint64_t k = 0;
    uint64_t r = 0;

    for (i = 0; i < size; i++) {

        if (source[i] != to_replace[0]) {
            result[r] = source[i];
            r++;
            continue;
        }

        if ((i + length_replace) > size) goto error;

        if ((i + length_replacement) > size) goto error;

        for (k = 0; k < length_replace; k++) {

            if (source[i + k] != to_replace[k]) break;
        }

        if (k == (length_replace)) {

            if (!memcpy(result + r, replacement, length_replacement))
                goto error;

            i += length_replace - 1;
            r += length_replacement;

        } else {

            // not a full match
            result[r] = source[i];
            r++;
        }
    }

    return true;
error:
    if (result) {
        memset(result, '\0', size);
    }

    return false;
}

/*----------------------------------------------------------------------------*/

static bool json_pointer_escape_token(char *start,
                                      size_t size,
                                      char *token,
                                      size_t length) {

    if (!start || size < 1 || !token || length < size) return false;

    // replacements ~1 -> \  and ~0 -> ~ (both smaller then source)

    char replace1[size + 1];
    memset(replace1, '\0', size + 1);

    if (!json_pointer_replace_special_encoding(
            replace1, size, (char *)start, "~1", "\\"))
        return false;

    if (!json_pointer_replace_special_encoding(
            (char *)token, size, replace1, "~0", "~"))
        return false;

    return true;
}

/*----------------------------------------------------------------------------*/

static ov_json_value *json_pointer_get_token_in_parent(ov_json_value *parent,
                                                       char *token,
                                                       size_t length) {

    if (!parent || !token) return NULL;

    if (length == 0) return parent;

    ov_json_value *result = NULL;
    char *ptr = NULL;
    uint64_t number = 0;

    switch (parent->type) {

        case OV_JSON_OBJECT:

            // token is the keyname
            result = json_object_get_with_length(parent, token, length);

            break;

        case OV_JSON_ARRAY:

            if (token[0] == '-') {

                // new array member after last array element
                if (strnlen((char *)token, length) == 1) {

                    result = ov_json_null();

                    if (!ov_json_array_push(parent, result))
                        result = ov_json_value_free(result);
                }

            } else {

                // parse for INT64
                number = strtoll((char *)token, &ptr, 10);
                if (ptr[0] != '\0') return NULL;

                result = ov_json_array_get(parent, number + 1);
            }

            break;

        default:
            return NULL;
    }

    return result;
}

/*----------------------------------------------------------------------------*/

static ov_json_value *ov_json_get_pointer(const ov_json_value *root,
                                          const char *buffer,
                                          size_t size) {

    if (!root || !buffer) return NULL;

    // empty buffer is the root document
    if (size < 1) return (ov_json_value *)root;

    ov_json_value *result = NULL;

    bool parsed = false;

    char *parse = NULL;
    size_t length = 0;

    char token[size + 1];
    memset(token, 0, size + 1);
    char *token_ptr = token;

    const char *ptr = buffer;

    result = (ov_json_value *)root;

    while (json_pointer_parse_token(ptr, size, &parse, &length)) {

        if (length == 0) break;

        parsed = true;
        ptr = ptr + length + 1;

        memset(token, 0, size + 1);
        token_ptr = token;

        if (!json_pointer_escape_token(parse, length, token_ptr, length))
            goto error;

        result = json_pointer_get_token_in_parent(result, token_ptr, length);

        if (!result) goto error;
    }

    if (!parsed) goto error;

    return result;

error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_json_value const *ov_json_get(const ov_json_value *value,
                                 const char *pointer) {

    if (!value || !pointer) return NULL;

    return ov_json_get_pointer(value, pointer, strlen(pointer));
}
