/***
        ------------------------------------------------------------------------

        Copyright (c) 2022 German Aerospace Center DLR e.V. (GSOC)

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

#include <ov_base/ov_registered_cache.h>
#include <ov_test/ov_test.h>

#include "ov_sip_serde.c"

/*----------------------------------------------------------------------------*/

static bool count_headers_func(char const *name, char const *value, void *add) {

  ov_log_info("Saw header %s:%s", name, value);

  size_t *num = add;
  ++(*num);

  return true;
}

/*----------------------------------------------------------------------------*/

static size_t count_headers(ov_sip_message const *self) {

  size_t num_headers = 0;
  ov_sip_message_header_for_each(self, count_headers_func, &num_headers);
  return num_headers;
}

/*----------------------------------------------------------------------------*/

static bool header_there(ov_sip_message const *msg, char const *header,
                         char const *value) {

  char const *hval = ov_sip_message_header(msg, header);

  if (0 == hval) {
    return 0 == value;
  } else {
    return (0 == ov_string_compare(value, hval));
  }
}

/*----------------------------------------------------------------------------*/

static ov_serde_state sip_serde_add(ov_serde *serde, char const *str) {

  ov_buffer *buf = ov_buffer_from_string(str);
  ov_serde_state state = ov_sip_serde_add_raw(serde, buf);
  ov_buffer_free(buf);

  return state;
}

/*----------------------------------------------------------------------------*/

static int test_impl_add_raw() {

  ov_serde *serde = ov_sip_serde();
  testrun(0 != serde);

  testrun(OV_SERDE_ERROR == ov_sip_serde_add_raw(serde, 0));

  ov_buffer *buf = ov_buffer_from_string("Never_to_appear");
  testrun(OV_SERDE_ERROR == ov_sip_serde_add_raw(0, buf));
  buf = ov_buffer_free(buf);

  ov_result res = {0};

  testrun(OV_SERDE_PROGRESS == sip_serde_add(serde, "Abr"));
  testrun(0 == ov_sip_serde_pop_datum(serde, &res));

  testrun(OV_SERDE_PROGRESS ==
          sip_serde_add(serde, " habemus.nuntium SIP/2.0" CRLF));
  testrun(0 == ov_sip_serde_pop_datum(serde, &res));

  testrun(OV_SERDE_END == sip_serde_add(serde, CRLF));
  ov_sip_message *msg = ov_sip_serde_pop_datum(serde, &res);
  testrun(0 != msg);

  testrun(OV_SIP_REQUEST == ov_sip_message_type_get(msg));
  testrun(0 == ov_string_compare("Abr", ov_sip_message_method(msg)));
  testrun(0 == ov_string_compare("habemus.nuntium", ov_sip_message_uri(msg)));

  msg = ov_sip_message_free(msg);
  testrun(0 == msg);

  // With some headers...

  testrun(OV_SERDE_PROGRESS == sip_serde_add(serde, "PAUPAU"));
  testrun(0 == ov_sip_serde_pop_datum(serde, &res));

  testrun(OV_SERDE_PROGRESS ==
          sip_serde_add(serde, "PAU sacrebleu.moulin.rou"));

  testrun(OV_SERDE_PROGRESS == sip_serde_add(serde, "ge SIP/2.0"));
  testrun(0 == ov_sip_serde_pop_datum(serde, &res));

  testrun(OV_SERDE_PROGRESS == sip_serde_add(serde, "\r"));
  testrun(OV_SERDE_PROGRESS == sip_serde_add(serde, "\nAbraka:dabra" CRLF));

  testrun(0 == ov_sip_serde_pop_datum(serde, &res));

  testrun(OV_SERDE_END == sip_serde_add(serde, CRLF));

  msg = ov_sip_serde_pop_datum(serde, 0);

  testrun(0 == ov_sip_serde_pop_datum(serde, &res));

  testrun(0 != msg);
  testrun(2 == count_headers(msg));
  testrun(header_there(msg, "Abraka", "dabra"));
  testrun(header_there(msg, "Content-Length", "0"));

  msg = ov_sip_message_free(msg);
  testrun(0 == msg);

  testrun(OV_SERDE_PROGRESS == sip_serde_add(serde, "BY"));
  testrun(0 == ov_sip_serde_pop_datum(serde, &res));

  testrun(OV_SERDE_PROGRESS == sip_serde_add(serde, "E "));
  testrun(0 == ov_sip_serde_pop_datum(serde, 0));

  testrun(OV_SERDE_PROGRESS == sip_serde_add(serde, "HTTPS://asb"));
  testrun(0 == ov_sip_serde_pop_datum(serde, &res));

  testrun(OV_SERDE_PROGRESS == sip_serde_add(serde, "HTTPS://yrg"));
  testrun(0 == ov_sip_serde_pop_datum(serde, &res));

  testrun(OV_SERDE_PROGRESS == sip_serde_add(serde, "i " SIP_VERSION));
  testrun(0 == ov_sip_serde_pop_datum(serde, &res));

  testrun(OV_SERDE_PROGRESS == sip_serde_add(serde, CRLF "Content-Length:"));
  testrun(0 == ov_sip_serde_pop_datum(serde, &res));

  testrun(OV_SERDE_PROGRESS == sip_serde_add(serde, "0" CRLF "vi"));
  testrun(0 == ov_sip_serde_pop_datum(serde, 0));

  testrun(OV_SERDE_PROGRESS == sip_serde_add(serde, "a:thorsmjoerk" CRLF));
  testrun(0 == ov_sip_serde_pop_datum(serde, 0));

  testrun(OV_SERDE_END == sip_serde_add(serde, CRLF));
  msg = ov_sip_serde_pop_datum(serde, 0);

  testrun(0 == ov_sip_serde_pop_datum(serde, 0));

  testrun(2 == count_headers(msg));
  testrun(header_there(msg, "Content-Length", "0"));
  testrun(header_there(msg, "via", "thorsmjoerk"));

  testrun(0 == ov_sip_message_body(msg));

  msg = ov_sip_message_free(msg);
  testrun(0 == msg);

  // Space in message
  testrun(OV_SERDE_PROGRESS == sip_serde_add(serde, "BYE urgend."));
  testrun(OV_SERDE_ERROR == sip_serde_add(serde, "a.be SIP/ 2.0" CRLF));

  ov_serde_clear_buffer(serde);

  // Leading CRLF should be ignored
  testrun(OV_SERDE_PROGRESS == sip_serde_add(serde, CRLF CRLF));
  testrun(OV_SERDE_PROGRESS == sip_serde_add(serde, CRLF));
  testrun(OV_SERDE_PROGRESS == sip_serde_add(serde, "BYE urgend."));
  testrun(OV_SERDE_PROGRESS == sip_serde_add(serde, "a.be"));
  testrun(OV_SERDE_PROGRESS == sip_serde_add(serde, " SIP/"));
  testrun(OV_SERDE_PROGRESS == sip_serde_add(serde, "2.0" CRLF));

  // Trailing data should not confuse parser.
  testrun(OV_SERDE_END == sip_serde_add(serde, CRLF "REGISTER blobb.db "));

  // There is a message ready, but even more to come

  msg = ov_sip_serde_pop_datum(serde, &res);
  testrun(0 != msg);

  testrun(0 == ov_string_compare("BYE", ov_sip_message_method(msg)));
  testrun(0 == ov_string_compare("urgend.a.be", ov_sip_message_uri(msg)));

  msg = ov_sip_message_free(msg);
  testrun(0 == msg);

  testrun(0 == ov_sip_serde_pop_datum(serde, &res));

  testrun(OV_SERDE_PROGRESS == sip_serde_add(serde, SIP_VERSION CRLF));
  testrun(OV_SERDE_PROGRESS ==
          sip_serde_add(serde, OV_SIP_HEADER_CONTENT_LENGTH ":12" CRLF));
  testrun(OV_SERDE_PROGRESS ==
          sip_serde_add(serde, OV_SIP_HEADER_CONTENT_TYPE ":argon" CRLF));
  testrun(OV_SERDE_PROGRESS ==
          sip_serde_add(serde, OV_SIP_HEADER_CONTENT_TYPE ":argon"));

  testrun(OV_SERDE_END ==
          sip_serde_add(serde, CRLF CRLF "1234567890ab" SIP_VERSION));

  msg = ov_sip_serde_pop_datum(serde, &res);

  testrun(0 == ov_sip_serde_pop_datum(serde, &res));

  testrun(0 == ov_string_compare("REGISTER", ov_sip_message_method(msg)));
  testrun(0 == ov_string_compare("blobb.db", ov_sip_message_uri(msg)));
  testrun(2 == count_headers(msg));
  testrun(header_there(msg, OV_SIP_HEADER_CONTENT_TYPE, "argon"));
  testrun(header_there(msg, OV_SIP_HEADER_CONTENT_LENGTH, "12"));

  ov_buffer const *body = ov_sip_message_body(msg);

  testrun(12 == body->length);
  testrun(0 == memcmp(body->start, "1234567890ab", body->length));

  msg = ov_sip_message_free(msg);
  testrun(0 == msg);

  body = 0;

  testrun(OV_SERDE_END ==
          sip_serde_add(serde, " 200 OK" CRLF OV_SIP_HEADER_CONTENT_LENGTH
                               ":2" CRLF OV_SIP_HEADER_CONTENT_TYPE
                               ":bor" CRLF CRLF "ZZ" SIP_VERSION
                               " 210 OK-OK" CRLF OV_SIP_HEADER_CONTENT_LENGTH
                               ":1" CRLF OV_SIP_HEADER_CONTENT_TYPE
                               ":caesium" CRLF CRLF "Y"));

  msg = ov_sip_serde_pop_datum(serde, 0);

  testrun(200 == ov_sip_message_response_code(msg));
  testrun(0 == ov_string_compare("OK", ov_sip_message_response_reason(msg)));

  testrun(2 == count_headers(msg));
  testrun(header_there(msg, OV_SIP_HEADER_CONTENT_TYPE, "bor"));
  testrun(header_there(msg, OV_SIP_HEADER_CONTENT_LENGTH, "2"));

  body = ov_sip_message_body(msg);

  testrun(2 == body->length);
  testrun(0 == memcmp(body->start, "ZZ", body->length));

  msg = ov_sip_message_free(msg);
  testrun(0 == msg);

  // Second message

  msg = ov_sip_serde_pop_datum(serde, 0);
  testrun(0 == ov_sip_serde_pop_datum(serde, &res));

  testrun(210 == ov_sip_message_response_code(msg));
  testrun(0 == ov_string_compare("OK-OK", ov_sip_message_response_reason(msg)));

  testrun(2 == count_headers(msg));
  testrun(header_there(msg, OV_SIP_HEADER_CONTENT_TYPE, "caesium"));
  testrun(header_there(msg, OV_SIP_HEADER_CONTENT_LENGTH, "1"));

  body = ov_sip_message_body(msg);

  testrun(1 == body->length);
  testrun(0 == memcmp(body->start, "Y", body->length));

  msg = ov_sip_message_free(msg);
  testrun(0 == msg);

  body = 0;

  serde = ov_serde_free(serde);
  testrun(0 == serde);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_sip_message_to_string() {

  ov_serde *serde = ov_sip_serde();
  testrun(0 != serde);

  ov_sip_message *msg = ov_sip_message_request_create("Abra", "nuuubi");
  testrun(0 != msg);

  testrun(0 == ov_sip_message_to_string(0, 0));
  testrun(0 == ov_sip_message_to_string(serde, 0));
  testrun(0 == ov_sip_message_to_string(0, msg));

  char *str = ov_sip_message_to_string(serde, msg);

  testrun(0 == ov_string_compare(str,
                                 "Abra nuuubi "
                                 "SIP/2.0" CRLF OV_SIP_HEADER_CONTENT_LENGTH
                                 ": 0" CRLF CRLF));
  free(str);

  msg = ov_sip_message_free(msg);
  testrun(0 == msg);

  msg = ov_sip_message_response_create(120, "I don't know");
  testrun(0 != msg);

  str = ov_sip_message_to_string(serde, msg);

  testrun(0 == ov_string_compare(str, "SIP/2.0 120 I don't "
                                      "know" CRLF OV_SIP_HEADER_CONTENT_LENGTH
                                      ": 0" CRLF CRLF));
  free(str);

  msg = ov_sip_message_free(msg);
  testrun(0 == msg);

  msg = ov_sip_message_request_create("INVITE", "sip://censored.cpu");
  testrun(ov_sip_message_header_set(msg, "Blur", "123"));
  testrun(ov_sip_message_header_set(msg, "Croak", "456"));

  str = ov_sip_message_to_string(serde, msg);

  testrun(0 == ov_string_compare(
                   str, "INVITE sip://censored.cpu " SIP_VERSION CRLF "Croak: "
                        "456" CRLF OV_SIP_HEADER_CONTENT_LENGTH ": 0" CRLF
                        "Blur: 123" CRLF CRLF));

  free(str);

  ov_buffer *body = ov_buffer_from_string("Trolladynga");

  ov_sip_message_body_set(msg, body, "Haematom");

  str = ov_sip_message_to_string(serde, msg);

  testrun(0 == ov_string_compare(
                   str, "INVITE sip://censored.cpu " SIP_VERSION CRLF "Croak: "
                        "456" CRLF OV_SIP_HEADER_CONTENT_LENGTH ": 11" CRLF
                        "Blur: 123" CRLF OV_SIP_HEADER_CONTENT_TYPE
                        ": Haematom" CRLF CRLF "Trolladynga"));

  free(str);

  msg = ov_sip_message_free(msg);
  testrun(0 == msg);

  serde = ov_serde_free(serde);
  testrun(0 == serde);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_impl_clear_buffer() {

  ov_serde *serde = ov_sip_serde();

  testrun(!serde->clear_buffer(0));

  testrun(ov_serde_clear_buffer(serde));

  testrun(OV_SERDE_PROGRESS ==
          sip_serde_add(serde, SIP_VERSION
                        " 200 "
                        "OK" CRLF OV_SIP_HEADER_CONTENT_LENGTH
                        ":2" CRLF OV_SIP_HEADER_CONTENT_TYPE ":bor" CRLF CRLF));

  testrun(ov_serde_clear_buffer(serde));

  testrun(OV_SERDE_PROGRESS ==
          sip_serde_add(serde, CRLF CRLF SIP_VERSION
                        " 290 OEL" CRLF OV_SIP_HEADER_CONTENT_LENGTH
                        ":5" CRLF OV_SIP_HEADER_CONTENT_TYPE ":hiffenmar"
                        "k" CRLF CRLF));

  testrun(OV_SERDE_PROGRESS == sip_serde_add(serde, "4321AchtundfuenzigUeber"));

  ov_sip_message *msg = ov_sip_serde_pop_datum(serde, 0);

  testrun(0 == ov_sip_serde_pop_datum(serde, 0));

  serde = ov_serde_free(serde);
  testrun(0 != msg);

  testrun(290 == ov_sip_message_response_code(msg));
  testrun(0 == ov_string_compare("OEL", ov_sip_message_response_reason(msg)));

  msg = ov_sip_message_free(msg);
  testrun(0 == msg);

  serde = ov_serde_free(serde);
  testrun(0 == serde);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int tear_down() {

  ov_registered_cache_free_all();
  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

OV_TEST_RUN("ov_sip_serde", test_impl_add_raw, test_ov_sip_message_to_string,
            test_impl_clear_buffer, tear_down);
