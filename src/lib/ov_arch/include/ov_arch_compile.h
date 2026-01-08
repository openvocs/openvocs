/***
        ------------------------------------------------------------------------

        Copyright (c) 2020 German Aerospace Center DLR e.V. (GSOC)

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
        @copyright (c) 2020 German Aerospace Center DLR e.V. (GSOC)

        ------------------------------------------------------------------------
*/
#ifndef OV_ARCH_COMPILE_H
#define OV_ARCH_COMPILE_H

#include <stdlib.h>

/*----------------------------------------------------------------------------*/

#define OV_ARCH_COMPILE_WARNING(message)                                       \
    OV_ARCH_COMPILE_WARNING_INTERNAL(message)

#define OV_ARCH_COMPILE_DEPRECATED(message)                                    \
    OV_ARCH_COMPILE_DEPRECATED_INTERNAL(message)

#define OV_ARCH_COMPILE_ATTRIBUTE_WARNING(message)                             \
    OV_ARCH_COMPILE_ATTRIBUTE_WARNING_INTERNAL(message)

/**
 * Returns the backtrace of the current function
 */
char *ov_arch_compile_backtrace(size_t max_frames);

/*----------------------------------------------------------------------------
                                   Recent GCC
  ----------------------------------------------------------------------------*/

#if defined(__GNUC__) &&                                                       \
    (((__GNUC__ == 4) && (__GNUC_MINOR__ >= 5)) || (__GNUC__ > 4))

#define OV_ARCH_COMPILE_WARNING_INTERNAL(message)

/* Supported since
 * [GCC 4.5.0](https://gcc.gnu.org/gcc-4.5/changes.html) */

#define OV_ARCH_COMPILE_DEPRECATED_INTERNAL(message)                           \
    __attribute__((deprecated(message)))

/* Earliest GCC version support unknown - 4.5.4 does it ... */
#define OV_ARCH_COMPILE_ATTRIBUTE_WARNING_INTERNAL(message)                    \
    __attribute__((warning(message)))

/*----------------------------------------------------------------------------
                                   Older GCC
  ----------------------------------------------------------------------------*/

#elif defined(__GNUC__)

/* Unfortunately, unknown since when this has been supported... */

#define OV_ARCH_COMPILE_WARNING_INTERNAL(message)

#define OV_ARCH_COMPILE_DEPRECATED_INTERNAL(message) __attribute__((deprecated))

#define OV_ARCH_COMPILE_ATTRIBUTE_WARNING_INTERNAL(message)                    \
    __attribute__((warning(message)))

/*----------------------------------------------------------------------------
                                     CLANG
  ----------------------------------------------------------------------------*/

#elif define(__clang__)

#define OV_ARCH_COMPILE_WARNING_INTERNAL(message)

#define OV_ARCH_COMPILE_DEPRECATED_INTERNAL(message) __attribute__((deprecated))

#define OV_ARCH_COMPILE_ATTRIBUTE_WARNING_INTERNAL(message)                    \
    __attribute__((warning(message)))

/*----------------------------------------------------------------------------
                                    GARBAGE
  ----------------------------------------------------------------------------*/

#else

#error("Unsupported compiler")

#endif /* defined(__clang__) */

/*----------------------------------------------------------------------------*/
#endif
