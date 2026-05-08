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

#include <pthread.h>
#include <time.h>

#include <ov_test/ov_test.h>

#include <ov_base/ov_event_loop.h>

#include "ov_rtp_buffering_io.c"

/*----------------------------------------------------------------------------*/

static bool initialize_rtp_io_resources(ov_rtp_buffering_io_config *cfg) {

    ov_event_loop *loop = 0;

    if (0 == cfg) {

        goto error;
    }

    loop = ov_event_loop_default(ov_event_loop_config_default());

    if (0 == loop) {

        goto error;
    }

    *cfg = (ov_rtp_buffering_io_config){

        .loop = loop,

    };

    return true;

error:

    loop = ov_event_loop_free(loop);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool release_rtp_io_resources(ov_rtp_buffering_io_config *cfg) {

    if (0 == cfg)
        return true;

    cfg->loop = ov_event_loop_free(cfg->loop);

    return (0 == cfg->loop);
}

/*----------------------------------------------------------------------------*/

int test_ov_rtp_buffering_io_create() {

    /* Prepare resources we will require */

    ov_rtp_buffering_io_config cfg = {0};

    testrun(initialize_rtp_io_resources(&cfg));

    ov_event_loop *loop = cfg.loop;

    /* Failure cases */

    cfg = (ov_rtp_buffering_io_config){0};

    testrun(0 == ov_rtp_buffering_io_create(cfg));

    testrun(0 == ov_rtp_buffering_io_create(cfg));

    testrun(0 == ov_rtp_buffering_io_create(cfg));

    testrun(0 == ov_rtp_buffering_io_create(cfg));

    cfg.loop = loop;
    testrun(0 != cfg.loop);

    /* Now these cases should succeed */

    ov_rtp_buffering_io *io = ov_rtp_buffering_io_create(cfg);
    testrun(0 != io);

    io = ov_rtp_buffering_io_free(io);
    testrun(0 == io);

    io = ov_rtp_buffering_io_create(cfg);
    testrun(0 != io);

    io = ov_rtp_buffering_io_free(io);
    testrun(0 == io);

    io = ov_rtp_buffering_io_create(cfg);
    testrun(0 != io);

    io = ov_rtp_buffering_io_free(io);
    testrun(0 == io);

    io = ov_rtp_buffering_io_create(cfg);
    testrun(0 != io);

    io = ov_rtp_buffering_io_free(io);
    testrun(0 == io);

    /* clean up */

    cfg = (ov_rtp_buffering_io_config){
        .loop = loop,
    };

    loop = 0;

    testrun(release_rtp_io_resources(&cfg));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_rtp_buffering_io_free() {

    testrun(0 == ov_rtp_buffering_io_free(0));

    /* Prepare resources we will require */

    ov_rtp_buffering_io_config cfg = {0};

    testrun(initialize_rtp_io_resources(&cfg));

    ov_rtp_buffering_io *io = ov_rtp_buffering_io_create(cfg);

    testrun(0 != io);

    io = ov_rtp_buffering_io_free(io);
    testrun(0 == io);

    /* clean up  - also ensures that the resources we passed to
     * rtp_io_create have not been released / freed */

    testrun(release_rtp_io_resources(&cfg));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_rtp_buffering_io_register_socket() {

    int fd = -1;

    ov_rtp_buffering_io_config cfg = {0};

    testrun(initialize_rtp_io_resources(&cfg));

    ov_rtp_buffering_io *io = ov_rtp_buffering_io_create(cfg);
    testrun(0 != io);

    ov_socket_configuration scfg = {

        .host = "127.0.0.1",
        .type = UDP,

    };

    ov_socket_error serr;
    fd = ov_socket_create(scfg, false, &serr);
    testrun(0 < fd);

    testrun(!ov_rtp_buffering_io_register_socket(0, -1));
    testrun(!ov_rtp_buffering_io_register_socket(io, -1));
    testrun(!ov_rtp_buffering_io_register_socket(0, fd));
    testrun(ov_rtp_buffering_io_register_socket(io, fd));

    io = ov_rtp_buffering_io_free(io);
    testrun(0 == io);

    testrun(release_rtp_io_resources(&cfg));

    close(fd);
    fd = -1;

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_rtp_buffering_io_unregister_socket() {

    int fd = -1;

    ov_rtp_buffering_io_config cfg = {0};

    testrun(initialize_rtp_io_resources(&cfg));

    ov_rtp_buffering_io *io = ov_rtp_buffering_io_create(cfg);
    testrun(0 != io);

    ov_socket_configuration scfg = {

        .host = "127.0.0.1",
        .type = UDP,

    };

    ov_socket_error serr;
    fd = ov_socket_create(scfg, false, &serr);
    testrun(0 < fd);

    /* Try without registered fd */
    testrun(!ov_rtp_buffering_io_unregister_socket(0));
    testrun(ov_rtp_buffering_io_unregister_socket(io));

    /* Test again with regsistered fd */

    testrun(ov_rtp_buffering_io_register_socket(io, fd));

    testrun(!ov_rtp_buffering_io_unregister_socket(0));
    testrun(ov_rtp_buffering_io_unregister_socket(io));

    /* release resources */

    io = ov_rtp_buffering_io_free(io);
    testrun(0 == io);

    testrun(release_rtp_io_resources(&cfg));

    close(fd);
    fd = -1;

    return testrun_log_success();
}

/******************************************************************************
 *                      ov_rtp_buffering_io_next_frame_list
 ******************************************************************************/

void send_frame(int fd, uint16_t ssrc, uint16_t seq) {

    uint32_t payload = seq;
    payload += ssrc;

    ov_rtp_frame_expansion fexp = {

        .version = RTP_VERSION_2,
        .sequence_number = seq,
        .timestamp = time(0),
        .ssrc = ssrc,

        .payload.data = (uint8_t *)&payload,
        .payload.length = sizeof(payload),

    };

    ov_rtp_frame *frame = ov_rtp_frame_encode(&fexp);

    OV_ASSERT(0 != frame);

    ssize_t octets_sent = write(fd, frame->bytes.data, frame->bytes.length);

    OV_ASSERT(octets_sent == (ssize_t)frame->bytes.length);

    frame = frame->free(frame);

    OV_ASSERT(0 == frame);
}

/*----------------------------------------------------------------------------*/

/**
 * Takes a list of (SSRC_ID, SEQUENCE_NUMBER)
 * Generates an RTP frame for each tuple and sends it over fd
 */
#define SEND_FRAMES(fd, ...)                                                   \
    send_frames(fd, (uint16_t[][2]){__VA_ARGS__, {0, 0}})

bool send_frames(int fd, uint16_t ssrc_seq_array[][2]) {

    OV_ASSERT(0 < fd);

    uint16_t *current = ssrc_seq_array[0];

    while (!((current[0] == 0) && (current[1] == 0))) {

        send_frame(fd, current[0], current[1]);

        fprintf(stderr, "Sent %" PRIu16 "\n", current[0]);

        current += 2;
    }

    return true;
}

/*----------------------------------------------------------------------------*/

void *run_loop(void *vloop) {

    ov_event_loop *loop = vloop;
    OV_ASSERT(0 != loop);

    loop->run(loop, UINT64_MAX);

    fprintf(stderr, "loop run done\n");

    return 0;
}

/*----------------------------------------------------------------------------*/

void run_loop_in_thread(ov_event_loop *loop, pthread_t *tid) {

    pthread_create(tid, 0, run_loop, loop);

    fprintf(stderr, "Thread created\n");
}

/*----------------------------------------------------------------------------*/

void stop_loop_thread(ov_event_loop *loop, pthread_t tid) {

    loop->stop(loop);

    pthread_join(tid, 0);
}

/*----------------------------------------------------------------------------*/

ssize_t find_index_for_ssid(uint32_t ssid, uint16_t ssrc_seq_array[][2]) {

    size_t i = 0;
    while ((0 != ssrc_seq_array[i][0]) && (0 != ssrc_seq_array[i][1])) {

        if (ssid == ssrc_seq_array[i][0])
            return i;

        ++i;
    }

    return -1;
}

/*----------------------------------------------------------------------------*/

#define CHECK_FRAMES_RECEIVED(fd, ...)                                         \
    check_frames_received(fd, (uint16_t[][2]){__VA_ARGS__, {0, 0}})

bool check_frames_received(ov_rtp_buffering_io *io,
                           uint16_t ssrc_seq_array[][2]) {

    ov_rtp_frame *frame = 0;

    ov_list *frames = ov_rtp_buffering_io_next_frame_list(io);

    if (0 == frames) {

        fprintf(stderr, "No list received\n");
        return false;
    }

    uint16_t *current = ssrc_seq_array[0];

    size_t no_frames_expected = 0;

    while (!((current[0] == 0) && (current[1] == 0))) {

        ++no_frames_expected;
        current += 2;
    }

    bool ssid_seen[no_frames_expected];

    for (size_t i = 0; i < no_frames_expected; ++i) {

        ssid_seen[i] = false;
    }

    frame = frames->pop(frames);

    while (0 != frame) {

        ssize_t i = find_index_for_ssid(frame->expanded.ssrc, ssrc_seq_array);

        if (0 > i) {

            fprintf(stderr, "%" PRIu32 " not expected\n", frame->expanded.ssrc);

            goto failed;
        }

        if (ssrc_seq_array[i][1] != frame->expanded.sequence_number) {

            fprintf(stderr,
                    "%" PRIu32 ": seq do not match (%" PRIu16 " vs %" PRIu16
                    ")\n",
                    frame->expanded.ssrc, frame->expanded.sequence_number,
                    ssrc_seq_array[i][1]);

            goto failed;
        }

        fprintf(stderr, "clonk: %" PRIu16 " found\n", ssrc_seq_array[i][0]);

        ssid_seen[i] = true;

        frame = frame->free(frame);

        frame = frames->pop(frames);
    }

    frames = ov_list_free(frames);

    OV_ASSERT(0 == frames);

    for (size_t i = 0; i < no_frames_expected; ++i) {

        if (!ssid_seen[i]) {

            fprintf(stderr, "%" PRIu16 " not found\n", ssrc_seq_array[i][0]);
            return false;
        }
    }

    return true;

failed:

    if (0 != frame) {

        frame = frame->free(frame);
    }

    return false;
}

/*----------------------------------------------------------------------------*/

int test_ov_rtp_buffering_io_next_frame_list() {

    /* acquire resources */

    ov_rtp_buffering_io_config cfg = {0};

    testrun(initialize_rtp_io_resources(&cfg));

    cfg.num_frames_to_buffer_per_stream = 3;

    ov_rtp_buffering_io *io = ov_rtp_buffering_io_create(cfg);
    testrun(0 != io);

    /* the fds to send the rtp over */

    int fd_pair[2] = {0};

    testrun(0 == socketpair(AF_UNIX, SOCK_DGRAM, 0, fd_pair));

    int send_fd = fd_pair[0];
    int recv_fd = fd_pair[1];

    /* Failure causes */

    testrun(0 == ov_rtp_buffering_io_next_frame_list(0));

    /* 1st: Even without any received data, even without any
     * socket
     * registered, we should get a list, albeit empty ...*/

    ov_list *recv = ov_rtp_buffering_io_next_frame_list(io);

    testrun(0 != recv);
    testrun(0 == recv->count(recv));

    recv = recv->free(recv);
    testrun(0 == recv);

    /* Now try with some frames */

    pthread_t tid;

    testrun(ov_rtp_buffering_io_register_socket(io, recv_fd));

    run_loop_in_thread(cfg.loop, &tid);

    testrun(SEND_FRAMES(send_fd, {1, 1}));

    /* Try to make the scheduler schedule our loop thread -
     * if this does not succeed, try sleep(2) ? */
    sched_yield();
    sleep(2);

    stop_loop_thread(cfg.loop, tid);

    testrun(CHECK_FRAMES_RECEIVED(io, {1, 1}));

    /* Test with several frames */

    run_loop_in_thread(cfg.loop, &tid);

    testrun(SEND_FRAMES(send_fd, {1, 1}, {2, 10}, {3, 2}));

    sched_yield();
    sleep(2);

    stop_loop_thread(cfg.loop, tid);

    testrun(CHECK_FRAMES_RECEIVED(io, {1, 1}, {2, 10}, {3, 2}));

    /* Check proper buffering (oldest first) */

    run_loop_in_thread(cfg.loop, &tid);

    testrun(SEND_FRAMES(send_fd, {1, 1}, {2, 10}, {3, 2}, {1, 2}));

    sched_yield();
    sleep(2);

    stop_loop_thread(cfg.loop, tid);

    testrun(CHECK_FRAMES_RECEIVED(io, {1, 1}, {2, 10}, {3, 2}));

    /* Check proper buffering - drop oldest if buffer overflows */
    run_loop_in_thread(cfg.loop, &tid);

    testrun(
        SEND_FRAMES(send_fd, {1, 1}, {2, 10}, {3, 2}, {1, 2}, {1, 3}, {1, 4}));

    sched_yield();
    sleep(2);

    stop_loop_thread(cfg.loop, tid);

    testrun(CHECK_FRAMES_RECEIVED(io, {1, 2}, {2, 10}, {3, 2}));

    /* release resources */

    io = ov_rtp_buffering_io_free(io);
    testrun(0 == io);

    testrun(release_rtp_io_resources(&cfg));

    close(send_fd);
    send_fd = -1;

    close(recv_fd);
    recv_fd = -1;

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

OV_TEST_RUN("ov_rtp_buffering_io", test_ov_rtp_buffering_io_create,
            test_ov_rtp_buffering_io_free,
            test_ov_rtp_buffering_io_register_socket,
            test_ov_rtp_buffering_io_unregister_socket,
            test_ov_rtp_buffering_io_next_frame_list);
