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
        @file           ov_byteorder_tests.c
        @author         Michael Beer

        @date           2017-12-10

        @brief


        ------------------------------------------------------------------------
*/

#include <ov_base/ov_utils.h>

#include "../include/testrun.h"
#include "ov_byteorder.c"

/*----------------------------------------------------------------------------*/

int test_OV_SWAP_16() {

    uint16_t in = 0x1122;

    testrun(0x2211 == OV_SWAP_16(in));
    testrun(0x1234 == OV_SWAP_16(0x3412));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_OV_SWAP_32() {

    uint32_t in = 0x11223344;

    testrun(0x44332211 == OV_SWAP_32(in));
    testrun(0x12345678 == OV_SWAP_32(0x78563412));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_OV_SWAP_64() {

    uint64_t in = 0x1122334455667788;

    testrun(0x8877665544332211 == OV_SWAP_64(in));
    testrun(0x1234567890123456 == OV_SWAP_64(0x5634129078563412));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_OV_BE16TOH() {

    uint8_t in[] = {0x11, 0x22};
    uint8_t in_2[] = {0x34, 0x12};

    uint16_t *in16_p = (uint16_t *)in;

    testrun(0x1122 == OV_BE16TOH(*in16_p));

    in16_p = (uint16_t *)in_2;

    testrun(0x3412 == OV_BE16TOH(*in16_p));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_OV_BE32TOH() {

    uint8_t in[] = {0x11, 0x22, 0x33, 0x44};
    uint8_t in_2[] = {0x78, 0x56, 0x34, 0x12};

    uint32_t *in32_p = (uint32_t *)in;

    testrun(0x11223344 == OV_BE32TOH(*in32_p));

    in32_p = (uint32_t *)in_2;

    testrun(0x78563412 == OV_BE32TOH(*in32_p));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_OV_BE64TOH() {

    uint8_t in[] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
    uint8_t in_2[] = {0x21, 0x43, 0x65, 0x87, 0x78, 0x56, 0x34, 0x12};

    uint64_t *in64_p = (uint64_t *)in;

    testrun(0x1122334455667788 == OV_BE64TOH(*in64_p));

    in64_p = (uint64_t *)in_2;

    testrun(0x2143658778563412 == OV_BE64TOH(*in64_p));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_OV_LE16TOH() {

    uint8_t in[] = {0x11, 0x22};
    uint8_t in_2[] = {0x34, 0x12};

    uint16_t *in16_p = (uint16_t *)in;

    testrun(0x2211 == OV_LE16TOH(*in16_p));

    in16_p = (uint16_t *)in_2;

    testrun(0x1234 == OV_LE16TOH(*in16_p));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_OV_LE32TOH() {

    uint8_t in[] = {0x11, 0x22, 0x33, 0x44};
    uint8_t in_2[] = {0x78, 0x56, 0x34, 0x12};

    uint32_t *in32_p = (uint32_t *)in;

    testrun(0x44332211 == OV_LE32TOH(*in32_p));

    in32_p = (uint32_t *)in_2;

    testrun(0x12345678 == OV_LE32TOH(*in32_p));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_OV_LE64TOH() {

    uint8_t in[] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
    uint8_t in_2[] = {0x21, 0x43, 0x65, 0x87, 0x78, 0x56, 0x34, 0x12};

    uint64_t *in64_p = (uint64_t *)in;

    testrun(0x8877665544332211 == OV_LE64TOH(*in64_p));

    in64_p = (uint64_t *)in_2;

    testrun(0x1234567887654321 == OV_LE64TOH(*in64_p));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_OV_H16TOBE() {

    testrun(0x1122 == OV_BE16TOH(OV_H16TOBE(0x1122)));
    testrun(0x3412 == OV_BE16TOH(OV_H16TOBE(0x3412)));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_OV_H32TOBE() {

    testrun(0x11223344 == OV_BE32TOH(OV_H32TOBE(0x11223344)));
    testrun(0x78563412 == OV_BE32TOH(OV_H32TOBE(0x78563412)));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_OV_H64TOBE() {

    testrun(0x1122334455667788 == OV_BE64TOH(OV_H64TOBE(0x1122334455667788)));
    testrun(0x8765432178563412 == OV_BE64TOH(OV_H64TOBE(0x8765432178563412)));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_OV_H16TOLE() {

    testrun(0x1122 == OV_LE16TOH(OV_H16TOLE(0x1122)));
    testrun(0x3412 == OV_LE16TOH(OV_H16TOLE(0x3412)));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_OV_H32TOLE() {

    testrun(0x11223344 == OV_LE32TOH(OV_H32TOLE(0x11223344)));
    testrun(0x78563412 == OV_LE32TOH(OV_H32TOLE(0x78563412)));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_OV_H64TOLE() {

    testrun(0x1122334455667788 == OV_LE64TOH(OV_H64TOLE(0x1122334455667788)));
    testrun(0x8765432178563412 == OV_LE64TOH(OV_H64TOLE(0x8765432178563412)));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_byteorder_swap_bytes_16_bit(void) {

    int16_t test_data[] = {0xff00, 0x00ff, 0x8008, 0x71A1};
    int16_t expect[] = {0x00ff, 0xff00, 0x0880, 0xA171};

    testrun(ov_byteorder_swap_bytes_16_bit(test_data, 4))

        for (size_t i = 0; i < 4; i++) {

        testrun(test_data[i] == expect[i]);
    }

    return testrun_log_success();
}

/*---------------------------------------------------------------------------*/
/*                          Endianness Conversion                            */
/*---------------------------------------------------------------------------*/

int test_ov_byteorder_to_little_endian_16_bit(void) {

    int16_t test_data[] = {0xff00, 0x00ff, 0x8008, 0x71A1};
    int8_t le_reference[] = {0x00, 0xff, 0xff, 0x00, 0x08, 0x80, 0xA1, 0x71};

    int16_t *little_endian = calloc(1, sizeof(test_data));

    for (size_t i = 0; i < sizeof(test_data) / sizeof(int16_t) + 1; i++) {

        memcpy(little_endian, test_data, sizeof(test_data));

        testrun(true == ov_byteorder_to_little_endian_16_bit(little_endian, i),
                "Conversion to little endian");

        int8_t *le_bytes = (int8_t *)little_endian;

        for (size_t t = 0; t < i * sizeof(int16_t); t++) {

            testrun(le_bytes[t] == le_reference[t], "To little endian");
        }
    }

    free(little_endian);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_byteorder_to_big_endian_16_bit(void) {

    int16_t test_data[] = {0xff00, 0x00ff, 0x8008, 0x71A1};
    int8_t be_reference[] = {0xff, 0x00, 0x00, 0xff, 0x80, 0x08, 0x71, 0xA1};

    int16_t *big_endian = calloc(1, sizeof(test_data));

    for (size_t i = 0; i < sizeof(test_data) / sizeof(int16_t) + 1; i++) {

        memcpy(big_endian, test_data, sizeof(test_data));

        testrun(true == ov_byteorder_to_big_endian_16_bit(big_endian, i),
                "Conversion to big endian");

        int8_t *be_bytes = (int8_t *)big_endian;

        for (size_t t = 0; t < i * sizeof(int16_t); t++) {

            testrun(be_bytes[t] == be_reference[t], "To big endian");
        }
    }

    free(big_endian);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_byteorder_from_little_endian_16_bit(void) {

    int8_t test_data_le[] = {0x00, 0xff, 0xff, 0x00, 0x08, 0x80, 0xA1, 0x71};
    int16_t reference[] = {0xff00, 0x00ff, 0x8008, 0x71A1};

    int16_t *from_little_endian = calloc(1, sizeof(test_data_le));

    for (size_t i = 0; i < sizeof(test_data_le) / sizeof(int16_t) + 1; i++) {

        memcpy(from_little_endian, test_data_le, sizeof(test_data_le));

        testrun(true == ov_byteorder_from_little_endian_16_bit(
                            from_little_endian, i),
                "Conversion from little endian");

        for (size_t t = 0; t < i; t++) {

            testrun(from_little_endian[t] == reference[t],
                    "From little endian");
        }
    }

    free(from_little_endian);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_byteorder_from_big_endian_16_bit(void) {

    int8_t test_data_be[] = {0xff, 0x00, 0x00, 0xff, 0x80, 0x08, 0x71, 0xA1};
    int16_t reference[] = {0xff00, 0x00ff, 0x8008, 0x71A1};

    int16_t *from_big_endian = calloc(1, sizeof(test_data_be));

    for (size_t i = 0; i < sizeof(test_data_be) / sizeof(int16_t) + 1; i++) {

        memcpy(from_big_endian, test_data_be, sizeof(test_data_be));

        testrun(true == ov_byteorder_from_big_endian_16_bit(from_big_endian, i),
                "Conversion from big endian");

        for (size_t t = 0; t < i; t++) {

            testrun(from_big_endian[t] == reference[t], "From big endian");
        }
    }

    free(from_big_endian);

    return testrun_log_success();
}

RUN_TESTS("ov_byteorder", test_OV_SWAP_16, test_OV_SWAP_32, test_OV_SWAP_64,

          test_OV_BE16TOH, test_OV_BE32TOH, test_OV_BE64TOH,

          test_OV_LE16TOH, test_OV_LE32TOH, test_OV_LE64TOH,

          test_OV_H16TOBE, test_OV_H32TOBE, test_OV_H64TOBE,

          test_OV_H16TOLE, test_OV_H32TOLE, test_OV_H64TOLE,

          test_ov_byteorder_swap_bytes_16_bit,
          test_ov_byteorder_to_little_endian_16_bit,
          test_ov_byteorder_to_big_endian_16_bit,
          test_ov_byteorder_from_little_endian_16_bit,
          test_ov_byteorder_from_big_endian_16_bit);
