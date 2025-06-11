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
        @file           ov_json_string.h
        @author         Markus Toepfer

        @date           2018-12-03

        @ingroup        ov_json_string

        ------------------------------------------------------------------------
*/
#ifndef ov_json_string_h
#define ov_json_string_h

#include <stdbool.h>
#include <stdlib.h>

#include "ov_data_function.h"

#include "ov_json_grammar.h"
#include "ov_json_value.h"

/*
 *      ------------------------------------------------------------------------
 *
 *      DEFAULT
 *
 *      ------------------------------------------------------------------------
 */

ov_json_value *ov_json_string(const char *content);
bool ov_json_is_string(const ov_json_value *value);
bool ov_json_string_is_valid(const ov_json_value *value);

/*
 *      ------------------------------------------------------------------------
 *
 *      GETTER / SETTER
 *
 *      ------------------------------------------------------------------------
 */

const char *ov_json_string_get(const ov_json_value *self);

bool ov_json_string_set(ov_json_value *self, const char *content);

bool ov_json_string_set_length(ov_json_value *self,
                               const char *content,
                               size_t length);

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ... implementation independent functions
 *
 *      ------------------------------------------------------------------------
 */

bool ov_json_string_clear(void *data);
void *ov_json_string_free(void *data);
void *ov_json_string_copy(void **dest, const void *src);
bool ov_json_string_dump(FILE *stream, const void *src);
ov_data_function ov_json_string_functions();

#endif /* ov_json_string_h */
