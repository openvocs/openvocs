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
#include "ov_format_rtp.c"

#include <ov_test/ov_test.h>

#include <ov_base/ov_rtp_frame.h>

#include "../include/ov_format_ethernet.h"
#include "../include/ov_format_ipv4.h"
#include "../include/ov_format_pcap.h"
#include "../include/ov_format_udp.h"

/*----------------------------------------------------------------------------*/

char const *TEST_PCAP_FILE = "resources/pcap/test_ipv4_tcp.pcap";

/*----------------------------------------------------------------------------*/

static bool header_equals_unsafe(ov_format_rtp_header hdr,
                                 ov_rtp_frame const *frame) {

    OV_ASSERT(0 != frame);
    if (hdr.version != frame->expanded.version) {

        ov_log_error("Versions differ: %i vs %i", hdr.version,
                     frame->expanded.version);
        return false;
    }

    if (frame->expanded.marker_bit != hdr.marker) {

        return false;
    }

    if (frame->expanded.payload_type != hdr.payload_type) {

        return false;
    }

    if (frame->expanded.sequence_number != hdr.sequence_number) {

        return false;
    }

    if (frame->expanded.timestamp != hdr.timestamp) {

        return false;
    }

    if (frame->expanded.ssrc != hdr.ssrc) {

        return false;
    }

    return true;
}

/*----------------------------------------------------------------------------*/

static ov_rtp_frame *get_encoded_frame(ov_rtp_frame_expansion *expansion) {

    ov_rtp_frame_expansion exp = {0};

    static uint8_t test_payload[] = {9, 1, 8, 2, 7, 3, 6, 4, 5};

    exp.version = RTP_VERSION_2;
    exp.padding_bit = false;
    exp.extension_bit = false;

    exp.marker_bit = false;
    exp.payload_type = 99;

    exp.csrc_count = 0;
    exp.ssrc = 190;
    exp.sequence_number = 12345;
    exp.timestamp = 0x12345678;
    exp.ssrc = 0x87654321;

    exp.payload.data = test_payload;
    exp.payload.length = sizeof(test_payload);

    if (0 != expansion) {

        exp = *expansion;
    }

    return ov_rtp_frame_encode(&exp);
}

/*----------------------------------------------------------------------------*/

static int test_ov_format_rtp_install() {

    ov_rtp_frame *rtp_frame = get_encoded_frame(0);
    testrun(0 != rtp_frame);

    ov_format *mem_fmt = ov_format_from_memory(
        rtp_frame->bytes.data, rtp_frame->bytes.length, OV_READ);

    testrun(0 != mem_fmt);

    testrun(0 == ov_format_as(mem_fmt, "rtp", 0, 0));

    testrun(ov_format_rtp_install(0));

    ov_format *rtp_fmt = ov_format_as(mem_fmt, "rtp", 0, 0);
    testrun(0 != rtp_fmt);

    mem_fmt = 0;

    rtp_fmt = ov_format_close(rtp_fmt);

    testrun(0 == rtp_fmt);

    /*************************************************************************
                                    Cleanup
     ************************************************************************/

    rtp_frame = rtp_frame->free(rtp_frame);

    ov_format_registry_clear(0);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_impl_next_chunk() {

    /*************************************************************************
                              Initialize test data
     ************************************************************************/

    ov_rtp_frame *rtp_frame = get_encoded_frame(0);
    testrun(0 != rtp_frame);

    ov_format *mem_fmt = ov_format_from_memory(
        rtp_frame->bytes.data, rtp_frame->bytes.length, OV_READ);

    testrun(0 != mem_fmt);

    testrun(ov_format_rtp_install(0));
    ov_format *rtp_fmt = ov_format_as(mem_fmt, "rtp", 0, 0);
    testrun(0 != rtp_fmt);
    mem_fmt = 0;

    /*************************************************************************
                                Single RTP frame
     ************************************************************************/

    ov_buffer payload = {0};

    payload = ov_format_payload_read_chunk_nocopy(0, 0);
    testrun(0 == payload.start);

    payload = ov_format_payload_read_chunk_nocopy(0, 1);
    testrun(0 == payload.start);

    payload = ov_format_payload_read_chunk_nocopy(rtp_fmt, 0);

    testrun(0 != payload.start);
    testrun(rtp_frame->expanded.payload.length == payload.length);
    testrun(0 == memcmp(rtp_frame->expanded.payload.data, payload.start,
                        payload.length));

    rtp_fmt = ov_format_close(rtp_fmt);

    testrun(0 == rtp_fmt);

    /*************************************************************************
                                    Cleanup
     ************************************************************************/

    rtp_frame = rtp_frame->free(rtp_frame);

    /*************************************************************************
      Read in a full-stack pcap containing ethernet -> ipv4 -> udp -> rtp
     ************************************************************************/

    ov_format_pcap_install(0);
    ov_format_ethernet_install(0);
    ov_format_ipv4_install(0);
    ov_format_udp_install(0);
    ov_format_rtp_install(0);

    char *abs_test_pcap_path = ov_test_get_resource_path(TEST_PCAP_FILE);

    ov_format *file_fmt = ov_format_open(abs_test_pcap_path, OV_READ);
    testrun(0 != file_fmt);

    free(abs_test_pcap_path);
    abs_test_pcap_path = 0;

    ov_format *pcap_fmt = ov_format_as(file_fmt, "pcap", 0, 0);
    testrun(0 != pcap_fmt);

    file_fmt = 0;

    /* PCAPs don't save the CRC32 footer */
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

    ov_format *udp_fmt = ov_format_as(ipv4_fmt, "udp", 0, 0);
    testrun(0 != udp_fmt);

    ipv4_fmt = 0;

    rtp_fmt = ov_format_as(udp_fmt, "rtp", 0, 0);
    testrun(0 != rtp_fmt);

    udp_fmt = 0;

    payload = ov_format_payload_read_chunk_nocopy(rtp_fmt, 0);

    while (ov_format_has_more_data(rtp_fmt)) {

        // testrun(0 != payload.start);

        ov_format_rtp_header hdr = {0};

        testrun(ov_format_rtp_get_header(rtp_fmt, &hdr));

        testrun_log("RTP frame: SSRC: %" PRIu32 " Sequence number: %" PRIu16
                    " timestamp: %" PRIu32 "   payload type: %u",
                    hdr.ssrc, hdr.sequence_number, hdr.timestamp,
                    hdr.payload_type);

        payload = ov_format_payload_read_chunk_nocopy(rtp_fmt, 0);
    };

    testrun(!ov_format_has_more_data(rtp_fmt));

    rtp_fmt = ov_format_close(rtp_fmt);
    testrun(0 == rtp_fmt);

    ov_format_registry_clear(0);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_format_rtp_get_header() {

    /*************************************************************************
                              Initialize test data
     ************************************************************************/

    ov_rtp_frame *rtp_frame = get_encoded_frame(0);
    testrun(0 != rtp_frame);

    ov_format *mem_fmt = ov_format_from_memory(
        rtp_frame->bytes.data, rtp_frame->bytes.length, OV_READ);

    testrun(0 != mem_fmt);

    testrun(ov_format_rtp_install(0));
    ov_format *rtp_fmt = ov_format_as(mem_fmt, "rtp", 0, 0);
    testrun(0 != rtp_fmt);
    mem_fmt = 0;

    /*************************************************************************
                                  Actual test
     ************************************************************************/

    ov_format_rtp_header hdr = {0};

    testrun(!ov_format_rtp_get_header(0, 0));
    testrun(!ov_format_rtp_get_header(rtp_fmt, 0));
    testrun(!ov_format_rtp_get_header(0, &hdr));

    ov_buffer payload = ov_format_payload_read_chunk_nocopy(rtp_fmt, 1);

    testrun(0 != payload.start);
    testrun(rtp_frame->expanded.payload.length == payload.length);
    testrun(0 == memcmp(rtp_frame->expanded.payload.data, payload.start,
                        payload.length));

    testrun(ov_format_rtp_get_header(rtp_fmt, &hdr));

    testrun(header_equals_unsafe(hdr, rtp_frame));

    rtp_fmt = ov_format_close(rtp_fmt);

    testrun(0 == rtp_fmt);

    /*************************************************************************
                                    Cleanup
     ************************************************************************/

    rtp_frame = rtp_frame->free(rtp_frame);

    ov_format_registry_clear(0);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_format_rtp_get_contributing_sources() {

    testrun(!ov_format_rtp_get_contributing_sources(0, 0));

    /*************************************************************************
                              Initialize test data
     ************************************************************************/

    ov_rtp_frame *rtp_frame = get_encoded_frame(0);
    testrun(0 != rtp_frame);

    ov_format *mem_fmt = ov_format_from_memory(
        rtp_frame->bytes.data, rtp_frame->bytes.length, OV_READ);

    testrun(0 != mem_fmt);

    testrun(ov_format_rtp_install(0));
    ov_format *rtp_fmt = ov_format_as(mem_fmt, "rtp", 0, 0);
    testrun(0 != rtp_fmt);
    mem_fmt = 0;

    testrun(!ov_format_rtp_get_contributing_sources(rtp_fmt, 0));

    ov_format_rtp_contributing_sources csrcs = {0};

    testrun(!ov_format_rtp_get_contributing_sources(0, &csrcs));

    testrun(ov_format_rtp_get_contributing_sources(rtp_fmt, &csrcs));

    testrun(0 == csrcs.num);

    ov_buffer buf = ov_format_payload_read_chunk_nocopy(rtp_fmt, 0);
    testrun(0 != buf.start);

    testrun(ov_format_rtp_get_contributing_sources(rtp_fmt, &csrcs));

    testrun(0 == csrcs.num);

    buf.start = 0;

    rtp_fmt = ov_format_close(rtp_fmt);

    testrun(0 == rtp_fmt);

    /*************************************************************************
                                 With CSRCS set
     ************************************************************************/

    uint32_t ref_csrcs[] = {

        11111,
        22222,
        33333,
        44444,

    };

    size_t ref_num_csrcs = sizeof(ref_csrcs) / sizeof(ref_csrcs[0]);

    ov_rtp_frame_expansion exp = rtp_frame->expanded;

    rtp_frame = rtp_frame->free(rtp_frame);
    testrun(0 == rtp_frame);

    /* Payload in expansion has been invalidated ... */

    const uint8_t ref_payload[] = {9, 1, 8, 2, 7, 3, 6, 4, 5};
    exp.payload.data = (uint8_t *)ref_payload;
    exp.payload.length = sizeof(ref_payload);

    exp.csrc_ids = ref_csrcs;
    exp.csrc_count = ref_num_csrcs;

    rtp_frame = get_encoded_frame(&exp);
    testrun(0 != rtp_frame);

    mem_fmt = ov_format_from_memory(rtp_frame->bytes.data,
                                    rtp_frame->bytes.length, OV_READ);

    testrun(0 != mem_fmt);

    rtp_fmt = ov_format_as(mem_fmt, "rtp", 0, 0);
    testrun(0 != rtp_fmt);
    mem_fmt = 0;

    testrun(ov_format_rtp_get_contributing_sources(rtp_fmt, &csrcs));

    testrun(0 == csrcs.num);

    buf = ov_format_payload_read_chunk_nocopy(rtp_fmt, 0);
    testrun(0 != buf.start);

    testrun(ov_format_rtp_get_contributing_sources(rtp_fmt, &csrcs));

    testrun(ref_num_csrcs == csrcs.num);

    for (size_t i = 0; ref_num_csrcs > i; ++i) {

        testrun(ref_csrcs[i] == csrcs.ids[i]);
    }

    buf.start = 0;

    rtp_fmt = ov_format_close(rtp_fmt);

    testrun(0 == rtp_fmt);

    /*************************************************************************
                                    Cleanup
     ************************************************************************/

    rtp_frame = rtp_frame->free(rtp_frame);

    ov_format_registry_clear(0);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_format_rtp_get_ext_header() {

    testrun(!ov_format_rtp_get_extension_header(0, 0));

    /*************************************************************************
                              Initialize test data
     ************************************************************************/

    testrun(ov_format_rtp_install(0));

    ov_rtp_frame *rtp_frame = get_encoded_frame(0);
    testrun(0 != rtp_frame);

    ov_format *mem_fmt = ov_format_from_memory(
        rtp_frame->bytes.data, rtp_frame->bytes.length, OV_READ);

    testrun(0 != mem_fmt);

    ov_format *rtp_fmt = ov_format_as(mem_fmt, "rtp", 0, 0);
    testrun(0 != rtp_fmt);
    mem_fmt = 0;

    testrun(!ov_format_rtp_get_extension_header(rtp_fmt, 0));

    ov_format_rtp_extension_header ext_header = {0};

    testrun(!ov_format_rtp_get_extension_header(0, &ext_header));

    testrun(!ov_format_rtp_get_extension_header(rtp_fmt, &ext_header));

    testrun(0 == ext_header.length_4_octets);
    testrun(0 == ext_header.payload);
    testrun(0 == ext_header.id);

    ov_buffer buf = ov_format_payload_read_chunk_nocopy(rtp_fmt, 0);

    testrun(rtp_frame->expanded.payload.length == buf.length);
    testrun(0 ==
            memcmp(rtp_frame->expanded.payload.data, buf.start, buf.length));

    rtp_fmt = ov_format_close(rtp_fmt);
    testrun(0 == rtp_fmt);

    /*************************************************************************
                         Test with extension header set
     ************************************************************************/

    char const ref_payload_str[] = "myinstenstinesarefullofSpanEggsSpamspam";
    uint8_t *ref_payload = (uint8_t *)ref_payload_str;

    const size_t ref_len = sizeof(ref_payload_str);

    /* length must be some multiple of 4 ... */
    char const ref_ext_str[] = "greatboobieshunnybun";
    uint8_t *ref_ext = (uint8_t *)ref_ext_str;

    /* Still need some multiple of 4 as length */
    const size_t ref_len_ext = sizeof(ref_ext_str) - 1;

    const uint16_t ref_id = 42;

    ov_rtp_frame_expansion exp = rtp_frame->expanded;
    exp.payload.data = ref_payload;
    exp.payload.length = ref_len;

    exp.extension_bit = true;

    exp.extension.allocated_bytes = 0;
    exp.extension.data = (uint8_t *)ref_ext;
    exp.extension.length = ref_len_ext;
    exp.extension.type = ref_id;

    rtp_frame = rtp_frame->free(rtp_frame);
    testrun(0 == rtp_frame);

    rtp_frame = get_encoded_frame(&exp);
    testrun(0 != rtp_frame);

    mem_fmt = ov_format_from_memory(rtp_frame->bytes.data,
                                    rtp_frame->bytes.length, OV_READ);

    testrun(0 != mem_fmt);

    rtp_fmt = ov_format_as(mem_fmt, "rtp", 0, 0);
    testrun(0 != rtp_fmt);
    mem_fmt = 0;

    testrun(!ov_format_rtp_get_extension_header(rtp_fmt, 0));

    testrun(!ov_format_rtp_get_extension_header(0, &ext_header));

    testrun(!ov_format_rtp_get_extension_header(rtp_fmt, &ext_header));

    testrun(0 == ext_header.length_4_octets);
    testrun(0 == ext_header.payload);
    testrun(0 == ext_header.id);

    buf = ov_format_payload_read_chunk_nocopy(rtp_fmt, 0);

    testrun(ref_len == buf.length);
    testrun(0 == memcmp(ref_payload, buf.start, buf.length));

    testrun(ov_format_rtp_get_extension_header(rtp_fmt, &ext_header));
    testrun(ref_len_ext == sizeof(uint32_t) * ext_header.length_4_octets);
    testrun(0 == memcmp(ext_header.payload, ref_ext, ref_len_ext));
    testrun(ref_id == ext_header.id);

    testrun(0 == memcmp(ref_ext, ext_header.payload, ref_len_ext));

    rtp_fmt = ov_format_close(rtp_fmt);
    testrun(0 == rtp_fmt);

    rtp_frame = rtp_frame->free(rtp_frame);
    testrun(0 == rtp_frame);

    /*************************************************************************
                                    Cleanup
     ************************************************************************/

    ov_format_registry_clear(0);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_format_rtp_get_padding() {

    testrun(0 == ov_format_rtp_get_padding(0));

    /*************************************************************************
                              Initialize test data
     ************************************************************************/

    testrun(ov_format_rtp_install(0));

    ov_rtp_frame *rtp_frame = get_encoded_frame(0);
    testrun(0 != rtp_frame);

    ov_format *mem_fmt = ov_format_from_memory(
        rtp_frame->bytes.data, rtp_frame->bytes.length, OV_READ);

    testrun(0 != mem_fmt);

    ov_format *rtp_fmt = ov_format_as(mem_fmt, "rtp", 0, 0);
    testrun(0 != rtp_fmt);
    mem_fmt = 0;

    testrun(0 == ov_format_rtp_get_padding(0));

    ov_buffer *padding = ov_format_rtp_get_padding(rtp_fmt);

    testrun(OV_FORMAT_RTP_NO_PADDING == padding);
    testrun(0 == padding->length);
    testrun(0 == padding->start);

    ov_buffer buf = ov_format_payload_read_chunk_nocopy(rtp_fmt, 0);

    padding = ov_format_rtp_get_padding(rtp_fmt);

    testrun(OV_FORMAT_RTP_NO_PADDING == padding);
    testrun(0 == padding->length);
    testrun(0 == padding->start);

    testrun(rtp_frame->expanded.payload.length == buf.length);
    testrun(0 ==
            memcmp(rtp_frame->expanded.payload.data, buf.start, buf.length));

    rtp_fmt = ov_format_close(rtp_fmt);
    testrun(0 == rtp_fmt);

    ov_rtp_frame_expansion exp = rtp_frame->expanded;

    /*************************************************************************
                                One byte padding
     ************************************************************************/

    const char ref_padding_str[] = "spamSpamSpamSpamSpammadySpam";

    uint8_t *ref_padding = (uint8_t *)ref_padding_str;

    const size_t ref_padding_len = sizeof(ref_padding_str);

    const char ref_payload_str[] = "myinstenstinesarefullofSpanEggsSpamspam";
    uint8_t *ref_payload = (uint8_t *)ref_payload_str;
    const size_t ref_payload_len = sizeof(ref_payload_str) - 1;

    exp.payload.data = ref_payload;
    exp.payload.length = ref_payload_len;

    exp.padding_bit = true;
    exp.padding.data = ref_padding;
    exp.padding.length = 1;
    ov_rtp_frame *rtp_frame_padded = get_encoded_frame(&exp);
    testrun(0 != rtp_frame_padded);

    mem_fmt = ov_format_from_memory(rtp_frame_padded->bytes.data,
                                    rtp_frame_padded->bytes.length, OV_READ);

    testrun(0 != mem_fmt);

    rtp_fmt = ov_format_as(mem_fmt, "rtp", 0, 0);
    testrun(0 != rtp_fmt);
    mem_fmt = 0;

    testrun(0 == ov_format_rtp_get_padding(0));

    padding = ov_format_rtp_get_padding(rtp_fmt);

    testrun(0 != padding);
    testrun(OV_FORMAT_RTP_NO_PADDING == padding);
    testrun(0 == padding->length);
    testrun(0 == padding->start);

    buf = ov_format_payload_read_chunk_nocopy(rtp_fmt, 0);

    padding = ov_format_rtp_get_padding(rtp_fmt);

    testrun(0 != padding);
    testrun(OV_FORMAT_RTP_NO_PADDING != padding);

    /* the padding length includes the padding length octet!!! */
    testrun(1 + 1 == padding->length);
    testrun((uint8_t)ref_padding[0] == padding->start[0]);
    testrun(2 == padding->start[1]);

    testrun(ref_payload_len == exp.payload.length);
    testrun(0 == memcmp(exp.payload.data, ref_payload, ref_payload_len));

    rtp_fmt = ov_format_close(rtp_fmt);
    testrun(0 == rtp_fmt);

    padding = ov_buffer_free(padding);
    testrun(0 == padding);

    rtp_frame_padded = rtp_frame_padded->free(rtp_frame_padded);
    testrun(0 == rtp_frame_padded);

    /*************************************************************************
                             Several bytes padding
     ************************************************************************/

    exp.padding_bit = true;
    exp.padding.data = ref_padding;
    exp.padding.length = ref_padding_len;
    rtp_frame_padded = get_encoded_frame(&exp);
    testrun(0 != rtp_frame_padded);

    mem_fmt = ov_format_from_memory(rtp_frame_padded->bytes.data,
                                    rtp_frame_padded->bytes.length, OV_READ);

    testrun(0 != mem_fmt);

    rtp_fmt = ov_format_as(mem_fmt, "rtp", 0, 0);
    testrun(0 != rtp_fmt);
    mem_fmt = 0;

    testrun(0 == ov_format_rtp_get_padding(0));

    padding = ov_format_rtp_get_padding(rtp_fmt);

    testrun(0 != padding);
    testrun(OV_FORMAT_RTP_NO_PADDING == padding);
    testrun(0 == padding->length);
    testrun(0 == padding->start);

    buf = ov_format_payload_read_chunk_nocopy(rtp_fmt, 0);

    padding = ov_format_rtp_get_padding(rtp_fmt);

    testrun(0 != padding);

    /* the padding length includes the padding length octet!!! */
    testrun(1 + ref_padding_len == padding->length);
    testrun(1 + ref_padding_len == padding->start[padding->length - 1]);

    testrun(0 == memcmp(padding->start, ref_padding, ref_padding_len));

    testrun(ref_payload_len == exp.payload.length);
    testrun(0 == memcmp(exp.payload.data, ref_payload, ref_payload_len));

    rtp_fmt = ov_format_close(rtp_fmt);
    testrun(0 == rtp_fmt);

    padding = ov_buffer_free(padding);
    testrun(0 == padding);

    rtp_frame_padded = rtp_frame_padded->free(rtp_frame_padded);
    testrun(0 == rtp_frame_padded);

    /*************************************************************************
                                    Cleanup
     ************************************************************************/

    rtp_frame = rtp_frame->free(rtp_frame);
    testrun(0 == rtp_frame);

    ov_format_registry_clear(0);
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

OV_TEST_RUN("ov_format_rtp", test_ov_format_rtp_install, test_impl_next_chunk,
            test_ov_format_rtp_get_header,
            test_ov_format_rtp_get_contributing_sources,
            test_ov_format_rtp_get_ext_header, test_ov_format_rtp_get_padding);
