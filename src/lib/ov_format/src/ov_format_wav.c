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


        WAV format, see

        https://web.archive.org/web/20080113195252/http://www.borg.com/~jglatt/tech/wave.htm

        or:

        http://www-mmsp.ece.mcgill.ca/Documents/AudioFormats/WAVE/Docs/riffmci.pdf p56 - p65

        In short:

        * all is little - endian
        * RIFF file: Main RIFF header followed by RIFF chunks
        * RIFF chunk: 4 chars type identifier + 4 bytes data length + data
          example:  data - chunk : |data|0005|12345|
                                     ^    ^     ^
                                     |    |     |
                 chunk type id: "data"    |     payload: 5 bytes
                           payload length (4 octets, little endian)

        * Main RIFF header

            |---|---|---|---||---         4 Bytes         ---||---|---|---|---||
              R   I   F   F || Length of remainder in octets || W   A   V   E ||

        * Mandatory riff chunks: "fmt ", "data"
        * Other riff chunks are possible, should be ignored if unknown
        * Order of riff chunks arbitrary, with exception:
        * "fmt " must precede "data" chunk
        * "data" chunk: See example above
        * "fmt " chunk:


        ------------------------------------------------------------------------
*/

#include "../include/ov_format_wav.h"
#include <ov_arch/ov_byteorder.h>
#include <ov_base/ov_constants.h>
#include <ov_base/ov_file.h>
#include <ov_base/ov_utils.h>

/*----------------------------------------------------------------------------*/

char const *OV_FORMAT_WAV_TYPE_STRING = "wav";

/*----------------------------------------------------------------------------*/

static ov_format_wav_options default_options = {

    .bitsPerSample = 16,
    .blockAlignmentBytes = 2,
    .channels = 1,
    .format = OV_FORMAT_WAV_PCM,
    .samplerate_hz = OV_DEFAULT_SAMPLERATE,

};

/*----------------------------------------------------------------------------*/

typedef struct {

  uint32_t magic_bytes;

  ov_format_wav_options parameters;

  struct {

    size_t length_bytes;

    union {
      size_t bytes_read;
      size_t bytes_written;
    };

    // Start of payload in lower layer
    size_t offset;

  } payload;

} wave_data;

#define WAV_MAGIC_BYTES 0x010203aa

/*----------------------------------------------------------------------------*/

static wave_data *as_wave_data(void *data) {

  if (0 == data)
    return 0;

  wave_data *wdata = data;

  if (WAV_MAGIC_BYTES != wdata->magic_bytes)
    return wdata;

  return wdata;
}

/*****************************************************************************
                                      RIFF
 ****************************************************************************/

struct riff_chunk {

  /* ID + trailing 0 */
  char id[4 + 1];
  uint32_t length;
  uint8_t *data;
};

static bool riff_chunk_header_read(uint8_t **in, size_t *length_octets,
                                   struct riff_chunk *chunk) {

  OV_ASSERT(0 != in);
  OV_ASSERT(0 != length_octets);
  OV_ASSERT(0 != chunk);

  OV_ASSERT(0 != *in);

  uint8_t *pos = *in;

  size_t len = *length_octets;

  if (4 > len) {

    ov_log_error("Too few data to form a riff chunk");
    goto error;
  }

  memcpy(chunk->id, pos, 4);
  pos += 4;
  len -= 4;

  chunk->id[4] = 0;

  bool ok = ov_file_get_32(&chunk->length, &pos, &len, OV_FILE_LITTLE_ENDIAN);

  if (!ok) {
    ov_log_error("Could not read RIFF chunk length");
    goto error;
  }

  OV_ASSERT(0 != chunk->length);

  chunk->data = pos;

  *length_octets = len;
  *in = pos;

  return true;

error:

  return false;
}

/*----------------------------------------------------------------------------*/

static bool riff_chunk_header_read_from_format(ov_format *f,
                                               struct riff_chunk *chunk) {

  OV_ASSERT(0 != f);
  OV_ASSERT(0 != chunk);

  ov_buffer in = ov_format_payload_read_chunk_nocopy(f, 8);

  if ((0 == in.start) || (8 > in.length)) {

    ov_log_error("Could not read sufficient data for RIFF header");
    goto error;
  }

  return riff_chunk_header_read(&in.start, &in.length, chunk);

error:

  return false;
}

/*----------------------------------------------------------------------------*/

static ssize_t read_riff_wav(ov_format *f) {

  ov_buffer in = ov_format_payload_read_chunk_nocopy(f, 12);

  if ((0 == in.start) || (12 > in.length)) {

    ov_log_error("could not get sufficient data for RIFF-WAV header");
    goto error;
  }

  struct riff_chunk riff_wav = {0};

  if (!riff_chunk_header_read(&in.start, &in.length, &riff_wav)) {

    goto error;
  }

  if (0 != strncmp(riff_wav.id, "RIFF", 5)) {

    ov_log_error("RIFF-WAV chunk has wrong type");
    goto error;
  }

  // Next should come the 'WAVE' format denominator
  if ((4 > in.length) || (0 != memcmp(in.start, "WAVE", 4))) {

    ov_log_error("WAVE denominator not found");
    goto error;
  }

  /* Length DOES include "WAVE" denominator - 4 bytes */
  return riff_wav.length - 4;

error:

  return -1;
}

/*****************************************************************************
                                   CALLBACKS
 ****************************************************************************/

static bool set_wdata_from_fmt_chunk(ov_format *f,
                                     size_t fmt_chunk_length_bytes,
                                     wave_data *wdata) {

  OV_ASSERT(0 != f);
  OV_ASSERT(0 != wdata);

  if (16 < fmt_chunk_length_bytes) {

    return false;
  }

  ov_buffer fmt_chunk =
      ov_format_payload_read_chunk_nocopy(f, fmt_chunk_length_bytes);

  if ((0 == fmt_chunk.start) || (fmt_chunk_length_bytes != fmt_chunk.length)) {

    return false;
  }

  uint8_t *ptr = fmt_chunk.start;
  size_t len = fmt_chunk.length;

  bool ok = ov_file_get_16(&wdata->parameters.format, &ptr, &len,
                           OV_FILE_LITTLE_ENDIAN);

  ok = ok && ov_file_get_16(&wdata->parameters.channels, &ptr, &len,
                            OV_FILE_LITTLE_ENDIAN);

  ok = ok && ov_file_get_32(&wdata->parameters.samplerate_hz, &ptr, &len,
                            OV_FILE_LITTLE_ENDIAN);

  uint32_t data_rate = 0;

  ok = ok && ov_file_get_32(&data_rate, &ptr, &len, OV_FILE_LITTLE_ENDIAN);

  ok = ok && ov_file_get_16(&wdata->parameters.blockAlignmentBytes, &ptr, &len,
                            OV_FILE_LITTLE_ENDIAN);

  ok = ok && ov_file_get_16(&wdata->parameters.bitsPerSample, &ptr, &len,
                            OV_FILE_LITTLE_ENDIAN);

  if (data_rate !=
      wdata->parameters.blockAlignmentBytes * wdata->parameters.samplerate_hz) {

    ov_log_error("Malformed fmt chunk: calculated data rate does not fit");
    goto error;
  }

  if (wdata->parameters.blockAlignmentBytes <
      (wdata->parameters.channels * wdata->parameters.bitsPerSample) / 8) {

    ov_log_error("Malformed fmt chunk: calculated block size does not "
                 "fit");
    goto error;
  }

  return ok;

error:

  return false;
}

/*----------------------------------------------------------------------------*/

static bool read_wave(ov_format *f, wave_data *wdata) {

  OV_ASSERT(0 != f);
  OV_ASSERT(0 != wdata);

  ssize_t file_length = read_riff_wav(f);

  if (0 > file_length) {

    goto error;
  }

  struct riff_chunk chunk = {0};

  bool fmt_chunk_read = false;

  while (true) {

    if (!riff_chunk_header_read_from_format(f, &chunk)) {

      goto error;
    }

    if (0 == strcmp("fmt ", chunk.id)) {

      fmt_chunk_read = set_wdata_from_fmt_chunk(f, chunk.length, wdata);

      if (!fmt_chunk_read) {

        ov_log_error("Could not read 'fmt ' chunk");
        goto error;
      }

      continue;
    }

    if (0 == strcmp("data", chunk.id)) {

      if (!fmt_chunk_read) {

        ov_log_error("Encountered 'data' chunk before 'fmt ' "
                     "chunk");
        goto error;
      }

      wdata->payload.length_bytes = chunk.length;
      wdata->payload.bytes_read = 0;

      break;
    }

    /* Default: Ignore ignore chunk */

    ov_log_info("Ignoring chunk %s with %zu bytes", chunk.id, chunk.length);

    ov_format_payload_read_chunk_nocopy(f, chunk.length);
  }

  return true;

error:

  return false;
}

/*----------------------------------------------------------------------------*/

static ov_buffer impl_next_chunk(ov_format *f, size_t requested_bytes,
                                 void *data) {

  UNUSED(requested_bytes);

  ov_buffer payload = {0};

  wave_data *wdata = as_wave_data(data);

  if (0 == wdata) {

    ov_log_error("Expected to be called with wave format");
    goto error;
  }

  size_t bytes_to_read = requested_bytes;

  size_t bytes_available =
      wdata->payload.length_bytes - wdata->payload.bytes_read;

  if (bytes_available < bytes_to_read) {

    bytes_to_read = bytes_available;
  }

  ov_buffer buf = ov_format_payload_read_chunk_nocopy(f, bytes_to_read);

  wdata->payload.bytes_read += buf.length;

  return buf;

error:

  return payload;
}

/*----------------------------------------------------------------------------*/

static ssize_t impl_write_chunk(ov_format *f, ov_buffer const *chunk,
                                void *data) {

  if (0 == f) {

    ov_log_error("No format to write to");
    goto error;
  }

  if ((0 == chunk) || (0 == chunk->start)) {

    ov_log_error("No chunk to write out");
    goto error;
  }

  wave_data *wdata = as_wave_data(data);

  if (0 == wdata) {
    ov_log_error("Expected wave format");
    goto error;
  }

  ssize_t bytes_written = ov_format_payload_write_chunk(f, chunk);

  if (0 > bytes_written) {

    goto error;
  }

  wdata->payload.bytes_written += (size_t)bytes_written;

  return bytes_written;

error:

  return -1;
}

/*----------------------------------------------------------------------------*/

static void *create_data_reading(ov_format *f) {

  wave_data *wdata = calloc(1, sizeof(wave_data));

  OV_ASSERT(0 != wdata);

  wdata->magic_bytes = WAV_MAGIC_BYTES;

  if (!read_wave(f, wdata)) {

    goto error;
  }

  return wdata;

error:

  if (0 != wdata) {

    free(wdata);
  }

  return 0;
}

/*----------------------------------------------------------------------------*/

#define MASTER_RIFF_CHUNK_LENGTH ((size_t)12)
#define DATA_RIFF_HEADER_LENGTH ((size_t)8)
#define FMT_CHUNK_LENGTH ((size_t)(4 + 2 + 2 + 4 + 4 + 2 + 2 + 4))

#define MASTER_RIFF_SIZE_OFFSET ((size_t)4)
#define PAYLOAD_SIZE_OFFSET                                                    \
  ((size_t)MASTER_RIFF_CHUNK_LENGTH + FMT_CHUNK_LENGTH + 4)

static bool write_wav_headers(ov_format *f, ov_format_wav_options *opts,
                              size_t *bytes_written_ptr) {

  OV_ASSERT(0 != f);
  OV_ASSERT(0 != opts);
  OV_ASSERT(0 != bytes_written_ptr);

  uint32_t data_rate = opts->samplerate_hz * opts->blockAlignmentBytes;

  uint8_t out[MASTER_RIFF_CHUNK_LENGTH + FMT_CHUNK_LENGTH +
              DATA_RIFF_HEADER_LENGTH] = {0};

  const size_t out_length = sizeof(out);

  OV_ASSERT(FMT_CHUNK_LENGTH == 8 + 16);

  size_t out_length_remaining = out_length;

  uint8_t *ptr = out;

  /* xxxx will be filled with actual length of RIFF chunk payload */
  memcpy(ptr, "RIFFxxxxWAVEfmt ", 16);
  ptr += 16;
  out_length_remaining -= 16;

  ov_file_write_32(&ptr, &out_length_remaining,
                   OV_H32TOLE(FMT_CHUNK_LENGTH - 8));

  ov_file_write_16(&ptr, &out_length_remaining, OV_H16TOLE(opts->format));

  ov_file_write_16(&ptr, &out_length_remaining, OV_H16TOLE(opts->channels));

  ov_file_write_32(&ptr, &out_length_remaining,
                   OV_H32TOLE(opts->samplerate_hz));

  ov_file_write_32(&ptr, &out_length_remaining, OV_H32TOLE(data_rate));

  ov_file_write_16(&ptr, &out_length_remaining,
                   OV_H16TOLE(opts->blockAlignmentBytes));

  ov_file_write_16(&ptr, &out_length_remaining,
                   OV_H16TOLE(opts->bitsPerSample));

  /* XXXX will be replaced by payload length upon closure */
  memcpy(ptr, "dataXXXX", 8);

  ptr += 8;
  out_length_remaining -= 8;

  OV_ASSERT(0 == out_length_remaining);

  ov_buffer headers = {
      .start = out,
      .length = out_length,
  };

  const ssize_t bytes_written = ov_format_payload_write_chunk(f, &headers);

  if (0 > bytes_written) {

    return false;
  }

  *bytes_written_ptr = (size_t)bytes_written;

  return headers.length == (size_t)bytes_written;
}

/*----------------------------------------------------------------------------*/

static void *create_data_writing(ov_format *f, ov_format_wav_options *opts) {

  wave_data *wdata = calloc(1, sizeof(wave_data));

  if (0 == f) {
    ov_log_error("No lower layer");
    goto error;
  }

  OV_ASSERT(0 != wdata);

  wdata->magic_bytes = WAV_MAGIC_BYTES;

  if (0 == opts) {

    opts = &default_options;
  }

  if (!write_wav_headers(f, opts, &wdata->payload.offset)) {

    goto error;
  }

  return wdata;

error:

  if (0 != wdata) {

    free(wdata);
  }

  return 0;
}

/*----------------------------------------------------------------------------*/

static void *impl_create_data(ov_format *f, void *options) {

  if (OV_READ == f->mode) {

    return create_data_reading(f);
  }

  if (OV_WRITE == f->mode) {

    return create_data_writing(f, options);
  }

  return 0;
}

/*----------------------------------------------------------------------------*/

static bool ready_data_write_nocheck(ov_format *f, wave_data *wdata) {

  OV_ASSERT(0 != f);
  OV_ASSERT(0 != wdata);

  /* Update sizes */

  uint32_t payload_size = wdata->payload.bytes_written;

  uint32_t size = OV_H32TOLE(payload_size);

  ov_buffer buf = {

      .start = (uint8_t *)&size,
      .length = sizeof(size),

  };

  ssize_t written = ov_format_payload_overwrite(f, PAYLOAD_SIZE_OFFSET, &buf);

  if ((0 > written) || (buf.length != (size_t)written)) {

    ov_log_error("Could not update payload size");
    goto error;
  }

  size =
      OV_H32TOLE(4 + DATA_RIFF_HEADER_LENGTH + FMT_CHUNK_LENGTH + payload_size);

  written = ov_format_payload_overwrite(f, MASTER_RIFF_SIZE_OFFSET, &buf);

  if ((0 > written) || (buf.length != (size_t)written)) {

    ov_log_error("Could not update payload size");
    goto error;
  }

  return true;

error:

  return false;
}

/*----------------------------------------------------------------------------*/

ssize_t impl_overwrite(ov_format *f, size_t offset, ov_buffer const *chunk,
                       void *data) {

  wave_data *wdata = as_wave_data(data);

  if (0 == wdata) {

    ov_log_error("Require wave data");
    goto error;
  }

  return ov_format_payload_overwrite(f, offset + wdata->payload.offset, chunk);

error:

  return -1;
}

/*----------------------------------------------------------------------------*/

static bool impl_ready_format(ov_format *f, void *data) {

  wave_data *wdata = as_wave_data(data);

  if ((0 == f) || (0 == wdata)) {

    goto error;
  }

  OV_ASSERT(0 != f);
  OV_ASSERT(0 != wdata);

  ov_format_mode mode = f->mode;

  if (OV_READ == mode) {

    return true;
  }

  if (OV_WRITE == mode) {

    return ready_data_write_nocheck(f, wdata);
  }

error:

  return data;
}

/*----------------------------------------------------------------------------*/

static void *impl_free_data(void *data) {

  wave_data *wdata = as_wave_data(data);

  if (0 == wdata)
    return data;

  free(wdata);

  return 0;
}

/*****************************************************************************
                                     PUBLIC
 ****************************************************************************/

bool ov_format_wav_install(ov_format_registry *registry) {

  ov_format_handler handler = {
      .next_chunk = impl_next_chunk,
      .write_chunk = impl_write_chunk,
      .overwrite = impl_overwrite,
      .ready_format = impl_ready_format,
      .create_data = impl_create_data,
      .free_data = impl_free_data,
  };

  return ov_format_registry_register_type(OV_FORMAT_WAV_TYPE_STRING, handler,
                                          registry);
}

/*----------------------------------------------------------------------------*/

bool ov_format_wav_get_header(ov_format *fmt, ov_format_wav_options *options) {

  if (0 == options) {

    ov_log_error("Require destination options, got 0 pointer");
    goto error;
  }

  wave_data *wdata = as_wave_data(ov_format_get_custom_data(fmt));

  if (0 == wdata) {

    ov_log_error("Expected to be called with wave format");
    goto error;
  }

  *options = wdata->parameters;

  return true;

error:

  return false;
}

/*----------------------------------------------------------------------------*/
