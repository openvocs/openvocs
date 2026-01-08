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

        @author         Michael J. Beer

        ------------------------------------------------------------------------
*/
#ifndef OV_ARCH_PTHREAD_H
#define OV_ARCH_PTHREAD_H
/*----------------------------------------------------------------------------*/

#include "ov_arch.h"
#include <pthread.h>

/**
 * pthread_mutex_timedwait is not available on all platforms.
 * Therefore, abstract it here and provide surrogate if necessary.
 */
#define OV_ARCH_PTHREAD_MUTEX_TIMEDLOCK(m, t)                                  \
  INTERNAL_OV_ARCH_PTHREAD_MUTEX_TIMEDLOCK(m, t)

/******************************************************************************
 *                                 INTERNALS
 ******************************************************************************/

#if OV_ARCH == OV_LINUX

#define INTERNAL_OV_ARCH_PTHREAD_MUTEX_TIMEDLOCK(m, t)                         \
  pthread_mutex_timedlock(m, t)

#endif

/*----------------------------------------------------------------------------*/

#ifndef INTERNAL_OV_ARCH_PTHREAD_MUTEX_TIMEDLOCK

int ov_arch_pthread_mutex_timedlock(pthread_mutex_t *restrict mutex,
                                    const struct timespec *restrict abstime);

#define INTERNAL_OV_ARCH_PTHREAD_MUTEX_TIMEDLOCK(m, t)                         \
  ov_arch_pthread_mutex_timedlock(m, t)

#endif

/*----------------------------------------------------------------------------*/
#endif
