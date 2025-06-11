/***
        ------------------------------------------------------------------------

        Copyright (c) 2022 German Aerospace Center DLR e.V. (GSOC)

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

        @author Michael J. Beer, DLR/GSOC
        @copyright (c) 2022 German Aerospace Center DLR e.V. (GSOC)

        ------------------------------------------------------------------------
*/
#ifndef OV_SERDE_APP_H
#define OV_SERDE_APP_H
/*----------------------------------------------------------------------------*/

#include <ov_base/ov_event_loop.h>
#include <ov_base/ov_registered_cache.h>
#include <ov_base/ov_serde.h>

/*----------------------------------------------------------------------------*/

typedef struct {

    ov_serde *serde;

    // If true, all i/o is logged to file /tmp/APP_NAME-PID_serde_in.log
    bool log_io;
    uint32_t reconnect_interval_secs;
    uint64_t accept_to_io_timeout_secs;

    void (*cb_closed)(int sckt, void *additional);
    void (*cb_reconnected)(int sckt, void *additional);
    void (*cb_accepted)(int sckt, void *additional);

    void *additional;

} ov_serde_app_configuration;

typedef struct ov_serde_app ov_serde_app;

/*****************************************************************************
                                CREATE / DESTROY
 ****************************************************************************/

ov_serde_app *ov_serde_app_create(char const *name,
                                  ov_event_loop *loop,
                                  ov_serde_app_configuration cfg);

ov_serde_app *ov_serde_app_free(ov_serde_app *app);

/*----------------------------------------------------------------------------*/

/**
 * Registers a handler to be called whenever there is parsed data ready,
 * i.e. when the underlying serde instance returned {.data_type == data_type,
 * .value}, the handler is called with .value .
 * Currently, there is only one handler allowed - i.e. multiplexing different
 * types of Serde data is not supported.
 * If the parsed data does not fit `data_type` , the parsed data is dropped.
 */
bool ov_serde_app_register_handler(ov_serde_app *self,
                                   uint64_t data_type,
                                   void (*handler)(void *data,
                                                   int socket,
                                                   void *additional));

/*----------------------------------------------------------------------------*/

/**
 * Beware: All file handles managed by a serde_app should be closed
 * exclusively by `ov_serde_app_close()`!
 */
bool ov_serde_app_connect(ov_serde_app *self, ov_socket_configuration config);

/*----------------------------------------------------------------------------*/

/**
 * Beware: All file handles managed by a serde_app should be closed
 * exclusively by `ov_serde_app_close()`!
 */
bool ov_serde_app_open_server_socket(ov_serde_app *self,
                                     ov_socket_configuration cfg);

/*----------------------------------------------------------------------------*/

int ov_serde_app_close(ov_serde_app *self, int fd);

/*----------------------------------------------------------------------------*/

bool ov_serde_app_send(ov_serde_app *self, int fd, ov_serde_data data);

/*----------------------------------------------------------------------------*/
#endif
