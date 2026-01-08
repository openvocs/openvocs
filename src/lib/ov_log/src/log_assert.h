/***
        ------------------------------------------------------------------------

        Copyright (c) 2022 German Aerospace Center DLR e.V. (GSOC)

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

        @author Michael J. Beer, DLR/GSOC
        @copyright (c) 2022 German Aerospace Center DLR e.V. (GSOC)

        ------------------------------------------------------------------------
*/
#ifndef LOG_ASSERT_H
#define LOG_ASSERT_H
/*----------------------------------------------------------------------------*/
#include <stdio.h>
/*----------------------------------------------------------------------------*/

/**
 * We do not use plain `assert` as we cannot really control how it behaves -
 * By using our own version, we can modify its behaviour.
 */
#define LOG_ASSERT(x) LOG_ASSERT_INTERNAL(x)

/*****************************************************************************
                                   INTERNALS
 ****************************************************************************/

#undef assert
#define assert(x) _Static_assert(0, "Trying to use 'assert'")

#ifdef DEBUG

#define LOG_ASSERT_TO_STR_HELPER_INTERNAL(x) #x
#define LOG_ASSERT_TO_STR_INTERNAL(x) LOG_ASSERT_TO_STR_HELPER_INTERNAL(x)

// Only reproduce the behaviour of `assert`
#define LOG_ASSERT_INTERNAL(x)                                                 \
  do {                                                                         \
    bool cond = (x);                                                           \
    if (!cond) {                                                               \
      fprintf(stderr, "%s\n", LOG_ASSERT_TO_STR_INTERNAL(x));                  \
      abort();                                                                 \
    }                                                                          \
  } while (0)

#else

#define LOG_ASSERT_INTERNAL(x)

#endif

/*----------------------------------------------------------------------------*/
#endif
