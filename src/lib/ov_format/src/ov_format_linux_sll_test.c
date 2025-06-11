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
#include "ov_format_linux_sll.c"

#include <ov_test/ov_test.h>

#include "../include/ov_format_pcap.h"

/*----------------------------------------------------------------------------*/

char const *TEST_PCAP_FILE = "resources/pcap/test_linux_sll_ipv4_udp.pcap";

static ov_buffer *get_linux_sll_frame(uint16_t packet_type,
                                      uint16_t arphrd_type,
                                      uint16_t address_len,
                                      uint8_t address[8],
                                      uint16_t protocol_type,
                                      ov_buffer *payload) {

    OV_ASSERT(0 != payload);

    ov_buffer *frame =
        ov_buffer_create(4 * sizeof(uint16_t) + 8 + payload->length);

    uint16_t *value_ptr = (uint16_t *)frame->start;

    value_ptr[0] = htons(packet_type);
    value_ptr[1] = htons(arphrd_type);
    value_ptr[2] = htons(address_len);
    memcpy(value_ptr + 3, address, 8);
    value_ptr[7] = htons(protocol_type);

    memcpy(frame->start + 16, payload->start, payload->length);

    frame->length = 16 + payload->length;

    return frame;
}

/*----------------------------------------------------------------------------*/

static int test_ov_format_linux_sll_install() {

    ov_buffer ref_payload = {
        .start = (uint8_t *)"abcde",
        .length = 6,
    };

    uint8_t address[8] = {1, 2, 3, 4, 5, 6, 7, 8};

    ov_buffer *frame = get_linux_sll_frame(0, 0, 0, address, 0, &ref_payload);

    testrun(0 != frame);
    testrun(0 != frame->start);
    testrun(0 != frame->length);

    ov_format *fmt =
        ov_format_from_memory(frame->start, frame->length, OV_READ);
    testrun(0 != fmt);

    ov_format *linux_sll_fmt = ov_format_as(fmt, "linux_sll", 0, 0);
    testrun(0 == linux_sll_fmt);

    ov_format_linux_sll_install(0);

    linux_sll_fmt = ov_format_as(fmt, "linux_sll", 0, 0);
    testrun(0 != linux_sll_fmt);

    fmt = 0;

    linux_sll_fmt = ov_format_close(linux_sll_fmt);
    testrun(0 == linux_sll_fmt);

    frame = ov_buffer_free(frame);
    testrun(0 == frame);

    ov_format_registry_clear(0);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_impl_next_chunk() {

    ov_buffer ref_payload = {
        .start = (uint8_t *)"abcde",
        .length = 6,
    };

    uint8_t address[8] = {1, 2, 3, 4, 5, 6, 7, 8};

    ov_buffer *frame = get_linux_sll_frame(0, 0, 0, address, 0, &ref_payload);

    ov_format *mem_fmt =
        ov_format_from_memory(frame->start, frame->length, OV_READ);

    testrun(0 != mem_fmt);

    testrun(0 == ov_format_as(mem_fmt, "linux_sll", 0, 0));

    testrun(ov_format_linux_sll_install(0));

    ov_format *linux_sll_fmt = ov_format_as(mem_fmt, "linux_sll", 0, 0);
    testrun(0 != linux_sll_fmt);

    mem_fmt = 0;

    /*************************************************************************
                               Test single frame
     ************************************************************************/

    ov_buffer buf =
        ov_format_payload_read_chunk_nocopy(linux_sll_fmt, frame->length);

    testrun(buf.length == ref_payload.length);
    testrun(0 == memcmp(buf.start, ref_payload.start, buf.length));

    linux_sll_fmt = ov_format_close(linux_sll_fmt);

    testrun(0 == linux_sll_fmt);

    frame = ov_buffer_free(frame);
    testrun(0 == frame);

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

    linux_sll_fmt = ov_format_as(pcap_fmt, "linux_sll", 0, 0);
    testrun(0 != linux_sll_fmt);

    pcap_fmt = 0;

    ov_buffer payload = ov_format_payload_read_chunk_nocopy(linux_sll_fmt, 0);

    while (0 != payload.start) {

        ov_format_linux_sll_header hdr = {0};

        testrun(ov_format_linux_sll_get_header(linux_sll_fmt, &hdr));

        testrun_log("SLL packet type: %" PRIu16 " ARPHRD_type: %" PRIu16
                    " LL address length: %" PRIu16 " Protocol type; %" PRIu16,
                    hdr.packet_type,
                    hdr.arphrd_type,
                    hdr.link_layer_address_length,
                    hdr.protocol_type);

        payload = ov_format_payload_read_chunk_nocopy(linux_sll_fmt, 0);
    };

    linux_sll_fmt = ov_format_close(linux_sll_fmt);

    testrun(0 == linux_sll_fmt);

    /*************************************************************************
                                    Cleanup
     ************************************************************************/

    ov_format_registry_clear(0);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_format_linux_sll_get_header() {

    ov_format_linux_sll_header hdr = {0};

    testrun(!ov_format_linux_sll_get_header(0, &hdr));
    testrun(0 == hdr.packet_type);
    testrun(0 == hdr.arphrd_type);
    testrun(0 == hdr.link_layer_address_length);
    testrun(0 == hdr.protocol_type);

    //    ov_buffer *frame = get_linux_sll_frame(0, 0, 0, 0, 0, 0);
    //
    //    ov_format *fmt =
    //        ov_format_from_memory(frame->start, frame->length, OV_READ);
    //    testrun(0 != fmt);
    //
    //    ov_format_linux_sll_install(0);
    //
    //    ov_format *linux_sll_fmt = ov_format_as(fmt, "linux_sll", 0, 0);
    //    testrun(0 != linux_sll_fmt);
    //
    //    fmt = 0;
    //
    //    ov_buffer payload = ov_format_payload_read_chunk_nocopy(linux_sll_fmt,
    //    0); testrun(0 != payload.start); testrun(ref_payload_len ==
    //    payload.length); testrun(0 == memcmp(ref_payload, payload.start,
    //    ref_payload_len));
    //
    //    testrun(ov_format_linux_sll_get_header(linux_sll_fmt, &hdr));
    //    testrun(0 == hdr.length);
    //    testrun(ref_ethertype == hdr.type);
    //    testrun(hdr.type_set);
    //    testrun(0 == memcmp(ref_dst_mac,
    //                        hdr.dst_mac,
    //                        OV_FORMAT_LINUX_SLL_MAC_LEN_OCTETS));
    //    testrun(0 == memcmp(ref_src_mac,
    //                        hdr.src_mac,
    //                        OV_FORMAT_LINUX_SLL_MAC_LEN_OCTETS));
    //
    //    testrun(ref_crc32 == ov_format_linux_sll_get_crc32(linux_sll_fmt));
    //
    //    linux_sll_fmt = ov_format_close(linux_sll_fmt);
    //    testrun(0 == linux_sll_fmt);
    //
    //    frame = ov_buffer_free(frame);
    //    testrun(0 == frame);
    //
    //    ov_format_registry_clear(0);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

OV_TEST_RUN("ov_format_linux_sll",
            test_ov_format_linux_sll_install,
            test_impl_next_chunk,
            test_ov_format_linux_sll_get_header);
