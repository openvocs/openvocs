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
        @file           ov_socket_test.c
        @author         Michael Beer
        @author         Markus Toepfer

        @date           2018-12-14

        @ingroup        ov_socket

        @brief          Unit tests of a ov_base socket interface.


        ------------------------------------------------------------------------
*/
#include "ov_socket.c"
#include <ov_test/ov_test.h>

#include <signal.h>

#include "../../include/ov_event_loop.h"

#ifndef OV_TEST_RESOURCE_DIR
#error "Must provide -D OV_TEST_RESOURCE_DIR=value while compiling this file."
#endif

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

bool check_hostname_localhost(const char *name) {

    if (0 == strncmp(name, "127.0.0.1", strlen("127.0.0.1"))) return true;

    if (0 == strncmp(name, "::1", strlen("::1"))) return true;

    return false;
}

/*---------------------------------------------------------------------------*/

int test_ov_socket_create() {

    ov_socket_configuration cfg;
    memset(&cfg, 0, sizeof(cfg));
    ov_socket_error err = {0};

    char ip[INET6_ADDRSTRLEN];

    size_t size = 100;
    char host[size];
    char service[size];
    memset(host, 0, size);
    memset(service, 0, size);

    socklen_t ss_len = sizeof(struct sockaddr_storage);
    struct sockaddr_storage ss = {0};
    struct sockaddr_in6 *sock6 = NULL;
    struct sockaddr_in *sock4 = NULL;

    int client = 0;
    int socket, so_opt = 0;
    size_t i = 0;
    socklen_t so_len = sizeof(so_opt);

    int tcp4, udp4, tcp6, udp6 = 0;
    bool created = false;

    // open an UDP server at a given port
    created = false;
    for (i = 2000; i < 65000; i += 11) {

        cfg = (ov_socket_configuration){

            .type = UDP,
            .host[0] = 0,
            .port = i,

        };

        socket = ov_socket_create(cfg, false, &err);

        if (socket > -1) {
            created = true;
            break;
        }
    }

    // check created socket
    memset(&ss, 0, sizeof(struct sockaddr_storage));
    memset(ip, 0, INET6_ADDRSTRLEN);
    testrun(created);
    testrun(0 == getsockopt(socket, SOL_SOCKET, SO_ERROR, &so_opt, &so_len));
    testrun(so_opt == 0);
    testrun(0 == getsockopt(socket, SOL_SOCKET, SO_TYPE, &so_opt, &so_len));
    testrun(so_opt == SOCK_DGRAM);
    testrun(0 == getsockname(socket, (struct sockaddr *)&ss, &ss_len));
    if (AF_INET != ss.ss_family) testrun(AF_INET6 == ss.ss_family);
    sock4 = (struct sockaddr_in *)&ss;
    testrun(i == ntohs(sock4->sin_port));
    testrun(0 != inet_ntop(AF_INET, &sock4->sin_addr, ip, INET6_ADDRSTRLEN));
    testrun(0 == strncmp("0.0.0.0", ip, strlen("0.0.0.0")));
    close(socket);

    // open an UDP (DTLS) server at a given port
    created = false;
    for (i = 2000; i < 65000; i += 11) {

        cfg = (ov_socket_configuration){

            .type = DTLS,
            .host[0] = 0,
            .port = i,

        };

        socket = ov_socket_create(cfg, false, &err);

        if (socket > -1) {
            created = true;
            break;
        }
    }

    // check created socket
    memset(&ss, 0, sizeof(struct sockaddr_storage));
    memset(ip, 0, INET6_ADDRSTRLEN);
    testrun(created);
    testrun(0 == getsockopt(socket, SOL_SOCKET, SO_ERROR, &so_opt, &so_len));
    testrun(so_opt == 0);
    testrun(0 == getsockopt(socket, SOL_SOCKET, SO_TYPE, &so_opt, &so_len));
    testrun(so_opt == SOCK_DGRAM);
    testrun(0 == getsockname(socket, (struct sockaddr *)&ss, &ss_len));
    if (AF_INET != ss.ss_family) testrun(AF_INET6 == ss.ss_family);
    sock4 = (struct sockaddr_in *)&ss;
    testrun(i == ntohs(sock4->sin_port));
    testrun(0 != inet_ntop(AF_INET, &sock4->sin_addr, ip, INET6_ADDRSTRLEN));
    testrun(0 == strncmp("0.0.0.0", ip, strlen("0.0.0.0")));
    close(socket);

    // open an TCP server at a given port
    created = false;
    for (i = 2000; i < 65000; i += 11) {

        cfg = (ov_socket_configuration){

            .type = TCP,
            .host[0] = 0,
            .port = i,

        };

        socket = ov_socket_create(cfg, false, &err);

        if (socket > -1) {
            created = true;
            break;
        }
    }

    // check created socket
    memset(&ss, 0, sizeof(struct sockaddr_storage));
    memset(ip, 0, INET6_ADDRSTRLEN);
    testrun(created);
    testrun(0 == getsockopt(socket, SOL_SOCKET, SO_ERROR, &so_opt, &so_len));
    testrun(so_opt == 0);
    testrun(0 == getsockopt(socket, SOL_SOCKET, SO_TYPE, &so_opt, &so_len));
    testrun(so_opt == SOCK_STREAM);
    testrun(0 == getsockname(socket, (struct sockaddr *)&ss, &ss_len));

    /*
     *  Default socket may be AF_INET or AF_INET6
     */

    if (AF_INET == ss.ss_family) {

        sock4 = (struct sockaddr_in *)&ss;
        testrun(i == ntohs(sock4->sin_port));
        testrun(0 !=
                inet_ntop(AF_INET, &sock4->sin_addr, ip, INET6_ADDRSTRLEN));
        testrun(0 == strncmp("0.0.0.0", ip, strlen("0.0.0.0")));

    } else {

        testrun(AF_INET6 == ss.ss_family);
        sock6 = (struct sockaddr_in6 *)&ss;
        testrun(i == ntohs(sock6->sin6_port));
        testrun(0 !=
                inet_ntop(AF_INET6, &sock6->sin6_addr, ip, INET6_ADDRSTRLEN));
        testrun(0 == strncmp("::", ip, strlen("::")));
    }

    close(socket);

    // open an TCP (TLS) server at a given port
    created = false;
    for (tcp4 = 2000; tcp4 < 65000; tcp4 += 11) {

        cfg = (ov_socket_configuration){

            .type = TLS,
            .host[0] = 0,
            .port = tcp4,

        };

        socket = ov_socket_create(cfg, false, &err);

        if (socket > -1) {
            created = true;
            break;
        }
    }

    // check created socket
    memset(&ss, 0, sizeof(struct sockaddr_storage));
    memset(ip, 0, INET6_ADDRSTRLEN);
    testrun(created);
    testrun(0 == getsockopt(socket, SOL_SOCKET, SO_ERROR, &so_opt, &so_len));
    testrun(so_opt == 0);
    testrun(0 == getsockopt(socket, SOL_SOCKET, SO_TYPE, &so_opt, &so_len));
    testrun(so_opt == SOCK_STREAM);
    testrun(0 == getsockname(socket, (struct sockaddr *)&ss, &ss_len));
    if (AF_INET == ss.ss_family) {

        sock4 = (struct sockaddr_in *)&ss;
        testrun(i == ntohs(sock4->sin_port));
        testrun(0 !=
                inet_ntop(AF_INET, &sock4->sin_addr, ip, INET6_ADDRSTRLEN));
        testrun(0 == strncmp("0.0.0.0", ip, strlen("0.0.0.0")));

    } else {

        testrun(AF_INET6 == ss.ss_family);
        sock6 = (struct sockaddr_in6 *)&ss;
        testrun(i == ntohs(sock6->sin6_port));
        testrun(0 !=
                inet_ntop(AF_INET6, &sock6->sin6_addr, ip, INET6_ADDRSTRLEN));
        testrun(0 == strncmp("::", ip, strlen("::")));
    }
    close(socket);

    // open UDP (IPv4) server at a given host and port
    created = false;
    for (udp4 = 2000; udp4 < 65000; udp4 += 11) {

        cfg = (ov_socket_configuration){

            .type = UDP,
            .host = "127.0.0.1",
            .port = udp4,

        };

        socket = ov_socket_create(cfg, false, &err);

        if (socket > -1) {
            created = true;
            break;
        }
    }

    // check created socket
    memset(&ss, 0, sizeof(struct sockaddr_storage));
    memset(ip, 0, INET6_ADDRSTRLEN);
    testrun(created);
    testrun(0 == getsockopt(socket, SOL_SOCKET, SO_ERROR, &so_opt, &so_len));
    testrun(so_opt == 0);
    testrun(0 == getsockopt(socket, SOL_SOCKET, SO_TYPE, &so_opt, &so_len));
    testrun(so_opt == SOCK_DGRAM);
    testrun(0 == getsockname(socket, (struct sockaddr *)&ss, &ss_len));
    testrun(AF_INET == ss.ss_family);
    sock4 = (struct sockaddr_in *)&ss;
    testrun(udp4 == ntohs(sock4->sin_port));
    testrun(0 != inet_ntop(AF_INET, &sock4->sin_addr, ip, INET6_ADDRSTRLEN));
    testrun(check_hostname_localhost(ip));

    // open client udp4
    client = ov_socket_create(cfg, true, &err);
    memset(&ss, 0, sizeof(struct sockaddr_storage));
    memset(ip, 0, INET6_ADDRSTRLEN);
    testrun(created);
    testrun(0 == getsockopt(client, SOL_SOCKET, SO_ERROR, &so_opt, &so_len));
    testrun(so_opt == 0);
    testrun(0 == getsockopt(client, SOL_SOCKET, SO_TYPE, &so_opt, &so_len));
    testrun(so_opt == SOCK_DGRAM);
    testrun(0 == getsockname(client, (struct sockaddr *)&ss, &ss_len));
    testrun(AF_INET == ss.ss_family);
    sock4 = (struct sockaddr_in *)&ss;
    testrun(udp4 != ntohs(sock4->sin_port));
    testrun(0 != inet_ntop(AF_INET, &sock4->sin_addr, ip, INET6_ADDRSTRLEN));
    testrun(check_hostname_localhost(ip));

    close(client);
    close(socket);

    // open client UDP no server up
    client = ov_socket_create(cfg, true, &err);
    testrun(client > 0);
    close(client);

    // open UDP (IPv6) server at a given host and port
    created = false;
    for (udp6 = 2000; udp6 < 65000; udp6 += 11) {

        cfg = (ov_socket_configuration){

            .type = UDP,
            .host = "::",
            .port = udp6,

        };

        socket = ov_socket_create(cfg, false, &err);

        if (socket > -1) {
            created = true;
            break;
        }
    }

    // check created socket
    memset(&ss, 0, sizeof(struct sockaddr_storage));
    memset(ip, 0, INET6_ADDRSTRLEN);
    testrun(created);
    testrun(0 == getsockopt(socket, SOL_SOCKET, SO_ERROR, &so_opt, &so_len));
    testrun(so_opt == 0);
    testrun(0 == getsockopt(socket, SOL_SOCKET, SO_TYPE, &so_opt, &so_len));
    testrun(so_opt == SOCK_DGRAM);
    testrun(0 == getsockname(socket, (struct sockaddr *)&ss, &ss_len));
    testrun(AF_INET6 == ss.ss_family);
    sock6 = (struct sockaddr_in6 *)&ss;
    testrun(udp6 == ntohs(sock6->sin6_port));
    testrun(0 != inet_ntop(AF_INET6, &sock6->sin6_addr, ip, INET6_ADDRSTRLEN));
    testrun(0 == strncmp("::", ip, strlen("::")));

    // open client udp6
    client = ov_socket_create(cfg, true, &err);
    memset(&ss, 0, sizeof(struct sockaddr_storage));
    memset(ip, 0, INET6_ADDRSTRLEN);
    testrun(created);
    testrun(0 == getsockopt(client, SOL_SOCKET, SO_ERROR, &so_opt, &so_len));
    testrun(so_opt == 0);
    testrun(0 == getsockopt(client, SOL_SOCKET, SO_TYPE, &so_opt, &so_len));
    testrun(so_opt == SOCK_DGRAM);
    testrun(0 == getsockname(client, (struct sockaddr *)&ss, &ss_len));
    testrun(AF_INET6 == ss.ss_family);
    sock6 = (struct sockaddr_in6 *)&ss;
    testrun(udp6 != ntohs(sock6->sin6_port));
    testrun(0 != inet_ntop(AF_INET6, &sock6->sin6_addr, ip, INET6_ADDRSTRLEN));
    testrun(0 == strncmp("::", ip, strlen("::")));

    close(client);
    close(socket);

    // open client UDP no server upp6
    client = ov_socket_create(cfg, true, &err);
    testrun(client > 0);
    close(client);

    // open UDP (IPv4) server at a given host and port
    created = false;
    for (udp4 = 2000; udp4 < 65000; udp4 += 11) {

        cfg = (ov_socket_configuration){

            .type = UDP,
            .host = "127.0.0.1",
            .port = udp4,

        };

        socket = ov_socket_create(cfg, false, &err);

        if (socket > -1) {
            created = true;
            break;
        }
    }

    // check created socket
    memset(&ss, 0, sizeof(struct sockaddr_storage));
    memset(ip, 0, INET6_ADDRSTRLEN);
    testrun(created);
    testrun(0 == getsockopt(socket, SOL_SOCKET, SO_ERROR, &so_opt, &so_len));
    testrun(so_opt == 0);
    testrun(0 == getsockopt(socket, SOL_SOCKET, SO_TYPE, &so_opt, &so_len));
    testrun(so_opt == SOCK_DGRAM);
    testrun(0 == getsockname(socket, (struct sockaddr *)&ss, &ss_len));
    testrun(AF_INET == ss.ss_family);
    sock4 = (struct sockaddr_in *)&ss;
    testrun(udp4 == ntohs(sock4->sin_port));
    testrun(0 != inet_ntop(AF_INET, &sock4->sin_addr, ip, INET6_ADDRSTRLEN));
    testrun(check_hostname_localhost(ip));

    // open client udp4
    client = ov_socket_create(cfg, true, &err);
    memset(&ss, 0, sizeof(struct sockaddr_storage));
    memset(ip, 0, INET6_ADDRSTRLEN);
    testrun(created);
    testrun(0 == getsockopt(client, SOL_SOCKET, SO_ERROR, &so_opt, &so_len));
    testrun(so_opt == 0);
    testrun(0 == getsockopt(client, SOL_SOCKET, SO_TYPE, &so_opt, &so_len));
    testrun(so_opt == SOCK_DGRAM);
    testrun(0 == getsockname(client, (struct sockaddr *)&ss, &ss_len));
    testrun(AF_INET == ss.ss_family);
    sock4 = (struct sockaddr_in *)&ss;
    testrun(udp4 != ntohs(sock4->sin_port));
    testrun(0 != inet_ntop(AF_INET, &sock4->sin_addr, ip, INET6_ADDRSTRLEN));
    testrun(check_hostname_localhost(ip));

    close(client);
    close(socket);

    // open client UDP no server up
    client = ov_socket_create(cfg, true, &err);
    testrun(client > 0);
    close(client);

    // open TCP (IPv6) server at a given host and port
    created = false;
    for (tcp6 = 2000; tcp6 < 65000; tcp6 += 11) {

        cfg = (ov_socket_configuration){

            .type = TCP,
            .host = "::",
            .port = tcp6,

        };

        socket = ov_socket_create(cfg, false, &err);

        if (socket > -1) {
            created = true;
            break;
        }
    }

    // check created socket
    memset(&ss, 0, sizeof(struct sockaddr_storage));
    memset(ip, 0, INET6_ADDRSTRLEN);
    testrun(created);
    testrun(0 == getsockopt(socket, SOL_SOCKET, SO_ERROR, &so_opt, &so_len));
    testrun(so_opt == 0);
    testrun(0 == getsockopt(socket, SOL_SOCKET, SO_TYPE, &so_opt, &so_len));
    testrun(so_opt == SOCK_STREAM);
    testrun(0 == getsockname(socket, (struct sockaddr *)&ss, &ss_len));
    testrun(AF_INET6 == ss.ss_family);
    sock6 = (struct sockaddr_in6 *)&ss;
    testrun(tcp6 == ntohs(sock6->sin6_port));
    testrun(0 != inet_ntop(AF_INET6, &sock6->sin6_addr, ip, INET6_ADDRSTRLEN));
    testrun(0 == strncmp("::", ip, strlen("::")));

    // open client tcp6
    client = ov_socket_create(cfg, true, &err);
    memset(&ss, 0, sizeof(struct sockaddr_storage));
    memset(ip, 0, INET6_ADDRSTRLEN);
    testrun(created);
    testrun(0 == getsockopt(client, SOL_SOCKET, SO_ERROR, &so_opt, &so_len));
    testrun(so_opt == 0);
    testrun(0 == getsockopt(client, SOL_SOCKET, SO_TYPE, &so_opt, &so_len));
    testrun(so_opt == SOCK_STREAM);
    testrun(0 == getsockname(client, (struct sockaddr *)&ss, &ss_len));
    testrun(AF_INET6 == ss.ss_family);
    sock6 = (struct sockaddr_in6 *)&ss;
    testrun(tcp6 != ntohs(sock6->sin6_port));
    testrun(0 != inet_ntop(AF_INET6, &sock6->sin6_addr, ip, INET6_ADDRSTRLEN));
    testrun(0 == strncmp("::", ip, strlen("::")));

    close(client);
    close(socket);

    // open client TCP no server tcp6
    client = ov_socket_create(cfg, true, &err);
    testrun(client < 0);
    close(client);

    // open a local server socket
    char *path = "/tmp/testsocket_path";
    unlink(path);

    cfg =
        (ov_socket_configuration){.type = LOCAL, .host = "/tmp/testsocket_path"

        };

    socket = ov_socket_create(cfg, false, &err);
    testrun(socket > -1);
    testrun(0 == access(path, F_OK));
    close(socket);
    unlink(path);

    socket = 0;
    testrun(-1 == access(path, F_OK));

    // reopen server socket
    socket = ov_socket_create(cfg, false, &err);
    testrun(socket > -1);
    testrun(0 == access(path, F_OK));

    // open client socket
    client = ov_socket_create(cfg, true, &err);
    testrun(socket > -1);

    close(socket);
    close(client);
    unlink(path);

    // try to open local without server
    socket = ov_socket_create(cfg, true, &err);
    testrun(socket == -1);

    return testrun_log_success();
}

/*---------------------------------------------------------------------------*/

int test_ov_socket_close() {

    testrun(0 > ov_socket_close(-1));
    testrun(0 > ov_socket_close(-13));

    ov_socket_configuration cfg = (ov_socket_configuration){

        .type = LOCAL, .host = "/tmp/testsocket_path"

    };

    int fd = ov_socket_create(cfg, false, NULL);

    testrun(0 > ov_socket_close(fd));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_socket_close_local() {

    char *path = "/tmp/testsocket_path";

    ov_socket_configuration cfg = (ov_socket_configuration){

        .type = LOCAL, .host = "/tmp/testsocket_path"

    };

    int socket = ov_socket_create(cfg, false, NULL);
    testrun(socket > -1);
    testrun(0 == access(path, F_OK));
    close(socket);
    testrun(0 == access(path, F_OK));

    // use close with unlink
    socket = ov_socket_create(cfg, false, NULL);
    testrun(socket > -1);
    testrun(0 == access(path, F_OK));
    testrun(0 == ov_socket_close_local(socket));
    testrun(0 != access(path, F_OK));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_socket_set_dont_fragment() {

    ov_socket_configuration cfg;
    memset(&cfg, 0, sizeof(cfg));

    int socket, so_opt = 0;
    socklen_t so_len = sizeof(so_opt);

    testrun(so_len > 0);

    // start some socket
    cfg = (ov_socket_configuration){.type = UDP, .host = "127.0.0.1"};

    socket = ov_socket_create(cfg, false, NULL);
    testrun(socket > -1);

#if defined(IP_DONTFRAG)
    testrun(0 == getsockopt(socket, IPPROTO_IP, IP_DONTFRAG, &so_opt, &so_len));
    testrun(0 == so_opt);
    testrun(ov_socket_set_dont_fragment(socket));
    testrun(0 == getsockopt(socket, IPPROTO_IP, IP_DONTFRAG, &so_opt, &so_len));
    testrun(1 == so_opt);
#elif defined(IP_MTU_DISCOVER)
    testrun(0 ==
            getsockopt(socket, IPPROTO_IP, IP_MTU_DISCOVER, &so_opt, &so_len));
    testrun(1 == so_opt);
    testrun(ov_socket_set_dont_fragment(socket));
    testrun(0 ==
            getsockopt(socket, IPPROTO_IP, IP_MTU_DISCOVER, &so_opt, &so_len));
    testrun(IP_PMTUDISC_DO == so_opt);
#endif

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_socket_ensure_nonblocking() {

    ov_socket_configuration cfg;
    memset(&cfg, 0, sizeof(cfg));

    int socket;

    // start some socket
    cfg = (ov_socket_configuration){.type = UDP};

    socket = ov_socket_create(cfg, false, NULL);
    testrun(socket > -1);

    int flags = 0;

    testrun(!(O_NONBLOCK & fcntl(socket, F_GETFL, flags)));
    testrun(ov_socket_ensure_nonblocking(socket));
    testrun((O_NONBLOCK & fcntl(socket, F_GETFL, flags)));
    testrun(ov_socket_ensure_nonblocking(socket));
    testrun((O_NONBLOCK & fcntl(socket, F_GETFL, flags)));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_socket_set_reuseaddress() {

    ov_socket_configuration cfg;
    memset(&cfg, 0, sizeof(cfg));

    int socket, so_opt = 0;
    socklen_t so_len = sizeof(so_opt);

    // start some socket
    cfg = (ov_socket_configuration){.type = TCP};

    socket = ov_socket_create(cfg, false, NULL);
    testrun(socket > -1);

    so_opt = 0;
    setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, &so_opt, so_len);

    testrun(0 ==
            getsockopt(socket, SOL_SOCKET, SO_REUSEADDR, &so_opt, &so_len));
    testrun(0 == so_opt);
    testrun(ov_socket_set_reuseaddress(socket));
    testrun(0 ==
            getsockopt(socket, SOL_SOCKET, SO_REUSEADDR, &so_opt, &so_len));

    /*
     *  macOS returns 4 instead of 1
     */
    testrun(0 < so_opt);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_socket_create_at_interface() {

    ov_socket_configuration cfg;
    memset(&cfg, 0, sizeof(cfg));
    ov_socket_configuration empty;
    memset(&empty, 0, sizeof(empty));

    char ip[INET6_ADDRSTRLEN];
    memset(&ip, 0, INET6_ADDRSTRLEN);

    socklen_t ss_len = sizeof(struct sockaddr_storage);
    struct sockaddr_storage ss = {0};
    struct sockaddr_in *sock4 = NULL;

    struct ifaddrs *if_lo_addr = NULL;
    struct ifaddrs *ifaddr = NULL, *ifa = NULL;

    char *if_lo = "lo";
    int socket = 0, n = 0, family = 0;

    // prepare and get localhost IPv4 or IPv6

    testrun(getifaddrs(&ifaddr) != -1);
    for (ifa = ifaddr, n = 0; ifa != NULL; ifa = ifa->ifa_next, n++) {

        if (ifa->ifa_addr == NULL) continue;

        family = ifa->ifa_addr->sa_family;
        if (family == AF_INET || family == AF_INET6) {

            if (0 == strncmp(if_lo, ifa->ifa_name, strlen(if_lo))) {
                if_lo_addr = ifa;
                break;
            }
        }
    }

    testrun(if_lo_addr != NULL);

    // create socket at interface localhost
    cfg = (ov_socket_configuration){.type = UDP};

    testrun(-1 == ov_socket_create_at_interface(NULL, empty, NULL));
    testrun(-1 == ov_socket_create_at_interface(if_lo_addr, empty, NULL));
    testrun(-1 == ov_socket_create_at_interface(NULL, cfg, NULL));

    socket = ov_socket_create_at_interface(if_lo_addr, cfg, NULL);
    testrun(socket >= 0);
    // check socket ip
    testrun(0 == getsockname(socket, (struct sockaddr *)&ss, &ss_len));
    testrun(AF_INET == ss.ss_family);
    sock4 = (struct sockaddr_in *)&ss;
    testrun(0 < ntohs(sock4->sin_port));
    testrun(0 != inet_ntop(AF_INET, &sock4->sin_addr, ip, INET6_ADDRSTRLEN));
    testrun(check_hostname_localhost(ip));
    close(socket);
    free(ifaddr);

    return testrun_log_success();
}

/*---------------------------------------------------------------------------*/

int test_ov_socket_config_from_sockaddr_storage() {

    ov_socket_configuration cfg;
    memset(&cfg, 0, sizeof(cfg));
    ov_socket_configuration res;
    memset(&res, 0, sizeof(res));

    struct sockaddr_storage sa = {0};
    socklen_t len_sa = sizeof(sa);

    int socket = 0;

    // start a socket at localhost
    cfg = (ov_socket_configuration){

        .type = UDP, .host = "localhost"};

    socket = ov_socket_create(cfg, false, NULL);
    testrun(socket);
    // parse sa
    testrun(0 == getsockname(socket, (struct sockaddr *)&sa, &len_sa));

    testrun(!ov_socket_config_from_sockaddr_storage(NULL, NULL, NULL));
    testrun(!ov_socket_config_from_sockaddr_storage(&sa, NULL, NULL));
    testrun(!ov_socket_config_from_sockaddr_storage(NULL, &res, NULL));

    testrun(ov_socket_config_from_sockaddr_storage(&sa, &res, NULL));
    testrun(res.type == NETWORK_TRANSPORT_TYPE_ERROR);
    testrun(check_hostname_localhost(res.host));
    testrun(res.port > 0);

    close(socket);

    // start a local socket
    cfg = (ov_socket_configuration){

        .type = LOCAL, .host = "/tmp/socket"

    };

    socket = ov_socket_create(cfg, false, NULL);
    testrun(socket >= 0);
    // parse sa
    testrun(0 == getsockname(socket, (struct sockaddr *)&sa, &len_sa));
    testrun(ov_socket_config_from_sockaddr_storage(&sa, &res, NULL));
    testrun(res.type == LOCAL);
    fprintf(stdout, "%s\n%s\n", res.host, cfg.host);
    testrun(0 == strncmp(res.host, cfg.host, strlen(cfg.host)));
    testrun(res.port == 0);

    close(socket);
    return testrun_log_success();
}

/*---------------------------------------------------------------------------*/

int test_ov_socket_get_config() {

    ov_socket_configuration cfg;
    memset(&cfg, 0, sizeof(cfg));
    ov_socket_configuration loc;
    memset(&loc, 0, sizeof(loc));
    ov_socket_configuration rem;
    memset(&rem, 0, sizeof(rem));
    ov_socket_error err = {0};

    int socket = 0, client = 0;

    // start a socket at localhost
    cfg = (ov_socket_configuration){

        .type = UDP, .host = "localhost"};

    socket = ov_socket_create(cfg, false, &err);
    testrun(socket);

    testrun(!ov_socket_get_config(-1, NULL, NULL, NULL));

    testrun(!ov_socket_get_config(0, NULL, NULL, NULL));
    testrun(ov_socket_get_config(socket, NULL, NULL, NULL));

    // fill everything
    testrun(ov_socket_get_config(socket, &loc, &rem, &err));
    testrun(loc.type == cfg.type);
    testrun(check_hostname_localhost(loc.host));
    testrun(loc.port > 0);
    testrun(rem.type == 0);
    testrun(rem.host[0] == 0);
    testrun(rem.port == 0);

    close(socket);

    // start a socket at localhost
    cfg = (ov_socket_configuration){

        .type = LOCAL, .host = "/tmp/some_unix_socket"};

    socket = ov_socket_create(cfg, false, &err);
    testrun(socket);
    testrun(ov_socket_get_config(socket, &loc, &rem, &err));
    testrun(loc.type == cfg.type);
    testrun(0 == strncmp(loc.host, cfg.host, strlen(cfg.host)));
    testrun(loc.port == 0);
    testrun(rem.type == 0);
    testrun(rem.host[0] == 0);
    testrun(rem.port == 0);

    close(socket);

    // start a socket at localhost
    cfg = (ov_socket_configuration){

        .type = TCP, .host = "localhost"};

    socket = ov_socket_create(cfg, false, &err);
    testrun(socket);
    testrun(ov_socket_get_config(socket, &loc, &rem, &err));
    testrun(loc.type == cfg.type);
    testrun(check_hostname_localhost(loc.host));
    testrun(loc.port > 0);
    testrun(rem.type == 0);
    testrun(rem.host[0] == 0);
    testrun(rem.port == 0);

    ov_socket_configuration cfg_client = loc;

    pid_t pid1 = -1;

    switch (pid1 = fork()) {
        case -1: // failure fork
            testrun(1 == 0, "failure fork");
            break;

        case 0:
            // child1
            // let the socket run here
            return 0;
            break;
        default:
            // parent
            // create client config from server

            client = ov_socket_create(cfg_client, true, &err);
            testrun(client);
            testrun(ov_socket_get_config(client, &loc, &rem, &err));
            testrun(loc.type == cfg_client.type);
            testrun(check_hostname_localhost(loc.host));
            testrun(loc.port > 0);
            testrun(rem.type == loc.type);
            testrun(check_hostname_localhost(rem.host));
            testrun(rem.port == cfg_client.port);

            close(socket);
            close(client);

            kill(pid1, SIGKILL);

            return testrun_log_success();
            break;
    }

    return testrun_log_success();
}

/*---------------------------------------------------------------------------*/

int test_ov_socket_get_sockaddr_storage() {

    ov_socket_configuration cfg;
    memset(&cfg, 0, sizeof(cfg));
    struct sockaddr_storage loc = {0};
    struct sockaddr_storage rem = {0};
    ov_socket_error err = {0};

    int socket = 0, client = 0;

    char ip[OV_HOST_NAME_MAX] = {0};
    uint16_t port = 0;

    // start a socket at localhost
    cfg = (ov_socket_configuration){

        .type = UDP,
        .host = "localhost",
    };

    socket = ov_socket_create(cfg, false, &err);
    testrun(socket);

    testrun(!ov_socket_get_sockaddr_storage(-1, NULL, NULL, NULL));

    testrun(ov_socket_get_sockaddr_storage(0, NULL, NULL, NULL));
    testrun(ov_socket_get_sockaddr_storage(socket, NULL, NULL, NULL));

    // fill everything
    testrun(ov_socket_get_sockaddr_storage(socket, &loc, &rem, &err));
    testrun((loc.ss_family == AF_INET) || (loc.ss_family == AF_INET6));
    // quick check with parse
    testrun(
        ov_socket_parse_sockaddr_storage(&loc, ip, OV_HOST_NAME_MAX, &port));
    testrun(port > 0);
    testrun(check_hostname_localhost(ip));
    close(socket);

    // start a socket at localhost
    cfg = (ov_socket_configuration){

        .type = TCP, .host = "localhost"};

    socket = ov_socket_create(cfg, false, &err);
    testrun(socket);

    // get client config from server config
    ov_socket_configuration cfg_client;
    memset(&cfg_client, 0, sizeof(cfg_client));
    testrun(ov_socket_get_config(socket, &cfg_client, NULL, NULL));

    pid_t pid1 = -1;

    switch (pid1 = fork()) {
        case -1: // failure fork
            testrun(1 == 0, "failure fork");
            break;

        case 0:
            // child1
            // let the socket run here
            return 0;
            break;
        default:
            // parent
            // create client

            client = ov_socket_create(cfg_client, true, &err);
            testrun(client);

            testrun(ov_socket_get_sockaddr_storage(client, &loc, &rem, &err));
            // quick check with parse
            memset(ip, 0, OV_HOST_NAME_MAX);
            port = 0;
            testrun(ov_socket_parse_sockaddr_storage(
                &loc, ip, OV_HOST_NAME_MAX, &port));
            testrun(port > 0);
            testrun(check_hostname_localhost(ip));
            memset(ip, 0, OV_HOST_NAME_MAX);
            port = 0;
            testrun(ov_socket_parse_sockaddr_storage(
                &rem, ip, OV_HOST_NAME_MAX, &port));
            testrun(port > 0);
            testrun(check_hostname_localhost(ip));

            close(socket);
            close(client);

            kill(pid1, SIGKILL);

            return testrun_log_success();
            break;
    }

    return testrun_log_success();
}

/*
 *      ------------------------------------------------------------------------
 *
 *                         ENCODING / DECODING
 *
 *      ------------------------------------------------------------------------
 */

int test_ov_socket_parse_sockaddr_storage() {

    struct sockaddr_storage s = {0};

    struct sockaddr_in6 *sock6 = (struct sockaddr_in6 *)&s;
    struct sockaddr_in *sock4 = (struct sockaddr_in *)&s;

    char ip[INET6_ADDRSTRLEN] = {0};
    uint16_t port = 0;

    s.ss_family = AF_INET;

    testrun(!ov_socket_parse_sockaddr_storage(NULL, NULL, 0, NULL));
    testrun(
        !ov_socket_parse_sockaddr_storage(NULL, ip, INET6_ADDRSTRLEN, &port));
    testrun(
        !ov_socket_parse_sockaddr_storage(&s, NULL, INET6_ADDRSTRLEN, &port));
    testrun(!ov_socket_parse_sockaddr_storage(&s, ip, 0, &port));
    testrun(!ov_socket_parse_sockaddr_storage(&s, ip, INET6_ADDRSTRLEN, NULL));

    testrun(ov_socket_parse_sockaddr_storage(&s, ip, INET6_ADDRSTRLEN, &port));
    testrun(port == 0);
    testrun(0 == strncmp(ip, "0.0.0.0", strlen("0.0.0.0")));

    s.ss_family = AF_INET6;
    testrun(ov_socket_parse_sockaddr_storage(&s, ip, INET6_ADDRSTRLEN, &port));
    testrun(port == 0);
    testrun(0 == strncmp(ip, "::", strlen("::")));

    // AF_INET6
    s.ss_family = AF_INET6;
    sock6->sin6_family = AF_INET6;
    sock6->sin6_port = htons(1234);
    testrun(1 == inet_pton(AF_INET6,
                           "2001:db8:85a3:8d3:1319:8a2e:370:7348",
                           &sock6->sin6_addr));

    testrun(ov_socket_parse_sockaddr_storage(&s, ip, INET6_ADDRSTRLEN, &port));
    testrun(port == 1234);
    testrun(0 == strncmp(ip,
                         "2001:db8:85a3:8d3:1319:8a2e:370:7348",
                         strlen("2001:db8:85a3:8d3:1319:8a2e:370:7348")));

    // AF_INET
    s.ss_family = AF_INET;
    sock4->sin_family = AF_INET;
    sock4->sin_port = htons(444);
    testrun(1 == inet_pton(AF_INET, "127.0.0.1", &sock4->sin_addr));

    testrun(ov_socket_parse_sockaddr_storage(&s, ip, INET6_ADDRSTRLEN, &port));
    testrun(port == 444);
    testrun(0 == strncmp(ip, "127.0.0.1", strlen("127.0.0.1")));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_socket_data_from_sockaddr_storage() {

    struct sockaddr_storage s = {0};

    struct sockaddr_in6 *sock6 = (struct sockaddr_in6 *)&s;
    struct sockaddr_in *sock4 = (struct sockaddr_in *)&s;

    s.ss_family = AF_INET;

    ov_socket_data empty;
    memset(&empty, 0, sizeof(empty));
    ov_socket_data data;
    memset(&data, 0, sizeof(data));

    data = ov_socket_data_from_sockaddr_storage(NULL);
    testrun(0 == memcmp(&data, &empty, sizeof(ov_socket_data)));

    data = ov_socket_data_from_sockaddr_storage(&s);
    testrun(0 != memcmp(&data, &empty, sizeof(ov_socket_data)));
    testrun(0 == data.port);
    testrun(0 == strncmp(data.host, "0.0.0.0", strlen("0.0.0.0")));

    s.ss_family = AF_INET6;
    data = ov_socket_data_from_sockaddr_storage(&s);
    testrun(0 != memcmp(&data, &empty, sizeof(ov_socket_data)));
    testrun(0 == data.port);
    testrun(0 == strncmp(data.host, "::", strlen("::")));

    // AF_INET6
    s.ss_family = AF_INET6;
    sock6->sin6_family = AF_INET6;
    sock6->sin6_port = htons(1234);
    testrun(1 == inet_pton(AF_INET6,
                           "2001:db8:85a3:8d3:1319:8a2e:370:7348",
                           &sock6->sin6_addr));

    data = ov_socket_data_from_sockaddr_storage(&s);
    testrun(1234 == data.port);
    testrun(0 == strncmp(data.host,
                         "2001:db8:85a3:8d3:1319:8a2e:370:7348",
                         strlen("2001:db8:85a3:8d3:1319:8a2e:370:7348")));

    // AF_INET
    s.ss_family = AF_INET;
    sock4->sin_family = AF_INET;
    sock4->sin_port = htons(444);
    testrun(1 == inet_pton(AF_INET, "127.0.0.1", &sock4->sin_addr));

    data = ov_socket_data_from_sockaddr_storage(&s);
    testrun(444 == data.port);
    testrun(0 == strncmp(data.host, "127.0.0.1", strlen("127.0.0.1")));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_socket_get_data() {

    ov_socket_configuration config = {.type = UDP, .host = "0.0.0.0"};

    int server = ov_socket_create(config, false, NULL);
    testrun(server);
    testrun(ov_socket_get_config(server, &config, NULL, NULL));
    int client = ov_socket_create(config, true, NULL);
    testrun(client);

    ov_socket_data local;
    memset(&local, 0, sizeof(local));
    ov_socket_data remote;
    memset(&remote, 0, sizeof(remote));

    struct sockaddr_storage sa_local = {0};
    struct sockaddr_storage sa_remote = {0};

    testrun(
        ov_socket_get_sockaddr_storage(client, &sa_local, &sa_remote, NULL));

    testrun(!ov_socket_get_data(-1, NULL, NULL));
    testrun(ov_socket_get_data(client, NULL, NULL));

    testrun(ov_socket_get_data(client, &local, &remote));
    testrun(0 == memcmp(&local.sa, &sa_local, sizeof(struct sockaddr_storage)));
    testrun(0 ==
            memcmp(&remote.sa, &sa_remote, sizeof(struct sockaddr_storage)));

    testrun(0 != local.host[0]);
    testrun(0 != remote.host[0]);
    testrun(0 == strncmp(remote.host, local.host, strlen(remote.host)));

    testrun(0 != local.port);
    testrun(0 != remote.port);

    close(client);
    close(server);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_socket_fill_sockaddr_storage() {

    struct sockaddr_storage s = {0};

    struct sockaddr_in6 *sock6 = NULL;
    struct sockaddr_in *sock4 = NULL;

    char *ip = "::";
    uint16_t port = 123;
    char data[INET6_ADDRSTRLEN] = {0};

    testrun(!ov_socket_fill_sockaddr_storage(NULL, 0, NULL, 0));
    testrun(!ov_socket_fill_sockaddr_storage(&s, 0, ip, 0));
    testrun(!ov_socket_fill_sockaddr_storage(&s, AF_INET6, NULL, 0));
    testrun(!ov_socket_fill_sockaddr_storage(NULL, AF_INET6, ip, 0));

    testrun(ov_socket_fill_sockaddr_storage(&s, AF_INET6, ip, port));
    testrun(s.ss_family == AF_INET6);
    sock6 = (struct sockaddr_in6 *)&s;
    testrun(sock6->sin6_family == AF_INET6);
    testrun(sock6->sin6_port == htons(123));
    testrun(sock6->sin6_flowinfo == 0);
    testrun(sock6->sin6_scope_id == 0);
    testrun(inet_ntop(AF_INET6, &sock6->sin6_addr, data, INET6_ADDRSTRLEN));
    testrun(0 == strncmp(ip, "::", strlen("::")));

    ip = "2001:db8:85a3:8d3:1319:8a2e:370:7348";
    port = 12345;

    testrun(ov_socket_fill_sockaddr_storage(&s, AF_INET6, ip, port));
    testrun(s.ss_family == AF_INET6);
    sock6 = (struct sockaddr_in6 *)&s;
    testrun(sock6->sin6_family == AF_INET6);
    testrun(sock6->sin6_port == htons(port));
    testrun(sock6->sin6_flowinfo == 0);
    testrun(sock6->sin6_scope_id == 0);
    testrun(inet_ntop(AF_INET6, &sock6->sin6_addr, data, INET6_ADDRSTRLEN));
    testrun(0 == strncmp(ip,
                         "2001:db8:85a3:8d3:1319:8a2e:370:7348",
                         strlen("2001:db8:85a3:8d3:1319:8a2e:370:7348")));

    // AF_INET
    ip = "127.0.0.1";

    // wrong string for type
    testrun(!ov_socket_fill_sockaddr_storage(&s, AF_INET6, ip, port));

    testrun(ov_socket_fill_sockaddr_storage(&s, AF_INET, ip, port));
    s.ss_family = AF_INET;
    sock4 = (struct sockaddr_in *)&s;
    testrun(sock4->sin_family == AF_INET);
    testrun(sock4->sin_port == htons(port));
    testrun(inet_ntop(AF_INET, &sock4->sin_addr, data, INET6_ADDRSTRLEN));
    testrun(0 == strncmp(ip, "127.0.0.1", strlen("127.0.0.1")));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_socket_configuration_from_json() {

    ov_json_value *value = ov_json_object();

    ov_socket_configuration empty;
    memset(&empty, 0, sizeof(empty));

    ov_socket_configuration config =
        ov_socket_configuration_from_json(value, empty);

    testrun(config.host[0] == 0);
    testrun(config.port == 0);
    testrun(config.type == NETWORK_TRANSPORT_TYPE_ERROR);

    testrun(ov_json_object_set(value, "host", ov_json_string("localhost")));

    config = ov_socket_configuration_from_json(value, empty);

    testrun(0 == strncmp(config.host, "localhost", strlen("localhost")));
    testrun(config.port == 0);
    testrun(config.type == NETWORK_TRANSPORT_TYPE_ERROR);

    testrun(ov_json_object_set(value, "port", ov_json_number(123)));

    config = ov_socket_configuration_from_json(value, empty);

    testrun(0 == strncmp(config.host, "localhost", strlen("localhost")));
    testrun(config.port == 123);
    testrun(config.type == NETWORK_TRANSPORT_TYPE_ERROR);

    testrun(ov_json_object_set(value, "type", ov_json_string("TCP")));

    config = ov_socket_configuration_from_json(value, empty);

    testrun(0 == strncmp(config.host, "localhost", strlen("localhost")));
    testrun(config.port == 123);
    testrun(config.type == TCP);

    testrun(ov_json_value_clear(value));

    config = ov_socket_configuration_from_json(value, empty);

    testrun(config.host[0] == 0);
    testrun(config.port == 0);
    testrun(config.type == NETWORK_TRANSPORT_TYPE_ERROR);

    config = ov_socket_configuration_from_json(
        value,
        (ov_socket_configuration){.host = "0.0.0.0", .port = 222, .type = UDP});

    testrun(0 == strncmp(config.host, "0.0.0.0", strlen("0.0.0.0")));
    testrun(config.port == 222);
    testrun(config.type == UDP);

    config = ov_socket_configuration_from_json(
        value,
        (ov_socket_configuration){.host = "0.0.0.0", .port = 222, .type = UDP});

    // check overrule by object

    testrun(ov_json_object_set(value, "host", ov_json_string("localhost")));

    config = ov_socket_configuration_from_json(
        value,
        (ov_socket_configuration){.host = "0.0.0.0", .port = 222, .type = UDP});

    testrun(0 == strncmp(config.host, "localhost", strlen("localhost")));
    testrun(config.port == 222);
    testrun(config.type == UDP);

    testrun(ov_json_object_set(value, "type", ov_json_string("TCP")));
    testrun(ov_json_object_set(value, "port", ov_json_number(123)));

    config = ov_socket_configuration_from_json(
        value,
        (ov_socket_configuration){.host = "0.0.0.0", .port = 222, .type = UDP});

    testrun(0 == strncmp(config.host, "localhost", strlen("localhost")));
    testrun(config.port == 123);
    testrun(config.type == TCP);

    ov_json_value_free(value);
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_socket_configuration_to_json() {

    ov_json_value *value = NULL;

    ov_socket_configuration config;
    memset(&config, 0, sizeof(config));

    testrun(!ov_socket_configuration_to_json(config, NULL));
    testrun(!ov_socket_configuration_to_json(config, &value));

    testrun(!ov_socket_configuration_to_json(config, &value));
    config.type = UDP;

    testrun(ov_socket_configuration_to_json(config, &value));
    testrun(value);
    testrun(ov_json_is_null(ov_json_get(value, "/host")));
    testrun(0 == ov_json_number_get(ov_json_get(value, "/port")));
    testrun(0 ==
            strncmp(ov_json_string_get(ov_json_get(value, "/type")), "UDP", 3));

    config.port = 123;
    config.type = DTLS;
    strcat(config.host, "localhost");

    testrun(ov_socket_configuration_to_json(config, &value));
    testrun(value);
    testrun(0 == strncmp(ov_json_string_get(ov_json_get(value, "/host")),
                         "localhost",
                         strlen("localhost")));
    testrun(123 == ov_json_number_get(ov_json_get(value, "/port")));
    testrun(0 == strncmp(ov_json_string_get(ov_json_get(value, "/type")),
                         "DTLS",
                         4));

    memset(config.host, 0, OV_HOST_NAME_MAX);
    config.port = 63333;
    config.type = TLS;
    strcat(config.host, "::");

    testrun(ov_socket_configuration_to_json(config, &value));
    testrun(value);
    testrun(0 ==
            strncmp(ov_json_string_get(ov_json_get(value, "/host")), "::", 2));
    testrun(63333 == ov_json_number_get(ov_json_get(value, "/port")));
    testrun(0 ==
            strncmp(ov_json_string_get(ov_json_get(value, "/type")), "TLS", 3));

    ov_json_value_free(value);
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_socket_transport_from_string() {

    testrun(UDP == ov_socket_transport_from_string("UDP"));
    testrun(TCP == ov_socket_transport_from_string("TCP"));
    testrun(TLS == ov_socket_transport_from_string("TLS"));
    testrun(DTLS == ov_socket_transport_from_string("DTLS"));
    testrun(LOCAL == ov_socket_transport_from_string("LOCAL"));
    testrun(UDP == ov_socket_transport_from_string("udp"));
    testrun(UDP == ov_socket_transport_from_string("Udp"));
    testrun(UDP == ov_socket_transport_from_string("uDP"));
    testrun(TCP == ov_socket_transport_from_string("tcp"));
    testrun(TLS == ov_socket_transport_from_string("TLs"));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_socket_configuration_equals() {

    ov_socket_configuration cfg1 = {0};
    ov_socket_configuration cfg2 = {0};

    testrun(ov_socket_configuration_equals(cfg1, cfg2));

    cfg1.port = 1245;

    testrun(!ov_socket_configuration_equals(cfg1, cfg2));

    cfg2.port = 1245;

    testrun(ov_socket_configuration_equals(cfg1, cfg2));

    cfg1.type = UDP;
    cfg2.type = TCP;

    testrun(!ov_socket_configuration_equals(cfg1, cfg2));

    cfg2.type = UDP;

    testrun(ov_socket_configuration_equals(cfg1, cfg2));

    strcpy(cfg1.host, "jormungandr");

    testrun(!ov_socket_configuration_equals(cfg1, cfg2));

    strcpy(cfg2.host, cfg1.host);

    testrun(ov_socket_configuration_equals(cfg1, cfg2));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_socket_transport_parse_string() {

    testrun(UDP == ov_socket_transport_parse_string("UDP", 3));
    testrun(TCP == ov_socket_transport_parse_string("TCP", 3));
    testrun(TLS == ov_socket_transport_parse_string("TLS", 3));
    testrun(DTLS == ov_socket_transport_parse_string("DTLS", 4));
    testrun(LOCAL == ov_socket_transport_parse_string("LOCAL", 5));

    testrun(UDP == ov_socket_transport_parse_string("uDp", 3));
    testrun(TCP == ov_socket_transport_parse_string("Tcp", 3));
    testrun(TLS == ov_socket_transport_parse_string("tlS", 3));
    testrun(DTLS == ov_socket_transport_parse_string("DTlS", 4));
    testrun(LOCAL == ov_socket_transport_parse_string("LocAL", 5));

    testrun(UDP == ov_socket_transport_parse_string("uDp1", 3));
    testrun(TCP == ov_socket_transport_parse_string("Tcp2", 3));
    testrun(TLS == ov_socket_transport_parse_string("tlS3", 3));
    testrun(DTLS == ov_socket_transport_parse_string("DTlS4", 4));
    testrun(LOCAL == ov_socket_transport_parse_string("LocAL5", 5));

    testrun(NETWORK_TRANSPORT_TYPE_ERROR ==
            ov_socket_transport_parse_string("uDp1", 4));
    testrun(NETWORK_TRANSPORT_TYPE_ERROR ==
            ov_socket_transport_parse_string("Tcp2", 2));
    testrun(NETWORK_TRANSPORT_TYPE_ERROR ==
            ov_socket_transport_parse_string("tlS3", 4));
    testrun(NETWORK_TRANSPORT_TYPE_ERROR ==
            ov_socket_transport_parse_string("DTlS4", 5));
    testrun(NETWORK_TRANSPORT_TYPE_ERROR ==
            ov_socket_transport_parse_string("LocAL5", 6));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_socket_transport_to_string() {

    testrun(0 == strncmp(ov_socket_transport_to_string(UDP), "UDP", 3));
    testrun(0 == strncmp(ov_socket_transport_to_string(TCP), "TCP", 3));
    testrun(0 == strncmp(ov_socket_transport_to_string(TLS), "TLS", 3));
    testrun(0 == strncmp(ov_socket_transport_to_string(DTLS), "DTLS", 4));
    testrun(0 == strncmp(ov_socket_transport_to_string(LOCAL), "LOCAL", 5));
    testrun(!ov_socket_transport_to_string(NETWORK_TRANSPORT_TYPE_ERROR));
    testrun(!ov_socket_transport_to_string(NETWORK_TRANSPORT_TYPE_OOB));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_socket_data_to_string() {

    testrun(!ov_socket_data_to_string(0, 0, 0));

    char buf[OV_HOST_NAME_MAX + 10] = {0};

    testrun(!ov_socket_data_to_string(buf, 0, 0));
    testrun(!ov_socket_data_to_string(0, sizeof(buf), 0));
    testrun(ov_socket_data_to_string(buf, sizeof(buf), 0));

    testrun(0 == strcmp(buf, "INVALID_SOCKET"));

    ov_socket_data sd = {
        .port = 15,
    };

    testrun(!ov_socket_data_to_string(0, 0, &sd));
    testrun(!ov_socket_data_to_string(buf, 0, &sd));
    testrun(ov_socket_data_to_string(buf, sizeof(buf), &sd));

    testrun(0 == strcmp(buf, "UNKNOWN_HOST:15"));

    testrun(ov_socket_data_to_string(buf, 5, &sd));
    testrun(0 == strcmp(buf, "UNKN"));

    strcpy(sd.host, "ABC");

    testrun(ov_socket_data_to_string(buf, sizeof(buf), &sd));

    testrun(0 == strcmp(buf, "ABC:15"));

    testrun(ov_socket_data_to_string(buf, 5, &sd));
    testrun(0 == strcmp(buf, "ABC:"));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_socket_log_error_with_config() {

    ov_socket_configuration config;
    memset(&config, 0, sizeof(config));
    ov_socket_error err = {0};

    testrun(ov_socket_log_error_with_config(config, err));

    // with some items
    strcat(config.host, "localhost");
    config.port = 123;
    config.type = UDP;

    err.err = 123;
    err.gai = 123;

    testrun(ov_socket_log_error_with_config(config, err));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_socket_log() {

    ov_socket_configuration config;
    memset(&config, 0, sizeof(config));

    testrun(!ov_socket_log(-1, "test"));
    testrun(!ov_socket_log(-1, NULL));

    config = (ov_socket_configuration){.type = TCP, .host = "localhost"};

    int server = ov_socket_create(config, false, NULL);

    testrun(ov_socket_log(server, NULL));
    testrun(ov_socket_log(server, "test"));
    testrun(ov_socket_log(server, "test %i %i %i", 1, 2, 3));

    testrun(ov_socket_get_config(server, &config, NULL, NULL));
    int client = ov_socket_create(config, true, NULL);
    int socket = accept(server, NULL, NULL);

    testrun(ov_socket_log(socket, NULL));
    testrun(ov_socket_log(socket, "test"));
    testrun(ov_socket_log(socket, "test %i %i %i", 1, 2, 3));

    close(socket);
    close(client);
    close(server);

    config = (ov_socket_configuration){
        .type = LOCAL, .host = "/tmp/some_unix_socket"};

    server = ov_socket_create(config, false, NULL);

    int x = 1;
    char *abc = "abc";
    double d = -1.234;
    uint64_t hex = 0xdeadface;

    testrun(ov_socket_log(server, NULL));
    testrun(ov_socket_log(server, "test"));
    testrun(ov_socket_log(server, "test %i %s %f %#x", x, abc, d, hex));
    client = ov_socket_create(config, true, NULL);

    testrun(ov_socket_log(server, NULL));
    testrun(ov_socket_log(server, "test"));
    testrun(ov_socket_log(server, "test %i %s %f %#x", x, abc, d, hex));

    close(client);
    close(server);

    unlink("localhost");
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_socket_configuration_list() {

    ov_list *list = ov_socket_configuration_list();
    ov_list *copy = NULL;

    testrun(list);
    testrun(list->config.item.free == socket_configuration_free);
    testrun(list->config.item.clear == socket_configuration_clear);
    testrun(list->config.item.copy == socket_configuration_copy);
    testrun(list->config.item.dump == socket_configuration_dump);

    ov_socket_configuration *config =
        calloc(1, sizeof(ov_socket_configuration));

    strcat(config->host, "test");
    config->port = 1234;
    config->type = UDP;

    testrun(ov_list_push(list, config));
    testrun(ov_list_dump(stdout, list));
    testrun(ov_list_copy((void **)&copy, list));
    testrun(1 == ov_list_count(copy));

    testrun(ov_list_clear(list));
    testrun(ov_list_is_empty(list));

    testrun(NULL == ov_list_free(list));
    testrun(NULL == ov_list_free(copy));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

/*
int test_ov_socket_config_from_args(){



        char *args[] = { "name", "--udp" };

        ov_socket_configuration config = ov_socket_config_from_args(2, args);
        testrun(     UDP == config.type);
        testrun(       0 == config.port)
        testrun(       0 == strncmp("0.0.0.0", config.host,
strlen(config.host)));

        args[0] = "name";
        args[1] = "-t";

        config = ov_socket_config_from_args(2, args);
        testrun(     TCP == config.type);
        testrun(       0 == config.port)
        testrun(       0 == strncmp("0.0.0.0", config.host,
strlen(config.host)));

        args[0] = "name";
        args[1] = "--dtls";
        args[2] = "--port";
        args[3] = "12345";
        args[2] = "--host";
        args[3] = "127.0.0.1";

        config = ov_socket_config_from_args(4, args);
        testrun(    DTLS == config.type);
        testrun(   12345 == config.port)
        testrun(       0 == strncmp("127.0.0.1", config.host,
strlen(config.host)));


        return testrun_log_success();
}
*/

/*----------------------------------------------------------------------------*/

int test_ov_socket_generate_5tuple() {

    size_t max_transport = 10;
    size_t max = (2 * OV_HOST_NAME_MAX) + 2 + 10 + max_transport + 1;
    char expect[max];
    memset(expect, 0, max);

    char *dest = NULL;
    size_t dest_len = 0;

    const char *transport = NULL;

    ov_socket_configuration socket_config =
        (ov_socket_configuration){.type = TCP, .host = "0.0.0.0"};

    ov_socket_data remote;
    memset(&remote, 0, sizeof(remote));

    int server = ov_socket_create(socket_config, false, NULL);
    testrun(server >= 0);
    testrun(ov_socket_get_config(server, &socket_config, NULL, NULL));
    int client = ov_socket_create(socket_config, true, NULL);
    testrun(ov_socket_get_data(client, &remote, NULL));
    transport = ov_socket_transport_to_string(socket_config.type);

    testrun(!ov_socket_generate_5tuple(NULL, NULL, 0, NULL));

    testrun(!ov_socket_generate_5tuple(NULL, &dest_len, server, &remote));
    testrun(!ov_socket_generate_5tuple(&dest, NULL, server, &remote));
    testrun(!ov_socket_generate_5tuple(&dest, &dest_len, server, NULL));

    testrun(snprintf(expect,
                     max,
                     "%s:%i%s%s:%i",
                     socket_config.host,
                     socket_config.port,
                     transport,
                     remote.host,
                     remote.port));

    testrun(ov_socket_generate_5tuple(&dest, &dest_len, server, &remote));
    testrun(dest);
    testrun(0 == strncmp(expect, dest, strlen(expect)));

    close(client);
    close(server);

    socket_config = (ov_socket_configuration){.type = UDP};

    server = ov_socket_create(socket_config, false, NULL);
    testrun(server >= 0);
    testrun(ov_socket_get_config(server, &socket_config, NULL, NULL));
    client = ov_socket_create(socket_config, true, NULL);
    testrun(ov_socket_get_data(client, &remote, NULL));
    transport = ov_socket_transport_to_string(socket_config.type);

    testrun(snprintf(expect,
                     max,
                     "%s:%i%s%s:%i",
                     socket_config.host,
                     socket_config.port,
                     transport,
                     remote.host,
                     remote.port));

    testrun(ov_socket_generate_5tuple(&dest, &dest_len, server, &remote));
    testrun(dest);
    testrun(0 == strncmp(expect, dest, strlen(expect)));

    close(client);
    close(server);

    free(dest);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_socket_is_dgram() {

    ov_socket_configuration config = {.type = UDP};

    int udp = ov_socket_create(config, false, NULL);
    config.type = TCP;
    int tcp = ov_socket_create(config, false, NULL);

    testrun(udp >= 0);
    testrun(tcp >= 0);

    testrun(ov_socket_is_dgram(udp));
    testrun(!ov_socket_is_dgram(tcp));
    testrun(!ov_socket_is_dgram(-1));

    close(udp);
    close(tcp);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_socket_open_server() {

    testrun(0 > ov_socket_open_server(0));

    ov_socket_configuration scfg = {0};

    testrun(0 > ov_socket_open_server(&scfg));

    strcpy(scfg.host, "localhost");
    testrun(0 > ov_socket_open_server(&scfg));

    scfg.type = TCP;

    int sfd = ov_socket_open_server(&scfg);
    testrun(-1 < sfd);
    testrun(0 != scfg.port);

    // try to actually connect to the presumedly opened port...
    int client = ov_socket_create(scfg, true, 0);
    testrun(-1 < client);

    close(client);
    close(sfd);

    return testrun_log_success();
}

/*---------------------------------------------------------------------------*/

int test_ov_socket_destination_address_type() {

    int type = 0;

    testrun(!ov_socket_destination_address_type(NULL, NULL));
    testrun(!ov_socket_destination_address_type("127.0.0.1", NULL));
    testrun(!ov_socket_destination_address_type(NULL, &type));

    testrun(ov_socket_destination_address_type("127.0.0.1", &type));
    testrun(type == AF_INET);
    testrun(ov_socket_destination_address_type("192.168.0.1", &type));
    testrun(type == AF_INET);
    testrun(ov_socket_destination_address_type("129.247.12.1", &type));
    testrun(type == AF_INET);
    testrun(ov_socket_destination_address_type("8.8.8.8", &type));
    testrun(type == AF_INET);
    testrun(ov_socket_destination_address_type("0.0.0.0", &type));
    testrun(type == AF_INET);
    testrun(ov_socket_destination_address_type("255.255.255.255", &type));
    testrun(type == AF_INET);

    testrun(ov_socket_destination_address_type("::", &type));
    testrun(type == AF_INET6);
    testrun(ov_socket_destination_address_type("::1", &type));
    testrun(type == AF_INET6);
    testrun(
        ov_socket_destination_address_type("fe80::5b8a:bf71:8968:ea0b", &type));
    testrun(type == AF_INET6);
    testrun(ov_socket_destination_address_type("2001:db8::1", &type));
    testrun(type == AF_INET6);

    testrun(!ov_socket_destination_address_type("0.0:0.0", &type));
    testrun(type == AF_UNSPEC);
    testrun(!ov_socket_destination_address_type("::1::", &type));
    testrun(type == AF_UNSPEC);

    return testrun_log_success();
}
/*----------------------------------------------------------------------------*/

int test_ov_socket_connect() {

    ov_socket_configuration config1 =
        (ov_socket_configuration){.host = "0.0.0.0", .type = UDP};

    ov_socket_configuration config2 =
        (ov_socket_configuration){.host = "0.0.0.0", .type = UDP};

    int socket1 = ov_socket_create(config1, false, NULL);
    int socket2 = ov_socket_create(config2, false, NULL);

    testrun(socket1 >= 0);
    testrun(socket2 >= 0);

    ov_socket_data data1 = {0};
    ov_socket_data data2 = {0};
    ov_socket_data data3 = {0};

    testrun(ov_socket_get_data(socket1, &data1, NULL));
    testrun(ov_socket_get_data(socket2, &data2, NULL));

    // connect socket2 to socket1

    testrun(!ov_socket_connect(-1, &data1.sa));
    testrun(!ov_socket_connect(socket2, NULL));
    testrun(!ov_socket_connect(-1, NULL));

    testrun(ov_socket_connect(socket2, &data1.sa));
    testrun(ov_socket_get_data(socket2, NULL, &data3));

    testrun(data1.sa.ss_family == data3.sa.ss_family);
    testrun(data1.port == data3.port);
    // Here will will get a specific IP interface
    // fprintf(stdout, "data3 %s\n", data3.host);
    // testrun(0 == strncmp(data1.host, data3.host, OV_HOST_NAME_MAX));

    data3 = (ov_socket_data){0};
    testrun(ov_socket_unconnect(socket2));
    testrun(ov_socket_get_data(socket2, &data3, NULL));

    testrun(data1.sa.ss_family == data2.sa.ss_family);
    testrun(data2.port == data3.port);
    // Here may get a specific IP interface
    // fprintf(stdout, "data3 %s\n", data3.host);
    // testrun(0 == strncmp(data2.host, data3.host, OV_HOST_NAME_MAX));

    testrun(ov_socket_unconnect(socket1));
    testrun(ov_socket_get_data(socket1, &data3, NULL));

    testrun(data1.sa.ss_family == data2.sa.ss_family);
    testrun(data1.port == data3.port);
    testrun(0 == strncmp(data1.host, data3.host, OV_HOST_NAME_MAX));

    close(socket2);
    config2 = (ov_socket_configuration){.host = "::1", .type = UDP};
    socket2 = ov_socket_create(config2, false, NULL);
    testrun(ov_socket_get_data(socket2, &data2, NULL));

    testrun(!ov_socket_connect(socket1, &data2.sa));
    close(socket1);
    config1 = (ov_socket_configuration){.host = "::1", .type = UDP};
    socket1 = ov_socket_create(config1, false, NULL);
    testrun(ov_socket_get_data(socket1, &data1, NULL));

    testrun(ov_socket_connect(socket1, &data2.sa));
    testrun(ov_socket_get_data(socket1, NULL, &data3));

    testrun(data1.sa.ss_family == data3.sa.ss_family);
    testrun(data2.port == data3.port);
    testrun(0 == strncmp(data2.host, data3.host, OV_HOST_NAME_MAX));

    testrun(ov_socket_unconnect(socket1));
    testrun(ov_socket_unconnect(socket2));

    // check we can send between unconnected sockets

    testrun(ov_socket_ensure_nonblocking(socket1));
    testrun(ov_socket_ensure_nonblocking(socket2));

    int r = -1;
    errno = EAGAIN;

    size_t len = sizeof(struct sockaddr_in6);

    while ((r == -1) && (errno == EAGAIN)) {

        r = sendto(socket1, "test", 4, 0, (struct sockaddr *)&data2.sa, len);
    }

    testrun(r == 4);

    char buf[10] = {0};

    r = -1;
    errno = EAGAIN;

    struct sockaddr_storage sa = {0};
    socklen_t sa_len = sizeof(sa);

    while ((r == -1) && (errno == EAGAIN)) {

        r = recvfrom(socket2, buf, 10, 0, (struct sockaddr *)&sa, &sa_len);
    }

    testrun(r == 4);
    testrun(0 == strncmp(buf, "test", 4));

    r = -1;
    errno = EAGAIN;

    while ((r == -1) && (errno == EAGAIN)) {

        r = sendto(socket2, "abcd", 4, 0, (struct sockaddr *)&data1.sa, len);
    }

    testrun(r == 4);

    memset(buf, 0, 10);

    r = -1;
    errno = EAGAIN;

    while ((r == -1) && (errno == EAGAIN)) {

        r = recvfrom(socket1, buf, 10, 0, (struct sockaddr *)&sa, &sa_len);
    }

    testrun(r == 4);
    testrun(0 == strncmp(buf, "abcd", 4));

    close(socket1);
    close(socket2);

    config1 = (ov_socket_configuration){.host = "::1", .type = UDP};
    socket1 = ov_socket_create(config1, false, NULL);
    testrun(ov_socket_get_data(socket1, &data1, NULL));

    config2 = (ov_socket_configuration){.host = "::1", .type = TCP};
    socket2 = ov_socket_create(config2, false, NULL);
    testrun(ov_socket_get_data(socket2, &data2, NULL));

    // we can connect a UDP to an TCP socket
    testrun(ov_socket_connect(socket1, &data2.sa));
    // we cannot connect a TCP to an UDP socket
    testrun(!ov_socket_connect(socket2, &data1.sa));

    close(socket1);
    close(socket2);
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_socket_unconnect() {

    ov_socket_configuration config1 =
        (ov_socket_configuration){.host = "127.0.0.1", .type = UDP};

    ov_socket_configuration config2 =
        (ov_socket_configuration){.host = "127.0.0.1", .type = UDP};

    int socket1 = ov_socket_create(config1, false, NULL);
    int socket2 = ov_socket_create(config2, false, NULL);

    testrun(socket1 >= 0);
    testrun(socket2 >= 0);

    ov_socket_data data1 = {0};
    ov_socket_data data2 = {0};
    ov_socket_data data3 = {0};

    testrun(ov_socket_get_data(socket1, &data1, NULL));
    testrun(ov_socket_get_data(socket2, &data2, NULL));
    testrun(0 != data1.port);
    testrun(0 != data2.port);

    testrun(!ov_socket_unconnect(-1));

    // unconnected socket
    testrun(ov_socket_unconnect(socket1));
    testrun(ov_socket_unconnect(socket2));

    // check port still set
    testrun(ov_socket_get_data(socket1, &data3, NULL));
    testrun(data3.port == data1.port);
    testrun(ov_socket_get_data(socket2, &data3, NULL));
    testrun(data3.port == data2.port);

    // connect socket2 to socket1
    testrun(ov_socket_connect(socket2, &data1.sa));

    testrun(ov_socket_get_data(socket2, NULL, &data3));
    testrun(data1.sa.ss_family == data3.sa.ss_family);
    testrun(data1.port == data3.port);
    testrun(0 == strncmp(data1.host, data3.host, OV_HOST_NAME_MAX));

    testrun(ov_socket_unconnect(socket2));
    testrun(ov_socket_get_data(socket2, &data3, NULL));

    testrun(data1.sa.ss_family == data2.sa.ss_family);
    testrun(data2.port == data3.port);
    // unconnect will set to all interfaces
    if (0 != strncmp("0.0.0.0", data3.host, OV_HOST_NAME_MAX))
        testrun(0 == strncmp("127.0.0.1", data3.host, OV_HOST_NAME_MAX))
            close(socket1);
    close(socket2);
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_socket_get_send_buffer_size() {

    ov_socket_configuration config =
        (ov_socket_configuration){.host = "127.0.0.1", .type = UDP};

    int socket = ov_socket_create(config, false, NULL);
    testrun(0 < socket);

    testrun(-1 == ov_socket_get_send_buffer_size(-1));
    int r = ov_socket_get_send_buffer_size(socket);
    testrun(r > 0);

    close(socket);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_socket_get_recv_buffer_size() {

    ov_socket_configuration config =
        (ov_socket_configuration){.host = "127.0.0.1", .type = UDP};

    int socket = ov_socket_create(config, false, NULL);
    testrun(0 < socket);

    testrun(-1 == ov_socket_get_recv_buffer_size(-1));
    int r = ov_socket_get_recv_buffer_size(socket);
    testrun(r > 0);

    close(socket);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_socket_configuration_to_sockaddr() {

    // bool ov_socket_configuration_to_sockaddr(const ov_socket_configuration
    // in,
    //                                          struct sockaddr_storage
    //                                          *sockaddr, socklen_t
    //                                          *sockaddr_len);

    ov_socket_configuration socket_conf = {

        .host = "localhost",
        .port = 2133,
        .type = UDP,

    };

    testrun(!ov_socket_configuration_to_sockaddr(socket_conf, 0, 0));

    struct sockaddr_storage soa = {0};

    socklen_t soa_len = sizeof(soa);

    testrun(!ov_socket_configuration_to_sockaddr(socket_conf, &soa, 0));
    testrun(!ov_socket_configuration_to_sockaddr(socket_conf, 0, &soa_len));
    testrun(ov_socket_configuration_to_sockaddr(socket_conf, &soa, &soa_len));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_socket_state_from_handle() {

    // bool ov_socket_state_from_handle(int socket_handle, ov_socket_state
    // *state);

    testrun(!ov_socket_state_from_handle(-1, 0));

    ov_socket_state state = {0};
    testrun(!ov_socket_state_from_handle(-1, &state));

    int s = socket(AF_INET, SOCK_DGRAM, 0);

    struct sockaddr_in saddr = {

        .sin_family = AF_INET,
        .sin_addr.s_addr = INADDR_ANY,

    };

    testrun(0 == bind(s, (struct sockaddr *)&saddr, sizeof(saddr)));

    memset(&state, 0, sizeof(state));

    testrun(ov_socket_state_from_handle(s, &state));

    close(s);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_socket_state_to_json() {

    ov_json_value *jval = ov_socket_state_to_json((ov_socket_state){0});
    testrun(0 != jval);

    jval = ov_json_value_free(jval);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/
OV_TEST_RUN("ov_socket",
            test_ov_socket_create,
            test_ov_socket_close,
            test_ov_socket_close_local,
            test_ov_socket_set_dont_fragment,
            test_ov_socket_ensure_nonblocking,
            test_ov_socket_set_reuseaddress,
            test_ov_socket_create_at_interface,
            test_ov_socket_config_from_sockaddr_storage,
            test_ov_socket_get_config,
            test_ov_socket_get_sockaddr_storage,
            test_ov_socket_parse_sockaddr_storage,
            test_ov_socket_data_from_sockaddr_storage,
            test_ov_socket_fill_sockaddr_storage,
            test_ov_socket_configuration_from_json,
            test_ov_socket_configuration_to_json,
            test_ov_socket_configuration_equals,
            test_ov_socket_transport_parse_string,
            test_ov_socket_transport_from_string,
            test_ov_socket_transport_to_string,
            test_ov_socket_data_to_string,
            test_ov_socket_log_error_with_config,
            test_ov_socket_log,
            test_ov_socket_configuration_list,
            test_ov_socket_get_data,
            test_ov_socket_get_data,
            test_ov_socket_generate_5tuple,
            test_ov_socket_is_dgram,
            test_ov_socket_connect,
            test_ov_socket_unconnect,
            test_ov_socket_open_server,
            test_ov_socket_destination_address_type,
            test_ov_socket_get_send_buffer_size,
            test_ov_socket_get_recv_buffer_size,
            test_ov_socket_configuration_to_sockaddr,
            test_ov_socket_state_from_handle,
            test_ov_socket_state_to_json);
