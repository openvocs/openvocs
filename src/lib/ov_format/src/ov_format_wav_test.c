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
#include "ov_format_wav.c"

#include <ov_arch/ov_byteorder.h>
#include <ov_test/ov_test.h>

/*----------------------------------------------------------------------------*/

char const *TEST_WAV_FILE = "resources/wav/test.wav";

/*----------------------------------------------------------------------------*/

static ov_buffer *get_wav(ov_buffer const *payload,
                          ov_format_wav_options *options,
                          uint32_t data_rate,
                          size_t num_chunks_before_fmt,
                          size_t num_chunks_after_fmt) {

    OV_ASSERT(0 != payload);
    UNUSED(options);
    UNUSED(num_chunks_before_fmt);
    UNUSED(num_chunks_after_fmt);

    if (0 == data_rate) {

        /* If not given explicitly, set correct value */
        data_rate = options->samplerate_hz * options->blockAlignmentBytes;
    }

    size_t out_length = MASTER_RIFF_CHUNK_LENGTH + DATA_RIFF_HEADER_LENGTH +
                        FMT_CHUNK_LENGTH + payload->length;

    testrun_log(
        "get_wav: length master: %zu   fmt: %zu   data:%zu   "
        "payload length: %zu,    out length: %zu",
        MASTER_RIFF_CHUNK_LENGTH,
        FMT_CHUNK_LENGTH,
        DATA_RIFF_HEADER_LENGTH,
        payload->length,
        out_length);

    OV_ASSERT(FMT_CHUNK_LENGTH == 8 + 16);

    ov_buffer *out = ov_buffer_create(out_length);
    size_t out_length_remaining = out_length;

    OV_ASSERT(0 != out);

    uint8_t *ptr = out->start;

    memcpy(ptr, "RIFF", 4);
    ptr += 4;
    out_length_remaining -= 4;

    ov_file_write_32(&ptr, &out_length_remaining, OV_H16TOLE(out_length - 8));

    memcpy(ptr, "WAVEfmt ", 8);
    ptr += 8;
    out_length_remaining -= 8;

    ov_file_write_32(
        &ptr, &out_length_remaining, OV_H32TOLE(FMT_CHUNK_LENGTH - 8));

    ov_file_write_16(&ptr, &out_length_remaining, OV_H16TOLE(options->format));

    ov_file_write_16(
        &ptr, &out_length_remaining, OV_H16TOLE(options->channels));

    ov_file_write_32(
        &ptr, &out_length_remaining, OV_H32TOLE(options->samplerate_hz));

    ov_file_write_32(&ptr, &out_length_remaining, OV_H32TOLE(data_rate));

    ov_file_write_16(
        &ptr, &out_length_remaining, OV_H16TOLE(options->blockAlignmentBytes));

    ov_file_write_16(
        &ptr, &out_length_remaining, OV_H16TOLE(options->bitsPerSample));

    memcpy(ptr, "data", 4);

    ptr += 4;
    out_length_remaining -= 4;

    ov_file_write_32(&ptr, &out_length_remaining, OV_H32TOLE(payload->length));

    memcpy(ptr, payload->start, payload->length);

    OV_ASSERT((ptr - out->start) + payload->length == out_length);

    out->length = out_length;

    return out;
}

/*----------------------------------------------------------------------------*/

static int test_ov_format_wav_install() {

    ov_buffer ref_payload = {
        .start = (uint8_t *)"abcde",
        .length = 6,
    };

    ov_format_wav_options options = {
        .format = OV_FORMAT_WAV_PCM,
        .channels = 1,
        .samplerate_hz = 44100,
        .blockAlignmentBytes = 1,
        .bitsPerSample = 8,
    };

    ov_buffer *frame = get_wav(&ref_payload, &options, 0, 0, 0);

    testrun(0 != frame);
    testrun(0 != frame->start);
    testrun(0 != frame->length);

    ov_format *fmt =
        ov_format_from_memory(frame->start, frame->length, OV_READ);
    testrun(0 != fmt);

    ov_format *wav_fmt = ov_format_as(fmt, "wav", 0, 0);
    testrun(0 == wav_fmt);

    ov_format_wav_install(0);

    wav_fmt = ov_format_as(fmt, "wav", 0, 0);
    testrun(0 != wav_fmt);

    fmt = 0;

    wav_fmt = ov_format_close(wav_fmt);
    testrun(0 == wav_fmt);

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

    ov_format_wav_options options = {
        .format = OV_FORMAT_WAV_PCM,
        .channels = 1,
        .samplerate_hz = 44100,
        .blockAlignmentBytes = 2,
        .bitsPerSample = 16,

    };

    ov_buffer *frame = get_wav(&ref_payload, &options, 0, 0, 0);

    ov_format *mem_fmt =
        ov_format_from_memory(frame->start, frame->length, OV_READ);

    testrun(0 != mem_fmt);

    testrun(0 == ov_format_as(mem_fmt, "wav", 0, 0));

    testrun(ov_format_wav_install(0));

    ov_format *wav_fmt = ov_format_as(mem_fmt, "wav", 0, 0);
    testrun(0 != wav_fmt);

    mem_fmt = 0;

    /*************************************************************************
                                Test simple WAV
     ************************************************************************/

    ov_buffer buf = ov_format_payload_read_chunk_nocopy(wav_fmt, frame->length);

    testrun(buf.length == ref_payload.length);
    testrun(0 == memcmp(buf.start, ref_payload.start, buf.length));

    wav_fmt = ov_format_close(wav_fmt);

    testrun(0 == wav_fmt);

    frame = ov_buffer_free(frame);
    testrun(0 == frame);

    /*************************************************************************
                          Test with consecutive reads
     ************************************************************************/

    frame = get_wav(&ref_payload, &options, 0, 0, 0);

    mem_fmt = ov_format_from_memory(frame->start, frame->length, OV_READ);

    testrun(0 != mem_fmt);

    wav_fmt = ov_format_as(mem_fmt, "wav", 0, 0);
    testrun(0 != wav_fmt);

    mem_fmt = 0;

    uint8_t *ref_payload_ptr = ref_payload.start;

    size_t nread = 0;

    while (ov_format_has_more_data(wav_fmt)) {

        buf = ov_format_payload_read_chunk_nocopy(wav_fmt, 2);

        testrun(0 != buf.start);
        testrun(0 != buf.length);

        testrun(0 == memcmp(ref_payload_ptr, buf.start, buf.length));
        ref_payload_ptr += buf.length;
        nread += buf.length;
    }

    testrun(nread == ref_payload.length);

    wav_fmt = ov_format_close(wav_fmt);

    testrun(0 == wav_fmt);

    frame = ov_buffer_free(frame);
    testrun(0 == frame);

    /*************************************************************************
                              Inconsistent options
     ************************************************************************/

    frame = get_wav(&ref_payload, &options, 1020, 0, 0);

    mem_fmt = ov_format_from_memory(frame->start, frame->length, OV_READ);

    testrun(0 != mem_fmt);

    testrun(0 == ov_format_as(mem_fmt, "wav", 0, 0));

    mem_fmt = ov_format_close(mem_fmt);
    testrun(0 == mem_fmt);

    frame = ov_buffer_free(frame);
    testrun(0 == frame);

    options.blockAlignmentBytes = 1;

    frame = get_wav(&ref_payload, &options, 0, 0, 0);

    mem_fmt = ov_format_from_memory(frame->start, frame->length, OV_READ);

    testrun(0 != mem_fmt);

    testrun(0 == ov_format_as(mem_fmt, "wav", 0, 0));

    mem_fmt = ov_format_close(mem_fmt);
    testrun(0 == mem_fmt);

    frame = ov_buffer_free(frame);
    testrun(0 == frame);

    /*************************************************************************
                          Test with real-life wav file
     ************************************************************************/

    char *wav_file = ov_test_get_resource_path(TEST_WAV_FILE);
    testrun(0 != wav_file);

    testrun_log("Trying to read %s", wav_file);

    ov_format *bare = ov_format_open(wav_file, OV_READ);
    testrun(0 != bare);

    wav_fmt = ov_format_as(bare, "wav", 0, 0);
    testrun(0 != wav_fmt);

    testrun(ov_format_wav_get_header(wav_fmt, &options));

    size_t read_bytes = 0;

    while (ov_format_has_more_data(wav_fmt)) {

        ov_buffer buf = ov_format_payload_read_chunk_nocopy(wav_fmt, 512);
        read_bytes += buf.length;
    }

    testrun_log("From %s got %zu bytes payload", wav_file, read_bytes);

    free(wav_file);
    wav_file = 0;

    testrun(768000 == read_bytes);

    wav_fmt = ov_format_close(wav_fmt);
    testrun(0 == wav_fmt);
    bare = 0;

    testrun_log(
        "Bits per sample: %u  Block alignment: %u  Channels: %u"
        "Format: %u   Samplerate (Hz): %u",
        options.bitsPerSample,
        options.blockAlignmentBytes,
        options.channels,
        options.format,
        options.samplerate_hz);

    /*************************************************************************
                                    Cleanup
     ************************************************************************/

    ov_format_registry_clear(0);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_impl_write_chunk() {
    ov_format *mem_fmt = ov_format_from_memory(0, 1, OV_WRITE);
    testrun(0 != mem_fmt);

    ov_format_wav_install(0);

    /* Check with default options */

    ov_format *wav = ov_format_as(mem_fmt, "wav", 0, 0);
    testrun(0 != wav);

    mem_fmt = 0;

    wav = ov_format_close(wav);
    testrun(0 == wav);

    /* Check with custom options */
    mem_fmt = ov_format_from_memory(0, 1, OV_WRITE);
    testrun(0 != mem_fmt);

    ov_format_wav_install(0);

    ov_format_wav_options opts = {
        .format = OV_FORMAT_WAV_PCM,
        .channels = 1,
        .samplerate_hz = 44100,
        .blockAlignmentBytes = 2,
        .bitsPerSample = 12,

    };

    wav = ov_format_as(mem_fmt, "wav", &opts, 0);
    testrun(0 != wav);

    mem_fmt = 0;

    testrun(0 > ov_format_payload_write_chunk(wav, 0));

    ov_buffer chunk = {
        .start = (uint8_t *)"abcde",
        .length = 6,
    };

    ssize_t bytes_written = ov_format_payload_write_chunk(wav, &chunk);
    testrun(0 <= bytes_written);
    testrun(chunk.length == (size_t)bytes_written);

    /* Wave now contains 'abcde\0' */

    chunk.start = (uint8_t *)"   ";
    chunk.length = 4;

    bytes_written = ov_format_payload_write_chunk(wav, &chunk);
    testrun(0 <= bytes_written);
    testrun(chunk.length == (size_t)bytes_written);

    /* Wave now contains 'abcdef\0   \0' */

    chunk.start = (uint8_t *)"fghi";
    chunk.length = 4;

    bytes_written = ov_format_payload_overwrite(wav, 5, &chunk);
    testrun(0 <= bytes_written);
    testrun(chunk.length == (size_t)bytes_written);

    /* We overwrote
     * 'abcdef\0   \0'
     *        ^   ^
     *        -----
     *        f ghi
     * yielding
     *
     * 'abcdefghi\0'
     */

    ov_buffer ref_payload = {
        .start = (uint8_t *)"abcdefghi",
        .length = 10,

    };

    /* Closing the wav now would free its memory, and we still want that ! */

    ov_buffer result_mem = ov_format_get_memory(wav);
    testrun(0 != result_mem.start);
    testrun(0 != result_mem.length);

    mem_fmt =
        ov_format_from_memory(result_mem.start, result_mem.length, OV_READ);
    testrun(0 != mem_fmt);

    ov_format *check_wav = ov_format_as(mem_fmt, "wav", 0, 0);
    testrun(0 != check_wav);

    ov_format_wav_options real_opts = {0};

    testrun(ov_format_wav_get_header(check_wav, &real_opts));

    testrun(opts.format == real_opts.format);
    testrun(opts.channels == real_opts.channels);
    testrun(opts.samplerate_hz == real_opts.samplerate_hz);
    testrun(opts.blockAlignmentBytes == real_opts.blockAlignmentBytes);
    testrun(opts.bitsPerSample == real_opts.bitsPerSample);

    ov_buffer payload = ov_format_payload_read_chunk_nocopy(check_wav, 150);

    testrun(payload.length == ref_payload.length);
    testrun(0 == memcmp(payload.start, ref_payload.start, ref_payload.length));

    check_wav = ov_format_close(check_wav);
    testrun(0 == check_wav);

    wav = ov_format_close(wav);
    testrun(0 == wav);

    testrun(0 == ov_format_registry_clear(0));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_ov_format_wav_get_header() {

    ov_format_wav_options options = {0};

    testrun(!ov_format_wav_get_header(0, &options));

    ov_buffer ref_payload = {
        .start = (uint8_t *)"abcde",
        .length = 6,
    };

    ov_format_wav_options ref_options = {
        .format = OV_FORMAT_WAV_PCM,
        .channels = 1,
        .samplerate_hz = 44100,
        .blockAlignmentBytes = 2,
        .bitsPerSample = 16,

    };

    ov_buffer *frame = get_wav(&ref_payload, &ref_options, 0, 0, 0);

    ov_format *mem_fmt =
        ov_format_from_memory(frame->start, frame->length, OV_READ);

    testrun(0 != mem_fmt);

    testrun(ov_format_wav_install(0));

    ov_format *wav_fmt = ov_format_as(mem_fmt, "wav", 0, 0);
    testrun(0 != wav_fmt);

    mem_fmt = 0;

    testrun(ov_format_wav_get_header(wav_fmt, &options));

    wav_fmt = ov_format_close(wav_fmt);
    testrun(0 == wav_fmt);

    testrun(ref_options.bitsPerSample == options.bitsPerSample);
    testrun(ref_options.blockAlignmentBytes == options.blockAlignmentBytes);
    testrun(ref_options.channels == options.channels);
    testrun(ref_options.format == options.format);
    testrun(ref_options.samplerate_hz == options.samplerate_hz);

    frame = ov_buffer_free(frame);
    testrun(0 == frame);

    /*************************************************************************
                          Test with real-life wav file
     ************************************************************************/

    char *wav_file = ov_test_get_resource_path(TEST_WAV_FILE);
    testrun(0 != wav_file);

    testrun_log("Trying to read %s", wav_file);

    ov_format *bare = ov_format_open(wav_file, OV_READ);
    testrun(0 != bare);

    free(wav_file);
    wav_file = 0;

    wav_fmt = ov_format_as(bare, "wav", 0, 0);
    testrun(0 != wav_fmt);

    testrun(ov_format_wav_get_header(wav_fmt, &options));

    wav_fmt = ov_format_close(wav_fmt);
    testrun(0 == wav_fmt);
    bare = 0;

    testrun_log(
        "Bits per sample: %u  Block alignment: %u  Channels: %u"
        "Format: %u   Samplerate (Hz): %u",
        options.bitsPerSample,
        options.blockAlignmentBytes,
        options.channels,
        options.format,
        options.samplerate_hz);

    testrun(16 == options.bitsPerSample);
    testrun(2 == options.blockAlignmentBytes);
    testrun(1 == options.channels);
    testrun(1 == options.format);
    testrun(48000 == options.samplerate_hz);

    /*************************************************************************
                                    CLEANUP
     ************************************************************************/

    ov_format_registry_clear(0);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

OV_TEST_RUN("ov_format_wav",
            test_ov_format_wav_install,
            test_impl_next_chunk,
            test_impl_write_chunk,
            test_ov_format_wav_get_header);
