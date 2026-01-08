/***
        ------------------------------------------------------------------------

        Copyright 2018 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_ice_string_test.c
        @author         Markus Toepfer

        @date           2018-05-28

        @ingroup        ov_ice

        @brief


        ------------------------------------------------------------------------
*/

#include "ov_ice_string.c"
#include <ov_test/testrun.h>

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

int test_ov_ice_string_is_ice_char() {

  char buffer[200] = {0};
  strcat(buffer, "abcdefghijklmnopqrstuvwxyz");
  char *start = buffer;

  testrun(!ov_ice_string_is_ice_char(NULL, strlen(buffer)));
  testrun(!ov_ice_string_is_ice_char(start, 0));

  strcat(buffer, "abcdefghijklmnopqrstuvwxyz");
  testrun(ov_ice_string_is_ice_char(start, strlen(start)));
  strcat(buffer, "0123456789");
  testrun(ov_ice_string_is_ice_char(start, strlen(start)));
  strcat(buffer, "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
  testrun(ov_ice_string_is_ice_char(start, strlen(start)));

  testrun(!ov_ice_string_is_ice_char(start, strlen(start) + 1));

  for (int i = 0; i < 0xff; i++) {

    buffer[10] = i;

    testrun(ov_ice_string_is_ice_char(start, 10));

    if (isalnum(i) || i == '+' || i == '/') {

      testrun(ov_ice_string_is_ice_char(start, 11));

    } else {

      testrun(!ov_ice_string_is_ice_char(start, 11));
    }
  }

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_ice_string_is_foundation() {

  char buffer1[200] = {0};
  char buffer2[200] = {0};
  char *string = "abcdefghijklmnopqrstuvwxyz";

  strcat(buffer1, string);
  strcat(buffer1, "+0123456789/");
  strcat(buffer1, "ABCDEFGHIJKLMNOPQRSTUVWXYZ");

  strcat(buffer2, string);
  strcat(buffer2, "-"); // invalid item
  strcat(buffer2, "ABCDEFGHIJKLMNOPQRSTUVWXYZ");

  char *valid = buffer1;
  char *invalid = buffer2;

  testrun(ov_ice_string_is_ice_char(valid, strlen(valid)));
  testrun(!ov_ice_string_is_ice_char(invalid, strlen(invalid)));
  testrun(ov_ice_string_is_ice_char(valid, strlen(string)));
  testrun(ov_ice_string_is_ice_char(invalid, strlen(string)));

  testrun(!ov_ice_string_is_foundation(string, 0));
  testrun(!ov_ice_string_is_foundation(NULL, strlen(string)));

  testrun(ov_ice_string_is_foundation(string, strlen(string)));

  testrun(ov_ice_string_is_foundation(valid, 32));
  testrun(!ov_ice_string_is_foundation(invalid, 32));
  testrun(ov_ice_string_is_foundation(valid, strlen(string)));
  testrun(ov_ice_string_is_foundation(invalid, strlen(string)));

  for (size_t i = 1; i < 0xff; i++) {

    if (i <= 32) {

      testrun(ov_ice_string_is_foundation(valid, i));

      if (i <= strlen(string)) {
        testrun(ov_ice_string_is_foundation(invalid, i));
      } else {
        testrun(!ov_ice_string_is_foundation(invalid, i));
      }

    } else {

      testrun(!ov_ice_string_is_foundation(valid, i));
      testrun(!ov_ice_string_is_foundation(invalid, i));
    }
  }

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_ice_string_is_component_id() {

  char *string = "12345";

  testrun(ov_ice_string_is_component_id(string, strlen(string)));

  testrun(!ov_ice_string_is_component_id(NULL, 0));
  testrun(!ov_ice_string_is_component_id(string, 0));
  testrun(!ov_ice_string_is_component_id(NULL, strlen(string)));

  string = "123456";
  testrun(!ov_ice_string_is_component_id(string, strlen(string)));

  string = "1";
  testrun(ov_ice_string_is_component_id(string, strlen(string)));
  string = "12";
  testrun(ov_ice_string_is_component_id(string, strlen(string)));
  string = "123";
  testrun(ov_ice_string_is_component_id(string, strlen(string)));
  string = "1234";
  testrun(ov_ice_string_is_component_id(string, strlen(string)));
  string = "99999";
  testrun(ov_ice_string_is_component_id(string, strlen(string)));
  string = "12a";
  testrun(!ov_ice_string_is_component_id(string, strlen(string)));

  string = "-12";
  testrun(!ov_ice_string_is_component_id(string, strlen(string)));
  string = "+12";
  testrun(!ov_ice_string_is_component_id(string, strlen(string)));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_ice_string_is_transport() {

  char *string = "UDP";

  testrun(ov_ice_string_is_transport(string, strlen(string)));

  testrun(!ov_ice_string_is_transport(NULL, 0));
  testrun(!ov_ice_string_is_transport(string, 0));
  testrun(!ov_ice_string_is_transport(NULL, strlen(string)));

  string = "udp";
  testrun(ov_ice_string_is_transport(string, strlen(string)));

  string = "uDp";
  testrun(ov_ice_string_is_transport(string, strlen(string)));
  string = "tcp";
  testrun(ov_ice_string_is_transport(string, strlen(string)));
  string = "adp";
  testrun(ov_ice_string_is_transport(string, strlen(string)));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_ice_string_is_transport_extension() {

  char *string = "somevalidtoken";

  testrun(ov_ice_string_is_transport_extension(string, strlen(string)));

  testrun(!ov_ice_string_is_transport_extension(NULL, 0));
  testrun(!ov_ice_string_is_transport_extension(string, 0));
  testrun(!ov_ice_string_is_transport_extension(NULL, strlen(string)));

  string = "udp";
  testrun(ov_ice_string_is_transport_extension(string, strlen(string)));

  string = "invalid\n";
  testrun(!ov_ice_string_is_transport_extension(string, strlen(string)));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_ice_string_is_priority() {

  char *string = "1234567890";

  testrun(ov_ice_string_is_priority(string, strlen(string)));

  testrun(!ov_ice_string_is_priority(NULL, 0));
  testrun(!ov_ice_string_is_priority(string, 0));
  testrun(!ov_ice_string_is_priority(NULL, strlen(string)));

  // including terminating zero
  testrun(!ov_ice_string_is_priority(string, strlen(string) + 1));

  // < 10 digits
  testrun(ov_ice_string_is_priority(string, strlen(string) - 1));
  testrun(ov_ice_string_is_priority(string, strlen(string) - 2));
  testrun(ov_ice_string_is_priority(string, strlen(string) - 3));
  testrun(ov_ice_string_is_priority(string, strlen(string) - 4));
  testrun(ov_ice_string_is_priority(string, strlen(string) - 5));
  testrun(ov_ice_string_is_priority(string, strlen(string) - 6));
  testrun(ov_ice_string_is_priority(string, strlen(string) - 7));
  testrun(ov_ice_string_is_priority(string, strlen(string) - 8));
  testrun(ov_ice_string_is_priority(string, strlen(string) - 9));

  // zero length
  testrun(!ov_ice_string_is_priority(string, strlen(string) - 10));

  // negative length
  testrun(!ov_ice_string_is_priority(string, strlen(string) - 11));

  string = "1";
  testrun(ov_ice_string_is_priority(string, strlen(string)));
  string = "12";
  testrun(ov_ice_string_is_priority(string, strlen(string)));
  string = "123";
  testrun(ov_ice_string_is_priority(string, strlen(string)));
  string = "1234";
  testrun(ov_ice_string_is_priority(string, strlen(string)));

  // > 10 digits
  string = "12345678901";
  testrun(!ov_ice_string_is_priority(string, strlen(string)));

  // non digit
  string = "12a";
  testrun(!ov_ice_string_is_priority(string, strlen(string)));

  // negative sign
  string = "-12";
  testrun(!ov_ice_string_is_priority(string, strlen(string)));

  // positive sign
  string = "+12";
  testrun(!ov_ice_string_is_priority(string, strlen(string)));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_ice_string_is_candidate_type() {

  char *string = "typ host";

  testrun(ov_ice_string_is_candidate_type(string, strlen(string)));

  testrun(!ov_ice_string_is_candidate_type(NULL, 0));
  testrun(!ov_ice_string_is_candidate_type(string, 0));
  testrun(!ov_ice_string_is_candidate_type(NULL, strlen(string)));

  // including terminating zero
  testrun(!ov_ice_string_is_candidate_type(string, strlen(string) + 1));

  // no case ignore
  string = "TYP host";
  testrun(!ov_ice_string_is_candidate_type(string, strlen(string)));
  string = "type HOST";
  testrun(!ov_ice_string_is_candidate_type(string, strlen(string)));

  string = "typ srflx";
  testrun(ov_ice_string_is_candidate_type(string, strlen(string)));
  string = "TYP SRFLX";
  testrun(!ov_ice_string_is_candidate_type(string, strlen(string)));
  testrun(!ov_ice_string_is_candidate_type(string, strlen(string) + 1));

  string = "typ prflx";
  testrun(ov_ice_string_is_candidate_type(string, strlen(string)));
  string = "TYP PRFLX";
  testrun(!ov_ice_string_is_candidate_type(string, strlen(string)));
  testrun(!ov_ice_string_is_candidate_type(string, strlen(string) + 1));

  string = "typ relay";
  testrun(ov_ice_string_is_candidate_type(string, strlen(string)));
  string = "TYP RELAY";
  testrun(!ov_ice_string_is_candidate_type(string, strlen(string)));
  testrun(!ov_ice_string_is_candidate_type(string, strlen(string) + 1));

  string = "typ token";
  testrun(ov_ice_string_is_candidate_type(string, strlen(string)));
  string = "TYP TOKEN";
  testrun(!ov_ice_string_is_candidate_type(string, strlen(string)));
  testrun(!ov_ice_string_is_candidate_type(string, strlen(string) + 1));

  string = "typ non\ntoken";
  testrun(!ov_ice_string_is_candidate_type(string, strlen(string)));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_ice_string_is_host() {

  char *string = "host";

  testrun(ov_ice_string_is_host(string, strlen(string)));

  testrun(!ov_ice_string_is_host(NULL, 0));
  testrun(!ov_ice_string_is_host(string, 0));
  testrun(!ov_ice_string_is_host(NULL, strlen(string)));

  // including terminating zero
  testrun(!ov_ice_string_is_host(string, strlen(string) + 1));

  // no case ignore
  string = "Host";
  testrun(!ov_ice_string_is_host(string, strlen(string)));
  string = "hosT";
  testrun(!ov_ice_string_is_candidate_type(string, strlen(string)));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_ice_string_is_srflx() {

  char *string = "srflx";

  testrun(ov_ice_string_is_srflx(string, strlen(string)));

  testrun(!ov_ice_string_is_srflx(NULL, 0));
  testrun(!ov_ice_string_is_srflx(string, 0));
  testrun(!ov_ice_string_is_srflx(NULL, strlen(string)));

  // including terminating zero
  testrun(!ov_ice_string_is_srflx(string, strlen(string) + 1));

  // no case ignore
  string = "Srflx";
  testrun(!ov_ice_string_is_srflx(string, strlen(string)));
  string = "srFlx";
  testrun(!ov_ice_string_is_srflx(string, strlen(string)));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_ice_string_is_prflx() {

  char *string = "prflx";

  testrun(ov_ice_string_is_prflx(string, strlen(string)));

  testrun(!ov_ice_string_is_prflx(NULL, 0));
  testrun(!ov_ice_string_is_prflx(string, 0));
  testrun(!ov_ice_string_is_prflx(NULL, strlen(string)));

  // including terminating zero
  testrun(!ov_ice_string_is_prflx(string, strlen(string) + 1));

  // no case ignore
  string = "Prflx";
  testrun(!ov_ice_string_is_prflx(string, strlen(string)));
  string = "prFlx";
  testrun(!ov_ice_string_is_prflx(string, strlen(string)));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_ice_string_is_relay() {

  char *string = "relay";

  testrun(ov_ice_string_is_relay(string, strlen(string)));

  testrun(!ov_ice_string_is_relay(NULL, 0));
  testrun(!ov_ice_string_is_relay(string, 0));
  testrun(!ov_ice_string_is_relay(NULL, strlen(string)));

  // including terminating zero
  testrun(!ov_ice_string_is_relay(string, strlen(string) + 1));

  // no case ignore
  string = "Relay";
  testrun(!ov_ice_string_is_relay(string, strlen(string)));
  string = "relAy";
  testrun(!ov_ice_string_is_relay(string, strlen(string)));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_ice_string_is_token() {

  char *string = "token";

  testrun(ov_ice_string_is_token(string, strlen(string)));

  testrun(!ov_ice_string_is_token(NULL, 0));
  testrun(!ov_ice_string_is_token(string, 0));
  testrun(!ov_ice_string_is_token(NULL, strlen(string)));

  // including terminating zero
  testrun(!ov_ice_string_is_token(string, strlen(string) + 1));

  // no token
  string = "no\ntoken";
  testrun(!ov_ice_string_is_token(string, strlen(string)));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_ice_string_is_connection_address() {

  char *string = "localhost\r\n";

  testrun(!ov_ice_string_is_connection_address(string, strlen(string)));

  string = "901234";
  testrun(ov_ice_string_is_connection_address(string, strlen(string)));

  testrun(!ov_ice_string_is_connection_address(NULL, 0));
  testrun(!ov_ice_string_is_connection_address(string, 0));
  testrun(!ov_ice_string_is_connection_address(NULL, strlen(string)));

  // including terminating zero
  testrun(!ov_ice_string_is_connection_address(string, strlen(string) + 1));

  // no connection
  string = "no\ntoken";
  testrun(!ov_ice_string_is_connection_address(string, strlen(string)));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_ice_string_is_connection_port() {

  char *string = "12345";

  testrun(ov_ice_string_is_connection_port(string, strlen(string)));

  testrun(!ov_ice_string_is_connection_port(NULL, 0));
  testrun(!ov_ice_string_is_connection_port(string, 0));
  testrun(!ov_ice_string_is_connection_port(NULL, strlen(string)));

  // including terminating zero
  testrun(!ov_ice_string_is_connection_port(string, strlen(string) + 1));

  string = "65535";
  testrun(ov_ice_string_is_connection_port(string, strlen(string)));
  string = "65536";
  testrun(!ov_ice_string_is_connection_port(string, strlen(string)));
  string = "0";
  testrun(!ov_ice_string_is_connection_port(string, strlen(string)));
  string = "1";
  testrun(ov_ice_string_is_connection_port(string, strlen(string)));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_ice_string_is_related_address() {

  char *string = "raddr 192.168.1.1";

  testrun(ov_ice_string_is_related_address(string, strlen(string)));

  testrun(!ov_ice_string_is_related_address(NULL, 0));
  testrun(!ov_ice_string_is_related_address(string, 0));
  testrun(!ov_ice_string_is_related_address(NULL, strlen(string)));

  // including terminating zero
  testrun(!ov_ice_string_is_related_address(string, strlen(string) + 1));

  // case
  string = "Raddr 192.168.1.1";
  testrun(!ov_ice_string_is_related_address(string, strlen(string)));

  // space
  string = "raddr192.168.1.1";
  testrun(!ov_ice_string_is_related_address(string, strlen(string)));

  // no address
  string = "raddr 192.168\n.1.1";
  testrun(!ov_ice_string_is_related_address(string, strlen(string)));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_ice_string_is_related_port() {

  char *string = "rport 123";

  testrun(ov_ice_string_is_related_port(string, strlen(string)));

  testrun(!ov_ice_string_is_related_port(NULL, 0));
  testrun(!ov_ice_string_is_related_port(string, 0));
  testrun(!ov_ice_string_is_related_port(NULL, strlen(string)));

  // including terminating zero
  testrun(!ov_ice_string_is_related_port(string, strlen(string) + 1));

  // case
  string = "rPort 123";
  testrun(!ov_ice_string_is_related_port(string, strlen(string)));

  // space
  string = "rport123";
  testrun(!ov_ice_string_is_related_port(string, strlen(string)));

  // no port number
  string = "rport 65536";
  testrun(!ov_ice_string_is_related_port(string, strlen(string)));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_ice_string_is_extension_name() {

  char *string = "byte 123 +-*/ string";

  testrun(ov_ice_string_is_extension_name(string, strlen(string)));

  testrun(!ov_ice_string_is_extension_name(NULL, 0));
  testrun(!ov_ice_string_is_extension_name(string, 0));
  testrun(!ov_ice_string_is_extension_name(NULL, strlen(string)));

  // including terminating zero
  testrun(!ov_ice_string_is_extension_name(string, strlen(string) + 1));

  // not a byte string
  string = "not\nvalid";
  testrun(!ov_ice_string_is_extension_name(string, strlen(string)));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_ice_string_is_extension_value() {

  char *string = "byte 123 +-*/ string";

  testrun(ov_ice_string_is_extension_value(string, strlen(string)));

  testrun(!ov_ice_string_is_extension_value(NULL, 0));
  testrun(!ov_ice_string_is_extension_value(string, 0));
  testrun(!ov_ice_string_is_extension_value(NULL, strlen(string)));

  // including terminating zero
  testrun(!ov_ice_string_is_extension_value(string, strlen(string) + 1));

  // not a byte string
  string = "not\nvalid";
  testrun(!ov_ice_string_is_extension_value(string, strlen(string)));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_ice_string_is_ice_lite_key() {

  char *string = "ice-lite";

  testrun(ov_ice_string_is_ice_lite_key(string, strlen(string)));

  testrun(!ov_ice_string_is_ice_lite_key(NULL, 0));
  testrun(!ov_ice_string_is_ice_lite_key(string, 0));
  testrun(!ov_ice_string_is_ice_lite_key(NULL, strlen(string)));

  // including terminating zero
  testrun(!ov_ice_string_is_ice_lite_key(string, strlen(string) + 1));

  // case
  string = "Ice-lite";
  testrun(!ov_ice_string_is_ice_lite_key(string, strlen(string)));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_ice_string_is_ice_mismatch_key() {

  char *string = "ice-mismatch";

  testrun(ov_ice_string_is_ice_mismatch_key(string, strlen(string)));

  testrun(!ov_ice_string_is_ice_mismatch_key(NULL, 0));
  testrun(!ov_ice_string_is_ice_mismatch_key(string, 0));
  testrun(!ov_ice_string_is_ice_mismatch_key(NULL, strlen(string)));

  // including terminating zero
  testrun(!ov_ice_string_is_ice_mismatch_key(string, strlen(string) + 1));

  // case
  string = "Ice-mismatch";
  testrun(!ov_ice_string_is_ice_mismatch_key(string, strlen(string)));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_ice_string_is_ice_pwd_key() {

  char *string = "ice-pwd";

  testrun(ov_ice_string_is_ice_pwd_key(string, strlen(string)));

  testrun(!ov_ice_string_is_ice_pwd_key(NULL, 0));
  testrun(!ov_ice_string_is_ice_pwd_key(string, 0));
  testrun(!ov_ice_string_is_ice_pwd_key(NULL, strlen(string)));

  // including terminating zero
  testrun(!ov_ice_string_is_ice_pwd_key(string, strlen(string) + 1));

  // case
  string = "Ice-pwd";
  testrun(!ov_ice_string_is_ice_pwd_key(string, strlen(string)));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_ice_string_is_ice_ufrag_key() {

  char *string = "ice-ufrag";

  testrun(ov_ice_string_is_ice_ufrag_key(string, strlen(string)));

  testrun(!ov_ice_string_is_ice_ufrag_key(NULL, 0));
  testrun(!ov_ice_string_is_ice_ufrag_key(string, 0));
  testrun(!ov_ice_string_is_ice_ufrag_key(NULL, strlen(string)));

  // including terminating zero
  testrun(!ov_ice_string_is_ice_ufrag_key(string, strlen(string) + 1));

  // case
  string = "Ice-ufrag";
  testrun(!ov_ice_string_is_ice_ufrag_key(string, strlen(string)));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_ice_string_is_ice_options_key() {

  char *string = "ice-options";

  testrun(ov_ice_string_is_ice_options_key(string, strlen(string)));

  testrun(!ov_ice_string_is_ice_options_key(NULL, 0));
  testrun(!ov_ice_string_is_ice_options_key(string, 0));
  testrun(!ov_ice_string_is_ice_options_key(NULL, strlen(string)));

  // including terminating zero
  testrun(!ov_ice_string_is_ice_options_key(string, strlen(string) + 1));

  // case
  string = "Ice-options";
  testrun(!ov_ice_string_is_ice_options_key(string, strlen(string)));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_ice_string_is_ice_pwd() {

  char buf[300] = {0};
  char *string = buf;

  for (int i = 0; i < 299; i++) {

    // set a non ice char
    buf[i] = '-';
    testrun(!ov_ice_string_is_ice_pwd(string, strlen(string)));

    // set an ice char
    buf[i] = 'x';

    if ((i < 21) || (i > 255)) {

      testrun(!ov_ice_string_is_ice_pwd(string, strlen(string)));

    } else {

      testrun(ov_ice_string_is_ice_pwd(string, strlen(string)));
    }
  }

  string = "valid";
  testrun(!ov_ice_string_is_ice_options_key(NULL, 0));
  testrun(!ov_ice_string_is_ice_options_key(string, 0));
  testrun(!ov_ice_string_is_ice_options_key(NULL, strlen(string)));

  // including terminating zero
  testrun(!ov_ice_string_is_ice_options_key(string, strlen(string) + 1));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_ice_string_is_ice_ufrag() {

  char buf[300] = {0};
  char *string = buf;

  for (int i = 0; i < 299; i++) {

    // set a non ice char
    buf[i] = '-';
    testrun(!ov_ice_string_is_ice_ufrag(string, strlen(string)));

    // set an ice char
    buf[i] = 'x';

    if ((i < 3) || (i > 255)) {

      testrun(!ov_ice_string_is_ice_ufrag(string, strlen(string)));

    } else {

      testrun(ov_ice_string_is_ice_ufrag(string, strlen(string)));
    }
  }

  string = "valid";
  testrun(!ov_ice_string_is_ice_ufrag(NULL, 0));
  testrun(!ov_ice_string_is_ice_ufrag(string, 0));
  testrun(!ov_ice_string_is_ice_ufrag(NULL, strlen(string)));

  // including terminating zero
  testrun(!ov_ice_string_is_ice_ufrag(string, strlen(string) + 1));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_ice_string_is_ice_option_tag() {

  char buf[300] = {0};
  char *string = buf;

  for (int i = 0; i < 299; i++) {

    // set a non ice char
    buf[i] = '-';
    testrun(!ov_ice_string_is_ice_option_tag(string, strlen(string)));

    // set an ice char
    buf[i] = 'x';

    testrun(ov_ice_string_is_ice_option_tag(string, strlen(string)));
  }

  string = "valid";
  testrun(!ov_ice_string_is_ice_option_tag(NULL, 0));
  testrun(!ov_ice_string_is_ice_option_tag(string, 0));
  testrun(!ov_ice_string_is_ice_option_tag(NULL, strlen(string)));

  // including terminating zero
  testrun(!ov_ice_string_is_ice_option_tag(string, strlen(string) + 1));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_ice_string_fill_random() {

  size_t size = 100;
  char buffer[size];
  memset(buffer, 0, size);

  for (size_t i = 1; i < size; i++) {

    memset(buffer, 0, size);

    testrun(ov_ice_string_fill_random(buffer, i));
    testrun(ov_ice_string_is_ice_char(buffer, i));
    testrun(buffer[i] == 0);
    testrun(strlen(buffer) == i);
  }

  char *buf = NULL;
  char *nxt = NULL;
  ov_list *list = ov_list_create((ov_list_config){0});

  for (size_t i = 1; i < 100; i++) {

    buf = calloc(size + 1, sizeof(uint8_t));
    testrun(buf);

    testrun(ov_ice_string_fill_random(buf, size));
    testrun(ov_ice_string_is_ice_char(buf, size));
    testrun(ov_list_push(list, buf));
  }

  for (size_t i = 1; i < 100; i++) {

    buf = ov_list_get(list, i);
    testrun(buf);

    for (size_t k = 1; k < i; k++) {

      nxt = ov_list_get(list, k);
      testrun(nxt);
      testrun(strlen(buf) == strlen(nxt));
      testrun(0 != strcmp(buf, nxt));
    }
  }

  buf = ov_list_pop(list);
  while (buf) {
    free(buf);
    buf = ov_list_pop(list);
  }

  testrun(NULL == ov_list_free(list));

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
  testrun_test(test_ov_ice_string_is_ice_char);
  testrun_test(test_ov_ice_string_is_foundation);
  testrun_test(test_ov_ice_string_is_component_id);
  testrun_test(test_ov_ice_string_is_transport);
  testrun_test(test_ov_ice_string_is_transport_extension);
  testrun_test(test_ov_ice_string_is_priority);

  testrun_test(test_ov_ice_string_is_candidate_type);
  testrun_test(test_ov_ice_string_is_host);
  testrun_test(test_ov_ice_string_is_srflx);
  testrun_test(test_ov_ice_string_is_prflx);
  testrun_test(test_ov_ice_string_is_relay);
  testrun_test(test_ov_ice_string_is_token);

  testrun_test(test_ov_ice_string_is_connection_address);
  testrun_test(test_ov_ice_string_is_connection_port);
  testrun_test(test_ov_ice_string_is_related_address);
  testrun_test(test_ov_ice_string_is_related_port);

  testrun_test(test_ov_ice_string_is_extension_name);
  testrun_test(test_ov_ice_string_is_extension_value);

  testrun_test(test_ov_ice_string_is_ice_lite_key);
  testrun_test(test_ov_ice_string_is_ice_mismatch_key);
  testrun_test(test_ov_ice_string_is_ice_pwd_key);
  testrun_test(test_ov_ice_string_is_ice_ufrag_key);
  testrun_test(test_ov_ice_string_is_ice_options_key);

  testrun_test(test_ov_ice_string_is_ice_pwd);
  testrun_test(test_ov_ice_string_is_ice_ufrag);
  testrun_test(test_ov_ice_string_is_ice_option_tag);

  testrun_test(test_ov_ice_string_fill_random);

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
