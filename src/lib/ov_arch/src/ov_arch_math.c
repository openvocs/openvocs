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

        @author         Michael J. Beer, DLR/GSOC

        ------------------------------------------------------------------------
*/

/*----------------------------------------------------------------------------*/

#include "../include/ov_arch_math.h"

#if OV_COMPILER != OV_GCC

/*----------------------------------------------------------------------------*/

int __ov_min_i(int const x, int const y) { return y < x ? y : x; }

/*----------------------------------------------------------------------------*/

unsigned __ov_min_u(unsigned const x, unsigned const y) {
    return y < x ? y : x;
}

/*----------------------------------------------------------------------------*/

long __ov_min_l(long const x, long const y) { return y < x ? y : x; }

/*----------------------------------------------------------------------------*/

unsigned long __ov_min_ul(unsigned long const x, unsigned long const y) {
    return y < x ? y : x;
}

/*----------------------------------------------------------------------------*/

long long __ov_min_ll(long long const x, long long const y) {
    return y < x ? y : x;
}

/*----------------------------------------------------------------------------*/

unsigned long long __ov_min_ull(unsigned long long const x,
                                unsigned long long const y) {
    return y < x ? y : x;
}

/*****************************************************************************
                                      MAX
 ****************************************************************************/

int __ov_max_i(int const x, int const y) { return y > x ? y : x; }

/*----------------------------------------------------------------------------*/

unsigned __ov_max_u(unsigned const x, unsigned const y) {
    return y > x ? y : x;
}

/*----------------------------------------------------------------------------*/

long __ov_max_l(long const x, long const y) { return y > x ? y : x; }

/*----------------------------------------------------------------------------*/

unsigned long __ov_max_ul(unsigned long const x, unsigned long const y) {
    return y > x ? y : x;
}

/*----------------------------------------------------------------------------*/

long long __ov_max_ll(long long const x, long long const y) {
    return y > x ? y : x;
}

/*----------------------------------------------------------------------------*/

unsigned long long __ov_max_ull(unsigned long long const x,
                                unsigned long long const y) {
    return y > x ? y : x;
}

/*----------------------------------------------------------------------------*/
#endif
