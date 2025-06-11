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
#include <ov_base/ov_utils.h>

#include <ov_arch/ov_byteorder.h>
#include <ov_base/ov_config_keys.h>
#include <ov_codec/ov_codec_factory.h>
#include <ov_format/ov_format.h>
#include <ov_format/ov_format_codec.h>
#include <ov_format/ov_format_wav.h>

/*----------------------------------------------------------------------------*/

/* The pcm options we assume
 * PCM 16Bit Little Endian, MONO, 48k
 */
static ov_format_wav_options wav_options = {

    .format = OV_FORMAT_WAV_PCM,

    .channels = 1,
    .samplerate_hz = 48000,

    .blockAlignmentBytes = 2,
    .bitsPerSample = 16,
};

/*----------------------------------------------------------------------------*/

#define PANIC(...)                                                             \
    do {                                                                       \
        fprintf(stderr, __VA_ARGS__);                                          \
        fprintf(stderr, "\n");                                                 \
        exit(EXIT_FAILURE);                                                    \
    } while (0)

/*----------------------------------------------------------------------------*/

static ov_format *as_wav(ov_format *fmt) {

    ov_format *wav = ov_format_as(fmt, "wav", &wav_options, 0);

    if (0 == wav) return 0;

    char const *pcm_le_str =
        "{\"" OV_KEY_ENDIANNESS "\":\"" OV_KEY_LITTLE_ENDIAN "\"}";

    ov_json_value *pcm_le =
        ov_json_value_from_string(pcm_le_str, strlen(pcm_le_str));

    ov_codec *pcm_codec =
        ov_codec_factory_get_codec(0, "pcm16_signed", 1, pcm_le);

    OV_ASSERT(0 != pcm_codec);

    ov_format *codec_format = ov_format_as(wav, "codec", pcm_codec, 0);
    OV_ASSERT(0 != codec_format);

    pcm_le = ov_json_value_free(pcm_le);
    OV_ASSERT(0 == pcm_le);

    return codec_format;
}

/*----------------------------------------------------------------------------*/

static ov_format *as_pcm(ov_format *fmt) {

    ov_codec *pcm_codec = ov_codec_factory_get_codec(0, "pcm16_signed", 1, 0);

    OV_ASSERT(0 != pcm_codec);

    ov_format *codec_format = ov_format_as(fmt, "codec", pcm_codec, 0);
    OV_ASSERT(0 != codec_format);

    return codec_format;
}

/*----------------------------------------------------------------------------*/

static void get_in_out_formats(int argc,
                               char **argv,
                               ov_format **in,
                               ov_format **out) {

    ov_format_wav_install(0);
    ov_format_codec_install(0);

    if (argc != 3) {
        PANIC(
            "Expect 2 arguments: INFILE OUTFILE\n\n"
            "If INFILE is wave, outputs PCM (parameters as in WAVE, esp. "
            "LITTLE ENDIAN)\n"
            "otherwise assumes INPUT to be PCM16S LITTLE ENDIAN and outputs "
            "wave");
    }

    ov_format *in_fmt = ov_format_open(argv[1], OV_READ);

    if (0 == in_fmt) {

        PANIC("Could not read %s", argv[1]);
    }

    /* Try to open input file as wav and write PCM.
     * If we fail wrapping in_fmt into wav, read in_fmt as raw PCM
     * and output to wav */
    ov_format *wav = as_wav(in_fmt);

    ov_format *out_fmt = ov_format_open(argv[2], OV_WRITE);

    if (0 == wav) {

        out_fmt = as_wav(out_fmt);
        wav = 0;
        in_fmt = as_pcm(in_fmt);

    } else {

        in_fmt = wav;
        wav = 0;
        out_fmt = as_pcm(out_fmt);
    }

    *in = in_fmt;
    *out = out_fmt;
}

int main(int argc, char **argv) {

    ov_format *in = 0;
    ov_format *out = 0;

    get_in_out_formats(argc, argv, &in, &out);

    OV_ASSERT(0 != in);
    OV_ASSERT(0 != out);

    while (ov_format_has_more_data(in)) {

        ov_buffer chunk = ov_format_payload_read_chunk_nocopy(in, 1024);

        OV_ASSERT(0 != chunk.start);

        ov_format_payload_write_chunk(out, &chunk);
    };

    ov_format_close(in);
    ov_format_close(out);

    ov_codec_factory_free(0);

    ov_format_registry_clear(0);

    exit(EXIT_SUCCESS);
}
