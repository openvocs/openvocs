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
        @file           ov_socket.h
        @author         Michael Beer
        @author         Markus Toepfer

        @date           2018-12-14

        @ingroup        ov_socket

        @brief          Definition of a ov_base socket interface.


        ------------------------------------------------------------------------
*/
#ifndef ov_socket_h
#define ov_socket_h

#include <limits.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "ov_constants.h"
#include "ov_json.h"

#define OV_5TUPLE_LEN 2 * OV_HOST_NAME_MAX + 20

/*----------------------------------------------------------------------------*/

typedef enum {

    NETWORK_TRANSPORT_TYPE_ERROR = 0,
    TCP,
    UDP,
    TLS,
    DTLS,
    LOCAL,
    NETWORK_TRANSPORT_TYPE_OOB

} ov_socket_transport;

/*---------------------------------------------------------------------------*/

typedef struct {

    char host[OV_HOST_NAME_MAX];
    uint16_t port;

    ov_socket_transport type;

} ov_socket_configuration;

/*---------------------------------------------------------------------------*/

typedef struct {

    int gai; // carry gai_error
    int err; // carry errno

} ov_socket_error;

/*---------------------------------------------------------------------------*/

/**
        ov_socket_data may be used to pass the sockaddr_storage,
        as well as parsed host and port data as pointer between functions.
*/
typedef struct {

    struct sockaddr_storage sa;

    char host[OV_HOST_NAME_MAX]; // parsed host from sa
    uint16_t port;               // parsed port from sa

} ov_socket_data;

/*---------------------------------------------------------------------------*/

/*
        Openvocs standard method to open or connect a socket.

        @EXAMPLE to create an IPv4/IPv6 (depending on 'host' string
        format .. ) UDP server socket

                r = ov_socket_create( (ov_socket_configuration) {

                                                .type = UDP
                                        },
                                        false,
                                        NULL );

        @param config           configuration to be used
        @param as_client        set to true if the socket shall be connected
        @param err_return       (optional) container for error messages
        @returns                on success (socket created)
                                on error -1
*/
int ov_socket_create(ov_socket_configuration config, bool as_client,
                     ov_socket_error *err_return);

/*
 *      ------------------------------------------------------------------------
 *
 *      SOCKET OPTIONS
 *
 *      ------------------------------------------------------------------------
 */

/**
    Get the send buffer size of some socket.

    @returns -1 on error
*/
int ov_socket_get_send_buffer_size(int socket);

/*---------------------------------------------------------------------------*/

/**
    Get the recv buffer size of some socket.

    @returns -1 on error
*/
int ov_socket_get_recv_buffer_size(int socket);

/*
 *      ------------------------------------------------------------------------
 *
 *      GET CONFIG FROM SOCKET
 *
 *      ------------------------------------------------------------------------
 */

/**
        Get the data from a sockaddr_storage struct and convert it to
        ov_socket_config.
*/
bool ov_socket_config_from_sockaddr_storage(const struct sockaddr_storage *sa,
                                            ov_socket_configuration *config,
                                            ov_socket_error *err_return);

/*---------------------------------------------------------------------------*/

/**
        Get ov_socket_config parameter of a socket.

        @NOTE this function is not able to parse any data of the server section.

        @param socket_fd        socket to retrieve information from
        @param local            (optional) config for the local end
        @param remote           (optional) config for the remote end
        @param err_return       (optional) container for error messages
        @returns                true if provided configs was filled
*/
bool ov_socket_get_config(int socket_fd, ov_socket_configuration *local,
                          ov_socket_configuration *remote,
                          ov_socket_error *err_return);

/*---------------------------------------------------------------------------*/

/**
        Get struct sockaddr_storage parameter of a socket.

        This function is a wrapper around getpeername and getsockname.

        @param socket_fd        socket to retrieve information from
        @param local            (optional) sa for the local end
        @param remote           (optional) sa for the remote end
        @param err_return       (optional) container for error messages
        @returns                true if provided configs was filled
*/

bool ov_socket_get_sockaddr_storage(int fd, struct sockaddr_storage *local,
                                    struct sockaddr_storage *remote,
                                    ov_socket_error *err_return);

/*
 *      ------------------------------------------------------------------------
 *
 *      GET CONFIG FROM JSON
 *
 *      ------------------------------------------------------------------------
 */

/**
        Parse a json object into a socket configuration.

        Expected JSON input

                {
                        "type" : "UDP",
                        "port" : 1234,
                        "host" : "localhost"
                }
*/
ov_socket_configuration
ov_socket_configuration_from_json(const ov_json_value *object,
                                  const ov_socket_configuration default_values);

/*---------------------------------------------------------------------------*/

/**
        Create a JSON object based on config.
        This function has 2 modes,

                (1) Fill an existing object pointer at *object
                (2) Create and fill a new object pointer *object

        The object created / filled will look like

                {
                        "type" : "UDP",
                        "port" : 1234,
                        "host" : "localhost"
                }

        @param config   config used to create the JSON object config
   will be used
        @param object   pointer to pointer of existing JSON object (FILL MODE)
                        or to NULL (CREATE MODE)
 */
bool ov_socket_configuration_to_json(const ov_socket_configuration config,
                                     ov_json_value **object);

/*----------------------------------------------------------------------------*/

bool ov_socket_configuration_equals(ov_socket_configuration cfg1,
                                    ov_socket_configuration cfg2);

/*---------------------------------------------------------------------------*/

ov_json_value *ov_socket_data_to_json(const ov_socket_data *data);

/*---------------------------------------------------------------------------*/

ov_socket_data ov_socket_data_from_json(const ov_json_value *value);

/*----------------------------------------------------------------------------*/

bool ov_socket_data_to_string(char *target, size_t target_max_len_bytes,
                              ov_socket_data const *sd);

/*
 *      ------------------------------------------------------------------------
 *
 *      SOCKADDR STORAGE ENCODING / DECODING
 *
 *      ------------------------------------------------------------------------
 */

/**
        Parse IP string and PORT out of a sockaddr_storage.

        @param sa       source  to be parsed
        @param ip       pointer to buffer to write IP
        @param ip_len   length of ip buffer
        @param port     pointer to port
 */
bool ov_socket_parse_sockaddr_storage(const struct sockaddr_storage *sa,
                                      char *ip, size_t ip_len, uint16_t *port);

/*----------------------------------------------------------------------------*/

/**
        Create ov_socket_data from a given
        sockaddr_storage
 */
ov_socket_data
ov_socket_data_from_sockaddr_storage(const struct sockaddr_storage *sa);

/*----------------------------------------------------------------------------*/

/**
        Create ov_socket_data from a given
        sockaddr_storage
 */
bool ov_socket_get_data(int socket, ov_socket_data *local,
                        ov_socket_data *remote);

/*----------------------------------------------------------------------------*/

/**
        Fill a sockaddr_storage with IP and PORT

        @param sa       pointer to sa
        @param family   IP family to create (AF_INET AF_INET&)
        @param ip       buffer of ip
        @param port     port to be used
        @return         true if the sockaddr_storage was filled with data
 */
bool ov_socket_fill_sockaddr_storage(struct sockaddr_storage *sa,
                                     sa_family_t family, const char *ip,
                                     uint16_t port);

/*
 *      ------------------------------------------------------------------------
 *
 *      HELPER FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

int ov_socket_close(int fd);

/**
        Close an unlink a local socket.

        This function will close a local socket and unlink the socket path.
*/
int ov_socket_close_local(int socket);

/*---------------------------------------------------------------------------*/

bool ov_socket_set_dont_fragment(int socket);
bool ov_socket_ensure_nonblocking(int socket);
bool ov_socket_set_reuseaddress(int socket);

bool ov_socket_disable_nagl(int socket);
bool ov_socket_disable_delayed_ack(int socket);

const char *ov_socket_transport_to_string(ov_socket_transport type);
const char *ov_socket_transport_to_string_lower(ov_socket_transport type);
ov_socket_transport ov_socket_transport_from_string(const char *string);
ov_socket_transport ov_socket_transport_parse_string(const char *string,
                                                     size_t length);

/*---------------------------------------------------------------------------*/

bool ov_socket_log_error_with_config(ov_socket_configuration config,
                                     ov_socket_error err);

/*---------------------------------------------------------------------------*/

bool ov_socket_log(int socket, const char *format, ...);

/*---------------------------------------------------------------------------*/

/**
        Create a list to store socket configurations.
*/
ov_list *ov_socket_configuration_list();

/*---------------------------------------------------------------------------*/

/**
        Generate a 5 tuple string in form:

        <localhost>:<localport><transport><remotehost>:<remoteport>
        e.g. 127.0.0.1:12345udp127.0.0.1:12346

        @param dest             pointer to buffer to be written/filled
        @param dest_len         length at *dest if *dest is set
        @param socket           local socket
        @param remote           ov_socket_date generate of remote using
                                @see ov_socket_data_from_sockaddr_storage
*/
bool ov_socket_generate_5tuple(char **dest, size_t *dest_len, int socket,
                               const ov_socket_data *remote);

/*---------------------------------------------------------------------------*/

/**
        Check if a socket is a dgram socket.
*/
bool ov_socket_is_dgram(int socket);

/*---------------------------------------------------------------------------*/

/**
        Get type of an socket e.g. SOCK_DGRAM SOCK_STREAM
*/
int ov_socket_get_type(int socket);

/*---------------------------------------------------------------------------*/

/**
        This function will check if 2 sockets are similar.
        The MUST have the same type, as well as the same transmission
        method.

        e.g.    AF_INET SOCK_STREAM == AF_INET SOCK_STREAM
                AF_INET SOCK_DGRAM  != AF_INET SOCK_STREAM
                AF_INET SOCK_DGRAM  == AF_INET SOCK_DGRAM
                AF_LOCAL SOCK_DGRAM == AF_LOCAL SOCK_DGRAM
                AF_INET6 SOCK_DGRAM != AF_INET SOCK_DGRAM

*/
bool ov_sockets_are_similar(int socket1, int socket2);

/*---------------------------------------------------------------------------*/

/**
        Get the destination type (AF_INET | AF_INET6)
        from a host.

        @param host     host to check
        @param type     type to set
*/
bool ov_socket_destination_address_type(const char *host, int *type);

/*---------------------------------------------------------------------------*/

/**
        Connect a socket to a remote destination.
        @param fd            socket fd
        @param remote   remote address
 */
bool ov_socket_connect(int fd, const struct sockaddr_storage *remote);

/*---------------------------------------------------------------------------*/

/**
       Unconnect a socket.
       @param fd            socket fd
*/
bool ov_socket_unconnect(int fd);

/*---------------------------------------------------------------------------*/

/**
 * Will try to open a server socket as described in `scfg`.
 * If scfg->port == 0, an arbitrary port will be chosen.
 * The actual socket bound to will be written into `scfg`.
 * @return the opened socket descriptor or -1 in case of error.
 */
int ov_socket_open_server(ov_socket_configuration *scfg);

/*----------------------------------------------------------------------------*/

/**
    Create some dynamic port for some socket configuration.
    This function will open some dynamic port, set the port in the configuration
    and imediately close the port.
    So this function may be used during testing to identify some unused port.

    This function is not reliable - it is not guaranteed that the port will stay
    unused ...

    It might just be replaced by choosing a random port.

    This function contains a race condition:
    As it does not actually keep the socket open, by the time you try to bind to
    the host:port returned, it might not be available any longer.

    Better use ov_socket_open_server instead.

    @param config   prepared config with host and type, port will be overriden
*/
ov_socket_configuration
ov_socket_load_dynamic_port(ov_socket_configuration config);

/*---------------------------------------------------------------------------*/

/**
    This function will return the maximum amount of supportet sockets
    of the process calling the function.

    @param sockets  (optional) user limit for sockets
*/
uint32_t ov_socket_get_max_supported_runtime_sockets(uint32_t sockets);

/*----------------------------------------------------------------------------*/

bool ov_socket_configuration_to_sockaddr(const ov_socket_configuration in,
                                         struct sockaddr_storage *sockaddr,
                                         socklen_t *sockaddr_len);

/*----------------------------------------------------------------------------*/

ov_socket_data
ov_socket_configuration_to_socket_data(const ov_socket_configuration in);

/*----------------------------------------------------------------------------*/

typedef struct {

    ov_socket_configuration peer;
    ov_socket_configuration local;
    bool connected;

} ov_socket_state;

bool ov_socket_state_from_handle(int socket_handle, ov_socket_state *state);

ov_json_value *ov_socket_state_to_json(ov_socket_state ss);

/*----------------------------------------------------------------------------*/
#endif /* ov_socket_h */
