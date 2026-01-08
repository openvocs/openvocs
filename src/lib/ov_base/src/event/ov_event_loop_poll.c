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
        @file           ov_event_loop_poll.c
        @author         Markus Toepfer
        @author         Michael Beer

        @date           2018-12-17

        @ingroup        ov_event_loop_poll

        @brief          Implementation of a poll based eventloop.


        ------------------------------------------------------------------------
*/

#include "../../include/ov_event_loop_poll.h"
#include "../../include/ov_dict.h"

#include "../../include/ov_time.h"
#include "../../include/ov_utils.h"
#include <time.h>

const uint16_t IMPL_POLL_LOOP_TYPE = 0x0001;
const uint64_t IMPL_TIMER_INVALID = UINT64_MAX;

#define IMPL_SOCKETS_MIN 10
#define IMPL_TIMERS_MIN 10

/*---------------------------------------------------------------------------*/

typedef struct {

  uint32_t size;
  struct pollfd array[];

} PollFdArray;

/*---------------------------------------------------------------------------*/

typedef struct {

  uint8_t events;
  bool (*func)(int, uint8_t, void *);
  void *data;

} DataSocket;

/*---------------------------------------------------------------------------*/

typedef struct {

  bool enabled;
  uint64_t absolute_usec;

  void *userdata;
  bool (*callback)(uint32_t id, void *userdata);

} DataTimer;

/*---------------------------------------------------------------------------*/

typedef struct {

  ov_event_loop public;
  ov_event_loop_config config;

  bool running;

  struct pollfd *fds;

  int wakeup[2];

  struct {

    DataSocket *socket;
    DataTimer *timer;

  } cb;

} PollLoop;

/*---------------------------------------------------------------------------*/

#define AS_POLL_LOOP(x)                                                        \
  (((ov_event_loop_cast(x) != 0) &&                                            \
    (IMPL_POLL_LOOP_TYPE == ((ov_event_loop *)x)->type))                       \
       ? (PollLoop *)(x)                                                       \
       : 0)

static bool flags_ov_to_poll(uint8_t *ov_flag, short *poll_event);

static bool close_all_sockets(PollLoop *loop);

static bool just_read_empty(int socket, uint8_t events, void *data);

/*---------------------------------------------------------------------------*/

static bool poll_loop_init(PollLoop *loop, ov_event_loop_config config);
static ov_event_loop *impl_poll_event_loop_free(ov_event_loop *self);

static bool impl_poll_event_loop_is_running(const ov_event_loop *self);
static bool impl_poll_event_loop_stop(ov_event_loop *self);
static bool impl_poll_event_loop_run(ov_event_loop *self, uint64_t max_runtime);

static bool impl_poll_event_loop_callback_set(
    ov_event_loop *self, int socket_fd, uint8_t events, void *data,
    bool (*callback)(int socket_fd, uint8_t events, void *data));

static bool impl_poll_event_loop_callback_unset(ov_event_loop *self,
                                                int socket_fd, void **userdata);

static uint32_t
impl_poll_event_loop_timer_set(ov_event_loop *self, uint64_t relative_usec,
                               void *data,
                               bool (*callback)(uint32_t id, void *data));

static bool impl_poll_event_loop_timer_unset(ov_event_loop *self, uint32_t id,
                                             void **userdata);

/*---------------------------------------------------------------------------*/

bool flags_ov_to_poll(uint8_t *ov_flag, short *poll_event) {

  if (!ov_flag || !poll_event)
    goto error;

  short flag = 0;

  if (*ov_flag & OV_EVENT_IO_IN) {
    flag |= POLLIN;
    flag |= POLLPRI;
  }

  if (*ov_flag & OV_EVENT_IO_OUT) {
    flag |= POLLOUT;
  }

  if (*ov_flag & OV_EVENT_IO_CLOSE) {
    flag |= POLLHUP;
  }

  if (*ov_flag & OV_EVENT_IO_ERR) {
    flag |= POLLERR;
  }

  *poll_event = flag;
  return true;

error:
  return false;
}

/*---------------------------------------------------------------------------*/

bool poll_loop_init(PollLoop *loop, ov_event_loop_config config) {

  if (!loop)
    goto error;

  if (!memset(loop, 0, sizeof(PollLoop)))
    goto error;

  if (!ov_event_loop_set_type(&loop->public, IMPL_POLL_LOOP_TYPE))
    goto error;

  config = ov_event_loop_config_adapt_to_runtime(config);

  if (config.max.sockets < IMPL_SOCKETS_MIN) {
    ov_log_error("Eventloop config with less then minimum sockets "
                 "%i",
                 IMPL_SOCKETS_MIN);
    goto error;
  }

  uint64_t size = 0;

  loop->config = config;

  loop->fds = calloc(loop->config.max.sockets, sizeof(struct pollfd));
  if (!loop->fds) {
    ov_log_error("Failed to allocate bytes"
                 " for poll fds.");
    goto error;
  }

  loop->cb.socket = calloc(loop->config.max.sockets, sizeof(DataSocket));
  if (!loop->cb.socket) {
    ov_log_error("Failed to allocate bytes"
                 " for socket callbacks.");
    goto error;
  }

  loop->cb.timer = calloc(loop->config.max.timers, sizeof(DataTimer));
  if (!loop->cb.timer) {
    ov_log_error("Failed to allocate bytes"
                 " for timer callbacks.");
    goto error;
  }

  size = loop->config.max.sockets * sizeof(struct pollfd) +
         loop->config.max.sockets * sizeof(DataSocket) +
         loop->config.max.timers * sizeof(DataTimer) + sizeof(PollLoop);

  ov_log_debug("Eventloop allocated  %" PRIu64 " bytes", size);

  // init sockets array to poll ignore fd
  for (uint32_t i = 0; i < loop->config.max.sockets; i++) {
    loop->fds[i].fd = -1;
  }

  // init timer callback array
  for (uint32_t i = 0; i < loop->config.max.timers; i++) {
    loop->cb.timer[i].enabled = false;
  }

  // init socket callback array
  for (uint32_t i = 0; i < loop->config.max.sockets; i++) {
    loop->cb.socket[i] = (DataSocket){0};
  }

  // set custom variables
  loop->running = false;

  // set impl functions
  loop->public.free = impl_poll_event_loop_free;

  loop->public.is_running = impl_poll_event_loop_is_running;
  loop->public.stop = impl_poll_event_loop_stop;
  loop->public.run = impl_poll_event_loop_run;

  loop->public.callback.set = impl_poll_event_loop_callback_set;
  loop->public.callback.unset = impl_poll_event_loop_callback_unset;

  loop->public.timer.set = impl_poll_event_loop_timer_set;
  loop->public.timer.unset = impl_poll_event_loop_timer_unset;

  /*
   *      Use a wakeup socket pair to ensure a poll wakeup.
   *
   *      loop->wakeup is only used to send dummy data to
   *      wakeup the poll loop.
   *
   */

  if (0 != socketpair(AF_UNIX, SOCK_STREAM, 0, loop->wakeup))
    goto error;

  if (loop->wakeup[0] >= (int)loop->config.max.sockets) {
    ov_log_error("Cannot create eventloop, socket index to small "
                 "increase config.max.socket at least to %i",
                 loop->wakeup[0]);
    goto error;
  }

  if (loop->wakeup[1] >= (int)loop->config.max.sockets) {
    ov_log_error("Cannot create eventloop, socket index to small "
                 "increase config.max.socket at least to %i",
                 loop->wakeup[1]);
    goto error;
  }

  loop->fds[loop->wakeup[0]].fd = loop->wakeup[0];
  loop->fds[loop->wakeup[1]].fd = loop->wakeup[1];
  loop->fds[loop->wakeup[0]].events = POLLIN | POLLERR;
  loop->fds[loop->wakeup[1]].events = POLLIN | POLLERR;

  loop->cb.socket[loop->wakeup[0]].events = OV_EVENT_IO_IN;
  loop->cb.socket[loop->wakeup[0]].data = loop;
  loop->cb.socket[loop->wakeup[0]].func = just_read_empty;
  loop->cb.socket[loop->wakeup[1]].events = OV_EVENT_IO_IN;
  loop->cb.socket[loop->wakeup[1]].data = loop;
  loop->cb.socket[loop->wakeup[1]].func = just_read_empty;

  return true;

error:
  if (loop && loop->fds) {
    free(loop->fds);
    loop->fds = NULL;
  }

  if (loop && loop->cb.socket) {
    free(loop->cb.socket);
    loop->cb.socket = NULL;
  }

  if (loop && loop->cb.timer) {
    free(loop->cb.timer);
    loop->cb.timer = NULL;
  }

  return false;
}

/*---------------------------------------------------------------------------*/

ov_event_loop *ov_event_loop_poll(ov_event_loop_config config) {

  if (0 == config.max.sockets)
    config.max.sockets = IMPL_SOCKETS_MIN;

  if (0 == config.max.timers)
    config.max.timers = IMPL_TIMERS_MIN;

  config = ov_event_loop_config_adapt_to_runtime(config);

  if ((config.max.sockets < 1) || (config.max.timers < 1)) {
    ov_log_error("Eventloop config not supported.");
    goto error;
  }

  PollLoop *loop = calloc(1, sizeof(PollLoop));
  if (!loop)
    goto error;

  if (poll_loop_init(loop, config)) {
    return (ov_event_loop *)loop;
  }

  free(loop);
error:
  return NULL;
}

/*---------------------------------------------------------------------------*/

bool close_all_sockets(PollLoop *loop) {

  if (!loop || !loop->fds)
    return false;

  for (uint32_t i = 0; i < loop->config.max.sockets; i++) {

    if (loop->fds[i].fd != -1) {
      close(loop->fds[i].fd);
      loop->fds[i].fd = -1;
    }
  }

  return true;
}

/*---------------------------------------------------------------------------*/

ov_event_loop *impl_poll_event_loop_free(ov_event_loop *self) {

  PollLoop *loop = AS_POLL_LOOP(self);
  if (!loop)
    goto error;

  loop->running = false;
  close_all_sockets(loop);

  if (loop->fds)
    loop->fds = ov_data_pointer_free(loop->fds);

  if (loop->cb.socket)
    loop->cb.socket = ov_data_pointer_free(loop->cb.socket);

  if (loop->cb.timer)
    loop->cb.timer = ov_data_pointer_free(loop->cb.timer);

  free(loop);
  return NULL;
error:
  return self;
}

/*---------------------------------------------------------------------------*/

bool impl_poll_event_loop_is_running(const ov_event_loop *self) {

  PollLoop *loop = AS_POLL_LOOP(self);
  if (!loop)
    return false;

  return loop->running;
}

/*----------------------------------------------------------------------------*/

bool impl_poll_event_loop_stop(ov_event_loop *self) {

  PollLoop *loop = AS_POLL_LOOP(self);
  if (!loop)
    return false;

  loop->running = false;

  // wakeup poll eventloop
  if (-1 != send(loop->wakeup[0], "stop", 4, 0))
    return true;

  return false;
}

/*---------------------------------------------------------------------------*/

static int calculate_timeout_msec(PollLoop *loop, uint64_t start_usec,
                                  uint64_t max_usec, uint64_t now_usec) {

  if (max_usec == OV_RUN_ONCE)
    return 0;

  if (now_usec - start_usec >= max_usec)
    return 0;

  if (max_usec > INT_MAX)
    max_usec = INT_MAX;

  uint64_t next_usec = max_usec;
  uint64_t next_elapse = 0;

  for (uint32_t i = 0; i < loop->config.max.timers; i++) {

    if (!loop->cb.timer[i].enabled)
      continue;

    if (loop->cb.timer[i].absolute_usec <= now_usec)
      return 0;

    next_elapse = loop->cb.timer[i].absolute_usec - now_usec;

    if (next_elapse < next_usec)
      next_usec = next_elapse;
  }

  // ov_log_debug("next_usec %zu", next_usec);
  return next_usec / 1000;
}

/*---------------------------------------------------------------------------*/

static void call_timer_callbacks(PollLoop *loop) {

  OV_ASSERT(loop);
  if (!loop)
    return;

  uint64_t now = ov_time_get_current_time_usecs();
  for (uint32_t i = 0; i < loop->config.max.timers; i++) {

    if (!loop->cb.timer[i].enabled)
      continue;

    if (now < loop->cb.timer[i].absolute_usec)
      continue;

    loop->cb.timer[i].callback(i, loop->cb.timer[i].userdata);
    memset(&loop->cb.timer[i], 0, sizeof(DataTimer));
  }

  return;
}

/*---------------------------------------------------------------------------*/

bool impl_poll_event_loop_run(ov_event_loop *self, uint64_t max) {

  PollLoop *loop = AS_POLL_LOOP(self);
  if (!loop || !loop->cb.socket || !loop->cb.timer || !loop->fds)
    goto error;

  uint64_t i = 0, event = 0;
  int timeout = 0, result = 0;

  loop->running = true;

  if (max == OV_RUN_ONCE)
    loop->running = false;

  uint64_t start = ov_time_get_current_time_usecs();
  uint64_t now = 0;

  errno = 0;
  do {
    now = ov_time_get_current_time_usecs();
    timeout = calculate_timeout_msec(loop, start, max, now);
    result = poll(loop->fds, loop->config.max.sockets, timeout);
    call_timer_callbacks(loop);

    if (result > 0) {

      for (i = 0; i < loop->config.max.sockets; i++) {

        // check if callback is set
        if (!loop->cb.socket[i].func)
          continue;

        // POLL to OV EVENTS
        event = 0;

        if ((loop->fds[i].revents & POLLIN) ||
            (loop->fds[i].revents & POLLPRI)) {

          event |= OV_EVENT_IO_IN;
        }

        if (loop->fds[i].revents & POLLOUT) {

          event |= OV_EVENT_IO_OUT;
        }

        if (loop->fds[i].revents & POLLHUP) {

          event |= OV_EVENT_IO_CLOSE;
        }

        if (loop->fds[i].revents & POLLERR) {

          event |= OV_EVENT_IO_ERR;
        }

        // check if event interest is set
        if (event == 0)
          continue;

        loop->cb.socket[i].func(i, event, loop->cb.socket[i].data);
      }

    } else if (result < 0) {
      ov_log_error("Could not run poll errno %i|%s", errno, strerror(errno));
      goto error;
    }

    if (max <= now - start)
      loop->running = false;

  } while (loop->running);

  return true;
error:
  return false;
}

/*------------------------------------------------------------------*/

bool impl_poll_event_loop_callback_set(
    ov_event_loop *self, int socket, uint8_t events, void *data,
    bool (*callback)(int socket_fd, uint8_t events, void *data)) {

  PollLoop *loop = AS_POLL_LOOP(self);
  if (!loop || !callback)
    goto error;

  short poll_event = 0;
  if (!flags_ov_to_poll(&events, &poll_event))
    goto error;

  int so_opt;
  socklen_t so_len = sizeof(so_opt);

  // check if socket is without error
  if (0 != getsockopt(socket, SOL_SOCKET, SO_ERROR, &so_opt, &so_len)) {
    ov_log_error("FAILURE callback listening "
                 "on socket with error "
                 "is NOT SUPPORTED.");
    goto error;
  }

  if (0 == poll_event) {
    ov_log_error("FAILURE callback listening "
                 "without events "
                 "is NOT SUPPORTED.");
    goto error;
  }

  if (!ov_socket_ensure_nonblocking(socket)) {
    close(socket);
    loop->fds[socket].fd = -1;
    goto error;
  }

  if ((uint32_t)socket >= loop->config.max.sockets) {
    ov_log_error("EVENTLOOP out of socket slots");
    goto error;
  }

  // ensure the socket is set as listener fd
  loop->fds[socket].fd = socket;

  // activate poll listening for events
  loop->fds[socket].events = poll_event;

  // activate callback data for event
  loop->cb.socket[socket].events = events;
  loop->cb.socket[socket].data = data;
  loop->cb.socket[socket].func = callback;

  /*
   *      FORCE the anouncement of the new
   *      callback over the wakeup trigger.
   *
   *      This will allow to add new callbacks
   *      to a blocking poll runnnig within a
   *      different thread.
   *
   *      NOTE development MUST ensure there
   *      is just one thread setting/unsetting
   *      a callback for a socket at a time.
   *
   */

  OV_ASSERT(loop->fds[socket].fd == socket);

  int r = -1;
  while ((r == -1) && (errno == EAGAIN)) {
    r = send(loop->wakeup[0], "wakeup", 6, 0);
  }
  return true;
error:
  return false;
}

/*------------------------------------------------------------------*/

bool impl_poll_event_loop_callback_unset(ov_event_loop *self, int socket,
                                         void **userdata) {

  PollLoop *loop = AS_POLL_LOOP(self);
  if (!loop || !loop->cb.socket || socket < 0)
    goto error;

  if (userdata)
    *userdata = loop->cb.socket[socket].data;

  if ((uint32_t)socket >= loop->config.max.sockets)
    goto error;

  // close poll listening at the socket
  loop->fds[socket].fd = -1;

  // deactivate poll listening
  loop->fds[socket].events = 0;

  // deactivate callback data
  loop->cb.socket[socket].events = 0;
  loop->cb.socket[socket].data = NULL;
  loop->cb.socket[socket].func = NULL;

  /*
   *      FORCE the anouncement of the new
   *      callback over the wakeup trigger.
   *
   *      This will allow to add unset callbacks
   *      to a blocking poll runnnig within a
   *      different thread.
   *
   *      NOTE development MUST ensure there
   *      is just one thread setting/unsetting
   *      a callback for a socket at a time.
   *
   */
  int r = -1;
  while ((r == -1) && (errno == EAGAIN)) {
    r = send(loop->wakeup[0], "wakeup", 6, 0);
  }
  return true;

error:
  return false;
}

/*------------------------------------------------------------------*/

uint32_t impl_poll_event_loop_timer_set(ov_event_loop *self,
                                        uint64_t relative_usec, void *data,
                                        bool (*callback)(uint32_t id,
                                                         void *data)) {

  PollLoop *loop = AS_POLL_LOOP(self);
  if (!loop || !callback || (relative_usec == IMPL_TIMER_INVALID))
    goto error;

  uint64_t now = ov_time_get_current_time_usecs();

  DataTimer *timer = NULL;

  if (0 == relative_usec)
    relative_usec = 1;

  uint32_t i = 0;

  for (i = 1; i < loop->config.max.timers; i++) {

    if (!loop->cb.timer[i].enabled) {
      timer = &loop->cb.timer[i];
      break;
    }
  }

  if (!timer)
    goto error;

  timer->enabled = true;
  timer->absolute_usec = now + relative_usec;
  timer->userdata = data;
  timer->callback = callback;

  /*
   *      FORCE the anouncement of the new
   *      callback over the wakeup trigger.
   *
   *      This will allow to add new callbacks
   *      to a blocking poll runnig within a
   *      different thread.
   *
   *      NOTE development MUST ensure there
   *      is just one thread setting/unsetting
   *      a callback for a socket at a time.
   *
   */

  int r = -1;
  while ((r == -1) && (errno == EAGAIN)) {
    r = send(loop->wakeup[0], "wakeup", 6, 0);
  }

  return i;
error:
  return OV_TIMER_INVALID;
}

/*------------------------------------------------------------------*/

bool impl_poll_event_loop_timer_unset(ov_event_loop *self, uint32_t id,
                                      void **userdata) {

  PollLoop *loop = AS_POLL_LOOP(self);
  if (!loop || (id == 0))
    goto error;

  if (userdata)
    *userdata = NULL;

  if (false == loop->cb.timer[id].enabled)
    return true;

  if (userdata)
    *userdata = loop->cb.timer[id].userdata;

  memset(&loop->cb.timer[id], 0, sizeof(DataTimer));

  loop->cb.timer[id].enabled = false;

  /*
   *      FORCE the anouncement of the new
   *      callback over the wakeup trigger.
   *
   *      This will allow to add new callbacks
   *      to a blocking poll runnnig within a
   *      different thread.
   *
   *      NOTE development MUST ensure there
   *      is just one thread setting/unsetting
   *      a callback for a socket at a time.
   *
   */

  int r = -1;
  while ((r == -1) && (errno == EAGAIN)) {
    r = send(loop->wakeup[0], "wakeup", 6, 0);
  }

  return true;
error:
  return false;
}

/*---------------------------------------------------------------------------*/

bool just_read_empty(int socket, uint8_t events, void *data) {

  if (socket < 0 || !events || !data)
    goto error;

  char buf[100];
  read(socket, buf, 100);

  return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/
