/***
        ------------------------------------------------------------------------

        Copyright (c) 2020 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_uri.h
        @author         Markus Töpfer

        @date           2020-01-03

        @ingroup        ov_uri

        @brief          Definition of URI RFC3986 functionality


        ------------------------------------------------------------------------
*/
#ifndef ov_uri_h
#define ov_uri_h

#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "ov_json.h"

typedef struct ov_uri {

    char *scheme;
    char *path;

    char *user;
    char *host;
    uint32_t port;

    char *query;
    char *fragment;

} ov_uri;

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

/**
        Free an allocated URI, with allocated string content.
        e.g. an uri created by @see ov_uri_from_string

        @param uri              uri to free
*/
ov_uri *ov_uri_free(ov_uri *uri);

/*
 *      ------------------------------------------------------------------------
 *
 *      DE / ENCODING (copy based)
 *
 *      ------------------------------------------------------------------------
 */

/**
        Parse a string to ov_uri. This function will allocate a uri structure
        and copy the content of the URI parts to the structure.

        @param string           start  of the string
        @param length           length of the string
*/
ov_uri *ov_uri_from_string(const char *string, size_t length);

/*----------------------------------------------------------------------------*/

/**
        Calculate the string length of an uri.

        @NOTE This function performs content validation.
*/
size_t ov_uri_string_length(const ov_uri *uri);

/*----------------------------------------------------------------------------*/

/**
        Create the URI string, based on the URI structures content.

        @param uri              uri to stringify
*/
char *ov_uri_to_string(const ov_uri *uri);

/*
 *      ------------------------------------------------------------------------
 *
 *      VALIDATION FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

/**
        Check if a buffer contains a valid uri (absolute or reference)

        @param string   start of the string to check
        @param length   length of the string to check
*/
bool ov_uri_string_is_valid(const char *string, size_t length);

/*----------------------------------------------------------------------------*/

/**
        Check if a buffer contains an absolute uri

        @param string   start of the string to check
        @param length   length of the string to check
*/
bool ov_uri_string_is_absolute(const char *string, size_t length);

/*----------------------------------------------------------------------------*/

/**
        Check if a buffer contains an uri reference

        @param string   start of the string to check
        @param length   length of the string to check
*/
bool ov_uri_string_is_reference(const char *string, size_t length);

/*----------------------------------------------------------------------------*/

/**
        This function will remove all dot segments ./ as well as ../ and all
        empty path // from in and write the result to out.

        NOTE //dir will become /dir

        @param in   string of max length PATH_MAX /0 terminated
        @param out  buffer of PATH_MAX

        @returns error if the path will run over (too many ../ contained)
*/
bool ov_uri_path_remove_dot_segments(const char *in, char *out);

/*----------------------------------------------------------------------------*/

ov_json_value *ov_uri_to_json(const ov_uri *self);

#endif /* ov_uri_h */
