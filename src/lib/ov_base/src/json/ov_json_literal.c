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
        @file           ov_json_literal.c
        @author         Markus Toepfer

        @date           2018-12-03

        @ingroup        ov_json_literal

        @brief          Implementation of ov_json_literal


        ------------------------------------------------------------------------
*/
#include "../../include/ov_json_literal.h"

/*----------------------------------------------------------------------------*/

ov_json_value *ov_json_literal(ov_json_t type) {

    switch (type) {

        case OV_JSON_TRUE:
        case OV_JSON_NULL:
        case OV_JSON_FALSE:
            break;
        default:
            goto error;
    }

    ov_json_value *literal = calloc(1, sizeof(ov_json_value));
    if (!literal) goto error;

    literal->magic_byte = OV_JSON_VALUE_MAGIC_BYTE;
    literal->type = type;
    literal->parent = NULL;

    literal->clear = ov_json_literal_clear;
    literal->free = ov_json_literal_free;

    return literal;
error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_json_bool(bool bval) {

    if (bval) {
        return ov_json_true();
    } else {
        return ov_json_false();
    }
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_json_true() { return ov_json_literal(OV_JSON_TRUE); }

/*----------------------------------------------------------------------------*/

ov_json_value *ov_json_false() { return ov_json_literal(OV_JSON_FALSE); }

/*----------------------------------------------------------------------------*/

ov_json_value *ov_json_null() { return ov_json_literal(OV_JSON_NULL); }

/*----------------------------------------------------------------------------*/

bool ov_json_is_true(const ov_json_value *value) {

    if (!value) goto error;

    if (value->type == OV_JSON_TRUE) return true;

error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_json_is_false(const ov_json_value *value) {

    if (!value) goto error;

    if (value->type == OV_JSON_FALSE) return true;

error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_json_is_null(const ov_json_value *value) {

    if (!value) goto error;

    if (value->type == OV_JSON_NULL) return true;

error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_json_is_literal(const ov_json_value *value) {

    if (!value) goto error;

    switch (value->type) {

        case OV_JSON_TRUE:
        case OV_JSON_NULL:
        case OV_JSON_FALSE:
            return true;
        default:
            break;
    }

error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_json_literal_set(ov_json_value *self, ov_json_t type) {

    if (!self) goto error;

    switch (self->type) {

        case OV_JSON_TRUE:
        case OV_JSON_NULL:
        case OV_JSON_FALSE:
            break;
        default:
            return false;
    }

    switch (type) {

        case OV_JSON_TRUE:
        case OV_JSON_NULL:
        case OV_JSON_FALSE:
            break;
        default:
            return false;
    }

    self->type = type;
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_json_literal_clear(void *self) {

    ov_json_value *value = ov_json_value_cast(self);
    if (!value) goto error;

    if (!ov_json_is_literal(value)) goto error;

    value->type = OV_JSON_NULL;
    return true;

error:
    return false;
}

/*----------------------------------------------------------------------------*/

void *ov_json_literal_free(void *self) {

    ov_json_value *value = ov_json_value_cast(self);
    if (!value) return self;

    if (!ov_json_is_literal(value)) return value;

    // in case of parent loop over ov_json_value_free
    if (value->parent) return ov_json_value_free(value);

    free(value);
    return NULL;
}

/*----------------------------------------------------------------------------*/

void *ov_json_literal_copy(void **dest, const void *self) {

    if (!dest || !self) goto error;

    ov_json_value *copy = NULL;
    ov_json_value *orig = ov_json_value_cast(self);
    if (!ov_json_is_literal(orig)) goto error;

    if (!*dest) {

        *dest = ov_json_literal(orig->type);
        if (*dest) return *dest;
    }

    copy = ov_json_value_cast(*dest);
    if (!ov_json_is_literal(copy)) goto error;

    copy->type = orig->type;
    return copy;
error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

bool ov_json_literal_dump(FILE *stream, const void *self) {

    ov_json_value *orig = ov_json_value_cast(self);
    if (!stream || !orig) goto error;

    char *string = NULL;

    switch (orig->type) {

        case OV_JSON_TRUE:
            string = "true";
            break;

        case OV_JSON_NULL:
            string = "null";
            break;

        case OV_JSON_FALSE:
            string = "false";
            break;
        default:
            goto error;
    }

    if (!fprintf(stream, " %s ", string)) goto error;

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

ov_data_function ov_json_literal_functions() {

    ov_data_function f = {

        .clear = ov_json_literal_clear,
        .free = ov_json_literal_free,
        .copy = ov_json_literal_copy,
        .dump = ov_json_literal_dump};

    return f;
}
