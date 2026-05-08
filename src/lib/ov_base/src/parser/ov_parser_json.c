/***
        ------------------------------------------------------------------------

        Copyright (c) 2019 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_parser_json.c
        @author         Markus Toepfer
        @author         Michael Beer

        @date           2019-01-31

        @ingroup        ov_parser_json

        @brief          Implementation of ov_parser_json*


        ------------------------------------------------------------------------
*/
#include "ov_parser_json.h"

#include "../../include/ov_buffer.h"
#include "../../include/ov_json.h"

#include "../../include/ov_utils.h"

#define MAGIC_BYTE 0x3101

typedef struct {

    ov_parser public;
    ov_parser_config config;

    ov_json_stringify_config stringify;

    ov_buffer *buffer;

} JsonParser;

/*---------------------------------------------------------------------------*/

#define AS_JSON_PARSER(x)                                                      \
    (((ov_parser_cast(x) != 0) && (MAGIC_BYTE == ((ov_parser *)x)->type))      \
         ? (JsonParser *)(x)                                                   \
         : 0)

/*
 *      ------------------------------------------------------------------------
 *
 *      PRIVATE FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

static ov_parser *impl_parser_json_free(ov_parser *self) {

    JsonParser *parser = AS_JSON_PARSER(self);
    if (!parser)
        goto error;

    ov_buffer_free(parser->buffer);
    free(parser);

    return NULL;
error:
    return self;
}

/*----------------------------------------------------------------------------*/

static bool impl_parser_json_buffer_is_enabled(const ov_parser *self) {

    JsonParser *parser = AS_JSON_PARSER(self);
    if (!parser)
        goto error;

    return parser->config.buffering;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool impl_parser_json_buffer_has_data(const ov_parser *self) {

    JsonParser *parser = AS_JSON_PARSER(self);
    if (!parser || !parser->buffer || !parser->buffer->start)
        goto error;

    if (!impl_parser_json_buffer_is_enabled(self))
        goto error;

    if (0 < parser->buffer->length)
        return true;

error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool impl_parser_json_buffer_empty_out(ov_parser *self, void **raw,
                                              void *(**free_raw)(void *)) {

    JsonParser *parser = AS_JSON_PARSER(self);
    if (!parser || !raw || !free_raw)
        return false;

    *free_raw = NULL;
    *raw = NULL;

    if (impl_parser_json_buffer_has_data(self)) {

        *raw = parser->buffer;
        parser->buffer = NULL;
        *free_raw = ov_buffer_free;
    }

    return true;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      DECODE
 *
 *      ------------------------------------------------------------------------
 */

static ov_parser_state decode_unbuffered(JsonParser *parser,
                                         ov_parser_data *data) {

    ov_json_value *value = NULL;
    ov_buffer *input = ov_buffer_cast(data->in.data);

    if (!parser || !input)
        goto error;

    /*
     *      Decode the buffer to a JSON value
     */

    if (-1 ==
        ov_json_parser_decode(&value, (char *)input->start, input->length))
        goto mismatch;

    if (data->in.free)
        data->in.free(data->in.data);

    data->in.data = NULL;
    data->in.free = NULL;

    data->out.data = value;
    data->out.free = ov_json_value_free;
    return OV_PARSER_SUCCESS;

mismatch:
    ov_json_value_free(value);
    return OV_PARSER_MISMATCH;

error:
    ov_json_value_free(value);
    return OV_PARSER_ERROR;
}

/*----------------------------------------------------------------------------*/

static bool parser_buffer_input(JsonParser *parser, ov_buffer *input) {

    if (!parser || !input)
        goto error;

    if (!parser->buffer) {

        parser->buffer = ov_buffer_create(input->length);
        if (!parser->buffer)
            goto error;
    }

    if (!ov_buffer_push(parser->buffer, input->start, input->length))
        goto error;

    return true;

error:
    return false;
}

/*----------------------------------------------------------------------------*/

static ov_parser_state decode_buffered(JsonParser *parser,
                                       ov_parser_data *data) {

    ov_json_value *value = NULL;
    ov_buffer *input = NULL;

    if (NULL != data->in.data) {

        input = ov_buffer_cast(data->in.data);
        if (!input)
            goto error;

        if (!parser_buffer_input(parser, input)) {
            ov_log_error("failed to buffer input");
            goto error;
        }

        if (data->in.free)
            data->in.free(data->in.data);

        data->in.data = NULL;
        data->in.free = NULL;
    }

    if (!parser->buffer)
        goto error;

    uint8_t *start = parser->buffer->start;
    size_t length = parser->buffer->length;

    uint8_t *end = NULL;
    uint8_t *last = NULL;

    /*      Implementation in strict mode, the buffering parser
     *      requires a JSON object as input to use
     *      the brackets count as framing information.
     */

    if (!ov_json_clear_whitespace(&start, &length)) {
        goto mismatch;
    }

    if (0 == start[0])
        goto progress;

    if (start[0] != '{') {
        goto mismatch;
    }

    if (!ov_json_match(parser->buffer->start, parser->buffer->length, true,
                       &last)) {
        goto mismatch;
    }

    if ((0 == last) || (last[0] != '}')) {
        goto progress;
    }

    /*      It is important to check if the bracket returned
     *      in last, is the matching bracket of start.
     *
     *      The fastet way is to count the brackets inbetween.
     *      This is what ov_json_match_object is doing. If it
     *      is not matching, the object is incomplete.
     */
    end = last;
    if (!ov_json_match_object(&start, &end, (end - start) + 1))
        goto progress;

    /*      If an object is matched as complete,
     *      parse the object.
     *
     *      Allow receive if additional JSON data,
     *      e.g. start of another JSON string.
     */
    size_t parsed_len = (last + 1) - parser->buffer->start;

    if (-1 == ov_json_parser_decode(&value, (char *)parser->buffer->start,
                                    parsed_len))
        goto error;

    if (parser->buffer->length > parsed_len) {

        /*      SHIFT the content of the parser buffer,
         *      the encoded data can be deleted, as the
         *      JSON object is already parsed.
         */
        uint8_t temp[parser->buffer->length + 1];
        memset(temp, 0, parser->buffer->length + 1);

        if (!memcpy(temp, last + 1, parser->buffer->length - parsed_len))
            goto error;

        if (!ov_buffer_set(parser->buffer, temp,
                           parser->buffer->length - parsed_len))
            goto error;

    } else {

        // full buffer parsed, clear buffer
        ov_buffer_clear(parser->buffer);
    }

    data->out.data = value;
    data->out.free = ov_json_value_free;
    return OV_PARSER_SUCCESS;

progress:

    data->out.data = NULL;
    data->out.free = NULL;

    return OV_PARSER_PROGRESS;

mismatch:

    data->out.data = NULL;
    data->out.free = NULL;
    ov_json_value_free(value);
    return OV_PARSER_MISMATCH;
error:
    ov_json_value_free(value);
    return OV_PARSER_ERROR;
}

/*----------------------------------------------------------------------------*/

static ov_parser_state impl_parser_json_decode(ov_parser *self,
                                               ov_parser_data *data) {

    JsonParser *parser = AS_JSON_PARSER(self);
    if (!parser || !data)
        goto error;

    if (data->in.data && !ov_buffer_cast(data->in.data)) {
        ov_log_error("parser input not ov_buffer, stopping");
        goto error;
    }

    if (data->out.data) {
        ov_log_error("parser output data slot not clean, stopping");
        goto error;
    }

    data->out.free = NULL;
    data->out.data = NULL;

    if (true == parser->config.buffering)
        return decode_buffered(parser, data);

    return decode_unbuffered(parser, data);

error:
    return OV_PARSER_ERROR;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      ENCODE
 *
 *      ------------------------------------------------------------------------
 */

static ov_parser_state impl_parser_json_encode(ov_parser *self,
                                               ov_parser_data *data) {

    ov_buffer *buffer = NULL;

    JsonParser *parser = AS_JSON_PARSER(self);
    if (!parser || !data)
        goto error;

    if (!ov_json_value_cast(data->in.data)) {
        ov_log_error("parser input not ov_json_value, stopping");
        goto error;
    }

    if (data->out.data) {
        ov_log_error("parser output data slot not clean, stopping");
        goto error;
    }

    data->out.free = NULL;
    data->out.data = NULL;

    // only unbuffered encoding supported

    int64_t bytes = ov_json_parser_calculate(data->in.data, &parser->stringify);
    if (-1 == bytes)
        goto error;

    buffer = ov_buffer_create(bytes + 1);
    if (!buffer)
        goto error;

    bytes = ov_json_parser_encode(
        data->in.data, &parser->stringify, ov_json_parser_collocate_ascending,
        (char *)buffer->start, (int64_t)buffer->capacity);

    if (-1 == bytes)
        goto error;

    if (data->in.free)
        data->in.free(data->in.data);

    data->in.data = NULL;
    data->in.free = NULL;

    buffer->length = bytes;
    data->out.data = buffer;
    data->out.free = ov_buffer_free;
    return true;
error:
    ov_buffer_free(buffer);
    return OV_PARSER_ERROR;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      INIT
 *
 *      ------------------------------------------------------------------------
 */

static bool json_parser_init(JsonParser *jp, ov_parser_config config,
                             ov_json_stringify_config stringify) {

    if (!jp)
        goto error;

    if (!ov_parser_set_head((ov_parser *)jp, MAGIC_BYTE))
        goto error;

    if (0 != config.name[0])
        strncpy(jp->public.name, config.name, OV_PARSER_NAME_MAX);

    jp->config = config;
    jp->stringify = stringify;

    // FUNCTION INIT
    jp->public.free = impl_parser_json_free;
    jp->public.decode = impl_parser_json_decode;
    jp->public.encode = impl_parser_json_encode;

    jp->public.buffer.is_enabled = impl_parser_json_buffer_is_enabled;
    jp->public.buffer.has_data = impl_parser_json_buffer_has_data;
    jp->public.buffer.empty_out = impl_parser_json_buffer_empty_out;

    // STATE DATA INIT
    // ... none ...

    return jp;

error:
    if (jp)
        memset(jp, 0, sizeof(JsonParser));
    return false;
}
/*
 *      ------------------------------------------------------------------------
 *
 *      PUBLIC FUNCTION IMPLEMENTATION
 *
 *      ------------------------------------------------------------------------
 */

/*----------------------------------------------------------------------------*/

ov_parser *ov_parser_json_create(ov_parser_config config,
                                 ov_json_stringify_config stringify) {

    // check config
    if (!config.name[0])
        strncat(config.name, OV_PARSER_JSON_NAME, OV_PARSER_NAME_MAX - 1);

    JsonParser *parser = calloc(1, sizeof(JsonParser));
    if (!parser)
        goto error;

    if (json_parser_init(parser, config, stringify))
        return (ov_parser *)parser;

    free(parser);
error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_parser *ov_parser_json_create_default(ov_parser_config config) {

    ov_json_stringify_config stringify = ov_json_config_stringify_minimal();
    return ov_parser_json_create(config, stringify);
}
