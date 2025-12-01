/***
        ------------------------------------------------------------------------

        Copyright (c) 2021 German Aerospace Center DLR e.V. (GSOC)

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
#include "ov_format_codec.c"

#include <ov_codec/ov_codec_pcm16_signed.h>

#include <ov_test/ov_test.h>

/*----------------------------------------------------------------------------*/

static int test_ov_format_codec_install() {

  testrun(ov_format_codec_install(0));

  ov_format_registry *registry = ov_format_registry_create();

  testrun(ov_format_codec_install(registry));

  ov_format_registry_clear(0);

  ov_format_registry_clear(registry);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_impl_next_chunk() {

  ov_format_codec_install(0);

  ov_buffer test_in = {

      .start = (uint8_t *)"abcdef",
      .length = 6,

  };

  ov_codec_factory *cf = ov_codec_factory_create_standard();

  ov_codec *pcm16s =
      ov_codec_factory_get_codec(cf, ov_codec_pcm16_signed_id(), 1, 0);
  testrun(0 != pcm16s);

  ov_format *mem_fmt =
      ov_format_from_memory(test_in.start, test_in.length, OV_READ);
  testrun(0 != mem_fmt);

  testrun(0 == ov_format_as(mem_fmt, OV_FORMAT_CODEC_TYPE_STRING, 0, 0));

  ov_format *codec_fmt =
      ov_format_as(mem_fmt, OV_FORMAT_CODEC_TYPE_STRING, pcm16s, 0);

  testrun(0 != codec_fmt);
  mem_fmt = 0;
  pcm16s = 0;

  ov_buffer read_buf =
      ov_format_payload_read_chunk_nocopy(codec_fmt, test_in.length);

  testrun(0 != read_buf.start);
  testrun(test_in.length == read_buf.length);

  testrun(0 == memcmp(read_buf.start, "badcfe", test_in.length));

  codec_fmt = ov_format_close(codec_fmt);
  testrun(0 == codec_fmt);

  cf = ov_codec_factory_free(cf);
  testrun(0 == cf);

  ov_format_registry_clear(0);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int test_impl_write_chunk() {

  ov_format_codec_install(0);

  ov_buffer test_in = {

      .start = (uint8_t *)"abcdef",
      .length = 6,

  };

  ov_codec_factory *cf = ov_codec_factory_create_standard();

  ov_codec *pcm16s =
      ov_codec_factory_get_codec(cf, ov_codec_pcm16_signed_id(), 1, 0);
  testrun(0 != pcm16s);

  ov_format *mem_fmt = ov_format_from_memory(0, 1, OV_WRITE);
  testrun(0 != mem_fmt);

  ov_format *codec_fmt =
      ov_format_as(mem_fmt, OV_FORMAT_CODEC_TYPE_STRING, pcm16s, 0);

  testrun(0 != codec_fmt);
  mem_fmt = 0;
  pcm16s = 0;

  ssize_t bytes_written = ov_format_payload_write_chunk(codec_fmt, &test_in);

  testrun(0 < bytes_written);
  testrun(test_in.length == (size_t)bytes_written);

  ov_buffer mem_buf = ov_format_get_memory(codec_fmt);

  testrun(0 != mem_buf.start);
  testrun(test_in.length == mem_buf.length);

  testrun(0 == memcmp(mem_buf.start, "badcfe", test_in.length));

  codec_fmt = ov_format_close(codec_fmt);
  testrun(0 == codec_fmt);

  cf = ov_codec_factory_free(cf);
  testrun(0 == cf);

  ov_format_registry_clear(0);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

OV_TEST_RUN("ov_format_codec", test_ov_format_codec_install,
            test_impl_next_chunk, test_impl_write_chunk);
