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
*
*\file               ov_thread_pool.c
*\author             Michael J. Beer, DLR/GSOC <michael.beer@dlr.de>
*\date               2018-01-18
*
*\ingroup            empty
*
*\brief              empty
*
*/
/*---------------------------------------------------------------------------*/

#ifdef __STDC_NO_ATOMICS__
#error("Compiler does not support C11 atomics")
#endif

#include <ov_log/ov_log.h>

#include "../../include/ov_constants.h"
#include "../../include/ov_thread_pool.h"
#include "../../include/ov_utils.h"
#include <pthread.h>
#include <sched.h>
#include <stdatomic.h>
#include <string.h>

/*---------------------------------------------------------------------------*/

static const uint32_t MAGIC_BYTES = 0x4d425472;

static const size_t MAX_NUM_THREADS = 10;

/******************************************************************************
 *
 *  TYPEDEFS
 *
 ******************************************************************************/

/**
 * States a thread might be in
 */
typedef enum {

    STOPPED,
    TO_STOP,
    STARTING,
    RUNNING

} ThreadState;

/*---------------------------------------------------------------------------*/

typedef struct {
    pthread_t id;
    ThreadState state;
    ov_thread_pool *pool;
} ThreadInfo;

/*---------------------------------------------------------------------------*/

/**
 * A processing thread - internal structure
 */
typedef struct {

    ov_thread_pool public;

    ov_thread_queue incoming;

    ThreadInfo *threads;
    _Atomic(int) pool_state;

    /*
     * Will be called whenever an element is received in incoming_queue
     * The return value will be pushed into the outgoing_queue.
     * @param additional_arg an additional pointer handed over to us upon
     * creation
     */
    ov_thread_pool_function process_func;

    ov_thread_pool_config config;

    ov_thread_pool_statistics statistics;

} internal_pool;

/******************************************************************************
 *
 *  INTERNAL MACROS
 *
 ******************************************************************************/

#define INC_COUNTER(x) ++(x)

/******************************************************************************
 *
 *  INTERNAL FUNCTIONS
 *
 ******************************************************************************/

static inline internal_pool *as_internal_pool(void *pool) {

    if ((0 == pool) || (MAGIC_BYTES != *(uint32_t *)pool))
        goto error;

    return (internal_pool *)pool;

error:

    return 0;
}

/*-*/

static bool spawn_threads(internal_pool *restrict pool);

/*---------------------------------------------------------------------------*/

static bool thread_start(ov_thread_pool *self);
static bool thread_stop(ov_thread_pool *self);
static ov_thread_pool *thread_free(ov_thread_pool *self);

static void *thread_run(void *arg);

static int try_lock_until_stop(ov_thread_lock *restrict lock,
                               _Atomic int *state,
                               uint64_t *restrict blocked_counter);

/******************************************************************************
 *
 *  FUNCTION IMPLEMENTATIONS
 *
 ******************************************************************************/

ov_thread_pool *ov_thread_pool_create(ov_thread_queue incoming,
                                      ov_thread_pool_function process_func,
                                      ov_thread_pool_config config) {

    if (!incoming.queue) {

        ov_log_error("No incoming queue given (0 pointer)");
        goto error;
    }

    if (!process_func) {

        ov_log_error("Require process function");
        goto error;
    }

    if (0 == config.num_threads) {
        config.num_threads = OV_DEFAULT_NUM_THREADS;
    }

    if (MAX_NUM_THREADS < config.num_threads) {
        config.num_threads = MAX_NUM_THREADS;
    }

    internal_pool *pool = calloc(1, sizeof(internal_pool));

    pool->public.magic_bytes = MAGIC_BYTES;
    pool->public.start = thread_start;
    pool->public.stop = thread_stop;
    pool->public.free = thread_free;
    pool->process_func = process_func;

    pool->incoming = incoming;

    pool->threads = calloc(config.num_threads, sizeof(ThreadInfo));

    for (size_t i = 0; i < config.num_threads; ++i) {
        pool->threads[i].state = STOPPED;
        pool->threads[i].pool = (ov_thread_pool *)pool;
    }

    atomic_init(&pool->pool_state, STOPPED);
    pool->config = config;

    return (ov_thread_pool *)pool;

error:

    return 0;
}

/******************************************************************************
 *
 *  INTERNAL FUNCTIONS
 *
 ******************************************************************************/

static bool spawn_threads(internal_pool *restrict pool) {

    /* Spawn the threads */
    for (size_t i = 0; i < pool->config.num_threads; ++i) {
        pool->threads[i].state = STARTING;
        int status = pthread_create(&pool->threads[i].id, 0, thread_run,
                                    &pool->threads[i]);

        if (0 != status) {

            ov_log_error("Thread could not be created/started");
            goto error;
        }
    };

    /* And wait until all threads declared themselves 'RUNNING' */

    static const struct timespec t = {
        .tv_sec = 0,
        .tv_nsec = 100 * 1000 * 1000,
    };

    bool all_running = false;

    while (!all_running) {

        all_running = true;

        for (size_t i = 0; i < pool->config.num_threads; ++i) {

            sched_yield();
            if (STOPPED == pool->threads[i].state)
                goto error;
            if (RUNNING != pool->threads[i].state) {
                nanosleep(&t, 0);
            }

            /* Wait */
            continue;
        }
    }

    ov_log_info("Spawned %zu threads", pool->config.num_threads);

    return true;

error:

    return false;
}

/*---------------------------------------------------------------------------*/

static bool thread_start(ov_thread_pool *self) {

    int pool_state = STOPPED;
    internal_pool *pool = as_internal_pool(self);

    if (!pool) {

        ov_log_error("Called with 0 pointer");
        goto error;
    }

    switch (atomic_load(&pool->pool_state)) {

    case TO_STOP:
    case RUNNING:

        ov_log_error("Thread already running - not going to "
                     "start");
        goto error;

    case STOPPED:

        atomic_store(&pool->pool_state, RUNNING);

        if (!spawn_threads(pool)) {
            ov_log_error("Could not spawn threads");
            goto error;
        }

        break;

    default:

        OV_ASSERT("! NEVER TO HAPPEN!");
    };

    return true;

error:

    pool_state = STOPPED;

    if (0 != pool) {
        pool_state = atomic_load(&pool->pool_state);
    }
    if (RUNNING == pool_state) {
        self->stop(self);
    }

    return false;
}

/*---------------------------------------------------------------------------*/

static bool thread_stop(ov_thread_pool *self) {

    if (!self)
        goto error;

    /* Simpler would be:
     * Enqueue a special pointer into incoming queue.
     * Problem: queue might try to free this pointer ...
     */
    internal_pool *internal = (internal_pool *)self;
    int old_state = atomic_load(&internal->pool_state);

    if (RUNNING != old_state)
        goto error;

    atomic_store(&internal->pool_state, TO_STOP);

    void *result = 0;

    for (size_t i = 0; i < internal->config.num_threads; ++i) {

        int status = pthread_join(internal->threads[i].id, &result);

        if ((0 != status) || (STOPPED != internal->threads[i].state)) {

            ov_log_error("Could not stop thread: %s", strerror(status));

            atomic_store(&internal->pool_state, old_state);
            goto error;
        }
    }

    atomic_store(&internal->pool_state, STOPPED);

    return true;

error:

    return false;
}

/*---------------------------------------------------------------------------*/

static ov_thread_pool *thread_free(ov_thread_pool *self) {

    if (!self)
        goto error;

    internal_pool *internal = (internal_pool *)self;

    if ((RUNNING == atomic_load(&internal->pool_state)) &&
        (!self->stop(self))) {

        ov_log_error("Could not free thread pool - "
                     " still running");
        goto error;
    }

    if (STOPPED != atomic_load(&internal->pool_state)) {
        ov_log_error("Could not stop thread pool");
        goto error;
    }

    free(internal->threads);
    free(self);

    return 0;

error:

    return self;
}

/*---------------------------------------------------------------------------*/

static void *thread_run(void *arg) {

    OV_ASSERT(arg);

    ThreadInfo *thread = (ThreadInfo *)arg;

    if (!thread) {

        ov_log_error("Did not receive a proper thread object (0 "
                     "pointer)");
        goto error;
    }

    internal_pool *pool = as_internal_pool(thread->pool);

    if (!pool) {

        ov_log_error("Did not receive a proper pool object (0 "
                     "pointer)");
        goto error;
    }

    ov_ringbuffer *in = pool->incoming.queue;

    if (!in) {

        ov_log_error("No incoming queue");
        goto error;
    }

    bool (*process)(void *, void *) = pool->process_func;

    if (!process) {

        ov_log_error("No processing function set(0 pointer)");
        goto error;
    }

    ov_thread_lock *in_lock = pool->incoming.lock;

    thread->state = RUNNING;

    if (!try_lock_until_stop(in_lock, &pool->pool_state,
                             &pool->statistics.lock_blocked.incoming)) {

        goto finish;
    }

    /* in_lock:locked */

    /*-----------------------------------------------------------------------
     * MAIN LOOP
     *-----------------------------------------------------------------------*/

    while (RUNNING == atomic_load(&pool->pool_state)) {

        /* in_lock:locked */
        /* element == 0 */

        /* If in_lock is there, its locked */
        void *element = in->pop(in);

        while (in_lock && (!element)) {

            /* in_lock:locked */
            /* element == 0 */

            ov_thread_lock_wait(in_lock);

            if (RUNNING != atomic_load(&pool->pool_state)) {

                ov_thread_lock_unlock(in_lock);

                /* in_lock:released */
                /* element == 0 */

                goto finish;
            }

            element = in->pop(in);
        }

        /* in_lock:locked */

        if (!element)
            continue;

        /* in_lock:locked */
        /* element != 0 */

        if (in_lock)
            ov_thread_lock_unlock(in_lock);

        /* in_lock:released */
        /* element != 0 */

        INC_COUNTER(pool->statistics.elements.received);

        if (process(pool->config.userdata, element)) {

            INC_COUNTER(pool->statistics.elements.processed);
        }

        /* in_lock:released */
        /* element == 0 */

        if (!try_lock_until_stop(in_lock, &pool->pool_state,
                                 &pool->statistics.lock_blocked.incoming)) {

            goto finish;
        }

        /* in_lock:locked */
        /* element == 0 */
    };

    /* in_lock:locked */

    if (in_lock) {

        ov_thread_lock_unlock(in_lock);
    }

finish:
error:

    /* in_lock:released */
    thread->state = STOPPED;

    return 0;
}

/*---------------------------------------------------------------------------*/

static int try_lock_until_stop(ov_thread_lock *restrict lock,
                               _Atomic int *state,
                               uint64_t *restrict blocked_counter) {

    if (lock) {

        /* Lock down on the lock and wait for notification ... */
        while (!ov_thread_lock_try_lock(lock)) {

            ov_log_warning("Could not lock down on incoming queue");

            INC_COUNTER(blocked_counter);

            if (RUNNING != atomic_load(state)) {
                return false;
            }
        }
    }

    return true;
}

/*---------------------------------------------------------------------------*/
