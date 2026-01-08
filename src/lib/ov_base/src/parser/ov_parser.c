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
        @file           ov_parser.c
        @author         Markus Toepfer

        @date           2019-01-30

        @ingroup        ov_parser

        @brief          Implementation of a parser interface for ov_base


        ------------------------------------------------------------------------
*/
#include "ov_parser.h"

#include "../../include/ov_utils.h"
#include "ov_config_keys.h"
#include <ov_log/ov_log.h>

#include <string.h>

#define OV_PARSER_MAGIC_BYTE 0xd0d0

/*----------------------------------------------------------------------------*/

ov_parser *ov_parser_cast(const void *data) {

    if (!data)
        return NULL;

    if (*(uint16_t *)data == OV_PARSER_MAGIC_BYTE)
        return (ov_parser *)data;

    return NULL;
}

/*----------------------------------------------------------------------------*/

bool ov_parser_verify_interface(const void *data) {

    ov_parser *parser = ov_parser_cast(data);

    if (!parser || !parser->free || !parser->decode ||
        !parser->buffer.is_enabled)
        goto error;

    // in buffering mode the buffer MUST be checkable and cleanable
    if (parser->buffer.is_enabled(parser))
        if (!parser->buffer.has_data || !parser->buffer.empty_out)
            goto error;

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

ov_parser *ov_parser_set_head(ov_parser *parser, uint16_t type) {

    if (!parser)
        return NULL;

    parser->magic_byte = OV_PARSER_MAGIC_BYTE;
    parser->type = type;

    return parser;
}

/*----------------------------------------------------------------------------*/

void *parser_free_chain(void *self) {

    ov_parser *next = ov_parser_cast(self);
    ov_parser *current = NULL;

    if (!next)
        return self;

    do {
        current = next;
        next = current->next;

        if (!ov_parser_cast(current) || !current->free) {
            ov_log_error("parser pointer in chain "
                         "NOT ov_parser OR free missing "
                         "for parser at pointer %p",
                         current);
            goto error;
        }

        OV_ASSERT(current->free);
        current = current->free(current);

    } while (next);

    return NULL;
error:
    return current;
}

/*----------------------------------------------------------------------------*/

void *ov_parser_free(void *self) {

    ov_parser *parser = ov_parser_cast(self);
    if (!parser || !parser->free)
        return self;

    return parser_free_chain(self);
}

/*----------------------------------------------------------------------------*/

bool ov_parser_trash_buffered_data(ov_parser *self) {

    void *raw = NULL;
    void *(*free_raw)(void *) = NULL;

    ov_parser *parser = self;

    do {
        if (!ov_parser_verify_interface(parser)) {
            ov_log_error("could not verify parser interface of %p", parser);
            goto error;
        }

        OV_ASSERT(parser->buffer.is_enabled);
        OV_ASSERT(parser->buffer.has_data);
        OV_ASSERT(parser->buffer.empty_out);

        raw = NULL;
        free_raw = NULL;

        // some enabled buffer to trash?
        if (!parser->buffer.is_enabled(parser)) {
            parser = parser->next;
            continue;
        }

        // some buffer data to trash?
        if (!parser->buffer.has_data(self)) {
            parser = parser->next;
            continue;
        }

        if (!parser->buffer.empty_out(parser, &raw, &free_raw)) {
            ov_log_error("failed to empty out buffer of %p|%s", parser,
                         parser->name);
            goto error;
        }

        // apply function to free the raw pointer
        if (free_raw)
            free_raw(raw);

        parser = parser->next;

    } while (parser);

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_parser_data_clear(ov_parser_data *data) {

    if (!data)
        return false;

    ov_parser_data_clear_in(data);
    ov_parser_data_clear_out(data);

    return true;
}

/*----------------------------------------------------------------------------*/

void ov_parser_data_clear_in(ov_parser_data *const data) {

    if (!data)
        return;

    if (data->in.data && data->in.free)
        data->in.free(data->in.data);

    data->in.data = NULL;
    data->in.free = NULL;
    return;
}

/*----------------------------------------------------------------------------*/

void ov_parser_data_clear_out(ov_parser_data *const data) {

    if (!data)
        return;

    if (data->out.data && data->out.free)
        data->out.free(data->out.data);

    data->out.data = NULL;
    data->out.free = NULL;
    return;
}

/*----------------------------------------------------------------------------*/

static void switch_output_to_input(ov_parser_data *const data) {

    if (!data)
        return;

    // free current input
    if (data->in.free) {
        data->in.free(data->in.data);
        data->in.data = NULL;
        data->in.free = NULL;
    }

    // swap output to input
    data->in.data = data->out.data;
    data->in.free = data->out.free;
    data->out.data = NULL;
    data->out.free = NULL;

    return;
}

/*----------------------------------------------------------------------------*/
/*

        ITERATIVE IMPLEMENTATION

ov_parser_state ov_parser_decode(
        ov_parser *self,
        ov_parser_data * const data){

        if (!data)
                goto error;

        ov_parser_state state = OV_PARSER_ERROR;
        ov_parser *parser = ov_parser_cast(self);
        if (!parser)
                goto error;

        do {

                if (!ov_parser_verify_interface(parser)){
                        ov_log_error(   "could not verify parser %p|%s",
                                        parser, parser->name);
                        goto error;
                }

                OV_ASSERT(parser->decode);

                state = parser->decode(parser, data);

                switch (state) {

                        case OV_PARSER_SUCCESS:
                                break;

                        case OV_PARSER_PROGRESS:
                                return state;

                        case OV_PARSER_MISMATCH:
                                return state;

                        default:
                                return state;

                }

                parser = parser->next;

                // switch IO if a next parser run is requested
                if (parser)
                        switch_output_to_input(data);

        } while (parser);

        return state;
error:
        return OV_PARSER_ERROR;
}
*/
/*----------------------------------------------------------------------------*/

ov_parser_state ov_parser_decode(ov_parser *self, ov_parser_data *const data) {

    if (!data)
        goto error;

    ov_parser *parser = ov_parser_cast(self);
    if (!parser || !parser->decode)
        goto error;

    ov_parser_state state = parser->decode(parser, data);

    ov_parser *next = parser->next;

    if (next) {

        if (state == OV_PARSER_SUCCESS) {

            switch_output_to_input(data);
            state = ov_parser_decode(next, data);
        }
    }

    return state;

error:
    return OV_PARSER_ERROR;
}

/*----------------------------------------------------------------------------*/

ov_parser_state ov_parser_encode(ov_parser *self, ov_parser_data *const data) {

    if (!data)
        goto error;

    ov_parser *parser = ov_parser_cast(self);
    if (!parser || !parser->encode)
        goto error;

    ov_parser *next = parser->next;

    if (next) {

        ov_parser_state state = ov_parser_encode(next, data);
        if (state != OV_PARSER_SUCCESS)
            return state;

        switch_output_to_input(data);
    }

    return parser->encode(parser, data);

error:
    return OV_PARSER_ERROR;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      DUMMY
 *
 *      ------------------------------------------------------------------------
 */

#define IMPL_DUMMY_PARSER 0

#define AS_DUMMY_PARSER(x)                                                     \
    (((ov_parser_cast(x) != 0) &&                                              \
      (IMPL_DUMMY_PARSER == ((ov_parser *)x)->type))                           \
         ? (ov_parser *)(x)                                                    \
         : 0)

/*----------------------------------------------------------------------------*/

static ov_parser *impl_dummy_parser_free(ov_parser *self) {

    ov_parser *parser = AS_DUMMY_PARSER(self);
    if (!parser)
        return self;

    free(parser);
    return NULL;
}

/*----------------------------------------------------------------------------*/

static ov_parser_state impl_dummy_parser_decode(ov_parser *self,
                                                ov_parser_data *const data) {

    ov_parser *parser = AS_DUMMY_PARSER(self);
    if (!parser || !data)
        goto error;

    if (!data->in.data)
        goto error;

    data->out.data = data->in.data;
    data->out.free = data->in.free;

    return OV_PARSER_SUCCESS;
error:
    return OV_PARSER_ERROR;
}

/*----------------------------------------------------------------------------*/

static ov_parser_state impl_dummy_parser_encode(ov_parser *self,
                                                ov_parser_data *const data) {

    ov_parser *parser = AS_DUMMY_PARSER(self);
    if (!parser || !data)
        goto error;

    if (!data->in.data)
        goto error;

    data->out.data = data->in.data;
    data->out.free = data->in.free;

    return OV_PARSER_SUCCESS;
error:
    return OV_PARSER_ERROR;
}

/*----------------------------------------------------------------------------*/

static bool impl_dummy_parser_buffer_is_enabled(const ov_parser *self) {

    ov_parser *parser = AS_DUMMY_PARSER(self);
    if (!parser)
        return false;

    return false;
}

/*----------------------------------------------------------------------------*/

static bool impl_dummy_parser_buffer_has_data(const ov_parser *self) {

    ov_parser *parser = AS_DUMMY_PARSER(self);
    if (!parser)
        return false;

    return false;
}

/*----------------------------------------------------------------------------*/

static bool impl_dummy_parser_buffer_empty_out(ov_parser *self, void **raw,
                                               void *(**free_raw)(void *)) {

    ov_parser *parser = AS_DUMMY_PARSER(self);
    if (!parser || !raw || !free_raw)
        return false;

    return false;
}

/*----------------------------------------------------------------------------*/

ov_parser *ov_parser_dummy_create(ov_parser_config config) {

    // check config
    if (true == config.buffering) {
        ov_log_error("DUMMY parser will not support buffering.");
        return NULL;
    }

    ov_parser *parser = calloc(1, sizeof(ov_parser));
    if (!parser)
        goto error;

    if (!ov_parser_set_head(parser, IMPL_DUMMY_PARSER)) {
        free(parser);
        parser = NULL;
        goto error;
    }

    // apply config
    if (config.name[0]) {
        strncpy(parser->name, config.name, OV_PARSER_NAME_MAX);
    } else {
        strncpy(parser->name, "DUMMY", OV_PARSER_NAME_MAX);
    }

    // set functions
    parser->free = impl_dummy_parser_free;
    parser->decode = impl_dummy_parser_decode;
    parser->encode = impl_dummy_parser_encode;
    parser->buffer.is_enabled = impl_dummy_parser_buffer_is_enabled;
    parser->buffer.has_data = impl_dummy_parser_buffer_has_data;
    parser->buffer.empty_out = impl_dummy_parser_buffer_empty_out;

    return parser;
error:
    impl_dummy_parser_free(parser);
    return NULL;
}

/*----------------------------------------------------------------------------*/

const char *ov_parser_state_to_string(ov_parser_state state) {

    switch (state) {

    case OV_PARSER_ERROR:
        return OV_KEY_ERROR;

    case OV_PARSER_MISMATCH:
        return OV_KEY_MISMATCH;

    case OV_PARSER_PROGRESS:
        return OV_KEY_PROGRESS;

    case OV_PARSER_SUCCESS:
        return OV_KEY_SUCCESS;

    case OV_PARSER_ANSWER:
        return OV_KEY_ANSWER;

    case OV_PARSER_ANSWER_CLOSE:
        return OV_KEY_ANSWER_CLOSE;

    case OV_PARSER_CLOSE:
        return OV_KEY_CLOSE;

    case OV_PARSER_DONE:
        return OV_KEY_DONE;
    }

    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_parser_state ov_parser_state_from_string(const char *string, size_t length) {

    if (!string || 0 == length)
        goto error;

    if (0 == strncmp(OV_KEY_SUCCESS, string, length))
        if (length == strlen(OV_KEY_SUCCESS))
            return OV_PARSER_SUCCESS;

    if (0 == strncmp(OV_KEY_MISMATCH, string, length))
        if (length == strlen(OV_KEY_MISMATCH))
            return OV_PARSER_MISMATCH;

    if (0 == strncmp(OV_KEY_PROGRESS, string, length))
        if (length == strlen(OV_KEY_PROGRESS))
            return OV_PARSER_PROGRESS;
error:
    return OV_PARSER_ERROR;
}
