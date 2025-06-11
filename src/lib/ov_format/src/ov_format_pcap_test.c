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

#include "ov_format_pcap.c"

#include <ov_test/ov_test.h>

/*----------------------------------------------------------------------------*/

char const *TEST_PCAP_FILE = "resources/pcap/test_ipv4_tcp.pcap";

/*----------------------------------------------------------------------------*/

static int test_ov_format_pcap_install() {

    testrun(ov_format_pcap_install(0));

    TODO("Test whether we can acutally create a 'pcap' format");

    ov_format_registry_clear(0);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_impl_next_chunk() {

    char *test_pcap_file_path = ov_test_get_resource_path(TEST_PCAP_FILE);
    testrun(0 != test_pcap_file_path);

    ov_format *fmt = ov_format_open(test_pcap_file_path, OV_READ);
    testrun(0 != fmt);

    ov_format_pcap_install(0);

    ov_format *pcap_fmt = ov_format_as(fmt, "pcap", 0, 0);
    fmt = 0;

    ov_buffer *buffer = ov_format_payload_read_chunk(pcap_fmt, 0);

    /* Expect at least  1 packet */
    testrun(0 != buffer);

    while (0 != buffer) {

        ov_format_pcap_packet_header phdr = {0};

        testrun(ov_format_pcap_get_current_packet_header(pcap_fmt, &phdr));

        ov_log_info("packet length acc. to packet header: %" PRIu32
                    ", got "
                    "%zu\n",
                    phdr.length_stored_bytes,
                    buffer->length);

        testrun(phdr.length_stored_bytes == buffer->length);

        buffer = ov_buffer_free(buffer);
        testrun(0 == buffer);

        buffer = ov_format_payload_read_chunk(pcap_fmt, 0);
    };

    testrun(0 == buffer);

    /*************************************************************************
                                    Cleanup
     ************************************************************************/

    pcap_fmt = ov_format_close(pcap_fmt);
    testrun(0 == pcap_fmt);

    free(test_pcap_file_path);

    ov_format_registry_clear(0);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_format_pcap_get_global_header() {

    char *test_pcap_file_path = ov_test_get_resource_path(TEST_PCAP_FILE);
    testrun(0 != test_pcap_file_path);

    ov_format *fmt = ov_format_open(test_pcap_file_path, OV_READ);
    testrun(0 != fmt);

    ov_format_pcap_global_header ghdr = {0};

    testrun(!ov_format_pcap_get_global_header(fmt, &ghdr));

    ov_format_pcap_install(0);

    ov_format *pcap_fmt = ov_format_as(fmt, "pcap", 0, 0);
    fmt = 0;

    testrun(!ov_format_pcap_get_global_header(pcap_fmt, 0));
    testrun(ov_format_pcap_get_global_header(pcap_fmt, &ghdr));

    pcap_fmt = ov_format_close(pcap_fmt);
    testrun(0 == pcap_fmt);

    free(test_pcap_file_path);

    ov_format_registry_clear(0);

    ov_format_pcap_print_global_header(stderr, &ghdr);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_format_pcap_get_current_packet_header() {

    testrun(!ov_format_pcap_get_current_packet_header(0, 0));

    ov_format_pcap_packet_header phdr = {0};

    testrun(!ov_format_pcap_get_current_packet_header(0, &phdr));

    testrun(0 == phdr.timestamp_secs);
    testrun(0 == phdr.timestamp_usecs);
    testrun(0 == phdr.length_origin_bytes);
    testrun(0 == phdr.length_stored_bytes);

    char *test_pcap_file_path = ov_test_get_resource_path(TEST_PCAP_FILE);
    testrun(0 != test_pcap_file_path);

    ov_format_pcap_install(0);

    ov_format *fmt = ov_format_open(test_pcap_file_path, OV_READ);
    testrun(0 != fmt);

    testrun(!ov_format_pcap_get_current_packet_header(fmt, &phdr));

    testrun(0 == phdr.timestamp_secs);
    testrun(0 == phdr.timestamp_usecs);
    testrun(0 == phdr.length_origin_bytes);
    testrun(0 == phdr.length_stored_bytes);

    ov_format *pcap_fmt = ov_format_as(fmt, "pcap", 0, 0);
    fmt = 0;

    testrun(!ov_format_pcap_get_current_packet_header(pcap_fmt, 0));

    testrun(0 == phdr.timestamp_secs);
    testrun(0 == phdr.timestamp_usecs);
    testrun(0 == phdr.length_origin_bytes);
    testrun(0 == phdr.length_stored_bytes);

    testrun(ov_format_pcap_get_current_packet_header(pcap_fmt, &phdr));

    testrun(0 == phdr.timestamp_secs);
    testrun(0 == phdr.timestamp_usecs);
    testrun(0 == phdr.length_origin_bytes);
    testrun(0 == phdr.length_stored_bytes);

    ov_buffer *chunk = ov_format_payload_read_chunk(pcap_fmt, 1);
    testrun(0 != chunk);

    testrun(ov_format_pcap_get_current_packet_header(pcap_fmt, &phdr));

    testrun(0 != phdr.length_origin_bytes);
    testrun(chunk->length == phdr.length_stored_bytes);

    chunk = ov_buffer_free(chunk);
    testrun(0 == chunk);

    pcap_fmt = ov_format_close(pcap_fmt);
    testrun(0 == pcap_fmt);

    free(test_pcap_file_path);

    ov_format_registry_clear(0);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

OV_TEST_RUN("ov_format_pcap",
            test_ov_format_pcap_install,
            test_impl_next_chunk,
            test_ov_format_pcap_get_global_header,
            test_ov_format_pcap_get_current_packet_header);

/*----------------------------------------------------------------------------*/
