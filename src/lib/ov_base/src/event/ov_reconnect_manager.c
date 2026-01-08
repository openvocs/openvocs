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

#include "../../include/ov_reconnect_manager.h"
#include "../../include/ov_thread_loop.h"
#include "../../include/ov_utils.h"

/*----------------------------------------------------------------------------*/

#define RECONNECT_DATA_MAGIC_BYTES 0x00726500
#define RECONNECT_MGR_MAGIC_BYTES 0x10726501

#define DEFAULT_RECONNECT_TIMEOUT_USECS 10 * 1000 * 1000

/*----------------------------------------------------------------------------*/

typedef struct {

  uint32_t magic_bytes;
  ov_socket_configuration sconfig;

  ov_reconnect_callbacks callbacks;

  int fd;

  /* if this is false, all other data is meaningless */
  struct {
    bool in_use : 1;
  };

  void *userdata;
  uint8_t events;

} reconnect_data;

/*----------------------------------------------------------------------------*/

struct ov_reconnect_manager_struct {

  uint32_t magic_bytes;

  ov_thread_loop *tloop;

  uint32_t reconnect_timer_id;
  ov_event_loop *loop;

  reconnect_data *fd_reconnect_data;

  uint64_t timer_interval_usecs;
  size_t capacity;
};

typedef struct ov_reconnect_manager_struct reconnect_mgr;

/******************************************************************************
 *                                  HELPERS
 ******************************************************************************/

static reconnect_data *as_reconnect_data(void *data) {

  if (0 == data)
    return 0;

  reconnect_data *rd = data;

  if (RECONNECT_DATA_MAGIC_BYTES != rd->magic_bytes)
    return 0;

  return rd;
}

/*----------------------------------------------------------------------------*/

static reconnect_mgr *as_reconnect_mgr(void *data) {

  if (0 == data)
    return 0;

  reconnect_mgr *rm = data;

  if (RECONNECT_MGR_MAGIC_BYTES != rm->magic_bytes)
    return 0;

  return rm;
}

/******************************************************************************
 *                                 RECONNECT
 ******************************************************************************/

#include <stdio.h>

static bool cb_io_callback(int fd, uint8_t events, void *userdata) {

  /* This callback better be FAST - it will be called WHENEVER
   * there is traffic on the fd */

  reconnect_data *rd = as_reconnect_data(userdata);

  if (0 == rd) {

    ov_log_error("Called with wrong userdata (expected "
                 "reconnect_data)");
    goto error;
  }

  if ((0 != (events & OV_EVENT_IO_CLOSE)) && (fd != rd->fd)) {

    ov_log_warning("Received CLOSE notification on already "
                   "removed FD, events: %" PRIu8,
                   events);

    close(fd);

    return false;
  }

  OV_ASSERT(fd == rd->fd);
  OV_ASSERT(0 != rd->callbacks.io);

  /* ensure we only call the actual user callback
   * on events he actually requested ... */
  uint8_t observed_events = events & rd->events;

  bool retval = true;

  if (0 != observed_events) {
    retval = rd->callbacks.io(fd, observed_events, rd->userdata);
  }

  if ((events & OV_EVENT_IO_CLOSE) || (events & OV_EVENT_IO_ERR) || (!retval)) {

    close(fd);

    ov_log_info("cb_io_callback: fd %i closed\n", fd);
    rd->fd = -1;
  }

  return retval;

error:

  return false;
}

/*----------------------------------------------------------------------------*/

typedef struct {

  ov_thread_message public;

  ov_socket_configuration sconfig;
  size_t i;

} connect_message;

#define CONNECT_MSG_MAGIC_BYTES 0x123A

static ov_thread_message *connect_message_free(ov_thread_message *self) {
  self = ov_thread_message_cast(self);

  if ((0 != self) && (CONNECT_MSG_MAGIC_BYTES == self->type)) {
    return ov_free(self);

  } else {
    ov_log_error("Cannot free connect message - wrong message type or invalid "
                 "message");
    return self;
  }
}

/*----------------------------------------------------------------------------*/

static ov_thread_message *create_connect_message(ov_socket_configuration scfg,
                                                 size_t i) {
  connect_message *msg = calloc(1, sizeof(connect_message));

  msg->public.magic_bytes = OV_THREAD_MESSAGE_MAGIC_BYTES;
  msg->public.type = CONNECT_MSG_MAGIC_BYTES;
  msg->public.free = connect_message_free;

  msg->sconfig = scfg;
  msg->i = i;

  return &msg->public;
}

/*----------------------------------------------------------------------------*/

typedef struct {

  ov_thread_message public;

  int fd;
  size_t i;

} new_con_message;

#define NEW_CON_MSG_MAGIC_BYTES 0x923F

static ov_thread_message *new_con_msg_free(ov_thread_message *self) {
  self = ov_thread_message_cast(self);

  if ((0 != self) && (NEW_CON_MSG_MAGIC_BYTES == self->type)) {
    return ov_free(self);

  } else {
    ov_log_error("Cannot free new connection message - wrong message type or "
                 "invalid message");
    return self;
  }
}

/*----------------------------------------------------------------------------*/

static ov_thread_message *new_con_msg_create(int fd, size_t i) {
  new_con_message *msg = calloc(1, sizeof(new_con_message));

  msg->public.magic_bytes = OV_THREAD_MESSAGE_MAGIC_BYTES;
  msg->public.type = NEW_CON_MSG_MAGIC_BYTES;
  msg->public.free = new_con_msg_free;

  msg->fd = fd;
  msg->i = i;

  return &msg->public;
}

/*----------------------------------------------------------------------------*/

static bool handle_in_thread(ov_thread_loop *self, ov_thread_message *msg) {
  msg = ov_thread_message_cast(msg);

  fprintf(stderr, "Connect thread woken up again...\n");

  if (0 == self) {
    ov_log_error("Cannot handle message - invalid thread loop");

  } else if ((0 == msg) || (CONNECT_MSG_MAGIC_BYTES != msg->type)) {
    ov_log_error("Cannot handle message - invalid message");

  } else {
    connect_message *cmsg = (connect_message *)msg;

    ov_socket_error err = {0};

    int fd = ov_socket_create(cmsg->sconfig, true, &err);

    if (0 >= fd) {
      ov_log_warning("Unable to reconnect - try again next cycle...");
    } else {
      ov_log_info("reconnected successfully");
      ov_thread_message *nc_msg = new_con_msg_create(fd, cmsg->i);

      if ((0 != nc_msg) &&
          ov_thread_loop_send_message(self, nc_msg, OV_RECEIVER_EVENT_LOOP)) {
        ov_log_info("Sent data for new connection to loop...");

      } else {
        nc_msg = ov_thread_message_free(nc_msg);
      }
    }
  }

  msg = ov_thread_message_free(msg);
  return true;
}

/*----------------------------------------------------------------------------*/

static bool handle_in_loop(ov_thread_loop *self, ov_thread_message *msg) {
  msg = ov_thread_message_cast(msg);

  ov_reconnect_manager *manager = ov_thread_loop_get_data(self);

  if (0 == self) {
    ov_log_error("Cannot handle message - invalid thread loop");

  } else if ((0 == manager) || (0 == manager->loop)) {
    ov_log_error("Cannot handle message - invalid reconnect manager");

  } else if ((0 == msg) || (NEW_CON_MSG_MAGIC_BYTES != msg->type)) {
    ov_log_error("Cannot handle message - invalid message");

  } else {
    new_con_message *cmsg = (new_con_message *)msg;

    if (manager->capacity <= cmsg->i) {
      ov_log_error("reconnect data index out of range");
    } else if (-1 < cmsg->fd) {
      reconnect_data *rd = &manager->fd_reconnect_data[cmsg->i];

      if ((!rd->in_use) || (-1 < rd->fd)) {
        ov_log_error("reconnect data index points to either unused or already "
                     "est connection");
        close(cmsg->fd);
        cmsg->fd = -1;
      } else {

        rd->fd = cmsg->fd;
        cmsg->fd = -1;

        uint8_t events = rd->events | OV_EVENT_IO_CLOSE | OV_EVENT_IO_ERR;

        bool registered = manager->loop->callback.set(
            manager->loop, rd->fd, events, rd, cb_io_callback);

        if (!registered) {
          ov_log_error("Unable to register fd with loop");
          close(rd->fd);
          rd->fd = -1;
        }

        if ((0 != rd->callbacks.reconnected) && (0 < rd->fd)) {
          rd->callbacks.reconnected(rd->fd, rd->userdata);
        }
      }
    } else {
      ov_log_error("Got invalid fd - ignoring");
    }
  }

  msg = ov_thread_message_free(msg);
  return true;
}

/*----------------------------------------------------------------------------*/

static bool cb_reconnect_timer(uint32_t id, void *data) {

  UNUSED(id);

  fprintf(stderr, "Reconnect timer called\n");
  reconnect_mgr *rm = as_reconnect_mgr(data);

  if (0 == rm) {
    ov_log_error("reconnect timer called with wrong userdata");
    goto error;
  }

  OV_ASSERT(0 != rm->fd_reconnect_data);

  for (size_t i = 0; rm->capacity > i; ++i) {
    if (!rm->fd_reconnect_data[i].in_use)
      continue;

    fprintf(stderr, "Reconnect timer: Checking %zu\n", i);

    if (0 > rm->fd_reconnect_data[i].fd) {
      fprintf(stderr, "Reconnect timer: %zu not connected\n", i);
      ov_thread_message *msg =
          create_connect_message(rm->fd_reconnect_data[i].sconfig, i);

      if (ov_thread_loop_send_message(rm->tloop, msg, OV_RECEIVER_THREAD)) {
        msg = 0;

      } else {
        ov_log_error("Could not send connect message to thread");
        msg = ov_thread_message_free(msg);
      }
    }
  }

  OV_ASSERT(0 != rm->loop->timer.set);

  /* enable timer */
  rm->reconnect_timer_id = rm->loop->timer.set(
      rm->loop, rm->timer_interval_usecs, rm, cb_reconnect_timer);

  if (OV_TIMER_INVALID == rm->reconnect_timer_id) {
    ov_log_error("Could not set up reconnect timer");
    goto error;
  }

  return true;

error:

  return false;
}

/******************************************************************************
 *                        ov_reconnect_manager_create
 ******************************************************************************/

static ov_thread_loop *start_connect_thread(ov_event_loop *loop,
                                            ov_reconnect_manager *manager) {
  return ov_thread_loop_create(loop,
                               (ov_thread_loop_callbacks){
                                   .handle_message_in_thread = handle_in_thread,
                                   .handle_message_in_loop = handle_in_loop,
                               },
                               manager);
}

/*----------------------------------------------------------------------------*/

ov_reconnect_manager *
ov_reconnect_manager_create(ov_event_loop *loop,
                            uint32_t reconnect_interval_secs,
                            size_t max_reconnect_sockets) {
  ov_reconnect_manager *rm = 0;

  fprintf(stderr, "Creating reconnect manager...\n");

  if (0 == loop)
    goto error;

  if (0 == max_reconnect_sockets)
    goto error;

  rm = calloc(1, sizeof(ov_reconnect_manager));

  rm->magic_bytes = RECONNECT_MGR_MAGIC_BYTES;

  rm->loop = loop;

  rm->timer_interval_usecs = reconnect_interval_secs * 1000 * 1000;

  if (0 == rm->timer_interval_usecs) {
    rm->timer_interval_usecs = DEFAULT_RECONNECT_TIMEOUT_USECS;
  }

  rm->capacity = max_reconnect_sockets;

  rm->fd_reconnect_data = calloc(max_reconnect_sockets, sizeof(reconnect_data));

  if (0 == rm->fd_reconnect_data) {
    goto error;
  }

  for (size_t i = 0; i < max_reconnect_sockets; ++i) {
    rm->fd_reconnect_data[i].in_use = false;
    rm->fd_reconnect_data[i].magic_bytes = RECONNECT_DATA_MAGIC_BYTES;
  }

  OV_ASSERT(0 != loop->timer.set);

  rm->tloop = start_connect_thread(loop, rm);
  if ((0 == rm->tloop) || (!ov_thread_loop_start_threads(rm->tloop))) {
    ov_log_error("Could not start connect thread");
    goto error;
  }

  /* enable timer */
  rm->reconnect_timer_id =
      loop->timer.set(loop, rm->timer_interval_usecs, rm, cb_reconnect_timer);

  if (OV_TIMER_INVALID == rm->reconnect_timer_id) {
    ov_log_error("Could not set up reconnect timer");
    goto error;
  }

  ov_log_info("Enabled reconnect with reconnect interval of "
              "%" PRIu64 " usecs",
              rm->timer_interval_usecs);

  return rm;

error:

  fprintf(stderr, "Error during reconnect manager creation\n");
  exit(1);

  if ((0 != rm) && (0 != rm->fd_reconnect_data)) {
    free(rm->fd_reconnect_data);
  }

  if (0 != rm) {
    ov_thread_loop_free(rm->tloop);
    free(rm);
  }

  return 0;
}

/******************************************************************************
 *                         ov_reconnect_manager_free
 *****************************************************************************/

static bool stop_timer(ov_event_loop *loop, uint32_t tid) {
  if (0 == loop) {
    ov_log_error("Loop not set");
    return false;
  }

  if (OV_TIMER_INVALID == tid) {
    ov_log_warning("Timer ID invalid -> Timer not set");
    return true;
  }

  return loop->timer.unset(loop, tid, 0);
}

/*----------------------------------------------------------------------------*/

static void close_and_unregister_fds_unsafe(ov_event_loop *loop,
                                            reconnect_data *rd,
                                            size_t number_entries) {
  OV_ASSERT(0 != loop);
  OV_ASSERT(0 != rd);

  for (size_t i = 0; number_entries > i; ++i) {
    int fd = rd[i].fd;

    if (0 > fd)
      continue;

    void *dummy = 0;

    loop->callback.unset(loop, fd, &dummy);

    close(fd);

    rd[i].fd = -1;
    rd[i].in_use = false;
  }
}
/*----------------------------------------------------------------------------*/

ov_reconnect_manager *ov_reconnect_manager_free(ov_reconnect_manager *self) {
  if (0 == self) {
    goto error;
  }

  self->tloop = ov_thread_loop_free(self->tloop);

  stop_timer(self->loop, self->reconnect_timer_id);

  if ((0 != self->fd_reconnect_data) && (0 != self->loop)) {
    close_and_unregister_fds_unsafe(self->loop, self->fd_reconnect_data,
                                    self->capacity);
  }

  if (0 != self->fd_reconnect_data) {
    free(self->fd_reconnect_data);
    self->fd_reconnect_data = 0;
  }

  free(self);
  self = 0;

error:

  return self;
}

/******************************************************************************
 *                        ov_reconnect_manager_connect
 ******************************************************************************/

bool ov_reconnect_manager_connect(ov_reconnect_manager *self,
                                  ov_socket_configuration cfg, uint8_t events,
                                  ov_reconnect_callbacks callbacks,
                                  void *userdata) {
  /* all we do is create an entry in the reconnect_manager for
   * this connection, mark it disconnected. The actual connect
   * will be done by the timer the next time it is called
   */

  fprintf(stderr, "reconnect connect called...\n");

  if (0 == self) {
    ov_log_error("No reconnect manager given (0 pointer)");
    goto error;
  }

  if ((0 == callbacks.io) || (0 == callbacks.reconnected)) {
    ov_log_error("Not all required callbacks given (0 "
                 "pointers)");
    goto error;
  }

  ssize_t found = -1;

  for (size_t i = 0; self->capacity > i; ++i) {
    if (!self->fd_reconnect_data[i].in_use) {
      found = i;
      break;
    }
  }

  if (0 > found) {
    ov_log_error("Max number of reconnecting connections "
                 "exhausted");
    goto error;
  }

  reconnect_data *rd = &self->fd_reconnect_data[found];

  rd->in_use = true;

  memcpy(&rd->sconfig, &cfg, sizeof(cfg));

  rd->events = events;
  rd->userdata = userdata;

  rd->callbacks = callbacks;

  rd->fd = -1;

  fprintf(stderr, "reconnect installed at %zu\n", found);

  return true;

error:

  return false;
}
/*----------------------------------------------------------------------------*/
