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
        @file           ov_utf8_tests.c
        @author         Markus Toepfer

        @date           2017-12-17

        @ingroup        ov_utf8

        @brief


        ------------------------------------------------------------------------
*/

#include "ov_utf8.c"
#include <ov_test/testrun.h>

/*
        Switch ON or OFF extendet testing. In this case testing of the
        creation of all possible unicode codepoints.

        This test runs slow in valgrind. Therefore this tag is included to
        switch off the really long running parts during test development.

        Valgrind RUN
        testrun        ./build/test/unit/ov_utf8_tests.test
        [OK]    test_ov_utf8_last_valid
        [OK]    test_ov_utf8_validate_sequence
        [OK]    testall_ov_utf8_validate_sequence
        [OK]    test_ov_utf8_encode_code_point
        [OK]    test_ov_utf8_decode_code_point
        ALL TESTS RUN (5 tests)
        Clock ticks function: ( main ) | 46.551532 s | 46552 ms

        Same machine running without valgrind
        testrun ./build/test/unit/ov_utf8_tests.test
        [OK]    test_ov_utf8_last_valid
        [OK]    test_ov_utf8_validate_sequence
        [OK]    testall_ov_utf8_validate_sequence
        [OK]    test_ov_utf8_encode_code_point
        [OK]    test_ov_utf8_decode_code_point
        ALL TESTS RUN (5 tests)
        Clock ticks function: ( main ) | 1.743846 s | 1744 ms

        The long running parts make heavy use of the CPU to calculate
        UTF content from uint and back.

*/

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST HELPER                                                     #HELPER
 *
 *      ------------------------------------------------------------------------
 */

/*----------------------------------------------------------------------------*/

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

/*----------------------------------------------------------------------------*/

int test_ov_utf8_last_valid() {

    size_t length = 100;
    uint8_t *buffer = calloc(length, sizeof(uint8_t));
    uint8_t *result = NULL;

    testrun(!ov_utf8_last_valid(NULL, 0, true));
    testrun(!ov_utf8_last_valid(buffer, 0, true));
    testrun(!ov_utf8_last_valid(NULL, length, true));

    result = ov_utf8_last_valid(buffer, 10, true);
    testrun(result == buffer + 9);
    testrun(result[0] == 0);
    result = ov_utf8_last_valid(buffer, 10, false);
    testrun(result == buffer + 10);
    testrun(result[0] == 0);

    buffer[5] = 0xff;
    result = ov_utf8_last_valid(buffer, 10, true);
    testrun(result == buffer + 4);
    testrun(result[0] == 0);
    result = ov_utf8_last_valid(buffer, 10, false);
    testrun(result == buffer + 5);
    testrun(result[0] == 0xff);

    buffer[5] = 0xC2;
    buffer[6] = 0xBF;

    result = ov_utf8_last_valid(buffer, 10, true);
    testrun(result == buffer + 9);
    testrun(result[0] == 0);
    result = ov_utf8_last_valid(buffer, 10, false);
    testrun(result == buffer + 10);
    testrun(result[0] == 0x00);

    buffer[6] = 0xFF;
    result = ov_utf8_last_valid(buffer, 10, true);
    testrun(result == buffer + 4);
    testrun(result[0] == 0);
    result = ov_utf8_last_valid(buffer, 10, false);
    testrun(result == buffer + 5);
    testrun(result[0] == 0xC2);

    buffer[5] = 0xEf;
    buffer[6] = 0xBB;
    buffer[7] = 0xBF;

    result = ov_utf8_last_valid(buffer, 10, true);
    testrun(result == buffer + 9);
    testrun(result[0] == 0);
    result = ov_utf8_last_valid(buffer, 10, false);
    testrun(result == buffer + 10);
    testrun(result[0] == 0x00);

    buffer[6] = 0xFF;
    result = ov_utf8_last_valid(buffer, 10, true);
    testrun(result == buffer + 4);
    testrun(result[0] == 0);
    result = ov_utf8_last_valid(buffer, 10, false);
    testrun(result == buffer + 5);
    testrun(result[0] == 0xEF);

    buffer[5] = 0xF0;
    buffer[6] = 0xA3;
    buffer[7] = 0x8E;
    buffer[8] = 0xB4;

    result = ov_utf8_last_valid(buffer, 10, true);
    testrun(result == buffer + 9);
    testrun(result[0] == 0);
    result = ov_utf8_last_valid(buffer, 10, false);
    testrun(result == buffer + 10);
    testrun(result[0] == 0x00);

    buffer[6] = 0xFF;
    result = ov_utf8_last_valid(buffer, 10, true);
    testrun(result == buffer + 4);
    testrun(result[0] == 0);
    result = ov_utf8_last_valid(buffer, 10, false);
    testrun(result == buffer + 5);
    testrun(result[0] == 0xF0);

    buffer[5] = 0xFF;
    buffer[6] = 0xA3;
    buffer[7] = 0x8E;
    buffer[8] = 0xB4;

    result = ov_utf8_last_valid(buffer, 10, true);
    testrun(result == buffer + 4);
    testrun(result[0] == 0);
    result = ov_utf8_last_valid(buffer, 10, false);
    testrun(result == buffer + 5);
    testrun(result[0] == 0xFF);

    buffer[5] = 0xF0;
    buffer[6] = 0xFF;
    buffer[7] = 0x8E;
    buffer[8] = 0xB4;

    result = ov_utf8_last_valid(buffer, 10, true);
    testrun(result == buffer + 4);
    testrun(result[0] == 0);
    result = ov_utf8_last_valid(buffer, 10, false);
    testrun(result == buffer + 5);
    testrun(result[0] == 0xF0);

    buffer[5] = 0xF0;
    buffer[6] = 0xA3;
    buffer[7] = 0xFF;
    buffer[8] = 0xB4;

    result = ov_utf8_last_valid(buffer, 10, true);
    testrun(result == buffer + 4);
    testrun(result[0] == 0);
    result = ov_utf8_last_valid(buffer, 10, false);
    testrun(result == buffer + 5);
    testrun(result[0] == 0xF0);

    buffer[5] = 0xF0;
    buffer[6] = 0xA3;
    buffer[7] = 0x8E;
    buffer[8] = 0xFF;

    result = ov_utf8_last_valid(buffer, 10, true);
    testrun(result == buffer + 4);
    testrun(result[0] == 0);
    result = ov_utf8_last_valid(buffer, 10, false);
    testrun(result == buffer + 5);
    testrun(result[0] == 0xF0);

    buffer[0] = 0x00;
    buffer[1] = 0xC2;
    buffer[2] = 0xBF;
    buffer[3] = 0x00;
    buffer[4] = 0xEf;
    buffer[5] = 0xBB;
    buffer[6] = 0xBF;
    buffer[7] = 0x00;
    buffer[8] = 0xF0;
    buffer[9] = 0xA3;
    buffer[10] = 0x8E;
    buffer[11] = 0xB4;
    buffer[12] = 0x00;
    buffer[13] = 0xF0;
    buffer[14] = 0xA3;
    buffer[15] = 0x8E;
    buffer[16] = 0xB4;
    buffer[17] = 0xC2;
    buffer[18] = 0xBF;
    buffer[19] = 0xEf;
    buffer[20] = 0xBB;
    buffer[21] = 0xBF;
    buffer[22] = 0xC2;
    buffer[23] = 0xBF;
    buffer[24] = 0x00;

    result = ov_utf8_last_valid(buffer, 25, true);
    testrun(result == buffer + 24);
    testrun(result[0] == 0);
    result = ov_utf8_last_valid(buffer, 25, false);
    testrun(result == buffer + 25);
    testrun(result[0] == 0x00);

    buffer[0] = 0xFF;
    buffer[1] = 0xC2;
    buffer[2] = 0xBF;
    buffer[3] = 0x00;
    buffer[4] = 0xEf;
    buffer[5] = 0xBB;
    buffer[6] = 0xBF;
    buffer[7] = 0x00;
    buffer[8] = 0xF0;
    buffer[9] = 0xA3;
    buffer[10] = 0x8E;
    buffer[11] = 0xB4;
    buffer[12] = 0x00;
    buffer[13] = 0xF0;
    buffer[14] = 0xA3;
    buffer[15] = 0x8E;
    buffer[16] = 0xB4;
    buffer[17] = 0xC2;
    buffer[18] = 0xBF;
    buffer[19] = 0xEf;
    buffer[20] = 0xBB;
    buffer[21] = 0xBF;
    buffer[22] = 0xC2;
    buffer[23] = 0xBF;
    buffer[24] = 0x00;

    result = ov_utf8_last_valid(buffer, 25, true);
    testrun(result == NULL);
    result = ov_utf8_last_valid(buffer, 25, false);
    testrun(result == buffer);

    buffer[0] = 0x00;
    buffer[1] = 0xFF;
    buffer[2] = 0xBF;
    buffer[3] = 0x00;
    buffer[4] = 0xEf;
    buffer[5] = 0xBB;
    buffer[6] = 0xBF;
    buffer[7] = 0x00;
    buffer[8] = 0xF0;
    buffer[9] = 0xA3;
    buffer[10] = 0x8E;
    buffer[11] = 0xB4;
    buffer[12] = 0x00;
    buffer[13] = 0xF0;
    buffer[14] = 0xA3;
    buffer[15] = 0x8E;
    buffer[16] = 0xB4;
    buffer[17] = 0xC2;
    buffer[18] = 0xBF;
    buffer[19] = 0xEf;
    buffer[20] = 0xBB;
    buffer[21] = 0xBF;
    buffer[22] = 0xC2;
    buffer[23] = 0xBF;
    buffer[24] = 0x00;

    result = ov_utf8_last_valid(buffer, 25, true);
    testrun(result == buffer);
    testrun(result[0] == 0);
    result = ov_utf8_last_valid(buffer, 25, false);
    testrun(result == buffer + 1);
    testrun(result[0] == 0xFF);

    buffer[0] = 0x00;
    buffer[1] = 0xC2;
    buffer[2] = 0xFF;
    buffer[3] = 0x00;
    buffer[4] = 0xEf;
    buffer[5] = 0xBB;
    buffer[6] = 0xBF;
    buffer[7] = 0x00;
    buffer[8] = 0xF0;
    buffer[9] = 0xA3;
    buffer[10] = 0x8E;
    buffer[11] = 0xB4;
    buffer[12] = 0x00;
    buffer[13] = 0xF0;
    buffer[14] = 0xA3;
    buffer[15] = 0x8E;
    buffer[16] = 0xB4;
    buffer[17] = 0xC2;
    buffer[18] = 0xBF;
    buffer[19] = 0xEf;
    buffer[20] = 0xBB;
    buffer[21] = 0xBF;
    buffer[22] = 0xC2;
    buffer[23] = 0xBF;
    buffer[24] = 0x00;

    result = ov_utf8_last_valid(buffer, 25, true);
    testrun(result == buffer);
    testrun(result[0] == 0);
    result = ov_utf8_last_valid(buffer, 25, false);
    testrun(result == buffer + 1);
    testrun(result[0] == 0xC2);

    buffer[0] = 0x00;
    buffer[1] = 0xC2;
    buffer[2] = 0xBF;
    buffer[3] = 0xFF;
    buffer[4] = 0xEf;
    buffer[5] = 0xBB;
    buffer[6] = 0xBF;
    buffer[7] = 0x00;
    buffer[8] = 0xF0;
    buffer[9] = 0xA3;
    buffer[10] = 0x8E;
    buffer[11] = 0xB4;
    buffer[12] = 0x00;
    buffer[13] = 0xF0;
    buffer[14] = 0xA3;
    buffer[15] = 0x8E;
    buffer[16] = 0xB4;
    buffer[17] = 0xC2;
    buffer[18] = 0xBF;
    buffer[19] = 0xEf;
    buffer[20] = 0xBB;
    buffer[21] = 0xBF;
    buffer[22] = 0xC2;
    buffer[23] = 0xBF;
    buffer[24] = 0x00;

    result = ov_utf8_last_valid(buffer, 25, true);
    testrun(result == buffer + 1);
    testrun(result[0] == 0xC2);
    result = ov_utf8_last_valid(buffer, 25, false);
    testrun(result == buffer + 3);
    testrun(result[0] == 0xFF);

    buffer[0] = 0x00;
    buffer[1] = 0xC2;
    buffer[2] = 0xBF;
    buffer[3] = 0x00;
    buffer[4] = 0xFF;
    buffer[5] = 0xBB;
    buffer[6] = 0xBF;
    buffer[7] = 0x00;
    buffer[8] = 0xF0;
    buffer[9] = 0xA3;
    buffer[10] = 0x8E;
    buffer[11] = 0xB4;
    buffer[12] = 0x00;
    buffer[13] = 0xF0;
    buffer[14] = 0xA3;
    buffer[15] = 0x8E;
    buffer[16] = 0xB4;
    buffer[17] = 0xC2;
    buffer[18] = 0xBF;
    buffer[19] = 0xEf;
    buffer[20] = 0xBB;
    buffer[21] = 0xBF;
    buffer[22] = 0xC2;
    buffer[23] = 0xBF;
    buffer[24] = 0x00;

    result = ov_utf8_last_valid(buffer, 25, true);
    testrun(result == buffer + 3);
    testrun(result[0] == 0);
    result = ov_utf8_last_valid(buffer, 25, false);
    testrun(result == buffer + 4);
    testrun(result[0] == 0xFF);

    buffer[0] = 0x00;
    buffer[1] = 0xC2;
    buffer[2] = 0xBF;
    buffer[3] = 0x00;
    buffer[4] = 0xEf;
    buffer[5] = 0xFF;
    buffer[6] = 0xBF;
    buffer[7] = 0x00;
    buffer[8] = 0xF0;
    buffer[9] = 0xA3;
    buffer[10] = 0x8E;
    buffer[11] = 0xB4;
    buffer[12] = 0x00;
    buffer[13] = 0xF0;
    buffer[14] = 0xA3;
    buffer[15] = 0x8E;
    buffer[16] = 0xB4;
    buffer[17] = 0xC2;
    buffer[18] = 0xBF;
    buffer[19] = 0xEf;
    buffer[20] = 0xBB;
    buffer[21] = 0xBF;
    buffer[22] = 0xC2;
    buffer[23] = 0xBF;
    buffer[24] = 0x00;

    result = ov_utf8_last_valid(buffer, 25, true);
    testrun(result == buffer + 3);
    testrun(result[0] == 0);
    result = ov_utf8_last_valid(buffer, 25, false);
    testrun(result == buffer + 4);
    testrun(result[0] == 0xEF);

    buffer[0] = 0x00;
    buffer[1] = 0xC2;
    buffer[2] = 0xBF;
    buffer[3] = 0x00;
    buffer[4] = 0xEf;
    buffer[5] = 0xBB;
    buffer[6] = 0xFF;
    buffer[7] = 0x00;
    buffer[8] = 0xF0;
    buffer[9] = 0xA3;
    buffer[10] = 0x8E;
    buffer[11] = 0xB4;
    buffer[12] = 0x00;
    buffer[13] = 0xF0;
    buffer[14] = 0xA3;
    buffer[15] = 0x8E;
    buffer[16] = 0xB4;
    buffer[17] = 0xC2;
    buffer[18] = 0xBF;
    buffer[19] = 0xEf;
    buffer[20] = 0xBB;
    buffer[21] = 0xBF;
    buffer[22] = 0xC2;
    buffer[23] = 0xBF;
    buffer[24] = 0x00;

    result = ov_utf8_last_valid(buffer, 25, true);
    testrun(result == buffer + 3);
    testrun(result[0] == 0);
    result = ov_utf8_last_valid(buffer, 25, false);
    testrun(result == buffer + 4);
    testrun(result[0] == 0xEF);

    buffer[0] = 0x00;
    buffer[1] = 0xC2;
    buffer[2] = 0xBF;
    buffer[3] = 0x00;
    buffer[4] = 0xEf;
    buffer[5] = 0xBB;
    buffer[6] = 0xBF;
    buffer[7] = 0xff;
    buffer[8] = 0xF0;
    buffer[9] = 0xA3;
    buffer[10] = 0x8E;
    buffer[11] = 0xB4;
    buffer[12] = 0x00;
    buffer[13] = 0xF0;
    buffer[14] = 0xA3;
    buffer[15] = 0x8E;
    buffer[16] = 0xB4;
    buffer[17] = 0xC2;
    buffer[18] = 0xBF;
    buffer[19] = 0xEf;
    buffer[20] = 0xBB;
    buffer[21] = 0xBF;
    buffer[22] = 0xC2;
    buffer[23] = 0xBF;
    buffer[24] = 0x00;

    result = ov_utf8_last_valid(buffer, 25, true);
    testrun(result == buffer + 4);
    testrun(result[0] == 0xEF);
    result = ov_utf8_last_valid(buffer, 25, false);
    testrun(result == buffer + 7);
    testrun(result[0] == 0xFF);

    buffer[0] = 0x00;
    buffer[1] = 0xC2;
    buffer[2] = 0xBF;
    buffer[3] = 0x00;
    buffer[4] = 0xEf;
    buffer[5] = 0xBB;
    buffer[6] = 0xBF;
    buffer[7] = 0x00;
    buffer[8] = 0xff;
    buffer[9] = 0xA3;
    buffer[10] = 0x8E;
    buffer[11] = 0xB4;
    buffer[12] = 0x00;
    buffer[13] = 0xF0;
    buffer[14] = 0xA3;
    buffer[15] = 0x8E;
    buffer[16] = 0xB4;
    buffer[17] = 0xC2;
    buffer[18] = 0xBF;
    buffer[19] = 0xEf;
    buffer[20] = 0xBB;
    buffer[21] = 0xBF;
    buffer[22] = 0xC2;
    buffer[23] = 0xBF;
    buffer[24] = 0x00;

    result = ov_utf8_last_valid(buffer, 25, true);
    testrun(result == buffer + 7);
    testrun(result[0] == 0);
    result = ov_utf8_last_valid(buffer, 25, false);
    testrun(result == buffer + 8);
    testrun(result[0] == 0xff);

    buffer[0] = 0x00;
    buffer[1] = 0xC2;
    buffer[2] = 0xBF;
    buffer[3] = 0x00;
    buffer[4] = 0xEf;
    buffer[5] = 0xBB;
    buffer[6] = 0xBF;
    buffer[7] = 0x00;
    buffer[8] = 0xF0;
    buffer[9] = 0xFF;
    buffer[10] = 0x8E;
    buffer[11] = 0xB4;
    buffer[12] = 0x00;
    buffer[13] = 0xF0;
    buffer[14] = 0xA3;
    buffer[15] = 0x8E;
    buffer[16] = 0xB4;
    buffer[17] = 0xC2;
    buffer[18] = 0xBF;
    buffer[19] = 0xEf;
    buffer[20] = 0xBB;
    buffer[21] = 0xBF;
    buffer[22] = 0xC2;
    buffer[23] = 0xBF;
    buffer[24] = 0x00;

    result = ov_utf8_last_valid(buffer, 25, true);
    testrun(result == buffer + 7);
    testrun(result[0] == 0);
    result = ov_utf8_last_valid(buffer, 25, false);
    testrun(result == buffer + 8);
    testrun(result[0] == 0xF0);

    buffer[0] = 0x00;
    buffer[1] = 0xC2;
    buffer[2] = 0xBF;
    buffer[3] = 0x00;
    buffer[4] = 0xEf;
    buffer[5] = 0xBB;
    buffer[6] = 0xBF;
    buffer[7] = 0x00;
    buffer[8] = 0xF0;
    buffer[9] = 0xA3;
    buffer[10] = 0x8E;
    buffer[11] = 0xB4;
    buffer[12] = 0x00;
    buffer[13] = 0xF0;
    buffer[14] = 0xA3;
    buffer[15] = 0xff;
    buffer[16] = 0xB4;
    buffer[17] = 0xC2;
    buffer[18] = 0xBF;
    buffer[19] = 0xEf;
    buffer[20] = 0xBB;
    buffer[21] = 0xBF;
    buffer[22] = 0xC2;
    buffer[23] = 0xBF;
    buffer[24] = 0x00;

    result = ov_utf8_last_valid(buffer, 25, true);
    testrun(result == buffer + 12);
    testrun(result[0] == 0);
    result = ov_utf8_last_valid(buffer, 25, false);
    testrun(result == buffer + 13);
    testrun(result[0] == 0xF0);

    buffer[0] = 0x00;
    buffer[1] = 0xC2;
    buffer[2] = 0xBF;
    buffer[3] = 0x00;
    buffer[4] = 0xEf;
    buffer[5] = 0xBB;
    buffer[6] = 0xBF;
    buffer[7] = 0x00;
    buffer[8] = 0xF0;
    buffer[9] = 0xA3;
    buffer[10] = 0x8E;
    buffer[11] = 0xB4;
    buffer[12] = 0x00;
    buffer[13] = 0xF0;
    buffer[14] = 0xA3;
    buffer[15] = 0x8E;
    buffer[16] = 0xff;
    buffer[17] = 0xC2;
    buffer[18] = 0xBF;
    buffer[19] = 0xEf;
    buffer[20] = 0xBB;
    buffer[21] = 0xBF;
    buffer[22] = 0xC2;
    buffer[23] = 0xBF;
    buffer[24] = 0x00;

    result = ov_utf8_last_valid(buffer, 25, true);
    testrun(result == buffer + 12);
    testrun(result[0] == 0);
    result = ov_utf8_last_valid(buffer, 25, false);
    testrun(result == buffer + 13);
    testrun(result[0] == 0xF0);

    buffer[0] = 0x00;
    buffer[1] = 0xC2;
    buffer[2] = 0xBF;
    buffer[3] = 0x00;
    buffer[4] = 0xEf;
    buffer[5] = 0xBB;
    buffer[6] = 0xBF;
    buffer[7] = 0x00;
    buffer[8] = 0xF0;
    buffer[9] = 0xA3;
    buffer[10] = 0x8E;
    buffer[11] = 0xB4;
    buffer[12] = 0x00;
    buffer[13] = 0xF0;
    buffer[14] = 0xA3;
    buffer[15] = 0x8E;
    buffer[16] = 0xB4;
    buffer[17] = 0xff;
    buffer[18] = 0xBF;
    buffer[19] = 0xEf;
    buffer[20] = 0xBB;
    buffer[21] = 0xBF;
    buffer[22] = 0xC2;
    buffer[23] = 0xBF;
    buffer[24] = 0x00;

    result = ov_utf8_last_valid(buffer, 25, true);
    testrun(result == buffer + 13);
    testrun(result[0] == 0xF0);
    result = ov_utf8_last_valid(buffer, 25, false);
    testrun(result == buffer + 17);
    testrun(result[0] == 0xff);

    buffer[0] = 0x00;
    buffer[1] = 0xC2;
    buffer[2] = 0xBF;
    buffer[3] = 0x00;
    buffer[4] = 0xEf;
    buffer[5] = 0xBB;
    buffer[6] = 0xBF;
    buffer[7] = 0x00;
    buffer[8] = 0xF0;
    buffer[9] = 0xA3;
    buffer[10] = 0x8E;
    buffer[11] = 0xB4;
    buffer[12] = 0x00;
    buffer[13] = 0xF0;
    buffer[14] = 0xA3;
    buffer[15] = 0x8E;
    buffer[16] = 0xB4;
    buffer[17] = 0xC2;
    buffer[18] = 0xff;
    buffer[19] = 0xEf;
    buffer[20] = 0xBB;
    buffer[21] = 0xBF;
    buffer[22] = 0xC2;
    buffer[23] = 0xBF;
    buffer[24] = 0x00;

    result = ov_utf8_last_valid(buffer, 25, true);
    testrun(result == buffer + 13);
    testrun(result[0] == 0xF0);
    result = ov_utf8_last_valid(buffer, 25, false);
    testrun(result == buffer + 17);
    testrun(result[0] == 0xc2);

    buffer[0] = 0x00;
    buffer[1] = 0xC2;
    buffer[2] = 0xBF;
    buffer[3] = 0x00;
    buffer[4] = 0xEf;
    buffer[5] = 0xBB;
    buffer[6] = 0xBF;
    buffer[7] = 0x00;
    buffer[8] = 0xF0;
    buffer[9] = 0xA3;
    buffer[10] = 0x8E;
    buffer[11] = 0xB4;
    buffer[12] = 0x00;
    buffer[13] = 0xF0;
    buffer[14] = 0xA3;
    buffer[15] = 0x8E;
    buffer[16] = 0xB4;
    buffer[17] = 0xC2;
    buffer[18] = 0xBF;
    buffer[19] = 0xff;
    buffer[20] = 0xBB;
    buffer[21] = 0xBF;
    buffer[22] = 0xC2;
    buffer[23] = 0xBF;
    buffer[24] = 0x00;

    result = ov_utf8_last_valid(buffer, 25, true);
    testrun(result == buffer + 17);
    testrun(result[0] == 0xc2);
    result = ov_utf8_last_valid(buffer, 25, false);
    testrun(result == buffer + 19);
    testrun(result[0] == 0xff);

    buffer[0] = 0x00;
    buffer[1] = 0xC2;
    buffer[2] = 0xBF;
    buffer[3] = 0x00;
    buffer[4] = 0xEf;
    buffer[5] = 0xBB;
    buffer[6] = 0xBF;
    buffer[7] = 0x00;
    buffer[8] = 0xF0;
    buffer[9] = 0xA3;
    buffer[10] = 0x8E;
    buffer[11] = 0xB4;
    buffer[12] = 0x00;
    buffer[13] = 0xF0;
    buffer[14] = 0xA3;
    buffer[15] = 0x8E;
    buffer[16] = 0xB4;
    buffer[17] = 0xC2;
    buffer[18] = 0xBF;
    buffer[19] = 0xEf;
    buffer[20] = 0xff;
    buffer[21] = 0xBF;
    buffer[22] = 0xC2;
    buffer[23] = 0xBF;
    buffer[24] = 0x00;

    result = ov_utf8_last_valid(buffer, 25, true);
    testrun(result == buffer + 17);
    testrun(result[0] == 0xC2);
    result = ov_utf8_last_valid(buffer, 25, false);
    testrun(result == buffer + 19);
    testrun(result[0] == 0xEF);

    buffer[0] = 0x00;
    buffer[1] = 0xC2;
    buffer[2] = 0xBF;
    buffer[3] = 0x00;
    buffer[4] = 0xEf;
    buffer[5] = 0xBB;
    buffer[6] = 0xBF;
    buffer[7] = 0x00;
    buffer[8] = 0xF0;
    buffer[9] = 0xA3;
    buffer[10] = 0x8E;
    buffer[11] = 0xB4;
    buffer[12] = 0x00;
    buffer[13] = 0xF0;
    buffer[14] = 0xA3;
    buffer[15] = 0x8E;
    buffer[16] = 0xB4;
    buffer[17] = 0xC2;
    buffer[18] = 0xBF;
    buffer[19] = 0xEf;
    buffer[20] = 0xBB;
    buffer[21] = 0xff;
    buffer[22] = 0xC2;
    buffer[23] = 0xBF;
    buffer[24] = 0x00;

    result = ov_utf8_last_valid(buffer, 25, true);
    testrun(result == buffer + 17);
    testrun(result[0] == 0xC2);
    result = ov_utf8_last_valid(buffer, 25, false);
    testrun(result == buffer + 19);
    testrun(result[0] == 0xEF);

    buffer[0] = 0x00;
    buffer[1] = 0xC2;
    buffer[2] = 0xBF;
    buffer[3] = 0x00;
    buffer[4] = 0xEf;
    buffer[5] = 0xBB;
    buffer[6] = 0xBF;
    buffer[7] = 0x00;
    buffer[8] = 0xF0;
    buffer[9] = 0xA3;
    buffer[10] = 0x8E;
    buffer[11] = 0xB4;
    buffer[12] = 0x00;
    buffer[13] = 0xF0;
    buffer[14] = 0xA3;
    buffer[15] = 0x8E;
    buffer[16] = 0xB4;
    buffer[17] = 0xC2;
    buffer[18] = 0xBF;
    buffer[19] = 0xEf;
    buffer[20] = 0xBB;
    buffer[21] = 0xBF;
    buffer[22] = 0xff;
    buffer[23] = 0xBF;
    buffer[24] = 0x00;

    result = ov_utf8_last_valid(buffer, 25, true);
    testrun(result == buffer + 19);
    testrun(result[0] == 0xEF);
    result = ov_utf8_last_valid(buffer, 25, false);
    testrun(result == buffer + 22);
    testrun(result[0] == 0xFF);

    buffer[0] = 0x00;
    buffer[1] = 0xC2;
    buffer[2] = 0xBF;
    buffer[3] = 0x00;
    buffer[4] = 0xEf;
    buffer[5] = 0xBB;
    buffer[6] = 0xBF;
    buffer[7] = 0x00;
    buffer[8] = 0xF0;
    buffer[9] = 0xA3;
    buffer[10] = 0x8E;
    buffer[11] = 0xB4;
    buffer[12] = 0x00;
    buffer[13] = 0xF0;
    buffer[14] = 0xA3;
    buffer[15] = 0x8E;
    buffer[16] = 0xB4;
    buffer[17] = 0xC2;
    buffer[18] = 0xBF;
    buffer[19] = 0xEf;
    buffer[20] = 0xBB;
    buffer[21] = 0xBF;
    buffer[22] = 0xC2;
    buffer[23] = 0xff;
    buffer[24] = 0x00;

    result = ov_utf8_last_valid(buffer, 25, true);
    testrun(result == buffer + 19);
    testrun(result[0] == 0xEF);
    result = ov_utf8_last_valid(buffer, 25, false);
    testrun(result == buffer + 22);
    testrun(result[0] == 0xC2);

    buffer[0] = 0x00;
    buffer[1] = 0xC2;
    buffer[2] = 0xBF;
    buffer[3] = 0x00;
    buffer[4] = 0xEf;
    buffer[5] = 0xBB;
    buffer[6] = 0xBF;
    buffer[7] = 0x00;
    buffer[8] = 0xF0;
    buffer[9] = 0xA3;
    buffer[10] = 0x8E;
    buffer[11] = 0xB4;
    buffer[12] = 0x00;
    buffer[13] = 0xF0;
    buffer[14] = 0xA3;
    buffer[15] = 0x8E;
    buffer[16] = 0xB4;
    buffer[17] = 0xC2;
    buffer[18] = 0xBF;
    buffer[19] = 0xEf;
    buffer[20] = 0xBB;
    buffer[21] = 0xBF;
    buffer[22] = 0xC2;
    buffer[23] = 0xBF;
    buffer[24] = 0xff;

    result = ov_utf8_last_valid(buffer, 25, true);
    testrun(result == buffer + 22);
    testrun(result[0] == 0xC2);
    result = ov_utf8_last_valid(buffer, 25, false);
    testrun(result == buffer + 24);
    testrun(result[0] == 0xff);

    free(buffer);
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_utf8_validate_sequence() {

    uint8_t *buffer = calloc(100, sizeof(uint8_t));

    testrun(!ov_utf8_validate_sequence(NULL, 0));
    testrun(!ov_utf8_validate_sequence(buffer, 0));

    testrun(ov_utf8_validate_sequence(buffer, 1));

    // 1 byte UTF8
    buffer[0] = 65;
    buffer[1] = 66;
    buffer[2] = 67;
    buffer[3] = 68;
    buffer[4] = 0xFF;
    testrun(ov_utf8_validate_sequence(buffer, 1), "valid ascii");
    testrun(ov_utf8_validate_sequence(buffer, 2), "valid ascii");
    testrun(ov_utf8_validate_sequence(buffer, 3), "valid ascii");
    testrun(ov_utf8_validate_sequence(buffer, 4), "valid ascii");
    testrun(!ov_utf8_validate_sequence(buffer, 5), "invalid ascii");

    // 2 byte UTF8
    buffer[0] = 0xCE; // alpha 0xCE91
    buffer[1] = 0x91;
    buffer[2] = 0xCE; // beta  0xCE92
    buffer[3] = 0x92;
    buffer[4] = 0xCE; // gamma 0xCE93
    buffer[5] = 0x92;
    buffer[6] = 0xCE; // delta 0xCE94
    buffer[7] = 0x93;
    buffer[8] = 0xCE; // INVALID LAST BYTE
    buffer[9] = 0x03;
    testrun(ov_utf8_validate_sequence(buffer, 2), "valid 2byte");
    testrun(ov_utf8_validate_sequence(buffer, 4), "valid 2byte");
    testrun(ov_utf8_validate_sequence(buffer, 6), "valid 2byte");
    testrun(ov_utf8_validate_sequence(buffer, 8), "valid 2byte");
    testrun(!ov_utf8_validate_sequence(buffer, 10), "invalid 2byte");
    testrun(!ov_utf8_validate_sequence(buffer, 1), "incomplete 2byte");
    testrun(!ov_utf8_validate_sequence(buffer, 3), "incomplete 2byte");
    testrun(!ov_utf8_validate_sequence(buffer, 5), "incomplete 2byte");
    testrun(!ov_utf8_validate_sequence(buffer, 7), "incomplete 2byte");
    testrun(!ov_utf8_validate_sequence(buffer, 9), "incomplete 2byte");

    // 3 byte UTF8
    buffer[0] = 0xE3; // Cjk unified ideograph U+3400
    buffer[1] = 0x90;
    buffer[2] = 0x80;
    buffer[3] = 0xE3; // Cjk unified ideograph U+3401
    buffer[4] = 0x90;
    buffer[5] = 0x81;
    buffer[6] = 0xE3; // Cjk unified ideograph U+3402
    buffer[7] = 0x90;
    buffer[8] = 0x82;
    buffer[9] = 0xE3; // invalid 3 byte
    buffer[10] = 0xFF;
    buffer[11] = 0x82;
    testrun(ov_utf8_validate_sequence(buffer, 3), "valid 3byte");
    testrun(ov_utf8_validate_sequence(buffer, 6), "valid 3byte");
    testrun(ov_utf8_validate_sequence(buffer, 9), "valid 3byte");
    testrun(!ov_utf8_validate_sequence(buffer, 11), "invalid 3byte");
    testrun(!ov_utf8_validate_sequence(buffer, 1), "incomplete 3byte");
    testrun(!ov_utf8_validate_sequence(buffer, 2), "incomplete 3byte");
    testrun(!ov_utf8_validate_sequence(buffer, 4), "incomplete 3byte");
    testrun(!ov_utf8_validate_sequence(buffer, 5), "incomplete 3byte");
    testrun(!ov_utf8_validate_sequence(buffer, 7), "incomplete 3byte");
    testrun(!ov_utf8_validate_sequence(buffer, 8), "incomplete 3byte");

    // 4 byte UTF8
    buffer[0] = 0xF0; // Gothic Letter Ahsa U+10330
    buffer[1] = 0x90;
    buffer[2] = 0x8C;
    buffer[3] = 0xB0;
    buffer[4] = 0xF0; // Gothic Letter Bairkan U+10331
    buffer[5] = 0x90;
    buffer[6] = 0x8C;
    buffer[7] = 0xB1;
    buffer[8] = 0xF0; // Gothic Letter Giba U+10332
    buffer[9] = 0x90;
    buffer[10] = 0x8C;
    buffer[11] = 0xB2;
    buffer[12] = 0xF0; // invalid 4 byte
    buffer[13] = 0xff;
    buffer[14] = 0x8C;
    buffer[15] = 0xB2;
    testrun(ov_utf8_validate_sequence(buffer, 4), "valid 4byte");
    testrun(ov_utf8_validate_sequence(buffer, 8), "valid 4byte");
    testrun(ov_utf8_validate_sequence(buffer, 12), "valid 4byte");
    testrun(!ov_utf8_validate_sequence(buffer, 16), "invalid 4byte");
    testrun(!ov_utf8_validate_sequence(buffer, 1), "incomplete 4byte");
    testrun(!ov_utf8_validate_sequence(buffer, 2), "incomplete 4byte");
    testrun(!ov_utf8_validate_sequence(buffer, 3), "incomplete 4byte");
    testrun(!ov_utf8_validate_sequence(buffer, 5), "incomplete 4byte");
    testrun(!ov_utf8_validate_sequence(buffer, 6), "incomplete 4byte");
    testrun(!ov_utf8_validate_sequence(buffer, 7), "incomplete 4byte");
    testrun(!ov_utf8_validate_sequence(buffer, 9), "incomplete 4byte");
    testrun(!ov_utf8_validate_sequence(buffer, 10), "incomplete 4byte");
    testrun(!ov_utf8_validate_sequence(buffer, 11), "incomplete 4byte");

    memset(buffer, 0, 100);

    // RFC examples

    // --+--------+-----+--
    // 41 E2 89 A2 CE 91 2E
    // --+--------+-----+--

    buffer[0] = 0x41;
    buffer[1] = 0xE2;
    buffer[2] = 0x89;
    buffer[3] = 0xA2;
    buffer[4] = 0xCE;
    buffer[5] = 0x91;
    buffer[6] = 0x2E;
    buffer[7] = 0x00;

    testrun(ov_utf8_validate_sequence(buffer, 1), "1 character");
    testrun(!ov_utf8_validate_sequence(buffer, 2), "incomplete");
    testrun(!ov_utf8_validate_sequence(buffer, 3), "incomplete");
    testrun(ov_utf8_validate_sequence(buffer, 4), "2 characters");
    testrun(!ov_utf8_validate_sequence(buffer, 5), "incomplete");
    testrun(ov_utf8_validate_sequence(buffer, 6), "3 characters");
    testrun(ov_utf8_validate_sequence(buffer, 7), "4 characters");

    // --------+--------+--------
    // ED 95 9C EA B5 AD EC 96 B4
    // --------+--------+--------

    buffer[0] = 0xED;
    buffer[1] = 0x95;
    buffer[2] = 0x9C;
    buffer[3] = 0xEA;
    buffer[4] = 0xB5;
    buffer[5] = 0xAD;
    buffer[6] = 0xEC;
    buffer[7] = 0x96;
    buffer[8] = 0xB4;
    buffer[9] = 0x00;

    testrun(!ov_utf8_validate_sequence(buffer, 1), "incomplete");
    testrun(!ov_utf8_validate_sequence(buffer, 2), "incomplete");
    testrun(ov_utf8_validate_sequence(buffer, 3), "1 character");
    testrun(!ov_utf8_validate_sequence(buffer, 4), "incomplete");
    testrun(!ov_utf8_validate_sequence(buffer, 5), "incomplete");
    testrun(ov_utf8_validate_sequence(buffer, 6), "2 characters");
    testrun(!ov_utf8_validate_sequence(buffer, 7), "incomplete");
    testrun(!ov_utf8_validate_sequence(buffer, 8), "incomplete");
    testrun(ov_utf8_validate_sequence(buffer, 9), "3 characters");
    testrun(ov_utf8_validate_sequence(buffer, 10), "4 characters");

    // --------+--------+--------
    // E6 97 A5 E6 9C AC E8 AA 9E
    // --------+--------+--------

    buffer[0] = 0xE6;
    buffer[1] = 0x97;
    buffer[2] = 0xA5;
    buffer[3] = 0xE6;
    buffer[4] = 0x9C;
    buffer[5] = 0xAC;
    buffer[6] = 0xE8;
    buffer[7] = 0xAA;
    buffer[8] = 0x9E;
    buffer[9] = 0x00;

    testrun(!ov_utf8_validate_sequence(buffer, 1), "incomplete");
    testrun(!ov_utf8_validate_sequence(buffer, 2), "incomplete");
    testrun(ov_utf8_validate_sequence(buffer, 3), "1 character");
    testrun(!ov_utf8_validate_sequence(buffer, 4), "incomplete");
    testrun(!ov_utf8_validate_sequence(buffer, 5), "incomplete");
    testrun(ov_utf8_validate_sequence(buffer, 6), "2 characters");
    testrun(!ov_utf8_validate_sequence(buffer, 7), "incomplete");
    testrun(!ov_utf8_validate_sequence(buffer, 8), "incomplete");
    testrun(ov_utf8_validate_sequence(buffer, 9), "3 characters");
    testrun(ov_utf8_validate_sequence(buffer, 10), "4 characters");

    // --------+-----------
    // EF BB BF F0 A3 8E B4
    // --------+-----------

    buffer[0] = 0xEf;
    buffer[1] = 0xBB;
    buffer[2] = 0xBF;
    buffer[3] = 0xF0;
    buffer[4] = 0xA3;
    buffer[5] = 0x8E;
    buffer[6] = 0xB4;
    buffer[7] = 0x00;

    testrun(!ov_utf8_validate_sequence(buffer, 1), "incomplete");
    testrun(!ov_utf8_validate_sequence(buffer, 2), "incomplete");
    testrun(ov_utf8_validate_sequence(buffer, 3), "1 character");
    testrun(!ov_utf8_validate_sequence(buffer, 4), "incomplete");
    testrun(!ov_utf8_validate_sequence(buffer, 5), "incomplete");
    testrun(!ov_utf8_validate_sequence(buffer, 6), "incomplete");
    testrun(ov_utf8_validate_sequence(buffer, 7), "2 characters");
    testrun(ov_utf8_validate_sequence(buffer, 8), "3 characters");

    // [a][b][c][d]

    int a, b;

    for (a = 0; a < 20; a++) {
        buffer[a] = 0;
    }

    // lightweigth byte pattern testing

    for (a = 0; a < 0xff; a++) {

        buffer[0] = a;
        buffer[1] = 0xBF;
        buffer[2] = 0xBF;
        buffer[3] = 0xBF;

        if (a <= 0x7F) {

            // ASCII
            testrun(ov_utf8_validate_sequence(buffer, 1));
            continue;
        } else if (a < 0xc2) {

            testrun(!ov_utf8_validate_sequence(buffer, 2));
            continue;

        } else if (a <= 0xDF) {

            testrun(ov_utf8_validate_sequence(buffer, 2));
            testrun(!ov_utf8_validate_sequence(buffer, 1));

            // validate all byte 2 combinations

            // iterate byte 2
            for (b = 0; b < 0xff; b++) {

                buffer[1] = b;
                buffer[2] = 0xBF;
                buffer[3] = 0xBF;

                if ((b < 0x80) || (b > 0xBF)) {
                    testrun(!ov_utf8_validate_sequence(buffer, 2));
                    continue;
                }

                // testrun_log("%d %.2x%.2x%.2x%.2x" ,a
                // ,buffer[0], buffer[1], buffer[2], buffer[3]);
                testrun(ov_utf8_validate_sequence(buffer, 2));
            }

        } else if (a < 0xF0) {

            buffer[1] = 0xBF;
            buffer[2] = 0xBF;
            buffer[3] = 0xBF;

            // 3 byte checks

            // iterate byte 2, 3
            for (b = 0; b < 0xff; b++) {

                buffer[1] = b;
                buffer[2] = 0xbf;

                testrun(!ov_utf8_validate_sequence(buffer, 1));
                testrun(!ov_utf8_validate_sequence(buffer, 2));

                switch (a) {

                    case 0xe0:

                        if ((b >= 0xA0) && (b <= 0xBF)) {

                            testrun(ov_utf8_validate_sequence(buffer, 3));
                        } else {
                            testrun(!ov_utf8_validate_sequence(buffer, 3));
                        }

                        break;
                    case 0xed:

                        if ((b >= 0x80) && (b <= 0x9F)) {
                            testrun(ov_utf8_validate_sequence(buffer, 3));
                        } else {
                            testrun(!ov_utf8_validate_sequence(buffer, 3));
                        }

                        break;
                    default:

                        if ((b >= 0x80) && (b <= 0xBF)) {
                            testrun(ov_utf8_validate_sequence(buffer, 3));

                        } else {
                            testrun(!ov_utf8_validate_sequence(buffer, 3));
                        }
                        break;
                }
            }

        } else if (a <= 0xf4) {

            // 4 byte checks

            // iterate byte 2,3,4
            for (b = 0; b < 0xff; b++) {

                buffer[1] = b;

                testrun(!ov_utf8_validate_sequence(buffer, 1));
                testrun(!ov_utf8_validate_sequence(buffer, 2));
                testrun(!ov_utf8_validate_sequence(buffer, 3));

                buffer[2] = 0xBF;
                buffer[3] = 0xBF;

                switch (a) {

                    case 0xf0:

                        if ((b >= 0x90) && (b <= 0xBF)) {

                            testrun(ov_utf8_validate_sequence(buffer, 4));
                        } else {
                            testrun(!ov_utf8_validate_sequence(buffer, 4));
                        }

                        break;
                    case 0xf4:

                        if ((b >= 0x80) && (b <= 0x8F)) {
                            testrun(ov_utf8_validate_sequence(buffer, 4));
                        } else {
                            testrun(!ov_utf8_validate_sequence(buffer, 4));
                        }

                        break;
                    default:

                        if ((b >= 0x80) && (b <= 0xBF)) {
                            testrun(ov_utf8_validate_sequence(buffer, 4));

                        } else {
                            testrun(!ov_utf8_validate_sequence(buffer, 4));
                        }
                        break;
                }
            }

        } else {
            // testrun_log("%d %.2x%.2x%.2x%.2x" ,a ,buffer[0],
            // buffer[1], buffer[2], buffer[3]);
            testrun(!ov_utf8_validate_sequence(buffer, 4));
        }
    }

    free(buffer);
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int testall_ov_utf8_validate_sequence() {

    uint8_t *buffer = calloc(100, sizeof(uint8_t));

    testrun(!ov_utf8_validate_sequence(NULL, 0));
    testrun(!ov_utf8_validate_sequence(buffer, 0));

    testrun(ov_utf8_validate_sequence(buffer, 1));

    // [a][b][c][d]

    int a, b, c, d;

    for (a = 0; a < 20; a++) {
        buffer[a] = 0;
    }

    for (a = 0; a < 0xff; a++) {

        buffer[0] = a;
        buffer[1] = 0xBF;
        buffer[2] = 0xBF;
        buffer[3] = 0xBF;

        if (a <= 0x7F) {

            // ASCII
            testrun(ov_utf8_validate_sequence(buffer, 1));
            continue;
        } else if (a < 0xc2) {

            testrun(!ov_utf8_validate_sequence(buffer, 2));
            continue;

        } else if (a <= 0xDF) {

            testrun(ov_utf8_validate_sequence(buffer, 2));
            testrun(!ov_utf8_validate_sequence(buffer, 1));

            // validate all byte 2 combinations

            // iterate byte 2
            for (b = 0; b < 0xff; b++) {

                buffer[1] = b;
                buffer[2] = 0xBF;
                buffer[3] = 0xBF;

                if ((b < 0x80) || (b > 0xBF)) {
                    testrun(!ov_utf8_validate_sequence(buffer, 2));
                    continue;
                }

                // testrun_log("%d %.2x%.2x%.2x%.2x" ,a
                // ,buffer[0], buffer[1], buffer[2], buffer[3]);
                testrun(ov_utf8_validate_sequence(buffer, 2));
            }

        } else if (a < 0xF0) {

            buffer[1] = 0xBF;
            buffer[2] = 0xBF;
            buffer[3] = 0xBF;

            // 3 byte checks

            // iterate byte 2
            for (b = 0; b < 0xff; b++) {

                buffer[1] = b;

                testrun(!ov_utf8_validate_sequence(buffer, 1));
                testrun(!ov_utf8_validate_sequence(buffer, 2));

                // iterate byte 3
                for (c = 0; c < 0xff; c++) {

                    buffer[2] = c;
                    switch (a) {

                        case 0xe0:

                            if ((b >= 0xA0) && (b <= 0xBF) && (c >= 0x80) &&
                                (c <= 0xBF)) {

                                testrun(ov_utf8_validate_sequence(buffer, 3));
                            } else {
                                testrun(!ov_utf8_validate_sequence(buffer, 3));
                            }

                            break;
                        case 0xed:

                            if ((b >= 0x80) && (b <= 0x9F) && (c >= 0x80) &&
                                (c <= 0xBF)) {
                                testrun(ov_utf8_validate_sequence(buffer, 3));
                            } else {
                                testrun(!ov_utf8_validate_sequence(buffer, 3));
                            }

                            break;
                        default:

                            if ((b >= 0x80) && (b <= 0xBF) && (c >= 0x80) &&
                                (c <= 0xBF)) {
                                testrun(ov_utf8_validate_sequence(buffer, 3));

                            } else {
                                testrun(!ov_utf8_validate_sequence(buffer, 3));
                            }
                            break;
                    }
                }
            }

        } else if (a <= 0xf4) {

            // 4 byte checks

            // iterate byte 2
            for (b = 0; b < 0xff; b++) {

                buffer[1] = b;

                testrun(!ov_utf8_validate_sequence(buffer, 1));
                testrun(!ov_utf8_validate_sequence(buffer, 2));
                testrun(!ov_utf8_validate_sequence(buffer, 3));

                // iterate byte 3
                for (c = 0; c < 0xff; c++) {

                    buffer[2] = c;
                    // testrun_log("%d %.2x%.2x%.2x%.2x" ,a
                    // ,buffer[0], buffer[1], buffer[2],
                    // buffer[3]);
                    testrun(!ov_utf8_validate_sequence(buffer, 3));

                    // iterate byte 4
                    for (d = 0; d < 0xff; d++) {

                        buffer[3] = d;

                        switch (a) {

                            case 0xf0:

                                if ((b >= 0x90) && (b <= 0xBF) && (c >= 0x80) &&
                                    (c <= 0xBF) && (d >= 0x80) && (d <= 0xBF)) {

                                    testrun(
                                        ov_utf8_validate_sequence(buffer, 4));
                                } else {
                                    testrun(
                                        !ov_utf8_validate_sequence(buffer, 4));
                                }

                                break;
                            case 0xf4:

                                if ((b >= 0x80) && (b <= 0x8F) && (c >= 0x80) &&
                                    (c <= 0xBF) && (d >= 0x80) && (d <= 0xBF)) {
                                    testrun(
                                        ov_utf8_validate_sequence(buffer, 4));
                                } else {
                                    testrun(
                                        !ov_utf8_validate_sequence(buffer, 4));
                                }

                                break;
                            default:

                                if ((b >= 0x80) && (b <= 0xBF) && (c >= 0x80) &&
                                    (c <= 0xBF) && (d >= 0x80) && (d <= 0xBF)) {
                                    testrun(
                                        ov_utf8_validate_sequence(buffer, 4));

                                } else {
                                    testrun(
                                        !ov_utf8_validate_sequence(buffer, 4));
                                }
                                break;
                        }
                    }
                }
            }

        } else {
            // testrun_log("%d %.2x%.2x%.2x%.2x" ,a ,buffer[0],
            // buffer[1], buffer[2], buffer[3]);
            testrun(!ov_utf8_validate_sequence(buffer, 4));
        }
    }

    free(buffer);
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_utf8_encode_code_point() {

    uint64_t i = 0;
    uint64_t value = 0;
    uint8_t *buffer = NULL;
    uint64_t length = 0;

    testrun(!ov_utf8_encode_code_point(0, NULL, 0, NULL));
    testrun(!ov_utf8_encode_code_point(0, &buffer, 1, NULL));
    testrun(!ov_utf8_encode_code_point(0, NULL, 1, &length));

    // TEST 1 BYTE RETURN

    testrun(buffer == NULL);
    testrun(length == 0);
    testrun(ov_utf8_encode_code_point(0, &buffer, 0, &length));
    testrun(buffer != NULL);
    testrun(length == 1);
    testrun(buffer[0] == 0);

    free(buffer);
    buffer = NULL;
    length = 0;

    testrun(ov_utf8_encode_code_point(1, &buffer, 0, &length));
    testrun(buffer != NULL);
    testrun(length == 1);
    testrun(buffer[0] == 1);

    free(buffer);
    buffer = NULL;
    length = 0;

    testrun(ov_utf8_encode_code_point(127, &buffer, 0, &length));
    testrun(buffer != NULL);
    testrun(length == 1);
    testrun(buffer[0] == 127);

    free(buffer);
    buffer = NULL;
    length = 0;

    // ASCII
    for (i = 0; i <= 127; i++) {

        testrun(ov_utf8_encode_code_point(i, &buffer, 0, &length));
        testrun(length == 1, "ascii length check");
        testrun(buffer[0] == i, "ascii value check");
        free(buffer);
        buffer = NULL;
        length = 0;
    }

    // 2 byte UTF8
    for (i = 128; i <= 0x07FF; i++) {

        testrun(ov_utf8_encode_code_point(i, &buffer, 0, &length));
        testrun(length == 2, "2byte length check");
        testrun(buffer[0] >> 5 == 0x06, "2byte header 0");
        testrun(buffer[1] >> 6 == 0x02, "2byte header 1");

        value = (buffer[1] & 0x3F) | ((buffer[0] << 6) & 0x07C0);
        testrun(value == i, "2byte value check");

        free(buffer);
        buffer = NULL;
        length = 0;
    }

    // non valid UTF8
    for (i = 0xD800; i <= 0xDFFF; i++) {

        testrun(!ov_utf8_encode_code_point(i, &buffer, 0, &length),
                "reserved for UTF16");
    }

    // 3 byte UTF8
    for (i = 0x0800; i < 0xD800; i++) {

        testrun(ov_utf8_encode_code_point(i, &buffer, 0, &length));
        testrun(length == 3, "3byte length check");
        testrun(buffer[0] >> 4 == 0x0E, "3byte header 0");
        testrun(buffer[1] >> 6 == 0x02, "3byte header 1");
        testrun(buffer[2] >> 6 == 0x02, "3byte header 2");

        value = (buffer[2] & 0x3F) | ((buffer[1] << 6) & 0x0FC0) |
                ((buffer[0] << 12) & 0xF000);

        testrun(value == i, "3byte value check");

        free(buffer);
        buffer = NULL;
        length = 0;
    }

    // 3 byte UTF8
    for (i = 0xE000; i < 0xFFFF; i++) {

        testrun(ov_utf8_encode_code_point(i, &buffer, 0, &length));
        testrun(length == 3, "3byte length check");
        testrun(buffer[0] >> 4 == 0x0E, "3byte header 0");
        testrun(buffer[1] >> 6 == 0x02, "3byte header 1");
        testrun(buffer[2] >> 6 == 0x02, "3byte header 2");

        value = (buffer[2] & 0x3F) | ((buffer[1] << 6) & 0x0FC0) |
                ((buffer[0] << 12) & 0xF000);

        testrun(value == i, "3byte value check");

        free(buffer);
        buffer = NULL;
        length = 0;
    }

    // 4 byte UTF8
    for (i = 0x10000; i < 0x10FFFF; i++) {

        testrun(ov_utf8_encode_code_point(i, &buffer, 0, &length));
        testrun(length == 4, "4byte length check");
        testrun(buffer[0] >> 3 == 0x1E, "4byte header 0");
        testrun(buffer[1] >> 6 == 0x02, "4byte header 1");
        testrun(buffer[2] >> 6 == 0x02, "4byte header 2");
        testrun(buffer[3] >> 6 == 0x02, "4byte header 3");

        value = (buffer[3] & 0x3F) | ((buffer[2] << 6) & 0x0FC0) |
                ((buffer[1] << 12) & 0x3F000) | ((buffer[0] << 18) & 0x1C0000);

        testrun(value == i, "4byte value check");

        free(buffer);
        buffer = NULL;
        length = 0;
    }

    // non valid UTF8
    for (i = 0x110000; i <= 0x11000F; i++) {
        testrun(!ov_utf8_encode_code_point(i, &buffer, 100, &length));
    }

    for (i = 0xF10000; i <= 0xF1000F; i++) {
        testrun(!ov_utf8_encode_code_point(i, &buffer, 100, &length));
    }

    // TEST OF WRITE TO FUNCTIONALITY

    length = 100;
    buffer = calloc(length, sizeof(uint8_t));
    uint8_t *pointer = buffer;

    // check ASCII (A)      A≢Α.
    testrun(length == 100);
    testrun(buffer[0] == 0);
    testrun(ov_utf8_encode_code_point(65, &pointer, 100, &length));
    testrun(length == 1);
    testrun(buffer[0] == 65);

    pointer += length;

    // add a 3 byte unicode (not identical to)
    testrun(!ov_utf8_encode_code_point(8802, &pointer, 2, &length));
    testrun(ov_utf8_encode_code_point(8802, &pointer, 100, &length));
    testrun(length == 3);
    testrun(buffer[0] == 65);
    testrun(buffer[1] == 0xE2);
    testrun(buffer[2] == 0x89);
    testrun(buffer[3] == 0xA2);

    pointer += length;

    // add a 2 byte unicode (greek big alpha)
    testrun(!ov_utf8_encode_code_point(913, &pointer, 1, &length));
    testrun(ov_utf8_encode_code_point(913, &pointer, 100, &length));
    testrun(length == 2);
    testrun(buffer[0] == 65);
    testrun(buffer[1] == 0xE2);
    testrun(buffer[2] == 0x89);
    testrun(buffer[3] == 0xA2);
    testrun(buffer[4] == 0xCE);
    testrun(buffer[5] == 0x91);

    pointer += length;

    // add a 4 byte unicode (ancient greek drachma sign 𐅻)
    testrun(!ov_utf8_encode_code_point(65915, &pointer, 3, &length));
    testrun(ov_utf8_encode_code_point(65915, &pointer, 100, &length));
    testrun(length == 4);
    testrun(buffer[0] == 65);
    testrun(buffer[1] == 0xE2);
    testrun(buffer[2] == 0x89);
    testrun(buffer[3] == 0xA2);
    testrun(buffer[4] == 0xCE);
    testrun(buffer[5] == 0x91);
    testrun(buffer[6] == 0xF0);
    testrun(buffer[7] == 0x90);
    testrun(buffer[8] == 0x85);
    testrun(buffer[9] == 0xBB);

    free(buffer);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_utf8_decode_code_point() {

    uint64_t bytes = 0;
    uint8_t *buffer = calloc(100, sizeof(uint8_t));

    testrun(0 == ov_utf8_decode_code_point(NULL, 0, NULL));
    testrun(0 == ov_utf8_decode_code_point(buffer, 0, NULL));
    testrun(0 == ov_utf8_decode_code_point(NULL, 1, NULL));
    testrun(0 == ov_utf8_decode_code_point(buffer, 1, NULL));

    // check for input failure (0 length)
    testrun(0 == ov_utf8_decode_code_point(buffer, 0, &bytes));
    testrun(bytes == 0);

    // special case, content is 0 (ASCII)
    testrun(0 == ov_utf8_decode_code_point(buffer, 1, &bytes));
    testrun(bytes == 1, "SPECIAL CASE 0 as CONTENT and 0 as RETURN");

    // standard case, content other then 0
    buffer[0] = 1;
    testrun(1 == ov_utf8_decode_code_point(buffer, 1, &bytes));
    testrun(bytes == 1);
    testrun(0 == ov_utf8_decode_code_point(NULL, 1, &bytes));
    testrun(0 == ov_utf8_decode_code_point(buffer, 0, &bytes));
    testrun(0 == ov_utf8_decode_code_point(buffer, 1, NULL));

    // failure case ascii wrong
    buffer[0] = 0xff;
    testrun(0 == ov_utf8_decode_code_point(buffer, 1, &bytes));
    testrun(bytes == 0);

    int a, b, c, d;

    for (a = 0; a < 20; a++) {
        buffer[a] = 0;
    }

    for (a = 0; a < 0xff; a++) {

        buffer[0] = a;
        buffer[1] = 0xBF;
        buffer[2] = 0xBF;
        buffer[3] = 0xBF;

        if (a <= 0x7F) {

            // ASCII
            testrun((uint64_t)a ==
                    ov_utf8_decode_code_point(buffer, 1, &bytes));

            testrun(bytes == 1);

            continue;

        } else if (a < 0xc2) {

            testrun(0 == ov_utf8_decode_code_point(buffer, 1, &bytes));

            testrun(bytes == 0);

            continue;

        } else if (a <= 0xDF) {

            // validate all byte 2 combinations

            // iterate byte 2
            for (b = 0; b < 0xff; b++) {

                buffer[1] = b;

                if ((b < 0x80) || (b > 0xBF)) {

                    testrun(0 == ov_utf8_decode_code_point(buffer, 4, &bytes));

                    testrun(bytes == 0);

                    continue;
                }

                // testrun_log("%d %.2x%.2x%.2x%.2x" ,a
                // ,buffer[0], buffer[1], buffer[2], buffer[3]);
                testrun((((buffer[0] << 6) & 0x07C0) | (buffer[1] & 0x3F)) ==
                        ov_utf8_decode_code_point(buffer, 4, &bytes));
                testrun(bytes == 2);
            }

        } else if (a < 0xF0) {

            buffer[1] = 0xBF;
            buffer[2] = 0xBF;
            buffer[3] = 0xBF;

            // 3 byte checks

            // iterate byte 2
            for (b = 0; b < 0xff; b++) {

                buffer[1] = b;

                // iterate byte 3
                for (c = 0; c < 0xff; c++) {

                    buffer[2] = c;
                    switch (a) {

                        case 0xe0:

                            if ((b >= 0xA0) && (b <= 0xBF) && (c >= 0x80) &&
                                (c <= 0xBF)) {

                                testrun((((buffer[0] << 12) & 0xF000) |
                                         ((buffer[1] << 6) & 0xfc0) |
                                         (buffer[2] & 0x3F)) ==
                                        ov_utf8_decode_code_point(
                                            buffer, 4, &bytes));

                                testrun(bytes == 3);

                            } else {

                                testrun(0 == ov_utf8_decode_code_point(
                                                 buffer, 4, &bytes));

                                testrun(bytes == 0);
                            }

                            break;
                        case 0xed:

                            if ((b >= 0x80) && (b <= 0x9F) && (c >= 0x80) &&
                                (c <= 0xBF)) {
                                testrun((((buffer[0] << 12) & 0xF000) |
                                         ((buffer[1] << 6) & 0xfc0) |
                                         (buffer[2] & 0x3F)) ==
                                        ov_utf8_decode_code_point(
                                            buffer, 4, &bytes));

                                testrun(bytes == 3);

                            } else {
                                testrun(0 == ov_utf8_decode_code_point(
                                                 buffer, 4, &bytes));

                                testrun(bytes == 0);
                            }

                            break;
                        default:

                            if ((b >= 0x80) && (b <= 0xBF) && (c >= 0x80) &&
                                (c <= 0xBF)) {
                                testrun((((buffer[0] << 12) & 0xF000) |
                                         ((buffer[1] << 6) & 0xfc0) |
                                         (buffer[2] & 0x3F)) ==
                                        ov_utf8_decode_code_point(
                                            buffer, 4, &bytes));

                                testrun(bytes == 3);

                            } else {
                                testrun(0 == ov_utf8_decode_code_point(
                                                 buffer, 4, &bytes));

                                testrun(bytes == 0);
                            }
                            break;
                    }
                }
            }

        } else if (a <= 0xf4) {

            // 4 byte checks

            // iterate byte 2
            for (b = 0; b < 0xff; b++) {

                buffer[1] = b;

                // iterate byte 3
                for (c = 0; c < 0xff; c++) {

                    buffer[2] = c;
                    // iterate byte 4
                    for (d = 0; d < 0xff; d++) {

                        buffer[3] = d;

                        switch (a) {

                            case 0xf0:

                                if ((b >= 0x90) && (b <= 0xBF) && (c >= 0x80) &&
                                    (c <= 0xBF) && (d >= 0x80) && (d <= 0xBF)) {

                                    testrun(ov_utf8_decode_code_point(
                                        buffer, 4, &bytes));

                                    testrun(bytes == 4);

                                } else {
                                    testrun(0 == ov_utf8_decode_code_point(
                                                     buffer, 4, &bytes));

                                    testrun(bytes == 0);
                                }

                                break;
                            case 0xf4:

                                if ((b >= 0x80) && (b <= 0x8F) && (c >= 0x80) &&
                                    (c <= 0xBF) && (d >= 0x80) && (d <= 0xBF)) {
                                    testrun(ov_utf8_decode_code_point(
                                        buffer, 4, &bytes));

                                    testrun(bytes == 4);

                                } else {
                                    testrun(0 == ov_utf8_decode_code_point(
                                                     buffer, 4, &bytes));

                                    testrun(bytes == 0);
                                }

                                break;
                            default:

                                if ((b >= 0x80) && (b <= 0xBF) && (c >= 0x80) &&
                                    (c <= 0xBF) && (d >= 0x80) && (d <= 0xBF)) {
                                    testrun(ov_utf8_decode_code_point(
                                        buffer, 4, &bytes));

                                    testrun(bytes == 4);

                                } else {
                                    testrun(0 == ov_utf8_decode_code_point(
                                                     buffer, 4, &bytes));

                                    testrun(bytes == 0);
                                }
                                break;
                        }
                    }
                }
            }

        } else {
            // testrun_log("%d %.2x%.2x%.2x%.2x" ,a ,buffer[0],
            // buffer[1], buffer[2], buffer[3]);
            testrun(0 == ov_utf8_decode_code_point(buffer, 4, &bytes));

            testrun(bytes == 0);
        }
    }

    free(buffer);
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_utf8_generate_random_buffer() {

    uint8_t *array[1000] = {0};
    size_t sizes[1000] = {0};

    uint8_t *buffer = NULL;
    size_t size = 100;

    testrun(!ov_utf8_generate_random_buffer(NULL, NULL, 0));
    testrun(!ov_utf8_generate_random_buffer(&buffer, &size, 0));
    testrun(!ov_utf8_generate_random_buffer(NULL, &size, 4));
    testrun(!ov_utf8_generate_random_buffer(&buffer, NULL, 4));

    size = 0;
    testrun(ov_utf8_generate_random_buffer(&buffer, &size, 4));
    testrun(ov_utf8_validate_sequence(buffer, size));
    free(buffer);

    for (size_t i = 0; i < 1000; i++) {

        buffer = NULL;
        size = 0;
        testrun(ov_utf8_generate_random_buffer(&buffer, &size, 50));
        testrun(ov_utf8_validate_sequence(buffer, size));

        // testrun_log("i %jd | %jd | %s |\n", i, size, (char*)buffer);

        for (size_t k = 0; k < i; k++) {

            if (sizes[k] == size) {
                testrun(0 != memcmp(array[k], buffer, size));
            }
        }

        array[i] = buffer;
        sizes[i] = size;
    }

    for (size_t i = 0; i < 1000; i++) {

        free(array[i]);
        array[i] = NULL;
    }

    // check fill mode

    uint8_t buf[100] = {0};
    size = 100;
    uint8_t *ptr = buf;
    testrun(ov_utf8_generate_random_buffer(&ptr, &size, 10));
    testrun(size != 100);
    testrun(ov_utf8_validate_sequence(buf, size));

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

    testrun_test(test_ov_utf8_last_valid);
    testrun_test(test_ov_utf8_validate_sequence);

    testrun_test(testall_ov_utf8_validate_sequence);

    testrun_test(test_ov_utf8_encode_code_point);

    testrun_test(test_ov_utf8_decode_code_point);

    testrun_test(test_ov_utf8_generate_random_buffer);

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
