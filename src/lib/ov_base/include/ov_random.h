/***
        ------------------------------------------------------------------------

        Copyright (c) 2019 German Aerospace Center DLR e.V. (GSOC)

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
        @author         Michael Beer
        @author         Markus Toepfer

        @date           2019-08-02

        @ingroup        ov_random

        These functions are NOT cryptographically secure!

        Look at the Monte Carlo way to approximate Pi in the tests...

        The random(3) function is a weak PRNG implementation - it is not
        even guaranteed to produce more than 16 (!!!) random bits.

        Hence we use our own PRNG, which will produce up to 32 random bits,
        but nevertheless is only suitable for choosing arbitrary ports etc.

        ------------------------------------------------------------------------
*/
#ifndef ov_random_h
#define ov_random_h

#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>

/*----------------------------------------------------------------------------*/

/**
        Create some random bytes.

        @param buffer   buffer to write to
        @param size     size of the buffer to write to
*/
bool ov_random_bytes_with_zeros(uint8_t *buffer, size_t size);

/*----------------------------------------------------------------------------*/

/**
        Create some random bytes.

        @NOTE this function will set at least one bit in each byte of the buffer
        if you want to have some random buffer including \0 buffer item, use
        @ov_random_bytes_with_zeros

        @param buffer   buffer to write to
        @param size     size of the buffer to write to
*/
bool ov_random_bytes(uint8_t *buffer, size_t size);

/*----------------------------------------------------------------------------*/

/**
        Creates a random string of length length .
        The length includes the terminal 0 .

        Thus fill_biffer_random_string(&my_buffer,0) will create an empty
   string. If *buffer == 0, a new buffer will be allocated and *buffer pointed
   to the address of this new buffer. If *buffer != 0, this function assumes
   that *buffer is a pointer to a buffer of a size of at least length bytes.

        @param alphabet_in      if not 0, uses the chars listed in this string
                                to fill the buffer

        @return                 true on success, false otherwise
 */
bool ov_random_string(char **buffer, size_t length, const char *alphabet_in);

/*----------------------------------------------------------------------------*/

/**
        Create a random uint32_t
*/
uint32_t ov_random_uint32();

/*----------------------------------------------------------------------------*/

/**
        Create a random uint64_t

*/
uint64_t ov_random_uint64();

/*----------------------------------------------------------------------------*/

/**
        Create a random uint64_t in given range
        (max MUST be higher than min)

        @param min min value to create
        @param max max value to create

        @returns 0 on error
*/
uint64_t ov_random_range(uint64_t min, uint64_t max);

/*----------------------------------------------------------------------------*/

/**
 * Gives up to 2 gauss distributed random values between -1.0 and 1.0
 * with variance 1 and mean value 0
 *
 * @param r2 pointer to write second random value, might be 0 if none desired
 * @return gauss distributed random value
 */
bool ov_random_gaussian(double *r1, double *r2);

/*----------------------------------------------------------------------------*/
#endif /* ov_random_h */
