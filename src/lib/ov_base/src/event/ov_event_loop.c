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
        @file           ov_event_loop.c
        @author         Markus Toepfer
        @author         Michael Beer

        @date           2018-12-17

        @ingroup        ov_event_loop

        @brief          Implementation of ov_event_loop common functions.


        ------------------------------------------------------------------------
*/

#include "ov_event_loop.h"

#include "../../include/ov_utils.h"
#include <signal.h>
#include <sys/resource.h>
#include <sys/time.h>

#include "../../include/ov_constants.h"

// include default implementation
#include "../../include/ov_event_loop_poll.h"

#define IMPL_RECONNECT_MAGIC_BYTES 0xbbaa

/*----------------------------------------------------------------------------*/

static ov_event_loop *g_event_loop = 0;

/*----------------------------------------------------------------------------*/

static bool register_loop(ov_event_loop *loop) {

    if (0 == loop) {
        ov_log_error("Got 0 pointer");
        goto error;
    }

    if (0 != g_event_loop) {
        ov_log_critical("Another event loop already there");
        goto error;
    }

    g_event_loop = loop;

    return true;

error:

    return false;
}

/*----------------------------------------------------------------------------*/

bool unregister_event_loop(ov_event_loop *loop) {

    if ((NULL != g_event_loop) && g_event_loop != loop) {

        ov_log_error("Not unregistering loop - another loop was registered");
        goto error;
    }

    g_event_loop = 0;

    return true;

error:

    return false;
}

/*----------------------------------------------------------------------------*/

static void event_loop_stop_sighandler(int signum) {

    UNUSED(signum);

    if (0 == g_event_loop)
        return;

    g_event_loop->stop(g_event_loop);
}

/*----------------------------------------------------------------------------*/

bool ov_event_loop_run(ov_event_loop *self, uint64_t max_runtime_usec) {

    if (0 == self) {
        return false;
    } else {
        return self->run(self, max_runtime_usec);
    }
}

/*----------------------------------------------------------------------------*/

bool ov_event_loop_stop(ov_event_loop *self) {

    if (0 == self) {
        return false;
    } else {

        OV_ASSERT(0 != self->stop);
        return self->stop(self);
    }
}

/*---------------------------------------------------------------------------*/

ov_event_loop *ov_event_loop_cast(const void *self) {

    if (!self)
        return NULL;

    const ov_event_loop *loop = self;
    if (loop->magic_byte != OV_EVENT_LOOP_MAGIC_BYTE)
        return NULL;

    return (ov_event_loop *)loop;
}

/*----------------------------------------------------------------------------*/

bool ov_event_loop_setup_signals(ov_event_loop *loop) {

    if (!register_loop(loop)) {
        return false;
    }

    struct sigaction ignore = {
        .sa_handler = SIG_IGN,
        .sa_flags = SA_NOCLDSTOP | SA_NOCLDWAIT,

    };

    sigaction(SIGPIPE, &ignore, 0);

    struct sigaction stop_event_loop = {
        .sa_handler = event_loop_stop_sighandler,
    };

    sigaction(SIGINT, &stop_event_loop, 0);

    return true;
}

/*---------------------------------------------------------------------------*/

bool ov_event_loop_set_type(ov_event_loop *self, uint16_t type) {

    if (!self)
        return false;

    self->magic_byte = OV_EVENT_LOOP_MAGIC_BYTE;
    self->type = type;

    return true;
}

/*---------------------------------------------------------------------------*/

ov_event_loop_config ov_event_loop_config_default() {

    ov_event_loop_config config = {

        .max.sockets = OV_EVENT_LOOP_MAX_SOCKETS_DEFAULT,
        .max.timers = OV_EVENT_LOOP_MAX_TIMERS_DEFAULT};

    return config;
}

/*---------------------------------------------------------------------------*/

void *ov_event_loop_free(void *eventloop) {

    ov_event_loop *loop = ov_event_loop_cast(eventloop);

    if (!loop) {
        return eventloop;
    }

    if (!loop->free) {
        ov_log_error("eventloop free not found");
        return eventloop;
    }

    unregister_event_loop(loop);

    return loop->free(loop);
}

/*----------------------------------------------------------------------------*/

uint32_t ov_event_loop_timer_set(ov_event_loop *self, uint64_t relative_usec,
                                 void *data,
                                 bool (*callback)(uint32_t id, void *data)) {

    if (0 == self) {
        return OV_TIMER_INVALID;
    } else {
        return self->timer.set(self, relative_usec, data, callback);
    }
}

/*----------------------------------------------------------------------------*/

bool ov_event_loop_timer_unset(ov_event_loop *self, uint32_t id,
                               void **userdata) {

    if (0 == self) {
        return false;
    } else {
        return self->timer.unset(self, id, userdata);
    }
}

/*----------------------------------------------------------------------------*/

bool ov_event_loop_set(ov_event_loop *self, int sfd, uint8_t events,
                       void *userdata,
                       bool (*callback)(int socket_fd, uint8_t events,
                                        void *userdata)) {

    if (ov_cond_valid(-1 < sfd,
                      "Cannot set callback on socket fd: Invalid fd") &&
        ov_ptr_valid(self, "Cannot set callback on event loop: Invalid loop")) {

        return self->callback.set(self, sfd, events, userdata, callback);

    } else {

        return false;
    }
}

/*----------------------------------------------------------------------------*/

bool ov_event_loop_unset(ov_event_loop *self, int socket, void **userdata) {

    if (ov_ptr_valid(self,
                     "Cannot remove callback on event loop: Invalid loop")) {

        return self->callback.unset(self, socket, userdata);

    } else {

        return false;
    }
}

/*---------------------------------------------------------------------------*/

ov_event_loop *ov_event_loop_default(ov_event_loop_config config) {

    return ov_event_loop_poll(config);
}

/*---------------------------------------------------------------------------*/

ov_event_loop_config
ov_event_loop_config_adapt_to_runtime(ov_event_loop_config config) {

    // Extend the config to DEFAULT in case of 0 to MIN
    if (config.max.sockets == 0)
        config.max.sockets = OV_EVENT_LOOP_SOCKETS_MIN;

    if (config.max.timers == 0)
        config.max.timers = OV_EVENT_LOOP_TIMERS_MIN;

    struct rlimit limit = {0};

    // Limit config to system MAX

    if (0 != getrlimit(RLIMIT_NOFILE, &limit)) {
        ov_log_error("Failed to get system limit"
                     " of open files errno %i|%s",
                     errno, strerror(errno));
    }

    if (limit.rlim_cur != RLIM_INFINITY) {

        if (config.max.sockets > limit.rlim_cur) {
            config.max.sockets = limit.rlim_cur;
            ov_log_notice("Limited max sockets to system limit "
                          "of files open %" PRIu32,
                          config.max.sockets);
        }
    }

    if (limit.rlim_cur != RLIM_INFINITY) {

        if (config.max.timers > limit.rlim_cur) {
            config.max.timers = limit.rlim_cur;
            ov_log_notice("Limited max timers to system limit "
                          "of signals pending %" PRIu32,
                          config.max.timers);
        }
    }

    /*

    TBD check for TIMER_MAX limit

    // Limit to max timers
    if (config.max.timers > TIMER_MAX){
            config.max.timers = TIMER_MAX;
                    ov_log_notice(  "Limited max timers to system limit "
                                    "of TIMER_MAX %jd",
                            config.max.timers);
    }
    */
    return config;
}

/*---------------------------------------------------------------------------*/

struct accept_container {

    ov_event_loop *loop;
    uint8_t events;
    void *data;
    bool (*callback)(int socket, uint8_t events, void *data);
};

/*---------------------------------------------------------------------------*/

static void *accept_container_free(void *data) {

    if (data)
        free(data);
    return NULL;
}

/*---------------------------------------------------------------------------*/

static bool accept_callback(int socket_fd, uint8_t events, void *data) {

    int nfd = 0;

    struct accept_container *container = (struct accept_container *)data;
    ov_event_loop *loop = container->loop;

    if (!loop)
        goto error;

    if (socket_fd < 1)
        goto error;

    if ((events & OV_EVENT_IO_CLOSE) || (events & OV_EVENT_IO_ERR)) {

        void *userdata = NULL;

        if (!loop->callback.unset(loop, socket_fd, &userdata)) {
            ov_log_error("Failed to unset accept callback");
            goto error;
        }

        accept_container_free(userdata);

        return true;
    }

    // accept MUST have some incoming IO
    if (!(events & OV_EVENT_IO_IN))
        goto error;

    struct sockaddr_storage remote_sa = {0};
    struct sockaddr_storage local_sa = {0};
    // struct sockaddr_un un_sa;

    socklen_t remote_sa_len = sizeof(remote_sa);

    char local_ip[OV_HOST_NAME_MAX] = {0};
    char remote_ip[OV_HOST_NAME_MAX] = {0};

    uint16_t local_port = 0;
    uint16_t remote_port = 0;

    // get local SA
    if (!ov_socket_get_sockaddr_storage(socket_fd, &local_sa, NULL, NULL))
        goto error;

    nfd = accept(socket_fd, (struct sockaddr *)&remote_sa, &remote_sa_len);

    // parse debug logging data
    switch (local_sa.ss_family) {

    case AF_INET:
    case AF_INET6:

        if (!ov_socket_parse_sockaddr_storage(&local_sa, local_ip,
                                              OV_HOST_NAME_MAX, &local_port)) {
            ov_log_error("Failed to parse data "
                         "from socket fd %i",
                         socket_fd);
            goto error;
        }

        if (nfd < 0)
            break;

        if (!ov_socket_parse_sockaddr_storage(&remote_sa, remote_ip,
                                              OV_HOST_NAME_MAX, &remote_port)) {
            ov_log_error("Failed to parse data "
                         "from socket fd %i",
                         nfd);
            goto error;
        }

        break;

    case AF_UNIX:

        break;

    default:
        ov_log_error("Family %i not supported", local_sa.ss_family);
        goto error;
    }

    if (nfd < 0) {

        ov_log_error("Failed to accept at socket %i", socket_fd);
        goto error;
    }

    if (!ov_socket_ensure_nonblocking(nfd))
        goto error;

    if (local_ip[0] != 0) {

        ov_log_debug("accepted at socket fd %i | "
                     "LOCAL %s:%i REMOTE %s:%i | "
                     "new connection fd %i",
                     socket_fd, local_ip, local_port, remote_ip, remote_port,
                     nfd);

    } else {

        ov_log_debug("accepted at socket fd %i | "
                     "new connection fd %i",
                     socket_fd, nfd);
    }

    if (!loop->callback.set(loop, nfd, container->events, container->data,
                            container->callback))
        goto error;

    return true;

error:
    if (nfd > -1)
        close(nfd);
    return false;
}

/*---------------------------------------------------------------------------*/

bool ov_event_add_default_connection_accept(
    ov_event_loop *loop, int socket, uint8_t events, void *data,
    bool (*callback)(int connection_socket, uint8_t connection_events,
                     void *data)) {

    struct accept_container *container = NULL;

    if (!loop || !callback || (events == 0))
        goto error;

    int so_opt;
    socklen_t so_len = sizeof(so_opt);

    // check if socket is without error
    if (0 != getsockopt(socket, SOL_SOCKET, SO_ERROR, &so_opt, &so_len))
        goto error;

    // check if socket is streaming socket
    if (0 != getsockopt(socket, SOL_SOCKET, SO_TYPE, &so_opt, &so_len))
        goto error;

    if (so_opt != SOCK_STREAM) {
        ov_log_error("accept applied to non streaming socket!");
        goto error;
    }

    container = calloc(1, sizeof(struct accept_container));

    container->loop = loop;
    container->events = events;
    container->data = data;
    container->callback = callback;

    if (!loop->callback.set(
            loop, socket, OV_EVENT_IO_IN | OV_EVENT_IO_CLOSE | OV_EVENT_IO_ERR,
            container, accept_callback))
        goto error;

    return true;
error:
    if (container)
        free(container);

    return false;
}

/*---------------------------------------------------------------------------*/

bool ov_event_remove_default_connection_accept(ov_event_loop *self, int s) {

    if (!self)
        return false;

    void *userdata = NULL;

    bool result = self->callback.unset(self, s, &userdata);

    if (userdata)
        accept_container_free(userdata);
    return result;
}

/*---------------------------------------------------------------------------*/

ov_event_loop_config
ov_event_loop_config_from_json(const ov_json_value *value) {

    ov_event_loop_config config = {0};

    if (!value)
        goto error;

    const ov_json_value *obj = ov_json_object_get(value, OV_EVENT_LOOP_KEY);
    if (!obj)
        obj = value;

    double sockets = ov_json_number_get(
        ov_json_object_get(obj, OV_EVENT_LOOP_KEY_MAX_SOCKETS));

    double timers = ov_json_number_get(
        ov_json_object_get(obj, OV_EVENT_LOOP_KEY_MAX_TIMERS));

    if (sockets > UINT32_MAX)
        goto error;

    if (timers > UINT32_MAX)
        goto error;

    config.max.sockets = (uint32_t)sockets;
    config.max.timers = (uint32_t)timers;

    return config;
error:
    return (ov_event_loop_config){0};
}

/*---------------------------------------------------------------------------*/

ov_json_value *ov_event_loop_config_to_json(ov_event_loop_config config) {

    ov_json_value *val = NULL;
    ov_json_value *out = ov_json_object();
    if (!out)
        goto error;

    val = ov_json_number(config.max.timers);
    if (!ov_json_object_set(out, OV_EVENT_LOOP_KEY_MAX_TIMERS, val))
        goto error;

    val = ov_json_number(config.max.sockets);
    if (!ov_json_object_set(out, OV_EVENT_LOOP_KEY_MAX_SOCKETS, val))
        goto error;

    return out;
error:
    ov_json_value_free(out);
    ov_json_value_free(val);
    return NULL;
}
