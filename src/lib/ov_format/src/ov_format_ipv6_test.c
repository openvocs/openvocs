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
#include "ov_format_ipv6.c"

#include <ov_test/ov_test.h>

#include "../include/ov_format_ipv6.h"
#include "../include/ov_format_linux_sll.h"
#include "../include/ov_format_pcap.h"

/*----------------------------------------------------------------------------*/

static char const *TEST_PCAP_FILE = "resources/pcap/test_ipv4_ipv6_udp.pcap";

static size_t ref_num_valid_ipv6_packets = 5;

/*----------------------------------------------------------------------------*/

static const uint8_t ref_traffic_class = 0xab;
static const uint32_t ref_flow_label = 0xcba98;
static const uint8_t ref_next_header = 0x06;
static const uint8_t ref_hop_limit = 0x02;

static const uint8_t ref_src_address[] = {'0',
                                          '1',
                                          '2',
                                          '3',
                                          '4',
                                          '5',
                                          '6',
                                          '7',
                                          '8',
                                          '9',
                                          'a',
                                          'b',
                                          'c',
                                          'd',
                                          'e',
                                          'f'};

static const uint8_t ref_dst_address[] = {'f',
                                          '2',
                                          'e',
                                          '4',
                                          'd',
                                          '6',
                                          'c',
                                          '8',
                                          'b',
                                          '1',
                                          'a',
                                          '3',
                                          '9',
                                          '5',
                                          '7',
                                          '0'};

static unsigned char ref_ipv6_frame_payload[] = {
    0x6a, // Version & traffic class
    0xbc, // traffic class & flow label
    0xba, // flow label
    0x98,
    0x00, // payload length (octets)
    0x05,
    0x06, // Next header (TCP)
    0x02, // HOP limit
          // SRC IP
    '0',
    '1',
    '2',
    '3',
    '4',
    '5',
    '6',
    '7',
    '8',
    '9',
    'a',
    'b',
    'c',
    'd',
    'e',
    'f',
    // DST IP
    'f',
    '2',
    'e',
    '4',
    'd',
    '6',
    'c',
    '8',
    'b',
    '1',
    'a',
    '3',
    '9',
    '5',
    '7',
    '0',
    // Payload
    'a',
    'b',
    'c',
    'd',
    'e'};

static const uint8_t *ref_payload =
    ref_ipv6_frame_payload + sizeof(ref_ipv6_frame_payload) - 5;

static const size_t ref_payload_length = 5;

/*----------------------------------------------------------------------------*/

static void print_header(FILE *out, ov_format *fmt) {

    OV_ASSERT(0 != out);

    ov_format const *pcap = ov_format_get(fmt, "pcap");

    ov_format_pcap_packet_header pcap_hdr = {0};

    if (!ov_format_pcap_get_current_packet_header(pcap, &pcap_hdr)) {

        testrun_log_error("No PCAP header found\n");
        return;
    }

    fprintf(out,
            "%" PRIu32 ".%" PRIu32 " - ",
            pcap_hdr.timestamp_secs,
            pcap_hdr.timestamp_usecs);

    ov_format const *ipv6 = ov_format_get(fmt, "ipv6");

    ov_format_ipv6_header ipv6_hdr = {0};

    if (!ov_format_ipv6_get_header(ipv6, &ipv6_hdr)) {

        fprintf(out, "No ipv6 packet\n");
        return;
    }

    fprintf(out,
            "SRC IP: %s  DST IP: %s    "
            "Payload Length: %" PRIu32 " next_header %" PRIu8
            "   hop limit: "
            "%" PRIu8 "\n",
            ov_format_ipv6_ip_to_string(ipv6_hdr.src_ip, 0, 0),
            ov_format_ipv6_ip_to_string(ipv6_hdr.dst_ip, 0, 0),
            ipv6_hdr.payload_length,
            ipv6_hdr.next_header,
            ipv6_hdr.hop_limit);
}

/*----------------------------------------------------------------------------*/

static int test_ov_format_ipv6_install() {

    ov_format *fmt = ov_format_from_memory(
        ref_ipv6_frame_payload, sizeof(ref_ipv6_frame_payload), OV_READ);
    testrun(0 != fmt);
    ov_format *ipv6_fmt = ov_format_as(fmt, "ipv6", 0, 0);
    testrun(0 == ipv6_fmt);

    ov_format_ipv6_install(0);

    ipv6_fmt = ov_format_as(fmt, "ipv6", 0, 0);
    testrun(0 != ipv6_fmt);

    fmt = 0;

    ipv6_fmt = ov_format_close(ipv6_fmt);
    testrun(0 == ipv6_fmt);

    ov_format_registry_clear(0);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_impl_next_chunk() {

    testrun(0 == ov_format_as(0, "ipv6", 0, 0));

    /*************************************************************************
                               Test single frame
     ************************************************************************/

    ov_format *mem_fmt = ov_format_from_memory(
        ref_ipv6_frame_payload, sizeof(ref_ipv6_frame_payload), OV_READ);

    testrun(0 != mem_fmt);

    testrun(0 == ov_format_as(mem_fmt, "ipv6", 0, 0));

    testrun(ov_format_ipv6_install(0));

    testrun(0 == ov_format_as(0, "ipv6", 0, 0));

    ov_format *ipv6_fmt = ov_format_as(mem_fmt, "ipv6", 0, 0);
    testrun(0 != ipv6_fmt);

    mem_fmt = 0;

    ov_buffer buf = ov_format_payload_read_chunk_nocopy(ipv6_fmt, 0);

    testrun(buf.length == ref_payload_length);
    testrun(0 == memcmp(buf.start, ref_payload, buf.length));

    ov_format_ipv6_header hdr = {0};

    testrun(ov_format_ipv6_get_header(ipv6_fmt, &hdr));

    testrun(hdr.traffic_class == ref_traffic_class);
    testrun(hdr.flow_label == ref_flow_label);
    testrun(hdr.next_header == ref_next_header);
    testrun(hdr.hop_limit == ref_hop_limit);

    testrun(0 == memcmp(ref_src_address, hdr.src_ip, 16));
    testrun(0 == memcmp(ref_dst_address, hdr.dst_ip, 16));

    ipv6_fmt = ov_format_close(ipv6_fmt);

    testrun(0 == ipv6_fmt);

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
    ov_format_linux_sll_install(0);

    ov_format *pcap_fmt = ov_format_as(file_fmt, "pcap", 0, 0);
    testrun(0 != pcap_fmt);

    file_fmt = 0;

    ov_format *sll_fmt = ov_format_as(pcap_fmt, "linux_sll", 0, 0);

    testrun(0 != sll_fmt);

    pcap_fmt = 0;

    ipv6_fmt = ov_format_as(sll_fmt, "ipv6", 0, 0);
    testrun(0 != ipv6_fmt);

    sll_fmt = 0;

    size_t valid_ipv6_packets = 0;
    size_t total_num_packets = 0;

    do {

        ov_buffer payload = ov_format_payload_read_chunk_nocopy(ipv6_fmt, 0);

        ++total_num_packets;

        if (0 == payload.start) continue;

        ov_format_ipv6_header hdr = {0};

        testrun(ov_format_ipv6_get_header(ipv6_fmt, &hdr));

        testrun(hdr.payload_length == payload.length);

        print_header(stderr, ipv6_fmt);

        ++valid_ipv6_packets;

    } while (ov_format_has_more_data(ipv6_fmt));

    ipv6_fmt = ov_format_close(ipv6_fmt);

    testrun_log("Read %zu packets, %zu valid ipv6 packets\n",
                total_num_packets,
                valid_ipv6_packets);

    testrun(ref_num_valid_ipv6_packets == valid_ipv6_packets);

    testrun(0 == ipv6_fmt);

    /*************************************************************************
                                    Cleanup
     ************************************************************************/

    ov_format_registry_clear(0);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_format_ipv6_get_header() {

    ov_format_ipv6_header hdr;

    testrun(!ov_format_ipv6_get_header(0, 0));
    testrun(!ov_format_ipv6_get_header(0, &hdr));

    ov_format_registry_clear(0);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

OV_TEST_RUN("ov_format_ipv6",
            test_ov_format_ipv6_install,
            test_impl_next_chunk,
            test_ov_format_ipv6_get_header);
