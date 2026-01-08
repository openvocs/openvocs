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

     \file               ov_thread_lock.h
     \author             Michael J. Beer, DLR/GSOC <michael.beer@dlr.de>
     \date               2018-01-19

     \ingroup            empty

     \brief              empty


    Provides locks with wait/notify functionality.

  */
/*---------------------------------------------------------------------------*/
#ifndef ov_thread_lock_h
#define ov_thread_lock_h
/*---------------------------------------------------------------------------*/
#include <inttypes.h>
#include <pthread.h>
#include <stdbool.h>
#include <unistd.h>

/******************************************************************************
 *
 *  TYPEDEFS
 *
 ******************************************************************************/

struct ov_thread_lock_struct {

  const uint8_t lock_type;
  pthread_mutex_t mutex;
  pthread_cond_t cond;
  struct timespec timeout;
};

typedef struct ov_thread_lock_struct ov_thread_lock;

/******************************************************************************
 *
 *  FUNCTIONS
 *
 ******************************************************************************/

/**
 * Initializes a new lock
 */
bool ov_thread_lock_init(ov_thread_lock *lock, uint64_t timeout_usecs);

/**
 * Clears a lock.
 * YOU need to ENSURE that the lock is not used any further and NOT LOCKED!
 */
bool ov_thread_lock_clear(ov_thread_lock *lock);

/**
 * @return true if acquisition was successful.
 */
bool ov_thread_lock_try_lock(ov_thread_lock *lock);

/**
 * Unlocks a lock.
 */
bool ov_thread_lock_unlock(ov_thread_lock *lock);

/**
 * Waits for notification.
 * Thread will be suspended until someone calls ov_thread_lock_notify()
 * on the lock or the timeout given to ov_thread_lock_init() has expired.
 *
 * In order to perform a wait, you need to lock the lock first.
 *
 * Unless an error occurs, the lock will be locked by the caller after return.
 * The lock will not be locked if the wait returns due to timeout having been
 * surpassed.
 *
 * Upon calling wait, the lock will become available for locking again.
 * A different thread might then  lock the lock and perform an
 * an ov_thread_lock_notify().
 */
bool ov_thread_lock_wait(ov_thread_lock *lock);

/**
 * Notifies a thread waiting on this lock.
 * The lock needs not to be locked to notify.
 */
bool ov_thread_lock_notify(ov_thread_lock *lock);

/*---------------------------------------------------------------------------*/
#endif /* ov_thread_lock.h */
