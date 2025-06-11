/***
        ------------------------------------------------------------------------

        Copyright (c) 2024 German Aerospace Center DLR e.V. (GSOC)

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

#include "ov_recorder_events.c"
#include <ov_test/ov_test.h>

/*----------------------------------------------------------------------------*/

int test_ov_recorder_event_start_clear() {

    testrun(!ov_recorder_event_start_clear(0));

    ov_recorder_event_start event = {0};

    testrun(ov_recorder_event_start_clear(&event));

    event.loop = strdup("avocado");
    testrun(ov_recorder_event_start_clear(&event));
    testrun(0 == event.loop);
    testrun(0 == event.mc_ip);

    event.mc_ip = strdup("roggen");
    testrun(ov_recorder_event_start_clear(&event));
    testrun(0 == event.loop);
    testrun(0 == event.mc_ip);

    event.loop = strdup("avocado");
    event.mc_ip = strdup("roggen");
    event.mc_port = 142;
    testrun(ov_recorder_event_start_clear(&event));
    testrun(0 == event.loop);
    testrun(0 == event.mc_ip);
    testrun(0 == event.mc_port);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_recorder_event_start_equal() {

    ov_recorder_event_start event1 = {0};
    ov_recorder_event_start event2 = {0};

    testrun(ov_recorder_event_start_equal(0, 0));
    testrun(!ov_recorder_event_start_equal(&event1, 0));
    testrun(!ov_recorder_event_start_equal(0, &event2));
    testrun(ov_recorder_event_start_equal(&event1, &event2));

    event1.loop = "hagen";
    testrun(!ov_recorder_event_start_equal(&event1, &event2));

    event2.loop = "hagen";
    testrun(ov_recorder_event_start_equal(&event1, &event2));

    event1.loop = "hagen2";
    testrun(!ov_recorder_event_start_equal(&event1, &event2));

    event1.loop = 0;
    testrun(!ov_recorder_event_start_equal(&event1, &event2));

    event2.loop = 0;
    testrun(ov_recorder_event_start_equal(&event1, &event2));

    event1.mc_ip = "1.2.3.4";
    testrun(!ov_recorder_event_start_equal(&event1, &event2));

    event2.mc_ip = "1.2.3.4";
    testrun(ov_recorder_event_start_equal(&event1, &event2));

    event1.mc_ip = "1.2.4.4";
    testrun(!ov_recorder_event_start_equal(&event1, &event2));

    event1.loop = "hagen";
    testrun(!ov_recorder_event_start_equal(&event1, &event2));

    event2.loop = "hagen";
    testrun(!ov_recorder_event_start_equal(&event1, &event2));

    event2.mc_ip = "1.2.4.4";
    testrun(ov_recorder_event_start_equal(&event1, &event2));

    event1.mc_port = 153;
    testrun(!ov_recorder_event_start_equal(&event1, &event2));

    event2.mc_port = 153;
    testrun(ov_recorder_event_start_equal(&event1, &event2));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_recorder_event_start_to_json() {

    testrun(!ov_recorder_event_start_to_json(0, 0));

    ov_recorder_event_start start = {0};
    testrun(!ov_recorder_event_start_to_json(0, &start));

    start.loop = "abrakadabra";
    testrun(!ov_recorder_event_start_to_json(0, &start));

    start.loop = 0;
    start.mc_ip = "1.2.3.4";
    testrun(!ov_recorder_event_start_to_json(0, &start));

    start.loop = "abrakadabra";
    start.mc_ip = "1.2.3.4";
    testrun(!ov_recorder_event_start_to_json(0, &start));

    ov_json_value *val = ov_json_object();
    start.loop = 0;
    start.mc_ip = 0;
    testrun(!ov_recorder_event_start_to_json(val, &start));

    start.loop = "abrakadabra";
    testrun(!ov_recorder_event_start_to_json(val, &start));

    start.loop = 0;
    start.mc_ip = "1.2.3.4";
    testrun(!ov_recorder_event_start_to_json(val, &start));

    start.loop = "abrakadabra";
    start.mc_ip = "1.2.3.4";
    testrun(ov_recorder_event_start_to_json(val, &start));

    val = ov_json_value_free(val);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_recorder_event_start_from_json() {

    bool ov_recorder_event_start_from_json(
        ov_json_value const *json, ov_recorder_event_start *event);

    testrun(!ov_recorder_event_start_from_json(0, 0));

    ov_recorder_event_start start_reference = {
        .loop = "bukephalos", .mc_ip = "2.2.1.3", .mc_port = 433};

    ov_json_value *jval = ov_json_object();

    testrun(ov_recorder_event_start_to_json(jval, &start_reference));

    testrun(!ov_recorder_event_start_from_json(0, 0));
    testrun(!ov_recorder_event_start_from_json(jval, 0));

    ov_recorder_event_start start_read = {0};
    testrun(ov_recorder_event_start_from_json(jval, &start_read));

    testrun(ov_recorder_event_start_equal(&start_reference, &start_read));

    jval = ov_json_value_free(jval);
    ov_recorder_event_start_clear(&start_read);

    start_reference = (ov_recorder_event_start){
        .loop = "rudi", .mc_ip = "9.0.1.3", .roll_after_secs = 1311};

    jval = ov_json_object();

    testrun(ov_recorder_event_start_to_json(jval, &start_reference));
    testrun(ov_recorder_event_start_from_json(jval, &start_read));
    testrun(ov_recorder_event_start_equal(&start_reference, &start_read));

    jval = ov_json_value_free(jval);
    ov_recorder_event_start_clear(&start_read);

    return testrun_log_success();
}

/*****************************************************************************
                                 start response
 ****************************************************************************/

int test_ov_recorder_response_start_clear() {

    testrun(!ov_recorder_response_start_clear(0));

    ov_recorder_response_start response = {0};

    testrun(ov_recorder_response_start_clear(&response));

    response.filename = strdup("avocado");
    testrun(ov_recorder_response_start_clear(&response));
    testrun(0 == response.filename);
    testrun(!ov_id_valid(response.id));

    ov_id_fill_with_uuid(response.id);
    testrun(ov_recorder_response_start_clear(&response));
    testrun(0 == response.filename);
    testrun(!ov_id_valid(response.id));

    ov_id_fill_with_uuid(response.id);
    response.filename = strdup("avocado");
    testrun(ov_recorder_response_start_clear(&response));
    testrun(0 == response.filename);
    testrun(!ov_id_valid(response.id));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_recorder_response_start_equal() {

    ov_recorder_response_start response1 = {0};
    ov_recorder_response_start response2 = {0};

    testrun(ov_recorder_response_start_equal(0, 0));
    testrun(!ov_recorder_response_start_equal(&response1, 0));
    testrun(!ov_recorder_response_start_equal(0, &response2));
    testrun(ov_recorder_response_start_equal(&response1, &response2));

    response1.filename = "hagen";
    testrun(!ov_recorder_response_start_equal(&response1, &response2));

    response2.filename = "hagen";
    testrun(ov_recorder_response_start_equal(&response1, &response2));

    response1.filename = "hagen2";
    testrun(!ov_recorder_response_start_equal(&response1, &response2));

    response1.filename = 0;
    testrun(!ov_recorder_response_start_equal(&response1, &response2));

    response2.filename = 0;
    testrun(ov_recorder_response_start_equal(&response1, &response2));

    ov_id_fill_with_uuid(response1.id);
    testrun(!ov_recorder_response_start_equal(&response1, &response2));

    ov_string_copy(response2.id, response1.id, sizeof(response2.id));
    testrun(ov_recorder_response_start_equal(&response1, &response2));

    ov_id_fill_with_uuid(response1.id);
    testrun(!ov_recorder_response_start_equal(&response1, &response2));

    response1.filename = "hagen";
    testrun(!ov_recorder_response_start_equal(&response1, &response2));

    response2.filename = "hagen";
    testrun(!ov_recorder_response_start_equal(&response1, &response2));

    ov_string_copy(response2.id, response1.id, sizeof(response2.id));
    testrun(ov_recorder_response_start_equal(&response1, &response2));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_recorder_response_start_to_json() {

    testrun(!ov_recorder_response_start_to_json(0, 0));

    ov_recorder_response_start start = {0};
    testrun(!ov_recorder_response_start_to_json(0, &start));

    start.filename = "abrakadabra";
    testrun(!ov_recorder_response_start_to_json(0, &start));

    start.filename = 0;
    ov_id_fill_with_uuid(start.id);
    testrun(!ov_recorder_response_start_to_json(0, &start));

    start.filename = "abrakadabra";
    ov_id_fill_with_uuid(start.id);
    testrun(!ov_recorder_response_start_to_json(0, &start));

    ov_json_value *val = ov_json_object();
    start.filename = 0;
    ov_id_clear(start.id);
    testrun(!ov_recorder_response_start_to_json(val, &start));

    start.filename = "abrakadabra";
    testrun(!ov_recorder_response_start_to_json(val, &start));

    start.filename = 0;
    ov_id_fill_with_uuid(start.id);
    testrun(!ov_recorder_response_start_to_json(val, &start));

    start.filename = "abrakadabra";
    ov_id_fill_with_uuid(start.id);
    testrun(ov_recorder_response_start_to_json(val, &start));

    val = ov_json_value_free(val);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_recorder_response_start_from_json() {

    testrun(!ov_recorder_response_start_from_json(0, 0));

    ov_recorder_response_start start_reference = {.filename = "bukephalos"};

    ov_id_fill_with_uuid(start_reference.id);

    ov_json_value *jval = ov_json_object();

    testrun(ov_recorder_response_start_to_json(jval, &start_reference));

    testrun(!ov_recorder_response_start_from_json(0, 0));
    testrun(!ov_recorder_response_start_from_json(jval, 0));

    ov_recorder_response_start start_read = {0};
    testrun(ov_recorder_response_start_from_json(jval, &start_read));

    testrun(ov_recorder_response_start_equal(&start_reference, &start_read));

    jval = ov_json_value_free(jval);
    ov_recorder_response_start_clear(&start_read);

    start_reference.filename = "rudi";
    ov_id_fill_with_uuid(start_reference.id);

    jval = ov_json_object();

    testrun(ov_recorder_response_start_to_json(jval, &start_reference));
    testrun(ov_recorder_response_start_from_json(jval, &start_read));
    testrun(ov_recorder_response_start_equal(&start_reference, &start_read));

    jval = ov_json_value_free(jval);
    ov_recorder_response_start_clear(&start_read);

    return testrun_log_success();
}

/*****************************************************************************
                                   stop event
 ****************************************************************************/

int test_ov_recorder_event_stop_clear() {

    testrun(!ov_recorder_event_stop_clear(0));

    ov_recorder_event_stop event = {0};

    testrun(ov_recorder_event_stop_clear(&event));

    ov_id_fill_with_uuid(event.id);
    testrun(ov_id_valid(event.id));
    testrun(ov_recorder_event_stop_clear(&event));
    testrun(!ov_id_valid(event.id));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_recorder_event_stop_equal() {

    ov_recorder_event_stop event1 = {0};
    ov_recorder_event_stop event2 = {0};

    testrun(ov_recorder_event_stop_equal(0, 0));
    testrun(!ov_recorder_event_stop_equal(&event1, 0));
    testrun(!ov_recorder_event_stop_equal(0, &event2));
    testrun(ov_recorder_event_stop_equal(&event1, &event2));

    ov_id_fill_with_uuid(event1.id);
    testrun(!ov_recorder_event_stop_equal(&event1, &event2));

    strcpy(event2.id, event1.id);
    testrun(ov_recorder_event_stop_equal(&event1, &event2));

    ov_id_clear(event2.id);
    testrun(!ov_recorder_event_stop_equal(&event1, &event2));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_recorder_event_stop_to_json() {

    testrun(!ov_recorder_event_stop_to_json(0, 0));

    ov_recorder_event_stop stop = {0};
    testrun(!ov_recorder_event_stop_to_json(0, &stop));

    ov_id_fill_with_uuid(stop.id);
    testrun(!ov_recorder_event_stop_to_json(0, &stop));

    ov_json_value *val = ov_json_object();
    ov_id_clear(stop.id);
    testrun(!ov_recorder_event_stop_to_json(val, &stop));

    ov_id_fill_with_uuid(stop.id);
    testrun(ov_recorder_event_stop_to_json(val, &stop));

    val = ov_json_value_free(val);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_recorder_event_stop_from_json() {

    testrun(!ov_recorder_event_stop_from_json(0, 0));

    ov_recorder_event_stop stop_reference = {0};
    ov_id_fill_with_uuid(stop_reference.id);

    ov_json_value *jval = ov_json_object();
    ov_recorder_event_stop stop_read = {0};
    testrun(!ov_recorder_event_stop_from_json(jval, 0));
    testrun(!ov_recorder_event_stop_from_json(0, &stop_read));
    testrun(!ov_recorder_event_stop_from_json(jval, &stop_read));

    testrun(ov_recorder_event_stop_to_json(jval, &stop_reference));

    testrun(!ov_recorder_event_stop_from_json(jval, 0));
    testrun(ov_recorder_event_stop_from_json(jval, &stop_read));

    testrun(ov_recorder_event_stop_equal(&stop_reference, &stop_read));

    jval = ov_json_value_free(jval);
    ov_recorder_event_stop_clear(&stop_read);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

OV_TEST_RUN("ov_recorder_events",
            test_ov_recorder_event_start_clear,
            test_ov_recorder_event_start_equal,
            test_ov_recorder_event_start_to_json,
            test_ov_recorder_event_start_from_json,
            test_ov_recorder_response_start_clear,
            test_ov_recorder_response_start_equal,
            test_ov_recorder_response_start_to_json,
            test_ov_recorder_response_start_from_json,
            test_ov_recorder_event_stop_clear,
            test_ov_recorder_event_stop_equal,
            test_ov_recorder_event_stop_to_json,
            test_ov_recorder_event_stop_from_json);
