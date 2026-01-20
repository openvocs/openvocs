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
        @file           ov_convert.c
        @author         Markus Toepfer
        @author         Michael Beer

        @date           2018-03-06

        @ingroup        ov_basics

        @brief          Implementation of basic convertions.


        ------------------------------------------------------------------------
*/
#include "../../include/ov_convert.h"
#include "../include/ov_utils.h"
#include <math.h>

/*
 *      ------------------------------------------------------------------------
 *
 *      STRING TO ...
 *
 *      ------------------------------------------------------------------------
 */

bool ov_convert_string_to_uint(const char *string, uint64_t size,
                               uint64_t *number, uint64_t max) {

    if (!string || size < 1 || !number)
        return false;

    char *ptr = NULL;

    char buffer[size + 1];
    memset(buffer, '\0', size + 1);

    if (!memcpy(buffer, string, size))
        return false;

    long long nbr = strtoll(buffer, &ptr, 10);

    if (nbr < 0)
        goto error;

    *number = nbr;

    if ((uint64_t)nbr > max)
        goto error;

    if (*ptr == 0)
        return true;

error:
    *number = 0;
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_convert_string_to_uint64(const char *string, uint64_t size,
                                 uint64_t *number) {

    if (!string || !number)
        return false;

    return ov_convert_string_to_uint(string, size, number, UINT64_MAX);
}

/*----------------------------------------------------------------------------*/

bool ov_convert_string_to_uint32(const char *string, uint64_t size,
                                 uint32_t *number) {

    uint64_t nbr = 0;

    if (!ov_convert_string_to_uint(string, size, &nbr, UINT32_MAX))
        return false;

    *number = (uint32_t)nbr;
    return true;
}

/*----------------------------------------------------------------------------*/

bool ov_convert_string_to_uint16(const char *string, uint64_t size,
                                 uint16_t *number) {

    uint64_t nbr = 0;

    if (!ov_convert_string_to_uint(string, size, &nbr, UINT16_MAX))
        return false;

    *number = (uint16_t)nbr;
    return true;
}

/*----------------------------------------------------------------------------*/

bool ov_convert_string_to_int64(const char *string, uint64_t size,
                                int64_t *number) {

    if (!string || size < 1 || !number)
        return false;

    char *ptr = NULL;

    char buffer[size + 1];
    memset(buffer, '\0', size + 1);

    if (!memcpy(buffer, string, size))
        return false;

    *number = strtoll(buffer, &ptr, 10);

    if (*number != (long long)*number)
        goto error;

    if (*ptr == 0)
        return true;

error:
    *number = 0;
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_convert_string_to_double(const char *string, uint64_t size,
                                 double *number) {

    if (!string || size < 1 || !number)
        return false;

    char *ptr = NULL;

    char buffer[size + 1];
    memset(buffer, '\0', size + 1);

    if (!memcpy(buffer, string, size))
        return false;

    *number = strtod(buffer, &ptr);

    if (*number != (double)*number)
        goto error;

    if (*ptr == 0)
        return true;

error:
    *number = 0;
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_convert_hex_string_to_uint64(const char *string, size_t size,
                                     uint64_t *number) {

    if (!string || size < 1 || !number)
        return false;

    char *ptr = NULL;

    char buffer[size + 1];
    memset(buffer, '\0', size + 1);

    if (!memcpy(buffer, string, size))
        return false;

    *number = strtoll(buffer, &ptr, 16);

    return true;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      DOUBLE TO ...
 *
 *      ------------------------------------------------------------------------
 */

bool ov_convert_double_to_string(double number, char **string, size_t *size) {

    if (!string || !size)
        return false;

    char local[100];
    size_t used = 0;

    used = snprintf(local, 100, "%g", number);
    if (used == 0)
        return false;

    // check for truncated (unlikely)
    if (used > 100)
        return false;

    if (*string) {

        // copy to buffer
        if (*size < used)
            return false;

        if (!memcpy(*string, local, used))
            return false;

    } else {

        *string = calloc(used + 1, sizeof(char *));
        if (!*string)
            return false;

        if (!memcpy(*string, local, used)) {
            free(*string);
            *string = NULL;
            return false;
        }
    }

    *size = used;
    return true;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      INT TO ...
 *
 *      ------------------------------------------------------------------------
 */

bool ov_convert_int64_to_string(int64_t number, char **string, size_t *size) {

    if (!string || !size)
        return false;

    char local[100];
    size_t used = 0;

    used = snprintf(local, 100, "%" PRIi64, number);
    if (used == 0)
        return false;

    // check for truncated (unlikely)
    if (used > 100)
        return false;

    if (*string) {

        // copy to buffer
        if (*size < used)
            return false;

        if (!memcpy(*string, local, used))
            return false;

    } else {

        *string = calloc(used + 1, sizeof(char *));
        if (!*string)
            return false;

        if (!memcpy(*string, local, used)) {
            free(*string);
            *string = NULL;
            return false;
        }
    }

    *size = used;
    return true;
}

/*----------------------------------------------------------------------------*/

bool ov_convert_uint64_to_string(uint64_t number, char **string, size_t *size) {

    if (!string || !size)
        return false;

    char local[100];
    size_t used = 0;

    used = snprintf(local, 100, "%" PRIu64, number);
    if (used == 0)
        return false;

    // check for truncated (unlikely)
    if (used > 100)
        return false;

    if (*string) {

        // copy to buffer
        if (*size < used)
            return false;

        if (!memcpy(*string, local, used))
            return false;

    } else {

        *string = calloc(used + 1, sizeof(char *));
        if (!*string)
            return false;

        if (!memcpy(*string, local, used)) {
            free(*string);
            *string = NULL;
            return false;
        }
    }

    *size = used;
    return true;
}

/*----------------------------------------------------------------------------*/

bool ov_convert_uint32_to_string(uint32_t number, char **string, size_t *size) {

    if (!string || !size)
        return false;

    char local[100];
    size_t used = 0;

    used = snprintf(local, 100, "%i", number);
    if (used == 0)
        return false;

    // check for truncated (unlikely)
    if (used > 100)
        return false;

    if (*string) {

        // copy to buffer
        if (*size < used)
            return false;

        if (!memcpy(*string, local, used))
            return false;

    } else {

        *string = calloc(used + 1, sizeof(char *));
        if (!*string)
            return false;

        if (!memcpy(*string, local, used)) {
            free(*string);
            *string = NULL;
            return false;
        }
    }

    *size = used;
    return true;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      HEX to BINARY ...
 *
 *      ------------------------------------------------------------------------
 */

bool ov_convert_binary_to_hex(const uint8_t *binary, size_t binary_length,
                              uint8_t **out, size_t *out_length) {

    bool created = false;

    if (!binary || !out || !out_length || binary_length < 1)
        goto error;

    if (*out) {

        if (*out_length < (binary_length * 2) + 1)
            goto error;

        *out_length = (binary_length * 2);

    } else {

        *out_length = (binary_length * 2);
        *out = calloc(*out_length + 1, sizeof(uint8_t));
        if (!*out)
            goto error;

        created = true;
    }

    uint8_t *ptr = *out;

    for (size_t i = 0; i < binary_length; i++) {

        ptr[i * 2] = (uint8_t)"0123456789ABCDEF"[binary[i] >> 4];
        ptr[i * 2 + 1] = (uint8_t)"0123456789ABCDEF"[binary[i] & 0x0F];
    }

    return true;
error:
    if (created) {
        free(*out);
        *out = NULL;
    }
    return false;
}

/*---------------------------------------------------------------------------*/

static bool hextobin(const char hex, char *out) {

    if (!out)
        goto error;

    switch (tolower(hex)) {

    case 'a':
        *out = 10;
        break;
    case 'b':
        *out = 11;
        break;
    case 'c':
        *out = 12;
        break;
    case 'd':
        *out = 13;
        break;
    case 'e':
        *out = 14;
        break;
    case 'f':
        *out = 15;
        break;
    case '0':
        *out = 0;
        break;
    case '1':
        *out = 1;
        break;
    case '2':
        *out = 2;
        break;
    case '3':
        *out = 3;
        break;
    case '4':
        *out = 4;
        break;
    case '5':
        *out = 5;
        break;
    case '6':
        *out = 6;
        break;
    case '7':
        *out = 7;
        break;
    case '8':
        *out = 8;
        break;
    case '9':
        *out = 9;
        break;
    default:
        goto error;
    }

    return true;

error:
    return false;
}

/*---------------------------------------------------------------------------*/

bool ov_convert_hex_to_binary(const uint8_t *hex, size_t hex_length,
                              uint8_t **out, size_t *out_length) {

    bool created = false;
    uint8_t clean[hex_length];
    memset(clean, 0, hex_length);

    if (!hex || !out || !out_length || hex_length < 1)
        goto error;

    size_t count = 0;

    char c = 0;

    for (size_t i = 0; i < hex_length; i++) {

        // allow and ignore spaces
        if (isspace(hex[i]))
            continue;

        // allow and ignore 0x prefixes
        if ((hex[i] == '0') && (hex[i + 1] == 'x')) {

            if (hex_length < i + 4)
                goto error;

            // check followed by 2 valid hex chars
            if (!hextobin((char)hex[i + 2], &c))
                goto error;
            if (!hextobin((char)hex[i + 3], &c))
                goto error;

            i++;
            continue;
        }

        if (!hextobin((char)hex[i], &c))
            goto error;

        clean[count] = tolower(hex[i]);
        count++;
    }

    if ((count % 2) != 0)
        goto error;

    if (*out) {

        if (*out_length < (count / 2))
            goto error;

        *out_length = (count / 2);

    } else {

        *out_length = (count / 2);
        *out = calloc(*out_length + 1, sizeof(uint8_t));
        if (!*out)
            goto error;

        created = true;
    }

    uint8_t *ptr = *out;
    char one = 0;
    char two = 0;

    size_t k = 0;
    for (size_t i = 0; i < count; i += 2) {

        if (!hextobin((char)clean[i], &one) ||
            !hextobin((char)clean[i + 1], &two))
            goto error;
        ptr[k] = (one << 4) | two;
        k++;
    }

    return true;

error:
    if (created) {
        free(*out);
        *out = NULL;
    }
    return false;
}

/*----------------------------------------------------------------------------*/

double ov_convert_from_db(double db) {

    double log_of_10 = log(10.0);

    return exp(log_of_10 * (db / 10.0));
}

/*----------------------------------------------------------------------------*/

double ov_convert_to_db(double multiplier) {

    if (0 >= multiplier) {

        return FP_NAN;

    } else {

        return 10.0 * log(multiplier) / log(10.0);
    }
}

/*----------------------------------------------------------------------------*/

static uint8_t vol_percent_to_multiplier[11] = {

    0,  1,  11, 14, 15, 26,
    46, 55, 70, 99, 100

};

uint8_t ov_convert_from_vol_percent(uint8_t percent,
                                    uint8_t db_drop_per_decade) {

    UNUSED(db_drop_per_decade);

    if (100 < percent) {
        percent = 100;
    } else if (0 == percent) {
        return 0;
    }

    return vol_percent_to_multiplier[percent / 10];
}

/*----------------------------------------------------------------------------*/

uint8_t ov_convert_to_vol_percent(uint8_t multiplier,
                                  uint8_t db_drop_per_decade) {

    UNUSED(db_drop_per_decade);

    if (0 == multiplier) {

        return 0;

    } else if (100 <= multiplier) {

        return 100;

    } else {

        for (uint8_t i = 0; i < sizeof(vol_percent_to_multiplier) /
                                    sizeof(vol_percent_to_multiplier[0]);
             ++i) {

            if (vol_percent_to_multiplier[i] >= multiplier) {
                return 10 * i;
            }
        }

        return 100;
    }
}

/*----------------------------------------------------------------------------*/
