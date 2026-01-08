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
        @file           ov_socket.c
        @author         Michael Beer
        @author         Markus Toepfer

        @date           2018-12-14

        @ingroup        ov_socket

        @brief          Implementation of a ov_base socket interface.


        ------------------------------------------------------------------------
*/

#include "../../include/ov_socket.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <ifaddrs.h>
#include <inttypes.h>
#include <netdb.h>
#include <ov_log/ov_log.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#include "../../include/ov_config_keys.h"
#include "../../include/ov_string.h"
#include "../../include/ov_utils.h"
#include "netinet/tcp.h"

#define OV_KEY_SOCKET_TYPE "type"
#define OV_KEY_SOCKET_HOST "host"
#define OV_KEY_SOCKET_PORT "port"

#define OV_SOCKET_TYPE_STRING_TCP "TCP"
#define OV_SOCKET_TYPE_STRING_UDP "UDP"
#define OV_SOCKET_TYPE_STRING_TLS "TLS"
#define OV_SOCKET_TYPE_STRING_DTLS "DTLS"
#define OV_SOCKET_TYPE_STRING_LOCAL "LOCAL"

#define OV_SOCKET_MSG_MAX 2500

/******************************************************************************
 *                                  HELPERS
 ******************************************************************************/

int open_local_client_socket(const char *path, size_t length,
                             ov_socket_error *err_return) {
  int socket_fd = -1;

  if (!path || length < 1 || length > 107)
    goto error;

  ov_socket_error e = {0};

  struct sockaddr_un addr;
  socklen_t len_sa = sizeof(addr);

  memset(&addr, 0, len_sa);

  socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (socket_fd < 0) {
    e.err = errno;
    goto error;
  }

  addr.sun_family = AF_UNIX;

  if (!strncpy(addr.sun_path, path, length))
    goto error;

  if (0 != connect(socket_fd, (struct sockaddr *)&addr, len_sa)) {
    e.err = errno;
    goto error;
  }

  return socket_fd;

error:
  if (socket_fd != -1)
    close(socket_fd);

  if (err_return)
    *err_return = e;
  return -1;
}

/*---------------------------------------------------------------------------*/

int open_local_server_socket(const char *path, size_t length,
                             ov_socket_error *err_return) {
  int socket_fd = -1;

  if (!path || length < 1 || length > 107)
    goto error;

  ov_socket_error e = {0};

  struct sockaddr_un addr;
  socklen_t len_sa = sizeof(addr);

  memset(&addr, 0, len_sa);

  socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (socket_fd < 0)
    goto error;

  addr.sun_family = AF_UNIX;

  if (!strncpy(addr.sun_path, path, length))
    goto error;

  // ensure path is not in use
  unlink(addr.sun_path);

  if (0 != bind(socket_fd, (struct sockaddr *)&addr, len_sa)) {
    e.err = errno;
    goto error;
  }

  if (0 != listen(socket_fd, SOMAXCONN)) {
    e.err = errno;
    goto error;
  }

  return socket_fd;

error:
  if (socket_fd != -1)
    close(socket_fd);

  if (err_return)
    *err_return = e;

  return -1;
}

/*---------------------------------------------------------------------------*/

int ov_socket_close(int fd) {
  if (-1 < fd) {
    close(fd);
    return -1;

  } else {
    return fd;
  }
}

/*----------------------------------------------------------------------------*/

int ov_socket_close_local(int socket) {
  if (socket < 0)
    goto error;

  struct sockaddr_un addr;
  socklen_t len = sizeof(struct sockaddr_un);
  memset(&addr, 0, len);

  int r = 0;

  r = getsockname(socket, (struct sockaddr *)&addr, &len);
  if (r < 0)
    goto close_anyway;

  if (addr.sun_family != AF_UNIX)
    goto error;

  r = unlink(addr.sun_path);
  if (r < 0)
    goto close_anyway;

  r = close(socket);
  if (r < 0)
    goto error;

  return 0;

close_anyway:

  r = close(socket);
  if (r < 0)
    goto error;

  return 1;

error:
  return -1;
}

/*---------------------------------------------------------------------------*/

int socket_create_local(ov_socket_configuration config, bool as_client,
                        ov_socket_error *err_return) {
  if (config.host[0] == 0)
    return -1;

  if (as_client)
    return open_local_client_socket(config.host, strlen(config.host),
                                    err_return);

  return open_local_server_socket(config.host, strlen(config.host), err_return);
}

/*---------------------------------------------------------------------------*/

static int setup_server_socket_nocheck(int fd,
                                       const struct addrinfo *addrinfo) {
  ov_socket_set_reuseaddress(fd);

  int backlog = 2048;

  if (0 != bind(fd, addrinfo->ai_addr, addrinfo->ai_addrlen)) {
    goto error;

  } else {
    // bind successful

    switch (addrinfo->ai_socktype) {
    case SOCK_STREAM:
    case SOCK_SEQPACKET:

      if (0 != listen(fd, backlog)) {
        goto error;
      }

      break;
    }
  }

  return 0;

error:

  return errno;
}

/*----------------------------------------------------------------------------*/

int ov_socket_create(ov_socket_configuration config, bool as_client,
                     ov_socket_error *err_return) {
  if (config.type == LOCAL)
    return socket_create_local(config, as_client, err_return);

  // reset errno
  errno = 0;
  ov_socket_error e = {0};

  char portstring[6];
  memset(portstring, 0, 6);

  struct addrinfo hints, *res = NULL;

  int fd = -1;

  if (as_client) {
    if (strlen(config.host) < 1 || config.port < 1) {
      e.err = EINVAL;
      goto error;
    }
  }

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;

  switch (config.type) {
  case TCP:
  case TLS:

    hints.ai_socktype = SOCK_STREAM;
    break;

  case UDP:
  case DTLS:

    hints.ai_socktype = SOCK_DGRAM;
    break;

  default:

    goto error;
  }

  if (config.port > 0) {
    if (!snprintf(portstring, 6, "%" PRIu16, config.port))
      goto error;
  }
  if (config.host[0] == 0) {
    strcat(config.host, "::");
  }

  if (strlen(config.host) > 0) {
    /* Problem: Synchronous.
     * Blocks until the call succeeds. Since this does network I/O,
     *  might take a long time where the thread/process is blocked.
     */
    e.gai = getaddrinfo(config.host, portstring, &hints, &res);

  } else {
    hints.ai_flags = AI_PASSIVE;
    e.gai = getaddrinfo(NULL, portstring, &hints, &res);
  }

  if (0 != e.gai)
    goto error;

  int opt = 0;
  socklen_t optlen = sizeof(opt);

  for (struct addrinfo *current = res; 0 != current;
       current = current->ai_next) {
    fd = socket(current->ai_family, current->ai_socktype, current->ai_protocol);

    // check if the socket returned is a valid fd and ok
    if (0 != getsockopt(fd, SOL_SOCKET, SO_TYPE, &opt, &optlen))
      continue;

    // check if the socket fd has some error
    if (0 != getsockopt(fd, SOL_SOCKET, SO_ERROR, &opt, &optlen))
      continue;

    // ov_socket_dump_addrinfo(stdout, current);

    if (as_client) {
      // connect a client socket

      if (0 != connect(fd, current->ai_addr, current->ai_addrlen)) {
        e.err = errno;
        continue;

      } else {
        e.err = 0;
      }

    } else {
      e.err = setup_server_socket_nocheck(fd, current);

      if (0 != e.err) {
        continue;
      }
    }

    break;
  }

  if (0 != e.err)
    goto error;

  if (res)
    freeaddrinfo(res);

  res = NULL;

  if (err_return)
    *err_return = e;

  if (fd >= 0)
    return fd;
error:
  if (err_return)
    *err_return = e;

  if (res)
    freeaddrinfo(res);

  if (fd > -1)
    close(fd);

  return -1;
}

/*---------------------------------------------------------------------------*/

int ov_socket_create_at_interface(struct ifaddrs *interface,
                                  ov_socket_configuration config,
                                  ov_socket_error *err_return) {
  if (!interface)
    goto error;

  ov_socket_error e = {0};

  switch (config.type) {
  case UDP:
  case TCP:
  case TLS:
  case DTLS:
    break;
  default:
    e.err = EINVAL;
    goto error;
  }

  memset(&config.host, 0, OV_HOST_NAME_MAX);

  int sfd;

  // get the interface IP
  e.gai = getnameinfo(interface->ifa_addr,
                      (AF_INET == interface->ifa_addr->sa_family)
                          ? sizeof(struct sockaddr_in)
                          : sizeof(struct sockaddr_in6),
                      config.host, OV_HOST_NAME_MAX, NULL, 0, NI_NUMERICHOST);

  if (e.gai != 0)
    goto error;

  sfd = ov_socket_create(config, false, &e);
  if (sfd < 0)
    goto error;

  if (err_return)
    *err_return = e;

  return sfd;
error:
  if (err_return)
    *err_return = e;
  return -1;
}

/*---------------------------------------------------------------------------*/

int ov_socket_get_send_buffer_size(int socket) {
  int size = 0;
  socklen_t len = sizeof(size);

  if (0 != getsockopt(socket, SOL_SOCKET, SO_SNDBUF, &size, &len))
    goto error;

  return size;
error:
  return -1;
}

/*---------------------------------------------------------------------------*/

int ov_socket_get_recv_buffer_size(int socket) {
  int size = 0;
  socklen_t len = sizeof(size);

  if (0 != getsockopt(socket, SOL_SOCKET, SO_RCVBUF, &size, &len))
    goto error;

  return size;
error:
  return -1;
}

/*---------------------------------------------------------------------------*/

bool ov_socket_set_dont_fragment(int socket) {
  if (socket < 0)
    goto error;

  int r = 0;
  int on = 0;

#if defined(IP_DONTFRAG)
  on = 1;
  r = setsockopt(socket, IPPROTO_IP, IP_DONTFRAG, (const void *)&on,
                 sizeof(on));
#elif defined(IP_MTU_DISCOVER)
  on = IP_PMTUDISC_DO;
  r = setsockopt(socket, IPPROTO_IP, IP_MTU_DISCOVER, (const void *)&on,
                 sizeof(on));
#else
  /* just some code using on */
  on = 1;
#endif

  if (r < 0)
    goto error;

  return true;

error:
  return false;
}

/*---------------------------------------------------------------------------*/

bool ov_socket_ensure_nonblocking(int socket) {
  int opt = 0;
  socklen_t opt_len = sizeof(opt);

  if (0 != getsockopt(socket, SOL_SOCKET, SO_TYPE, &opt, &opt_len)) {
    ov_log_error("Could not get type of socket %i errno %i | %s", socket, errno,
                 strerror(errno));
    goto error;
  }

  int flags, nflags;

  // Get the current flags
  flags = fcntl(socket, F_GETFL, 0);

  if (flags < 0)
    goto error;

  // add the non-blocking flag
  nflags = flags | O_NONBLOCK;

  // Set the new flags (force mode)
  if (fcntl(socket, F_SETFL, nflags) < 0)
    goto error;

  return true;

error:
  return false;
}

/*----------------------------------------------------------------------------*/

bool ov_socket_set_reuseaddress(int socket) {
  int opt = 1;

  int r = setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  if (0 > r)
    goto error;

  return true;

error:
  return false;
}

/*----------------------------------------------------------------------------*/

bool ov_socket_disable_nagl(int fh) {
  if (0 > fh) {
    return false;

  } else {
    int opt = 1;
    return (setsockopt(fh, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt)) == 0);
  }
}

/*----------------------------------------------------------------------------*/

bool ov_socket_disable_delayed_ack(int fh) {
  if (0 > fh) {
    return false;

  } else {
    int opt = 1;
    return (setsockopt(fh, IPPROTO_TCP, TCP_QUICKACK, &opt, sizeof(opt)) == 0);
  }
}

/*---------------------------------------------------------------------------*/

bool ov_socket_config_from_sockaddr_storage(const struct sockaddr_storage *sa,
                                            ov_socket_configuration *config,
                                            ov_socket_error *err_return) {
  ov_socket_error e = {0};

  if (!sa || !config)
    goto error;

  int port = -1;

  memset(config->host, 0, OV_HOST_NAME_MAX);

  struct sockaddr_in *sock4 = NULL;
  struct sockaddr_in6 *sock6 = NULL;
  struct sockaddr_un *so_un = NULL;

  switch (sa->ss_family) {
  case AF_INET:

    sock4 = (struct sockaddr_in *)sa;
    port = ntohs(sock4->sin_port);

    if (0 ==
        inet_ntop(AF_INET, &sock4->sin_addr, config->host, OV_HOST_NAME_MAX)) {
      e.err = errno;
      goto error;
    }

    break;

  case AF_INET6:

    sock6 = (struct sockaddr_in6 *)sa;
    port = ntohs(sock6->sin6_port);

    if (0 == inet_ntop(AF_INET6, &sock6->sin6_addr, config->host,
                       OV_HOST_NAME_MAX)) {
      e.err = errno;
      goto error;
    }

    break;

  case AF_UNIX:

    config->type = LOCAL;
    so_un = (struct sockaddr_un *)sa;
    port = 0;
    strncpy(config->host, so_un->sun_path, OV_HOST_NAME_MAX);
    break;

  default:

    return false;
  }

  if (0 > port)
    goto error;
  if (UINT16_MAX < port)
    goto error;
  config->port = port;

  return true;
error:
  if (err_return)
    *err_return = e;
  return false;
}

/*---------------------------------------------------------------------------*/

bool ov_socket_get_config(int fd, ov_socket_configuration *local,
                          ov_socket_configuration *remote,
                          ov_socket_error *err_return) {
  errno = 0;

  ov_socket_error e = {0};

  if (fd < 0)
    goto error;

  ov_socket_transport transport = NETWORK_TRANSPORT_TYPE_ERROR;

  int type = 0;
  socklen_t typelen = sizeof(type);

  if (0 != getsockopt(fd, SOL_SOCKET, SO_TYPE, &type, &typelen)) {
    e.err = errno;
    goto error;
  }

  struct sockaddr_storage sa = {0};
  socklen_t len_sa = sizeof(sa);

  if (0 != getsockname(fd, (struct sockaddr *)&sa, &len_sa)) {
    e.err = errno;
    goto error;
  }

  switch (sa.ss_family) {
  case AF_UNIX:

    transport = LOCAL;
    break;

  case AF_INET:
  case AF_INET6:

    if (type == SOCK_STREAM)
      transport = TCP;
    if (type == SOCK_DGRAM)
      transport = UDP;
    break;

  default:
    goto error;
  }

  if (0 != local) {
    local->type = transport;

    if (!ov_socket_config_from_sockaddr_storage(&sa, local, &e))
      goto error;
  }

  sa = (struct sockaddr_storage){0};

  if (0 != remote) {
    if (0 != getpeername(fd, (struct sockaddr *)&sa, &len_sa)) {
      e.err = errno;
      if (errno == ENOTCONN)
        goto done;

      goto error;
    }

    if (!ov_socket_config_from_sockaddr_storage(&sa, remote, &e))
      goto error;

    remote->type = transport;
  }

done:
  if (err_return)
    *err_return = e;
  return true;

error:
  if (err_return)
    *err_return = e;
  return false;
}

/*---------------------------------------------------------------------------*/

bool ov_socket_get_sockaddr_storage(int fd, struct sockaddr_storage *local,
                                    struct sockaddr_storage *remote,
                                    ov_socket_error *err_return) {
  errno = 0;

  ov_socket_error e = {0};

  if (fd < 0)
    goto error;

  socklen_t len_sa = 0;

  if (0 != local) {
    len_sa = sizeof(struct sockaddr_storage);
    memset(local, 0, len_sa);

    if (0 != getsockname(fd, (struct sockaddr *)local, &len_sa)) {
      e.err = errno;
      goto error;
    }
  }

  if (0 != remote) {
    len_sa = sizeof(struct sockaddr_storage);
    memset(remote, 0, len_sa);

    if (0 != getpeername(fd, (struct sockaddr *)remote, &len_sa)) {
      e.err = errno;
      if (errno == ENOTCONN)
        goto done;
      goto error;
    }
  }

done:
  if (err_return)
    *err_return = e;
  return true;

error:
  if (err_return)
    *err_return = e;
  return false;
}

/*
 *      ------------------------------------------------------------------------
 *
 *                         ENCODING / DECODING
 *
 *      ------------------------------------------------------------------------
 */

bool ov_socket_parse_sockaddr_storage(const struct sockaddr_storage *sa_master,
                                      char *ip, size_t ip_len, uint16_t *port) {
  if (!sa_master || !ip || ip_len < INET_ADDRSTRLEN || !port)
    return false;

  struct sockaddr_in *sock4 = NULL;
  struct sockaddr_in6 *sock6 = NULL;

  // IPv4 and IPv6:
  if (sa_master->ss_family == AF_INET) {
    sock4 = (struct sockaddr_in *)sa_master;
    *port = ntohs(sock4->sin_port);

    if (!inet_ntop(AF_INET, &sock4->sin_addr, ip, INET6_ADDRSTRLEN))
      return false;

  } else { // AF_INET6

    if (ip_len < INET6_ADDRSTRLEN)
      return false;

    sock6 = (struct sockaddr_in6 *)sa_master;
    *port = ntohs(sock6->sin6_port);

    if (!inet_ntop(AF_INET6, &sock6->sin6_addr, ip, INET6_ADDRSTRLEN))
      return false;
  }

  return true;
}

/*----------------------------------------------------------------------------*/

ov_socket_data
ov_socket_data_from_sockaddr_storage(const struct sockaddr_storage *sa) {
  ov_socket_data data;
  memset(&data, 0, sizeof(ov_socket_data));

  if (!sa)
    goto error;

  if (!memcpy(&data.sa, sa, sizeof(struct sockaddr_storage)))
    goto error;

  if (!ov_socket_parse_sockaddr_storage(sa, data.host, OV_HOST_NAME_MAX,
                                        &data.port))
    goto error;

  return data;
error:
  memset(&data, 0, sizeof(ov_socket_data));
  return data;
}

/*----------------------------------------------------------------------------*/

bool ov_socket_get_data(int socket, ov_socket_data *local,
                        ov_socket_data *remote) {
  if (socket < 0)
    goto error;

  struct sockaddr_storage sa_local = {0};
  struct sockaddr_storage sa_remote = {0};

  if (!ov_socket_get_sockaddr_storage(socket, &sa_local, &sa_remote, NULL))
    goto error;

  if (local)
    *local = ov_socket_data_from_sockaddr_storage(&sa_local);

  if (remote)
    *remote = ov_socket_data_from_sockaddr_storage(&sa_remote);

  return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

bool ov_socket_fill_sockaddr_storage(struct sockaddr_storage *sa,
                                     sa_family_t family, const char *ip,
                                     uint16_t port) {
  if (!sa || !ip)
    return false;

  if ((family != AF_INET) && (family != AF_INET6))
    return false;

  if (!memset(sa, 0, sizeof(struct sockaddr_storage)))
    goto error;

  int r;

  struct sockaddr_in *sock4 = NULL;
  struct sockaddr_in6 *sock6 = NULL;

  sa->ss_family = family;

  if (sa->ss_family == AF_INET) {
    sock4 = (struct sockaddr_in *)sa;
    sock4->sin_family = AF_INET;
    sock4->sin_port = htons(port);
    r = inet_pton(AF_INET, ip, &sock4->sin_addr);

  } else if (sa->ss_family == AF_INET6) {
    sock6 = (struct sockaddr_in6 *)sa;
    sock6->sin6_family = AF_INET6;
    sock6->sin6_port = htons(port);
    r = inet_pton(AF_INET6, ip, &sock6->sin6_addr);

  } else {
    goto error;
  }

  if (r == 1)
    return true;

error:
  return false;
}

#define OV_SOCKET_TYPE_STRING_TCP "TCP"
#define OV_SOCKET_TYPE_STRING_UDP "UDP"
#define OV_SOCKET_TYPE_STRING_TLS "TLS"
#define OV_SOCKET_TYPE_STRING_DTLS "DTLS"
#define OV_SOCKET_TYPE_STRING_LOCAL "LOCAL"

#define OV_SOCKET_TYPE_STRING_LOWER_TCP "tcp"
#define OV_SOCKET_TYPE_STRING_LOWER_UDP "udp"
#define OV_SOCKET_TYPE_STRING_LOWER_TLS "tls"
#define OV_SOCKET_TYPE_STRING_LOWER_DTLS "dtls"
#define OV_SOCKET_TYPE_STRING_LOWER_LOCAL "local"

/*---------------------------------------------------------------------------*/

const char *ov_socket_transport_to_string(ov_socket_transport type) {
  switch (type) {
  case TCP:
    return OV_SOCKET_TYPE_STRING_TCP;

  case UDP:
    return OV_SOCKET_TYPE_STRING_UDP;

  case TLS:
    return OV_SOCKET_TYPE_STRING_TLS;

  case DTLS:
    return OV_SOCKET_TYPE_STRING_DTLS;

  case LOCAL:
    return OV_SOCKET_TYPE_STRING_LOCAL;

  default:
    return NULL;
  }

  return NULL;
}

/*---------------------------------------------------------------------------*/

const char *ov_socket_transport_to_string_lower(ov_socket_transport type) {
  switch (type) {
  case TCP:
    return OV_SOCKET_TYPE_STRING_LOWER_TCP;

  case UDP:
    return OV_SOCKET_TYPE_STRING_LOWER_UDP;

  case TLS:
    return OV_SOCKET_TYPE_STRING_LOWER_TLS;

  case DTLS:
    return OV_SOCKET_TYPE_STRING_LOWER_DTLS;

  case LOCAL:
    return OV_SOCKET_TYPE_STRING_LOWER_LOCAL;

  default:
    return NULL;
  }

  return NULL;
}

/*---------------------------------------------------------------------------*/

ov_socket_transport ov_socket_transport_from_string(const char *string) {
  if (!string)
    goto error;

  size_t len = strlen(string);

  for (ov_socket_transport i = NETWORK_TRANSPORT_TYPE_ERROR + 1;
       i < NETWORK_TRANSPORT_TYPE_OOB; i++) {
    if (0 == strncasecmp(ov_socket_transport_to_string(i), string, len))
      return i;
  }

error:
  return NETWORK_TRANSPORT_TYPE_ERROR;
}

/*---------------------------------------------------------------------------*/

ov_socket_transport ov_socket_transport_parse_string(const char *string,
                                                     size_t length) {
  if (!string)
    goto error;

  if (0 == strncasecmp(OV_SOCKET_TYPE_STRING_TCP, string, length))
    if (length == strlen(OV_SOCKET_TYPE_STRING_TCP))
      return TCP;

  if (0 == strncasecmp(OV_SOCKET_TYPE_STRING_UDP, string, length))
    if (length == strlen(OV_SOCKET_TYPE_STRING_UDP))
      return UDP;

  if (0 == strncasecmp(OV_SOCKET_TYPE_STRING_TLS, string, length))
    if (length == strlen(OV_SOCKET_TYPE_STRING_TLS))
      return TLS;

  if (0 == strncasecmp(OV_SOCKET_TYPE_STRING_DTLS, string, length))
    if (length == strlen(OV_SOCKET_TYPE_STRING_DTLS))
      return DTLS;

  if (0 == strncasecmp(OV_SOCKET_TYPE_STRING_LOCAL, string, length))
    if (length == strlen(OV_SOCKET_TYPE_STRING_LOCAL))
      return LOCAL;

error:
  return NETWORK_TRANSPORT_TYPE_ERROR;
}

/*---------------------------------------------------------------------------*/

ov_socket_configuration ov_socket_configuration_from_json(
    const ov_json_value *object, const ov_socket_configuration default_values) {
  if (!ov_json_is_object(object))
    goto error;

  ov_socket_configuration config = default_values;

  ov_json_value *value =
      ov_json_object_get((ov_json_value *)object, OV_KEY_SOCKET_TYPE);

  if (value) {
    ov_socket_transport transport =
        ov_socket_transport_from_string(ov_json_string_get(value));

    if (transport == NETWORK_TRANSPORT_TYPE_ERROR)
      goto error;

    config.type = transport;
  }

  value = ov_json_object_get((ov_json_value *)object, OV_KEY_SOCKET_HOST);

  if (value) {
    const char *content = ov_json_string_get(value);
    if (!content)
      goto error;

    if (!strncpy(config.host, content, OV_HOST_NAME_MAX))
      goto error;
  }

  value = ov_json_object_get((ov_json_value *)object, OV_KEY_SOCKET_PORT);

  if (value) {
    double port = ov_json_number_get(value);
    if (port < 0 || port > UINT16_MAX)
      goto error;

    if (port != floor(port))
      goto error;

    config.port = port;
  }

  return config;
error:
  return default_values;
}

/*----------------------------------------------------------------------------*/

bool ov_socket_configuration_to_json(const ov_socket_configuration config,
                                     ov_json_value **destination) {
  bool created = false;
  if (!destination)
    goto error;

  if (!*destination) {
    *destination = ov_json_object();
    if (!*destination)
      goto error;

    created = true;
  }

  ov_json_value *value = NULL;
  ov_json_value *object = ov_json_value_cast(*destination);
  if (!ov_json_value_clear(object))
    goto error;

  if (config.host[0]) {
    value = ov_json_string(config.host);
    if (0 == value)
      goto error;

  } else {
    value = ov_json_null();
  }

  if (!value)
    goto error;

  if (!ov_json_object_set(object, OV_KEY_SOCKET_HOST, value))
    goto error;

  value = ov_json_number(config.port);

  if (0 == value)
    goto error;

  if (!ov_json_object_set(object, OV_KEY_SOCKET_PORT, value))
    goto error;

  const char *type = ov_socket_transport_to_string(config.type);
  if (!type)
    goto error;

  value = ov_json_string(type);
  if (0 == value)
    goto error;

  if (!ov_json_object_set(object, OV_KEY_SOCKET_TYPE, value))
    goto error;

  return true;
error:
  if (created)
    *destination = ov_json_value_free(*destination);
  return false;
}

/*----------------------------------------------------------------------------*/

bool ov_socket_configuration_equals(ov_socket_configuration cfg1,
                                    ov_socket_configuration cfg2) {
  if (cfg1.port != cfg2.port)
    return false;

  if (cfg1.type != cfg2.type)
    return false;

  if (0 != strncmp(cfg1.host, cfg2.host, OV_HOST_NAME_MAX))
    return false;

  return true;
}

/*------------------------------------------------------------------*/

ov_json_value *ov_socket_data_to_json(const ov_socket_data *data) {
  ov_json_value *out = NULL;
  ov_json_value *val = NULL;

  if (!data)
    goto error;

  out = ov_json_object();
  if (!out)
    goto error;

  if (0 == data->host[0]) {
    val = ov_json_null();
  } else {
    val = ov_json_string(data->host);
  }
  if (!ov_json_object_set(out, OV_KEY_HOST, val))
    goto error;

  val = ov_json_number(data->port);
  if (!ov_json_object_set(out, OV_KEY_PORT, val))
    goto error;

  return out;
error:
  ov_json_value_free(out);
  ov_json_value_free(val);
  return NULL;
}

/*------------------------------------------------------------------*/

ov_socket_data ov_socket_data_from_json(const ov_json_value *value) {
  ov_socket_data data;
  memset(&data, 0, sizeof(ov_socket_data));
  if (!value)
    goto error;

  const char *host = ov_json_string_get(ov_json_object_get(value, OV_KEY_HOST));

  if (host)
    strncpy(data.host, host, OV_HOST_NAME_MAX);

  data.port =
      (uint16_t)ov_json_number_get(ov_json_object_get(value, OV_KEY_PORT));

  return data;
error:
  memset(&data, 0, sizeof(ov_socket_data));
  return data;
}

/*----------------------------------------------------------------------------*/

bool ov_socket_data_to_string(char *target, size_t target_max_len_bytes,
                              ov_socket_data const *sd) {
  if ((0 == target) || (0 == target_max_len_bytes)) {
    goto error;
  }

  char const *host = 0;
  uint16_t port = 0;

  if (0 == sd) {
    strncpy(target, "INVALID_SOCKET", target_max_len_bytes);

  } else {
    host = sd->host;
    port = sd->port;

    if (0 == host[0]) {
      host = "UNKNOWN_HOST";
    }

    snprintf(target, target_max_len_bytes, "%s:%" PRIu16, host, port);
  }

  target[target_max_len_bytes - 1] = 0;

  return true;

error:

  return false;
}

/*------------------------------------------------------------------*/

bool ov_socket_log_error_with_config(ov_socket_configuration config,
                                     ov_socket_error err) {
  return ov_log_error("Failed to open socket\n"
                      "HOST:PORT|TYPE %s:%i|%s\n"
                      "errno          %i|%s\n"
                      "gaierror       %i|%s\n",
                      config.host, config.port,
                      ov_socket_transport_to_string(config.type), err.err,
                      strerror(err.err), err.gai, gai_strerror(err.gai));
}

/*---------------------------------------------------------------------------*/

bool ov_socket_log(int socket, const char *message, ...) {
  if (socket < 0)
    return false;

  struct sockaddr_storage remote_sa = {0};
  struct sockaddr_storage local_sa = {0};

  char local_ip[OV_HOST_NAME_MAX] = {0};
  char remote_ip[OV_HOST_NAME_MAX] = {0};

  uint16_t local_port = 0;
  uint16_t remote_port = 0;

  char msg[OV_SOCKET_MSG_MAX] = {0};

  if (message) {
    va_list args;
    va_start(args, message);
    vsnprintf(msg, OV_SOCKET_MSG_MAX, message, args);
    va_end(args);
  }

  // get local SA
  if (ov_socket_get_sockaddr_storage(socket, &local_sa, &remote_sa, NULL)) {
    // parse debug logging data
    switch (local_sa.ss_family) {
    case AF_INET:
    case AF_INET6:

      if (!ov_socket_parse_sockaddr_storage(&local_sa, local_ip,
                                            OV_HOST_NAME_MAX, &local_port)) {
        ov_log_error("Failed to parse data "
                     "from socket fd %i",
                     socket);
        return false;
      }

      if (!ov_socket_parse_sockaddr_storage(&remote_sa, remote_ip,
                                            OV_HOST_NAME_MAX, &remote_port)) {
        ov_log_error("Failed to parse data "
                     "from socket fd %i",
                     socket);
        return false;
      }

      break;

    case AF_UNIX:

      break;

    default:
      ov_log_error("Family %i not supported", local_sa.ss_family);
      break;
    }

    if (local_ip[0] != 0) {
      ov_log_debug("socket fd %i | "
                   "LOCAL %s:%i REMOTE %s:%i | %s",
                   socket, local_ip, local_port, remote_ip, remote_port, msg);

    } else {
      ov_log_debug("socket fd %i | "
                   "PATH %s | %s",
                   socket, ((struct sockaddr_un *)&local_sa)->sun_path, msg);
    }

  } else {
    ov_log_debug("socket fd %i | %s", socket, msg);
  }

  return true;
}

/*---------------------------------------------------------------------------*/

static void *socket_configuration_free(void *data) {
  if (data)
    free(data);
  return NULL;
}

/*---------------------------------------------------------------------------*/

static bool socket_configuration_clear(void *data) {
  if (!data)
    return false;

  memset(data, 0, sizeof(ov_socket_configuration));
  return true;
}

/*---------------------------------------------------------------------------*/

static void *socket_configuration_copy(void **dest, const void *src) {
  bool created = false;

  if (!dest || !src)
    goto error;

  if (!*dest) {
    *dest = calloc(1, sizeof(ov_socket_configuration));
    if (!*dest)
      goto error;

    created = true;
  }

  if (!memcpy(*dest, src, sizeof(ov_socket_configuration)))
    goto error;

  return *dest;
error:
  if (created) {
    free(*dest);
    *dest = NULL;
  }
  return NULL;
}

/*---------------------------------------------------------------------------*/

static bool socket_configuration_dump(FILE *stream, const void *data) {
  if (!stream || !data)
    return false;

  ov_socket_configuration *config = (ov_socket_configuration *)data;

  if (!fprintf(stream, "HOST:PORT | type %s:%i | %i", config->host,
               config->port, config->type))
    return false;

  return true;
}

/*---------------------------------------------------------------------------*/

ov_list *ov_socket_configuration_list() {
  ov_list *list = ov_list_create((ov_list_config){

      .item.free = socket_configuration_free,
      .item.clear = socket_configuration_clear,
      .item.copy = socket_configuration_copy,
      .item.dump = socket_configuration_dump});

  return list;
}

/*---------------------------------------------------------------------------*/

bool ov_socket_generate_5tuple(char **dest, size_t *dest_len, int socket,
                               const ov_socket_data *remote) {
  bool created = false;

  if (!dest || !remote || !dest_len)
    goto error;

  ov_socket_configuration local;
  memset(&local, 0, sizeof(ov_socket_configuration));

  if (!ov_socket_get_config(socket, &local, NULL, NULL))
    goto error;

  const char *transport = ov_socket_transport_to_string(local.type);
  if (!transport)
    goto error;

  size_t required = strlen(local.host) + strlen(transport) +
                    strlen(remote->host) + 2 + 10 + 1;

  if (*dest) {
    if (*dest_len < required)
      goto error;

  } else {
    *dest = calloc(required + 1, sizeof(char));
    if (!*dest)
      goto error;

    *dest_len = required;
    created = true;
  }

  if (!snprintf(*dest, required, "%s:%i%s%s:%i", local.host, local.port,
                transport, remote->host, remote->port))
    goto error;

  return true;

error:
  if (created) {
    free(*dest);
    *dest = NULL;
  }
  return false;
}

/*---------------------------------------------------------------------------*/

bool ov_socket_is_dgram(int socket) {
  if (socket < 0)
    goto error;

  int opt = 0;
  socklen_t optlen = sizeof(opt);

  if (0 != getsockopt(socket, SOL_SOCKET, SO_TYPE, &opt, &optlen))
    goto error;

  if (opt == SOCK_DGRAM)
    return true;
error:
  return false;
}

/*---------------------------------------------------------------------------*/

int ov_socket_get_type(int socket) {
  if (socket < 0)
    goto error;

  int opt = 0;
  socklen_t optlen = sizeof(opt);

  if (0 != getsockopt(socket, SOL_SOCKET, SO_TYPE, &opt, &optlen))
    goto error;

  return opt;
error:
  return -1;
}

/*---------------------------------------------------------------------------*/

bool ov_sockets_are_similar(int socket1, int socket2) {
  if (socket1 < 0 || socket2 < 0)
    goto error;

  struct sockaddr_storage sa1 = {0};
  struct sockaddr_storage sa2 = {0};

  if (!ov_socket_get_sockaddr_storage(socket1, &sa1, NULL, NULL))
    goto error;

  if (!ov_socket_get_sockaddr_storage(socket2, &sa2, NULL, NULL))
    goto error;

  if (sa1.ss_family != sa2.ss_family)
    goto error;

  int eins = ov_socket_get_type(socket1);
  int zwei = ov_socket_get_type(socket2);

  if (eins < 0 || zwei < 0)
    goto error;

  return (eins == zwei);
error:
  return false;
}

/*---------------------------------------------------------------------------*/

bool ov_socket_connect(int fd, const struct sockaddr_storage *remote) {
  if (fd < 1 || !remote)
    goto error;

  ov_socket_data local = {0};

  if (!ov_socket_get_data(fd, &local, NULL))
    goto error;

  if (local.sa.ss_family != remote->ss_family)
    goto error;

  size_t len = sizeof(struct sockaddr_in);
  if (local.sa.ss_family == AF_INET6)
    len = sizeof(struct sockaddr_in6);

  if (0 != connect(fd, (struct sockaddr *)remote, len))
    goto error;

  return true;
error:
  return false;
}

/*---------------------------------------------------------------------------*/

bool ov_socket_unconnect(int fd) {
  if (fd < 0)
    goto error;

  struct sockaddr_in *in = NULL;
  struct sockaddr_in6 *in6 = NULL;

  struct sockaddr_storage sa_local = {0};
  struct sockaddr_storage sa_unset = {0};

  socklen_t len_sa = sizeof(struct sockaddr_storage);

  if (0 != getsockname(fd, (struct sockaddr *)&sa_local, &len_sa))
    goto error;

  switch (sa_local.ss_family) {
  case AF_INET:
    in = (struct sockaddr_in *)&sa_local;
    break;

  case AF_INET6:
    in6 = (struct sockaddr_in6 *)&sa_local;
    break;

  default:
    goto error;
  }

  sa_unset.ss_family = AF_UNSPEC;

  /*
   *  We ignore the connect result,
   *  as it may be different, based on the os.
   *
   *  (linux returns 0, macOS returns -1 and sets errno to EAFNOSUPPORT
   */
  connect(fd, (struct sockaddr *)&sa_unset, len_sa);

  /*
   *  We do check the actual connected state instead
   */

  errno = 0;
  if (-1 != getpeername(fd, (struct sockaddr *)&sa_unset, &len_sa))
    goto error;

  if (errno != ENOTCONN)
    goto error;

  /*
   *  We perform a rebind (required on Linux, no issue on macOS)
   *  and check if the port is the same after rebinding.
   */

  if (0 != bind(fd, (struct sockaddr *)&sa_local, len_sa)) {
    if (0 != getsockname(fd, (struct sockaddr *)&sa_unset, &len_sa))
      goto error;

    if (in) {
      if (in->sin_port == ((struct sockaddr_in *)&sa_unset)->sin_port)
        goto done;

    } else if (in6) {
      if (in6->sin6_port == ((struct sockaddr_in6 *)&sa_unset)->sin6_port)
        goto done;
    }

    ov_log_critical(
        "local port could not be verified. for FD %i after unconnect", fd);

    goto error;
  }

done:
  return true;
error:
  if (fd != 0)
    ov_log_critical("Could not unconnect fd %i", fd);

  return false;
}

/*---------------------------------------------------------------------------*/

bool ov_socket_destination_address_type(const char *host, int *type) {
  struct addrinfo hint, *result = NULL;
  memset(&hint, 0, sizeof(struct addrinfo));

  if (!host || !type)
    goto error;

  int r = 0;

  hint.ai_family = PF_UNSPEC;
  hint.ai_flags = AI_NUMERICHOST;

  r = getaddrinfo(host, NULL, &hint, &result);
  if (r != 0)
    goto error;

  *type = result->ai_family;
  freeaddrinfo(result);
  return true;
error:
  if (result)
    freeaddrinfo(result);
  if (type)
    *type = AF_UNSPEC;
  return false;
}

/*---------------------------------------------------------------------------*/

int ov_socket_open_server(ov_socket_configuration *scfg) {
  int sfd = -1;
  ov_socket_data us = {0};

  if (!ov_ptr_valid(scfg,
                    "Cannot open server socket: No socket configuration") ||
      (!ov_cond_valid(-1 < (sfd = ov_socket_create(*scfg, false, 0)),
                      "Opening server socket failed: IO error")) ||
      (!ov_cond_valid(ov_socket_get_data(sfd, &us, 0),
                      "Could not parse back server socket information")) ||
      (!ov_cond_valid(sizeof(scfg->host) > strlen(us.host),
                      "Host name too long to be kept in socket "
                      "configuration"))) {
    ov_socket_close(sfd);
    return -1;

  } else {
    strncpy(scfg->host, us.host, sizeof(scfg->host));
    scfg->host[sizeof(scfg->host) - 1] = 0;
    scfg->port = us.port;

    return sfd;
  }
}

/*----------------------------------------------------------------------------*/

ov_socket_configuration
ov_socket_load_dynamic_port(ov_socket_configuration config) {
  ov_socket_configuration actual = config;
  int sfd = ov_socket_open_server(&actual);
  ov_socket_close(sfd);

  if (-1 < sfd) {
    return actual;
  } else {
    return (ov_socket_configuration){0};
  }
}

/*---------------------------------------------------------------------------*/

uint32_t ov_socket_get_max_supported_runtime_sockets(uint32_t sockets) {
  /* Max support for 4294967295 socket FDs here.
   * 4,2 billion ... enough connections for everything we do and support. */

  struct rlimit limit = {0};

  // Limit config to system MAX

  if (0 != getrlimit(RLIMIT_NOFILE, &limit)) {
    ov_log_error("Failed to get system limit"
                 " of open files errno %i|%s",
                 errno, strerror(errno));
  }

  if (limit.rlim_cur != RLIM_INFINITY) {
    if (sockets > limit.rlim_cur) {
      sockets = limit.rlim_cur;
      ov_log_notice("Limited max sockets to system limit "
                    "of files open %" PRIu32,
                    sockets);
    }
  }

  if (limit.rlim_cur > UINT32_MAX) {
    if (sockets < UINT32_MAX)
      return sockets;

    return UINT32_MAX;

    TODO("... fix rlim_t > UINT32_MAX in ov_*");
    OV_ASSERT(1 == 0);
    return 0;
  }

  if (0 == sockets)
    sockets = limit.rlim_cur;

  return sockets;
}

/*----------------------------------------------------------------------------*/

static bool get_sockaddr(int domain, ov_socket_transport transport,
                         const char *host, int port,
                         struct sockaddr_storage *sockaddr,
                         socklen_t *sockaddr_len) {
  struct addrinfo hints = {0};
  struct addrinfo *result;

  char portstring[6 + 1];
  if (1 > snprintf(portstring, sizeof(portstring), "%i", port)) {
    goto error;
  }
  portstring[6] = 0;

  int transport_int = SOCK_DGRAM;

  switch (transport) {
  case UDP:
    transport_int = SOCK_DGRAM;
    break;

  case TCP:
    transport_int = SOCK_STREAM;
    break;

  default:

    ov_log_error("Unsupported transport type");
    goto error;
  };

  hints.ai_family = domain;
  hints.ai_socktype = transport_int;
  // hints.ai_flags = AI_PASSIVE;

  int s = getaddrinfo(host, portstring, &hints, &result);

  if (s != 0) {
    ov_log_error("getaddrinfo: %s\n", gai_strerror(s));
    goto error;
  }

  *sockaddr_len = result->ai_addrlen;
  memcpy(sockaddr, result->ai_addr, *sockaddr_len);

  freeaddrinfo(result);
  result = 0;

  return true;

error:

  return false;
}

/*----------------------------------------------------------------------------*/

bool ov_socket_configuration_to_sockaddr(const ov_socket_configuration in,
                                         struct sockaddr_storage *sockaddr,
                                         socklen_t *sockaddr_len) {
  if (0 == sockaddr) {
    ov_log_error("Expected sockaddr_storage, got 0 pointer");
    goto error;
  }

  if (0 == sockaddr_len) {
    ov_log_error("Expected pointer to sockaddr_storage_len, got 0 pointer");
    goto error;
  }

  return get_sockaddr(AF_UNSPEC, in.type, in.host, in.port, sockaddr,
                      sockaddr_len);

error:

  return false;
}

/*----------------------------------------------------------------------------*/

ov_socket_data
ov_socket_configuration_to_socket_data(const ov_socket_configuration in) {
  ov_socket_data out = {0};

  socklen_t len = sizeof(out.sa);

  if (!ov_socket_configuration_to_sockaddr(in, &out.sa, &len))
    goto error;

  if (!ov_socket_parse_sockaddr_storage(&out.sa, out.host, OV_HOST_NAME_MAX,
                                        &out.port))
    goto error;

  return out;
error:
  return (ov_socket_data){0};
}

/*----------------------------------------------------------------------------*/

bool ov_socket_state_from_handle(int socket_handle, ov_socket_state *state) {
  ov_socket_data local = {0};
  ov_socket_data remote = {0};

  if (!ov_ptr_valid(state, "Could get socket state: No target state object")) {
    return false;

  } else if (!ov_socket_get_data(socket_handle, &local, &remote)) {
    ov_log_error("Could not get state for socket handle %i", socket_handle);
    return false;

  } else {
    *state = (ov_socket_state){
        .connected = (-1 < socket_handle),
        .peer.port = remote.port,
        .peer.type = TCP,
        .local.port = local.port,
        .local.type = TCP,

    };

    memcpy(state->peer.host, remote.host, sizeof(state->peer.host));
    memcpy(state->local.host, local.host, sizeof(state->local.host));

    return true;
  }
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_socket_state_to_json(ov_socket_state ss) {
  ov_json_value *peer = 0;
  ov_json_value *local = 0;
  ov_socket_configuration_to_json(ss.peer, &peer);
  ov_socket_configuration_to_json(ss.local, &local);

  ov_json_value *state = ov_json_object();

  if ((!ov_ptr_valid(peer, "Cannot turn socket state to JSON")) ||
      (!ov_ptr_valid(local, "Cannot turn socket state to JSON")) ||
      (!ov_json_object_set(state, OV_KEY_LOCAL, local)) ||
      (!ov_json_object_set(state, OV_KEY_PEER, peer)) ||
      (!ov_json_object_set(state, OV_KEY_CONNECTION,
                           ov_json_bool(ss.connected)))) {
    ov_json_object_set(state, OV_KEY_CONNECTION, ov_json_false());
    return state;

  } else {
    return state;
  }
}

/*----------------------------------------------------------------------------*/
