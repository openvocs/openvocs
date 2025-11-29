/***
        ------------------------------------------------------------------------

        Copyright (c) 2025 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_item_json.c
        @author         Töpfer, Markus

        @date           2025-11-29


        ------------------------------------------------------------------------
*/
#include "../include/ov_item_json.h"

#include <errno.h>
#include <ctype.h>
#include <math.h>
#include <ov_base/ov_list.h>

#define ENCODING_STRING_NULL "null"
#define ENCODING_STRING_TRUE "true"
#define ENCODING_STRING_FALSE "false"

/*----------------------------------------------------------------------------*/

typedef struct {

    ov_item_json_stringify_config config;   // write config
    char *buffer;                           // buffer start
    size_t size;                            // buffer size
    size_t depth;                           // current item depth
    size_t counter;                         // length counter

    bool (*collocate_keys)(const char *key, ov_list *list);

} EncodingParameter;

/*----------------------------------------------------------------------------*/

static int64_t json_decode(ov_item **value,
                              const char *buffer,
                              size_t length);

/*----------------------------------------------------------------------------*/

static int64_t json_number_decode(ov_item **value,
                                  uint8_t *buffer,
                                  size_t size) {

    bool created = false;
    if (!value || !buffer || size < 1) goto error;

    char *ptr = NULL;
    double number = 0;

    /* number = [ minus ] int [ frac ] [ exp ] */

    /* CHECK start */
    switch (buffer[0]) {

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
    number = strtod((char *)buffer, &ptr);
    if (errno != 0) goto error;

    // testrun_log("\nNUMNER\t|\nSTART\t|%s\nEND\t%s\n", (char*) buffer,
    // (char*) ptr);

    if (ptr == (char *)buffer) goto error;

    // check parsing beyond length
    if ((uint64_t)((char *)ptr - (char *)buffer) > size) goto error;

    /* NOTE:    strtod breaks on space to check for wrong input
     *          of the form below, the best way is to check if the last char
     *          was a number. (positive exclusive check)
     *
     *          e.g     |0. 1|
     *                  |1e+ 1|
     *                  |1E |
     *
     *         last successfull parsed byte is either a digit, or any of
     *         .+-eE, where anything else but a digit is a failed result.
     */

    if (ptr[0] == ' ')
        if (!isdigit(ptr[-1])) goto error;
/*
    // Check next (MUST be some JSON closing token or whitespace)
    switch (ptr[0]) {
        case ',':
        case ']':
        case '}':
        case 0x20:
        case 0x09:
        case 0x0A:
        case 0x0D:
            break;

        default:
            goto error;
    }
*/
    if (!*value) {

        *value = ov_item_number(number);
        if (!*value) goto error;

        created = true;
    }

    if (!ov_item_set_number(*value, number)) goto error;

    return (ptr - (char *)buffer);
error:
    if (created) *value = ov_item_free(*value);
    return -1;
}

/*----------------------------------------------------------------------------*/

static int64_t json_literal_decode(ov_item **value,
                                   uint8_t *buffer,
                                   size_t size) {

    bool created = false;
    if (!value || !buffer || size < 4) goto error;

    if (*value) goto error;
    created = true;

    switch (buffer[0]) {

        case 'n':

            if (strncmp((char *)buffer, "null", 4) != 0) goto error;
            *value = ov_item_null();
            return 4;

            break;

        case 't':

            if (strncmp((char *)buffer, "true", 4) != 0) goto error;

            *value = ov_item_true();
            return 4;

            break;

        case 'f':

            if (size < 5) goto error;

            if (strncmp((char *)buffer, "false", 5) != 0) goto error;

            *value = ov_item_false();
            return 5;

            break;

        default:
            break;
    }

error:
    if (created) *value = ov_item_free(*value);
    return -1;
}

/*----------------------------------------------------------------------------*/

static int64_t json_string_decode(ov_item **value,
                                  uint8_t *buffer,
                                  size_t size) {

    if (!value || !buffer || size < 2) return -1;

    if (*value) return -1;

    if (buffer[0] != '"') return -1;

    uint8_t *start = (uint8_t *)buffer;
    uint8_t *end = NULL;

    if (!ov_json_match_string(&start, &end, size)) return -1;

    char string[end - start +1 ];
    memset(string, 0, end-start+1);

    strncpy(string, (char*)start, end - start);

    *value = ov_item_string(string);

    return ((end - start) + 3);
}


/*----------------------------------------------------------------------------*/

static int64_t json_array_decode(ov_item **value,
                                 uint8_t *buffer,
                                 size_t size) {

    bool created = false;
    if (!value || !buffer || size < 2) goto error;

    uint8_t *start = (uint8_t *)buffer;
    uint8_t *end = NULL;

    if (*value) goto error;

    if (!ov_json_match_array(&start, &end, size)) goto error;

    *value = ov_item_array();
    if (!*value) goto error;
    created = true;

    ov_item *child = NULL;
    uint8_t *content = start;

    int64_t len = 0;

    /* NOTE need to include the terminating ] for number end evaluations */
    size_t content_size = (end - start) + 2;

    // check whitespace only (empty array)
    if (!ov_json_clear_whitespace(&content, &content_size)) goto error;

    while (content_size > 1) {

        child = NULL;
        len = json_decode(&child, (char *)content, content_size);
        if (len < 0) goto error;

        if (!ov_item_array_push(*value, child)) {
            child = ov_item_free(child);
            goto error;
        }

        child = NULL;
        content_size -= len;
        content += len;

        // clear whitespace
        if (!ov_json_clear_whitespace(&content, &content_size)) goto error;

        if (content_size == 1) break;

        if (content[0] != ',') goto error;

        content++;
        content_size--;

        if (!ov_json_clear_whitespace(&content, &content_size)) goto error;
    }

    return ((end - buffer) + 2);
error:
    if (created) *value = ov_item_free(*value);
    return -1;
}

/*----------------------------------------------------------------------------*/

static bool json_object_set_with_length(ov_item *obj,
                                        const char *key,
                                        size_t length,
                                        ov_item *value) {

    if (!obj || !key || length < 1 || !value) return false;

    char buffer[length + 1];
    memset(buffer, 0, length + 1);

    if (!strncat(buffer, key, length)) goto error;

    return ov_item_object_set(obj, buffer, value);
error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

static int64_t json_object_decode(ov_item **value,
                                  uint8_t *buffer,
                                  size_t size) {

    bool created = false;
    if (!value || !buffer || size < 2) goto error;

    uint8_t *start = (uint8_t *)buffer;
    uint8_t *end = NULL;

    if (*value) goto error;

    if (!ov_json_match_object(&start, &end, size)) goto error;

    // testrun_log("\nOBJECT\t|%s\nSTART\t|%s\nEND\t%s\n", (char*) buffer,
    // (char*) start, (char*) end);

    // matched JSON object
    *value = ov_item_object();
    if (!*value) goto error;
    created = true;

    ov_item *child = NULL;
    uint8_t *ptr = NULL;
    uint8_t *key = NULL;
    uint8_t *content = start;

    uint64_t key_len = 0;

    int64_t len = 0;

    /* NOTE need to include the terminating } */
    size_t content_size = (end - start) + 2;

    // check whitespace only (empty object)
    if (!ov_json_clear_whitespace(&content, &content_size)) goto error;

    while (content_size > 1) {

        /* parse a key */
        key = content;

        if (!ov_json_match_string(&key, &ptr, content_size)) goto error;

        key_len = (ptr - key) + 1;

        // point to buffer behind \"
        content_size -= (ptr - content) + 1;
        content = key + key_len + 1;

        if (!ov_json_clear_whitespace(&content, &content_size)) goto error;

        if (content[0] != ':') goto error;

        content++;
        content_size--;

        if (!ov_json_clear_whitespace(&content, &content_size)) goto error;

        /* parse a value */
        child = NULL;
        len = json_decode(&child, (char *)content, content_size);
        if (len < 0) goto error;

        content_size -= len;
        content += len;

        // testrun_log("CHILD next %jd |%s\n", content_size, content);

        /* add key/value pair as member to object */

        /* Set new pair */
        if (!json_object_set_with_length(*value, (char *)key, key_len, child)) {
            child = ov_item_free(child);
            goto error;
        }

        key = NULL;
        child = NULL;

        if (!ov_json_clear_whitespace(&content, &content_size)) goto error;

        if (content[0] == '}') break;

        if (content[0] != ',') goto error;

        content += 1;
        content_size -= 1;
    }

    // content MUST point to closing bracket
    if (*content != '}') goto error;

    return ((end - buffer) + 2);
error:
    if (created) *value = ov_item_free(*value);
    return -1;
}

/*----------------------------------------------------------------------------*/

static int64_t json_decode(ov_item **value,
                              const char *buffer,
                              size_t length) {

    bool created = false;
    if (!value || !buffer || length < 1) goto error;

    size_t size = length;
    int64_t len = -1;

    uint8_t *ptr = (uint8_t *)buffer;

    if (!ov_json_clear_whitespace(&ptr, &size)) goto error;

    switch (ptr[0]) {

        case 0x7B: // ={ need to parse an object

            len = json_object_decode(value, ptr, size);
            break;

        case 0x5B: // =[ need to parse an array

            len = json_array_decode(value, ptr, size);
            break;

        case 0x22: // ="  need to parse for string

            len = json_string_decode(value, ptr, size);
            break;

        case 0x6E: // =n need to parse for "null"
        case 0x66: // =f need to parse for "false"
        case 0x74: // =t need to parse for "true"

            len = json_literal_decode(value, ptr, size);
            break;

            // number

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

            len = json_number_decode(value, ptr, size);
            break;
        default:
            goto error;
            break;
    }

    if (len < 0) goto error;

    return ((char *)ptr - (char *)buffer) + len;
error:
    if (created) *value = ov_item_free(*value);

    return -1;
}

/*---------------------------------------------------------------------------*/

ov_item *ov_item_from_json(const char *string){

    ov_item *self = NULL;
    if (!string) goto error;

    size_t len = strlen(string);

    int64_t result = json_decode(&self, string, len);
    if (-1 == result) goto error;
    if (len !=(size_t) result) goto error;

    return self;

error:
    ov_item_free(self);
    return NULL;
}

/*---------------------------------------------------------------------------*/

ov_item_json_stringify_config ov_item_json_config_stringify_minimal() {

    /*      JSON with minimal config like (minimum JSON string)
     *
     *      {"key1":"value","key2":["string",null,false,true,5]}
     *
     */

    ov_item_json_stringify_config config = {

        .string = {.item =
                       {
                           .intro = "\"",
                           .outro = "\"",
                       }},

        .array = {.item =
                      {
                          .intro = "[",
                          .outro = "]",
                          .separator = ",",
                      }},

        .object = {.item = {
                       .intro = "{",
                       .outro = "}",
                       .separator = ",",
                       .delimiter = ":",
                   }}};

    return config;
}

/*----------------------------------------------------------------------------*/

ov_item_json_stringify_config ov_item_json_config_stringify_default() {

    /*      JSON with default config like
     *      @Note indent depth must be applied in stringify
     *
     *      {
     *              "key1":"value",
     *              "key2":
     *              [
     *                      "string",
     *                      null,
     *                      false,
     *                      true,
     *                      5
     *              ]
     *      }
     *
     */

    ov_item_json_stringify_config config = {

        .string = {.item = {.intro = "\"", .outro = "\""}},

        .array = {.item = {.intro = "[\n",
                           .out = "\n",
                           .outro = "]",
                           .separator = ",\n"},
                  .entry = {.depth = true, .indent = "\t"}},

        .object = {.item = {.intro = "{\n",
                            .out = "\n",
                            .outro = "}",
                            .separator = ",\n",
                            .delimiter = ":"},
                   .entry = {.depth = true, .indent = "\t"}}};

    return config;
}

/*----------------------------------------------------------------------------*/

static int64_t parser_calculate_value(const ov_item *value,
                                     EncodingParameter *parameter);

/*----------------------------------------------------------------------------*/

static int64_t parser_calculate_null(const ov_item *value,
                                          EncodingParameter *parameter) {

    if (!value || !parameter) goto error;

    int64_t length = 0;

    length = 4;

    if (parameter->config.literal.item.intro)
        length += strlen(parameter->config.literal.item.intro);

    if (parameter->config.literal.item.outro)
        length += strlen(parameter->config.literal.item.outro);

    return length;
error:
    return -1;
}

/*----------------------------------------------------------------------------*/

static int64_t parser_calculate_true(const ov_item *value,
                                          EncodingParameter *parameter) {

    if (!value || !parameter) goto error;

    int64_t length = 0;

    length = 4;

    if (parameter->config.literal.item.intro)
        length += strlen(parameter->config.literal.item.intro);

    if (parameter->config.literal.item.outro)
        length += strlen(parameter->config.literal.item.outro);

    return length;
error:
    return -1;
}

/*----------------------------------------------------------------------------*/

static int64_t parser_calculate_false(const ov_item *value,
                                          EncodingParameter *parameter) {

    if (!value || !parameter) goto error;

    int64_t length = 0;

    length = 5;

    if (parameter->config.literal.item.intro)
        length += strlen(parameter->config.literal.item.intro);

    if (parameter->config.literal.item.outro)
        length += strlen(parameter->config.literal.item.outro);

    return length;
error:
    return -1;
}

/*----------------------------------------------------------------------------*/

static int64_t parser_calculate_string(const ov_item *value,
                                            EncodingParameter *parameter) {

    if (!value || !parameter) goto error;

    const char *content = ov_item_get_string((ov_item*)value);
    if (!content) goto error;

    size_t length = strlen(content);

    if (parameter->config.string.item.intro)
        length += strlen(parameter->config.string.item.intro);

    if (parameter->config.string.item.outro)
        length += strlen(parameter->config.string.item.outro);

    return length;
error:
    return -1;
}

/*----------------------------------------------------------------------------*/

static bool json_number_fill_string(const ov_item *self, char *string) {

    if (!self || !string) goto error;

    double number = ov_item_get_number((ov_item*)self);
    size_t length = 0;

    if (number == floor(number)) {

        length = sprintf(string, "%" PRIi64, (int64_t)number);

    } else {

        length = sprintf(string, "%.15g", number);
    }

    if (length == 0) goto error;

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static int64_t parser_calculate_number(const ov_item *value,
                                            EncodingParameter *parameter) {

    char string[25] = {};

    if (!value || !parameter) goto error;

    if (!json_number_fill_string(value, string)) goto error;

    size_t length = strlen(string);

    if (parameter->config.number.item.intro)
        length += strlen(parameter->config.number.item.intro);

    if (parameter->config.number.item.outro)
        length += strlen(parameter->config.number.item.outro);

    return length;

error:
    return -1;
}

/*----------------------------------------------------------------------------*/

static int64_t parser_calculate_array(const ov_item *value,
                                           EncodingParameter *p) {

    if (!value || !p) goto error;

    size_t length = 0;
    size_t items = ov_item_count((ov_item*)value);
    if (items == 0) return 2;

    size_t indent = 0;
    size_t separator = strlen(p->config.array.item.separator);

    if (p->config.array.entry.depth) {

        if (p->config.array.entry.indent)
            indent = strlen(p->config.array.entry.indent);
    }

    // intro length
    length += (p->depth * indent) + strlen(p->config.array.item.intro);
    // outro length
    length += (p->depth * indent) + strlen(p->config.array.item.outro);
    // out length
    if (p->config.array.item.out) length += strlen(p->config.array.item.out);

    ov_item *child = NULL;

    p->depth++;

    for (size_t i = 1; i <= items; i++) {

        child = ov_item_array_get((ov_item *)value, i);
        if (!child) goto error;

        if (ov_item_is_array(child)){

            if (ov_item_is_empty(child))
                length += (p->depth * indent);

            length += parser_calculate_value(child, p);

        } else if (ov_item_is_object(child)){

            if (ov_item_is_empty(child))
                length += (p->depth * indent);

            length += parser_calculate_value(child, p);

        } else {

            length += (p->depth * indent);
            length += parser_calculate_value(child, p);
        }

        if (i < items) length += separator;
    }

    p->depth--;
    return length;

error:
    return -1;
}

/*----------------------------------------------------------------------------*/

static size_t json_object_calculate_indent(EncodingParameter *parameter) {

    if (!parameter) goto error;

    bool enabled = parameter->config.object.entry.depth;
    char *indent = parameter->config.object.entry.indent;

    if (!enabled || !indent) goto error;

    return strlen(indent) * parameter->depth;

error:
    return 0;
}

/*----------------------------------------------------------------------------*/

static bool add_encoding_size(const char *key, const ov_item *value, void *data) {

    if (!key && !value) return true;

    if (!key || !value || !data) goto error;

    ov_item *val = ov_item_cast(value);
    if (!val) goto error;

    EncodingParameter *p = (EncodingParameter *)data;

    size_t length = p->counter;

    length = length + json_object_calculate_indent(p);
    length = length + strlen(key) + 2;
    length = length + strlen(p->config.object.item.delimiter);

    if (p->config.object.entry.depth) {

        if (ov_item_is_array(val)){

            if (!ov_item_is_empty(val)) length = length + 1;

        } else if (ov_item_is_object(val)) {

            if (!ov_item_is_empty(val)) length = length + 1;
        }

    }

    size_t vlen = parser_calculate_value(value, p);
    if (vlen < 1) goto error;

    length = length + vlen;
    length = length + strlen(p->config.object.item.separator);

    p->counter = length;
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static int64_t parser_calculate_object(const ov_item *value,
                                            EncodingParameter *p) {

    if (!value || !p) goto error;

    size_t length = 0;
    if (ov_item_is_empty((ov_item*)value)) return 2;

    size_t indent = json_object_calculate_indent(p);

    // intro length {\n
    length += indent + strlen(p->config.object.item.intro);
    // outro length }
    length += indent + strlen(p->config.object.item.outro);
    // out length \n
    if (p->config.object.item.out) length += strlen(p->config.object.item.out);

    p->counter = length;
    p->depth++;

    if (!ov_item_object_for_each((ov_item *)value, add_encoding_size, p))
        goto error;

    p->depth--;

    length = p->counter;
    length -= strlen(p->config.object.item.separator);

    return length;

error:
    return -1;
}

/*----------------------------------------------------------------------------*/

static int64_t parser_calculate_value(const ov_item *value,
                                     EncodingParameter *parameter) {

    if (!value || !parameter) goto error;

    if (ov_item_is_null((ov_item*)value)){

        return parser_calculate_null(value, parameter);

    } else if (ov_item_is_false((ov_item*)value)){

        return parser_calculate_false(value, parameter);

    } else if (ov_item_is_true((ov_item*)value)){

        return parser_calculate_true(value, parameter);

    } else if (ov_item_is_string((ov_item*)value)){

        return parser_calculate_string(value, parameter);

    } else if (ov_item_is_number((ov_item*)value)){

        return parser_calculate_number(value, parameter);

    } else if (ov_item_is_array((ov_item*)value)){

        return parser_calculate_array(value, parameter);

    } else if (ov_item_is_object((ov_item*)value)){

        return parser_calculate_object(value, parameter);

    } 

error:
    return -1;
}

/*----------------------------------------------------------------------------*/

static int64_t parser_calculate(const ov_item *value,
                                const ov_item_json_stringify_config *conf) {

    if (!value) goto error;

    EncodingParameter parameter;
    memset(&parameter, 0, sizeof(EncodingParameter));
    
    if (conf) {

        parameter.config = *conf;

    } else {

        parameter.config = ov_item_json_config_stringify_minimal();

    }

    int64_t result = parser_calculate_value(value, &parameter);
    if (result < 0) goto error;

    if (NULL != parameter.config.intro)
        result += strlen(parameter.config.intro);

    if (NULL != parameter.config.outro)
        result += strlen(parameter.config.outro);

    return result;
error:
    return -1;
}

/*----------------------------------------------------------------------------*/

static bool json_parser_write_if_not_null(const char *content,
                                          char **next,
                                          size_t *open){

    if (!content) return true;

    if (!next || !open) return false;

    size_t length = strlen(content);

    if (*open < length) return false;

    char *p = (char *)*next;

    p = strncpy(p, content, length);
    if (!p) return false;

    *next += length;
    *open -= length;

    return true;
}

/*----------------------------------------------------------------------------*/

static bool json_literal_encode_pack(const ov_item_json_config *config,
                                     const char *content,
                                     char **next,
                                     size_t *open) {

    if (!config) return false;

    if (!json_parser_write_if_not_null(config->item.intro, next, open))
        return false;

    if (!json_parser_write_if_not_null(content, next, open)) return false;

    if (!json_parser_write_if_not_null(config->item.outro, next, open))
        return false;

    return true;
}

/*----------------------------------------------------------------------------*/

static int64_t parser_encode_null(const ov_item *self,
                                   EncodingParameter *parameter) {

    if (!self || !parameter || parameter->size < 4)
        goto error;

    char *start = parameter->buffer;

    if (!json_literal_encode_pack(&parameter->config.literal,
                                          ENCODING_STRING_NULL,
                                          &parameter->buffer,
                                          &parameter->size))
        goto error;

    return (parameter->buffer - start);

error:
    return -1;
}

/*----------------------------------------------------------------------------*/

static int64_t parser_encode_true(const ov_item *self,
                                   EncodingParameter *parameter) {

    if (!self || !parameter || parameter->size < 4)
        goto error;

    char *start = parameter->buffer;

    if (!json_literal_encode_pack(&parameter->config.literal,
                                          ENCODING_STRING_TRUE,
                                          &parameter->buffer,
                                          &parameter->size))
        goto error;

    return (parameter->buffer - start);

error:
    return -1;
}

/*----------------------------------------------------------------------------*/

static int64_t parser_encode_false(const ov_item *self,
                                   EncodingParameter *parameter) {

    if (!self || !parameter || parameter->size < 5)
        goto error;

    char *start = parameter->buffer;

    if (!json_literal_encode_pack(&parameter->config.literal,
                                          ENCODING_STRING_FALSE,
                                          &parameter->buffer,
                                          &parameter->size))
        goto error;

    return (parameter->buffer - start);

error:
    return -1;
}

/*----------------------------------------------------------------------------*/

static int64_t parser_encode_string(ov_item *value,
                                  EncodingParameter *parameter) {

    if (!value || !parameter) goto error;

    char *start = parameter->buffer;

    const char *string = ov_item_get_string(value);
    if (!string) goto error;

    if (parameter->size < (strlen(string) + 2)) goto error;

    if (!json_parser_write_if_not_null(parameter->config.string.item.intro,
                                       &parameter->buffer,
                                       &parameter->size))
        goto error;

    if (!json_parser_write_if_not_null(
            string, &parameter->buffer, &parameter->size))
        goto error;

    if (!json_parser_write_if_not_null(parameter->config.string.item.outro,
                                       &parameter->buffer,
                                       &parameter->size))
        goto error;

    return (parameter->buffer - start);
error:
    return -1;
}

/*----------------------------------------------------------------------------*/

static int64_t parser_encode_number(const ov_item *self,
                                  EncodingParameter *parameter) {

    char string[25] = {0};

    if (!self || !parameter) goto error;

    char *start = parameter->buffer;

    if (!json_number_fill_string(self, string)) goto error;

    if (parameter->size < strlen(string)) goto error;

    if (!json_parser_write_if_not_null(parameter->config.number.item.intro,
                                       &parameter->buffer,
                                       &parameter->size))
        goto error;

    if (!json_parser_write_if_not_null(
            string, &parameter->buffer, &parameter->size))
        goto error;

    if (!json_parser_write_if_not_null(parameter->config.number.item.outro,
                                       &parameter->buffer,
                                       &parameter->size))
        goto error;

    return (parameter->buffer - start);

error:
    return -1;
}

/*----------------------------------------------------------------------------*/

static bool object_write_indent(EncodingParameter *parameter) {

    if (!parameter) goto error;

    bool enabled = parameter->config.object.entry.depth;
    char *indent = parameter->config.object.entry.indent;

    size_t len = 0;

    if (!enabled || !indent) return true;

    len = strlen(indent);

    if (parameter->size < parameter->depth * len) goto error;

    for (size_t i = 0; i < parameter->depth; i++) {

        if (parameter->size <= len) goto error;

        if (!memcpy(parameter->buffer, indent, len)) goto error;

        parameter->buffer += len;
        parameter->size -= len;
    }

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool object_write_intro(EncodingParameter *parameter) {

    if (!parameter || !parameter->config.object.item.intro) goto error;

    if (!object_write_indent(parameter)) goto error;

    return json_parser_write_if_not_null(parameter->config.object.item.intro,
                                         &parameter->buffer,
                                         &parameter->size);

error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool object_write_outro(EncodingParameter *parameter) {

    if (!parameter || !parameter->config.object.item.intro) goto error;

    if (!parameter->config.object.item.outro) goto error;

    // write out e.g. \n
    if (!json_parser_write_if_not_null(parameter->config.object.item.out,
                                       &parameter->buffer,
                                       &parameter->size))
        goto error;

    // write outro indent e.g. \t

    if (!object_write_indent(parameter)) goto error;

    // write outro e.g. }

    return json_parser_write_if_not_null(parameter->config.object.item.outro,
                                         &parameter->buffer,
                                         &parameter->size);

error:
    return false;
}

/*----------------------------------------------------------------------------*/

static int64_t json_value_encode(ov_item *value,
                                 EncodingParameter *parameter);

/*----------------------------------------------------------------------------*/

static bool object_write_entry(const char *key_in,
                               const ov_item *value,
                               void *parameter) {

    if (!key_in) return true;

    if (!key_in || !value || !parameter) return false;

    ov_item *val = ov_item_cast(value);
    if (!val) goto error;

    char *key = (char *)key_in;
    size_t key_len = strlen(key);
    EncodingParameter *para = (EncodingParameter *)parameter;

    if (!object_write_indent(parameter)) goto error;

    if (para->size < (strlen(key) + 2)) goto error;

    // write key

    *para->buffer = '"';
    para->buffer++;

    if (!memcpy(para->buffer, key, key_len)) goto error;

    para->buffer += key_len;

    *para->buffer = '"';
    para->buffer++;

    para->size -= key_len + 2;

    if (!json_parser_write_if_not_null(
            para->config.object.item.delimiter, &para->buffer, &para->size))
        goto error;

    if (para->size < 2) goto error;

    if (para->config.object.entry.depth) {

        if (ov_item_is_array(val)){

            if (!ov_item_is_empty(val)) {

                    para->buffer[0] = '\n';
                    para->buffer++;
                    para->size--;
                }

        } else if (ov_item_is_object(val)){

            if (!ov_item_is_empty(val)) {

                    para->buffer[0] = '\n';
                    para->buffer++;
                    para->size--;
                }
        }
    }

    if (json_value_encode(val, parameter) < 1) goto error;

    if (!json_parser_write_if_not_null(
            para->config.object.item.separator, &para->buffer, &para->size))
        goto error;

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

struct container1 {

    ov_item *object;
    EncodingParameter *parameter;
};

/*----------------------------------------------------------------------------*/

static bool object_write_ordered(void *item, void *data) {

    if (!item || !data) goto error;

    struct container1 *container = (struct container1 *)data;

    if (!container->object || !container->parameter) goto error;

    ov_item *value = ov_item_object_get(container->object, (char *)item);

    if (!value) goto error;

    return object_write_entry(item, value, container->parameter);

error:
    return false;
}

/*----------------------------------------------------------------------------*/

struct parameter_order {

    ov_list *list;
    bool (*collocate_key)(const char *key, ov_list *list);
};

/*----------------------------------------------------------------------------*/

static bool add_key_collocated(const char *key, const ov_item *value, void *data) {

    if (!key) return true;

    if (!value || !data) return false;

    struct parameter_order *d = (struct parameter_order *)data;
    if (!d->list || !d->collocate_key) goto error;

    return d->collocate_key((char *)key, d->list);
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static ov_list *collocate_object_keys(const ov_item *value,
                                      bool (*function)(const char *key,
                                                       ov_list *list)) {

    ov_list *list = NULL;

    if (!value || !function) goto error;

    list = ov_list_create((ov_list_config){0});

    struct parameter_order data = {

        .list = list,
        .collocate_key = function,
    };

    if (!ov_item_object_for_each(
            (ov_item *)value, add_key_collocated, &data))
        goto error;

    return list;
error:
    if (list) list->free(list);
    return NULL;
}

/*----------------------------------------------------------------------------*/

static int64_t parser_encode_object(ov_item *value,
                                  EncodingParameter *parameter) {

    ov_list *list = NULL;

    if (!value || !parameter) goto error;

    char *start = parameter->buffer;
    size_t items = ov_item_count(value);

    if (items == 0) {

        if (parameter->size < 2) goto error;

        if (!memcpy(parameter->buffer, "{}", 2)) goto error;

        parameter->buffer += 2;
        parameter->size -= 2;
        goto done;
    }

    // not empty

    if (!object_write_intro(parameter)) goto error;

    // write childs with depth +1
    parameter->depth++;

    if (parameter->collocate_keys) {

        // write ordered
        list = collocate_object_keys(value, parameter->collocate_keys);
        if (!list) goto error;

        struct container1 container = {

            .object = (ov_item *)value, .parameter = parameter};

        if (!ov_list_for_each(list, &container, object_write_ordered))
            goto error;

    } else {

        // unordered
        if (!ov_item_object_for_each(value, 
            object_write_entry, 
            parameter))goto error;

            goto error;
    }

    
    
    parameter->depth--;

    // offset last separator
    size_t len = strlen(parameter->config.object.item.separator);
    parameter->buffer -= len;
    parameter->size += len;

    if (!object_write_outro(parameter)) goto error;
done:
    list = ov_list_free(list);
    return (parameter->buffer - start);

error:
    list = ov_list_free(list);
    return -1;
}

/*----------------------------------------------------------------------------*/

static bool array_write_indent(EncodingParameter *parameter) {

    if (!parameter) goto error;

    bool enabled = parameter->config.array.entry.depth;
    char *indent = parameter->config.array.entry.indent;

    size_t len = 0;

    if (!enabled || !indent) return true;

    len = strlen(indent);

    if (parameter->size < parameter->depth * len) goto error;

    for (size_t i = 0; i < parameter->depth; i++) {

        if (parameter->size <= len) goto error;

        if (!memcpy(parameter->buffer, indent, len)) goto error;

        parameter->buffer += len;
        parameter->size -= len;
    }

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool array_write_intro(EncodingParameter *parameter) {

    if (!parameter || !parameter->config.array.item.intro) goto error;

    if (!array_write_indent(parameter)) goto error;

    return json_parser_write_if_not_null(parameter->config.array.item.intro,
                                         &parameter->buffer,
                                         &parameter->size);

error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool array_write_outro(EncodingParameter *parameter) {

    if (!parameter || !parameter->config.array.item.intro) goto error;

    if (!parameter->config.array.item.outro) goto error;

    // write out e.g. \n
    if (!json_parser_write_if_not_null(parameter->config.array.item.out,
                                       &parameter->buffer,
                                       &parameter->size))
        goto error;

    // write outro indent e.g. \t

    if (!array_write_indent(parameter)) goto error;

    // write outro e.g. ]

    return json_parser_write_if_not_null(parameter->config.array.item.outro,
                                         &parameter->buffer,
                                         &parameter->size);

error:
    return false;
}

/*----------------------------------------------------------------------------*/

static int64_t parser_encode_array(ov_item *value,
                                 EncodingParameter *parameter) {

    if (!value || !parameter) goto error;

    char *start = parameter->buffer;

    // empty (always without indent!)
    size_t items = ov_item_count(value);

    if (items == 0) {

        if (!memcpy(parameter->buffer, "[]", 2)) goto error;

        parameter->buffer += 2;
        parameter->size -= 2;
        goto done;
    }

    // non empty (use of configured indent)

    if (!array_write_intro(parameter)) goto error;

    ov_item *child = NULL;

    // write childs with depth +1
    parameter->depth++;

    bool write_indent = false;
    for (size_t i = 1; i <= items; i++) {

        child = ov_item_array_get(value, i);
        if (!child) goto error;

        write_indent = true;

        if (ov_item_is_array(child)){

            if (!ov_item_is_empty(child)) write_indent = false;

        } else if (ov_item_is_object(child)){

            if (!ov_item_is_empty(child)) write_indent = false;
        }

        if (write_indent)
            if (!array_write_indent(parameter)) goto error;

        if (json_value_encode(child, parameter) < 1) goto error;

        if (i < items) {

            if (!json_parser_write_if_not_null(
                    parameter->config.array.item.separator,
                    &parameter->buffer,
                    &parameter->size))
                goto error;
        }
    }

    parameter->depth--;

    if (!array_write_outro(parameter)) goto error;

done:
    return (parameter->buffer - start);

error:
    return -1;
}


/*----------------------------------------------------------------------------*/

static int64_t json_value_encode(ov_item *value,
                                 EncodingParameter *parameter) {

     if (!value || !parameter) goto error;

    if (ov_item_is_null((ov_item*)value)){

        return parser_encode_null(value, parameter);

    } else if (ov_item_is_false((ov_item*)value)){

        return parser_encode_false(value, parameter);

    } else if (ov_item_is_true((ov_item*)value)){

        return parser_encode_true(value, parameter);

    } else if (ov_item_is_string((ov_item*)value)){

        return parser_encode_string(value, parameter);

    } else if (ov_item_is_number((ov_item*)value)){

        return parser_encode_number(value, parameter);

    } else if (ov_item_is_array((ov_item*)value)){

        return parser_encode_array(value, parameter);

    } else if (ov_item_is_object((ov_item*)value)){

        return parser_encode_object(value, parameter);

    } 

error:
    return -1;
}

/*----------------------------------------------------------------------------*/

static bool ov_json_parser_collocate_ascending(const char *key, ov_list *list) {

    if (!key || !ov_list_cast(list)) return false;

    uint8_t *k = (uint8_t *)key;
    uint8_t *s = NULL;

    size_t klen = strlen(key);
    size_t slen = 0;
    size_t len = 0;

    for (size_t i = 1; i <= list->count(list); i++) {

        s = list->get(list, i);
        slen = strlen((char *)s);
        len = klen;
        if (slen < klen) len = slen;

        for (size_t x = 0; x <= len; x++) {

            if (k[x] < s[x]) {

                if (!list->insert(list, i, (void *)key)) goto error;

                return true;
            }

            if (k[x] != s[x]) break;
        }

        // check against next item
    }

    // not returned?
    if (!list->push(list, (void *)key)) goto error;

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static int64_t parser_encode(ov_item *value,
                              const ov_item_json_stringify_config *conf,
                              char *buffer,
                              size_t size) {

    if (!value || !buffer || size < 1) goto error;

    EncodingParameter parameter = {

        .buffer = buffer,
        .size = size,
        .depth = 0,
        .counter = 0,
        .collocate_keys = ov_json_parser_collocate_ascending
    };

    if (conf) {
        parameter.config = *conf;
    } else {
        parameter.config = ov_item_json_config_stringify_minimal();
    }

    size_t len = 0;

    if (NULL != parameter.config.intro) {

        len = strlen(parameter.config.intro);
        if (len >= size) goto error;

        if (!memcpy(buffer, parameter.config.intro, len)) goto error;

        parameter.buffer += len;
        parameter.size -= len;
    }

    int64_t result = json_value_encode(value, &parameter);
    if (result < 0) goto error;
    result += len;

    if (NULL != parameter.config.outro) {

        int64_t used = parameter.buffer - buffer;
        if (used < 0) goto error;

        len = strlen(parameter.config.outro);
        if (len > size - used) goto error;

        if (!memcpy(parameter.buffer, parameter.config.outro, len)) goto error;

        result += len;
    }

    return result;

error:
    return -1;
}

/*----------------------------------------------------------------------------*/

char *ov_item_to_json_with_config(const ov_item *value,
                                  ov_item_json_stringify_config config) {

    char *string = NULL;

    if (!ov_item_cast(value)) goto error;

    int64_t size = parser_calculate(value, &config);
    if (size < 0) goto error;

    string = calloc(size + 1, sizeof(char));
    if (!string) goto error;

    if (0 > parser_encode((ov_item*) value, &config, string, size))
        goto error;

    return string;
error:
    if (string) free(string);
    return NULL;
}

/*---------------------------------------------------------------------------*/

char *ov_item_to_json(const ov_item *self){

    char *out = NULL;

    if (!self) goto error;

    return ov_item_to_json_with_config(self, 
        ov_item_json_config_stringify_default());

    return out;
error:
    return NULL;
}