/***
        ------------------------------------------------------------------------

        Copyright (c) 2021 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_event_io.h
        @author         Markus Töpfer

        @date           2021-01-17

        This implementation is some interface between some IO processing engine,
        hosting events described over this interface AND the event processing
        implementation self.

        Example :

            To create some custom event processing engine for JSON input,
            and enable this engine within some webserver at some URI you
            need some create function, as well as some callback for processing.
            Optional is some callback to be informed about any socket close,
            if connection state is of importance for my_engine.

            my_custom_event_engine *my_engine = create( ... );

            if (ov_webserver_base_configure_event_handler(
                    webserver_intance,
                    const ov_memory_pointer {
                        .name = "my_server",
                        .length = 9
                    },
                    const ov_event_io_config {
                        .name = "some_uri_for_my_engine"
                        .userdata = my_engine,
                        .callback.process = my_engine_process,
                        .callback.close = my_engine_socket_close

                    }){

                ... log something like my_engine is available at:
                wss://my_server/some_uri_for_my_engine

                NOTE it actually IS immediately available over that uri,
                if the server is configured for the domain "my_server"
            }

        NOTE a handler may be enabled on ANY kind of socket based IO processing,
        which implements the following functions:

        ... configure_event_handler
        ... socket_close_forwarding
        ... raw_to_json_forwarding
        ... json_send

        ov_webserver_base is just some example, which happens to provide
        ov_event_io interfaces as JSON over WSS.

        Functionality: some IO engine will process incoming data as JSON and
        forward any JSON input to the process function, with some additional
        parameter, which include the actual send configuration.

        ------------------------------------------------------------------------
*/
#ifndef ov_event_io_h
#define ov_event_io_h

#include <limits.h>
#include <stdbool.h>

#include <ov_base/ov_buffer.h>
#include <ov_base/ov_json.h>

/* PATH_MAX is due to the fact of supprt of registrations
 * as URI within a webserver, which have PATH_MAX as maximum length */

#define OV_EVENT_IO_NAME_MAX PATH_MAX

/*----------------------------------------------------------------------------*/

typedef struct ov_event_parameter_send {

  void *instance;
  bool (*send)(void *instance, int socket, const ov_json_value *response);

} ov_event_parameter_send;

/*----------------------------------------------------------------------------*/

typedef struct ov_event_parameter {

  /*  Event parameter will be set by the caller of callback->process
   *  it is NOT guaranted all parameter are set within callback->process
   *
   *  There parameter set defines which parameter MAY be handover to the
   *  event processing engine.
   *
   *  Due to the handover and to guarantee no changes during processing
   *  a copy based handover is implemented. */

  struct {

    /* included as param to provide the actual request DOMAIN
     * use of uint8_t due to unicode domain names */

    uint8_t name[PATH_MAX];

  } domain;

  struct {

    /* included as param to provide the actual request URI
     * use of uint8_t due to unicode uri names */

    uint8_t name[PATH_MAX];

  } uri;

  /* SOME VALID SEND MUST BE SET FROM IO ENGINE */
  ov_event_parameter_send send;

} ov_event_parameter;

/*----------------------------------------------------------------------------*/

typedef struct ov_event_io_config {

  /* Each event handler MUST have some name,
   * which MUST be unique within some instance using this config */

  char name[OV_EVENT_IO_NAME_MAX];

  /* Custom userdata of the actual event processing handler implementation */

  void *userdata;

  struct {

    void (*close)(void *userdata, int socket);

    /*  NOTE The process function may run within threads,
     *  depending on the server type used.
     *
     *  NOTE input is a full pointer handover and MUST be freed
     *  using ov_json_value_free, when no longer required
     */
    bool (*process)(void *userdata, const int socket,
                    const ov_event_parameter *parameter, ov_json_value *input);

  } callback;

} ov_event_io_config;

/*----------------------------------------------------------------------------*/

/**
    This function is using the ov_event_parameter provided send function
    to send some JSON value to some socket.

    @param params   pointer to ov_event_parameter
    @param socket   socket to be send at (source TCP socket)
    @param val      pointer to JSON instance to be send

    NOTE Socket may be the socket hand over during the process function,
    OR some other socket somehow learned by userdata.

    NOTE Sending is NOT guaranted.
    A return of true indicates the send is accepted and scheduled, but may
    be missed. e.g. If the destination is already gone.
    This is most likely not the case for the socket handover with process,
    but may be for other socket destinations.
*/
static inline bool ov_event_io_send(const ov_event_parameter *params,
                                    int socket, const ov_json_value *val) {

  if (!params || !params->send.instance || !params->send.send)
    return false;

  return params->send.send(params->send.instance, socket, val);
}

#endif /* ov_event_io_h */
