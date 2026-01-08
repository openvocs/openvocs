/***

  Copyright   2018    German Aerospace Center DLR e.V., German Space Operations
Center (GSOC)

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

 ***/
/**
 * @author Michael J. Beer
 *
 */
/*----------------------------------------------------------------------------*/

#include <netdb.h>
#include <ov_base/ov_utils.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <ov_base/ov_rtp_frame.h>

#include <ov_os/ov_os_event_loop.h>

#include <ov_codec/ov_codec_factory.h>
#include <ov_codec/ov_codec_pcm16_signed.h>

#include "ov_base/ov_rtcp.h"
#include "ov_rtp_client.h"
#include "ov_rtp_client_receive.h"
#include "ov_rtp_client_send.h"
#include <arpa/inet.h>
#include <ov_base/ov_rtcp.h>
#include <sys/ioctl.h>

/******************************************************************************
 *                           SYSTEMD EVENT HANDLERS
 ******************************************************************************/

ov_event_loop *g_event = 0;

static void exit_handler(int s) {

    UNUSED(s);

    if (0 == g_event) {

        fprintf(stderr, "event loop not set\n");
        goto error;
    }

    /* Exit event loop */

    fprintf(stdout, "Stopping...\n");

    g_event->stop(g_event);

    g_event = 0;

    /* Pulse will be disconnected when the loop ended ...*/

error:

    NOP;
}

/******************************************************************************
 *                         CONSTRUCTOR HELPERS
 ******************************************************************************/

static bool setup_event_loop(ov_rtp_client *client) {

    if (0 == client) {
        ov_log_error("got no client (0 pointer)");
        goto error;
    }

    client->event = ov_os_event_loop(ov_event_loop_config_default());

    if (0 == client->event) {
        goto error;
    }

    g_event = client->event;

    /* And install exit handler for all kinds of signals -
       Ruthlessly copied over from ov/ov_base/src/ov_server.c */

    int r = -1;

    sigset_t mask;

    r = sigemptyset(&mask);
    if (r != 0) {
        fprintf(stderr, "could not create empty signal set.\n");
        goto error;
    }

    r = sigaddset(&mask, SIGINT);
    if (r != 0) {
        fprintf(stderr, "signal mask could not add SIGINT\n");
        goto error;
    }

    r = sigaddset(&mask, SIGTERM);
    if (r != 0) {
        fprintf(stderr, "signal mask could not add SIGTERM\n");
        goto error;
    }

    r = sigaddset(&mask, SIGQUIT);
    if (r != 0) {
        fprintf(stderr, "signal mask could not add SIGQUIT\n");
        goto error;
    }

    // block the signals to add them to the client->event loop
    if (pthread_sigmask(SIG_BLOCK, &mask, NULL) != 0) {
        fprintf(stderr, "Failure blocking signal with signal "
                        "mask.\n");
        goto error;
    }

    if (SIG_ERR == signal(SIGTERM, exit_handler)) {

        fprintf(stderr, "Failure registering for SIGTERM\n");
        goto error;
    }

    if (SIG_ERR == signal(SIGQUIT, exit_handler)) {

        fprintf(stderr, "Failure registering for SIGQUIT\n");
        goto error;
    }

    if (SIG_ERR == signal(SIGINT, exit_handler)) {

        fprintf(stderr, "Failure registering for SIGINT\n");
        goto error;
    }

    if (pthread_sigmask(SIG_UNBLOCK, &mask, NULL) != 0) {
        fprintf(stderr, "Failure blocking signal with signal "
                        "mask.\n");
        goto error;
    }

    return true;
error:

    if ((0 != client) && (0 != client->event)) {

        client->event = client->event->free(client->event);
        g_event = 0;
    }

    return false;
}

/*----------------------------------------------------------------------------*/

static int try_bind_to(char const *local_if, uint16_t local_port, bool mc) {

    int sfd = -1;

    char port[5 + 1] = {0};

    snprintf(port, 6, "%i", local_port);
    port[5] = 0;

    struct addrinfo *result = 0;
    struct addrinfo hints = {0};

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    if (!mc) {
        hints.ai_flags = AI_PASSIVE;
    }

    int s = getaddrinfo(local_if, port, &hints, &result);

    if (s != 0) {
        fprintf(stderr, "getaddrinfo failed for %s: %s\n", local_if,
                gai_strerror(s));
    } else {

        int opt = 0;
        socklen_t optlen = sizeof(opt);

        int optone = 1;

        for (struct addrinfo *rp = result; rp != NULL; rp = rp->ai_next) {
            sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);

            if ((-1 < sfd) &&
                (0 == getsockopt(sfd, SOL_SOCKET, SO_TYPE, &opt, &optlen)) &&
                (0 == getsockopt(sfd, SOL_SOCKET, SO_ERROR, &opt, &optlen)) &&
                (0 <= setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &optone,
                                 sizeof(optone))) &&
                (0 == bind(sfd, rp->ai_addr, rp->ai_addrlen))) {
                break;
            } else {
                close(sfd);
                sfd = -1;
            }
        }
    }

    if (0 != result) {
        freeaddrinfo(result);
        result = 0;
    }

    return sfd;
}

/*----------------------------------------------------------------------------*/

static int open_rtcp_socket(ov_socket_data rtp_socket_data) {
    ov_socket_configuration cfg = {
        .port = rtp_socket_data.port,
        .type = UDP,
    };

    memcpy(cfg.host, rtp_socket_data.host, sizeof(rtp_socket_data.host));

    return ov_socket_create(cfg, false, 0);
}

/*----------------------------------------------------------------------------*/

static bool enable_multicast_loopback(int fd) {
    int loop = 1;

    int retval =
        setsockopt(fd, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop));

    if (0 != retval) {
        fprintf(stderr, "Could not enable mutlicast loopback: %i (%s / %s)\n",
                retval, strerror(errno), strerror(retval));

    } else {
        fprintf(stdout, "Enabled multicast loopback\n");
    }

    return 0 == retval;
}

/*----------------------------------------------------------------------------*/

static bool join_multicast_group(int fd, struct ip_mreq command,
                                 char const *group) {
    int retval = setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &command,
                            sizeof(command));

    if (0 == retval) {
        fprintf(stdout, "Joined multicast group %s\n", group);

    } else {
        fprintf(stderr, "Could not join multicast group %s: %i (%s / %s)\n",
                group, retval, strerror(retval), strerror(errno));
    }

    return 0 == retval;
}

/*----------------------------------------------------------------------------*/

static bool setup_multicast(int fd, char const *mc_group) {
    if (0 == mc_group) {
        return true;

    } else {
        static struct ip_mreq command = {0};
        command.imr_interface.s_addr = htonl(INADDR_ANY);
        command.imr_multiaddr.s_addr = inet_addr(mc_group);

        return enable_multicast_loopback(fd) &&
               join_multicast_group(fd, command, mc_group);
    }
}

/*----------------------------------------------------------------------------*/

static bool connect_udp(ov_rtp_client *client, ov_rtp_client_parameters *cp) {
    int sfd =
        try_bind_to(cp->local_if, cp->local_port, (0 != cp->multicast_group));

    ov_socket_data local = {0};
    ov_socket_get_data(sfd, &local, 0);

    if (0 > sfd) {
        fprintf(stderr, "Could not bind to %s:%" PRIu16 "\n", cp->local_if,
                cp->local_port);

        return false;

    } else if (!ov_socket_ensure_nonblocking(sfd)) {
        fprintf(stderr, "could not ensure a non-blocking socket.\n");
        close(sfd);
        return false;

    } else if (!setup_multicast(sfd, cp->multicast_group)) {
        fprintf(stderr, "Multicast group could not be joined");
        return false;
    }

    ov_socket_data rtcp_socket = local;
    ++rtcp_socket.port;
    int rtcp_fd = open_rtcp_socket(rtcp_socket);

    if (0 > rtcp_fd) {
        fprintf(stderr, "Could not open RTCP port at %s:%" PRIu16 " :%s\n",
                rtcp_socket.host, rtcp_socket.port, strerror(errno));
        close(sfd);
        return false;
    }

    fprintf(stderr, "Bound to %s:%" PRIu16 " - RTCP at %s:%" PRIu16 "\n",
            local.host, local.port, rtcp_socket.host, rtcp_socket.port);
    fflush(stderr);

    char port[6] = {0};

    snprintf(port, 6, "%i", cp->remote_port);
    port[5] = 0;

    struct addrinfo *result;
    struct addrinfo *rtcp_remote;
    struct addrinfo hints = {0};

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    int s = getaddrinfo(cp->remote_if, port, &hints, &result);
    if (s != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        goto error;
    }

    snprintf(port, 6, "%i", 1 + cp->remote_port);
    port[5] = 0;

    s = getaddrinfo(cp->remote_if, port, &hints, &rtcp_remote);
    if (s != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        goto error;
    }

    if (SEND == client->mode) {
        client->send.dest_sockaddr_len = result->ai_addrlen;
        client->send.dest_sockaddr = calloc(1, client->send.dest_sockaddr_len);
        memcpy(client->send.dest_sockaddr, result->ai_addr,
               client->send.dest_sockaddr_len);

        client->send.rtcp_dest_sockaddr_len = rtcp_remote->ai_addrlen;
        client->send.rtcp_dest_sockaddr =
            calloc(1, client->send.dest_sockaddr_len);
        memcpy(client->send.rtcp_dest_sockaddr, rtcp_remote->ai_addr,
               client->send.rtcp_dest_sockaddr_len);
    }

    freeaddrinfo(result);
    result = 0;
    freeaddrinfo(rtcp_remote);
    rtcp_remote = 0;

    fprintf(stderr, "Remote host %s:%i\n", cp->remote_if, cp->remote_port);

    client->udp_socket = sfd;
    client->rtcp_socket = rtcp_fd;

    return true;

error:

    if (sfd)
        close(sfd);

    return false;
}

/*----------------------------------------------------------------------------*/

static bool set_codec(ov_rtp_client *client, ov_rtp_client_parameters *cp,
                      ov_rtp_client_audio_parameters *ap) {
    if (0 == client)
        goto error;
    if (0 == cp)
        goto error;
    if (0 == ap)
        goto error;

    ov_json_value *codec_parameters = ov_json_object();
    ov_codec_parameters_set_sample_rate_hertz(
        codec_parameters, ap->general_config.sample_rate_hertz);

    if (0 == ap->codec_name) {
        ap->codec_name = ov_codec_pcm16_signed_id();
    }

    client->codec = ov_codec_factory_get_codec(
        0, ap->codec_name, client->send.ssrc_id, codec_parameters);

    codec_parameters = ov_json_value_free(codec_parameters);

    if (0 == client->codec) {
        fprintf(stderr, "Could not find codec for %s\n", ap->codec_name);
        goto error;
    }

    return true;

error:

    return false;
}

/******************************************************************************
 *                            TERMINATION METHODS
 ******************************************************************************/

static void shutdown_io(ov_rtp_client *client) {
    if (-1 < client->udp_socket) {
        close(client->udp_socket);
        client->udp_socket = -1;
    }

    if (-1 < client->rtcp_socket) {
        close(client->rtcp_socket);
        client->rtcp_socket = -1;
    }
}

/*****************************************************************************
                                 RTCP Reception
 ****************************************************************************/

static void describe_rtcp(FILE *out, ov_rtcp_message const *msg) {
    if ((0 == out) || (0 == msg)) {
        return;
    }

    ov_rtcp_type type = ov_rtcp_message_type(msg);

    fprintf(out, "RTCP received: %s ", ov_rtcp_type_to_string(type));

    switch (type) {
    case OV_RTCP_SOURCE_DESC:

        fprintf(out, "SSRC: %" PRIu32 " CNAME: %s",
                ov_rtcp_message_sdes_ssrc(msg, 0),
                ov_rtcp_message_sdes_cname(msg, 0));
        break;

    default:
        break;
    };

    fprintf(out, "\n");
}

/*----------------------------------------------------------------------------*/

static bool receive_rtcp_handler(int fd, uint8_t events, void *userdata) {
    UNUSED(events);

    ov_buffer *buffer = 0;

    ov_rtp_client *client = userdata;

    OV_ASSERT(0 != client);

    int sz = 0;

    if ((ioctl(fd, FIONREAD, &sz) < 0) || (0 >= sz)) {
        return false;
    }

    buffer = ov_buffer_create(sz);

    OV_ASSERT(0 != buffer);

    ssize_t received = recv(fd, buffer->start, sz, 0);

    if (0 > received) {
        fprintf(stderr, "Failed during reading from socket: %s",
                strerror(errno));
        buffer = ov_buffer_free(buffer);
        return false;
    }

    buffer->length = (size_t)received;

    uint8_t const *data = buffer->start;
    size_t len = buffer->length;

    while (0 != len) {
        ov_rtcp_message *msg = ov_rtcp_message_decode(&data, &len);
        describe_rtcp(stdout, msg);
        msg = ov_rtcp_message_free(msg);
    };

    buffer = ov_buffer_free(buffer);

    return true;
}

/*----------------------------------------------------------------------------*/

static void install_rtcp_handler(ov_rtp_client *client) {
    if (0 != client) {
        client->event->callback.set(client->event, client->rtcp_socket,
                                    OV_EVENT_IO_IN, client,
                                    receive_rtcp_handler);
    }
}

/*----------------------------------------------------------------------------*/

static bool setup_transmission(ov_rtp_client *client,
                               ov_rtp_client_parameters *client_params,
                               ov_rtp_client_audio_parameters *audio_params) {
    if (0 == client) {
        return false;

    } else if (SEND == client->mode) {
        install_rtcp_handler(client);
        return setup_sending_client(client, client_params, audio_params);

    } else {
        install_rtcp_handler(client);
        return setup_receiving_client(client, client_params, audio_params);
    }
}

/******************************************************************************
 *                              PUBLIC FUNCTIONS
 ******************************************************************************/

static bool adapt_config_on_multicast(ov_rtp_client_parameters *client_params) {
    if (0 != client_params) {
        if (0 != client_params->multicast_group) {
            client_params->local_if = client_params->multicast_group;
        }
        return true;
    } else {
        return false;
    }
}

/*----------------------------------------------------------------------------*/

ov_rtp_client *
ov_rtp_client_create(ov_rtp_client_parameters *client_params,
                     ov_rtp_client_audio_parameters *audio_params) {
    ov_rtp_client *client = calloc(1, sizeof(ov_rtp_client));
    client->mode = client_params->mode;
    client->debug = client_params->debug;

    client->audio = *audio_params;

    if ((!setup_event_loop(client)) ||
        (!set_codec(client, client_params, audio_params)) ||
        (!adapt_config_on_multicast(client_params)) ||
        (!connect_udp(client, client_params)) ||
        (!setup_transmission(client, client_params, audio_params))) {
        return ov_rtp_client_free(client);

    } else {
        return client;
    }
}

/*----------------------------------------------------------------------------*/

void ov_rtp_client_run(ov_rtp_client *client) {
    if (0 == client)
        goto error;

    client->event->run(client->event, UINT64_MAX);

error:
    return;
}
/*----------------------------------------------------------------------------*/

ov_rtp_client *ov_rtp_client_free(ov_rtp_client *client) {
    if (0 == client)
        goto error;

    if (0 != client->codec) {
        client->codec = ov_codec_free(client->codec);
    }

    if (0 != client->event) {
        client->event = client->event->free(client->event);
    }

    switch (client->mode) {
    case SEND:

        shutdown_sending_client(client);
        break;

    case RECEIVE:

        shutdown_receiving_client(client);
        break;

    default:

        OV_ASSERT(!"MUST NEVER EVER HAPPEN");
    };

    shutdown_io(client);

    free(client);
    client = 0;

    return client;

error:

    return client;
}

/*----------------------------------------------------------------------------*/

char const *ov_operation_mode_to_string(operation_mode mode) {
    switch (mode) {
    case SEND:
        return "SEND";
    case RECEIVE:
        return "RECEIVE";
    default:
        OV_ASSERT(!"MUST NEVER HAPPEN");
    }

    abort();
}

/*----------------------------------------------------------------------------*/

void ov_rtp_client_parameters_print(
    FILE *out, ov_rtp_client_parameters const *client_params,
    size_t indentation_level) {
    OV_ASSERT(0 != out);
    OV_ASSERT(INT_MAX >= indentation_level);

    int indent = (int)indentation_level;

    if (0 == client_params) {
        fprintf(out, "(NULL POINTER)");
        return;
    }

    fprintf(out, "%*.sLocal socket: %s:%" PRIu16, indent, " ",
            client_params->local_if, client_params->local_port);
    fprintf(out, "    Remote socket: %s:%" PRIu16 "\n",
            client_params->remote_if, client_params->remote_port);

    fprintf(out, "%*.sMode: %7s\n", indent, " ",
            ov_operation_mode_to_string(client_params->mode));

    fprintf(out,
            "%*.sSSID: %" PRIu32 " Payload type: %" PRIu8 "    SEQ: "
            "%" PRIu16 "\n",
            indent, " ", client_params->ssrc_id, client_params->payload_type,
            client_params->sequence_number);

    fprintf(out, "\n");

    if (0 != client_params->sdes) {
        fprintf(out, "Sending RTCP SDES messages, CNAME %s\n",
                client_params->sdes);
    }
}

/*----------------------------------------------------------------------------*/

void ov_rtp_client_audio_parameters_print(
    FILE *out, ov_rtp_client_audio_parameters const *params,
    size_t indentation_level) {
    OV_ASSERT(0 != out);
    OV_ASSERT(INT_MAX >= indentation_level);

    int indent = (int)indentation_level;

    if (0 == params) {
        fprintf(out, "(NULL POINTER)\n");
        return;
    }

    char const *codec_name = params->codec_name;

    if (0 == codec_name) {
        codec_name = "NOT SET";
    }

    fprintf(out, "%*.sCodec: %s     Codec parameters: %s\n", indent, " ",
            codec_name, "");
    ov_pcm_gen_config_print(stdout, &params->general_config, indentation_level);
}

/*----------------------------------------------------------------------------*/

static void print_client_send(FILE *out, struct send_t const *send,
                              size_t indentation_level) {
    OV_ASSERT(0 != out);
    OV_ASSERT(0 != send);

    OV_ASSERT(INT_MAX >= indentation_level);
    int indent = (int)indentation_level;

    fprintf(
        out,
        "%*.sSSID: %" PRIu32 " Payload type: %" PRIu8 "   SEQ: %" PRIu16 "\n",
        indent, " ", send->ssrc_id, send->payload_type, send->sequence_number);
}

/*----------------------------------------------------------------------------*/

static void print_client_receive(FILE *out, struct receive_t const *receive,
                                 size_t indentation_level) {
    UNUSED(out);
    UNUSED(receive);
    UNUSED(indentation_level);

    TODO("IMPLEMENT IF REQUIRED");
}

/*----------------------------------------------------------------------------*/

void ov_rtp_client_print(FILE *out, ov_rtp_client const *client,
                         size_t indentation_level) {
    OV_ASSERT(0 != out);

    if (0 == client) {
        fprintf(out, "(NULL POINTER)\n");
        return;
    }

    OV_ASSERT(INT_MAX >= indentation_level);
    int indent = (int)indentation_level;

    fprintf(out, "%*.sMode: %7s\n", indent, " ",
            ov_operation_mode_to_string(client->mode));

    if (0 != client->codec) {
        ov_json_value *params = ov_codec_get_parameters(client->codec);
        char *params_string = "NONE";

        if (0 != params) {
            params_string = ov_json_value_to_string(params);
        }

        fprintf(out, "%*.sCodec: %s    Codec parameters: %s\n", indent, "",
                ov_codec_type_id(client->codec), params_string);

        if (0 != params) {
            params = params->free(params);
            free(params_string);
        }
    }

    if (SEND == client->mode) {
        print_client_send(out, &client->send, 4);
    } else {
        print_client_receive(out, &client->receive, 4);
    }
}

/*----------------------------------------------------------------------------*/
