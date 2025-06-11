/***
        ------------------------------------------------------------------------

        Copyright 2017 German Aerospace Center DLR e.V. (GSOC)

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

        ------------------------------------------------------------------------
*//**
        @file           ov_dump_tests.c
        @author         Markus Toepfer
        @author         Michael Beer

        @date           2017-12-10

        @ingroup        ov_utils

        @brief


        ------------------------------------------------------------------------
*/
#ifndef OV_TEST_RESOURCE_DIR
#error "Must provide -D OV_TEST_RESOURCE_DIR=value while compiling this file."
#endif

#include "ov_dump.c"
#include <ov_test/testrun.h>

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

int test_ov_dump_binary_as_hex(void) {

    char *path = OV_TEST_RESOURCE_DIR "/dump_hex";
    char *buffer = "ABCD";
    char *expect = " 41 42 43 44";
    char *read = NULL;

    FILE *fp = fopen(path, "w");

    uint64_t filesize = 0;
    size_t count = 0;

    testrun(fp != NULL, "File opened");

    testrun(!ov_dump_binary_as_hex(NULL, NULL, 0), "NULL");
    testrun(!ov_dump_binary_as_hex(fp, (uint8_t *)buffer, 0), "NULL");
    testrun(!ov_dump_binary_as_hex(fp, 0, strlen(buffer)), "NULL");
    testrun(
        !ov_dump_binary_as_hex(0, (uint8_t *)buffer, strlen(buffer)), "NULL");

    testrun(
        ov_dump_binary_as_hex(fp, (uint8_t *)buffer, strlen(buffer)), "dump");
    testrun(fclose(fp) == 0, "File closed");

    fp = fopen(path, "r");
    testrun(fp != NULL, "File reopened");
    testrun(fseek(fp, 0L, SEEK_END) == 0, "seek to EOF");
    filesize = ftell(fp);
    testrun(filesize > 0, "filesize < 1");

    read = calloc(filesize + 1, sizeof(char));

    testrun(fseek(fp, 0L, SEEK_SET) == 0, "seek pointer to SOF");

    count = fread(read, sizeof(char), filesize, fp);
    testrun(count > 0, "Failure reading data from file");

    testrun(fclose(fp) == 0, "File closed");

    testrun(strncmp(read, expect, strlen(expect)) == 0);
    free(read);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_dump_socket_addrinfo() {

    struct addrinfo data = {0};

    testrun(!ov_dump_socket_addrinfo(NULL, NULL));
    testrun(!ov_dump_socket_addrinfo(stdout, NULL));
    testrun(!ov_dump_socket_addrinfo(NULL, &data));

    // empty
    testrun(ov_dump_socket_addrinfo(stdout, &data));

    // set some data
    data.ai_flags = 1;
    data.ai_family = 2;
    data.ai_socktype = 3;
    data.ai_protocol = 4;
    data.ai_addrlen = 5;
    data.ai_canonname = "canon";
    data.ai_next = &data;

    testrun(ov_dump_socket_addrinfo(stdout, &data));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_dump_socket_sockaddr() {

    struct sockaddr_in v4 = {0};
    struct sockaddr_in6 v6 = {0};

    struct sockaddr *data = (struct sockaddr *)&v4;

    testrun(!ov_dump_socket_sockaddr(NULL, NULL));
    testrun(!ov_dump_socket_sockaddr(stdout, NULL));
    testrun(!ov_dump_socket_sockaddr(NULL, data));

    // empty
    testrun(!ov_dump_socket_sockaddr(stdout, data));
    data->sa_family = AF_INET;
    testrun(ov_dump_socket_sockaddr(stdout, data));

    data = (struct sockaddr *)&v4;
    v4.sin_family = AF_INET;
    v4.sin_port = htons(1234);
    v4.sin_addr.s_addr = 1;

    testrun(ov_dump_socket_sockaddr(stdout, data));

    data = (struct sockaddr *)&v6;
    v6.sin6_family = AF_INET6;
    v6.sin6_port = htons(1234);
    v6.sin6_flowinfo = 1;
    v6.sin6_scope_id = 2;
    v6.sin6_addr.s6_addr[1] = 3;

    testrun(ov_dump_socket_sockaddr(stdout, data));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_dump_socket_sockaddr_storage() {

    struct sockaddr_in v4 = {0};
    struct sockaddr_in6 v6 = {0};

    struct sockaddr_storage *data = (struct sockaddr_storage *)&v4;

    testrun(!ov_dump_socket_sockaddr_storage(NULL, NULL));
    testrun(!ov_dump_socket_sockaddr_storage(stdout, NULL));
    testrun(!ov_dump_socket_sockaddr_storage(NULL, data));

    // empty
    testrun(!ov_dump_socket_sockaddr_storage(stdout, data));

    data->ss_family = AF_INET;
    testrun(ov_dump_socket_sockaddr_storage(stdout, data));

    data = (struct sockaddr_storage *)&v4;
    v4.sin_family = AF_INET;
    v4.sin_port = htons(1234);
    v4.sin_addr.s_addr = 1;

    testrun(ov_dump_socket_sockaddr_storage(stdout, data));

    data = (struct sockaddr_storage *)&v6;
    v6.sin6_family = AF_INET6;
    v6.sin6_port = htons(1234);
    v6.sin6_flowinfo = 1;
    v6.sin6_scope_id = 2;
    v6.sin6_addr.s6_addr[1] = 3;

    testrun(ov_dump_socket_sockaddr_storage(stdout, data));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_dump_socket_sockaddr_in() {

    struct sockaddr_in data = {0};

    testrun(!ov_dump_socket_sockaddr_in(NULL, NULL));
    testrun(!ov_dump_socket_sockaddr_in(stdout, NULL));
    testrun(!ov_dump_socket_sockaddr_in(NULL, &data));

    // empty
    testrun(ov_dump_socket_sockaddr_in(stdout, &data));

    data.sin_family = AF_INET;
    data.sin_port = htons(1234);
    data.sin_addr.s_addr = 1;

    testrun(ov_dump_socket_sockaddr_in(stdout, &data));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_dump_socket_sockaddr_in6() {

    struct sockaddr_in6 data = {0};

    testrun(!ov_dump_socket_sockaddr_in6(NULL, NULL));
    testrun(!ov_dump_socket_sockaddr_in6(stdout, NULL));
    testrun(!ov_dump_socket_sockaddr_in6(NULL, &data));

    // empty
    testrun(ov_dump_socket_sockaddr_in6(stdout, &data));

    data.sin6_family = AF_INET6;
    data.sin6_port = htons(1234);
    data.sin6_flowinfo = 1;
    data.sin6_scope_id = 2;
    data.sin6_addr.s6_addr[1] = 3;

    testrun(ov_dump_socket_sockaddr_in6(stdout, &data));

    return testrun_log_success();
}

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CLUSTER                                                    #CLUSTER
 *
 *      ------------------------------------------------------------------------
 */

int all_tests() {

    testrun_init();
    testrun_test(test_ov_dump_binary_as_hex);
    testrun_test(test_ov_dump_socket_addrinfo);
    testrun_test(test_ov_dump_socket_sockaddr);
    testrun_test(test_ov_dump_socket_sockaddr_storage);
    testrun_test(test_ov_dump_socket_sockaddr_in);
    testrun_test(test_ov_dump_socket_sockaddr_in6);

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
