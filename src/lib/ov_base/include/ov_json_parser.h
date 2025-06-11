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
        @file           ov_json_parser.h
        @author         Markus Toepfer

        @date           2018-12-05

        @ingroup        ov_json_parser

        @brief          Definition of an interface for JSON parser
                        implementations, as well as provision of
                        a default JSON string parser implementation.

        @decode         Decode will parse any valid json value string.
                        e.g. "null" to create an ov_json_value null.

                        The parser will a provided value, if the
                        value is of the same type as the parsed content.

        @encode         Encode will write any JSON value to the buffer,
                        if the size of the buffer is big enough.

                        Encode MAY use an optional provided stringify config
                        to create better human readable string.
                        @see ov_json_stringify_config_default

                        Encode MAY use an optional function to sort object
                        keys, using the custom collocation function to order
                        keys at all levels, before stringify.

        @calculate      Will calculate the required encoding size,
                        based on an optional stringify config.

        ------------------------------------------------------------------------
*/
#ifndef ov_json_parser_h
#define ov_json_parser_h

typedef struct ov_json_parser ov_json_parser;
typedef struct ov_json_stringify_config ov_json_stringify_config;

#include <ctype.h>
#include <math.h>

#include "ov_json_grammar.h"
#include "ov_json_value.h"
#include <ov_test/testrun.h>

/*----------------------------------------------------------------------------*/

/**
 *      Decode some string to ov_json_value
 *
 *      @params out             out pointer to fill
 *      @params in              buffer to decode
 *      @params size            buffer size to decode
 *
 *      @returns                length of decoded string,
 *                              -1 on error
 */
int64_t ov_json_parser_decode(ov_json_value **out, const char *in, size_t size);

/*----------------------------------------------------------------------------*/

/**
 *      Encode some ov_json_value to string.
 *
 *      @params value           value to encode
 *      @params stringify       stringify config to use
 *      @params collocation     optional collocation function
 *      @params buffer          buffer to write to
 *      @params size            buffer size
 *
 *      @returns        length of encoded string,
 *                      -1 on error
 */
int64_t ov_json_parser_encode(const ov_json_value *value,
                              const ov_json_stringify_config *stringify,
                              bool (*collocation)(const char *key,
                                                  ov_list *list),
                              char *buffer,
                              size_t size);

/*----------------------------------------------------------------------------*/

/**
 *      Calculate some ov_json_value string length.
 *
 *      @params value           value to encode
 *      @params stringify       stringify config to use
 *
 *      @returns                length of encoded string,
 *                              -1 on error
 */
int64_t ov_json_parser_calculate(const ov_json_value *value,
                                 const ov_json_stringify_config *stringify);

/*----------------------------------------------------------------------------*/

/**
        Collocate all keys of a JSON object byte based ascending.

        The input key MUST be placed at the correct position within list.

        @param key      key pointer to add to a given list
        @param list     list of already ordered keys (pointer only list)
*/
bool ov_json_parser_collocate_ascending(const char *key, ov_list *list);

/*
 *      ------------------------------------------------------------------------
 *
 *      STRINGIFY CONFIGS
 *
 *      ... full customizable stringify setup
 *
 *      ------------------------------------------------------------------------
 */

/*----------------------------------------------------------------------------*/

typedef struct ov_json_value_config {

    struct entry {

        char *intro;  //      ... item intro e.g. space, quote
        char *outro;  //      ... item intro e.g. space, quote
        bool depth;   //      ... enable depth
        char *indent; //      ... whitespace indent (e.g. tab)
    } entry;

    struct item {

        char *intro;     // {    ... intro after depth
        char *out;       // \n   ... out before depth
        char *outro;     // }    ... outro after depth
        char *separator; // ,    ... separation of collection items
        char *delimiter; // :    ... delimiter within object

    } item;

} ov_json_value_config;

/*----------------------------------------------------------------------------*/

typedef struct ov_json_stringify_config {

    char *intro;

    ov_json_value_config literal;
    ov_json_value_config number;
    ov_json_value_config string;
    ov_json_value_config array;
    ov_json_value_config object;

    char *outro;

} ov_json_stringify_config;

/*----------------------------------------------------------------------------*/

/**
        Use a stringify config to create a string.
*/
char *ov_json_value_to_string_with_config(const ov_json_value *value,
                                          ov_json_stringify_config config);

/*----------------------------------------------------------------------------*/

bool ov_json_stringify_config_validate(const ov_json_stringify_config *config);

/*----------------------------------------------------------------------------*/

/**
        A minial stringify config without whitespaces.

        Example:

        {"key1":null,"key2":1,"key3":"string","key4":[],"key5":[1,2,3],...}
*/
ov_json_stringify_config ov_json_config_stringify_minimal();

/*----------------------------------------------------------------------------*/

/**
        A default stringify config with whitespaces and depth.

        Example:

        {
                "key1" : null,
                "key2" : 1,
                "key3" : "string",
                "key4" : [],
                "key5" :
                [
                        1,
                        2,
                        3
                ],
                "key6" : {},
                "key7" :
                {
                        "key7.1" : {},
                        "key7.2" :
                        {
                                "key7.3" : null,
                                "key7.4" : true
                        }
                }
        }
*/
ov_json_stringify_config ov_json_config_stringify_default();

#endif /* ov_json_parser_h */
