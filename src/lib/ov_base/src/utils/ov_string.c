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
        @file           ov_string.c
        @author         Markus Toepfer
        @author         Michael Beer

        @date           2018-11-08

        @ingroup        ov_basics

        @brief          Implementation of some string functions


        ------------------------------------------------------------------------
*/
#include "../../include/ov_string.h"
#include "../include/ov_utils.h"

/*----------------------------------------------------------------------------*/

size_t ov_string_len(char const *str) {

    if (0 == str) {
        return 0;
    }

    return strlen(str);
}

/*----------------------------------------------------------------------------*/

char const *ov_string_chr(char const *str, char c) {

    if (0 == str) {
        return 0;
    } else {
        return strchr(str, c);
    }
}

/*----------------------------------------------------------------------------*/

char *ov_string_copy(char *restrict target, char const *restrict source,
                     size_t max_len) {

    if (0 == source) {
        goto error;
    }

    if ((0 == target) && (0 == max_len)) {
        return ov_string_dup(source);
    }

    if (0 == target) {
        size_t len = ov_string_len(source);
        if (len > max_len) {
            len = max_len;
        }
        target = malloc(len);
    }

    OV_ASSERT(0 != target);

    if (0 == max_len) {
        return strcpy(target, source);
    }

    strncpy(target, source, max_len);
    target[max_len - 1] = 0;

    return target;

error:

    return 0;
}

/*----------------------------------------------------------------------------*/

char *ov_string_dup(char const *to_dup) {

    if (0 == to_dup)
        goto error;

    size_t len = strnlen(to_dup, OV_STRING_DEFAULT_SIZE);

    if ((OV_STRING_DEFAULT_SIZE < len + 1) || (0 != to_dup[len])) {

        goto error;
    }

    return strdup(to_dup);

error:

    return 0;
}

/*----------------------------------------------------------------------------*/

int ov_string_compare(char const *str1, char const *str2) {

    if ((0 == str1) || (0 == str2)) {
        return (str1 == str2) ? 0 : -1;
    } else {
        return strncmp(str1, str2, OV_STRING_DEFAULT_SIZE);
    }
}

/*----------------------------------------------------------------------------*/

int ov_string_compare_case_insensitive(char const *str1, char const *str2) {

    if ((0 == str1) || (0 == str2)) {
        return (str1 == str2) ? 0 : -1;
    } else {

        for (; (*str1 != 0) && (*str2 != 0); ++str1, ++str2) {
            int d = tolower(*str1) - tolower(*str2);
            if ((0 != d) && (0 == *str1)) {
                return d;
            }
        }

        return *str1;
    }
}

/*----------------------------------------------------------------------------*/

bool ov_string_equal(char const *str1, char const *str2) {

    if ((0 == str1) || (0 == str2)) {
        return str1 == str2;
    } else {

        for (; (*str1 != 0) && (*str2 != 0); ++str1, ++str2) {
            if (*str1 != *str2) {
                return false;
            }
        }

        return (0 == *str1) && (0 == *str2);
    }
}

/*----------------------------------------------------------------------------*/

bool ov_string_equal_nocase(char const *str1, char const *str2) {

    if ((0 == str1) || (0 == str2)) {
        return str1 == str2;
    } else {

        for (; (*str1 != 0) && (*str2 != 0); ++str1, ++str2) {
            if (0 != tolower(*str1) - tolower(*str2)) {
                return false;
            }
        }

        return (0 == *str1) && (0 == *str2);
    }
}

/*----------------------------------------------------------------------------*/

bool ov_string_startswith(char const *str, char const *prefix) {

    if (ov_ptr_valid(prefix, "No prefix given") &&
        ov_ptr_valid(str, "No string given")) {

        for (char const *r = str; (*r != 0) && (*prefix != 0); ++prefix, ++r) {

            if (*r != *prefix) {
                return false;
            }
        }

        return 0 == *prefix;

    } else {
        return false;
    }
}

/*----------------------------------------------------------------------------*/

char const *ov_string_sanitize(char const *str) {

    if (0 == str) {
        return "";
    }

    return str;
}

/*----------------------------------------------------------------------------*/

char const *ov_string_ltrim(char const *str, char const *chars_to_skip) {

    char *other = (char *)str;

    if ((0 == str) || (0 == chars_to_skip)) {
        return 0;
    } else {

        while ((0 != *other) && (0 != strchr(chars_to_skip, *other)))
            ++other;

        return other;
    }
}

/*----------------------------------------------------------------------------*/

char *ov_string_rtrim(char *str, char const *chars_to_skip) {

    if ((0 == str) || (0 == chars_to_skip)) {
        return 0;
    } else {

        char *end = str;

        for (char *iter = str; 0 != *iter; ++iter) {
            if (0 == strchr(chars_to_skip, *iter)) {
                end = iter;
            }
        }

        if (0 != *end) {
            ++end;
            *end = 0;
        }

        return str;
    }
}

/*----------------------------------------------------------------------------*/

uint16_t ov_string_to_uint16(const char *string, bool *ok) {

    char *end_ptr = 0;
    long long_number = strtol(string, &end_ptr, 10);

    *ok = false;

    if (0 == end_ptr) {

        ov_log_error("strtol set end_ptr to 0?");
        exit(1);
    }

    if (string == end_ptr) {

        ov_log_error("Empty string");
        goto error;
    }

    if (0 != *end_ptr) {

        ov_log_error("Not a valid integer: %s", string);
        goto error;
    }

    uint16_t uint16_number = (uint16_t)long_number;

    if (long_number != (long)uint16_number) {

        ov_log_error("Number is out of range: %li", long_number);
        goto error;
    }

    *ok = true;

    return uint16_number;

error:

    return 0;
}

/*----------------------------------------------------------------------------*/

uint32_t ov_string_to_uint32(const char *string, bool *ok) {

    char *end_ptr = 0;
    long long_number = strtol(string, &end_ptr, 10);

    *ok = false;

    if (0 == end_ptr) {

        ov_log_error("strtol set end_ptr to 0?");
        exit(1);
    }

    if (string == end_ptr) {

        ov_log_error("Empty string");
        goto error;
    }

    if (0 != *end_ptr) {

        ov_log_error("Not a valid integer: %s", string);
        goto error;
    }

    uint32_t uint32_number = (uint32_t)long_number;

    if (long_number != (long)uint32_number) {

        ov_log_error("Number is out of range: %li", long_number);
        goto error;
    }

    *ok = true;

    return uint32_number;

error:

    return 0;
}

/*----------------------------------------------------------------------------*/

// The following functions are pretty much useless -
//  wrappers around methods, whereby the wrappers don't do anything

bool ov_string_to_uint64(const char *string, uint64_t size, uint64_t *number) {

    if (!string || size < 1 || !number)
        return false;

    return ov_convert_string_to_uint64(string, size, number);
}

/*----------------------------------------------------------------------------*/

bool ov_string_to_int64(const char *string, uint64_t size, int64_t *number) {

    if (!string || size < 1 || !number)
        return false;

    return ov_convert_string_to_int64(string, size, number);
}

/*----------------------------------------------------------------------------*/

bool ov_string_to_double(const char *string, uint64_t size, double *number) {

    if (!string || size < 1 || !number)
        return false;

    return ov_convert_string_to_double(string, size, number);
}

/*****************************************************************************
                            ov_string_value_for_key
 ****************************************************************************/

static char *strip_quotes(char *s) {

    if (!ov_ptr_valid(s, "Invalid string")) {

        return 0;

    } else if ('"' == s[0]) {

        ++s;
        char *endquote = strchr(s, '"');

        if (0 != endquote) {
            *endquote = 0;
            return s;
        } else {
            ov_log_error("Missing final quotes: %s", s);
            return 0;
        }

    } else {
        return s;
    }
}

/*----------------------------------------------------------------------------*/

static ov_buffer *get_tag_value(char *s, char const *key, char separator) {

    if (ov_ptr_valid(s, "Invalid token") && ov_ptr_valid(key, "Invalid key")) {

        s = (char *)ov_string_ltrim(s, " ");
        char *value = strchr(s, separator);

        if (0 != value) {
            *value = 0;
            ++value; // *value now '"'
            if (0 == strcmp(s, key)) {
                ov_buffer *b = ov_buffer_from_string(strip_quotes(value));
                return b;
            } else {
                return 0;
            }
        } else {
            return 0;
        }

    } else {
        return 0;
    }
}

/*----------------------------------------------------------------------------*/

ov_buffer *ov_string_value_for_key(char const *s, char const *key,
                                   char assigner, char const *separator) {

    if (ov_ptr_valid(s, "Invalid token") && ov_ptr_valid(key, "Invalid key") &&
        ov_ptr_valid(separator, "No separator to split key-value list given")) {

        ov_buffer *copy = ov_buffer_from_string(s);
        char *s = (char *)copy->start;
        char *sp = 0;

        ov_buffer *value_buf = 0;

        for (s = strtok_r(s, separator, &sp); (s != 0) && (value_buf == 0);
             s = strtok_r(0, separator, &sp)) {
            value_buf = get_tag_value(s, key, assigner);
        }

        copy = ov_buffer_free(copy);
        return value_buf;

    } else {
        return 0;
    }
}

/*****************************************************************************
                                OV_DATAFUNCTIONS
 ****************************************************************************/

ov_data_function ov_string_data_functions() {

    ov_data_function functions = {

        .clear = ov_string_data_clear,
        .free = ov_string_data_free,
        .copy = ov_string_data_copy,
        .dump = ov_string_data_dump};

    return functions;
}

/*----------------------------------------------------------------------------*/

bool ov_string_data_clear(void *data) {

    if (!data)
        return false;

    char *string = (char *)data;

    string[0] = 0;

    return true;
}

/*----------------------------------------------------------------------------*/

void *ov_string_data_free(void *data) {

    if (data)
        free(data);

    return NULL;
}

/*----------------------------------------------------------------------------*/

void *ov_string_data_copy(void **destination, const void *string) {

    if (!destination || !string)
        return NULL;

    if (*destination)
        ov_string_data_free(*destination);

    *destination = (void *)strndup((char *)string, SIZE_MAX);
    return *destination;
}

/*----------------------------------------------------------------------------*/

bool ov_string_data_dump(FILE *stream, const void *string) {

    if (!stream || !string)
        return false;

    if (fprintf(stream, "%s \n", (char *)string))
        return true;

    return false;
}

/*
 *      ------------------------------------------------------------------------
 *
 *                        STRING PARING
 *
 *      ------------------------------------------------------------------------
 */

const char *ov_string_find(const char *source, size_t sc_len, const char *delim,
                           size_t dm_len) {

    if (!source || !delim || sc_len < 1 || dm_len < 1)
        goto error;

    size_t i = 0;
    size_t k = 0;
    bool valid = false;

    for (i = 0; i < sc_len; i++) {

        if (source[i] != delim[0])
            continue;

        valid = true;
        if (dm_len + i > sc_len)
            goto error;

        for (k = 0; k < dm_len; k++) {

            if (source[i + k] != delim[k]) {
                valid = false;
                break;
            }
        }

        if (valid) {

            if ((i + dm_len) > sc_len)
                break;

            return source + i;
        }
    }

error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_list *ov_string_pointer(const char *source, size_t sc_len, const char *delim,
                           size_t dm_len) {

    if (!source || !delim)
        return NULL;

    ov_list *list = ov_list_create((ov_list_config){0});
    if (!list)
        return NULL;

    if ((sc_len < 1) || (dm_len < 1))
        goto error;

    if (dm_len > sc_len)
        goto error;

    char *start = (char *)source;
    char *pointer = (char *)ov_string_find(source, sc_len, delim, dm_len);

    if (!list->push(list, (void *)start))
        goto error;

    while (pointer) {

        start = pointer + dm_len;

        if ((start - source) >= (int64_t)sc_len)
            break;

        if (start[0] == 0)
            break;

        if (!list->push(list, start))
            goto error;

        pointer = (char *)ov_string_find(start, sc_len - (start - source),
                                         delim, dm_len);
    }

    return list;

error:
    list = list->free(list);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_list *ov_string_split(const char *source, size_t sc_len, const char *delim,
                         size_t dm_len, bool copy_delimiter) {

    if (!source || !delim)
        return NULL;

    if ((sc_len < 1) || (dm_len < 1))
        return NULL;

    void *ptr1 = NULL;
    void *ptr2 = NULL;
    char *string = NULL;
    int64_t length = 0;
    size_t items = 0;

    ov_list *list_pointer = NULL;
    ov_list *list_copies = NULL;

    list_pointer = ov_string_pointer(source, sc_len, delim, dm_len);
    if (!list_pointer)
        goto error;

    list_copies = ov_list_create((ov_list_config){
        .item = ov_data_string_data_functions(),
    });

    if (!list_copies)
        goto error;

    items = list_pointer->count(list_pointer);

    for (size_t i = 1; i <= items; i++) {

        ptr1 = list_pointer->get(list_pointer, i);
        if (!ptr1)
            goto error;

        if (i < items) {

            ptr2 = list_pointer->get(list_pointer, i + 1);
            if (!ptr2)
                goto error;

            length = ptr2 - ptr1;

            if (!copy_delimiter)
                length -= dm_len;

        } else {

            ptr2 = (char *)source + strlen(source);
            length = ptr2 - ptr1;

            if (ov_string_find(ptr1, length, delim, dm_len))
                if (!copy_delimiter)
                    length -= dm_len;
        }

        if (length < 0)
            goto error;

        if (length == 0) {
            list_copies->set(list_copies, i, NULL, NULL);
            continue;
        }

        string = strndup(ptr1, length);
        if (!string)
            goto error;

        if (!list_copies->set(list_copies, i, string, NULL))
            goto error;
    }

    list_pointer = list_pointer->free(list_pointer);
    return list_copies;

error:
    list_copies = list_copies->free(list_copies);
    list_pointer = list_pointer->free(list_pointer);
    return NULL;
}

/*----------------------------------------------------------------------------*/

static size_t str_size(char const *str, size_t maxlen) {

    if (0 == str) {
        return 0;
    } else {
        size_t i = 0;
        for (i = 0; (str[i] != 0) && i < maxlen; ++i)
            ;
        return i;
    }
}

/*----------------------------------------------------------------------------*/

static bool ensure_capacity(char **in, size_t *current_capacity,
                            size_t required_capacity) {

    if ((0 == in) || (0 == current_capacity)) {
        return false;
    } else if (required_capacity > *current_capacity) {
        char *new_in = calloc(1, required_capacity);
        memcpy(new_in, *in, *current_capacity);
        free(*in);
        *in = new_in;
        *current_capacity = required_capacity;
        return true;
    } else {
        return true;
    }
}

/*----------------------------------------------------------------------------*/

bool ov_string_append(char **dest, size_t *const size, const char *source,
                      size_t len) {

    if ((0 == dest) || (0 == size) || (0 == source) || (0 == len)) {
        return false;
    } else {

        char *target = *dest;

        size_t destlen = str_size(target, *size);
        size_t srclen = str_size(source, len);

        size_t total_len = 1 + destlen + srclen;

        ensure_capacity(&target, size, total_len);

        for (size_t i = 0; i < srclen; ++i) {
            target[destlen + i] = source[i];
        }

        target[destlen + srclen] = 0;
        *dest = target;

        return true;
    }
}

/*----------------------------------------------------------------------------*/

static bool str_append(char **result, size_t *result_capacity,
                       size_t *write_index, char const *str2, size_t str2_len) {

    if ((0 == result) || (0 == result_capacity) || (0 == write_index) ||
        (0 == str2)) {
        return false;
    } else if (0 == str2_len) {
        return true;
    } else {

        ensure_capacity(result, result_capacity, *write_index + str2_len + 1);

        size_t wri = *write_index;
        char *write_ptr = *result;

        for (size_t i = 0; (i < str2_len) && (0 != str2[i]); ++i, ++wri) {
            write_ptr[wri] = str2[i];
        }

        write_ptr[wri] = 0;
        *write_index = wri;

        return true;
    }
}

/*----------------------------------------------------------------------------*/

bool ov_string_replace_all(char **result, size_t *size, const char *source,
                           size_t sc_len, const char *delim1, size_t d1_len,
                           const char *delim2, size_t d2_len, bool set_last) {

    UNUSED(set_last);

    if ((0 == result) || (0 == size) || (0 == source) || (0 == sc_len) ||
        (0 == delim1) || (0 == d1_len)) {
        return false;

    } else {

        ensure_capacity(result, size, OV_STRING_DEFAULT_SIZE);
        size_t result_capacity = *size;

        size_t true_sc_len = strnlen(source, sc_len);

        size_t delim1_len = strnlen(delim1, d1_len);
        size_t delim2_len = str_size(delim2, d2_len);

        size_t write_index = 0;

        size_t delim_index = 0;

        // Always points to the first char of next delimiter in source
        char const *start_delim_in_source = source;
        // Always points to the first char of token BEFORE next delimiter
        // At start of string or after a delimiter was found,
        // both point to same point
        char const *start_last_token = source;

        for (size_t i = 0; i < true_sc_len; ++i) {

            if (delim1[delim_index] != source[i]) {

                delim_index = 0;
                start_delim_in_source = source + i + 1;

            } else if (++delim_index == delim1_len) {

                // We found the entire delimiter in source

                str_append(result, &result_capacity, &write_index,
                           start_last_token,
                           start_delim_in_source - start_last_token);

                str_append(result, &result_capacity, &write_index, delim2,
                           delim2_len);

                start_delim_in_source = 1 + source + i;
                start_last_token = 1 + source + i;
                delim_index = 0;
            }
        }

        bool does_not_end_in_delimiter =
            (start_last_token - source) < (ptrdiff_t)true_sc_len;

        if (does_not_end_in_delimiter) {
            str_append(result, &result_capacity, &write_index, start_last_token,
                       1 + true_sc_len - (start_last_token - source));
        }

        if (does_not_end_in_delimiter && set_last) {
            str_append(result, &result_capacity, &write_index, delim2, d2_len);
        }

        *size = result_capacity;

        return true;
    }
}

/*----------------------------------------------------------------------------*/

int64_t ov_string_parse_hex_digits(const char *start, uint64_t size,
                                   char **next) {

    int64_t number = 0;

    if (!start || (0 == size))
        goto error;

    int64_t i = 0;

    /* MAX int64_t digits is 16 HEX */

    int max = 16;
    if (size < 16)
        max = size;

    for (i = 0; i < max; i++) {

        if (!isxdigit(start[i]))
            break;
    }

    char *end = NULL;
    number = strtoll(start, &end, 16);

    if (end != start + i)
        goto error;

    if (i == 16) {

        if (start[0] > 0x39) {

            /* int64_t overflow  */

            if (next)
                *next = (char *)start;

            goto error;
        }
    }

    if (next)
        *next = (char *)(start + i);

    return number;

error:

    if ((0 == size) && (next))
        *next = (char *)start;

    return 0;
}
