/***

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

***/ /**

     \file               ov_thread_loop.h
     \author             Michael J. Beer, DLR/GSOC
     \date               2018-07-11

     \ingroup            empty

     Ties together an event loop with a thread pool.
     Provides for means to send information from the callbacks of the loop
     to the pool and back.

 **/
/*---------------------------------------------------------------------------*/

#ifndef ov_thread_loop_h
#define ov_thread_loop_h

#include "ov_event_loop.h"
#include "ov_thread_message.h"

/******************************************************************************
 *                                  TYPEDEFS
 ******************************************************************************/

typedef enum {

  RESERVED_OV_RECEIVER_ENSURE_SIGNED_INT = INT_MIN,
  OV_RECEIVER_EVENT_LOOP = 0,
  OV_RECEIVER_THREAD

} ov_thread_receiver;

/*----------------------------------------------------------------------------*/

typedef struct ov_thread_loop ov_thread_loop;

/**
 * Configuration.
 * Values that are zero will be replaced by default values.
 */
typedef struct {

  /**
   * If true, no queue will be used to send messages (from the threads) to
   * the event loop.
   * Using a queue incurs some performance penalty, (using the queue,
   * having to lock down on mutex in EL -> 2 additional context
   * switches).
   * On the other hand, if no queue is used, messages that are lost cause
   * mem leaks.
   * Thus if there is no chance that the EL might be too slow to process
   * all the messages from the threads, it might be useful to switch
   * using the queue off.
   * If in doubt, use the queue.
   */
  bool disable_to_loop_queue;

  uint64_t message_queue_capacity;

  uint64_t lock_timeout_usecs;

  size_t num_threads;

} ov_thread_loop_config;

/*----------------------------------------------------------------------------*/

/**
 * The functions that are called whenever a message has been sent.
 *
 * BEWARE: All handlers need to free the messages themselves.
 */
typedef struct {

  /**
   * Executed in the threads.
   * Do the processing.
   * Avoid I/O here.
   */
  bool (*handle_message_in_thread)(ov_thread_loop *, ov_thread_message *);

  /** Executed in the loop thread - called by the loop.
   * Avoid processing.
   * Do I/O here.
   */
  bool (*handle_message_in_loop)(ov_thread_loop *, ov_thread_message *);

} ov_thread_loop_callbacks;

/******************************************************************************
 *
 *  FUNCTIONS
 *
 ******************************************************************************/

/**
 * Create a new thread_pool_process.
 * In here, only a minimalistic config is done.
 * The actual configuration is done via a call to ov_thread_loop_reconfigure.
 *
 * @param event_loop the event loop to use
 * @param thread_pool_process_data additional data that can be retrieved
 * in the handlers via ov_thread_loop_get_data()
 */
ov_thread_loop *ov_thread_loop_create(ov_event_loop *event_loop,
                                      ov_thread_loop_callbacks callbacks,
                                      void *thread_pool_process_data);

/*----------------------------------------------------------------------------*/

/**
 * Stop thread pool, free resources.
 */
ov_thread_loop *ov_thread_loop_free(ov_thread_loop *self);

/**
 * Get the thread_pool_process specific data that was supplied to
 * ov_thread_loop_create()
 *
 * BEWARE: The data is NOT synced!
 */
void *ov_thread_loop_get_data(ov_thread_loop *self);

/*----------------------------------------------------------------------------*/

bool ov_thread_loop_reconfigure(ov_thread_loop *self,
                                ov_thread_loop_config config);

/*----------------------------------------------------------------------------*/

/**
 * Send messages along.
 * @param self the threaded thread_pool_process.
 * @param message message to be sent.
 * @param receiver denotes the receiver of the message.
 * @return true on success, false else
 */
bool ov_thread_loop_send_message(ov_thread_loop *self,
                                 ov_thread_message *message,
                                 ov_thread_receiver receiver);

/*----------------------------------------------------------------------------*/

bool ov_thread_loop_start_threads(ov_thread_loop *self);

/**
 * Stops all threads of the thread pool.
 * Must be done before e.g. resetting the thread_pool_process_data if the
 * threads access it.
 */
bool ov_thread_loop_stop_threads(ov_thread_loop *self);

/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/**
 * Serialize ov_threads_pool_process_config as JSON.
 * Configuration will be overwritten.
 * @target Target JSON object. If null, a new JSON object will be created.
 */
ov_json_value *ov_thread_loop_config_to_json(const ov_thread_loop_config config,
                                             ov_json_value *target);

/*----------------------------------------------------------------------------*/

/**
 * Deserialize ov_thread_loop_config from JSON
 */
ov_thread_loop_config
ov_thread_loop_config_from_json(const ov_json_value *restrict json);

/*----------------------------------------------------------------------------*/
#endif /* ov_thread_loop_h */
