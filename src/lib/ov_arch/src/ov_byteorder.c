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
        @file           ov_byteorder.c
        @author         Michael Beer

        @date           2017-12-10

        ------------------------------------------------------------------------
*/
#include "../include/ov_byteorder.h"

#ifndef OV_SWAP_16

#error("Unsupported Architecture");

#endif

#define __UNUSED(x)                                                            \
    do {                                                                       \
        (void)(x);                                                             \
    } while (0)

/*---------------------------------------------------------------------------*/

bool ov_byteorder_swap_bytes_16_bit(int16_t *array,
                                    size_t number_16_bit_words) {

    if (0 == array)
        goto error;

    for (size_t i = 0; i < number_16_bit_words; i++) {

        array[i] = OV_SWAP_16(array[i]);
    };

    return true;

error:

    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_byteorder_swap_bytes_64_bit(int64_t *array,
                                    size_t number_64_bit_words) {

    if (0 == array)
        goto error;

    for (size_t i = 0; i < number_64_bit_words; i++) {

        array[i] = OV_SWAP_64(array[i]);
    };

    return true;

error:

    return false;
}

/*---------------------------------------------------------------------------*/

bool ov_byteorder_from_little_endian_16_bit(int16_t *array,
                                            size_t number_16_bit_words) {

#if OV_BYTE_ORDER == OV_BIG_ENDIAN

    return ov_byteorder_swap_bytes_16_bit(array, number_16_bit_words);

#else

    __UNUSED(array);
    __UNUSED(number_16_bit_words);

    return true;

#endif
}

/*---------------------------------------------------------------------------*/

bool ov_byteorder_from_big_endian_16_bit(int16_t *array,
                                         size_t number_16_bit_words) {

#if OV_BYTE_ORDER == OV_LITTLE_ENDIAN

    return ov_byteorder_swap_bytes_16_bit(array, number_16_bit_words);

#else

    __UNUSED(array);
    __UNUSED(number_16_bit_words);

    return true;

#endif
}

/*---------------------------------------------------------------------------*/

bool ov_byteorder_to_little_endian_16_bit(int16_t *array,
                                          size_t number_16_bit_words) {

#if OV_BYTE_ORDER == OV_BIG_ENDIAN

    return ov_byteorder_swap_bytes_16_bit(array, number_16_bit_words);

#else

    __UNUSED(array);
    __UNUSED(number_16_bit_words);

    return true;

#endif
}

/*---------------------------------------------------------------------------*/

bool ov_byteorder_to_big_endian_16_bit(int16_t *array,
                                       size_t number_16_bit_words) {

#if OV_BYTE_ORDER == OV_LITTLE_ENDIAN

    return ov_byteorder_swap_bytes_16_bit(array, number_16_bit_words);

#else

    __UNUSED(array);
    __UNUSED(number_16_bit_words);

    return true;

#endif
}

/*----------------------------------------------------------------------------*/

#undef __UNUSED
