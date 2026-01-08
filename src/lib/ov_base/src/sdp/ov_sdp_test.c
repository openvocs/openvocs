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
        @file           ov_sdp_test.c
        @author         Markus Töpfer

        @date           2019-12-08

        @ingroup        ov_sdp

        @brief          Unit tests of


        ------------------------------------------------------------------------
*/
#include "ov_sdp.c"
#include <ov_test/testrun.h>

#include "../../include/ov_sdp_attribute.h"

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

/*----------------------------------------------------------------------------*/

int test_ov_sdp_to_json() {

  ov_json_value *value = NULL;

  ov_sdp_session *session = NULL;
  ov_sdp_session *reparsed = NULL;

  ov_sdp_list *node = NULL;
  ov_sdp_connection *connection = NULL;
  ov_sdp_description *description = NULL;
  ov_sdp_time *time = NULL;

  char *output = NULL;
  char *expect = NULL;

  session = ov_sdp_session_create();
  session->time = calloc(1, sizeof(ov_sdp_time));

  /*
   *      Set min required
   */

  session->name = "name";

  session->origin.name = "-";
  session->origin.id = 0;
  session->origin.version = 0;

  session->origin.connection.nettype = "net";
  session->origin.connection.addrtype = "type";
  session->origin.connection.address = "addr";

  time = calloc(1, sizeof(ov_sdp_time));
  time->start = 1234567890;
  time->stop = 1234567890;
  testrun(ov_node_push((void **)&session->time, time));

  /*
   *  Check minimal
   */

  expect = ov_sdp_stringify(session, false);
  testrun(expect);

  // testrun_log("\nexpect ->\n%s\n", expect);

  value = ov_sdp_to_json(session);
  testrun(value);

  output = ov_json_value_to_string(value);
  // testrun_log("\n------\n%s\n", output);
  output = ov_data_pointer_free(output);

  reparsed = ov_sdp_from_json(value);
  testrun(reparsed);

  output = ov_sdp_stringify(session, false);
  testrun(output);
  // testrun_log("\n------\n%s<- output | expect ->\n%s------\n", output,
  // expect);
  output = ov_data_pointer_free(output);
  expect = ov_data_pointer_free(expect);
  value = ov_json_value_free(value);
  reparsed = ov_sdp_session_free(reparsed);

  session->info = "info";
  session->uri = "http://openvocs.org";

  session->email = calloc(1, sizeof(ov_sdp_list));
  session->email->value = "someone@mail";
  node = calloc(1, sizeof(ov_sdp_list));
  node->value = "someoneelse@mail";
  testrun(ov_node_push((void **)&session->email, node));
  node = calloc(1, sizeof(ov_sdp_list));
  node->value = "anotherone@mail";
  testrun(ov_node_push((void **)&session->email, node));

  session->phone = calloc(1, sizeof(ov_sdp_list));
  session->phone->value = "+1234";
  node = calloc(1, sizeof(ov_sdp_list));
  node->value = "+5678(name)";
  testrun(ov_node_push((void **)&session->phone, node));

  session->connection = calloc(1, sizeof(ov_sdp_connection));
  session->connection->nettype = "IN";
  session->connection->addrtype = "IP4";
  session->connection->address = "0.0.0.0";

  session->bandwidth = ov_dict_create(ov_dict_string_key_config(255));
  testrun(ov_dict_set(session->bandwidth, strdup("1"), (void *)1, NULL));
  testrun(ov_dict_set(session->bandwidth, strdup("2"), (void *)2, NULL));
  testrun(ov_dict_set(session->bandwidth, strdup("3"), (void *)3, NULL));

  node = calloc(1, sizeof(ov_sdp_list));
  node->key = "1";
  node->value = "1s 1d 1m";
  testrun(ov_node_push((void **)&session->time->repeat, node));
  node = calloc(1, sizeof(ov_sdp_list));
  node->key = "2";
  node->value = "2s 2d 2m";
  testrun(ov_node_push((void **)&session->time->repeat, node));

  node = calloc(1, sizeof(ov_sdp_list));
  node->key = "0";
  node->value = "-25s";
  testrun(ov_node_push((void **)&session->time->zone, node));

  testrun(ov_sdp_attribute_add(&session->attributes, "1", NULL));
  testrun(ov_sdp_attribute_add(&session->attributes, "2", "1"));
  testrun(ov_sdp_attribute_add(&session->attributes, "2", "2"));
  testrun(ov_sdp_attribute_add(&session->attributes, "2", "3"));
  testrun(ov_sdp_attribute_add(&session->attributes, "3", "3"));
  testrun(ov_sdp_attribute_add(&session->attributes, "4", NULL));

  /*
   *  Check without description
   */

  expect = ov_sdp_stringify(session, false);
  testrun(expect);

  // testrun_log("\nexpect ->\n%s\n", expect);

  value = ov_sdp_to_json(session);
  testrun(value);

  // ov_json_value_dump(stdout, value);

  output = ov_json_value_to_string(value);
  // testrun_log("\n------\n%s\n", output);
  output = ov_data_pointer_free(output);

  reparsed = ov_sdp_from_json(value);
  testrun(reparsed);

  output = ov_sdp_stringify(session, false);
  testrun(output);
  // testrun_log("\n------\n%s<- output | expect ->\n%s------\n", output,
  // expect);
  output = ov_data_pointer_free(output);
  expect = ov_data_pointer_free(expect);
  value = ov_json_value_free(value);
  reparsed = ov_sdp_session_free(reparsed);

  // testrun_log("CHECK with DESCRIPTION");

  /*
   *  Check with description
   */

  session->description = calloc(1, sizeof(ov_sdp_description));
  session->description->media.name = "audio";
  session->description->media.protocol = "RTP/AVP";
  session->description->media.port = 12345;
  node = calloc(1, sizeof(ov_sdp_list));
  node->value = "100";
  testrun(ov_node_push((void **)&session->description->media.formats, node));
  node = calloc(1, sizeof(ov_sdp_list));
  node->value = "101";
  testrun(ov_node_push((void **)&session->description->media.formats, node));
  node = calloc(1, sizeof(ov_sdp_list));
  node->value = "102";
  testrun(ov_node_push((void **)&session->description->media.formats, node));

  expect = ov_sdp_stringify(session, false);
  testrun(expect);

  // testrun_log("\nexpect ->\n%s\n", expect);

  value = ov_sdp_to_json(session);
  testrun(value);
  testrun(0 < ov_json_object_count(value));

  output = ov_json_value_to_string(value);
  // testrun_log("\n------\n%s\n", output);
  output = ov_data_pointer_free(output);

  reparsed = ov_sdp_from_json(value);
  testrun(reparsed);

  output = ov_sdp_stringify(session, false);
  testrun(output);
  // testrun_log("\n------\n%s<- output | expect ->\n%s------\n", output,
  // expect);
  output = ov_data_pointer_free(output);
  expect = ov_data_pointer_free(expect);
  value = ov_json_value_free(value);
  reparsed = ov_sdp_session_free(reparsed);

  session->description->info = "info";
  session->description->key = "prompt";
  session->description->connection = calloc(1, sizeof(ov_sdp_connection));
  session->description->connection->nettype = "IN";
  session->description->connection->addrtype = "IP4";
  session->description->connection->address = "0.0.0.0";
  session->description->bandwidth =
      ov_dict_create(ov_dict_string_key_config(255));
  testrun(ov_dict_set(session->description->bandwidth, strdup("1"), (void *)1,
                      NULL));
  testrun(ov_dict_set(session->description->bandwidth, strdup("2"), (void *)2,
                      NULL));
  testrun(ov_dict_set(session->description->bandwidth, strdup("3"), (void *)3,
                      NULL));
  testrun(ov_sdp_attribute_add(&session->description->attributes, "1", "one"));
  testrun(ov_sdp_attribute_add(&session->description->attributes, "2", "x"));
  testrun(ov_sdp_attribute_add(&session->description->attributes, "2", "y"));

  description = calloc(1, sizeof(ov_sdp_description));
  testrun(ov_node_push((void **)&session->description, description));

  description->media.name = "video";
  description->media.protocol = "RTP/AVP";
  description->media.port = 1;
  node = calloc(1, sizeof(ov_sdp_list));
  node->value = "101";
  testrun(ov_node_push((void **)&description->media.formats, node));

  connection = calloc(1, sizeof(ov_sdp_connection));
  testrun(connection);
  testrun(ov_node_push((void **)&description->connection, connection))
      connection->nettype = "1";
  connection->addrtype = "2";
  connection->address = "3";

  connection = calloc(1, sizeof(ov_sdp_connection));
  testrun(connection);
  testrun(ov_node_push((void **)&description->connection, connection))
      connection->nettype = "4";
  connection->addrtype = "5";
  connection->address = "6";

  expect = ov_sdp_stringify(session, false);
  testrun(expect);

  // testrun_log("\nexpect ->\n%s\n", expect);

  reparsed = NULL;

  value = ov_sdp_to_json(session);
  testrun(value);

  output = ov_json_value_to_string(value);
  // testrun_log("\n------\n%s\n", output);
  output = ov_data_pointer_free(output);

  reparsed = ov_sdp_from_json(value);
  testrun(reparsed);

  output = ov_sdp_stringify(session, false);
  testrun(output);
  // testrun_log("\n------\n%s<- output | expect ->\n%s------\n", output,
  // expect);
  output = ov_data_pointer_free(output);

  expect = ov_data_pointer_free(expect);
  value = ov_json_value_free(value);
  testrun(NULL == ov_sdp_session_free(reparsed));
  testrun(NULL == ov_sdp_session_free(session));
  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_sdp_validate() {

  char *string = "v=0\r\n"
                 "o=u 1 2 n a x\r\n"
                 "s=n\r\n"
                 "t=0 0\r\n";

  size_t len = strlen(string);

  testrun(!ov_sdp_validate(NULL, 0));
  testrun(!ov_sdp_validate(string, 0));
  testrun(!ov_sdp_validate(NULL, len));

  testrun(ov_sdp_validate(string, len));
  testrun(!ov_sdp_validate(string, len - 1));

  // check with attributes
  string = "v=0\r\n"
           "o=u 1 2 n a x\r\n"
           "s=n\r\n"
           "t=0 0\r\n"
           "a=whatever:1 1\r\n"
           "a=key:value\r\n"
           "a=key:some value\r\n"
           "a=fmtp:1 1\r\n"
           "a=fmtp:2 2\r\n"
           "a=fmtp:valid true\r\n"
           "a=something\r\n";

  testrun(ov_sdp_validate(string, strlen(string)));

  // check with attributes in media descriptions
  string = "v=0\r\n"
           "o=u 1 2 n a x\r\n"
           "s=n\r\n"
           "c=1 2 3\r\n"
           "t=0 0\r\n"
           "a=whatever:1 1\r\n"
           "a=key:value\r\n"
           "a=key:some value\r\n"
           "a=fmtp:1 1\r\n"
           "a=fmtp:2 2\r\n"
           "a=fmtp:valid true\r\n"
           "a=something\r\n"

           "m=1 2 3 4 5\r\n"
           "a=what:1 1\r\n"
           "a=k:value\r\n"
           "a=k:some value\r\n"
           "a=fmtp:x 1\r\n"
           "a=fmtp:y 2\r\n"
           "a=fmtp:z 3\r\n"
           "a=something\r\n"

           "m=1 2 3 4 5\r\n"

           "m=1 2 3 4 5\r\n"
           "a=what:1 1\r\n"
           "a=k:value\r\n"
           "a=k:some value\r\n"
           "a=fmtp:1 1\r\n"
           "a=fmtp:2 2\r\n"
           "a=fmtp:3 3\r\n"
           "a=something\r\n";

  testrun(ov_sdp_validate(string, strlen(string)));

  // check all
  string = "v=0\r\n"
           "o=user 1 2 n a x\r\n"
           "s=name\r\n"
           "i=info\r\n"
           "u=s:p\r\n"
           "e=1@1\r\n"
           "e=2@2\r\n"
           "e=3@3\r\n"
           "p=1234\r\n"
           "p=1235\r\n"
           "c=1 2 3\r\n"
           "b=1:2\r\n"
           "b=2:2\r\n"
           "b=3:2\r\n"
           "t=0 0\r\n"
           "r=0 0 0\r\n"
           "r=1h 1d 1m\r\n"
           "z=1 2\r\n"
           "t=0 0\r\n"
           "t=0 0\r\n"
           "k=clear:text\r\n"
           "a=key1:value\r\n"
           "a=key2:value\r\n"
           "a=key3:value\r\n"
           "a=key4\r\n"
           "m=1 2 3 4\r\n"
           "m=1 1 1/2 1\r\n"
           "m=audio 1 RTP/AVP 99\r\n"
           "i=info\r\n"
           "c=1 2 3\r\n"
           "m=audio 1 RTP/AVP 99\r\n"
           "b=2:1\r\n"
           "m=audio 1 RTP/AVP 99\r\n"
           "i=info\r\n"
           "c=1 2 3\r\n"
           "c=2 2 3\r\n"
           "c=3 2 3\r\n"
           "b=1:1\r\n"
           "b=2:1\r\n"
           "b=3:1\r\n"
           "b=4:1\r\n"
           "k=clear:text\r\n"
           "a=key1\r\n"
           "a=key2\r\n";

  len = strlen(string);
  testrun(ov_sdp_validate(string, len));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int check_time() {

  char *string = NULL;
  char *next = NULL;
  size_t len = 0;

  string = "t=0 0\r\n"
           "r=0 0 0\r\n"
           "r=1h 1d 1m\r\n"
           "z=1 2\r\n"
           "t=0 0\r\n"
           "t=0 0\r\n"
           "k=clear:text\r\n";

  len = strlen(string);

  testrun(validate_time(string, len, &next));
  testrun(next[0] == 'k');

  // check all
  string = "t=0 0\r\n"
           "r=1h 1d 1m\r\n"
           "t=0 0\r\n"
           "r=0 0 0\r\n"
           "r=1h 1d 1m\r\n"
           "z=1 2\r\n"
           "t=0 0\r\n"
           "r=0 0 0\r\n"
           "r=1h 1d 1m\r\n"
           "t=0 0\r\n"
           "z=1 2d 3s 4 5h 6 7 8\r\n"
           "t=0 0\r\n"
           "t=0 0\r\n"
           "z=1 2d 3s 4 5h 6 7 8\r\n"
           "k=clear:text\r\n";

  len = strlen(string);
  testrun(validate_time(string, len, &next));
  testrun(next[0] == 'k');

  string = "t=0 1\r\n";
  len = strlen(string);
  testrun(!validate_time(string, len, &next));

  string = "t=0 123456789\r\n";
  len = strlen(string);
  testrun(!validate_time(string, len, &next));

  string = "t=0 1234567890\r\n";
  len = strlen(string);
  testrun(validate_time(string, len, &next));

  string = "t=1 1234567890\r\n";
  len = strlen(string);
  testrun(!validate_time(string, len, &next));

  string = "t=123456789 1234567890\r\n";
  len = strlen(string);
  testrun(!validate_time(string, len, &next));

  string = "t=1234567890 1234567890\r\n";
  len = strlen(string);
  testrun(validate_time(string, len, &next));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int check_attribute() {

  char *string = NULL;
  char *next = NULL;
  size_t len = 0;

  string = "a=test\r\nx";
  len = strlen(string);
  testrun(validate_attributes(string, len, &next));
  testrun(*next == 'x');

  string = "a=key:value\r\nx";
  len = strlen(string);
  testrun(validate_attributes(string, len, &next));
  testrun(*next == 'x');

  string = "a=key:1\r\na=key:2\r\na=x\r\na=z:v\r\nx";
  len = strlen(string);
  testrun(validate_attributes(string, len, &next));
  testrun(*next == 'x');

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int check_parse_number() {

  char *str = "1";
  size_t len = strlen(str);
  uint64_t nbr = 0;

  testrun(!parse_number(NULL, 0, NULL));
  testrun(!parse_number(NULL, len, &nbr));
  testrun(!parse_number(str, 0, &nbr));
  testrun(!parse_number(str, len, NULL));

  testrun(parse_number(str, len, &nbr));
  testrun(nbr == 1);

  str = "1234abc";
  len = strlen(str);
  testrun(parse_number(str, 1, &nbr));
  testrun(nbr == 1);
  testrun(parse_number(str, 2, &nbr));
  testrun(nbr == 12);
  testrun(parse_number(str, 3, &nbr));
  testrun(nbr == 123);
  testrun(parse_number(str, 4, &nbr));
  testrun(nbr == 1234);
  testrun(!parse_number(str, 5, &nbr));
  testrun(nbr == 0);
  testrun(!parse_number(str, len, &nbr));
  testrun(nbr == 0);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int check_media_descriptions() {

  char *string = NULL;
  char *next = NULL;
  size_t len = 0;

  string = "m=1 2 3 4\r\nx";
  len = strlen(string);
  testrun(validate_media(string, len, &next));
  testrun(*next == 'x');

  string = "m=1 2/3 4 5\r\nx";
  len = strlen(string);
  testrun(validate_media(string, len, &next));
  testrun(*next == 'x');

  string = "m=1 2 1/2 1\r\nx";
  len = strlen(string);
  testrun(validate_media(string, len, &next));
  testrun(*next == 'x');

  string = "m=1 2/3 4 5 6 7 8 9\r\nx";
  len = strlen(string);
  testrun(validate_media_descriptions(string, len, &next));
  testrun(*next == 'x');

  string = "m=1 2/3 4 5 6 7 8 9\r\ni=1\r\nx";
  len = strlen(string);
  testrun(validate_media_descriptions(string, len, &next));
  testrun(*next == 'x');

  string = "m=1 2/3 4 5 6 7 8 9\r\nc=a b c\r\nx";
  len = strlen(string);
  testrun(validate_media_descriptions(string, len, &next));
  testrun(*next == 'x');

  string = "m=1 2/3 4 5 6 7 8 9\r\nb=0:0\r\nx";
  len = strlen(string);
  testrun(validate_media_descriptions(string, len, &next));
  testrun(*next == 'x');

  string = "m=1 2/3 4 5 6 7 8 9\r\nb=0:0\r\nb=1:1\r\nx";
  len = strlen(string);
  testrun(validate_media_descriptions(string, len, &next));
  testrun(*next == 'x');

  string = "m=1 2/3 4 5 6 7 8 9\r\nk=prompt\r\nx";
  len = strlen(string);
  testrun(validate_media_descriptions(string, len, &next));
  testrun(*next == 'x');

  string = "m=1 2/3 4 5 6 7 8 9\r\na=test\r\na=x:y\r\nx";
  len = strlen(string);
  testrun(validate_media_descriptions(string, len, &next));
  testrun(*next == 'x');

  string = "m=1 2/3 4 5 6 7 8 9\r\n"
           "i=1\r\n"
           "c=a b c\r\n"
           "b=0:0\r\n"
           "k=clear:text\r\n"
           "a=test\r\na=x:y\r\nx";
  len = strlen(string);
  testrun(validate_media_descriptions(string, len, &next));
  testrun(*next == 'x');

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_sdp_session_create() {

  ov_sdp_session *session = NULL;
  ov_sdp_session *session0 = NULL;
  ov_sdp_session *session1 = NULL;
  ov_sdp_session *session2 = NULL;
  ov_sdp_session *session3 = NULL;
  DefaultSession *sess = NULL;

  session0 = ov_sdp_session_create();
  sess = AS_DEFAULT_SESSION(session0);

  testrun(sess);
  testrun(sess->buffer);

  /*
   *      Check session structure
   */

  session = session0;

  testrun(0 == session->version);
  testrun(NULL == session->name);

  testrun(NULL == session->origin.name);
  testrun(0 == session->origin.id);
  testrun(0 == session->origin.version);
  testrun(NULL == session->origin.connection.nettype);
  testrun(NULL == session->origin.name);

  testrun(NULL == session->time);

  testrun(NULL == session->info);
  testrun(NULL == session->uri);
  testrun(NULL == session->key);

  testrun(NULL == session->connection);
  testrun(NULL == session->email);
  testrun(NULL == session->phone);

  testrun(NULL == session->bandwidth);
  testrun(NULL == session->attributes);
  testrun(NULL == session->description);

  testrun(NULL == ov_sdp_session_free(session));

  /*
   *      Create session with (empty cache)
   */

  session = ov_sdp_session_create();
  sess = AS_DEFAULT_SESSION(session);
  testrun(sess);
  testrun(sess->buffer);
  session0 = session;

  session1 = ov_sdp_session_create();
  sess = AS_DEFAULT_SESSION(session1);
  testrun(sess);
  testrun(sess->buffer);

  session2 = ov_sdp_session_create();
  sess = AS_DEFAULT_SESSION(session2);
  testrun(sess);
  testrun(sess->buffer);

  session3 = ov_sdp_session_create();
  sess = AS_DEFAULT_SESSION(session3);
  testrun(sess);
  testrun(sess->buffer);

  ov_sdp_enable_caching(50);

  /*
   *      Recache some sessions
   *
   *      @NOTE this test is build around LIFO cache
   *
   */

  testrun(NULL == ov_sdp_session_free(session0));
  testrun(NULL == ov_sdp_session_free(session1));
  testrun(NULL == ov_sdp_session_free(session2));
  testrun(NULL == ov_sdp_session_free(session3));

  session = ov_sdp_session_create();
  sess = AS_DEFAULT_SESSION(session);
  testrun(session == session3);
  testrun(sess);
  testrun(sess->buffer);

  session = ov_sdp_session_create();
  testrun(session);
  testrun(session == session2);

  session = ov_sdp_session_create();
  testrun(session);
  testrun(session == session1);

  session = ov_sdp_session_create();
  testrun(session);
  testrun(session == session0);

  session = ov_sdp_session_create();
  testrun(session);

  testrun(NULL == ov_sdp_session_free(session));
  testrun(NULL == ov_sdp_session_free(session0));
  testrun(NULL == ov_sdp_session_free(session1));
  testrun(NULL == ov_sdp_session_free(session2));
  testrun(NULL == ov_sdp_session_free(session3));

  ov_sdp_disable_caching();

  session = ov_sdp_session_create();
  sess = AS_DEFAULT_SESSION(session);
  /*
   *  On mocOS the pointer of session0 is reassigned. TBD debug?
   */
  // testrun(session != session3);
  // testrun(session != session2);
  // testrun(session != session1);
  // testrun(session != session0);

  testrun(NULL == ov_sdp_session_free(session));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_sdp_parse() {

  ov_sdp_session *session = NULL;

  char *string = "v=0\r\n"
                 "o=u 1 2 n a x\r\n"
                 "s=n\r\n"
                 "t=0 0\r\n";

  size_t len = strlen(string);

  testrun(!ov_sdp_parse(NULL, 0));
  testrun(!ov_sdp_parse(string, 0));
  testrun(!ov_sdp_parse(NULL, len));

  session = ov_sdp_parse(string, len);
  testrun(session);

  /*
   *      Check session structure
   */

  testrun(0 == session->version);
  testrun(NULL != session->name);

  testrun(NULL != session->origin.name);
  testrun(1 == session->origin.id);
  testrun(2 == session->origin.version);
  testrun(NULL != session->origin.connection.nettype);
  testrun(NULL != session->origin.connection.addrtype);
  testrun(NULL != session->origin.connection.address);

  testrun(NULL != session->time);
  testrun(0 == session->time->start);
  testrun(0 == session->time->stop);

  testrun(NULL == session->info);
  testrun(NULL == session->uri);
  testrun(NULL == session->key);

  testrun(NULL == session->connection);
  testrun(NULL == session->email);
  testrun(NULL == session->phone);

  testrun(NULL == session->bandwidth);
  testrun(NULL == session->attributes);
  testrun(NULL == session->description);

  /*
   *      value check
   */

  testrun(0 == strncmp(session->name, "n", 1));
  testrun(0 == strncmp(session->origin.name, "u", 1));
  testrun(0 == strncmp(session->origin.connection.nettype, "n", 1));
  testrun(0 == strncmp(session->origin.connection.addrtype, "a", 1));
  testrun(0 == strncmp(session->origin.connection.address, "x", 1));

  // ov_buffer_dump(stdout, sess->buffer);

  session = ov_sdp_session_free(session);

  string = "v=1\r\n"
           "o=user 1234 5678 net type addr\r\n"
           "s=name\r\n"
           "t=0 1234567890\r\n";

  len = strlen(string);

  session = ov_sdp_parse(string, len);
  testrun(session);

  /*
   *      Check session structure
   */

  testrun(1 == session->version);
  testrun(NULL != session->name);

  testrun(NULL != session->origin.name);
  testrun(1234 == session->origin.id);
  testrun(5678 == session->origin.version);
  testrun(NULL != session->origin.connection.nettype);
  testrun(NULL != session->origin.connection.addrtype);
  testrun(NULL != session->origin.connection.address);

  testrun(NULL != session->time);
  testrun(0 == session->time->start);
  testrun(1234567890 == session->time->stop);

  testrun(NULL == session->info);
  testrun(NULL == session->uri);
  testrun(NULL == session->key);

  testrun(NULL == session->connection);
  testrun(NULL == session->email);
  testrun(NULL == session->phone);

  testrun(NULL == session->bandwidth);
  testrun(NULL == session->attributes);
  testrun(NULL == session->description);

  /*
   *      value check
   */

  testrun(0 == strncmp(session->name, "name", 4));
  testrun(0 == strncmp(session->origin.name, "user", 4));
  testrun(0 == strncmp(session->origin.connection.nettype, "net", 3));
  testrun(0 == strncmp(session->origin.connection.addrtype, "type", 4));
  testrun(0 == strncmp(session->origin.connection.address, "addr", 4));

  // ov_buffer_dump(stdout, sess->buffer);

  session = ov_sdp_session_free(session);

  /*
   *      Check wrong input to min items
   */

  string = "v 1\r\n"
           "o=user 1234 5678 net type addr\r\n"
           "s=name\r\n"
           "t=0 1234567890\r\n";

  len = strlen(string);
  testrun(!ov_sdp_parse(string, len));

  string = "v=1\r\n"
           "s=name\r\n"
           "o=user 1234 5678 net type addr\r\n"
           "t=0 1234567890\r\n";

  len = strlen(string);
  testrun(!ov_sdp_parse(string, len));

  string = "v=1\r\n"
           "o=user 5678 net type addr\r\n"
           "s=name\r\n"
           "t=0 1234567890\r\n";

  len = strlen(string);
  testrun(!ov_sdp_parse(string, len));

  string = "v=1\r\n"
           "o=user 1234 5678 net type addr\r\n"
           "s=name\r\n"
           "t=0 3\r\n";

  len = strlen(string);
  testrun(!ov_sdp_parse(string, len));

  string = "v=1\r\n"
           "o=user 1234 5678 net type addr\r\n"
           "t=0 1234567890\r\n";

  len = strlen(string);
  testrun(!ov_sdp_parse(string, len));

  string = "v=0\r\n"
           "o=u 1 2 n a x\r\n"
           "s=n\r\n"
           "i=info\r\n"
           "t=0 0\r\n";

  len = strlen(string);
  session = ov_sdp_parse(string, len);
  testrun(session->info);
  testrun(0 == strncmp(session->info, "info", 4));
  session = ov_sdp_session_free(session);

  string = "v=0\r\n"
           "o=u 1 2 n a x\r\n"
           "s=n\r\n"
           "u=http://info.de\r\n"
           "t=0 0\r\n";

  len = strlen(string);
  session = ov_sdp_parse(string, len);
  testrun(0 == strncmp(session->uri, "http://info.de", 14));
  session = ov_sdp_session_free(session);

  string = "v=0\r\n"
           "o=u 1 2 n a x\r\n"
           "s=n\r\n"
           "e=e@mail\r\n"
           "t=0 0\r\n";

  len = strlen(string);
  session = ov_sdp_parse(string, len);
  testrun(0 == strncmp(session->email->value, "e@mail", 6));
  session = ov_sdp_session_free(session);

  ov_sdp_list *node = NULL;
  string = "v=0\r\n"
           "o=u 1 2 n a x\r\n"
           "s=n\r\n"
           "e=e@mail\r\n"
           "e=a@mail\r\n"
           "t=0 0\r\n";

  len = strlen(string);
  session = ov_sdp_parse(string, len);
  testrun(0 == strncmp(session->email->value, "e@mail", 6));
  node = ov_node_next(session->email);
  testrun(node);
  testrun(0 == strncmp(node->value, "a@mail", 6));
  session = ov_sdp_session_free(session);

  string = "v=0\r\n"
           "o=u 1 2 n a x\r\n"
           "s=n\r\n"
           "p=+123\r\n"
           "t=0 0\r\n";

  len = strlen(string);
  session = ov_sdp_parse(string, len);
  testrun(session);
  testrun(0 == strncmp(session->phone->value, "+123", 4));
  session = ov_sdp_session_free(session);

  node = NULL;
  string = "v=0\r\n"
           "o=u 1 2 n a x\r\n"
           "s=n\r\n"
           "p=+123\r\n"
           "p=+456(safe)\r\n"
           "t=0 0\r\n";

  len = strlen(string);
  session = ov_sdp_parse(string, len);
  testrun(0 == strncmp(session->phone->value, "+123", 4));
  node = ov_node_next(session->phone);
  testrun(0 == strncmp(node->value, "+456(safe)", 10));
  session = ov_sdp_session_free(session);

  node = NULL;
  string = "v=0\r\n"
           "o=u 1 2 n a x\r\n"
           "s=n\r\n"
           "c=net type addr\r\n"
           "t=0 0\r\n";

  len = strlen(string);
  session = ov_sdp_parse(string, len);
  testrun(session->connection);
  testrun(0 == strncmp(session->connection->nettype, "net", 3));
  testrun(0 == strncmp(session->connection->addrtype, "type", 4));
  testrun(0 == strncmp(session->connection->address, "addr", 4));
  session = ov_sdp_session_free(session);

  string = "v=0\r\n"
           "o=u 1 2 n a x\r\n"
           "s=n\r\n"
           "c=net type addr\r\n"
           "c=1 2 3\r\n"
           "t=0 0\r\n";

  len = strlen(string);
  testrun(!ov_sdp_parse(string, len));

  node = NULL;
  string = "v=0\r\n"
           "o=u 1 2 n a x\r\n"
           "s=n\r\n"
           "b=one:1\r\n"
           "t=0 0\r\n";

  len = strlen(string);
  session = ov_sdp_parse(string, len);
  testrun(session);
  testrun(session->bandwidth);
  testrun(1 == (intptr_t)ov_dict_get(session->bandwidth, "one"));
  session = ov_sdp_session_free(session);

  string = "v=0\r\n"
           "o=u 1 2 n a x\r\n"
           "s=n\r\n"
           "b=one:1\r\n"
           "b=two:2\r\n"
           "b=three:3\r\n"
           "t=0 0\r\n";

  len = strlen(string);
  session = ov_sdp_parse(string, len);
  testrun(session->bandwidth);
  testrun(1 == (intptr_t)ov_dict_get(session->bandwidth, "one"));
  testrun(2 == (intptr_t)ov_dict_get(session->bandwidth, "two"));
  testrun(3 == (intptr_t)ov_dict_get(session->bandwidth, "three"));
  session = ov_sdp_session_free(session);

  string = "v=0\r\n"
           "o=u 1 2 n a x\r\n"
           "s=n\r\n"
           "t=0 0\r\n"
           "k=prompt\r\n";

  len = strlen(string);
  session = ov_sdp_parse(string, len);
  testrun(session->key);
  testrun(0 == strncmp(session->key, "prompt", 6));
  session = ov_sdp_session_free(session);

  string = "v=0\r\n"
           "o=u 1 2 n a x\r\n"
           "s=n\r\n"
           "t=0 0\r\n"
           "a=prompt\r\n";

  len = strlen(string);
  session = ov_sdp_parse(string, len);
  testrun(session->attributes);
  testrun(0 == strncmp(session->attributes->key, "prompt", 6));
  testrun(0 == session->attributes->value);
  session = ov_sdp_session_free(session);

  string = "v=0\r\n"
           "o=u 1 2 n a x\r\n"
           "s=n\r\n"
           "t=0 0\r\n"
           "a=prompt\r\n"
           "a=x:y\r\n";

  len = strlen(string);
  session = ov_sdp_parse(string, len);
  testrun(session->attributes);
  testrun(0 == strncmp(session->attributes->key, "prompt", 6));
  node = ov_node_next(session->attributes);
  testrun(0 == strncmp(node->key, "x", 1));
  testrun(0 == strncmp(node->value, "y", 1));
  session = ov_sdp_session_free(session);

  string = "v=0\r\n"
           "o=u 1 2 n a x\r\n"
           "s=n\r\n"
           "t=0 0\r\n"
           "a=prompt\r\n"
           "a=x:y\r\n"
           "a=x:z\r\n";

  len = strlen(string);
  session = ov_sdp_parse(string, len);
  testrun(session->attributes);
  testrun(0 == strncmp(session->attributes->key, "prompt", 6));
  node = ov_node_next(session->attributes);
  testrun(0 == strncmp(node->key, "x", 1));
  testrun(0 == strncmp(node->value, "y", 1));
  node = ov_node_next(node);
  testrun(0 == strncmp(node->key, "x", 1));
  testrun(0 == strncmp(node->value, "z", 1));
  session = ov_sdp_session_free(session);

  string = "v=0\r\n"
           "o=u 1 2 n a x\r\n"
           "s=n\r\n"
           "t=0 0\r\n"
           "m=1 2 3 4\r\n";

  len = strlen(string);
  session = ov_sdp_parse(string, len);
  testrun(session->description);
  testrun(0 == strncmp(session->description->media.name, "1", 1));
  testrun(0 == strncmp(session->description->media.protocol, "3", 1));
  testrun(0 == strncmp(session->description->media.formats->value, "4", 1));
  testrun(1 == ov_node_count(session->description->media.formats));
  testrun(2 == session->description->media.port);
  testrun(0 == session->description->media.port2);
  session = ov_sdp_session_free(session);

  ov_sdp_description *desc = NULL;
  string = "v=0\r\n"
           "o=u 1 2 n a x\r\n"
           "s=n\r\n"
           "t=0 0\r\n"
           "m=audio 2/3 RTP/AVP 4 5 6 7\r\n"
           "m=1 2 3 4\r\n";

  len = strlen(string);
  session = ov_sdp_parse(string, len);
  testrun(session->description);
  testrun(0 == strncmp(session->description->media.name, "audio", 5));
  testrun(0 == strncmp(session->description->media.protocol, "RTP/AVP", 7));
  testrun(0 == strncmp(session->description->media.formats->value, "4", 1));
  testrun(4 == ov_node_count(session->description->media.formats));
  node = ov_node_next(session->description->media.formats);
  testrun(0 == strncmp(node->value, "5", 1));
  node = ov_node_next(node);
  testrun(0 == strncmp(node->value, "6", 1));
  node = ov_node_next(node);
  testrun(0 == strncmp(node->value, "7", 1));
  testrun(2 == session->description->media.port);
  testrun(3 == session->description->media.port2);
  testrun(2 == ov_node_count(session->description));
  desc = ov_node_next(session->description);
  testrun(0 == strncmp(desc->media.name, "1", 1));
  testrun(0 == strncmp(desc->media.protocol, "3", 1));
  testrun(0 == strncmp(desc->media.formats->value, "4", 1));
  testrun(1 == ov_node_count(desc->media.formats));
  testrun(2 == desc->media.port);
  testrun(0 == desc->media.port2);
  session = ov_sdp_session_free(session);

  string = "v=0\r\n"
           "o=u 1 2 n a x\r\n"
           "s=n\r\n"
           "t=0 0\r\n"
           "m=1 2 3 4\r\n"
           "i=info\r\n";

  len = strlen(string);
  session = ov_sdp_parse(string, len);
  testrun(session->description);
  testrun(0 == strncmp(session->description->media.name, "1", 1));
  testrun(0 == strncmp(session->description->media.protocol, "3", 1));
  testrun(0 == strncmp(session->description->media.formats->value, "4", 1));
  testrun(1 == ov_node_count(session->description->media.formats));
  testrun(2 == session->description->media.port);
  testrun(0 == session->description->media.port2);
  testrun(0 == strncmp(session->description->info, "info", 4));
  session = ov_sdp_session_free(session);

  string = "v=0\r\n"
           "o=u 1 2 n a x\r\n"
           "s=n\r\n"
           "t=0 0\r\n"
           "m=1 2 3 4\r\n"
           "i=info\r\n"
           "m=5 6 7 8\r\n";

  len = strlen(string);
  session = ov_sdp_parse(string, len);
  testrun(session->description);
  testrun(0 == strncmp(session->description->media.name, "1", 1));
  testrun(0 == strncmp(session->description->media.protocol, "3", 1));
  testrun(0 == strncmp(session->description->media.formats->value, "4", 1));
  testrun(1 == ov_node_count(session->description->media.formats));
  testrun(2 == session->description->media.port);
  testrun(0 == session->description->media.port2);
  testrun(0 == strncmp(session->description->info, "info", 4));
  desc = ov_node_next(session->description);
  testrun(0 == strncmp(desc->media.name, "5", 1));
  testrun(0 == strncmp(desc->media.protocol, "7", 1));
  testrun(0 == strncmp(desc->media.formats->value, "8", 1));
  testrun(1 == ov_node_count(desc->media.formats));
  testrun(6 == desc->media.port);
  testrun(0 == desc->media.port2);
  session = ov_sdp_session_free(session);

  string = "v=0\r\n"
           "o=u 1 2 n a x\r\n"
           "s=n\r\n"
           "t=0 0\r\n"
           "m=1 2 3 4\r\n"
           "c=net type addr\r\n";

  len = strlen(string);
  session = ov_sdp_parse(string, len);
  testrun(session->description);
  testrun(0 == strncmp(session->description->media.name, "1", 1));
  testrun(0 == strncmp(session->description->media.protocol, "3", 1));
  testrun(0 == strncmp(session->description->media.formats->value, "4", 1));
  testrun(1 == ov_node_count(session->description->media.formats));
  testrun(2 == session->description->media.port);
  testrun(0 == session->description->media.port2);
  testrun(0 == strncmp(session->description->connection->nettype, "net", 3));
  testrun(0 == strncmp(session->description->connection->addrtype, "type", 4));
  testrun(0 == strncmp(session->description->connection->address, "addr", 4));
  session = ov_sdp_session_free(session);

  ov_sdp_connection *connection = NULL;
  string = "v=0\r\n"
           "o=u 1 2 n a x\r\n"
           "s=n\r\n"
           "t=0 0\r\n"
           "m=1 2 3 4\r\n"
           "c=net type addr\r\n"
           "c=1 2 3\r\n"
           "c=4 5 6\r\n"
           "m=5 6 7 8\r\n";

  len = strlen(string);
  session = ov_sdp_parse(string, len);
  testrun(session->description);
  testrun(0 == strncmp(session->description->media.name, "1", 1));
  testrun(0 == strncmp(session->description->media.protocol, "3", 1));
  testrun(0 == strncmp(session->description->media.formats->value, "4", 1));
  testrun(1 == ov_node_count(session->description->media.formats));
  testrun(2 == session->description->media.port);
  testrun(0 == session->description->media.port2);
  testrun(0 == strncmp(session->description->connection->nettype, "net", 3));
  testrun(0 == strncmp(session->description->connection->addrtype, "type", 4));
  testrun(0 == strncmp(session->description->connection->address, "addr", 4));
  testrun(3 == ov_node_count(session->description->connection));
  connection = ov_node_next(session->description->connection);
  testrun(0 == strncmp(connection->nettype, "1", 1));
  testrun(0 == strncmp(connection->addrtype, "2", 1));
  testrun(0 == strncmp(connection->address, "3", 1));
  connection = ov_node_next(connection);
  testrun(0 == strncmp(connection->nettype, "4", 1));
  testrun(0 == strncmp(connection->addrtype, "5", 1));
  testrun(0 == strncmp(connection->address, "6", 1));
  desc = ov_node_next(session->description);
  testrun(0 == strncmp(desc->media.name, "5", 1));
  testrun(0 == strncmp(desc->media.protocol, "7", 1));
  testrun(0 == strncmp(desc->media.formats->value, "8", 1));
  testrun(1 == ov_node_count(desc->media.formats));
  testrun(6 == desc->media.port);
  testrun(0 == desc->media.port2);
  session = ov_sdp_session_free(session);
  session = ov_sdp_session_free(session);

  string = "v=0\r\n"
           "o=u 1 2 n a x\r\n"
           "s=n\r\n"
           "t=0 0\r\n"
           "m=1 2 3 4\r\n"
           "b=one:1\r\n"
           "b=two:2\r\n"
           "b=three:3\r\n";

  len = strlen(string);
  session = ov_sdp_parse(string, len);
  testrun(session->description);
  testrun(0 == strncmp(session->description->media.name, "1", 1));
  testrun(0 == strncmp(session->description->media.protocol, "3", 1));
  testrun(0 == strncmp(session->description->media.formats->value, "4", 1));
  testrun(1 == ov_node_count(session->description->media.formats));
  testrun(2 == session->description->media.port);
  testrun(0 == session->description->media.port2);
  testrun(session->description->bandwidth);
  testrun(1 == (intptr_t)ov_dict_get(session->description->bandwidth, "one"));
  testrun(2 == (intptr_t)ov_dict_get(session->description->bandwidth, "two"));
  testrun(3 == (intptr_t)ov_dict_get(session->description->bandwidth, "three"));
  session = ov_sdp_session_free(session);

  string = "v=0\r\n"
           "o=u 1 2 n a x\r\n"
           "s=n\r\n"
           "t=0 0\r\n"
           "m=1 2 3 4\r\n"
           "b=one:1\r\n"
           "b=two:2\r\n"
           "b=three:3\r\n"
           "m=5 6 7 8\r\n";

  len = strlen(string);
  session = ov_sdp_parse(string, len);
  testrun(session->description);
  testrun(0 == strncmp(session->description->media.name, "1", 1));
  testrun(0 == strncmp(session->description->media.protocol, "3", 1));
  testrun(0 == strncmp(session->description->media.formats->value, "4", 1));
  testrun(1 == ov_node_count(session->description->media.formats));
  testrun(2 == session->description->media.port);
  testrun(0 == session->description->media.port2);
  testrun(session->description->bandwidth);
  testrun(1 == (intptr_t)ov_dict_get(session->description->bandwidth, "one"));
  testrun(2 == (intptr_t)ov_dict_get(session->description->bandwidth, "two"));
  testrun(3 == (intptr_t)ov_dict_get(session->description->bandwidth, "three"));
  desc = ov_node_next(session->description);
  testrun(0 == strncmp(desc->media.name, "5", 1));
  testrun(0 == strncmp(desc->media.protocol, "7", 1));
  testrun(0 == strncmp(desc->media.formats->value, "8", 1));
  testrun(1 == ov_node_count(desc->media.formats));
  testrun(6 == desc->media.port);
  testrun(0 == desc->media.port2);
  session = ov_sdp_session_free(session);

  string = "v=0\r\n"
           "o=u 1 2 n a x\r\n"
           "s=n\r\n"
           "t=0 0\r\n"
           "m=1 2 3 4\r\n"
           "k=prompt\r\n"
           "m=5 6 7 8\r\n";

  len = strlen(string);
  session = ov_sdp_parse(string, len);
  testrun(session->description);
  testrun(0 == strncmp(session->description->media.name, "1", 1));
  testrun(0 == strncmp(session->description->media.protocol, "3", 1));
  testrun(0 == strncmp(session->description->media.formats->value, "4", 1));
  testrun(1 == ov_node_count(session->description->media.formats));
  testrun(2 == session->description->media.port);
  testrun(0 == session->description->media.port2);
  testrun(session->description->key);
  testrun(0 == strncmp(session->description->key, "prompt", 6));
  session = ov_sdp_session_free(session);

  string = "v=0\r\n"
           "o=u 1 2 n a x\r\n"
           "s=n\r\n"
           "t=0 0\r\n"
           "m=1 2 3 4\r\n"
           "k=prompt\r\n"
           "m=5 6 7 8\r\n";

  len = strlen(string);
  session = ov_sdp_parse(string, len);
  testrun(session->description);
  testrun(0 == strncmp(session->description->media.name, "1", 1));
  testrun(0 == strncmp(session->description->media.protocol, "3", 1));
  testrun(0 == strncmp(session->description->media.formats->value, "4", 1));
  testrun(1 == ov_node_count(session->description->media.formats));
  testrun(2 == session->description->media.port);
  testrun(0 == session->description->media.port2);
  testrun(session->description->key);
  testrun(0 == strncmp(session->description->key, "prompt", 6));
  desc = ov_node_next(session->description);
  testrun(0 == strncmp(desc->media.name, "5", 1));
  testrun(0 == strncmp(desc->media.protocol, "7", 1));
  testrun(0 == strncmp(desc->media.formats->value, "8", 1));
  testrun(1 == ov_node_count(desc->media.formats));
  testrun(6 == desc->media.port);
  testrun(0 == desc->media.port2);
  session = ov_sdp_session_free(session);

  string = "v=0\r\n"
           "o=u 1 2 n a x\r\n"
           "s=n\r\n"
           "t=0 0\r\n"
           "m=1 2 3 4\r\n"
           "a=x:y\r\n";

  len = strlen(string);
  session = ov_sdp_parse(string, len);
  testrun(session->description);
  testrun(0 == strncmp(session->description->media.name, "1", 1));
  testrun(0 == strncmp(session->description->media.protocol, "3", 1));
  testrun(0 == strncmp(session->description->media.formats->value, "4", 1));
  testrun(1 == ov_node_count(session->description->media.formats));
  testrun(2 == session->description->media.port);
  testrun(0 == session->description->media.port2);
  testrun(session->description->attributes);
  testrun(0 == strncmp(session->description->attributes->key, "x", 1));
  testrun(0 == strncmp(session->description->attributes->value, "y", 1));
  session = ov_sdp_session_free(session);

  string = "v=0\r\n"
           "o=u 1 2 n a x\r\n"
           "s=n\r\n"
           "t=0 0\r\n"
           "m=1 2 3 4\r\n"
           "a=x:y\r\n"
           "a=z\r\n";

  len = strlen(string);
  session = ov_sdp_parse(string, len);
  testrun(session->description);
  testrun(0 == strncmp(session->description->media.name, "1", 1));
  testrun(0 == strncmp(session->description->media.protocol, "3", 1));
  testrun(0 == strncmp(session->description->media.formats->value, "4", 1));
  testrun(1 == ov_node_count(session->description->media.formats));
  testrun(2 == session->description->media.port);
  testrun(0 == session->description->media.port2);
  testrun(session->description->attributes);
  testrun(0 == strncmp(session->description->attributes->key, "x", 1));
  testrun(0 == strncmp(session->description->attributes->value, "y", 1));
  node = ov_node_next(session->description->attributes);
  testrun(0 == strncmp(node->key, "z", 1));
  testrun(0 == node->value);
  session = ov_sdp_session_free(session);

  string = "v=0\r\n"
           "o=u 1 2 n a x\r\n"
           "s=n\r\n"
           "t=0 0\r\n"
           "m=1 2 3 4\r\n"
           "a=x:y\r\n"
           "a=x\r\n"
           "m=5 6 7 8\r\n";
  ;

  len = strlen(string);
  session = ov_sdp_parse(string, len);
  testrun(session->description);
  testrun(0 == strncmp(session->description->media.name, "1", 1));
  testrun(0 == strncmp(session->description->media.protocol, "3", 1));
  testrun(0 == strncmp(session->description->media.formats->value, "4", 1));
  testrun(1 == ov_node_count(session->description->media.formats));
  testrun(2 == session->description->media.port);
  testrun(0 == session->description->media.port2);
  testrun(session->description->attributes);
  testrun(0 == strncmp(session->description->attributes->key, "x", 1));
  testrun(0 == strncmp(session->description->attributes->value, "y", 1));
  node = ov_node_next(session->description->attributes);
  testrun(0 == strncmp(node->key, "x", 1));
  testrun(0 == node->value);
  desc = ov_node_next(session->description);
  testrun(0 == strncmp(desc->media.name, "5", 1));
  testrun(0 == strncmp(desc->media.protocol, "7", 1));
  testrun(0 == strncmp(desc->media.formats->value, "8", 1));
  testrun(1 == ov_node_count(desc->media.formats));
  testrun(6 == desc->media.port);
  testrun(0 == desc->media.port2);
  session = ov_sdp_session_free(session);

  /*
   *      Firefox example
   */
  string = "v=0\r\n"
           "o=mozilla...THIS_IS_SDPARTA-70.0.1 1391370773220577571 0 IN "
           "IP4 0.0.0.0\r\n"
           "s=-\r\n"
           "t=0 0\r\n"
           "a=fingerprint:sha-256 "
           "0B:E9:A9:4F:8B:38:A2:35:AA:B9:41:0F:64:C7:FE:A6:A7:4F:75:E4:"
           "F2:CC:A4:5B:C9:07:F1:13:36:CC:05:2D\r\n"
           "a=ice-options:trickle\r\n"
           "a=msid-semantic:WMS *\r\n"
           "m=audio 9 UDP/TLS/RTP/SAVP 100\r\n"
           "c=IN IP4 0.0.0.0\r\n"
           "a=sendrecv\r\n"
           "a=fmtp:100 maxplaybackrate=48000;stereo=1;useinbandfec=1\r\n"
           "a=ice-pwd:eb6f342865269bb691cce2c73f27d836\r\n"
           "a=ice-ufrag:eb8dd7cb\r\n"
           "a=mid:0\r\n"
           "a=msid:{267c6307-3c24-4762-85e8-c8e8e8346fcf} "
           "{1cc30fae-3974-4671-a210-5788043d2259}\r\n"
           "a=rtcp-mux\r\n"
           "a=rtpmap:100 opus/48000/2\r\n"
           "a=setup:active\r\n"
           "a=ssrc:3835337359 "
           "cname:{d5014593-8812-4140-9535-1ca252a3dc10}\r\n";

  len = strlen(string);
  session = ov_sdp_parse(string, len);
  testrun(0 == session->version);
  testrun(NULL != session->name);

  testrun(NULL != session->origin.name);
  testrun(1391370773220577571 == session->origin.id);
  testrun(0 == session->origin.version);
  testrun(NULL != session->origin.connection.nettype);
  testrun(NULL != session->origin.connection.addrtype);
  testrun(NULL != session->origin.connection.address);

  testrun(NULL != session->time);
  testrun(0 == session->time->start);
  testrun(0 == session->time->stop);

  testrun(NULL == session->info);
  testrun(NULL == session->uri);
  testrun(NULL == session->key);

  testrun(NULL == session->connection);
  testrun(NULL == session->email);
  testrun(NULL == session->phone);

  testrun(NULL == session->bandwidth);
  testrun(NULL != session->attributes);
  testrun(NULL != session->description);

  /*
   *      value check
   */
  char *str = "-";
  testrun(0 == strncmp(session->name, str, strlen(str)));
  str = "mozilla...THIS_IS_SDPARTA-70.0.1";
  testrun(0 == strncmp(session->origin.name, str, strlen(str)));
  str = "IN";
  testrun(0 == strncmp(session->origin.connection.nettype, str, strlen(str)));
  str = "IP4";
  testrun(0 == strncmp(session->origin.connection.addrtype, str, strlen(str)));
  str = "0.0.0.0";
  testrun(0 == strncmp(session->origin.connection.address, str, strlen(str)));

  testrun(3 == ov_node_count(session->attributes));
  str = "sha-256 "
        "0B:E9:A9:4F:8B:38:A2:35:AA:B9:41:0F:64:C7:FE:A6:A7:4F:75:E4:F2:"
        "CC:A4:5B:C9:07:F1:13:36:CC:05:2D";
  testrun(0 == strncmp(ov_sdp_attribute_get(session->attributes, "fingerprint"),
                       str, strlen(str)));

  str = "trickle";
  testrun(0 == strncmp(ov_sdp_attribute_get(session->attributes, "ice-options"),
                       str, strlen(str)));

  str = "WMS *";
  testrun(0 ==
          strncmp(ov_sdp_attribute_get(session->attributes, "msid-semantic"),
                  str, strlen(str)));

  testrun(1 == ov_node_count(session->description));
  testrun(10 == ov_node_count(session->description->attributes));

  str = "audio";
  testrun(0 == strncmp(session->description->media.name, str, strlen(str)));
  str = "UDP/TLS/RTP/SAVP";
  testrun(0 == strncmp(session->description->media.protocol, str, strlen(str)));
  str = "100";
  testrun(0 == strncmp(session->description->media.formats->value, str,
                       strlen(str)));
  testrun(1 == ov_node_count(session->description->media.formats));
  testrun(9 == session->description->media.port);
  testrun(0 == session->description->media.port2);
  str = "IN";
  testrun(0 ==
          strncmp(session->description->connection->nettype, str, strlen(str)));
  str = "IP4";
  testrun(0 == strncmp(session->description->connection->addrtype, str,
                       strlen(str)));
  str = "0.0.0.0";
  testrun(0 ==
          strncmp(session->description->connection->address, str, strlen(str)));
  testrun(ov_sdp_is_sendrecv(session->description));
  str = "maxplaybackrate=48000;stereo=1;useinbandfec=1";
  testrun(0 ==
          strncmp(ov_sdp_get_fmtp(session->description,
                                  session->description->media.formats->value),
                  str, strlen(str)));
  str = "opus/48000/2";
  testrun(0 ==
          strncmp(ov_sdp_get_rtpmap(session->description,
                                    session->description->media.formats->value),
                  str, strlen(str)));

  str = "eb6f342865269bb691cce2c73f27d836";
  testrun(0 == strncmp(ov_sdp_attribute_get(session->description->attributes,
                                            "ice-pwd"),
                       str, strlen(str)));

  str = "eb8dd7cb";
  testrun(0 == strncmp(ov_sdp_attribute_get(session->description->attributes,
                                            "ice-ufrag"),
                       str, strlen(str)));

  str = "0";
  testrun(0 ==
          strncmp(ov_sdp_attribute_get(session->description->attributes, "mid"),
                  str, strlen(str)));

  str = "{267c6307-3c24-4762-85e8-c8e8e8346fcf} "
        "{1cc30fae-3974-4671-a210-5788043d2259}";
  testrun(0 == strncmp(ov_sdp_attribute_get(session->description->attributes,
                                            "msid"),
                       str, strlen(str)));

  str = "active";
  testrun(0 == strncmp(ov_sdp_attribute_get(session->description->attributes,
                                            "setup"),
                       str, strlen(str)));

  str = "3835337359 cname:{d5014593-8812-4140-9535-1ca252a3dc10}";
  testrun(0 == strncmp(ov_sdp_attribute_get(session->description->attributes,
                                            "ssrc"),
                       str, strlen(str)));

  testrun(
      ov_sdp_attribute_is_set(session->description->attributes, "rtcp-mux"));

  testrun(session->description->attributes);

  // ov_buffer_dump(stdout, sess->buffer);
  session = ov_sdp_session_free(session);

  /*
   *      TBD add negative testing (wrong order / value)
   */

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_sdp_stringify() {

  ov_sdp_session *session = NULL;
  ov_sdp_list *node = NULL;
  ov_sdp_connection *connection = NULL;
  ov_sdp_description *description = NULL;
  ov_sdp_time *time = NULL;

  char *output = NULL;
  char *expect = NULL;

  session = ov_sdp_session_create();
  session->time = calloc(1, sizeof(ov_sdp_time));

  testrun(!ov_sdp_stringify(NULL, false));
  testrun(!ov_sdp_stringify(session, false));

  /*
   *      Set min required
   */

  session->name = "name";

  session->origin.name = "-";
  session->origin.id = 0;
  session->origin.version = 0;

  session->origin.connection.nettype = "net";
  session->origin.connection.addrtype = "type";
  session->origin.connection.address = "addr";

  expect = "v=0\r\n"
           "o=- 0 0 net type addr\r\n"
           "s=name\r\n"
           "t=0 0\r\n";

  output = ov_sdp_stringify(session, false);
  testrun(output);
  // testrun_log("\n------\n%s<- output | expect ->\n%s------\n", output,
  // expect);
  testrun(0 == strncmp(output, expect, strlen(expect)));
  output = ov_data_pointer_free(output);

  session->info = "info";
  expect = "v=0\r\n"
           "o=- 0 0 net type addr\r\n"
           "s=name\r\n"
           "i=info\r\n"
           "t=0 0\r\n";

  output = ov_sdp_stringify(session, false);
  testrun(output);
  // testrun_log("\n------\n%s<- output | expect ->\n%s------\n", output,
  // expect);
  testrun(0 == strncmp(output, expect, strlen(expect)));
  output = ov_data_pointer_free(output);

  session->uri = "http://openvocs.org";
  expect = "v=0\r\n"
           "o=- 0 0 net type addr\r\n"
           "s=name\r\n"
           "i=info\r\n"
           "u=http://openvocs.org\r\n"
           "t=0 0\r\n";

  output = ov_sdp_stringify(session, false);
  testrun(output);
  // testrun_log("\n------\n%s<- output | expect ->\n%s------\n", output,
  // expect);
  testrun(0 == strncmp(output, expect, strlen(expect)));
  output = ov_data_pointer_free(output);

  session->email = calloc(1, sizeof(ov_sdp_list));
  session->email->value = "someone@mail";
  expect = "v=0\r\n"
           "o=- 0 0 net type addr\r\n"
           "s=name\r\n"
           "i=info\r\n"
           "u=http://openvocs.org\r\n"
           "e=someone@mail\r\n"
           "t=0 0\r\n";

  output = ov_sdp_stringify(session, false);
  testrun(output);
  // testrun_log("\n------\n%s<- output | expect ->\n%s------\n", output,
  // expect);
  testrun(0 == strncmp(output, expect, strlen(expect)));
  output = ov_data_pointer_free(output);

  node = calloc(1, sizeof(ov_sdp_list));
  node->value = "someoneelse@mail";
  testrun(ov_node_push((void **)&session->email, node));
  expect = "v=0\r\n"
           "o=- 0 0 net type addr\r\n"
           "s=name\r\n"
           "i=info\r\n"
           "u=http://openvocs.org\r\n"
           "e=someone@mail\r\n"
           "e=someoneelse@mail\r\n"
           "t=0 0\r\n";

  output = ov_sdp_stringify(session, false);
  testrun(output);
  // testrun_log("\n------\n%s<- output | expect ->\n%s------\n", output,
  // expect);
  testrun(0 == strncmp(output, expect, strlen(expect)));
  output = ov_data_pointer_free(output);

  session->phone = calloc(1, sizeof(ov_sdp_list));
  session->phone->value = "+1234";
  expect = "v=0\r\n"
           "o=- 0 0 net type addr\r\n"
           "s=name\r\n"
           "i=info\r\n"
           "u=http://openvocs.org\r\n"
           "e=someone@mail\r\n"
           "e=someoneelse@mail\r\n"
           "p=+1234\r\n"
           "t=0 0\r\n";

  output = ov_sdp_stringify(session, false);
  testrun(output);
  // testrun_log("\n------\n%s<- output | expect ->\n%s------\n", output,
  // expect);
  testrun(0 == strncmp(output, expect, strlen(expect)));
  output = ov_data_pointer_free(output);

  node = calloc(1, sizeof(ov_sdp_list));
  node->value = "+5678(name)";
  testrun(ov_node_push((void **)&session->phone, node));
  expect = "v=0\r\n"
           "o=- 0 0 net type addr\r\n"
           "s=name\r\n"
           "i=info\r\n"
           "u=http://openvocs.org\r\n"
           "e=someone@mail\r\n"
           "e=someoneelse@mail\r\n"
           "p=+1234\r\n"
           "p=+5678(name)\r\n"
           "t=0 0\r\n";

  output = ov_sdp_stringify(session, false);
  testrun(output);
  // testrun_log("\n------\n%s<- output | expect ->\n%s------\n", output,
  // expect);
  testrun(0 == strncmp(output, expect, strlen(expect)));
  output = ov_data_pointer_free(output);

  session->connection = calloc(1, sizeof(ov_sdp_connection));
  session->connection->nettype = "IN";
  session->connection->addrtype = "IP4";
  session->connection->address = "0.0.0.0";

  expect = "v=0\r\n"
           "o=- 0 0 net type addr\r\n"
           "s=name\r\n"
           "i=info\r\n"
           "u=http://openvocs.org\r\n"
           "e=someone@mail\r\n"
           "e=someoneelse@mail\r\n"
           "p=+1234\r\n"
           "p=+5678(name)\r\n"
           "c=IN IP4 0.0.0.0\r\n"
           "t=0 0\r\n";

  output = ov_sdp_stringify(session, false);
  testrun(output);
  // testrun_log("\n------\n%s<- output | expect ->\n%s------\n", output,
  // expect);
  testrun(0 == strncmp(output, expect, strlen(expect)));
  output = ov_data_pointer_free(output);

  session->bandwidth = ov_dict_create(ov_dict_string_key_config(255));
  testrun(ov_dict_set(session->bandwidth, strdup("1"), (void *)1, NULL));
  expect = "v=0\r\n"
           "o=- 0 0 net type addr\r\n"
           "s=name\r\n"
           "i=info\r\n"
           "u=http://openvocs.org\r\n"
           "e=someone@mail\r\n"
           "e=someoneelse@mail\r\n"
           "p=+1234\r\n"
           "p=+5678(name)\r\n"
           "c=IN IP4 0.0.0.0\r\n"
           "b=1:1\r\n"
           "t=0 0\r\n";

  output = ov_sdp_stringify(session, false);
  testrun(output);
  // testrun_log("\n------\n%s<- output | expect ->\n%s------\n", output,
  // expect);
  testrun(0 == strncmp(output, expect, strlen(expect)));
  output = ov_data_pointer_free(output);

  testrun(ov_dict_set(session->bandwidth, strdup("2"), (void *)2, NULL));
  expect = "v=0\r\n"
           "o=- 0 0 net type addr\r\n"
           "s=name\r\n"
           "i=info\r\n"
           "u=http://openvocs.org\r\n"
           "e=someone@mail\r\n"
           "e=someoneelse@mail\r\n"
           "p=+1234\r\n"
           "p=+5678(name)\r\n"
           "c=IN IP4 0.0.0.0\r\n"
           "b=1:1\r\n"
           "b=2:2\r\n"
           "t=0 0\r\n";

  output = ov_sdp_stringify(session, false);
  testrun(output);
  // testrun_log("\n------\n%s<- output | expect ->\n%s------\n", output,
  // expect);
  testrun(0 == strncmp(output, expect, strlen(expect)));
  output = ov_data_pointer_free(output);

  node = calloc(1, sizeof(ov_sdp_list));
  node->key = "1";
  node->value = "1s 1d 1m";
  testrun(ov_node_push((void **)&session->time->repeat, node));

  expect = "v=0\r\n"
           "o=- 0 0 net type addr\r\n"
           "s=name\r\n"
           "i=info\r\n"
           "u=http://openvocs.org\r\n"
           "e=someone@mail\r\n"
           "e=someoneelse@mail\r\n"
           "p=+1234\r\n"
           "p=+5678(name)\r\n"
           "c=IN IP4 0.0.0.0\r\n"
           "b=1:1\r\n"
           "b=2:2\r\n"
           "t=0 0\r\n"
           "r=1 1s 1d 1m\r\n";

  output = ov_sdp_stringify(session, false);
  testrun(output);
  // testrun_log("\n------\n%s<- output | expect ->\n%s------\n", output,
  // expect);
  testrun(0 == strncmp(output, expect, strlen(expect)));
  output = ov_data_pointer_free(output);

  node = calloc(1, sizeof(ov_sdp_list));
  node->key = "2";
  node->value = "2s 2d 2m";
  testrun(ov_node_push((void **)&session->time->repeat, node));

  expect = "v=0\r\n"
           "o=- 0 0 net type addr\r\n"
           "s=name\r\n"
           "i=info\r\n"
           "u=http://openvocs.org\r\n"
           "e=someone@mail\r\n"
           "e=someoneelse@mail\r\n"
           "p=+1234\r\n"
           "p=+5678(name)\r\n"
           "c=IN IP4 0.0.0.0\r\n"
           "b=1:1\r\n"
           "b=2:2\r\n"
           "t=0 0\r\n"
           "r=1 1s 1d 1m\r\n"
           "r=2 2s 2d 2m\r\n";

  output = ov_sdp_stringify(session, false);
  testrun(output);
  // testrun_log("\n------\n%s<- output | expect ->\n%s------\n", output,
  // expect);
  testrun(0 == strncmp(output, expect, strlen(expect)));
  output = ov_data_pointer_free(output);

  node = calloc(1, sizeof(ov_sdp_list));
  node->key = "0";
  node->value = "-25s";
  testrun(ov_node_push((void **)&session->time->zone, node));

  expect = "v=0\r\n"
           "o=- 0 0 net type addr\r\n"
           "s=name\r\n"
           "i=info\r\n"
           "u=http://openvocs.org\r\n"
           "e=someone@mail\r\n"
           "e=someoneelse@mail\r\n"
           "p=+1234\r\n"
           "p=+5678(name)\r\n"
           "c=IN IP4 0.0.0.0\r\n"
           "b=1:1\r\n"
           "b=2:2\r\n"
           "t=0 0\r\n"
           "r=1 1s 1d 1m\r\n"
           "r=2 2s 2d 2m\r\n"
           "z=0 -25s\r\n";

  output = ov_sdp_stringify(session, false);
  testrun(output);
  // testrun_log("\n------\n%s<- output | expect ->\n%s------\n", output,
  // expect);
  testrun(0 == strncmp(output, expect, strlen(expect)));
  output = ov_data_pointer_free(output);

  session->key = "clear:text";
  expect = "v=0\r\n"
           "o=- 0 0 net type addr\r\n"
           "s=name\r\n"
           "i=info\r\n"
           "u=http://openvocs.org\r\n"
           "e=someone@mail\r\n"
           "e=someoneelse@mail\r\n"
           "p=+1234\r\n"
           "p=+5678(name)\r\n"
           "c=IN IP4 0.0.0.0\r\n"
           "b=1:1\r\n"
           "b=2:2\r\n"
           "t=0 0\r\n"
           "r=1 1s 1d 1m\r\n"
           "r=2 2s 2d 2m\r\n"
           "z=0 -25s\r\n"
           "k=clear:text\r\n";

  output = ov_sdp_stringify(session, false);
  testrun(output);
  // testrun_log("\n------\n%s<- output | expect ->\n%s------\n", output,
  // expect);
  testrun(0 == strncmp(output, expect, strlen(expect)));
  output = ov_data_pointer_free(output);

  testrun(ov_sdp_attribute_add(&session->attributes, "1", NULL));
  expect = "v=0\r\n"
           "o=- 0 0 net type addr\r\n"
           "s=name\r\n"
           "i=info\r\n"
           "u=http://openvocs.org\r\n"
           "e=someone@mail\r\n"
           "e=someoneelse@mail\r\n"
           "p=+1234\r\n"
           "p=+5678(name)\r\n"
           "c=IN IP4 0.0.0.0\r\n"
           "b=1:1\r\n"
           "b=2:2\r\n"
           "t=0 0\r\n"
           "r=1 1s 1d 1m\r\n"
           "r=2 2s 2d 2m\r\n"
           "z=0 -25s\r\n"
           "k=clear:text\r\n"
           "a=1\r\n";

  output = ov_sdp_stringify(session, false);
  testrun(output);
  // testrun_log("\n------\n%s<- output | expect ->\n%s------\n", output,
  // expect);
  testrun(0 == strncmp(output, expect, strlen(expect)));
  output = ov_data_pointer_free(output);

  testrun(ov_sdp_attribute_add(&session->attributes, "2", "two"));
  testrun(ov_sdp_attribute_add(&session->attributes, "2", "x"));
  testrun(ov_sdp_attribute_add(&session->attributes, "3", "y"));
  expect = "v=0\r\n"
           "o=- 0 0 net type addr\r\n"
           "s=name\r\n"
           "i=info\r\n"
           "u=http://openvocs.org\r\n"
           "e=someone@mail\r\n"
           "e=someoneelse@mail\r\n"
           "p=+1234\r\n"
           "p=+5678(name)\r\n"
           "c=IN IP4 0.0.0.0\r\n"
           "b=1:1\r\n"
           "b=2:2\r\n"
           "t=0 0\r\n"
           "r=1 1s 1d 1m\r\n"
           "r=2 2s 2d 2m\r\n"
           "z=0 -25s\r\n"
           "k=clear:text\r\n"
           "a=1\r\n"
           "a=2:two\r\n"
           "a=2:x\r\n"
           "a=3:y\r\n";

  output = ov_sdp_stringify(session, false);
  testrun(output);
  // testrun_log("\n------\n%s<- output | expect ->\n%s------\n", output,
  // expect);
  testrun(0 == strncmp(output, expect, strlen(expect)));
  output = ov_data_pointer_free(output);

  session->description = calloc(1, sizeof(ov_sdp_description));
  session->description->media.name = "audio";
  session->description->media.protocol = "RTP/AVP";
  session->description->media.port = 12345;
  node = calloc(1, sizeof(ov_sdp_list));
  node->value = "100";
  testrun(ov_node_push((void **)&session->description->media.formats, node));

  expect = "v=0\r\n"
           "o=- 0 0 net type addr\r\n"
           "s=name\r\n"
           "i=info\r\n"
           "u=http://openvocs.org\r\n"
           "e=someone@mail\r\n"
           "e=someoneelse@mail\r\n"
           "p=+1234\r\n"
           "p=+5678(name)\r\n"
           "c=IN IP4 0.0.0.0\r\n"
           "b=1:1\r\n"
           "b=2:2\r\n"
           "t=0 0\r\n"
           "r=1 1s 1d 1m\r\n"
           "r=2 2s 2d 2m\r\n"
           "z=0 -25s\r\n"
           "k=clear:text\r\n"
           "a=1\r\n"
           "a=2:two\r\n"
           "a=2:x\r\n"
           "a=3:y\r\n"
           "m=audio 12345 RTP/AVP 100\r\n";

  output = ov_sdp_stringify(session, false);
  testrun(output);
  // testrun_log("\n------\n%s<- output | expect ->\n%s------\n", output,
  // expect);
  testrun(0 == strncmp(output, expect, strlen(expect)));
  output = ov_data_pointer_free(output);

  session->description->media.port2 = 6789;

  expect = "v=0\r\n"
           "o=- 0 0 net type addr\r\n"
           "s=name\r\n"
           "i=info\r\n"
           "u=http://openvocs.org\r\n"
           "e=someone@mail\r\n"
           "e=someoneelse@mail\r\n"
           "p=+1234\r\n"
           "p=+5678(name)\r\n"
           "c=IN IP4 0.0.0.0\r\n"
           "b=1:1\r\n"
           "b=2:2\r\n"
           "t=0 0\r\n"
           "r=1 1s 1d 1m\r\n"
           "r=2 2s 2d 2m\r\n"
           "z=0 -25s\r\n"
           "k=clear:text\r\n"
           "a=1\r\n"
           "a=2:two\r\n"
           "a=2:x\r\n"
           "a=3:y\r\n"
           "m=audio 12345/6789 RTP/AVP 100\r\n";

  output = ov_sdp_stringify(session, false);
  testrun(output);
  // testrun_log("\n------\n%s<- output | expect ->\n%s------\n", output,
  // expect);
  testrun(0 == strncmp(output, expect, strlen(expect)));
  output = ov_data_pointer_free(output);

  session->description->info = "info";
  session->description->key = "prompt";
  session->description->connection = calloc(1, sizeof(ov_sdp_connection));
  session->description->connection->nettype = "IN";
  session->description->connection->addrtype = "IP4";
  session->description->connection->address = "0.0.0.0";
  session->description->bandwidth =
      ov_dict_create(ov_dict_string_key_config(255));
  testrun(ov_dict_set(session->description->bandwidth, strdup("1"), (void *)1,
                      NULL));
  testrun(ov_dict_set(session->description->bandwidth, strdup("2"), (void *)2,
                      NULL));
  testrun(ov_dict_set(session->description->bandwidth, strdup("3"), (void *)3,
                      NULL));
  testrun(ov_sdp_attribute_add(&session->description->attributes, "1", "one"));
  testrun(ov_sdp_attribute_add(&session->description->attributes, "2", "x"));
  testrun(ov_sdp_attribute_add(&session->description->attributes, "2", "y"));

  expect = "v=0\r\n"
           "o=- 0 0 net type addr\r\n"
           "s=name\r\n"
           "i=info\r\n"
           "u=http://openvocs.org\r\n"
           "e=someone@mail\r\n"
           "e=someoneelse@mail\r\n"
           "p=+1234\r\n"
           "p=+5678(name)\r\n"
           "c=IN IP4 0.0.0.0\r\n"
           "b=1:1\r\n"
           "b=2:2\r\n"
           "t=0 0\r\n"
           "r=1 1s 1d 1m\r\n"
           "r=2 2s 2d 2m\r\n"
           "z=0 -25s\r\n"
           "k=clear:text\r\n"
           "a=1\r\n"
           "a=2:two\r\n"
           "a=2:x\r\n"
           "a=3:y\r\n"
           "m=audio 12345/6789 RTP/AVP 100\r\n"
           "i=info\r\n"
           "c=IN IP4 0.0.0.0\r\n"
           "b=1:1\r\n"
           "b=3:3\r\n"
           "b=2:2\r\n"
           "k=prompt\r\n"
           "a=1:one\r\n"
           "a=2:x\r\n"
           "a=2:y\r\n";
  output = ov_sdp_stringify(session, false);
  testrun(output);
  // testrun_log("\n------\n%s<- output | expect ->\n%s------\n", output,
  // expect);
  testrun(0 == strncmp(output, expect, strlen(expect)));
  output = ov_data_pointer_free(output);

  description = calloc(1, sizeof(ov_sdp_description));
  testrun(ov_node_push((void **)&session->description, description));

  description->media.name = "video";
  description->media.protocol = "RTP/AVP";
  description->media.port = 1;
  node = calloc(1, sizeof(ov_sdp_list));
  node->value = "101";
  testrun(ov_node_push((void **)&description->media.formats, node));

  time = calloc(1, sizeof(ov_sdp_time));
  time->start = 1234567890;
  time->stop = 1234567890;
  testrun(ov_node_push((void **)&session->time, time));

  expect = "v=0\r\n"
           "o=- 0 0 net type addr\r\n"
           "s=name\r\n"
           "i=info\r\n"
           "u=http://openvocs.org\r\n"
           "e=someone@mail\r\n"
           "e=someoneelse@mail\r\n"
           "p=+1234\r\n"
           "p=+5678(name)\r\n"
           "c=IN IP4 0.0.0.0\r\n"
           "b=1:1\r\n"
           "b=2:2\r\n"
           "t=0 0\r\n"
           "r=1 1s 1d 1m\r\n"
           "r=2 2s 2d 2m\r\n"
           "z=0 -25s\r\n"
           "t=1234567890 1234567890\r\n"
           "k=clear:text\r\n"
           "a=1\r\n"
           "a=2:two\r\n"
           "a=2:x\r\n"
           "a=3:y\r\n"
           "m=audio 12345/6789 RTP/AVP 100\r\n"
           "i=info\r\n"
           "c=IN IP4 0.0.0.0\r\n"
           "b=1:1\r\n"
           "b=3:3\r\n"
           "b=2:2\r\n"
           "k=prompt\r\n"
           "a=1:one\r\n"
           "a=2:x\r\n"
           "a=2:y\r\n"
           "m=video 1 RTP/AVP 101\r\n";
  output = ov_sdp_stringify(session, false);
  testrun(output);
  // testrun_log("\n------\n%s<- output | expect ->\n%s------\n", output,
  // expect);
  testrun(0 == strncmp(output, expect, strlen(expect)));
  output = ov_data_pointer_free(output);

  connection = calloc(1, sizeof(ov_sdp_connection));
  testrun(connection);
  testrun(ov_node_push((void **)&session->description->connection, connection))
      connection->nettype = "1";
  connection->addrtype = "2";
  connection->address = "3";

  connection = calloc(1, sizeof(ov_sdp_connection));
  testrun(connection);
  testrun(ov_node_push((void **)&session->description->connection, connection))
      connection->nettype = "4";
  connection->addrtype = "5";
  connection->address = "6";

  expect = "v=0\r\n"
           "o=- 0 0 net type addr\r\n"
           "s=name\r\n"
           "i=info\r\n"
           "u=http://openvocs.org\r\n"
           "e=someone@mail\r\n"
           "e=someoneelse@mail\r\n"
           "p=+1234\r\n"
           "p=+5678(name)\r\n"
           "c=IN IP4 0.0.0.0\r\n"
           "b=1:1\r\n"
           "b=2:2\r\n"
           "t=0 0\r\n"
           "r=1 1s 1d 1m\r\n"
           "r=2 2s 2d 2m\r\n"
           "z=0 -25s\r\n"
           "t=1234567890 1234567890\r\n"
           "k=clear:text\r\n"
           "a=1\r\n"
           "a=2:two\r\n"
           "a=2:x\r\n"
           "a=3:y\r\n"
           "m=audio 12345/6789 RTP/AVP 100\r\n"
           "i=info\r\n"
           "c=IN IP4 0.0.0.0\r\n"
           "c=1 2 3\r\n"
           "c=4 5 6\r\n"
           "b=1:1\r\n"
           "b=3:3\r\n"
           "b=2:2\r\n"
           "k=prompt\r\n"
           "a=1:one\r\n"
           "a=2:x\r\n"
           "a=2:y\r\n"
           "m=video 1 RTP/AVP 101\r\n";
  output = ov_sdp_stringify(session, false);
  testrun(output);
  // testrun_log("\n------\n%s<- output | expect ->\n%s------\n", output,
  // expect);
  testrun(0 == strncmp(output, expect, strlen(expect)));
  output = ov_data_pointer_free(output);

  testrun(NULL == ov_sdp_session_free(session));
  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_sdp_enable_caching() {

  ov_sdp_session *s1[10] = {0};
  ov_sdp_session *s2[10] = {0};

  for (size_t i = 0; i < 10; i++)
    s1[i] = ov_sdp_session_create();

  testrun(NULL == ov_sdp_cache.session);
  testrun(NULL == ov_sdp_cache.buffer);
  testrun(NULL == ov_sdp_cache.description);
  testrun(NULL == ov_sdp_cache.bandwidth);
  testrun(NULL == ov_sdp_cache.list);
  testrun(NULL == ov_sdp_cache.time);
  testrun(NULL == ov_sdp_cache.connection);

  ov_sdp_enable_caching(5);

  testrun(NULL != ov_sdp_cache.session);
  testrun(NULL != ov_sdp_cache.buffer);
  testrun(NULL != ov_sdp_cache.description);
  testrun(NULL != ov_sdp_cache.bandwidth);
  testrun(NULL != ov_sdp_cache.list);
  testrun(NULL != ov_sdp_cache.time);
  testrun(NULL != ov_sdp_cache.connection);

  for (size_t i = 0; i < 10; i++)
    ov_sdp_session_free(s1[i]);

  for (size_t i = 0; i < 10; i++)
    s2[i] = ov_sdp_session_create();

  /*
   *  Check pop first (stack)
   */

  testrun(s2[0] == s1[4]);
  testrun(s2[1] == s1[3]);
  testrun(s2[2] == s1[2]);
  testrun(s2[3] == s1[1]);
  testrun(s2[4] == s1[0]);

  ov_sdp_disable_caching();

  testrun(NULL == ov_sdp_cache.session);
  testrun(NULL == ov_sdp_cache.buffer);
  testrun(NULL == ov_sdp_cache.description);
  testrun(NULL == ov_sdp_cache.bandwidth);
  testrun(NULL == ov_sdp_cache.list);
  testrun(NULL == ov_sdp_cache.time);
  testrun(NULL == ov_sdp_cache.connection);

  for (size_t i = 0; i < 10; i++)
    s2[i] = ov_sdp_session_free(s2[i]);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_sdp_disable_caching() {

  ov_sdp_session *sess[10] = {0};
  ov_buffer *buff[10] = {0};
  ov_dict *band[10] = {0};
  ov_sdp_list *list[10] = {0};
  ov_sdp_connection *conn[10] = {0};
  ov_sdp_time *time[10] = {0};

  ov_sdp_disable_caching();

  testrun(false == ov_sdp_cache.enabled);
  testrun(NULL == ov_sdp_cache.session);
  testrun(NULL == ov_sdp_cache.buffer);
  testrun(NULL == ov_sdp_cache.description);
  testrun(NULL == ov_sdp_cache.bandwidth);
  testrun(NULL == ov_sdp_cache.list);
  testrun(NULL == ov_sdp_cache.time);
  testrun(NULL == ov_sdp_cache.connection);

  ov_sdp_enable_caching(10);

  for (size_t i = 0; i < 10; i++) {
    sess[i] = ov_sdp_session_create();
    buff[i] = ov_buffer_create(100);
    band[i] = ov_sdp_bandwidth_create();
    list[i] = ov_sdp_list_create();
    conn[i] = ov_sdp_connection_create();
    time[i] = ov_sdp_time_create();
  }

  for (size_t i = 0; i < 10; i++) {
    sess[i] = ov_sdp_session_free(sess[i]);
    buff[i] = ov_sdp_buffer_free(buff[i]);
    band[i] = ov_sdp_bandwidth_free(band[i]);
    list[i] = ov_sdp_list_free(list[i]);
    conn[i] = ov_sdp_connection_free(conn[i]);
    time[i] = ov_sdp_time_free(time[i]);
  }

  ov_sdp_disable_caching();

  testrun(false == ov_sdp_cache.enabled);
  testrun(NULL == ov_sdp_cache.session);
  testrun(NULL == ov_sdp_cache.buffer);
  testrun(NULL == ov_sdp_cache.description);
  testrun(NULL == ov_sdp_cache.bandwidth);
  testrun(NULL == ov_sdp_cache.list);
  testrun(NULL == ov_sdp_cache.time);
  testrun(NULL == ov_sdp_cache.connection);

  /*
   *  Check valgrind run
   */

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_sdp_buffer_create() {

  ov_buffer *b1[10] = {0};
  ov_buffer *b2[10] = {0};

  ov_sdp_disable_caching();

  testrun(false == ov_sdp_cache.enabled);

  for (size_t i = 0; i < 10; i++)
    b1[i] = ov_sdp_buffer_create();

  for (size_t i = 0; i < 10; i++)
    b1[i] = ov_sdp_buffer_free(b1[i]);

  ov_sdp_enable_caching(10);

  testrun(true == ov_sdp_cache.enabled);

  for (size_t i = 0; i < 10; i++)
    b1[i] = ov_sdp_buffer_create();

  for (size_t i = 0; i < 10; i++) {
    ov_sdp_buffer_free(b1[i]);
    b2[i] = ov_sdp_buffer_create();
    testrun(b1[i] == b2[i]);
  }

  for (size_t i = 0; i < 10; i++)
    b2[i] = ov_sdp_buffer_free(b2[i]);

  ov_sdp_disable_caching();

  testrun(false == ov_sdp_cache.enabled);
  testrun(NULL == ov_sdp_cache.session);
  testrun(NULL == ov_sdp_cache.buffer);
  testrun(NULL == ov_sdp_cache.description);
  testrun(NULL == ov_sdp_cache.bandwidth);
  testrun(NULL == ov_sdp_cache.list);
  testrun(NULL == ov_sdp_cache.time);
  testrun(NULL == ov_sdp_cache.connection);

  /*
   *  Check valgrind run
   */

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_sdp_buffer_free() {

  // tested @see test_ov_sdp_buffer_create
  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_sdp_description_create() {

  ov_sdp_description *b1[10] = {0};
  ov_sdp_description *b2[10] = {0};

  ov_sdp_disable_caching();

  testrun(false == ov_sdp_cache.enabled);

  for (size_t i = 0; i < 10; i++)
    b1[i] = ov_sdp_description_create();

  for (size_t i = 0; i < 10; i++)
    b1[i] = ov_sdp_description_free(b1[i]);

  ov_sdp_enable_caching(10);

  testrun(true == ov_sdp_cache.enabled);

  for (size_t i = 0; i < 10; i++)
    b1[i] = ov_sdp_description_create();

  for (size_t i = 0; i < 10; i++) {
    ov_sdp_description_free(b1[i]);
    b2[i] = ov_sdp_description_create();
    testrun(b1[i] == b2[i]);
  }

  for (size_t i = 0; i < 10; i++)
    b2[i] = ov_sdp_description_free(b2[i]);

  ov_sdp_disable_caching();

  testrun(false == ov_sdp_cache.enabled);
  testrun(NULL == ov_sdp_cache.session);
  testrun(NULL == ov_sdp_cache.buffer);
  testrun(NULL == ov_sdp_cache.description);
  testrun(NULL == ov_sdp_cache.bandwidth);
  testrun(NULL == ov_sdp_cache.list);
  testrun(NULL == ov_sdp_cache.time);
  testrun(NULL == ov_sdp_cache.connection);

  /*
   *  Check valgrind run
   */

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_sdp_description_free() {

  // tested @see test_ov_sdp_description_create
  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_sdp_bandwidth_create() {

  ov_dict *b1[10] = {0};
  ov_dict *b2[10] = {0};

  ov_sdp_disable_caching();

  testrun(false == ov_sdp_cache.enabled);

  for (size_t i = 0; i < 10; i++)
    b1[i] = ov_sdp_bandwidth_create();

  for (size_t i = 0; i < 10; i++)
    b1[i] = ov_sdp_bandwidth_free(b1[i]);

  ov_sdp_enable_caching(10);

  testrun(true == ov_sdp_cache.enabled);

  for (size_t i = 0; i < 10; i++)
    b1[i] = ov_sdp_bandwidth_create();

  for (size_t i = 0; i < 10; i++) {
    ov_sdp_bandwidth_free(b1[i]);
    b2[i] = ov_sdp_bandwidth_create();
    testrun(b1[i] == b2[i]);
  }

  for (size_t i = 0; i < 10; i++)
    b2[i] = ov_sdp_bandwidth_free(b2[i]);

  ov_sdp_disable_caching();

  testrun(false == ov_sdp_cache.enabled);
  testrun(NULL == ov_sdp_cache.session);
  testrun(NULL == ov_sdp_cache.buffer);
  testrun(NULL == ov_sdp_cache.description);
  testrun(NULL == ov_sdp_cache.bandwidth);
  testrun(NULL == ov_sdp_cache.list);
  testrun(NULL == ov_sdp_cache.time);
  testrun(NULL == ov_sdp_cache.connection);

  /*
   *  Check valgrind run
   */

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_sdp_bandwidth_free() {

  // tested @see test_ov_sdp_bandwidth_create
  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_sdp_list_create() {

  ov_sdp_list *b1[10] = {0};
  ov_sdp_list *b2[10] = {0};

  ov_sdp_disable_caching();

  testrun(false == ov_sdp_cache.enabled);

  for (size_t i = 0; i < 10; i++)
    b1[i] = ov_sdp_list_create();

  for (size_t i = 0; i < 10; i++)
    b1[i] = ov_sdp_list_free(b1[i]);

  ov_sdp_enable_caching(10);

  testrun(true == ov_sdp_cache.enabled);

  for (size_t i = 0; i < 10; i++)
    b1[i] = ov_sdp_list_create();

  for (size_t i = 0; i < 10; i++) {
    ov_sdp_list_free(b1[i]);
    b2[i] = ov_sdp_list_create();
    testrun(b1[i] == b2[i]);
  }

  for (size_t i = 0; i < 10; i++)
    b2[i] = ov_sdp_list_free(b2[i]);

  ov_sdp_disable_caching();

  testrun(false == ov_sdp_cache.enabled);
  testrun(NULL == ov_sdp_cache.session);
  testrun(NULL == ov_sdp_cache.buffer);
  testrun(NULL == ov_sdp_cache.description);
  testrun(NULL == ov_sdp_cache.bandwidth);
  testrun(NULL == ov_sdp_cache.list);
  testrun(NULL == ov_sdp_cache.time);
  testrun(NULL == ov_sdp_cache.connection);

  /*
   *  Check valgrind run
   */

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_sdp_list_free() {

  // tested @see ov_sdp_list_create
  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_sdp_time_create() {

  ov_sdp_time *b1[10] = {0};
  ov_sdp_time *b2[10] = {0};

  ov_sdp_disable_caching();

  testrun(false == ov_sdp_cache.enabled);

  for (size_t i = 0; i < 10; i++)
    b1[i] = ov_sdp_time_create();

  for (size_t i = 0; i < 10; i++)
    b1[i] = ov_sdp_time_free(b1[i]);

  ov_sdp_enable_caching(10);

  testrun(true == ov_sdp_cache.enabled);

  for (size_t i = 0; i < 10; i++)
    b1[i] = ov_sdp_time_create();

  for (size_t i = 0; i < 10; i++) {
    ov_sdp_time_free(b1[i]);
    b2[i] = ov_sdp_time_create();
    testrun(b1[i] == b2[i]);
  }

  for (size_t i = 0; i < 10; i++)
    b2[i] = ov_sdp_time_free(b2[i]);

  ov_sdp_disable_caching();

  testrun(false == ov_sdp_cache.enabled);
  testrun(NULL == ov_sdp_cache.session);
  testrun(NULL == ov_sdp_cache.buffer);
  testrun(NULL == ov_sdp_cache.description);
  testrun(NULL == ov_sdp_cache.bandwidth);
  testrun(NULL == ov_sdp_cache.list);
  testrun(NULL == ov_sdp_cache.time);
  testrun(NULL == ov_sdp_cache.connection);

  /*
   *  Check valgrind run
   */

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_sdp_time_free() {

  // tested @see test_ov_sdp_time_create
  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_sdp_connection_create() {

  ov_sdp_connection *b1[10] = {0};
  ov_sdp_connection *b2[10] = {0};

  ov_sdp_disable_caching();

  testrun(false == ov_sdp_cache.enabled);

  for (size_t i = 0; i < 10; i++)
    b1[i] = ov_sdp_connection_create();

  for (size_t i = 0; i < 10; i++)
    b1[i] = ov_sdp_connection_free(b1[i]);

  ov_sdp_enable_caching(10);

  testrun(true == ov_sdp_cache.enabled);

  for (size_t i = 0; i < 10; i++)
    b1[i] = ov_sdp_connection_create();

  for (size_t i = 0; i < 10; i++) {
    ov_sdp_connection_free(b1[i]);
    b2[i] = ov_sdp_connection_create();
    testrun(b1[i] == b2[i]);
  }

  for (size_t i = 0; i < 10; i++)
    b2[i] = ov_sdp_connection_free(b2[i]);

  ov_sdp_disable_caching();

  testrun(false == ov_sdp_cache.enabled);
  testrun(NULL == ov_sdp_cache.session);
  testrun(NULL == ov_sdp_cache.buffer);
  testrun(NULL == ov_sdp_cache.description);
  testrun(NULL == ov_sdp_cache.bandwidth);
  testrun(NULL == ov_sdp_cache.list);
  testrun(NULL == ov_sdp_cache.time);
  testrun(NULL == ov_sdp_cache.connection);

  /*
   *  Check valgrind run
   */

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_sdp_connection_free() {

  // tested @see test_ov_sdp_connection_create
  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_sdp_session_cast() {

  for (size_t i = 0; i < 0xffff; i++) {

    ov_sdp_session session = (ov_sdp_session){.magic_byte = i};

    if (i == OV_SDP_SESSION_MAGIC_BYTE) {
      testrun(ov_sdp_session_cast(&session))
    } else {
      testrun(!ov_sdp_session_cast(&session));
    }
  }

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_sdp_session_clear() {

  ov_dict *dict = ov_sdp_bandwidth_create();
  ov_sdp_session *session = ov_sdp_session_create();
  testrun(session);

  testrun(!ov_sdp_session_clear(NULL));
  testrun(!ov_sdp_session_clear((ov_sdp_session *)dict));

  testrun(ov_sdp_session_clear(session));

  testrun(0 == session->version);
  testrun(NULL == session->name);
  testrun(NULL == session->origin.name);
  testrun(0 == session->origin.id);
  testrun(0 == session->origin.version);
  testrun(NULL == session->origin.connection.nettype);
  testrun(NULL == session->origin.connection.addrtype);
  testrun(NULL == session->origin.connection.address);

  testrun(NULL == session->info);
  testrun(NULL == session->uri);
  testrun(NULL == session->key);

  testrun(NULL == session->connection);
  testrun(NULL == session->email);
  testrun(NULL == session->phone);
  testrun(NULL == session->bandwidth);
  testrun(NULL == session->attributes);
  testrun(NULL == session->description);

  session->version = 10;
  session->name = "name";
  session->origin.name = "origin";
  session->origin.id = 100;
  session->origin.version = 1;
  session->origin.connection.nettype = "IN";
  session->origin.connection.addrtype = "IP4";
  session->origin.connection.address = "127.0.0.1";
  session->info = "info";
  session->uri = "uri";
  session->key = "plain";
  session->connection = ov_sdp_connection_create();
  session->email = ov_sdp_list_create();
  session->phone = ov_sdp_list_create();
  session->bandwidth = ov_sdp_bandwidth_create();
  session->attributes = ov_sdp_list_create();
  session->description = ov_sdp_description_create();

  testrun(ov_sdp_session_clear(session));

  testrun(0 == session->version);
  testrun(NULL == session->name);
  testrun(NULL == session->origin.name);
  testrun(0 == session->origin.id);
  testrun(0 == session->origin.version);
  testrun(NULL == session->origin.connection.nettype);
  testrun(NULL == session->origin.connection.addrtype);
  testrun(NULL == session->origin.connection.address);

  testrun(NULL == session->info);
  testrun(NULL == session->uri);
  testrun(NULL == session->key);

  testrun(NULL == session->connection);
  testrun(NULL == session->email);
  testrun(NULL == session->phone);
  testrun(NULL == session->bandwidth);
  testrun(NULL == session->attributes);
  testrun(NULL == session->description);

  DefaultSession *sess = AS_DEFAULT_SESSION(session);
  testrun(NULL == sess->buffer);

  session = ov_sdp_session_free(session);
  dict = ov_sdp_bandwidth_free(dict);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_sdp_session_free() {

  ov_dict *dict = ov_sdp_bandwidth_create();
  ov_sdp_session *session = ov_sdp_session_create();
  testrun(session);

  testrun(NULL == ov_sdp_session_free(NULL));
  testrun(dict == ov_sdp_session_free(dict));

  testrun(NULL == ov_sdp_session_free(session));

  session = ov_sdp_session_create();
  session->version = 10;
  session->name = "name";
  session->origin.name = "origin";
  session->origin.id = 100;
  session->origin.version = 1;
  session->origin.connection.nettype = "IN";
  session->origin.connection.addrtype = "IP4";
  session->origin.connection.address = "127.0.0.1";
  session->info = "info";
  session->uri = "uri";
  session->key = "plain";
  session->connection = ov_sdp_connection_create();
  session->email = ov_sdp_list_create();
  session->phone = ov_sdp_list_create();
  session->bandwidth = ov_sdp_bandwidth_create();
  session->attributes = ov_sdp_list_create();
  session->description = ov_sdp_description_create();

  testrun(NULL == ov_sdp_session_free(session));

  session = ov_sdp_session_create();

  /*
   *      Set min required
   */

  session->name = "name";

  session->origin.name = "-";
  session->origin.id = 0;
  session->origin.version = 0;

  session->origin.connection.nettype = "net";
  session->origin.connection.addrtype = "type";
  session->origin.connection.address = "addr";

  session->time = ov_sdp_time_create();

  testrun(ov_sdp_session_persist(session));

  session->version = 0;
  session->name = "-";
  session->origin.name = "origin";
  session->origin.id = 100;
  session->origin.version = 1;
  session->origin.connection.nettype = "IN";
  session->origin.connection.addrtype = "IP4";
  session->origin.connection.address = "127.0.0.1";
  session->info = "info";
  session->uri = "uri";
  session->key = "clear:text";
  session->connection = ov_sdp_connection_create();
  session->connection->nettype = "IN";
  session->connection->addrtype = "IP6";
  session->connection->address = "::1";
  session->attributes = ov_sdp_list_create();
  session->attributes->key = "1";
  session->attributes->value = "2";
  session->description = ov_sdp_description_create();
  session->description->media.name = "audio";
  session->description->media.protocol = "RTP/AVP";
  session->description->media.port = 1234;
  session->description->media.formats = ov_sdp_list_create();
  session->description->media.formats->value = "100";
  testrun(ov_sdp_session_persist(session));

  testrun(NULL == ov_sdp_session_free(session));
  dict = ov_sdp_bandwidth_free(dict);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_sdp_session_persist() {

  ov_dict *dict = ov_sdp_bandwidth_create();
  ov_sdp_session *session = ov_sdp_session_create();
  testrun(session);

  testrun(!ov_sdp_session_persist(NULL));
  testrun(!ov_sdp_session_persist((ov_sdp_session *)dict));

  // invalid session
  testrun(!ov_sdp_session_persist(session));

  session->version = 0;
  session->name = "name";
  session->time = ov_sdp_time_create();
  session->origin.name = "origin";
  session->origin.id = 100;
  session->origin.version = 1;
  session->origin.connection.nettype = "IN";
  session->origin.connection.addrtype = "IP4";
  session->origin.connection.address = "127.0.0.1";
  session->info = "info";
  session->uri = "uri";
  session->key = "clear:text";
  session->connection = ov_sdp_connection_create();
  session->connection->nettype = "IN";
  session->connection->addrtype = "IP6";
  session->connection->address = "::";
  session->attributes = ov_sdp_list_create();
  session->attributes->key = "1";
  session->attributes->value = "2";
  session->description = ov_sdp_description_create();
  session->description->media.name = "audio";
  session->description->media.protocol = "RTP/AVP";
  session->description->media.port = 1234;
  session->description->media.formats = ov_sdp_list_create();
  session->description->media.formats->value = "100";

  testrun(ov_sdp_session_persist(session));

  /*
   *  We check persisance over the implementation
   *  (content written to internal buffer)
   */

  DefaultSession *sess = AS_DEFAULT_SESSION(session);

  size_t offset = session->name - (char *)sess->buffer->start;
  testrun(0 == strncmp((char *)sess->buffer->start + offset, "name", 4));

  offset = session->origin.connection.address - (char *)sess->buffer->start;
  testrun(0 == strncmp((char *)sess->buffer->start + offset, "127.0.0.1", 9));

  offset = session->info - (char *)sess->buffer->start;
  testrun(0 == strncmp((char *)sess->buffer->start + offset, "info", 4));

  offset = session->key - (char *)sess->buffer->start;
  testrun(0 == strncmp((char *)sess->buffer->start + offset, "clear:text", 10));

  offset = session->uri - (char *)sess->buffer->start;
  testrun(0 == strncmp((char *)sess->buffer->start + offset, "uri", 3));

  offset = session->connection->address - (char *)sess->buffer->start;
  testrun(0 == strncmp((char *)sess->buffer->start + offset, "::", 2));

  offset = session->description->media.name - (char *)sess->buffer->start;
  testrun(0 == strncmp((char *)sess->buffer->start + offset, "audio", 5));

  offset = session->description->media.protocol - (char *)sess->buffer->start;
  testrun(0 == strncmp((char *)sess->buffer->start + offset, "RTP/AVP", 7));

  testrun(NULL == ov_sdp_session_free(session));
  dict = ov_sdp_bandwidth_free(dict);
  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_sdp_session_copy() {

  ov_dict *dict = ov_sdp_bandwidth_create();
  ov_sdp_session *session = NULL;
  ov_sdp_session *copy = NULL;

  session = ov_sdp_session_create();
  testrun(session);

  testrun(NULL == ov_sdp_session_copy(NULL));
  testrun(NULL == ov_sdp_session_copy((ov_sdp_session *)dict));

  // copy empty (invalid)
  testrun(NULL == ov_sdp_session_copy(session));

  // copy valid
  session->version = 0;
  session->name = "name";
  session->time = ov_sdp_time_create();
  session->origin.name = "origin";
  session->origin.id = 100;
  session->origin.version = 1;
  session->origin.connection.nettype = "IN";
  session->origin.connection.addrtype = "IP4";
  session->origin.connection.address = "127.0.0.1";
  session->info = "info";
  session->uri = "uri";
  session->key = "clear:text";
  session->connection = ov_sdp_connection_create();
  session->connection->nettype = "IN";
  session->connection->addrtype = "IP6";
  session->connection->address = "::";
  session->attributes = ov_sdp_list_create();
  session->attributes->key = "1";
  session->attributes->value = "2";
  session->description = ov_sdp_description_create();
  session->description->media.name = "audio";
  session->description->media.protocol = "RTP/AVP";
  session->description->media.port = 1234;
  session->description->media.formats = ov_sdp_list_create();
  session->description->media.formats->value = "100";

  copy = ov_sdp_session_copy(session);
  testrun(copy);

  /*
   *  We check persisance over the implementation
   *  (content written to internal buffer)
   */

  DefaultSession *sess = AS_DEFAULT_SESSION(copy);

  size_t offset = copy->name - (char *)sess->buffer->start;
  testrun(0 == strncmp((char *)sess->buffer->start + offset, "name", 4));

  offset = copy->origin.connection.address - (char *)sess->buffer->start;
  testrun(0 == strncmp((char *)sess->buffer->start + offset, "127.0.0.1", 9));

  offset = copy->info - (char *)sess->buffer->start;
  testrun(0 == strncmp((char *)sess->buffer->start + offset, "info", 4));

  offset = copy->key - (char *)sess->buffer->start;
  testrun(0 == strncmp((char *)sess->buffer->start + offset, "clear:text", 10));

  offset = copy->uri - (char *)sess->buffer->start;
  testrun(0 == strncmp((char *)sess->buffer->start + offset, "uri", 3));

  offset = copy->connection->address - (char *)sess->buffer->start;
  testrun(0 == strncmp((char *)sess->buffer->start + offset, "::", 2));

  offset = copy->description->media.name - (char *)sess->buffer->start;
  testrun(0 == strncmp((char *)sess->buffer->start + offset, "audio", 5));

  offset = copy->description->media.protocol - (char *)sess->buffer->start;
  testrun(0 == strncmp((char *)sess->buffer->start + offset, "RTP/AVP", 7));

  testrun(NULL == ov_sdp_session_free(copy));
  testrun(NULL == ov_sdp_session_free(session));
  dict = ov_sdp_bandwidth_free(dict);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_sdp_session_from_json() {

  char *string = "v=0\\r\\n"
                 "o=u 1 2 n a x\\r\\n"
                 "s=n\\r\\n"
                 "t=0 0\\r\\n";

  ov_json_value *obj = NULL;
  ov_json_value *val = ov_json_string(string);
  testrun(val);

  obj = ov_json_object();
  testrun(ov_json_object_set(obj, OV_KEY_SDP, val));

  testrun(NULL == ov_sdp_session_from_json(NULL));

  ov_sdp_session *session = ov_sdp_session_from_json(val);
  testrun(session);
  session = ov_sdp_session_free(session);

  session = ov_sdp_session_from_json(obj);
  testrun(session);
  session = ov_sdp_session_free(session);

  obj = ov_json_value_free(obj);
  obj = ov_json_object();
  testrun(NULL == ov_sdp_session_from_json(obj));
  obj = ov_json_value_free(obj);

  string = "v=0\\r\\n"
           "o=- 0 0 IN IP4 0.0.0.0\\r\\n"
           "s=-\\r\\n"
           "t=0 0\\r\\n"
           "m=audio 0 UDP/TLS/RTP/SAVPF 100\\r\\n"
           "a=rtpmap:100 opus/48000/2\\r\\n"
           "a=fmtp:100 maxplaybackrate=48000;useinbandfec=1\\r\\n";

  val = ov_json_string(string);
  testrun(val);

  session = ov_sdp_session_from_json(val);
  testrun(session);
  session = ov_sdp_session_free(session);
  val = ov_json_value_free(val);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_sdp_session_to_json() {

  char *string = "v=0\\r\\n"
                 "o=u 1 2 n a x\\r\\n"
                 "s=n\\r\\n"
                 "t=0 0\\r\\n";

  ov_json_value *val = ov_json_string(string);
  testrun(val);

  ov_sdp_session *session = ov_sdp_session_from_json(val);
  testrun(session);

  ov_json_value *out = ov_sdp_session_to_json(session);
  testrun(out);
  testrun(0 == strncmp(ov_json_string_get(out), string, strlen(string)));

  testrun(NULL == ov_sdp_session_to_json(NULL));

  // invalid session
  session->time->start = 1;
  testrun(NULL == ov_sdp_session_to_json(session));

  session = ov_sdp_session_free(session);
  val = ov_json_value_free(val);
  out = ov_json_value_free(out);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int check_create_masked_list() {

  ov_sdp_list *list = NULL;

  char *next = NULL;
  char *string = "e=a@mail\r\n"
                 "e=b@mail\r\n"
                 "e=c@mail\r\n"
                 "e=d@mail\r\n"
                 "e=e@mail\r\n"
                 "e=f@mail\r\n"
                 "x=abc\r\n";

  ov_buffer *buffer = ov_buffer_from_string(string);
  testrun(buffer);
  testrun(buffer->length == strlen(string));

  testrun(create_masked_list(&list, (char *)buffer->start, buffer->length,
                             &next, 'e', ov_sdp_is_email));

  testrun(list);
  testrun(6 == ov_node_count(list));
  testrun(next);
  testrun('x' == next[0]);

  buffer = ov_buffer_free(buffer);
  list = ov_sdp_list_free(list);

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

  testrun_test(check_create_masked_list);

  testrun_test(test_ov_sdp_validate);

  testrun_test(check_time);
  testrun_test(check_attribute);
  testrun_test(check_media_descriptions);
  testrun_test(check_parse_number);

  testrun_test(test_ov_sdp_session_create);

  testrun_test(test_ov_sdp_parse);

  testrun_test(test_ov_sdp_stringify);

  testrun_test(test_ov_sdp_enable_caching);
  testrun_test(test_ov_sdp_disable_caching);

  testrun_test(test_ov_sdp_buffer_create);
  testrun_test(test_ov_sdp_buffer_free);

  testrun_test(test_ov_sdp_description_create);
  testrun_test(test_ov_sdp_description_free);

  testrun_test(test_ov_sdp_bandwidth_create);
  testrun_test(test_ov_sdp_bandwidth_free);

  testrun_test(test_ov_sdp_list_create);
  testrun_test(test_ov_sdp_list_free);

  testrun_test(test_ov_sdp_time_create);
  testrun_test(test_ov_sdp_time_free);

  testrun_test(test_ov_sdp_connection_create);
  testrun_test(test_ov_sdp_connection_free);

  testrun_test(test_ov_sdp_session_cast);
  testrun_test(test_ov_sdp_session_clear);
  testrun_test(test_ov_sdp_session_free);
  testrun_test(test_ov_sdp_session_persist);
  testrun_test(test_ov_sdp_session_copy);

  testrun_test(test_ov_sdp_session_from_json);
  testrun_test(test_ov_sdp_session_to_json);

  testrun_test(test_ov_sdp_to_json);

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
