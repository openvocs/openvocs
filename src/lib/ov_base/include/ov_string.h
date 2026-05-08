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
        @file           ov_string.h
        @author         Markus Toepfer
        @author         Michael Beer

        @date           2018-11-08

        @ingroup        ov_base

        @brief          Definition of some string functions.

        ------------------------------------------------------------------------
*/
#ifndef ov_string_h
#define ov_string_h

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <ctype.h>
#include <string.h>
#include <strings.h>
#include <wchar.h>

#include "ov_buffer.h"
#include "ov_convert.h"
#include "ov_data_function.h"
#include "ov_list.h"

#define OV_STRING_DEFAULT_SIZE 255
#define OV_DEFAULT_LINEBREAK "\n"

const char *ov_string_find(const char *source, size_t sc_len, const char *delim,
                           size_t dm_len);

/*
 *      ------------------------------------------------------------------------
 *
 *                        STRING PARING
 *
 *      ------------------------------------------------------------------------
 */

/**
        Point to chunks of a string separated by a delimiter. This pointing
        will be nonintrusive and the result vector will point to the start
        of each chunk within the source string. (Non-intrusive, Zero-Copy)

        Example:

                delim  = "\n"
                source = "line1\nline2\nline3\nline4\nline5\n"
               list[0] _^      ^      ^      ^      ^
               list[1] ________|      |      |      |
               list[2] _______________|      |      |
               list[3] ______________________|      |
               list[4] _____________________________|

        NOTE This will point to the start of the string, but NOT set or change
        any lineend. So as an result, the lineend of all strings stays the same.
        You MAY use pointer arithmetik to mask the lines of the above example:

        e.g. to print the chunk "line2\n"
        printf("%.s", vector->items[2] - vector->items[1], vector->items[1]);

        @param  source  source to write to result
        @param  sc_len  length of source (at most')
        @param  delim   delimiter to use
        @param  dm_len  length of the delimiter (at most)
*/
ov_list *ov_string_pointer(const char *source, size_t sc_len, const char *delim,
                           size_t dm_len);

/*----------------------------------------------------------------------------*/

/**
        Split a string based on a delimiter. The testvector will contain
        value copies of the chunks separated by the delimiter, the source
        will be unchanged. The chunks of the vector are independent of the
        source (value copy)

        Example:

                delim   = "\n"
                source   = "line1\nline2\nline3\nline4\nline5\n"
                            ^      ^      ^      ^      ^
                 list[0] = "line1"
                 list[1] =        "line2"
                 list[2] =               "line3"
                 list[3] =                       "line4"
                 list[4] =                              "line5"

        NOTE The example shows the use with copy_delimter == false

        @param  source          source to write to result
        @param  sc_len          length of source (at most)
        @param  delim           delimiter to use
        @param  dm_len          length of the delimiter (at most)
        @param  copy_delimiter  if true, the chunks will contain the delimiter
*/
ov_list *ov_string_split(const char *source, size_t sc_len, const char *delim,
                         size_t dm_len, bool copy_delimiter);

/*
 *      ------------------------------------------------------------------------
 *
 *                        STRING CONVERSIONS
 *
 *      ------------------------------------------------------------------------
 */

/**
        Convert a c string into an uint16_t.

        @param ok       Set to false in case of conversion failure
        @return         the converted value or 0 in case of error.
 */
uint16_t ov_string_to_uint16(const char *string, bool *ok);

/*----------------------------------------------------------------------------*/

/**
        Convert a c string into an uint16_t.

        @param ok       Set to false in case of conversion failure
        @return         the converted value or 0 in case of error.
 */
uint32_t ov_string_to_uint32(const char *string, bool *ok);

/*----------------------------------------------------------------------------*/

/**
        Converts a string to uint64_t. This function ensures to read upto size.
        @returns        true if the whole string up to size is a valid positive
                        integer.
 */
bool ov_string_to_uint64(const char *string, uint64_t size, uint64_t *number);

/*----------------------------------------------------------------------------*/

/**
        Converts a string to int64_t. This function ensures to read upto size.
        @returns        true if the whole string up to size is a valid integer.
 */
bool ov_string_to_int64(const char *string, uint64_t size, int64_t *number);

/*----------------------------------------------------------------------------*/

/**
        Converts a string to int64_t. This function ensures to read upto size.
        @returns        true if the whole string up to size is a valid double.
 */
bool ov_string_to_double(const char *string, uint64_t size, double *number);

/*----------------------------------------------------------------------------*/

/**
        Parse a string of HEX digits, as long as HEX digits are present.

        Valid input

        HEX            = "A" | "B" | "C" | "D" | "E" | "F"
                      | "a" | "b" | "c" | "d" | "e" | "f" | DIGIT

        NOTE 0x something is NOT valid. This function will parse some
        HEX string "1AD13442DF16" but NOT "0xff0x12"

        @param start    start of string
        @param size     size of buffer
        @param next     pointer to first non valid

        @returns number parsed

        NOTE on return of 0 and if next == start no valid content was found

        NOTE parses up to 16 digits from start (charater limit int64_t) if the
        string contains more than 16 valid HEX digits some additional handling
        for the correct number needs to be done!

        @returns 0 on error with next == start if next != start the string is
        actually 0
 */
int64_t ov_string_parse_hex_digits(const char *start, uint64_t size,
                                   char **next);

/*****************************************************************************
                          'Safer' std string functions
 ****************************************************************************/

/**
 * @return length of string (just like strlen(2)). 0 in case of error.
 */
size_t ov_string_len(char const *str);

/*----------------------------------------------------------------------------*/

/**
 * Returns pointer to first occurence of character `c` in `str` or
 * 0 in case of no valid input string or char `c` not found.
 * See strchr(3)
 */
char const *ov_string_chr(char const *str, char c);

/*----------------------------------------------------------------------------*/

/**
 * Safe version ov strcpy(3).
 * @param target pointer to copy string to. If zero, memory will be allocated.
 * @param source pointer to copy string from. If zero, 0 string is returned
 * @param max_len If not 0, at most max_len bytes are copied (like strncpy(3) )
 * - the last character will always be set to 0
 *
 * @return pointer to copy of source. Is either 0 (in case of error) or
 * zero-terminated!
 */
char *ov_string_copy(char *restrict target, char const *restrict source,
                     size_t max_len);

/*----------------------------------------------------------------------------*/

/**
 * Duplicate a string safely.
 * Checks if `to_copy` is a null pointer, if it is zero terminated within
 * the first OV_STRING_DEFAULT_SIZE chars...
 * Ensures that the returned duplicate is zero terminated.
 * @return a duplicate of `to_dup`.
 */
char *ov_string_dup(char const *to_copy);

/*----------------------------------------------------------------------------*/

/**
 * Compares 2 strings in a safe way.
 * Max length of either of the strings is ov_string_DEFAULT_SIZE
 * If called with zero pointers, the 2 strings are assumed equal if both
 * are 0 pointers.
 * @return see strcmp(3)
 */
int ov_string_compare(char const *str1, char const *str2);

/*----------------------------------------------------------------------------*/

/**
 * @return true if both strings equal (also if both are 0), otherwise false
 */
bool ov_string_equal(char const *str1, char const *str2);

/*----------------------------------------------------------------------------*/

/**
 * Like ov_string_equal, but case insensitively
 */
bool ov_string_equal_nocase(char const *str1, char const *str2);

/*----------------------------------------------------------------------------*/

bool ov_string_startswith(char const *str, char const *prefix);

/*----------------------------------------------------------------------------*/

/**
 * Sanitize a string.
 * Will never return a NULL pointer.
 */
char const *ov_string_sanitize(char const *str);

/*----------------------------------------------------------------------------*/

/**
 * @return pointer that points into str at the first pos without a char
 * contained within `chars_to_skip`
 */
char const *ov_string_ltrim(char const *str, char const *chars_to_skip);

/*----------------------------------------------------------------------------*/

/**
 * Cuts off trailing chars that are contained within `chars_to_skip`.
 * @return pointer to str or 0 in case of error
 */
char *ov_string_rtrim(char *str, char const *chars_to_skip);

/*----------------------------------------------------------------------------*/

/**
 * In a list of 'key=value' assignments, search for
 * 'key' and returns its associated value, e.g.
 *
 * ("a=true;b=23", "b", '=', ";") -> "23"
 * ("a=true;b=23", "b", '=', ",") -> 0
 *        Since there is no ',' inside the input, the string consists of one
 *        single kv pair:     "a"   =   "true;b=23"
 * ("a=true;b=23", "a", '=', ",") -> "true;b=23"
 *        See above.
 * ("a=true;b=23", "b", ':', ";") -> 0
 * ("a:true;b:23", "b", ':', ";") -> "23"
 *
 * @param assigner Char used to denote assignment of value to key: '=' in "a=b"
 * @param separator to split list into key-value pairs. Must not be zero.
 */
ov_buffer *ov_string_value_for_key(char const *s, char const *key,
                                   char assigner, char const *separator);

/*****************************************************************************
                                OV_DATAFUNCTIONS
 ****************************************************************************/

/**
        Create string data functions.
*/
ov_data_function ov_string_data_functions();

/*----------------------------------------------------------------------------*/

/**
        Clear a string pointer.
*/
bool ov_string_data_clear(void *string);

/*----------------------------------------------------------------------------*/

/**
        Free a string pointer.
*/
void *ov_string_data_free(void *string);

/*----------------------------------------------------------------------------*/

/**
        Copy a string pointer.
*/
void *ov_string_data_copy(void **destination, const void *string);

/*----------------------------------------------------------------------------*/

/**
        Copy a string pointer.
*/
bool ov_string_data_dump(FILE *stream, const void *string);

/*----------------------------------------------------------------------------*/

/**
        Append a source string to a destination string. The destination MUST
        be an allocated string, as the function is based on reallocations.

        If the allocated buffer at destination is big enough to contain the
        source string, the string will be append without reallocation,
        otherwise a reallocation with the minimum required size to contain
        both string will be done and set at *dest. In addition the size of
        the reallocation will be set at size.

        @param dest     pointer to buffer for the the result of the string
                        concationation. If the dest points to NULL, a new
                        buffer will be allocated and size set to the allocated
                        size.
        @param size     size of the destination buffer
        @param source   pointer to buffer to be appended to dest
        @param len      length of the source to be added to dest (at most)

        @return         returns true on success,  false on error
 */
bool ov_string_append(char **dest, size_t *const size, const char *source,
                      size_t len);

/*----------------------------------------------------------------------------*/

/**
        Change all occurances of old_item in source to new_item.

        USE CASE - exchange of all occurances of any string

                e.g. linbreak exchange
                testrun_...(&r, &s, "a\nb\nc\n", "\n", 1, "\r\n", 2, true);
                r = "a\r\nb\r\nc\r\n"


        USE CASE of set_last

                r = "a\nr\nc"           // no final linebreak if false
                r = "a\nb\nc\n"         // ensured final linebreak if true

        EXAMPLE

                r = calloc(10, sizeof(char));

                s = "whenever_whenever";
                o = "en";
                n = "_at_"

                tr...(&r, &size, s, strlen(s), o, 3, n, 3);

                r = "wh_at_ever_wh_at_ever"

        @param  result  (mandatory) pointer to write to (allocated OR null) -
   will be OVERWRITTEN!
        @param  size    (mandatory) pointer to size of result (may be 0)
        @param  source  (mandatory) source to write to result
        @param  sc_len  (mandatory) length of source (at most)
        @param  old_item (mandatory) item to be replaced
        @param  old_len  (mandatory) length of item to be replaced
        @param  new_item (optional) new_item to be written at position of old
        @param  new_len  (optional) length of new_item
        @param  set_last (mandatory) if false a possible last item will be unset

        @return         true on success, false on error
 */
bool ov_string_replace_all(char **result, size_t *const size,
                           const char *source, size_t sc_len,
                           const char *old_item, size_t old_len,
                           const char *new_item, size_t new_len, bool set_last);

/*----------------------------------------------------------------------------*/

#endif /* ov_string_h */
