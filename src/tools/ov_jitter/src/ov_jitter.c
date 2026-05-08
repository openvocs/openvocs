/***
        ------------------------------------------------------------------------

        Copyright (c) 2022 German Aerospace Center DLR e.V. (GSOC)

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

        Emulate a shaky UDP connection.


        # Sample setups

        ## 2 ov_rtp_cli instances with emulated shaky connection:

        Record & send:
           build/bin/ov_rtp_cli -p 20002 -H 224.0.0.1 -m -

        Receive and replay:
           build/bin/ov_rtp_cli -d - -M  224.0.0.2 -q 20003


        Linking both:
build/bin/ov_jitter -m -r 0.4,10 -d 0.5,3000 224.0.0.1 20002 224.0.0.2 20003


        ## 1 test_mc instance

                  ----------->
        Generator              Jitter Gen
                  <-----------

        Generator
        build/bin/ov_test_mc 224.0.0.10 224.0.0.4 20003

        Jitter Gen
        build/bin/ov_jitter -m r 0.4,10 -d 0.5,3000 224.0.0.4 20003 224.0.0.10 20003


        ## 2 test_mc instances

        Generator -------> Jitter Gen
           ^                  /
            \                /
             \              /
              \            /
                 Loopback

        Generator:
        build/bin/ov_test_mc 224.0.0.10 224.0.0.4 20003

        This test_mc sends packets on regular intervals, and receives them
        back.
        Connectivity / packet ordering can be checkt visually.

        Loopback:
        build/bin/ov_test_mc 224.0.0.3 224.0.0.10 20003 --Loopback

        Linking both:
        build/bin/ov_jitter -m -r 0.4,10 -d 0.5,3000 224.0.0.4 20003 224.0.0.3 20003



        @author         Michael J. Beer, DLR/GSOC

        ------------------------------------------------------------------------
*/

#include "ov_arch/ov_arch_compile.h"
#include <ov_arch/ov_arch_math.h>
#include <ov_base/ov_event_loop.h>
#include <ov_base/ov_mc_socket.h>
#include <ov_base/ov_random.h>
#include <ov_base/ov_ringbuffer.h>
#include <ov_base/ov_socket.h>
#include <ov_base/ov_string.h>
#include <ov_os/ov_os_event_loop.h>
#include <unistd.h>

/*----------------------------------------------------------------------------*/

#define MAX_PACKETS_TO_BUFFER 1000

/*----------------------------------------------------------------------------*/

static struct jitter_app_t {

    ov_ringbuffer *in_packets_buffer;
    uint32_t timer_id;

    struct {
        int sd;
        struct sockaddr_storage dest;
        socklen_t dest_len;
    } send;

    ov_event_loop *loop;

} jitter_app = {

    .in_packets_buffer = 0,
    .send.sd = -1,
    .send.dest_len = 0,
    .timer_id = OV_TIMER_INVALID,

};

/*----------------------------------------------------------------------------*/

static struct configuration_t {

    struct {
        float probability;
        size_t span;
    } reorder;

    struct {
        float probability;
        size_t span;
    } delay;

    struct {
        float probability;
    } drop_single_packet;

    ov_socket_configuration recv;
    ov_socket_configuration send;

    struct {
        bool multicast : 1;
    };

} configuration = {0};

/*----------------------------------------------------------------------------*/

static void free_ov_buffer(void *userdata, void *element_to_free) {

    UNUSED(userdata);

    ov_buffer *buffer = ov_buffer_cast(element_to_free);

    if (ov_ptr_valid(buffer, "Cannot free element: Not a valid ov_buffer")) {
        ov_buffer_free(buffer);
    }
}

/*----------------------------------------------------------------------------*/

_Noreturn static void usage(char const *cmd) {

    fprintf(stderr,
            "\n"
            "    USAGE:\n"
            "        %s [OPTION [OPTION ...]] LISTENIF LISTENPORT TARGETIP "
            "TARGETPORT\n"
            "    Listens on LISTENIF:LISTENPORT and relays any incoming "
            "traffic\n"
            "    to TARGETIP:TARGETPORT\n"
            "    Options\n\n"
            "      -r PROB,SPAN, --reorder=PROB,SPAN\n"
            "          Reorder packets. SPAN indicates how deep reodering\n"
            "might "
            "          get: 4 indicates that packet N is sent as packet N+4 at "
            "most.\n"
            "          PROB indicates the probability that reordering occurs, "
            "as decimal fraction, e.g:\n"
            "          -r 0.3,3 indicates a probability of 0.3 ( = 30%%) with "
            "packet N being sent as N+3 at most\n"
            "          Only effective if delay is set, too\n"
            "      -d PROB,SPAN, --delay=PROB,SPAN\n"
            "         Delay packets. PROB is the probability that a single "
            "packet is delayed,"
            " SPAN indicates the maximum delay of that packet (in msecs)\n"
            "         See `reorder` option for further details on PROB and "
            "SPAN\n"
            "         Delaying one packet also delays the following packets.\n"
            "         Those are sent in a burst then\n"

            "      -D PROB,   --drop=PROB\n"
            "         Drop packets. PROB is the probability to drop one single"
            " packet, e.g.:\n"
            "         -D 0.01  Will drop every 100th packet on average\n"
            "      -t use TCP instead of UDP\n",
            cmd);

    fprintf(stderr, "\n"
                    "\n"
                    "\n");

    exit(EXIT_FAILURE);
}

/*----------------------------------------------------------------------------*/

static char *strchr_safe(char *s, int c) {

    if (ov_ptr_valid(s, "Cannot parse probability & span: No string")) {
        return strchr(s, c);
    } else {
        return 0;
    }
}

/*----------------------------------------------------------------------------*/

static bool split_prob_span_destructively(char *str, float *prob,
                                          size_t *span) {

    char *endofprob = strchr_safe(str, ',');

    if (ov_ptr_valid(str, "Cannot parse probability & span: No string") &&
        ov_ptr_valid(prob,
                     "Cannot parse probability & span: No probability variable "
                     "to store result to") &&
        ov_ptr_valid(span,
                     "Cannot parse probability & span: No span variable to "
                     "store result to") &&
        ov_ptr_valid(endofprob,
                     "Cannot parse probability & span: No '.' found")) {

        *endofprob = 0;

        char *prob_endptr = 0;
        *prob = strtof(str, &prob_endptr);

        char *span_endptr = 0;
        *span = (size_t)strtol(++endofprob, &span_endptr, 0);

        printf("Parsed %s to probability: %f  span: %zu\n", str, *prob, *span);

        return true;

    } else {

        return false;
    }
}

/*----------------------------------------------------------------------------*/

static bool split_prob_span(char const *str, float *prob, size_t *span) {

    char *str_copy = ov_string_dup(str);

    bool retval = split_prob_span_destructively(str_copy, prob, span);

    ov_free(str_copy);

    return retval;
}

/*----------------------------------------------------------------------------*/

static uint16_t port_from_string(char const *str) {

    char *endptr = 0;
    long port = strtol(str, &endptr, 10);

    errno = 0;

    if ((port > UINT16_MAX) || (port < 0) || (errno == ERANGE) ||
        (endptr == 0) || (*endptr != '\0')) {

        fprintf(stderr, "%s is not a valid port number\n", str);
        return 0;

    } else {

        return (uint16_t)port;
    }
}

/*----------------------------------------------------------------------------*/

struct configuration_t parse_arguments(int argc, char const **argv) {

    char *endptr = 0;
    int c = 0;
    struct configuration_t args = {0};

    args.recv.type = UDP;
    args.send.type = UDP;

    while ((c = getopt(argc, (char **)argv, "tmr:d:D:")) != -1) {

        switch (c) {

        case '?':
            usage(argv[0]);

        case 't':

            args.recv.type = TCP;
            args.send.type = TCP;
            break;

        case 'm':
            args.multicast = true;
            break;

        case 'r':

            if (!split_prob_span(optarg, &args.reorder.probability,
                                 &args.reorder.span)) {
                fprintf(stderr, "Invalid arguments to 'reorder' option\n");
                usage(argv[0]);
            }

            break;

        case 'd':

            if (!split_prob_span(optarg, &args.delay.probability,
                                 &args.delay.span)) {
                fprintf(stderr, "Invalid arguments to 'reorder' option\n");
                usage(argv[0]);
            }

            break;

        case 'D':

            args.drop_single_packet.probability = strtof(optarg, &endptr);

            break;
        };
    };

    if (optind + 4 > argc) {

        fprintf(stderr,
                "Missing LISTENIF, LISTENPORT, TARGETIP or TARGETOPRT\n");
        usage(argv[0]);
    }

    strncpy(args.recv.host, argv[optind++], sizeof(args.recv.host));
    args.recv.port = port_from_string(argv[optind++]);

    strncpy(args.send.host, argv[optind++], sizeof(args.send.host));
    args.send.port = port_from_string(argv[optind++]);

    return args;
}

/*----------------------------------------------------------------------------*/

static int open_listen_socket(ov_socket_configuration scfg, bool multicast) {

    if (UDP == scfg.type) {
        printf("RECV: Using UDP ");
    } else {
        printf(
            "RECV: Using TCP - Not supported yet - need to listen and accept "
            "- ");
        exit(1);
    }

    printf("%s:%" PRIu16, ov_string_sanitize(scfg.host), scfg.port);

    if (multicast) {
        printf(" multicast\n");
        return ov_mc_socket(scfg);
    } else {
        printf(" unicast\n");
        return ov_socket_create(scfg, false, 0);
    }
}

/*----------------------------------------------------------------------------*/

static ov_event_loop *get_loop() {

    ov_event_loop *loop = ov_os_event_loop(ov_event_loop_config_default());

    if (!ov_event_loop_setup_signals(loop)) {

        ov_log_error("Could not setup signal handling");
        loop = ov_event_loop_free(loop);
    }

    return loop;
}

/*----------------------------------------------------------------------------*/

static void senddata(struct jitter_app_t app, uint8_t const *data,
                     size_t num_octets) {

    ssize_t bytes_sent = 0;

    printf("Sending %zu octets\n", num_octets);

    if (TCP == app.send.sd) {

        bytes_sent = send(app.send.sd, data, num_octets, 0);

    } else {

        bytes_sent =
            sendto(app.send.sd, data, num_octets, 0,
                   (struct sockaddr *)&app.send.dest, app.send.dest_len);
    }

    if ((bytes_sent < 0) || ((size_t)bytes_sent != num_octets)) {

        fprintf(stderr, "Could not send: %s\n", strerror(errno));
    }
}

/*----------------------------------------------------------------------------*/

static void insert_past(ov_buffer **array, size_t array_capacity,
                        size_t lowest_index, ov_buffer *to_insert) {

    for (size_t i = lowest_index; i < array_capacity; ++i) {

        if (0 == array[i]) {
            printf("Inserting at position %zu\n", i);
            array[i] = to_insert;
            to_insert = 0;
        }
    }

    if (0 != to_insert) {

        fprintf(stderr, "Failed to insert packet\n");
        to_insert = ov_buffer_free(to_insert);
    }
}

/*----------------------------------------------------------------------------*/

static bool trigger_sending() {

    ov_buffer *reordered[MAX_PACKETS_TO_BUFFER] = {0};
    size_t reordered_capacity = sizeof(reordered) / sizeof(reordered[0]);

    printf("Sending ...");

    ov_buffer *buffer = ov_ringbuffer_pop(jitter_app.in_packets_buffer);

    bool retval = true;

    memset(reordered, 0, sizeof(reordered));

    while (0 != buffer) {

        printf(" got buffer %p\n", buffer);

        if (ov_ptr_valid(buffer, "Cannot send data - no data")) {

            uint32_t rel_prob =
                configuration.reorder.probability * (float)UINT32_MAX;
            if (ov_random_uint32() < rel_prob) {

                float reorder_index = ov_random_uint32();
                reorder_index /= (float)UINT32_MAX;
                reorder_index *= reordered_capacity;

                insert_past(reordered, reordered_capacity,
                            (size_t)reorder_index, buffer);

            } else {

                senddata(jitter_app, buffer->start, buffer->length);
                buffer = ov_buffer_free(buffer);
            }
        }

        buffer = ov_ringbuffer_pop(jitter_app.in_packets_buffer);
    };

    for (size_t i = 0; i < reordered_capacity; ++i) {

        ov_buffer *packet = reordered[i];

        if (0 != packet) {
            senddata(jitter_app, packet->start, packet->length);
        }

        packet = ov_buffer_free(packet);
    }

    return retval;
}

/*----------------------------------------------------------------------------*/

static bool cb_delay_timer(uint32_t id, void *data) {

    UNUSED(id);
    UNUSED(data);

    jitter_app.timer_id = OV_TIMER_INVALID;
    trigger_sending();

    return true;
}

/*----------------------------------------------------------------------------*/

static bool delay_sending() {

    if (jitter_app.timer_id != OV_TIMER_INVALID) {

        return true;

    } else if (ov_random_uint32() <
               configuration.delay.probability * (float)UINT32_MAX) {

        uint64_t delay_msecs = configuration.delay.span * ov_random_uint32();
        delay_msecs /= (float)UINT32_MAX;

        jitter_app.timer_id = jitter_app.loop->timer.set(
            jitter_app.loop, delay_msecs * 1000, 0, cb_delay_timer);

        ov_log_debug("Delaying packets for %" PRIu64 " msecs", delay_msecs);

        return true;

    } else {

        return false;
    }
}

/*----------------------------------------------------------------------------*/

static ov_buffer *forward_packet(ov_buffer *buffer) {

    if (ov_random_uint32() >
        configuration.drop_single_packet.probability * (float)UINT32_MAX) {

        printf("Put into ringbuffer...\n");
        if (ov_ringbuffer_insert(jitter_app.in_packets_buffer, buffer)) {
            buffer = 0;
        }

        if (!delay_sending()) {
            trigger_sending();
        }
    } else {
        printf("Dropping packet...\n");
    }

    return ov_buffer_free(buffer);
}

/*----------------------------------------------------------------------------*/

static bool cb_io(int fd, uint8_t events, void *userdata) {

    UNUSED(userdata);
    UNUSED(events);

    printf("RECV... ");

    ov_buffer *buffer = ov_buffer_create(OV_UDP_PAYLOAD_OCTETS);
    ssize_t in = read(fd, buffer->start, buffer->capacity);
    printf("Read %i octets ...", (int)in);

    bool retval = false;

    if (in < 0) {

        fprintf(stderr, " Failed: %s\n", strerror(errno));
        usleep(100000);

        retval = true;

    } else {

        buffer->length = (size_t)in;
        buffer = forward_packet(buffer);
    }

    buffer = ov_buffer_free(buffer);

    return retval;
}

/*----------------------------------------------------------------------------*/

static bool register_socket(int s, ov_event_loop *loop) {

    return ov_event_loop_set(
        loop, s, OV_EVENT_IO_IN | OV_EVENT_IO_ERR | OV_EVENT_IO_CLOSE, 0,
        cb_io);
}

/*----------------------------------------------------------------------------*/

static int open_send_socket(struct configuration_t cfg) {

    ov_socket_configuration send_cfg = cfg.send;

    int fd = -1;

    if (UDP == send_cfg.type) {
        printf("SEND: Using UDP ");
        send_cfg.port = 0;
        fd = ov_socket_create(send_cfg, false, 0);
    } else {
        printf("SEND: Using TCP ");
        fd = ov_socket_create(send_cfg, true, 0);
    }

    printf("%s:%" PRIu16 "\n", ov_string_sanitize(send_cfg.host),
           send_cfg.port);

    return fd;
}

/*----------------------------------------------------------------------------*/

static bool initialize_sockaddr_if_udp(ov_socket_configuration send_cfg) {

    jitter_app.send.dest_len = sizeof(struct sockaddr_in);

    return (UDP != send_cfg.type) ||
           ov_socket_fill_sockaddr_storage(&jitter_app.send.dest, AF_INET,
                                           send_cfg.host, send_cfg.port);
}

/*----------------------------------------------------------------------------*/

int main(int argc, char const **argv) {

    configuration = parse_arguments(argc, argv);

    jitter_app.in_packets_buffer =
        ov_ringbuffer_create(MAX_PACKETS_TO_BUFFER, free_ov_buffer, 0);

    int listen_socket =
        open_listen_socket(configuration.recv, configuration.multicast);

    jitter_app.send.sd = open_send_socket(configuration);
    initialize_sockaddr_if_udp(configuration.send);

    jitter_app.loop = get_loop();

    int retval = EXIT_FAILURE;

    if (ov_cond_valid(listen_socket > -1,
                      "Could not open socket for receiving") &&
        register_socket(listen_socket, jitter_app.loop) &&
        ov_cond_valid(jitter_app.send.sd > -1,
                      "Could not open socket for sending") &&
        ov_event_loop_run(jitter_app.loop, OV_RUN_MAX)) {

        retval = EXIT_SUCCESS;
    }

    jitter_app.loop = ov_event_loop_free(jitter_app.loop);

    ov_socket_close(listen_socket);
    jitter_app.send.sd = ov_socket_close(jitter_app.send.sd);

    jitter_app.in_packets_buffer =
        ov_ringbuffer_free(jitter_app.in_packets_buffer);

    return retval;
}
