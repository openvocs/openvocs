/***
        ------------------------------------------------------------------------

        Copyright (c) 2018 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_event_loop_linux.c
        @author         Michael Beer

        @date           2019-09-04

        @ingroup        ov_event_loop_linux

        @brief          Linux version of ov_event_loop_poll

        ------------------------------------------------------------------------
*/

#if defined __linux__

#include "../include/ov_event_loop_linux.h"

_Static_assert(OV_TIMER_INVALID == 0, "OV_TIMER_INVALID is a valid timer id");

#include <limits.h>
#include <ov_base/ov_dict.h>
#include <ov_base/ov_time.h>

#include <ov_base/ov_utils.h>
#include <time.h>

/* Thats the linux specific part ... */
#include <sys/epoll.h>
#include <sys/timerfd.h>

/*-----------------------------------*/

static const uint16_t IMPL_POLL_LOOP_TYPE = 0xf001;

static const uint8_t WAKEUP_SIGNAL = (uint8_t)'w';

#define MAX_NO_READY_EVENTS 1
const uint64_t TIMER_NAGGING_INTERVAL_USEC = 50 * 1000;

/*---------------------------------------------------------------------------*/

struct callback {

    int fd;
    bool (*callback)(int socket_fd, uint8_t events, void *data);
    void *data;
};

/*----------------------------------------------------------------------------*/

struct timer_cache_entry {
    int fd;
    void *tdata;
};

/*----------------------------------------------------------------------------*/

typedef struct {

    ov_event_loop public;
    ov_event_loop_config config;

    bool running;

    int wakeup_fd;

    int epoll_fd;

    // We assume that the fd numbers are allocated in order, starting
    // from 0, thus there are at most max_sockets fds + 0,1 and 2
    // and we can use all of them to safely index into this array...
    struct callback *callbacks;

    size_t max_callbacks;

    struct {
        struct timer_cache_entry *timers;
        size_t current;
    } timers_available;

} Loop;

/*----------------------------------------------------------------------------*/

static char *append_str(char *restrict write_ptr, char const *restrict str) {

    OV_ASSERT(0 != write_ptr);
    OV_ASSERT(0 != str);

    if (0 == *str)
        return write_ptr;

    for (; 0 != *str; ++str, ++write_ptr) {

        *write_ptr = *str;
    }

    ++write_ptr;
    *write_ptr = 0;

    return write_ptr;
}

/*----------------------------------------------------------------------------*/

static char *epoll_event_to_str(uint32_t events, char *out, size_t out_size) {

    char *end_ptr = out + out_size;

    if (0 != (events & EPOLLIN)) {
        out = append_str(out, "IN");
    }
    if (0 != (events & EPOLLPRI)) {
        out = append_str(out, "PRI");
    }
    if (0 != (events & EPOLLOUT)) {
        out = append_str(out, "OUT");
    }
    if (0 != (events & EPOLLRDNORM)) {
        out = append_str(out, "RDNORM");
    }
    if (0 != (events & EPOLLRDBAND)) {
        out = append_str(out, "RDBAND");
    }
    if (0 != (events & EPOLLWRNORM)) {
        out = append_str(out, "WRNORM");
    }
    if (0 != (events & EPOLLWRBAND)) {
        out = append_str(out, "WRBAND");
    }
    if (0 != (events & EPOLLMSG)) {
        out = append_str(out, "MSG");
    }
    if (0 != (events & EPOLLERR)) {
        out = append_str(out, "ERR");
    }
    if (0 != (events & EPOLLHUP)) {
        out = append_str(out, "HUP");
    }
    if (0 != (events & EPOLLRDHUP)) {
        out = append_str(out, "RDHUP");
    }
    if (0 != (events & EPOLLEXCLUSIVE)) {
        out = append_str(out, "EXCLUSIVE");
    }
    if (0 != (events & EPOLLWAKEUP)) {
        out = append_str(out, "WAKEUP");
    }
    if (0 != (events & EPOLLONESHOT)) {
        out = append_str(out, "ONESHOT");
    }
    if (0 != (events & EPOLLET)) {
        out = append_str(out, "ET");
    }

    OV_ASSERT(out < end_ptr);

    out[out_size - 1] = 0;

    return out;
}

/*----------------------------------------------------------------------------*/

static void log_epoll_event(int log_fd, struct epoll_event *event) {

    OV_ASSERT(0 != event);

    if (0 >= log_fd)
        return;

    int fd = event->data.fd;

    char str_events[255] = {0};

    epoll_event_to_str(event->events, str_events, sizeof(str_events));

    int np = dprintf(log_fd, "    ep-evt: (%i) events: %s\n", fd, str_events);

    if (np <= 0) {

        fprintf(stderr, "Could not write to log fd: %s  %s\n", strerror(np),
                strerror(errno));
    }
}

/*----------------------------------------------------------------------------*/

static uint32_t to_epoll_flags(uint8_t ov_flag) {

    uint32_t events = 0;

    if (ov_flag & OV_EVENT_IO_IN) {
        events |= EPOLLIN;
        events |= EPOLLPRI;
    }

    if (ov_flag & OV_EVENT_IO_OUT) {
        events |= EPOLLOUT;
    }

    if (ov_flag & OV_EVENT_IO_CLOSE) {
        events |= EPOLLHUP;
    }

    if (ov_flag & OV_EVENT_IO_ERR) {
        events |= EPOLLERR;
    }

    return events;
}

/******************************************************************************
 *                                 CALLBACKS
 ******************************************************************************/

ov_event_loop *impl_event_loop_linux_free(ov_event_loop *self);

bool impl_event_loop_linux_is_running(const ov_event_loop *self);

bool impl_event_loop_linux_stop(ov_event_loop *self);

bool impl_event_loop_linux_run(ov_event_loop *self, uint64_t max);

bool impl_event_loop_linux_callback_set(
    ov_event_loop *self, int socket, uint8_t events, void *data,
    bool (*callback)(int socket_fd, uint8_t events, void *data));

bool impl_event_loop_linux_callback_unset(ov_event_loop *self, int socket,
                                          void **userdata);

uint32_t impl_event_loop_linux_timer_set(ov_event_loop *self,
                                         uint64_t relative_usec, void *data,
                                         bool (*callback)(uint32_t id,
                                                          void *data));

bool impl_event_loop_linux_timer_unset(ov_event_loop *self, uint32_t id,
                                       void **userdata);

/*----------------------------------------------------------------------------*/

static Loop *cast_to_loop(const void *x) {

    if (0 == x)
        return 0;
    if (0 == ov_event_loop_cast(x))
        return 0;
    if (IMPL_POLL_LOOP_TYPE != ((ov_event_loop *)x)->type)
        return 0;

    return (Loop *)x;
}

/*----------------------------------------------------------------------------*/

static bool wakeup_unsafe(Loop *restrict loop) {

    OV_ASSERT(0 != loop);

    if (0 == loop->callbacks)
        return false;

    int wakeup_fd = loop->wakeup_fd;

    OV_ASSERT(-1 < wakeup_fd);

    return sizeof(WAKEUP_SIGNAL) ==
           send(wakeup_fd, &WAKEUP_SIGNAL, sizeof(WAKEUP_SIGNAL), 0);
}

/*----------------------------------------------------------------------------*/

static bool read_and_drop(int fd, uint8_t events, void *data) {

    UNUSED(events);
    UNUSED(data);

    char buf[100];
    return -1 < read(fd, buf, sizeof(buf) / sizeof(buf[0]));
}

/******************************************************************************
 *                            CALLBACK REGISTERING
 ******************************************************************************/

/**
 * Searches callback array for fd and returns hit, or empty callback.
 * You have to check callback->event.data.fd!
 */
static struct callback *get_callback_for_fd_unsafe(Loop *restrict loop,
                                                   int fd) {

    OV_ASSERT(0 != loop);

    if (0 > fd)
        return 0;

    /* loop->callbacks array is organized as a simple hash table */

    size_t max_callbacks = loop->max_callbacks;

    size_t start_index = fd % max_callbacks;

    struct callback *cb = loop->callbacks + start_index;

    if ((fd == cb->fd) || (0 > cb->fd))
        return cb;

    for (size_t i = start_index + 1; i < max_callbacks; ++i) {

        cb = loop->callbacks + i;
        if ((fd == cb->fd) || (0 > cb->fd))
            return cb;
    }

    for (size_t i = 0; i < 1 + start_index; ++i) {

        cb = loop->callbacks + i;
        if ((fd == cb->fd) || (0 > cb->fd))
            return cb;
    }

    return 0;
}

/*----------------------------------------------------------------------------*/

static bool release_fd_unsafe(Loop *loop, int fd, void **attached_data) {

    OV_ASSERT(0 != loop);

    if (0 > fd)
        return false;

    if (-1 < loop->epoll_fd)
        epoll_ctl(loop->epoll_fd, EPOLL_CTL_DEL, fd, 0);

    struct callback *cb = get_callback_for_fd_unsafe(loop, fd);

    void *data = 0;

    if ((0 == cb) || (fd != cb->fd)) {
        return true;
    }

    data = cb->data;
    memset(cb, 0, sizeof(struct callback));
    cb->fd = -1;

    if (0 != attached_data) {
        *attached_data = data;
    }

    return true;
}

/*----------------------------------------------------------------------------*/

static bool register_fd_with_epoll(
    Loop *loop, int fd, uint32_t events, void *data,
    bool (*callback)(int socket_fd, uint8_t events, void *data)) {
    bool callback_has_been_set = false;

    struct epoll_event ev = {0};

    ev.events = events;
    ev.data.fd = fd;

    struct callback *cb = 0;

    if (0 == loop) {
        goto error;
    }

    if (0 == events) {
        ov_log_error("FAILURE callback listening "
                     "without events "
                     "is NOT SUPPORTED.");
        goto error;
    }

    if (0 == callback) {
        ov_log_error("Wont register without callback");
        goto error;
    }

    if (0 > loop->epoll_fd) {
        ov_log_error("Loop does not seem to run? epoll fd not "
                     "set");
        goto error;
    }

    cb = get_callback_for_fd_unsafe(loop, fd);

    if (0 == cb) {
        ov_log_error("Could not register socket %i on loop", fd);
        goto error;
    }

    OV_ASSERT(0 != cb);
    OV_ASSERT((0 > cb->fd) || (fd == cb->fd));

    if (-1 < cb->fd) {
        release_fd_unsafe(loop, fd, 0);
    }

    cb->fd = fd;
    cb->data = data;
    cb->callback = callback;

    callback_has_been_set = true;

    if (epoll_ctl(loop->epoll_fd, EPOLL_CTL_ADD, fd, &ev) == -1) {

        ov_log_error("Could not add new fd to epoll: %i", fd);
        goto error;
    }

    return true;

error:

    if (callback_has_been_set) {
        release_fd_unsafe(loop, fd, 0);
    }

    return false;
}

/******************************************************************************
 *                               TIMER HANDLING
 ******************************************************************************/

struct timer_data {
    uint32_t magic_bytes;
    void *data;
    Loop *loop;
    bool (*callback)(uint32_t id, void *data);
};

#define TIMER_DATA_MAGIC_BYTES 0x54696d65

/*----------------------------------------------------------------------------*/

static struct timer_data *cast_to_timer_data(void *data) {

    if (0 == data)
        return 0;

    struct timer_data *tdata = data;

    if (TIMER_DATA_MAGIC_BYTES != tdata->magic_bytes)
        return 0;

    return tdata;
}

/*----------------------------------------------------------------------------*/

static bool cache_timer_fd_unsafe(Loop *loop, int fd,
                                  struct timer_data *tdata) {

    OV_ASSERT(0 != loop);
    OV_ASSERT(0 != tdata);
    OV_ASSERT(-1 < fd);

    OV_ASSERT(0 != loop->timers_available.timers);

    size_t current = loop->timers_available.current;

    if (!(current < loop->config.max.timers)) {
        return false;
    }

    loop->timers_available.timers[current].fd = fd;
    loop->timers_available.timers[current].tdata = tdata;

    loop->timers_available.current += 1;

    tdata->data = 0;

    return true;
}

/*----------------------------------------------------------------------------*/

static bool release_timer_fd_unsafe(Loop *loop, int fd, void **userdata) {

    OV_ASSERT(0 != loop);

    if (OV_TIMER_INVALID == fd)
        return false;

    struct callback *cb = get_callback_for_fd_unsafe(loop, fd);

    if ((0 == cb) || (0 > cb->fd)) {
        /* timer was not registered - that-s fine */
        return true;
    }

    if (fd != cb->fd) {
        ov_log_error("Tried to unregister non-existent timer %i ", fd);
        return false;
    }

    OV_ASSERT(fd == cb->fd);

    struct timer_data *tdata = cast_to_timer_data(cb->data);
    if (0 == tdata) {

        ov_log_error("Tried to unregister fd %i as timer, which is no timer",
                     fd);

        return false;
    }

    struct itimerspec none = {0};

    if (0 != timerfd_settime(fd, 0, &none, 0)) {
        ov_log_warning("Could not disarm timer %i", fd);
    }

    void *vdata = 0;

    release_fd_unsafe(loop, fd, &vdata);

    OV_ASSERT(vdata == tdata);

    if (0 != userdata) {
        *userdata = tdata->data;
    }

    if (!cache_timer_fd_unsafe(loop, fd, tdata)) {

        close(fd);
        free(tdata);

        return true;
    }

    return true;
}

/*----------------------------------------------------------------------------*/

static bool callback_for_timer(int fd, uint8_t events, void *data) {

    UNUSED(events);
    bool retval = false;
    Loop *loop = 0;

    if (0 > fd) {
        ov_log_error("Invalid fd");
        return false;
    }

    struct timer_data *tdata = cast_to_timer_data(data);

    if (0 == tdata) {
        ov_log_error("Timer callback did not receive valid timer data!");
        return false;
    }

    loop = tdata->loop;

    if (0 == loop) {
        ov_log_error("Expected event loop as argument");
        close(fd);
        return false;
    }

    OV_ASSERT(0 != loop);

    /* Check whether this timer was actually triggered */
    uint64_t expired_count = 0;

    if (8 != read(fd, &expired_count, sizeof(expired_count))) {
        ov_log_error("Could not read expiration counter for timer %i", fd);
        goto error;
    }

    void *userdata = 0;
    bool (*callback)(uint32_t id, void *data) = tdata->callback;

    release_timer_fd_unsafe(loop, fd, &userdata);
    tdata = 0;

    if (expired_count != 1) {

        ov_log_warning("Timer %i expired %" PRIu64 " times before callback was "
                       "issued",
                       fd, expired_count);
    }

    if (0 != callback) {
        retval = callback(fd, userdata);
    }

    return retval;

error:
    release_timer_fd_unsafe(loop, fd, 0);

    return retval;
}

/*----------------------------------------------------------------------------*/

static bool acquire_timer_data_unsafe(Loop *loop, int *fd,
                                      struct timer_data **tdata) {

    OV_ASSERT(0 != loop);
    OV_ASSERT(0 != loop->timers_available.timers);
    OV_ASSERT(0 != fd);
    OV_ASSERT(0 != tdata);

    int tid = -1;

    if (0 > loop->epoll_fd) {
        ov_log_error("Loop does not seem to run? epoll fd not set");
        goto error;
    }

    size_t current_cache_index = loop->timers_available.current;
    if (0 < current_cache_index) {

        loop->timers_available.current -= 1;

        --current_cache_index;

        tid = loop->timers_available.timers[current_cache_index].fd;
        *tdata = loop->timers_available.timers[current_cache_index].tdata;
    }

    if (0 > tid)
        tid = timerfd_create(OV_CLOCK_ID, TFD_NONBLOCK | TFD_CLOEXEC);

    OV_ASSERT(OV_TIMER_INVALID != tid);

    *fd = tid;

    if (*tdata == 0) {
        *tdata = calloc(1, sizeof(struct timer_data));
    }

    OV_ASSERT(OV_TIMER_INVALID != tid);

    return true;

error:

    if (-1 < tid)
        close(tid);

    return false;
}

/******************************************************************************
 *                           I/O CALLBACK EXECUTION
 ******************************************************************************/

static bool trigger_callback(Loop *loop, struct epoll_event *ep_event) {

    OV_ASSERT(0 != loop);

    bool closed = false;

    if (0 == ep_event)
        return false;

    int fd = ep_event->data.fd;

    struct callback *cb = get_callback_for_fd_unsafe(loop, fd);

    if ((0 == cb) || (0 > cb->fd)) {
        ov_log_error("No callback for fd %i", fd);
        return false;
    }

    if (0 == cb->callback) {
        ov_log_error("Invalid callback: Function pointer 0");
        goto error;
    }

    uint64_t events = 0;

    if ((ep_event->events & EPOLLIN) || (ep_event->events & EPOLLPRI)) {

        events |= OV_EVENT_IO_IN;
    }

    if (ep_event->events & EPOLLOUT) {

        events |= OV_EVENT_IO_OUT;
    }

    if (ep_event->events & EPOLLHUP) {

        events |= OV_EVENT_IO_CLOSE;
        closed = true;
    }

    if (ep_event->events & EPOLLERR) {

        events |= OV_EVENT_IO_ERR;
    }

    if (events == 0) {
        goto error;
    }

    bool retval = cb->callback(fd, events, cb->data);

    if (closed) {

        release_fd_unsafe(loop, fd, 0);
    }

    return retval;

error:

    return false;
}

/*----------------------------------------------------------------------------*/

static int get_rel_timeout_msecs(uint64_t start_usec,
                                 uint64_t rel_timeout_usecs) {

    if (rel_timeout_usecs == OV_RUN_ONCE)
        return 0;

    uint64_t now_usec = ov_time_get_current_time_usecs();

    if (now_usec - start_usec >= rel_timeout_usecs)
        return 0;

    uint64_t remaining_usec = rel_timeout_usecs - (now_usec - start_usec);

    uint64_t remaining_msec = remaining_usec / 1000;

    if (remaining_msec > INT_MAX)
        remaining_msec = INT_MAX;

    return (int)remaining_msec;
}

/*---------------------------------------------------------------------------*/

static bool loop_init(Loop *loop, ov_event_loop_config config) {

    struct callback *callbacks = 0;
    int epoll_fd = -1;

    if (!loop)
        goto error;

    if (!memset(loop, 0, sizeof(Loop)))
        goto error;

    if (!ov_event_loop_set_type(&loop->public, IMPL_POLL_LOOP_TYPE))
        goto error;

    config = ov_event_loop_config_adapt_to_runtime(config);

    if (config.max.sockets < OV_EVENT_LOOP_SOCKETS_MIN) {
        ov_log_error("Eventloop config with less then minimum "
                     "sockets %i",
                     OV_EVENT_LOOP_SOCKETS_MIN);
        goto error;
    }

    loop->config = config;

    loop->max_callbacks = 2;
    loop->max_callbacks += loop->config.max.sockets;
    loop->max_callbacks += loop->config.max.timers;

    loop->callbacks = calloc(loop->max_callbacks, sizeof(struct callback));

    size_t num_timers = loop->config.max.timers;

    loop->timers_available.timers =
        calloc(num_timers, sizeof(struct timer_cache_entry));
    loop->timers_available.current = 0;

    if (0 == loop->callbacks) {
        ov_log_error("Failed to allocate bytes for callbacks.");
        goto error;
    }

    loop->epoll_fd = -1;

    for (size_t i = 0; i < loop->max_callbacks; i++) {
        loop->callbacks[i].fd = -1;
    }

    loop->running = false;

    loop->public.free = impl_event_loop_linux_free;

    loop->public.is_running = impl_event_loop_linux_is_running;
    loop->public.stop = impl_event_loop_linux_stop;
    loop->public.run = impl_event_loop_linux_run;

    loop->public.callback.set = impl_event_loop_linux_callback_set;
    loop->public.callback.unset = impl_event_loop_linux_callback_unset;

    loop->public.timer.set = impl_event_loop_linux_timer_set;
    loop->public.timer.unset = impl_event_loop_linux_timer_unset;

    loop->epoll_fd = epoll_create1(EPOLL_CLOEXEC);

    if (0 > loop->epoll_fd) {
        ov_log_error("Could not create epoll fd");
        goto error;
    }

    /*
     *      Use a wakeup socket pair to ensure a poll wakeup.
     *
     *      loop->wakeup is only used to send dummy data to
     *      wakeup the poll loop.
     *
     */

    int wakeup_fds[2] = {0};

    if (0 != socketpair(AF_LOCAL, SOCK_STREAM, 0, wakeup_fds))
        goto error;

    int fd0 = wakeup_fds[0];
    int fd1 = wakeup_fds[1];

    if (!impl_event_loop_linux_callback_set((ov_event_loop *)loop, fd0, EPOLLIN,
                                            0, read_and_drop)) {
        ov_log_error("Could not register wakeup fd");
        goto error;
    }

    if (!impl_event_loop_linux_callback_set((ov_event_loop *)loop, fd1, EPOLLIN,
                                            0, read_and_drop)) {
        ov_log_error("Could not register wakeup fd");
        goto error;
    }

    loop->wakeup_fd = fd0;

    loop->public.log_fd = -1;

    return true;

error:

    epoll_fd = -1;

    if (0 != loop) {
        callbacks = loop->callbacks;
        free(loop);
        loop = 0;
        epoll_fd = loop->epoll_fd;
    }

    OV_ASSERT(0 == loop);

    if (0 != callbacks) {
        free(callbacks);
        callbacks = 0;
    }

    callbacks = 0;

    if (-1 < epoll_fd)
        close(epoll_fd);

    return false;
}

/*---------------------------------------------------------------------------*/

ov_event_loop *ov_event_loop_linux(ov_event_loop_config config) {

    Loop *loop = 0;

    config = ov_event_loop_config_adapt_to_runtime(config);

    if ((config.max.sockets < 1) || (config.max.timers < 1)) {
        ov_log_error("Eventloop config not supported.");
        goto error;
    }

    loop = calloc(1, sizeof(Loop));

    if (0 == loop)
        goto error;

    if (!loop_init(loop, config))
        goto error;

    return (ov_event_loop *)loop;

error:

    if (0 != loop)
        free(loop);

    return 0;
}

/******************************************************************************
 *                                    FREE
 ******************************************************************************/

static void close_all_sockets(Loop *loop) {

    OV_ASSERT(0 != loop);

    /* Close cached timers */

    for (size_t i = 0; i < loop->timers_available.current; ++i) {

        int fd = loop->timers_available.timers[i].fd;
        void *tdata = loop->timers_available.timers[i].tdata;

        if (-1 < fd)
            close(fd);

        if (0 != tdata)
            free(tdata);
    }

    /* And prevent caching again */
    loop->timers_available.current = loop->config.max.timers;

    for (size_t i = 0; i < loop->max_callbacks; ++i) {

        int fd = loop->callbacks[i].fd;

        if (callback_for_timer == loop->callbacks[i].callback) {

            release_timer_fd_unsafe(loop, fd, 0);
            continue;
        }

        if (release_fd_unsafe(loop, fd, 0)) {
            close(fd);
        }
    }
}

/*---------------------------------------------------------------------------*/

ov_event_loop *impl_event_loop_linux_free(ov_event_loop *self) {

    Loop *loop = cast_to_loop(self);
    if (!loop)
        goto error;

    self = 0;

    loop->running = false;

    if (0 != loop->callbacks) {

        close_all_sockets(loop);
        free(loop->callbacks);
        loop->callbacks = 0;
    }

    if (-1 < loop->epoll_fd) {
        close(loop->epoll_fd);
        loop->epoll_fd = -1;
    }

    if (0 != loop->timers_available.timers) {

        free(loop->timers_available.timers);
    }

    OV_ASSERT(0 > loop->epoll_fd);

    free(loop);

    loop = 0;

    return 0;

error:
    return self;
}

/******************************************************************************
 *                                RUN CONTROL
 ******************************************************************************/

bool impl_event_loop_linux_is_running(const ov_event_loop *self) {

    Loop *loop = cast_to_loop(self);
    if (!loop)
        return false;

    return loop->running;
}

/*----------------------------------------------------------------------------*/

bool impl_event_loop_linux_stop(ov_event_loop *self) {

    Loop *loop = cast_to_loop(self);
    if (!loop)
        return false;

    loop->running = false;

    return wakeup_unsafe(loop);
}

/*---------------------------------------------------------------------------*/

bool impl_event_loop_linux_run(ov_event_loop *self, uint64_t max_usecs) {

    struct epoll_event ready_events[MAX_NO_READY_EVENTS] = {0};

    Loop *loop = cast_to_loop(self);

    if (0 == loop)
        goto error;

    if (1000 > max_usecs)
        max_usecs = 1000;

    int epfs = loop->epoll_fd;

    if (0 > epfs) {
        ov_log_error("epoll not initialized properly");
        goto error;
    }

    loop->running = true;

    if (max_usecs == OV_RUN_ONCE) {
        loop->running = false;
        max_usecs = 0;
    }

    uint64_t start_usecs = ov_time_get_current_time_usecs();

    do {

        int timeout_msecs = get_rel_timeout_msecs(start_usecs, max_usecs);

        if (0 == timeout_msecs) {
            loop->running = false;
            break;
        }

        int num_ready =
            epoll_wait(epfs, ready_events, MAX_NO_READY_EVENTS, timeout_msecs);

        if (0 > num_ready) {
            ov_log_error("I/O error occured: %s\n", strerror(errno));
            goto error;
        }

        if (0 < loop->public.log_fd) {
            dprintf(loop->public.log_fd, "epoll: Ready events: %i\n",
                    num_ready);
        }

        for (size_t r = 0; r < (size_t)num_ready; ++r) {
            log_epoll_event(loop->public.log_fd, &ready_events[r]);
            trigger_callback(loop, &ready_events[r]);
        }

    } while (loop->running);

    return true;

error:

    return false;
}

/******************************************************************************
 *                             CALLBACK HANDLING
 ******************************************************************************/

bool impl_event_loop_linux_callback_set(
    ov_event_loop *self, int fd, uint8_t events, void *data,
    bool (*callback)(int socket_fd, uint8_t events, void *data)) {

    Loop *loop = cast_to_loop(self);

    if (0 == loop)
        goto error;

    int so_opt = 0;
    socklen_t so_len = sizeof(so_opt);

    if (0 != getsockopt(fd, SOL_SOCKET, SO_ERROR, &so_opt, &so_len)) {
        ov_log_error("FAILURE callback listening on socket with error "
                     "is NOT SUPPORTED.");
        goto error;
    }

    if (!ov_socket_ensure_nonblocking(fd)) {
        close(fd);
        goto error;
    }

    uint32_t epoll_events = to_epoll_flags(events);

    if (0 == epoll_events)
        goto error;

    return register_fd_with_epoll(loop, fd, epoll_events, data, callback);

error:

    return false;
}

/*------------------------------------------------------------------*/

bool impl_event_loop_linux_callback_unset(ov_event_loop *self, int fd,
                                          void **userdata) {

    Loop *loop = cast_to_loop(self);

    if (0 == loop)
        goto error;

    if (0 > fd)
        goto error;

    return release_fd_unsafe(loop, fd, userdata);

error:

    return false;
}

/*------------------------------------------------------------------*/

uint32_t impl_event_loop_linux_timer_set(ov_event_loop *self,
                                         uint64_t relative_usec, void *data,
                                         bool (*callback)(uint32_t id,
                                                          void *data)) {
    Loop *loop = cast_to_loop(self);
    int timer_fd = -1;
    struct timer_data *tdata = 0;

    if ((0 == loop) || (0 == callback)) {
        goto error;
    }

    if (0 == relative_usec) {
        /* Setting 0 would disable timer ... */
        relative_usec = 1;
    }

    acquire_timer_data_unsafe(loop, &timer_fd, &tdata);

    if ((0 > timer_fd) || (0 == tdata)) {
        ov_log_error("could not acquire a new timer");
        goto error;
    }

    if (UINT32_MAX < (unsigned)timer_fd) {
        ov_log_error("timer fd %i is not handleable as it "
                     "exceeds the "
                     "numerical limit",
                     timer_fd);
        goto error;
    }

    uint64_t sec = relative_usec / 1000 / 1000;
    uint64_t usec = relative_usec - 1000 * 1000 * sec;

    struct itimerspec tspec = {0};

    tspec.it_value.tv_sec = sec;
    tspec.it_value.tv_nsec = 1000 * usec;

    tspec.it_interval.tv_nsec = TIMER_NAGGING_INTERVAL_USEC * 1000;

    tdata->magic_bytes = TIMER_DATA_MAGIC_BYTES;
    tdata->loop = loop;
    tdata->data = data;
    tdata->callback = callback;

    if (!register_fd_with_epoll(loop, timer_fd, EPOLLIN, tdata,
                                callback_for_timer)) {

        ov_log_error("Could not register timer fd with epoll");
        goto error;
    }

    if (0 != timerfd_settime(timer_fd, 0, &tspec, 0)) {

        ov_log_error("Could not set time on timer");
        goto error;
    }

    return timer_fd;

error:

    if (-1 < timer_fd)
        release_timer_fd_unsafe(loop, timer_fd, 0);

    if (0 != tdata)
        free(tdata);

    return OV_TIMER_INVALID;
}

/*------------------------------------------------------------------*/

bool impl_event_loop_linux_timer_unset(ov_event_loop *self, uint32_t id,
                                       void **userdata) {

    Loop *loop = cast_to_loop(self);

    if (0 == loop) {
        ov_log_error("called with wrong argument");
        goto error;
    }

    return release_timer_fd_unsafe(loop, id, userdata);

error:

    return false;
}

/*----------------------------------------------------------------------------*/

#endif /* linux */
