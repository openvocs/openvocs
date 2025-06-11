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
        @file           ov_stun_fingerprint_test.c
        @author         Markus Toepfer

        @date           2018-10-22

        @ingroup        ov_stun

        @brief          Unit tests of RFC 5389 attribute "fingerprint"


        ------------------------------------------------------------------------
*/

#include "ov_stun_fingerprint.c"
#include <ov_test/testrun.h>

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

int test_ov_stun_attribute_frame_is_fingerprint() {

    size_t size = 1000;
    uint8_t buf[size];
    uint8_t *buffer = buf;

    memset(buf, 'a', size);

    // prepare valid frame
    testrun(ov_stun_attribute_set_type(buffer, size, STUN_FINGERPRINT));
    testrun(ov_stun_attribute_set_length(buffer, size, 4));

    testrun(ov_stun_attribute_frame_is_fingerprint(buffer, size));

    testrun(!ov_stun_attribute_frame_is_fingerprint(NULL, size));
    testrun(!ov_stun_attribute_frame_is_fingerprint(buffer, 0));
    testrun(!ov_stun_attribute_frame_is_fingerprint(buffer, 7));

    // min buffer min valid
    testrun(ov_stun_attribute_frame_is_fingerprint(buffer, 8));

    // length != 8
    testrun(ov_stun_attribute_set_length(buffer, size, 3));
    testrun(!ov_stun_attribute_frame_is_fingerprint(buffer, size));
    testrun(ov_stun_attribute_set_length(buffer, size, 5));
    testrun(!ov_stun_attribute_frame_is_fingerprint(buffer, size));
    testrun(ov_stun_attribute_set_length(buffer, size, 4));
    testrun(ov_stun_attribute_frame_is_fingerprint(buffer, size));

    // type not fingerprint
    testrun(ov_stun_attribute_set_type(buffer, size, 0));
    testrun(!ov_stun_attribute_frame_is_fingerprint(buffer, size));
    testrun(ov_stun_attribute_set_type(buffer, size, STUN_FINGERPRINT));
    testrun(ov_stun_attribute_frame_is_fingerprint(buffer, size));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_stun_fingerprint_encoding_length() {

    testrun(8 == ov_stun_fingerprint_encoding_length());
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_stun_add_fingerprint() {

    uint8_t expect[100] = {0};
    uint8_t buffer[100] = {0};

    uint8_t *head = buffer;

    // min valid settings

    uint8_t *ptr = head + 20;
    uint8_t *next = NULL;
    size_t length = 100;
    uint32_t crc = 0;

    testrun(!ov_stun_add_fingerprint(NULL, 0, NULL, NULL));
    testrun(!ov_stun_add_fingerprint(head, 0, ptr, NULL));
    testrun(!ov_stun_add_fingerprint(head, 100, NULL, NULL));
    testrun(!ov_stun_add_fingerprint(NULL, 100, ptr, NULL));

    // < min
    testrun(!ov_stun_add_fingerprint(head, 27, ptr, NULL));

    memset(buffer, 0, 100);
    memset(expect, 0, 100);
    // min
    testrun(ov_stun_add_fingerprint(head, 28, ptr, NULL));

    // check length is set to attribute length
    testrun(ov_stun_frame_get_length(buffer, length) == 8);

    crc = ov_crc32_zlib(0, head, (ptr - head));
    crc = crc ^ 0x5354554e;

    testrun(ptr[0] == 0x80);
    testrun(ptr[1] == 0x28);
    testrun(ptr[2] == 0x00);
    testrun(ptr[3] == 0x04);
    testrun(crc == htonl(*(uint32_t *)(ptr + 4)));

    // ptr - head < 20
    testrun(!ov_stun_add_fingerprint(head, length, head + 16, NULL));
    testrun(!ov_stun_add_fingerprint(head, length, head + 12, NULL));
    testrun(!ov_stun_add_fingerprint(head, length, head + 8, NULL));
    testrun(!ov_stun_add_fingerprint(head, length, head + 4, NULL));

    // ptr - stun->head == 20;
    testrun(ov_stun_add_fingerprint(head, length, head + 20, &next));
    testrun(ptr[0] == 0x80);
    testrun(ptr[1] == 0x28);
    testrun(ptr[2] == 0x00);
    testrun(ptr[3] == 0x04);
    testrun(crc == htonl(*(uint32_t *)(ptr + 4)));
    testrun(next == head + 28);

    // start not at 32 bit multiple
    testrun(!ov_stun_add_fingerprint(head, length, head + 21, &next));
    testrun(!ov_stun_add_fingerprint(head, length, head + 22, &next));
    testrun(!ov_stun_add_fingerprint(head, length, head + 23, &next));

    testrun(!ov_stun_add_fingerprint(head, length, head + 25, &next));
    testrun(!ov_stun_add_fingerprint(head, length, head + 26, &next));
    testrun(!ov_stun_add_fingerprint(head, length, head + 27, &next));

    memset(buffer, 0, 100);

    // ptr - stun->head > 20;
    ptr = head + 24;
    testrun(ov_stun_add_fingerprint(head, length, ptr, &next));
    testrun(ptr[0] == 0x80);
    testrun(ptr[1] == 0x28);
    testrun(ptr[2] == 0x00);
    testrun(ptr[3] == 0x04);

    // recalculate with new length
    crc = ov_crc32_zlib(0, head, (ptr - head));
    crc = crc ^ 0x5354554e;
    testrun(crc == htonl(*(uint32_t *)(ptr + 4)));

    // open 8 bytes
    testrun(ov_stun_add_fingerprint(head, length, head + length - 8, &next));
    // open less than 8 bytes
    testrun(!ov_stun_add_fingerprint(head, length, head + length - 7, &next));

    // check with some content preset
    memset(buffer, 0, 100);

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
    testrun(ov_stun_add_fingerprint(head, length, ptr, &next));
    testrun(ptr[0] == 0x80);
    testrun(ptr[1] == 0x28);
    testrun(ptr[2] == 0x00);
    testrun(ptr[3] == 0x04);

    // recalculate with new length
    crc = ov_crc32_zlib(0, head, (ptr - head));
    crc = crc ^ 0x5354554e;
    testrun(crc == htonl(*(uint32_t *)(ptr + 4)));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_stun_check_fingerprint() {

    uint8_t buffer[100] = {0};
    uint8_t *head = buffer;
    size_t length = 100;

    size_t az = 100;
    uint8_t *attr[az];

    for (size_t i = 0; i < az; i++) {
        attr[i] = NULL;
    }

    // min valid settings

    uint8_t *ptr = head + 20;
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

    ptr = head + 20;
    testrun(ov_stun_add_fingerprint(head, length, ptr, NULL));

    testrun(ov_stun_frame_slice(head, 28, attr, az));
    testrun(attr[0] != NULL);
    for (size_t i = 1; i < az; i++) {
        testrun(attr[i] == NULL);
    }

    testrun(ov_stun_attribute_frame_is_fingerprint(attr[0], 8));

    testrun(!ov_stun_check_fingerprint(NULL, 0, NULL, 0, true));
    testrun(!ov_stun_check_fingerprint(NULL, 0, attr, az, true));
    testrun(!ov_stun_check_fingerprint(head, length, attr, 0, true));
    testrun(!ov_stun_check_fingerprint(head, length, NULL, az, true));

    testrun(ov_stun_check_fingerprint(head, 28, attr, az, true));

    // different content
    buffer[10] = 0x00;
    testrun(!ov_stun_check_fingerprint(head, 28, attr, az, true));

    buffer[10] = 0x11;
    testrun(ov_stun_check_fingerprint(head, 28, attr, az, true));

    // no fingerprint used
    ptr[0] = 0x00; // attribute type username
    ptr[1] = 0x06; // attribute type username

    testrun(!ov_stun_check_fingerprint(head, 28, attr, az, true));
    testrun(ov_stun_check_fingerprint(head, 28, attr, az, false));

    ptr[0] = 0x80;
    ptr[1] = 0x28;
    testrun(ov_stun_check_fingerprint(head, 28, attr, az, true));

    // fingerprint length failure
    ptr[2] = 0x20;
    testrun(!ov_stun_check_fingerprint(head, 28, attr, az, true));
    ptr[2] = 0x00;
    testrun(ov_stun_check_fingerprint(head, 28, attr, az, true));

    // attribute behind fingerprint
    buffer[2] = 0x00; // LENGTH
    buffer[3] = 16;   // LENGTH
    length = 36;
    buffer[28] = 0x00; // type    username
    buffer[29] = 0x06; // type    username
    buffer[30] = 0x00; // length  username
    buffer[31] = 0x04; // length  username
    buffer[32] = 'u';  // content username
    buffer[33] = 's';  // content username
    buffer[34] = 'e';  // content username
    buffer[35] = 'r';  // content username

    // reslice
    testrun(ov_stun_frame_slice(head, length, attr, az));
    testrun(attr[0] != NULL);
    testrun(attr[1] != NULL);
    for (size_t i = 2; i < az; i++) {
        testrun(attr[i] == NULL);
    }

    testrun(ov_stun_check_fingerprint(head, 28, attr, az, true));
    // check username will be ignored
    testrun(attr[0] != NULL);
    testrun(attr[1] == NULL);
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
    testrun_test(test_ov_stun_attribute_frame_is_fingerprint);
    testrun_test(test_ov_stun_fingerprint_encoding_length);
    testrun_test(test_ov_stun_add_fingerprint);
    testrun_test(test_ov_stun_check_fingerprint);

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
