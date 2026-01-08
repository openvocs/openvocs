/***
        ------------------------------------------------------------------------

        Copyright (c) 2022 German Aerospace Center DLR e.V. (GSOC)

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

#include <inttypes.h>
#include <math.h>
#include <ov_base/ov_file.h>
#include <ov_base/ov_string.h>
#include <ov_base/ov_utils.h>
#include <ov_codec/ov_codec_factory.h>
#include <ov_codec/ov_codec_pcm16_signed.h>
#include <ov_format/ov_format.h>
#include <ov_format/ov_format_ogg.h>
#include <ov_format/ov_format_ogg_opus.h>
#include <ov_format/ov_format_registry.h>
#include <ov_pcm16s/ov_pcm_resampler.h>
#include <ov_test/ov_test.h>
#include <stdarg.h>
#include <stdint.h>
#include <sys/resource.h>
#include <sys/time.h>

/*----------------------------------------------------------------------------*/

_Noreturn static void usage(char const *cmd) {

  fprintf(stderr,
          "\n"
          "    USAGE:\n"
          "        %s [-O] OGG_FILE_NAME\n\n"
          "    Tries to open OGG file, verify it and\n"
          "    copy payload to another OGG\n\n"
          "    Options\n\n"
          "      -O   Read as OGG Opus instead of plain OGG\n\n\n",
          cmd);

  fprintf(stderr, "\n"
                  "\n"
                  "\n");

  exit(EXIT_FAILURE);
}

/*----------------------------------------------------------------------------*/

static bool process_ogg(ov_format *ogg, ov_format *out) {

  ov_buffer *chunk = ov_format_payload_read_chunk(ogg, 100);
  size_t total = 0;
  size_t c = 0;

  while (0 != chunk) {

    total += chunk->length;
    fprintf(stdout, "Chunk %zu: Read %zu bytes\n", c++, chunk->length);

    ov_format_payload_write_chunk(out, chunk);

    chunk = ov_buffer_free(chunk);
    chunk = ov_format_payload_read_chunk(ogg, 100);
  };

  fprintf(stdout, "Read %zu bytes\n", total);

  return true;
}

/*----------------------------------------------------------------------------*/

static ov_format *create_format(char const *fpath, bool opus,
                                ov_format_mode mode) {

  ov_format *format = 0;

  if (0 != fpath) {

    format = ov_format_as(ov_format_open(fpath, mode),
                          OV_FORMAT_OGG_TYPE_STRING, 0, 0);

    if (opus) {

      fprintf(stdout, "Accessing %s as OGG OPUS\n", ov_string_sanitize(fpath));

      format = ov_format_as(format, OV_FORMAT_OGG_OPUS_TYPE_STRING, 0, 0);

    } else {

      fprintf(stdout, "Accessing %s as plain OGG\n", ov_string_sanitize(fpath));
    }
  }

  return format;
}

/*----------------------------------------------------------------------------*/

typedef struct {

  char const *infile;

  struct {

    bool opus : 1;
  };

} configuration;

configuration parse_arguments(int argc, char const **argv) {

  int c = 0;

  configuration args = {0};

  while ((c = getopt(argc, (char **)argv, "O")) != -1) {

    switch (c) {

    case 'O':
      args.opus = true;
      break;
    };
  };

  args.infile = argv[optind];

  return args;
}

/*----------------------------------------------------------------------------*/

int main(int argc, char const **argv) {

  ov_format_ogg_install(0);
  ov_format_ogg_opus_install(0);

  configuration args = parse_arguments(argc, argv);

  if (0 == args.infile) {

    usage(argv[0]);

  } else {

    char outfile[PATH_MAX + 1] = {0};
    snprintf(outfile, sizeof(outfile), "%s.out", args.infile);
    outfile[sizeof(outfile) - 1] = 0;

    ov_format *out = create_format(outfile, args.opus, OV_WRITE);
    ov_format *in = create_format(args.infile, args.opus, OV_READ);

    process_ogg(in, out);

    out = ov_format_close(out);
    in = ov_format_close(in);

    return EXIT_SUCCESS;
  }
}
