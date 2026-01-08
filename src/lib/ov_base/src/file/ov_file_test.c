/***
        ------------------------------------------------------------------------

        Copyright (c) 2019 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_file_test.c
        @author         Markus Toepfer
        @author         Michael Beer

        @date           2019-07-23

        @ingroup        ov_websocket

        @brief          Unit tests of


        ------------------------------------------------------------------------
*/

#include "../../include/ov_json.h"
#include "ov_file.c"
#include <limits.h>
#include <ov_base/ov_string.h>
#include <ov_test/ov_test.h>
#include <ov_test/ov_test_file.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

int test_ov_file_read() {

    char *path = ov_test_get_resource_path("resources/JSON/json.ok1");
    uint8_t *buffer = NULL;
    size_t size = 0;

    testrun(OV_FILE_ERROR == ov_file_read(NULL, NULL, NULL));
    testrun(OV_FILE_ERROR == ov_file_read(NULL, &buffer, &size));
    testrun(OV_FILE_ERROR == ov_file_read(path, NULL, &size));
    testrun(OV_FILE_ERROR == ov_file_read(path, &buffer, NULL));

    testrun(OV_FILE_SUCCESS == ov_file_read(path, &buffer, &size));
    testrun(buffer);
    testrun(size > 0);

    ov_json_value *check = ov_json_read((char *)buffer, size);
    testrun(check);

    testrun(NULL == ov_json_value_free(check));

    free(path);

    path = ov_test_get_resource_path("resources/"
                                     "not_existing12131134524124124315");

    free(buffer);
    buffer = NULL;
    size = 0;

    ov_file_handle_state r = ov_file_read(path, &buffer, &size);

    testrun(OV_FILE_SUCCESS != r);
    testrun(NULL == buffer);
    testrun(0 == size);

    free(path);

    path = "not valid";
    testrun(OV_FILE_SUCCESS != ov_file_read(path, &buffer, &size));
    testrun(NULL == buffer);
    testrun(0 == size);

    testrun_log("PERMISSION CHECKS NOT DONE");

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_file_read_check_bytes() {

    uint8_t *content = (uint8_t *)"something unimportant, but some bytes";

    char *files[] = {ov_test_get_resource_path("resources/afddd.testfile"),
                     ov_test_get_resource_path("resources/dnoaofa.testfile"),
                     ov_test_get_resource_path("resources/vnijfisfn.testfile"),
                     ov_test_get_resource_path("resources/fonff.testfile"),
                     ov_test_get_resource_path("resources/af.testfile"),
                     ov_test_get_resource_path("resources/1.testfile"),
                     ov_test_get_resource_path("resources/afafffsf.testfile"),
                     ov_test_get_resource_path("resources/dd.testfile")};

    ssize_t items = sizeof(files) / sizeof(files[0]);
    testrun(items > 0);

    FILE *fp = 0;

    for (ssize_t i = 0; i < items; i++) {

        unlink(files[i]);

        if (0 == i) {

            fp = fopen(files[i], "w");
            testrun(fp);
            fclose(fp);

        } else {

            testrun(OV_FILE_SUCCESS ==
                    ov_file_write(files[i], content, i, "w"));
        }

        testrun(NULL == ov_file_read_check(files[i]));
        testrun(i == ov_file_read_check_get_bytes(files[i]));
    }

    char *p = ov_test_get_resource_path(0);
    // check -1 on dir
    testrun(-1 == ov_file_read_check_get_bytes(p));

    free(p);

    // check -1 on non existing
    p = ov_test_get_resource_path("resources/"
                                  "this_file_shall_not_exists_fajfnafnf");
    unlink(p);
    testrun(-1 == ov_file_read_check_get_bytes(p));

    free(p);
    p = 0;
    // check no read permission

    /* Permission test not running under CI/CD gitlab-runner 14.x
        testrun(0 == chmod(files[1], S_IWUSR | S_IXUSR));
        testrun(NULL != ov_file_read_check(files[1]));
        testrun(-1 == ov_file_read_check_get_bytes(files[1]));
    */
    for (ssize_t i = 0; i < items; i++) {
        unlink(files[i]);
        free(files[i]);
        files[i] = 0;
    }

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_file_exists() {

    testrun(!ov_file_exists(0));
    testrun(ov_file_exists("/tmp"));
    testrun(!ov_file_exists("/abracadabra"));

    char *test_file = tempnam("/tmp", "ov_file_test");
    testrun(0 != test_file);

    testrun(!ov_file_exists(test_file));

    testrun(OV_FILE_SUCCESS == ov_file_write(test_file, (uint8_t *)test_file,
                                             strlen(test_file), 0));

    testrun(ov_file_exists(test_file));

    unlink(test_file);

    free(test_file);
    test_file = 0;

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_file_write() {

    char *path = ov_test_get_resource_path("resources/testfile");
    uint8_t *buffer = (uint8_t *)"test";
    size_t size = 4;

    uint8_t *out = NULL;
    size_t len = 0;

    unlink(path);

    testrun(OV_FILE_ERROR == ov_file_write(NULL, NULL, 0, NULL));
    testrun(OV_FILE_ERROR == ov_file_write(NULL, buffer, size, NULL));
    testrun(OV_FILE_ERROR == ov_file_write(path, NULL, size, NULL));
    testrun(OV_FILE_ERROR == ov_file_write(path, buffer, 0, NULL));

    testrun(-1 == access(path, R_OK));

    testrun(OV_FILE_SUCCESS == ov_file_write(path, buffer, size, NULL));
    testrun(OV_FILE_SUCCESS == ov_file_read(path, &out, &len));

    testrun(len == size);
    testrun(0 == memcmp(out, buffer, size));
    free(out);
    out = NULL;

    // check append
    testrun(OV_FILE_SUCCESS == ov_file_write(path, buffer, size, "a"));
    testrun(OV_FILE_SUCCESS == ov_file_read(path, &out, &len));

    testrun(len == 2 * size);
    testrun(0 == memcmp(out, "testtest", len));
    free(out);
    out = NULL;

    // check override
    testrun(OV_FILE_SUCCESS == ov_file_write(path, buffer, size, "w"));
    testrun(OV_FILE_SUCCESS == ov_file_read(path, &out, &len));

    testrun(len == size);
    testrun(0 == memcmp(out, buffer, size));
    free(out);
    out = NULL;

    free(path);
    path = 0;

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_file_get_16() {

    const uint16_t test_in[] = {0x1122, 0x3344};
    const uint16_t ref_swapped[] = {0x2211, 0x4433};

    uint8_t *test_in_ptr = (uint8_t *)test_in;

    uint16_t out = 0;
    size_t length = 2;

    testrun(!ov_file_get_16(0, 0, 0, OV_FILE_RAW));
    testrun(test_in_ptr == (uint8_t *)test_in);
    testrun(2 == length);

    testrun(!ov_file_get_16(&out, 0, 0, OV_FILE_RAW));
    testrun(test_in_ptr == (uint8_t *)test_in);
    testrun(2 == length);

    testrun(!ov_file_get_16(0, &test_in_ptr, 0, OV_FILE_RAW));
    testrun(test_in_ptr == (uint8_t *)test_in);
    testrun(2 == length);

    testrun(!ov_file_get_16(&out, &test_in_ptr, 0, OV_FILE_RAW));
    testrun(test_in_ptr == (uint8_t *)test_in);
    testrun(2 == length);

    length = 0;

    testrun(!ov_file_get_16(0, 0, &length, OV_FILE_RAW));
    testrun(test_in_ptr == (uint8_t *)test_in);
    testrun(0 == length);

    length = 2;

    testrun(!ov_file_get_16(0, 0, &length, OV_FILE_RAW));
    testrun(test_in_ptr == (uint8_t *)test_in);
    testrun(2 == length);

    testrun(!ov_file_get_16(&out, 0, &length, OV_FILE_RAW));
    testrun(test_in_ptr == (uint8_t *)test_in);
    testrun(2 == length);

    length = 0;
    testrun(!ov_file_get_16(&out, &test_in_ptr, &length, OV_FILE_RAW));
    testrun(test_in_ptr == (uint8_t *)test_in);
    testrun(0 == length);

    length = 1;
    testrun(!ov_file_get_16(&out, &test_in_ptr, &length, OV_FILE_RAW));
    testrun(test_in_ptr == (uint8_t *)test_in);
    testrun(1 == length);

    length = 4;
    testrun(ov_file_get_16(&out, &test_in_ptr, &length, OV_FILE_RAW));
    testrun(test_in[0] == out);
    testrun((uint16_t *)test_in_ptr == test_in + 1);
    testrun(2 == length);

    testrun(ov_file_get_16(&out, &test_in_ptr, &length, OV_FILE_RAW));
    testrun(test_in[1] == out);
    testrun((uint16_t *)test_in_ptr == test_in + 2);
    testrun(0 == length);

    /**************************************************************************
     *                                 Swap bytes
     **************************************************************************/

    test_in_ptr = (uint8_t *)test_in;

    out = 0;
    length = 2;

    testrun(!ov_file_get_16(0, 0, 0, OV_FILE_SWAP_BYTES));
    testrun(test_in_ptr == (uint8_t *)test_in);
    testrun(2 == length);

    testrun(!ov_file_get_16(&out, 0, 0, OV_FILE_SWAP_BYTES));
    testrun(test_in_ptr == (uint8_t *)test_in);
    testrun(2 == length);

    testrun(!ov_file_get_16(0, &test_in_ptr, 0, OV_FILE_SWAP_BYTES));
    testrun(test_in_ptr == (uint8_t *)test_in);
    testrun(2 == length);

    testrun(!ov_file_get_16(&out, &test_in_ptr, 0, OV_FILE_SWAP_BYTES));
    testrun(test_in_ptr == (uint8_t *)test_in);
    testrun(2 == length);

    length = 0;
    testrun(!ov_file_get_16(0, 0, &length, OV_FILE_SWAP_BYTES));
    testrun(test_in_ptr == (uint8_t *)test_in);
    testrun(0 == length);

    length = 2;

    testrun(!ov_file_get_16(0, 0, &length, OV_FILE_SWAP_BYTES));
    testrun(test_in_ptr == (uint8_t *)test_in);
    testrun(2 == length);

    length = 0;
    testrun(!ov_file_get_16(&out, 0, &length, OV_FILE_SWAP_BYTES));
    testrun(test_in_ptr == (uint8_t *)test_in);
    testrun(0 == length);

    length = 2;
    testrun(!ov_file_get_16(&out, 0, &length, OV_FILE_SWAP_BYTES));
    testrun(test_in_ptr == (uint8_t *)test_in);
    testrun(2 == length);

    length = 0;
    testrun(!ov_file_get_16(&out, &test_in_ptr, &length, OV_FILE_SWAP_BYTES));
    testrun(test_in_ptr == (uint8_t *)test_in);
    testrun(0 == length);

    length = 1;
    testrun(!ov_file_get_16(&out, &test_in_ptr, &length, OV_FILE_SWAP_BYTES));
    testrun(test_in_ptr == (uint8_t *)test_in);
    testrun(1 == length);

    length = 4;
    testrun(ov_file_get_16(&out, &test_in_ptr, &length, OV_FILE_SWAP_BYTES));
    testrun(ref_swapped[0] == out);
    testrun(test_in_ptr == (uint8_t *)(test_in + 1));
    testrun(2 == length);

    testrun(ov_file_get_16(&out, &test_in_ptr, &length, OV_FILE_SWAP_BYTES));
    testrun(ref_swapped[1] == out);
    testrun(test_in_ptr == (uint8_t *)(test_in + 2));
    testrun(0 == length);

    /*************************************************************************
                               Dedicated LE / BE
     ************************************************************************/

    /* Should be 0x1122 if properly converted from LE/BE */
    const uint8_t test_in_le[] = {0x22, 0x11};
    const uint8_t test_in_be[] = {0x11, 0x22};

    length = sizeof(test_in_le) / sizeof(test_in_le[0]);

    test_in_ptr = (uint8_t *)test_in_le;

    testrun(ov_file_get_16(&out, &test_in_ptr, &length, OV_FILE_LITTLE_ENDIAN));
    testrun(0x1122 == out);
    testrun(test_in_ptr == (uint8_t *)(test_in_le + sizeof(uint16_t)));
    testrun(0 == length);

    length = sizeof(test_in_be) / sizeof(test_in_be[0]);

    test_in_ptr = (uint8_t *)test_in_be;

    testrun(ov_file_get_16(&out, &test_in_ptr, &length, OV_FILE_BIG_ENDIAN));
    testrun(0x1122 == out);
    testrun(test_in_ptr == (uint8_t *)(test_in_be + sizeof(uint16_t)));
    testrun(0 == length);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_file_get_32() {

    const uint32_t test_in[] = {0x11223344, 0x55667788};
    const uint32_t ref_swapped[] = {0x44332211, 0x88776655};

    uint8_t *test_in_ptr = (uint8_t *)test_in;

    uint32_t out = 0;
    size_t length = 4;

    testrun(!ov_file_get_32(0, 0, 0, OV_FILE_RAW));
    testrun(test_in_ptr == (uint8_t *)test_in);
    testrun(4 == length);

    testrun(!ov_file_get_32(&out, 0, 0, OV_FILE_RAW));
    testrun(test_in_ptr == (uint8_t *)test_in);
    testrun(4 == length);

    testrun(!ov_file_get_32(0, &test_in_ptr, 0, OV_FILE_RAW));
    testrun(test_in_ptr == (uint8_t *)test_in);
    testrun(4 == length);

    testrun(!ov_file_get_32(&out, &test_in_ptr, 0, OV_FILE_RAW));
    testrun(test_in_ptr == (uint8_t *)test_in);
    testrun(4 == length);

    length = 0;

    testrun(!ov_file_get_32(0, 0, &length, OV_FILE_RAW));
    testrun(test_in_ptr == (uint8_t *)test_in);
    testrun(0 == length);

    length = 4;

    testrun(!ov_file_get_32(0, 0, &length, OV_FILE_RAW));
    testrun(test_in_ptr == (uint8_t *)test_in);
    testrun(4 == length);

    testrun(!ov_file_get_32(&out, 0, &length, OV_FILE_RAW));
    testrun(test_in_ptr == (uint8_t *)test_in);
    testrun(4 == length);

    length = 0;
    testrun(!ov_file_get_32(&out, &test_in_ptr, &length, OV_FILE_RAW));
    testrun(test_in_ptr == (uint8_t *)test_in);
    testrun(0 == length);

    length = 1;
    testrun(!ov_file_get_32(&out, &test_in_ptr, &length, OV_FILE_RAW));
    testrun(test_in_ptr == (uint8_t *)test_in);
    testrun(1 == length);

    length = 2;
    testrun(!ov_file_get_32(&out, &test_in_ptr, &length, OV_FILE_RAW));
    testrun(test_in_ptr == (uint8_t *)test_in);
    testrun(2 == length);

    length = 3;
    testrun(!ov_file_get_32(&out, &test_in_ptr, &length, OV_FILE_RAW));
    testrun(test_in_ptr == (uint8_t *)test_in);
    testrun(3 == length);

    length = 8;
    testrun(ov_file_get_32(&out, &test_in_ptr, &length, OV_FILE_RAW));
    testrun(test_in[0] == out);
    testrun((uint32_t *)test_in_ptr == test_in + 1);
    testrun(4 == length);

    testrun(ov_file_get_32(&out, &test_in_ptr, &length, OV_FILE_RAW));
    testrun(test_in[1] == out);
    testrun((uint32_t *)test_in_ptr == test_in + 2);
    testrun(0 == length);

    /**************************************************************************
     *                                 Swap bytes
     **************************************************************************/

    test_in_ptr = (uint8_t *)test_in;

    out = 0;
    length = 4;

    testrun(!ov_file_get_32(0, 0, 0, OV_FILE_SWAP_BYTES));
    testrun(test_in_ptr == (uint8_t *)test_in);
    testrun(4 == length);

    testrun(!ov_file_get_32(&out, 0, 0, OV_FILE_SWAP_BYTES));
    testrun(test_in_ptr == (uint8_t *)test_in);
    testrun(4 == length);

    testrun(!ov_file_get_32(0, &test_in_ptr, 0, OV_FILE_SWAP_BYTES));
    testrun(test_in_ptr == (uint8_t *)test_in);
    testrun(4 == length);

    testrun(!ov_file_get_32(&out, &test_in_ptr, 0, OV_FILE_SWAP_BYTES));
    testrun(test_in_ptr == (uint8_t *)test_in);
    testrun(4 == length);

    length = 0;
    testrun(!ov_file_get_32(0, 0, &length, OV_FILE_SWAP_BYTES));
    testrun(test_in_ptr == (uint8_t *)test_in);
    testrun(0 == length);

    length = 4;

    testrun(!ov_file_get_32(0, 0, &length, OV_FILE_SWAP_BYTES));
    testrun(test_in_ptr == (uint8_t *)test_in);
    testrun(4 == length);

    length = 0;
    testrun(!ov_file_get_32(&out, 0, &length, OV_FILE_SWAP_BYTES));
    testrun(test_in_ptr == (uint8_t *)test_in);
    testrun(0 == length);

    length = 4;
    testrun(!ov_file_get_32(&out, 0, &length, OV_FILE_SWAP_BYTES));
    testrun(test_in_ptr == (uint8_t *)test_in);
    testrun(4 == length);

    length = 0;
    testrun(!ov_file_get_32(&out, &test_in_ptr, &length, OV_FILE_SWAP_BYTES));
    testrun(test_in_ptr == (uint8_t *)test_in);
    testrun(0 == length);

    length = 1;
    testrun(!ov_file_get_32(&out, &test_in_ptr, &length, OV_FILE_SWAP_BYTES));
    testrun(test_in_ptr == (uint8_t *)test_in);
    testrun(1 == length);

    length = 2;
    testrun(!ov_file_get_32(&out, &test_in_ptr, &length, OV_FILE_SWAP_BYTES));
    testrun(test_in_ptr == (uint8_t *)test_in);
    testrun(2 == length);

    length = 3;
    testrun(!ov_file_get_32(&out, &test_in_ptr, &length, OV_FILE_SWAP_BYTES));
    testrun(test_in_ptr == (uint8_t *)test_in);
    testrun(3 == length);

    length = 8;
    testrun(ov_file_get_32(&out, &test_in_ptr, &length, OV_FILE_SWAP_BYTES));
    testrun(ref_swapped[0] == out);
    testrun(test_in_ptr == (uint8_t *)(test_in + 1));
    testrun(4 == length);

    testrun(ov_file_get_32(&out, &test_in_ptr, &length, OV_FILE_SWAP_BYTES));
    testrun(ref_swapped[1] == out);
    testrun(test_in_ptr == (uint8_t *)(test_in + 2));
    testrun(0 == length);

    /*************************************************************************
                               Dedicated LE / BE
     ************************************************************************/

    /* Should be 0x11223344 if properly converted from LE/BE */
    const uint8_t test_in_le[] = {0x44, 0x33, 0x22, 0x11};
    const uint8_t test_in_be[] = {0x11, 0x22, 0x33, 0x44};

    length = sizeof(test_in_le) / sizeof(test_in_le[0]);

    test_in_ptr = (uint8_t *)test_in_le;

    testrun(ov_file_get_32(&out, &test_in_ptr, &length, OV_FILE_LITTLE_ENDIAN));
    testrun(0x11223344 == out);
    testrun(test_in_ptr == (uint8_t *)(test_in_le + sizeof(uint32_t)));
    testrun(0 == length);

    length = sizeof(test_in_be) / sizeof(test_in_be[0]);

    test_in_ptr = (uint8_t *)test_in_be;

    testrun(ov_file_get_32(&out, &test_in_ptr, &length, OV_FILE_BIG_ENDIAN));
    testrun(0x11223344 == out);
    testrun(test_in_ptr == (uint8_t *)(test_in_be + sizeof(uint32_t)));
    testrun(0 == length);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_file_write_16() {

    uint8_t ref[2] = {0x12, 0xab};
    uint8_t swap[2] = {0xab, 0x12};
    uint8_t le[2] = {0x12, 0xab};
    uint8_t be[2] = {0xab, 0x12};

    uint16_t test_val = 0;
    test_val = *(uint16_t *)ref;

    uint16_t output[2 + 2 + 2 + 2] = {0};
    size_t output_len = sizeof(output);
    uint8_t *out_ptr = (uint8_t *)output;

    testrun(!ov_file_write_16(0, 0, 0));
    testrun(!ov_file_write_16(&out_ptr, 0, 0));
    testrun(!ov_file_write_16(0, &output_len, 0));

    testrun(ov_file_write_16(&out_ptr, &output_len, test_val));
    testrun(output_len = sizeof(output) - sizeof(uint16_t));
    testrun(out_ptr == (uint8_t *)(output + 1));
    testrun(test_val == output[0]);

    testrun(ov_file_write_16(&out_ptr, &output_len, OV_SWAP_16(test_val)));
    testrun(output_len = sizeof(output) - 2 * sizeof(uint16_t));
    testrun(out_ptr == (uint8_t *)(output + 2));
    testrun(0 == memcmp(swap, output + 1, sizeof(uint16_t)));

    testrun(ov_file_write_16(&out_ptr, &output_len, OV_H16TOLE(test_val)));
    testrun(output_len = sizeof(output) - 3 * sizeof(uint16_t));
    testrun(out_ptr == (uint8_t *)(output + 3));
    testrun(0 == memcmp(le, output + 2, sizeof(uint16_t)));

    testrun(ov_file_write_16(&out_ptr, &output_len, OV_H16TOBE(test_val)));
    testrun(output_len = sizeof(output) - 4 * sizeof(uint16_t));
    testrun(out_ptr == (uint8_t *)(output + 4));
    testrun(0 == memcmp(be, output + 3, sizeof(uint16_t)));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_file_write_32() {

    uint8_t ref[4] = {0x12, 0xab, 0x34, 0xcd};
    uint8_t swap[4] = {0xcd, 0x34, 0xab, 0x12};
    uint8_t le[4] = {0x12, 0xab, 0x34, 0xcd};
    uint8_t be[4] = {0xcd, 0x34, 0xab, 0x12};

    uint32_t test_val = 0;
    test_val = *(uint32_t *)ref;

    uint32_t output[4 + 4 + 4 + 4] = {0};
    size_t output_len = sizeof(output);
    uint8_t *out_ptr = (uint8_t *)output;

    testrun(!ov_file_write_32(0, 0, 0));
    testrun(!ov_file_write_32(&out_ptr, 0, 0));
    testrun(!ov_file_write_32(0, &output_len, 0));

    testrun(ov_file_write_32(&out_ptr, &output_len, test_val));
    testrun(output_len = sizeof(output) - sizeof(uint32_t));
    testrun(out_ptr == (uint8_t *)(output + 1));
    testrun(test_val == output[0]);

    testrun(ov_file_write_32(&out_ptr, &output_len, OV_SWAP_32(test_val)));
    testrun(output_len = sizeof(output) - 2 * sizeof(uint32_t));
    testrun(out_ptr == (uint8_t *)(output + 2));
    testrun(0 == memcmp(swap, output + 1, sizeof(uint32_t)));

    testrun(ov_file_write_32(&out_ptr, &output_len, OV_H32TOLE(test_val)));
    testrun(output_len = sizeof(output) - 3 * sizeof(uint32_t));
    testrun(out_ptr == (uint8_t *)(output + 3));
    testrun(0 == memcmp(le, output + 2, sizeof(uint32_t)));

    testrun(ov_file_write_32(&out_ptr, &output_len, OV_H32TOBE(test_val)));
    testrun(output_len = sizeof(output) - 4 * sizeof(uint32_t));
    testrun(out_ptr == (uint8_t *)(output + 4));
    testrun(0 == memcmp(be, output + 3, sizeof(uint32_t)));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

OV_TEST_RUN("ov_file", test_ov_file_exists, test_ov_file_write,
            test_ov_file_read, test_ov_file_read_check_bytes,
            test_ov_file_get_16, test_ov_file_get_32, test_ov_file_write_16,
            test_ov_file_write_32);

/*----------------------------------------------------------------------------*/
