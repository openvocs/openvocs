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
        @file           ov_stun_message_integrity_test.c
        @author         Markus Toepfer

        @date           2018-10-22

        @ingroup        ov_stun

        @brief          Unit tests of RFC 5389 attribute "message integrity"


        ------------------------------------------------------------------------
*/

#include "ov_stun_message_integrity.c"
#include <ov_test/testrun.h>

#include <ov_base/ov_dump.h>

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

int test_ov_stun_attribute_frame_is_message_integrity() {

    size_t size = 1000;
    uint8_t buf[size];
    uint8_t *buffer = buf;

    memset(buf, 'a', size);

    // prepare valid frame
    testrun(ov_stun_attribute_set_type(buffer, size, STUN_MESSAGE_INTEGRITY));
    testrun(ov_stun_attribute_set_length(buffer, size, 20));

    testrun(ov_stun_attribute_frame_is_message_integrity(buffer, size));

    testrun(!ov_stun_attribute_frame_is_message_integrity(NULL, size));
    testrun(!ov_stun_attribute_frame_is_message_integrity(buffer, 0));
    testrun(!ov_stun_attribute_frame_is_message_integrity(buffer, 23));

    // min buffer min valid
    testrun(ov_stun_attribute_frame_is_message_integrity(buffer, 24));

    // length != 24
    testrun(ov_stun_attribute_set_length(buffer, size, 19));
    testrun(!ov_stun_attribute_frame_is_message_integrity(buffer, size));
    testrun(ov_stun_attribute_set_length(buffer, size, 21));
    testrun(!ov_stun_attribute_frame_is_message_integrity(buffer, size));
    testrun(ov_stun_attribute_set_length(buffer, size, 20));
    testrun(ov_stun_attribute_frame_is_message_integrity(buffer, size));

    // type not message integrity
    testrun(ov_stun_attribute_set_type(buffer, size, 0));
    testrun(!ov_stun_attribute_frame_is_message_integrity(buffer, size));
    testrun(ov_stun_attribute_set_type(buffer, size, STUN_MESSAGE_INTEGRITY));
    testrun(ov_stun_attribute_frame_is_message_integrity(buffer, size));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_stun_message_integrity_encoding_length() {

    testrun(24 == ov_stun_message_integrity_encoding_length());
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_stun_add_message_integrity() {

    uint8_t expect[100] = {0};
    uint8_t buffer[100] = {0};

    uint8_t *head = buffer;

    // min valid settings

    uint8_t *ptr = head + 20;
    uint8_t *next = NULL;
    uint8_t *key = (uint8_t *)"test";
    size_t len = 4;
    size_t length = 100;
    size_t l1 = 0;

    testrun(!ov_stun_add_message_integrity(NULL, 0, NULL, NULL, NULL, 0));

    testrun(!ov_stun_add_message_integrity(head, 100, ptr, NULL, key, 0));
    testrun(!ov_stun_add_message_integrity(head, 100, ptr, NULL, NULL, len));
    testrun(!ov_stun_add_message_integrity(head, 100, NULL, NULL, key, len));
    testrun(!ov_stun_add_message_integrity(head, 0, ptr, NULL, key, len));
    testrun(!ov_stun_add_message_integrity(NULL, 100, ptr, NULL, key, len));

    // < min
    testrun(!ov_stun_add_message_integrity(head, 43, ptr, NULL, key, len));

    memset(buffer, 0, 100);
    memset(expect, 0, 100);
    // min
    testrun(ov_stun_add_message_integrity(head, 44, ptr, NULL, key, len));

    // check length is set to attribute length
    testrun(ov_stun_frame_get_length(buffer, length) == 24);

    testrun(20 == ptr - buffer);

    testrun(ov_hmac(OV_HASH_SHA1, head, ptr - head, key, len, expect, &l1));

    testrun(l1 == 20);

    testrun(STUN_MESSAGE_INTEGRITY == ov_stun_attribute_get_type(ptr, 4));

    testrun(20 == ov_stun_attribute_get_length(ptr, 4));
    for (int i = 0; i < 20; i++) {
        testrun(ptr[4 + i] == expect[i]);
    }

    // start not at 32 bit multiple
    ptr++;
    testrun(!ov_stun_add_message_integrity(head, length, ptr, NULL, key, len));

    ptr++;
    testrun(!ov_stun_add_message_integrity(head, length, ptr, NULL, key, len));

    ptr++;
    testrun(!ov_stun_add_message_integrity(head, length, ptr, NULL, key, len));

    ptr++;
    testrun(ov_stun_add_message_integrity(head, length, ptr, NULL, key, len));

    memset(buffer, 0, 100);

    // open 24 bytes
    ptr = buffer + 76;
    testrun(ov_stun_add_message_integrity(buffer, 100, ptr, NULL, key, len));

    // open < 24 bytes
    ptr = buffer + 77;
    testrun(!ov_stun_add_message_integrity(buffer, 100, ptr, &next, key, len));

    // check with some content preset

    buffer[0] = 0x00;  // TYPE
    buffer[1] = 0x00;  // TYPE
    buffer[2] = 0x00;  // LENGTH
    buffer[3] = 0x08;  // LENGTH 8
    buffer[4] = 0x21;  // MAGIC COOKIE
    buffer[5] = 0x12;  // MAGIC COOKIE
    buffer[6] = 0xA4;  // MAGIC COOKIE
    buffer[7] = 0x42;  // MAGIC COOKIE
    buffer[8] = 0xFA;  // ID
    buffer[9] = 0xCE;  // ID
    buffer[10] = 0x11; // ID
    buffer[11] = 0x22; // ID
    buffer[12] = 0x33; // ID
    buffer[13] = 0x44; // ID
    buffer[14] = 0x55; // ID
    buffer[15] = 0x66; // ID
    buffer[16] = 0x77; // ID
    buffer[17] = 0x88; // ID
    buffer[18] = 0x99; // ID
    buffer[19] = 0x00; // ID
    buffer[20] = 0x00; // type    username
    buffer[21] = 0x06; // type    username
    buffer[22] = 0x00; // length  username
    buffer[23] = 0x04; // length  username
    buffer[24] = 'u';  // content username
    buffer[25] = 's';  // content username
    buffer[26] = 'e';  // content username
    buffer[27] = 'r';  // content username

    ptr = buffer + 28;
    testrun(ov_stun_add_message_integrity(buffer, 100, ptr, NULL, key, len));

    testrun(next = ptr + 24);

    // CHECK LENGTH includes the message integrity
    testrun(ov_stun_frame_get_length(buffer, length) == 32);

    // recalculate with new length
    testrun(ov_hmac(OV_HASH_SHA1, buffer, ptr - buffer, key, len, expect, &l1));

    testrun(ptr[0] == 0x00);
    testrun(ptr[1] == 0x08);
    testrun(ptr[2] == 0x00);
    testrun(ptr[3] == 0x14);
    for (int i = 0; i < 20; i++) {
        testrun(ptr[4 + i] == expect[i]);
    }

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_stun_check_message_integrity() {

    uint8_t buffer[100] = {0};
    uint8_t *head = buffer;
    size_t length = 100;

    size_t az = 100;
    uint8_t *arr[az];

    for (size_t i = 0; i < az; i++) {
        arr[i] = NULL;
    }

    // min valid settings

    uint8_t *ptr = head + 20;
    uint8_t *key = (uint8_t *)"test";
    size_t len = strlen((char *)key);

    // prepare HEAD with message integrity set

    buffer[0] = 0x00;  // TYPE
    buffer[1] = 0x00;  // TYPE
    buffer[2] = 0x00;  // LENGTH
    buffer[3] = 0x08;  // LENGTH 8
    buffer[4] = 0x21;  // MAGIC COOKIE
    buffer[5] = 0x12;  // MAGIC COOKIE
    buffer[6] = 0xA4;  // MAGIC COOKIE
    buffer[7] = 0x42;  // MAGIC COOKIE
    buffer[8] = 0xFA;  // ID
    buffer[9] = 0xCE;  // ID
    buffer[10] = 0x11; // ID
    buffer[11] = 0x22; // ID
    buffer[12] = 0x33; // ID
    buffer[13] = 0x44; // ID
    buffer[14] = 0x55; // ID
    buffer[15] = 0x66; // ID
    buffer[16] = 0x77; // ID
    buffer[17] = 0x88; // ID
    buffer[18] = 0x99; // ID
    buffer[19] = 0x00; // ID

    ptr = buffer + 20;
    testrun(ov_stun_add_message_integrity(head, length, ptr, NULL, key, len));

    testrun(ov_stun_frame_slice(head, 44, arr, az));
    testrun(arr[0] != NULL);
    for (size_t i = 1; i < az; i++) {
        testrun(arr[i] == NULL);
    }

    testrun(ov_stun_attribute_frame_is_message_integrity(arr[0], 30));

    testrun(!ov_stun_check_message_integrity(NULL, 0, NULL, 0, NULL, 0, true));
    testrun(!ov_stun_check_message_integrity(NULL, length, arr, az, key, len,
                                             true));
    testrun(!ov_stun_check_message_integrity(head, 0, arr, az, key, len, true));
    testrun(!ov_stun_check_message_integrity(head, length, NULL, az, key, len,
                                             true));
    testrun(
        !ov_stun_check_message_integrity(head, length, arr, 0, key, len, true));
    testrun(!ov_stun_check_message_integrity(head, length, arr, az, NULL, len,
                                             true));
    testrun(
        !ov_stun_check_message_integrity(head, length, arr, az, key, 0, true));

    testrun(ov_stun_check_message_integrity(head, 44, arr, az, key, len, true));

    // different key
    testrun(!ov_stun_check_message_integrity(head, length, arr, az, key,
                                             len - 1, true));

    // different content
    buffer[10] = 0x00;
    testrun(!ov_stun_check_message_integrity(head, length, arr, az, key, len,
                                             true));

    buffer[10] = 0x11;
    testrun(
        ov_stun_check_message_integrity(head, length, arr, az, key, len, true));

    // no message integrity used
    ptr[0] = 0x00; // attribute type username
    ptr[1] = 0x06; // attribute type username

    testrun(!ov_stun_check_message_integrity(head, length, arr, az, key, len,
                                             true));
    testrun(ov_stun_check_message_integrity(head, length, arr, az, key, len,
                                            false));

    ptr[0] = 0x00;
    ptr[1] = 0x08;
    testrun(
        ov_stun_check_message_integrity(head, length, arr, az, key, len, true));

    // integrity length failure
    ptr[2] = 0x20;
    testrun(!ov_stun_check_message_integrity(head, length, arr, az, key, len,
                                             true));
    ptr[2] = 0x00;
    testrun(
        ov_stun_check_message_integrity(head, length, arr, az, key, len, true));

    // attribute behind integrity
    buffer[2] = 0x00; // LENGTH
    buffer[3] = 32;   // LENGTH 32
    length = 52;
    buffer[44] = 0x00; // type    username
    buffer[45] = 0x06; // type    username
    buffer[46] = 0x00; // length  username
    buffer[47] = 0x04; // length  username
    buffer[48] = 'u';  // content username
    buffer[49] = 's';  // content username
    buffer[50] = 'e';  // content username
    buffer[51] = 'r';  // content username

    // reslice
    testrun(ov_stun_frame_slice(buffer, length, arr, az));
    testrun(arr[0] != NULL);
    testrun(arr[1] != NULL);
    for (size_t i = 2; i < az; i++) {
        testrun(arr[i] == NULL);
    }

    testrun(
        ov_stun_check_message_integrity(head, length, arr, az, key, len, true));
    // check username will be ignored
    testrun(arr[0] != NULL);
    testrun(arr[1] == NULL);

    // fingerprint behind integrity
    buffer[2] = 0x00; // LENGTH
    buffer[3] = 32;   // LENGTH 32
    length = 52;
    buffer[44] = 0x80; // type    fingerprint
    buffer[45] = 0x28; // type    fingerprint
    buffer[46] = 0x00; // length  fingerprint
    buffer[47] = 0x04; // length  fingerprint
    buffer[48] = 'c';  // dummy content fingerprint
    buffer[49] = 'r';  // dummy content fingerprint
    buffer[50] = 'c';  // dummy content fingerprint
    buffer[51] = ' ';  // dummy content fingerprint

    // reslice
    testrun(ov_stun_frame_slice(buffer, length, arr, az));
    testrun(arr[0] != NULL);
    testrun(arr[1] != NULL);
    for (size_t i = 2; i < az; i++) {
        testrun(arr[i] == NULL);
    }
    testrun(
        ov_stun_check_message_integrity(head, length, arr, az, key, len, true));
    // check fingerprint will not be ignored
    testrun(arr[0] != NULL);
    testrun(arr[1] != NULL);

    // some attribute behind finger print
    buffer[2] = 0x00; // LENGTH
    buffer[3] = 40;   // LENGTH 40
    length = 60;
    buffer[44] = 0x80; // type    fingerprint
    buffer[45] = 0x28; // type    fingerprint
    buffer[46] = 0x00; // length  fingerprint
    buffer[47] = 0x04; // length  fingerprint
    buffer[48] = 'c';  // dummy content fingerprint
    buffer[49] = 'r';  // dummy content fingerprint
    buffer[50] = 'c';  // dummy content fingerprint
    buffer[51] = ' ';  // dummy content fingerprint
    buffer[52] = 0x00; // type    username
    buffer[53] = 0x06; // type    username
    buffer[54] = 0x00; // length  username
    buffer[55] = 0x04; // length  username
    buffer[56] = 'u';  // content username
    buffer[57] = 's';  // content username
    buffer[58] = 'e';  // content username
    buffer[59] = 'r';  // content username

    // reslice
    testrun(ov_stun_frame_slice(buffer, length, arr, az));
    testrun(arr[0] != NULL);
    testrun(arr[1] != NULL);
    testrun(arr[2] != NULL);
    for (size_t i = 3; i < az; i++) {
        testrun(arr[i] == NULL);
    }
    testrun(
        ov_stun_check_message_integrity(head, length, arr, az, key, len, true));
    // check fingerprint will not be ignored, others will
    testrun(arr[0] != NULL);
    testrun(arr[1] != NULL);
    testrun(arr[2] == NULL);

    // reslice
    testrun(ov_stun_frame_slice(buffer, length, arr, az));
    testrun(
        ov_stun_check_message_integrity(head, length, arr, 1, key, len, true));
    // will ignore any pointer behind 1
    testrun(arr[0] != NULL);
    testrun(arr[1] != NULL);
    testrun(arr[2] != NULL);
    for (size_t i = 3; i < az; i++) {
        testrun(arr[i] == NULL);
    }

    // fingerprint behind some other attribute
    buffer[2] = 0x00; // LENGTH
    buffer[3] = 40;   // LENGTH 40
    length = 60;
    buffer[44] = 0x00; // type    username
    buffer[45] = 0x06; // type    username
    buffer[46] = 0x00; // length  username
    buffer[47] = 0x04; // length  username
    buffer[48] = 'x';  // content username
    buffer[49] = 'x';  // content username
    buffer[50] = 'x';  // content username
    buffer[51] = 'x';  // content username
    buffer[52] = 0x80; // type    fingerprint
    buffer[53] = 0x28; // type    fingerprint
    buffer[54] = 0x00; // length  fingerprint
    buffer[55] = 0x04; // length  fingerprint
    buffer[56] = 'c';  // content fingerprint
    buffer[57] = 'c';  // content fingerprint
    buffer[58] = 'c';  // content fingerprint
    buffer[59] = 'c';  // content fingerprint

    // reslice
    testrun(ov_stun_frame_slice(buffer, length, arr, az));
    testrun(arr[0] != NULL);
    testrun(arr[1] != NULL);
    testrun(arr[2] != NULL);
    for (size_t i = 3; i < az; i++) {
        testrun(arr[i] == NULL);
    }
    testrun(
        ov_stun_check_message_integrity(head, length, arr, az, key, len, true));
    // check fingerprint will not be ignored, others will
    testrun(arr[0] != NULL);
    testrun(arr[1] == NULL);
    testrun(arr[2] == NULL);

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
    testrun_test(test_ov_stun_attribute_frame_is_message_integrity);
    testrun_test(test_ov_stun_message_integrity_encoding_length);

    testrun_test(test_ov_stun_add_message_integrity);
    testrun_test(test_ov_stun_check_message_integrity);

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
