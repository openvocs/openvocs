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
        @file           ov_json_literal_test.c
        @author         Markus Toepfer

        @date           2018-12-03

        @ingroup        ov_json_literal

        @brief          Unit tests of


        ------------------------------------------------------------------------
*/

#include "ov_json_literal.c"
#include <ov_test/testrun.h>

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

int test_ov_json_literal() {

  ov_json_value *value = NULL;

  testrun(!ov_json_literal(0));
  testrun(!ov_json_literal(NOT_JSON));
  testrun(!ov_json_literal(OV_JSON_ARRAY));
  testrun(!ov_json_literal(OV_JSON_OBJECT));

  value = ov_json_literal(OV_JSON_TRUE);
  testrun(ov_json_value_cast(value));
  testrun(ov_json_is_literal(value));
  testrun(value->type == OV_JSON_TRUE);
  value = ov_json_literal_free(value);

  value = ov_json_literal(OV_JSON_NULL);
  testrun(ov_json_value_cast(value));
  testrun(ov_json_is_literal(value));
  testrun(value->type == OV_JSON_NULL);
  value = ov_json_literal_free(value);

  value = ov_json_literal(OV_JSON_FALSE);
  testrun(ov_json_value_cast(value));
  testrun(ov_json_is_literal(value));
  testrun(value->type == OV_JSON_FALSE);
  value = ov_json_literal_free(value);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_json_true() {

  ov_json_value *value = ov_json_true();
  testrun(ov_json_value_cast(value));
  testrun(ov_json_is_literal(value));
  testrun(value->type == OV_JSON_TRUE);
  value = ov_json_literal_free(value);
  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_json_false() {

  ov_json_value *value = ov_json_false();
  testrun(ov_json_value_cast(value));
  testrun(ov_json_is_literal(value));
  testrun(value->type == OV_JSON_FALSE);
  value = ov_json_literal_free(value);
  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_json_null() {

  ov_json_value *value = ov_json_null();
  testrun(ov_json_value_cast(value));
  testrun(ov_json_is_literal(value));
  testrun(value->type == OV_JSON_NULL);
  value = ov_json_literal_free(value);
  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_json_is_true() {

  ov_json_value *value = ov_json_true();
  testrun(ov_json_is_true(value));
  value->type = OV_JSON_FALSE;
  testrun(!ov_json_is_true(value));
  value = ov_json_literal_free(value);
  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_json_is_false() {

  ov_json_value *value = ov_json_false();
  testrun(ov_json_is_false(value));
  value->type = OV_JSON_TRUE;
  testrun(!ov_json_is_false(value));
  value = ov_json_literal_free(value);
  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_json_is_null() {

  ov_json_value *value = ov_json_null();
  testrun(ov_json_is_null(value));
  value->type = OV_JSON_FALSE;
  testrun(!ov_json_is_null(value));
  value = ov_json_literal_free(value);
  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_json_is_literal() {

  ov_json_value *value = NULL;

  value = ov_json_literal(OV_JSON_TRUE);
  testrun(ov_json_is_literal(value));
  value->type = OV_JSON_NULL;
  testrun(ov_json_is_literal(value));
  value->type = OV_JSON_FALSE;
  testrun(ov_json_is_literal(value));
  value->type = OV_JSON_STRING;
  testrun(!ov_json_is_literal(value));
  value->type = OV_JSON_NUMBER;
  testrun(!ov_json_is_literal(value));
  value->type = OV_JSON_ARRAY;
  testrun(!ov_json_is_literal(value));
  value->type = OV_JSON_OBJECT;
  testrun(!ov_json_is_literal(value));
  value->type = NOT_JSON;
  testrun(!ov_json_is_literal(value));
  value->type = 0;
  testrun(!ov_json_is_literal(value));

  // reset for free
  value->type = OV_JSON_TRUE;
  testrun(ov_json_is_literal(value));
  value = ov_json_literal_free(value);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_json_literal_set() {

  ov_json_value *value = NULL;

  value = ov_json_literal(OV_JSON_TRUE);
  testrun(ov_json_is_literal(value));
  testrun(value->type == OV_JSON_TRUE);

  testrun(ov_json_literal_set(value, OV_JSON_NULL));
  testrun(ov_json_is_literal(value));
  testrun(value->type == OV_JSON_NULL);

  // wrong

  testrun(!ov_json_literal_set(value, OV_JSON_STRING));
  testrun(ov_json_is_literal(value));
  testrun(value->type == OV_JSON_NULL);

  testrun(!ov_json_literal_set(value, OV_JSON_NUMBER));
  testrun(ov_json_is_literal(value));
  testrun(value->type == OV_JSON_NULL);

  testrun(!ov_json_literal_set(value, OV_JSON_ARRAY));
  testrun(ov_json_is_literal(value));
  testrun(value->type == OV_JSON_NULL);

  testrun(!ov_json_literal_set(value, OV_JSON_OBJECT));
  testrun(ov_json_is_literal(value));
  testrun(value->type == OV_JSON_NULL);

  // correct

  testrun(ov_json_literal_set(value, OV_JSON_NULL));
  testrun(ov_json_is_literal(value));
  testrun(value->type == OV_JSON_NULL);

  testrun(ov_json_literal_set(value, OV_JSON_FALSE));
  testrun(ov_json_is_literal(value));
  testrun(value->type == OV_JSON_FALSE);

  testrun(ov_json_literal_set(value, OV_JSON_TRUE));
  testrun(ov_json_is_literal(value));
  testrun(value->type == OV_JSON_TRUE);

  // set on non literal value

  value->type = OV_JSON_NUMBER;
  testrun(!ov_json_literal_set(value, OV_JSON_TRUE));
  testrun(!ov_json_is_literal(value));
  testrun(value->type == OV_JSON_NUMBER);

  value->type = OV_JSON_OBJECT;
  testrun(!ov_json_literal_set(value, OV_JSON_TRUE));
  testrun(!ov_json_is_literal(value));
  testrun(value->type == OV_JSON_OBJECT);

  // reset for free
  value->type = OV_JSON_TRUE;
  testrun(ov_json_is_literal(value));
  value = ov_json_literal_free(value);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_json_literal_clear() {

  ov_json_value *value = NULL;

  value = ov_json_literal(OV_JSON_TRUE);
  testrun(ov_json_is_literal(value));
  testrun(value->type == OV_JSON_TRUE);

  testrun(ov_json_literal_clear(value));
  testrun(ov_json_is_literal(value));
  testrun(value->type == OV_JSON_NULL);

  // wrong

  value->type = OV_JSON_STRING;
  testrun(!ov_json_is_literal(value));
  testrun(!ov_json_literal_clear(value));
  testrun(value->type == OV_JSON_STRING);

  // all correct

  value->type = OV_JSON_NULL;
  testrun(ov_json_literal_clear(value));
  testrun(ov_json_is_literal(value));
  testrun(value->type == OV_JSON_NULL);

  value->type = OV_JSON_FALSE;
  testrun(ov_json_literal_clear(value));
  testrun(ov_json_is_literal(value));
  testrun(value->type == OV_JSON_NULL);

  value->type = OV_JSON_TRUE;
  testrun(ov_json_literal_clear(value));
  testrun(ov_json_is_literal(value));
  testrun(value->type == OV_JSON_NULL);

  // all wrong

  value->type = NOT_JSON;
  testrun(!ov_json_literal_clear(value));
  testrun(value->type == NOT_JSON);

  value->type = OV_JSON_STRING;
  testrun(!ov_json_literal_clear(value));
  testrun(value->type == OV_JSON_STRING);

  value->type = OV_JSON_NUMBER;
  testrun(!ov_json_literal_clear(value));
  testrun(value->type == OV_JSON_NUMBER);

  value->type = OV_JSON_ARRAY;
  testrun(!ov_json_literal_clear(value));
  testrun(value->type == OV_JSON_ARRAY);

  value->type = OV_JSON_OBJECT;
  testrun(!ov_json_literal_clear(value));
  testrun(value->type == OV_JSON_OBJECT);

  // reset for free
  value->type = OV_JSON_TRUE;
  testrun(ov_json_is_literal(value));
  value = ov_json_literal_free(value);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_json_literal_free() {

  ov_json_value *value = NULL;

  value = ov_json_literal(OV_JSON_TRUE);
  testrun(ov_json_is_literal(value));

  testrun(NULL == ov_json_literal_free(NULL));
  testrun(NULL == ov_json_literal_free(value));

  // all correct

  value = ov_json_literal(OV_JSON_NULL);
  testrun(NULL == ov_json_literal_free(value));

  value = ov_json_literal(OV_JSON_TRUE);
  testrun(NULL == ov_json_literal_free(value));

  value = ov_json_literal(OV_JSON_FALSE);
  testrun(NULL == ov_json_literal_free(value));

  // all wrong
  value = ov_json_literal(OV_JSON_NULL);
  testrun(value);

  value->type = NOT_JSON;
  testrun(value == ov_json_literal_free(value));
  testrun(value->type == NOT_JSON);

  value->type = OV_JSON_STRING;
  testrun(value == ov_json_literal_free(value));
  testrun(value->type == OV_JSON_STRING);

  value->type = OV_JSON_NUMBER;
  testrun(value == ov_json_literal_free(value));
  testrun(value->type == OV_JSON_NUMBER);

  value->type = OV_JSON_ARRAY;
  testrun(value == ov_json_literal_free(value));
  testrun(value->type == OV_JSON_ARRAY);

  value->type = OV_JSON_OBJECT;
  testrun(value == ov_json_literal_free(value));
  testrun(value->type == OV_JSON_OBJECT);

  // reset for free
  value->type = OV_JSON_TRUE;
  testrun(ov_json_is_literal(value));
  testrun(NULL == ov_json_literal_free(value));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_json_literal_copy() {

  ov_json_value *orig = NULL;
  ov_json_value *copy = NULL;

  orig = ov_json_literal(OV_JSON_TRUE);
  testrun(ov_json_is_literal(orig));

  testrun(!ov_json_literal_copy(NULL, NULL));
  testrun(!ov_json_literal_copy(NULL, orig));
  testrun(!ov_json_literal_copy((void **)&copy, NULL));

  testrun(ov_json_literal_copy((void **)&copy, orig));
  testrun(copy);
  testrun(copy->type == orig->type);

  // all correct

  testrun(ov_json_literal_set(orig, OV_JSON_NULL));
  testrun(ov_json_literal_copy((void **)&copy, orig));
  testrun(copy);
  testrun(copy->type == orig->type);

  testrun(ov_json_literal_set(orig, OV_JSON_FALSE));
  testrun(ov_json_literal_copy((void **)&copy, orig));
  testrun(copy);
  testrun(copy->type == orig->type);

  testrun(ov_json_literal_set(orig, OV_JSON_TRUE));
  testrun(ov_json_literal_copy((void **)&copy, orig));
  testrun(copy);
  testrun(copy->type == orig->type);
  testrun(copy->type == OV_JSON_TRUE);

  // orig not a literal

  orig->type = OV_JSON_STRING;
  testrun(!ov_json_literal_copy((void **)&copy, orig));
  testrun(copy);
  testrun(copy->type == OV_JSON_TRUE);

  orig->type = OV_JSON_FALSE;
  testrun(ov_json_literal_copy((void **)&copy, orig));
  testrun(copy);
  testrun(copy->type == OV_JSON_FALSE);

  // dest not a literal
  copy->type = OV_JSON_STRING;
  testrun(!ov_json_literal_copy((void **)&copy, orig));
  testrun(copy);
  testrun(copy->type == OV_JSON_STRING);
  testrun(orig->type == OV_JSON_FALSE);

  // reset for free
  copy->type = OV_JSON_TRUE;
  testrun(NULL == ov_json_literal_free(copy));
  testrun(NULL == ov_json_literal_free(orig));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_json_literal_dump() {

  ov_json_value *orig = NULL;

  orig = ov_json_literal(OV_JSON_TRUE);
  testrun(ov_json_is_literal(orig));

  testrun(!ov_json_literal_dump(NULL, NULL));
  testrun(!ov_json_literal_dump(NULL, orig));
  testrun(!ov_json_literal_dump(stdout, NULL));

  // all correct

  testrun_log("TESTDUMP follows");

  testrun(ov_json_literal_set(orig, OV_JSON_NULL));
  testrun(ov_json_literal_dump(stdout, orig));

  testrun(ov_json_literal_set(orig, OV_JSON_FALSE));
  testrun(ov_json_literal_dump(stdout, orig));

  testrun(ov_json_literal_set(orig, OV_JSON_TRUE));
  testrun(ov_json_literal_dump(stdout, orig));

  testrun_log("\n\tTESTDUMP done");

  // orig not a literal

  orig->type = OV_JSON_STRING;
  testrun(!ov_json_literal_dump(stdout, orig));

  // reset for free
  orig->type = OV_JSON_TRUE;
  testrun(NULL == ov_json_literal_free(orig));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_json_literal_functions() {

  ov_data_function f = ov_json_literal_functions();

  testrun(f.clear == ov_json_literal_clear);
  testrun(f.free == ov_json_literal_free);
  testrun(f.copy == ov_json_literal_copy);
  testrun(f.dump == ov_json_literal_dump);

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
  testrun_test(test_ov_json_literal);
  testrun_test(test_ov_json_true);
  testrun_test(test_ov_json_false);
  testrun_test(test_ov_json_null);
  testrun_test(test_ov_json_is_literal);
  testrun_test(test_ov_json_literal_set);

  testrun_test(test_ov_json_literal_clear);
  testrun_test(test_ov_json_literal_free);
  testrun_test(test_ov_json_literal_copy);
  testrun_test(test_ov_json_literal_dump);

  testrun_test(test_ov_json_literal_functions);

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
