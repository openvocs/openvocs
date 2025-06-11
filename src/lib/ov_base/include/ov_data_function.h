/***
        ------------------------------------------------------------------------

        Copyright 2018 German Aerospace Center DLR e.V. (GSOC)

        Licensed under the Apache License, Version 2.0 (the "License");
        you may not use this file except in compliance with the License.
        You may obtain a copy of the License at

                http://www.apache.org/licenses/LICENSE-2.0

        Unless required by applicable law or agreed to in writing, software
        distributed under the License is distributed on an "AS IS" BASIS,
        WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
        See the License for the specific language governing permissions and
        limitations under the License.

        This file is part of the openvocs project. http://openvocs.org

        ------------------------------------------------------------------------
*//**
        @file           ov_data_function.h
        @author         Markus Toepfer

        @date           2018-03-23

        @ingroup        ov_base

        @brief          Definition of standard functions for operations on
                        "unspecific" data structures at a void pointer.

                        These functions are the abstract function set to be
                        implemented by data structures, which shall be handled
                        in some kind of ov_* data container.

                                e.g. @see ov_data
                                e.g. @see ov_list
                                e.g. @see ov_vector

        ------------------------------------------------------------------------
*/
#ifndef ov_data_function_h
#define ov_data_function_h

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/time.h>
#include <time.h>

typedef struct ov_data_function ov_data_function;

typedef bool (*OV_DATA_CLEAR)(void *source);
typedef void *(*OV_DATA_FREE)(void *source);
typedef void *(*OV_DATA_COPY)(void **destination, const void *source);
typedef bool (*OV_DATA_DUMP)(FILE *stream, const void *source);

/*
 *      ------------------------------------------------------------------------
 *
 *                        FUNCTION STRUCTURE DEFINITION
 *
 *      ------------------------------------------------------------------------
 *
 *      ov_data_functions are used as interface for functions
 *      related to handle some kind of "undefined" void data structure,
 *      within some kind of data structure container.
 *
 *      @NOTE create is not a standard function (MAY use parameter init)
 *      @NOTE clear MUST clear and reinit a structure to default
 *      @NOTE copy  MUST create, or fill the structure destination.
 *      @NOTE free  MUST free the structure and set the pointer to NULL
 */

struct ov_data_function {

    OV_DATA_FREE free;   // terminate struct  and content
    OV_DATA_CLEAR clear; // terminate content of a struct
    OV_DATA_COPY copy;   // copy a struct including content
    OV_DATA_DUMP dump;   // default dump a struct (e.g. debug)
};

/*
 *      ------------------------------------------------------------------------
 *
 *      STANDARD FUNCTIONS
 *
 *      Functions defined in this block SHOULD be used for structure
 *      related functionality e.g.
 *
 *              create  a structure (allocate and initialize)
 *              destroy a structure and terminate all of it's content
 *              copy    a structure including all of it's content
 *
 *      ------------------------------------------------------------------------
 */

/**
        Allocate a default empty ov_data_function structure.
*/
ov_data_function *ov_data_function_create();

/*----------------------------------------------------------------------------*/

/**
        Clear a ov_data_function structure.
*/
bool ov_data_function_clear(ov_data_function *func);

/*----------------------------------------------------------------------------*/

/**
        Terminate a ov_data_function structure.
*/
ov_data_function *ov_data_function_free(ov_data_function *func);

/*----------------------------------------------------------------------------*/

/**
        Copy a ov_data_function structure.
*/
ov_data_function *ov_data_function_copy(ov_data_function **destination,
                                        const ov_data_function *source);

/*----------------------------------------------------------------------------*/

/**
        Dump a ov_data_function structure.
*/
bool ov_data_function_dump(FILE *stream, const ov_data_function *func);

/*
 *      ------------------------------------------------------------------------
 *
 *      EXAMPLE FUNCTION IMPLEMENTATIONS (string based)
 *
 *      ------------------------------------------------------------------------
 */

bool ov_data_string_clear(void *string);
void *ov_data_string_free(void *string);
void *ov_data_string_copy(void **destination, const void *string);
bool ov_data_string_dump(FILE *stream, const void *string);

ov_data_function ov_data_string_data_functions();
bool ov_data_string_data_functions_are_valid(const ov_data_function *functions);

/*
 *      ------------------------------------------------------------------------
 *
 *      EXAMPLE IMPLEMENTATIONS
 *
 *      ------------------------------------------------------------------------
 */

bool ov_data_int64_clear(void *number);
void *ov_data_int64_free(void *number);
void *ov_data_int64_copy(void **destination, const void *number);
bool ov_data_int64_dump(FILE *stream, const void *number);
ov_data_function ov_data_int64_data_functions();

void *ov_data_int64_direct_copy(void **destination, const void *number);
bool ov_data_int64_direct_dump(FILE *stream, const void *number);

void *ov_data_uint64_free(void *data);
bool ov_data_uint64_clear(void *number);
void *ov_data_uint64_copy(void **destination, const void *number);
bool ov_data_uint64_dump(FILE *stream, const void *number);
ov_data_function ov_data_uint64_data_functions();

void *ov_data_timeval_free(void *data);
bool ov_data_timeval_clear(void *number);
void *ov_data_timeval_copy(void **destination, const void *number);
bool ov_data_timeval_dump(FILE *stream, const void *number);
ov_data_function ov_data_timeval_data_functions();

void *ov_data_pointer_free(void *data);

/*
 *      ------------------------------------------------------------------------
 *
 *      Standard free
 *
 *      ------------------------------------------------------------------------
 */
void *ov_data_function_wrapper_free(void *ptr);

/*
 *      ------------------------------------------------------------------------
 *
 *      Additional helper
 *
 *      ------------------------------------------------------------------------
 */

/**
        Create an allocated struct for ov_data_functions and fill
        the struct using a custom function.
*/
ov_data_function *ov_data_function_allocated(
    ov_data_function (*function_fill_struct)());

#endif /* ov_data_function_h */
