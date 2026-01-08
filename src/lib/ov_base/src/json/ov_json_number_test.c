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
        @file           ov_json_number_test.c
        @author         Markus Toepfer

        @date           2018-12-03

        @ingroup        ov_json_number

        @brief          Unit tests of 


        ------------------------------------------------------------------------
*/
#include "ov_json_number.c"
#include <ov_test/testrun.h>

#include <float.h>

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

int test_ov_json_number() {

  ov_json_value *value = NULL;
  JsonNumber *number = NULL;

  value = ov_json_number(0);
  testrun(ov_json_value_cast(value));
  testrun(ov_json_is_number(value));
  number = AS_JSON_NUMBER(value);
  testrun(number);
  testrun(0 == number->data);
  value = ov_json_number_free(value);

  value = ov_json_number(-1234);
  number = AS_JSON_NUMBER(value);
  testrun(number);
  testrun(-1234 == number->data);
  value = ov_json_number_free(value);

  value = ov_json_number(DBL_MAX);
  number = AS_JSON_NUMBER(value);
  testrun(number);
  testrun(DBL_MAX == number->data);
  value = ov_json_number_free(value);

  value = ov_json_number(DBL_MIN);
  number = AS_JSON_NUMBER(value);
  testrun(number);
  testrun(DBL_MIN == number->data);
  value = ov_json_number_free(value);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_json_is_number() {

  ov_json_value *value = NULL;

  value = ov_json_number(0);
  testrun(ov_json_is_number(value));
  value->type = OV_JSON_NULL;
  testrun(!ov_json_is_number(value));
  value->type = OV_JSON_FALSE;
  testrun(!ov_json_is_number(value));
  value->type = OV_JSON_TRUE;
  testrun(!ov_json_is_number(value));
  value->type = OV_JSON_STRING;
  testrun(!ov_json_is_number(value));
  value->type = OV_JSON_ARRAY;
  testrun(!ov_json_is_number(value));
  value->type = OV_JSON_OBJECT;
  testrun(!ov_json_is_number(value));
  value->type = NOT_JSON;
  testrun(!ov_json_is_number(value));
  value->type = 0;
  testrun(!ov_json_is_number(value));

  // reset for free
  value->type = OV_JSON_NUMBER;
  testrun(ov_json_is_number(value));
  value = ov_json_number_free(value);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_json_number_clear() {

  ov_json_value *value = NULL;
  JsonNumber *number = NULL;

  value = ov_json_number(0);
  number = AS_JSON_NUMBER(value);

  testrun(ov_json_number_clear(value));
  number = AS_JSON_NUMBER(value);
  testrun(0 == number->data);

  // with content
  testrun(ov_json_number_set(value, 123));
  testrun(ov_json_number_clear(value));
  testrun(0 == number->data);

  // wrong type
  value->type = OV_JSON_FALSE;
  testrun(!ov_json_is_number(value));
  testrun(!ov_json_number_clear(value));

  value->type = OV_JSON_NUMBER;
  testrun(ov_json_is_number(value));
  value = ov_json_number_free(value);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_json_number_free() {

  ov_json_value *value = ov_json_number(0);

  testrun(NULL == ov_json_number_free(NULL));
  testrun(NULL == ov_json_number_free(value));

  // with content
  value = ov_json_number(123);
  testrun(NULL == ov_json_number_free(value));

  // wrong type
  value = ov_json_number(0);
  value->type = OV_JSON_TRUE;
  testrun(value == ov_json_number_free(value));

  value->type = OV_JSON_NUMBER;
  testrun(NULL == ov_json_number_free(value));
  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_json_number_copy() {

  ov_json_value *orig = ov_json_number(0);
  ov_json_value *copy = NULL;
  JsonNumber *number = NULL;

  testrun(!ov_json_number_copy(NULL, NULL));
  testrun(!ov_json_number_copy(NULL, orig));
  testrun(!ov_json_number_copy((void **)&copy, NULL));

  testrun(ov_json_number_copy((void **)&copy, orig));
  testrun(copy);
  testrun(copy->type == orig->type);
  number = AS_JSON_NUMBER(copy);
  testrun(0 == number->data);

  // all correct

  testrun(ov_json_number_set(orig, 123));
  testrun(ov_json_number_copy((void **)&copy, orig));
  testrun(copy);
  testrun(copy->type == orig->type);
  number = AS_JSON_NUMBER(copy);
  testrun(123 == number->data);

  testrun(ov_json_number_set(orig, -1e3));
  testrun(ov_json_number_copy((void **)&copy, orig));
  testrun(copy);
  testrun(copy->type == orig->type);
  number = AS_JSON_NUMBER(copy);
  testrun(-1e3 == number->data);

  // orig not a number

  testrun(ov_json_number_set(orig, 123.456));

  orig->type = OV_JSON_FALSE;
  testrun(!ov_json_number_copy((void **)&copy, orig));
  // check copy unchanged
  testrun(copy);
  number = AS_JSON_NUMBER(copy);
  testrun(-1e3 == number->data);

  orig->type = OV_JSON_NUMBER;
  copy->type = OV_JSON_FALSE;
  testrun(!ov_json_number_copy((void **)&copy, orig));

  // both correct again
  copy->type = OV_JSON_NUMBER;
  testrun(ov_json_number_copy((void **)&copy, orig));
  testrun(copy);
  number = AS_JSON_NUMBER(copy);
  testrun(123.456 == number->data);

  testrun(NULL == ov_json_number_free(copy));
  testrun(NULL == ov_json_number_free(orig));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_json_number_dump() {

  ov_json_value *orig = ov_json_number(0);
  testrun(ov_json_is_number(orig));

  testrun(!ov_json_number_dump(NULL, NULL));
  testrun(!ov_json_number_dump(NULL, orig));
  testrun(!ov_json_number_dump(stdout, NULL));

  // all correct

  testrun_log("TESTDUMP follows");
  testrun(ov_json_number_dump(stdout, orig));

  // number with content
  testrun(ov_json_number_set(orig, -123.123));
  testrun(ov_json_number_dump(stdout, orig));

  testrun_log("\n\tTESTDUMP done");

  // orig not a number

  orig->type = OV_JSON_FALSE;
  testrun(!ov_json_number_dump(stdout, orig));

  // reset for free
  orig->type = OV_JSON_NUMBER;
  testrun(NULL == ov_json_number_free(orig));

  return testrun_log_success();
}

int test_ov_json_number_set() {

  ov_json_value *value = NULL;
  JsonNumber *number = NULL;

  value = ov_json_number(0);
  number = AS_JSON_NUMBER(value);
  testrun(number);
  testrun(0 == number->data);

  testrun(!ov_json_number_set(NULL, 0));
  testrun(!ov_json_number_set(NULL, 12345));

  testrun(ov_json_number_set(value, 0));
  testrun(0 == number->data);

  testrun(ov_json_number_set(value, DBL_MIN));
  testrun(DBL_MIN == number->data);

  testrun(ov_json_number_set(value, DBL_MAX));
  testrun(DBL_MAX == number->data);

  testrun(ov_json_number_set(value, 434.1212e-17));
  testrun(434.1212e-17 == number->data);

  testrun(ov_json_number_set(value, -123.4567e8));
  testrun(-123.4567e8 == number->data);

  // not a number
  value->type = OV_JSON_NULL;
  testrun(!ov_json_number_set(value, 1));
  // nothing changed
  testrun(-123.4567e8 == number->data);

  // reset for free
  value->type = OV_JSON_NUMBER;
  testrun(ov_json_is_number(value));
  value = ov_json_number_free(value);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_json_number_get() {

  ov_json_value *value = NULL;
  JsonNumber *number = NULL;

  value = ov_json_number(0);
  number = AS_JSON_NUMBER(value);
  testrun(number);
  testrun(0 == number->data);

  testrun(!ov_json_number_get(NULL));
  testrun(!ov_json_number_get(value));

  testrun(ov_json_number_set(value, 1));
  testrun(1 == ov_json_number_get(value));

  testrun(ov_json_number_set(value, 0));
  testrun(0 == ov_json_number_get(value));

  testrun(ov_json_number_set(value, -1));
  testrun(-1 == ov_json_number_get(value));

  testrun(ov_json_number_set(value, DBL_MAX));
  testrun(DBL_MAX == ov_json_number_get(value));

  testrun(ov_json_number_set(value, DBL_MIN));
  testrun(DBL_MIN == ov_json_number_get(value));

  // type not number
  value->type = OV_JSON_STRING;
  testrun(0 == ov_json_number_get(value));

  // reset for free
  value->type = OV_JSON_NUMBER;
  testrun(ov_json_is_number(value));
  value = ov_json_number_free(value);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_json_number_functions() {

  ov_data_function f = ov_json_number_functions();

  testrun(f.clear == ov_json_number_clear);
  testrun(f.free == ov_json_number_free);
  testrun(f.copy == ov_json_number_copy);
  testrun(f.dump == ov_json_number_dump);

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
  testrun_test(test_ov_json_number);
  testrun_test(test_ov_json_is_number);

  testrun_test(test_ov_json_number_clear);
  testrun_test(test_ov_json_number_free);
  testrun_test(test_ov_json_number_copy);
  testrun_test(test_ov_json_number_dump);

  testrun_test(test_ov_json_number_set);
  testrun_test(test_ov_json_number_get);

  testrun_test(test_ov_json_number_functions);

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
