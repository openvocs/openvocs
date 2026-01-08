/***
        ------------------------------------------------------------------------

        Copyright (c) 2023 German Aerospace Center DLR e.V. (GSOC)

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
#include "../include/ov_value_serde.h"
#include "../include/ov_registered_cache.h"
#include "../include/ov_value_parse.h"
#include "ov_error_codes.h"
#include <wchar.h>

/*----------------------------------------------------------------------------*/

#define MAGIC_BYTES 0xa121bc218714bbbb

/*****************************************************************************
                                Our linked list
 ****************************************************************************/

struct joint;

typedef struct joint {

    ov_value *value;
    struct joint *next;

} Joint;

/*----------------------------------------------------------------------------*/

static ov_registered_cache *g_joint_cache = 0;

/*----------------------------------------------------------------------------*/

static Joint *joint(ov_value *value) {

    Joint *joint = ov_registered_cache_get(g_joint_cache);
    joint = OV_OR_DEFAULT(joint, calloc(1, sizeof(Joint)));

    joint->value = value;

    return joint;
}

/*----------------------------------------------------------------------------*/

// Free joint and return next joint. Frees possibly contained value
static Joint *joint_free(Joint *joint) {

    if (0 == joint) {
        return 0;
    } else {

        joint->value = ov_value_free(joint->value);
        Joint *next = joint->next;
        joint->next = 0;
        ov_free(ov_registered_cache_put(g_joint_cache, joint));
        return next;
    }
}

/*----------------------------------------------------------------------------*/

static void *joint_free_vptr(void *vjoint) { return joint_free(vjoint); }

/*****************************************************************************
                                     Serde
 ****************************************************************************/

typedef struct {

    ov_serde public;

    ov_buffer *incomplete_input;

    Joint *ready_values;
    Joint *last_value; // points to the last element in ready_values if
                       // ready_values contains anything

} JsonValueSerde;

/*----------------------------------------------------------------------------*/

static ov_value *pop_ready_value(JsonValueSerde *serde) {

    ov_value *value = 0;

    if ((0 != serde) && (0 != serde->ready_values)) {
        value = serde->ready_values->value;
        serde->ready_values->value = 0;
        serde->ready_values = joint_free(serde->ready_values);
    }

    return value;
}

/*----------------------------------------------------------------------------*/

static JsonValueSerde *as_json_value_serde(ov_serde *self) {

    if ((ov_ptr_valid(self, "Invalid serde: 0 pointer")) &&
        (ov_cond_valid(MAGIC_BYTES == self->magic_bytes,
                       "Invalid serde: Unexpected magic bytes"))) {
        return (JsonValueSerde *)self;
    } else {
        return 0;
    }
}

/*----------------------------------------------------------------------------*/

static ov_serde_state impl_add_raw(ov_serde *self, ov_buffer const *raw,
                                   ov_result *res) {

    JsonValueSerde *serde = as_json_value_serde(self);

    if (!ov_ptr_valid(serde, "Cannot add data: Invalid serde object")) {
        ov_result_set(res, OV_ERROR_INTERNAL_SERVER,
                      "Cannot add data: Invalid serde object");

        return OV_SERDE_ERROR;

    } else if (0 != raw) {

        serde->incomplete_input =
            ov_buffer_concat(serde->incomplete_input, raw);

        return OV_SERDE_PROGRESS;

    } else {

        return OV_SERDE_PROGRESS;
    }
}

/*----------------------------------------------------------------------------*/

static void update_incomplete_input(JsonValueSerde *serde,
                                    char const *remainder) {

    if ((0 != serde) && (0 == remainder)) {

        serde->incomplete_input = ov_buffer_free(serde->incomplete_input);

    } else if ((0 != serde) && (0 != serde->incomplete_input)) {

        ptrdiff_t consumed_bytes =
            (uint8_t *)remainder - serde->incomplete_input->start;

        size_t remaining_bytes =
            serde->incomplete_input->length - consumed_bytes;

        ov_buffer *remainder_buffer = ov_buffer_create(remaining_bytes);
        memcpy(remainder_buffer->start, remainder, remaining_bytes);
        remainder_buffer->length = remaining_bytes;
        ov_buffer_free(serde->incomplete_input);
        serde->incomplete_input = remainder_buffer;
    }
}

/*----------------------------------------------------------------------------*/

static void add_datum(ov_value *value, void *additional) {

    JsonValueSerde *serde = as_json_value_serde(additional);

    if (0 == value) {
        return;

    } else if (0 == serde) {

        ov_log_error("Cannot process parsed value - serde object invalid");
        ov_value_free(value);
        return;

    } else if (0 == serde->ready_values) {

        serde->ready_values = joint(value);
        serde->last_value = serde->ready_values;
        return;

    } else {

        OV_ASSERT(0 != serde->last_value);
        serde->last_value->next = joint(value);
        serde->last_value = serde->last_value->next;
        return;
    }
}

/*----------------------------------------------------------------------------*/

static bool parse_incomplete(JsonValueSerde *serde, ov_result *result) {

    char const *remainder = 0;

    if (!ov_ptr_valid(serde, "Cannot parse JSON - invalid serde object")) {

        ov_result_set(result, OV_ERROR_INTERNAL_SERVER, "Invalid serde object");
        return false;

    } else if (0 == serde->incomplete_input) {

        return true;

    } else if (!ov_value_parse_stream(
                   (char const *)serde->incomplete_input->start,
                   serde->incomplete_input->length, add_datum, serde,
                   &remainder)) {

        ov_result_set(result, OV_ERROR_CODE_INPUT_ERROR,
                      "Could not parse input");
        return false;

    } else if (0 == remainder) {

        return true;

    } else {

        update_incomplete_input(serde, remainder);
        return true;
    }
}

/*----------------------------------------------------------------------------*/

static ov_serde_data impl_pop_datum(ov_serde *self, ov_result *res) {

    JsonValueSerde *serde = as_json_value_serde(self);

    if (!ov_ptr_valid(serde, "Cannot pop next datum: Invalid serde object")) {

        ov_result_set(res, OV_ERROR_INTERNAL_SERVER, "Invalid serde object");
        return OV_SERDE_DATA_NO_MORE;

    } else {

        if ((0 == serde->ready_values) && (!parse_incomplete(serde, res))) {
            return OV_SERDE_DATA_NO_MORE;
        }

        ov_serde_data data = {
            .data_type = OV_VALUE_SERDE_TYPE,
            .data = pop_ready_value(serde),
        };

        if (0 == data.data) {
            return OV_SERDE_DATA_NO_MORE;
        } else {
            return data;
        }
    }
}

/*----------------------------------------------------------------------------*/

static bool impl_clear_buffer(ov_serde *self) {

    JsonValueSerde *serde = as_json_value_serde(self);

    if (0 != serde) {

        for (Joint *j = serde->ready_values; 0 != j; j = joint_free(j))
            ;

        serde->incomplete_input = ov_buffer_free(serde->incomplete_input);
        serde->last_value = 0;

        return true;

    } else {

        return false;
    }
}

/*----------------------------------------------------------------------------*/

static bool write_string(int fh, char const *str, ov_result *res) {

    if (0 == str) {

        ov_result_set(res, OV_ERROR_CODE_INPUT_ERROR,
                      "Could not serialize datum to string");
        return false;

    } else if (!ov_cond_valid(-1 < fh,
                              "Cannot serialize: Invalid file handle")) {

        ov_result_set(res, OV_ERROR_CODE_INPUT_ERROR,
                      "Cannot serialize: Invalid file handle");
        return false;

    } else {

        size_t len = strlen(str);
        ssize_t written_len = write(fh, str, len);

        if ((written_len < 0) || ((size_t)written_len != len)) {

            ov_log_error("Cannot serialize: Cannot write to handle %i", fh);
            ov_result_set(res, OV_ERROR_INTERNAL_SERVER,
                          "Cannot serialize: Cannot write to file handle");
            return false;

        } else {

            return true;
        }
    }
}

/*----------------------------------------------------------------------------*/

static bool impl_serialize(ov_serde *self, int fh, ov_serde_data data,
                           ov_result *res) {

    UNUSED(self);

    if (!ov_cond_valid(OV_VALUE_SERDE_TYPE == data.data_type,
                       "Unexpected data: Wrong data type")) {

        ov_result_set(res, OV_ERROR_CODE_INPUT_ERROR,
                      "Cannot serialize: Wrong data type");
        return false;

    } else {

        char *serialized = ov_value_to_string(data.data);
        bool ok = write_string(fh, serialized, res);
        serialized = ov_free(serialized);

        return ok;
    }
}

/*----------------------------------------------------------------------------*/

static ov_serde *impl_free(ov_serde *self) {

    JsonValueSerde *serde = as_json_value_serde(self);

    if (0 != serde) {

        impl_clear_buffer(self);
        serde = ov_free(serde);
        return 0;

    } else {

        return self;
    }
}

/*----------------------------------------------------------------------------*/

ov_serde *ov_value_serde_create() {

    JsonValueSerde *serde = calloc(1, sizeof(JsonValueSerde));

    serde->public = (ov_serde){
        .magic_bytes = MAGIC_BYTES,
        .add_raw = impl_add_raw,
        .pop_datum = impl_pop_datum,
        .clear_buffer = impl_clear_buffer,
        .serialize = impl_serialize,
        .free = impl_free,

    };

    serde->incomplete_input = 0;

    return (ov_serde *)serde;
}

/*----------------------------------------------------------------------------*/

ov_serde_state ov_value_serde_serialize(ov_serde *self, int fh,
                                        ov_value const *value, ov_result *res) {

    ov_serde_data datum = {
        .data_type = OV_VALUE_SERDE_TYPE,
        .data = (void *)value,
    };

    return ov_serde_serialize(self, fh, datum, res);
}

/*----------------------------------------------------------------------------*/

ov_value *ov_value_serde_pop_datum(ov_serde *self, ov_result *res) {

    ov_serde_data data = ov_serde_pop_datum(self, res);

    if (OV_VALUE_SERDE_TYPE != data.data_type) {

        ov_result_set(res, OV_ERROR_CODE_INPUT_ERROR,
                      "Cannot get next value from serde: Wrong data type");
        return 0;

    } else {

        return data.data;
    }
}

/*----------------------------------------------------------------------------*/

void ov_value_serde_enable_caching(size_t capacity) {

    ov_registered_cache_config cfg = {

        .capacity = capacity,
        .item_free = joint_free_vptr,

    };

    g_joint_cache = ov_registered_cache_extend("value_serde_joint", cfg);
    ov_value_enable_caching(capacity, capacity, capacity, capacity);
}

/*----------------------------------------------------------------------------*/
