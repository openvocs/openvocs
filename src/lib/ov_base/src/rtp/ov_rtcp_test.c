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

#include "../../include/ov_string.h"
#include "ov_rtcp.c"
#include <math.h>
#include <ov_test/ov_test.h>

/*----------------------------------------------------------------------------*/

static int test_ov_rtcp_message_free() {

  testrun(0 == ov_rtcp_message_free(0));

  ov_rtcp_message *sdes = ov_rtcp_message_sdes("aquaplaning", 123);
  testrun(0 != sdes);

  sdes = ov_rtcp_message_free(sdes);
  testrun(0 == sdes);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static bool check_message(ov_rtcp_message const *msg, ov_rtcp_type type,
                          uint8_t const *original_in, size_t original_len,
                          uint8_t const *remainder, size_t remainder_len) {

  const size_t msglen = ov_rtcp_message_len_octets(msg);

  if (remainder_len + msglen != original_len) {

    testrun_log_error(
        "Lengths do not match, RTCP message length is %zu, but consumed "
        "length is %zu(original data length was %zu, remaining length is "
        "%zu) ",
        msglen, original_len - remainder_len, original_len, remainder_len);

    return false;

  } else if (original_in > remainder) {

    testrun_log_error(
        "Data pointer %p violates expection, should be bigger than "
        "original pointer %p, but is not",
        remainder, original_in);
    return false;

  } else if (msglen + original_in != remainder) {

    testrun_log_error(
        "Data pointer %p violates expectation - should be %zu apart from "
        "%p, but is only %zu apart",
        remainder, msglen, original_in, remainder - original_in);
    return false;

  } else if (type != ov_rtcp_message_type(msg)) {
    testrun_log_error("RTCP message has wrong type, Expected %s, got %s",
                      ov_rtcp_type_to_string(type),
                      ov_rtcp_type_to_string(ov_rtcp_message_type(msg)));
    return false;
  } else {
    return true;
  }
}

/*----------------------------------------------------------------------------*/

static bool next_message_is(uint8_t const **in, size_t *in_len,
                            ov_rtcp_type type, ov_rtcp_message **target) {

  size_t olen = *in_len;
  uint8_t const *oin = *in;

  ov_rtcp_message *msg = ov_rtcp_message_decode(in, in_len);

  if (0 == msg) {

    testrun_log_error("Could not parse to RTCP message");
    return false;

  } else if (!check_message(msg, type, oin, olen, *in, *in_len)) {

    msg = ov_rtcp_message_free(msg);
    return false;

  } else if (0 != target) {

    *target = msg;
    return true;

  } else {

    msg = ov_rtcp_message_free(msg);
    return true;
  }
}

/*----------------------------------------------------------------------------*/

static int test_ov_rtcp_message_decode() {

  testrun(0 == ov_rtcp_message_decode(0, 0));

  uint8_t sdes_message_raw[] = {
      0x81, 0xca, 0x00, 0x20, 0x5a, 0xd5, 0xb6, 0x7d, 0x01, 0x18, 0x73, 0x69,
      0x70, 0x3a, 0x34, 0x31, 0x30, 0x31, 0x40, 0x31, 0x39, 0x32, 0x2e, 0x31,
      0x36, 0x38, 0x2e, 0x31, 0x30, 0x30, 0x2e, 0x32, 0x30, 0x35, 0x06, 0x5d,
      0x4c, 0x69, 0x6e, 0x70, 0x68, 0x6f, 0x6e, 0x65, 0x20, 0x44, 0x65, 0x73,
      0x6b, 0x74, 0x6f, 0x70, 0x2f, 0x34, 0x2e, 0x34, 0x2e, 0x31, 0x30, 0x20,
      0x28, 0x6d, 0x75, 0x73, 0x70, 0x65, 0x6c, 0x29, 0x20, 0x44, 0x65, 0x62,
      0x69, 0x61, 0x6e, 0x20, 0x47, 0x4e, 0x55, 0x2f, 0x4c, 0x69, 0x6e, 0x75,
      0x78, 0x20, 0x62, 0x6f, 0x6f, 0x6b, 0x77, 0x6f, 0x72, 0x6d, 0x2f, 0x73,
      0x69, 0x64, 0x2c, 0x20, 0x51, 0x74, 0x20, 0x35, 0x2e, 0x31, 0x35, 0x2e,
      0x38, 0x20, 0x4c, 0x69, 0x6e, 0x70, 0x68, 0x6f, 0x6e, 0x65, 0x43, 0x6f,
      0x72, 0x65, 0x2f, 0x35, 0x2e, 0x31, 0x2e, 0x36, 0x35, 0x00, 0x00, 0x00};

  uint8_t const *in = sdes_message_raw;
  size_t in_len = sizeof(sdes_message_raw);

  ov_rtcp_message *msg = 0;

  testrun(next_message_is(&in, &in_len, OV_RTCP_SOURCE_DESC, &msg));

  testrun(0x5ad5b67d == ov_rtcp_message_sdes_ssrc(msg, 0));

  testrun(0 == ov_string_compare(ov_rtcp_message_sdes_cname(msg, 0),
                                 "sip:4101@192.168.100.205"));

  msg = ov_rtcp_message_free(msg);
  testrun(0 == msg);

  uint8_t sender_report[] = {0x80, 0xc8, 0x00, 0x06, 0x5a, 0xd5, 0xb6,
                             0x7d, 0xe7, 0xa1, 0xa2, 0xa0, 0xd8, 0x21,
                             0xc0, 0x44, 0x57, 0x49, 0x68, 0xf6, 0x00,
                             0x00, 0x00, 0x48, 0x00, 0x00, 0x2d, 0x00};

  in = sender_report;
  in_len = sizeof(sender_report);

  testrun(next_message_is(&in, &in_len, OV_RTCP_SENDER_REPORT, 0));

  uint8_t combined_sender_report_sdes[] = {
      0x80, 0xc8, 0x00, 0x06, 0x5a, 0xd5, 0xb6, 0x7d, 0xe7, 0xa1, 0xa2, 0xa0,
      0xd8, 0x21, 0xc0, 0x44, 0x57, 0x49, 0x68, 0xf6, 0x00, 0x00, 0x00, 0x48,
      0x00, 0x00, 0x2d, 0x00, 0x81, 0xca, 0x00, 0x20, 0x5a, 0xd5, 0xb6, 0x7d,
      0x01, 0x18, 0x73, 0x69, 0x70, 0x3a, 0x34, 0x31, 0x30, 0x31, 0x40, 0x31,
      0x39, 0x32, 0x2e, 0x31, 0x36, 0x38, 0x2e, 0x31, 0x30, 0x30, 0x2e, 0x32,
      0x30, 0x35, 0x06, 0x5d, 0x4c, 0x69, 0x6e, 0x70, 0x68, 0x6f, 0x6e, 0x65,
      0x20, 0x44, 0x65, 0x73, 0x6b, 0x74, 0x6f, 0x70, 0x2f, 0x34, 0x2e, 0x34,
      0x2e, 0x31, 0x30, 0x20, 0x28, 0x6d, 0x75, 0x73, 0x70, 0x65, 0x6c, 0x29,
      0x20, 0x44, 0x65, 0x62, 0x69, 0x61, 0x6e, 0x20, 0x47, 0x4e, 0x55, 0x2f,
      0x4c, 0x69, 0x6e, 0x75, 0x78, 0x20, 0x62, 0x6f, 0x6f, 0x6b, 0x77, 0x6f,
      0x72, 0x6d, 0x2f, 0x73, 0x69, 0x64, 0x2c, 0x20, 0x51, 0x74, 0x20, 0x35,
      0x2e, 0x31, 0x35, 0x2e, 0x38, 0x20, 0x4c, 0x69, 0x6e, 0x70, 0x68, 0x6f,
      0x6e, 0x65, 0x43, 0x6f, 0x72, 0x65, 0x2f, 0x35, 0x2e, 0x31, 0x2e, 0x36,
      0x35, 0x00, 0x00, 0x00};

  in = combined_sender_report_sdes;
  in_len = sizeof(combined_sender_report_sdes);

  testrun(next_message_is(&in, &in_len, OV_RTCP_SENDER_REPORT, 0));
  testrun(next_message_is(&in, &in_len, OV_RTCP_SOURCE_DESC, &msg));

  testrun(0x5ad5b67d == ov_rtcp_message_sdes_ssrc(msg, 0));

  testrun(0 == ov_string_compare(ov_rtcp_message_sdes_cname(msg, 0),
                                 "sip:4101@192.168.100.205"));

  msg = ov_rtcp_message_free(msg);
  testrun(0 == msg);

  return testrun_log_success();
}

/*****************************************************************************
                                     encode
 ****************************************************************************/

static bool sdes_messages_equal(ov_rtcp_message const *m1,
                                ov_rtcp_message const *m2) {

  uint32_t ssrc1 = ov_rtcp_message_sdes_ssrc(m1, 0);
  uint32_t ssrc2 = ov_rtcp_message_sdes_ssrc(m2, 0);

  char const *cname1 = ov_rtcp_message_sdes_cname(m1, 0);
  char const *cname2 = ov_rtcp_message_sdes_cname(m2, 0);

  if (OV_RTCP_SOURCE_DESC != ov_rtcp_message_type(m1)) {

    testrun_log_error("Cannot compare: Message 1 Not a SDES message");
    return false;

  } else if (OV_RTCP_SOURCE_DESC != ov_rtcp_message_type(m2)) {

    testrun_log_error("Cannot compare: Message 2 Not a SDES message");
    return false;

  } else if (ssrc1 != ssrc2) {

    testrun_log_error("Messages differ: Message 1 has SSRC %" PRIu32
                      " Message 2 has SSRC %" PRIu32,
                      ssrc1, ssrc2);
    return false;

  } else if (0 != ov_string_compare(cname1, cname2)) {

    testrun_log_error("Messages differ: Message 1 has CNAME %s"
                      " Message 2 has CNAME %s",
                      ov_string_sanitize(cname1), ov_string_sanitize(cname2));

    return false;

  } else {

    return true;
  }
}

/*----------------------------------------------------------------------------*/

static bool messages_equal(ov_rtcp_message const *m1,
                           ov_rtcp_message const *m2) {

  if (0 == m1) {
    testrun_log_error("Messages do not equal - message 1 is 0");
    return false;
  } else if (0 == m2) {
    testrun_log_error("Messages do not equal - message 2 is 0");
    return false;
  } else {

    switch (ov_rtcp_message_type(m1)) {

    case OV_RTCP_SOURCE_DESC:

      return sdes_messages_equal(m1, m2);

    default:

      if (ov_rtcp_message_type(m1) != ov_rtcp_message_type(m2)) {

        testrun_log_error("Messages do not equal - types differ: %s vs %s",
                          ov_rtcp_type_to_string(ov_rtcp_message_type(m1)),
                          ov_rtcp_type_to_string(ov_rtcp_message_type(m2)));

        return false;

      } else {

        return true;
      }
      return false;
    };
  }
}

/*----------------------------------------------------------------------------*/

static bool check_encoding(ov_rtcp_message const *msg) {

  ov_buffer *encoded = ov_rtcp_message_encode(msg);

  if (0 == encoded) {

    testrun_log_error("Could not encode message");
    return false;

  } else {

    uint8_t const *edata = encoded->start;
    size_t len = encoded->length;

    ov_rtcp_message *dmsg = ov_rtcp_message_decode(&edata, &len);
    encoded = ov_buffer_free(encoded);

    bool ok = messages_equal(msg, dmsg);

    dmsg = ov_rtcp_message_free(dmsg);

    if (0 != dmsg) {

      testrun_log_error("Could not free message");
      return false;

    } else {

      return ok;
    }
  }
}

/*----------------------------------------------------------------------------*/

static int test_ov_rtcp_message_encode() {

  testrun(0 == ov_rtcp_message_encode(0));

  ov_rtcp_message *msg = ov_rtcp_message_sdes("alberg", 4122);
  testrun(0 != msg);

  testrun(16 == ov_rtcp_message_len_octets(msg));

  testrun(check_encoding(msg));

  msg = ov_rtcp_message_free(msg);
  testrun(0 == msg);

  msg = ov_rtcp_message_sdes("albergt", 1922222);
  testrun(0 != msg);

  testrun(20 == ov_rtcp_message_len_octets(msg));

  testrun(check_encoding(msg));

  ov_rtcp_message *msg2 = ov_rtcp_message_sdes("belbert", 3);
  testrun(0 != msg2);

  ov_buffer *buf = ov_rtcp_message_encode(msg, msg2);
  testrun(0 != buf);

  uint8_t const *read_ptr = buf->start;
  size_t len = buf->length;

  ov_rtcp_message *dmsg = ov_rtcp_message_decode(&read_ptr, &len);
  testrun(0 != dmsg);

  testrun(messages_equal(msg, dmsg));
  dmsg = ov_rtcp_message_free(dmsg);

  msg = ov_rtcp_message_free(msg);
  testrun(0 == msg);

  dmsg = ov_rtcp_message_decode(&read_ptr, &len);
  testrun(0 != dmsg);

  testrun(messages_equal(msg2, dmsg));
  dmsg = ov_rtcp_message_free(dmsg);

  msg2 = ov_rtcp_message_free(msg2);
  testrun(0 == msg2);

  testrun(read_ptr == buf->start + buf->length);
  testrun(0 == len);

  buf = ov_buffer_free(buf);
  testrun(0 == buf);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_rtcp_message_sdes() {

  ov_rtcp_message *msg = ov_rtcp_message_sdes(0, 0);

  testrun(0 != msg);
  testrun(0 == ov_rtcp_message_sdes_ssrc(msg, 0));
  testrun(0 == ov_rtcp_message_sdes_ssrc(msg, 1));
  testrun(0 == ov_rtcp_message_sdes_cname(msg, 0));
  testrun(0 == ov_rtcp_message_sdes_cname(msg, 1));

  msg = ov_rtcp_message_free(msg);

  msg = ov_rtcp_message_sdes("rUdOlF", 31);
  testrun(0 != msg);
  testrun(0 == ov_string_compare("rUdOlF", ov_rtcp_message_sdes_cname(msg, 0)));
  testrun(31 == ov_rtcp_message_sdes_ssrc(msg, 0));
  testrun(16 == ov_rtcp_message_len_octets(msg));

  testrun(0 == ov_rtcp_message_sdes_ssrc(msg, 1));
  testrun(0 == ov_rtcp_message_sdes_cname(msg, 1));

  testrun(16 == ov_rtcp_message_len_octets(msg));

  msg = ov_rtcp_message_free(msg);
  testrun(0 == msg);

  msg = ov_rtcp_message_sdes("rUodOlF", 32);
  testrun(0 != msg);
  testrun(0 ==
          ov_string_compare("rUodOlF", ov_rtcp_message_sdes_cname(msg, 0)));
  testrun(32 == ov_rtcp_message_sdes_ssrc(msg, 0));

  testrun(0 == ov_rtcp_message_sdes_ssrc(msg, 1));
  testrun(0 == ov_rtcp_message_sdes_cname(msg, 1));

  testrun(20 == ov_rtcp_message_len_octets(msg));

  msg = ov_rtcp_message_free(msg);
  testrun(0 == msg);

  // RTCP SDES message from 'the wild'

  uint8_t const rtcp_sdes_message_from_the_wild[] = {
      0x81, 0xc8, 0x00, 0x0c, 0x63, 0x9e, 0xda, 0x7b, 0xe8, 0x40, 0x07, 0x5b,
      0x1c, 0xc5, 0x64, 0x80, 0x00, 0x0e, 0x7e, 0xd0, 0x00, 0x00, 0x03, 0xde,
      0x00, 0x01, 0xb0, 0x52, 0x07, 0x5b, 0xcd, 0x15, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x1c, 0x28, 0x00, 0x00, 0x00, 0x2e, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x81, 0xca, 0x00, 0x0b, 0x63, 0x9e, 0xda, 0x7b,
      0x01, 0x24, 0x34, 0x65, 0x34, 0x35, 0x33, 0x39, 0x33, 0x33, 0x2d, 0x65,
      0x38, 0x33, 0x62, 0x2d, 0x34, 0x65, 0x35, 0x64, 0x2d, 0x62, 0x36, 0x61,
      0x34, 0x2d, 0x34, 0x65, 0x33, 0x36, 0x32, 0x36, 0x31, 0x64, 0x35, 0x32,
      0x39, 0x35, 0x00, 0x00};

  uint8_t const *buf = rtcp_sdes_message_from_the_wild;
  size_t len = sizeof(rtcp_sdes_message_from_the_wild);

  msg = ov_rtcp_message_decode(&buf, &len);

  testrun(0 != msg);
  testrun(OV_RTCP_SENDER_REPORT == ov_rtcp_message_type(msg));

  msg = ov_rtcp_message_free(msg);

  msg = ov_rtcp_message_decode(&buf, &len);

  testrun(0 != msg);
  testrun(OV_RTCP_SOURCE_DESC == ov_rtcp_message_type(msg));
  testrun(0x639eda7b == ov_rtcp_message_sdes_ssrc(msg, 0));

  msg = ov_rtcp_message_free(msg);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_rtcp_message_sdes_cname() {

  for (size_t i = 0; i < 1000; ++i) {
    testrun(0 == ov_rtcp_message_sdes_cname(0, i));
  }

  ov_rtcp_message *msg = ov_rtcp_message_sdes("abc", 132);
  testrun(0 != msg);

  testrun(0 == ov_string_compare("abc", ov_rtcp_message_sdes_cname(msg, 0)));

  for (size_t i = 1; i < 1000; ++i) {
    testrun(0 == ov_rtcp_message_sdes_cname(msg, i));
  }

  msg = ov_rtcp_message_free(msg);
  testrun(0 == msg);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_rtcp_message_sdes_ssrc() {

  for (size_t i = 0; i < 1000; ++i) {
    testrun(0 == ov_rtcp_message_sdes_ssrc(0, i));
  }

  ov_rtcp_message *msg = ov_rtcp_message_sdes("abc", 132);
  testrun(0 != msg);

  testrun(132 == ov_rtcp_message_sdes_ssrc(msg, 0));

  for (size_t i = 1; i < 1000; ++i) {
    testrun(0 == ov_rtcp_message_sdes_ssrc(msg, i));
  }

  msg = ov_rtcp_message_free(msg);
  testrun(0 == msg);

  return testrun_log_success();
}
/*----------------------------------------------------------------------------*/

OV_TEST_RUN("ov_rtpc", test_ov_rtcp_message_free, test_ov_rtcp_message_decode,
            test_ov_rtcp_message_encode, test_ov_rtcp_message_sdes,
            test_ov_rtcp_message_sdes_cname, test_ov_rtcp_message_sdes_ssrc);
