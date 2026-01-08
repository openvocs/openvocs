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
        @file           ov_parser.h
        @author         Markus Toepfer

        @date           2019-01-30

        @ingroup        ov_parser

        @brief          Definition of a parser interface for ov_base

                        A ov_parser SHOULD be able to parse some defined input
                        to some defined output.

                        The parser defines the input it accepts, as well as
                        the output it creates.

                        ov_parsers are configurable in the way to use some
                        kind of optional factory, work in buffering or strict
                        mode and MAY be chained with other parsers.

                        A parser MAY reject a config during creation time.
                        If the parser is not rejecting the config, all configured
                        functionality MUST be supported by the implementation.

                        Based on a parser config, a chained list of parsers
                        may be created.

                        OV_PARSER_ANSWER &
                        OV_PARSER_ANSWER_CLOSE ... will send some answer back to
                        the requester. The answer MUST be a buffer of RAW bytes
                        send at the socket. So in case of OV_PARSER_ANSWER*

                        ov_parser_data->out.data        MUST be ov_buffer.
                        ov_parser_data->out.data.free   MUST be ov_buffer_free.

        ------------------------------------------------------------------------
*/
#ifndef ov_parser_h
#define ov_parser_h

#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>

#define OV_PARSER_NAME_MAX 100

/*----------------------------------------------------------------------------*/

typedef struct ov_parser ov_parser;

/*----------------------------------------------------------------------------*/

typedef struct {

  bool buffering; // enable buffering mode
  bool debug;     // (optional) debug

  char name[OV_PARSER_NAME_MAX]; // (optional) custom name
  uint8_t flags;                 // (optional) custom flags
  void *custom;                  // (optional) custom data

} ov_parser_config;

/*----------------------------------------------------------------------------*/

typedef enum {

  OV_PARSER_ERROR = -2,       // processing error
  OV_PARSER_MISMATCH = -1,    // content mismatch
  OV_PARSER_PROGRESS = 0,     // still matching, need more input data
  OV_PARSER_SUCCESS = 1,      // content match
  OV_PARSER_ANSWER = 2,       // use to request more information
  OV_PARSER_ANSWER_CLOSE = 3, // send some (error) answer before close
  OV_PARSER_CLOSE = 4,        // parser close (e.g. websocket close)
  OV_PARSER_DONE = 5,         // done with processing

} ov_parser_state;

/*----------------------------------------------------------------------------*/

const char *ov_parser_state_to_string(ov_parser_state state);
ov_parser_state ov_parser_state_from_string(const char *string, size_t length);

/*----------------------------------------------------------------------------*/

typedef struct {

  struct {

    void *data;
    void *(*free)(void *input_data);

  } in;

  struct {

    void *data;
    void *(*free)(void *output_data);

  } out;

} ov_parser_data;

void ov_parser_data_clear_in(ov_parser_data *const data);
void ov_parser_data_clear_out(ov_parser_data *const data);

/*----------------------------------------------------------------------------
 *
 *      @NOTE all functions of ov_parser MUST be related to the current
 *      implementation of the parser, and MUST ignore next parsing.
 *
 *      To use free, parse and buffer functions for in chained mode see
 *
 *              @ov_parser_free
 *              @ov_parser_decode
 *              @ov_parser_encode
 *              @ov_parser_empty_out_buffer
 *
 *
 *      @NOTE in buffered MODE parser->parse SHOULD parse buffered content,
 *      even if data->in.data == NULL
 *
 *----------------------------------------------------------------------------*/

struct ov_parser {

  uint16_t magic_byte;
  uint16_t type;

  char name[OV_PARSER_NAME_MAX];

  ov_parser *(*free)(ov_parser *self);

  ov_parser_state (*decode)(ov_parser *self, ov_parser_data *const data);

  ov_parser_state (*encode)(ov_parser *self, ov_parser_data *const data);

  // data buffering (if enabled)
  struct {

    bool (*is_enabled)(const ov_parser *self);

    // check if the parser has some (more) bufferd data
    bool (*has_data)(const ov_parser *self);

    // get whatever raw data the parser buffer has set
    bool (*empty_out)(ov_parser *self, void **raw, void *(**free_raw)(void *));
  } buffer;

  // pointer to next if chained (MAY be set or NOT)
  ov_parser *next;
};

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

/**
        Default cast to identify a pointer as ov_parser
*/
ov_parser *ov_parser_cast(const void *data);

/*----------------------------------------------------------------------------*/

/**
        Cast including function pointer check.
        (function pointers are set)
*/
bool ov_parser_verify_interface(const void *parser);

/*----------------------------------------------------------------------------*/

/**
        Set magic_byte and type
*/
ov_parser *ov_parser_set_head(ov_parser *parser, uint16_t type);

/*----------------------------------------------------------------------------*/

/**
        Standard free from external.
        (the whole chain if parser->next)

        @returns pointer to last failed free of the chain (MAY not be self)
        @returns NULL on success
*/
void *ov_parser_free(void *self);

/*----------------------------------------------------------------------------*/

/**
        Default function to empty out the parser buffer.
        (the whole chain if parser->next)

        @param self     pointer to parser
*/
bool ov_parser_trash_buffered_data(ov_parser *self);

/*----------------------------------------------------------------------------*/

/**
        Use the parser to decode the input data to the output data.
        (the whole chain if parser->next)

        In chained mode the output of the parser will be set as input
        to the next parser. If an input free function is provided, the
        input will be freed using this function.

        @NOTE   if data->input.data is pointer data, just don't set
                data->input.free

        @param self     pointer to start parser
        @param data     pointer to data structure for IO data
*/
ov_parser_state ov_parser_decode(ov_parser *self, ov_parser_data *const data);

/*----------------------------------------------------------------------------*/

/**
        Use the parser to encode the input data to the output data.
        (the whole chain if parser->next)

        In chained mode the output of the parser will be set as input
        to the next parser. If an input free function is provided, the
        input will be freed using this function.

        @NOTE   if data->input.data is pointer data, just don't set
                data->input.free

        @param self     pointer to start parser
        @param data     pointer to data structure for IO data
*/
ov_parser_state ov_parser_encode(ov_parser *self, ov_parser_data *const data);

/*----------------------------------------------------------------------------*/

/**
        Clear the parser data using the functions set.
*/
bool ov_parser_data_clear(ov_parser_data *data);

/*
 *      ------------------------------------------------------------------------
 *
 *      DUMMY IMPLEMENTATION
 *
 *      ------------------------------------------------------------------------
 */

/**
        Implementation of a dummy parser for testing.

        This parser expects some pointer data as input and will set the
        same pointer to output.

        It will not support buffering.
        It will not free input.

        INPUT  - whatever
        OUTPUT - INPUT
*/
ov_parser *ov_parser_dummy_create(ov_parser_config config);

#endif /* ov_parser_h */
