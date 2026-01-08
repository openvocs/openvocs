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
        @file           ov_hash_functions.c
        @author         Michael Beer
        @author         Markus Toepfer

        @date           2018-05-07

        @ingroup        ov_basics

        @brief


        ------------------------------------------------------------------------
*/
#include "../../include/ov_hash_functions.h"

uint64_t ov_hash_simple_c_string(const void *c_string) {

  if (0 == c_string)
    return 0;

  const char *s = c_string;

  uint8_t hash = 0;

  uint8_t c = (uint8_t)*s;

  while (0 != c) {

    /* Use a prime as factor to prevent short cycles  -
     * if you dont understand, leave it as it is ... */
    hash += 13 * c;
    c = (uint8_t) * (++s);
  }

  return hash;
}

/******************************************************************************
 *                               PEARSON HASHING
 *                       Pearson, Peter K. (June 1990),
 *              "Fast Hashing of Variable-Length Text Strings",
 *                     Communications of the ACM, 33 (6)
 ******************************************************************************/

static const uint8_t PEARSON_PERMUTATION_TABLE[256] = {
    98,  6,   85,  150, 36,  23,  112, 164,
    135, 207, 169, 5,   26,  64,  165, 219, //  1
    61,  20,  68,  89,  130, 63,  52,  102,
    24,  229, 132, 245, 80,  216, 195, 115, //  2
    90,  168, 156, 203, 177, 120, 2,   190,
    188, 7,   100, 185, 174, 243, 162, 10, //  3
    237, 18,  253, 225, 8,   208, 172, 244,
    255, 126, 101, 79,  145, 235, 228, 121, //  4
    123, 251, 67,  250, 161, 0,   107, 97,
    241, 111, 181, 82,  249, 33,  69,  55, //  5
    59,  153, 29,  9,   213, 167, 84,  93,
    30,  46,  94,  75,  151, 114, 73,  222, //  6
    197, 96,  210, 45,  16,  227, 248, 202,
    51,  152, 252, 125, 81,  206, 215, 186, //  7
    39,  158, 178, 187, 131, 136, 1,   49,
    50,  17,  141, 91,  47,  129, 60,  99, //  8
    154, 35,  86,  171, 105, 34,  38,  200,
    147, 58,  77,  118, 173, 246, 76,  254, //  9
    133, 232, 196, 144, 198, 124, 53,  4,
    108, 74,  223, 234, 134, 230, 157, 139, // 10
    189, 205, 199, 128, 176, 19,  211, 236,
    127, 192, 231, 70,  233, 88,  146, 44, // 11
    183, 201, 22,  83,  13,  214, 116, 109,
    159, 32,  95,  226, 140, 220, 57,  12, // 12
    221, 31,  209, 182, 143, 92,  149, 184,
    148, 62,  113, 65,  37,  27,  106, 166, // 13
    3,   14,  204, 72,  21,  41,  56,  66,
    28,  193, 40,  217, 25,  54,  179, 117, // 14
    238, 87,  240, 155, 180, 170, 242, 212,
    191, 163, 78,  218, 137, 194, 175, 110, // 15
    43,  119, 224, 71,  122, 142, 42,  160,
    104, 48,  247, 103, 15,  11,  138, 239 // 16
};

uint64_t ov_hash_pearson_c_string(const void *c_string) {

  if (0 == c_string)
    return 0;

  const char *s = c_string;
  uint8_t h = 0;
  uint8_t c = (uint8_t)*s;

  while (0 != c) {

    h = PEARSON_PERMUTATION_TABLE[h ^ c];
    c = (uint8_t) * (++s);
  }

  return h;
}

/*----------------------------------------------------------------------------*/

uint64_t ov_hash_intptr(const void *intptr) {
  uint64_t u64_ptr = (intptr_t)intptr;
  return u64_ptr;
}

/*----------------------------------------------------------------------------*/

uint64_t ov_hash_uint64(const void *uint64) {

  if (!uint64)
    return 0;

  uint64_t *ptr = (uint64_t *)uint64;
  if (!ptr)
    return 0;

  return (uint64_t)*ptr;
}

/*----------------------------------------------------------------------------*/

uint64_t ov_hash_int64(const void *int64) {

  if (!int64)
    return 0;

  int64_t *ptr = (int64_t *)int64;
  if (!ptr)
    return 0;

  return (uint64_t)*ptr;
}
