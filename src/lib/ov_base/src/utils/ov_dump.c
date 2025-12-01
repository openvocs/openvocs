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
        @file           ov_dump.c
        @author         Markus Toepfer
        @author         Michael Beer

        @date           2017-12-10

        @ingroup        ov_basics

        @brief


        ------------------------------------------------------------------------
*/
#include "../../include/ov_dump.h"

/*---------------------------------------------------------------------------*/

bool ov_dump_binary_as_hex(FILE *stream, uint8_t *binary, uint64_t length) {

  uint64_t i = 0;

  if ((!binary) || (!length))
    goto error;

  if (!stream)
    goto error;

  for (i = 0; i < length; i++) {
    fprintf(stream, " %02x", binary[i]);
  }

  return true;

error:
  return false;
}

/*---------------------------------------------------------------------------*/

bool ov_dump_socket_addrinfo(FILE *stream, struct addrinfo *info) {

  if (!stream || !info)
    goto error;

  if (!fprintf(stream,
               "flags          %i\n"
               "family         %i\n"
               "type           %i\n"
               "protocol       %i\n"
               "sock_addr_len  %i\n"
               "canonname      %s\n"
               "next           %p\n",
               info->ai_flags, info->ai_family, info->ai_socktype,
               info->ai_protocol, info->ai_addrlen, info->ai_canonname,
               info->ai_next))
    goto error;

  if (info->ai_addr)
    return ov_dump_socket_sockaddr(stream, info->ai_addr);

  if (fprintf(stream, "ai_addr          (null)\n"))
    return true;

error:
  return false;
}

/*---------------------------------------------------------------------------*/

bool ov_dump_socket_sockaddr(FILE *stream, struct sockaddr *addr) {

  if (!stream || !addr)
    goto error;

  if (addr->sa_family == AF_INET) {

    return ov_dump_socket_sockaddr_in(stream, (struct sockaddr_in *)addr);

  } else if (addr->sa_family == AF_INET6) {

    return ov_dump_socket_sockaddr_in6(stream, (struct sockaddr_in6 *)addr);
  }

error:
  return false;
}

/*---------------------------------------------------------------------------*/

bool ov_dump_socket_sockaddr_storage(FILE *stream,
                                     struct sockaddr_storage *addr) {

  if (!stream || !addr)
    goto error;

  if (addr->ss_family == AF_INET) {

    return ov_dump_socket_sockaddr_in(stream, (struct sockaddr_in *)addr);

  } else if (addr->ss_family == AF_INET6) {

    return ov_dump_socket_sockaddr_in6(stream, (struct sockaddr_in6 *)addr);
  }

error:
  return false;
}

/*---------------------------------------------------------------------------*/

bool ov_dump_socket_sockaddr_in(FILE *stream, struct sockaddr_in *addr) {

  if (!stream || !addr)
    goto error;

  char dest[INET6_ADDRSTRLEN] = {0};

  if (0 == inet_ntop(AF_INET, &addr->sin_addr, dest, INET6_ADDRSTRLEN))
    goto error;

  if (!fprintf(stream,
               "sin_family     %i\n"
               "sin_port       %hu\n"
               "IP             %s\n",
               addr->sin_family, ntohs(addr->sin_port), dest))
    goto error;

  return true;
error:
  return false;
}

/*---------------------------------------------------------------------------*/

bool ov_dump_socket_sockaddr_in6(FILE *stream, struct sockaddr_in6 *addr) {

  if (!stream || !addr)
    goto error;

  char dest[INET6_ADDRSTRLEN] = {0};

  if (0 == inet_ntop(AF_INET6, &addr->sin6_addr, dest, INET6_ADDRSTRLEN))
    goto error;

  if (!fprintf(stream,
               "sin6_family    %i\n"
               "sin6_port      %u\n"
               "sin6_flowinfo  %u\n"
               "sin6_scope_id  %u\n"
               "IP             %s\n",
               addr->sin6_family, ntohs(addr->sin6_port), addr->sin6_flowinfo,
               addr->sin6_scope_id, dest))
    goto error;

  return true;
error:
  return false;
}
