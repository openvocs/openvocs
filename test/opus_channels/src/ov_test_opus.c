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

char const *USAGE =
    "\n"
    "   USAGE:\n"
    "\n"
    "      ov_test_opus NUM_INPUT_CHANNELS NUM_OUTPUT_CHANNELS\n"
    "\n"
    "      NUM_INPUT_CHANNELS and NUM_OUTPUT_CHANNELS must be either 1 or 2\n"
    "\n"
    "      Generates 2 sin waves, interleaves them just as opus expects 2 "
    "channels "
    " to be interleaved.\n"
    "      Encodes with opus encoder configured for NUM_INPUT_CHANNELS,"
    " decodes result with opus decoder configured for NUM_OUTPUT_CHANNELS.\n"
    "      Writes PCM before encoding and PCM retrieved from decoding in 2 "
    "columns to STDOUT\n\n";

#include <opus.h>

#include <ov_base/ov_buffer.h>
#include <ov_base/ov_utils.h>

#include <ov_pcm16s/ov_pcm_gen.h>

/*----------------------------------------------------------------------------*/

static ov_buffer *generate_pcm_sin(double freq_hz,
                                   uint64_t frame_length_usecs) {

    ov_buffer *pcm = 0;

    ov_pcm_gen_config pcm_gen_config = {
        .sample_rate_hertz = 48000,
        .frame_length_usecs = frame_length_usecs,
    };

    ov_pcm_gen_sinusoids sinusoids_config = {

        .frequency_hertz = freq_hz,
        .wobble.period_secs = 0,

    };

    ov_pcm_gen *pcm_gen =
        ov_pcm_gen_create(OV_SINUSOIDS, pcm_gen_config, &sinusoids_config);

    OV_ASSERT(0 != pcm_gen);

    pcm = pcm_gen->generate_frame(pcm_gen);

    pcm_gen = pcm_gen->free(pcm_gen);

    OV_ASSERT(0 == pcm_gen);

    return pcm;
}

/*----------------------------------------------------------------------------*/

ov_buffer *interleave_buffers(ov_buffer const *b1, ov_buffer const *b2) {

    OV_ASSERT(0 != b1);
    OV_ASSERT(0 != b2);
    OV_ASSERT(0 != b1->start);
    OV_ASSERT(0 != b2->start);

    int16_t *i16_1 = (int16_t *)b1->start;
    int16_t *i16_2 = (int16_t *)b2->start;

    ov_buffer *interleaved = ov_buffer_create(2 * b1->length);

    OV_ASSERT(0 != interleaved);
    OV_ASSERT(0 != interleaved->start);
    OV_ASSERT(2 * b1->length <= interleaved->capacity);

    interleaved->length = 2 * b1->length;

    int16_t *i16_i = (int16_t *)interleaved->start;

    for (size_t i = 0; b1->length / 2 > i; ++i) {

        int16_t sample = i16_1[i];
        i16_i[2 * i] = sample;

        sample = i16_2[i];
        i16_i[2 * i + 1] = sample;
    }

    return interleaved;
}

/*----------------------------------------------------------------------------*/

void dump_buffers_as_16s(FILE *out, ov_buffer const **buffers,
                         size_t num_buffers) {

    OV_ASSERT(0 != out);
    OV_ASSERT(0 != buffers);

    size_t sample_index = 0;

    bool not_all_buffers_exhausted = true;

    while (not_all_buffers_exhausted) {

        not_all_buffers_exhausted = false;

        for (size_t i = 0; i < num_buffers; ++i) {

            int16_t sample = 0;

            if (buffers[i]->length > sample_index * sizeof(int16_t)) {

                not_all_buffers_exhausted = true;

                int16_t *i16_pointer =
                    (int16_t *)buffers[i]->start + sample_index;

                sample = *i16_pointer;
            }

            fprintf(out, "%" PRIi16 " ", sample);
        }

        ++sample_index;

        fprintf(out, "\n");
    }
}

/*----------------------------------------------------------------------------*/

ov_buffer *encode(ov_buffer const *in, int channels) {

    const size_t samples_per_frame = in->length / (sizeof(int16_t) * channels);
    const size_t out_max_len = 8 * 1024;

    int error = OPUS_OK;

    OpusEncoder *encoder =
        opus_encoder_create(48000, channels, OPUS_APPLICATION_VOIP, &error);

    OV_ASSERT(0 != encoder);

    fprintf(stderr, "Opus encoder: Error %i (OK = %i)\n", error, OPUS_OK);

    ov_buffer *out = ov_buffer_create(out_max_len);

    int retval = opus_encode(encoder, (int16_t *)in->start, samples_per_frame,
                             (unsigned char *)out->start, out->capacity);

    opus_encoder_destroy(encoder);

    out->length = (size_t)retval;

    if (0 > retval) {

        fprintf(stderr, "Opus encode error: %i\n", retval);
        out = ov_buffer_free(out);
    }

    encoder = 0;

    return out;
}

/*----------------------------------------------------------------------------*/

ov_buffer *decode(ov_buffer const *in, int channels) {

    const size_t channels_from_packet =
        (size_t)opus_packet_get_nb_channels(in->start);

    OV_ASSERT(0 != in);

    /* Must be equivalent of 120ms */
    const int max_samples_per_channel = 120 * 48000 / 1000;

    int error = OPUS_OK;

    OpusDecoder *decoder = opus_decoder_create(48000, channels, &error);

    OV_ASSERT(0 != decoder);

    fprintf(stderr, "Opus decoder: Error %i (OK = %i)\n", error, OPUS_OK);

    ov_buffer *out =
        ov_buffer_create(max_samples_per_channel * sizeof(int16_t) * 2);

    OV_ASSERT(0 != out);
    OV_ASSERT((size_t)max_samples_per_channel <= out->capacity);

    int retval = opus_decode(decoder, in->start, in->length,
                             (int16_t *)out->start, max_samples_per_channel, 0);

    opus_decoder_destroy(decoder);
    decoder = 0;

    out->length = (size_t)retval * sizeof(int16_t) * channels_from_packet;

    const size_t samples_per_channel =
        out->length / (sizeof(int16_t) * channels);

    if (0 > retval) {
        fprintf(stderr, "Opus decode error: %i\n", retval);
        out = ov_buffer_free(out);
    }

    fprintf(stderr,
            "Opus packet contains %i channels, %i samples per "
            "frame&channel "
            "%zu samples per frame&channel (calculated)\n"
            "Max samples per frame & channel: %i\n",
            opus_packet_get_nb_channels(in->start),
            opus_packet_get_samples_per_frame(in->start, 48000),
            samples_per_channel, max_samples_per_channel);

    return out;
}

/*----------------------------------------------------------------------------*/

void OV_ASSERT_or_abort(bool condition) {

    if (!condition) {

        fprintf(stderr, USAGE);
        exit(EXIT_FAILURE);
    }
}

/*----------------------------------------------------------------------------*/

size_t get_arg_as_size_t(int argc, char **argv, size_t index) {

    OV_ASSERT(0 < argc);

    OV_ASSERT_or_abort(index < (size_t)argc);

    char *endptr = 0;

    long val = strtol(argv[index], &endptr, 0);

    OV_ASSERT_or_abort(0 != endptr);
    OV_ASSERT_or_abort(0 == *endptr);

    OV_ASSERT_or_abort(-1 < val);

    return (size_t)val;
}

/*----------------------------------------------------------------------------*/

int main(int argc, char **argv) {

    size_t num_input_channels = get_arg_as_size_t(argc, argv, 1);
    size_t num_output_channels = get_arg_as_size_t(argc, argv, 2);

    OV_ASSERT_or_abort(0 < num_input_channels);
    OV_ASSERT_or_abort(3 > num_input_channels);

    OV_ASSERT_or_abort(0 < num_output_channels);
    OV_ASSERT_or_abort(3 > num_output_channels);

    fprintf(stderr, "Using %zu input channels, %zu output channels\n",
            num_input_channels, num_output_channels);

    ov_buffer *pcm = generate_pcm_sin(150, 60000);
    ov_buffer *pcm_2 = generate_pcm_sin(400, 60000);

    OV_ASSERT(0 != pcm);
    OV_ASSERT(0 != pcm_2);

    const size_t num_samples = pcm->length / 2;
    OV_ASSERT(num_samples * 2 == pcm->length);

    ov_buffer *pcm_interleaved = interleave_buffers(pcm, pcm_2);
    OV_ASSERT(0 != pcm_interleaved);

    pcm = ov_buffer_free(pcm);
    pcm_2 = ov_buffer_free(pcm_2);

    printf("Interleaved length: %zu\n", pcm_interleaved->length);

    ov_buffer *encoded = encode(pcm_interleaved, num_input_channels);

    OV_ASSERT(0 != encoded);

    fprintf(stderr, "Encoded %zu samples to %zu bytes\n", num_samples,
            encoded->length);

    ov_buffer *decoded = decode(encoded, num_output_channels);

    encoded = ov_buffer_free(encoded);

    dump_buffers_as_16s(stdout, (ov_buffer const *[]){pcm_interleaved, decoded},
                        2);

    pcm_interleaved = ov_buffer_free(pcm_interleaved);
    decoded = ov_buffer_free(decoded);

    OV_ASSERT(0 == decoded);
    OV_ASSERT(0 == encoded);
    OV_ASSERT(0 == pcm);
    OV_ASSERT(0 == pcm_2);
    OV_ASSERT(0 == pcm_interleaved);

    return EXIT_SUCCESS;
}
