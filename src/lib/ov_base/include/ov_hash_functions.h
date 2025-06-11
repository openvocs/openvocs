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
        @file           ov_hash_functions.h
        @author         Michael Beer
        @author         Markus Toepfer

        @date           2018-05-07

        @ingroup        ov_base

        @brief


        ------------------------------------------------------------------------
*/
#ifndef ov_hash_functions_h
#define ov_hash_functions_h

#include <inttypes.h>

/**
        Simple hash function for c strings (null-terminated!).
        @param c_string zero-terminated array of bytes
        @return values in the range of 0 ... 255
 */
uint64_t ov_hash_simple_c_string(const void *c_string);

/*---------------------------------------------------------------------------*/

/**
        Pearson hashing for c strings.
        See Pearson, Peter K. (June 1990),
               "Fast Hashing of Variable-Length Text Strings",
               Communications of the ACM, 33 (6)

        @param c_string zero-terminated array of bytes
        @return values in the range of 0 ... 255

 */
uint64_t ov_hash_pearson_c_string(const void *c_string);

/*---------------------------------------------------------------------------*/

/**
        Returns the content of the intptr.
*/
uint64_t ov_hash_intptr(const void *intptr);

/*---------------------------------------------------------------------------*/

/**
        Returns the content of the uint64_t pointer.
*/
uint64_t ov_hash_uint64(const void *uint64);

/*---------------------------------------------------------------------------*/

/**
        Returns the content of int64.
*/
uint64_t ov_hash_int64(const void *int64);

#endif /* ov_hash_functions_h */
