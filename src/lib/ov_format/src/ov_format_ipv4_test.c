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
#include "ov_format_ipv4.c"

#include <ov_test/ov_test.h>

#include "../include/ov_format_ethernet.h"
#include "../include/ov_format_ipv4.h"
#include "../include/ov_format_pcap.h"

/*----------------------------------------------------------------------------*/

static char const *TEST_PCAP_FILE = "resources/pcap/test_ipv4_tcp.pcap";

static uint8_t ref_ipv4_frame_min[] = {0x45, // Version & IHL
                                       0x00, // ignored
                                       0x00, // total length octets
                                       0x14, // total length octets
                                       0x00, // ignored
                                       0x00, 0x00, 0x00,
                                       0x02, // ttl
                                       0x04, // Protocol
                                       0x14, // header checksum
                                       0x12, // SRC IP
                                       0x01, 0x02, 0x03, 0x04,
                                       0x08, // DST IP
                                       0x07, 0x06, 0x05};

/*----------------------------------------------------------------------------*/

static uint8_t ref_ipv4_frame_payload[] = {0x45, // Version & IHL
                                           0x00, // ignored
                                           0x00, // total length octets
                                           0x19, // total length octets
                                           0x00, // ignored
                                           0x00, 0x00, 0x00,
                                           0x02, // ttl
                                           0x04, // Protocol
                                           0x14, // header checksum
                                           0x12,
                                           0x01, // SRC IP
                                           0x02, 0x03, 0x04,
                                           0x08, // DST IP
                                           0x07, 0x06, 0x05,
                                           'a', // Payload
                                           'b',  'c',  'd',  'e'};

static const uint8_t *ref_payload = ref_ipv4_frame_payload + 20;
static const size_t ref_payload_length = 5;

/*----------------------------------------------------------------------------*/

static void print_header(FILE *out, ov_format_ipv4_header hdr) {

    // uint8_t src_ip[4];
    // uint8_t dst_ip[4];

    // size_t header_length_octets;
    // uint16_t total_length_octets;
    // uint8_t protocol;
    // uint8_t time_to_live;

    // uint16_t header_checksum;

    OV_ASSERT(0 != out);

    fprintf(out,
            "SRC IP: %s  DST IP: %s\n"
            "Header Length: %zu    Total length: %" PRIu16
            "\n"
            "protocol %" PRIu8 "   TTL: %" PRIu8 "\n",
            ov_format_ipv4_ip_to_string(hdr.src_ip, 0, 0),
            ov_format_ipv4_ip_to_string(hdr.dst_ip, 0, 0),
            hdr.header_length_octets,
            hdr.total_length_octets,
            hdr.protocol,
            hdr.time_to_live);
}

/*----------------------------------------------------------------------------*/

static int test_ov_format_ipv4_install() {

    ov_format *fmt = ov_format_from_memory(
        ref_ipv4_frame_min, sizeof(ref_ipv4_frame_min), OV_READ);
    testrun(0 != fmt);
    ov_format *ipv4_fmt = ov_format_as(fmt, "ipv4", 0, 0);
    testrun(0 == ipv4_fmt);

    ov_format_ipv4_install(0);

    ipv4_fmt = ov_format_as(fmt, "ipv4", 0, 0);
    testrun(0 != ipv4_fmt);

    fmt = 0;

    ipv4_fmt = ov_format_close(ipv4_fmt);
    testrun(0 == ipv4_fmt);

    ov_format_registry_clear(0);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_impl_next_chunk() {

    ov_format *mem_fmt = ov_format_from_memory(
        ref_ipv4_frame_min, sizeof(ref_ipv4_frame_min), OV_READ);

    testrun(0 != mem_fmt);

    testrun(0 == ov_format_as(mem_fmt, "ipv4", 0, 0));

    testrun(ov_format_ipv4_install(0));

    ov_format *ipv4_fmt = ov_format_as(mem_fmt, "ipv4", 0, 0);
    testrun(0 != ipv4_fmt);

    mem_fmt = 0;

    /*************************************************************************
                               Test single frame - header only
     ************************************************************************/

    ov_buffer buf = ov_format_payload_read_chunk_nocopy(ipv4_fmt, 0);

    testrun(buf.length == 0);

    ipv4_fmt = ov_format_close(ipv4_fmt);

    testrun(0 == ipv4_fmt);

    /*************************************************************************
                               Test single frame - with payload
     ************************************************************************/

    mem_fmt = ov_format_from_memory(
        ref_ipv4_frame_payload, sizeof(ref_ipv4_frame_payload), OV_READ);

    testrun(0 != mem_fmt);

    ipv4_fmt = ov_format_as(mem_fmt, "ipv4", 0, 0);
    testrun(0 != ipv4_fmt);

    mem_fmt = 0;

    buf = ov_format_payload_read_chunk_nocopy(ipv4_fmt, 0);

    testrun(buf.length == ref_payload_length);
    testrun(0 == memcmp(buf.start, ref_payload, buf.length));

    ipv4_fmt = ov_format_close(ipv4_fmt);

    testrun(0 == ipv4_fmt);

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
    ov_format_ethernet_install(0);

    ov_format *pcap_fmt = ov_format_as(file_fmt, "pcap", 0, 0);
    testrun(0 != pcap_fmt);

    file_fmt = 0;

    ov_format_option_ethernet fo_ethernet = {
        .crc_present = false,
    };

    ov_format *ethernet_fmt =
        ov_format_as(pcap_fmt, "ethernet", &fo_ethernet, 0);

    testrun(0 != ethernet_fmt);

    pcap_fmt = 0;

    ipv4_fmt = ov_format_as(ethernet_fmt, "ipv4", 0, 0);
    testrun(0 != ipv4_fmt);

    ethernet_fmt = 0;

    ov_buffer payload = ov_format_payload_read_chunk_nocopy(ipv4_fmt, 0);

    while (0 != payload.start) {

        ov_format_ipv4_header hdr = {0};

        testrun(ov_format_ipv4_get_header(ipv4_fmt, &hdr));

        testrun(hdr.header_length_octets + payload.length ==
                hdr.total_length_octets);

        print_header(stderr, hdr);

        testrun_log("Read payload octets: %zu", payload.length);

        payload = ov_format_payload_read_chunk_nocopy(ipv4_fmt, 0);
    };

    ipv4_fmt = ov_format_close(ipv4_fmt);

    testrun(0 == ipv4_fmt);

    /*************************************************************************
                                    Cleanup
     ************************************************************************/

    ov_format_registry_clear(0);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_format_ipv4_get_header() {

    // const uint8_t empty_mac[OV_FORMAT_IPV4_MAC_LEN_OCTETS] = {0};

    // ov_format_ipv4_header hdr = {0};

    // testrun(!ov_format_ipv4_get_header(0, &hdr));
    // testrun(0 == hdr.length);
    // testrun(0 == hdr.type);
    // testrun(!hdr.type_set);
    // testrun(0 == memcmp(empty_mac, hdr.dst_mac,
    // OV_FORMAT_IPV4_MAC_LEN_OCTETS)); testrun(0 == memcmp(empty_mac,
    // hdr.src_mac, OV_FORMAT_IPV4_MAC_LEN_OCTETS));

    // ov_buffer *frame = get_ipv4_frame(0, 0, 0, 0, 0, 0);

    // ov_format *fmt =
    //    ov_format_from_memory(frame->start, frame->length, OV_READ);
    // testrun(0 != fmt);

    // ov_format_ipv4_install(0);

    // ov_format *ipv4_fmt = ov_format_as(fmt, "ipv4", 0, 0);
    // testrun(0 != ipv4_fmt);

    // fmt = 0;

    // ov_buffer payload = ov_format_payload_read_chunk_nocopy(ipv4_fmt, 0);
    // testrun(0 != payload.start);
    // testrun(ref_payload_len == payload.length);
    // testrun(0 == memcmp(ref_payload, payload.start, ref_payload_len));

    // testrun(ov_format_ipv4_get_header(ipv4_fmt, &hdr));
    // testrun(0 == hdr.length);
    // testrun(ref_ethertype == hdr.type);
    // testrun(hdr.type_set);
    // testrun(0 ==
    //        memcmp(ref_dst_mac, hdr.dst_mac,
    //        OV_FORMAT_IPV4_MAC_LEN_OCTETS));
    // testrun(0 ==
    //        memcmp(ref_src_mac, hdr.src_mac,
    //        OV_FORMAT_IPV4_MAC_LEN_OCTETS));

    // testrun(ref_crc32 == ov_format_ipv4_get_crc32(ipv4_fmt));

    // ipv4_fmt = ov_format_close(ipv4_fmt);
    // testrun(0 == ipv4_fmt);

    // frame = ov_buffer_free(frame);
    // testrun(0 == frame);

    ov_format_registry_clear(0);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

OV_TEST_RUN("ov_format_ipv4",
            test_ov_format_ipv4_install,
            test_impl_next_chunk,
            test_ov_format_ipv4_get_header);
