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
        @file           ov_json_value.c
        @author         Markus Toepfer
        @author         Michael Beer

        @date           2018-12-02

        @ingroup        ov_json_value

        @brief          Implementation of ov_json_value


        ------------------------------------------------------------------------
*/
#include "../../include/ov_json_value.h"
#include "../../include/ov_json.h"
#include "../../include/ov_json_parser.h"

bool ov_json_value_validate(const ov_json_value *self) {

    if (!ov_json_value_cast(self))
        goto error;

    switch (self->type) {

    case OV_JSON_OBJECT:
    case OV_JSON_ARRAY:
    case OV_JSON_STRING:
    case OV_JSON_NUMBER:
    case OV_JSON_NULL:
    case OV_JSON_FALSE:
    case OV_JSON_TRUE:
        break;

    default:
        goto error;
    }

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_json_value_cast(const void *data) {

    if (!data)
        return NULL;

    if (*(uint16_t *)data != OV_JSON_VALUE_MAGIC_BYTE)
        return NULL;

    return (ov_json_value *)data;
}

/*----------------------------------------------------------------------------*/

bool ov_json_value_clear(void *data) {

    ov_json_value *value = ov_json_value_cast(data);
    if (!value)
        goto error;

    switch (value->type) {

    case OV_JSON_OBJECT:
        return ov_json_object_clear(value);

    case OV_JSON_ARRAY:
        return ov_json_array_clear(value);

    case OV_JSON_STRING:
        return ov_json_string_clear(value);

    case OV_JSON_NUMBER:
        return ov_json_number_clear(value);

    case OV_JSON_NULL:
    case OV_JSON_FALSE:
    case OV_JSON_TRUE:
        return ov_json_literal_clear(value);

    default:
        break;
    }

error:
    return false;
}

/*----------------------------------------------------------------------------*/

void *ov_json_value_free(void *data) {

    ov_json_value *value = ov_json_value_cast(data);
    if (!value)
        goto error;

    if (!ov_json_value_unset_parent(value))
        goto error;

    switch (value->type) {

    case OV_JSON_OBJECT:
        return ov_json_object_free(value);

    case OV_JSON_ARRAY:
        return ov_json_array_free(value);

    case OV_JSON_STRING:
        return ov_json_string_free(value);

    case OV_JSON_NUMBER:
        return ov_json_number_free(value);

    case OV_JSON_NULL:
    case OV_JSON_FALSE:
    case OV_JSON_TRUE:
        return ov_json_literal_free(value);

    default:
        break;
    }

error:
    return value;
}

/*----------------------------------------------------------------------------*/

void *ov_json_value_copy(void **dest, const void *data) {

    ov_json_value *value = ov_json_value_cast((void *)data);
    if (!dest || !value)
        goto error;

    switch (value->type) {

    case OV_JSON_OBJECT:

        return ov_json_object_copy(dest, value);

    case OV_JSON_ARRAY:

        return ov_json_array_copy(dest, value);

    case OV_JSON_STRING:

        return ov_json_string_copy(dest, value);

    case OV_JSON_NUMBER:

        return ov_json_number_copy(dest, value);

    case OV_JSON_NULL:
    case OV_JSON_FALSE:
    case OV_JSON_TRUE:

        return ov_json_literal_copy(dest, value);

    default:
        break;
    }

    // run into error
error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_json_value_dump(FILE *stream, const void *data) {

    ov_json_value *value = ov_json_value_cast(data);
    if (!value)
        goto error;

    switch (value->type) {

    case OV_JSON_OBJECT:
        return ov_json_object_dump(stream, value);

    case OV_JSON_ARRAY:
        return ov_json_array_dump(stream, value);

    case OV_JSON_STRING:
        return ov_json_string_dump(stream, value);

    case OV_JSON_NUMBER:
        return ov_json_number_dump(stream, value);

    case OV_JSON_NULL:
    case OV_JSON_FALSE:
    case OV_JSON_TRUE:
        return ov_json_literal_dump(stream, value);

    default:
        break;
    }

error:
    return false;
}

/*----------------------------------------------------------------------------*/

ov_data_function ov_json_value_data_functions() {

    ov_data_function f = {

        .clear = ov_json_value_clear,
        .free = ov_json_value_free,
        .copy = ov_json_value_copy,
        .dump = ov_json_value_dump,
    };

    return f;
}

/*----------------------------------------------------------------------------*/

bool ov_json_value_set_parent(ov_json_value *value,
                              const ov_json_value *parent) {

    if (!value || !parent)
        return false;

    if (value->parent)
        if (value->parent != parent)
            goto error;

    switch (parent->type) {

    case OV_JSON_OBJECT:
    case OV_JSON_ARRAY:
        break;

    default:
        goto error;
    }

    value->parent = (ov_json_value *)parent;
    return true;
error:
    return false;
};

/*----------------------------------------------------------------------------*/

bool ov_json_value_unset_parent(ov_json_value *value) {

    if (!value)
        return false;

    if (!value->parent)
        return true;

    switch (value->parent->type) {

    case OV_JSON_OBJECT:

        if (!ov_json_object_remove_child(value->parent, value))
            goto error;

        break;

    case OV_JSON_ARRAY:

        if (!ov_json_array_remove_child(value->parent, value))
            goto error;

        break;

    default:
        break;
    }

    // forced unset of parent
    value->parent = NULL;
    return true;
error:
    return false;
};

/*----------------------------------------------------------------------------*/

char *ov_json_value_to_string(const ov_json_value *value) {

    return ov_json_encode(value);
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_json_value_from_string(const char *string, size_t length) {

    if (!string || length < 1)
        goto error;

    ov_json_value *out = NULL;
    if (0 > ov_json_parser_decode(&out, string, length))
        goto error;

    return out;
error:

    return NULL;
}
