/***
        ------------------------------------------------------------------------

        Copyright (c) 2021 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_value_json.h

        @author         Michael J. Beer, DLR/GSOC
        @author         Markus Töpfer

        @date           2021-04-01


        ------------------------------------------------------------------------
*/
#ifndef ov_value_json_h
#define ov_value_json_h

#include "ov_buffer.h"
#include "ov_value.h"

/**
    Streaming parser of ov_value from buffer.

    @param in           input buffer
    @param remainder    pointer to next byte after parsed content
                        e.g. the next JSON string within the buffer.

    @returns            first complete ov_value within the input buffer
*/
ov_value *ov_value_parse_json(ov_buffer const *in, char const **remainder);

/*----------------------------------------------------------------------------*/

/**
   Parse some JSON string to ov_value.

   @param in    buffer with string to parse
   @returns parsed value or NULL if the string does not contain exactely one
   JSON value.
*/
ov_value *ov_value_from_json(ov_buffer const *in);

/*----------------------------------------------------------------------------*/

/**
    Create some JSON content buffer from a value.

    @param value    value to encode
    @returns buffer with encoded JSON string or NULL
*/
ov_buffer *ov_value_to_json(const ov_value *value);

#endif /* ov_value_json_h */
