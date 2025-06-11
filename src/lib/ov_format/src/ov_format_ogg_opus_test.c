/***
        ------------------------------------------------------------------------

        Copyright (c) 2023 German Aerospace Center DLR e.V. (GSOC)

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
/*----------------------------------------------------------------------------*/

#include "ov_format_ogg_opus.c"
#include <ov_base/ov_registered_cache.h>
#include <ov_test/ov_test.h>

/*----------------------------------------------------------------------------*/

static int initialize() {

    testrun(ov_format_registry_register_default(0));
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_id_header() {

    uint8_t buf[100];

    IdHeader ref_header = {

        .num_channels = 1,
        .options.output_gain_db = 0,
        .options.preskip_samples = 1022,
        .options.samplerate_hz = 1431,

    };

    uint8_t *write_ptr = write_id_header_to(ref_header, 1, buf, sizeof(buf));
    testrun(write_ptr == buf + ID_HEADER_SIZE);

    IdHeader header = {0};

    testrun(0 == read_id_header_from(buf, sizeof(buf), 0));
    testrun(ID_HEADER_SIZE == read_id_header_from(buf, sizeof(buf), &header));

    testrun(1 == header.num_channels);
    testrun(0 == header.options.output_gain_db);
    testrun(1022 == header.options.preskip_samples);
    testrun(1431 == header.options.samplerate_hz);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static bool strings_equal(char const *s1, char const *s2) {

    return (s1 == s2) || ((s1 != 0) && (s2 != 0) && (0 == strcmp(s1, s2)));
}

/*----------------------------------------------------------------------------*/

static bool string_arrays_equal(size_t num_strings,
                                char const **a1,
                                char const **a2) {

    for (size_t i = 0; i < num_strings; ++i) {
        if ((a1 != a2) && (0 != strcmp(a1[i], a2[i]))) {
            return false;
        }
    }

    return true;
}

/*****************************************************************************
                                Comment headers
 ****************************************************************************/

static char *decode_string_with_len(uint8_t const **read_ptr,
                                    size_t *available_octets,
                                    size_t str_len) {

    if (ov_ptr_valid(read_ptr, "Cannot read string - no data") &&
        ov_ptr_valid(*read_ptr, "Cannot read string - no data") &&
        ov_ptr_valid(available_octets, "Cannot read string - internal error") &&
        ov_cond_valid(*available_octets >= str_len,
                      "Cannot read string - insufficient data")) {

        char *str = malloc(str_len + 1);
        memcpy(str, *read_ptr, str_len);
        str[str_len] = 0;

        *read_ptr += str_len;
        *available_octets -= str_len;

        return str;

    } else {

        return 0;
    }
}

/*----------------------------------------------------------------------------*/

static char *decode_string(uint8_t const **read_ptr, size_t *available_octets) {

    if (ov_ptr_valid(available_octets, "Cannot decode string - no data") &&
        ov_cond_valid(4 <= *available_octets,
                      "Cannot decode string - insufficient data")) {

        *available_octets -= 4;

        return decode_string_with_len(
            read_ptr, available_octets, decode32(read_ptr));

    } else {

        return 0;
    }
}

/*----------------------------------------------------------------------------*/

static size_t read_comment_header_from(uint8_t const *read_ptr,
                                       size_t available_octets,
                                       CommentHeader *header) {

    if (ov_cond_valid(MIN_COMMENT_HEADER_SIZE <= available_octets,
                      "Cannot read COMMENT header - header incomplete") &&
        ov_ptr_valid(
            read_ptr, "Cannot read COMMENT Header - no input (0 pointer)") &&
        ov_ptr_valid(header,
                     "Cannot read COMMENT Header - no id header object to "
                     "store data") &&
        ov_cond_valid(
            0 == memcmp(read_ptr, COMMENT_HEADER_ID, COMMENT_HEADER_ID_LEN),
            "Cannot read COMMENT Header - Invalid magic bytes")) {

        uint8_t const *current_pos = read_ptr + 8;

        header->vendor = decode_string(&current_pos, &available_octets);
        header->num_comments = decode32(&current_pos);

        available_octets -= 4;

        header->comments = calloc(1, header->num_comments * sizeof(char *));

        for (size_t i = 0; i < header->num_comments; ++i) {
            header->comments[i] =
                decode_string(&current_pos, &available_octets);
        }

        return current_pos - read_ptr;

    } else {

        return 0;
    }
}

/*----------------------------------------------------------------------------*/

static bool comment_headers_equal(const CommentHeader h1,
                                  const CommentHeader h2) {

    return strings_equal(h1.vendor, h2.vendor) &&
           (h1.num_comments == h2.num_comments) &&
           string_arrays_equal(h1.num_comments,
                               (char const **)h1.comments,
                               (char const **)h2.comments);
}

/*----------------------------------------------------------------------------*/

int test_comment_header() {

    char const *comm1 = "COMMENT";
    char const *comm2 = "Another comment";
    char const *comm3 = "Last comment";
    char const *comments[] = {comm1, comm2, comm3};

    CommentHeader ref_header = {0};

    testrun(0 == write_comment_header_to(ref_header, 0, 0));

    uint8_t buf[10000] = {0};

    testrun(0 == write_comment_header_to(ref_header, buf, 0));
    testrun(0 == write_comment_header_to(ref_header, 0, sizeof(buf)));
    testrun(0 == write_comment_header_to(ref_header, buf, sizeof(buf)));

    ref_header.vendor = "";

    uint8_t *endptr = write_comment_header_to(ref_header, buf, sizeof(buf));

    testrun(buf < endptr);
    size_t written_octets = endptr - buf;

    CommentHeader header = {0};

    testrun(written_octets ==
            read_comment_header_from(buf, written_octets, &header));

    testrun(comment_headers_equal(ref_header, header));

    comment_header_clear(&header);
    memset(buf, 0, sizeof(buf));

    ref_header = (CommentHeader){
        .vendor = "Aaaaaaaah",
        .num_comments = 3,
        .comments = (char **)comments,
    };

    testrun(0 == write_comment_header_to(ref_header, buf, 0));
    testrun(0 == write_comment_header_to(ref_header, 0, sizeof(buf)));

    endptr = write_comment_header_to(ref_header, buf, sizeof(buf));

    testrun(buf < endptr);
    written_octets = endptr - buf;

    testrun(0 == read_comment_header_from(0, 0, 0));
    testrun(0 == read_comment_header_from(buf, 0, 0));
    testrun(0 == read_comment_header_from(0, written_octets, 0));
    testrun(0 == read_comment_header_from(buf, written_octets, 0));
    testrun(0 == read_comment_header_from(0, 0, &header));
    testrun(0 == read_comment_header_from(buf, 0, &header));
    testrun(0 == read_comment_header_from(buf, 0, &header));
    testrun(written_octets ==
            read_comment_header_from(buf, written_octets, &header));

    comment_header_clear(&header);

    memset(&ref_header, 0, sizeof(ref_header));

    // Proper key-value handling

    ref_header.vendor = strdup("Collybia");

    testrun(comment_add(&ref_header, "Oekologie", "Saprobiontisch"));
    testrun(comment_add(&ref_header, "Familie", "Tricholomaceae"));
    testrun(comment_add(&ref_header, "Gattung", "Collybia"));
    testrun(comment_add(&ref_header, "Epitheton", "tuberosa"));
    testrun(comment_add(&ref_header, "Sporenpulver", "weiss"));

    testrun(0 == comment_get(&ref_header, "Lebensraum"));
    testrun(0 == comment_get(&ref_header, ""));
    testrun(0 == comment_get(&ref_header, "G"));
    testrun(0 == comment_get(&ref_header, "Ga"));
    testrun(0 == comment_get(&ref_header, "Gat"));
    testrun(0 == comment_get(&ref_header, "Gatt"));
    testrun(0 == comment_get(&ref_header, "Gattu"));
    testrun(0 == comment_get(&ref_header, "Gattun"));

    char const *value = comment_get(&ref_header, "Gattung");

    testrun(0 != value);
    testrun(0 == strcmp(value, "Collybia"));

    testrun(0 == comment_get(&ref_header, "Gattunge"));

    comment_header_clear(&ref_header);

    // Check the comments are recoverable after (de-) serialization
    //

    ref_header.vendor = strdup("lawn");

    testrun(comment_add(&ref_header, "Oekologie", "Saprobiontisch"));
    testrun(comment_add(&ref_header, "Familie", "Tricholomaceae"));
    testrun(comment_add(&ref_header, "Gattung", "Collybia"));
    testrun(comment_add(&ref_header, "Epitheton", "tuberosa"));
    testrun(comment_add(&ref_header, "Sporenpulver", "weiss"));

    endptr = write_comment_header_to(ref_header, buf, sizeof(buf));

    testrun(buf < endptr);
    written_octets = endptr - buf;

    memset(&header, 0, sizeof(header));

    testrun(written_octets ==
            read_comment_header_from(buf, written_octets, &header));

    testrun(comment_headers_equal(ref_header, header));

    testrun(0 == strcmp(header.vendor, "lawn"));

    testrun(0 == comment_get(&header, "Lebensraum"));
    testrun(0 == comment_get(&header, ""));
    testrun(0 == comment_get(&header, "G"));
    testrun(0 == comment_get(&header, "Ga"));
    testrun(0 == comment_get(&header, "Gat"));
    testrun(0 == comment_get(&header, "Gatt"));
    testrun(0 == comment_get(&header, "Gattu"));
    testrun(0 == comment_get(&header, "Gattun"));

    value = comment_get(&header, "Gattung");

    testrun(0 != value);
    testrun(0 == strcmp(value, "Collybia"));

    testrun(0 == comment_get(&header, "Gattunge"));

    comment_header_clear(&ref_header);
    comment_header_clear(&header);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_format_ogg_opus_create() {

    ov_format *fmt = ov_format_open("/tmp/ogg_opus.ogg", OV_WRITE);
    // ov_format *fmt = ov_format_from_memory(0, 64, OV_WRITE);
    testrun(0 != fmt);

    ov_format *oggopus = ov_format_ogg_opus_create(fmt, 0);
    testrun(0 != oggopus);

    testrun(ov_format_ogg_opus_comment_set(oggopus, "Ali", "baba"));
    testrun(ov_format_ogg_opus_comment_set(oggopus, "Buch", "botanik"));

    ov_buffer *chunk =
        ov_buffer_from_string("Abra kadabra- dreimal schwarzer Kater");

    testrun(-1 < ov_format_payload_write_chunk(oggopus, chunk));

    chunk = ov_buffer_free(chunk);

    oggopus = ov_format_close(oggopus);

    return testrun_log_success();
}

/*****************************************************************************
                                    HELPERS
 ****************************************************************************/

typedef struct {

    char const *filepath;
    uint8_t *mem;

    size_t mem_len;

} FormatSource;

/*----------------------------------------------------------------------------*/

#define format(mode, ...) format_int(mode, (FormatSource){__VA_ARGS__})

static ov_format *format_int(ov_format_mode mode, FormatSource source) {

    if (0 != source.filepath) {

        return ov_format_open(source.filepath, mode);

    } else if (0 != source.mem_len) {

        return 0;

    } else {

        return 0;
    }
}

/*----------------------------------------------------------------------------*/

#define ogg_opus_format(lower_format, ...)                                     \
    ogg_opus_format_int(lower_format, (ov_format_ogg_opus_options){__VA_ARGS__})

static ov_format *ogg_opus_format_int(ov_format *lower_format,
                                      ov_format_ogg_opus_options opts) {

    if (0 != lower_format) {
        return ov_format_ogg_opus_create(lower_format, &opts);
    } else {
        return 0;
    }
}

/*----------------------------------------------------------------------------*/

int test_impl_write_chunk() {

    testrun(0 > impl_write_chunk(0, 0, 0));

    ov_buffer *data =
        ov_buffer_from_string("abra kadabra dreimal schwarzer Kater");

    ov_format *oggopus = ogg_opus_format(
        format(OV_WRITE, .filepath = "/tmp/ogg_opus_test_impl_write_chunk.ogg"),
        .preskip_samples = 132,
        .output_gain_db = 0.2,
        .samplerate_hz = 41289);

    testrun(0 > impl_write_chunk(0, 0, 0));
    testrun(0 > impl_write_chunk(oggopus, 0, 0));
    testrun(0 > impl_write_chunk(0, data, 0));

    testrun(0 > ov_format_payload_write_chunk(oggopus, 0));
    testrun(0 > ov_format_payload_write_chunk(0, data));
    testrun(ov_buffer_len(data) ==
            ov_format_payload_write_chunk(oggopus, data));

    data = ov_buffer_free(data);
    testrun(0 == data);

    oggopus = ov_format_close(oggopus);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static bool comment_is(ov_format *f,
                       char const *comment_key,
                       char const *expected_value) {

    char const *c = ov_format_ogg_opus_comment(f, comment_key);

    if (0 == expected_value) {
        return 0 == c;
    } else {

        return (0 != c) && (0 == strcmp(c, expected_value));
    }
}

/*----------------------------------------------------------------------------*/

int test_impl_next_chunk() {

    ov_buffer *data =
        ov_buffer_from_string("abra kadabra dreimal schwarzer Kater");

    ov_format *oggopus = ogg_opus_format(
        format(OV_WRITE, .filepath = "/tmp/ogg_opus_test_impl_next_chunk.ogg"),
        .preskip_samples = 132,
        .output_gain_db = 0.2,
        .samplerate_hz = 41289);

    testrun(ov_buffer_len(data) ==
            ov_format_payload_write_chunk(oggopus, data));

    oggopus = ov_format_close(oggopus);
    testrun(0 == oggopus);

    /*------------------------------------------------------------------------*/
    // Now read it back

    oggopus = ogg_opus_format(
        format(OV_READ, .filepath = "/tmp/ogg_opus_test_impl_next_chunk.ogg"),
        .preskip_samples = 132,
        .output_gain_db = 0.2,
        .samplerate_hz = 41289);

    ov_buffer chunk = ov_format_payload_read_chunk_nocopy(oggopus, 0);

    testrun(0 != chunk.start);

    testrun(chunk.length == data->length);
    testrun(0 == strcmp((char const *)chunk.start, (char const *)data->start));

    chunk.start = ov_free(chunk.start);

    data = ov_buffer_free(data);
    testrun(0 == data);

    oggopus = ov_format_close(oggopus);
    testrun(0 == oggopus);

    /*------------------------------------------------------------------------*/
    // With single comment

    data = ov_buffer_from_string("Das ist das Hexeneinmaleins");

    oggopus = ogg_opus_format(
        format(OV_WRITE, .filepath = "/tmp/ogg_opus_test_impl_next_chunk.ogg"),
        .preskip_samples = 132,
        .output_gain_db = 0.2,
        .samplerate_hz = 41289);

    testrun(0 != oggopus);

    ov_format_ogg_opus_comment_set(oggopus,
                                   "albatross",
                                   "Soll das Werk den Meister loben, doch der "
                                   "Segen kommt von oben");

    testrun(ov_buffer_len(data) ==
            ov_format_payload_write_chunk(oggopus, data));

    oggopus = ov_format_close(oggopus);
    testrun(0 == oggopus);

    oggopus = ogg_opus_format(
        format(OV_READ, .filepath = "/tmp/ogg_opus_test_impl_next_chunk.ogg"));

    testrun(0 != oggopus);

    testrun(comment_is(oggopus,
                       "albatross",
                       "Soll das Werk den Meister loben, doch der "
                       "Segen kommt von oben"));

    testrun(comment_is(oggopus, "b", 0));
    testrun(comment_is(oggopus, "beta", 0));

    chunk = ov_format_payload_read_chunk_nocopy(oggopus, 0);

    testrun(comment_is(oggopus, "b", 0));
    testrun(comment_is(oggopus, "beta", 0));

    testrun(comment_is(oggopus,
                       "albatross",
                       "Soll das Werk den Meister loben, doch der "
                       "Segen kommt von oben"));

    testrun(0 != chunk.start);

    testrun(chunk.length == data->length);
    testrun(0 == strcmp((char const *)chunk.start, (char const *)data->start));

    chunk.start = ov_free(chunk.start);

    data = ov_buffer_free(data);
    testrun(0 == data);

    oggopus = ov_format_close(oggopus);
    testrun(0 == oggopus);

    /*------------------------------------------------------------------------*/
    // With several comments

    data = ov_buffer_from_string("Das ist das Hexeneinmaleins");

    oggopus = ogg_opus_format(
        format(OV_WRITE, .filepath = "/tmp/ogg_opus_test_impl_next_chunk.ogg"),
        .preskip_samples = 132,
        .output_gain_db = 0.2,
        .samplerate_hz = 41289);

    testrun(0 != oggopus);

    ov_format_ogg_opus_comment_set(
        oggopus, "gamma", "Es ist was faul im Staate Daenemark");

    ov_format_ogg_opus_comment_set(oggopus, "epsilon", "Soll das");
    ov_format_ogg_opus_comment_set(oggopus, "zeta", "Werk");
    ov_format_ogg_opus_comment_set(oggopus, "eta", "den Meister");
    ov_format_ogg_opus_comment_set(oggopus, "theta", "loben");

    ov_format_ogg_opus_comment_set(oggopus,
                                   "albatross",
                                   "Soll das Werk den Meister loben, doch der "
                                   "Segen kommt von oben");

    ov_format_ogg_opus_comment_set(oggopus, "delta", "Soll");

    testrun(ov_buffer_len(data) ==
            ov_format_payload_write_chunk(oggopus, data));

    oggopus = ov_format_close(oggopus);
    testrun(0 == oggopus);

    oggopus = ogg_opus_format(
        format(OV_READ, .filepath = "/tmp/ogg_opus_test_impl_next_chunk.ogg"),
        .preskip_samples = 132,
        .output_gain_db = 0.2,
        .samplerate_hz = 41289);

    testrun(comment_is(oggopus,
                       "albatross",
                       "Soll das Werk den Meister loben, doch der "
                       "Segen kommt von oben"));

    testrun(comment_is(oggopus, "Simmsalabim", 0));

    testrun(
        comment_is(oggopus, "gamma", "Es ist was faul im Staate Daenemark"));
    testrun(comment_is(oggopus, "epsilon", "Soll das"));
    testrun(comment_is(oggopus, "zeta", "Werk"));
    testrun(comment_is(oggopus, "eta", "den Meister"));
    ov_format_ogg_opus_comment_set(oggopus, "theta", "loben");

    testrun(comment_is(oggopus,
                       "albatross",
                       "Soll das Werk den Meister loben, doch der "
                       "Segen kommt von oben"));

    testrun(comment_is(oggopus, "Simmsalabim", 0));

    testrun(comment_is(oggopus, "delta", "Soll"));

    chunk = ov_format_payload_read_chunk_nocopy(oggopus, 0);

    testrun(comment_is(oggopus, "epsilon", "Soll das"));
    testrun(comment_is(oggopus, "zeta", "Werk"));
    testrun(comment_is(oggopus, "eta", "den Meister"));
    testrun(comment_is(oggopus,
                       "albatross",
                       "Soll das Werk den Meister loben, doch der "
                       "Segen kommt von oben"));

    testrun(comment_is(oggopus, "Simmsalabim", 0));

    testrun(
        comment_is(oggopus, "gamma", "Es ist was faul im Staate Daenemark"));
    ov_format_ogg_opus_comment_set(oggopus, "theta", "loben");

    testrun(comment_is(oggopus,
                       "albatross",
                       "Soll das Werk den Meister loben, doch der "
                       "Segen kommt von oben"));

    testrun(comment_is(oggopus, "Simmsalabim", 0));

    testrun(comment_is(oggopus, "delta", "Soll"));

    testrun(0 != chunk.start);

    testrun(chunk.length == data->length);
    testrun(0 == strcmp((char const *)chunk.start, (char const *)data->start));

    chunk.start = ov_free(chunk.start);

    data = ov_buffer_free(data);
    testrun(0 == data);

    oggopus = ov_format_close(oggopus);
    testrun(0 == oggopus);

    /*------------------------------------------------------------------------*/
    // With several comments & giant comment

    char const *die_glocke =
        "Fest gemauert in der Erden"
        " Steht die Form aus Lehm gebrannt."
        " Heute muß die Glocke werden!"
        " Frisch, Gesellen, seid zur Hand!"
        "5Von der Stirne heiß"
        " Rinnen muß der Schweiß,"
        " Soll das Werk den Meister loben;"
        " Doch der Segen kommt von oben."
        ""
        " Zum Werke, das wir ernst bereiten,"
        "10Geziemt sich wohl ein ernstes Wort;"
        " Wenn gute Reden sie begleiten,"
        " Dann fließt die Arbeit munter fort."
        " So laßt uns jetzt mit Fleiß betrachten,"
        " Was durch die schwache Kraft entspringt;"
        "15Den schlechten Mann muss man verachten,"
        " Der nie bedacht, was er vollbringt."
        " Das ist’s ja, was den Menschen zieret,"
        " Und dazu ward ihm der Verstand,"
        " Daß er im innern Herzen spüret,"
        "20Was er erschafft mit seiner Hand."
        ""
        " Nehmet Holz vom Fichtenstamme,"
        " Doch recht trocken laßt es sein,"
        " Daß die eingepresste Flamme"
        " Schlage zu dem Schwalch hinein!"
        "25Kocht des Kupfers Brei!"
        " Schnell das Zinn herbei,"
        " Daß die zähe Glockenspeise"
        " Fließe nach der rechten Weise!"
        ""
        " Was in des Dammes tiefer Grube"
        "30Die Hand mit Feuers Hülfe baut,"
        " Hoch auf des Turmes Glockenstube,"
        " Da wird es von uns zeugen laut."
        " Noch dauern wird’s in späten Tagen"
        " Und rühren vieler Menschen Ohr,"
        "35Und wird mit den Betrübten klagen"
        " Und stimmen zu der Andacht Chor."
        " Was unten tief dem Erdensohne"
        " Das wechselnde Verhängnis bringt,"
        " Das schlägt an die metallne Krone,"
        "40Die es erbaulich weiter klingt."
        ""
        " Weiße Blasen seh’ ich springen;"
        " Wohl! Die Massen sind im Fluß."
        " Laßt’s mit Aschenfalz durchdringen,"
        " Das befördert schnell den Guss."
        "45Auch vom Schaume rein"
        " Muß die Mischung sein,"
        " Daß vom reinlichen Metalle"
        " Rein und voll die Stimme schalle."
        ""
        " Denn mit der Freude Feierklange"
        "50Begrüßt sie das geliebte Kind"
        " Auf seines Lebens erstem Gange,"
        " Den es in Schlafes Arm beginnt;"
        " Ihm ruhen noch im Zeitenschoße"
        " Die schwarzen und die heitern Lose;"
        "55Der Mutterliebe zarte Sorgen"
        " Bewachen seinen goldnen Morgen –"
        " Die Jahre fliehen pfeilgeschwind."
        " Vom Mädchen reißt sich stolz der Knabe,"
        " Er stürmt ins Leben wild hinaus,"
        "60Durchmisst die Welt am Wanderstabe,"
        " Fremd kehrt er heim ins Vaterhaus."
        " Und herrlich in der Jugend Prangen,"
        " Wie ein Gebild aus Himmelshöhn,"
        " Mit züchtigen, verschämten Wangen"
        "65Sieht er die Jungfrau vor sich stehn."
        " Da faßt ein namenloses Sehnen"
        " Des Jünglings Herz, er irrt allein,"
        " Aus seinen Augen brechen Tränen,"
        " Er flieht der Brüder wilden Reihn."
        "70Errötend folgt er ihren Spuren"
        " Und ist von ihrem Gruß beglückt,"
        " Das Schönste sucht er auf den Fluren,"
        " Womit er seine Liebe schmückt."
        " O zarte Sehnsucht, süßes Hoffen,"
        "75Der ersten Liebe goldne Zeit,"
        " Das Auge sieht den Himmel offen,"
        " Es schwelgt das Herz in Seligkeit;"
        " O dass sie ewig grünen bleibe,"
        " Die schöne Zeit der jungen Liebe!"
        ""
        "80Wie sich schon die Pfeifen bräunen!"
        " Dieses Stäbchen tauch’ ich ein,"
        " Sehn wir’s überglast erscheinen,"
        " Wird’s zum Gusse zeitig sein,"
        " Jetzt, Gesellen, frisch!"
        "85Prüft mir das Gemisch,"
        " Ob das Spröde mit dem Weichen"
        " Sich vereint zum guten Zeichen."
        ""
        " Denn wo das Strenge mit dem Zarten,"
        " Wo Starkes sich und Mildes paarten,"
        "90Da gibt es einen guten Klang."
        " Drum prüfe, wer sich ewig bindet,"
        " Ob sich das Herz zum Herzen findet!"
        " Der Wahn ist kurz, die Reu ist lang."
        " Lieblich in der Bräute Locken"
        "95Spielt der jungfräuliche Kranz,"
        " Wenn die hellen Kirchenglocken"
        " Laden zu des Festes Glanz."
        " Ach! des Lebens schönste Feier"
        " Endigt auch den Lebensmai,"
        "100Mit dem Gürtel, mit dem Schleier"
        " Reißt der schöne Wahn entzwei"
        " Die Leidenschaft flieht,"
        " Die Liebe muß bleiben;"
        " Die Blume verblüht,"
        "105Die Frucht muß treiben."
        " Der Mann muß hinaus"
        " Ins feindliche Leben,"
        " Muss wirken und streben"
        " Und pflanzen und schaffen,"
        "110Erlisten, erraffen,"
        " Muss wetten und wagen,"
        " Das Glück zu erjagen."
        " Da strömet herbei die unendliche Gabe,"
        " Es füllt sich der Speicher mit köstlicher Habe,"
        "115Die Räume wachsen, es dehnt sich das Haus."
        " Und drinnen waltet"
        " Die züchtige Hausfrau,"
        " Die Mutter der Kinder,"
        " Und herrschet weise"
        "120Im häuslichen Kreise,"
        " Und lehret die Mädchen"
        " Und wehret den Knaben,"
        " Und reget ohn’ Ende"
        " Die fleißigen Hände,"
        "125Und mehrt den Gewinn"
        " Mit ordnendem Sinn,"
        " Und füllet mit Schätzen die duftenden Laden,"
        " Und dreht um die schnurrende Spindel den Faden,"
        " Und sammelt im reinlich geglätteten Schrein"
        "130Die schimmernde Wolle, den schneeigten Lein,"
        " Und füget zum Guten den Glanz und den Schimmer"
        " Und ruhet nimmer."
        ""
        " Und der Vater mit frohem Blick"
        " Von des Hauses weitschauendem Giebel"
        "135Überzählet sein blühend Glück."
        " Siehet der Pfosten ragende Bäume"
        " Und der Scheunen gefüllte Räume,"
        " Und die Speicher, vom Segen gebogen,"
        " Und des Kornes bewegte Wogen,"
        "140Rühmt sich mit stolzem Mund:"
        " Fest, wie der Erde Grund,"
        " Gegen des Unglücks Macht"
        " Steht mir des Hauses Pracht!"
        " Doch mit des Geschickes Mächten"
        "145Ist kein ew’ger Bund zu flechten,"
        " Und das Unglück schreitet schnell."
        ""
        " Wohl! nun kann der Guss beginnen,"
        " Schön gezacket ist der Bruch."
        " Doch bevor wir’s lassen rinnen,"
        "150Betet einen frommen Spruch!"
        " Stoßt den Zapfen aus!"
        " Gott bewahr’ das Haus!"
        " Rauchend in des Henkels Bogen"
        " Schießt’s mit feuerbraunen Wogen."
        ""
        "155Wohltätig ist des Feuers Macht,"
        " Wenn sie der Mensch bezähmt, bewacht,"
        " Und was er bildet, was er schafft,"
        " Das dankt er dieser Himmelskraft;"
        " Doch furchtbar wird die Himmelskraft,"
        "160Wenn sie der Fessel sich entrafft,"
        " Einhertritt auf der eignen Spur,"
        " Die freie Tochter der Natur."
        " Wehe, wenn sie losgelassen,"
        " Wachsend ohne Widerstand,"
        "165Durch die volkbelebten Gassen"
        " Wälzt den ungeheuren Brand!"
        " Denn die Elemente hassen"
        " Das Gebild der Menschenhand."
        " Aus der Wolke"
        "170Quillt der Segen,"
        " Strömt der Regen;"
        " Aus der Wolke, ohne Wahl,"
        " Zuckt der Strahl."
        " Hört ihr’s wimmern hoch vom Turm?"
        "175Das ist Sturm!"
        " Rot, wie Blut,"
        " Ist der Himmel;"
        " Das ist nicht des Tages Glut!"
        " Welch Getümmel"
        "180Straßen auf!"
        " Dampf wallt auf!"
        " Flackernd steigt die Feuersäule,"
        " Durch der Straße lange Zeile"
        " Wächst es fort mit Windeseile;"
        "185Kochend, wie aus Ofens Rachen,"
        " Glühn die Lüfte, Balken krachen,"
        " Pfosten stürzen, Fenster klirren,"
        " Kinder jammern, Mütter irren,"
        " Tiere wimmern"
        "190Unter Trümmern;"
        " Alles rennet, rettet, flüchtet,"
        " Taghell ist die Nacht gelichtet;"
        " Durch der Hände lange Kette"
        " Um die Wette"
        "195Fliegt der Eimer; hoch im Bogen"
        " Spritzen Quellen Wasserwogen."
        " Heulend kommt der Sturm geflogen,"
        " Der die Flamme brausend sucht;"
        " Prasselnd in die dürre Frucht"
        "200Fällt sie, in des Speichers Räume,"
        " In der Sparren dürre Bäume,"
        " Und als wollte sie im Wehen"
        " Mit sich fort der Erde Wucht"
        " Reißen in gewalt’ger Flucht,"
        "205Wächst sie in des Himmels Höhen"
        " Riesengroß!"
        " Hoffnungslos"
        " Weicht der Mensch der Götterstärke,"
        " Müßig sieht er seine Werke"
        "210Und bewundernd untergehn."
        ""
        " Leergebrannt"
        " Ist die Stätte,"
        " Wilder Stürme rauhes Bette."
        " In den öden Fensterhöhlen"
        "215Wohnt das Grauen,"
        " Und des Himmels Wolken schauen"
        " Hoch hinein."
        ""
        " Einen Blick"
        " Nach dem Grabe"
        "220Seiner Habe"
        " Sendet noch der Mensch zurück –"
        " Greift fröhlich dann zum Wanderstabe."
        " Was Feuers Wut ihm auch geraubt,"
        " Ein süßer Trost ist ihm geblieben,"
        "225Er zählt die Häupter seiner Lieben,"
        " Und sieh! ihm fehlt kein teures Haupt.";

    data = ov_buffer_from_string("Das ist das Hexeneinmaleins");

    oggopus = ogg_opus_format(
        format(OV_WRITE, .filepath = "/tmp/ogg_opus_test_impl_next_chunk.ogg"),
        .preskip_samples = 132,
        .output_gain_db = 0.2,
        .samplerate_hz = 41289);

    testrun(0 != oggopus);

    ov_format_ogg_opus_comment_set(
        oggopus, "gamma", "Es ist was faul im Staate Daenemark");

    ov_format_ogg_opus_comment_set(oggopus, "epsilon", "Soll das");
    ov_format_ogg_opus_comment_set(oggopus, "zeta", "Werk");
    ov_format_ogg_opus_comment_set(oggopus, "iota", die_glocke);
    ov_format_ogg_opus_comment_set(oggopus, "eta", "den Meister");
    ov_format_ogg_opus_comment_set(oggopus, "theta", "loben");

    ov_format_ogg_opus_comment_set(oggopus,
                                   "albatross",
                                   "Soll das Werk den Meister loben, doch der "
                                   "Segen kommt von oben");

    ov_format_ogg_opus_comment_set(oggopus, "delta", "Soll");

    testrun(ov_buffer_len(data) ==
            ov_format_payload_write_chunk(oggopus, data));

    oggopus = ov_format_close(oggopus);
    testrun(0 == oggopus);

    oggopus = ogg_opus_format(
        format(OV_READ, .filepath = "/tmp/ogg_opus_test_impl_next_chunk.ogg"),
        .preskip_samples = 132,
        .output_gain_db = 0.2,
        .samplerate_hz = 41289);

    testrun(comment_is(oggopus,
                       "albatross",
                       "Soll das Werk den Meister loben, doch der "
                       "Segen kommt von oben"));

    testrun(comment_is(oggopus, "Simmsalabim", 0));

    testrun(comment_is(oggopus, "iota", die_glocke));

    testrun(
        comment_is(oggopus, "gamma", "Es ist was faul im Staate Daenemark"));
    testrun(comment_is(oggopus, "epsilon", "Soll das"));
    testrun(comment_is(oggopus, "zeta", "Werk"));
    testrun(comment_is(oggopus, "eta", "den Meister"));
    ov_format_ogg_opus_comment_set(oggopus, "theta", "loben");

    testrun(comment_is(oggopus,
                       "albatross",
                       "Soll das Werk den Meister loben, doch der "
                       "Segen kommt von oben"));

    testrun(comment_is(oggopus, "Simmsalabim", 0));

    testrun(comment_is(oggopus, "delta", "Soll"));

    chunk = ov_format_payload_read_chunk_nocopy(oggopus, 0);

    testrun(comment_is(oggopus, "epsilon", "Soll das"));
    testrun(comment_is(oggopus, "zeta", "Werk"));
    testrun(comment_is(oggopus, "eta", "den Meister"));
    testrun(comment_is(oggopus,
                       "albatross",
                       "Soll das Werk den Meister loben, doch der "
                       "Segen kommt von oben"));

    testrun(comment_is(oggopus, "iota", die_glocke));
    testrun(comment_is(oggopus, "Simmsalabim", 0));

    testrun(
        comment_is(oggopus, "gamma", "Es ist was faul im Staate Daenemark"));
    ov_format_ogg_opus_comment_set(oggopus, "theta", "loben");

    testrun(comment_is(oggopus,
                       "albatross",
                       "Soll das Werk den Meister loben, doch der "
                       "Segen kommt von oben"));

    testrun(comment_is(oggopus, "Simmsalabim", 0));

    testrun(comment_is(oggopus, "delta", "Soll"));

    testrun(0 != chunk.start);

    testrun(chunk.length == data->length);
    testrun(0 == strcmp((char const *)chunk.start, (char const *)data->start));

    chunk.start = ov_free(chunk.start);

    data = ov_buffer_free(data);
    testrun(0 == data);

    oggopus = ov_format_close(oggopus);
    testrun(0 == oggopus);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int tear_down() {

    ov_format_registry_clear(0);
    ov_registered_cache_free_all();
    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

OV_TEST_RUN("ov_format_ogg_opus",
            initialize,
            test_id_header,
            test_comment_header,
            test_ov_format_ogg_opus_create,
            test_impl_write_chunk,
            test_impl_next_chunk,
            tear_down);

/*----------------------------------------------------------------------------*/
