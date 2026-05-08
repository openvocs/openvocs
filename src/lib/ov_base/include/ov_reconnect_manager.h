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
        @author         Michael Beer

        @ingroup        ov_reconnect_manager

        Implement reconnecting network connections.
        Currently supports TCP.

        The principle is rather simple:

        1. Create an reconnect manager using `ov_reconnect_manager_create`

        2. Trigger a connect request to a remote socket via `ov_reconnect_manager_connect`
        3. In regular intervals, try to connect to that remote socket

        4. When the connection could be established, call a use-supplied callback

        4. If loss of the connection has been detected, goto step 3

        5. After usage, free the reconnect manager using `ov_reconnect_manager_free`


        The entire procedure relies on `event_loop` - timers.

        For keeping management data, you must create an
        ov_reconnect_manager` entity before being  able to trigger connect requests.

        For all connect requests handled via one `ov_reconnect_manager`,
        there is only *one* reconnect interval.

        However, if you require different reconnect intervals for different
        connections, you are free to create several `ov_reconnect_managers`.

        BEWARE: Free any reconnect_managers *before* freeing the underlying
        event loop.

        BEWARE: Each `ov_reconnect_manager` consumes one timer of the
        underlying event loop.

        BEWARE: Currently, it is not foreseen to 'revoke' connect request,
        but if need be, it could easily be implemented.
        You can, however, just `free` the `ov_reconnect_manager` to
        stop all connect requests managed by this `ov_reconnect_manager`.

        The reconnect manager has been proven to work with both
        - the default ( `poll(2)` based ) event loop
        - the linux ( `epoll(2)` based) event loop (see `ov_linux` ).

        ------------------------------------------------------------------------
*/
#ifndef OV_RECONNECT_MANAGER_H
#define OV_RECONNECT_MANAGER_H

#include "ov_constants.h"
#include "ov_event_loop.h"

/*----------------------------------------------------------------------------*/

struct ov_reconnect_manager_struct;

typedef struct ov_reconnect_manager_struct ov_reconnect_manager;

/*----------------------------------------------------------------------------*/

/**
 * Create a reconnect manager to connect to TCP servers with automatic
 * reconnects.<br>
 *
 * If you require different reconnect intervals, use several reconnect managers.
 *
 * BEWARE: Each reconnect manager consumes one timer of the underlying loop!
 *
 * BEWARE: Free the reconnect_manager *before* freeing the attached
 * loop!
 */
ov_reconnect_manager *
ov_reconnect_manager_create(ov_event_loop *loop,
                            uint32_t reconnect_interval_secs,
                            size_t max_reconnect_sockets);

/*----------------------------------------------------------------------------*/

/**
 * Frees the reconnect manager, but *not* the loop within!
 *
 * BEWARE: Respect correct order: *first* free the reconnect manager, *then*
 * free the loop !
 */
ov_reconnect_manager *ov_reconnect_manager_free(ov_reconnect_manager *self);

/*----------------------------------------------------------------------------*/

typedef struct {

    bool (*io)(int, uint8_t, void *);
    bool (*reconnected)(int, void *);

} ov_reconnect_callbacks;

/**
 * Triggers the connection to a TCP server.
 * The actual reconnect will be attempted the next time the reconnect timer
 * is triggered.
 * After each successful connect, `reconnect_callback` will be called.
 * @return True if successful, false otherwise.
 */
bool ov_reconnect_manager_connect(ov_reconnect_manager *self,
                                  ov_socket_configuration cfg, uint8_t events,
                                  ov_reconnect_callbacks callbacks,
                                  void *userdata);

/*----------------------------------------------------------------------------*/
#endif /* OV_RECONNECT_MANAGER_H */
