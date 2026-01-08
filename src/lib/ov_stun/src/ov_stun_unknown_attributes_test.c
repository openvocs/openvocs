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
        @file           ov_stun_unknown_attributes_test.c
        @author         Markus Toepfer

        @date           2018-10-23

        @ingroup        ov_stun

        @brief          Unit tests of RFC 5389 attribute "unknown attributes"


        ------------------------------------------------------------------------
*/

#include "ov_stun_unknown_attributes.c"
#include <ov_test/testrun.h>

#include "../include/ov_stun_nonce.h"
#include "../include/ov_stun_realm.h"
#include "../include/ov_stun_username.h"

#include <ov_base/ov_dump.h>

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

int test_ov_stun_attribute_frame_is_unknown_attributes() {

    size_t size = 1000;
    uint8_t buf[size];
    uint8_t *buffer = buf;

    memset(buf, 'a', size);

    // prepare valid frame
    testrun(ov_stun_attribute_set_type(buffer, size, STUN_UNKNOWN_ATTRIBUTES));
    testrun(ov_stun_attribute_set_length(buffer, size, 1));

    testrun(ov_stun_attribute_frame_is_unknown_attributes(buffer, size));

    testrun(!ov_stun_attribute_frame_is_unknown_attributes(NULL, size));
    testrun(!ov_stun_attribute_frame_is_unknown_attributes(buffer, 0));
    testrun(!ov_stun_attribute_frame_is_unknown_attributes(buffer, 7));

    // min buffer min valid
    testrun(ov_stun_attribute_set_length(buffer, size, 1));
    testrun(ov_stun_attribute_frame_is_unknown_attributes(buffer, 9));

    // length < size
    testrun(ov_stun_attribute_set_length(buffer, size, 8));
    testrun(!ov_stun_attribute_frame_is_unknown_attributes(buffer, 11));
    testrun(ov_stun_attribute_frame_is_unknown_attributes(buffer, 12));

    // type not error_code
    testrun(ov_stun_attribute_set_type(buffer, size, 0));
    testrun(!ov_stun_attribute_frame_is_unknown_attributes(buffer, size));
    testrun(ov_stun_attribute_set_type(buffer, size, STUN_UNKNOWN_ATTRIBUTES));
    testrun(ov_stun_attribute_frame_is_unknown_attributes(buffer, size));

    // check length (no max length specified)
    for (size_t i = 8; i < size - 4; i++) {

        testrun(
            ov_stun_attribute_set_type(buffer, size, STUN_UNKNOWN_ATTRIBUTES));
        testrun(ov_stun_attribute_set_length(buffer, size, i));
        testrun(ov_stun_attribute_frame_is_unknown_attributes(buffer, size));
    }

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_stun_unknown_attributes_decode() {

    size_t size = 100;
    uint8_t buf[size];
    memset(buf, 0, size);

    uint8_t *buffer = buf;
    ov_list *list = NULL;

    testrun(ov_stun_attribute_set_type(buffer, size, STUN_UNKNOWN_ATTRIBUTES));
    testrun(ov_stun_attribute_set_length(buffer, size, 10));

    // prepare some valid attributes
    for (uint8_t i = 0; i < 10; i += 2) {
        *(uint16_t *)(buffer + 4 + i) = htons(i);
    }

    // ov_dump_binary_as_hex(stdout, buffer, 20);

    testrun(!ov_stun_unknown_attributes_decode(NULL, 0));
    testrun(!ov_stun_unknown_attributes_decode(buffer, 0));
    testrun(!ov_stun_unknown_attributes_decode(NULL, 100));

    list = ov_stun_unknown_attributes_decode(buffer, size);
    testrun(list);
    testrun(ov_list_count(list) == 5);
    testrun(ov_list_get(list, 1) == buffer + 4 + 0);
    testrun(ov_list_get(list, 2) == buffer + 4 + 2);
    testrun(ov_list_get(list, 3) == buffer + 4 + 4);
    testrun(ov_list_get(list, 4) == buffer + 4 + 6);
    testrun(ov_list_get(list, 5) == buffer + 4 + 8);
    // check content
    testrun(0 == ntohs(*(uint16_t *)ov_list_get(list, 1)));
    testrun(2 == ntohs(*(uint16_t *)ov_list_get(list, 2)));
    testrun(4 == ntohs(*(uint16_t *)ov_list_get(list, 3)));
    testrun(6 == ntohs(*(uint16_t *)ov_list_get(list, 4)));
    testrun(8 == ntohs(*(uint16_t *)ov_list_get(list, 5)));
    list = ov_list_free(list);

    *(buffer + 4 + 2) = 0x1;
    *(buffer + 4 + 3) = 0xFF;

    *(buffer + 4 + 4) = 0xFA;
    *(buffer + 4 + 5) = 0xCE;

    list = ov_stun_unknown_attributes_decode(buffer, size);
    testrun(list);
    testrun(ov_list_count(list) == 5);
    testrun(ov_list_get(list, 1) == buffer + 4 + 0);
    testrun(ov_list_get(list, 2) == buffer + 4 + 2);
    testrun(ov_list_get(list, 3) == buffer + 4 + 4);
    testrun(ov_list_get(list, 4) == buffer + 4 + 6);
    testrun(ov_list_get(list, 5) == buffer + 4 + 8);
    // check content
    testrun(0 == ntohs(*(uint16_t *)ov_list_get(list, 1)));
    testrun(0x1FF == ntohs(*(uint16_t *)ov_list_get(list, 2)));
    testrun(0xFACE == ntohs(*(uint16_t *)ov_list_get(list, 3)));
    testrun(6 == ntohs(*(uint16_t *)ov_list_get(list, 4)));
    testrun(8 == ntohs(*(uint16_t *)ov_list_get(list, 5)));
    list = ov_list_free(list);

    // frame error length
    testrun(!ov_stun_unknown_attributes_decode(buffer, 4 + 9));

    // frame min
    list = ov_stun_unknown_attributes_decode(buffer, 4 + 10);
    testrun(list);
    testrun(ov_list_count(list) == 5);
    list = ov_list_free(list);

    // frame error type
    testrun(ov_stun_attribute_set_type(buffer, size, 0));
    testrun(!ov_stun_unknown_attributes_decode(buffer, 14));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_stun_unknown_attributes_decode_pointer() {

    size_t size = 100;
    uint8_t buf[size];
    memset(buf, 0, size);

    uint8_t *buffer = buf;
    ov_list *list = NULL;

    testrun(ov_stun_attribute_set_type(buffer, size, STUN_UNKNOWN_ATTRIBUTES));
    testrun(ov_stun_attribute_set_length(buffer, size, 10));

    // prepare some valid attributes
    for (uint8_t i = 0; i < 10; i += 2) {
        *(uint16_t *)(buffer + 4 + i) = htons(i);
    }

    *(buffer + 4 + 2) = 0x1;
    *(buffer + 4 + 3) = 0xFF;

    *(buffer + 4 + 4) = 0xFA;
    *(buffer + 4 + 5) = 0xCE;

    // prepare pointer list
    list = ov_stun_unknown_attributes_decode(buffer, size);
    testrun(list);
    testrun(ov_list_count(list) == 5);
    testrun(0 == ov_stun_unknown_attributes_decode_pointer(NULL));
    testrun(0 ==
            ov_stun_unknown_attributes_decode_pointer(ov_list_get(list, 1)));
    testrun(0x1FF ==
            ov_stun_unknown_attributes_decode_pointer(ov_list_get(list, 2)));
    testrun(0xFACE ==
            ov_stun_unknown_attributes_decode_pointer(ov_list_get(list, 3)));
    testrun(6 ==
            ov_stun_unknown_attributes_decode_pointer(ov_list_get(list, 4)));
    testrun(8 ==
            ov_stun_unknown_attributes_decode_pointer(ov_list_get(list, 5)));
    list = ov_list_free(list);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_stun_unknown_attributes_encode() {

    size_t length = 100;
    uint8_t buf[length];
    memset(buf, 0, length);

    uint8_t *next = 0;
    uint8_t *buffer = buf;

    uint16_t unknown[100] = {0};

    // empty list
    testrun(
        !ov_stun_unknown_attributes_encode(buffer, length, &next, unknown, 0));

    unknown[0] = STUN_NONCE;
    testrun(
        !ov_stun_unknown_attributes_encode(NULL, length, &next, unknown, 1));
    testrun(!ov_stun_unknown_attributes_encode(buffer, 0, &next, unknown, 1));
    testrun(!ov_stun_unknown_attributes_encode(buffer, length, &next, NULL, 1));

    testrun(
        ov_stun_unknown_attributes_encode(buffer, length, NULL, unknown, 1));
    testrun(STUN_UNKNOWN_ATTRIBUTES ==
            ov_stun_attribute_get_type(buffer, length));
    testrun(2 == ov_stun_attribute_get_length(buffer, length));
    testrun(ntohs(*(uint16_t *)(buffer + 4)) == STUN_NONCE);
    testrun(buffer[6] == 0);
    testrun(buffer[7] == 0);

    memset(buf, 0, length);
    testrun(
        ov_stun_unknown_attributes_encode(buffer, length, &next, unknown, 1));
    testrun(STUN_UNKNOWN_ATTRIBUTES ==
            ov_stun_attribute_get_type(buffer, length));
    testrun(2 == ov_stun_attribute_get_length(buffer, length));
    testrun(ntohs(*(uint16_t *)(buffer + 4)) == STUN_NONCE);
    testrun(buffer[6] == 0);
    testrun(buffer[7] == 0);
    testrun(next == buffer + 8);

    // check with 2 attribute list
    unknown[1] = STUN_REALM;
    testrun(
        ov_stun_unknown_attributes_encode(buffer, length, &next, unknown, 2));
    testrun(STUN_UNKNOWN_ATTRIBUTES ==
            ov_stun_attribute_get_type(buffer, length));
    testrun(4 == ov_stun_attribute_get_length(buffer, length));
    testrun(ntohs(*(uint16_t *)(buffer + 4)) == STUN_NONCE);
    testrun(ntohs(*(uint16_t *)(buffer + 6)) == STUN_REALM);
    testrun(next == buffer + 8);

    // check with 3 attribute list
    unknown[2] = STUN_USERNAME;
    testrun(
        ov_stun_unknown_attributes_encode(buffer, length, &next, unknown, 3));
    testrun(STUN_UNKNOWN_ATTRIBUTES ==
            ov_stun_attribute_get_type(buffer, length));
    testrun(6 == ov_stun_attribute_get_length(buffer, length));
    testrun(ntohs(*(uint16_t *)(buffer + 4)) == STUN_NONCE);
    testrun(ntohs(*(uint16_t *)(buffer + 6)) == STUN_REALM);
    testrun(ntohs(*(uint16_t *)(buffer + 8)) == STUN_USERNAME);
    testrun(next == buffer + 12);

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
    testrun_test(test_ov_stun_attribute_frame_is_unknown_attributes);
    testrun_test(test_ov_stun_unknown_attributes_decode);
    testrun_test(test_ov_stun_unknown_attributes_decode_pointer);
    testrun_test(test_ov_stun_unknown_attributes_encode);

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
