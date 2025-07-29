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
#ifndef OV_SIP_APP_H
#define OV_SIP_APP_H

#include "ov_sip_message.h"
#include <ov_base/ov_event_loop.h>
#include <stdint.h>

/*----------------------------------------------------------------------------*/

typedef struct {

    // If true, all i/o is logged to file /tmp/APP_NAME-PID_serde_in.log
    bool log_io;

    uint32_t reconnect_interval_secs;
    uint32_t accept_to_io_timeout_secs;

    void (*cb_closed)(int sckt, void *additional);
    void (*cb_reconnected)(int sckt, void *additional);
    void (*cb_accepted)(int sckt, void *additional);

    void *additional;

    bool are_methods_case_sensitive;

} ov_sip_app_configuration;

typedef struct ov_sip_app ov_sip_app;

/*****************************************************************************
                                CREATE / DESTROY
 ****************************************************************************/

ov_sip_app *ov_sip_app_create(char const *name,
                              ov_event_loop *loop,
                              ov_sip_app_configuration cfg);

ov_sip_app *ov_sip_app_free(ov_sip_app *app);

/*****************************************************************************
                                    Logging
 ****************************************************************************/

bool ov_sip_app_enable_logging(ov_sip_app *self, char const *path);

/*----------------------------------------------------------------------------*/

bool ov_sip_app_disable_logging(ov_sip_app *self);

/**
 * Registers a handler to be called whenever there is parsed data ready,
 * i.e. when the underlying sip instance returned {.data_type == data_type,
 * .value}, the handler is called with .value .
 * Currently, there is only one handler allowed - i.e. multiplexing different
 * types of Serde data is not supported.
 * If the parsed data does not fit `data_type` , the parsed data is dropped.
 */
bool ov_sip_app_register_handler(ov_sip_app *self,
                                 char const *method,
                                 void (*handler)(ov_sip_message const *message,
                                                 int socket,
                                                 void *additional));

/*----------------------------------------------------------------------------*/

/**
 * Registers a handler that will be called whenever a SIP response message
 * arrives
 */
bool ov_sip_app_register_response_handler(
    ov_sip_app *self,
    void (*handler)(ov_sip_message const *message,
                    int socket,
                    void *additional));

/*----------------------------------------------------------------------------*/

/**
 * Beware: ALL file descriptors handled by a sip app require to be closed
 * by `ov_sip_app_close()`.
 */
bool ov_sip_app_connect(ov_sip_app *self, ov_socket_configuration config);

/*----------------------------------------------------------------------------*/

/**
 * Beware: ALL file descriptors handled by a sip app require to be closed
 * by `ov_sip_app_close()`.
 */
bool ov_sip_app_open_server_socket(ov_sip_app *self,
                                   ov_socket_configuration cfg);

/*----------------------------------------------------------------------------*/

int ov_sip_app_close(ov_sip_app *self, int fd);

/*----------------------------------------------------------------------------*/

bool ov_sip_app_send(ov_sip_app *self, int fd, ov_sip_message *msg);

/*----------------------------------------------------------------------------*/
#endif
