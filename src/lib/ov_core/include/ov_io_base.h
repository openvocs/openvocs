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
        @file           ov_io_base.h
        @author         Markus Töpfer

        @date           2021-01-30

        Some basic IO abstraction laver for connection based sockets.

        This layer will abstract event loop based socket IO and actual
        IO to process. It contains some socket connection management including
        automated reconnects and MAY be used as IO layer independent of the
        security reqirements.

        Using this interface for IO allows to switch between socket types TCP,
        TLS and LOCAL without any changes in the processing chain by
        configuration only.

        It is able to manage Listener, as well as connection sockets.

        ------------------------------------------------------------------------
*/
#ifndef ov_io_base_h
#define ov_io_base_h

#define OV_IO_BASE_MAGIC_BYTE 0xBA10
#define OV_IO_BASE_KEY "io_base"
#define OV_IO_BASE_NAME_MAX 255
#define OV_IO_BASE_DEFAULT_BLOCK_SIZE 1024
#define OV_IO_BASE_DEFAULT_THREADLOCK_TIMEOUT_USEC 100000
#define OV_IO_BASE_DEFAULT_ACCEPT_TIMEOUT_USEC 10 * 1000 * 1000

typedef struct ov_io_base ov_io_base;
typedef struct ov_io_base_config ov_io_base_config;

#include <stdbool.h>

#include <ov_base/ov_event_loop.h>
#include <ov_base/ov_memory_pointer.h>
#include <ov_base/ov_socket.h>

/*----------------------------------------------------------------------------*/

typedef struct ov_io_base_statistics {

    uint64_t created_usec;

    ov_socket_transport type;
    ov_socket_data remote;

    struct {

        uint64_t bytes;
        uint64_t last_usec;

    } recv;

    struct {

        uint64_t bytes;
        uint64_t last_usec;

    } send;

} ov_io_base_statistics;

/*----------------------------------------------------------------------------*/

typedef struct ov_io_base_listener_config {

    ov_socket_configuration socket;

    struct {

        void *userdata;

        bool (*accept)(void *userdata, int listener, int connection);

        /**
         * @return true if data data in buffer was consumed, false if callback
         * should be called again with same data
         */
        bool (*io)(void *userdata, int connection,
                   const ov_memory_pointer buffer);

        void (*close)(void *userdata, int connection);

    } callback;

} ov_io_base_listener_config;

/*----------------------------------------------------------------------------*/

typedef struct ov_io_base_connection_config {

    ov_io_base_listener_config connection;

    uint64_t client_connect_trigger_usec;

    struct {

        /* SSL client data */

        char domain[PATH_MAX]; // hostname to use in handshake

        struct {

            char file[PATH_MAX]; // path to CA verify file
            char path[PATH_MAX]; // path to CAs to use

        } ca;

    } ssl;

    bool auto_reconnect;
    void (*connected)(void *userdata, int socket, bool result);

} ov_io_base_connection_config;

/*----------------------------------------------------------------------------*/

struct ov_io_base_config {

    bool debug;

    /* Switch off IO buffering and prevent double copies
       Unfortunately, for compatibility, this flag needs to be off by default,
       hence the weird semantics */
    bool dont_buffer;

    char name[OV_IO_BASE_NAME_MAX];

    /* !!! LEVEL TRIGGERED EVENTLOOP ONLY !!! */
    ov_event_loop *loop;

    struct {

        uint32_t max_sockets;
        uint64_t max_send; // may be used to limit max send size
        uint64_t thread_lock_timeout_usec;

    } limit;

    struct {

        /*  Timeout checks will be done at ANY accept_to_io_timeout_usec
         *  and timeout all connection with io_timeout_usec without input.
         *
         *  NOTE io_timeout_usec of 0 will NOT timeout ANY IDLE connection,
         *  ONCE some initial data was received after accept_to_io_timeout_usec.
         *
         *  NOTE timeout resolution is implemented dependent on
         *  accept_to_io_timeout_usec to use exactly ONE timer_fd for all
         *  timeout checks.
         *  (Exception: TLS client handshakes, which will run faster and use
         *   some custom timer per connection during the handshake phase) */

        /*  Default IO timeout between IO messages
         *  (may be 0 for no timeout) */
        uint64_t io_timeout_usec;

        /*  Default IO timeout between ACCEPT and FIRST IO message
         *  (0 => default timeout, NO timeout is not supported for accept) */
        uint64_t accept_to_io_timeout_usec;

        /*  Default reconnect interval, which will be executed in each
         *  accept_to_io_timeout_usec interval, so the min resolution is
         *  accept_to_io_timeout_usec! May run slower, but will be checked
         *  ANY accept_to_io_timeout_usec. So it SHOULD be some multiple of
         *  the accept timeout. */
        uint64_t reconnect_interval_usec;

    } timer;

    struct {

        size_t capacity;

    } cache;
};

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_io_base *ov_io_base_create(ov_io_base_config config);

/*----------------------------------------------------------------------------*/

ov_io_base *ov_io_base_free(ov_io_base *data);

/*
 *      ------------------------------------------------------------------------
 *
 *      CONFIG FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_io_base_config ov_io_base_config_from_json(const ov_json_value *input);
ov_json_value *ov_io_base_config_to_json(const ov_io_base_config config);

/*----------------------------------------------------------------------------*/

/**
    Load (D)TLS domain configs from path.

    NOTE This function is intended to be run in the main.
    If this will be run in a thread, the eventloop MUST be threadsafe

    @param base     instance pointer
    @param json     pointer to some json instance
    @param path     path to domain config
*/
bool ov_io_base_load_ssl_config(ov_io_base *base, const char *path);

/*----------------------------------------------------------------------------*/

/**
    Switch on/off debug logging.

    NOTE This function may be run in a thread.

    @param self     instance pointer
    @param on       switch to be set
*/
bool ov_io_base_debug(ov_io_base *self, bool on);

/*
 *      ------------------------------------------------------------------------
 *
 *      SOCKET FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

/**
    NOTE supports only connection based listener sockets with the socket as
    connection identifier e.g. TCP, TLS, LOCAL but not UDP and DTLS

    @param self     instance pointer
    @param config   config to be set

    @returns listener socket OR -1 on error
*/
int ov_io_base_create_listener(ov_io_base *self,
                               ov_io_base_listener_config config);

/*----------------------------------------------------------------------------*/

/**
    NOTE supports only connection based listener sockets with the socket as
    connection identifier e.g. TCP, TLS, LOCAL but not UDP and DTLS

    @param self     instance pointer
    @param config   config to be set

    @returns listener socket OR -1 on error

    Unfortunately, this method treats "cannot connect" as error, even if
    reconnects are enabled - leaving us with no possibilty to distinguish
    real errors from 'could not connect immediately'.
*/
int ov_io_base_create_connection(ov_io_base *self,
                                 ov_io_base_connection_config config);

/*----------------------------------------------------------------------------*/

/**
    Close any socket.

    Socket may be some listener or connection socket. Listener socket close
    will result in connection close for all sockets accepted by the listener.

    @param self     instance pointer
    @param socket   socket to be closed

    NOTE some connection socket created using ov_io_base_create_connection
    will no longer try to auto reconnect!

    @returns true if the socket was closed OR not present within the IO layer.
*/
bool ov_io_base_close(ov_io_base *self, int socket);

/*----------------------------------------------------------------------------*/

/**
    Get statistics for some connection socket.

    @param self     instance pointer
    @param socket   socket id

    @returns ov_io_base_statistics

    NOTE if statistics.created == 0 there is some error. The statistic will be
    initialized for each connection after acceptance with a created timestamp.
*/
ov_io_base_statistics ov_io_base_get_statistics(ov_io_base *self, int socket);

/*
 *      ------------------------------------------------------------------------
 *
 *      IO FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

/**
    Send IO from socket.

    NOTE This function is threadsafe and will fail if the connection
    cannot be locked.

    This function will perform actual IO (sending), whenever the event loop is
    ready to do so. The input buffer is queued within some internal queue and
    send as soon as possible.

    @param self     instance pointer
    @param socket   socket to be used
    @param buffer   buffer to be send

    @returns true, if some buffer was added to the send queue
*/
bool ov_io_base_send(ov_io_base *self, int socket,
                     const ov_memory_pointer buffer);

#endif /* ov_io_base_h */
