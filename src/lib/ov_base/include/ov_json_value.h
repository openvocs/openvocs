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
        @file           ov_json_value.h
        @author         Markus Toepfer

        @date           2018-12-02

        @ingroup        ov_json_value

        @brief          Definition of a minimal common interface of
                        all JSON values.

                        This interface MAY be used to switch between different
                        JSON values, but NOT as a generic interface for all
                        values.


        ------------------------------------------------------------------------
*/
#ifndef ov_json_value_h
#define ov_json_value_h

#define OV_JSON_VALUE_MAGIC_BYTE 0xabcd

typedef struct ov_json_value ov_json_value;

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

/*----------------------------------------------------------------------------*/

typedef enum ov_json_types {
  NOT_JSON = 0,
  OV_JSON_OBJECT = 1,
  OV_JSON_ARRAY = 2,
  OV_JSON_STRING = 3,
  OV_JSON_NUMBER = 4,
  OV_JSON_NULL = 5,
  OV_JSON_TRUE = 6,
  OV_JSON_FALSE = 7

} ov_json_t;

/*
 *      ------------------------------------------------------------------------
 *
 *      ov_json_value
 *
 *      ... is the struct head used in every ov_json_value implementation
 *      ... it is the generic interface to route between different value types
 *      ... it is the connection for parent <-> child relations
 *
 *      ------------------------------------------------------------------------
 */

struct ov_json_value {

  uint16_t magic_byte;
  ov_json_t type;
  ov_json_value *parent;

  bool (*clear)(void *self);
  void *(*free)(void *self);
};

/*
 *      ------------------------------------------------------------------------
 *
 *      include of all ov_json_value interface implementations.
 *
 *      ------------------------------------------------------------------------
 */

#include "ov_json_array.h"
#include "ov_json_literal.h"
#include "ov_json_number.h"
#include "ov_json_object.h"
#include "ov_json_string.h"

/*
 *      ------------------------------------------------------------------------
 *
 *      VALIDATE
 *
 *      ------------------------------------------------------------------------
 */

bool ov_json_value_validate(const ov_json_value *self);

/*
 *      ------------------------------------------------------------------------
 *
 *      PARENT / CHILD RELATION HANDLING
 *
 *      ------------------------------------------------------------------------
 */

/**
        This function will connect a value with a parent and SHOULD be
        used in implementations, which add a JSON value to a collection of
        JSON values. (Where collection is either an object or array).

        It implements all checks required to set an exclusive
        child->parent connection.

        Once applied, the CHILD is aware of its parent collection.

        (1)     This way the child is able to disconnect /
                remove self from parent before termination.
        (2)     This way the child will be freed /
                removed on parent termination.

        To be used, when the CHILD value was added to a parent collection
        as a backpointer.
                 __________                  __________
                |          |  1    <>--- *  |          |
                |  PARENT  |                |  CHILD   |
                |__________|  0..1 ----- 1  |__________|

        @param child    new value (MUST be without parent)
        @param parent   parent value structure (MUST be collection)
*/
bool ov_json_value_set_parent(ov_json_value *child,
                              const ov_json_value *parent);

/*----------------------------------------------------------------------------*/

/**
        This function will unset the child -> parent relation
        (call its parent to remove itself, if a parent is set)

        @success ensures the parent is unset, and informed about child removal
        @error   removal was not successful
*/
bool ov_json_value_unset_parent(ov_json_value *value);

/*
 *      ------------------------------------------------------------------------
 *
 *      ABSTRACT DATA FUNCTIONS
 *
 *      ... unifying all ov_json_value implementations.
 *
 *      ------------------------------------------------------------------------
 */

/**
        Cast a data pointer as a "class" of ov_json_values.
        @returns true if the magic_byte is of JSON is set.
*/
ov_json_value *ov_json_value_cast(const void *data);

/*----------------------------------------------------------------------------*/

/**
        Copy a source value to a dest value.

        @NOTE   This function will always use the default implementation value,
                if *dest is not set

        @NOTE   if *dest is set, it MUST be of the same value type as
                the value to copy.

        @NOTE   the copy will be an independent structure
                and unrelated to ANY parent.

        @param dest     pointer to ov_json_value pointer
        @param value    pointer to source (MUST not point to NULL)
*/
void *ov_json_value_copy(void **dest, const void *value);

/*----------------------------------------------------------------------------*/

/**
        Free ANY ov_json_value structure.
        This function will remove the value from its parent collection,
        if it is associated to a collection, and free the structure.
*/
void *ov_json_value_free(void *value);

/*----------------------------------------------------------------------------*/

/**
        Clear content of ANY ov_json_value structure.
*/
bool ov_json_value_clear(void *value);

/*----------------------------------------------------------------------------*/

/**
        Dump content of ANY ov_json_value structure.
        @NOTE this is not a full compliant stringify, but a content dump.
*/
bool ov_json_value_dump(FILE *stream, const void *value);

/*----------------------------------------------------------------------------*/

/**
        Create the data_function set to handle ANY ov_json_value.
*/
ov_data_function ov_json_value_data_functions();

/*----------------------------------------------------------------------------*/

char *ov_json_value_to_string(const ov_json_value *value);

/*----------------------------------------------------------------------------*/

ov_json_value *ov_json_value_from_string(const char *string, size_t length);

#endif /* ov_json_value_h */
