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
#include <ov_base/ov_utils.h>
#include <ov_codec/ov_codec_factory.h>
#include <ov_codec/ov_codec_pcm16_signed.h>
#include <ov_pcm16s/ov_pcm_resampler.h>
#include <ov_test/ov_test.h>
#include <stdarg.h>
#include <stdint.h>
#include <sys/resource.h>
#include <sys/time.h>

/*----------------------------------------------------------------------------*/

#define BENCHMARK_RUNS 100000

/*----------------------------------------------------------------------------*/

static struct timeval get_time() {
    struct timeval t;
    struct timezone tzp;
    gettimeofday(&t, &tzp);
    return t;
}

/*----------------------------------------------------------------------------*/

struct usage {

    struct rusage rusage;
    struct timeval time;
};

/*----------------------------------------------------------------------------*/

static struct usage get_usage() {

    struct usage usage = {0};

    int retval = getrusage(RUSAGE_SELF, &usage.rusage);

    if (0 != retval) {

        fprintf(stderr, "Could not get resource usage: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    usage.time = get_time();

    return usage;
}

/*----------------------------------------------------------------------------*/

#define print_field(desc, field)                                               \
    printf("\n\n %s   " #field " %li\n",                                       \
           desc,                                                               \
           after.rusage.ru_##field - before.rusage.ru_##field);

/*----------------------------------------------------------------------------*/

void print_usage(struct usage before) {

    struct usage after = {0};

    after.time = get_time();

    int retval = getrusage(RUSAGE_SELF, &after.rusage);

    if (0 != retval) {

        fprintf(stderr, "Could not get resource usage: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    int64_t sec = after.time.tv_sec - before.time.tv_sec;
    int64_t usec = after.time.tv_usec - before.time.tv_usec;

    if (0 > usec) {
        sec -= 1;
        usec += 1000 * 1000;
    }

    printf("\nTime passed   %" PRIi64 ".%" PRIi64 "\n", sec, usec);

    printf("\n");

    print_field("unshared memory size", idrss);
}

/*----------------------------------------------------------------------------*/

static char const *in_signal_path = "/tmp/in.csv";
static char const *out_signal_path = "/tmp/out.csv";

static size_t const OUT_BUFFER_SIZE_SAMPLES = 5000;

/*****************************************************************************
                                   RESAMPLING
 ****************************************************************************/

static void print_samples(FILE *f,
                          int16_t const *samples,
                          size_t num_samples,
                          double samplerate_hz) {

    TEST_ASSERT(0 != f);
    TEST_ASSERT(0 != samples);
    TEST_ASSERT(0 < num_samples);
    TEST_ASSERT(0 < samplerate_hz);

    fprintf(f, "#\n# Samples: %zu\n#\n", num_samples);

    double t = 0;

    for (size_t i = 0; i < num_samples; ++i) {

        t = i;
        t /= samplerate_hz;

        fprintf(f, "%f    %" PRIi16 "\n", t, samples[i]);
    }

    fprintf(f, "#\n#\n");
}

/*----------------------------------------------------------------------------*/

static void print_samples_to_file(FILE *out,
                                  int16_t const *sig_in,
                                  size_t num_samples,
                                  double samplerate_hz) {

    if (0 == out) {
        return;
    }

    TEST_ASSERT(0 != out);

    print_samples(out, sig_in, num_samples, samplerate_hz);
    fprintf(out, "##########################\n#\n#\n#\n#\n");
}

/*----------------------------------------------------------------------------*/
// Signal generator

double g_sig_default_periods[] = {100, 82, 19, 6};

double const *g_sig_periods_s = g_sig_default_periods;
size_t g_sig_num_periods = 4;

int16_t g_sig_max_amplitude = 1000;

static int16_t sig(size_t i) {

    double ma = g_sig_max_amplitude;
    ma /= g_sig_num_periods;

    double result = 0;

    double t = i;
    double t_2_pi = 2.0 * M_PI * t;

    for (size_t i_p = 0; i_p < g_sig_num_periods; ++i_p) {

        result += ma * sin(t_2_pi / g_sig_periods_s[i_p]);
    }

    return (int16_t)result;
}

/*----------------------------------------------------------------------------*/

static void fill_with_signal(int16_t *in, size_t num_samples) {

    for (size_t i = 0; i < num_samples; ++i) {
        in[i] = sig(i);
    }
}

/*----------------------------------------------------------------------------*/

static void setup_signal_generator(double const *periods) {

    if (0 == periods) {
        return;
    }

    size_t num = 0;

    for (;; ++num) {
        if (0 == periods[num]) {
            break;
        }
        printf("%f  ", periods[num]);
    }

    g_sig_periods_s = periods;
    g_sig_num_periods = num;
    printf(" - using %zu periods\n", num);
}

/*----------------------------------------------------------------------------*/

typedef enum { PRECALC, NO_PRECALC } resample_method;

struct args {

    resample_method method;
    FILE *in_file;
    char const *in_file_path;

    FILE *out_file;
    char const *out_file_path;

    // Only used if in_file == 0
    double *periods;
    size_t num_periods;

    FILE *dump_in_signal_file;
    FILE *dump_out_signal_file;

    // Internal stuff

    ov_codec *in_file_codec;
    ov_codec *out_file_codec;

    uint8_t *buf;
    size_t buf_length_bytes;
};

/*----------------------------------------------------------------------------*/

static bool signal_generator_more_chunks = true;

static bool chunk_from_signal_generator(struct args *args,
                                        int16_t *buffer,
                                        size_t num_samples) {

    OV_ASSERT(0 != args);

    if (!signal_generator_more_chunks) {
        return false;
    }

    signal_generator_more_chunks = false;

    if (0 == args->periods) {
        return false;
    }

    fill_with_signal(buffer, num_samples);

    return true;
}

/*----------------------------------------------------------------------------*/

static ov_codec *get_codec() {

    uint32_t dummy_ssid = 12;

    return ov_codec_factory_get_codec(
        0, ov_codec_pcm16_signed_id(), dummy_ssid, 0);
}

/*----------------------------------------------------------------------------*/

static bool write_samples_to_bin_file(struct args *args,
                                      int16_t *signal,
                                      size_t num_samples) {

    OV_ASSERT(0 != args);
    OV_ASSERT(0 != signal);

    if (0 == args->out_file) {
        return true;
    }

    if (0 == args->out_file_codec) {
        args->out_file_codec = get_codec();
    }

    int32_t written_bytes = ov_codec_encode(args->out_file_codec,
                                            (uint8_t *)signal,
                                            sizeof(int16_t) * num_samples,
                                            (uint8_t *)args->buf,
                                            args->buf_length_bytes);

    if ((0 > written_bytes) ||
        ((size_t)written_bytes != sizeof(int16_t) * num_samples)) {
        fprintf(stderr, "Could not encode from file\n");
        return false;
    }

    if (0 == fwrite(args->buf, 1, (size_t)written_bytes, args->out_file)) {
        fprintf(stderr, "Could not write signal to file\n");
        return false;
    }

    return true;
}

/*----------------------------------------------------------------------------*/

static bool chunk_from_file(struct args *args,
                            int16_t *buffer,
                            size_t num_samples) {

    OV_ASSERT(0 != args);
    OV_ASSERT(0 != args->buf);

    if (0 == args->in_file_codec) {
        args->in_file_codec = get_codec();
    }

    size_t read_bytes =
        fread(args->buf, 1, args->buf_length_bytes, args->in_file);

    if (0 >= read_bytes) {
        return false;
    }

    uint32_t dummy_ssid = 12;

    int32_t written_bytes = ov_codec_decode(args->in_file_codec,
                                            dummy_ssid,
                                            args->buf,
                                            read_bytes,
                                            (uint8_t *)buffer,
                                            num_samples * sizeof(int16_t));

    if ((0 > written_bytes) || ((size_t)written_bytes != read_bytes)) {
        fprintf(stderr, "Could not decode from file\n");
        return false;
    }

    return true;
}

/*----------------------------------------------------------------------------*/

static void resample(double samplerate_in_hz,
                     double samplerate_out_hz,
                     struct args args) {

    bool (*next_sample_chunk)(
        struct args *args, int16_t *buffer, size_t num_samples) = 0;

    const size_t num_samples = 200;

    args.buf_length_bytes = num_samples * sizeof(int16_t);
    args.buf = calloc(1, args.buf_length_bytes);

    int16_t sig_in[num_samples];

    OV_ASSERT((0 == args.in_file) || (0 == args.periods));

    if (0 != args.periods) {
        setup_signal_generator(args.periods);
        next_sample_chunk = chunk_from_signal_generator;
    }

    if (0 != args.in_file) {
        next_sample_chunk = chunk_from_file;
    }

    OV_ASSERT(0 != next_sample_chunk);

    ov_pcm_16_resampler *resampler = ov_pcm_16_resampler_create(
        num_samples, num_samples, samplerate_in_hz, samplerate_out_hz);
    OV_ASSERT(0 != resampler);

    int16_t *out = calloc(1, sizeof(int16_t) * OUT_BUFFER_SIZE_SAMPLES);
    size_t out_length_samples = OUT_BUFFER_SIZE_SAMPLES;

    while (next_sample_chunk(&args, sig_in, num_samples)) {

        print_samples_to_file(
            args.dump_in_signal_file, sig_in, num_samples, samplerate_in_hz);

        ssize_t out_samples = 0;

        struct usage usage = get_usage();

        switch (args.method) {

            case NO_PRECALC:

                out_samples = ov_pcm_16_resample_uncached(
                    resampler, sig_in, num_samples, out, out_length_samples);

                break;

            case PRECALC:

                out_samples = ov_pcm_16_resample(
                    resampler, sig_in, num_samples, out, out_length_samples);

                break;
        };

        print_usage(usage);

        OV_ASSERT(0 < out_samples);

        size_t num_out_samples = (size_t)out_samples;

        if (num_out_samples > out_length_samples) {
            fprintf(stdout,
                    "Warning: Number of required output samples %zu higher "
                    "than "
                    "actual output buffer: %zu\n",
                    num_out_samples,
                    out_length_samples);
            num_out_samples = out_length_samples;
        }

        print_samples_to_file(
            args.dump_out_signal_file, out, num_out_samples, samplerate_out_hz);

        write_samples_to_bin_file(&args, out, num_out_samples);
    };

    printf("Done resampling\n");

    /*------------------------------------------------------------------------*/

    free(out);
    out = 0;

    resampler = ov_pcm_16_resampler_free(resampler);
    OV_ASSERT(0 == resampler);
}

/*****************************************************************************
                                  Arg parsing
 ****************************************************************************/

static double to_double(char const *a) {

    char *endptr = 0;

    double val = strtod(a, &endptr);

    if ((0 == endptr) || (0 != *endptr)) {

        fprintf(stderr, "Could not convert '%s' to floating point\n", a);
        exit(EXIT_FAILURE);
    }

    return val;
}

/*----------------------------------------------------------------------------*/

size_t parse_periods(size_t argc,
                     char const **argv,
                     size_t start_index,
                     double **periods,
                     size_t *num_periods) {

    OV_ASSERT(0 != argv);
    OV_ASSERT(0 != periods);
    OV_ASSERT(0 != num_periods);

    size_t num = 0;

    for (size_t i = start_index; i < argc; ++i) {

        if ('-' == argv[i][0]) {
            break;
        }

        ++num;
    }

    OV_ASSERT(start_index + num <= argc);

    if (0 == num) {
        return start_index;
    }

    double *p = calloc(1 + num, sizeof(double));
    *periods = p;
    *num_periods = num;

    size_t k = 0;
    for (size_t i = start_index; i < start_index + num; ++i, ++k) {
        p[k] = to_double(argv[i]);
    }

    p[num] = 0;

    return start_index + num - 1;
}

/*----------------------------------------------------------------------------*/

_Noreturn static void usage(char const *cmd) {

    fprintf(stderr,
            "\n"
            "    USAGE:\n"
            "        %s SAMPLERATE_IN SAMPLERATE_OUT [arg ...]\n",
            cmd);

    fprintf(stderr,
            "\n"
            "        Resamples a signal from SAMPLERATE_IN to SAMPLERATE_OUT.\n"
            "\n"
            "    ARGUMENTS\n"
            "\n"
            "        Samplerates are in Hz\n"
            "\n"
            "        It takes zero, one or several additional arguments.\n"
            "\n"
            "        They are either\n"
            "        -n\n"
            "           Use non-precalculated resampling method\n"
            "        everything else is assumed to be a floating point "
            "number\n"
            "        That number is taken as another period (in Hz)\n"
            "        of the artificial signal\n"
            "        -p [PERIOD1 ...]\n"
            "          Generate artificial signal. Can be followed by one "
            "or\n"
            "          several float value - those are the periods of sig\n"
            "        -i BIN_PCM_FILE\n"
            "          Read input from binary file\n"
            "        -o BIN_PCM_FILE\n"
            "          Write result in binary to BIN_PCM_FILE\n"
            "        -d dump input signal as CSV to %s\n"
            "                output signal as CSV to %s\n",
            in_signal_path,
            out_signal_path);

    exit(EXIT_FAILURE);
}

/*----------------------------------------------------------------------------*/

_Noreturn static void error_exit(char const **argv, char const *msg, ...) {

    OV_ASSERT(msg);

    va_list ap;

    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    fprintf(stderr, "\n");
    va_end(ap);
    usage(argv[0]);
}

/*----------------------------------------------------------------------------*/

static FILE *open_file(char const **argv, char const *path, char const *mode) {

    OV_ASSERT(0 != argv);
    OV_ASSERT(0 != path);
    OV_ASSERT(0 != mode);

    if ((mode[0] == 'r') && (!ov_file_exists(path))) {
        error_exit(argv, "File %s does not exists", path);
    }

    FILE *f = fopen(path, mode);

    if (0 == f) {
        error_exit(argv, "Could not open file %s for %s", path, mode);
    }

    return f;
}

/*----------------------------------------------------------------------------*/

static bool parse_args(size_t argc, char const **argv, struct args *args) {

    size_t i = 3;
    while (i < argc) {

        char const *arg = argv[i];

        if ('-' != arg[0]) {
            error_exit(argv, "Invalid argument: %s\n", arg);
        }

        switch (arg[1]) {

            case 'n':
                args->method = NO_PRECALC;
                break;

            case 'p':
                if (0 != args->in_file) {
                    error_exit(argv, "Cannot use both '-i' and '-p'");
                }

                i = parse_periods(
                    argc, argv, ++i, &args->periods, &args->num_periods);
                break;

            case 'i':
                if (0 != args->periods) {
                    error_exit(argv, "Cannot use both '-i' and '-p'");
                }

                ++i;
                if (i >= argc) {
                    error_exit(argv, "Expect file name after '-i'");
                }
                args->in_file = open_file(argv, argv[i], "r");
                args->in_file_path = argv[i];

                break;

            case 'o':

                ++i;
                if (i >= argc) {
                    error_exit(argv, "Expect file name after '-o'");
                }

                args->out_file = open_file(argv, argv[i], "w");
                args->out_file_path = argv[i];

                break;

            case 'd':
                args->dump_in_signal_file =
                    open_file(argv, in_signal_path, "w");
                args->dump_out_signal_file =
                    open_file(argv, out_signal_path, "w");

                break;
        }
        ++i;
    }

    return true;
}

/*----------------------------------------------------------------------------*/

int main(int argc, char const **argv) {

    if (3 > argc) {
        usage(argv[0]);
    }

    // const double our_periods[] = {100, 82, 19, 6, 0};

    OV_ASSERT(argc > 1);

    double samplerate_in_hz = to_double(argv[1]);
    double samplerate_out_hz = to_double(argv[2]);

    struct args args = {
        .method = PRECALC,
    };

    if (3 < argc) {
        parse_args(argc, (char const **)argv, &args);
    }

    printf("Using Precalculated convolution coefficients: %s\n",
           (args.method == PRECALC) ? "Yes" : "No");

    if (0 != args.in_file) {
        printf("Reading from binary file %s\n", args.in_file_path);
    }

    if (0 != args.periods) {
        printf("Using artificial signal with periods: ");
        for (size_t i = 0; i < args.num_periods; ++i) {
            printf("%f ", args.periods[i]);
        }
        printf("\n");
    }

    if (args.out_file) {
        printf("Writing to binary file %s\n", args.out_file_path);
    }

    if (0 != args.dump_in_signal_file) {
        printf("Dumping input signal as CSV to %s\n", in_signal_path);
    }

    if (0 != args.dump_out_signal_file) {
        printf("Dumping output signal as CSV to %s\n", out_signal_path);
    }

    if ((0 == args.in_file) && (0 == args.periods)) {
        error_exit(argv, "Require either '-i' or '-p'");
    }

    resample(samplerate_in_hz, samplerate_out_hz, args);

    if (0 != args.periods) {
        free(args.periods);
    }

    if (0 != args.dump_in_signal_file) {
        fclose(args.dump_in_signal_file);
    }

    if (0 != args.dump_out_signal_file) {
        fclose(args.dump_out_signal_file);
    }

    if (0 != args.in_file) {
        fclose(args.in_file);
    }

    if (0 != args.out_file) {
        fclose(args.out_file);
    }

    if (0 != args.buf) {
        free(args.buf);
    }

    if (0 != args.in_file_codec) {
        args.in_file_codec = ov_codec_free(args.in_file_codec);
    }

    if (0 != args.out_file_codec) {
        args.out_file_codec = ov_codec_free(args.out_file_codec);
    }

    return EXIT_SUCCESS;
}
