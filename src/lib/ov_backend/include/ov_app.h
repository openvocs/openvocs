/***
        ------------------------------------------------------------------------

        Copyright (c) 2019 German Aerospace Center DLR e.V. (GSOC)

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
        @author         Markus Toepfer
        @author         Michael Beer

        @date           2020-04-08

        The app is the standard interface between network and business
        logic for openvocs applications.

        All network IO shall be handeled within the app, which uses some
        eventloop to process network request and callbacks to some actual
        utilization of the network IO.

        The app interface itself is configurable at programming level using
        the ov_app_config.

        Actuall used sockets and transports are configurable
        over the app interface.

        ------------------------------------------------------------------------
*/
#ifndef ov_app_h
#define ov_app_h

/*
 *      ------------------------------------------------------------------------
 *
 *      CONFIGURATION KEY DEFINITIONS
 *
 *      ------------------------------------------------------------------------
 */

#define OV_APP_KEY "app"
#define OV_APP_KEY_NAME "name"
#define OV_APP_KEY_DEBUG "debug"
#define OV_APP_KEY_CACHE_CAPACITY "cache capacity"
#define OV_APP_KEY_PRECACHE "precache"
/*
 *      ------------------------------------------------------------------------
 *
 *      DEFINITIONS
 *
 *      ------------------------------------------------------------------------
 */

#define OV_APP_MAGIC_BYTE 0xAA42
#define OV_APP_NAME_MAX 512 // name in e.g. log messages
#define OV_APP_NO_IDLE_TIMEOUT 0
#define OV_APP_MIN_IDLE_TIMEOUT_USEC 10000 // 10 milliseconds

#define OV_APP_DEFAULT_CACHE 100

#define OV_APP_DEFAULT_NAME "ov_app"

#include <ov_base/ov_buffer.h>
#include <ov_base/ov_const.h>
#include <ov_base/ov_event_loop.h>
#include <ov_base/ov_parser.h>

typedef struct ov_app ov_app;

typedef enum { OV_IN = 1, OV_OUT = 2 } ov_direction;

typedef struct {

  char name[OV_APP_NAME_MAX];

  ov_event_loop *loop;
  void *userdata;

  struct {

    bool precache;   // precache up to capacity at init
    size_t capacity; // default cache capacity to use

  } cache;

  struct {

    size_t interval_secs;
    size_t max_connections;

  } reconnect;

} ov_app_config;

/*----------------------------------------------------------------------------*/

typedef struct {

  ov_socket_configuration socket_config;

  bool as_client; // if true -> connect socket to remote

  struct {

    ov_parser_config config;
    ov_parser *(*create)(ov_parser_config config);

  } parser;

  struct {

    void *userdata;

    /**
     * Will be called if socket was closed
     */
    void (*close)(ov_app *app, int socket, const char *uuid, void *userdata);

    /*
     *      If a parser was set for the socket,
     *      parser_io_data is the output of the
     *      parser. This value may be consumend (set to NULL)
     *      If is is not consumed, it will be cleared within
     *      the APP.
     *
     *      The callback will receive:
     *
     *              app     always
     *              uuid    connection uuid set, NULL on UDP sockets
     *              local   parsed local connection socket (always)
     *              remote  parsed remote connection data (always)
     *              io_data parsed IO data delivered by parser,
     *                      defaults to ov_buffer with RAW socket
     data

     *      By default, if no parser is configured,
     *      ANY RAW socket input is delivered as ov_buffer.
     *      use ov_buffer_cast(*parsed_io_data)
     *
     */
    bool (*io)(ov_app *app, int socket, const char *uuid,
               const ov_socket_data *remote, void **parsed_io_data);

    /**
     * Only used if as_client == false .
     * Callback to be called if a new connection was
     * accepted on a (TCP) server socket.
     */
    bool (*accepted)(ov_app *app, int server_socket, int accepted_socket,
                     void *userdata);

    /**
     * Only used if as_client == true .
     * If set, the socket will be reconnected
     * if closed.
     * On successful reconnect,
     * this callback will be called.
     *
     * Currently supported only with TCP sockets!
     * @param fd the new client socket.
     * @param app the app this socket is handled by
     */
    bool (*reconnected)(ov_app *app, int fd, void *userdata);

  } callback;

} ov_app_socket_config;

/*----------------------------------------------------------------------------*/

struct ov_app {

  uint16_t magic_byte;
  uint16_t type;

  ov_app_config config;

  ov_app *(*free)(ov_app *self);

  struct {

    /*
     *      Socket open is acync and will deliver the
     *      opened socket fd over a success callback.
     *
     *      @returns true   if opening is triggered
     *      @returns false  if opening failed
     *
     *      @NOTE in case of a server socket, true means the
     *      socket is opened.
     */

    bool (*open)(ov_app *self, ov_app_socket_config config,
                 void (*optional_success_callback)(int socket, void *self),
                 void (*optional_failure_callback)(int socket, void *self));

    bool (*close)(ov_app *self, int socket);

    bool (*add)(ov_app *self, ov_app_socket_config config, int socket);

  } socket;

  struct {

    bool (*close)(ov_app *self, const char *uuid);

    /*
     *      @NOTE This function is expected to use
     *      some async behaviour and close
     *      all loops within some next eventloop run.
     *      It MUST be implemented to allow the caller
     *      to call it and let the calling function finish,
     *      before the connections are closed.
     *
     *      @returns true if the close was scheduled
     */
    bool (*close_all)(ov_app *self);

    int (*get_socket)(ov_app *self, const char *uuid);

  } connection;

  /*
   *      Custom send function
   *
   *      data MUST fit with the parser of the socket,
   *      OR be of type ov_buffer.
   */

  bool (*send)(ov_app *self, int socket, const ov_socket_data *remote,
               void *data);
};

/*----------------------------------------------------------------------------*/

/**
        Create the default APP.
*/
ov_app *ov_app_create(ov_app_config config);

/*----------------------------------------------------------------------------*/

ov_app *ov_app_cast(const void *self);

/*---------------------------------------------------------------------------*/

/**
        Function interface for free, calling the internal free function.
        (if available)
*/
void *ov_app_free(void *self);

/*----------------------------------------------------------------------------*/

bool ov_app_open_socket(ov_app *self, ov_app_socket_config config,
                        void (*success)(int socket, void *self),
                        void (*failure)(int socket, void *self));

/*---------------------------------------------------------------------------*/

bool ov_app_close_socket(ov_app *self, int socket);

/*----------------------------------------------------------------------------*/

bool ov_app_send(ov_app *app, int socket, const ov_socket_data *remote,
                 void *data);

/*----------------------------------------------------------------------------*/

bool ov_app_stop(ov_app *app);

/******************************************************************************
 *                                  HELPERS
 ******************************************************************************/

typedef struct {

  const char *config_file;
  bool version_request;

} ov_app_parameters;

/**
 * This optstring is ALWAYS expected.
 * If you try to parse command line options using getopt(3),
 * ensure
 * 1. That you reset `optind` to 1
 * 2. That your optstring should contain OV_APP_DEFAULT_OPTSTRING
 *
 * That is, your code should look like
 *
 * optind = 1;
 * char const *my_options = OV_APP_DEFAULT_OPTSTRING "ad:";
 *
 * while(getopt(argc, argv, my_options ...)) {...
 */
#define OV_APP_DEFAULT_OPTSTRING "c:v"

extern const char *UNKNOWN_ARGUMENT_PRESENT;

const char *
ov_app_parse_command_line_optargs(int argc, char *const argv[],
                                  ov_app_parameters *restrict params,
                                  char const *optargs);

const char *ov_app_parse_command_line(int argc, char *const argv[],
                                      ov_app_parameters *restrict params);

/**
 * Open a socket.
 * For a client connection (currently TCP only) reconnections are
 * supported:
 * If a reconnect callback is set, the connection will be re-connected
 * if lost.
 * The connection is established asynchronously, so the connection will
 * continue to be tried to be established.
 *
 * As soon as the connection has been (re-) established, the reconnect
 * callback is triggered.
 */
bool ov_app_connect(ov_app *self, ov_app_socket_config config);

#endif /* ov_app_h */
