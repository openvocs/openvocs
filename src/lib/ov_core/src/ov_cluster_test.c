/***
        ------------------------------------------------------------------------

        Copyright (c) 2026 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_cluster_test.c
        @author         Töpfer, Markus

        @date           2026-03-11


        ------------------------------------------------------------------------
*/
#include <ov_test/testrun.h>
#include "ov_cluster.c"

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

int test_ov_cluster_create(){
    
    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);

    ov_cluster_config config = (ov_cluster_config){
        .loop = loop,
        .multicast = (ov_socket_configuration){
            .host = "224.0.0.1",
            .port = 60000
        }
    };

    ov_cluster *cluster = ov_cluster_create(config);
    testrun(cluster);
    testrun(ov_cluster_cast(cluster));
    testrun(cluster->socket > 0);
    testrun(cluster->buffer);

    testrun(NULL == ov_cluster_free(cluster));
    testrun(NULL == ov_event_loop_free(loop));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_cluster_free(){
    
    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);

    ov_cluster_config config = (ov_cluster_config){
        .loop = loop,
        .multicast = (ov_socket_configuration){
            .host = "224.0.0.1",
            .port = 60000,
            .type = UDP
        }
    };

    ov_cluster *cluster = ov_cluster_create(config);

    ov_socket_data remote = ov_socket_configuration_to_socket_data(
        config.multicast);

    socklen_t len = sizeof(struct sockaddr_in);
    
    if (remote.sa.ss_family == AF_INET6)
        len = sizeof(struct sockaddr_in6);

    ov_socket_configuration client = config.multicast;
    client.port = 0;

    int one = ov_socket_create(client, false, NULL);
    int two = ov_socket_create(client, false, NULL);

    char *msg = "{\"incomplete_one\":";
    testrun(0 < sendto(one, msg, strlen(msg), 0, 
        (struct sockaddr *)&remote.sa, len));

    ov_event_loop_run(loop, OV_RUN_ONCE);

    msg = "{\"incomplete_two\":";
    testrun(0 < sendto(two, msg, strlen(msg), 0,
        (struct sockaddr *)&remote.sa, len));

    ov_event_loop_run(loop, OV_RUN_ONCE);

    testrun(NULL == ov_cluster_free(cluster));
    testrun(NULL == ov_event_loop_free(loop));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

struct userdata {

    ov_json_value *msg;
};

/*----------------------------------------------------------------------------*/

static void dummy_io(void *userdata, ov_json_value *msg){

    struct userdata *data = (struct userdata*) userdata;
    data->msg = ov_json_value_free(data->msg);
    data->msg = msg;
    return;
} 

/*----------------------------------------------------------------------------*/

int test_ov_cluster_send(){
    
    struct userdata userdata = {0};

    ov_event_loop *loop = ov_event_loop_default(
        (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

    testrun(loop);

    ov_cluster_config config = (ov_cluster_config){
        .loop = loop,
        .multicast = (ov_socket_configuration){
            .host = "224.0.0.1",
            .port = 60000,
            .type = UDP
        },
        .callback.userdata = &userdata,
        .callback.io = dummy_io
    };

    ov_cluster *one = ov_cluster_create(config);
    ov_cluster *two = ov_cluster_create(config);

    char *m = "{\"test\":\"message\"}";
    ov_json_value *msg = ov_json_value_from_string(m, strlen(m));
    testrun(msg);
    
    testrun(ov_cluster_send(one, msg));

    ov_event_loop_run(loop, OV_RUN_ONCE);

    testrun(0 != userdata.msg);
    userdata.msg = ov_json_value_free(userdata.msg);
    msg = ov_json_value_free(msg);

    testrun(NULL == ov_cluster_free(one));
    testrun(NULL == ov_cluster_free(two));
    testrun(NULL == ov_event_loop_free(loop));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/



/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CLUSTER                                                    #CLUSTER
 *
 *      ------------------------------------------------------------------------
 */

int all_tests() {

    testrun_init();
    testrun_test(test_ov_cluster_create);
    testrun_test(test_ov_cluster_free);
    testrun_test(test_ov_cluster_send);

    return testrun_counter;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST EXECUTION                                                  #EXEC
 *
 *      ------------------------------------------------------------------------
 */

testrun_run(all_tests);
