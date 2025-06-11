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

        ------------------------------------------------------------------------
*/
#include "../../include/ov_random.h"
#include "../../include/ov_time.h"
#include "../../include/ov_utils.h"
#include <math.h>
#include <stdint.h>
#include <string.h>

/*****************************************************************************
                                    The PRNG
 ****************************************************************************/

/*
 * As the library function random(3) is not guaranteed to produce more than
 * 16 bit random numbers, we need to rely on our own prng - it is not that hard
 * if we do not want cryptographically safe rngs...
 *
 * However, this manually implemented PRNG (with a little help by
 * Mr Donald Knuth) performs better than the currently available
 * glibc implementation of random(3) under linux
 *
 * The Monte Carlo calculation of PI yields results closer to PI by
 * 1/100, and plotting the points shows an EVEN distribution.
 *
 * If we feel the need for a better PRNG, we could use the Mersenne Twister:
 *
 * https://web.archive.org/web/20070918014705/http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/MT2002/CODES/mt19937ar.c
 *
 * It would be MIT
 *
 */

static uint32_t g_random_number = 144312;
static bool g_random_seeded = false;

/*----------------------------------------------------------------------------*/

static void init_genrand(uint32_t s) { g_random_number = s; }

/*----------------------------------------------------------------------------*/

static void genrand_reseed() {

    if (g_random_seeded) return;

    init_genrand(ov_time_get_current_time_usecs());
    g_random_seeded = true;
}

/*----------------------------------------------------------------------------*/

static uint32_t genrand_int32() {

    // This is the general form of a linear congruental method -
    // with reasonable values for A, B, M
    // static uint64_t a = ...;
    // static uint64_t c = ...;
    // static uint64_t m = ...;
    // g_random_number = (a * g_random_number + c) % m;

    // We choose m to be 0xffffffff == UINT32_MAX + 1, then
    // r mod m is calculated easily by r overflowing on 32 bit
    // (See Knuth, TAOCP Vol2, 12pp)

    // m = 2^32, hence its only prime factor is '2'
    // thus, according to TAOCP, 3.2.1.2, Theorem A,
    // a - 1 should be a multiple of 2
    // Since m is a multiple of 4, a - 1 should also be a multiple of 4
    static uint64_t a = 0xea5f3f01;

    uint64_t r = g_random_number;
    r *= a;

    // This new random number does not really alternate much in the lower order
    // bits
    // Thus, we do it once more, then combine the 2 higher order 16 bit chunks
    // into our 'real' number
    // Does not add randomness, but our PRNG is just for basics
    // like choosing arbitrary ports...

    uint32_t r1 = (uint32_t)r;

    uint64_t r2 = r1;
    r *= a;

    g_random_number = r1 + (r2 >> 16);

    return g_random_number;
}

/*----------------------------------------------------------------------------*/

void ov_random_reseed() { genrand_reseed(); }

/*----------------------------------------------------------------------------*/

bool ov_random_bytes_with_zeros(uint8_t *buffer, size_t size) {

    genrand_reseed();

    if (!buffer || size < 1) return false;

    for (size_t i = 0; i < size; i++) {
        uint32_t r = genrand_int32();
        uint8_t *bytes = (uint8_t *)&r;
        buffer[i] = bytes[3];
    }

    return true;
}

/*----------------------------------------------------------------------------*/

bool ov_random_bytes(uint8_t *buffer, size_t size) {

    if (!buffer || size < 1) return false;

    genrand_reseed();

    for (size_t i = 0; i < size; i++) {
        uint32_t r = 0;
        uint8_t *bytes = 0;
        do {
            r = genrand_int32();
            bytes = (uint8_t *)&r;
            buffer[i] = bytes[3];
        } while (0 == buffer[i]);
    }

    return true;
}

/*----------------------------------------------------------------------------*/

bool ov_random_string(char **buffer, size_t length, const char *alphabet_in) {

    const char *alphabet = OV_OR_DEFAULT(
        alphabet_in, "1234567890ABCDEFGHJKMNPQRSTWXZabcdefghjkmnpqrstwxz");

    size_t alphabet_max_index = strlen(alphabet) - 1;

    size_t index = 0;
    char *pointer = 0;

    if (0 == length) {
        goto error;
    }

    if (0 == *buffer) {
        pointer = calloc(length, sizeof(uint8_t));
        *buffer = pointer;
    } else {
        pointer = *buffer;
    }

    genrand_reseed();

    for (index = 0; index < length - 1; ++index) {
        pointer[index] = alphabet[ov_random_range(0, alphabet_max_index)];
    }

    pointer[length - 1] = 0;

    return true;

error:
    return false;
}

/*----------------------------------------------------------------------------*/

uint32_t ov_random_uint32() {

    genrand_reseed();
    return genrand_int32();
}

/*----------------------------------------------------------------------------*/

uint64_t ov_random_uint64() {

    genrand_reseed();

    uint64_t out = genrand_int32();
    return (out << 32) | genrand_int32();
}

/*----------------------------------------------------------------------------*/

uint64_t ov_random_range(uint64_t min, uint64_t max) {

    if (0 == max) {
        max = INT64_MAX;
    }

    if (0 == min) {
        min = 1;
    }

    if (min > max) {
        goto error;
    }

    if (min == max) {
        return min;
    }

    double r = ov_random_uint64();
    r /= (double)UINT64_MAX;
    r *= (max - min);

    uint64_t r_i = (uint64_t)r;

    return min + r_i;

error:

    return 0;
}

/*----------------------------------------------------------------------------*/

static double rnd_unit_interval() {

    static const double SCALE_FACTOR = 2.0 / ((double)UINT32_MAX);

    double rnd = ov_random_uint32();
    rnd *= SCALE_FACTOR;

    rnd -= 1.0;

    return rnd;
}

/*----------------------------------------------------------------------------*/

bool ov_random_gaussian(double *r1, double *r2) {

    if ((0 == r1) && (0 == r2)) {
        return false;
    }

    // Knuth, TAOCP Vol. 1, pp. 122:
    // 'Algorithm P'

    double v1 = 0.0;
    double v2 = 0.0;
    double s = 0.0;

    do {

        v1 = rnd_unit_interval();
        v2 = rnd_unit_interval();

        s = v1 * v1 + v2 * v2;

    } while (s >= 1.0);

    OV_ASSERT(0 < s);
    OV_ASSERT(s < 1.0);

    if (0 != s) {
        double l = log(s);
        s = -2.0 * l / s;
        s = sqrt(s);
    }

    if (0 != r1) {
        *r1 = v1 * s;
    }

    if (0 != r2) {
        *r2 = v2 * s;
    }

    return true;
}

/*----------------------------------------------------------------------------*/
