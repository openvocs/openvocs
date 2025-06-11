/***

Copyright   2018        German Aerospace Center DLR e.V.,
                        German Space Operations Center (GSOC)

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

***/ /**

     \file               ov_thread_lock.c
     \author             Michael J. Beer, DLR/GSOC <michael.beer@dlr.de>
     \date               2018-01-19

     \ingroup            empty

     \brief              empty

  */
/*---------------------------------------------------------------------------*/

#include <ov_arch/ov_arch_pthread.h>
#include <ov_log/ov_log.h>

#include "../../include/ov_thread_lock.h"
#include <string.h>

/*****************************************************************************
 * INTERNAL MACROS
 *****************************************************************************/

#define CALC_ABS_TIMEOUT(target, relative)                                     \
    do {                                                                       \
        if (0 != clock_gettime(CLOCK_REALTIME, &target)) {                     \
            goto error;                                                        \
        }                                                                      \
        uint64_t carry_secs = 0;                                               \
        uint64_t nsecs = target.tv_nsec + relative.tv_nsec;                    \
        if (nsecs > 1000 * 1000 * 1000) {                                      \
            carry_secs = nsecs / 1000 / 1000 / 1000;                           \
            nsecs -= carry_secs * 1000 * 1000 * 1000;                          \
        }                                                                      \
        target.tv_sec += relative.tv_sec + carry_secs;                         \
        target.tv_nsec = nsecs;                                                \
    } while (0)

/*---------------------------------------------------------------------------*/

bool ov_thread_lock_init(ov_thread_lock *lock, uint64_t timeout_usecs) {

    if (!lock) {

        ov_log_error("Got 0 pointer");
        goto error;
    }

    if (0 == timeout_usecs) {

        ov_log_error(
            "Refusing to create lock with a default timeout "
            "of 0");
        goto error;
    }

    if (0 != pthread_mutex_init(&lock->mutex, 0)) {

        ov_log_error("Could not initialize mutex");
        goto error;
    }

    if (0 != pthread_cond_init(&lock->cond, 0)) {

        ov_log_error("Could not initialize condition variable");
        pthread_mutex_destroy(&lock->mutex);
        goto error;
    }

    time_t secs = timeout_usecs / 1000 / 1000;
    lock->timeout.tv_sec = secs;
    lock->timeout.tv_nsec = (timeout_usecs - (secs * 1000 * 1000)) * 1000;

    return true;

error:

    return false;
}

/*---------------------------------------------------------------------------*/

bool ov_thread_lock_clear(ov_thread_lock *lock) {

    if (!lock) {

        ov_log_error("Called with 0 pointer");
        goto error;
    }

    if (0 != pthread_cond_destroy(&lock->cond)) {

        ov_log_error("Could not destroy condition");
        goto error;
    }

    if (0 != pthread_mutex_destroy(&lock->mutex)) {

        /* Try not to leave the lock in an undefined state ... */
        ov_log_error("Could not destroy mutex");
        pthread_cond_init(&lock->cond, 0);
        goto error;
    }

    return true;

error:

    return false;
}

/*---------------------------------------------------------------------------*/

bool ov_thread_lock_try_lock(ov_thread_lock *lock) {

    struct timespec abs_timeout = {0};

    if (!lock) {

        ov_log_error("Called with 0 pointer");
        goto error;
    }

    struct timespec rel_timeout = lock->timeout;
    CALC_ABS_TIMEOUT(abs_timeout, rel_timeout);

    int retval = 0;

    retval = OV_ARCH_PTHREAD_MUTEX_TIMEDLOCK(&lock->mutex, &abs_timeout);

    if (0 != retval) {

        ov_log_warning("Could not acquire lock: %s", strerror(retval));
        goto error;
    }

    return true;

error:

    return false;
}

/*---------------------------------------------------------------------------*/

bool ov_thread_lock_unlock(ov_thread_lock *lock) {

    if (!lock) {

        ov_log_error("Called with 0 pointer");
        goto error;
    }

    if (0 != pthread_mutex_unlock(&lock->mutex)) {

        ov_log_error("Could not unlock mutex");
        goto error;
    }

    return true;

error:

    return false;
}

/*---------------------------------------------------------------------------*/

bool ov_thread_lock_wait(ov_thread_lock *lock) {

    if (!lock) {

        ov_log_error("Called with 0 pointer");
        goto error;
    }

    /* Assert: lock->mutex locked! */

    struct timespec rel_timeout = lock->timeout;
    struct timespec abs_timeout = {0};
    CALC_ABS_TIMEOUT(abs_timeout, rel_timeout);

    int retval =
        pthread_cond_timedwait(&lock->cond, &lock->mutex, &abs_timeout);

    if (0 != retval) {

        goto error;
    }

    /* Assert: lock->mutex locked */

    return true;

error:

    /* Assert: lock->mutex not locked */

    return false;
}

/*---------------------------------------------------------------------------*/

bool ov_thread_lock_notify(ov_thread_lock *lock) {

    if (!lock) {

        ov_log_error("Called with 0 pointer");
        goto error;
    }

    int retval = pthread_cond_signal(&lock->cond);

    if (0 != retval) {

        ov_log_error("Could not notify: %s", strerror(retval));
        goto error;
    }

    return true;

error:

    return false;
}

/*---------------------------------------------------------------------------*/
