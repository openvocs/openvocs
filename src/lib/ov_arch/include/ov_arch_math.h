/***
        ------------------------------------------------------------------------

        Copyright (c) 2021 German Aerospace Center DLR e.V. (GSOC)

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
        @copyright (c) 2021 German Aerospace Center DLR e.V. (GSOC)

        ------------------------------------------------------------------------
*/
#ifndef OV_MATH_H
#define OV_MATH_H
/*----------------------------------------------------------------------------*/

#include "ov_arch.h"
#include <math.h>

/*----------------------------------------------------------------------------*/

#define OV_MIN(x, y) __OV_MIN(x, y)

#define OV_MAX(x, y) __OV_MAX(x, y)

/*****************************************************************************
                                   internals
 ****************************************************************************/

/*----------------------------------------------------------------------------*/
//                                    MIN

#if OV_COMPILER == OV_GCC

/* Fast, because we just use macros, no function calls required,
 * but no standard C11 */

#define __OV_MIN(x, y)                                                         \
    ({                                                                         \
        __typeof__(x) _x = (x);                                                \
        __typeof__(y) _y = (y);                                                \
        _x < _y ? _x : _y;                                                     \
    })

#else

/* Every other compiler falls back to plain C11 */

int __ov_min_i(int const x, int const y);
unsigned __ov_min_u(unsigned const x, unsigned const y);
long __ov_min_l(long const x, long const y);
unsigned long __ov_min_ul(unsigned long const x, unsigned long const y);
long long __ov_min_ll(long long const x, long long const y);
unsigned long long __ov_min_ull(unsigned long long const x,
                                unsigned long long const y);

#define __OV_MIN(X, Y)                                                         \
    (_Generic((X) + (Y),                                                       \
        int: __ov_min_i,                                                       \
        unsigned: __ov_min_u,                                                  \
        long: __ov_min_l,                                                      \
        unsigned long: __ov_min_ul,                                            \
        long long: __ov_min_ll,                                                \
        unsigned long long: __ov_min_ull,                                      \
        float: fminf,                                                          \
        double: fmin,                                                          \
        long double: fminl)((X), (Y)))

#endif

/*----------------------------------------------------------------------------*/
//                                    MAX

#if OV_COMPILER == OV_GCC

/* Fast, because we just use macros, no function calls required,
 * but no standard C11 */

#define __OV_MAX(x, y)                                                         \
    ({                                                                         \
        __typeof__(x) _x = (x);                                                \
        __typeof__(y) _y = (y);                                                \
        _x > _y ? _x : _y;                                                     \
    })

#else

/* Every other compiler falls back to plain C11 */

int __ov_max_i(int const x, int const y);
unsigned __ov_max_u(unsigned const x, unsigned const y);
long __ov_max_l(long const x, long const y);
unsigned long __ov_max_ul(unsigned long const x, unsigned long const y);
long long __ov_max_ll(long long const x, long long const y);
unsigned long long __ov_max_ull(unsigned long long const x,
                                unsigned long long const y);

#define __OV_MAX(X, Y)                                                         \
    (_Generic((X) + (Y),                                                       \
        int: __ov_max_i,                                                       \
        unsigned: __ov_max_u,                                                  \
        long: __ov_max_l,                                                      \
        unsigned long: __ov_max_ul,                                            \
        long long: __ov_max_ll,                                                \
        unsigned long long: __ov_max_ull,                                      \
        float: fmaxf,                                                          \
        double: fmax,                                                          \
        long double: fmaxl)((X), (Y)))

#endif

/*----------------------------------------------------------------------------*/
#endif
