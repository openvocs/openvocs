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
        @author         Michael Beer

        @date           2020-04-05

        @ingroup        ov_test

        ------------------------------------------------------------------------
*/
#include "../include/ov_test_tcp.h"

/*----------------------------------------------------------------------------*/

#include <netdb.h>
#include <netinet/tcp.h>
#include <signal.h>
#include <sys/socket.h>
#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>

#include <fcntl.h>

#include <stdbool.h>
#include <string.h>

#include <assert.h>
#include <errno.h>
#include <poll.h>
#include <pthread.h>
#include <stdarg.h>
#include <sys/wait.h>
#include <time.h>
#if defined __linux__
#include <sys/prctl.h>
#endif

/*----------------------------------------------------------------------------*/

static bool log_to_file = false;

#define ERR(...) err(__LINE__, __VA_ARGS__)
#define OUT(...) out(__LINE__, __VA_ARGS__)

static FILE *g_log_file = 0;

/*----------------------------------------------------------------------------*/

static char *make_log_file_name(char *buf, size_t len) {

    if (0 == buf) {
        return "/tmp/test_tcp.log";
    } else {
        snprintf(buf, len, "/tmp/test_tcp-%i.log", getpid());
        return buf;
    }
}

/*----------------------------------------------------------------------------*/

static FILE *try_open_log_file(char const *fname) {

    if (0 == fname) {

        return 0;

    } else {

        char buf[255] = {0};

        int logfd = open(make_log_file_name(buf, sizeof(buf)),
                         O_WRONLY | O_CLOEXEC | O_CREAT | O_TRUNC, S_IRWXU);

        if (0 > logfd) {

            fprintf(stderr, "Could not open log file: %s\n", strerror(errno));
        }

        return fdopen(logfd, "w");
    }
}

/*----------------------------------------------------------------------------*/

static FILE *open_log_file(char const *fname) {

    FILE *f = try_open_log_file(fname);

    if (0 == f) {

        return stdout;

    } else {

        g_log_file = f;
        return f;
    }
}

/*----------------------------------------------------------------------------*/

static FILE *get_log_file() {

    if (0 != g_log_file) {

        return g_log_file;

    } else if (log_to_file) {

        char buf[300] = {0};

        return open_log_file(make_log_file_name(buf, sizeof(buf)));

    } else {

        return stdout;
    }
}

/*----------------------------------------------------------------------------*/

static bool no_output = false;

static char *make_our_msg(char const *prefix, char const *msg, int lineno) {

    if (0 == msg) {

        return 0;

    } else {

        char errno_str[10] = {0};
        snprintf(errno_str, sizeof(errno_str), "%i", errno);
        errno_str[9] = 0;

        size_t len = strlen(msg);
        len += 10 + 12 + 500;
        char *header = malloc(len);
        snprintf(header, len, "%7i - %s [errno: ", lineno, prefix);
        strcat(header, errno_str);

        strcat(header, "  ");
        strcat(header, strerror(errno));

        strcat(header, "] ");

        char *our_msg = malloc(len);
        strcpy(our_msg, header);
        strcat(our_msg, msg);
        strcat(our_msg, "\n");

        free(header);

        return our_msg;
    }
}

/*----------------------------------------------------------------------------*/

static void err(int lineno, char const *msg, ...) {

    if (no_output) {

        return;

    } else {

        FILE *log_file = get_log_file();

        char *our_msg = make_our_msg("ERROR", msg, lineno);

        if (0 != our_msg) {

            va_list va;
            va_start(va, msg);

            vfprintf(log_file, our_msg, va);

            va_end(va);
            free(our_msg);
        }
    }
}

/*----------------------------------------------------------------------------*/

static void out(int lineno, char const *msg, ...) {

    if (no_output) {

        return;

    } else {

        FILE *log_file = get_log_file();

        char *our_msg = make_our_msg("INFO ", msg, lineno);

        if (0 != our_msg) {

            va_list va;
            va_start(va, msg);
            vfprintf(log_file, our_msg, va);
            va_end(va);
            fprintf(log_file, "\n");

            free(our_msg);
        }
    }
}

/*----------------------------------------------------------------------------*/

int ov_test_get_random_port() {

    uint64_t port = (uint32_t)random();
    port *= UINT16_MAX;

    port = port / UINT32_MAX;

    if (1024 > port) {
        port += 1024;
    }

    return port;
}

/*----------------------------------------------------------------------------*/

int ov_test_tcp_disable_nagls_alg(int fd) {

    int flag = 1;
    int result =
        setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));

    return result;
}

/*----------------------------------------------------------------------------*/

int ov_test_tcp_read_int(int fd, int *target) {

    size_t read_octets = 0;

    unsigned char *target_ptr = (unsigned char *)target;

    while (sizeof(int) > read_octets) {

        int retval = read(fd, target_ptr, sizeof(int) - read_octets);

        if (0 == retval) {
            /*EOF*/
            close(fd);
            return 0;
        }

        if (0 > retval) {

            return retval;
        }

        read_octets += retval;
        target_ptr += retval;
    }

    char str[sizeof(int) + 1];
    memcpy(str, target, sizeof(int));
    str[sizeof(int)] = 0;

    return read_octets;
}

/*----------------------------------------------------------------------------*/

static bool did_write_succeed(ssize_t v) {

    if (0 > v) {

        ERR("Could not send");
        return false;

    } else {

        return true;
    }
}

/*----------------------------------------------------------------------------*/

bool ov_test_tcp_send(int fd, char *msg) {

    if ((0 > fd) || (0 == msg)) {

        return false;

    } else {

        return did_write_succeed(write(fd, msg, strlen(msg)));
    }
}

/*----------------------------------------------------------------------------*/

static void *sfree(void *ptr) {

    if (0 != ptr) {
        free(ptr);
        return 0;
    } else {
        return ptr;
    }
}

/*----------------------------------------------------------------------------*/

static size_t string_length(char const *str) {

    if (0 != str) {
        return strlen(str);
    } else {
        return 0;
    }
}

/*----------------------------------------------------------------------------*/

static bool mem_equals(void const *ptr1, void const *ptr2, size_t len) {

    if (0 == ptr1) {

        return 0 == ptr2;

    } else if (0 == ptr2) {

        assert(0 != ptr1);
        return false;

    } else if (0 == len) {

        assert((0 != ptr1) && (0 != ptr2));
        return true;

    } else {

        assert((0 != ptr1) && (0 != ptr2) && (0 < len));
        return 0 == memcmp(ptr1, ptr2, len);
    }
}

/*----------------------------------------------------------------------------*/

static bool setsockettimeouts(int fd) {

    struct timeval timeout = {
        .tv_sec = 100,
        .tv_usec = 0,
    };

    if (0 > fd) {

        ERR("invalid fd");
        return false;

    } else if (0 > setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout,
                              sizeof timeout)) {

        ERR("Could not set Read timeout");
        return false;

    } else if (0 > setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &timeout,
                              sizeof timeout)) {

        ERR("Could not set Send timeout");
        return false;

    } else {

        return true;
    }
}

/*----------------------------------------------------------------------------*/

static uint8_t *from_socket(int fd, size_t len) {

    if (!setsockettimeouts(fd)) {

        return 0;

    } else if (0 == len) {

        ERR("len is zero");
        return 0;

    } else {

        sleep(2); // Sometimes if we are too fast - for some reason
                  // read(2) then would return with EAGAIN immediately...

        uint8_t *buf = calloc(1, len);
        uint8_t *wr_ptr = buf;

        while (len > 0) {

            ssize_t nbytes = read(fd, wr_ptr, len);

            if (nbytes < 0) {

                ERR("Read failed");
                free(buf);
                buf = 0;
                break;

            } else {

                wr_ptr += nbytes;
                len -= nbytes;
            }
        }

        return buf;
    }
}

/*----------------------------------------------------------------------------*/

bool ov_test_tcp_received(int fd, char const *msg) {

    size_t len = string_length(msg);
    uint8_t *received = from_socket(fd, len);
    bool have_received = mem_equals(msg, received, len);
    received = sfree(received);

    if (have_received) {

        OUT("received %s", msg);
        return true;

    } else {

        ERR("Did not receive %s", msg);
        return false;
    }
}

/*****************************************************************************
                                   TCP Server
 ****************************************************************************/

static int port_from_addr(struct sockaddr *sa) {

    assert(0 != sa);

    int port = -1;

    switch (sa->sa_family) {

    case AF_INET:
        port = ((struct sockaddr_in *)sa)->sin_port;
        break;

    case AF_INET6:
        port = ((struct sockaddr_in6 *)sa)->sin6_port;
        break;

    default:

        assert(!"UNSUPPORTED INTERNET FAMILY");
    }

    return ntohs(port);
}

/******************************************************************************
 *                              TCP Test Server
 ******************************************************************************/

static void *cb_client_fd_userdata = 0;

static void (*cb_serve_client)(int fd, void *userdata);

/*----------------------------------------------------------------------------*/

static int listen_fd = -1;

static void sighandler_close_listen_fd(int signum) {

    do {
        (void)(signum);
    } while (0);

    close(listen_fd);
}

/*----------------------------------------------------------------------------*/

static void handle_client_connection(int fd) {

    if (fd <= 0)
        return;

    ov_test_tcp_disable_nagls_alg(fd);
    cb_serve_client(fd, cb_client_fd_userdata);
}

/*----------------------------------------------------------------------------*/

static void serve_to_kill(int fd) {

    struct sockaddr_storage sa = {0};
    socklen_t sa_len = 0;

    listen_fd = fd;

    if (0 != listen(listen_fd, 50)) {

        exit(1);
    }

    struct sigaction sigact = {

        .sa_handler = sighandler_close_listen_fd,

    };

    sigaction(SIGTERM, &sigact, 0);

    int client_fd = 1;

    while (true) {

        OUT("Waiting for another round...");

        client_fd = accept(listen_fd, (void *)&sa, &sa_len);

        handle_client_connection(client_fd);

        if (-1 != client_fd)
            close(client_fd);

        OUT("Connection done - next round...");
    }
}

/*----------------------------------------------------------------------------*/

enum cb_loopback_state { WAITING, Q, U, I, T };

static enum cb_loopback_state propagate_state(enum cb_loopback_state state,
                                              char *buf, size_t nbytes) {

    assert(0 != buf);

    for (size_t i = 0; i < nbytes; ++i) {

        char c = buf[i];

        switch (state) {

        case WAITING:

            if ('q' == c) {
                state = Q;
            } else {
                state = WAITING;
            }

            break;

        case Q:

            if ('u' == c) {
                state = U;
            } else {
                state = WAITING;
            }
            break;

        case U:

            if ('i' == c) {
                state = I;
            } else {
                state = WAITING;
            }
            break;

        case I:

            if ('t' == c) {
                state = T;
            } else {
                state = WAITING;
            }
            break;

        case T:
            break;
        }

        if (T == state) {
            break;
        }
    }

    return state;
}

/*----------------------------------------------------------------------------*/

void ov_test_tcp_server_loopback(int fd, void *unused) {

    do {
        (void)(unused);
    } while (0);

    enum cb_loopback_state state = WAITING;

    char buf[512];

    while (T != state) {

        OUT("Server waiting for data...");

        ssize_t nbytes = read(fd, &buf, sizeof(buf));

        if (nbytes < 0) {
            ERR("Read failed");
            state = T;
        } else {

            state = propagate_state(state, buf, (size_t)nbytes);
            did_write_succeed(write(fd, buf, nbytes));
        }
    }

    OUT("Received `quit` - Terminating connection...");
}

/*----------------------------------------------------------------------------*/

static void dummy_tcp_handler(int fd, void *unused) {

    do {
        (void)(unused);
    } while (0);

    size_t total_bytes_read = 0;
    ssize_t num_bytes_read = 1;

    char buf[512];

    do {
        total_bytes_read += num_bytes_read;
        num_bytes_read = read(fd, &buf, sizeof(buf));
    } while (num_bytes_read > 0);

    OUT("Read %zu bytes", total_bytes_read);

    if (0 > num_bytes_read) {

        ERR("Could not read from socket");
    }
}

/*----------------------------------------------------------------------------*/

static char const *port_to_str(char *buf, size_t buflen, int port) {

    if (0 == port) {

        OUT("Using arbitrary port to bind to");
        return 0;

    } else if (0 == buf) {

        ERR("No buffer given");
        return 0;

    } else {

        snprintf(buf, buflen, "%i", port);
        return buf;
    }
}

/*----------------------------------------------------------------------------*/

typedef struct server_socket {
    int fd;
    int port;
} ServerSocket;

static ServerSocket open_server_socket(int port) {

    ServerSocket sock = {
        .fd = -1,
        .port = 0,
    };

#if defined __linux__
    int fd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
#else
    int fd = socket(AF_INET, SOCK_STREAM, 0);
#endif

    while (0 == fd) {
        // We do not want an fd == 0 since this is usually stdout...
        int fd_old = fd;

#if defined __linux__
        fd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
#else
        fd = socket(AF_INET, SOCK_STREAM, 0);
#endif
        close(fd_old);
    }

    if (0 > fd) {
        ERR("Could not create socket");
        return sock;
    }

    int enable = 1;

    if (0 !=
        setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable))) {
        ERR("Could not enable socket reuse");
        close(fd);
        return sock;
    }

    char port_str[7] = {0};

    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    struct addrinfo *result;

    if (0 != getaddrinfo("127.0.0.1",
                         port_to_str(port_str, sizeof(port_str), port), &hints,
                         &result)) {
        close(fd);
        return sock;
    }

    if (0 != bind(fd, result->ai_addr, result->ai_addrlen)) {
        ERR("Could not bind fd");
        close(fd);
        return sock;
    }

    if (0 != listen(fd, 2)) {
        ERR("Could not listen on %i", fd);
        close(fd);
        return sock;
    }

    struct sockaddr_storage sas;
    socklen_t sa_len = sizeof(sas);

    /* Unfortunately, ov_socket_config_from_sockaddr_storage does only
     * parse addr->sin_port / sin6_port
     * thus we cannot use it here
     */
    if (0 != getsockname(fd, (struct sockaddr *)&sas, &sa_len)) {
        ERR("Could not get socket name");
        close(fd);
        return sock;
    }

    struct sockaddr *sa = (struct sockaddr *)&sas;

    int server_port = port_from_addr(sa);

    fprintf(stderr, "Opened server port on %i, expected %i\n", server_port,
            port);

    freeaddrinfo(result);

    sock.fd = fd;
    sock.port = server_port;

    return sock;
}

/*----------------------------------------------------------------------------*/

static void ensure_we_die_with_parent() {

#if defined __linux__
    prctl(PR_SET_PDEATHSIG, SIGHUP);
#endif
}

/*----------------------------------------------------------------------------*/

bool ov_test_tcp_server(pid_t *server_pid, int *server_port,
                        ov_test_tcp_server_config cfg) {

    assert(0 != server_port);

    if (0 > *server_port) {
        *server_port = 0;
    }

    if (0 == cfg.serve_client_callback) {
        cfg.serve_client_callback = dummy_tcp_handler;
    }

    ServerSocket ss = open_server_socket(*server_port);

    pid_t pid = fork();

    if (0 == pid) {

        log_to_file = true;
        ensure_we_die_with_parent();
        cb_client_fd_userdata = cfg.userdata;
        cb_serve_client = cfg.serve_client_callback;
        serve_to_kill(ss.fd);
        exit(EXIT_SUCCESS);

    } else {

        /* We are the parent process */

        close(ss.fd);
        ss.fd = -1;

        if (0 != server_pid) {
            *server_pid = pid;
        }

        *server_port = ss.port;

        return true;
    }
}

/*----------------------------------------------------------------------------*/

bool ov_test_tcp_stop_process(pid_t pid) {

    OUT("We are %i", getpid());

    OUT("Sending SIGTERM to %i", pid);
    kill(pid, SIGTERM);
    sleep(2);
    OUT("Sending SIGKILL to %i\n", pid);
    kill(pid, SIGKILL);

    int wstat = 0;

    if (0 > waitpid(pid, &wstat, 0)) {

        ERR("Could not wait for tcp server process to terminate");
        return false;

    } else {

        OUT("TCP Server process exited");
        return true;
    }
}

/*----------------------------------------------------------------------------*/
