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
        @file           ov_utils.h
        @author         Michael Beer

        @date           2017-12-10

        @ingroup        ov_base

        @brief


        ------------------------------------------------------------------------
*/
#ifndef ov_utils_h
#define ov_utils_h

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "ov_constants.h"
#include <ov_arch/ov_arch_compile.h>
#include <ov_log/ov_log.h>

/*----------------------------------------------------------------------------*/

#define OV_OR_DEFAULT(x, y) ((0 == (x)) ? (y) : (x))

/*----------------------------------------------------------------------------*/

/**
 * Our objects start with a magic number of 32 bit:
 *
 * typedef struct {
 *  uint32_t magic_number;
 *  ...
 *  } our_object;
 *
 * This number is to be unique.
 *
 * This function returns the pointer if it is not null and its magic bytes match
 * the expected magic bytes.
 *
 * Use like
 *
 * void our_function(void *vptr) {
 *     ...
 *     our_object *o = OV_UTILS_AS_TYPE(vptr, OUR_OBJECT_MAGIC_BYTES);
 *     ...
 * }
 */

#define OV_UTILS_AS_TYPE(vptr, magic_bytes_32bit)                              \
    (0 != vptr) && (magic_bytes_32bit == *((uint32_t *)vptr)) ? vptr : 0;

/*----------------------------------------------------------------------------*/

/**
 * Output trace (for 'step' like debugging)
 */
#define OV_TRACE fprintf(stderr, "TRACE %s: %i\n", __FUNCTION__, __LINE__)

/*----------------------------------------------------------------------------*/

#define TO_STR(x) __TO_STR_HELPER(x)

/*----------------------------------------------------------------------------*/

#define OV_ASSERT(cond) OV_ASSERT_INTERNAL(cond)

#undef assert
#define assert(x) _Static_assert(0, "Trying to use 'assert'")

/*----------------------------------------------------------------------------*/

/**
        Declare an argument of a function to be unused
        to prevent compiler to complain
 */
#define UNUSED(x)                                                              \
    do {                                                                       \
        (void)(x);                                                             \
    } while (0)

/*----------------------------------------------------------------------------*/

/**
 * NO-OPERATION.
 *
 * Use like
 *
 * `if(BRANCH_NOT_IMPLEMENTED_YET) NOP;`
 *
 * Helper mainly for development.
 */
#define NOP                                                                    \
    while (0) {                                                                \
    }

#define OV_PANIC(msg)                                                          \
    do {                                                                       \
        fprintf(stderr, msg);                                                  \
        abort();                                                               \
    } while (0)

/*----------------------------------------------------------------------------*/

/**
 * Declare a function deprecated.
 *
 * Use like
 *
 * `DEPRECATED("Use get_new_style() instead") int get_old_style() {
 *    return 0;
 * };`
 *
 * Depending on the compiler, issues a warning if the function is used or
 * has no effect.
 */
#define DEPRECATED(message) DEPRECATED_INTERNAL(message)

/**
 * Produce warning on compile time
 */
#define WARNING(message) WARNING_INTERNAL(message)

/*----------------------------------------------------------------------------*/

/**
 * Declare missing Unit Test
 *
 * Use like
 *
 * `TEST_MISSING int a_func() {
 *    return 0;
 * };`
 *
 */
#define TEST_MISSING TEST_MISSING_INTERNAL

/*----------------------------------------------------------------------------*/

/**
 * Mark TODOs and ensure they will not go unnoticed...
 */

#define TODO(message) TODO_INTERNAL(message)

/*----------------------------------------------------------------------------*/

/**
 * Mark stuff that should not remain in release build
 */

#define NOT_IN_RELEASE(msg) NOT_IN_RELEASE_INTERNAL(msg)

/*----------------------------------------------------------------------------*/

/**
 * Convert dB to an actual factor.
 *
 * We use the ACTUAL mathematical definition: 1Bell = 10^(1) = 10
 * 1 dB = 1/10 Bell => 10dB = 10^(10 * 1/10) = 10
 *
 * If you deal with dBs that some weird engineers redefined, this function
 * of course does not apply!
 *
 */
double ov_utils_db_to_factor(double db);

/*----------------------------------------------------------------------------*/

/**
 * Safe free.
 * Fress vptr if it is not null.
 * @return 0 on success, otherwise vptr
 */
void *ov_free(void *vptr);

/*----------------------------------------------------------------------------*/

/**
 * If cond is false, logs `msg` as error.
 * Returns cond
 */
#define ov_cond_valid(cond, msg)                                               \
    ov_cond_valid_internal(__FILE__, OV_LOG_ERR, __FUNCTION__, __LINE__, 0,    \
                           cond, 0, msg)

/*----------------------------------------------------------------------------*/

/**
 * If ptr is null, prints appropriate error message
 */
#define ov_ptr_valid(ptr, msg)                                                 \
    ov_ptr_valid_internal(__FILE__, OV_LOG_ERR, __FUNCTION__, __LINE__, 0,     \
                          ptr, 0, msg)

/*----------------------------------------------------------------------------*/

/**
 * If cond is false, logs `msg` as warning.
 * Returns cond
 */
#define ov_cond_valid_warn(cond, msg)                                          \
    ov_cond_valid_internal(__FILE__, OV_LOG_WARNING, __FUNCTION__, __LINE__,   \
                           0, cond, 0, msg)

/*----------------------------------------------------------------------------*/

/**
 * If ptr is null, prints appropriate warn message
 */
#define ov_ptr_valid_warn(ptr, msg)                                            \
    ov_ptr_valid_internal(__FILE__, OV_LOG_WARNING, __FUNCTION__, __LINE__, 0, \
                          ptr, 0, msg)

/*----------------------------------------------------------------------------*/

/**
 * If cond is false, logs `msg` as debug message.
 * Returns cond
 */
#define ov_cond_valid_debug(cond, msg)                                         \
    ov_cond_valid_internal(__FILE__, OV_LOG_DEBUG, __FUNCTION__, __LINE__, 0,  \
                           cond, 0, msg)

/*----------------------------------------------------------------------------*/

/**
 * If ptr is null, prints appropriate debug message
 */
#define ov_ptr_valid_debug(ptr, msg)                                           \
    ov_ptr_valid_internal(__FILE__, OV_LOG_DEBUG, __FUNCTION__, __LINE__, 0,   \
                          ptr, 0, msg)

/*----------------------------------------------------------------------------*/

/**
 * Like ov_cond_valid, but sets result in case of error.
 */
#define ov_cond_valid_result(cond, res, code, msg)                             \
    ov_cond_valid_internal(__FILE__, OV_LOG_ERR, __FUNCTION__, __LINE__, res,  \
                           cond, code, msg)

/*----------------------------------------------------------------------------*/

/**
 * Like ov_cond_valid, but sets result in case of error.
 */
#define ov_ptr_valid_result(ptr, res, code, msg)                               \
    ov_ptr_valid_internal(__FILE__, OV_LOG_ERR, __FUNCTION__, __LINE__, res,   \
                          ptr, code, msg)

/*----------------------------------------------------------------------------*/

bool ov_utils_init_random_generator();

/*****************************************************************************
                                 POINTER ARRAYS
 ****************************************************************************/

/**
 * Search an array of pointer for pointer_to_remove and set it to 0 if found.
 * @return true if the pointer was found and set to 0, false otherwise
 */
bool ov_utils_add_to_array(void *array, size_t array_length,
                           void *pointer_to_add);

/*----------------------------------------------------------------------------*/

/**
 * Search an array of pointer for pointer_to_remove and set it to 0 if found.
 * @return true if the pointer was found and set to 0, false otherwise
 */
bool ov_utils_del_from_array(void *array, size_t array_length,
                             void *pointer_to_remove);

/*----------------------------------------------------------------------------*/

/**
 * Search an array of pointers for `pointer`.
 * @return true if `pointer` is contained in array
 */
bool ov_utils_is_in_array(void const *array, size_t array_length,
                          void const *pointer);

/******************************************************************************
 *                                 INTERNALS
 ******************************************************************************/

struct ov_result;

bool ov_cond_valid_internal(char const *file, ov_log_level loglevel,
                            char const *function, size_t line,
                            struct ov_result *result, bool condition,
                            uint64_t error_code, char const *msg);

/*----------------------------------------------------------------------------*/

bool ov_ptr_valid_internal(char const *file, ov_log_level loglevel,
                           char const *function, size_t line,
                           struct ov_result *result, void const *ptr,
                           uint64_t error_code, char const *msg);

/* Unfortunately, since we deal with macros here, all this internal stuff
 * must go into this header ... */

/*----------------------------------------------------------------------------
                                   Recent GCC
  ----------------------------------------------------------------------------*/

#if defined(__GNUC__) &&                                                       \
    (((__GNUC__ == 4) && (__GNUC_MINOR__ >= 5)) || (__GNUC__ > 4))

/* Supported since
 * [GCC 4.5.0](https://gcc.gnu.org/gcc-4.5/changes.html) */

#define DEPRECATED_INTERNAL_IMPL(message) __attribute__((deprecated(message)))

/* Earliest GCC version support unknown - 4.5.4 does it ... */
#define TEST_MISSING_INTERNAL_IMPL __attribute__((warning("UNIT Test missing")))

#define WARNING_INTERNAL_IMPL(message) __attribute__((warning(message)))

/*----------------------------------------------------------------------------
                                   Older GCC
  ----------------------------------------------------------------------------*/

#elif defined(__GNUC__)

#define TEST_MISSING_INTERNAL_IMPL

/* Unfortunately, unknown since when this has been supported... */

#define DEPRECATED_INTERNAL_IMPL(message) __attribute__((deprecated))

#define WARNING_INTERNAL_IMPL(message) __attribute__((warning(message)))

/*----------------------------------------------------------------------------
                                     CLANG
  ----------------------------------------------------------------------------*/

#elif define(__clang__)

#define TEST_MISSING_INTERNAL_IMPL __attribute__((warning("UNIT Test missing")))

#define DEPRECATED_INTERNAL_IMPL(message) __attribute__((deprecated))

#define WARNING_INTERNAL_IMPL(message) __attribute__((warning(message)))

/*----------------------------------------------------------------------------
                                    GARBAGE
  ----------------------------------------------------------------------------*/

#else

#error("Unsupported compiler")

#endif /* defined(__clang__) */

/*----------------------------------------------------------------------------*/

#ifdef DEBUG

#define EXIT_ON_DEBUG_INTERNAL abort()

#define DEPRECATED_INTERNAL(message) DEPRECATED_INTERNAL_IMPL(message)

#define TEST_MISSING_INTERNAL TEST_MISSING_INTERNAL_IMPL

#define WARNING_INTERNAL(message) WARNING_INTERNAL_IMPL(message)

#define NOT_IN_RELEASE_INTERNAL(msg)

#define PARANOID

#define TODO_INTERNAL(message)                                                 \
    fprintf(stderr, "%s:%i   TODO: %s\n", __FILE__, __LINE__, message)

#else

#define EXIT_ON_DEBUG_INTERNAL // NOTHING TO DO

#define DEPRECATED_INTERNAL(message)

#define TEST_MISSING_INTERNAL

#define WARNING_INTERNAL(message)

#define NOT_IN_RELEASE_INTERNAL(msg)                                           \
    fprintf(stderr, "%s:%i  %s\n", __FILE__, __LINE__, msg)

#define TODO_INTERNAL(message)

#endif /* DEBUG */

#ifndef TEST_MISSING_INTERNAL

#error("Not defined")

#endif

#ifdef PARANOID

/**
 * Enables to compile additional code (for debugging/performing additional
 * checks...) Use like
 *
 *     PARANOID_CODE({if(MY_MAGIC_NUMBER == object->magic_number) {
 *            fprintf(stderr, "% %" PRIu64, object->magic_number);
 *            goto error;
 *     }})
 *
 * The code will be compiled only if `PARANOID` is defined.
 * If `DEBUG` is enabled, `PARANOID` will be enabled automatically.
 *
 * You might temporarily enable by enhancing the CFLAGS passed to the compiler
 * with CFLAGS+=-DPARANOID
 * in the call to the compiler.
 * You need not modify the makefile itself for that but can set the additional
 * CFLAGS when calling make:
 *
 *     make CFLAGS=-DPARANOID
 */
#define PARANOID_CODE(code) code

#else

#define PARANOID_CODE(code)

#endif /* PARANOID */

/*----------------------------------------------------------------------------*/

#define OV_ASSERT_INTERNAL(cond)                                               \
    do {                                                                       \
        bool _our_internal_condition = cond;                                   \
        if (!_our_internal_condition) {                                        \
            ov_log_error("Condition " TO_STR(cond) " does not hold");          \
            char *_our_internal_backtrace =                                    \
                ov_arch_compile_backtrace(OV_MAX_BACKTRACE_DEPTH);             \
            if (0 != _our_internal_backtrace) {                                \
                ov_log_error("Backtrace: %s", _our_internal_backtrace);        \
                free(_our_internal_backtrace);                                 \
                _our_internal_backtrace = 0;                                   \
            } else {                                                           \
                ov_log_error("Backtrace disabled");                            \
            }                                                                  \
            EXIT_ON_DEBUG_INTERNAL;                                            \
        }                                                                      \
    } while (0)

#define __TO_STR_HELPER(x) #x

/*----------------------------------------------------------------------------*/

#endif /* ov_utils_h */
