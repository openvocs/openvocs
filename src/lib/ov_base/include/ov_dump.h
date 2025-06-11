/***
        ------------------------------------------------------------------------

        Copyright 2017 German Aerospace Center DLR e.V. (GSOC)

        Licensed under the Apache License, Version 2.0 (the "License");
        you may not use this file except in compliance with the License.
        You may obtain a copy of the License at

                http://www.apache.org/licenses/LICENSE-2.0

        Unless required by applicable law or agreed to in writing, software
        distributed under the License is distributed on an "AS IS" BASIS,
        WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
        See the License for the specific language governing permissions and
        limitations under the License.

        This file is part of the openvocs project. http://openvocs.org

        ------------------------------------------------------------------------
*//**
        @file           ov_dump.h
        @author         Markus Toepfer
        @author         Michael Beer

        @date           2017-12-10

        @ingroup        ov_base

        @brief          Definition of dumps.


        ------------------------------------------------------------------------
*/
#ifndef ov_dump_h
#define ov_dump_h

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>

#include <string.h>

#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/types.h>

/*----------------------------------------------------------------------------*/

/**
        Dump binary as hex to stream.
        @return true on success, false on error
 */
bool ov_dump_binary_as_hex(FILE *stream, uint8_t *binary, uint64_t length);

/*----------------------------------------------------------------------------*/

/**
        Dump the content of the addrinfo to stream.
*/
bool ov_dump_socket_addrinfo(FILE *stream, struct addrinfo *info);

/*---------------------------------------------------------------------------*/

/**
        Dump the content of the sockaddr to stream.
*/
bool ov_dump_socket_sockaddr(FILE *stream, struct sockaddr *addr);

/*---------------------------------------------------------------------------*/

/**
        Dump the content of the sockaddr_storage to stream.
*/
bool ov_dump_socket_sockaddr_storage(FILE *stream,
                                     struct sockaddr_storage *addr);

/*---------------------------------------------------------------------------*/

/**
        Dump the content of the sockaddr_in to stream.
*/
bool ov_dump_socket_sockaddr_in(FILE *stream, struct sockaddr_in *addr);

/*---------------------------------------------------------------------------*/

/**
        Dump the content of the sockaddr_in6 to stream.
*/
bool ov_dump_socket_sockaddr_in6(FILE *stream, struct sockaddr_in6 *addr);

#endif /* ov_dump_h */
