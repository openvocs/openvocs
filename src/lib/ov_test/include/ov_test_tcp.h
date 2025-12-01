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

        @author         Michael J. Beer

        ------------------------------------------------------------------------
*/
#ifndef OV_TEST_TCP_H
#define OV_TEST_TCP_H
/*----------------------------------------------------------------------------*/

#include <stdbool.h>
#include <sys/types.h>

int ov_test_get_random_port();

/**
 * Disable Nagl's algorithm on fd - only for fds referring to TCP sockets
 */
int ov_test_tcp_disable_nagls_alg(int fd);

/**
 * Read an integer from an fd.
 * Checks for errors, EOF etc.
 * @return Number of read octets. 0 indicates EOF (TCP connection closed at
 * least partially)
 */
int ov_test_tcp_read_int(int fd, int *target);

/*----------------------------------------------------------------------------*/

bool ov_test_tcp_send(int fd, char *msg);

/*----------------------------------------------------------------------------*/

/**
 * Returns true if `msg` can be read from `fd`.
 */
bool ov_test_tcp_received(int fd, char const *msg);

/*----------------------------------------------------------------------------*/

typedef struct {
  void (*serve_client_callback)(int fd, void *userdata);
  void *userdata;

} ov_test_tcp_server_config;

/*----------------------------------------------------------------------------*/

/**
 * Callback for incoming connections.
 * Sends back everything it receives.
 * If it receives "quit", it terminates the connection.
 */
void ov_test_tcp_server_loopback(int fd, void *userdata);

/*----------------------------------------------------------------------------*/

/**
 * Spawn a new process that binds to an arbitrary TCP port and listens for
 * incoming connections.
 * Works on each connection sequentially.
 * If connection accepted, calls `serve_client_callback`.
 * By default, if serve_client_callback not given, just reads from new conn.
 * until no more data, then terminates.
 *
 * If you want a reflecting server, just give
 *
 * .serve_client_callback = ov_test_tcp_server_loopback
 *
 * in the config.
 *
 */
bool ov_test_tcp_server(pid_t *server_pid, int *server_port,
                        ov_test_tcp_server_config cfg);

/*----------------------------------------------------------------------------*/

bool ov_test_tcp_stop_process(pid_t pid);

/*----------------------------------------------------------------------------*/
#endif
