/***
        ------------------------------------------------------------------------

        Copyright (c) 2021 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_password_test.c
        @author         Markus Töpfer

        @date           2021-02-20


        ------------------------------------------------------------------------
*/
#include "ov_password.c"
#include <ov_test/testrun.h>

#include <ov_base/ov_dump.h>
#include <ov_base/ov_time.h>

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

int test_ov_password_hash_scrypt() {

  /* using the example of openssl
   * https://www.openssl.org/docs/man1.1.1/man7/scrypt.html */

  const unsigned char expected[64] = {
      0xfd, 0xba, 0xbe, 0x1c, 0x9d, 0x34, 0x72, 0x00, 0x78, 0x56, 0xe7,
      0x19, 0x0d, 0x01, 0xe9, 0xfe, 0x7c, 0x6a, 0xd7, 0xcb, 0xc8, 0x23,
      0x78, 0x30, 0xe7, 0x73, 0x76, 0x63, 0x4b, 0x37, 0x31, 0x62, 0x2e,
      0xaf, 0x30, 0xd9, 0x2e, 0x22, 0xa3, 0x88, 0x6f, 0xf1, 0x09, 0x27,
      0x9d, 0x98, 0x30, 0xda, 0xc7, 0x27, 0xaf, 0xb9, 0x4a, 0x83, 0xee,
      0x6d, 0x83, 0x60, 0xcb, 0xdf, 0xa2, 0xcc, 0x06, 0x40};

  size_t size = 1024;
  char buffer[size];
  memset(buffer, 0, size);

  uint8_t *out = (uint8_t *)buffer;
  size_t len = 64;

  uint64_t start = ov_time_get_current_time_usecs();
  testrun(ov_password_hash_scrypt(out, &len, "password", "NaCl",
                                  (ov_password_hash_parameter){0}));
  uint64_t end = ov_time_get_current_time_usecs();

  testrun(len == 64);
  testrun(0 == memcmp(out, expected, 64));
  fprintf(stdout, "HASH time %" PRIu64 " usec\n", end - start);
  // ov_dump_binary_as_hex(stdout, out, len);
  fprintf(stdout, "\n");

  // double all defaults
  start = ov_time_get_current_time_usecs();
  memset(buffer, 0, size);
  testrun(ov_password_hash_scrypt(
      out, &len, "password", "NaCl",
      (ov_password_hash_parameter){
          .workfactor = 2048, .blocksize = 16, .parallel = 32}));
  end = ov_time_get_current_time_usecs();
  fprintf(stdout, "HASH time %" PRIu64 " usec\n", end - start);
  // ov_dump_binary_as_hex(stdout, out, len);
  testrun(0 != memcmp(out, expected, 64));
  fprintf(stdout, "\n");

  // double all defaults and len
  len = 128;
  start = ov_time_get_current_time_usecs();
  memset(buffer, 0, size);
  testrun(ov_password_hash_scrypt(
      out, &len, "password", "NaCl",
      (ov_password_hash_parameter){
          .workfactor = 2048, .blocksize = 16, .parallel = 32}));
  end = ov_time_get_current_time_usecs();
  fprintf(stdout, "HASH time %" PRIu64 " usec\n", end - start);
  // ov_dump_binary_as_hex(stdout, out, len);
  testrun(0 != memcmp(out, expected, 64));
  fprintf(stdout, "\n");

  // check with small parameter
  len = 64;
  start = ov_time_get_current_time_usecs();
  memset(buffer, 0, size);
  testrun(ov_password_hash_scrypt(
      out, &len, "password", "NaCl",
      (ov_password_hash_parameter){
          .workfactor = 1024, .blocksize = 8, .parallel = 1}));
  end = ov_time_get_current_time_usecs();
  fprintf(stdout, "HASH time %" PRIu64 " usec\n", end - start);
  // ov_dump_binary_as_hex(stdout, out, len);
  testrun(0 != memcmp(out, expected, 64));
  fprintf(stdout, "\n");

  // check with tiny parameter
  len = 64;
  start = ov_time_get_current_time_usecs();
  memset(buffer, 0, size);
  testrun(ov_password_hash_scrypt(out, &len, "password", "NaCl",
                                  (ov_password_hash_parameter){.workfactor = 2,
                                                               .blocksize = 2,
                                                               .parallel = 1}));
  end = ov_time_get_current_time_usecs();
  fprintf(stdout, "HASH time %" PRIu64 " usec\n", end - start);
  // ov_dump_binary_as_hex(stdout, out, len);
  testrun(0 != memcmp(out, expected, 64));
  fprintf(stdout, "\n");

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_password_hash_pdkdf2() {

  size_t size = 1024;
  char buffer[size];
  memset(buffer, 0, size);

  uint8_t *out = (uint8_t *)buffer;
  size_t len = 64;

  uint64_t start = ov_time_get_current_time_usecs();
  testrun(ov_password_hash_pdkdf2(out, &len, "password", "NaCl",
                                  (ov_password_hash_parameter){0}));
  uint64_t end = ov_time_get_current_time_usecs();

  fprintf(stdout, "HASH time %" PRIu64 " usec\n", end - start);
  // ov_dump_binary_as_hex(stdout, out, len);
  fprintf(stdout, "\n");

  start = ov_time_get_current_time_usecs();
  testrun(
      ov_password_hash_pdkdf2(out, &len, "password", "NaCl",
                              (ov_password_hash_parameter){.workfactor = 1}));
  end = ov_time_get_current_time_usecs();

  fprintf(stdout, "HASH time %" PRIu64 " usec\n", end - start);
  // ov_dump_binary_as_hex(stdout, out, len);
  fprintf(stdout, "\n");

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_password_params_from_json() {

  ov_password_hash_parameter params = {0};

  params = ov_password_params_from_json(NULL);
  testrun(0 == params.workfactor);
  testrun(0 == params.blocksize);
  testrun(0 == params.parallel);

  ov_json_value *object = ov_json_object();
  params = ov_password_params_from_json(object);
  testrun(0 == params.workfactor);
  testrun(0 == params.blocksize);
  testrun(0 == params.parallel);

  ov_json_value *val = ov_json_number(1);
  testrun(ov_json_object_set(object, OV_AUTH_KEY_WORKFACTOR, val));
  val = ov_json_number(2);
  testrun(ov_json_object_set(object, OV_AUTH_KEY_BLOCKSIZE, val));
  val = ov_json_number(3);
  testrun(ov_json_object_set(object, OV_AUTH_KEY_PARALLEL, val));

  params = ov_password_params_from_json(object);
  testrun(1 == params.workfactor);
  testrun(2 == params.blocksize);
  testrun(3 == params.parallel);

  testrun(NULL == ov_json_value_free(object));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_password_params_to_json() {

  ov_password_hash_parameter params = {0};

  ov_json_value *val = ov_password_params_to_json(params);
  testrun(val);
  testrun(ov_json_object_get(val, OV_AUTH_KEY_WORKFACTOR));
  testrun(ov_json_object_get(val, OV_AUTH_KEY_BLOCKSIZE));
  testrun(ov_json_object_get(val, OV_AUTH_KEY_PARALLEL));
  testrun(0 ==
          ov_json_number_get(ov_json_object_get(val, OV_AUTH_KEY_WORKFACTOR)));
  testrun(0 ==
          ov_json_number_get(ov_json_object_get(val, OV_AUTH_KEY_BLOCKSIZE)));
  testrun(0 ==
          ov_json_number_get(ov_json_object_get(val, OV_AUTH_KEY_PARALLEL)));
  val = ov_json_value_free(val);

  params.workfactor = 1;
  params.blocksize = 2;
  params.parallel = 3;
  val = ov_password_params_to_json(params);
  testrun(val);
  testrun(ov_json_object_get(val, OV_AUTH_KEY_WORKFACTOR));
  testrun(ov_json_object_get(val, OV_AUTH_KEY_BLOCKSIZE));
  testrun(ov_json_object_get(val, OV_AUTH_KEY_PARALLEL));
  testrun(1 ==
          ov_json_number_get(ov_json_object_get(val, OV_AUTH_KEY_WORKFACTOR)));
  testrun(2 ==
          ov_json_number_get(ov_json_object_get(val, OV_AUTH_KEY_BLOCKSIZE)));
  testrun(3 ==
          ov_json_number_get(ov_json_object_get(val, OV_AUTH_KEY_PARALLEL)));
  val = ov_json_value_free(val);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_password_is_valid() {

  const char *pass = "password";
  const char *salt = "NaCl";

  /* prepare valid input */

  uint8_t buffer[64] = {0};
  size_t len = 64;
  testrun(ov_password_hash_pdkdf2(buffer, &len, pass, salt,
                                  (ov_password_hash_parameter){0}));

  uint8_t *base64_hash_buffer = NULL;
  size_t base64_hash_buffer_length = 0;
  testrun(ov_base64_encode(buffer, len, &base64_hash_buffer,
                           &base64_hash_buffer_length));

  uint8_t *base64_salt_buffer = NULL;
  size_t base64_salt_buffer_length = 0;
  testrun(ov_base64_encode((uint8_t *)salt, strlen(salt), &base64_salt_buffer,
                           &base64_salt_buffer_length));

  ov_json_value *json = ov_json_object();
  ov_json_value *val = ov_json_string((char *)base64_salt_buffer);
  testrun(ov_json_object_set(json, OV_AUTH_KEY_SALT, val));
  val = ov_json_string((char *)base64_hash_buffer);
  testrun(ov_json_object_set(json, OV_AUTH_KEY_HASH, val));

  testrun(!ov_password_is_valid(NULL, NULL));
  testrun(!ov_password_is_valid(pass, NULL));
  testrun(!ov_password_is_valid(NULL, json));

  char *s = ov_json_value_to_string(json);
  fprintf(stdout, "%s\n", s);
  s = ov_data_pointer_free(s);

  testrun(ov_password_is_valid(pass, json));

  // check missing input
  testrun(ov_json_object_del(json, OV_AUTH_KEY_HASH));
  testrun(!ov_password_is_valid(pass, json));

  val = ov_json_string((char *)base64_hash_buffer);
  testrun(ov_json_object_set(json, OV_AUTH_KEY_HASH, val));
  testrun(ov_json_object_del(json, OV_AUTH_KEY_SALT));
  testrun(!ov_password_is_valid(pass, json));

  val = ov_json_string((char *)base64_salt_buffer);
  testrun(ov_json_object_set(json, OV_AUTH_KEY_SALT, val));
  testrun(ov_password_is_valid(pass, json));

  // the hash was created without any params,
  // which means IMPL_WORKFACTOR_DEFAULT was uses as iteration
  // check with a different iteration size
  /*
      for (size_t i = 1; i < 2048; i++){

          val = ov_json_number( i);
          testrun(ov_json_object_set(json, OV_AUTH_KEY_WORKFACTOR, val));
          if (i == IMPL_WORKFACTOR_DEFAULT){
              testrun(ov_password_is_valid(pass, json));
          } else {
              testrun(!ov_password_is_valid(pass, json));
          }
      }

      val = ov_json_number( IMPL_WORKFACTOR_DEFAULT);
      testrun(ov_json_object_set(json, OV_AUTH_KEY_WORKFACTOR, val));
      testrun(ov_password_is_valid(pass, json));
  */
  // set some parallel to force to use scrypt instead of pdkdf2
  val = ov_json_number(1);
  testrun(ov_json_object_set(json, OV_AUTH_KEY_PARALLEL, val));
  testrun(!ov_password_is_valid(pass, json));
  testrun(ov_json_object_del(json, OV_AUTH_KEY_PARALLEL));
  testrun(ov_password_is_valid(pass, json));

  // set some blocksize to force to use srcypt instead of pkdf2
  val = ov_json_number(1);
  testrun(ov_json_object_set(json, OV_AUTH_KEY_BLOCKSIZE, val));
  testrun(!ov_password_is_valid(pass, json));
  testrun(ov_json_object_del(json, OV_AUTH_KEY_BLOCKSIZE));
  testrun(ov_password_is_valid(pass, json));

  // check different password
  testrun(!ov_password_is_valid("abcd", json));
  // check case sensitive password
  testrun(!ov_password_is_valid("Password", json));

  json = ov_json_value_free(json);

  // check scrypt

  memset(buffer, 0, 64);
  testrun(ov_password_hash_scrypt(buffer, &len, pass, salt,
                                  (ov_password_hash_parameter){0}));

  base64_hash_buffer = ov_data_pointer_free(base64_hash_buffer);
  testrun(ov_base64_encode(buffer, len, &base64_hash_buffer,
                           &base64_hash_buffer_length));

  json = ov_json_object();
  val = ov_json_string((char *)base64_salt_buffer);
  testrun(ov_json_object_set(json, OV_AUTH_KEY_SALT, val));
  val = ov_json_string((char *)base64_hash_buffer);
  testrun(ov_json_object_set(json, OV_AUTH_KEY_HASH, val));

  testrun(!ov_password_is_valid(pass, json));

  // set correct params for scrypt
  val = ov_json_number(IMPL_BLOCKSIZE_DEFAULT);
  testrun(ov_json_object_set(json, OV_AUTH_KEY_BLOCKSIZE, val));
  val = ov_json_number(IMPL_WORKFACTOR_DEFAULT);
  testrun(ov_json_object_set(json, OV_AUTH_KEY_WORKFACTOR, val));
  val = ov_json_number(IMPL_PARALLEL_DEFAULT);
  testrun(ov_json_object_set(json, OV_AUTH_KEY_PARALLEL, val));

  testrun(ov_password_is_valid(pass, json));

  // wrong blocksize
  val = ov_json_number(1);
  testrun(ov_json_object_set(json, OV_AUTH_KEY_BLOCKSIZE, val));
  testrun(!ov_password_is_valid(pass, json));
  val = ov_json_number(IMPL_BLOCKSIZE_DEFAULT);
  testrun(ov_json_object_set(json, OV_AUTH_KEY_BLOCKSIZE, val));
  testrun(ov_password_is_valid(pass, json));

  // wrong parallel
  val = ov_json_number(1);
  testrun(ov_json_object_set(json, OV_AUTH_KEY_PARALLEL, val));
  testrun(!ov_password_is_valid(pass, json));
  val = ov_json_number(IMPL_PARALLEL_DEFAULT);
  testrun(ov_json_object_set(json, OV_AUTH_KEY_PARALLEL, val));
  testrun(ov_password_is_valid(pass, json));

  // wrong workfactor
  val = ov_json_number(1);
  testrun(ov_json_object_set(json, OV_AUTH_KEY_WORKFACTOR, val));
  testrun(!ov_password_is_valid(pass, json));
  val = ov_json_number(IMPL_WORKFACTOR_DEFAULT);
  testrun(ov_json_object_set(json, OV_AUTH_KEY_WORKFACTOR, val));
  testrun(ov_password_is_valid(pass, json));

  // check different password
  testrun(!ov_password_is_valid("abcd", json));
  // check case sensitive password
  testrun(!ov_password_is_valid("Password", json));

  base64_hash_buffer = ov_data_pointer_free(base64_hash_buffer);
  base64_salt_buffer = ov_data_pointer_free(base64_salt_buffer);
  json = ov_json_value_free(json);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_password_hash() {

  const char *pass = "password";
  char *str = NULL;

  testrun(!ov_password_hash(NULL, (ov_password_hash_parameter){0}, 0));

  ov_json_value *out =
      ov_password_hash(pass, (ov_password_hash_parameter){0}, 0);

  testrun(out);
  str = ov_json_value_to_string(out);
  fprintf(stdout, "%s\n", str);
  str = ov_data_pointer_free(str);
  testrun(2 == ov_json_object_count(out));
  testrun(ov_json_object_get(out, OV_AUTH_KEY_SALT));
  testrun(ov_json_object_get(out, OV_AUTH_KEY_HASH));
  testrun(ov_password_is_valid(pass, out));
  out = ov_json_value_free(out);

  // set explicit length
  out = ov_password_hash(pass, (ov_password_hash_parameter){0}, 16);

  testrun(out);
  str = ov_json_value_to_string(out);
  fprintf(stdout, "%s\n", str);
  str = ov_data_pointer_free(str);
  testrun(2 == ov_json_object_count(out));
  testrun(ov_json_object_get(out, OV_AUTH_KEY_SALT));
  testrun(ov_json_object_get(out, OV_AUTH_KEY_HASH));
  testrun(ov_password_is_valid(pass, out));
  out = ov_json_value_free(out);

  // set explicit length and iteration
  out = ov_password_hash(pass, (ov_password_hash_parameter){.workfactor = 2048},
                         0);

  testrun(out);
  str = ov_json_value_to_string(out);
  fprintf(stdout, "%s\n", str);
  str = ov_data_pointer_free(str);
  testrun(3 == ov_json_object_count(out));
  testrun(ov_json_object_get(out, OV_AUTH_KEY_SALT));
  testrun(ov_json_object_get(out, OV_AUTH_KEY_HASH));
  testrun(ov_json_object_get(out, OV_AUTH_KEY_WORKFACTOR));
  testrun(2048 ==
          ov_json_number_get(ov_json_object_get(out, OV_AUTH_KEY_WORKFACTOR)));
  testrun(ov_password_is_valid(pass, out));
  out = ov_json_value_free(out);

  // set explicit blocksize
  out =
      ov_password_hash(pass, (ov_password_hash_parameter){.blocksize = 16}, 0);

  testrun(out);
  str = ov_json_value_to_string(out);
  fprintf(stdout, "%s\n", str);
  str = ov_data_pointer_free(str);
  testrun(3 == ov_json_object_count(out));
  testrun(ov_json_object_get(out, OV_AUTH_KEY_SALT));
  testrun(ov_json_object_get(out, OV_AUTH_KEY_HASH));
  testrun(ov_json_object_get(out, OV_AUTH_KEY_BLOCKSIZE));
  testrun(16 ==
          ov_json_number_get(ov_json_object_get(out, OV_AUTH_KEY_BLOCKSIZE)));
  testrun(ov_password_is_valid(pass, out));
  out = ov_json_value_free(out);

  // set explicit parallel
  out = ov_password_hash(pass, (ov_password_hash_parameter){.parallel = 16}, 0);

  testrun(out);
  str = ov_json_value_to_string(out);
  fprintf(stdout, "%s\n", str);
  str = ov_data_pointer_free(str);
  testrun(3 == ov_json_object_count(out));
  testrun(ov_json_object_get(out, OV_AUTH_KEY_SALT));
  testrun(ov_json_object_get(out, OV_AUTH_KEY_HASH));
  testrun(ov_json_object_get(out, OV_AUTH_KEY_PARALLEL));
  testrun(16 ==
          ov_json_number_get(ov_json_object_get(out, OV_AUTH_KEY_PARALLEL)));
  testrun(ov_password_is_valid(pass, out));
  out = ov_json_value_free(out);

  // set all
  out = ov_password_hash(pass,
                         (ov_password_hash_parameter){
                             .parallel = 2, .blocksize = 4, .workfactor = 1024},
                         128);

  testrun(out);
  str = ov_json_value_to_string(out);
  fprintf(stdout, "%s\n", str);
  str = ov_data_pointer_free(str);
  testrun(5 == ov_json_object_count(out));
  testrun(ov_json_object_get(out, OV_AUTH_KEY_SALT));
  testrun(ov_json_object_get(out, OV_AUTH_KEY_HASH));
  testrun(ov_json_object_get(out, OV_AUTH_KEY_WORKFACTOR));
  testrun(1024 ==
          ov_json_number_get(ov_json_object_get(out, OV_AUTH_KEY_WORKFACTOR)));
  testrun(ov_json_object_get(out, OV_AUTH_KEY_PARALLEL));
  testrun(2 ==
          ov_json_number_get(ov_json_object_get(out, OV_AUTH_KEY_PARALLEL)));
  testrun(ov_json_object_get(out, OV_AUTH_KEY_BLOCKSIZE));
  testrun(4 ==
          ov_json_number_get(ov_json_object_get(out, OV_AUTH_KEY_BLOCKSIZE)));
  testrun(ov_password_is_valid(pass, out));
  out = ov_json_value_free(out);

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

  testrun_test(test_ov_password_hash_scrypt);
  testrun_test(test_ov_password_hash_pdkdf2);

  testrun_test(test_ov_password_params_from_json);
  testrun_test(test_ov_password_params_to_json);

  testrun_test(test_ov_password_is_valid);
  testrun_test(test_ov_password_hash);

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
