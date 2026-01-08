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
        @file           ov_stun_attribute_test.c
        @author         Markus Toepfer

        @date           2018-10-18

        @ingroup        ov_stun

        @brief          Unit tests of ov_stun_attribute


        ------------------------------------------------------------------------
*/

#include "ov_stun_attribute.c"
#include <ov_test/testrun.h>

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

/*----------------------------------------------------------------------------*/

int test_ov_stun_attribute_get_length() {

    uint8_t buf[100] = {0};
    uint8_t *buffer = buf;

    memset(buf, 0, 100);

    buf[2] = 0x00; // LENGTH
    buf[3] = 0x01; // LENGTH

    testrun(-1 == ov_stun_attribute_get_length(NULL, 0));
    testrun(-1 == ov_stun_attribute_get_length(buffer, 0));
    testrun(-1 == ov_stun_attribute_get_length(NULL, 4));
    testrun(-1 == ov_stun_attribute_get_length(buffer, 3));
    testrun(1 == ov_stun_attribute_get_length(buffer, 4));

    buf[2] = 0x00;
    buf[3] = 0x1F;
    testrun(0x1F == ov_stun_attribute_get_length(buffer, 4));

    buf[2] = 0x00;
    buf[3] = 0xFF;
    testrun(0xFF == ov_stun_attribute_get_length(buffer, 4));

    buf[2] = 0xFF;
    buf[3] = 0x00;
    testrun(0xFF00 == ov_stun_attribute_get_length(buffer, 4));

    buf[2] = 0xFA;
    buf[3] = 0xCE;
    testrun(0xFACE == ov_stun_attribute_get_length(buffer, 4));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_stun_attribute_set_length() {

    uint8_t buf[100] = {0};
    uint8_t *buffer = buf;

    memset(buf, 0, 100);

    testrun(!ov_stun_attribute_set_length(NULL, 0, 0));
    testrun(!ov_stun_attribute_set_length(buffer, 0, 0));
    testrun(!ov_stun_attribute_set_length(NULL, 4, 0));
    testrun(!ov_stun_attribute_set_length(buffer, 3, 0));
    testrun(ov_stun_attribute_set_length(buffer, 4, 0));

    testrun(buffer[2] == 0);
    testrun(buffer[3] == 0);

    for (int i = 0; i <= 0xffff; i++) {

        testrun(ov_stun_attribute_set_length(buffer, 4, i));
        testrun(i == ntohs(*(uint16_t *)(buffer + 2)));
    }

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_stun_attribute_frame_is_valid() {

    uint8_t buf[100] = {0};
    uint8_t *buffer = buf;

    memset(buf, 0, 100);

    testrun(!ov_stun_attribute_frame_is_valid(NULL, 0));
    testrun(!ov_stun_attribute_frame_is_valid(buffer, 0));
    testrun(!ov_stun_attribute_frame_is_valid(NULL, 4));
    testrun(!ov_stun_attribute_frame_is_valid(buffer, 3));

    testrun(ov_stun_attribute_frame_is_valid(buffer, 4));
    testrun(ov_stun_attribute_frame_is_valid(buffer, 5));
    testrun(ov_stun_attribute_frame_is_valid(buffer, 6));
    testrun(ov_stun_attribute_frame_is_valid(buffer, 7));
    testrun(ov_stun_attribute_frame_is_valid(buffer, 8));
    testrun(ov_stun_attribute_frame_is_valid(buffer, 9));
    testrun(ov_stun_attribute_frame_is_valid(buffer, 10));

    // MUST be at least 4 + 4 == 8 bytes
    testrun(ov_stun_attribute_set_length(buffer, 4, 2));
    testrun(!ov_stun_attribute_frame_is_valid(buffer, 4));
    testrun(!ov_stun_attribute_frame_is_valid(buffer, 5));
    testrun(!ov_stun_attribute_frame_is_valid(buffer, 6));
    testrun(!ov_stun_attribute_frame_is_valid(buffer, 7));
    testrun(ov_stun_attribute_frame_is_valid(buffer, 8));
    testrun(ov_stun_attribute_frame_is_valid(buffer, 9));
    testrun(ov_stun_attribute_frame_is_valid(buffer, 10));

    // MUST be at least 4 + 4 + 4 == 8 bytes
    testrun(ov_stun_attribute_set_length(buffer, 4, 5));
    testrun(!ov_stun_attribute_frame_is_valid(buffer, 4));
    testrun(!ov_stun_attribute_frame_is_valid(buffer, 5));
    testrun(!ov_stun_attribute_frame_is_valid(buffer, 6));
    testrun(!ov_stun_attribute_frame_is_valid(buffer, 7));
    testrun(!ov_stun_attribute_frame_is_valid(buffer, 8));
    testrun(!ov_stun_attribute_frame_is_valid(buffer, 9));
    testrun(!ov_stun_attribute_frame_is_valid(buffer, 10));
    testrun(!ov_stun_attribute_frame_is_valid(buffer, 11));
    testrun(ov_stun_attribute_frame_is_valid(buffer, 12));
    testrun(ov_stun_attribute_frame_is_valid(buffer, 13));
    testrun(ov_stun_attribute_frame_is_valid(buffer, 14));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_stun_attribute_frame_content() {

    uint8_t buf[100] = {0};
    uint8_t *buffer = buf;

    memset(buf, 0, 100);

    for (int i = 0; i < 100; i++) {

        buf[i] = i;
    }

    uint8_t *content = NULL;
    size_t length = 0;

    // set length 0
    testrun(ov_stun_attribute_set_length(buffer, 4, 0));

    testrun(!ov_stun_attribute_frame_content(NULL, 100, &content, &length));
    testrun(!ov_stun_attribute_frame_content(buffer, 0, &content, &length));
    testrun(!ov_stun_attribute_frame_content(buffer, 100, NULL, &length));
    testrun(!ov_stun_attribute_frame_content(buffer, 100, &content, NULL));

    testrun(ov_stun_attribute_frame_content(buffer, 100, &content, &length));
    testrun(content == NULL);
    testrun(length == 0);

    testrun(ov_stun_attribute_set_length(buffer, 4, 1));
    testrun(ov_stun_attribute_frame_content(buffer, 100, &content, &length));
    testrun(content == buffer + 4);
    testrun(length == 1);
    testrun(ov_stun_attribute_set_length(buffer, 4, 2));
    testrun(ov_stun_attribute_frame_content(buffer, 100, &content, &length));
    testrun(content == buffer + 4);
    testrun(length == 2);
    testrun(ov_stun_attribute_set_length(buffer, 4, 3));
    testrun(ov_stun_attribute_frame_content(buffer, 100, &content, &length));
    testrun(content == buffer + 4);
    testrun(length == 3);
    testrun(ov_stun_attribute_set_length(buffer, 4, 4));
    testrun(ov_stun_attribute_frame_content(buffer, 100, &content, &length));
    testrun(content == buffer + 4);
    testrun(length == 4);
    testrun(ov_stun_attribute_set_length(buffer, 4, 5));
    testrun(ov_stun_attribute_frame_content(buffer, 100, &content, &length));
    testrun(content == buffer + 4);
    testrun(length == 5);
    testrun(ov_stun_attribute_set_length(buffer, 4, 6));
    testrun(ov_stun_attribute_frame_content(buffer, 100, &content, &length));
    testrun(content == buffer + 4);
    testrun(length == 6);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_stun_attribute_set_type() {

    uint8_t buf[100] = {0};
    uint8_t *buffer = buf;

    memset(buf, 0, 100);

    testrun(!ov_stun_attribute_set_type(NULL, 0, 0));
    testrun(!ov_stun_attribute_set_type(buffer, 0, 0));
    testrun(!ov_stun_attribute_set_type(buffer, 1, 0));

    for (int i = 0; i < 0xffff; i++) {

        testrun(ov_stun_attribute_set_type(buffer, 2, i));
        testrun(ntohs(*(uint16_t *)buffer) == i);
    }

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_stun_attribute_get_type() {

    uint8_t buf[100] = {0};
    uint8_t *buffer = buf;

    memset(buf, 0, 100);

    testrun(0 == ov_stun_attribute_get_type(NULL, 0));
    testrun(0 == ov_stun_attribute_get_type(buffer, 0));
    testrun(0 == ov_stun_attribute_get_type(buffer, 1));

    for (int i = 0; i < 0xffff; i++) {

        testrun(ov_stun_attribute_set_type(buffer, 2, i));
        testrun(i == ov_stun_attribute_get_type(buffer, 2));
    }

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_stun_attribute_encode() {

    uint8_t buf[100] = {0};
    uint8_t *buffer = buf;
    uint8_t *next = NULL;

    uint8_t *data = (uint8_t *)"test12345";

    memset(buf, 0, 100);

    testrun(!ov_stun_attribute_encode(NULL, 0, NULL, 0, NULL, 0));
    testrun(!ov_stun_attribute_encode(NULL, 100, NULL, 0, data, 4));
    testrun(!ov_stun_attribute_encode(buffer, 0, NULL, 0, data, 4));
    testrun(ov_stun_attribute_encode(buffer, 100, NULL, 0, NULL, 4));

    memset(buf, 'x', 100);
    testrun(ov_stun_attribute_encode(buffer, 100, NULL, 0, data, 0));
    testrun(buf[0] == 0);
    testrun(buf[1] == 0);
    testrun(buf[2] == 0);
    testrun(buf[3] == 0);
    testrun(buf[4] == 'x');
    testrun(buf[5] == 'x');
    testrun(buf[6] == 'x');
    testrun(buf[7] == 'x');
    testrun(buf[8] == 'x');
    testrun(buf[9] == 'x');
    testrun(buf[10] == 'x');
    testrun(buf[11] == 'x');
    testrun(buf[12] == 'x');

    memset(buf, 'x', 100);
    testrun(ov_stun_attribute_encode(buffer, 100, NULL, 0, data, 1));
    testrun(buf[0] == 0);
    testrun(buf[1] == 0);
    testrun(buf[2] == 0);
    testrun(buf[3] == 1);
    testrun(buf[4] == 't');
    testrun(buf[5] == 0);
    testrun(buf[6] == 0);
    testrun(buf[7] == 0);
    testrun(buf[8] == 'x');
    testrun(buf[9] == 'x');
    testrun(buf[10] == 'x');
    testrun(buf[11] == 'x');
    testrun(buf[12] == 'x');

    memset(buf, 'x', 100);
    testrun(ov_stun_attribute_encode(buffer, 100, NULL, 1, data, 2));
    testrun(buf[0] == 0);
    testrun(buf[1] == 1);
    testrun(buf[2] == 0);
    testrun(buf[3] == 2);
    testrun(buf[4] == 't');
    testrun(buf[5] == 'e');
    testrun(buf[6] == 0);
    testrun(buf[7] == 0);
    testrun(buf[8] == 'x');
    testrun(buf[9] == 'x');
    testrun(buf[10] == 'x');
    testrun(buf[11] == 'x');
    testrun(buf[12] == 'x');

    memset(buf, 'x', 100);
    testrun(ov_stun_attribute_encode(buffer, 100, &next, 2, data, 3));
    testrun(buf[0] == 0);
    testrun(buf[1] == 2);
    testrun(buf[2] == 0);
    testrun(buf[3] == 3);
    testrun(buf[4] == 't');
    testrun(buf[5] == 'e');
    testrun(buf[6] == 's');
    testrun(buf[7] == 0);
    testrun(buf[8] == 'x');
    testrun(buf[9] == 'x');
    testrun(buf[10] == 'x');
    testrun(buf[11] == 'x');
    testrun(buf[12] == 'x');
    testrun(next == buffer + 8);

    memset(buf, 'x', 100);
    testrun(ov_stun_attribute_encode(buffer, 100, &next, 3, data, 4));
    testrun(buf[0] == 0);
    testrun(buf[1] == 3);
    testrun(buf[2] == 0);
    testrun(buf[3] == 4);
    testrun(buf[4] == 't');
    testrun(buf[5] == 'e');
    testrun(buf[6] == 's');
    testrun(buf[7] == 't');
    testrun(buf[8] == 'x');
    testrun(buf[9] == 'x');
    testrun(buf[10] == 'x');
    testrun(buf[11] == 'x');
    testrun(buf[12] == 'x');
    testrun(next == buffer + 8);

    memset(buf, 'x', 100);
    testrun(ov_stun_attribute_encode(buffer, 100, &next, 4, data, 5));
    testrun(buf[0] == 0);
    testrun(buf[1] == 4);
    testrun(buf[2] == 0);
    testrun(buf[3] == 5);
    testrun(buf[4] == 't');
    testrun(buf[5] == 'e');
    testrun(buf[6] == 's');
    testrun(buf[7] == 't');
    testrun(buf[8] == '1');
    testrun(buf[9] == 0);
    testrun(buf[10] == 0);
    testrun(buf[11] == 0);
    testrun(buf[12] == 'x');
    testrun(next == buffer + 12);

    memset(buf, 'x', 100);
    testrun(ov_stun_attribute_encode(buffer, 100, &next, 5, data, 6));
    testrun(buf[0] == 0);
    testrun(buf[1] == 5);
    testrun(buf[2] == 0);
    testrun(buf[3] == 6);
    testrun(buf[4] == 't');
    testrun(buf[5] == 'e');
    testrun(buf[6] == 's');
    testrun(buf[7] == 't');
    testrun(buf[8] == '1');
    testrun(buf[9] == '2');
    testrun(buf[10] == 0);
    testrun(buf[11] == 0);
    testrun(buf[12] == 'x');
    testrun(next == buffer + 12);

    memset(buf, 'x', 100);
    testrun(ov_stun_attribute_encode(buffer, 100, &next, 6, data, 7));
    testrun(buf[0] == 0);
    testrun(buf[1] == 6);
    testrun(buf[2] == 0);
    testrun(buf[3] == 7);
    testrun(buf[4] == 't');
    testrun(buf[5] == 'e');
    testrun(buf[6] == 's');
    testrun(buf[7] == 't');
    testrun(buf[8] == '1');
    testrun(buf[9] == '2');
    testrun(buf[10] == '3');
    testrun(buf[11] == 0);
    testrun(buf[12] == 'x');
    testrun(next == buffer + 12);

    memset(buf, 'x', 100);
    testrun(ov_stun_attribute_encode(buffer, 100, &next, 7, data, 8));
    testrun(buf[0] == 0);
    testrun(buf[1] == 7);
    testrun(buf[2] == 0);
    testrun(buf[3] == 8);
    testrun(buf[4] == 't');
    testrun(buf[5] == 'e');
    testrun(buf[6] == 's');
    testrun(buf[7] == 't');
    testrun(buf[8] == '1');
    testrun(buf[9] == '2');
    testrun(buf[10] == '3');
    testrun(buf[11] == '4');
    testrun(buf[12] == 'x');
    testrun(next == buffer + 12);

    memset(buf, 'x', 100);
    testrun(ov_stun_attribute_encode(buffer, 100, &next, 8, data, 9));
    testrun(buf[0] == 0);
    testrun(buf[1] == 8);
    testrun(buf[2] == 0);
    testrun(buf[3] == 9);
    testrun(buf[4] == 't');
    testrun(buf[5] == 'e');
    testrun(buf[6] == 's');
    testrun(buf[7] == 't');
    testrun(buf[8] == '1');
    testrun(buf[9] == '2');
    testrun(buf[10] == '3');
    testrun(buf[11] == '4');
    testrun(buf[12] == '5');
    testrun(buf[13] == 0);
    testrun(buf[14] == 0);
    testrun(buf[15] == 0);
    testrun(buf[16] == 'x');
    testrun(buf[17] == 'x');
    testrun(buf[18] == 'x');
    testrun(next == buffer + 16);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_stun_attribute_decode() {

    uint8_t buf[100] = {0};
    uint8_t *buffer = buf;
    memset(buf, 0, 100);

    size_t size = 0;
    uint16_t type = 0;
    uint8_t *content = NULL;

    buf[0] = 0;
    buf[1] = 1;
    buf[2] = 0;
    buf[3] = 1;
    memcpy(buffer + 4, "test", 4);
    testrun(!ov_stun_attribute_decode(NULL, 0, NULL, NULL));
    testrun(!ov_stun_attribute_decode(NULL, 100, &type, &content));
    testrun(!ov_stun_attribute_decode(buffer, 0, &type, &content));
    testrun(!ov_stun_attribute_decode(buffer, 100, NULL, &content));
    testrun(!ov_stun_attribute_decode(buffer, 100, &type, NULL));

    testrun(ov_stun_attribute_decode(buffer, 100, &type, &content));
    size = strlen((char *)content);
    testrun(1 == size);
    testrun(1 == type);
    testrun(0 == strncmp((char *)buffer + 4, (char *)content, size));
    free(content);
    content = NULL;

    buf[3] = 2;
    testrun(ov_stun_attribute_decode(buffer, 100, &type, &content));
    size = strlen((char *)content);
    testrun(2 == size);
    testrun(1 == type);
    testrun(0 == strncmp((char *)buffer + 4, (char *)content, size));
    free(content);
    content = NULL;

    buf[3] = 3;
    testrun(ov_stun_attribute_decode(buffer, 100, &type, &content));
    size = strlen((char *)content);
    testrun(3 == size);
    testrun(1 == type);
    testrun(0 == strncmp((char *)buffer + 4, (char *)content, size));
    free(content);
    content = NULL;

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_stun_attributes_get_type() {

    size_t size = 100;
    uint8_t *attr[size];
    memset(attr, 0, sizeof(uint8_t *) * size);

    uint8_t attr0[4] = {0, 0, 0, 0};
    uint8_t attr1[4] = {0, 1, 0, 0};
    uint8_t attr2[4] = {0, 2, 0, 0};
    uint8_t attr3[4] = {0, 3, 0, 0};
    uint8_t attr4[4] = {0, 4, 0, 0};

    testrun(!ov_stun_attributes_get_type(NULL, 0, 0));
    testrun(!ov_stun_attributes_get_type(attr, 0, 0));
    testrun(!ov_stun_attributes_get_type(attr, 1, 0));

    attr[0] = attr0;
    attr[1] = attr1;
    attr[2] = attr2;
    attr[3] = attr3;
    attr[4] = attr4;

    testrun(attr0 == ov_stun_attributes_get_type(attr, 1, 0));
    testrun(attr0 == ov_stun_attributes_get_type(attr, 2, 0));
    testrun(attr0 == ov_stun_attributes_get_type(attr, 3, 0));
    testrun(attr0 == ov_stun_attributes_get_type(attr, 4, 0));
    testrun(attr0 == ov_stun_attributes_get_type(attr, 5, 0));

    testrun(NULL == ov_stun_attributes_get_type(attr, 1, 1));
    testrun(attr1 == ov_stun_attributes_get_type(attr, 2, 1));
    testrun(attr1 == ov_stun_attributes_get_type(attr, 3, 1));
    testrun(attr1 == ov_stun_attributes_get_type(attr, 4, 1));
    testrun(attr1 == ov_stun_attributes_get_type(attr, 5, 1));

    testrun(NULL == ov_stun_attributes_get_type(attr, 1, 2));
    testrun(NULL == ov_stun_attributes_get_type(attr, 2, 2));
    testrun(attr2 == ov_stun_attributes_get_type(attr, 3, 2));
    testrun(attr2 == ov_stun_attributes_get_type(attr, 4, 2));
    testrun(attr2 == ov_stun_attributes_get_type(attr, 5, 2));

    testrun(NULL == ov_stun_attributes_get_type(attr, 1, 3));
    testrun(NULL == ov_stun_attributes_get_type(attr, 2, 3));
    testrun(NULL == ov_stun_attributes_get_type(attr, 3, 3));
    testrun(attr3 == ov_stun_attributes_get_type(attr, 4, 3));
    testrun(attr3 == ov_stun_attributes_get_type(attr, 5, 3));

    testrun(NULL == ov_stun_attributes_get_type(attr, 1, 4));
    testrun(NULL == ov_stun_attributes_get_type(attr, 2, 4));
    testrun(NULL == ov_stun_attributes_get_type(attr, 3, 4));
    testrun(NULL == ov_stun_attributes_get_type(attr, 4, 4));
    testrun(attr4 == ov_stun_attributes_get_type(attr, 5, 4));

    // not contained
    testrun(NULL == ov_stun_attributes_get_type(attr, size, 6));

    // stop on NULL
    attr[3] = 0;

    testrun(attr0 == ov_stun_attributes_get_type(attr, size, 0));
    testrun(attr1 == ov_stun_attributes_get_type(attr, size, 1));
    testrun(attr2 == ov_stun_attributes_get_type(attr, size, 2));
    testrun(NULL == ov_stun_attributes_get_type(attr, size, 3));
    testrun(NULL == ov_stun_attributes_get_type(attr, size, 4));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_stun_attribute_frame_copy() {

    uint8_t dest[100] = {0};
    uint8_t orig[100] = {0};
    uint8_t *next = NULL;

    uint8_t *data = (uint8_t *)"test12345";

    memset(orig, 0, 100);
    memset(dest, 0, 100);

    testrun(ov_stun_attribute_encode(orig, 100, NULL, 1, data, 4));
    testrun(orig[0] == 0);
    testrun(orig[1] == 1);
    testrun(orig[2] == 0);
    testrun(orig[3] == 4);
    testrun(orig[4] == 't');
    testrun(orig[5] == 'e');
    testrun(orig[6] == 's');
    testrun(orig[7] == 't');
    testrun(orig[8] == 0);
    testrun(orig[9] == 0);
    testrun(orig[10] == 0);
    testrun(orig[11] == 0);
    testrun(orig[12] == 0);

    testrun(!ov_stun_attribute_frame_copy(NULL, NULL, 0, NULL));
    testrun(!ov_stun_attribute_frame_copy(orig, dest, 0, NULL));
    testrun(!ov_stun_attribute_frame_copy(NULL, dest, 100, NULL));
    testrun(!ov_stun_attribute_frame_copy(orig, NULL, 100, NULL));

    // dest to small
    testrun(!ov_stun_attribute_frame_copy(orig, dest, 7, NULL));

    // dest min size
    testrun(ov_stun_attribute_frame_copy(orig, dest, 8, NULL));
    testrun(orig[0] == 0);
    testrun(orig[1] == 1);
    testrun(orig[2] == 0);
    testrun(orig[3] == 4);
    testrun(orig[4] == 't');
    testrun(orig[5] == 'e');
    testrun(orig[6] == 's');
    testrun(orig[7] == 't');
    testrun(orig[8] == 0);
    testrun(orig[9] == 0);
    testrun(orig[10] == 0);
    testrun(orig[11] == 0);
    testrun(orig[12] == 0);
    testrun(dest[0] == 0);
    testrun(dest[1] == 1);
    testrun(dest[2] == 0);
    testrun(dest[3] == 4);
    testrun(dest[4] == 't');
    testrun(dest[5] == 'e');
    testrun(dest[6] == 's');
    testrun(dest[7] == 't');
    testrun(dest[8] == 0);
    testrun(dest[9] == 0);
    testrun(dest[10] == 0);
    testrun(dest[11] == 0);
    testrun(dest[12] == 0);

    memset(orig, 0, 100);
    memset(dest, 0, 100);

    testrun(ov_stun_attribute_encode(orig, 100, NULL, 3, data, 5));
    testrun(orig[0] == 0);
    testrun(orig[1] == 3);
    testrun(orig[2] == 0);
    testrun(orig[3] == 5);
    testrun(orig[4] == 't');
    testrun(orig[5] == 'e');
    testrun(orig[6] == 's');
    testrun(orig[7] == 't');
    testrun(orig[8] == '1');
    testrun(orig[9] == 0);
    testrun(orig[10] == 0);
    testrun(orig[11] == 0);
    testrun(orig[12] == 0);

    testrun(!ov_stun_attribute_frame_copy(orig, dest, 8, NULL));
    testrun(ov_stun_attribute_frame_copy(orig, dest, 9, &next));
    testrun(dest[0] == 0);
    testrun(dest[1] == 3);
    testrun(dest[2] == 0);
    testrun(dest[3] == 5);
    testrun(dest[4] == 't');
    testrun(dest[5] == 'e');
    testrun(dest[6] == 's');
    testrun(dest[7] == 't');
    testrun(dest[8] == '1');
    testrun(dest[9] == 0);
    testrun(dest[10] == 0);
    testrun(dest[11] == 0);
    testrun(dest[12] == 0);
    testrun(next == dest + 9);

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
    testrun_test(test_ov_stun_attribute_set_length);
    testrun_test(test_ov_stun_attribute_get_length);

    testrun_test(test_ov_stun_attribute_frame_is_valid);
    testrun_test(test_ov_stun_attribute_frame_content);

    testrun_test(test_ov_stun_attribute_set_type);
    testrun_test(test_ov_stun_attribute_get_type);

    testrun_test(test_ov_stun_attribute_encode);
    testrun_test(test_ov_stun_attribute_decode);

    testrun_test(test_ov_stun_attributes_get_type);
    testrun_test(test_ov_stun_attribute_frame_copy);

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
