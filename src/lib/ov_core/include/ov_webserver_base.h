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
        @file           ov_webserver_base.h
        @author         Markus Töpfer

        @date           2020-12-07

        Implementation of a base functionality for webserver implementations
        with SSL Server Name Indication multiplexing. (SNI)

        This implementation MAY be used for webserver implementations as IO
        layer for HTTPS and WSS messaging. It includes some integrated redirect
        from some (optional) HTTP socket, based on the config, but is intendent
        to be used SECURE ONLY otherwise.

        Funtionality:

            1)  HTTPS and WSS are multiplexed on the same port using the same
                certificate. WSS will instantiate on a Websocket upgrade,
                otherwise the connection will be served over the configured
                HTTPS callback

            2)  SNI multiplexing is done on the same socket, so the instance
                is able to serve certificates requested for some specific
                domain, but is not implementing ANY http methods.

                Methods MUST be implemented in the configured HTTPS callback.
                It is safe to use the HTTP Header host, to route the request,
                as the host header is verified against the SSL request header
                within this implementation.

                ov_webserver_base is method transparent and NOT evaluating ANY
                http specific message handling. (except http to https redirect)

            3)  Websocket callbacks MAY be set uri based. For websockets some
                request routing is implemented, as websocket frames will not
                contain any information about the requested URI or domain once
                upgraded. Websocket callbacks will be served per URI and fall
                back to some generic callback, which may be set to serve all
                uris of some domain. (domain based instead of uri based access)

        NOTE Implemented functionality is minimal to serve the stated
        functionality of HTTPS / WSS multiplexing in combination with SNI.

        NOTE This is NOT a full webserver implementation, but MAY be the base
        for some webserver implementation which is required to support both:
        SNI as well as websocket multiplexing over a single port.

        ------------------------------------------------------------------------
*/
#ifndef ov_webserver_base_h
#define ov_webserver_base_h

#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>

#include <ov_base/ov_config_keys.h>
#include <ov_base/ov_event_loop.h>
#include <ov_base/ov_json.h>
#include <ov_base/ov_socket.h>
#include <ov_base/ov_uri.h>

#include <ov_format/ov_file_format.h>

#include "ov_domain.h"
#include "ov_event_io.h"
#include "ov_http_pointer.h"
#include "ov_websocket_message.h"
#include "ov_websocket_pointer.h"

#define OV_WEBSERVER_BASE_MAGIC_BYTE 0x200A
#define OV_WEBSERVER_BASE_NAME_MAX 255
#define OV_WEBSERVER_BASE_MAX_STUN_LISTENER 10

#define OV_WEBSERVER_BASE_DOMAIN_CONFIG_PATH_DEFAULT                           \
    "/etc/openvocs/ov_webserver/domains"

typedef struct ov_webserver_base ov_webserver_base;
typedef struct ov_webserver_base_config ov_webserver_base_config;

/*----------------------------------------------------------------------------*/

struct ov_webserver_base_config {

    bool debug;
    bool ip4_only;

    char name[OV_WEBSERVER_BASE_NAME_MAX];
    char domain_config_path[PATH_MAX];

    struct {

        uint32_t max_sockets;
        uint32_t max_content_bytes_per_websocket_frame;

    } limit;

    ov_http_message_config http_message;
    ov_websocket_frame_config websocket_frame;

    ov_event_loop *loop;

    struct {

        ov_socket_configuration redirect; // default 80 HTTP redirect only
        ov_socket_configuration secure;   // default 443 HTTPS

    } http;

    struct {

        ov_socket_configuration socket[OV_WEBSERVER_BASE_MAX_STUN_LISTENER];

    } stun;

    struct {

        void *userdata;

        bool (*accept)(void *userdata, int server_socket, int accepted_socket);

        /*
         *  This callback SHOULD be set to process incoming HTTPs messages.
         *
         *  NOTE msg MUST be freed by the callback handler.
         *  NOTE an error return will close the connection socket
         *  NOTE websocket upgrades are already processed within
         * ov_webserver_base NOTE this function will get the buffer as read from
         * the wire and validated as some valid HTTP content with all pointer of
         * the msg pointing to the data read. Nothing is copied, but data is
         * preparsed.
         */
        bool (*https)(void *userdata, int connection_socket,
                      ov_http_message *msg);

        /*
         * Close callback for connection_sockets to
         * cleanup connection based settings in userdata.
         */
        void (*close)(void *userdata, int connection_socket);

    } callback;

    struct {

        /*
         *  Timeout checks will be done at ANY accept_to_io_timeout_usec
         *  and timeout all connection with io_timeout_usec without input.
         *
         *  NOTE io_timeout_usec of 0 will NOT timeout ANY IDLE connection,
         *  ONCE some initial data was received after accept_to_io_timeout_usec.
         *
         *  NOTE timeout resolution MAY be implemented dependent of
         *  accept_to_io_timeout_usec
         */

        /*  Default IO timeout
         *  between IO messages
         *  (may be 0 for no timeout) */
        uint64_t io_timeout_usec;

        /*  Default IO timeout
         *  between ACCEPT and ANY IO message
         *  (may be 0 for default of implementation e.g. 1 sec) */
        uint64_t accept_to_io_timeout_usec;

    } timer;
};

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_webserver_base *ov_webserver_base_create(ov_webserver_base_config config);

/*----------------------------------------------------------------------------*/

ov_webserver_base *ov_webserver_base_free(ov_webserver_base *data);

/*----------------------------------------------------------------------------*/

/*  Close some socket within the webserver */
bool ov_webserver_base_close(ov_webserver_base *self, int socket);

/*
 *      ------------------------------------------------------------------------
 *
 *      SEND FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

/**
    Send on a connected secure socket.

    Valid data input is a pointer to some allocated ov_buffer. If the buffer
    will be send immediately, the buffer will be cleaned.
    If sending immediately is NOT possible, *buffer will be set to NULL and
    data lifetime management will be transfered to ov_webserver_base.
    (the buffer will be unplugged instead of copied)

    NOTE the function will only send on secured handshaked connection sockets

    NOTE Keep in mind ov_webserver_base is NOT intendet to be used with any TCP
    sockets, but with HTTPS and WSS connection sockets.

    NOTE only TCP functionality implemented in ov_webserver_base is some
    redirect at the HTTP socket to the HTTPS socket

    USAGE e.g.

        ov_webserver_base_send(self, socket, &websocket_frame->buffer);
        ov_webserver_base_send(self, socket, &http_message->buffer);
        ov_webserver_base_send(self, socket, &some_buffer);

    @param self     instance
    @param socket   connection socket
    @param data     pointer to buffer to send

    @returns true if sending of the buffer is done or triggered

    NOTE it is ensured all data of the buffer will be send up to buffer->length
*/
bool ov_webserver_base_send_secure(ov_webserver_base *self, int socket,
                                   ov_buffer const *const data);

/*----------------------------------------------------------------------------*/

/**
    Send some JSON over WSS sockets.

    @param self     instance
    @param socket   connection socket
    @param data     json message to send
*/
bool ov_webserver_base_send_json(ov_webserver_base *self, int socket,
                                 ov_json_value const *const data);

/*
 *      ------------------------------------------------------------------------
 *
 *      METHOD GET implementation
 *
 *      ------------------------------------------------------------------------
 */

/**
    Answer some GET request with a standard file send procedure.

    @param self     instance pointer
    @param socket   socket connection
    @param fmt      prepared format with mime type parameter
    @param request  request received

    @returns true if the send process was started.
*/
bool ov_webserver_base_answer_get(ov_webserver_base *self, int socket,
                                  ov_file_format_desc fmt,
                                  const ov_http_message *request);

/*----------------------------------------------------------------------------*/

/**
    Answer some HEAD request with a standard file send procedure.

    @param self     instance pointer
    @param socket   socket connection
    @param fmt      prepared format with mime type parameter
    @param request  request received

    @returns true if the send process was started.
*/
bool ov_webserver_base_answer_head(ov_webserver_base *srv, int socket,
                                   ov_file_format_desc fmt,
                                   const ov_http_message *request);

/*
 *      ------------------------------------------------------------------------
 *
 *      DOMAIN FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

/**
    Ceate the file path for some request.

    @param self     instance pointer
    @param socket   request socket, required to find domain document root
    @param request  received request
    @param length   length of path (SHOULD be PATH_MAX)
    @param path     pointer to buffer to fill with path

    @returns true, if the request is within the limits of the document root
    and the path for the file was writen.
*/
bool ov_webserver_base_uri_file_path(ov_webserver_base *self, int socket,
                                     const ov_http_message *request,
                                     size_t length, char *path);

/*----------------------------------------------------------------------------*/

ov_domain *ov_webserver_base_find_domain(const ov_webserver_base *srv,
                                         const ov_memory_pointer hostname);

/*
 *      ------------------------------------------------------------------------
 *
 *      CONFIG FUNCTIONS
 *
 *      TBD config to be changed to different (or additional) config format
 *      (JSON is not optimal due to support of non ASCII domain names!)
 *
 *      ------------------------------------------------------------------------
 */

/**
    Configure some websocket callback for a domain.

    @param self         instance pointer
    @param hostname     hostname as used in domain config and HTTP requests
    @param config       config for websocket message callbacks

    NOTE if config.userdata == NULL all callbacks will be unset
    NOTE if config.userdata is set and config.callback == NULL
        (a) if no uri is set, the callback for the domain will be unset
        (b) if uri is set, the callback for the uri will be unset

    @returns false if no domain with hostname is configured.
*/
bool ov_webserver_base_configure_websocket_callback(
    ov_webserver_base *self, const ov_memory_pointer hostname,
    ov_websocket_message_config config);

/*----------------------------------------------------------------------------*/

/**
    Configure some WSS JSON event handler for some domain.
    config.name is some required input and will become the URI for the events.

    @param self         instance pointer
    @param hostname     hostname as used in domain config and HTTP requests
    @param config       event handler configuration.
    @param wss_io       (optional) websocket layer IO plugin which will be
                        called after websocket IO processing

    NOTE if config.userdata == NULL all callbacks will be unset
    NOTE if config.callback.process is not set and config.name is set,
    the event for the name (URI) will be disabled.
*/
bool ov_webserver_base_configure_uri_event_io(
    ov_webserver_base *self, const ov_memory_pointer hostname,
    const ov_event_io_config config, const ov_websocket_message_config *wss_io);

/*----------------------------------------------------------------------------*/

ov_webserver_base_config
ov_webserver_base_config_from_json(const ov_json_value *value);

/*----------------------------------------------------------------------------*/

ov_json_value *
ov_webserver_base_config_to_json(ov_webserver_base_config config);

/*----------------------------------------------------------------------------*/

#endif /* ov_webserver_base_h */
