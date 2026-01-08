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
        @file           ov_event_loop.h
        @author         Markus Toepfer
        @author         Michael Beer

        @date           2018-12-17

        @ingroup        ov_event_loop

        @brief          Definition of a standard eventloop interface
                        for ov_ development

                        This eventloop interface defines an eventloop able
                        to handle timer based, as well as socket IO based
                        event handling. Precision and performance of the
                        eventloop is dependent on the implementation.

                        Event loops also handle signals - BUT: This will
                        only work properly if there is only one event loop
                        per process!

                        Eventloops MUST be configurable with a maximum amount
                        of supported timers, as well as a maximum amount of
                        supported sockets.

                        NOTE for STREAM based sockets, each connection will
                        make use of one socket. So this eventloop configuration
                        will define the capacity of the "server" or "service".

                        The default eventloop implementation is POSIX based
                        without external dependencies.

                        The default timer implementation will deliver timer
                        events inline and multiplexed with IO. If you need
                        some realtime clock based timer events the default
                        implementation is NOT the way to go.

                        This header defines the interface, as well as the
                        interface tests for ov_ eventloops.

                        NOTE for implementors of new loops:
                        Always call `ov_event_loop_setup_signals()` in your
                        constructor!

        ------------------------------------------------------------------------
*/
#ifndef ov_event_loop_h
#define ov_event_loop_h

#define OV_EVENT_IO_IN 0x01
#define OV_EVENT_IO_OUT 0x02
#define OV_EVENT_IO_ERR 0x04
/**
 * Do *not* rely on the CLOSE event - it is not triggered
 * as you might expect!
 * Instead, check for EOF in your io_callbacks.
 */
#define OV_EVENT_IO_CLOSE 0x08
#define OV_EVENT_IO_OPEN 0x10

#define OV_TIMER_INVALID 0
#define OV_TIMER_MAX UINT_MAX
#define OV_RUN_MAX UINT64_MAX
#define OV_RUN_ONCE 0

#define OV_EVENT_LOOP_MAX_SOCKETS_DEFAULT 1024
#define OV_EVENT_LOOP_MAX_TIMERS_DEFAULT 1024

#define OV_EVENT_LOOP_SOCKETS_MIN 1
#define OV_EVENT_LOOP_TIMERS_MIN 1

#include <stdbool.h>
#include <stdint.h>

/* Threading is used in test cases */
#include "ov_socket.h"
#include <ov_test/testrun.h>
#include <pthread.h>

#define OV_EVENT_LOOP_MAGIC_BYTE 0xabba

#define OV_EVENT_LOOP_KEY "eventloop"
#define OV_EVENT_LOOP_KEY_MAX_SOCKETS "sockets"
#define OV_EVENT_LOOP_KEY_MAX_TIMERS "timers"

typedef struct ov_event_loop ov_event_loop;
typedef struct ov_event_loop_config ov_event_loop_config;
typedef struct ov_event_loop_event ov_event_loop_event;

/*---------------------------------------------------------------------------*/

struct ov_event_loop_config {

  struct {

    uint32_t sockets;
    uint32_t timers;

  } max;
};

/*---------------------------------------------------------------------------*/

struct ov_event_loop {

  uint16_t magic_byte; // identify an event loop structure
  uint16_t type;       // identify a custom implementation type

  /*------------------------------------------------------------------*/

  /** Don't call that method directly */
  ov_event_loop *(*free)(ov_event_loop *self);
  bool (*is_running)(const ov_event_loop *self);

  /*
   *      Stop self from running.
   */
  bool (*stop)(ov_event_loop *self);

  /*
   *      Run the loop and handle events until
   *      (A) the loop is stopped,
   *      (B) or max_runtime exceeded.
   *
   *      @NOTE   a max_runtime of OV_RUN_ONCE
   *              MUST run the loop exactely ONCE on all sockets
   *
   *      @NOTE   maximum supported runtime is UINT64_MAX usec
   *
   *      @param self             eventloop to run
   *      @param max_runtime      max loop runtime
   */
  bool (*run)(ov_event_loop *self, uint64_t max_runtime_usec);

  /*------------------------------------------------------------------*/

  struct {

    /*
     *      Set a callback to a socket_fd
     *
     *      @param self     eventloop to use
     *      @param socket   socket fd to use
     *      @param events   events to listen to (OR of ov_events)
     *      @param userdata optional data for callback
     *      @param callback mandatory callback for fd
     *
     *      BEWARE: the callback *must* check for EOF (`read` /
     *      `write` returns 0) and return `false` in that case!
     *
     *      @NOTE           access over socket_fd number
     *
     */
    bool (*set)(ov_event_loop *self, int socket, uint8_t events, void *userdata,
                bool (*callback)(int socket_fd, uint8_t events,
                                 void *userdata));

    /*
     *      Unset a callback of a socket_fd
     *
     *      @param self     eventloop to use
     *      @param socket   socket fd to use
     *      @param userdata optional pointer to get out some
     *                      userdata set during callback.set
     */
    bool (*unset)(ov_event_loop *self, int socket, void **userdata);

  } callback;

  /*------------------------------------------------------------------*/

  struct {

    /*
     *      Set a timer callback at target_usec. ONE SHOT ONLY
     *
     *      The timer is invalid after call. Any data pointer
     *      MAY be reused in a next timer->set to create intervall
     *      style timers.
     *
     *      Time accuracy MAY be dependent on the implementation and
     *      load.
     *
     *      @param self     eventloop to use
     *      @param relative relative target callback time in usecs
     *      @param data     optional user data for callback
     *      @param callback mandatory callback at target time
     *
     *      @returns        uint32_t id for timer
     *                      OV_TIMER_INVALID on error
     *
     *      @NOTE           There is no guarantee whatsoever about
     *                      the way the timer ids are acquired,
     *                      they might start off with 0 and
     *                      increase, but that is in no way
     *                     guaranteed!
     *
     */

    uint32_t (*set)(ov_event_loop *self, uint64_t relative_usec, void *data,
                    bool (*callback)(uint32_t id, void *data));

    /*
     *      Unset a timer with timer_id
     *
     *      @param self     eventloop to use
     *      @param id       timer id to use
     *      @param dfree    optional free function for data
     *                      handover with set
     */
    bool (*unset)(ov_event_loop *self, uint32_t id, void **userdata);

  } timer;

  int log_fd;
};

/*
 *      ------------------------------------------------------------------------
 *
 *      DEFAULT EVENT LOOP CREATION
 *
 *      ------------------------------------------------------------------------
 */

/**
        Create a default event loop config

                {
                        .sockets = OV_EVENT_LOOP_MAX_SOCKETS_DEFAULT,
                        .timers  = OV_EVENT_LOOP_MAX_TIMERS_DEFAULT
                } max;
*/
ov_event_loop_config ov_event_loop_config_default();

/*---------------------------------------------------------------------------*/

/**
        Return a default event loop.
*/
ov_event_loop *ov_event_loop_default(ov_event_loop_config config);

/*---------------------------------------------------------------------------*/

/**
        Call the free function on the eventloop
*/
void *ov_event_loop_free(void *eventloop);

/*----------------------------------------------------------------------------*/

uint32_t ov_event_loop_timer_set(ov_event_loop *self, uint64_t relative_usec,
                                 void *data,
                                 bool (*callback)(uint32_t id, void *data));

bool ov_event_loop_timer_unset(ov_event_loop *self, uint32_t id,
                               void **userdata);

/*----------------------------------------------------------------------------*/

bool ov_event_loop_set(ov_event_loop *self, int socket, uint8_t events,
                       void *userdata,
                       bool (*callback)(int socket_fd, uint8_t events,
                                        void *userdata));

bool ov_event_loop_unset(ov_event_loop *self, int socket, void **userdata);

/*----------------------------------------------------------------------------*/

bool ov_event_loop_run(ov_event_loop *self, uint64_t max_runtime_usec);

bool ov_event_loop_stop(ov_event_loop *self);

/*----------------------------------------------------------------------------*/

/*
 *      ------------------------------------------------------------------------
 *
 *      COMMON EVENT LOOP FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

/**
        Check if the magic_byte is set and cast on success to
        ov_event_loop.
*/
ov_event_loop *ov_event_loop_cast(const void *);

/**
 * Setup signals in event loop:
 * Disable signals like SIGPIPE which should be ignored anyways because
 * they bear no benefit.
 * Register appropriate handlers for signals like SIGINT.
 */
bool ov_event_loop_setup_signals(ov_event_loop *loop);

/*---------------------------------------------------------------------------*/

/**
        Set the magic_byte,
        and the custom type field.
*/
bool ov_event_loop_set_type(ov_event_loop *self, uint16_t type);

/*---------------------------------------------------------------------------*/

/**
        Adapt the eventloop config to the runtime environment.
*/
ov_event_loop_config
ov_event_loop_config_adapt_to_runtime(ov_event_loop_config config);

/*---------------------------------------------------------------------------*/

/**
        Default connection accept implementation for STREAM based sockets.

        This function will accept all incomming connections and set the
        event callbacks at the accepted connection sockets.

        @NOTE This function uses the IO callback interface of the actual
        (ANY) ov_event_loop implementation and add the IO callback to all
        accepted connection fds.

        @NOTE To cleanup data @see ov_event_remove_default_connection_accept
        MUST be used at the socket.

        @NOTE This function MAY be used to add some auto accept like behaviour
        to STREAM based sockets.

        @NOTE The added connection callback SHOULD unset the callback
        at the socket on connection close.

        @param loop     eventloop to use
        @param socket   listening socket_fd
        @param events   events to listen at connection socket
        @param data     data to be passed to callback
        @param callback connection callback to be called on IO events
        @returns true if the accept callback was set at the listener socket.
*/
bool ov_event_add_default_connection_accept(
    ov_event_loop *loop, int socket, uint8_t events, void *data,
    bool (*callback)(int connection_socket, uint8_t connection_events,
                     void *data));

/*---------------------------------------------------------------------------*/

/**
        Remove all default connection accept data from a socket.
        Counterpart to @see ov_event_add_default_connection_accept.
*/
bool ov_event_remove_default_connection_accept(ov_event_loop *self, int socket);

/*---------------------------------------------------------------------------*/

/**
        Create an event loop config, based on a JSON config.

        Valid input:

        {
                "OV_EVENT_LOOP_KEY" :
                {
                        "OV_EVENT_LOOP_KEY_MAX_SOCKETS" : 1,
                        "OV_EVENT_LOOP_KEY_MAX_TIMERS" : 1,
                }
        }

        OR without the additional keyed object

        {
                "OV_EVENT_LOOP_KEY_MAX_SOCKETS" : 1,
                "OV_EVENT_LOOP_KEY_MAX_TIMERS" : 1,
        }

        @NOTE this double input is done to support automated
        load out of some bigger config e.g.

        {
                "OV_EVENT_LOOP_KEY" : { ... },
                "OV_APP_KEY" : { ... },
                "OV_SSL_KEY" : { ... }
        }

        @param value    some config
        @returns parsed items set to config or empty config.
*/
ov_event_loop_config ov_event_loop_config_from_json(const ov_json_value *value);

/*---------------------------------------------------------------------------*/

/**
        Create some JSON value based on an event loop config

        @param config   Config to create

        @returns
        {
                "OV_EVENT_LOOP_KEY_MAX_SOCKETS" : 1,
                "OV_EVENT_LOOP_KEY_MAX_TIMERS" : 1,
        }

        The return value was choosen to add the output to some
        bigger config using OV_EVENT_LOOP_KEY
*/
ov_json_value *ov_event_loop_config_to_json(ov_event_loop_config config);

#endif /* ov_event_loop_h */
