/***
        ------------------------------------------------------------------------

        Copyright (c) 2023 German Aerospace Center DLR e.V. (GSOC)

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

        @author         Michael J. Beer, DLR/GSOC

        ------------------------------------------------------------------------
*/
#define OV_VALUE_TEST
#include "ov_value_serde.c"
#include <ov_base/ov_value.h>
#include <ov_test/ov_test.h>
#include <sys/socket.h>

/*----------------------------------------------------------------------------*/

static int test_ov_value_serde_create() {

    ov_serde *serde = ov_value_serde_create();
    testrun(0 != serde);

    serde = ov_serde_free(serde);
    testrun(0 == serde);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_impl_add_raw() {

    ov_value_serde_enable_caching(2);
    ov_serde *serde = ov_value_serde_create();

    ov_result res = {0};

    testrun(OV_SERDE_ERROR == ov_serde_add_raw(0, 0, 0));
    testrun(OV_SERDE_PROGRESS == ov_serde_add_raw(serde, 0, 0));

    ov_buffer *input = ov_buffer_from_string("{\"anton\": 12}");
    testrun(0 != input);

    testrun(OV_SERDE_ERROR == ov_serde_add_raw(0, input, 0));
    testrun(OV_SERDE_PROGRESS == ov_serde_add_raw(serde, input, 0));

    testrun(OV_SERDE_ERROR == ov_serde_add_raw(0, 0, &res));
    testrun(OV_ERROR_NOERROR != res.error_code);
    testrun(0 != res.message);

    ov_result_clear(&res);

    testrun(OV_SERDE_PROGRESS == ov_serde_add_raw(serde, 0, &res));
    testrun(OV_ERROR_NOERROR == res.error_code);
    testrun(0 == res.message);

    testrun(OV_SERDE_PROGRESS == ov_serde_add_raw(serde, input, &res));
    testrun(OV_ERROR_NOERROR == res.error_code);
    testrun(0 == res.message);

    serde = ov_serde_free(serde);
    testrun(0 == serde);

    input = ov_buffer_free(input);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static bool no_more_data(ov_serde_data datum) {

    return (OV_SERDE_DATA_NO_MORE.data_type == datum.data_type) &&
           (OV_SERDE_DATA_NO_MORE.data == datum.data);
}

/*----------------------------------------------------------------------------*/

static bool add_raw(ov_serde *serde, char const *datum) {

    ov_buffer *buf = ov_buffer_from_string(datum);

    ov_result res = {0};
    ov_serde_add_raw(serde, buf, &res);

    buf = ov_buffer_free(buf);

    return OV_ERROR_NOERROR == res.error_code;
}

/*----------------------------------------------------------------------------*/

static bool next_datum_is(ov_serde *serde, ov_value const *ref) {

    ov_result res = {0};

    ov_serde_data data = ov_serde_pop_datum(serde, &res);

    if (OV_ERROR_NOERROR != res.error_code) {
        TEST_ASSERT(0 == data.data);
        return false;
    } else {

        bool ok = (OV_VALUE_SERDE_TYPE == data.data_type) &&
                  ov_value_match(ref, data.data);
        data.data = ov_value_free(data.data);

        return ok;
    }
}

/*----------------------------------------------------------------------------*/

static bool next_datum_is_number(ov_serde *serde, double ref) {

    ov_value *ref_value = ov_value_number(ref);
    bool ok = next_datum_is(serde, ref_value);
    ref_value = ov_value_free(ref_value);

    return ok;
}

/*----------------------------------------------------------------------------*/

static int test_impl_pop_datum() {

    testrun(no_more_data(ov_serde_pop_datum(0, 0)));

    ov_result res = {0};

    testrun(no_more_data(ov_serde_pop_datum(0, &res)));
    testrun(OV_ERROR_NOERROR != res.error_code);

    ov_serde *serde = ov_value_serde_create();
    testrun(0 != serde);

    ov_result_clear(&res);

    testrun(no_more_data(ov_serde_pop_datum(serde, 0)));
    testrun(OV_ERROR_NOERROR == res.error_code);

    add_raw(serde, "   {\"adolf\": \"bedolf\"}");
    add_raw(serde, " 1");
    add_raw(serde, "   {      \"adolf\"   :  \"bedolf\"   }   ");
    add_raw(serde, "2 ");
    add_raw(serde, "   {\"adolf\":");
    add_raw(serde, "\"bedolf\"   } ");
    add_raw(serde, "3");
    add_raw(serde, "[]");
    add_raw(serde, "   4   ");
    add_raw(serde, "[");
    add_raw(serde, "]");
    add_raw(serde, "\t5\t");
    add_raw(serde, "\t[");

    ov_value *adolf = OBJECT(0, PAIR("adolf", ov_value_string("bedolf")));
    testrun(0 != adolf);

    ov_value *empty_list = ov_value_list(0);
    testrun(0 != empty_list);

    ov_value *list_5_6 = ov_value_list(ov_value_number(5), ov_value_number(6));
    testrun(0 != list_5_6);

    testrun(next_datum_is(serde, adolf));
    testrun(next_datum_is_number(serde, 1));
    testrun(next_datum_is(serde, adolf));
    testrun(next_datum_is_number(serde, 2));
    testrun(next_datum_is(serde, adolf));
    testrun(next_datum_is_number(serde, 3));
    testrun(next_datum_is(serde, empty_list));
    testrun(next_datum_is_number(serde, 4));
    testrun(next_datum_is(serde, empty_list));
    testrun(next_datum_is_number(serde, 5));

    testrun(no_more_data(ov_serde_pop_datum(serde, 0)));

    add_raw(serde, "5,6] ");
    testrun(next_datum_is(serde, list_5_6));

    // check proper removal of remaining parsed but not yet popped values
    add_raw(serde, "       [ ");
    add_raw(serde, "][   1   ,\t2     , 3  \t ,4][1,2,3,4,5]");

    testrun(next_datum_is(serde, empty_list));

    // Clean up

    adolf = ov_value_free(adolf);
    testrun(0 == adolf);

    empty_list = ov_value_free(empty_list);
    testrun(0 == empty_list);

    list_5_6 = ov_value_free(list_5_6);
    testrun(0 == list_5_6);

    serde = ov_serde_free(serde);
    testrun(0 == serde);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_impl_clear_buffer() { return testrun_log_success(); }

/*----------------------------------------------------------------------------*/

static ov_value *read_value_using_buffer(ov_serde *serde, int shandle,
                                         ov_buffer *buf) {

    ssize_t read_bytes = recv(shandle, buf->start, buf->capacity, 0);

    if (0 > read_bytes) {
        testrun_log_error("Reading from %i failed", shandle);
        return 0;
    } else {

        buf->length = (size_t)read_bytes;
        ov_serde_add_raw(serde, buf, 0);
        ov_serde_data data = ov_serde_pop_datum(serde, 0);

        if (OV_VALUE_SERDE_TYPE != data.data_type) {
            testrun_log_error("Read wrong data type");
            return 0;
        } else {
            return data.data;
        }
    }
}

/*----------------------------------------------------------------------------*/

ov_value *read_value(ov_serde *serde, int shandle) {

    ov_buffer *buf = ov_buffer_create(500);

    ov_value *value = read_value_using_buffer(serde, shandle, buf);

    buf = ov_buffer_free(buf);

    return value;
}

/*----------------------------------------------------------------------------*/

static int test_impl_serialize() {

    ov_serde_data data = {0};
    ov_result res = {0};

    testrun(!ov_serde_serialize(0, -1, data, 0));
    testrun(!ov_serde_serialize(0, -1, data, &res));

    testrun(OV_ERROR_NOERROR != res.error_code);
    ov_result_clear(&res);

    ov_serde *serde = ov_value_serde_create();
    testrun(0 != serde);

    testrun(!ov_serde_serialize(serde, -1, data, 0));
    testrun(!ov_serde_serialize(serde, -1, data, &res));

    testrun(OV_ERROR_NOERROR != res.error_code);
    ov_result_clear(&res);

    int spair[2] = {0};
    testrun(0 == socketpair(AF_UNIX, SOCK_STREAM, 0, spair));

    testrun(!ov_serde_serialize(0, spair[1], data, 0));
    testrun(!ov_serde_serialize(0, spair[1], data, &res));

    testrun(OV_ERROR_NOERROR != res.error_code);
    ov_result_clear(&res);

    // Data type is wrong / no data at all

    testrun(!ov_serde_serialize(serde, spair[1], data, 0));
    testrun(!ov_serde_serialize(serde, spair[1], data, &res));

    testrun(OV_ERROR_NOERROR != res.error_code);
    ov_result_clear(&res);

    // No data
    data.data_type = OV_VALUE_SERDE_TYPE;

    testrun(!ov_serde_serialize(serde, spair[1], data, 0));
    testrun(!ov_serde_serialize(serde, spair[1], data, &res));

    testrun(OV_ERROR_NOERROR != res.error_code);
    ov_result_clear(&res);

    data.data = ov_value_list(ov_value_number(1), ov_value_number(2),
                              ov_value_number(3));
    testrun(0 != data.data);

    testrun(ov_serde_serialize(serde, spair[1], data, 0));

    ov_value *recv_value = read_value(serde, spair[0]);
    testrun(ov_value_match(recv_value, data.data));

    recv_value = ov_value_free(recv_value);

    data.data = ov_value_free(data.data);
    testrun(0 == data.data);

    serde = ov_serde_free(serde);
    testrun(0 == serde);

    close(spair[0]);
    close(spair[1]);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_impl_free() {

    ov_serde *serde = ov_value_serde_create();
    testrun(0 != serde);

    serde = ov_serde_free(serde);
    testrun(0 == serde);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int cleanup() {

    ov_registered_cache_free_all();
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

OV_TEST_RUN("ov_value_serde", test_ov_value_serde_create, test_impl_add_raw,
            test_impl_pop_datum, test_impl_clear_buffer, test_impl_serialize,
            test_impl_free, cleanup);

/*----------------------------------------------------------------------------*/
