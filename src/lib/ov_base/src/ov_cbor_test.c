/***
        ------------------------------------------------------------------------

        Copyright (c) 2025 German Aerospace Center DLR e.V. (GSOC)

        Licensed under the Apache License, Version 2.0 (the "License");
        you may not use this file except in compliance with the License.
        You may obtain a copy of the License at

                http://www.apache.org/licenses/LICENSE-2.0

        Unless required by applicable law or agreed to in writing, software
        distributed under the License is distributed on an "AS IS" BASIS,
        WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
        See the License for the specific language governing permissions and
        limitations under the License.

        This file is part of the opendtn project. https://opendtn.com

        ------------------------------------------------------------------------
*//**
        @file           ov_cbor_test.c
        @author         Töpfer, Markus

        @date           2025-12-12


        ------------------------------------------------------------------------
*/
#include "ov_cbor.c"
#include <ov_test/testrun.h>

#include <ov_base/ov_random.h>
#include <ov_base/ov_utf8.h>

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

int test_cbor_clear() {

    ov_cbor *self = ov_cbor_create(ov_CBOR_UNDEF);
    testrun(cbor_clear(self));
    testrun(self->type == ov_CBOR_UNDEF);
    self = ov_cbor_free(self);

    self = ov_cbor_create(ov_CBOR_FALSE);
    testrun(cbor_clear(self));
    testrun(self->type == ov_CBOR_UNDEF);
    testrun(self->data == NULL);
    testrun(self->nbr_uint == 0);
    self = ov_cbor_free(self);

    self = ov_cbor_create(ov_CBOR_TRUE);
    testrun(cbor_clear(self));
    testrun(self->type == ov_CBOR_UNDEF);
    testrun(self->data == NULL);
    testrun(self->nbr_uint == 0);
    self = ov_cbor_free(self);

    self = ov_cbor_create(ov_CBOR_NULL);
    testrun(cbor_clear(self));
    testrun(self->type == ov_CBOR_UNDEF);
    testrun(self->data == NULL);
    testrun(self->nbr_uint == 0);
    self = ov_cbor_free(self);

    self = ov_cbor_create(ov_CBOR_UINT64);
    self->nbr_uint = 1234;
    testrun(cbor_clear(self));
    testrun(self->type == ov_CBOR_UNDEF);
    testrun(self->data == NULL);
    testrun(self->nbr_uint == 0);
    self = ov_cbor_free(self);

    self = ov_cbor_create(ov_CBOR_INT64);
    self->nbr_int = 1234;
    testrun(cbor_clear(self));
    testrun(self->type == ov_CBOR_UNDEF);
    testrun(self->data == NULL);
    testrun(self->nbr_uint == 0);
    self = ov_cbor_free(self);

    self = ov_cbor_create(ov_CBOR_STRING);
    self->string = ov_string_dup("test");
    testrun(cbor_clear(self));
    testrun(self->type == ov_CBOR_UNDEF);
    testrun(self->data == NULL);
    testrun(self->nbr_uint == 0);
    self = ov_cbor_free(self);

    self = ov_cbor_create(ov_CBOR_UTF8);
    self->string = ov_string_dup("test");
    testrun(cbor_clear(self));
    testrun(self->type == ov_CBOR_UNDEF);
    testrun(self->data == NULL);
    testrun(self->nbr_uint == 0);
    self = ov_cbor_free(self);

    self = ov_cbor_create(ov_CBOR_ARRAY);
    self->data =
        ov_linked_list_create((ov_list_config){.item.free = cbor_free});
    testrun(cbor_clear(self));
    testrun(self->type == ov_CBOR_UNDEF);
    testrun(self->data == NULL);
    testrun(self->nbr_uint == 0);
    self = ov_cbor_free(self);

    self = ov_cbor_create(ov_CBOR_MAP);
    self->data = ov_dict_create(ov_cbor_dict_config(255));
    testrun(cbor_clear(self));
    testrun(self->type == ov_CBOR_UNDEF);
    testrun(self->data == NULL);
    testrun(self->nbr_uint == 0);
    self = ov_cbor_free(self);

    self = ov_cbor_create(ov_CBOR_DATE_TIME);
    self->string = ov_string_dup("timestamp");
    testrun(cbor_clear(self));
    testrun(self->type == ov_CBOR_UNDEF);
    testrun(self->data == NULL);
    testrun(self->nbr_uint == 0);
    self = ov_cbor_free(self);

    self = ov_cbor_create(ov_CBOR_DATE_TIME_EPOCH);
    self->nbr_uint = 12345;
    testrun(cbor_clear(self));
    testrun(self->type == ov_CBOR_UNDEF);
    testrun(self->data == NULL);
    testrun(self->nbr_uint == 0);
    self = ov_cbor_free(self);

    self = ov_cbor_create(ov_CBOR_UBIGNUM);
    self->string = ov_string_dup("ov_CBOR_UBIGNUM");
    testrun(cbor_clear(self));
    testrun(self->type == ov_CBOR_UNDEF);
    testrun(self->data == NULL);
    testrun(self->nbr_uint == 0);
    self = ov_cbor_free(self);

    self = ov_cbor_create(ov_CBOR_IBIGNUM);
    self->string = ov_string_dup("ov_CBOR_IBIGNUM");
    testrun(cbor_clear(self));
    testrun(self->type == ov_CBOR_UNDEF);
    testrun(self->data == NULL);
    testrun(self->nbr_uint == 0);
    self = ov_cbor_free(self);

    self = ov_cbor_create(ov_CBOR_DEC_FRACTION);
    self->data =
        ov_linked_list_create((ov_list_config){.item.free = cbor_free});
    testrun(cbor_clear(self));
    testrun(self->type == ov_CBOR_UNDEF);
    testrun(self->data == NULL);
    testrun(self->nbr_uint == 0);
    self = ov_cbor_free(self);

    self = ov_cbor_create(ov_CBOR_BIGFLOAT);
    self->data =
        ov_linked_list_create((ov_list_config){.item.free = cbor_free});
    testrun(cbor_clear(self));
    testrun(self->type == ov_CBOR_UNDEF);
    testrun(self->data == NULL);
    testrun(self->nbr_uint == 0);
    self = ov_cbor_free(self);

    self = ov_cbor_create(ov_CBOR_TAG);
    self->nbr_uint = 1234;
    self->data = ov_cbor_create(ov_CBOR_NULL);
    testrun(cbor_clear(self));
    testrun(self->type == ov_CBOR_UNDEF);
    testrun(self->data == NULL);
    testrun(self->nbr_uint == 0);
    self = ov_cbor_free(self);

    self = ov_cbor_create(ov_CBOR_SIMPLE);
    self->nbr_uint = 1234;
    testrun(cbor_clear(self));
    testrun(self->type == ov_CBOR_UNDEF);
    testrun(self->data == NULL);
    testrun(self->nbr_uint == 0);
    self = ov_cbor_free(self);

    self = ov_cbor_create(ov_CBOR_FLOAT);
    self->nbr_float = 1.2;
    testrun(cbor_clear(self));
    testrun(self->type == ov_CBOR_UNDEF);
    testrun(self->data == NULL);
    testrun(self->nbr_uint == 0);
    self = ov_cbor_free(self);

    self = ov_cbor_create(ov_CBOR_DOUBLE);
    self->nbr_double = 1.2e4;
    testrun(cbor_clear(self));
    testrun(self->type == ov_CBOR_UNDEF);
    testrun(self->data == NULL);
    testrun(self->nbr_uint == 0);
    self = ov_cbor_free(self);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_cbor_free() {

    ov_cbor *self = ov_cbor_create(ov_CBOR_UNDEF);
    testrun(cbor_clear(self));
    testrun(self->type == ov_CBOR_UNDEF);
    testrun(NULL == cbor_free(self));

    self = ov_cbor_create(ov_CBOR_FALSE);
    testrun(NULL == cbor_free(self));

    self = ov_cbor_create(ov_CBOR_TRUE);
    testrun(NULL == cbor_free(self));

    self = ov_cbor_create(ov_CBOR_NULL);
    testrun(NULL == cbor_free(self));

    self = ov_cbor_create(ov_CBOR_UINT64);
    self->nbr_uint = 1234;
    testrun(NULL == cbor_free(self));

    self = ov_cbor_create(ov_CBOR_INT64);
    self->nbr_int = 1234;
    testrun(NULL == cbor_free(self));

    self = ov_cbor_create(ov_CBOR_STRING);
    self->string = ov_string_dup("test");
    testrun(NULL == cbor_free(self));

    self = ov_cbor_create(ov_CBOR_UTF8);
    self->string = ov_string_dup("test");
    testrun(NULL == cbor_free(self));

    self = ov_cbor_create(ov_CBOR_ARRAY);
    self->data =
        ov_linked_list_create((ov_list_config){.item.free = cbor_free});
    testrun(NULL == cbor_free(self));

    self = ov_cbor_create(ov_CBOR_MAP);
    self->data = ov_dict_create(ov_cbor_dict_config(255));
    testrun(NULL == cbor_free(self));

    self = ov_cbor_create(ov_CBOR_DATE_TIME);
    self->string = ov_string_dup("timestamp");
    testrun(NULL == cbor_free(self));

    self = ov_cbor_create(ov_CBOR_DATE_TIME_EPOCH);
    self->nbr_uint = 12345;
    testrun(NULL == cbor_free(self));

    self = ov_cbor_create(ov_CBOR_UBIGNUM);
    self->string = ov_string_dup("ov_CBOR_UBIGNUM");
    testrun(NULL == cbor_free(self));

    self = ov_cbor_create(ov_CBOR_IBIGNUM);
    self->string = ov_string_dup("ov_CBOR_IBIGNUM");
    testrun(NULL == cbor_free(self));

    self = ov_cbor_create(ov_CBOR_DEC_FRACTION);
    self->data =
        ov_linked_list_create((ov_list_config){.item.free = cbor_free});
    testrun(NULL == cbor_free(self));

    self = ov_cbor_create(ov_CBOR_BIGFLOAT);
    self->data =
        ov_linked_list_create((ov_list_config){.item.free = cbor_free});
    testrun(NULL == cbor_free(self));

    self = ov_cbor_create(ov_CBOR_TAG);
    self->nbr_uint = 1234;
    self->data = ov_cbor_create(ov_CBOR_FALSE);
    testrun(NULL == cbor_free(self));

    self = ov_cbor_create(ov_CBOR_SIMPLE);
    self->nbr_uint = 1234;
    testrun(NULL == cbor_free(self));

    self = ov_cbor_create(ov_CBOR_FLOAT);
    self->nbr_float = 1.2;
    testrun(NULL == cbor_free(self));

    self = ov_cbor_create(ov_CBOR_DOUBLE);
    self->nbr_double = 1.2e4;
    testrun(NULL == cbor_free(self));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_cbor_copy() {

    ov_cbor *copy = NULL;
    ov_cbor *self = ov_cbor_create(ov_CBOR_UNDEF);

    testrun(cbor_copy((void **)&copy, self));
    testrun(copy);
    testrun(copy->type == ov_CBOR_UNDEF);
    self = cbor_free(self);
    copy = cbor_free(copy);

    self = ov_cbor_create(ov_CBOR_FALSE);
    testrun(cbor_copy((void **)&copy, self));
    testrun(copy);
    testrun(copy->type == ov_CBOR_FALSE);
    self = cbor_free(self);
    copy = cbor_free(copy);

    self = ov_cbor_create(ov_CBOR_TRUE);
    testrun(cbor_copy((void **)&copy, self));
    testrun(copy);
    testrun(copy->type == ov_CBOR_TRUE);
    self = cbor_free(self);
    copy = cbor_free(copy);

    self = ov_cbor_create(ov_CBOR_NULL);
    testrun(cbor_copy((void **)&copy, self));
    testrun(copy);
    testrun(copy->type == ov_CBOR_NULL);
    self = cbor_free(self);
    copy = cbor_free(copy);

    self = ov_cbor_create(ov_CBOR_UINT64);
    self->nbr_uint = 1234;
    testrun(cbor_copy((void **)&copy, self));
    testrun(copy);
    testrun(copy->type == ov_CBOR_UINT64);
    testrun(copy->nbr_uint == self->nbr_uint);
    self = cbor_free(self);
    copy = cbor_free(copy);

    self = ov_cbor_create(ov_CBOR_INT64);
    self->nbr_int = 1234;
    testrun(cbor_copy((void **)&copy, self));
    testrun(copy);
    testrun(copy->type == ov_CBOR_INT64);
    testrun(copy->nbr_uint == self->nbr_uint);
    self = cbor_free(self);
    copy = cbor_free(copy);

    self = ov_cbor_create(ov_CBOR_STRING);
    self->string = ov_string_dup("test");
    self->nbr_uint = strlen("test");
    testrun(cbor_copy((void **)&copy, self));
    testrun(copy);
    testrun(copy->type == ov_CBOR_STRING);
    testrun(copy->nbr_uint == self->nbr_uint);
    testrun(0 == ov_string_compare(self->string, copy->string));
    self = cbor_free(self);
    copy = cbor_free(copy);

    self = ov_cbor_create(ov_CBOR_UTF8);
    self->string = ov_string_dup("test");
    self->nbr_uint = strlen("test");
    testrun(cbor_copy((void **)&copy, self));
    testrun(copy);
    testrun(copy->type == ov_CBOR_UTF8);
    testrun(copy->nbr_uint == self->nbr_uint);
    testrun(0 == ov_string_compare(self->string, copy->string));
    self = cbor_free(self);
    copy = cbor_free(copy);

    self = ov_cbor_create(ov_CBOR_ARRAY);
    self->data = ov_linked_list_create(
        (ov_list_config){.item.free = cbor_free, .item.copy = cbor_copy});
    testrun(ov_list_push(self->data, ov_cbor_create(ov_CBOR_TRUE)));
    testrun(cbor_copy((void **)&copy, self));
    testrun(copy);
    testrun(copy->type == ov_CBOR_ARRAY);
    testrun(1 == ov_list_count(copy->data));
    self = cbor_free(self);
    copy = cbor_free(copy);

    self = ov_cbor_create(ov_CBOR_MAP);
    self->data = ov_dict_create(ov_cbor_dict_config(255));
    ov_cbor *key = ov_cbor_create(ov_CBOR_STRING);
    key->string = ov_string_dup("key");
    ov_cbor *val = ov_cbor_create(ov_CBOR_STRING);
    val->string = ov_string_dup("val");
    testrun(ov_dict_set(self->data, key, val, NULL));
    testrun(cbor_copy((void **)&copy, self));
    testrun(copy);
    testrun(copy->type == ov_CBOR_MAP);
    testrun(1 == ov_dict_count(copy->data));
    self = cbor_free(self);
    copy = cbor_free(copy);

    self = ov_cbor_create(ov_CBOR_DATE_TIME);
    self->string = ov_string_dup("timestamp");
    self->nbr_uint = strlen("timestamp");
    testrun(cbor_copy((void **)&copy, self));
    testrun(copy);
    testrun(copy->type == ov_CBOR_DATE_TIME);
    testrun(copy->nbr_uint == self->nbr_uint);
    testrun(0 == ov_string_compare(self->string, copy->string));
    self = cbor_free(self);
    copy = cbor_free(copy);

    self = ov_cbor_create(ov_CBOR_DATE_TIME_EPOCH);
    self->nbr_uint = 12345;
    testrun(cbor_copy((void **)&copy, self));
    testrun(copy);
    testrun(copy->type == ov_CBOR_DATE_TIME_EPOCH);
    testrun(copy->nbr_uint == self->nbr_uint);
    self = cbor_free(self);
    copy = cbor_free(copy);

    self = ov_cbor_create(ov_CBOR_UBIGNUM);
    self->string = ov_string_dup("ov_CBOR_UBIGNUM");
    self->nbr_uint = strlen("ov_CBOR_UBIGNUM");
    testrun(cbor_copy((void **)&copy, self));
    testrun(copy);
    testrun(copy->type == ov_CBOR_UBIGNUM);
    testrun(copy->nbr_uint == self->nbr_uint);
    testrun(0 == ov_string_compare(self->string, copy->string));
    self = cbor_free(self);
    copy = cbor_free(copy);

    self = ov_cbor_create(ov_CBOR_IBIGNUM);
    self->string = ov_string_dup("ov_CBOR_IBIGNUM");
    self->nbr_uint = strlen("ov_CBOR_IBIGNUM");
    testrun(cbor_copy((void **)&copy, self));
    testrun(copy);
    testrun(copy->type == ov_CBOR_IBIGNUM);
    testrun(copy->nbr_uint == self->nbr_uint);
    testrun(0 == ov_string_compare(self->string, copy->string));
    self = cbor_free(self);
    copy = cbor_free(copy);

    self = ov_cbor_create(ov_CBOR_DEC_FRACTION);
    self->data = ov_cbor_array();
    testrun(ov_cbor_array_push(self->data, ov_cbor_create(ov_CBOR_TRUE)));
    testrun(cbor_copy((void **)&copy, self));
    testrun(copy);
    testrun(copy->type == ov_CBOR_DEC_FRACTION);
    testrun(1 == ov_cbor_array_count(copy->data));
    self = cbor_free(self);
    copy = cbor_free(copy);

    self = ov_cbor_create(ov_CBOR_BIGFLOAT);
    self->data = ov_cbor_array();
    testrun(cbor_copy((void **)&copy, self));
    testrun(copy);
    testrun(copy->type == ov_CBOR_BIGFLOAT);
    testrun(copy->nbr_uint == self->nbr_uint);
    self = cbor_free(self);
    copy = cbor_free(copy);

    self = ov_cbor_create(ov_CBOR_TAG);
    self->nbr_uint = 1234;
    self->data = ov_cbor_create(ov_CBOR_NULL);
    testrun(cbor_copy((void **)&copy, self));
    testrun(copy);
    testrun(copy->type == ov_CBOR_TAG);
    testrun(copy->nbr_uint == self->nbr_uint);
    testrun(copy->data);
    self = cbor_free(self);
    copy = cbor_free(copy);

    self = ov_cbor_create(ov_CBOR_SIMPLE);
    self->nbr_uint = 1234;
    testrun(cbor_copy((void **)&copy, self));
    testrun(copy);
    testrun(copy->type == ov_CBOR_SIMPLE);
    testrun(copy->nbr_uint == self->nbr_uint);
    self = cbor_free(self);
    copy = cbor_free(copy);

    self = ov_cbor_create(ov_CBOR_FLOAT);
    self->nbr_float = 1.2;
    testrun(cbor_copy((void **)&copy, self));
    testrun(copy);
    testrun(copy->type == ov_CBOR_FLOAT);
    testrun(copy->nbr_float == self->nbr_float);
    self = cbor_free(self);
    copy = cbor_free(copy);

    self = ov_cbor_create(ov_CBOR_DOUBLE);
    self->nbr_double = 1.2e4;
    testrun(cbor_copy((void **)&copy, self));
    testrun(copy);
    testrun(copy->type == ov_CBOR_DOUBLE);
    testrun(copy->nbr_double == self->nbr_double);
    self = cbor_free(self);
    copy = cbor_free(copy);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_cbor_dump() {

    ov_cbor *self = ov_cbor_create(ov_CBOR_UNDEF);
    testrun(cbor_clear(self));
    testrun(self->type == ov_CBOR_UNDEF);
    testrun(cbor_dump(stdout, self));
    testrun(NULL == cbor_free(self));

    self = ov_cbor_create(ov_CBOR_FALSE);
    testrun(cbor_dump(stdout, self));
    testrun(NULL == cbor_free(self));

    self = ov_cbor_create(ov_CBOR_TRUE);
    testrun(cbor_dump(stdout, self));
    testrun(NULL == cbor_free(self));

    self = ov_cbor_create(ov_CBOR_NULL);
    testrun(cbor_dump(stdout, self));
    testrun(NULL == cbor_free(self));

    self = ov_cbor_create(ov_CBOR_UINT64);
    self->nbr_uint = 1234;
    testrun(cbor_dump(stdout, self));
    testrun(NULL == cbor_free(self));

    self = ov_cbor_create(ov_CBOR_INT64);
    self->nbr_int = 1234;
    testrun(cbor_dump(stdout, self));
    testrun(NULL == cbor_free(self));

    self = ov_cbor_create(ov_CBOR_STRING);
    self->string = ov_string_dup("test");
    testrun(cbor_dump(stdout, self));
    testrun(NULL == cbor_free(self));

    self = ov_cbor_create(ov_CBOR_UTF8);
    self->string = ov_string_dup("test");
    self->nbr_uint = 4;
    testrun(cbor_dump(stdout, self));
    testrun(NULL == cbor_free(self));

    self = ov_cbor_create(ov_CBOR_ARRAY);
    self->data =
        ov_linked_list_create((ov_list_config){.item.free = cbor_free});
    testrun(cbor_dump(stdout, self));
    testrun(NULL == cbor_free(self));

    self = ov_cbor_create(ov_CBOR_MAP);
    self->data = ov_dict_create(ov_cbor_dict_config(255));
    testrun(cbor_dump(stdout, self));
    testrun(NULL == cbor_free(self));

    self = ov_cbor_create(ov_CBOR_DATE_TIME);
    self->string = ov_string_dup("timestamp");
    self->nbr_uint = strlen("timestamp");
    testrun(cbor_dump(stdout, self));
    testrun(NULL == cbor_free(self));

    self = ov_cbor_create(ov_CBOR_DATE_TIME_EPOCH);
    self->nbr_uint = 12345;
    testrun(cbor_dump(stdout, self));
    testrun(NULL == cbor_free(self));

    self = ov_cbor_create(ov_CBOR_UBIGNUM);
    self->string = ov_string_dup("ov_CBOR_UBIGNUM");
    self->nbr_uint = strlen("ov_CBOR_UBIGNUM");
    testrun(cbor_dump(stdout, self));
    testrun(NULL == cbor_free(self));

    self = ov_cbor_create(ov_CBOR_IBIGNUM);
    self->string = ov_string_dup("ov_CBOR_IBIGNUM");
    self->nbr_uint = strlen("ov_CBOR_IBIGNUM");
    testrun(cbor_dump(stdout, self));
    testrun(NULL == cbor_free(self));

    self = ov_cbor_create(ov_CBOR_DEC_FRACTION);
    self->data =
        ov_linked_list_create((ov_list_config){.item.free = cbor_free});
    testrun(cbor_dump(stdout, self));
    testrun(NULL == cbor_free(self));

    self = ov_cbor_create(ov_CBOR_BIGFLOAT);
    self->data =
        ov_linked_list_create((ov_list_config){.item.free = cbor_free});
    testrun(cbor_dump(stdout, self));
    testrun(NULL == cbor_free(self));

    self = ov_cbor_create(ov_CBOR_TAG);
    self->nbr_uint = 1234;
    testrun(cbor_dump(stdout, self));
    testrun(NULL == cbor_free(self));

    self = ov_cbor_create(ov_CBOR_SIMPLE);
    self->nbr_uint = 1234;
    testrun(cbor_dump(stdout, self));
    testrun(NULL == cbor_free(self));

    self = ov_cbor_create(ov_CBOR_FLOAT);
    self->nbr_float = 1.2;
    testrun(cbor_dump(stdout, self));
    testrun(NULL == cbor_free(self));

    self = ov_cbor_create(ov_CBOR_DOUBLE);
    self->nbr_double = 1.2e4;
    testrun(cbor_dump(stdout, self));
    testrun(NULL == cbor_free(self));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_cbor_hash() {

    ov_cbor *self = ov_cbor_create(ov_CBOR_UNDEF);
    testrun(cbor_clear(self));
    testrun(self->type == ov_CBOR_UNDEF);
    uint64_t result = cbor_hash(self);
    testrun(result == 0);
    testrun(NULL == cbor_free(self));

    self = ov_cbor_create(ov_CBOR_FALSE);
    result = cbor_hash(self);
    testrun(result == 0);
    testrun(NULL == cbor_free(self));

    self = ov_cbor_create(ov_CBOR_TRUE);
    result = cbor_hash(self);
    testrun(result == 0);
    testrun(NULL == cbor_free(self));

    self = ov_cbor_create(ov_CBOR_NULL);
    result = cbor_hash(self);
    testrun(result == 0);
    testrun(NULL == cbor_free(self));

    self = ov_cbor_create(ov_CBOR_UINT64);
    self->nbr_uint = 1234;
    result = cbor_hash(self);
    testrun(result == 1234);
    testrun(NULL == cbor_free(self));

    self = ov_cbor_create(ov_CBOR_INT64);
    self->nbr_int = 1234;
    result = cbor_hash(self);
    testrun(result == 1234);
    testrun(NULL == cbor_free(self));

    self = ov_cbor_create(ov_CBOR_STRING);
    self->string = ov_string_dup("test");
    self->nbr_uint = strlen("test");
    result = cbor_hash(self);
    testrun(result != 0);
    testrun(NULL == cbor_free(self));

    self = ov_cbor_create(ov_CBOR_UTF8);
    self->string = ov_string_dup("test");
    self->nbr_uint = strlen("test");
    result = cbor_hash(self);
    testrun(result != 0);
    testrun(NULL == cbor_free(self));

    self = ov_cbor_create(ov_CBOR_ARRAY);
    self->data =
        ov_linked_list_create((ov_list_config){.item.free = cbor_free});
    result = cbor_hash(self);
    testrun(result != 0);
    testrun(NULL == cbor_free(self));

    self = ov_cbor_create(ov_CBOR_MAP);
    self->data = ov_dict_create(ov_cbor_dict_config(255));
    result = cbor_hash(self);
    testrun(result != 0);
    testrun(NULL == cbor_free(self));

    self = ov_cbor_create(ov_CBOR_DATE_TIME);
    self->string = ov_string_dup("timestamp");
    self->nbr_uint = strlen("timestamp");
    result = cbor_hash(self);
    testrun(result != 0);
    testrun(NULL == cbor_free(self));

    self = ov_cbor_create(ov_CBOR_DATE_TIME_EPOCH);
    self->nbr_uint = 12345;
    result = cbor_hash(self);
    testrun(result != 0);
    testrun(NULL == cbor_free(self));

    self = ov_cbor_create(ov_CBOR_UBIGNUM);
    self->string = ov_string_dup("ov_CBOR_UBIGNUM");
    self->nbr_uint = strlen("ov_CBOR_UBIGNUM");
    result = cbor_hash(self);
    testrun(result != 0);
    testrun(NULL == cbor_free(self));

    self = ov_cbor_create(ov_CBOR_IBIGNUM);
    self->string = ov_string_dup("ov_CBOR_IBIGNUM");
    self->nbr_uint = strlen("ov_CBOR_IBIGNUM");
    result = cbor_hash(self);
    testrun(result != 0);
    testrun(NULL == cbor_free(self));

    self = ov_cbor_create(ov_CBOR_DEC_FRACTION);
    self->data =
        ov_linked_list_create((ov_list_config){.item.free = cbor_free});
    result = cbor_hash(self);
    testrun(result != 0);
    testrun(NULL == cbor_free(self));

    self = ov_cbor_create(ov_CBOR_BIGFLOAT);
    self->data =
        ov_linked_list_create((ov_list_config){.item.free = cbor_free});
    result = cbor_hash(self);
    testrun(result != 0);
    testrun(NULL == cbor_free(self));

    self = ov_cbor_create(ov_CBOR_TAG);
    self->nbr_uint = 1234;
    result = cbor_hash(self);
    testrun(result != 0);
    testrun(NULL == cbor_free(self));

    self = ov_cbor_create(ov_CBOR_SIMPLE);
    self->nbr_uint = 1234;
    result = cbor_hash(self);
    testrun(result != 0);
    testrun(NULL == cbor_free(self));

    self = ov_cbor_create(ov_CBOR_FLOAT);
    self->nbr_float = 1.2;
    result = cbor_hash(self);
    testrun(result != 0);
    testrun(NULL == cbor_free(self));

    self = ov_cbor_create(ov_CBOR_DOUBLE);
    self->nbr_double = 1.2e4;
    result = cbor_hash(self);
    testrun(result != 0);
    testrun(NULL == cbor_free(self));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_cbor_match() {

    ov_cbor *one = ov_cbor_create(ov_CBOR_UNDEF);
    ov_cbor *two = ov_cbor_create(ov_CBOR_UNDEF);
    testrun(cbor_match(one, two));
    two = ov_cbor_free(two);
    two = ov_cbor_create(ov_CBOR_FALSE);
    testrun(!cbor_match(one, two));
    one = cbor_free(one);
    two = cbor_free(two);

    one = ov_cbor_create(ov_CBOR_FALSE);
    two = ov_cbor_create(ov_CBOR_FALSE);
    testrun(cbor_match(one, two));
    two = ov_cbor_free(two);
    two = ov_cbor_create(ov_CBOR_UNDEF);
    testrun(!cbor_match(one, two));
    one = cbor_free(one);
    two = cbor_free(two);

    one = ov_cbor_create(ov_CBOR_TRUE);
    two = ov_cbor_create(ov_CBOR_TRUE);
    testrun(cbor_match(one, two));
    two = ov_cbor_free(two);
    two = ov_cbor_create(ov_CBOR_UNDEF);
    testrun(!cbor_match(one, two));
    one = cbor_free(one);
    two = cbor_free(two);

    one = ov_cbor_create(ov_CBOR_NULL);
    two = ov_cbor_create(ov_CBOR_NULL);
    testrun(cbor_match(one, two));
    two = ov_cbor_free(two);
    two = ov_cbor_create(ov_CBOR_UNDEF);
    testrun(!cbor_match(one, two));
    one = cbor_free(one);
    two = cbor_free(two);

    one = ov_cbor_create(ov_CBOR_UINT64);
    two = ov_cbor_create(ov_CBOR_UINT64);
    one->nbr_uint = 123;
    two->nbr_uint = 123;
    testrun(cbor_match(one, two));
    two->nbr_uint = 1234;
    testrun(!cbor_match(one, two));
    two = ov_cbor_free(two);
    two = ov_cbor_create(ov_CBOR_UNDEF);
    testrun(!cbor_match(one, two));
    one = cbor_free(one);
    two = cbor_free(two);

    one = ov_cbor_create(ov_CBOR_INT64);
    two = ov_cbor_create(ov_CBOR_INT64);
    one->nbr_int = 123;
    two->nbr_int = 123;
    testrun(cbor_match(one, two));
    two->nbr_int = -123;
    testrun(!cbor_match(one, two));
    two = ov_cbor_free(two);
    two = ov_cbor_create(ov_CBOR_UNDEF);
    testrun(!cbor_match(one, two));
    one = cbor_free(one);
    two = cbor_free(two);

    one = ov_cbor_create(ov_CBOR_STRING);
    two = ov_cbor_create(ov_CBOR_STRING);
    one->string = ov_string_dup("test");
    one->nbr_uint = strlen("test");
    two->string = ov_string_dup("test");
    two->nbr_uint = strlen("test");
    testrun(cbor_match(one, two));
    one->string = ov_data_pointer_free(one->string);
    one->string = ov_string_dup("tes");
    one->nbr_uint = strlen("tes");
    testrun(!cbor_match(one, two));
    two = ov_cbor_free(two);
    two = ov_cbor_create(ov_CBOR_UNDEF);
    testrun(!cbor_match(one, two));
    one = cbor_free(one);
    two = cbor_free(two);

    one = ov_cbor_create(ov_CBOR_UTF8);
    two = ov_cbor_create(ov_CBOR_UTF8);
    one->string = ov_string_dup("test");
    one->nbr_uint = strlen("test");
    two->string = ov_string_dup("test");
    two->nbr_uint = strlen("test");
    testrun(cbor_match(one, two));
    one->string = ov_data_pointer_free(one->string);
    one->string = ov_string_dup("tes");
    one->nbr_uint = strlen("tes");
    testrun(!cbor_match(one, two));
    two = ov_cbor_free(two);
    two = ov_cbor_create(ov_CBOR_UNDEF);
    testrun(!cbor_match(one, two));
    one = cbor_free(one);
    two = cbor_free(two);

    one = ov_cbor_create(ov_CBOR_ARRAY);
    two = ov_cbor_create(ov_CBOR_ARRAY);
    one->data =
        ov_linked_list_create((ov_list_config){.item.free = cbor_free});
    two->data =
        ov_linked_list_create((ov_list_config){.item.free = cbor_free});
    testrun(cbor_match(one, two));
    testrun(ov_list_push(one->data, ov_cbor_create(ov_CBOR_UNDEF)));
    testrun(!cbor_match(one, two));
    two = ov_cbor_free(two);
    two = ov_cbor_create(ov_CBOR_UNDEF);
    testrun(!cbor_match(one, two));
    one = cbor_free(one);
    two = cbor_free(two);

    one = ov_cbor_create(ov_CBOR_MAP);
    two = ov_cbor_create(ov_CBOR_MAP);
    one->data = ov_dict_create(ov_cbor_dict_config(255));
    two->data = ov_dict_create(ov_cbor_dict_config(255));
    testrun(cbor_match(one, two));
    ov_cbor *key = ov_cbor_create(ov_CBOR_UINT64);
    key->nbr_uint = 1234;
    ov_cbor *val = ov_cbor_create(ov_CBOR_UINT64);
    val->nbr_uint = 12345;
    testrun(ov_dict_set(one->data, key, val, NULL));
    testrun(!cbor_match(one, two));
    key = ov_cbor_create(ov_CBOR_UINT64);
    key->nbr_uint = 1234;
    val = ov_cbor_create(ov_CBOR_UINT64);
    val->nbr_uint = 12345;
    testrun(ov_dict_set(two->data, key, val, NULL));
    testrun(cbor_match(one, two));
    two = ov_cbor_free(two);
    two = ov_cbor_create(ov_CBOR_UNDEF);
    testrun(!cbor_match(one, two));
    one = cbor_free(one);
    two = cbor_free(two);

    one = ov_cbor_create(ov_CBOR_DATE_TIME);
    two = ov_cbor_create(ov_CBOR_DATE_TIME);
    one->string = ov_string_dup("timestamp");
    one->nbr_uint = strlen("timestamp");
    two->string = ov_string_dup("timestamp");
    two->nbr_uint = strlen("timestamp");
    testrun(cbor_match(one, two));
    one->string = ov_data_pointer_free(one->string);
    one->string = ov_string_dup("tes");
    one->nbr_uint = strlen("tes");
    testrun(!cbor_match(one, two));
    two = ov_cbor_free(two);
    two = ov_cbor_create(ov_CBOR_UNDEF);
    testrun(!cbor_match(one, two));
    one = cbor_free(one);
    two = cbor_free(two);

    one = ov_cbor_create(ov_CBOR_DATE_TIME_EPOCH);
    two = ov_cbor_create(ov_CBOR_DATE_TIME_EPOCH);
    one->nbr_uint = 12345;
    two->nbr_uint = 12345;
    testrun(cbor_match(one, two));
    one->nbr_uint = 123456;
    testrun(!cbor_match(one, two));
    two = ov_cbor_free(two);
    two = ov_cbor_create(ov_CBOR_UNDEF);
    testrun(!cbor_match(one, two));
    one = cbor_free(one);
    two = cbor_free(two);

    one = ov_cbor_create(ov_CBOR_UBIGNUM);
    two = ov_cbor_create(ov_CBOR_UBIGNUM);
    one->string = ov_string_dup("ov_CBOR_UBIGNUM");
    one->nbr_uint = strlen("ov_CBOR_UBIGNUM");
    two->string = ov_string_dup("ov_CBOR_UBIGNUM");
    two->nbr_uint = strlen("ov_CBOR_UBIGNUM");
    testrun(cbor_match(one, two));
    one->string = ov_data_pointer_free(one->string);
    one->string = ov_string_dup("tes");
    one->nbr_uint = strlen("tes");
    testrun(!cbor_match(one, two));
    two = ov_cbor_free(two);
    two = ov_cbor_create(ov_CBOR_UNDEF);
    testrun(!cbor_match(one, two));
    one = cbor_free(one);
    two = cbor_free(two);

    one = ov_cbor_create(ov_CBOR_IBIGNUM);
    two = ov_cbor_create(ov_CBOR_IBIGNUM);
    one->string = ov_string_dup("ov_CBOR_IBIGNUM");
    one->nbr_uint = strlen("ov_CBOR_IBIGNUM");
    two->string = ov_string_dup("ov_CBOR_IBIGNUM");
    two->nbr_uint = strlen("ov_CBOR_IBIGNUM");
    testrun(cbor_match(one, two));
    one->string = ov_data_pointer_free(one->string);
    one->string = ov_string_dup("tes");
    one->nbr_uint = strlen("tes");
    testrun(!cbor_match(one, two));
    two = ov_cbor_free(two);
    two = ov_cbor_create(ov_CBOR_UNDEF);
    testrun(!cbor_match(one, two));
    one = cbor_free(one);
    two = cbor_free(two);

    one = ov_cbor_create(ov_CBOR_DEC_FRACTION);
    two = ov_cbor_create(ov_CBOR_DEC_FRACTION);
    one->data = ov_cbor_array();
    two->data = ov_cbor_array();
    testrun(cbor_match(one, two));
    testrun(ov_cbor_array_push(one->data, ov_cbor_create(ov_CBOR_UNDEF)));
    testrun(!cbor_match(one, two));
    two = ov_cbor_free(two);
    two = ov_cbor_create(ov_CBOR_UNDEF);
    testrun(!cbor_match(one, two));
    one = cbor_free(one);
    two = cbor_free(two);

    one = ov_cbor_create(ov_CBOR_TAG);
    two = ov_cbor_create(ov_CBOR_TAG);
    one->nbr_uint = 123;
    two->nbr_uint = 123;
    one->data = ov_cbor_create(ov_CBOR_NULL);
    two->data = ov_cbor_create(ov_CBOR_NULL);
    testrun(cbor_match(one, two));
    two->nbr_uint = 1;
    testrun(!cbor_match(one, two));
    two->nbr_uint = 123;
    testrun(cbor_match(one, two));
    one->data = cbor_free(one->data);
    one->data = ov_cbor_create(ov_CBOR_FALSE);
    testrun(!cbor_match(one, two));
    two = ov_cbor_free(two);
    two = ov_cbor_create(ov_CBOR_UNDEF);
    testrun(!cbor_match(one, two));
    one = cbor_free(one);
    two = cbor_free(two);

    one = ov_cbor_create(ov_CBOR_SIMPLE);
    two = ov_cbor_create(ov_CBOR_SIMPLE);
    one->nbr_uint = 12345;
    two->nbr_uint = 12345;
    testrun(cbor_match(one, two));
    one->nbr_uint = 123456;
    testrun(!cbor_match(one, two));
    two = ov_cbor_free(two);
    two = ov_cbor_create(ov_CBOR_UNDEF);
    testrun(!cbor_match(one, two));
    one = cbor_free(one);
    two = cbor_free(two);

    one = ov_cbor_create(ov_CBOR_FLOAT);
    two = ov_cbor_create(ov_CBOR_FLOAT);
    one->nbr_float = 1.2;
    two->nbr_float = 1.2;
    testrun(cbor_match(one, two));
    one->nbr_float = 123.456;
    testrun(!cbor_match(one, two));
    two = ov_cbor_free(two);
    two = ov_cbor_create(ov_CBOR_UNDEF);
    testrun(!cbor_match(one, two));
    one = cbor_free(one);
    two = cbor_free(two);

    one = ov_cbor_create(ov_CBOR_DOUBLE);
    two = ov_cbor_create(ov_CBOR_DOUBLE);
    one->nbr_float = 1.2e4;
    two->nbr_float = 1.2e4;
    testrun(cbor_match(one, two));
    one->nbr_float = 123.456;
    testrun(!cbor_match(one, two));
    two = ov_cbor_free(two);
    two = ov_cbor_create(ov_CBOR_UNDEF);
    testrun(!cbor_match(one, two));
    one = cbor_free(one);
    two = cbor_free(two);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_cbor_dict_config() {

    ov_dict_config config = ov_cbor_dict_config(255);
    testrun(config.slots == 255);
    testrun(config.key.data_function.clear == cbor_clear);
    testrun(config.key.data_function.copy == cbor_copy);
    testrun(config.key.data_function.free == cbor_free);
    testrun(config.key.data_function.dump == cbor_dump);
    testrun(config.value.data_function.clear == cbor_clear);
    testrun(config.value.data_function.copy == cbor_copy);
    testrun(config.value.data_function.free == cbor_free);
    testrun(config.value.data_function.dump == cbor_dump);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_cbor_create() {

    ov_cbor *self = ov_cbor_create(ov_CBOR_UNDEF);
    testrun(self->type == ov_CBOR_UNDEF);
    testrun(self->data == NULL);
    testrun(self->nbr_uint == 0);
    testrun(NULL == cbor_free(self));

    self = ov_cbor_create(ov_CBOR_FALSE);
    testrun(self->type == ov_CBOR_FALSE);
    testrun(self->data == NULL);
    testrun(self->nbr_uint == 0);
    testrun(NULL == cbor_free(self));

    self = ov_cbor_create(ov_CBOR_TRUE);
    testrun(self->type == ov_CBOR_TRUE);
    testrun(self->data == NULL);
    testrun(self->nbr_uint == 0);
    testrun(NULL == cbor_free(self));

    self = ov_cbor_create(ov_CBOR_NULL);
    testrun(self->type == ov_CBOR_NULL);
    testrun(self->data == NULL);
    testrun(self->nbr_uint == 0);
    testrun(NULL == cbor_free(self));

    self = ov_cbor_create(ov_CBOR_UINT64);
    testrun(self->type == ov_CBOR_UINT64);
    testrun(self->data == NULL);
    testrun(self->nbr_uint == 0);
    testrun(NULL == cbor_free(self));

    self = ov_cbor_create(ov_CBOR_INT64);
    testrun(self->type == ov_CBOR_INT64);
    testrun(self->data == NULL);
    testrun(self->nbr_uint == 0);
    testrun(NULL == cbor_free(self));

    self = ov_cbor_create(ov_CBOR_STRING);
    testrun(self->type == ov_CBOR_STRING);
    testrun(self->data == NULL);
    testrun(self->nbr_uint == 0);
    testrun(NULL == cbor_free(self));

    self = ov_cbor_create(ov_CBOR_UTF8);
    testrun(self->type == ov_CBOR_UTF8);
    testrun(self->data == NULL);
    testrun(self->nbr_uint == 0);
    testrun(NULL == cbor_free(self));

    self = ov_cbor_create(ov_CBOR_ARRAY);
    testrun(self->type == ov_CBOR_ARRAY);
    testrun(self->data == NULL);
    testrun(self->nbr_uint == 0);
    testrun(NULL == cbor_free(self));

    self = ov_cbor_create(ov_CBOR_MAP);
    testrun(self->type == ov_CBOR_MAP);
    testrun(self->data == NULL);
    testrun(self->nbr_uint == 0);
    testrun(NULL == cbor_free(self));

    self = ov_cbor_create(ov_CBOR_DATE_TIME);
    testrun(self->type == ov_CBOR_DATE_TIME);
    testrun(self->data == NULL);
    testrun(self->nbr_uint == 0);
    testrun(NULL == cbor_free(self));

    self = ov_cbor_create(ov_CBOR_DATE_TIME_EPOCH);
    testrun(self->type == ov_CBOR_DATE_TIME_EPOCH);
    testrun(self->data == NULL);
    testrun(self->nbr_uint == 0);
    testrun(NULL == cbor_free(self));

    self = ov_cbor_create(ov_CBOR_UBIGNUM);
    testrun(self->type == ov_CBOR_UBIGNUM);
    testrun(self->data == NULL);
    testrun(self->nbr_uint == 0);
    testrun(NULL == cbor_free(self));

    self = ov_cbor_create(ov_CBOR_IBIGNUM);
    testrun(self->type == ov_CBOR_IBIGNUM);
    testrun(self->data == NULL);
    testrun(self->nbr_uint == 0);
    testrun(NULL == cbor_free(self));

    self = ov_cbor_create(ov_CBOR_DEC_FRACTION);
    testrun(self->type == ov_CBOR_DEC_FRACTION);
    testrun(self->data == NULL);
    testrun(self->nbr_uint == 0);
    testrun(NULL == cbor_free(self));

    self = ov_cbor_create(ov_CBOR_BIGFLOAT);
    testrun(self->type == ov_CBOR_BIGFLOAT);
    testrun(self->data == NULL);
    testrun(self->nbr_uint == 0);
    testrun(NULL == cbor_free(self));

    self = ov_cbor_create(ov_CBOR_TAG);
    testrun(self->type == ov_CBOR_TAG);
    testrun(self->data == NULL);
    testrun(self->nbr_uint == 0);
    testrun(NULL == cbor_free(self));

    self = ov_cbor_create(ov_CBOR_SIMPLE);
    testrun(self->type == ov_CBOR_SIMPLE);
    testrun(self->data == NULL);
    testrun(self->nbr_uint == 0);
    testrun(NULL == cbor_free(self));

    self = ov_cbor_create(ov_CBOR_FLOAT);
    testrun(self->type == ov_CBOR_FLOAT);
    testrun(self->data == NULL);
    testrun(self->nbr_uint == 0);
    testrun(NULL == cbor_free(self));

    self = ov_cbor_create(ov_CBOR_DOUBLE);
    testrun(self->type == ov_CBOR_DOUBLE);
    testrun(self->data == NULL);
    testrun(self->nbr_uint == 0);
    testrun(NULL == cbor_free(self));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_cbor_free() {

    ov_cbor *self = ov_cbor_create(ov_CBOR_UNDEF);
    testrun(cbor_clear(self));
    testrun(self->type == ov_CBOR_UNDEF);
    testrun(NULL == ov_cbor_free(self));

    self = ov_cbor_create(ov_CBOR_FALSE);
    testrun(NULL == ov_cbor_free(self));

    self = ov_cbor_create(ov_CBOR_TRUE);
    testrun(NULL == ov_cbor_free(self));

    self = ov_cbor_create(ov_CBOR_NULL);
    testrun(NULL == ov_cbor_free(self));

    self = ov_cbor_create(ov_CBOR_UINT64);
    self->nbr_uint = 1234;
    testrun(NULL == ov_cbor_free(self));

    self = ov_cbor_create(ov_CBOR_INT64);
    self->nbr_int = 1234;
    testrun(NULL == ov_cbor_free(self));

    self = ov_cbor_create(ov_CBOR_STRING);
    self->string = ov_string_dup("test");
    testrun(NULL == ov_cbor_free(self));

    self = ov_cbor_create(ov_CBOR_UTF8);
    self->string = ov_string_dup("test");
    testrun(NULL == ov_cbor_free(self));

    self = ov_cbor_create(ov_CBOR_ARRAY);
    self->data =
        ov_linked_list_create((ov_list_config){.item.free = cbor_free});
    testrun(NULL == ov_cbor_free(self));

    self = ov_cbor_create(ov_CBOR_MAP);
    self->data = ov_dict_create(ov_cbor_dict_config(255));
    testrun(NULL == ov_cbor_free(self));

    self = ov_cbor_create(ov_CBOR_DATE_TIME);
    self->string = ov_string_dup("timestamp");
    testrun(NULL == ov_cbor_free(self));

    self = ov_cbor_create(ov_CBOR_DATE_TIME_EPOCH);
    self->nbr_uint = 12345;
    testrun(NULL == ov_cbor_free(self));

    self = ov_cbor_create(ov_CBOR_UBIGNUM);
    self->string = ov_string_dup("ov_CBOR_UBIGNUM");
    testrun(NULL == ov_cbor_free(self));

    self = ov_cbor_create(ov_CBOR_IBIGNUM);
    self->string = ov_string_dup("ov_CBOR_IBIGNUM");
    testrun(NULL == ov_cbor_free(self));

    self = ov_cbor_create(ov_CBOR_DEC_FRACTION);
    self->data =
        ov_linked_list_create((ov_list_config){.item.free = cbor_free});
    testrun(NULL == ov_cbor_free(self));

    self = ov_cbor_create(ov_CBOR_BIGFLOAT);
    self->data =
        ov_linked_list_create((ov_list_config){.item.free = cbor_free});
    testrun(NULL == ov_cbor_free(self));

    self = ov_cbor_create(ov_CBOR_TAG);
    self->nbr_uint = 1234;
    self->data = ov_cbor_create(ov_CBOR_NULL);
    testrun(NULL == ov_cbor_free(self));

    self = ov_cbor_create(ov_CBOR_SIMPLE);
    self->nbr_uint = 1234;
    testrun(NULL == ov_cbor_free(self));

    self = ov_cbor_create(ov_CBOR_FLOAT);
    self->nbr_float = 1.2;
    testrun(NULL == ov_cbor_free(self));

    self = ov_cbor_create(ov_CBOR_DOUBLE);
    self->nbr_double = 1.2e4;
    testrun(NULL == ov_cbor_free(self));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_decode_uint() {

    uint8_t buffer[100] = {0};

    ov_cbor_match match = ov_CBOR_NO_MATCH;
    ov_cbor *out = NULL;
    uint8_t *next = NULL;

    for (int i = 0; i < 0x18; i++) {

        buffer[0] = i;
        match = decode_uint(buffer, 1, &out, &next);
        testrun(match == ov_CBOR_MATCH_FULL);
        testrun(out);
        testrun(out->type == ov_CBOR_UINT64);
        testrun(out->nbr_uint == (uint64_t)i);
        testrun(next);
        testrun(next == buffer + 1);
        out = cbor_free(out);
    }

    buffer[0] = 0x18;

    match = decode_uint(buffer, 1, &out, &next);
    testrun(match == ov_CBOR_MATCH_PARTIAL);

    for (int i = 0; i < 0xff; i++) {

        buffer[1] = i;
        match = decode_uint(buffer, 2, &out, &next);
        testrun(match == ov_CBOR_MATCH_FULL);
        testrun(out);
        testrun(out->type == ov_CBOR_UINT64);
        testrun(out->nbr_uint == (uint64_t)i);
        testrun(next);
        testrun(next == buffer + 2);
        out = cbor_free(out);
    }

    next = NULL;
    buffer[0] = 0x19;
    uint64_t nbr = 0;
    uint64_t local = 0;

    match = decode_uint(buffer, 1, &out, &next);
    testrun(match == ov_CBOR_MATCH_PARTIAL);
    match = decode_uint(buffer, 2, &out, &next);
    testrun(match == ov_CBOR_MATCH_PARTIAL);
    testrun(NULL == out);

    for (int i = 0; i < 0xff; i++) {

        buffer[1] = i;
        buffer[2] = i;
        match = decode_uint(buffer, 3, &out, &next);
        testrun(match == ov_CBOR_MATCH_FULL);
        testrun(out);
        testrun(out->type == ov_CBOR_UINT64);
        nbr = buffer[1] << 8;
        nbr += buffer[2];
        testrun(out->nbr_uint == nbr);
        testrun(next);
        testrun(next == buffer + 3);
        out = cbor_free(out);
    }

    next = NULL;
    buffer[0] = 0x1A;
    nbr = 0;

    match = decode_uint(buffer, 1, &out, &next);
    testrun(match == ov_CBOR_MATCH_PARTIAL);
    match = decode_uint(buffer, 2, &out, &next);
    testrun(match == ov_CBOR_MATCH_PARTIAL);
    match = decode_uint(buffer, 3, &out, &next);
    testrun(match == ov_CBOR_MATCH_PARTIAL);
    match = decode_uint(buffer, 4, &out, &next);
    testrun(match == ov_CBOR_MATCH_PARTIAL);
    testrun(NULL == out);

    for (int i = 0; i < 0xff; i++) {

        buffer[1] = i;
        buffer[2] = i;
        buffer[3] = i;
        buffer[4] = i;
        match = decode_uint(buffer, 5, &out, &next);
        testrun(match == ov_CBOR_MATCH_FULL);
        testrun(out);
        testrun(out->type == ov_CBOR_UINT64);
        local = buffer[1];
        nbr = local << 24;
        local = buffer[2];
        nbr += local << 16;
        local = buffer[3];
        nbr += local << 8;
        local = buffer[4];
        nbr += local;
        testrun(out->nbr_uint == nbr);
        testrun(next);
        testrun(next == buffer + 5);
        out = cbor_free(out);
    }

    next = NULL;
    buffer[0] = 0x1B;
    nbr = 0;

    match = decode_uint(buffer, 1, &out, &next);
    testrun(match == ov_CBOR_MATCH_PARTIAL);
    match = decode_uint(buffer, 2, &out, &next);
    testrun(match == ov_CBOR_MATCH_PARTIAL);
    match = decode_uint(buffer, 3, &out, &next);
    testrun(match == ov_CBOR_MATCH_PARTIAL);
    match = decode_uint(buffer, 4, &out, &next);
    testrun(match == ov_CBOR_MATCH_PARTIAL);
    match = decode_uint(buffer, 5, &out, &next);
    testrun(match == ov_CBOR_MATCH_PARTIAL);
    match = decode_uint(buffer, 6, &out, &next);
    testrun(match == ov_CBOR_MATCH_PARTIAL);
    match = decode_uint(buffer, 7, &out, &next);
    testrun(match == ov_CBOR_MATCH_PARTIAL);
    match = decode_uint(buffer, 8, &out, &next);
    testrun(match == ov_CBOR_MATCH_PARTIAL);
    testrun(NULL == out);

    for (int i = 0; i < 0xff; i++) {

        buffer[1] = i;
        buffer[2] = i;
        buffer[3] = i;
        buffer[4] = i;
        buffer[5] = i;
        buffer[6] = i;
        buffer[7] = i;
        buffer[8] = i;
        match = decode_uint(buffer, 9, &out, &next);
        testrun(match == ov_CBOR_MATCH_FULL);
        testrun(out->type == ov_CBOR_UINT64);
        testrun(out);
        local = buffer[1];
        nbr = local << 56;
        local = buffer[2];
        nbr += local << 48;
        local = buffer[3];
        nbr += local << 40;
        local = buffer[4];
        nbr += local << 32;
        local = buffer[5];
        nbr += local << 24;
        local = buffer[6];
        nbr += local << 16;
        local = buffer[7];
        nbr += local << 8;
        local = buffer[8];
        nbr += local;
        testrun(out->nbr_uint == nbr);
        testrun(next);
        testrun(next == buffer + 9);
        out = cbor_free(out);
    }
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_decode_int() {

    uint8_t buffer[100] = {0};

    ov_cbor_match match = ov_CBOR_NO_MATCH;
    ov_cbor *out = NULL;
    uint8_t *next = NULL;

    for (int i = 0x20; i < 0x38; i++) {

        buffer[0] = i;
        match = decode_int(buffer, 1, &out, &next);
        testrun(match == ov_CBOR_MATCH_FULL);
        testrun(out);
        testrun(out->type == ov_CBOR_INT64);
        testrun(out->nbr_int == -(int64_t)i);
        testrun(next);
        testrun(next == buffer + 1);
        out = cbor_free(out);
    }

    buffer[0] = 0x38;

    match = decode_int(buffer, 1, &out, &next);
    testrun(match == ov_CBOR_MATCH_PARTIAL);

    for (int i = 0; i < 0xff; i++) {

        buffer[1] = i;
        match = decode_int(buffer, 2, &out, &next);
        testrun(match == ov_CBOR_MATCH_FULL);
        testrun(out);
        testrun(out->type == ov_CBOR_INT64);
        testrun(out->nbr_int == -(int64_t)i);
        testrun(next);
        testrun(next == buffer + 2);
        out = cbor_free(out);
    }

    next = NULL;
    buffer[0] = 0x39;
    int64_t nbr = 0;
    uint64_t local = 0;

    match = decode_int(buffer, 1, &out, &next);
    testrun(match == ov_CBOR_MATCH_PARTIAL);
    match = decode_int(buffer, 2, &out, &next);
    testrun(match == ov_CBOR_MATCH_PARTIAL);
    testrun(NULL == out);

    for (int i = 0; i < 0xff; i++) {

        buffer[1] = i;
        buffer[2] = i;
        match = decode_int(buffer, 3, &out, &next);
        testrun(match == ov_CBOR_MATCH_FULL);
        testrun(out);
        testrun(out->type == ov_CBOR_INT64);
        nbr = buffer[1] << 8;
        nbr += buffer[2];
        testrun(out->nbr_int == -nbr);
        testrun(next);
        testrun(next == buffer + 3);
        out = cbor_free(out);
    }

    next = NULL;
    buffer[0] = 0x3A;
    nbr = 0;

    match = decode_int(buffer, 1, &out, &next);
    testrun(match == ov_CBOR_MATCH_PARTIAL);
    match = decode_int(buffer, 2, &out, &next);
    testrun(match == ov_CBOR_MATCH_PARTIAL);
    match = decode_int(buffer, 3, &out, &next);
    testrun(match == ov_CBOR_MATCH_PARTIAL);
    match = decode_int(buffer, 4, &out, &next);
    testrun(match == ov_CBOR_MATCH_PARTIAL);
    testrun(NULL == out);

    for (int i = 0; i < 0xff; i++) {

        buffer[1] = i;
        buffer[2] = i;
        buffer[3] = i;
        buffer[4] = i;
        match = decode_int(buffer, 5, &out, &next);
        testrun(match == ov_CBOR_MATCH_FULL);
        testrun(out);
        testrun(out->type == ov_CBOR_INT64);
        local = buffer[1];
        nbr = local << 24;
        local = buffer[2];
        nbr += local << 16;
        local = buffer[3];
        nbr += local << 8;
        local = buffer[4];
        nbr += local;
        testrun(out->nbr_int == -nbr);
        testrun(next);
        testrun(next == buffer + 5);
        out = cbor_free(out);
    }

    next = NULL;
    buffer[0] = 0x3B;
    nbr = 0;

    match = decode_int(buffer, 1, &out, &next);
    testrun(match == ov_CBOR_MATCH_PARTIAL);
    match = decode_int(buffer, 2, &out, &next);
    testrun(match == ov_CBOR_MATCH_PARTIAL);
    match = decode_int(buffer, 3, &out, &next);
    testrun(match == ov_CBOR_MATCH_PARTIAL);
    match = decode_int(buffer, 4, &out, &next);
    testrun(match == ov_CBOR_MATCH_PARTIAL);
    match = decode_int(buffer, 5, &out, &next);
    testrun(match == ov_CBOR_MATCH_PARTIAL);
    match = decode_int(buffer, 6, &out, &next);
    testrun(match == ov_CBOR_MATCH_PARTIAL);
    match = decode_int(buffer, 7, &out, &next);
    testrun(match == ov_CBOR_MATCH_PARTIAL);
    match = decode_int(buffer, 8, &out, &next);
    testrun(match == ov_CBOR_MATCH_PARTIAL);
    testrun(NULL == out);

    for (int i = 0; i < 0xff; i++) {

        buffer[1] = i;
        buffer[2] = i;
        buffer[3] = i;
        buffer[4] = i;
        buffer[5] = i;
        buffer[6] = i;
        buffer[7] = i;
        buffer[8] = i;
        match = decode_int(buffer, 9, &out, &next);
        testrun(match == ov_CBOR_MATCH_FULL);
        testrun(out);
        testrun(out->type == ov_CBOR_INT64);
        local = buffer[1];
        nbr = local << 56;
        local = buffer[2];
        nbr += local << 48;
        local = buffer[3];
        nbr += local << 40;
        local = buffer[4];
        nbr += local << 32;
        local = buffer[5];
        nbr += local << 24;
        local = buffer[6];
        nbr += local << 16;
        local = buffer[7];
        nbr += local << 8;
        local = buffer[8];
        nbr += local;
        testrun(out->nbr_int == -nbr);
        testrun(next);
        testrun(next == buffer + 9);
        out = cbor_free(out);
    }
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_decode_text_string() {

    uint8_t buffer[0xFFFF] = {0};

    ov_cbor_match match = ov_CBOR_NO_MATCH;
    ov_cbor *out = NULL;
    uint8_t *next = NULL;
    size_t strlen = 0;

    for (int i = 0x40; i < 0x58; i++) {

        buffer[0] = i;
        buffer[1] = 'a';
        buffer[2] = 'b';
        buffer[3] = 'c';
        buffer[4] = 'd';
        buffer[5] = 'e';
        buffer[6] = 'f';
        buffer[7] = 'g';
        buffer[8] = 'h';
        buffer[9] = 'i';
        buffer[10] = 'j';
        buffer[11] = 'k';
        buffer[12] = 'l';
        buffer[13] = 'm';
        buffer[14] = 'n';
        buffer[15] = 'o';
        buffer[16] = 'p';
        buffer[17] = 'q';
        buffer[18] = 'r';
        buffer[19] = 's';
        buffer[20] = 't';
        buffer[21] = 'u';
        buffer[22] = 'v';
        buffer[23] = 'w';
        buffer[24] = 'x';
        buffer[25] = 'y';
        buffer[26] = 'z';

        strlen = buffer[0] & 0x1F;

        // testrun_log("%x\n", i);

        if (i == 0x40) {

            match = decode_text_string(buffer, 1, &out, &next);
            testrun(match == ov_CBOR_MATCH_FULL);
            testrun(out);
            testrun(out->type == ov_CBOR_STRING);
            testrun(!out->string);
            testrun(out->nbr_uint == 0);
            out = cbor_free(out);

        } else {

            match = decode_text_string(buffer, 1, &out, &next);
            testrun(match == ov_CBOR_MATCH_PARTIAL);
            match = decode_text_string(buffer, 2 + i, &out, &next);
            testrun(match == ov_CBOR_MATCH_FULL);
            testrun(out);
            testrun(out->type == ov_CBOR_STRING);
            testrun(out->string);
            testrun(out->nbr_uint == strlen);
            // fprintf(stdout, "%"PRIu64"|%s", out->nbr_uint, out->string);
            testrun(0 == strncmp(out->string, (char *)buffer + 1, strlen));
            testrun(next);
            testrun(next == buffer + strlen + 1);
            out = cbor_free(out);
        }
    }

    buffer[0] = 0x58;
    match = decode_text_string(buffer, 1, &out, &next);
    testrun(match == ov_CBOR_MATCH_PARTIAL);

    for (int i = 0x00; i < 26; i++) {

        buffer[1] = i;
        buffer[2] = 'a';
        buffer[3] = 'b';
        buffer[4] = 'c';
        buffer[5] = 'd';
        buffer[6] = 'e';
        buffer[7] = 'f';
        buffer[8] = 'g';
        buffer[9] = 'h';
        buffer[10] = 'i';
        buffer[11] = 'j';
        buffer[12] = 'k';
        buffer[13] = 'l';
        buffer[14] = 'm';
        buffer[15] = 'n';
        buffer[16] = 'o';
        buffer[17] = 'p';
        buffer[18] = 'q';
        buffer[19] = 'r';
        buffer[20] = 's';
        buffer[21] = 't';
        buffer[22] = 'u';
        buffer[23] = 'v';
        buffer[24] = 'w';
        buffer[25] = 'x';
        buffer[26] = 'y';
        buffer[27] = 'z';

        strlen = i;

        // testrun_log("%x\n", i);

        if (i == 0x00) {

            match = decode_text_string(buffer, 2, &out, &next);
            testrun(match == ov_CBOR_MATCH_FULL);
            testrun(out);
            testrun(out->type == ov_CBOR_STRING);
            testrun(!out->string);
            testrun(out->nbr_uint == 0);
            out = cbor_free(out);

        } else {

            match = decode_text_string(buffer, 1, &out, &next);
            testrun(match == ov_CBOR_MATCH_PARTIAL);
            match = decode_text_string(buffer, 2 + i, &out, &next);
            testrun(match == ov_CBOR_MATCH_FULL);
            testrun(out);
            testrun(out->type == ov_CBOR_STRING);
            testrun(out->string);
            testrun(out->nbr_uint == strlen);
            // fprintf(stdout, "\n%"PRIu64"|%s", out->nbr_uint, out->string);
            testrun(0 == strncmp(out->string, (char *)buffer + 2, strlen));
            testrun(next);
            testrun(next == buffer + strlen + 2);
            out = cbor_free(out);
        }
    }

    buffer[0] = 0x58;
    buffer[1] = 0xFF;

    strlen = 0xFF;

    match = decode_text_string(buffer, 2 + 0xFF, &out, &next);
    testrun(match == ov_CBOR_MATCH_FULL);
    testrun(out);
    testrun(out->type == ov_CBOR_STRING);
    testrun(out->string);
    testrun(out->nbr_uint == strlen);
    testrun(next);
    testrun(next == buffer + strlen + 2);
    out = cbor_free(out);

    buffer[0] = 0x59;
    match = decode_text_string(buffer, 1, &out, &next);
    testrun(match == ov_CBOR_MATCH_PARTIAL);
    match = decode_text_string(buffer, 2, &out, &next);
    testrun(match == ov_CBOR_MATCH_PARTIAL);

    for (int i = 0x00; i < 26; i++) {

        buffer[1] = 0;
        buffer[2] = i;
        buffer[3] = 'b';
        buffer[4] = 'c';
        buffer[5] = 'd';
        buffer[6] = 'e';
        buffer[7] = 'f';
        buffer[8] = 'g';
        buffer[9] = 'h';
        buffer[10] = 'i';
        buffer[11] = 'j';
        buffer[12] = 'k';
        buffer[13] = 'l';
        buffer[14] = 'm';
        buffer[15] = 'n';
        buffer[16] = 'o';
        buffer[17] = 'p';
        buffer[18] = 'q';
        buffer[19] = 'r';
        buffer[20] = 's';
        buffer[21] = 't';
        buffer[22] = 'u';
        buffer[23] = 'v';
        buffer[24] = 'w';
        buffer[25] = 'x';
        buffer[26] = 'y';
        buffer[27] = 'z';

        strlen = i;

        // testrun_log("%x\n", i);

        if (i == 0x00) {

            match = decode_text_string(buffer, 3, &out, &next);
            testrun(match == ov_CBOR_MATCH_FULL);
            testrun(out);
            testrun(out->type == ov_CBOR_STRING);
            testrun(!out->string);
            testrun(out->nbr_uint == 0);
            out = cbor_free(out);

        } else {

            match = decode_text_string(buffer, 2 + i, &out, &next);
            testrun(match == ov_CBOR_MATCH_FULL);
            testrun(out);
            testrun(out->type == ov_CBOR_STRING);
            testrun(out->string);
            testrun(out->nbr_uint == strlen);
            // fprintf(stdout, "\n%"PRIu64"|%s", out->nbr_uint, out->string);
            testrun(0 == strncmp(out->string, (char *)buffer + 3, strlen));
            testrun(next);
            testrun(next == buffer + strlen + 3);
            out = cbor_free(out);
        }
    }

    buffer[0] = 0x59;
    buffer[1] = 0xFF;
    buffer[2] = 0xFF;

    strlen = 0xFFFF;

    match = decode_text_string(buffer, 2 + 0xFFFF, &out, &next);
    testrun(match == ov_CBOR_MATCH_FULL);
    testrun(out);
    testrun(out->type == ov_CBOR_STRING);
    testrun(out->string);
    testrun(out->nbr_uint == strlen);
    testrun(next);
    testrun(next == buffer + strlen + 3);
    out = cbor_free(out);

    buffer[0] = 0x5A;
    match = decode_text_string(buffer, 1, &out, &next);
    testrun(match == ov_CBOR_MATCH_PARTIAL);
    match = decode_text_string(buffer, 2, &out, &next);
    testrun(match == ov_CBOR_MATCH_PARTIAL);
    match = decode_text_string(buffer, 3, &out, &next);
    testrun(match == ov_CBOR_MATCH_PARTIAL);
    match = decode_text_string(buffer, 4, &out, &next);
    testrun(match == ov_CBOR_MATCH_PARTIAL);

    for (int i = 0x00; i < 26; i++) {

        buffer[1] = 0;
        buffer[2] = 0;
        buffer[3] = 0;
        buffer[4] = i;
        buffer[5] = 'd';
        buffer[6] = 'e';
        buffer[7] = 'f';
        buffer[8] = 'g';
        buffer[9] = 'h';
        buffer[10] = 'i';
        buffer[11] = 'j';
        buffer[12] = 'k';
        buffer[13] = 'l';
        buffer[14] = 'm';
        buffer[15] = 'n';
        buffer[16] = 'o';
        buffer[17] = 'p';
        buffer[18] = 'q';
        buffer[19] = 'r';
        buffer[20] = 's';
        buffer[21] = 't';
        buffer[22] = 'u';
        buffer[23] = 'v';
        buffer[24] = 'w';
        buffer[25] = 'x';
        buffer[26] = 'y';
        buffer[27] = 'z';

        strlen = i;

        // testrun_log("%x\n", i);

        if (i == 0x00) {

            match = decode_text_string(buffer, 5, &out, &next);
            testrun(match == ov_CBOR_MATCH_FULL);
            testrun(out);
            testrun(out->type == ov_CBOR_STRING);
            testrun(!out->string);
            testrun(out->nbr_uint == 0);
            out = cbor_free(out);

        } else {

            match = decode_text_string(buffer, 5 + i, &out, &next);
            testrun(match == ov_CBOR_MATCH_FULL);
            testrun(out);
            testrun(out->type == ov_CBOR_STRING);
            testrun(out->string);
            testrun(out->nbr_uint == strlen);
            // fprintf(stdout, "\n%"PRIu64"|%s", out->nbr_uint, out->string);
            testrun(0 == strncmp(out->string, (char *)buffer + 5, strlen));
            testrun(next);
            testrun(next == buffer + strlen + 5);
            out = cbor_free(out);
        }
    }

    /*
        // test offline due to big buffer allocation
        buffer[0] = 0x5A;
        buffer[1] = 0xFF;
        buffer[2] = 0xFF;
        buffer[3] = 0xFF;
        buffer[4] = 0xFF;

        strlen = 0xFFFFffff;

        match = decode_text_string(buffer, 2 + strlen, &out, &next);
        testrun(match == ov_CBOR_MATCH_FULL);
        testrun(out);
        testrun(out->type == ov_CBOR_STRING);
        testrun(out->string);
        testrun(out->nbr_uint == strlen);
        testrun(next);
        testrun(next == buffer + strlen + 5);
        out = cbor_free(out);
    */

    buffer[0] = 0x5B;
    match = decode_text_string(buffer, 1, &out, &next);
    testrun(match == ov_CBOR_MATCH_PARTIAL);
    match = decode_text_string(buffer, 2, &out, &next);
    testrun(match == ov_CBOR_MATCH_PARTIAL);
    match = decode_text_string(buffer, 3, &out, &next);
    testrun(match == ov_CBOR_MATCH_PARTIAL);
    match = decode_text_string(buffer, 4, &out, &next);
    testrun(match == ov_CBOR_MATCH_PARTIAL);
    match = decode_text_string(buffer, 5, &out, &next);
    testrun(match == ov_CBOR_MATCH_PARTIAL);
    match = decode_text_string(buffer, 6, &out, &next);
    testrun(match == ov_CBOR_MATCH_PARTIAL);
    match = decode_text_string(buffer, 7, &out, &next);
    testrun(match == ov_CBOR_MATCH_PARTIAL);
    match = decode_text_string(buffer, 8, &out, &next);
    testrun(match == ov_CBOR_MATCH_PARTIAL);

    for (int i = 0x00; i < 10; i++) {

        buffer[1] = 0;
        buffer[2] = 0;
        buffer[3] = 0;
        buffer[4] = 0;
        buffer[5] = 0;
        buffer[6] = 0;
        buffer[7] = 0;
        buffer[8] = i;
        buffer[9] = 'h';
        buffer[10] = 'i';
        buffer[11] = 'j';
        buffer[12] = 'k';
        buffer[13] = 'l';
        buffer[14] = 'm';
        buffer[15] = 'n';
        buffer[16] = 'o';
        buffer[17] = 'p';
        buffer[18] = 'q';
        buffer[19] = 'r';
        buffer[20] = 's';
        buffer[21] = 't';
        buffer[22] = 'u';
        buffer[23] = 'v';
        buffer[24] = 'w';
        buffer[25] = 'x';
        buffer[26] = 'y';
        buffer[27] = 'z';

        strlen = i;

        // testrun_log("%x\n", i);

        if (i == 0x00) {

            match = decode_text_string(buffer, 9, &out, &next);
            testrun(match == ov_CBOR_MATCH_FULL);
            testrun(out);
            testrun(out->type == ov_CBOR_STRING);
            testrun(!out->string);
            testrun(out->nbr_uint == 0);
            out = cbor_free(out);

        } else {

            match = decode_text_string(buffer, 9 + i, &out, &next);
            testrun(match == ov_CBOR_MATCH_FULL);
            testrun(out);
            testrun(out->type == ov_CBOR_STRING);
            testrun(out->string);
            testrun(out->nbr_uint == strlen);
            // fprintf(stdout, "\n%"PRIu64"|%s", out->nbr_uint, out->string);
            testrun(0 == strncmp(out->string, (char *)buffer + 9, strlen));
            testrun(next);
            testrun(next == buffer + strlen + 9);
            out = cbor_free(out);
        }
    }

    buffer[0] = 0x5F;
    buffer[1] = '1';
    buffer[2] = 'a';
    buffer[3] = 'b';
    buffer[4] = 'c';
    buffer[5] = 'd';
    buffer[6] = 'e';
    buffer[7] = 'f';
    buffer[8] = 'g';
    buffer[9] = 'h';
    buffer[10] = 'i';
    buffer[11] = 'j';
    buffer[12] = 'k';
    buffer[13] = 'l';
    buffer[14] = 'm';
    buffer[15] = 'n';
    buffer[16] = 'o';
    buffer[17] = 'p';
    buffer[18] = 'q';
    buffer[19] = 'r';
    buffer[20] = 's';
    buffer[21] = 't';
    buffer[22] = 'u';
    buffer[23] = 'v';
    buffer[24] = 'w';
    buffer[25] = 'x';
    buffer[26] = 'y';
    buffer[27] = 'z';
    buffer[28] = 0xff;

    for (int i = 1; i < 29; i++) {

        match = decode_text_string(buffer, i, &out, &next);
        testrun(match == ov_CBOR_MATCH_PARTIAL);
    }

    match = decode_text_string(buffer, 29, &out, &next);
    testrun(match == ov_CBOR_MATCH_FULL);
    testrun(out);
    // fprintf(stdout, "\n%"PRIu64"|%s", out->nbr_uint, out->string);
    testrun(out->type == ov_CBOR_STRING);
    testrun(out->string);
    testrun(out->nbr_uint == 27);
    testrun(0 == strncmp(out->string, (char *)buffer + 1, strlen));
    testrun(next);
    testrun(next == buffer + 29);
    out = cbor_free(out);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_decode_utf8_string() {

    uint8_t buffer[0xffff] = {0};

    ov_cbor_match match = ov_CBOR_NO_MATCH;
    ov_cbor *out = NULL;
    uint8_t *next = NULL;
    size_t strlen = 0;

    for (int i = 0x60; i < 0x77; i++) {

        buffer[0] = i;
        buffer[1] = 'a';
        buffer[2] = 'b';
        buffer[3] = 'c';
        buffer[4] = 'd';
        buffer[5] = 'e';
        buffer[6] = 'f';
        buffer[7] = 'g';
        buffer[8] = 'h';
        buffer[9] = 'i';
        buffer[10] = 'j';
        buffer[11] = 'k';
        buffer[12] = 'l';
        buffer[13] = 'm';
        buffer[14] = 'n';
        buffer[15] = 'o';
        buffer[16] = 'p';
        buffer[17] = 'q';
        buffer[18] = 'r';
        buffer[19] = 's';
        buffer[20] = 't';
        buffer[21] = 'u';
        buffer[22] = 'v';
        buffer[23] = 'w';
        buffer[24] = 'x';
        buffer[25] = 'y';
        buffer[26] = 'z';

        strlen = buffer[0] & 0x1F;

        // testrun_log("%x\n", i);

        if (i == 0x60) {

            match = decode_utf8_string(buffer, 1, &out, &next);
            testrun(match == ov_CBOR_MATCH_FULL);
            testrun(out);
            testrun(out->type == ov_CBOR_UTF8);
            testrun(!out->string);
            testrun(out->nbr_uint == 0);
            out = cbor_free(out);

        } else {

            match = decode_utf8_string(buffer, 1, &out, &next);
            testrun(match == ov_CBOR_MATCH_PARTIAL);
            match = decode_utf8_string(buffer, 2 + i, &out, &next);
            testrun(match == ov_CBOR_MATCH_FULL);
            testrun(out);
            testrun(out->type == ov_CBOR_UTF8);
            testrun(out->string);
            testrun(out->nbr_uint == strlen);
            // fprintf(stdout, "%"PRIu64"|%s", out->nbr_uint, out->string);
            testrun(0 == strncmp(out->string, (char *)buffer + 1, strlen));
            testrun(next);
            testrun(next == buffer + strlen + 1);
            out = cbor_free(out);
        }
    }

    buffer[0] = 0x78;
    match = decode_utf8_string(buffer, 1, &out, &next);
    testrun(match == ov_CBOR_MATCH_PARTIAL);

    for (int i = 0x00; i < 26; i++) {

        buffer[1] = i;
        buffer[2] = 'a';
        buffer[3] = 'b';
        buffer[4] = 'c';
        buffer[5] = 'd';
        buffer[6] = 'e';
        buffer[7] = 'f';
        buffer[8] = 'g';
        buffer[9] = 'h';
        buffer[10] = 'i';
        buffer[11] = 'j';
        buffer[12] = 'k';
        buffer[13] = 'l';
        buffer[14] = 'm';
        buffer[15] = 'n';
        buffer[16] = 'o';
        buffer[17] = 'p';
        buffer[18] = 'q';
        buffer[19] = 'r';
        buffer[20] = 's';
        buffer[21] = 't';
        buffer[22] = 'u';
        buffer[23] = 'v';
        buffer[24] = 'w';
        buffer[25] = 'x';
        buffer[26] = 'y';
        buffer[27] = 'z';

        strlen = i;

        // testrun_log("%x\n", i);

        if (i == 0x00) {

            match = decode_utf8_string(buffer, 2, &out, &next);
            testrun(match == ov_CBOR_MATCH_FULL);
            testrun(out);
            testrun(out->type == ov_CBOR_UTF8);
            testrun(!out->string);
            testrun(out->nbr_uint == 0);
            out = cbor_free(out);

        } else {

            match = decode_utf8_string(buffer, 1, &out, &next);
            testrun(match == ov_CBOR_MATCH_PARTIAL);
            match = decode_utf8_string(buffer, 2 + i, &out, &next);
            testrun(match == ov_CBOR_MATCH_FULL);
            testrun(out);
            testrun(out->type == ov_CBOR_UTF8);
            testrun(out->string);
            testrun(out->nbr_uint == strlen);
            // fprintf(stdout, "\n%"PRIu64"|%s", out->nbr_uint, out->string);
            testrun(0 == strncmp(out->string, (char *)buffer + 2, strlen));
            testrun(next);
            testrun(next == buffer + strlen + 2);
            out = cbor_free(out);
        }
    }

    buffer[0] = 0x78;
    buffer[1] = 0xFF;

    for (int i = 2; i < 0xffff; i++) {

        buffer[i] = 'a';
    }

    strlen = 0xFF;

    match = decode_utf8_string(buffer, 2 + 0xFF, &out, &next);
    testrun(match == ov_CBOR_MATCH_FULL);
    testrun(out);
    testrun(out->type == ov_CBOR_UTF8);
    testrun(out->string);
    testrun(out->nbr_uint == strlen);
    testrun(next);
    testrun(next == buffer + strlen + 2);
    out = cbor_free(out);

    buffer[0] = 0x79;
    match = decode_utf8_string(buffer, 1, &out, &next);
    testrun(match == ov_CBOR_MATCH_PARTIAL);
    match = decode_utf8_string(buffer, 2, &out, &next);
    testrun(match == ov_CBOR_MATCH_PARTIAL);

    for (int i = 0x00; i < 26; i++) {

        buffer[1] = 0;
        buffer[2] = i;
        buffer[3] = 'b';
        buffer[4] = 'c';
        buffer[5] = 'd';
        buffer[6] = 'e';
        buffer[7] = 'f';
        buffer[8] = 'g';
        buffer[9] = 'h';
        buffer[10] = 'i';
        buffer[11] = 'j';
        buffer[12] = 'k';
        buffer[13] = 'l';
        buffer[14] = 'm';
        buffer[15] = 'n';
        buffer[16] = 'o';
        buffer[17] = 'p';
        buffer[18] = 'q';
        buffer[19] = 'r';
        buffer[20] = 's';
        buffer[21] = 't';
        buffer[22] = 'u';
        buffer[23] = 'v';
        buffer[24] = 'w';
        buffer[25] = 'x';
        buffer[26] = 'y';
        buffer[27] = 'z';

        strlen = i;

        // testrun_log("%x\n", i);

        if (i == 0x00) {

            match = decode_utf8_string(buffer, 3, &out, &next);
            testrun(match == ov_CBOR_MATCH_FULL);
            testrun(out);
            testrun(out->type == ov_CBOR_UTF8);
            testrun(!out->string);
            testrun(out->nbr_uint == 0);
            out = cbor_free(out);

        } else {

            match = decode_utf8_string(buffer, 2 + i, &out, &next);
            testrun(match == ov_CBOR_MATCH_FULL);
            testrun(out);
            testrun(out->type == ov_CBOR_UTF8);
            testrun(out->string);
            testrun(out->nbr_uint == strlen);
            // fprintf(stdout, "\n%"PRIu64"|%s", out->nbr_uint, out->string);
            testrun(0 == strncmp(out->string, (char *)buffer + 3, strlen));
            testrun(next);
            testrun(next == buffer + strlen + 3);
            out = cbor_free(out);
        }
    }

    buffer[0] = 0x79;
    buffer[1] = 0xFF;
    buffer[2] = 0xF0;

    for (int i = 3; i < 0xffff; i++) {

        buffer[i] = 'a';
    }

    strlen = 0xFFF0;

    match = decode_utf8_string(buffer, 2 + 0xFFF0, &out, &next);
    testrun(match == ov_CBOR_MATCH_FULL);
    testrun(out);
    testrun(out->type == ov_CBOR_UTF8);
    testrun(out->string);
    testrun(out->nbr_uint == strlen);
    testrun(next);
    testrun(next == buffer + strlen + 3);
    out = cbor_free(out);

    buffer[0] = 0x7A;
    match = decode_utf8_string(buffer, 1, &out, &next);
    testrun(match == ov_CBOR_MATCH_PARTIAL);
    match = decode_utf8_string(buffer, 2, &out, &next);
    testrun(match == ov_CBOR_MATCH_PARTIAL);
    match = decode_utf8_string(buffer, 3, &out, &next);
    testrun(match == ov_CBOR_MATCH_PARTIAL);
    match = decode_utf8_string(buffer, 4, &out, &next);
    testrun(match == ov_CBOR_MATCH_PARTIAL);

    for (int i = 0x00; i < 26; i++) {

        buffer[1] = 0;
        buffer[2] = 0;
        buffer[3] = 0;
        buffer[4] = i;
        buffer[5] = 'd';
        buffer[6] = 'e';
        buffer[7] = 'f';
        buffer[8] = 'g';
        buffer[9] = 'h';
        buffer[10] = 'i';
        buffer[11] = 'j';
        buffer[12] = 'k';
        buffer[13] = 'l';
        buffer[14] = 'm';
        buffer[15] = 'n';
        buffer[16] = 'o';
        buffer[17] = 'p';
        buffer[18] = 'q';
        buffer[19] = 'r';
        buffer[20] = 's';
        buffer[21] = 't';
        buffer[22] = 'u';
        buffer[23] = 'v';
        buffer[24] = 'w';
        buffer[25] = 'x';
        buffer[26] = 'y';
        buffer[27] = 'z';

        strlen = i;

        // testrun_log("%x\n", i);

        if (i == 0x00) {

            match = decode_utf8_string(buffer, 5, &out, &next);
            testrun(match == ov_CBOR_MATCH_FULL);
            testrun(out);
            testrun(out->type == ov_CBOR_UTF8);
            testrun(!out->string);
            testrun(out->nbr_uint == 0);
            out = cbor_free(out);

        } else {

            match = decode_utf8_string(buffer, 5 + i, &out, &next);
            testrun(match == ov_CBOR_MATCH_FULL);
            testrun(out);
            testrun(out->type == ov_CBOR_UTF8);
            testrun(out->string);
            testrun(out->nbr_uint == strlen);
            // fprintf(stdout, "\n%"PRIu64"|%s", out->nbr_uint, out->string);
            testrun(0 == strncmp(out->string, (char *)buffer + 5, strlen));
            testrun(next);
            testrun(next == buffer + strlen + 5);
            out = cbor_free(out);
        }
    }

    /*
        // test offline due to big buffer allocation
        buffer[0] = 0x7A;
        buffer[1] = 0xFF;
        buffer[2] = 0xFF;
        buffer[3] = 0xFF;
        buffer[4] = 0xFF;

        strlen = 0xFFFFffff;

        match = decode_utf8_string(buffer, 2 + strlen, &out, &next);
        testrun(match == ov_CBOR_MATCH_FULL);
        testrun(out);
        testrun(out->type == ov_CBOR_UTF8);
        testrun(out->string);
        testrun(out->nbr_uint == strlen);
        testrun(next);
        testrun(next == buffer + strlen + 5);
        out = cbor_free(out);
    */

    buffer[0] = 0x7B;
    match = decode_utf8_string(buffer, 1, &out, &next);
    testrun(match == ov_CBOR_MATCH_PARTIAL);
    match = decode_utf8_string(buffer, 2, &out, &next);
    testrun(match == ov_CBOR_MATCH_PARTIAL);
    match = decode_utf8_string(buffer, 3, &out, &next);
    testrun(match == ov_CBOR_MATCH_PARTIAL);
    match = decode_utf8_string(buffer, 4, &out, &next);
    testrun(match == ov_CBOR_MATCH_PARTIAL);
    match = decode_utf8_string(buffer, 5, &out, &next);
    testrun(match == ov_CBOR_MATCH_PARTIAL);
    match = decode_utf8_string(buffer, 6, &out, &next);
    testrun(match == ov_CBOR_MATCH_PARTIAL);
    match = decode_utf8_string(buffer, 7, &out, &next);
    testrun(match == ov_CBOR_MATCH_PARTIAL);
    match = decode_utf8_string(buffer, 8, &out, &next);
    testrun(match == ov_CBOR_MATCH_PARTIAL);

    for (int i = 0x00; i < 10; i++) {

        buffer[1] = 0;
        buffer[2] = 0;
        buffer[3] = 0;
        buffer[4] = 0;
        buffer[5] = 0;
        buffer[6] = 0;
        buffer[7] = 0;
        buffer[8] = i;
        buffer[9] = 'h';
        buffer[10] = 'i';
        buffer[11] = 'j';
        buffer[12] = 'k';
        buffer[13] = 'l';
        buffer[14] = 'm';
        buffer[15] = 'n';
        buffer[16] = 'o';
        buffer[17] = 'p';
        buffer[18] = 'q';
        buffer[19] = 'r';
        buffer[20] = 's';
        buffer[21] = 't';
        buffer[22] = 'u';
        buffer[23] = 'v';
        buffer[24] = 'w';
        buffer[25] = 'x';
        buffer[26] = 'y';
        buffer[27] = 'z';

        strlen = i;

        // testrun_log("%x\n", i);

        if (i == 0x00) {

            match = decode_utf8_string(buffer, 9, &out, &next);
            testrun(match == ov_CBOR_MATCH_FULL);
            testrun(out);
            testrun(out->type == ov_CBOR_UTF8);
            testrun(!out->string);
            testrun(out->nbr_uint == 0);
            out = cbor_free(out);

        } else {

            match = decode_utf8_string(buffer, 9 + i, &out, &next);
            testrun(match == ov_CBOR_MATCH_FULL);
            testrun(out);
            testrun(out->type == ov_CBOR_UTF8);
            testrun(out->string);
            testrun(out->nbr_uint == strlen);
            // fprintf(stdout, "\n%"PRIu64"|%s", out->nbr_uint, out->string);
            testrun(0 == strncmp(out->string, (char *)buffer + 9, strlen));
            testrun(next);
            testrun(next == buffer + strlen + 9);
            out = cbor_free(out);
        }
    }

    buffer[0] = 0x7F;
    buffer[1] = '1';
    buffer[2] = 'a';
    buffer[3] = 'b';
    buffer[4] = 'c';
    buffer[5] = 'd';
    buffer[6] = 'e';
    buffer[7] = 'f';
    buffer[8] = 'g';
    buffer[9] = 'h';
    buffer[10] = 'i';
    buffer[11] = 'j';
    buffer[12] = 'k';
    buffer[13] = 'l';
    buffer[14] = 'm';
    buffer[15] = 'n';
    buffer[16] = 'o';
    buffer[17] = 'p';
    buffer[18] = 'q';
    buffer[19] = 'r';
    buffer[20] = 's';
    buffer[21] = 't';
    buffer[22] = 'u';
    buffer[23] = 'v';
    buffer[24] = 'w';
    buffer[25] = 'x';
    buffer[26] = 'y';
    buffer[27] = 'z';
    buffer[28] = 0xff;

    for (int i = 1; i < 29; i++) {

        match = decode_utf8_string(buffer, i, &out, &next);
        testrun(match == ov_CBOR_MATCH_PARTIAL);
    }

    match = decode_utf8_string(buffer, 29, &out, &next);
    testrun(match == ov_CBOR_MATCH_FULL);
    testrun(out);
    // fprintf(stdout, "\n%"PRIu64"|%s", out->nbr_uint, out->string);
    testrun(out->type == ov_CBOR_UTF8);
    testrun(out->string);
    testrun(out->nbr_uint == 27);
    testrun(0 == strncmp(out->string, (char *)buffer + 1, strlen));
    testrun(next);
    testrun(next == buffer + 29);
    out = cbor_free(out);

    // check limits
    testrun(
        ov_cbor_configure((ov_cbor_config){.limits.utf8_string_size = 20}));

    match = decode_utf8_string(buffer, 29, &out, &next);
    testrun(match == ov_CBOR_NO_MATCH);

    testrun(
        ov_cbor_configure((ov_cbor_config){.limits.utf8_string_size = 200}));

    // check some valid utf8

    buffer[0] = 0x7F;
    buffer[1] = 0x00;
    buffer[2] = 0xC2;
    buffer[3] = 0xBF;
    buffer[4] = 0x00;
    buffer[5] = 0xEf;
    buffer[6] = 0xBB;
    buffer[7] = 0xBF;
    buffer[8] = 0x00;
    buffer[9] = 0xF0;
    buffer[10] = 0xA3;
    buffer[11] = 0x8E;
    buffer[12] = 0xB4;
    buffer[13] = 0x00;
    buffer[14] = 0xF0;
    buffer[15] = 0xA3;
    buffer[16] = 0x8E;
    buffer[17] = 0xB4;
    buffer[18] = 0xC2;
    buffer[19] = 0xBF;
    buffer[20] = 0xEf;
    buffer[21] = 0xBB;
    buffer[22] = 0xBF;
    buffer[23] = 0xC2;
    buffer[24] = 0xBF;
    buffer[25] = 0x00;
    buffer[26] = 0xFF;

    match = decode_utf8_string(buffer, 27, &out, &next);
    testrun(match == ov_CBOR_MATCH_FULL);
    testrun(out);
    testrun(out->type == ov_CBOR_UTF8);
    testrun(out->string);
    testrun(out->nbr_uint == 25);
    testrun(next);
    testrun(next == buffer + 27);
    out = cbor_free(out);

    // check some invalid utf8

    buffer[0] = 0x7F;
    buffer[1] = 0xF0; // Gothic Letter Ahsa U+10330
    buffer[2] = 0x90;
    buffer[3] = 0x8C;
    buffer[4] = 0xB0;
    buffer[5] = 0xF0; // Gothic Letter Bairkan U+10331
    buffer[6] = 0x90;
    buffer[7] = 0x8C;
    buffer[8] = 0xB1;
    buffer[9] = 0xF0; // Gothic Letter Giba U+10332
    buffer[10] = 0x90;
    buffer[11] = 0x8C;
    buffer[12] = 0xB2;
    buffer[13] = 0xF0; // invalid 4 byte
    buffer[14] = 0xff;
    buffer[15] = 0x8C;
    buffer[16] = 0xB2;
    buffer[17] = 0xFF;

    match = decode_utf8_string(buffer, 17, &out, &next);
    testrun(match == ov_CBOR_NO_MATCH);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_decode_array() {

    uint8_t buffer[0xffff] = {0};

    ov_cbor_match match = ov_CBOR_NO_MATCH;
    ov_cbor *out = NULL;
    uint8_t *next = NULL;
    uint64_t count = 0;

    for (int i = 0x80; i < 0x97; i++) {

        count++;
        buffer[0] = i;
        buffer[1] = 0x10;
        buffer[2] = 0x11;
        buffer[3] = 0x12;
        buffer[4] = 0x13;
        buffer[5] = 0x14;
        buffer[6] = 0x15;
        buffer[7] = 0x16;
        buffer[8] = 0x17;
        buffer[9] = 0x10;
        buffer[10] = 0x11;
        buffer[11] = 0x12;
        buffer[12] = 0x13;
        buffer[13] = 0x14;
        buffer[14] = 0x15;
        buffer[15] = 0x16;
        buffer[16] = 0x17;
        buffer[17] = 0x10;
        buffer[18] = 0x11;
        buffer[19] = 0x12;
        buffer[20] = 0x13;

        // testrun_log("%x\n", i);

        if (i == 0x80) {

            match = decode_array(buffer, 1, &out, &next);
            testrun(match == ov_CBOR_MATCH_FULL);
            testrun(out);
            testrun(out->type == ov_CBOR_ARRAY);
            testrun(out->data);
            testrun(ov_list_is_empty(out->data));
            testrun(out->nbr_uint == 0);
            out = cbor_free(out);

        } else {

            match = decode_array(buffer, 1, &out, &next);
            testrun(match == ov_CBOR_MATCH_PARTIAL);
            match = decode_array(buffer, 2 + i, &out, &next);
            testrun(match == ov_CBOR_MATCH_FULL);
            testrun(out);
            testrun(out->type == ov_CBOR_ARRAY);
            testrun(count - 1 == ov_list_count(out->data));
            testrun(next);
            testrun(next == buffer + count);
            out = cbor_free(out);
            // fprintf(stdout, "%i - %x\n", (int)count - 1, next[0]);
        }
    }

    buffer[0] = 0x98;
    count = 0;

    for (int i = 0; i < 10; i++) {

        buffer[1] = i;
        buffer[2] = 0x10;
        buffer[3] = 0x11;
        buffer[4] = 0x12;
        buffer[5] = 0x13;
        buffer[6] = 0x14;
        buffer[7] = 0x15;
        buffer[8] = 0x16;
        buffer[9] = 0x17;
        buffer[10] = 0x10; // integer 10
        buffer[11] = 0x10; // integer 10
        buffer[12] = 0x10; // integer 10
        buffer[13] = 0x10; // integer 10
        buffer[14] = 0x10; // integer 10
        buffer[15] = 0x10; // integer 10
        buffer[16] = 0x10; // integer 10
        buffer[17] = 0x10; // integer 10
        buffer[18] = 0x10; // integer 10
        buffer[19] = 0x10; // integer 10
        buffer[20] = 0x10; // integer 10

        // testrun_log("%x\n", i);

        if (i > 3) {
            match = decode_array(buffer, i - 2, &out, &next);
            testrun(match == ov_CBOR_MATCH_PARTIAL);
            match = decode_array(buffer, i - 1, &out, &next);
            testrun(match == ov_CBOR_MATCH_PARTIAL);
        }
        match = decode_array(buffer, 3 + i, &out, &next);
        testrun(match == ov_CBOR_MATCH_FULL);
        testrun(out);
        testrun(out->type == ov_CBOR_ARRAY);
        testrun(count == ov_list_count(out->data));
        testrun(next);
        // fprintf(stdout, "%x|%li\n", next[0], next - buffer);
        testrun(next == buffer + count + 2);
        out = cbor_free(out);

        count++;
    }

    buffer[0] = 0x99;
    count = 0;

    for (int i = 0; i < 10; i++) {

        buffer[1] = 0;
        buffer[2] = i;
        buffer[3] = 0x10;  // integer 10
        buffer[4] = 0x10;  // integer 10
        buffer[5] = 0x10;  // integer 10
        buffer[6] = 0x10;  // integer 10
        buffer[7] = 0x10;  // integer 10
        buffer[8] = 0x10;  // integer 10
        buffer[9] = 0x10;  // integer 10
        buffer[10] = 0x10; // integer 10
        buffer[11] = 0x10; // integer 10
        buffer[12] = 0x10; // integer 10
        buffer[13] = 0x10; // integer 10
        buffer[14] = 0x10; // integer 10
        buffer[15] = 0x10; // integer 10
        buffer[16] = 0x10; // integer 10
        buffer[17] = 0x10; // integer 10
        buffer[18] = 0x10; // integer 10
        buffer[19] = 0x10; // integer 10
        buffer[20] = 0x10; // integer 10

        // testrun_log("%x\n", i);

        if (i > 3) {
            match = decode_array(buffer, i - 2, &out, &next);
            testrun(match == ov_CBOR_MATCH_PARTIAL);
            match = decode_array(buffer, i - 1, &out, &next);
            testrun(match == ov_CBOR_MATCH_PARTIAL);
        }
        match = decode_array(buffer, 3 + i, &out, &next);
        testrun(match == ov_CBOR_MATCH_FULL);
        testrun(out);
        testrun(out->type == ov_CBOR_ARRAY);
        fprintf(stdout, "%li|%li\n", count, ov_list_count(out->data));
        testrun(count == ov_list_count(out->data));
        testrun(next);
        // fprintf(stdout, "%li\n", next - buffer);
        testrun(next == buffer + count + 3);
        out = cbor_free(out);

        count++;
    }

    buffer[0] = 0x9A;
    count = 0;

    for (int i = 0; i < 10; i++) {

        buffer[1] = 0;
        buffer[2] = 0;
        buffer[3] = 0;
        buffer[4] = i;
        buffer[5] = 0x10;  // integer 10
        buffer[6] = 0x10;  // integer 10
        buffer[7] = 0x10;  // integer 10
        buffer[8] = 0x10;  // integer 10
        buffer[9] = 0x10;  // integer 10
        buffer[10] = 0x10; // integer 10
        buffer[11] = 0x10; // integer 10
        buffer[12] = 0x10; // integer 10
        buffer[13] = 0x10; // integer 10
        buffer[14] = 0x10; // integer 10
        buffer[15] = 0x10; // integer 10
        buffer[16] = 0x10; // integer 10
        buffer[17] = 0x10; // integer 10
        buffer[18] = 0x10; // integer 10
        buffer[19] = 0x10; // integer 10
        buffer[20] = 0x10; // integer 10

        if (i > 3) {
            match = decode_array(buffer, i - 2, &out, &next);
            testrun(match == ov_CBOR_MATCH_PARTIAL);
            match = decode_array(buffer, i - 1, &out, &next);
            testrun(match == ov_CBOR_MATCH_PARTIAL);
        }
        match = decode_array(buffer, 5 + i, &out, &next);
        testrun(match == ov_CBOR_MATCH_FULL);
        testrun(out);
        testrun(out->type == ov_CBOR_ARRAY);
        testrun(count == ov_list_count(out->data));
        testrun(next);
        testrun(next == buffer + count + 5);
        out = cbor_free(out);

        count++;
    }

    buffer[0] = 0x9B;
    count = 0;

    for (int i = 0; i < 10; i++) {

        buffer[1] = 0;
        buffer[2] = 0;
        buffer[3] = 0;
        buffer[4] = 0;
        buffer[5] = 0;
        buffer[6] = 0;
        buffer[7] = 0;
        buffer[8] = i;
        buffer[9] = 0x10;  // integer 10
        buffer[10] = 0x10; // integer 10
        buffer[11] = 0x10; // integer 10
        buffer[12] = 0x10; // integer 10
        buffer[13] = 0x10; // integer 10
        buffer[14] = 0x10; // integer 10
        buffer[15] = 0x10; // integer 10
        buffer[16] = 0x10; // integer 10
        buffer[17] = 0x10; // integer 10
        buffer[18] = 0x10; // integer 10
        buffer[19] = 0x10; // integer 10
        buffer[20] = 0x10; // integer 10

        if (i > 3) {
            match = decode_array(buffer, i - 2, &out, &next);
            testrun(match == ov_CBOR_MATCH_PARTIAL);
            match = decode_array(buffer, i - 1, &out, &next);
            testrun(match == ov_CBOR_MATCH_PARTIAL);
        }
        match = decode_array(buffer, 9 + i, &out, &next);
        testrun(match == ov_CBOR_MATCH_FULL);
        testrun(out);
        testrun(out->type == ov_CBOR_ARRAY);
        testrun(count == ov_list_count(out->data));
        testrun(next);
        testrun(next == buffer + count + 9);
        out = cbor_free(out);

        count++;
    }

    // check limitation

    testrun(ov_cbor_configure((ov_cbor_config){.limits.array_size = 4}));

    match = decode_array(buffer, 10, &out, &next);
    testrun(match == ov_CBOR_NO_MATCH);

    buffer[0] = 0x9F;
    buffer[1] = 0x01;
    buffer[2] = 0x02;
    buffer[3] = 0x03;
    buffer[4] = 0x04;
    buffer[5] = 0x05;
    buffer[6] = 0x06;
    buffer[7] = 0x07;
    buffer[8] = 0x08;
    buffer[9] = 0xff;

    match = decode_array(buffer, 10, &out, &next);
    testrun(match == ov_CBOR_MATCH_FULL);
    testrun(out);
    testrun(out->type == ov_CBOR_ARRAY);
    testrun(8 == ov_list_count(out->data));
    testrun(next);
    testrun(next == buffer + 10);
    out = cbor_free(out);

    // check valid items with 0xFF included
    buffer[0] = 0x9F;
    buffer[1] = 0x18;
    buffer[2] = 0xFF;
    buffer[3] = 0x03;
    buffer[4] = 0x18;
    buffer[5] = 0xFF;
    buffer[6] = 0x06;
    buffer[7] = 0x18;
    buffer[8] = 0xFF;
    buffer[9] = 0xff;

    for (int i = 3; i < 9; i++) {

        fprintf(stdout, "i == %i\n", i);

        testrun(ov_CBOR_MATCH_PARTIAL == decode_array(buffer, i, &out, &next));
    }

    match = decode_array(buffer, 10, &out, &next);
    testrun(match == ov_CBOR_MATCH_FULL);
    testrun(out);
    testrun(out->type == ov_CBOR_ARRAY);
    testrun(5 == ov_list_count(out->data));
    testrun(next);
    testrun(next == buffer + 10);
    out = cbor_free(out);

    // check limitation

    testrun(
        ov_cbor_configure((ov_cbor_config){.limits.undef_length_array = 4}));

    match = decode_array(buffer, 10, &out, &next);
    testrun(match == ov_CBOR_NO_MATCH);

    buffer[0] = 0x82; // params
    buffer[1] = 0x82; // 2
    buffer[2] = 0x01; // id
    buffer[3] = 0x06; // variant
    buffer[4] = 0x82; // 2
    buffer[5] = 0x03; // id
    buffer[6] = 0x07; // all flags
    buffer[7] = 0xFF;

    testrun(ov_CBOR_MATCH_FULL == decode_array(buffer, 7, &out, &next));

    testrun(2 == ov_cbor_array_count(out));
    testrun(ov_cbor_is_array(ov_cbor_array_get(out, 0)));
    testrun(ov_cbor_is_array(ov_cbor_array_get(out, 1)));
    testrun(*next == 0xFF);
    out = ov_cbor_free(out);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_decode_map() {

    uint8_t buffer[0xffff] = {0};

    ov_cbor_match match = ov_CBOR_NO_MATCH;
    ov_cbor *out = NULL;
    uint8_t *next = NULL;
    int64_t count = 0;
    uint64_t two = 0;

    for (int i = 0xA0; i < 0xB7; i++) {

        two = i % 2;

        buffer[0] = i;
        buffer[1] = 0x10;
        buffer[2] = 0x11;
        buffer[3] = 0x12;
        buffer[4] = 0x13;
        buffer[5] = 0x14;
        buffer[6] = 0x15;
        buffer[7] = 0x16;
        buffer[8] = 0x17;
        buffer[9] = 0x10;
        buffer[10] = 0x11;
        buffer[11] = 0x12;
        buffer[12] = 0x13;
        buffer[13] = 0x14;
        buffer[14] = 0x15;
        buffer[15] = 0x16;
        buffer[16] = 0x17;
        buffer[17] = 0x10;
        buffer[18] = 0x11;
        buffer[19] = 0x12;
        buffer[20] = 0x13;

        // testrun_log("%x\n", i);

        if (i == 0xA0) {

            match = decode_map(buffer, 1, &out, &next);
            testrun(match == ov_CBOR_MATCH_FULL);
            testrun(out);
            testrun(out->type == ov_CBOR_MAP);
            testrun(out->data);
            testrun(ov_dict_is_empty(out->data));
            testrun(out->nbr_uint == 0);
            out = cbor_free(out);

        } else if (two == 0) {

            match = decode_map(buffer, 1 + i, &out, &next);
            testrun(match == ov_CBOR_MATCH_FULL);
            testrun(out);
            testrun(out->type == ov_CBOR_MAP);
            testrun(!ov_dict_is_empty(out->data));
            testrun(next);
            out = cbor_free(out);

        } else {

            match = decode_map(buffer, 1 + count, &out, &next);
            testrun(match == ov_CBOR_MATCH_PARTIAL);
        }

        count++;
    }

    // check next

    buffer[0] = 0xA1;
    buffer[1] = 0x10;
    buffer[2] = 0x11;
    buffer[3] = 0x12;
    buffer[4] = 0x13;
    buffer[5] = 0x14;
    buffer[6] = 0x15;
    buffer[7] = 0x16;
    buffer[8] = 0x17;
    buffer[9] = 0x10;
    buffer[10] = 0x11;
    buffer[11] = 0x12;
    buffer[12] = 0x13;
    buffer[13] = 0x14;
    buffer[14] = 0x15;
    buffer[15] = 0x16;
    buffer[16] = 0x17;
    buffer[17] = 0x10;
    buffer[18] = 0x11;
    buffer[19] = 0x12;
    buffer[20] = 0x13;

    match = decode_map(buffer, 20, &out, &next);
    testrun(match == ov_CBOR_MATCH_FULL);
    testrun(out);
    testrun(out->type == ov_CBOR_MAP);
    testrun(1 == ov_dict_count(out->data));
    testrun(next);
    // fprintf(stdout, "%li|%x", next-buffer, next[0]);
    testrun(next == buffer + 3);
    out = cbor_free(out);

    buffer[0] = 0xA3;
    buffer[1] = 0x10;
    buffer[2] = 0x11;
    buffer[3] = 0x12;
    buffer[4] = 0x13;
    buffer[5] = 0x14;
    buffer[6] = 0x15;
    buffer[7] = 0x16;
    buffer[8] = 0x17;
    buffer[9] = 0x10;
    buffer[10] = 0x11;
    buffer[11] = 0x12;
    buffer[12] = 0x13;
    buffer[13] = 0x14;
    buffer[14] = 0x15;
    buffer[15] = 0x16;
    buffer[16] = 0x17;
    buffer[17] = 0x10;
    buffer[18] = 0x11;
    buffer[19] = 0x12;
    buffer[20] = 0x13;

    match = decode_map(buffer, 20, &out, &next);
    testrun(match == ov_CBOR_MATCH_FULL);
    testrun(out);
    testrun(out->type == ov_CBOR_MAP);
    testrun(3 == ov_dict_count(out->data));
    testrun(next);
    testrun(next == buffer + 7);
    out = cbor_free(out);

    buffer[0] = 0xB8;
    buffer[1] = 0x00;
    buffer[2] = 0x11;
    buffer[3] = 0x12;
    buffer[4] = 0x13;
    buffer[5] = 0x14;
    buffer[6] = 0x15;
    buffer[7] = 0x16;
    buffer[8] = 0x17;
    buffer[9] = 0x10;
    buffer[10] = 0x11;
    buffer[11] = 0x12;
    buffer[12] = 0x13;
    buffer[13] = 0x14;
    buffer[14] = 0x15;
    buffer[15] = 0x16;
    buffer[16] = 0x17;
    buffer[17] = 0x10;
    buffer[18] = 0x11;
    buffer[19] = 0x12;
    buffer[20] = 0x13;

    match = decode_map(buffer, 20, &out, &next);
    testrun(match == ov_CBOR_MATCH_FULL);
    testrun(out);
    testrun(out->type == ov_CBOR_MAP);
    testrun(0 == ov_dict_count(out->data));
    testrun(next);
    testrun(next == buffer + 2);
    out = cbor_free(out);

    buffer[1] = 0x01;
    match = decode_map(buffer, 20, &out, &next);
    testrun(match == ov_CBOR_MATCH_FULL);
    testrun(out);
    testrun(out->type == ov_CBOR_MAP);
    testrun(1 == ov_dict_count(out->data));
    testrun(next);

    testrun(next == buffer + 4);
    out = cbor_free(out);

    buffer[1] = 0x02;
    match = decode_map(buffer, 20, &out, &next);
    testrun(match == ov_CBOR_MATCH_FULL);
    testrun(out);
    testrun(out->type == ov_CBOR_MAP);
    testrun(2 == ov_dict_count(out->data));
    testrun(next);
    // fprintf(stdout, "%li|%x", next-buffer, next[0]);
    testrun(next == buffer + 6);
    out = cbor_free(out);

    buffer[1] = 0x03;
    match = decode_map(buffer, 20, &out, &next);
    testrun(match == ov_CBOR_MATCH_FULL);
    testrun(out);
    testrun(out->type == ov_CBOR_MAP);
    testrun(3 == ov_dict_count(out->data));
    testrun(next);
    // fprintf(stdout, "%li|%x", next-buffer, next[0]);
    testrun(next == buffer + 8);
    out = cbor_free(out);

    buffer[0] = 0xB9;
    buffer[1] = 0x00;
    buffer[2] = 0x00;
    buffer[3] = 0x12;
    buffer[4] = 0x13;
    buffer[5] = 0x14;
    buffer[6] = 0x15;
    buffer[7] = 0x16;
    buffer[8] = 0x17;
    buffer[9] = 0x10;
    buffer[10] = 0x11;
    buffer[11] = 0x12;
    buffer[12] = 0x13;
    buffer[13] = 0x14;
    buffer[14] = 0x15;
    buffer[15] = 0x16;
    buffer[16] = 0x17;
    buffer[17] = 0x10;
    buffer[18] = 0x11;
    buffer[19] = 0x12;
    buffer[20] = 0x13;

    match = decode_map(buffer, 20, &out, &next);
    testrun(match == ov_CBOR_MATCH_FULL);
    testrun(out);
    testrun(out->type == ov_CBOR_MAP);
    testrun(0 == ov_dict_count(out->data));
    testrun(next);
    testrun(next == buffer + 3);
    out = cbor_free(out);

    buffer[2] = 0x01;
    match = decode_map(buffer, 20, &out, &next);
    testrun(match == ov_CBOR_MATCH_FULL);
    testrun(out);
    testrun(out->type == ov_CBOR_MAP);
    testrun(1 == ov_dict_count(out->data));
    testrun(next);
    testrun(next == buffer + 5);
    out = cbor_free(out);

    buffer[2] = 0x02;
    match = decode_map(buffer, 20, &out, &next);
    testrun(match == ov_CBOR_MATCH_FULL);
    testrun(out);
    testrun(out->type == ov_CBOR_MAP);
    testrun(2 == ov_dict_count(out->data));
    testrun(next);
    // fprintf(stdout, "%li|%x", next-buffer, next[0]);
    testrun(next == buffer + 7);
    out = cbor_free(out);

    buffer[2] = 0x03;
    match = decode_map(buffer, 20, &out, &next);
    testrun(match == ov_CBOR_MATCH_FULL);
    testrun(out);
    testrun(out->type == ov_CBOR_MAP);
    testrun(3 == ov_dict_count(out->data));
    testrun(next);
    // fprintf(stdout, "%li|%x", next-buffer, next[0]);
    testrun(next == buffer + 9);
    out = cbor_free(out);

    buffer[0] = 0xBA;
    buffer[1] = 0x00;
    buffer[2] = 0x00;
    buffer[3] = 0x00;
    buffer[4] = 0x00;
    buffer[5] = 0x14;
    buffer[6] = 0x15;
    buffer[7] = 0x16;
    buffer[8] = 0x17;
    buffer[9] = 0x10;
    buffer[10] = 0x11;
    buffer[11] = 0x12;
    buffer[12] = 0x13;
    buffer[13] = 0x14;
    buffer[14] = 0x15;
    buffer[15] = 0x16;
    buffer[16] = 0x17;
    buffer[17] = 0x10;
    buffer[18] = 0x11;
    buffer[19] = 0x12;
    buffer[20] = 0x13;

    match = decode_map(buffer, 20, &out, &next);
    testrun(match == ov_CBOR_MATCH_FULL);
    testrun(out);
    testrun(out->type == ov_CBOR_MAP);
    testrun(0 == ov_dict_count(out->data));
    testrun(next);
    testrun(next == buffer + 5);
    out = cbor_free(out);

    buffer[4] = 0x01;
    match = decode_map(buffer, 20, &out, &next);
    testrun(match == ov_CBOR_MATCH_FULL);
    testrun(out);
    testrun(out->type == ov_CBOR_MAP);
    testrun(1 == ov_dict_count(out->data));
    testrun(next);
    testrun(next == buffer + 7);
    out = cbor_free(out);

    buffer[4] = 0x02;
    match = decode_map(buffer, 20, &out, &next);
    testrun(match == ov_CBOR_MATCH_FULL);
    testrun(out);
    testrun(out->type == ov_CBOR_MAP);
    testrun(2 == ov_dict_count(out->data));
    testrun(next);
    // fprintf(stdout, "%li|%x", next-buffer, next[0]);
    testrun(next == buffer + 9);
    out = cbor_free(out);

    buffer[4] = 0x03;
    match = decode_map(buffer, 20, &out, &next);
    testrun(match == ov_CBOR_MATCH_FULL);
    testrun(out);
    testrun(out->type == ov_CBOR_MAP);
    testrun(3 == ov_dict_count(out->data));
    testrun(next);
    // fprintf(stdout, "%li|%x", next-buffer, next[0]);
    testrun(next == buffer + 11);
    out = cbor_free(out);

    buffer[0] = 0xBB;
    buffer[1] = 0x00;
    buffer[2] = 0x00;
    buffer[3] = 0x00;
    buffer[4] = 0x00;
    buffer[5] = 0x00;
    buffer[6] = 0x00;
    buffer[7] = 0x00;
    buffer[8] = 0x00;
    buffer[9] = 0x10;
    buffer[10] = 0x11;
    buffer[11] = 0x12;
    buffer[12] = 0x13;
    buffer[13] = 0x14;
    buffer[14] = 0x15;
    buffer[15] = 0x16;
    buffer[16] = 0x17;
    buffer[17] = 0x10;
    buffer[18] = 0x11;
    buffer[19] = 0x12;
    buffer[20] = 0x13;

    match = decode_map(buffer, 20, &out, &next);
    testrun(match == ov_CBOR_MATCH_FULL);
    testrun(out);
    testrun(out->type == ov_CBOR_MAP);
    testrun(0 == ov_dict_count(out->data));
    testrun(next);
    testrun(next == buffer + 9);
    out = cbor_free(out);

    buffer[8] = 0x01;
    match = decode_map(buffer, 20, &out, &next);
    testrun(match == ov_CBOR_MATCH_FULL);
    testrun(out);
    testrun(out->type == ov_CBOR_MAP);
    testrun(1 == ov_dict_count(out->data));
    testrun(next);
    testrun(next == buffer + 11);
    out = cbor_free(out);

    buffer[8] = 0x02;
    match = decode_map(buffer, 20, &out, &next);
    testrun(match == ov_CBOR_MATCH_FULL);
    testrun(out);
    testrun(out->type == ov_CBOR_MAP);
    testrun(2 == ov_dict_count(out->data));
    testrun(next);
    // fprintf(stdout, "%li|%x", next-buffer, next[0]);
    testrun(next == buffer + 13);
    out = cbor_free(out);

    buffer[8] = 0x03;
    match = decode_map(buffer, 20, &out, &next);
    testrun(match == ov_CBOR_MATCH_FULL);
    testrun(out);
    testrun(out->type == ov_CBOR_MAP);
    testrun(3 == ov_dict_count(out->data));
    testrun(next);
    // fprintf(stdout, "%li|%x", next-buffer, next[0]);
    testrun(next == buffer + 15);
    out = cbor_free(out);

    buffer[0] = 0xBF;
    buffer[1] = 0xFF;
    buffer[2] = 0x11;
    buffer[3] = 0x12;
    buffer[4] = 0x13;
    buffer[5] = 0x14;
    buffer[6] = 0x15;
    buffer[7] = 0x16;
    buffer[8] = 0x17;
    buffer[9] = 0x10;
    buffer[10] = 0x11;
    buffer[11] = 0x12;
    buffer[12] = 0x13;
    buffer[13] = 0x14;
    buffer[14] = 0x15;
    buffer[15] = 0x16;
    buffer[16] = 0x17;
    buffer[17] = 0x10;
    buffer[18] = 0x11;
    buffer[19] = 0x12;
    buffer[20] = 0x13;

    match = decode_map(buffer, 20, &out, &next);
    testrun(match == ov_CBOR_MATCH_FULL);
    testrun(out);
    testrun(out->type == ov_CBOR_MAP);
    testrun(0 == ov_dict_count(out->data));
    testrun(next);
    // fprintf(stdout, "%li|%x", next-buffer, next[0]);
    testrun(next == buffer + 2);
    out = cbor_free(out);

    buffer[0] = 0xBF;
    buffer[1] = 0x10;
    buffer[2] = 0xFF;
    buffer[3] = 0x12;
    buffer[4] = 0x13;
    buffer[5] = 0x14;
    buffer[6] = 0x15;
    buffer[7] = 0x16;
    buffer[8] = 0x17;
    buffer[9] = 0x10;
    buffer[10] = 0x11;
    buffer[11] = 0x12;
    buffer[12] = 0x13;
    buffer[13] = 0x14;
    buffer[14] = 0x15;
    buffer[15] = 0x16;
    buffer[16] = 0x17;
    buffer[17] = 0x10;
    buffer[18] = 0x11;
    buffer[19] = 0x12;
    buffer[20] = 0x13;

    match = decode_map(buffer, 20, &out, &next);
    testrun(match == ov_CBOR_NO_MATCH);

    buffer[0] = 0xBF;
    buffer[1] = 0x01;
    buffer[2] = 0x11;
    buffer[3] = 0xFF;
    buffer[4] = 0x13;
    buffer[5] = 0x14;
    buffer[6] = 0x15;
    buffer[7] = 0x16;
    buffer[8] = 0x17;
    buffer[9] = 0x10;
    buffer[10] = 0x11;
    buffer[11] = 0x12;
    buffer[12] = 0x13;
    buffer[13] = 0x14;
    buffer[14] = 0x15;
    buffer[15] = 0x16;
    buffer[16] = 0x17;
    buffer[17] = 0x10;
    buffer[18] = 0x11;
    buffer[19] = 0x12;
    buffer[20] = 0x13;

    match = decode_map(buffer, 20, &out, &next);
    testrun(match == ov_CBOR_MATCH_FULL);
    testrun(out);
    testrun(out->type == ov_CBOR_MAP);
    testrun(1 == ov_dict_count(out->data));
    testrun(next);
    // fprintf(stdout, "%li|%x", next-buffer, next[0]);
    testrun(next == buffer + 4);
    out = cbor_free(out);

    buffer[0] = 0xBF;
    buffer[1] = 0x01;
    buffer[2] = 0x11;
    buffer[3] = 0x10;
    buffer[4] = 0xff;
    buffer[5] = 0x14;
    buffer[6] = 0x15;
    buffer[7] = 0x16;
    buffer[8] = 0x17;
    buffer[9] = 0x10;
    buffer[10] = 0x11;
    buffer[11] = 0x12;
    buffer[12] = 0x13;
    buffer[13] = 0x14;
    buffer[14] = 0x15;
    buffer[15] = 0x16;
    buffer[16] = 0x17;
    buffer[17] = 0x10;
    buffer[18] = 0x11;
    buffer[19] = 0x12;
    buffer[20] = 0x13;

    match = decode_map(buffer, 20, &out, &next);
    testrun(match == ov_CBOR_NO_MATCH);

    buffer[0] = 0xBF;
    buffer[1] = 0x01;
    buffer[2] = 0x11;
    buffer[3] = 0x10;
    buffer[4] = 0x10;
    buffer[5] = 0xff;
    buffer[6] = 0x15;
    buffer[7] = 0x16;
    buffer[8] = 0x17;
    buffer[9] = 0x10;
    buffer[10] = 0x11;
    buffer[11] = 0x12;
    buffer[12] = 0x13;
    buffer[13] = 0x14;
    buffer[14] = 0x15;
    buffer[15] = 0x16;
    buffer[16] = 0x17;
    buffer[17] = 0x10;
    buffer[18] = 0x11;
    buffer[19] = 0x12;
    buffer[20] = 0x13;

    match = decode_map(buffer, 20, &out, &next);
    testrun(match == ov_CBOR_MATCH_FULL);
    testrun(out);
    testrun(out->type == ov_CBOR_MAP);
    testrun(2 == ov_dict_count(out->data));
    testrun(next);
    // fprintf(stdout, "%li|%x", next-buffer, next[0]);
    testrun(next == buffer + 6);
    out = cbor_free(out);

    buffer[0] = 0xBF;
    buffer[1] = 0x01; // key
    buffer[2] = 0x18; // val
    buffer[3] = 0xff;
    buffer[4] = 0x10; // key
    buffer[5] = 0x18; // val
    buffer[6] = 0xFF;
    buffer[7] = 0xFF;
    buffer[8] = 0x00;
    buffer[9] = 0x10;
    buffer[10] = 0x11;
    buffer[11] = 0x12;
    buffer[12] = 0x13;
    buffer[13] = 0x14;
    buffer[14] = 0x15;
    buffer[15] = 0x16;
    buffer[16] = 0x17;
    buffer[17] = 0x10;
    buffer[18] = 0x11;
    buffer[19] = 0x12;
    buffer[20] = 0x13;

    match = decode_map(buffer, 20, &out, &next);
    testrun(match == ov_CBOR_MATCH_FULL);
    testrun(out);
    testrun(out->type == ov_CBOR_MAP);
    testrun(2 == ov_dict_count(out->data));
    testrun(next);
    // fprintf(stdout, "%li|%x", next-buffer, next[0]);
    testrun(next == buffer + 8);
    out = cbor_free(out);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_decode_tag() {

    uint8_t buffer[0xffff] = {0};

    ov_cbor_match match = ov_CBOR_NO_MATCH;
    ov_cbor *out = NULL;
    uint8_t *next = NULL;

    buffer[0] = 0xc0;
    buffer[1] = 0x45;
    buffer[2] = 'a';
    buffer[3] = 'b';
    buffer[4] = 'c';
    buffer[5] = 'd';
    buffer[6] = 'e';
    buffer[7] = 'f';
    buffer[8] = 'g';

    match = decode_tag(buffer, 1, &out, &next);
    testrun(match == ov_CBOR_MATCH_PARTIAL);
    testrun(!out);

    match = decode_tag(buffer, 2, &out, &next);
    testrun(match == ov_CBOR_MATCH_PARTIAL);
    testrun(!out);

    match = decode_tag(buffer, 7, &out, &next);
    testrun(match == ov_CBOR_MATCH_FULL);
    testrun(out);
    testrun(out->type == ov_CBOR_DATE_TIME);
    fprintf(stdout, "%s", out->string);
    testrun(0 == strcmp(out->string, "abcde"));
    testrun(next == buffer + 7);
    out = cbor_free(out);

    buffer[0] = 0xc1;
    buffer[1] = 0x01;
    buffer[2] = 'a';
    buffer[3] = 'b';
    buffer[4] = 'c';
    buffer[5] = 'd';
    buffer[6] = 'e';
    buffer[7] = 'f';
    buffer[8] = 'g';

    match = decode_tag(buffer, 1, &out, &next);
    testrun(match == ov_CBOR_MATCH_PARTIAL);
    testrun(!out);

    match = decode_tag(buffer, 2, &out, &next);
    testrun(match == ov_CBOR_MATCH_FULL);
    testrun(out);
    testrun(out->type == ov_CBOR_DATE_TIME_EPOCH);
    testrun(1 == out->nbr_uint);
    testrun(next == buffer + 2);
    out = cbor_free(out);

    buffer[0] = 0xc2;
    buffer[1] = 0x45;
    buffer[2] = 'a';
    buffer[3] = 'b';
    buffer[4] = 'c';
    buffer[5] = 'd';
    buffer[6] = 'e';
    buffer[7] = 'f';
    buffer[8] = 'g';

    match = decode_tag(buffer, 1, &out, &next);
    testrun(match == ov_CBOR_MATCH_PARTIAL);
    testrun(!out);

    match = decode_tag(buffer, 7, &out, &next);
    testrun(match == ov_CBOR_MATCH_FULL);
    testrun(out);
    testrun(out->type == ov_CBOR_UBIGNUM);
    testrun(0 == strcmp(out->string, "abcde"));
    testrun(next == buffer + 7);
    out = cbor_free(out);

    buffer[0] = 0xc3;
    buffer[1] = 0x45;
    buffer[2] = 'a';
    buffer[3] = 'b';
    buffer[4] = 'c';
    buffer[5] = 'd';
    buffer[6] = 'e';
    buffer[7] = 'f';
    buffer[8] = 'g';

    match = decode_tag(buffer, 1, &out, &next);
    testrun(match == ov_CBOR_MATCH_PARTIAL);
    testrun(!out);

    match = decode_tag(buffer, 7, &out, &next);
    testrun(match == ov_CBOR_MATCH_FULL);
    testrun(out);
    testrun(out->type == ov_CBOR_IBIGNUM);
    testrun(0 == strcmp(out->string, "abcde"));
    testrun(next == buffer + 7);
    out = cbor_free(out);

    buffer[0] = 0xc4;
    buffer[1] = 0x80;
    buffer[2] = 'a';
    buffer[3] = 'b';
    buffer[4] = 'c';
    buffer[5] = 'd';
    buffer[6] = 'e';
    buffer[7] = 'f';
    buffer[8] = 'g';

    match = decode_tag(buffer, 1, &out, &next);
    testrun(match == ov_CBOR_MATCH_PARTIAL);
    testrun(!out);

    match = decode_tag(buffer, 2, &out, &next);
    testrun(match == ov_CBOR_MATCH_FULL);
    testrun(out);
    testrun(out->type == ov_CBOR_DEC_FRACTION);
    testrun(0 == ov_cbor_array_count(out->data));
    testrun(next == buffer + 2);
    out = cbor_free(out);

    buffer[0] = 0xc5;
    buffer[1] = 0x80;
    buffer[2] = 'a';
    buffer[3] = 'b';
    buffer[4] = 'c';
    buffer[5] = 'd';
    buffer[6] = 'e';
    buffer[7] = 'f';
    buffer[8] = 'g';

    match = decode_tag(buffer, 1, &out, &next);
    testrun(match == ov_CBOR_MATCH_PARTIAL);
    testrun(!out);

    match = decode_tag(buffer, 2, &out, &next);
    testrun(match == ov_CBOR_MATCH_FULL);
    testrun(out);
    testrun(out->type == ov_CBOR_BIGFLOAT);
    testrun(0 == ov_cbor_array_count(out->data));
    testrun(next == buffer + 2);
    out = cbor_free(out);

    for (int i = 0xc6; i <= 0xd4; i++) {

        buffer[0] = i;

        // testrun_log("%x\n", i);

        match = decode_tag(buffer, 1, &out, &next);
        testrun(match == ov_CBOR_MATCH_FULL);
        testrun(out);
        testrun(out->type == ov_CBOR_TAG);
        testrun(out->nbr_uint == (i & 0x1F));
        testrun(next == buffer + 1);
        out = cbor_free(out);
    }

    buffer[0] = 0xD5;
    buffer[1] = 0x80;
    buffer[2] = 'a';
    buffer[3] = 'b';
    buffer[4] = 'c';
    buffer[5] = 'd';
    buffer[6] = 'e';
    buffer[7] = 'f';
    buffer[8] = 'g';

    match = decode_tag(buffer, 1, &out, &next);
    testrun(match == ov_CBOR_MATCH_PARTIAL);
    testrun(!out);

    match = decode_tag(buffer, 2, &out, &next);
    testrun(match == ov_CBOR_MATCH_FULL);
    testrun(out);
    testrun(out->type == ov_CBOR_TAG);
    testrun(0 == ov_cbor_array_count(out->data));
    testrun(next == buffer + 2);
    out = cbor_free(out);

    buffer[0] = 0xD6;
    buffer[1] = 0x45;
    buffer[2] = 'a';
    buffer[3] = 'b';
    buffer[4] = 'c';
    buffer[5] = 'd';
    buffer[6] = 'e';
    buffer[7] = 'f';
    buffer[8] = 'g';

    match = decode_tag(buffer, 1, &out, &next);
    testrun(match == ov_CBOR_MATCH_PARTIAL);
    testrun(!out);

    match = decode_tag(buffer, 8, &out, &next);
    testrun(match == ov_CBOR_MATCH_FULL);
    testrun(out);
    testrun(out->type == ov_CBOR_TAG);
    testrun(out->data);
    testrun(next == buffer + 7);
    out = cbor_free(out);

    buffer[0] = 0xD7;
    buffer[1] = 0x45;
    buffer[2] = 'a';
    buffer[3] = 'b';
    buffer[4] = 'c';
    buffer[5] = 'd';
    buffer[6] = 'e';
    buffer[7] = 'f';
    buffer[8] = 'g';

    match = decode_tag(buffer, 1, &out, &next);
    testrun(match == ov_CBOR_MATCH_PARTIAL);
    testrun(!out);

    match = decode_tag(buffer, 8, &out, &next);
    testrun(match == ov_CBOR_MATCH_FULL);
    testrun(out);
    testrun(out->type == ov_CBOR_TAG);
    testrun(out->data);
    testrun(next == buffer + 7);
    out = cbor_free(out);

    buffer[0] = 0xd8;
    buffer[1] = 0x0F;
    buffer[2] = 0xF0;
    buffer[3] = 0x00;
    buffer[4] = 0x00;
    buffer[5] = 0x00;
    buffer[6] = 0x00;
    buffer[7] = 0x00;
    buffer[8] = 0x00;

    match = decode_tag(buffer, 1, &out, &next);
    testrun(match == ov_CBOR_MATCH_PARTIAL);
    testrun(!out);

    match = decode_tag(buffer, 2, &out, &next);
    testrun(match == ov_CBOR_MATCH_FULL);
    testrun(out);
    testrun(out->type == ov_CBOR_TAG);
    testrun(out->nbr_uint == 0x0F);
    testrun(next == buffer + 2);
    out = cbor_free(out);

    buffer[0] = 0xd9;
    buffer[1] = 0x0F;
    buffer[2] = 0xF0;
    buffer[3] = 0x01;
    buffer[4] = 0x00;
    buffer[5] = 0x00;
    buffer[6] = 0x00;
    buffer[7] = 0x00;
    buffer[8] = 0x00;

    match = decode_tag(buffer, 1, &out, &next);
    testrun(match == ov_CBOR_MATCH_PARTIAL);
    testrun(!out);

    match = decode_tag(buffer, 4, &out, &next);
    testrun(match == ov_CBOR_MATCH_FULL);
    testrun(out);
    testrun(out->type == ov_CBOR_TAG);
    testrun(out->nbr_uint == 0x0FF0);
    testrun(next == buffer + 4);
    out = cbor_free(out);

    buffer[0] = 0xdA;
    buffer[1] = 0x01;
    buffer[2] = 0x02;
    buffer[3] = 0x03;
    buffer[4] = 0x04;
    buffer[5] = 0x00;
    buffer[6] = 0x00;
    buffer[7] = 0x00;
    buffer[8] = 0x00;

    match = decode_tag(buffer, 1, &out, &next);
    testrun(match == ov_CBOR_MATCH_PARTIAL);
    testrun(!out);

    match = decode_tag(buffer, 6, &out, &next);
    testrun(match == ov_CBOR_MATCH_FULL);
    testrun(out);
    testrun(out->type == ov_CBOR_TAG);
    testrun(out->nbr_uint == 0x01020304);
    testrun(next == buffer + 6);
    out = cbor_free(out);

    buffer[0] = 0xdB;
    buffer[1] = 0x01;
    buffer[2] = 0x02;
    buffer[3] = 0x03;
    buffer[4] = 0x04;
    buffer[5] = 0x05;
    buffer[6] = 0x06;
    buffer[7] = 0x07;
    buffer[8] = 0x08;
    buffer[9] = 0x09;
    buffer[10] = 0x10;

    match = decode_tag(buffer, 1, &out, &next);
    testrun(match == ov_CBOR_MATCH_PARTIAL);
    testrun(!out);

    match = decode_tag(buffer, 10, &out, &next);
    testrun(match == ov_CBOR_MATCH_FULL);
    testrun(out);
    testrun(out->type == ov_CBOR_TAG);
    testrun(out->nbr_uint == 0x0102030405060708);
    testrun(next == buffer + 10);
    out = cbor_free(out);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_decode_simple() {

    uint8_t buffer[0xffff] = {0};

    ov_cbor_match match = ov_CBOR_NO_MATCH;
    ov_cbor *out = NULL;
    uint8_t *next = NULL;

    for (int i = 0xE0; i <= 0xF3; i++) {

        buffer[0] = i;

        match = decode_simple(buffer, 1, &out, &next);
        testrun(match == ov_CBOR_MATCH_FULL);
        testrun(out);
        testrun(out->type == ov_CBOR_SIMPLE);
        testrun(out->nbr_uint == (i & 0x1F));
        testrun(next == buffer + 1);
        out = cbor_free(out);
    }

    buffer[0] = 0xf4;
    match = decode_simple(buffer, 1, &out, &next);
    testrun(match == ov_CBOR_MATCH_FULL);
    testrun(out);
    testrun(out->type == ov_CBOR_FALSE);
    testrun(next == buffer + 1);
    out = cbor_free(out);

    buffer[0] = 0xF5;
    match = decode_simple(buffer, 1, &out, &next);
    testrun(match == ov_CBOR_MATCH_FULL);
    testrun(out);
    testrun(out->type == ov_CBOR_TRUE);
    testrun(next == buffer + 1);
    out = cbor_free(out);

    buffer[0] = 0xF6;
    match = decode_simple(buffer, 1, &out, &next);
    testrun(match == ov_CBOR_MATCH_FULL);
    testrun(out);
    testrun(out->type == ov_CBOR_NULL);
    testrun(next == buffer + 1);
    out = cbor_free(out);

    buffer[0] = 0xF7;
    match = decode_simple(buffer, 1, &out, &next);
    testrun(match == ov_CBOR_MATCH_FULL);
    testrun(out);
    testrun(out->type == ov_CBOR_UNDEF);
    testrun(next == buffer + 1);
    out = cbor_free(out);

    buffer[0] = 0xF8;

    for (int i = 0; i <= 0xFF; i++) {

        buffer[1] = i;

        match = decode_simple(buffer, 2, &out, &next);
        testrun(match == ov_CBOR_MATCH_FULL);
        testrun(out);
        testrun(out->type == ov_CBOR_SIMPLE);
        testrun(out->nbr_uint == (uint64_t)i);
        testrun(next == buffer + 2);
        out = cbor_free(out);
    }

    buffer[0] = 0xF9;

    for (int i = 0; i <= 0xFF; i++) {

        buffer[1] = 0x00;
        buffer[2] = i;

        match = decode_simple(buffer, 3, &out, &next);
        testrun(match == ov_CBOR_MATCH_FULL);
        testrun(out);
        testrun(out->type == ov_CBOR_FLOAT);
        testrun(next == buffer + 3);
        out = cbor_free(out);
    }

    buffer[0] = 0xFA;

    for (int i = 0; i <= 0xFF; i++) {

        buffer[1] = 0x00;
        buffer[2] = 0x00;
        buffer[3] = 0x00;
        buffer[4] = i;

        match = decode_simple(buffer, 5, &out, &next);
        testrun(match == ov_CBOR_MATCH_FULL);
        testrun(out);
        testrun(out->type == ov_CBOR_FLOAT);
        testrun(next == buffer + 5);
        out = cbor_free(out);
    }

    buffer[0] = 0xfB;

    for (int i = 0; i <= 0xFF; i++) {

        buffer[1] = 0x00;
        buffer[2] = 0x00;
        buffer[3] = 0x00;
        buffer[4] = 0x00;
        buffer[5] = 0x00;
        buffer[6] = 0x00;
        buffer[7] = i;
        buffer[8] = 0x00;
        buffer[9] = 0x00;

        match = decode_simple(buffer, 9, &out, &next);
        testrun(match == ov_CBOR_MATCH_FULL);
        testrun(out);
        testrun(out->type == ov_CBOR_DOUBLE);
        testrun(next == buffer + 9);
        out = cbor_free(out);
    }

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_cbor_decode() {

    uint8_t buffer[100] = {0};

    ov_cbor_match match = ov_CBOR_NO_MATCH;
    ov_cbor *out = NULL;
    uint8_t *next = NULL;

    buffer[0] = 0x10;
    match = ov_cbor_decode(buffer, 1, &out, &next);
    testrun(match == ov_CBOR_MATCH_FULL);
    testrun(out);
    testrun(out->type == ov_CBOR_UINT64);
    testrun(next == buffer + 1);
    out = cbor_free(out);

    buffer[0] = 0x25;
    match = ov_cbor_decode(buffer, 1, &out, &next);
    testrun(match == ov_CBOR_MATCH_FULL);
    testrun(out);
    testrun(out->type == ov_CBOR_INT64);
    testrun(next == buffer + 1);
    out = cbor_free(out);

    buffer[0] = 0x45;
    buffer[1] = 'a';
    buffer[2] = 'b';
    buffer[3] = 'c';
    buffer[4] = 'd';
    buffer[5] = 'e';
    match = ov_cbor_decode(buffer, 6, &out, &next);
    testrun(match == ov_CBOR_MATCH_FULL);
    testrun(out);
    testrun(out->type == ov_CBOR_STRING);
    testrun(next == buffer + 6);
    out = cbor_free(out);

    buffer[0] = 0x65;
    buffer[1] = 'a';
    buffer[2] = 'b';
    buffer[3] = 'c';
    buffer[4] = 'd';
    buffer[5] = 'e';
    match = ov_cbor_decode(buffer, 6, &out, &next);
    testrun(match == ov_CBOR_MATCH_FULL);
    testrun(out);
    testrun(out->type == ov_CBOR_UTF8);
    testrun(next == buffer + 6);
    out = cbor_free(out);

    buffer[0] = 0x80;
    match = ov_cbor_decode(buffer, 1, &out, &next);
    testrun(match == ov_CBOR_MATCH_FULL);
    testrun(out);
    testrun(out->type == ov_CBOR_ARRAY);
    testrun(next == buffer + 1);
    out = cbor_free(out);

    buffer[0] = 0xA0;
    match = ov_cbor_decode(buffer, 1, &out, &next);
    testrun(match == ov_CBOR_MATCH_FULL);
    testrun(out);
    testrun(out->type == ov_CBOR_MAP);
    testrun(next == buffer + 1);
    out = cbor_free(out);

    buffer[0] = 0xc6;
    match = ov_cbor_decode(buffer, 1, &out, &next);
    testrun(match == ov_CBOR_MATCH_FULL);
    testrun(out);
    testrun(out->type == ov_CBOR_TAG);
    testrun(next == buffer + 1);
    out = cbor_free(out);

    buffer[0] = 0xE0;
    match = ov_cbor_decode(buffer, 1, &out, &next);
    testrun(match == ov_CBOR_MATCH_FULL);
    testrun(out);
    testrun(out->type == ov_CBOR_SIMPLE);
    testrun(next == buffer + 1);
    out = cbor_free(out);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_cbor_encoding_size() {

    ov_cbor *self = ov_cbor_create(ov_CBOR_UNDEF);
    testrun(1 == cbor_encoding_size(self));
    self = cbor_free(self);

    self = ov_cbor_create(ov_CBOR_FALSE);
    testrun(1 == cbor_encoding_size(self));
    self = cbor_free(self);

    self = ov_cbor_create(ov_CBOR_TRUE);
    testrun(1 == cbor_encoding_size(self));
    self = cbor_free(self);

    self = ov_cbor_create(ov_CBOR_NULL);
    testrun(1 == cbor_encoding_size(self));
    self = cbor_free(self);

    self = ov_cbor_create(ov_CBOR_UINT64);
    testrun(1 == cbor_encoding_size(self));
    self->nbr_uint = 0x18;
    testrun(2 == cbor_encoding_size(self));
    self->nbr_uint = 0xFF;
    testrun(2 == cbor_encoding_size(self));
    self->nbr_uint = 0x1FF;
    testrun(3 == cbor_encoding_size(self));
    self->nbr_uint = 0xFFFFffff;
    testrun(5 == cbor_encoding_size(self));
    self->nbr_uint = 0x1FFFFffff;
    testrun(9 == cbor_encoding_size(self));
    self = cbor_free(self);

    self = ov_cbor_create(ov_CBOR_INT64);
    testrun(1 == cbor_encoding_size(self));
    self->nbr_int = -0x18;
    testrun(2 == cbor_encoding_size(self));
    self->nbr_int = -0xFF;
    testrun(2 == cbor_encoding_size(self));
    self->nbr_int = -0x1FF;
    testrun(3 == cbor_encoding_size(self));
    self->nbr_uint = -0xEFFFffff;
    testrun(5 == cbor_encoding_size(self));
    self->nbr_int = -0x1FFFFffff;
    testrun(9 == cbor_encoding_size(self));
    self = cbor_free(self);

    self = ov_cbor_create(ov_CBOR_STRING);
    testrun(1 == cbor_encoding_size(self));
    self->nbr_uint = 0x18;
    testrun(2 + 0x18 == cbor_encoding_size(self));
    self->nbr_uint = 0xFF;
    testrun(2 + 0xFF == cbor_encoding_size(self));
    self->nbr_uint = 0x1FF;
    testrun(3 + 0x1FF == cbor_encoding_size(self));
    self->nbr_uint = 0x1FFFFffff;
    testrun(9 + 0x1FFFFffff == cbor_encoding_size(self));
    self = cbor_free(self);

    self = ov_cbor_create(ov_CBOR_UTF8);
    testrun(1 == cbor_encoding_size(self));
    self->nbr_uint = 0x18;
    testrun(2 + 0x18 == cbor_encoding_size(self));
    self->nbr_uint = 0xFF;
    testrun(2 + 0xFF == cbor_encoding_size(self));
    self->nbr_uint = 0x1FF;
    testrun(3 + 0x1FF == cbor_encoding_size(self));
    self->nbr_uint = 0x1FFFFffff;
    testrun(9 + 0x1FFFFffff == cbor_encoding_size(self));
    self = cbor_free(self);

    self = ov_cbor_create(ov_CBOR_ARRAY);
    testrun(1 == cbor_encoding_size(self));
    self->data =
        ov_linked_list_create((ov_list_config){.item.free = cbor_free});
    testrun(ov_list_push(self->data, ov_cbor_create(ov_CBOR_FALSE)))
        testrun(2 == cbor_encoding_size(self));
    testrun(ov_list_push(self->data, ov_cbor_create(ov_CBOR_FALSE)))
        testrun(3 == cbor_encoding_size(self));
    testrun(ov_list_push(self->data, ov_cbor_create(ov_CBOR_FALSE)))
        testrun(4 == cbor_encoding_size(self));
    testrun(ov_list_push(self->data, ov_cbor_create(ov_CBOR_FALSE)))
        testrun(5 == cbor_encoding_size(self));
    testrun(ov_list_push(self->data, ov_cbor_create(ov_CBOR_FALSE)))
        testrun(6 == cbor_encoding_size(self));
    testrun(ov_list_push(self->data, ov_cbor_create(ov_CBOR_FALSE)))
        testrun(7 == cbor_encoding_size(self));
    testrun(ov_list_push(self->data, ov_cbor_create(ov_CBOR_FALSE)))
        testrun(8 == cbor_encoding_size(self));
    testrun(ov_list_push(self->data, ov_cbor_create(ov_CBOR_FALSE)))
        testrun(9 == cbor_encoding_size(self));
    testrun(ov_list_push(self->data, ov_cbor_create(ov_CBOR_FALSE)))
        testrun(10 == cbor_encoding_size(self));
    testrun(ov_list_push(self->data, ov_cbor_create(ov_CBOR_FALSE)))
        testrun(11 == cbor_encoding_size(self));
    testrun(ov_list_push(self->data, ov_cbor_create(ov_CBOR_FALSE)))
        testrun(12 == cbor_encoding_size(self));
    testrun(ov_list_push(self->data, ov_cbor_create(ov_CBOR_FALSE)))
        testrun(13 == cbor_encoding_size(self));
    testrun(ov_list_push(self->data, ov_cbor_create(ov_CBOR_FALSE)))
        testrun(14 == cbor_encoding_size(self));
    testrun(ov_list_push(self->data, ov_cbor_create(ov_CBOR_FALSE)))
        testrun(15 == cbor_encoding_size(self));
    testrun(ov_list_push(self->data, ov_cbor_create(ov_CBOR_FALSE)))
        testrun(16 == cbor_encoding_size(self));
    testrun(ov_list_push(self->data, ov_cbor_create(ov_CBOR_FALSE)))
        testrun(17 == cbor_encoding_size(self));
    testrun(ov_list_push(self->data, ov_cbor_create(ov_CBOR_FALSE)))
        testrun(18 == cbor_encoding_size(self));
    testrun(ov_list_push(self->data, ov_cbor_create(ov_CBOR_FALSE)))
        testrun(19 == cbor_encoding_size(self));
    testrun(ov_list_push(self->data, ov_cbor_create(ov_CBOR_FALSE)))
        testrun(20 == cbor_encoding_size(self));
    testrun(ov_list_push(self->data, ov_cbor_create(ov_CBOR_FALSE)))
        testrun(21 == cbor_encoding_size(self));
    testrun(ov_list_push(self->data, ov_cbor_create(ov_CBOR_FALSE)))
        testrun(22 == cbor_encoding_size(self));
    testrun(ov_list_push(self->data, ov_cbor_create(ov_CBOR_FALSE)))
        testrun(23 == cbor_encoding_size(self));
    testrun(ov_list_push(self->data, ov_cbor_create(ov_CBOR_FALSE)))
        testrun(24 == cbor_encoding_size(self));
    testrun(ov_list_push(self->data, ov_cbor_create(ov_CBOR_FALSE)))
        testrun(26 == cbor_encoding_size(self));
    self = cbor_free(self);

    self = ov_cbor_create(ov_CBOR_MAP);
    testrun(0 == cbor_encoding_size(self));
    self->data = ov_dict_create(ov_cbor_dict_config(255));
    testrun(1 == cbor_encoding_size(self));
    testrun(ov_dict_set(self->data, ov_cbor_create(ov_CBOR_UINT64),
                         ov_cbor_create(ov_CBOR_UINT64), NULL));
    testrun(3 == cbor_encoding_size(self));
    self = cbor_free(self);

    self = ov_cbor_create(ov_CBOR_DATE_TIME);
    testrun(1 == cbor_encoding_size(self));
    self->nbr_uint = 0x18;
    testrun(2 + 0x18 == cbor_encoding_size(self));
    self->nbr_uint = 0xFF;
    testrun(2 + 0xFF == cbor_encoding_size(self));
    self->nbr_uint = 0x1FF;
    testrun(3 + 0x1FF == cbor_encoding_size(self));
    self->nbr_uint = 0x1FFFFffff;
    testrun(9 + 0x1FFFFffff == cbor_encoding_size(self));
    self = cbor_free(self);

    self = ov_cbor_create(ov_CBOR_DATE_TIME_EPOCH);
    testrun(1 == cbor_encoding_size(self));
    self->nbr_uint = 0x18;
    testrun(2 == cbor_encoding_size(self));
    self->nbr_uint = 0xFF;
    testrun(2 == cbor_encoding_size(self));
    self->nbr_uint = 0x1FF;
    testrun(3 == cbor_encoding_size(self));
    self->nbr_uint = 0x1FFFFffff;
    testrun(9 == cbor_encoding_size(self));
    self = cbor_free(self);

    self = ov_cbor_create(ov_CBOR_UBIGNUM);
    testrun(1 == cbor_encoding_size(self));
    self->nbr_uint = 0x18;
    testrun(2 + 0x18 == cbor_encoding_size(self));
    self->nbr_uint = 0xFF;
    testrun(2 + 0xFF == cbor_encoding_size(self));
    self->nbr_uint = 0x1FF;
    testrun(3 + 0x1FF == cbor_encoding_size(self));
    self->nbr_uint = 0x1FFFFffff;
    testrun(9 + 0x1FFFFffff == cbor_encoding_size(self));
    self = cbor_free(self);

    self = ov_cbor_create(ov_CBOR_IBIGNUM);
    testrun(1 == cbor_encoding_size(self));
    self->nbr_uint = 0x18;
    testrun(2 + 0x18 == cbor_encoding_size(self));
    self->nbr_uint = 0xFF;
    testrun(2 + 0xFF == cbor_encoding_size(self));
    self->nbr_uint = 0x1FF;
    testrun(3 + 0x1FF == cbor_encoding_size(self));
    self->nbr_uint = 0x1FFFFffff;
    testrun(9 + 0x1FFFFffff == cbor_encoding_size(self));
    self = cbor_free(self);

    self = ov_cbor_create(ov_CBOR_DEC_FRACTION);
    testrun(1 == cbor_encoding_size(self));
    self->data = ov_cbor_array();
    testrun(ov_cbor_array_push(self->data, ov_cbor_create(ov_CBOR_FALSE)))
        testrun(3 == cbor_encoding_size(self));
    self = cbor_free(self);

    self = ov_cbor_create(ov_CBOR_BIGFLOAT);
    testrun(1 == cbor_encoding_size(self));
    self->data = ov_cbor_array();
    testrun(ov_cbor_array_push(self->data, ov_cbor_create(ov_CBOR_FALSE)))
        testrun(3 == cbor_encoding_size(self));
    self = cbor_free(self);

    ov_cbor *child = ov_cbor_create(ov_CBOR_STRING);
    self = ov_cbor_create(ov_CBOR_TAG);
    self->data = child;
    testrun(2 == cbor_encoding_size(self));
    child->nbr_uint = 0x18;
    testrun(3 + 0x18 == cbor_encoding_size(self));
    child->nbr_uint = 0xFF;
    testrun(3 + 0xFF == cbor_encoding_size(self));
    child->nbr_uint = 0x1FF;
    testrun(4 + 0x1FF == cbor_encoding_size(self));
    child->nbr_uint = 0x1FFFFffff;
    testrun(10 + 0x1FFFFffff == cbor_encoding_size(self));
    self = cbor_free(self);

    self = ov_cbor_create(ov_CBOR_SIMPLE);
    testrun(1 == cbor_encoding_size(self));
    self->nbr_uint = 0x18;
    testrun(2 == cbor_encoding_size(self));
    self->nbr_uint = 0xFF;
    testrun(2 == cbor_encoding_size(self));
    self->nbr_uint = 0x1FF;
    testrun(3 == cbor_encoding_size(self));
    self->nbr_uint = 0xFFFFffff;
    testrun(5 == cbor_encoding_size(self));
    self->nbr_uint = 0x1FFFFffff;
    testrun(9 == cbor_encoding_size(self));
    self = cbor_free(self);

    self = ov_cbor_create(ov_CBOR_FLOAT);
    self->nbr_float = 1.2;
    testrun(9 == cbor_encoding_size(self));
    self->nbr_float = 1.2000000001323;
    testrun(9 == cbor_encoding_size(self));
    self = cbor_free(self);

    self = ov_cbor_create(ov_CBOR_DOUBLE);
    self->nbr_float = 1.2;
    testrun(13 == cbor_encoding_size(self));
    self->nbr_float = 1.2000000001323;
    testrun(13 == cbor_encoding_size(self));
    self = cbor_free(self);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_encode_uint() {

    uint8_t buffer[0xffff] = {0};
    uint8_t *next = 0;
    size_t size = 0xffff;

    ov_cbor *self = ov_cbor_create(ov_CBOR_UINT64);
    self->nbr_uint = 0;

    testrun(!encode_uint(self, buffer, 0, &next));
    testrun(!encode_uint(NULL, buffer, size, &next));
    testrun(!encode_uint(self, NULL, size, &next));
    testrun(!encode_uint(self, buffer, size, NULL));

    self->nbr_uint = 0;
    testrun(encode_uint(self, buffer, size, &next));
    testrun(next == buffer + 1);
    testrun(buffer[0] == 0x00);
    testrun(buffer[1] == 0x00);

    self->nbr_uint = 1;
    testrun(encode_uint(self, buffer, size, &next));
    testrun(next == buffer + 1);
    testrun(buffer[0] == 0x01);
    testrun(buffer[1] == 0x00);

    self->nbr_uint = 0x17;
    testrun(encode_uint(self, buffer, size, &next));
    testrun(next == buffer + 1);
    testrun(buffer[0] == 0x17);
    testrun(buffer[1] == 0x00);

    self->nbr_uint = 0x18;
    testrun(encode_uint(self, buffer, size, &next));
    testrun(next == buffer + 2);
    testrun(buffer[0] == 0x18);
    testrun(buffer[1] == 0x18);
    testrun(buffer[2] == 0x00);

    self->nbr_uint = 0xFF;
    testrun(encode_uint(self, buffer, size, &next));
    testrun(next == buffer + 2);
    testrun(buffer[0] == 0x18);
    testrun(buffer[1] == 0xFF);
    testrun(buffer[2] == 0x00);

    self->nbr_uint = 0x1FF;
    testrun(encode_uint(self, buffer, size, &next));
    testrun(next == buffer + 3);
    testrun(buffer[0] == 0x19);
    testrun(buffer[1] == 0x01);
    testrun(buffer[2] == 0xFF);
    testrun(buffer[3] == 0x00);

    self->nbr_uint = 0xFFFF;
    testrun(encode_uint(self, buffer, size, &next));
    testrun(next == buffer + 3);
    testrun(buffer[0] == 0x19);
    testrun(buffer[1] == 0xFF);
    testrun(buffer[2] == 0xFF);
    testrun(buffer[3] == 0x00);

    self->nbr_uint = 0x1FFFF;
    testrun(encode_uint(self, buffer, size, &next));
    testrun(next == buffer + 5);
    testrun(buffer[0] == 0x1A);
    testrun(buffer[1] == 0x00);
    testrun(buffer[2] == 0x01);
    testrun(buffer[3] == 0xFF);
    testrun(buffer[4] == 0xFF);
    testrun(buffer[5] == 0x00);
    testrun(buffer[6] == 0x00);

    self->nbr_uint = 0xFFFFffff;
    testrun(encode_uint(self, buffer, size, &next));
    testrun(next == buffer + 5);
    testrun(buffer[0] == 0x1A);
    testrun(buffer[1] == 0xFF);
    testrun(buffer[2] == 0xFF);
    testrun(buffer[3] == 0xFF);
    testrun(buffer[4] == 0xFF);
    testrun(buffer[5] == 0x00);
    testrun(buffer[6] == 0x00);

    self->nbr_uint = 0x1FFFFffff;
    testrun(encode_uint(self, buffer, size, &next));
    testrun(next == buffer + 9);
    testrun(buffer[0] == 0x1B);
    testrun(buffer[1] == 0x00);
    testrun(buffer[2] == 0x00);
    testrun(buffer[3] == 0x00);
    testrun(buffer[4] == 0x01);
    testrun(buffer[5] == 0xFF);
    testrun(buffer[6] == 0xFF);
    testrun(buffer[7] == 0xFF);
    testrun(buffer[8] == 0xFF);
    testrun(buffer[9] == 0x00);

    ov_cbor *copy = NULL;
    testrun(ov_CBOR_MATCH_FULL == decode_uint(buffer, size, &copy, &next));
    testrun(next == buffer + 9);
    testrun(copy->nbr_uint == self->nbr_uint);

    copy = cbor_free(copy);
    self = cbor_free(self);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_encode_int() {

    uint8_t buffer[0xffff] = {0};
    uint8_t *next = 0;
    size_t size = 0xffff;

    ov_cbor *self = ov_cbor_create(ov_CBOR_INT64);
    self->nbr_uint = 0;

    testrun(!encode_int(self, buffer, 0, &next));
    testrun(!encode_int(NULL, buffer, size, &next));
    testrun(!encode_int(self, NULL, size, &next));
    testrun(!encode_int(self, buffer, size, NULL));

    self->nbr_int = 0;
    testrun(encode_int(self, buffer, size, &next));
    testrun(next == buffer + 1);
    testrun(buffer[0] == 0x20);
    testrun(buffer[1] == 0x00);

    self->nbr_int = -1;
    testrun(encode_int(self, buffer, size, &next));
    testrun(next == buffer + 1);
    testrun(buffer[0] == 0x21);
    testrun(buffer[1] == 0x00);

    self->nbr_int = -0x17;
    testrun(encode_int(self, buffer, size, &next));
    testrun(next == buffer + 1);
    testrun(buffer[0] == (0x30 | 0x17));
    testrun(buffer[1] == 0x00);

    self->nbr_int = -0x18;
    testrun(encode_int(self, buffer, size, &next));
    testrun(next == buffer + 2);
    testrun(buffer[0] == 0x38);
    testrun(buffer[1] == 0x18);
    testrun(buffer[2] == 0x00);

    self->nbr_int = -0xFF;
    testrun(encode_int(self, buffer, size, &next));
    testrun(next == buffer + 2);
    testrun(buffer[0] == 0x38);
    testrun(buffer[1] == 0xFF);
    testrun(buffer[2] == 0x00);

    self->nbr_int = -0x1FF;
    testrun(encode_int(self, buffer, size, &next));
    testrun(next == buffer + 3);
    testrun(buffer[0] == 0x39);
    testrun(buffer[1] == 0x01);
    testrun(buffer[2] == 0xFF);
    testrun(buffer[3] == 0x00);

    self->nbr_int = -0xFFFF;
    testrun(encode_int(self, buffer, size, &next));
    testrun(next == buffer + 3);
    testrun(buffer[0] == 0x39);
    testrun(buffer[1] == 0xFF);
    testrun(buffer[2] == 0xFF);
    testrun(buffer[3] == 0x00);

    self->nbr_int = -0x1FFFF;
    testrun(encode_int(self, buffer, size, &next));
    testrun(next == buffer + 5);
    testrun(buffer[0] == 0x3A);
    testrun(buffer[1] == 0x00);
    testrun(buffer[2] == 0x01);
    testrun(buffer[3] == 0xFF);
    testrun(buffer[4] == 0xFF);
    testrun(buffer[5] == 0x00);
    testrun(buffer[6] == 0x00);

    self->nbr_int = -0x1FFFFffff;
    testrun(encode_int(self, buffer, size, &next));
    testrun(next == buffer + 9);
    testrun(buffer[0] == 0x3B);
    testrun(buffer[1] == 0x00);
    testrun(buffer[2] == 0x00);
    testrun(buffer[3] == 0x00);
    testrun(buffer[4] == 0x01);
    testrun(buffer[5] == 0xFF);
    testrun(buffer[6] == 0xFF);
    testrun(buffer[7] == 0xFF);
    testrun(buffer[8] == 0xFF);
    testrun(buffer[9] == 0x00);

    ov_cbor *copy = NULL;
    testrun(ov_CBOR_MATCH_FULL == decode_int(buffer, size, &copy, &next));
    testrun(next == buffer + 9);
    testrun(copy->nbr_int == self->nbr_int);

    copy = cbor_free(copy);
    self = cbor_free(self);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_encode_string() {

    uint8_t buffer[0xffff] = {0};
    uint8_t *next = 0;
    size_t size = 0xffff;

    ov_cbor *self = ov_cbor_create(ov_CBOR_STRING);
    self->nbr_uint = 0;

    testrun(!encode_string(self, buffer, 0, &next));
    testrun(!encode_string(NULL, buffer, size, &next));
    testrun(!encode_string(self, NULL, size, &next));
    testrun(!encode_string(self, buffer, size, NULL));

    self->nbr_int = 0;
    self->string = NULL;
    testrun(encode_string(self, buffer, size, &next));
    testrun(next == buffer + 1);
    testrun(buffer[0] == 0x40);
    testrun(buffer[1] == 0x00);

    self->string = ov_string_dup("test");
    self->nbr_uint = strlen(self->string);
    testrun(encode_string(self, buffer, size, &next));
    testrun(next == buffer + 5);
    testrun(buffer[0] == 0x44);
    testrun(buffer[1] == 't');
    testrun(buffer[2] == 'e');
    testrun(buffer[3] == 's');
    testrun(buffer[4] == 't');
    testrun(buffer[5] == 0x00);

    self->string = ov_data_pointer_free(self->string);
    testrun(ov_random_string(&self->string, 0xff, NULL));
    self->nbr_uint = strlen(self->string);

    testrun(encode_string(self, buffer, size, &next));
    testrun(next == buffer + 2 + strlen(self->string));
    testrun(buffer[0] == 0x58);
    for (size_t i = 0; i < strlen(self->string); i++) {
        testrun(buffer[2 + i] != 0x00);
    }
    testrun(next[0] == 0x00);

    /*
        // commented due to very large sizes
        self->string = ov_data_pointer_free(self->string);
        testrun(ov_random_string(&self->string, 0xffFF, NULL));
        self->nbr_uint = strlen(self->string);


        ov_buffer *buf = ov_buffer_create(0xffffFFF);
        testrun(buf);

        testrun(encode_string(self, buf->start, buf->capacity, &next));
        testrun(next == buf->start + 3 + strlen(self->string));
        testrun(buf->start[0] == 0x59);
        for (size_t i = 0; i < strlen(self->string); i++){
            testrun(buf->start[3 + i] != 0x00);
        }
        testrun(next[0] == 0x00);
        buf = ov_buffer_free(buf);
    */

    self = cbor_free(self);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_encode_uft8() {

    uint8_t buffer[0xffff] = {0};
    uint8_t *next = 0;
    size_t size = 0xffff;

    ov_cbor *self = ov_cbor_create(ov_CBOR_UTF8);
    self->nbr_uint = 0;

    testrun(!encode_uft8(self, buffer, 0, &next));
    testrun(!encode_uft8(NULL, buffer, size, &next));
    testrun(!encode_uft8(self, NULL, size, &next));
    testrun(!encode_uft8(self, buffer, size, NULL));

    self->nbr_int = 0;
    self->string = NULL;
    testrun(encode_uft8(self, buffer, size, &next));
    testrun(next == buffer + 1);
    testrun(buffer[0] == 0x60);
    testrun(buffer[1] == 0x00);

    self->string = ov_string_dup("test");
    self->nbr_uint = strlen(self->string);
    testrun(encode_uft8(self, buffer, size, &next));
    testrun(next == buffer + 5);
    testrun(buffer[0] == 0x64);
    testrun(buffer[1] == 't');
    testrun(buffer[2] == 'e');
    testrun(buffer[3] == 's');
    testrun(buffer[4] == 't');
    testrun(buffer[5] == 0x00);

    self->string = ov_data_pointer_free(self->string);
    testrun(ov_random_string(&self->string, 0xff, NULL));
    self->nbr_uint = strlen(self->string);

    testrun(encode_uft8(self, buffer, size, &next));
    testrun(next == buffer + 2 + strlen(self->string));
    testrun(buffer[0] == 0x78);
    for (size_t i = 0; i < strlen(self->string); i++) {
        testrun(buffer[2 + i] != 0x00);
    }
    testrun(next[0] == 0x00);

    self->string = ov_data_pointer_free(self->string);
    testrun(ov_random_string(&self->string, 0x1ff, NULL));
    self->nbr_uint = strlen(self->string);

    testrun(encode_uft8(self, buffer, size, &next));
    testrun(next == buffer + 3 + strlen(self->string));
    testrun(buffer[0] == 0x79);
    for (size_t i = 0; i < strlen(self->string); i++) {
        testrun(buffer[2 + i] != 0x00);
    }
    testrun(next[0] == 0x00);

    memset(buffer, 0, size);
    self->string = ov_data_pointer_free(self->string);
    testrun(ov_utf8_generate_random_buffer(&self->bytes, &self->nbr_uint, 30));

    testrun(encode_uft8(self, buffer, size, &next));
    testrun(buffer[0] == 0x78);
    testrun(next[0] == 0x00);

    self = cbor_free(self);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_encode_array() {

    uint8_t buffer[0xffff] = {0};
    uint8_t *next = 0;
    size_t size = 0xffff;

    ov_cbor *self = ov_cbor_create(ov_CBOR_ARRAY);
    self->nbr_uint = 0;

    self->data = NULL;
    testrun(encode_array(self, buffer, size, &next));
    testrun(next == buffer + 1);
    testrun(buffer[0] == 0x80);
    testrun(buffer[1] == 0x00);

    self->data =
        ov_linked_list_create((ov_list_config){.item.free = cbor_free});

    testrun(ov_list_push(self->data, ov_cbor_create(ov_CBOR_TRUE)));

    testrun(encode_array(self, buffer, size, &next));
    testrun(next == buffer + 2);
    testrun(buffer[0] == 0x81);
    testrun(buffer[1] == 0xf5);
    testrun(buffer[2] == 0x00);

    ov_cbor *string = ov_cbor_create(ov_CBOR_STRING);
    string->string = ov_string_dup("test");
    string->nbr_uint = strlen(self->string);
    testrun(ov_list_push(self->data, string));

    testrun(encode_array(self, buffer, size, &next));
    testrun(next == buffer + 7);
    testrun(buffer[0] == 0x82);
    testrun(buffer[1] == 0xf5);
    testrun(buffer[2] == 0x44);
    testrun(buffer[3] == 't');
    testrun(buffer[4] == 'e');
    testrun(buffer[5] == 's');
    testrun(buffer[6] == 't');
    testrun(buffer[7] == 0x00);

    memset(buffer, 0, size);
    testrun(!encode_array(self, buffer, 6, &next));

    string = ov_cbor_create(ov_CBOR_STRING);
    testrun(ov_random_string(&string->string, 0xff, NULL));
    string->nbr_uint = strlen(string->string);
    testrun(ov_list_push(self->data, string));

    testrun(encode_array(self, buffer, size, &next));
    ov_dump_binary_as_hex(stdout, buffer, 10);
    testrun(buffer[0] == 0x83);
    testrun(buffer[1] == 0xf5);
    testrun(buffer[2] == 0x44);
    testrun(buffer[3] == 't');
    testrun(buffer[4] == 'e');
    testrun(buffer[5] == 's');
    testrun(buffer[6] == 't');
    testrun(buffer[7] == 0x58);
    testrun(next[0] == 0x00);

    self = cbor_free(self);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_encode_map() {

    uint8_t buffer[0xffff] = {0};
    uint8_t *next = 0;
    size_t size = 0xffff;

    ov_cbor *self = ov_cbor_create(ov_CBOR_MAP);
    self->nbr_uint = 0;

    self->data = NULL;
    testrun(encode_map(self, buffer, size, &next));
    testrun(next == buffer + 1);
    testrun(buffer[0] == 0xA0);
    testrun(buffer[1] == 0x00);

    self->data = ov_dict_create(ov_cbor_dict_config(255));
    testrun(encode_map(self, buffer, size, &next));
    testrun(next == buffer + 1);
    testrun(buffer[0] == 0xA0);
    testrun(buffer[1] == 0x00);

    ov_cbor *key = ov_cbor_create(ov_CBOR_STRING);
    key->string = ov_string_dup("key");
    key->nbr_uint = strlen(key->string);

    ov_cbor *val = ov_cbor_create(ov_CBOR_STRING);
    val->string = ov_string_dup("val");
    val->nbr_uint = strlen(val->string);

    testrun(ov_dict_set(self->data, key, val, NULL));
    testrun(encode_map(self, buffer, size, &next));
    testrun(next == buffer + 9);
    testrun(buffer[0] == 0xA8);
    testrun(buffer[1] == 0x43);
    testrun(buffer[2] == 'k');
    testrun(buffer[3] == 'e');
    testrun(buffer[4] == 'y');
    testrun(buffer[5] == 0x43);
    testrun(buffer[6] == 'v');
    testrun(buffer[7] == 'a');
    testrun(buffer[8] == 'l');
    testrun(buffer[9] == 0x00);

    memset(buffer, 0, size);
    testrun(!encode_map(self, buffer, 1, &next));
    testrun(!encode_map(self, buffer, 2, &next));
    testrun(!encode_map(self, buffer, 3, &next));
    testrun(!encode_map(self, buffer, 4, &next));
    testrun(!encode_map(self, buffer, 5, &next));
    testrun(!encode_map(self, buffer, 6, &next));
    testrun(!encode_map(self, buffer, 7, &next));
    testrun(!encode_map(self, buffer, 8, &next));
    testrun(buffer[0] == 0x00);

    key = ov_cbor_create(ov_CBOR_STRING);
    key->string = ov_string_dup("key2");
    key->nbr_uint = strlen(key->string);
    val = ov_cbor_create(ov_CBOR_STRING);
    testrun(ov_random_string(&val->string, 0xff, NULL));
    val->nbr_uint = strlen(val->string);
    testrun(ov_dict_set(self->data, key, val, NULL));
    testrun(encode_map(self, buffer, size, &next));
    testrun(next == buffer + 2 + 9 + 1 + 6 + val->nbr_uint);
    testrun(buffer[0] == 0xb9);
    testrun(buffer[1] == 0x01);
    testrun(buffer[2] == 0x0d);
    testrun(buffer[3] == 0x44);
    testrun(next[0] == 0x00);

    self = cbor_free(self);

    memset(buffer, 0, size);
    self = ov_cbor_create(ov_CBOR_MAP);
    self->data = ov_dict_create(ov_cbor_dict_config(255));
    key = ov_cbor_create(ov_CBOR_ARRAY);
    key->data =
        ov_linked_list_create((ov_list_config){.item.free = cbor_free});
    val = ov_cbor_create(ov_CBOR_UINT64);
    val->nbr_uint = 1;
    testrun(ov_dict_set(self->data, key, val, NULL));
    testrun(encode_map(self, buffer, size, &next));
    testrun(next == buffer + 3);
    testrun(buffer[0] == 0xA2);
    testrun(buffer[1] == 0x80);
    testrun(buffer[2] == 0x01);
    testrun(buffer[3] == 0x00);
    testrun(next[0] == 0x00);

    val = ov_cbor_create(ov_CBOR_NULL);
    testrun(ov_list_push(key->data, val));
    testrun(encode_map(self, buffer, size, &next));
    testrun(next == buffer + 4);
    testrun(buffer[0] == 0xA3);
    testrun(buffer[1] == 0x81);
    testrun(buffer[2] == 0xf6);
    testrun(buffer[3] == 0x01);
    testrun(next[0] == 0x00);

    self = cbor_free(self);
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_encode_date_time() {

    uint8_t buffer[0xffff] = {0};
    uint8_t *next = 0;
    size_t size = 0xffff;

    ov_cbor *self = ov_cbor_create(ov_CBOR_DATE_TIME);
    self->nbr_uint = 0;
    self->string = NULL;

    testrun(encode_date_time(self, buffer, size, &next));
    testrun(next == buffer + 2);
    testrun(buffer[0] == 0xC0);
    testrun(buffer[1] == 0x40);

    self->string = ov_string_dup("time");
    self->nbr_uint = strlen(self->string);
    testrun(encode_date_time(self, buffer, size, &next));
    testrun(next == buffer + 6);
    testrun(buffer[0] == 0xC0);
    testrun(buffer[1] == 0x44);
    testrun(buffer[2] == 't');
    testrun(buffer[3] == 'i');
    testrun(buffer[4] == 'm');
    testrun(buffer[5] == 'e');
    testrun(buffer[6] == 0x00);

    self->string = ov_data_pointer_free(self->string);
    testrun(ov_random_string(&self->string, 0xff, NULL));
    self->nbr_uint = strlen(self->string);

    testrun(encode_date_time(self, buffer, size, &next));
    testrun(next == buffer + 3 + self->nbr_uint);
    testrun(buffer[0] == 0xC0);
    testrun(buffer[1] == 0x58);
    testrun(buffer[2] == 0xfe);

    self = cbor_free(self);
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_encode_date_time_epoch() {

    uint8_t buffer[0xffff] = {0};
    uint8_t *next = 0;
    size_t size = 0xffff;

    ov_cbor *self = ov_cbor_create(ov_CBOR_DATE_TIME_EPOCH);
    self->nbr_uint = 0;

    testrun(!encode_date_time_epoch(self, buffer, 0, &next));
    testrun(!encode_date_time_epoch(NULL, buffer, size, &next));
    testrun(!encode_date_time_epoch(self, NULL, size, &next));
    testrun(!encode_date_time_epoch(self, buffer, size, NULL));

    self->nbr_uint = 0;
    testrun(encode_date_time_epoch(self, buffer, size, &next));
    testrun(next == buffer + 2);
    testrun(buffer[0] == 0xc1);
    testrun(buffer[1] == 0x00);

    self->nbr_uint = 1;
    testrun(encode_date_time_epoch(self, buffer, size, &next));
    testrun(next == buffer + 2);
    testrun(buffer[0] == 0xc1);
    testrun(buffer[1] == 0x01);

    self->nbr_uint = 0x17;
    testrun(encode_date_time_epoch(self, buffer, size, &next));
    testrun(next == buffer + 2);
    testrun(buffer[0] == 0xc1);
    testrun(buffer[1] == 0x17);

    self->nbr_uint = 0x18;
    testrun(encode_date_time_epoch(self, buffer, size, &next));
    testrun(next == buffer + 3);
    testrun(buffer[0] == 0xc1);
    testrun(buffer[1] == 0x18);
    testrun(buffer[2] == 0x18);

    self->nbr_uint = 0xFF;
    testrun(encode_date_time_epoch(self, buffer, size, &next));
    testrun(next == buffer + 3);
    testrun(buffer[0] == 0xc1);
    testrun(buffer[1] == 0x18);
    testrun(buffer[2] == 0xFF);

    self->nbr_uint = 0x1FF;
    testrun(encode_date_time_epoch(self, buffer, size, &next));
    testrun(next == buffer + 4);
    testrun(buffer[0] == 0xc1);
    testrun(buffer[1] == 0x19);
    testrun(buffer[2] == 0x01);
    testrun(buffer[3] == 0xFF);

    self->nbr_uint = 0xFFFF;
    testrun(encode_date_time_epoch(self, buffer, size, &next));
    testrun(next == buffer + 4);
    testrun(buffer[0] == 0xc1);
    testrun(buffer[1] == 0x19);
    testrun(buffer[2] == 0xFF);
    testrun(buffer[3] == 0xFF);

    self->nbr_uint = 0x1FFFF;
    testrun(encode_date_time_epoch(self, buffer, size, &next));
    testrun(next == buffer + 6);
    testrun(buffer[0] == 0xC1);
    testrun(buffer[1] == 0x1A);
    testrun(buffer[2] == 0x00);
    testrun(buffer[3] == 0x01);
    testrun(buffer[4] == 0xFF);
    testrun(buffer[5] == 0xFF);
    testrun(buffer[6] == 0x00);

    self->nbr_uint = 0xFFFFffff;
    testrun(encode_date_time_epoch(self, buffer, size, &next));
    testrun(next == buffer + 6);
    testrun(buffer[0] == 0xC1);
    testrun(buffer[1] == 0x1A);
    testrun(buffer[2] == 0xFF);
    testrun(buffer[3] == 0xFF);
    testrun(buffer[4] == 0xFF);
    testrun(buffer[5] == 0xFF);
    testrun(buffer[6] == 0x00);

    self->nbr_uint = 0x1FFFFffff;
    testrun(encode_date_time_epoch(self, buffer, size, &next));
    testrun(next == buffer + 10);
    testrun(buffer[0] == 0xC1);
    testrun(buffer[1] == 0x1B);
    testrun(buffer[2] == 0x00);
    testrun(buffer[3] == 0x00);
    testrun(buffer[4] == 0x00);
    testrun(buffer[5] == 0x01);
    testrun(buffer[6] == 0xFF);
    testrun(buffer[7] == 0xFF);
    testrun(buffer[8] == 0xFF);
    testrun(buffer[9] == 0xFF);

    ov_cbor *copy = NULL;
    testrun(ov_CBOR_MATCH_FULL == decode_tag(buffer, size, &copy, &next));
    testrun(next == buffer + 10);
    testrun(copy->nbr_uint == self->nbr_uint);

    copy = cbor_free(copy);
    self = cbor_free(self);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_encode_ubignum() {

    uint8_t buffer[0xffff] = {0};
    uint8_t *next = 0;
    size_t size = 0xffff;

    ov_cbor *self = ov_cbor_create(ov_CBOR_UBIGNUM);
    self->nbr_uint = 0;

    testrun(!encode_ubignum(self, buffer, 0, &next));
    testrun(!encode_ubignum(NULL, buffer, size, &next));
    testrun(!encode_ubignum(self, NULL, size, &next));
    testrun(!encode_ubignum(self, buffer, size, NULL));

    self->nbr_int = 0;
    self->string = NULL;
    testrun(encode_ubignum(self, buffer, size, &next));
    testrun(next == buffer + 2);
    testrun(buffer[0] == 0xC2);
    testrun(buffer[1] == 0x40);

    self->string = ov_string_dup("test");
    self->nbr_uint = strlen(self->string);
    testrun(encode_ubignum(self, buffer, size, &next));
    testrun(next == buffer + 6);
    testrun(buffer[0] == 0xC2) testrun(buffer[1] == 0x44);
    testrun(buffer[2] == 't');
    testrun(buffer[3] == 'e');
    testrun(buffer[4] == 's');
    testrun(buffer[5] == 't');
    testrun(buffer[6] == 0x00);

    self->string = ov_data_pointer_free(self->string);
    testrun(ov_random_string(&self->string, 0xff, NULL));
    self->nbr_uint = strlen(self->string);

    testrun(encode_ubignum(self, buffer, size, &next));
    testrun(next == buffer + 3 + strlen(self->string));
    testrun(buffer[0] == 0xC2);
    for (size_t i = 0; i < strlen(self->string); i++) {
        testrun(buffer[2 + i] != 0x00);
    }
    testrun(next[0] == 0x00);

    self = cbor_free(self);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_encode_ibignum() {

    uint8_t buffer[0xffff] = {0};
    uint8_t *next = 0;
    size_t size = 0xffff;

    ov_cbor *self = ov_cbor_create(ov_CBOR_IBIGNUM);
    self->nbr_uint = 0;

    testrun(!encode_ibignum(self, buffer, 0, &next));
    testrun(!encode_ibignum(NULL, buffer, size, &next));
    testrun(!encode_ibignum(self, NULL, size, &next));
    testrun(!encode_ibignum(self, buffer, size, NULL));

    self->nbr_int = 0;
    self->string = NULL;
    testrun(encode_ibignum(self, buffer, size, &next));
    testrun(next == buffer + 2);
    testrun(buffer[0] == 0xc3);
    testrun(buffer[1] == 0x40);

    self->string = ov_string_dup("test");
    self->nbr_uint = strlen(self->string);
    testrun(encode_ibignum(self, buffer, size, &next));
    testrun(next == buffer + 6);
    testrun(buffer[0] == 0xc3) testrun(buffer[1] == 0x44);
    testrun(buffer[2] == 't');
    testrun(buffer[3] == 'e');
    testrun(buffer[4] == 's');
    testrun(buffer[5] == 't');
    testrun(buffer[6] == 0x00);

    self->string = ov_data_pointer_free(self->string);
    testrun(ov_random_string(&self->string, 0xff, NULL));
    self->nbr_uint = strlen(self->string);

    testrun(encode_ibignum(self, buffer, size, &next));
    testrun(next == buffer + 3 + strlen(self->string));
    testrun(buffer[0] == 0xc3);
    for (size_t i = 0; i < strlen(self->string); i++) {
        testrun(buffer[2 + i] != 0x00);
    }
    testrun(next[0] == 0x00);

    self = cbor_free(self);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_encode_fraction() {

    uint8_t buffer[0xffff] = {0};
    uint8_t *next = 0;
    size_t size = 0xffff;

    ov_cbor *self = ov_cbor_create(ov_CBOR_DEC_FRACTION);
    self->nbr_uint = 0;

    self->data = NULL;
    testrun(encode_fraction(self, buffer, size, &next));
    testrun(next == buffer + 2);
    testrun(buffer[0] == 0xC4);
    testrun(buffer[1] == 0x80);

    self->data = ov_cbor_array();

    testrun(ov_cbor_array_push(self->data, ov_cbor_create(ov_CBOR_TRUE)));

    testrun(encode_fraction(self, buffer, size, &next));
    testrun(next == buffer + 3);
    testrun(buffer[0] == 0xC4) testrun(buffer[1] == 0x81);
    testrun(buffer[2] == 0xf5);
    testrun(buffer[3] == 0x00);

    ov_cbor *string = ov_cbor_string("test");
    testrun(ov_cbor_array_push(self->data, string));

    memset(buffer, 0, size);
    testrun(encode_fraction(self, buffer, size, &next));
    testrun(next == buffer + 8);
    testrun(buffer[0] == 0xC4) testrun(buffer[1] == 0x86);
    testrun(buffer[2] == 0xf5);
    testrun(buffer[3] == 0x44);
    testrun(buffer[4] == 't');
    testrun(buffer[5] == 'e');
    testrun(buffer[6] == 's');
    testrun(buffer[7] == 't');
    testrun(buffer[8] == 0x00);

    memset(buffer, 0, size);
    testrun(!encode_fraction(self, buffer, 6, &next));

    string = ov_cbor_create(ov_CBOR_STRING);
    testrun(ov_random_string(&string->string, 0xff, NULL));
    string->nbr_uint = strlen(string->string);
    testrun(ov_cbor_array_push(self->data, string));

    testrun(encode_fraction(self, buffer, size, &next));
    testrun(next == buffer + 4 + 1 + 5 + 2 + string->nbr_uint);
    testrun(buffer[0] == 0xC4);
    testrun(buffer[1] == 0x99);
    testrun(buffer[2] == 0x01);
    testrun(buffer[3] == 0x06);
    testrun(buffer[4] == 0xf5);
    testrun(buffer[5] == 0x44);
    testrun(buffer[6] == 't');
    testrun(buffer[7] == 'e');
    testrun(buffer[8] == 's');
    testrun(buffer[9] == 't');
    testrun(buffer[10] == 0x58);
    testrun(next[0] == 0x00);

    self = cbor_free(self);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_encode_bigfloat() {

    uint8_t buffer[0xffff] = {0};
    uint8_t *next = 0;
    size_t size = 0xffff;

    ov_cbor *self = ov_cbor_create(ov_CBOR_BIGFLOAT);
    self->nbr_uint = 0;

    self->data = NULL;
    testrun(encode_bigfloat(self, buffer, size, &next));
    testrun(next == buffer + 2);
    testrun(buffer[0] == 0xC5);
    testrun(buffer[1] == 0x80);

    self->data = ov_cbor_array();

    testrun(ov_cbor_array_push(self->data, ov_cbor_create(ov_CBOR_TRUE)));

    testrun(encode_bigfloat(self, buffer, size, &next));
    testrun(next == buffer + 3);
    testrun(buffer[0] == 0xC5) testrun(buffer[1] == 0x81);
    testrun(buffer[2] == 0xf5);
    testrun(buffer[3] == 0x00);

    ov_cbor *string = ov_cbor_string("test");
    testrun(ov_cbor_array_push(self->data, string));

    testrun(encode_bigfloat(self, buffer, size, &next));
    testrun(next == buffer + 8);
    testrun(buffer[0] == 0xC5) testrun(buffer[1] == 0x86);
    testrun(buffer[2] == 0xf5);
    testrun(buffer[3] == 0x44);
    testrun(buffer[4] == 't');
    testrun(buffer[5] == 'e');
    testrun(buffer[6] == 's');
    testrun(buffer[7] == 't');
    testrun(buffer[8] == 0x00);

    memset(buffer, 0, size);
    testrun(!encode_bigfloat(self, buffer, 6, &next));

    string = ov_cbor_create(ov_CBOR_STRING);
    testrun(ov_random_string(&string->string, 0xff, NULL));
    string->nbr_uint = strlen(string->string);
    testrun(ov_cbor_array_push(self->data, string));

    testrun(encode_bigfloat(self, buffer, size, &next));
    testrun(next == buffer + 4 + 1 + 5 + 2 + string->nbr_uint);
    testrun(buffer[0] == 0xC5);
    testrun(buffer[1] == 0x99);
    testrun(buffer[2] == 0x01);
    testrun(buffer[3] == 0x06);
    testrun(buffer[4] == 0xf5);
    testrun(buffer[5] == 0x44);
    testrun(buffer[6] == 't');
    testrun(buffer[7] == 'e');
    testrun(buffer[8] == 's');
    testrun(buffer[9] == 't');
    testrun(buffer[10] == 0x58);
    testrun(next[0] == 0x00);

    self = cbor_free(self);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_encode_tag() {

    uint8_t buffer[0xffff] = {0};
    uint8_t *next = 0;
    size_t size = 0xffff;

    ov_cbor *self = ov_cbor_create(ov_CBOR_TAG);
    self->nbr_uint = 0;
    self->tag = 0;

    testrun(!encode_tag(self, buffer, size, &next));

    for (size_t i = 0xc6; i < 0xd4; i++) {

        self->tag = i;
        self->nbr_uint = 1;

        testrun(encode_tag(self, buffer, size, &next));
        testrun(next == buffer + 1);
        testrun(buffer[0] == (self->tag | 0x01));
    }

    ov_cbor *string = ov_cbor_create(ov_CBOR_STRING);
    string->string = ov_string_dup("test");
    string->nbr_uint = strlen(string->string);
    self->data = string;

    self->tag = 0xD5;
    testrun(encode_tag(self, buffer, size, &next));
    testrun(next == buffer + 6);
    testrun(buffer[0] == 0xD5);
    testrun(buffer[1] == 0x44);
    testrun(buffer[2] == 't');
    testrun(buffer[3] == 'e');
    testrun(buffer[4] == 's');
    testrun(buffer[5] == 't');
    testrun(buffer[6] == 0x00);

    self->tag = 0xD6;
    testrun(encode_tag(self, buffer, size, &next));
    testrun(next == buffer + 6);
    testrun(buffer[0] == 0xD6);
    testrun(buffer[1] == 0x44);
    testrun(buffer[2] == 't');
    testrun(buffer[3] == 'e');
    testrun(buffer[4] == 's');
    testrun(buffer[5] == 't');
    testrun(buffer[6] == 0x00);

    self->tag = 0xD7;
    testrun(encode_tag(self, buffer, size, &next));
    testrun(next == buffer + 6);
    testrun(buffer[0] == 0xD7);
    testrun(buffer[1] == 0x44);
    testrun(buffer[2] == 't');
    testrun(buffer[3] == 'e');
    testrun(buffer[4] == 's');
    testrun(buffer[5] == 't');
    testrun(buffer[6] == 0x00);

    string->string = ov_data_pointer_free(string->string);
    testrun(ov_random_string(&string->string, 0xff, NULL));
    string->nbr_uint = strlen(string->string);

    self->tag = 0xD7;
    testrun(encode_tag(self, buffer, size, &next));
    testrun(next == buffer + 3 + string->nbr_uint);
    testrun(buffer[0] == 0xD7);
    testrun(buffer[1] == 0x58);
    testrun(buffer[2] == 0xFE);

    self->tag = 0xd9;
    self->nbr_uint = 0x1f12;
    testrun(encode_tag(self, buffer, size, &next));
    testrun(next == buffer + 5 + string->nbr_uint);
    testrun(buffer[0] == 0xD9);
    testrun(buffer[1] == 0x1F);
    testrun(buffer[2] == 0x12);
    testrun(buffer[3] == 0x58);
    testrun(buffer[4] == 0xFE);

    self->tag = 0xdA;
    self->nbr_uint = 0x1f12;
    testrun(encode_tag(self, buffer, size, &next));

    testrun(next == buffer + 7 + string->nbr_uint);
    testrun(buffer[0] == 0xDA);
    testrun(buffer[1] == 0x00);
    testrun(buffer[2] == 0x00);
    testrun(buffer[3] == 0x1F);
    testrun(buffer[4] == 0x12);
    testrun(buffer[5] == 0x58);
    testrun(buffer[6] == 0xFE);

    self->tag = 0xdB;
    self->nbr_uint = 0x1f12;
    testrun(encode_tag(self, buffer, size, &next));
    testrun(next == buffer + 11 + string->nbr_uint);
    testrun(buffer[0] == 0xDB);
    testrun(buffer[1] == 0x00);
    testrun(buffer[2] == 0x00);
    testrun(buffer[3] == 0x00);
    testrun(buffer[4] == 0x00);
    testrun(buffer[5] == 0x00);
    testrun(buffer[6] == 0x00);
    testrun(buffer[7] == 0x1F);
    testrun(buffer[8] == 0x12);
    testrun(buffer[9] == 0x58);
    testrun(buffer[10] == 0xFE);

    self = cbor_free(self);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_encode_encode_simple() {

    uint8_t buffer[0xffff] = {0};
    uint8_t *next = 0;
    size_t size = 0xffff;

    ov_cbor *self = ov_cbor_create(ov_CBOR_SIMPLE);
    self->nbr_uint = 0;
    self->tag = 0;

    testrun(!encode_simple(self, buffer, size, &next));

    for (size_t i = 0xE0; i < 0xF3; i++) {

        self->tag = i;
        self->nbr_uint = 1;

        testrun(encode_simple(self, buffer, size, &next));
        testrun(next == buffer + 1);
        testrun(buffer[0] == (self->tag | 0x01));
    }

    self->tag = 0xF8;
    self->nbr_uint = 0x1234;
    testrun(encode_simple(self, buffer, size, &next));
    testrun(next == buffer + 2);
    testrun(buffer[0] == 0xF8);
    testrun(buffer[1] == 0x34);

    self->tag = 0xF9;
    testrun(encode_simple(self, buffer, size, &next));
    testrun(next == buffer + 3);
    testrun(buffer[0] == 0xF9);
    testrun(buffer[1] == 0x12);
    testrun(buffer[2] == 0x34);
    testrun(buffer[3] == 0x00);

    self->tag = 0xFA;
    testrun(encode_simple(self, buffer, size, &next));
    testrun(next == buffer + 5);
    testrun(buffer[0] == 0xFA);
    testrun(buffer[1] == 0x00);
    testrun(buffer[2] == 0x00);
    testrun(buffer[3] == 0x12);
    testrun(buffer[4] == 0x34);
    testrun(buffer[5] == 0x00);
    testrun(buffer[6] == 0x00);

    self->tag = 0xFB;
    self->nbr_uint = 0x1f12;
    testrun(encode_simple(self, buffer, size, &next));
    testrun(next == buffer + 9);
    testrun(buffer[0] == 0xFB);
    testrun(buffer[1] == 0x00);
    testrun(buffer[2] == 0x00);
    testrun(buffer[3] == 0x00);
    testrun(buffer[4] == 0x00);
    testrun(buffer[5] == 0x00);
    testrun(buffer[6] == 0x00);
    testrun(buffer[7] == 0x1F);
    testrun(buffer[8] == 0x12);
    testrun(buffer[9] == 0x00);
    testrun(buffer[10] == 0x00);

    self = cbor_free(self);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_encode_encode_float() {

    uint8_t buffer[0xffff] = {0};
    uint8_t value[0xffff] = {0};
    uint8_t *next = 0;
    size_t size = 0xffff;

    ov_cbor *self = ov_cbor_create(ov_CBOR_FLOAT);
    self->nbr_float = 0;
    self->tag = 0;

    uint64_t len = snprintf((char *)value, 100, "%f", self->nbr_float);
    testrun(encode_float(self, buffer, size, &next));
    testrun(buffer[0] == 0xFA);
    for (uint64_t i = 0; i < len; i++) {
        testrun(buffer[i + 1] == value[i]);
    }

    self->nbr_float = 123.4567;
    len = snprintf((char *)value, 100, "%f", self->nbr_float);
    testrun(encode_float(self, buffer, size, &next));
    testrun(buffer[0] == 0xFA);
    for (uint64_t i = 0; i < len; i++) {
        testrun(buffer[i + 1] == value[i]);
    }

    self = cbor_free(self);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_encode_encode_double() {

    uint8_t buffer[0xffff] = {0};
    uint8_t value[0xffff] = {0};
    uint8_t *next = 0;
    size_t size = 0xffff;

    ov_cbor *self = ov_cbor_create(ov_CBOR_DOUBLE);
    self->nbr_float = 0;
    self->tag = 0;

    uint64_t len = snprintf((char *)value, 100, "%g", self->nbr_float);
    testrun(encode_double(self, buffer, size, &next));
    testrun(buffer[0] == 0xfb);
    for (uint64_t i = 0; i < len; i++) {
        testrun(buffer[i + 1] == value[i]);
    }

    self->nbr_float = 123.4567;
    len = snprintf((char *)value, 100, "%g", self->nbr_float);
    testrun(encode_double(self, buffer, size, &next));
    testrun(buffer[0] == 0xfb);
    for (uint64_t i = 0; i < len; i++) {
        testrun(buffer[i + 1] == value[i]);
    }

    self = cbor_free(self);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_cbor_encode() {

    uint8_t buffer[0xffff] = {0};
    uint8_t value[0xffff] = {0};
    uint8_t *next = 0;
    size_t size = 0xffff;

    ov_cbor *self = ov_cbor_create(ov_CBOR_DOUBLE);

    uint64_t len = snprintf((char *)value, 100, "%g", self->nbr_float);
    testrun(ov_cbor_encode(self, buffer, size, &next));
    testrun(buffer[0] == 0xfb);
    for (uint64_t i = 0; i < len; i++) {
        testrun(buffer[i + 1] == value[i]);
    }

    self->nbr_float = 123.4567;
    len = snprintf((char *)value, 100, "%g", self->nbr_float);
    testrun(ov_cbor_encode(self, buffer, size, &next));
    testrun(buffer[0] == 0xfb);
    for (uint64_t i = 0; i < len; i++) {
        testrun(buffer[i + 1] == value[i]);
    }

    self = cbor_free(self);

    self = ov_cbor_create(ov_CBOR_FLOAT);
    self->nbr_float = 123.4567;
    len = snprintf((char *)value, 100, "%f", self->nbr_float);
    testrun(ov_cbor_encode(self, buffer, size, &next));
    testrun(buffer[0] == 0xfa);
    for (uint64_t i = 0; i < len; i++) {
        testrun(buffer[i + 1] == value[i]);
    }

    self = cbor_free(self);

    self = ov_cbor_create(ov_CBOR_SIMPLE);
    self->nbr_uint = 0;
    self->tag = 0xE0;
    testrun(ov_cbor_encode(self, buffer, size, &next));
    testrun(buffer[0] == 0xE0);
    testrun(next == buffer + 1);
    self = cbor_free(self);

    self = ov_cbor_create(ov_CBOR_TAG);
    self->nbr_uint = 0;
    self->tag = 0xc6;
    testrun(ov_cbor_encode(self, buffer, size, &next));
    testrun(buffer[0] == 0xc6);
    testrun(next == buffer + 1);
    self = cbor_free(self);

    self = ov_cbor_create(ov_CBOR_BIGFLOAT);
    self->nbr_uint = 0;
    testrun(ov_cbor_encode(self, buffer, size, &next));
    testrun(buffer[0] == 0xC5);
    testrun(buffer[1] == 0x80);
    testrun(next == buffer + 2);
    self = cbor_free(self);

    self = ov_cbor_create(ov_CBOR_DEC_FRACTION);
    self->nbr_uint = 0;
    testrun(ov_cbor_encode(self, buffer, size, &next));
    testrun(buffer[0] == 0xC4);
    testrun(buffer[1] == 0x80);
    testrun(next == buffer + 2);
    self = cbor_free(self);

    self = ov_cbor_create(ov_CBOR_IBIGNUM);
    self->nbr_uint = 0;
    testrun(ov_cbor_encode(self, buffer, size, &next));
    testrun(buffer[0] == 0xc3);
    testrun(buffer[1] == 0x40);
    testrun(next == buffer + 2);
    self = cbor_free(self);

    self = ov_cbor_create(ov_CBOR_UBIGNUM);
    self->nbr_uint = 0;
    testrun(ov_cbor_encode(self, buffer, size, &next));
    testrun(buffer[0] == 0xc2);
    testrun(buffer[1] == 0x40);
    testrun(next == buffer + 2);
    self = cbor_free(self);

    self = ov_cbor_create(ov_CBOR_DATE_TIME_EPOCH);
    self->nbr_uint = 0;
    testrun(ov_cbor_encode(self, buffer, size, &next));
    testrun(buffer[0] == 0xc1);
    testrun(buffer[1] == 0x00);
    testrun(next == buffer + 2);
    self = cbor_free(self);

    memset(buffer, 0, size);

    self = ov_cbor_create(ov_CBOR_DATE_TIME);
    self->string = ov_string_dup("time");
    self->nbr_uint = strlen(self->string);
    testrun(ov_cbor_encode(self, buffer, size, &next));
    testrun(next == buffer + 6);
    testrun(buffer[0] == 0xC0);
    testrun(buffer[1] == 0x44);
    testrun(buffer[2] == 't');
    testrun(buffer[3] == 'i');
    testrun(buffer[4] == 'm');
    testrun(buffer[5] == 'e');
    testrun(buffer[6] == 0x00);
    self = cbor_free(self);

    memset(buffer, 0, size);

    self = ov_cbor_create(ov_CBOR_MAP);
    testrun(ov_cbor_encode(self, buffer, size, &next));
    testrun(next == buffer + 1);
    testrun(buffer[0] == 0xA0);
    testrun(buffer[1] == 0x00);
    self = cbor_free(self);

    memset(buffer, 0, size);

    self = ov_cbor_create(ov_CBOR_ARRAY);
    testrun(ov_cbor_encode(self, buffer, size, &next));
    testrun(next == buffer + 1);
    testrun(buffer[0] == 0x80);
    testrun(buffer[1] == 0x00);
    self = cbor_free(self);

    memset(buffer, 0, size);

    self = ov_cbor_create(ov_CBOR_UTF8);
    testrun(ov_cbor_encode(self, buffer, size, &next));
    self->string = ov_string_dup("test");
    self->nbr_uint = strlen(self->string);
    testrun(ov_cbor_encode(self, buffer, size, &next));
    testrun(next == buffer + 5);
    testrun(buffer[0] == 0x64);
    testrun(buffer[1] == 't');
    testrun(buffer[2] == 'e');
    testrun(buffer[3] == 's');
    testrun(buffer[4] == 't');
    testrun(buffer[5] == 0x00);
    testrun(buffer[6] == 0x00);
    self = cbor_free(self);

    memset(buffer, 0, size);

    self = ov_cbor_create(ov_CBOR_STRING);
    testrun(ov_cbor_encode(self, buffer, size, &next));
    self->string = ov_string_dup("test");
    self->nbr_uint = strlen(self->string);
    testrun(ov_cbor_encode(self, buffer, size, &next));
    testrun(next == buffer + 5);
    testrun(buffer[0] == 0x44);
    testrun(buffer[1] == 't');
    testrun(buffer[2] == 'e');
    testrun(buffer[3] == 's');
    testrun(buffer[4] == 't');
    testrun(buffer[5] == 0x00);
    testrun(buffer[6] == 0x00);
    self = cbor_free(self);

    memset(buffer, 0, size);

    self = ov_cbor_create(ov_CBOR_INT64);
    testrun(ov_cbor_encode(self, buffer, size, &next));
    testrun(next == buffer + 1);
    testrun(buffer[0] == 0x20);
    testrun(buffer[1] == 0x00);
    self = cbor_free(self);

    memset(buffer, 0, size);

    self = ov_cbor_create(ov_CBOR_UINT64);
    testrun(ov_cbor_encode(self, buffer, size, &next));
    testrun(next == buffer + 1);
    testrun(buffer[0] == 0x00);
    testrun(buffer[1] == 0x00);
    self = cbor_free(self);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_cbor_map() {

    ov_cbor *self = ov_cbor_map();
    testrun(self->type == ov_CBOR_MAP);
    testrun(ov_dict_cast(self->data));
    testrun(ov_cbor_is_map(self));
    self = cbor_free(self);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_cbor_map_set() {

    ov_cbor *self = ov_cbor_map();

    ov_cbor *key = ov_cbor_create(ov_CBOR_UINT64);
    key->nbr_uint = 1234;

    ov_cbor *val = ov_cbor_create(ov_CBOR_UINT64);
    val->nbr_uint = 5678;

    testrun(!ov_cbor_map_set(self, NULL, NULL));
    testrun(!ov_cbor_map_set(self, key, NULL));
    testrun(!ov_cbor_map_set(self, NULL, val));
    testrun(!ov_cbor_map_set(NULL, key, val));

    testrun(0 == ov_dict_count(self->data));
    testrun(ov_cbor_map_set(self, key, val));
    testrun(1 == ov_dict_count(self->data));

    self = cbor_free(self);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_cbor_map_set_string() {

    ov_cbor *self = ov_cbor_map();

    const char *key = "test";

    ov_cbor *val = ov_cbor_create(ov_CBOR_UINT64);
    val->nbr_uint = 5678;

    testrun(!ov_cbor_map_set_string(self, NULL, NULL));
    testrun(!ov_cbor_map_set_string(self, key, NULL));
    testrun(!ov_cbor_map_set_string(self, NULL, val));
    testrun(!ov_cbor_map_set_string(NULL, key, val));

    testrun(0 == ov_dict_count(self->data));
    testrun(ov_cbor_map_set_string(self, key, val));
    testrun(1 == ov_dict_count(self->data));
    testrun(val == ov_cbor_map_get_string(self, "test"));

    self = cbor_free(self);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_cbor_map_get_string() {

    ov_cbor *self = ov_cbor_map();

    ov_cbor *key = ov_cbor_create(ov_CBOR_STRING);
    key->string = ov_string_dup("test");

    ov_cbor *val = ov_cbor_create(ov_CBOR_UINT64);
    val->nbr_uint = 5678;

    testrun(ov_cbor_map_set(self, key, val));
    testrun(val == ov_cbor_map_get_string(self, "test"));

    self = cbor_free(self);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_cbor_map_get() {

    ov_cbor *self = ov_cbor_map();

    const char *key = "test";

    ov_cbor *val = ov_cbor_create(ov_CBOR_UINT64);
    val->nbr_uint = 5678;
    testrun(0 == ov_dict_count(self->data));
    testrun(ov_cbor_map_set_string(self, key, val));

    ov_cbor *k = ov_cbor_create(ov_CBOR_STRING);
    k->string = ov_string_dup("test");

    testrun(val == ov_cbor_map_get(self, k));

    self = cbor_free(self);
    k = cbor_free(k);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_cbor_map_count() {

    ov_cbor *self = ov_cbor_map();

    const char *key = "test";

    ov_cbor *val = ov_cbor_create(ov_CBOR_UINT64);
    val->nbr_uint = 5678;
    testrun(0 == ov_dict_count(self->data));
    testrun(0 == ov_cbor_map_count(self));
    testrun(ov_cbor_map_set_string(self, key, val));
    testrun(1 == ov_cbor_map_count(self));

    val = ov_cbor_create(ov_CBOR_UINT64);
    val->nbr_uint = 5678;
    testrun(ov_cbor_map_set_string(self, "key2", val));
    testrun(2 == ov_cbor_map_count(self));

    testrun(ov_cbor_map_set_string(self, "key3", ov_cbor_true()));
    testrun(3 == ov_cbor_map_count(self));

    self = cbor_free(self);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static bool count_true(const void *key, void *val, void *data) {

    if (!key)
        return true;
    ov_cbor *v = (ov_cbor *)val;
    uint64_t *counter = (uint64_t *)data;
    if (ov_cbor_is_true(v))
        *counter = *counter + 1;
    return true;
}

/*----------------------------------------------------------------------------*/

int test_ov_cbor_map_for_each() {

    ov_cbor *self = ov_cbor_map();

    testrun(ov_cbor_map_set_string(self, "1", ov_cbor_null()));
    testrun(ov_cbor_map_set_string(self, "2", ov_cbor_null()));
    testrun(ov_cbor_map_set_string(self, "3", ov_cbor_true()));
    testrun(ov_cbor_map_set_string(self, "4", ov_cbor_null()));
    testrun(ov_cbor_map_set_string(self, "5", ov_cbor_true()));
    testrun(ov_cbor_map_set_string(self, "6", ov_cbor_true()));
    testrun(ov_cbor_map_set_string(self, "7", ov_cbor_null()));
    testrun(ov_cbor_map_set_string(self, "8", ov_cbor_null()));

    uint64_t counter = 0;
    testrun(ov_cbor_map_for_each(self, &counter, count_true));
    testrun(counter == 3);

    self = cbor_free(self);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_cbor_array() {

    ov_cbor *self = ov_cbor_array();
    testrun(self->type == ov_CBOR_ARRAY);
    testrun(ov_list_cast(self->data));
    testrun(ov_cbor_is_array(self));
    self = cbor_free(self);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_cbor_array_push() {

    ov_cbor *self = ov_cbor_array();

    testrun(ov_cbor_array_push(self, ov_cbor_true()));
    testrun(1 == ov_cbor_array_count(self));
    testrun(ov_cbor_array_push(self, ov_cbor_true()));
    testrun(2 == ov_cbor_array_count(self));
    testrun(ov_cbor_array_push(self, ov_cbor_true()));
    testrun(3 == ov_cbor_array_count(self));

    self = cbor_free(self);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_cbor_array_count() {

    ov_cbor *self = ov_cbor_array();

    testrun(ov_cbor_array_push(self, ov_cbor_true()));
    testrun(1 == ov_cbor_array_count(self));
    testrun(ov_cbor_array_push(self, ov_cbor_true()));
    testrun(2 == ov_cbor_array_count(self));
    testrun(ov_cbor_array_push(self, ov_cbor_true()));
    testrun(3 == ov_cbor_array_count(self));

    self = cbor_free(self);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_cbor_array_get() {

    ov_cbor *self = ov_cbor_array();

    testrun(ov_cbor_array_push(self, ov_cbor_true()));
    testrun(1 == ov_cbor_array_count(self));
    testrun(ov_cbor_array_push(self, ov_cbor_false()));
    testrun(2 == ov_cbor_array_count(self));
    testrun(ov_cbor_array_push(self, ov_cbor_null()));
    testrun(3 == ov_cbor_array_count(self));

    testrun(ov_cbor_is_true(ov_cbor_array_get(self, 0)));
    testrun(ov_cbor_is_false(ov_cbor_array_get(self, 1)));
    testrun(ov_cbor_is_null(ov_cbor_array_get(self, 2)));
    testrun(NULL == ov_cbor_array_get(self, 3));

    self = cbor_free(self);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_cbor_array_pop_queue() {

    ov_cbor *self = ov_cbor_array();

    testrun(ov_cbor_array_push(self, ov_cbor_true()));
    testrun(1 == ov_cbor_array_count(self));
    testrun(ov_cbor_array_push(self, ov_cbor_false()));
    testrun(2 == ov_cbor_array_count(self));
    testrun(ov_cbor_array_push(self, ov_cbor_null()));
    testrun(3 == ov_cbor_array_count(self));

    ov_cbor *out = ov_cbor_array_pop_queue(self);
    testrun(ov_cbor_is_true(out));
    testrun(2 == ov_cbor_array_count(self));
    out = cbor_free(out);

    out = ov_cbor_array_pop_queue(self);
    testrun(ov_cbor_is_false(out));
    testrun(1 == ov_cbor_array_count(self));
    out = cbor_free(out);

    out = ov_cbor_array_pop_queue(self);
    testrun(ov_cbor_is_null(out));
    testrun(0 == ov_cbor_array_count(self));
    out = cbor_free(out);

    self = cbor_free(self);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_cbor_array_pop_stack() {

    ov_cbor *self = ov_cbor_array();

    testrun(ov_cbor_array_push(self, ov_cbor_true()));
    testrun(1 == ov_cbor_array_count(self));
    testrun(ov_cbor_array_push(self, ov_cbor_false()));
    testrun(2 == ov_cbor_array_count(self));
    testrun(ov_cbor_array_push(self, ov_cbor_null()));
    testrun(3 == ov_cbor_array_count(self));

    ov_cbor *out = ov_cbor_array_pop_stack(self);
    testrun(ov_cbor_is_null(out));
    testrun(2 == ov_cbor_array_count(self));
    out = cbor_free(out);

    out = ov_cbor_array_pop_stack(self);
    testrun(ov_cbor_is_false(out));
    testrun(1 == ov_cbor_array_count(self));
    out = cbor_free(out);

    out = ov_cbor_array_pop_stack(self);
    testrun(ov_cbor_is_true(out));
    testrun(0 == ov_cbor_array_count(self));
    out = cbor_free(out);

    self = cbor_free(self);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static bool array_count_true(void *item, void *data) {

    uint64_t *counter = (uint64_t *)data;
    if (ov_cbor_is_true(item))
        *counter = *counter + 1;
    return true;
}

/*----------------------------------------------------------------------------*/

int test_ov_cbor_array_for_each() {

    ov_cbor *self = ov_cbor_array();

    testrun(ov_cbor_array_push(self, ov_cbor_true()));
    testrun(1 == ov_cbor_array_count(self));
    testrun(ov_cbor_array_push(self, ov_cbor_false()));
    testrun(2 == ov_cbor_array_count(self));
    testrun(ov_cbor_array_push(self, ov_cbor_null()));
    testrun(3 == ov_cbor_array_count(self));
    testrun(ov_cbor_array_push(self, ov_cbor_true()));
    testrun(4 == ov_cbor_array_count(self));
    testrun(ov_cbor_array_push(self, ov_cbor_true()));
    testrun(5 == ov_cbor_array_count(self));

    uint64_t counter = 0;
    testrun(ov_cbor_array_for_each(self, &counter, array_count_true));
    testrun(3 == counter);

    self = cbor_free(self);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_cbor_string() {

    ov_cbor *self = ov_cbor_string(NULL);
    testrun(self->type == ov_CBOR_STRING);
    testrun(NULL == self->string);
    testrun(ov_cbor_is_string(self));
    self = cbor_free(self);

    self = ov_cbor_string("test");
    testrun(self->type == ov_CBOR_STRING);
    testrun(NULL != self->string);
    testrun(0 == strcmp("test", ov_cbor_get_string(self)));
    self = cbor_free(self);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_cbor_get_string() {

    ov_cbor *self = ov_cbor_string(NULL);
    testrun(self->type == ov_CBOR_STRING);
    testrun(NULL == self->string);
    self = cbor_free(self);

    self = ov_cbor_string("test");
    testrun(self->type == ov_CBOR_STRING);
    testrun(NULL != self->string);
    testrun(0 == strcmp("test", ov_cbor_get_string(self)));
    self = cbor_free(self);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_cbor_set_string() {

    ov_cbor *self = ov_cbor_string(NULL);
    testrun(self->type == ov_CBOR_STRING);
    testrun(NULL == self->string);
    self = cbor_free(self);

    self = ov_cbor_string("test");
    testrun(self->type == ov_CBOR_STRING);
    testrun(NULL != self->string);
    testrun(0 == strcmp("test", ov_cbor_get_string(self)));
    testrun(ov_cbor_set_string(self, "someother"));
    testrun(0 == strcmp("someother", ov_cbor_get_string(self)));

    self = cbor_free(self);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_cbor_utf8() {

    ov_cbor *self = ov_cbor_utf8(NULL, 0);
    testrun(self->type == ov_CBOR_UTF8);
    testrun(NULL == self->bytes);
    testrun(ov_cbor_is_utf8(self));
    self = cbor_free(self);

    self = ov_cbor_utf8((uint8_t *)"test", 4);
    testrun(self->type == ov_CBOR_UTF8);
    testrun(NULL != self->string);
    testrun(0 == strncmp("test", self->string, self->nbr_uint));
    self = cbor_free(self);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_cbor_get_utf8() {

    ov_cbor *self = ov_cbor_utf8((uint8_t *)"test", 4);
    testrun(self->type == ov_CBOR_UTF8);

    uint8_t *bytes = NULL;
    size_t size = 0;

    testrun(ov_cbor_get_utf8(self, &bytes, &size));
    testrun(size == 4);
    testrun(0 == memcmp(bytes, "test", 4));
    self = cbor_free(self);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_cbor_set_utf8() {

    ov_cbor *self = ov_cbor_utf8((uint8_t *)"test", 4);
    testrun(self->type == ov_CBOR_UTF8);

    uint8_t *bytes = NULL;
    size_t size = 0;

    testrun(ov_cbor_get_utf8(self, &bytes, &size));
    testrun(size == 4);
    testrun(0 == memcmp(bytes, "test", 4));

    testrun(ov_cbor_set_utf8(self, (uint8_t *)"some", 4));

    testrun(ov_cbor_get_utf8(self, &bytes, &size));
    testrun(size == 4);
    testrun(0 == memcmp(bytes, (uint8_t *)"some", 4));

    self = cbor_free(self);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_cbor_true() {

    ov_cbor *self = ov_cbor_true();
    testrun(self->type == ov_CBOR_TRUE);
    self = cbor_free(self);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_cbor_is_true() {

    ov_cbor *self = ov_cbor_true();
    testrun(self->type == ov_CBOR_TRUE);
    testrun(ov_cbor_is_true(self));
    self->type = ov_CBOR_FALSE;
    testrun(!ov_cbor_is_true(self));
    self = cbor_free(self);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_cbor_false() {

    ov_cbor *self = ov_cbor_false();
    testrun(self->type == ov_CBOR_FALSE);
    self = cbor_free(self);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_cbor_is_false() {

    ov_cbor *self = ov_cbor_false();
    testrun(self->type == ov_CBOR_FALSE);
    testrun(ov_cbor_is_false(self));
    self->type = ov_CBOR_TRUE;
    testrun(!ov_cbor_is_false(self));
    self = cbor_free(self);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_cbor_null() {

    ov_cbor *self = ov_cbor_null();
    testrun(self->type == ov_CBOR_NULL);
    self = cbor_free(self);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_cbor_is_null() {

    ov_cbor *self = ov_cbor_null();
    testrun(self->type == ov_CBOR_NULL);
    testrun(ov_cbor_is_null(self));
    self->type = ov_CBOR_TRUE;
    testrun(!ov_cbor_is_null(self));
    self = cbor_free(self);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_cbor_undef() {

    ov_cbor *self = ov_cbor_undef();
    testrun(self->type == ov_CBOR_UNDEF);
    self = cbor_free(self);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_cbor_is_undef() {

    ov_cbor *self = ov_cbor_undef();
    testrun(self->type == ov_CBOR_UNDEF);
    testrun(ov_cbor_is_undef(self));
    self->type = ov_CBOR_TRUE;
    testrun(!ov_cbor_is_undef(self));
    self = cbor_free(self);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_cbor_uint() {

    ov_cbor *self = ov_cbor_uint(0);
    testrun(self->type == ov_CBOR_UINT64);
    testrun(self->nbr_uint == 0);
    testrun(ov_cbor_is_uint(self));
    self = cbor_free(self);

    self = ov_cbor_uint(100);
    testrun(self->type == ov_CBOR_UINT64);
    testrun(self->nbr_uint == 100);
    self = cbor_free(self);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_cbor_get_uint() {

    ov_cbor *self = ov_cbor_uint(0);
    testrun(self->type == ov_CBOR_UINT64);
    testrun(0 == ov_cbor_get_uint(self));
    self = cbor_free(self);

    self = ov_cbor_uint(100);
    testrun(self->type == ov_CBOR_UINT64);
    testrun(100 == ov_cbor_get_uint(self));
    self = cbor_free(self);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_cbor_set_uint() {

    ov_cbor *self = ov_cbor_uint(0);
    testrun(self->type == ov_CBOR_UINT64);
    testrun(0 == ov_cbor_get_uint(self));
    testrun(ov_cbor_set_uint(self, 100));
    testrun(100 == ov_cbor_get_uint(self));
    self = cbor_free(self);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_cbor_int() {

    ov_cbor *self = ov_cbor_int(0);
    testrun(self->type == ov_CBOR_INT64);
    testrun(self->nbr_uint == 0);
    testrun(ov_cbor_is_int(self));
    self = cbor_free(self);

    self = ov_cbor_int(100);
    testrun(!self);
    self = ov_cbor_int(-100);
    testrun(self->type == ov_CBOR_INT64);
    testrun(self->nbr_int == -100);
    self = cbor_free(self);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_cbor_get_int() {

    ov_cbor *self = ov_cbor_int(0);
    testrun(self->type == ov_CBOR_INT64);
    testrun(0 == ov_cbor_get_int(self));
    testrun(ov_cbor_set_int(self, -100));
    testrun(-100 == ov_cbor_get_int(self));
    self = cbor_free(self);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_cbor_set_int() {

    ov_cbor *self = ov_cbor_int(0);
    testrun(self->type == ov_CBOR_INT64);
    testrun(!ov_cbor_set_int(self, 100));
    testrun(ov_cbor_set_int(self, -100));
    testrun(-100 == ov_cbor_get_int(self));
    self = cbor_free(self);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_cbor_time() {

    ov_cbor *self = ov_cbor_time(NULL);
    testrun(self->type == ov_CBOR_DATE_TIME);
    testrun(ov_cbor_is_time(self));
    testrun(NULL == self->string);
    self = cbor_free(self);

    self = ov_cbor_time("test");
    testrun(self->type == ov_CBOR_DATE_TIME);
    testrun(NULL != self->string);
    testrun(0 == strcmp("test", self->string));
    self = cbor_free(self);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_cbor_get_time() {

    ov_cbor *self = ov_cbor_time("test");
    testrun(self->type == ov_CBOR_DATE_TIME);
    testrun(NULL != self->string);
    testrun(0 == strcmp("test", self->string));
    testrun(0 == strcmp("test", ov_cbor_get_time(self)));
    self = cbor_free(self);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_cbor_set_time() {

    ov_cbor *self = ov_cbor_time("test");
    testrun(self->type == ov_CBOR_DATE_TIME);
    testrun(NULL != self->string);
    testrun(0 == strcmp("test", self->string));
    testrun(0 == strcmp("test", ov_cbor_get_time(self)));
    testrun(ov_cbor_set_time(self, "someother"));
    testrun(0 == strcmp("someother", ov_cbor_get_time(self)));

    self = cbor_free(self);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_cbor_time_epoch() {

    ov_cbor *self = ov_cbor_time_epoch(0);
    testrun(self->type == ov_CBOR_DATE_TIME_EPOCH);
    testrun(self->nbr_uint == 0);
    testrun(ov_cbor_is_time_epoch(self));
    self = cbor_free(self);

    self = ov_cbor_time_epoch(100);
    testrun(self->type == ov_CBOR_DATE_TIME_EPOCH);
    testrun(self->nbr_uint == 100);
    self = cbor_free(self);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_cbor_get_time_epoch() {

    ov_cbor *self = ov_cbor_time_epoch(0);
    testrun(self->type == ov_CBOR_DATE_TIME_EPOCH);
    testrun(0 == ov_cbor_get_time_epoch(self));
    self = cbor_free(self);

    self = ov_cbor_time_epoch(100);
    testrun(self->type == ov_CBOR_DATE_TIME_EPOCH);
    testrun(100 == ov_cbor_get_time_epoch(self));
    self = cbor_free(self);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_cbor_set_time_epoch() {

    ov_cbor *self = ov_cbor_time_epoch(0);
    testrun(self->type == ov_CBOR_DATE_TIME_EPOCH);
    testrun(0 == ov_cbor_get_time_epoch(self));
    testrun(ov_cbor_set_time_epoch(self, 100));
    testrun(100 == ov_cbor_get_time_epoch(self));
    self = cbor_free(self);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_cbor_ubignum() {

    ov_cbor *self = ov_cbor_ubignum(NULL);
    testrun(self->type == ov_CBOR_UBIGNUM);
    testrun(NULL == self->string);
    testrun(ov_cbor_is_ubignum(self));
    self = cbor_free(self);

    self = ov_cbor_ubignum("test");
    testrun(self->type == ov_CBOR_UBIGNUM);
    testrun(NULL != self->string);
    testrun(0 == strcmp("test", self->string));
    self = cbor_free(self);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_cbor_get_ubignum() {

    ov_cbor *self = ov_cbor_ubignum("test");
    testrun(self->type == ov_CBOR_UBIGNUM);
    testrun(NULL != self->string);
    testrun(0 == strcmp("test", self->string));
    testrun(0 == strcmp("test", ov_cbor_get_ubignum(self)));
    self = cbor_free(self);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_cbor_set_ubignum() {

    ov_cbor *self = ov_cbor_ubignum("test");
    testrun(self->type == ov_CBOR_UBIGNUM);
    testrun(NULL != self->string);
    testrun(0 == strcmp("test", self->string));
    testrun(0 == strcmp("test", ov_cbor_get_ubignum(self)));
    testrun(ov_cbor_set_ubignum(self, "someother"));
    testrun(0 == strcmp("someother", ov_cbor_get_ubignum(self)));

    self = cbor_free(self);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_cbor_ibignum() {

    ov_cbor *self = ov_cbor_ibignum(NULL);
    testrun(self->type == ov_CBOR_IBIGNUM);
    testrun(NULL == self->string);
    testrun(ov_cbor_is_ibignum(self));
    self = cbor_free(self);

    self = ov_cbor_ibignum("test");
    testrun(self->type == ov_CBOR_IBIGNUM);
    testrun(NULL != self->string);
    testrun(0 == strcmp("test", self->string));
    self = cbor_free(self);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_cbor_get_ibignum() {

    ov_cbor *self = ov_cbor_ibignum("test");
    testrun(self->type == ov_CBOR_IBIGNUM);
    testrun(NULL != self->string);
    testrun(0 == strcmp("test", self->string));
    testrun(0 == strcmp("test", ov_cbor_get_ibignum(self)));
    self = cbor_free(self);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_cbor_set_ibignum() {

    ov_cbor *self = ov_cbor_ibignum("test");
    testrun(self->type == ov_CBOR_IBIGNUM);
    testrun(NULL != self->string);
    testrun(0 == strcmp("test", self->string));
    testrun(0 == strcmp("test", ov_cbor_get_ibignum(self)));
    testrun(ov_cbor_set_ibignum(self, "someother"));
    testrun(0 == strcmp("someother", ov_cbor_get_ibignum(self)));

    self = cbor_free(self);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_cbor_dec_fraction() {

    ov_cbor *self = ov_cbor_dec_fraction(NULL);
    testrun(self->type == ov_CBOR_DEC_FRACTION);
    testrun(NULL == self->data);
    testrun(ov_cbor_is_dec_fraction(self));
    self = cbor_free(self);

    self = ov_cbor_dec_fraction(ov_cbor_array());
    testrun(self->type == ov_CBOR_DEC_FRACTION);
    testrun(NULL != self->data);
    self = cbor_free(self);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_cbor_get_dec_fraction() {

    ov_cbor *self = ov_cbor_dec_fraction(NULL);
    testrun(self->type == ov_CBOR_DEC_FRACTION);
    testrun(NULL == self->data);
    testrun(ov_cbor_is_dec_fraction(self));
    self = cbor_free(self);

    self = ov_cbor_dec_fraction(ov_cbor_array());
    testrun(self->type == ov_CBOR_DEC_FRACTION);
    testrun(NULL != self->data);
    testrun(ov_cbor_is_array(ov_cbor_get_dec_fraction(self)));
    self = cbor_free(self);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_cbor_set_dec_fraction() {

    ov_cbor *self = ov_cbor_dec_fraction(ov_cbor_array());
    testrun(self->type == ov_CBOR_DEC_FRACTION);
    testrun(ov_cbor_is_array(ov_cbor_get_dec_fraction(self)));
    testrun(ov_cbor_set_dec_fraction(self, ov_cbor_array()));
    self = cbor_free(self);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_cbor_bigfloat() {

    ov_cbor *self = ov_cbor_bigfloat(NULL);
    testrun(self->type == ov_CBOR_BIGFLOAT);
    testrun(NULL == self->data);
    testrun(ov_cbor_is_bigfloat(self));
    self = cbor_free(self);

    self = ov_cbor_bigfloat(ov_cbor_array());
    testrun(self->type == ov_CBOR_BIGFLOAT);
    testrun(NULL != self->data);
    self = cbor_free(self);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_cbor_get_bigfloat() {

    ov_cbor *self = ov_cbor_bigfloat(NULL);
    testrun(self->type == ov_CBOR_BIGFLOAT);
    testrun(NULL == self->data);
    testrun(ov_cbor_is_bigfloat(self));
    self = cbor_free(self);

    self = ov_cbor_bigfloat(ov_cbor_array());
    testrun(self->type == ov_CBOR_BIGFLOAT);
    testrun(NULL != self->data);
    testrun(ov_cbor_is_array(ov_cbor_get_bigfloat(self)));
    self = cbor_free(self);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_cbor_set_bigfloat() {

    ov_cbor *self = ov_cbor_bigfloat(ov_cbor_array());
    testrun(self->type == ov_CBOR_BIGFLOAT);
    testrun(ov_cbor_is_array(ov_cbor_get_bigfloat(self)));
    testrun(ov_cbor_set_bigfloat(self, ov_cbor_array()));
    self = cbor_free(self);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_cbor_tag() {

    ov_cbor *self = ov_cbor_tag(0);
    testrun(self->type == ov_CBOR_TAG);
    testrun(NULL == self->data);
    testrun(ov_cbor_is_tag(self));
    self = cbor_free(self);

    self = ov_cbor_tag(123);
    testrun(self->type == ov_CBOR_TAG);
    testrun(123 == self->tag);
    self = cbor_free(self);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_cbor_get_tag() {

    ov_cbor *self = ov_cbor_tag(123);
    testrun(123 == ov_cbor_get_tag(self));
    self = cbor_free(self);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_cbor_get_tag_value() {

    ov_cbor *self = ov_cbor_tag(123);
    testrun(123 == ov_cbor_get_tag(self));
    testrun(0 == ov_cbor_get_tag_value(self));
    self->nbr_uint = 456;
    testrun(456 == ov_cbor_get_tag_value(self));
    self = cbor_free(self);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_cbor_get_tag_data() {

    ov_cbor *self = ov_cbor_tag(123);
    testrun(123 == ov_cbor_get_tag(self));
    testrun(NULL == ov_cbor_get_tag_data(self));
    self->data = ov_cbor_map();
    testrun(ov_cbor_is_map(ov_cbor_get_tag_data(self)));
    self = cbor_free(self);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_cbor_set_tag() {

    ov_cbor *self = ov_cbor_tag(123);
    testrun(123 == ov_cbor_get_tag(self));
    testrun(ov_cbor_set_tag(self, 456));
    testrun(456 == ov_cbor_get_tag(self));
    self = cbor_free(self);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_cbor_set_tag_data() {

    ov_cbor *self = ov_cbor_tag(123);
    testrun(NULL == ov_cbor_get_tag_data(self));
    testrun(ov_cbor_set_tag_data(self, ov_cbor_array()));
    testrun(ov_cbor_is_array(ov_cbor_get_tag_data(self)));
    self = cbor_free(self);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_cbor_set_tag_value() {

    ov_cbor *self = ov_cbor_tag(123);
    testrun(0 == ov_cbor_get_tag_value(self));
    testrun(ov_cbor_set_tag_value(self, 456));
    testrun(456 == ov_cbor_get_tag_value(self));
    self = cbor_free(self);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_cbor_simple() {

    ov_cbor *self = ov_cbor_simple(0);
    testrun(self->type == ov_CBOR_SIMPLE);
    testrun(NULL == self->data);
    testrun(ov_cbor_is_simple(self));
    self = cbor_free(self);

    self = ov_cbor_simple(123);
    testrun(self->type == ov_CBOR_SIMPLE);
    testrun(123 == self->tag);
    self = cbor_free(self);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_cbor_get_simple() {

    ov_cbor *self = ov_cbor_simple(123);
    testrun(123 == ov_cbor_get_simple(self));
    self = cbor_free(self);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_cbor_get_simple_value() {

    ov_cbor *self = ov_cbor_simple(123);
    testrun(123 == ov_cbor_get_simple(self));
    testrun(0 == ov_cbor_get_simple_value(self));
    self->nbr_uint = 456;
    testrun(456 == ov_cbor_get_simple_value(self));
    self = cbor_free(self);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_cbor_set_simple() {

    ov_cbor *self = ov_cbor_simple(123);
    testrun(123 == ov_cbor_get_simple(self));
    testrun(ov_cbor_set_simple(self, 456));
    testrun(456 == ov_cbor_get_simple(self));
    self = cbor_free(self);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_cbor_set_simple_value() {

    ov_cbor *self = ov_cbor_simple(123);
    testrun(0 == ov_cbor_get_simple_value(self));
    testrun(ov_cbor_set_simple_value(self, 456));
    testrun(456 == ov_cbor_get_simple_value(self));
    self = cbor_free(self);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_cbor_float() {

    ov_cbor *self = ov_cbor_float(0);
    testrun(self->type == ov_CBOR_FLOAT);
    testrun(NULL == self->data);
    testrun(ov_cbor_is_float(self));
    self = cbor_free(self);

    self = ov_cbor_float(123);
    testrun(self->type == ov_CBOR_FLOAT);
    testrun(123 == ov_cbor_get_float(self));
    self = cbor_free(self);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_cbor_get_float() {

    ov_cbor *self = ov_cbor_float(123);
    testrun(123 == ov_cbor_get_float(self));
    self = cbor_free(self);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_cbor_set_float() {

    ov_cbor *self = ov_cbor_float(123);
    testrun(123 == ov_cbor_get_float(self));
    testrun(ov_cbor_set_float(self, 1.5));
    testrun(1.5 == ov_cbor_get_float(self));
    self = cbor_free(self);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_cbor_double() {

    ov_cbor *self = ov_cbor_double(0);
    testrun(self->type == ov_CBOR_DOUBLE);
    testrun(NULL == self->data);
    testrun(ov_cbor_is_double(self));
    self = cbor_free(self);

    self = ov_cbor_double(123);
    testrun(self->type == ov_CBOR_DOUBLE);
    testrun(123 == ov_cbor_get_double(self));
    self = cbor_free(self);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_cbor_get_double() {

    ov_cbor *self = ov_cbor_double(0.123);
    testrun(0.123 == ov_cbor_get_double(self));
    self = cbor_free(self);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_cbor_set_double() {

    ov_cbor *self = ov_cbor_double(0.123);
    testrun(0.123 == ov_cbor_get_double(self));
    testrun(ov_cbor_set_double(self, 1.5));
    testrun(1.5 == ov_cbor_get_double(self));
    self = cbor_free(self);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_cbor_encode_array_of_indefinite_length() {

    uint8_t buffer[0xffff] = {0};
    size_t size = 0xffff;
    uint8_t *next = NULL;

    ov_cbor *self = ov_cbor_double(0.123);
    testrun(
        !ov_cbor_encode_array_of_indefinite_length(self, buffer, size, &next));
    self = cbor_free(self);

    self = ov_cbor_array();
    testrun(
        ov_cbor_encode_array_of_indefinite_length(self, buffer, size, &next));
    testrun(next == buffer + 2);
    testrun(buffer[0] == 0x9F);
    testrun(buffer[1] == 0xFF);

    testrun(ov_cbor_encode_array_of_indefinite_length(self, buffer, 2, &next));
    testrun(next == buffer + 2);
    testrun(buffer[0] == 0x9F);
    testrun(buffer[1] == 0xFF);

    testrun(
        !ov_cbor_encode_array_of_indefinite_length(self, buffer, 1, &next));

    testrun(ov_cbor_array_push(self, ov_cbor_true()));
    testrun(
        !ov_cbor_encode_array_of_indefinite_length(self, buffer, 2, &next));
    testrun(ov_cbor_encode_array_of_indefinite_length(self, buffer, 3, &next));
    testrun(next == buffer + 3);
    testrun(buffer[0] == 0x9F);
    testrun(buffer[1] == 0xf5);
    testrun(buffer[2] == 0xFF);

    testrun(ov_cbor_array_push(self, ov_cbor_uint(1)));
    testrun(
        !ov_cbor_encode_array_of_indefinite_length(self, buffer, 2, &next));
    testrun(
        !ov_cbor_encode_array_of_indefinite_length(self, buffer, 3, &next));
    testrun(ov_cbor_encode_array_of_indefinite_length(self, buffer, 4, &next));
    testrun(next == buffer + 4);
    testrun(buffer[0] == 0x9F);
    testrun(buffer[1] == 0xf5);
    testrun(buffer[2] == 0x01);
    testrun(buffer[3] == 0xFF);

    self = cbor_free(self);
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int check_string() {

    uint8_t buffer[0xffff] = {0};
    size_t size = 0xffff;
    uint8_t *next = NULL;

    ov_cbor *self = ov_cbor_string("test");
    testrun(encode_string(self, buffer, size, &next));
    testrun(buffer[0] == 0x44);
    testrun(buffer[1] == 't');
    testrun(buffer[2] == 'e');
    testrun(buffer[3] == 's');
    testrun(buffer[4] == 't');
    testrun(next == buffer + 5);

    ov_cbor *out = NULL;
    testrun(decode_text_string(buffer, size, &out, &next));
    testrun(next == buffer + 5);
    testrun(out);
    testrun(0 == ov_string_compare("test", ov_cbor_get_string(out)));
    out = cbor_free(out);

    //              123456789012345678901234
    char *string = "dtn://test/two/text.data";
    self = ov_cbor_string(string);
    testrun(encode_string(self, buffer, size, &next));
    testrun(buffer[0] == 0x58) testrun(buffer[1] == strlen(string));

    out = NULL;
    testrun(decode_text_string(buffer, size, &out, &next));
    testrun(out);
    testrun(0 == ov_string_compare(string, ov_cbor_get_string(out)));
    out = cbor_free(out);

    self = cbor_free(self);
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
    testrun_test(test_cbor_clear);
    testrun_test(test_cbor_free);
    testrun_test(test_cbor_copy);
    testrun_test(test_cbor_dump);
    testrun_test(test_cbor_hash);
    testrun_test(test_cbor_match);
    testrun_test(test_ov_cbor_dict_config);
    testrun_test(test_ov_cbor_create);
    testrun_test(test_ov_cbor_free);
    testrun_test(test_decode_uint);
    testrun_test(test_decode_int);
    testrun_test(test_decode_text_string);
    testrun_test(test_decode_utf8_string);
    testrun_test(test_decode_array);
    testrun_test(test_decode_map);
    testrun_test(test_decode_tag);
    testrun_test(test_decode_simple);
    testrun_test(test_ov_cbor_decode);
    testrun_test(test_cbor_encoding_size);
    testrun_test(test_encode_uint);
    testrun_test(test_encode_int);
    testrun_test(test_encode_string);
    testrun_test(test_encode_uft8);
    testrun_test(test_encode_array);
    testrun_test(test_encode_map);
    testrun_test(test_encode_date_time);
    testrun_test(test_encode_date_time_epoch);
    testrun_test(test_encode_ubignum);
    testrun_test(test_encode_ibignum);
    testrun_test(test_encode_fraction);
    testrun_test(test_encode_bigfloat);
    testrun_test(test_encode_tag);
    testrun_test(test_encode_encode_simple);
    testrun_test(test_encode_encode_float);
    testrun_test(test_encode_encode_double);
    testrun_test(test_cbor_encode);
    testrun_test(test_ov_cbor_map);
    testrun_test(test_ov_cbor_map_set);
    testrun_test(test_ov_cbor_map_set_string);
    testrun_test(test_ov_cbor_map_get);
    testrun_test(test_ov_cbor_map_get_string);
    testrun_test(test_ov_cbor_map_count);
    testrun_test(test_ov_cbor_map_for_each);
    testrun_test(test_ov_cbor_array);
    testrun_test(test_ov_cbor_array_push);
    testrun_test(test_ov_cbor_array_count);
    testrun_test(test_ov_cbor_array_get);
    testrun_test(test_ov_cbor_array_pop_queue);
    testrun_test(test_ov_cbor_array_pop_stack);
    testrun_test(test_ov_cbor_array_for_each);
    testrun_test(test_ov_cbor_string);
    testrun_test(test_ov_cbor_get_string);
    testrun_test(test_ov_cbor_set_string);
    testrun_test(test_ov_cbor_utf8);
    testrun_test(test_ov_cbor_get_utf8);
    testrun_test(test_ov_cbor_set_utf8);
    testrun_test(test_ov_cbor_true);
    testrun_test(test_ov_cbor_is_true);
    testrun_test(test_ov_cbor_false);
    testrun_test(test_ov_cbor_is_false);
    testrun_test(test_ov_cbor_null);
    testrun_test(test_ov_cbor_is_null);
    testrun_test(test_ov_cbor_undef);
    testrun_test(test_ov_cbor_is_undef);
    testrun_test(test_ov_cbor_uint);
    testrun_test(test_ov_cbor_get_uint);
    testrun_test(test_ov_cbor_set_uint);
    testrun_test(test_ov_cbor_int);
    testrun_test(test_ov_cbor_get_int);
    testrun_test(test_ov_cbor_set_int);
    testrun_test(test_ov_cbor_time);
    testrun_test(test_ov_cbor_get_time);
    testrun_test(test_ov_cbor_set_time);
    testrun_test(test_ov_cbor_time_epoch);
    testrun_test(test_ov_cbor_get_time_epoch);
    testrun_test(test_ov_cbor_set_time_epoch);
    testrun_test(test_ov_cbor_ubignum);
    testrun_test(test_ov_cbor_get_ubignum);
    testrun_test(test_ov_cbor_set_time_epoch);
    testrun_test(test_ov_cbor_ibignum);
    testrun_test(test_ov_cbor_get_ibignum);
    testrun_test(test_ov_cbor_set_ibignum);
    testrun_test(test_ov_cbor_dec_fraction);
    testrun_test(test_ov_cbor_get_dec_fraction);
    testrun_test(test_ov_cbor_set_dec_fraction);
    testrun_test(test_ov_cbor_bigfloat);
    testrun_test(test_ov_cbor_get_bigfloat);
    testrun_test(test_ov_cbor_set_bigfloat);
    testrun_test(test_ov_cbor_tag);
    testrun_test(test_ov_cbor_get_tag);
    testrun_test(test_ov_cbor_get_tag_value);
    testrun_test(test_ov_cbor_get_tag_data);
    testrun_test(test_ov_cbor_set_tag);
    testrun_test(test_ov_cbor_set_tag_data);
    testrun_test(test_ov_cbor_set_tag_value);
    testrun_test(test_ov_cbor_simple);
    testrun_test(test_ov_cbor_get_simple);
    testrun_test(test_ov_cbor_get_simple_value);
    testrun_test(test_ov_cbor_set_simple);
    testrun_test(test_ov_cbor_set_simple_value);
    testrun_test(test_ov_cbor_float);
    testrun_test(test_ov_cbor_get_float);
    testrun_test(test_ov_cbor_set_float);
    testrun_test(test_ov_cbor_double);
    testrun_test(test_ov_cbor_get_double);
    testrun_test(test_ov_cbor_set_double);
    testrun_test(test_ov_cbor_encode_array_of_indefinite_length);
    testrun_test(check_string);

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
