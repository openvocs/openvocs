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
        @file           ov_stun_frame_test.c
        @author         Markus Toepfer

        @date           2018-10-18

        @ingroup        ov_stun

        @brief          Unit tests of ov_stun_frame


        ------------------------------------------------------------------------
*/

#include "ov_stun_frame.c"
#include <ov_test/testrun.h>

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

int test_ov_stun_frame_is_valid() {

    uint8_t buf[100] = {0};
    uint8_t *buffer = buf;

    /*
             0                   1                   2                   3
             0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
            +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
            |0 0|     STUN Message Type     |         Message Length        |
            +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
            |                         Magic Cookie                          |
            +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
            |                                                               |
            |                     Transaction ID (96 bits)                  |
            |                                                               |
            +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

    */
    buf[0] = 0x00;  // TYPE
    buf[1] = 0x00;  // TYPE
    buf[2] = 0x00;  // LENGTH
    buf[3] = 0x00;  // LENGTH
    buf[4] = 0x21;  // MAGIC COOKIE
    buf[5] = 0x12;  // MAGIC COOKIE
    buf[6] = 0xA4;  // MAGIC COOKIE
    buf[7] = 0x42;  // MAGIC COOKIE
    buf[8] = 0x00;  // ID
    buf[9] = 0x00;  // ID
    buf[10] = 0x00; // ID
    buf[11] = 0x00; // ID
    buf[12] = 0x00; // ID
    buf[13] = 0x00; // ID
    buf[14] = 0x00; // ID
    buf[15] = 0x00; // ID
    buf[16] = 0x00; // ID
    buf[17] = 0x00; // ID
    buf[18] = 0x00; // ID
    buf[19] = 0x00; // ID

    // check min positive
    testrun(ov_stun_frame_is_valid(buffer, 20));

    testrun(!ov_stun_frame_is_valid(buffer, 19));
    testrun(!ov_stun_frame_is_valid(NULL, 20));
    testrun(!ov_stun_frame_is_valid(NULL, 0));

    for (int i = 0; i < 0xff; i++) {

        // no leading zeros
        buf[0] = i;
        if ((i & 0x80) || (i & 0x40)) {
            testrun(!ov_stun_frame_is_valid(buffer, 20));
        } else {
            testrun(ov_stun_frame_is_valid(buffer, 20));
        }
        buf[0] = 0x00;

        // no trailing zeros
        buf[3] = i;
        if ((i & 0x01) || (i & 0x02)) {
            testrun(!ov_stun_frame_is_valid(buffer, i + 20));
        } else {

            testrun(ov_stun_frame_is_valid(buffer, i + 20));

            // check length mismatch
            testrun(!ov_stun_frame_is_valid(buffer, i + 19));
            testrun(!ov_stun_frame_is_valid(buffer, i + 21));
            testrun(!ov_stun_frame_is_valid(buffer, i));
        }

        buf[3] = 0x00;

        // MAGIC COOKIE MISMATCH
        buf[4] = i;
        if (i == 0x21) {
            testrun(ov_stun_frame_is_valid(buffer, 20));
        } else {
            testrun(!ov_stun_frame_is_valid(buffer, 20));
        }
        buf[4] = 0x21;

        buf[5] = i;
        if (i == 0x12) {
            testrun(ov_stun_frame_is_valid(buffer, 20));
        } else {
            testrun(!ov_stun_frame_is_valid(buffer, 20));
        }
        buf[5] = 0x12;

        buf[6] = i;
        if (i == 0xA4) {
            testrun(ov_stun_frame_is_valid(buffer, 20));
        } else {
            testrun(!ov_stun_frame_is_valid(buffer, 20));
        }
        buf[6] = 0xA4;

        buf[7] = i;
        if (i == 0x42) {
            testrun(ov_stun_frame_is_valid(buffer, 20));
        } else {
            testrun(!ov_stun_frame_is_valid(buffer, 20));
        }
        buf[7] = 0x42;
    }

    testrun(ov_stun_frame_is_valid(buffer, 20));

    // length mismatch (explicit)
    buffer[2] = 0x01;
    testrun(!ov_stun_frame_is_valid(buffer, 20));
    buffer[2] = 0x00;
    testrun(ov_stun_frame_is_valid(buffer, 20));

    buffer[3] = 0x08;
    testrun(!ov_stun_frame_is_valid(buffer, 20));
    testrun(ov_stun_frame_is_valid(buffer, 28));
    buffer[3] = 0x04;
    testrun(!ov_stun_frame_is_valid(buffer, 28));
    testrun(ov_stun_frame_is_valid(buffer, 24));
    buffer[3] = 0x00;
    testrun(!ov_stun_frame_is_valid(buffer, 24));
    testrun(ov_stun_frame_is_valid(buffer, 20));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_stun_frame_slice() {

    size_t az = STUN_FRAME_ATTRIBUTE_ARRAY_SIZE;
    size_t bz = 100;

    uint8_t *arr[az];
    uint8_t buf[bz];
    uint8_t *buffer = buf;

    memset(buf, 0, bz);
    memset(arr, 0, az);

    for (size_t i = 0; i < az; i++) {
        arr[i] = NULL;
    }

    buf[2] = 0x00; // LENGTH
    buf[3] = 0x00; // LENGTH

    testrun(!ov_stun_frame_slice(NULL, 0, NULL, 0));
    testrun(!ov_stun_frame_slice(NULL, bz, arr, az));
    testrun(!ov_stun_frame_slice(buf, 0, arr, az));
    testrun(!ov_stun_frame_slice(buf, bz, NULL, az));
    testrun(!ov_stun_frame_slice(buf, bz, arr, 0));

    // check min positive ( no attributes contained )
    testrun(ov_stun_frame_slice(buf, 20, arr, az));
    for (size_t i = 0; i < az; i++) {
        testrun(arr[i] == 0);
    }

    // padded length including 1 TLV attribute
    buf[3] = 0x08;
    buf[20] = 0x00; // ATTR TYPE
    buf[21] = 0x00; // ATTR TYPE
    buf[22] = 0x00; // ATTR LENGTH
    buf[23] = 0x01; // ATTR LENGTH - 1 byte

    testrun(!ov_stun_frame_slice(buf, 20, arr, az));
    testrun(!ov_stun_frame_slice(buf, 21, arr, az));
    testrun(!ov_stun_frame_slice(buf, 22, arr, az));
    testrun(!ov_stun_frame_slice(buf, 23, arr, az));
    testrun(!ov_stun_frame_slice(buf, 24, arr, az));
    testrun(!ov_stun_frame_slice(buf, 25, arr, az));
    testrun(!ov_stun_frame_slice(buf, 26, arr, az));
    testrun(!ov_stun_frame_slice(buf, 27, arr, az));

    for (size_t i = 0; i < az; i++) {
        testrun(arr[i] == 0);
    }

    testrun(ov_stun_frame_slice(buf, 28, arr, az));
    testrun(arr[0] == buffer + 20);
    for (size_t i = 1; i < az; i++) {
        testrun(arr[i] == 0);
    }

    // check padding filled
    buf[23] = 0x02; // ATTR LENGTH - 2 byte
    testrun(ov_stun_frame_slice(buf, 28, arr, az));
    testrun(arr[0] == buffer + 20);
    for (size_t i = 1; i < az; i++) {
        testrun(arr[i] == 0);
    }

    buf[23] = 0x03; // ATTR LENGTH - 3 byte
    testrun(ov_stun_frame_slice(buf, 28, arr, az));
    testrun(arr[0] == buffer + 20);
    for (size_t i = 1; i < az; i++) {
        testrun(arr[i] == 0);
    }

    buf[23] = 0x04; // ATTR LENGTH - 4 byte
    testrun(ov_stun_frame_slice(buf, 28, arr, az));
    testrun(arr[0] == buffer + 20);
    for (size_t i = 1; i < az; i++) {
        testrun(arr[i] == 0);
    }

    buf[23] = 0x05; // ATTR LENGTH - 5 byte
    testrun(!ov_stun_frame_slice(buf, 28, arr, az));
    testrun(!ov_stun_frame_slice(buf, 32, arr, az));

    buf[3] = 0x0C;
    testrun(ov_stun_frame_slice(buf, 32, arr, az));
    testrun(arr[0] == buffer + 20);
    for (size_t i = 1; i < az; i++) {
        testrun(arr[i] == 0);
    }

    buf[23] = 0x06; // ATTR LENGTH - 6 byte
    testrun(!ov_stun_frame_slice(buf, 28, arr, az));
    testrun(ov_stun_frame_slice(buf, 32, arr, az));
    testrun(arr[0] == buffer + 20);
    for (size_t i = 1; i < az; i++) {
        testrun(arr[i] == 0);
    }

    buf[23] = 0x07; // ATTR LENGTH - 7 byte
    testrun(!ov_stun_frame_slice(buf, 28, arr, az));
    testrun(ov_stun_frame_slice(buf, 32, arr, az));
    testrun(arr[0] == buffer + 20);
    for (size_t i = 1; i < az; i++) {
        testrun(arr[i] == 0);
    }

    buf[23] = 0x08; // ATTR LENGTH - 8 byte
    testrun(!ov_stun_frame_slice(buf, 28, arr, az));
    testrun(ov_stun_frame_slice(buf, 32, arr, az));
    testrun(arr[0] == buffer + 20);
    for (size_t i = 1; i < az; i++) {
        testrun(arr[i] == 0);
    }

    buf[23] = 0x09; // ATTR LENGTH - 9 byte
    testrun(!ov_stun_frame_slice(buf, 32, arr, az));
    testrun(!ov_stun_frame_slice(buf, 36, arr, az));
    buf[3] = 0x10;
    testrun(ov_stun_frame_slice(buf, 36, arr, az));
    testrun(arr[0] == buffer + 20);
    for (size_t i = 1; i < az; i++) {
        testrun(arr[i] == 0);
    }

    // one attribute
    buf[3] = 0x08;
    buf[23] = 0x01; // ATTR 1 - 1 byte
    testrun(ov_stun_frame_slice(buf, 28, arr, az));
    testrun(arr[0] == buffer + 20);
    for (size_t i = 1; i < az; i++) {
        testrun(arr[i] == 0);
    }

    // two attributes
    buf[3] = 0x10;  // 16 bytes content
    buf[23] = 0x01; // ATTR 1 - 1 byte
    buf[31] = 0x01; // ATTR 2 - 1 byte
    testrun(ov_stun_frame_slice(buf, 36, arr, az));
    testrun(arr[0] == buffer + 20);
    testrun(arr[1] == buffer + 28);
    for (size_t i = 2; i < az; i++) {
        testrun(arr[i] == 0);
    }

    // three attributes
    buf[3] = 0x18;  // 24 bytes content
    buf[23] = 0x01; // ATTR 1 - 1 byte
    buf[31] = 0x01; // ATTR 2 - 1 byte
    buf[39] = 0x01; // ATTR 3 - 1 byte
    testrun(ov_stun_frame_slice(buf, 44, arr, az));
    testrun(arr[0] == buffer + 20);
    testrun(arr[1] == buffer + 28);
    testrun(arr[2] == buffer + 36);
    for (size_t i = 3; i < az; i++) {
        testrun(arr[i] == 0);
    }

    // three attributes - first 0 length
    buf[3] = 0x14;  // 20 bytes content
    buf[23] = 0x00; // ATTR 1 - 0 byte
    buf[27] = 0x01; // ATTR 2 - 1 byte
    buf[35] = 0x01; // ATTR 3 - 1 byte
    testrun(ov_stun_frame_slice(buf, 40, arr, az));
    testrun(arr[0] == buffer + 20);
    testrun(arr[1] == buffer + 24);
    testrun(arr[2] == buffer + 32);
    for (size_t i = 3; i < az; i++) {
        testrun(arr[i] == 0);
    }

    // three attributes - second 0 length
    buf[3] = 0x14;  // 20 bytes content
    buf[23] = 0x01; // ATTR 1 - 1 byte
    buf[31] = 0x00; // ATTR 2 - 0 byte
    buf[35] = 0x01; // ATTR 3 - 1 byte
    testrun(ov_stun_frame_slice(buf, 40, arr, az));
    testrun(arr[0] == buffer + 20);
    testrun(arr[1] == buffer + 28);
    testrun(arr[2] == buffer + 32);
    for (size_t i = 3; i < az; i++) {
        testrun(arr[i] == 0);
    }

    // three attributes - last 0 length
    buf[3] = 0x14;  // 20 bytes content
    buf[23] = 0x01; // ATTR 1 - 1 byte
    buf[31] = 0x01; // ATTR 2 - 1 byte
    buf[39] = 0x00; // ATTR 3 - 0 byte
    testrun(ov_stun_frame_slice(buf, 40, arr, az));
    testrun(arr[0] == buffer + 20);
    testrun(arr[1] == buffer + 28);
    testrun(arr[2] == buffer + 36);
    for (size_t i = 3; i < az; i++) {
        testrun(arr[i] == 0);
    }

    // check length negative
    testrun(!ov_stun_frame_slice(buf, 43, arr, az));
    testrun(!ov_stun_frame_slice(buf, 42, arr, az));
    testrun(!ov_stun_frame_slice(buf, 45, arr, az));
    testrun(!ov_stun_frame_slice(buf, 46, arr, az));
    testrun(!ov_stun_frame_slice(buf, 36, arr, az));

    // three attributes
    buf[3] = 0x1C;  // 28 bytes content
    buf[23] = 0x01; // ATTR 1 - 1 byte
    buf[31] = 0x05; // ATTR 2 - 5 byte
    buf[43] = 0x01; // ATTR 3 - 1 byte
    testrun(ov_stun_frame_slice(buf, 48, arr, az));
    testrun(arr[0] == buffer + 20);
    testrun(arr[1] == buffer + 28);
    testrun(arr[2] == buffer + 40);
    for (size_t i = 3; i < az; i++) {
        testrun(arr[i] == 0);
    }

    // three attributes
    buf[3] = 0x20;  // 32 bytes content
    buf[23] = 0x01; // ATTR 1 - 1 byte
    buf[31] = 0x09; // ATTR 2 - 9 byte
    buf[47] = 0x01; // ATTR 3 - 1 byte
    testrun(ov_stun_frame_slice(buf, 52, arr, az));
    testrun(arr[0] == buffer + 20);
    testrun(arr[1] == buffer + 28);
    testrun(arr[2] == buffer + 44);
    for (size_t i = 3; i < az; i++) {
        testrun(arr[i] == 0);
    }

    // failure three attributes
    buf[3] = 0x20;  // 32 bytes content
    buf[23] = 0x01; // ATTR 1 - 1 byte
    buf[31] = 0x09; // ATTR 2 - 9 byte
    buf[46] = 0x01; // <-- failure
    testrun(!ov_stun_frame_slice(buf, 52, arr, az));
    for (size_t i = 0; i < az; i++) {
        testrun(arr[i] == 0);
    }

    // failure three attributes
    buf[3] = 0x21;  // <-- failure
    buf[23] = 0x04; // ATTR 1 - 1 byte
    buf[31] = 0x09; // ATTR 2 - 9 byte
    buf[47] = 0x01; // ATTR 3 - 1 byte
    testrun(!ov_stun_frame_slice(buf, 52, arr, az));
    for (size_t i = 0; i < az; i++) {
        testrun(arr[i] == 0);
    }

    // failure three attributes
    buf[3] = 0x1F;  // <-- failure
    buf[23] = 0x04; // ATTR 1 - 1 byte
    buf[31] = 0x09; // ATTR 2 - 9 byte
    buf[47] = 0x01; // ATTR 3 - 1 byte
    testrun(!ov_stun_frame_slice(buf, 52, arr, az));
    for (size_t i = 0; i < az; i++) {
        testrun(arr[i] == 0);
    }

    // three attributes
    buf[3] = 0x24;  // 36 bytes content
    buf[23] = 0x08; // ATTR 1 - 8 byte
    buf[35] = 0x09; // ATTR 2 - 9 byte
    buf[51] = 0x01; // ATTR 3 - 1 byte
    testrun(ov_stun_frame_slice(buf, 56, arr, az));
    testrun(arr[0] == buffer + 20);
    testrun(arr[1] == buffer + 32);
    testrun(arr[2] == buffer + 48);
    for (size_t i = 3; i < az; i++) {
        testrun(arr[i] == 0);
    }

    memset(arr, 0, az);

    // check array size to small
    testrun(!ov_stun_frame_slice(buf, 56, arr, 2));
    testrun(!ov_stun_frame_slice(buf, 56, arr, 3));
    testrun(ov_stun_frame_slice(buf, 56, arr, 4));
    testrun(arr[0] == buffer + 20);
    testrun(arr[1] == buffer + 32);
    testrun(arr[2] == buffer + 48);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_stun_frame_get_length() {

    uint8_t buf[100] = {0};
    uint8_t *buffer = buf;

    memset(buf, 0, 100);

    buf[2] = 0x00; // LENGTH
    buf[3] = 0x01; // LENGTH

    testrun(-1 == ov_stun_frame_get_length(NULL, 0));
    testrun(-1 == ov_stun_frame_get_length(buffer, 0));
    testrun(-1 == ov_stun_frame_get_length(NULL, 4));
    testrun(-1 == ov_stun_frame_get_length(buffer, 3));
    testrun(1 == ov_stun_frame_get_length(buffer, 4));

    buf[2] = 0x00;
    buf[3] = 0x1F;
    testrun(0x1F == ov_stun_frame_get_length(buffer, 4));

    buf[2] = 0x00;
    buf[3] = 0xFF;
    testrun(0xFF == ov_stun_frame_get_length(buffer, 4));

    buf[2] = 0xFF;
    buf[3] = 0x00;
    testrun(0xFF00 == ov_stun_frame_get_length(buffer, 4));

    buf[2] = 0xFA;
    buf[3] = 0xCE;
    testrun(0xFACE == ov_stun_frame_get_length(buffer, 4));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_stun_frame_set_length() {

    uint8_t buf[100] = {0};
    uint8_t *buffer = buf;

    memset(buf, 0, 100);

    testrun(!ov_stun_frame_set_length(NULL, 0, 0));
    testrun(!ov_stun_frame_set_length(buffer, 0, 0));
    testrun(!ov_stun_frame_set_length(NULL, 20, 0));
    testrun(!ov_stun_frame_set_length(buffer, 19, 0));
    testrun(ov_stun_frame_set_length(buffer, 20, 0));

    testrun(buffer[2] == 0);
    testrun(buffer[3] == 0);

    for (int i = 0; i <= 0xffff; i++) {

        testrun(ov_stun_frame_set_length(buffer, 20, i));
        testrun(i == ntohs(*(uint16_t *)(buffer + 2)));
    }

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static bool item_not_equals_buffer(void *item, void *buffer) {

    if (!item || !buffer) return false;

    // item equals buffer?
    if (0 == memcmp(item, buffer, 12)) return false;

    return true;
}

/*----------------------------------------------------------------------------*/

int test_ov_stun_frame_generate_transaction_id() {

    // Test will run exponetional longer with bigger uniques checks
    //
    // Example time run 0xffff on Intel(R) Core(TM) i7-4600U CPU @ 2.10GHz
    // ALL TESTS RUN (30 tests)
    // Clock ticks function: ( main ) | 573.479020 s | 573479 ms

    size_t unique = 255;
    ov_list *list = ov_list_create(
        (ov_list_config){.item = ov_data_string_data_functions()});

    uint8_t *buffer = NULL;

    for (size_t i = 0; i < unique; i++) {

        buffer = calloc(13, sizeof(char));
        testrun(ov_stun_frame_generate_transaction_id(buffer));
        testrun(strlen((char *)buffer) == 12);

        if (!ov_list_is_empty(list)) {

            testrun(ov_list_for_each(list, buffer, item_not_equals_buffer));
        }

        testrun(ov_list_push(list, buffer));
    }

    ov_list_free(list);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_stun_frame_set_transaction_id() {

    uint8_t buf[100] = {0};
    uint8_t *buffer = buf;

    memset(buf, 0, 100);

    uint8_t id[12] = {
        0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC};

    testrun(!ov_stun_frame_set_transaction_id(NULL, 0, NULL));
    testrun(!ov_stun_frame_set_transaction_id(buffer, 0, id));
    testrun(!ov_stun_frame_set_transaction_id(NULL, 20, id));
    testrun(!ov_stun_frame_set_transaction_id(buffer, 19, id));
    testrun(!ov_stun_frame_set_transaction_id(buffer, 20, NULL));

    testrun(buffer[0] == 0);
    testrun(buffer[1] == 0);

    for (int i = 0; i <= 20; i++) {

        testrun(0 == buffer[i]);
    }

    testrun(ov_stun_frame_set_transaction_id(buffer, 20, id));
    testrun(buffer[0] == 0);
    testrun(buffer[1] == 0);
    testrun(buffer[2] == 0);
    testrun(buffer[3] == 0);
    testrun(buffer[4] == 0);
    testrun(buffer[5] == 0);
    testrun(buffer[6] == 0);
    testrun(buffer[7] == 0);
    testrun(buffer[8] == 0x11);
    testrun(buffer[9] == 0x22);
    testrun(buffer[10] == 0x33);
    testrun(buffer[11] == 0x44);
    testrun(buffer[12] == 0x55);
    testrun(buffer[13] == 0x66);
    testrun(buffer[14] == 0x77);
    testrun(buffer[15] == 0x88);
    testrun(buffer[16] == 0x99);
    testrun(buffer[17] == 0xAA);
    testrun(buffer[18] == 0xBB);
    testrun(buffer[19] == 0xCC);
    testrun(buffer[20] == 0);
    testrun(buffer[21] == 0);

    id[0] = 0xFA;
    id[1] = 0xCE;

    testrun(ov_stun_frame_set_transaction_id(buffer, 20, id));
    testrun(buffer[0] == 0);
    testrun(buffer[1] == 0);
    testrun(buffer[2] == 0);
    testrun(buffer[3] == 0);
    testrun(buffer[4] == 0);
    testrun(buffer[5] == 0);
    testrun(buffer[6] == 0);
    testrun(buffer[7] == 0);
    testrun(buffer[8] == 0xFA);
    testrun(buffer[9] == 0xCE);
    testrun(buffer[10] == 0x33);
    testrun(buffer[11] == 0x44);
    testrun(buffer[12] == 0x55);
    testrun(buffer[13] == 0x66);
    testrun(buffer[14] == 0x77);
    testrun(buffer[15] == 0x88);
    testrun(buffer[16] == 0x99);
    testrun(buffer[17] == 0xAA);
    testrun(buffer[18] == 0xBB);
    testrun(buffer[19] == 0xCC);
    testrun(buffer[20] == 0);
    testrun(buffer[21] == 0);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_stun_frame_get_transaction_id() {

    uint8_t buf[100] = {0};
    uint8_t *buffer = buf;

    memset(buf, 0, 100);

    testrun(!ov_stun_frame_get_transaction_id(NULL, 0));
    testrun(!ov_stun_frame_get_transaction_id(buffer, 0));
    testrun(!ov_stun_frame_get_transaction_id(NULL, 20));
    testrun(!ov_stun_frame_get_transaction_id(buffer, 19));
    testrun(buffer + 8 == ov_stun_frame_get_transaction_id(buffer, 20));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_stun_frame_has_magic_cookie() {

    uint8_t buf[100] = {0};
    uint8_t *buffer = buf;

    memset(buf, 0, 100);

    buf[4] = 0x21;
    buf[5] = 0x12;
    buf[6] = 0xA4;
    buf[7] = 0x42;

    testrun(!ov_stun_frame_has_magic_cookie(NULL, 0));
    testrun(!ov_stun_frame_has_magic_cookie(buffer, 0));
    testrun(!ov_stun_frame_has_magic_cookie(NULL, 20));
    testrun(!ov_stun_frame_has_magic_cookie(buffer, 19));
    testrun(ov_stun_frame_has_magic_cookie(buffer, 20));

    for (int i = 0; i <= 0xff; i++) {

        buf[4] = i;
        if (i != 0x21) {
            testrun(!ov_stun_frame_has_magic_cookie(buffer, 20));
        } else {
            testrun(ov_stun_frame_has_magic_cookie(buffer, 20));
        }
        buf[4] = 0x21;

        buf[5] = i;
        if (i != 0x12) {
            testrun(!ov_stun_frame_has_magic_cookie(buffer, 20));
        } else {
            testrun(ov_stun_frame_has_magic_cookie(buffer, 20));
        }
        buf[5] = 0x12;

        buf[6] = i;
        if (i != 0xA4) {
            testrun(!ov_stun_frame_has_magic_cookie(buffer, 20));
        } else {
            testrun(ov_stun_frame_has_magic_cookie(buffer, 20));
        }
        buf[6] = 0xA4;

        buf[7] = i;
        if (i != 0x42) {
            testrun(!ov_stun_frame_has_magic_cookie(buffer, 20));
        } else {
            testrun(ov_stun_frame_has_magic_cookie(buffer, 20));
        }
        buf[7] = 0x42;
    }

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_stun_frame_set_magic_cookie() {

    uint8_t buf[100] = {0};
    uint8_t *buffer = buf;

    memset(buf, 0, 100);

    testrun(!ov_stun_frame_set_magic_cookie(NULL, 0));
    testrun(!ov_stun_frame_set_magic_cookie(buffer, 0));
    testrun(!ov_stun_frame_set_magic_cookie(NULL, 20));
    testrun(!ov_stun_frame_set_magic_cookie(buffer, 19));
    testrun(ov_stun_frame_set_magic_cookie(buffer, 20));

    testrun(ov_stun_frame_has_magic_cookie(buffer, 20));

    for (int i = 0; i <= 0xff; i++) {

        buf[4] = i;
        testrun(ov_stun_frame_set_magic_cookie(buffer, 20));
        testrun(ov_stun_frame_has_magic_cookie(buffer, 20));

        buf[5] = i;
        testrun(ov_stun_frame_set_magic_cookie(buffer, 20));
        testrun(ov_stun_frame_has_magic_cookie(buffer, 20));

        buf[6] = i;
        testrun(ov_stun_frame_set_magic_cookie(buffer, 20));
        testrun(ov_stun_frame_has_magic_cookie(buffer, 20));

        buf[7] = i;
        testrun(ov_stun_frame_set_magic_cookie(buffer, 20));
        testrun(ov_stun_frame_has_magic_cookie(buffer, 20));
    }

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_stun_frame_get_method() {

    uint8_t buffer[100] = {0};
    uint8_t *start = buffer;

    buffer[0] = 0x00;
    buffer[1] = 0x00;

    testrun(0 == ov_stun_frame_get_method(NULL, 0));
    testrun(0 == ov_stun_frame_get_method(start, 0));
    testrun(0 == ov_stun_frame_get_method(NULL, 2));
    testrun(0 == ov_stun_frame_get_method(start, 1));

    testrun(0 == ov_stun_frame_get_method(start, 2));

    buffer[0] = 0x00;
    buffer[1] = 0x01;

    testrun(1 == ov_stun_frame_get_method(start, 2));
    testrun(1 == ov_stun_frame_get_method(start, 3));
    testrun(1 == ov_stun_frame_get_method(start, 4));
    testrun(1 == ov_stun_frame_get_method(start, 5));

    uint16_t type = 0;

    for (int i = 0; i <= 0xff; i++) {

        buffer[0] = 0;
        buffer[1] = i;

        if (i < 0x10) {

            testrun(i == ov_stun_frame_get_method(start, 2));
            testrun(i == ov_stun_frame_get_method(start, 3));
            testrun(i == ov_stun_frame_get_method(start, 4));
        }

        type = i & 0x0F;
        type += ((i >> 5) << 4);

        testrun(type == ov_stun_frame_get_method(start, 2));
        testrun(type == ov_stun_frame_get_method(start, 3));
        testrun(type == ov_stun_frame_get_method(start, 4));

        buffer[0] = i;

        type += ((i >> 1) << 7);
        testrun(type == ov_stun_frame_get_method(start, 2));
        testrun(type == ov_stun_frame_get_method(start, 3));
        testrun(type == ov_stun_frame_get_method(start, 4));
    }

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_stun_frame_set_method() {

    uint8_t buf[100] = {0};
    uint8_t *buffer = buf;

    memset(buf, 0, 100);

    testrun(!ov_stun_frame_set_method(NULL, 0, 0));
    testrun(!ov_stun_frame_set_method(buffer, 0, 0));
    testrun(!ov_stun_frame_set_method(NULL, 20, 0));
    testrun(!ov_stun_frame_set_method(buffer, 19, 0));
    testrun(ov_stun_frame_set_method(buffer, 20, 0));

    testrun(buffer[0] == 0);
    testrun(buffer[1] == 0);

    for (int i = 0; i <= 0xffff; i++) {

        if (i <= 0xfff) {

            testrun(ov_stun_frame_set_request(buffer, 20));
            testrun(ov_stun_frame_set_method(buffer, 20, i));
            testrun(i == ov_stun_frame_get_method(buffer, 20));
            testrun(ov_stun_frame_class_is_request(buffer, 20));

            testrun(ov_stun_frame_set_indication(buffer, 20));
            testrun(ov_stun_frame_set_method(buffer, 20, i));
            testrun(i == ov_stun_frame_get_method(buffer, 20));
            testrun(ov_stun_frame_class_is_indication(buffer, 20));

            testrun(ov_stun_frame_set_success_response(buffer, 20));
            testrun(ov_stun_frame_set_method(buffer, 20, i));
            testrun(i == ov_stun_frame_get_method(buffer, 20));
            testrun(ov_stun_frame_class_is_success_response(buffer, 20));

            testrun(ov_stun_frame_set_error_response(buffer, 20));
            testrun(ov_stun_frame_set_method(buffer, 20, i));
            testrun(i == ov_stun_frame_get_method(buffer, 20));
            testrun(ov_stun_frame_class_is_error_response(buffer, 20));

        } else {
            testrun(!ov_stun_frame_set_method(buffer, 20, i));
        }
    }

    return testrun_log_success();
}

int test_ov_stun_frame_class_is_request() {

    uint8_t buffer[100] = {0};
    uint8_t *start = buffer;

    buffer[0] = 0x00;
    buffer[1] = 0x00;

    testrun(!ov_stun_frame_class_is_request(NULL, 0));
    testrun(!ov_stun_frame_class_is_request(start, 0));
    testrun(!ov_stun_frame_class_is_request(NULL, 2));
    testrun(!ov_stun_frame_class_is_request(start, 1));

    // bit 0x01 and 0x10 are not set
    testrun(ov_stun_frame_class_is_request(start, 2));
    testrun(ov_stun_frame_class_is_request(start, 3));
    testrun(ov_stun_frame_class_is_request(start, 4));
    testrun(ov_stun_frame_class_is_request(start, 5));

    for (int i = 0; i <= 0xff; i++) {

        buffer[0] = 0x00;
        buffer[1] = 0x00;

        buffer[0] = i;
        if (i & 0x01) {

            // first set second not set
            testrun(!ov_stun_frame_class_is_request(start, 2));
            // both set
            buffer[1] = 0x10;
            testrun(!ov_stun_frame_class_is_request(start, 2));
            buffer[1] = 0x00;

        } else {

            // both not set
            testrun(ov_stun_frame_class_is_request(start, 2));
        }

        buffer[0] = 0x00;
        buffer[1] = 0x00;

        buffer[1] = i;
        if (i & 0x10) {

            // first not set, second set
            testrun(!ov_stun_frame_class_is_request(start, 2));
            // both set
            buffer[0] = 0x01;
            testrun(!ov_stun_frame_class_is_request(start, 2));
            buffer[0] = 0x00;

        } else {

            // both not set
            testrun(ov_stun_frame_class_is_request(start, 2));
        }
        buffer[1] = 0;
    }

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_stun_frame_class_is_indication() {

    uint8_t buffer[100] = {0};
    uint8_t *start = buffer;

    buffer[0] = 0x00;
    buffer[1] = 0x00;

    testrun(!ov_stun_frame_class_is_indication(NULL, 0));
    testrun(!ov_stun_frame_class_is_indication(start, 0));
    testrun(!ov_stun_frame_class_is_indication(NULL, 2));
    testrun(!ov_stun_frame_class_is_indication(start, 1));

    testrun(!ov_stun_frame_class_is_indication(start, 2));

    buffer[0] = 0x00;
    buffer[1] = 0x10;
    testrun(ov_stun_frame_class_is_indication(start, 2));
    testrun(ov_stun_frame_class_is_indication(start, 3));
    testrun(ov_stun_frame_class_is_indication(start, 4));
    testrun(ov_stun_frame_class_is_indication(start, 5));

    for (int i = 0; i <= 0xff; i++) {

        buffer[0] = 0x00;
        buffer[1] = 0x00;

        buffer[0] = i;
        if (i & 0x01) {

            // first set second not set
            testrun(!ov_stun_frame_class_is_indication(start, 2));
            // both set
            buffer[1] = 0x10;
            testrun(!ov_stun_frame_class_is_indication(start, 2));
            buffer[1] = 0x00;

        } else {

            // both not set
            testrun(!ov_stun_frame_class_is_indication(start, 2));
        }

        buffer[0] = 0x00;
        buffer[1] = 0x00;

        buffer[1] = i;
        if (i & 0x10) {

            // first not set, second set
            testrun(ov_stun_frame_class_is_indication(start, 2));
            // both set
            buffer[0] = 0x01;
            testrun(!ov_stun_frame_class_is_indication(start, 2));
            buffer[0] = 0x00;

        } else {

            // both not set
            testrun(!ov_stun_frame_class_is_indication(start, 2));
        }
        buffer[1] = 0;
    }

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_stun_frame_class_is_success_response() {

    uint8_t buffer[100] = {0};
    uint8_t *start = buffer;

    buffer[0] = 0x00;
    buffer[1] = 0x00;

    testrun(!ov_stun_frame_class_is_success_response(NULL, 0));
    testrun(!ov_stun_frame_class_is_success_response(start, 0));
    testrun(!ov_stun_frame_class_is_success_response(NULL, 2));
    testrun(!ov_stun_frame_class_is_success_response(start, 1));

    testrun(!ov_stun_frame_class_is_success_response(start, 2));

    buffer[0] = 0x01;
    buffer[1] = 0x00;
    testrun(ov_stun_frame_class_is_success_response(start, 2));
    testrun(ov_stun_frame_class_is_success_response(start, 3));
    testrun(ov_stun_frame_class_is_success_response(start, 4));
    testrun(ov_stun_frame_class_is_success_response(start, 5));

    for (int i = 0; i <= 0xff; i++) {

        buffer[0] = 0x00;
        buffer[1] = 0x00;

        buffer[0] = i;
        if (i & 0x01) {

            // first set second not set
            testrun(ov_stun_frame_class_is_success_response(start, 2));
            // both set
            buffer[1] = 0x10;
            testrun(!ov_stun_frame_class_is_success_response(start, 2));
            buffer[1] = 0x00;

        } else {

            // both not set
            testrun(!ov_stun_frame_class_is_success_response(start, 2));
        }

        buffer[0] = 0x00;
        buffer[1] = 0x00;

        buffer[1] = i;
        if (i & 0x10) {

            // first not set, second set
            testrun(!ov_stun_frame_class_is_success_response(start, 2));
            // both set
            buffer[0] = 0x01;
            testrun(!ov_stun_frame_class_is_success_response(start, 2));
            buffer[0] = 0x00;

        } else {

            // both not set
            testrun(!ov_stun_frame_class_is_success_response(start, 2));
        }
        buffer[1] = 0;
    }

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_stun_frame_class_is_error_response() {

    uint8_t buffer[100] = {0};
    uint8_t *start = buffer;

    buffer[0] = 0x00;
    buffer[1] = 0x00;

    testrun(!ov_stun_frame_class_is_error_response(NULL, 0));
    testrun(!ov_stun_frame_class_is_error_response(start, 0));
    testrun(!ov_stun_frame_class_is_error_response(NULL, 2));
    testrun(!ov_stun_frame_class_is_error_response(start, 1));

    testrun(!ov_stun_frame_class_is_error_response(start, 2));

    buffer[0] = 0x01;
    buffer[1] = 0x10;
    testrun(ov_stun_frame_class_is_error_response(start, 2));
    testrun(ov_stun_frame_class_is_error_response(start, 3));
    testrun(ov_stun_frame_class_is_error_response(start, 4));
    testrun(ov_stun_frame_class_is_error_response(start, 5));

    for (int i = 0; i <= 0xff; i++) {

        buffer[0] = 0x00;
        buffer[1] = 0x00;

        buffer[0] = i;
        if (i & 0x01) {

            // first set second not set
            testrun(!ov_stun_frame_class_is_error_response(start, 2));
            // both set
            buffer[1] = 0x10;
            testrun(ov_stun_frame_class_is_error_response(start, 2));
            buffer[1] = 0x00;

        } else {

            // both not set
            testrun(!ov_stun_frame_class_is_error_response(start, 2));
        }

        buffer[0] = 0x00;
        buffer[1] = 0x00;

        buffer[1] = i;
        if (i & 0x10) {

            // first not set, second set
            testrun(!ov_stun_frame_class_is_error_response(start, 2));
            // both set
            buffer[0] = 0x01;
            testrun(ov_stun_frame_class_is_error_response(start, 2));
            buffer[0] = 0x00;

        } else {

            // both not set
            testrun(!ov_stun_frame_class_is_error_response(start, 2));
        }
        buffer[1] = 0;
    }

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_stun_frame_set_error_response() {

    uint8_t buf[100] = {0};
    uint8_t *buffer = buf;

    memset(buf, 0, 100);

    testrun(!ov_stun_frame_set_error_response(NULL, 0));
    testrun(!ov_stun_frame_set_error_response(buffer, 0));
    testrun(!ov_stun_frame_set_error_response(NULL, 20));
    testrun(!ov_stun_frame_set_error_response(buffer, 19));
    testrun(ov_stun_frame_set_error_response(buffer, 20));

    testrun(ov_stun_frame_class_is_error_response(buffer, 20));

    for (int i = 0; i <= 0xff; i++) {

        buf[0] = i;
        buf[1] = i;
        testrun(ov_stun_frame_set_error_response(buffer, 20));
        testrun(ov_stun_frame_class_is_error_response(buffer, 20));
    }

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_stun_frame_set_success_response() {

    uint8_t buf[100] = {0};
    uint8_t *buffer = buf;

    memset(buf, 0, 100);

    testrun(!ov_stun_frame_set_success_response(NULL, 0));
    testrun(!ov_stun_frame_set_success_response(buffer, 0));
    testrun(!ov_stun_frame_set_success_response(NULL, 20));
    testrun(!ov_stun_frame_set_success_response(buffer, 19));
    testrun(ov_stun_frame_set_success_response(buffer, 20));

    testrun(ov_stun_frame_class_is_success_response(buffer, 20));

    for (int i = 0; i <= 0xff; i++) {

        buf[0] = i;
        buf[1] = i;
        testrun(ov_stun_frame_set_success_response(buffer, 20));
        testrun(ov_stun_frame_class_is_success_response(buffer, 20));
    }

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_stun_frame_set_request() {

    uint8_t buf[100] = {0};
    uint8_t *buffer = buf;

    memset(buf, 0, 100);

    testrun(!ov_stun_frame_set_request(NULL, 0));
    testrun(!ov_stun_frame_set_request(buffer, 0));
    testrun(!ov_stun_frame_set_request(NULL, 20));
    testrun(!ov_stun_frame_set_request(buffer, 19));
    testrun(ov_stun_frame_set_request(buffer, 20));

    testrun(ov_stun_frame_class_is_request(buffer, 20));

    for (int i = 0; i <= 0xff; i++) {

        buf[0] = i;
        buf[1] = i;
        testrun(ov_stun_frame_set_request(buffer, 20));
        testrun(ov_stun_frame_class_is_request(buffer, 20));
    }

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_stun_frame_set_indication() {
    uint8_t buf[100] = {0};
    uint8_t *buffer = buf;

    memset(buf, 0, 100);

    testrun(!ov_stun_frame_set_indication(NULL, 0));
    testrun(!ov_stun_frame_set_indication(buffer, 0));
    testrun(!ov_stun_frame_set_indication(NULL, 20));
    testrun(!ov_stun_frame_set_indication(buffer, 19));
    testrun(ov_stun_frame_set_indication(buffer, 20));

    testrun(ov_stun_frame_class_is_indication(buffer, 20));

    for (int i = 0; i <= 0xff; i++) {

        buf[0] = i;
        buf[1] = i;
        testrun(ov_stun_frame_set_indication(buffer, 20));
        testrun(ov_stun_frame_class_is_indication(buffer, 20));
    }

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_stun_frame_add_padding() {

    uint8_t buf[100] = {0};
    uint8_t *start = buf;
    uint8_t *ptr = start;

    testrun(!ov_stun_frame_add_padding(NULL, NULL));
    testrun(!ov_stun_frame_add_padding(start, NULL));
    testrun(!ov_stun_frame_add_padding(NULL, &ptr));

    testrun(ov_stun_frame_add_padding(start, &ptr));
    testrun(ptr == start);

    ptr = start + 1;
    testrun(ov_stun_frame_add_padding(start, &ptr));
    testrun(ptr == start + 4);

    ptr = start + 2;
    testrun(ov_stun_frame_add_padding(start, &ptr));
    testrun(ptr == start + 4);

    ptr = start + 3;
    testrun(ov_stun_frame_add_padding(start, &ptr));
    testrun(ptr == start + 4);

    ptr = start + 4;
    testrun(ov_stun_frame_add_padding(start, &ptr));
    testrun(ptr == start + 4);

    ptr = start + 5;
    testrun(ov_stun_frame_add_padding(start, &ptr));
    testrun(ptr == start + 8);

    ptr = start + 6;
    testrun(ov_stun_frame_add_padding(start, &ptr));
    testrun(ptr == start + 8);

    ptr = start + 7;
    testrun(ov_stun_frame_add_padding(start, &ptr));
    testrun(ptr == start + 8);

    ptr = start + 8;
    testrun(ov_stun_frame_add_padding(start, &ptr));
    testrun(ptr == start + 8);

    ptr = start + 9;
    testrun(ov_stun_frame_add_padding(start, &ptr));
    testrun(ptr == start + 12);

    ptr = start + 10;
    testrun(ov_stun_frame_add_padding(start, &ptr));
    testrun(ptr == start + 12);

    ptr = start + 11;
    testrun(ov_stun_frame_add_padding(start, &ptr));
    testrun(ptr == start + 12);

    ptr = start + 12;
    testrun(ov_stun_frame_add_padding(start, &ptr));
    testrun(ptr == start + 12);

    ptr = start + 13;
    testrun(ov_stun_frame_add_padding(start, &ptr));
    testrun(ptr == start + 16);

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

    testrun_test(test_ov_stun_frame_is_valid);
    testrun_test(test_ov_stun_frame_slice);

    testrun_test(test_ov_stun_frame_get_length);
    testrun_test(test_ov_stun_frame_set_length);

    testrun_test(test_ov_stun_frame_generate_transaction_id);
    testrun_test(test_ov_stun_frame_get_transaction_id);
    testrun_test(test_ov_stun_frame_set_transaction_id);

    testrun_test(test_ov_stun_frame_has_magic_cookie);
    testrun_test(test_ov_stun_frame_set_magic_cookie);

    testrun_test(test_ov_stun_frame_get_method);
    testrun_test(test_ov_stun_frame_set_method);

    testrun_test(test_ov_stun_frame_class_is_error_response);
    testrun_test(test_ov_stun_frame_class_is_indication);
    testrun_test(test_ov_stun_frame_class_is_request);
    testrun_test(test_ov_stun_frame_class_is_success_response);

    testrun_test(test_ov_stun_frame_set_error_response);
    testrun_test(test_ov_stun_frame_set_indication);
    testrun_test(test_ov_stun_frame_set_request);
    testrun_test(test_ov_stun_frame_set_success_response);

    testrun_test(test_ov_stun_frame_add_padding);

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
