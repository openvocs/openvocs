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
        @file           ov_parser_json.h
        @author         Markus Toepfer

        @date           2019-01-31

        @ingroup        ov_parser_json

        @brief          Definition of JSON parsersing using the ov_parser
                        interface.


        ------------------------------------------------------------------------
*/
#ifndef ov_parser_json_h
#define ov_parser_json_h

#define OV_PARSER_JSON_NAME "JSON_PARSER"

#include "ov_json.h"
#include "ov_parser.h"

/**
        Create a independent JSON parser instance.

        INPUT   ov_buffer
        OUTPUT  ov_json_value

        STRICT MODE

        In strict mode the parser will accept valid JSON object strings.
        In non strict mode the parser will accept valid JSON strings.

        BUFFERING MODE

        (1) In buffering mode this parser will work in strict mode.
        (2) In buffering mode this parser will work "streamalike".
            The buffer will work for a countinious stream of JSON objects
            following each other. Each OV_PARSER_SUCCESS means the next
            JSON object of the stream was successfully parsed.
            So after success, the parser MAY hold some additional data.
            To check if all input to the parser was actually parsed
            use parser->buffer.has_data(parser) after success.
*/
ov_parser *ov_parser_json_create(ov_parser_config config,
                                 ov_json_stringify_config stringify);

ov_parser *ov_parser_json_create_default(ov_parser_config config);

#endif /* ov_parser_json_h */
