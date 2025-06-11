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

        ------------------------------------------------------------------------
*/
#include "ov_rtp_frame_message.c"
#include <ov_test/ov_test.h>

const char TEST_IP[] = "192.0.2.123";

char const *CODEC_TEST_KEY = "testme";

/*----------------------------------------------------------------------------*/

int test_ov_rtp_frame_message_enable_caching() {

    ov_rtp_frame_message_enable_caching(0);

    ov_rtp_frame_message_enable_caching(2);

    ov_thread_message *msg = ov_rtp_frame_message_create(0);
    testrun(0 != msg);

    testrun(0 != msg->free);

    testrun(0 == msg->free(msg));

    ov_thread_message *msg_2 = ov_rtp_frame_message_create(0);
    testrun(0 != msg_2);
    testrun(msg == msg_2);

    msg = 0;

    msg_2 = msg_2->free(msg_2);
    testrun(0 == msg_2);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_rtp_frame_message_create() {

    ov_thread_message *msg = ov_rtp_frame_message_create(0);
    testrun(0 != msg);

    testrun(0 != ov_thread_message_cast(msg));
    testrun(OV_RTP_FRAME_MESSAGE_TYPE == msg->type);

    testrun(0 != msg->free);

    ov_rtp_frame_message *rf_msg = (ov_rtp_frame_message *)msg;

    testrun(0 == rf_msg->frame);

    rf_msg = 0;

    msg = msg->free(msg);
    testrun(0 == msg);

    ov_rtp_frame_expansion data = {};

    data.version = RTP_VERSION_2;

    ov_rtp_frame *frame = ov_rtp_frame_encode(&data);
    testrun(0 != frame);

    msg = ov_rtp_frame_message_create(frame);
    testrun(0 != msg);

    testrun(0 != ov_thread_message_cast(msg));
    testrun(OV_RTP_FRAME_MESSAGE_TYPE == msg->type);

    testrun(0 != msg->free);

    rf_msg = (ov_rtp_frame_message *)msg;

    testrun(frame == rf_msg->frame);

    frame = 0;
    rf_msg = 0;

    msg = msg->free(msg);
    testrun(0 == msg);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int free_caches() {

    ov_registered_cache_free_all();

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

OV_TEST_RUN("ov_rtp_frame_message",
            test_ov_rtp_frame_message_enable_caching,
            test_ov_rtp_frame_message_create,
            free_caches);

/*----------------------------------------------------------------------------*/
