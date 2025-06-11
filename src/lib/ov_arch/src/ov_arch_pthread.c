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

        @author         Markus Töpfer
        @author         Michael J. Beer

        ------------------------------------------------------------------------
*/

#include <pthread.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/*----------------------------------------------------------------------------*/

int ov_arch_pthread_mutex_timedlock(pthread_mutex_t *restrict mutex,
                                    const struct timespec *restrict abstime) {

    int retval = -1;

    if ((0 == mutex) || (0 == abstime)) {

        return -1;
    }
    /*
     *
     *      pthread_mutex_try_lock is POSIX 1 ISO/IEC 9945-1:1996
     *
     */

    struct timespec now = {0};

    do {

        memset(&now, 0, sizeof(struct timespec));
        clock_gettime(CLOCK_REALTIME, &now);

        retval = pthread_mutex_trylock(mutex);
        if (0 == retval) break;

        usleep(1000); // 1 millisecond

        if (now.tv_sec == abstime->tv_sec)
            if (now.tv_nsec >= abstime->tv_nsec) return retval;

    } while (now.tv_sec <= abstime->tv_sec);

    return retval;
}

/*----------------------------------------------------------------------------*/
