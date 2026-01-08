/***
        ------------------------------------------------------------------------

        Copyright (c) 2023 German Aerospace Center DLR e.V. (GSOC)

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

        @author         Michael J. Beer, DLR/GSOC

        ------------------------------------------------------------------------
*/

#include "ov_base/ov_socket.h"
#include <netinet/in.h>
#include <ov_base/ov_mc_socket.h>

/*----------------------------------------------------------------------------*/

static int recv_socket(char const *mc_group, int port) {

  ov_socket_configuration scfg = {.port = port, .type = UDP};

  size_t max_hostlen = sizeof(scfg.host);

  strncpy(scfg.host, mc_group, max_hostlen);
  scfg.host[max_hostlen - 1] = 0;

  int fd = ov_mc_socket(scfg);
  ov_socket_set_reuseaddress(fd);
  return fd;
}

/*----------------------------------------------------------------------------*/

static int send_socket() {

  ov_socket_configuration scfg = {.port = 0, .type = UDP};

  size_t max_hostlen = sizeof(scfg.host);

  strcpy(scfg.host, "0.0.0.0");
  scfg.host[max_hostlen - 1] = 0;

  return ov_socket_create(scfg, false, 0);
}

/*----------------------------------------------------------------------------*/

static void receive_from(int fd) {

  char buf[1001] = {0};

  while (true) {

    ssize_t read_octets = recv(fd, buf, sizeof(buf) - 1, 0);

    if (read_octets >= 0) {

      buf[(size_t)read_octets] = 0;
      fprintf(stdout, "    GOT: %s\n", buf);

    } else if (EAGAIN != errno) {

      fprintf(stderr, "Could not read from socket: %s\n", strerror(errno));
    }
  }
}

/*----------------------------------------------------------------------------*/

static void send_to(int fd, const struct sockaddr *dest, socklen_t destlen) {

  uint64_t counter = 0;
  char buf[31] = {0};

  while (true) {

    snprintf(buf, sizeof(buf), "Paket no %18" PRIu64, ++counter);

    if (sizeof(buf) != sendto(fd, buf, sizeof(buf), 0, dest, destlen)) {
      fprintf(stderr, "Could not send: %s\n", strerror(errno));
    } else {
      fprintf(stderr, "Sent %s\n", buf);
    }

    sleep(1);
  };
}

/*----------------------------------------------------------------------------*/

static void loopback(int read_fd, int fd, const struct sockaddr *dest,
                     socklen_t destlen) {

  char buf[1001] = {0};

  while (true) {

    ssize_t read_octets = recv(read_fd, buf, sizeof(buf) - 1, 0);

    if (read_octets >= 0) {

      buf[(size_t)read_octets] = 0;
      fprintf(stdout, "     GOT: %s\n", buf);

      if (read_octets ==
          sendto(fd, buf, (size_t)read_octets, 0, dest, destlen)) {

        fprintf(stderr, "Sent %s\n", buf);

      } else {

        fprintf(stderr, "Could not send: %s\n", strerror(errno));
      }

    } else if (errno != EAGAIN) {

      fprintf(stderr, "Could not read from socket: %s\n", strerror(errno));
    }
  }
}

/*----------------------------------------------------------------------------*/

int main(int argc, char const **argv) {

  if (4 > argc) {

    fprintf(stderr,
            " Multicast test server/client\n\n"
            " Joins multicast group and prints any payload received by "
            "this group to stdout.\n"
            " Simultaneously, loop infinitly and send an increasing "
            "sequence number via UDP to another mutlicast group, one "
            "packet per second.\n\n"
            "     Usage: %s OUR_MULTICAST_IP THEIR_MULTICAST_IP PORT "
            "[--loopback]\n"
            "\n\n"
            "          --loopback Loopback - mode: Don't send "
            "continuously, but wait for incoming data and send it back to "
            "sender unaltered\n",
            argv[0]);

    return EXIT_FAILURE;

  } else {

    int recv_fd = recv_socket(argv[1], atoi(argv[3]));
    int send_fd = send_socket();

    struct sockaddr_storage dest = {0};
    ov_socket_fill_sockaddr_storage(&dest, AF_INET, argv[2], atoi(argv[3]));
    socklen_t destlen = sizeof(struct sockaddr_in);

    if ((0 > recv_fd) || (0 > send_fd)) {

      fprintf(stderr, "Could not open socket: %s\n", strerror(errno));
      return EXIT_FAILURE;

    } else if ((4 < argc) && (0 == strcmp("--loopback", argv[4]))) {

      loopback(recv_fd, send_fd, (struct sockaddr *)&dest, destlen);
      close(send_fd);
      close(recv_fd);

    } else if (0 == fork()) {
      close(send_fd);
      receive_from(recv_fd);
      close(recv_fd);
    } else {
      close(recv_fd);
      send_to(send_fd, (struct sockaddr *)&dest, destlen);
      close(send_fd);
    }

    return EXIT_SUCCESS;
  }
}
