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
#include "ov_format_udp.c"

#include <ov_test/ov_test.h>

#include "../include/ov_format_ethernet.h"
#include "../include/ov_format_ipv4.h"
#include "../include/ov_format_pcap.h"

/*----------------------------------------------------------------------------*/

static char const *TEST_PCAP_FILE = "resources/pcap/test_ipv4_udp.pcap";

static uint8_t ref_udp_frame_min[8] = {0x01, // Src port
                                       0x02,
                                       0x08, // Dest port
                                       0x07,
                                       0x00, // Total datagram length in octets
                                       0x08,
                                       0x12, // Checksum dummy
                                       0x34};

/*----------------------------------------------------------------------------*/

static uint8_t ref_udp_frame_payload[] = {
    0x01, // Src port
    0x02,
    0x08, // Dest port
    0x07,
    0x00, // Total datagram length in octets
    0x0d,
    0x12, // Checksum dummy
    0x34,
    'y', // payload
    'm',
    'i',
    'r',
    0x00};

_Static_assert(sizeof(ref_udp_frame_payload) >= 8,
               "UDP frame must be at least 8 bytes");

static uint8_t *ref_payload = ref_udp_frame_payload + 8;

static size_t ref_payload_length = sizeof(ref_udp_frame_payload) - 8;

/*----------------------------------------------------------------------------*/

static void print_header(FILE *out, ov_format_udp_header hdr) {

    OV_ASSERT(0 != out);

    fprintf(out,
            "SRC Port: %" PRIu16 "   DST Port: %" PRIu16
            " Total Length: %" PRIu16 " Checksum: %" PRIu16 "\n",
            hdr.source_port,
            hdr.destination_port,
            hdr.length_octets,
            hdr.checksum);
}

/*----------------------------------------------------------------------------*/

static int test_ov_format_udp_install() {

    ov_format *fmt = ov_format_from_memory(
        ref_udp_frame_min, sizeof(ref_udp_frame_min), OV_READ);
    testrun(0 != fmt);
    ov_format *udp_fmt = ov_format_as(fmt, "udp", 0, 0);
    testrun(0 == udp_fmt);

    ov_format_udp_install(0);

    udp_fmt = ov_format_as(fmt, "udp", 0, 0);
    testrun(0 != udp_fmt);

    fmt = 0;

    udp_fmt = ov_format_close(udp_fmt);
    testrun(0 == udp_fmt);

    ov_format_registry_clear(0);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_impl_next_chunk() {

    ov_format *mem_fmt = ov_format_from_memory(
        ref_udp_frame_min, sizeof(ref_udp_frame_min), OV_READ);

    testrun(0 != mem_fmt);

    testrun(0 == ov_format_as(mem_fmt, "udp", 0, 0));

    testrun(ov_format_udp_install(0));

    ov_format *udp_fmt = ov_format_as(mem_fmt, "udp", 0, 0);
    testrun(0 != udp_fmt);

    mem_fmt = 0;

    /*************************************************************************
                               Test single frame - header only
     ************************************************************************/

    ov_buffer buf = ov_format_payload_read_chunk_nocopy(udp_fmt, 0);

    testrun(buf.length == 0);

    ov_format_udp_header hdr = {0};

    testrun(ov_format_udp_get_header(udp_fmt, &hdr));
    testrun(0x1234 == hdr.checksum);
    testrun(8 == hdr.length_octets);
    testrun(0x0102 == hdr.source_port);
    testrun(0x0807 == hdr.destination_port);

    udp_fmt = ov_format_close(udp_fmt);

    testrun(0 == udp_fmt);

    /*************************************************************************
                               Test single frame - with payload
     ************************************************************************/

    mem_fmt = ov_format_from_memory(
        ref_udp_frame_payload, sizeof(ref_udp_frame_payload), OV_READ);

    testrun(0 != mem_fmt);

    udp_fmt = ov_format_as(mem_fmt, "udp", 0, 0);
    testrun(0 != udp_fmt);

    mem_fmt = 0;

    buf = ov_format_payload_read_chunk_nocopy(udp_fmt, 0);

    testrun(buf.length == ref_payload_length);
    testrun(0 == memcmp(buf.start, ref_payload, buf.length));

    hdr = (ov_format_udp_header){0};

    testrun(ov_format_udp_get_header(udp_fmt, &hdr));
    testrun(0x1234 == hdr.checksum);
    testrun(sizeof(ref_udp_frame_payload) == hdr.length_octets);
    testrun(0x0102 == hdr.source_port);
    testrun(0x0807 == hdr.destination_port);

    udp_fmt = ov_format_close(udp_fmt);

    testrun(0 == udp_fmt);

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
    ov_format_ipv4_install(0);

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

    ov_format *ipv4_fmt = ov_format_as(ethernet_fmt, "ipv4", 0, 0);
    testrun(0 != ipv4_fmt);

    ethernet_fmt = 0;

    udp_fmt = ov_format_as(ipv4_fmt, "udp", 0, 0);

    testrun(0 != udp_fmt);

    ipv4_fmt = 0;

    ov_buffer payload = ov_format_payload_read_chunk_nocopy(udp_fmt, 0);

    while (0 != payload.start) {

        ov_format_udp_header hdr = {0};

        testrun(ov_format_udp_get_header(udp_fmt, &hdr));

        testrun(INT_HEADER_LENGTH + payload.length == hdr.length_octets);

        print_header(stderr, hdr);

        testrun_log("Read payload octets: %zu", payload.length);

        payload = ov_format_payload_read_chunk_nocopy(udp_fmt, 0);
    };

    udp_fmt = ov_format_close(udp_fmt);

    testrun(0 == udp_fmt);

    /*************************************************************************
                                    Cleanup
     ************************************************************************/

    ov_format_registry_clear(0);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_format_udp_get_header() {

    /* Tested in test_impl_next_chunk */

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

OV_TEST_RUN("ov_format_udp",
            test_ov_format_udp_install,
            test_impl_next_chunk,
            test_ov_format_udp_get_header);
