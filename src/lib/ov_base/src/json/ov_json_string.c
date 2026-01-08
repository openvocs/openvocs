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
        @file           ov_json_string.c
        @author         Markus Toepfer

        @date           2018-12-03

        @ingroup        ov_json_string

        @brief          Implementation of ov_json_string


        ------------------------------------------------------------------------
*/
#include "../../include/ov_json_string.h"

typedef struct {

    ov_json_value head;

    struct {

        char *start;
        size_t size;

    } buffer;

} JsonString;

#define AS_JSON_STRING(x)                                                      \
    (((ov_json_value_cast(x) != 0) &&                                          \
      (OV_JSON_STRING == ((ov_json_value *)x)->type))                          \
         ? (JsonString *)(x)                                                   \
         : 0)

/*----------------------------------------------------------------------------*/

bool json_string_init(JsonString *self, const char *content) {

    if (!self)
        goto error;

    self->head.magic_byte = OV_JSON_VALUE_MAGIC_BYTE;
    self->head.type = OV_JSON_STRING;
    self->head.parent = NULL;

    self->head.clear = ov_json_string_clear;
    self->head.free = ov_json_string_free;

    if (self->buffer.start) {
        free(self->buffer.start);
        self->buffer.start = NULL;
    }

    if (content) {
        self->buffer.start = strdup(content);
        if (!self->buffer.start)
            goto error;
        self->buffer.size = strlen(self->buffer.start);
    }

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_json_string(const char *content) {

    JsonString *string = calloc(1, sizeof(JsonString));
    if (!string)
        goto error;

    if (json_string_init(string, content))
        return (ov_json_value *)string;

    free(string);
error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

bool ov_json_is_string(const ov_json_value *value) {

    return AS_JSON_STRING(value);
}

/*----------------------------------------------------------------------------*/

bool ov_json_string_is_valid(const ov_json_value *value) {

    JsonString *string = AS_JSON_STRING(value);
    if (!string || !string->buffer.start || string->buffer.size < 1)
        goto error;

    return ov_json_validate_string((uint8_t *)string->buffer.start,
                                   strlen(string->buffer.start), false);

error:
    return false;
}

/*----------------------------------------------------------------------------*/

const char *ov_json_string_get(const ov_json_value *self) {

    JsonString *string = AS_JSON_STRING(self);
    if (!string)
        return NULL;

    return string->buffer.start;
}

/*----------------------------------------------------------------------------*/

bool ov_json_string_set_length(ov_json_value *self, const char *content,
                               size_t length) {

    JsonString *string = AS_JSON_STRING(self);
    if (!string || !content || length < 1)
        goto error;

    // only allow to set valid content
    if (!ov_json_validate_string((uint8_t *)content, length, false))
        goto error;

    if (!string->buffer.start) {
        string->buffer.start = strndup(content, length);
        string->buffer.size = length;

    } else if (string->buffer.size > length) {

        if (!memset(string->buffer.start, 0, string->buffer.size))
            goto error;

        if (!strncpy(string->buffer.start, content, length))
            goto error;

    } else {

        free(string->buffer.start);
        string->buffer.start = strndup(content, length);
        string->buffer.size = length;
    }

    return true;

error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_json_string_set(ov_json_value *self, const char *content) {

    if (!self || !content)
        return false;

    size_t length = strlen(content);
    return ov_json_string_set_length(self, content, length);
}

/*----------------------------------------------------------------------------*/

bool ov_json_string_clear(void *self) {

    JsonString *string = AS_JSON_STRING(self);
    if (!string)
        goto error;

    if (string->buffer.start) {
        memset(string->buffer.start, 0, string->buffer.size);
    }

    return true;

error:
    return false;
}

/*----------------------------------------------------------------------------*/

void *ov_json_string_free(void *self) {

    JsonString *string = AS_JSON_STRING(self);
    if (!string)
        return self;

    // in case of parent loop over ov_json_value_free
    if (string->head.parent)
        return ov_json_value_free(self);

    if (string->buffer.start)
        free(string->buffer.start);

    free(string);
    return NULL;
}

/*----------------------------------------------------------------------------*/

void *ov_json_string_copy(void **dest, const void *self) {

    if (!dest || !self)
        goto error;

    JsonString *copy = NULL;
    JsonString *orig = AS_JSON_STRING(self);
    if (!orig)
        goto error;

    if (!*dest) {

        *dest = ov_json_string(ov_json_string_get((ov_json_value *)orig));
        if (*dest)
            return *dest;
    }

    copy = AS_JSON_STRING(*dest);
    if (!copy)
        goto error;

    if (ov_json_string_set((ov_json_value *)copy,
                           ov_json_string_get((ov_json_value *)orig)))
        return copy;

error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

bool ov_json_string_dump(FILE *stream, const void *self) {

    JsonString *orig = AS_JSON_STRING(self);
    if (!stream || !orig)
        goto error;

    if (!fprintf(stream, " \"%s\" ", orig->buffer.start))
        goto error;

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

ov_data_function ov_json_string_functions() {

    ov_data_function f = {

        .clear = ov_json_string_clear,
        .free = ov_json_string_free,
        .dump = ov_json_string_dump,
        .copy = ov_json_string_copy};

    return f;
}
