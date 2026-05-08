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

#include <ov_format/ov_format.h>
#include <ov_format/ov_format_registry.h>

#include <ov_format/ov_format_pcap.h>
#include <ov_format/ov_format_rtp.h>

#include <ov_codec/ov_codec.h>
#include <ov_codec/ov_codec_opus.h>

#include <ov_base/ov_dict.h>

#include <opus.h>
#include <ov_base/ov_utils.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <getopt.h>
#include <ov_format/ov_format_udp.h>
#include <ov_log/ov_log.h>
#include <unistd.h>

#define MAX_SAMPLES_PER_FRAME 48000
#define MAX_NUM_STREAMS 10

/*----------------------------------------------------------------------------*/

typedef struct {

    char const *pcap_file;
    char const *codec_name;
    bool enable_log;
    bool list_streams;

    struct {

        bool analyze;
        uint32_t ssid;
        bool csv;

    } stream;

} analyser_config;

analyser_config configuration = {0};

/*----------------------------------------------------------------------------*/

#define PANIC(...)                                                             \
    do {                                                                       \
        fprintf(stderr, __VA_ARGS__);                                          \
        exit(EXIT_FAILURE);                                                    \
    } while (0)

#define LOG_WARN(...) fprintf(stderr, __VA_ARGS__)

#define LOG(...) fprintf(stderr, __VA_ARGS__)

/*----------------------------------------------------------------------------*/

static const char usage_string[] = "Prints analysis of an IP/UDP/RTP/OPUS "
                                   "stream captured in a PCAP file\n\n"
                                   "      PATH_TO_FILE\n"
                                   "      OPTION might be one of\n"
                                   "      -s SSID, --stream=SSID Analyse Opus "
                                   "stream with SSID\n"
                                   "      -L, --log      turn on logging\n"
                                   "      -l, --list     list ssids of all "
                                   "streams found at the end of the report\n"
                                   "      -c, --csv      Output PCM as Tex \n";

_Noreturn void usage(char const *binary_path) {

    fprintf(stderr,
            "Usage:\n\n"
            "    %s -f PATH_TO_FILE [OPTION [...]]\n\n"
            "%s\n\n\n",
            binary_path, usage_string);

    exit(EXIT_SUCCESS);
}

/*----------------------------------------------------------------------------*/

static analyser_config get_config(int argc, char **argv) {

    analyser_config cfg = {0};

    cfg = (analyser_config){

        .pcap_file = 0,
        .codec_name = ov_codec_opus_id(),
        .enable_log = false,

    };

    struct option opt_list_streams = {

        .name = "list",
        .has_arg = no_argument,
        .flag = 0,
        .val = 'l',

    };

    struct option opt_stream = {

        .name = "stream",
        .has_arg = required_argument,
        .flag = 0,
        .val = 's',

    };

    struct option opt_csv = {

        .name = "csv",
        .has_arg = no_argument,
        .flag = 0,
        .val = 'c',

    };

    struct option opt_log = {

        .name = "log",
        .has_arg = no_argument,
        .flag = 0,
        .val = 'L',

    };

    struct option opt_pcap_file = {

        .name = "file",
        .has_arg = required_argument,
        .flag = 0,
        .val = 'f',

    };

    struct option opt_null = {0};

    struct option longopts[] = {opt_list_streams, opt_csv,       opt_stream,
                                opt_log,          opt_pcap_file, opt_null};

    int c = 0;

    while (true) {

        int option_index = 0;
        c = getopt_long(argc, argv, "lLf:s:c", longopts, &option_index);

        if (-1 == c) {
            break;
        }

        switch (c) {

        case 'c':

            LOG("Outputting as csv\n");
            cfg.stream.csv = true;
            break;

        case 's':

            cfg.stream.ssid = strtol(optarg, 0, 0);
            cfg.stream.analyze = true;

            LOG("Analysing single stream SSID %" PRIu32 "\n", cfg.stream.ssid);

            break;

        case 'f':

            LOG("setting to %s\n", optarg);
            cfg.pcap_file = optarg;

            break;

        case 'l':

            cfg.list_streams = true;
            break;

        case 'L':

            cfg.enable_log = true;
            break;

        default:

            LOG_WARN("unknown option %c\n", c);
            usage(argv[0]);
        };
    }

    return cfg;
}

/*****************************************************************************
                                     CODECS
 ****************************************************************************/

ov_dict *g_codecs = 0;

/*----------------------------------------------------------------------------*/

static bool create_codec(char const *name, uint32_t ssid) {

    ov_json_value *parameters = 0;

    ov_codec *codec = ov_codec_factory_get_codec(0, name, ssid, parameters);

    intptr_t ssid_ptr = ssid;

    ov_dict_set(g_codecs, (void *)ssid_ptr, codec, 0);

    return 0 != codec;
}

/*----------------------------------------------------------------------------*/

static void *free_codec(void *vptr) {

    if (0 == vptr)
        return vptr;

    ov_codec *codec = (ov_codec *)vptr;

    if (0 != codec->free) {

        codec = codec->free(codec);
    }

    return codec;
}

/*----------------------------------------------------------------------------*/

static bool initialize_codec_registry() {

    ov_dict_config cfg = ov_dict_intptr_key_config(MAX_NUM_STREAMS);
    cfg.value.data_function.free = free_codec;

    g_codecs = ov_dict_create(cfg);

    if (0 == g_codecs) {
        return false;
    }

    if (!configuration.stream.analyze)
        return true;

    return create_codec(configuration.codec_name, configuration.stream.ssid);
}

/*----------------------------------------------------------------------------*/

static bool free_codec_registry() {

    g_codecs = ov_dict_free(g_codecs);
    return 0 == g_codecs;
}

/*----------------------------------------------------------------------------*/

static ov_codec *get_codec_for(uint32_t ssid) {

    intptr_t ssid_ptr = ssid;

    return ov_dict_get(g_codecs, (void *)ssid_ptr);
}

/*----------------------------------------------------------------------------*/

static bool is_srtp(ov_format *rtp_fmt) {

    ov_buffer *padding = ov_format_rtp_get_padding(rtp_fmt);

    if (0 == padding)
        return false;
    if (OV_FORMAT_RTP_NO_PADDING == padding)
        return false;

    return true;
}

/*****************************************************************************
                                   RTP FORMAT
 ****************************************************************************/

ov_format *open_pcap_as_rtp_format(char const *pcap_path) {

    ov_format *file_fmt = 0;
    ov_format *pcap_fmt = 0;
    ov_format *network_layer_fmt = 0;
    ov_format *udp_fmt = 0;
    ov_format *rtp_fmt = 0;

    file_fmt = ov_format_open(pcap_path, OV_READ);
    if (0 == file_fmt)
        goto error;

    pcap_fmt = ov_format_as(file_fmt, "pcap", 0, 0);
    if (0 == pcap_fmt)
        goto error;

    file_fmt = 0;

    ov_format_pcap_global_header hdr = {0};

    if (!ov_format_pcap_get_global_header(pcap_fmt, &hdr)) {

        PANIC("could not get global PCAP header");
    }

    LOG("PCAP file version: %" PRIu16 ".%" PRIu16 "  Bytes swapped: %s   "
        "snaplen %" PRIu32 "\n",
        hdr.version_major, hdr.version_minor, hdr.bytes_swapped ? "Yes" : "No",
        hdr.snaplen);

    network_layer_fmt = ov_format_pcap_create_network_layer_format(pcap_fmt);

    if (0 == network_layer_fmt) {

        PANIC("Could not create link layer format");
    }

    LOG("Found network layer type %s\n", network_layer_fmt->type);

    pcap_fmt = 0;

    udp_fmt = ov_format_as(network_layer_fmt, "udp", 0, 0);

    if (0 == udp_fmt)
        goto error;

    network_layer_fmt = 0;

    rtp_fmt = ov_format_as(udp_fmt, "rtp", 0, 0);

    udp_fmt = 0;

    return rtp_fmt;

error:

    if (0 != file_fmt) {
        file_fmt = ov_format_close(file_fmt);
    }

    if (0 != pcap_fmt) {
        pcap_fmt = ov_format_close(pcap_fmt);
    }

    if (0 != network_layer_fmt) {
        network_layer_fmt = ov_format_close(network_layer_fmt);
    }

    if (0 != udp_fmt) {
        udp_fmt = ov_format_close(udp_fmt);
    }

    if (0 != rtp_fmt) {
        rtp_fmt = ov_format_close(rtp_fmt);
    }

    OV_ASSERT(0 == file_fmt);
    OV_ASSERT(0 == pcap_fmt);
    OV_ASSERT(0 == network_layer_fmt);
    OV_ASSERT(0 == udp_fmt);
    OV_ASSERT(0 == rtp_fmt);

    return 0;
}

/*----------------------------------------------------------------------------*/

static bool analyse_frame(ov_format const *fmt) {

    ov_format const *pcap = ov_format_get(fmt, "pcap");

    if (0 == pcap) {

        LOG_WARN("PCAP format not found in stack!\n");
        goto error;
    }

    ov_format_pcap_packet_header hdr = {0};

    if (!ov_format_pcap_get_current_packet_header(pcap, &hdr)) {

        LOG_WARN("Could not get PCAP packet header\n");
        goto error;
    }

    LOG("PCAP num bytes: %" PRIu32 " (stored)  %" PRIu32 " (orig)\n",
        hdr.length_stored_bytes, hdr.length_origin_bytes);

    return true;

error:

    return false;
}

/*----------------------------------------------------------------------------*/

static void write_binary(int fd, uint8_t *pcm_buffer, size_t num_bytes) {

    if (0 >= fd)
        return;

    int retval = write(fd, pcm_buffer, num_bytes);

    if (0 > retval) {

        LOG_WARN("Could not write to file: %s (%i)\n", strerror(errno), errno);
    }
}

/*----------------------------------------------------------------------------*/

static void write_csv(FILE *fh, uint8_t *pcm_buffer, size_t num_bytes) {

    int16_t *sample = (int16_t *)pcm_buffer;

    if (0 == fh)
        return;

    fprintf(fh, "# Frame: %zu samples\n", num_bytes / 2);

    for (size_t i = 0; num_bytes / 2 > i; ++i, ++sample) {

        fprintf(fh, "%" PRIi16 "\n", *sample);
    }

    /* 'Frame delimiter' */

    fprintf(fh, "%" PRIuMAX "\n", (uintmax_t)UINT16_MAX);
}

/*----------------------------------------------------------------------------*/

static void analyse_stream(int out_fd, FILE *out_file, ov_buffer encoded_audio,
                           ov_format *fmt, uint32_t ssid) {

    OV_ASSERT((0 != out_fd) || (0 != out_file));
    OV_ASSERT(0 != fmt);

    ov_format const *rtp = ov_format_get(fmt, "rtp");

    OV_ASSERT(0 != rtp);

    ov_format_rtp_header rhdr;

    if ((0 == rtp) || (!ov_format_rtp_get_header(rtp, &rhdr))) {

        LOG_WARN("Could not get RTP header\n");
        return;
    }

    if (ssid != rhdr.ssrc) {

        LOG("Wrong ssid: %" PRIu32 " - ignoring\n", rhdr.ssrc);
        return;
    }

    LOG("Found another frame for %" PRIu32 " (%" PRIu32 " in header)\n", ssid,
        rhdr.ssrc);

    ov_codec *codec = get_codec_for(ssid);

    OV_ASSERT(0 != codec);

    uint8_t pcm_buffer[MAX_SAMPLES_PER_FRAME * sizeof(int16_t)] = {0};

    size_t num_bytes_written =
        codec->decode(codec, rhdr.sequence_number, encoded_audio.start,
                      encoded_audio.length, pcm_buffer, sizeof(pcm_buffer));

    if (0 == num_bytes_written) {
        LOG_WARN("Could not decode audio\n");
    }

    write_binary(out_fd, pcm_buffer, num_bytes_written);

    write_csv(out_file, pcm_buffer, num_bytes_written);
}

/*----------------------------------------------------------------------------*/

static void parse_rtp_analyze_stream(ov_format *rtp_fmt, uint32_t ssid) {

    OV_ASSERT(0 != rtp_fmt);

    ov_buffer encoded_audio = {0};

    size_t num_frames_read = 0;
    size_t num_faulty_chunks = 0;
    size_t total_payload_bytes = 0;

    FILE *out_file = 0;

    char out_file_path[PATH_MAX] = {0};

    char const *extension = "pcm";

    if (configuration.stream.csv) {
        extension = "csv";
    }

    snprintf(out_file_path, sizeof(out_file_path), "%" PRIu32 ".%s", ssid,
             extension);

    int out_fd =
        open(out_file_path, O_CREAT | O_CLOEXEC | O_TRUNC | O_RDWR, S_IRWXU);

    OV_ASSERT(0 < out_fd);

    if (configuration.stream.csv) {

        out_file = fdopen(out_fd, "w");
        OV_ASSERT(0 != out_file);
        out_fd = -1;
    }

    while (ov_format_has_more_data(rtp_fmt)) {

        encoded_audio = ov_format_payload_read_chunk_nocopy(rtp_fmt, 0);

        if (is_srtp(rtp_fmt)) {
            LOG_WARN("Ignoring SRTP frame...\n");
            continue;
        }

        if (!analyse_frame(rtp_fmt)) {

            LOG_WARN("Ignoring frame...\n");
            continue;
        }

        if (0 == encoded_audio.start) {

            ++num_faulty_chunks;
            fprintf(stderr, "Did not get any data - ignoring chunk\n");
            continue;
        }

        ++num_frames_read;
        total_payload_bytes += encoded_audio.length;

        analyse_stream(out_fd, out_file, encoded_audio, rtp_fmt, ssid);
    }

    if (out_file) {

        fclose(out_file);
        out_file = 0;
    }

    if (out_fd > 0) {

        close(out_fd);
        out_fd = -1;
    }

    fprintf(stdout,
            "Analysed %zu RTP frames, totalling to %zu bytes of encoded "
            "audio.    Encountered %zu faulty chunks\n",
            num_frames_read, total_payload_bytes, num_faulty_chunks);
}

/*----------------------------------------------------------------------------*/

static void print_analysis_header(FILE *out) {

    fprintf(out, "# timestamp    timestamp(rtp)   SSRC (Src Port>Dst Port)   "
                 "SEQ number     Num samples    Num channels     encoded "
                 "(octets)\n");
}

/*----------------------------------------------------------------------------*/

static void analyse_audio(FILE *out, ov_buffer encoded_audio, ov_format *fmt) {

    OV_ASSERT(0 != out);
    OV_ASSERT(0 != fmt);

    ov_format const *rtp = ov_format_get(fmt, "rtp");

    OV_ASSERT(0 != rtp);

    ov_format_rtp_header rhdr;

    if ((0 == rtp) || (!ov_format_rtp_get_header(rtp, &rhdr))) {

        LOG_WARN("Could not get RTP header\n");
        return;
    }

    ov_format const *pcap = ov_format_get(fmt, "pcap");

    OV_ASSERT(0 != pcap);

    ov_format_pcap_packet_header phdr;

    if ((0 == pcap) ||
        (!ov_format_pcap_get_current_packet_header(pcap, &phdr))) {

        LOG_WARN("Could not get PCAP header\n");
        return;
    }

    ov_format const *udp = ov_format_get(fmt, "udp");

    OV_ASSERT(0 != udp);

    ov_format_udp_header uhdr;

    if ((0 == udp) || (!ov_format_udp_get_header(udp, &uhdr))) {

        LOG_WARN("Could not get UDP header\n");
        return;
    }

    int num_samples =
        opus_packet_get_samples_per_frame(encoded_audio.start, 48000);

    int num_channels = opus_packet_get_nb_channels(encoded_audio.start);

    fprintf(out,
            "%" PRIu32 ".%" PRIu32 "   %" PRIu32 "   %" PRIu32 " (%" PRIu16
            ">%" PRIu16 ")   %" PRIu16 "    %i   %i   %zu\n",
            phdr.timestamp_secs, phdr.timestamp_usecs, rhdr.timestamp,
            rhdr.ssrc, uhdr.source_port, uhdr.destination_port,
            rhdr.sequence_number, num_samples, num_channels,
            encoded_audio.length);
}

/*----------------------------------------------------------------------------*/

static void track_ssrc(ov_dict *ssrc_dict, ov_format *fmt) {

    ov_format_rtp_header hdr = {0};

    if (!ov_format_rtp_get_header(fmt, &hdr)) {

        LOG_WARN("Could not get RTP header - ignoring frame");
        return;
    }

    uintptr_t ssrc = hdr.ssrc;

    uintptr_t num_ssrc_frames = (uintptr_t)ov_dict_get(ssrc_dict, (void *)ssrc);
    ++num_ssrc_frames;

    ov_dict_set(ssrc_dict, (void *)ssrc, (void *)num_ssrc_frames, 0);
}

/*----------------------------------------------------------------------------*/

static bool print_ssrc(const void *key, void *value, void *data) {

    uintptr_t ssrc = (uintptr_t)key;
    uintptr_t num_frames = (uintptr_t)value;

    fprintf(data, "%12" PRIuPTR "  :  %12" PRIuPTR "\n", ssrc, num_frames);
    return true;
}

/*----------------------------------------------------------------------------*/

static void print_ssrc_dict(FILE *out, ov_dict const *ssrcs) {

    fprintf(out, "Found %" PRIi64 " streams: \n", ov_dict_count(ssrcs));

    ov_dict_for_each((ov_dict *)ssrcs, out, print_ssrc);
}

/*----------------------------------------------------------------------------*/

static uint64_t hash_uintptr_t(const void *uintptr) {

    return (uintptr_t)uintptr;
}

/*----------------------------------------------------------------------------*/

static bool match_uintptr_t(const void *uintptr1, const void *uintptr2) {

    return uintptr1 == uintptr2;
}

/*----------------------------------------------------------------------------*/

static void parse_rtp(ov_format *rtp_fmt, bool print_ssrcs) {

    OV_ASSERT(0 != rtp_fmt);

    ov_dict_config cfg = {
        .slots = 20,
        .key.hash = hash_uintptr_t,
        .key.match = match_uintptr_t,
    };

    ov_dict *ssrc_dict = ov_dict_create(cfg);

    OV_ASSERT(0 != ssrc_dict);

    ov_buffer encoded_audio = {0};

    size_t num_srtp_frames = 0;
    size_t num_frames_read = 0;
    size_t num_faulty_chunks = 0;
    size_t total_payload_bytes = 0;

    print_analysis_header(stdout);

    while (ov_format_has_more_data(rtp_fmt)) {

        encoded_audio = ov_format_payload_read_chunk_nocopy(rtp_fmt, 0);

        if (is_srtp(rtp_fmt)) {
            ++num_srtp_frames;
            continue;
        }

        if (!analyse_frame(rtp_fmt)) {
            LOG_WARN("Ignoring frame...\n");
            continue;
        }

        if (0 == encoded_audio.start) {
            ++num_faulty_chunks;
            fprintf(stderr, "Did not get any data - ignoring chunk\n");
            continue;
        }

        ++num_frames_read;
        total_payload_bytes += encoded_audio.length;

        if (!print_ssrcs) {
            analyse_audio(stdout, encoded_audio, rtp_fmt);
        }

        track_ssrc(ssrc_dict, rtp_fmt);
    }

    fprintf(stdout,
            "Analysed %zu RTP frames, totalling to %zu bytes of encoded "
            "audio.    Encountered %zu SRTP, %zu faulty chunks\n",
            num_frames_read, total_payload_bytes, num_srtp_frames,
            num_faulty_chunks);

    if (print_ssrcs) {
        print_ssrc_dict(stdout, ssrc_dict);
    }

    ssrc_dict = ov_dict_free(ssrc_dict);
}

/*----------------------------------------------------------------------------*/

int main(int argc, char **argv) {

    ov_log_init();

    ov_format *rtp_fmt = 0;

    configuration = get_config(argc, argv);

    if (configuration.enable_log) {
        ov_log_set_output(0, 0, OV_LOG_INFO,
                          (ov_log_output){
                              .use.systemd = true,
                              .filehandle = fileno(stderr),
                          });
    }

    if (0 == configuration.pcap_file) {

        usage(argv[0]);
    }

    if (!ov_format_registry_register_default(0)) {

        PANIC("Could not initialize formats");
    }

    if (!initialize_codec_registry()) {

        PANIC("Could not initialize codec registry");
    }

    rtp_fmt = open_pcap_as_rtp_format(configuration.pcap_file);

    if (0 == rtp_fmt) {

        PANIC("Could not open %s as PCAP/Ethernet/IPv4/UDP/RTP format",
              configuration.pcap_file);
    }

    if (configuration.stream.analyze) {

        parse_rtp_analyze_stream(rtp_fmt, configuration.stream.ssid);

    } else {

        parse_rtp(rtp_fmt, configuration.list_streams);
    }

    rtp_fmt = ov_format_close(rtp_fmt);

    if (!free_codec_registry()) {

        LOG_WARN("Could not free codec registry");
    }

    if (!ov_format_registry_clear(0)) {

        LOG_WARN("Could not free format registry");
    }

    OV_ASSERT(0 == rtp_fmt);

    ov_codec_factory_free(0);

    ov_log_close();

    return EXIT_SUCCESS;
}
