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

#include <ov_codec/ov_codec.h>

#include "../include/ov_format_codec.h"
#include "assert.h"

uint32_t MAGIC_BYTES = 0x1A21BC01;

/*----------------------------------------------------------------------------*/

typedef struct {

  uint32_t magic_bytes;
  ov_codec *codec;

  ov_buffer *out_buffer;

  // emulated sequence numbers
  uint64_t sequence_number;

} codec_data;

/*----------------------------------------------------------------------------*/

static codec_data *as_codec_data(void *data) {

  if (0 == data)
    return 0;

  codec_data *cdata = data;

  if (MAGIC_BYTES != cdata->magic_bytes)
    return 0;

  return cdata;
}

/*----------------------------------------------------------------------------*/

static void *impl_create_data(ov_format *f, void *options) {

  UNUSED(f);

  if (0 == options) {

    ov_log_error("No codec given");
    goto error;
  }

  ov_codec *codec = options;
  codec_data *cdata = calloc(1, sizeof(codec_data));
  OV_ASSERT(0 != cdata);

  cdata->magic_bytes = MAGIC_BYTES;
  cdata->codec = codec;

  return cdata;

error:

  return 0;
}

/*----------------------------------------------------------------------------*/

static void *impl_free_data(void *data) {

  codec_data *cdata = as_codec_data(data);

  if (0 == cdata) {

    ov_log_error("Not a %s format", OV_FORMAT_CODEC_TYPE_STRING);
    goto error;
  }

  cdata->codec = ov_codec_free(cdata->codec);

  if (0 != cdata->codec) {

    ov_log_error("Could not free codec");
  }

  cdata->out_buffer = ov_buffer_free(cdata->out_buffer);

  if (0 != cdata->out_buffer) {

    ov_log_error("Could not free output out_buffer");
  }

  cdata = ov_free(cdata);

  data = 0;

error:

  return data;
}

/*----------------------------------------------------------------------------*/

static ov_buffer *adjust_buffer_size_to(ov_buffer *buffer, size_t size) {

  if ((0 != buffer) && (size > buffer->capacity)) {

    buffer = ov_buffer_free(buffer);
  }

  if (0 == buffer) {

    buffer = ov_buffer_create(size);
  }

  return buffer;
}

/*----------------------------------------------------------------------------*/

static ov_buffer impl_next_chunk(ov_format *f, size_t requested_bytes,
                                 void *data) {

  /*
   * Non-optimal implementation: Better would be to
   * buffer result and return only the amount of bytes actually requested
   */

  if (0 == f) {

    ov_log_error("No format to write to");
    goto error;
  }

  codec_data *cdata = as_codec_data(data);

  if (0 == cdata) {

    ov_log_error("Not a %s format", OV_FORMAT_CODEC_TYPE_STRING);
    goto error;
  }

  ov_codec *codec = cdata->codec;

  if (0 == codec) {

    ov_log_error("No codec given");
    goto error;
  }

  ov_buffer in = ov_format_payload_read_chunk_nocopy(f, requested_bytes);

  if (0 == in.start) {

    ov_log_error("Could not read from format");
    goto error;
  }

  cdata->out_buffer =
      adjust_buffer_size_to(cdata->out_buffer, requested_bytes * 20);

  int32_t bytes_out =
      codec->decode(codec, cdata->sequence_number, in.start, in.length,
                    cdata->out_buffer->start, cdata->out_buffer->capacity);

  if (0 > bytes_out) {

    ov_log_error("Could not decode");
    goto error;
  }

  ++cdata->sequence_number;
  cdata->out_buffer->length = (size_t)bytes_out;

  return *cdata->out_buffer;

error:

  return (ov_buffer){0};
}

/*----------------------------------------------------------------------------*/

static ssize_t impl_write_chunk(ov_format *f, ov_buffer const *chunk,
                                void *data) {

  ssize_t retval = -1;
  ov_buffer *out = 0;

  if (0 == f) {

    ov_log_error("No format to write to");
    goto error;
  }

  if (0 == chunk) {

    ov_log_error("No chunk to write given");
    goto error;
  }

  codec_data *cdata = as_codec_data(data);

  if (0 == cdata) {

    ov_log_error("Not a %s format", OV_FORMAT_CODEC_TYPE_STRING);
    goto error;
  }

  ov_codec *codec = cdata->codec;

  if (0 == codec) {

    ov_log_error("No codec given");
    goto error;
  }

  out = ov_buffer_create(chunk->length);

  OV_ASSERT(0 != out);

  ssize_t bytes_written = codec->encode(codec, chunk->start, chunk->length,
                                        out->start, out->capacity);

  if (0 > bytes_written) {

    ov_log_error("Could not encode");
    goto error;
  }

  out->length = (size_t)bytes_written;

  retval = ov_format_payload_write_chunk(f, out);

error:

  if (0 != out) {

    out = ov_buffer_free(out);
  }

  OV_ASSERT(0 == out);

  return retval;
}

/*----------------------------------------------------------------------------*/

bool ov_format_codec_install(ov_format_registry *registry) {

  ov_format_handler codec_handler = {

      .next_chunk = impl_next_chunk,
      .write_chunk = impl_write_chunk,
      .create_data = impl_create_data,
      .free_data = impl_free_data,

  };

  return ov_format_registry_register_type(OV_FORMAT_CODEC_TYPE_STRING,
                                          codec_handler, registry);
}

/*----------------------------------------------------------------------------*/
