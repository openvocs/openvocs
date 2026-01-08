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

        @author Michael J. Beer, DLR/GSOC
        @copyright (c) 2021 German Aerospace Center DLR e.V. (GSOC)

        ------------------------------------------------------------------------
*/
#ifndef OV_VALUE_PARSE_H
#define OV_VALUE_PARSE_H

#include "ov_buffer.h"
#include "ov_result.h"
#include "ov_value.h"

/*----------------------------------------------------------------------------*/

/**
 * Tries to parse a value from `in`.
 *
 * If an incomplete string is passed in, 0 is returned, remainder is set to
 * `in.start` .
 *
 * On error, 0 is returned and remainder is set to 0.
 *
 * 0 might be passed in as `remainder`.
 *
 * @param remainder pointer to end of parsed partial string (optional)
 */
ov_value *ov_value_parse(ov_buffer const *in, char const **remainder);

/**
 * Keeps parsing values from in until end of string or no complete value found.
 * Calls value_consumer on each parsed value.
 *
 * On error, false is returned and remainder is set to 0.
 *
 * @param in String to parse from
 * @param in_len_bytes length of string to parse from
 * @param value_consumer function to call on each parsed value
 * @param userdata additional pointer to pass to `value_consumer` (optional)
 * @param remainder will be pointed behind last parsed character (optional)
 */
bool ov_value_parse_stream(char const *in, size_t in_len_bytes,
                           void (*value_consumer)(ov_value *, void *),
                           void *userdata, char const **remainder);

/*----------------------------------------------------------------------------*/
#endif
