/***
        ------------------------------------------------------------------------

        Copyright 2017 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_utf8.c
        @author         Markus Toepfer

        @date           2017-12-17

        @ingroup        ov_basics

        @brief          Implementation of UTF8 functionality.

        This implementation SHALL provide the interface for UTF8 functionality
        needed for ov_service instances.

        In particular it SHALL provide some basic functions needed to verify
        and test if a byte (octett) stream contains UTF8 encoded data.

        ------------------------------------------------------------------------
*/
#include "../../include/ov_utf8.h"

/*----------------------------------------------------------------------------*/

uint8_t *ov_utf8_last_valid(const uint8_t *start, uint64_t length,
                            bool character_start) {

    if (!start)
        return NULL;

    uint8_t *p = (uint8_t *)start;
    uint8_t *c = NULL;

    uint64_t open = length;

    while (open > 0) {

        if (p[0] <= 0x7f) {
            /* ASCII UTF8-1 = %x00-7F*/
            c = p;
            p++;
            open--;

        } else if (p[0] < 0xC2) {

            break;

        } else if (p[0] <= 0xDF) {

            /* UTF8-2 = %xC2-DF UTF8-tail */

            /* 110xxxxx 10xxxxxx */

            if (open < 2)
                break;

            if ((p[1] >> 6) != 0x02)
                break;

            c = p;
            p += 2;
            open -= 2;

        } else if (p[0] < 0xF0) {

            /*
               UTF8-3 =     %xE0 %xA0-BF UTF8-tail /
                            %xE1-EC 2( UTF8-tail ) /
                            %xED %x80-9F UTF8-tail /
                            %xEE-EF 2( UTF8-tail )
            */

            /* 1110xxxx 10xxxxxx 10xxxxxx*/
            if (open < 3)
                break;

            if (p[0] == 0xE0)
                if (p[1] < 0xA0)
                    break;

            if (p[0] == 0xED)
                if (p[1] > 0x9F)
                    break;

            if ((p[1] >> 6) != 0x02)
                break;
            if ((p[2] >> 6) != 0x02)
                break;

            c = p;
            p += 3;
            open -= 3;

        } else if (p[0] <= 0xF4) {

            /*
               UTF8-4 =     %xF0 %x90-BF 2( UTF8-tail ) /
                            %xF1-F3 3( UTF8-tail ) /
                            %xF4 %x80-8F 2( UTF8-tail )
            */

            /* 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx*/
            if (open < 4)
                break;

            if (p[0] == 0xf0)
                if (p[1] < 0x90)
                    break;

            if (p[0] == 0xf4)
                if (p[1] > 0x8f)
                    break;

            if ((p[1] >> 6) != 0x02)
                break;
            if ((p[2] >> 6) != 0x02)
                break;
            if ((p[3] >> 6) != 0x02)
                break;

            c = p;
            p += 4;
            open -= 4;

        } else {

            break;
        }
    }

    if (character_start)
        return c;

    return p;
}

/*----------------------------------------------------------------------------*/

bool ov_utf8_validate_sequence(const uint8_t *start, uint64_t length) {

    if (!start || length < 1)
        return false;

    uint8_t *result = ov_utf8_last_valid(start, length, false);
    if (!result)
        return false;

    if ((uint64_t)(result - start) != length)
        return false;

    return true;
}

/*----------------------------------------------------------------------------*/

bool ov_utf8_encode_code_point(uint64_t number, uint8_t **start, uint64_t open,
                               uint64_t *used) {

    if (!start || !used)
        return false;

    uint8_t *array = *start;
    *used = 0;

    if (number <= 127) {

        if (array == NULL) {

            array = calloc(1, sizeof(uint8_t));
            *start = array;

        } else if (open < 1) {

            return false;
        }

        *used = 1;

        /* UTF-8 is ASCII */
        array[0] = number & 0x7F;
        return true;

    } else if (number <= 0x07FF) {

        if (array == NULL) {

            array = calloc(2, sizeof(uint8_t));
            *start = array;

        } else if (open < 2) {

            return false;
        }

        *used = 2;

        array[1] = 0x80 | (0x3F & number);
        array[0] = 0xC0 | (0x1F & (number >> 6));

        return true;

    } else if ((number >= 0xD800) && (number <= 0xDFFF)) {

        // reserved for UTF 16
        return false;

    } else if (number <= 0xFFFF) {

        if (array == NULL) {

            array = calloc(3, sizeof(uint8_t));
            *start = array;

        } else if (open < 3) {

            return false;
        }

        *used = 3;

        array[2] = 0x80 | (0x3F & number);
        array[1] = 0x80 | (0x3F & number >> 6);
        array[0] = 0xE0 | (0x0F & number >> 12);

        return true;

    } else if (number <= 0x10FFFF) {

        if (array == NULL) {

            array = calloc(4, sizeof(uint8_t));
            *start = array;

        } else if (open < 4) {

            return false;
        }

        *used = 4;

        array[3] = 0x80 | (0x3F & number);
        array[2] = 0x80 | (0x3F & number >> 6);
        array[1] = 0x80 | (0x3F & number >> 12);
        array[0] = 0xF0 | (0x07 & number >> 18);
        return true;
    }

    return false;
}

/*----------------------------------------------------------------------------*/

uint64_t ov_utf8_decode_code_point(const uint8_t *p, uint64_t open,
                                   uint64_t *bytes) {

    if (!p || open < 1 || !bytes)
        return 0;

    *bytes = 0;

    if (p[0] <= 0x7f) {

        /* ASCII */
        *bytes = 1;
        return p[0] & 0x7F;

    } else if (p[0] < 0xC2) {

        return 0;

    } else if (p[0] <= 0xDF) {

        /* 110xxxxx 10xxxxxx */
        if (open < 2)
            return 0;

        if ((p[1] >> 6) != 0x02)
            return 0;

        /*
         *      110aaaaa10bbbbbb
         *          aaaaaa000000 (c[0] << 6)
         *          111111000000 0x07C0
         *              00bbbbbb c[1] & 0x3F
         *     -----------------
         *          aaaaaabbbbbb
         */
        *bytes = 2;
        return (((p[0] << 6) & 0x07C0) | (p[1] & 0x3F));

    } else if (p[0] < 0xF0) {

        /* 1110xxxx 10xxxxxx 10xxxxxx*/
        if (open < 3)
            return 0;

        if (p[0] == 0xE0)
            if (p[1] < 0xA0)
                return 0;

        if (p[0] == 0xED)
            if (p[1] > 0x9F)
                return 0;

        if ((p[1] >> 6) != 0x02)
            return 0;
        if ((p[2] >> 6) != 0x02)
            return 0;
        ;

        /*
         *      1110aaaa10bbbbbb10cccccc
         *              aaaa000000000000 (c[0] << 12)
         *              1111000000000000 0xF000
         *                  bbbbbb000000 (c[2] << 6)
         *                  111111000000 0x0FC0
         *                      00cccccc c[3] & 0x3F
         *     -------------------------
         *              aaaabbbbbbcccccc
         */
        *bytes = 3;
        return (((p[0] << 12) & 0xF000) | ((p[1] << 6) & 0x0FC0) |
                (p[2] & 0x3F));

    } else if (p[0] <= 0xF4) {

        /* 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx*/
        if (open < 4)
            return 0;

        if (p[0] == 0xf0)
            if (p[1] < 0x90)
                return 0;

        if (p[0] == 0xf4)
            if (p[1] > 0x8f)
                return 0;

        if ((p[1] >> 6) != 0x02)
            return 0;
        if ((p[2] >> 6) != 0x02)
            return 0;
        if ((p[3] >> 6) != 0x02)
            return 0;

        /*
         *      11110aaa10bbbbbb10cccccc10dddddd
         *                 aaa000000000000000000 (c[0] << 18)
         *                 111000000000000000000 0x1C0000
         *                    bbbbbb000000000000 (c[1] << 12)
         *                    111111000000000000 0x3F000
         *                          cccccc000000 (c[2] << 6)
         *                          111111000000 0x0FC0
         *                              00dddddd c[3] & 0x3F
         *     ---------------------------------
         *                 aaabbbbbbccccccdddddd
         */
        *bytes = 4;
        return (((p[0] << 18) & 0x1C0000) | ((p[1] << 12) & 0x3F000) |
                ((p[2] << 6) & 0x0FC0) | (p[3] & 0x3F));
    }

    if (bytes)
        *bytes = 0;

    return 0;
}

/*----------------------------------------------------------------------------*/

bool ov_utf8_generate_random_buffer(uint8_t **buffer, size_t *size,
                                    uint16_t unicode_chars) {

    bool created = false;

    if (!buffer || !size || unicode_chars < 1)
        goto error;

    if (!*buffer) {

        *size = 4 * unicode_chars;
        *buffer = calloc(*size, sizeof(uint8_t));
        created = true;

    } else {

        if (*size < 4 * unicode_chars)
            goto error;
    }

    uint64_t number = 0;
    uint8_t *pointer = *buffer;
    size_t open = *size;
    uint64_t used = 0;

    if (!pointer)
        goto error;

    bool done = false;

    for (uint16_t i = 0; i < unicode_chars; i++) {

        done = false;
        while (!done) {

            number = random();

            // get a random value of the basic multilungual plane
            // only
            // 0x0000 - 0xFFFF
            number = (number * 0xFFFF) / RAND_MAX;

            // a uint64_t in range 0x0000 to 0xFFFF
            // may not be a valid utf8 input
            if (ov_utf8_encode_code_point(number, &pointer, open, &used))
                done = true;
        }

        open -= used;
        pointer = pointer + used;
    }

    // set size to used bytes
    *size = *size - open;
    return true;

error:

    if (buffer) {

        if (created) {
            free(*buffer);
            *buffer = NULL;
            *size = 0;
        }
    }

    return false;
}
