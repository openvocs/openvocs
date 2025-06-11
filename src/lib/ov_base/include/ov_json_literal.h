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
        @file           ov_json_literal.h
        @author         Markus Toepfer

        @date           2018-12-03

        @ingroup        ov_json_literal


        ------------------------------------------------------------------------
*/
#ifndef ov_json_literal_h
#define ov_json_literal_h

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

ov_json_value *ov_json_bool(bool bval);
ov_json_value *ov_json_true();
ov_json_value *ov_json_false();
ov_json_value *ov_json_null();

bool ov_json_is_true(const ov_json_value *value);
bool ov_json_is_false(const ov_json_value *value);
bool ov_json_is_null(const ov_json_value *value);

ov_json_value *ov_json_literal(ov_json_t type);
bool ov_json_is_literal(const ov_json_value *value);

/*
 *      ------------------------------------------------------------------------
 *
 *      GETTER / SETTER
 *
 *      ------------------------------------------------------------------------
 */

bool ov_json_literal_set(ov_json_value *self, ov_json_t type);

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ... implementation independent functions
 *
 *      ------------------------------------------------------------------------
 */

bool ov_json_literal_clear(void *data);
void *ov_json_literal_free(void *data);
void *ov_json_literal_copy(void **dest, const void *src);
bool ov_json_literal_dump(FILE *stream, const void *src);
ov_data_function ov_json_literal_functions();

#endif /* ov_json_literal_h */
