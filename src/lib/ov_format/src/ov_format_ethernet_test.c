/***
        ------------------------------------------------------------------------

        Copyright (c) 2020 German Aerospace Center DLR e.V. (GSOC)

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
#include "ov_format_ethernet.c"

#include <ov_test/ov_test.h>

#include "../include/ov_format_ethernet.h"
#include "../include/ov_format_ipv4.h"
#include "../include/ov_format_ipv6.h"
#include "../include/ov_format_pcap.h"
#include "../include/ov_format_udp.h"

/*----------------------------------------------------------------------------*/

char const *TEST_PCAP_FILE = "resources/pcap/test_ipv4_tcp.pcap";
char const *TEST_IP_PCAP_FILE = "resources/pcap/"
                                "test_ethernet_ipv4_ipv6_udp.pcap";

/*----------------------------------------------------------------------------*/

static const unsigned char ref_dst_mac[OV_FORMAT_ETHERNET_MAC_LEN_OCTETS] = "Fr"
                                                                            "ey"
                                                                            "r";
static const unsigned char ref_src_mac[OV_FORMAT_ETHERNET_MAC_LEN_OCTETS] = "Yn"
                                                                            "gv"
                                                                            "i";

static const uint16_t ref_ethertype = 1636;

static const unsigned char ref_payload[] = "1234567890";
static const size_t ref_payload_len = sizeof(ref_payload);

static const uint8_t ref_payload_crc32[] = {0x01, 0x35, 0xd0, 0xb2};

static const uint32_t ref_crc32 = 20304050;

static ov_buffer *get_ethernet_frame(uint8_t const *dst_mac, /* 6 octets */
                                     uint8_t const *src_mac, /* 6 octets */
                                     uint16_t ethertype, uint8_t const *payload,
                                     size_t payload_len, uint32_t crc32) {

  if (0 == dst_mac) {

    dst_mac = ref_dst_mac;
  }

  if (0 == src_mac) {

    src_mac = ref_src_mac;
  }

  if (0 == ethertype) {

    ethertype = ref_ethertype;
  }

  if (0 == payload) {
    payload = ref_payload;
    payload_len = sizeof(ref_payload);
  }

  if (0 == crc32) {

    crc32 = ref_crc32;
  }

  size_t len = OV_FORMAT_ETHERNET_MAC_LEN_OCTETS +
               OV_FORMAT_ETHERNET_MAC_LEN_OCTETS + sizeof(uint16_t) +
               payload_len + sizeof(crc32);

  ov_buffer *frame = ov_buffer_create(len);
  OV_ASSERT(len <= frame->capacity);
  frame->length = len;

  uint8_t *write_ptr = frame->start;

  memcpy(write_ptr, dst_mac, OV_FORMAT_ETHERNET_MAC_LEN_OCTETS);
  write_ptr += OV_FORMAT_ETHERNET_MAC_LEN_OCTETS;

  memcpy(write_ptr, src_mac, OV_FORMAT_ETHERNET_MAC_LEN_OCTETS);
  write_ptr += OV_FORMAT_ETHERNET_MAC_LEN_OCTETS;

  uint16_t ethertype_be = htons(ethertype);

  memcpy(write_ptr, &ethertype_be, sizeof(ethertype_be));
  write_ptr += 2;

  memcpy(write_ptr, payload, payload_len);
  write_ptr += payload_len;

  uint32_t crc32_be = htonl(crc32);

  memcpy(write_ptr, &crc32_be, sizeof(crc32_be));

  return frame;
}

/*----------------------------------------------------------------------------*/

static int test_ov_format_ethernet_install() {

  ov_buffer *frame = get_ethernet_frame(0, 0, 0, 0, 0, 0);

  ov_format *fmt = ov_format_from_memory(frame->start, frame->length, OV_READ);
  testrun(0 != fmt);

  ov_format *ethernet_fmt = ov_format_as(fmt, "ethernet", 0, 0);
  testrun(0 == ethernet_fmt);

  ov_format_ethernet_install(0);

  ethernet_fmt = ov_format_as(fmt, "ethernet", 0, 0);
  testrun(0 != ethernet_fmt);

  fmt = 0;

  ethernet_fmt = ov_format_close(ethernet_fmt);
  testrun(0 == ethernet_fmt);

  frame = ov_buffer_free(frame);
  testrun(0 == frame);

  ov_format_registry_clear(0);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_impl_next_chunk() {

  ov_buffer *ethernet_frame = get_ethernet_frame(0, 0, 0, 0, 0, 0);

  ov_format *mem_fmt = ov_format_from_memory(ethernet_frame->start,
                                             ethernet_frame->length, OV_READ);

  testrun(0 != mem_fmt);

  testrun(0 == ov_format_as(mem_fmt, "ethernet", 0, 0));

  testrun(ov_format_ethernet_install(0));

  ov_format *ethernet_fmt = ov_format_as(mem_fmt, "ethernet", 0, 0);
  testrun(0 != ethernet_fmt);

  mem_fmt = 0;

  /*************************************************************************
                             Test single frame
   ************************************************************************/

  ov_buffer buf = ov_format_payload_read_chunk_nocopy(ethernet_fmt, 0);

  testrun(buf.length == ref_payload_len);
  testrun(0 == memcmp(buf.start, ref_payload, buf.length));
  testrun(ref_crc32 == ov_format_ethernet_get_crc32(ethernet_fmt));

  ethernet_fmt = ov_format_close(ethernet_fmt);

  testrun(0 == ethernet_fmt);

  ethernet_frame = ov_buffer_free(ethernet_frame);
  testrun(0 == ethernet_frame);

  /*************************************************************************
                             Test single frame - no crc
   ************************************************************************/

  ethernet_frame = get_ethernet_frame(0, 0, 0, 0, 0, 0);

  mem_fmt = ov_format_from_memory(ethernet_frame->start, ethernet_frame->length,
                                  OV_READ);

  testrun(0 != mem_fmt);

  ov_format_option_ethernet fo_ether = {

      .crc_present = false,

  };

  ethernet_fmt = ov_format_as(mem_fmt, "ethernet", &fo_ether, 0);
  testrun(0 != ethernet_fmt);

  mem_fmt = 0;
  buf = ov_format_payload_read_chunk_nocopy(ethernet_fmt, 0);

  /* Since checksum is not treated as checksum, it becomes part of the payload
   * and thus we get 4 additional paylaod bytes...
   */
  testrun(buf.length == ref_payload_len + 4);
  testrun(0 == memcmp(buf.start, ref_payload, buf.length - 4));
  testrun(0 == memcmp(buf.start + ref_payload_len, ref_payload_crc32, 4));
  testrun(0 == ov_format_ethernet_get_crc32(ethernet_fmt));

  ethernet_fmt = ov_format_close(ethernet_fmt);

  testrun(0 == ethernet_fmt);

  ethernet_frame = ov_buffer_free(ethernet_frame);
  testrun(0 == ethernet_frame);

  /*************************************************************************
                 Test with several frames in PCAP container
   ************************************************************************/

  char *test_pcap_file_path = ov_test_get_resource_path(TEST_PCAP_FILE);
  testrun(0 != test_pcap_file_path);

  ov_format *file_fmt = ov_format_open(test_pcap_file_path, OV_READ);
  testrun(0 != file_fmt);

  free(test_pcap_file_path);
  test_pcap_file_path = 0;

  ov_format_pcap_install(0);
  ov_format *pcap_fmt = ov_format_as(file_fmt, "pcap", 0, 0);
  testrun(0 != pcap_fmt);

  file_fmt = 0;

  ethernet_fmt = ov_format_as(pcap_fmt, "ethernet", 0, 0);
  testrun(0 != ethernet_fmt);

  pcap_fmt = 0;

  ov_buffer payload = ov_format_payload_read_chunk_nocopy(ethernet_fmt, 0);

  while (0 != payload.start) {

    ov_format_ethernet_header hdr = {0};

    testrun(ov_format_ethernet_get_header(ethernet_fmt, &hdr));
    uint32_t crc32 = ov_format_ethernet_get_crc32(ethernet_fmt);

    uint32_t crc32_calc = ov_format_ethernet_calculate_crc32(ethernet_fmt);

    testrun_log("Got another frame: src mac: %s dst_mad: %s, CRC32: "
                "%" PRIu32 ", calculated %" PRIu32 " ethertype: %i"
                "\n ",
                ov_format_ethernet_mac_to_string(hdr.src_mac, 0, 0),
                ov_format_ethernet_mac_to_string(hdr.dst_mac, 0, 0), crc32,
                crc32_calc, hdr.type_set ? hdr.type : hdr.length);

    payload = ov_format_payload_read_chunk_nocopy(ethernet_fmt, 0);
  };

  ethernet_fmt = ov_format_close(ethernet_fmt);

  testrun(0 == ethernet_fmt);

  /*************************************************************************
                                  Cleanup
   ************************************************************************/

  ov_format_registry_clear(0);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_format_ethernet_get_header() {

  const uint8_t empty_mac[OV_FORMAT_ETHERNET_MAC_LEN_OCTETS] = {0};

  ov_format_ethernet_header hdr = {0};

  testrun(!ov_format_ethernet_get_header(0, &hdr));
  testrun(0 == hdr.length);
  testrun(0 == hdr.type);
  testrun(!hdr.type_set);
  testrun(0 ==
          memcmp(empty_mac, hdr.dst_mac, OV_FORMAT_ETHERNET_MAC_LEN_OCTETS));
  testrun(0 ==
          memcmp(empty_mac, hdr.src_mac, OV_FORMAT_ETHERNET_MAC_LEN_OCTETS));

  ov_buffer *frame = get_ethernet_frame(0, 0, 0, 0, 0, 0);

  ov_format *fmt = ov_format_from_memory(frame->start, frame->length, OV_READ);
  testrun(0 != fmt);

  ov_format_ethernet_install(0);

  ov_format *ethernet_fmt = ov_format_as(fmt, "ethernet", 0, 0);
  testrun(0 != ethernet_fmt);

  fmt = 0;

  ov_buffer payload = ov_format_payload_read_chunk_nocopy(ethernet_fmt, 0);
  testrun(0 != payload.start);
  testrun(ref_payload_len == payload.length);
  testrun(0 == memcmp(ref_payload, payload.start, ref_payload_len));

  testrun(ov_format_ethernet_get_header(ethernet_fmt, &hdr));
  testrun(0 == hdr.length);
  testrun(ref_ethertype == hdr.type);
  testrun(hdr.type_set);
  testrun(0 ==
          memcmp(ref_dst_mac, hdr.dst_mac, OV_FORMAT_ETHERNET_MAC_LEN_OCTETS));
  testrun(0 ==
          memcmp(ref_src_mac, hdr.src_mac, OV_FORMAT_ETHERNET_MAC_LEN_OCTETS));

  testrun(ref_crc32 == ov_format_ethernet_get_crc32(ethernet_fmt));

  ethernet_fmt = ov_format_close(ethernet_fmt);
  testrun(0 == ethernet_fmt);

  frame = ov_buffer_free(frame);
  testrun(0 == frame);

  ov_format_registry_clear(0);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_format_ethernet_get_crc32_checksum() {

  testrun(0 == ov_format_ethernet_get_crc32(0));

  ov_buffer *frame = get_ethernet_frame(0, 0, 0, 0, 0, 0);

  ov_format *fmt = ov_format_from_memory(frame->start, frame->length, OV_READ);
  testrun(0 != fmt);

  ov_format_ethernet_install(0);

  ov_format *ethernet_fmt = ov_format_as(fmt, "ethernet", 0, 0);
  testrun(0 != ethernet_fmt);

  fmt = 0;

  ov_buffer payload = ov_format_payload_read_chunk_nocopy(ethernet_fmt, 0);
  testrun(0 != payload.start);
  testrun(ref_payload_len == payload.length);
  testrun(0 == memcmp(ref_payload, payload.start, ref_payload_len));

  testrun(ref_crc32 == ov_format_ethernet_get_crc32(ethernet_fmt));

  ethernet_fmt = ov_format_close(ethernet_fmt);
  testrun(0 == ethernet_fmt);

  frame = ov_buffer_free(frame);
  testrun(0 == frame);

  ov_format_registry_clear(0);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_format_ethernet_calculate_crc32() {

  testrun(0 == ov_format_ethernet_calculate_crc32(0));

  ov_buffer *frame = get_ethernet_frame(0, 0, 0, 0, 0, 0);

  uint32_t expected_crc32 = ov_crc32_zlib(0, frame->start, frame->length - 4);

  ov_format *fmt = ov_format_from_memory(frame->start, frame->length, OV_READ);
  testrun(0 != fmt);

  ov_format_ethernet_install(0);

  ov_format *ethernet_fmt = ov_format_as(fmt, "ethernet", 0, 0);
  testrun(0 != ethernet_fmt);

  fmt = 0;

  ov_buffer payload = ov_format_payload_read_chunk_nocopy(ethernet_fmt, 0);
  testrun(0 != payload.start);
  testrun(ref_payload_len == payload.length);
  testrun(0 == memcmp(ref_payload, payload.start, ref_payload_len));

  testrun(expected_crc32 == ov_format_ethernet_calculate_crc32(ethernet_fmt));

  ethernet_fmt = ov_format_close(ethernet_fmt);
  testrun(0 == ethernet_fmt);

  frame = ov_buffer_free(frame);
  testrun(0 == frame);

  ov_format_registry_clear(0);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_format_ethernet_mac_to_string() {

  testrun(0 == ov_format_ethernet_mac_to_string(0, 0, 0));

  char const ref_mac[] = "Egill";

  char const ref_mac_str[] = "45:67:69:6c:6c:0";

  char *mac_str = ov_format_ethernet_mac_to_string((uint8_t *)ref_mac, 0, 0);
  testrun(0 == strcmp(ref_mac_str, mac_str));

  char const ref_mac2[] = "Hymir";
  char mac_str2[3 * OV_FORMAT_ETHERNET_MAC_LEN_OCTETS] = {0};

  char ref_mac_str2[] = "48:79:6d:69:72:0";

  testrun(mac_str2 == ov_format_ethernet_mac_to_string(
                          (uint8_t *)ref_mac2, mac_str2, sizeof(mac_str2)));

  testrun(0 == strcmp(ref_mac_str2, mac_str2));

  testrun(mac_str != mac_str2);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static char const *ref_payload_dispatcher_test[] = {

    "adfsdafsdf\n", "crucial\n", "renowned\n", "Pimping\n", "noodle\n",
    "dump\n",       "cooker\n",  "in\n",       "the\n",     "fields\n"};

static int test_ov_format_ethernet_dispatcher_install() {

  ov_buffer *frame = get_ethernet_frame(0, 0, 0, 0, 0, 0);

  ov_format *fmt = ov_format_from_memory(frame->start, frame->length, OV_READ);

  testrun(0 != fmt);

  testrun(0 ==
          ov_format_as(fmt, OV_FORMAT_ETHERNET_DISPATCHER_TYPE_STRING, 0, 0));

  /* Won't work because we require ipv4 and ipv6 installed ... */
  testrun(ov_format_ethernet_dispatcher_install(0));

  ov_format *dispatcher =
      ov_format_as(fmt, OV_FORMAT_ETHERNET_DISPATCHER_TYPE_STRING, 0, 0);

  testrun(0 == dispatcher);

  fmt = ov_format_close(fmt);
  testrun(0 == fmt);
  frame = ov_buffer_free(frame);
  testrun(0 == frame);

  /* Check mixed IP version reading */

  testrun(ov_format_pcap_install(0));
  testrun(ov_format_ethernet_install(0));
  testrun(ov_format_ipv4_install(0));
  testrun(ov_format_ipv6_install(0));
  testrun(ov_format_udp_install(0));

  char *test_ip_pcap = ov_test_get_resource_path(TEST_IP_PCAP_FILE);
  testrun(0 != test_ip_pcap);

  fmt = ov_format_open(test_ip_pcap, OV_READ);
  testrun(0 != fmt);

  free(test_ip_pcap);
  test_ip_pcap = 0;

  fmt = ov_format_as(fmt, "pcap", 0, 0);
  testrun(0 != fmt);

  ov_format_option_ethernet fo_ether = {

      .crc_present = false,

  };

  dispatcher = ov_format_as(fmt, OV_FORMAT_ETHERNET_DISPATCHER_TYPE_STRING,
                            &fo_ether, 0);
  fmt = 0;

  testrun(0 != dispatcher);

  ov_format *udp = ov_format_as(dispatcher, "udp", 0, 0);
  testrun(0 != udp);

  dispatcher = 0;

  size_t num_ipv4_frames_seen = 0;
  size_t num_ipv6_frames_seen = 0;

  size_t num_frame = 0;

  while (ov_format_has_more_data(udp)) {

    ov_buffer chunk = ov_format_payload_read_chunk_nocopy(udp, 0);

    bool ipv4 = false;

    testrun(chunk.length == strlen(ref_payload_dispatcher_test[num_frame]));

    testrun(0 == memcmp(ref_payload_dispatcher_test[num_frame++], chunk.start,
                        chunk.length));

    if (0 != ov_format_get(udp, "ipv4")) {
      ipv4 = true;
      ++num_ipv4_frames_seen;
    }

    if (0 != ov_format_get(udp, "ipv6")) {
      ++num_ipv6_frames_seen;
      testrun(!ipv4);
    }
  };

  testrun_log("IPv4 seen: %zu     IPv6 seen: %zu\n", num_ipv4_frames_seen,
              num_ipv6_frames_seen);

  testrun(6 == num_ipv4_frames_seen);
  testrun(4 == num_ipv6_frames_seen);

  udp = ov_format_close(udp);

  testrun(0 == udp);
  ov_format_registry_clear(0);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

OV_TEST_RUN("ov_format_ethernet", test_ov_format_ethernet_install,
            test_impl_next_chunk, test_ov_format_ethernet_get_header,
            test_ov_format_ethernet_get_crc32_checksum,
            test_ov_format_ethernet_calculate_crc32,
            test_ov_format_ethernet_mac_to_string,
            test_ov_format_ethernet_dispatcher_install);
