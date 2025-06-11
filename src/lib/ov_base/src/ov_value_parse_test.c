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
#define OV_VALUE_TEST

#include "ov_value_parse.c"
#include <math.h>
#include <ov_test/ov_test.h>
#include <stdarg.h>

/*----------------------------------------------------------------------------*/

static bool check_parse(char const *str,
                        ov_value *ref,
                        char const *ref_remainder) {

    bool success_p = false;

    ov_buffer buf = {

        .start = (uint8_t *)str,
        .length = strlen(str) + 1,

    };

    char const *remainder = 0;

    ov_value *v = ov_value_parse(&buf, &remainder);

    char *parsed_str = ov_value_to_string(v);
    fprintf(stderr, "'%s' parsed to '%s'\n", str, parsed_str);
    free(parsed_str);

    if (!ov_value_match(v, ref)) {

        testrun_log_error("Parsed object does not match reference");
        goto error;
    }

    if ((0 == ref_remainder) && (0 != remainder)) {

        testrun_log_error(
            "Remainders don't match: Expected null pointer, got "
            "'%s'",
            remainder);

        goto error;
    }

    if ((0 != ref_remainder) && (0 == remainder)) {

        testrun_log_error(
            "Remainders don't match: Expected '%s', got null "
            "pointer",
            ref_remainder);
        goto error;
    }

    if ((0 != ref_remainder) && (0 != remainder) &&
        (0 != strcmp(ref_remainder, remainder))) {

        testrun_log_error("Remainders don't match- expected '%s' - got '%s'\n",
                          ref_remainder,
                          remainder);

        goto error;
    }

    success_p = true;

error:

    ref = ov_value_free(ref);
    v = ov_value_free(v);
    return success_p;
}

/*----------------------------------------------------------------------------*/

static bool check_dump_parse(ov_value *reference) {

    bool matches = false;

    ov_buffer buf = {

        .start = (uint8_t *)ov_value_to_string(reference),

    };

    OV_ASSERT(0 != buf.start);

    fprintf(stderr, "Checking '%s'\n", buf.start);

    buf.length = 1 + strlen((char *)buf.start);

    char const *remainder = 0;

    ov_value *value = ov_value_parse(&buf, &remainder);

    free(buf.start);

    buf = (ov_buffer){0};

    if (0 == value) goto error;

    matches = ov_value_match(value, reference);

error:

    OV_ASSERT(0 == ov_value_free(reference));
    OV_ASSERT(0 == ov_value_free(value));

    return matches;
}

/*----------------------------------------------------------------------------*/

static int test_ov_value_parse() {

    testrun(check_parse("null", ov_value_null(), ""));
    testrun(check_parse("true", ov_value_true(), ""));
    testrun(check_parse("false", ov_value_false(), ""));

    testrun(check_parse("    null", ov_value_null(), ""));
    testrun(check_parse("    null bgh", ov_value_null(), " bgh"));

    testrun(check_parse("\"vali\"", ov_value_string("vali"), ""));
    testrun(check_parse("    \"vali\"", ov_value_string("vali"), ""));
    testrun(check_parse(
        "    \"vali\" \"Baldr\"", ov_value_string("vali"), " \"Baldr\""));

    testrun(check_parse("    \"vali \\\"the avenger\\\"\" \"Baldr\"",
                        ov_value_string("vali \"the avenger\""),
                        " \"Baldr\""));

    /* This test from ov_value_parse_test is NOT a valid JSON string !!!
     * @see ov_value_json_test.c line 143
     * MUST be 7 slashes instead of 5 to generate this, otherwise you miss the
     * escape of the slash within the quoted string, which is invalid JSON! */

    /* MIB:
     * I know it's confusing.
     *
     * We want:
     *                   vali "the avenger \
     *
     *
     * Escaped for JSON:
     *    (Compare https://www.freeformatter.com/json-escape.html )
     *
     *
     * 1. Escape each backslash:
     *
     *                   vali "the avenger \
     *                                     |
     *                                     v
     *                   vali "the avenger \\
     *
     * 2. Escape each quote:
     *
     *                   vali "the avenger \\
     *                        /
     *                       v
     *                  vali \"the avenger \\
     *
     * 3. Include in quotes to make it a JSON string:
     *
     *                  vali \"the avenger \\
     *                 "vali \"the avenger \\"
     *
     *
     * Escaping for C:
     *
     * Same procedure all over:
     *
     * 1. Escape every backslash with an additional backslash:
     *                 "vali \"the avenger \\"
     *                       /              |\
     *                       v              v v
     *                 "vali \\"the avenger \\\\"
     *
     * 2. Escape every quote with an backslash and add quotes at front & end:
     *
     *                 "vali \\"the avenger \\\\"
     *                 /       |                \
     *                v        v                 v
     *               "\"vali \\\"the avenger \\\\\""
     *
     *
     * Now compare to string below:
     *
     *               "\"vali \\\"the avenger \\\\\""
     *           "    \"vali \\\"the avenger \\\\\" Baldr",
     *
     * What's the problem?
     *
     * Unfortunately, if ov_json_value_test.c expects seven slashes to yield
     * vali "the avenger \
     *
     * ov_value_json_test.c is wrong IMHO.
     *
     */

    testrun(check_parse("    \"vali \\\"the avenger \\\\\" Baldr",
                        ov_value_string("vali \"the avenger \\"),
                        " Baldr"));

    testrun(check_parse(
        " \t\r\n\"all whitespaces\"", ov_value_string("all whitespaces"), ""));

    // Unterminated JSON defs

    // 1. string

    char const *unterminated_string = "    \"vali \\\"the avenger \\\\ Baldr";

    ov_buffer buf = {
        .start = (uint8_t *)unterminated_string,
        .length = strlen(unterminated_string),
    };

    testrun(0 == ov_value_parse(&buf, 0));

    buf.length += 10;
    buf.start = calloc(1, buf.length);
    strcpy((char *)buf.start, unterminated_string);

    char const *remainder = 0;

    testrun(0 == ov_value_parse(&buf, &remainder));

    testrun(remainder == (char *)buf.start);

    remainder = 0;

    free(buf.start);
    buf.start = 0;

    // 2. List

    unterminated_string = "{\"key1\" : [1,";

    buf.start = (uint8_t *)unterminated_string;
    buf.length = strlen(unterminated_string);

    testrun(0 == ov_value_parse(&buf, &remainder));

    testrun(remainder == (char *)buf.start);
    remainder = 0;

    // 3. Object

    unterminated_string = " { ";

    buf.start = (uint8_t *)unterminated_string;
    buf.length = strlen(unterminated_string);

    testrun(0 == ov_value_parse(&buf, &remainder));

    testrun(remainder == (char *)buf.start);
    remainder = 0;

    unterminated_string = " { \"";

    buf.start = (uint8_t *)unterminated_string;
    buf.length = strlen(unterminated_string);

    testrun(0 == ov_value_parse(&buf, &remainder));

    testrun(remainder == (char *)buf.start);
    remainder = 0;

    unterminated_string = " { \"key1";

    buf.start = (uint8_t *)unterminated_string;
    buf.length = strlen(unterminated_string);

    testrun(0 == ov_value_parse(&buf, &remainder));

    testrun(remainder == (char *)buf.start);
    remainder = 0;

    unterminated_string = " { \"key1\"";

    buf.start = (uint8_t *)unterminated_string;
    buf.length = strlen(unterminated_string);

    testrun(0 == ov_value_parse(&buf, &remainder));

    testrun(remainder == (char *)buf.start);
    remainder = 0;

    unterminated_string = "{\"key1\" :";

    buf.start = (uint8_t *)unterminated_string;
    buf.length = strlen(unterminated_string);

    testrun(0 == ov_value_parse(&buf, &remainder));

    testrun(remainder == (char *)buf.start);
    remainder = 0;

    unterminated_string = "{\"key1\" :1";

    buf.start = (uint8_t *)unterminated_string;
    buf.length = strlen(unterminated_string);

    testrun(0 == ov_value_parse(&buf, &remainder));

    testrun(remainder == (char *)buf.start);
    remainder = 0;

    unterminated_string = "   { \"key1\" : 12";

    buf.start = (uint8_t *)unterminated_string;
    buf.length = strlen(unterminated_string);

    testrun(0 == ov_value_parse(&buf, &remainder));
    testrun(remainder == (char *)buf.start);

    remainder = 0;

    /* Object string complete */
    char const *terminated_string = "   { \"key1\" : 12}";

    buf.start = (uint8_t *)terminated_string;
    buf.length = strlen(terminated_string);

    ov_value *value = ov_value_parse(&buf, &remainder);
    testrun(ov_value_is_object(value));
    ov_value const *entry = ov_value_object_get(value, "key1");
    testrun(ov_value_is_number(entry));
    double number = ov_value_get_number(entry);
    testrun(0.00001 > fabs(number - 12));

    value = ov_value_free(value);
    testrun(0 == value);

    testrun(remainder == (char *)(buf.start + buf.length));
    remainder = 0;

    /* comma, but nothing follows */

    unterminated_string = "{\"key1\" :1,";

    buf.start = (uint8_t *)unterminated_string;
    buf.length = strlen(unterminated_string);

    testrun(0 == ov_value_parse(&buf, &remainder));

    testrun(remainder == (char *)buf.start);
    remainder = 0;

    // Numbers

    testrun(check_parse("42", ov_value_number(42), ""));
    testrun(check_parse("+42", ov_value_number(42), ""));
    testrun(check_parse("-42", ov_value_number(-42), ""));

    testrun(check_parse("13.37", ov_value_number(13.37), ""));
    testrun(check_parse("+13.37", ov_value_number(13.37), ""));
    testrun(check_parse("-13.37", ov_value_number(-13.37), ""));
    testrun(check_parse("\t    \t-13.37", ov_value_number(-13.37), ""));
    testrun(check_parse(
        " \t-13.37\t\"vali\"", ov_value_number(-13.37), "\t\"vali\""));

    // Lists

    testrun(check_parse(" []", ov_value_list(0), ""));
    testrun(check_parse("\t [true]\n", ov_value_list(ov_value_true()), "\n"));

    testrun(check_parse("\t [1916, null]\n",
                        ov_value_list(ov_value_number(1916), ov_value_null()),
                        "\n"));

    testrun(
        check_parse(" \n\n\n   [\"\\\"Naftagn\\\"\", \"Cthulhu\", 12.112] "
                    ", \"",
                    ov_value_list(ov_value_string("\"Naftagn\""),
                                  ov_value_string("Cthulhu"),
                                  ov_value_number(12.112)),
                    " , \""));

    testrun(check_parse(
        " \n\n\n   [\"\\\"Naftagn\\\"\", [\"Cthulhu\", "
        "12.112]] , \"",
        ov_value_list(
            ov_value_string("\"Naftagn\""),
            ov_value_list(ov_value_string("Cthulhu"), ov_value_number(12.112))),
        " , \""));

    testrun(check_parse(
        " \n\n\n   [[\"Naftagn\"], [\"Cthulhu\", "
        "[true, null], 12.112]] , \"",
        ov_value_list(
            ov_value_list(ov_value_string("Naftagn")),
            ov_value_list(ov_value_string("Cthulhu"),
                          ov_value_list(ov_value_true(), ov_value_null()),
                          ov_value_number(12.112))),
        " , \""));

    testrun(check_parse(
        "{\"key1\" : [1, 2]}blablablubb",
        OBJECT(0,
               PAIR("key1",
                    ov_value_list(ov_value_number(1), ov_value_number(2)))),
        "blablablubb"));

    // Some real errors

    char const *faulty_string = "}";

    // Have remainder point to anything but 0 */
    remainder = (char *)faulty_string;

    buf.start = (uint8_t *)faulty_string;
    buf.length = strlen(faulty_string);

    testrun(0 == ov_value_parse(&buf, &remainder));

    testrun(remainder == 0);

    faulty_string = "xRRRRRRRollg[1,2]";

    buf.start = (uint8_t *)faulty_string;
    buf.length = strlen(faulty_string);
    remainder = (char *)faulty_string;

    testrun(0 == ov_value_parse(&buf, &remainder));

    testrun(remainder == 0);
    remainder = 0;

    // Check error without given remainder
    //
    buf.start = (uint8_t *)faulty_string;
    buf.length = strlen(faulty_string);
    remainder = (char *)faulty_string;

    testrun(0 == ov_value_parse(&buf, 0));

    /*************************************************************************
           Dump and subsequent parsing should yield matching objects
     ************************************************************************/

    testrun(check_dump_parse(ov_value_null()));
    testrun(check_dump_parse(ov_value_true()));
    testrun(check_dump_parse(ov_value_false()));
    testrun(check_dump_parse(ov_value_number(13)));
    testrun(check_dump_parse(ov_value_string("Trece")));
    testrun(check_dump_parse(
        ov_value_list(ov_value_true(),
                      ov_value_string("Catorce"),
                      ov_value_list(ov_value_number(1), ov_value_number(2)),
                      ov_value_null())));

    testrun(check_dump_parse(ov_value_string("vali \\ \"the avenger\"")));

    testrun(check_dump_parse(ov_value_string("vali \\ \"the avenger\"")));

    testrun(check_dump_parse(OBJECT(
        0,
        PAIR("true", ov_value_true()),
        PAIR("Catorce", ov_value_string("Catorce")),
        PAIR("a_list", ov_value_list(ov_value_number(1), ov_value_number(2))),
        PAIR("null", ov_value_null()))));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static void *received_arg = 0;

static void dummy_consumer(ov_value *value, void *anything) {

    ov_value_free(value);

    received_arg = anything;
}

/*----------------------------------------------------------------------------*/

static ov_value *values_expected[255] = {0};

static bool values_expected_found = false;
static size_t values_mismatch_index = 0;

static void value_checker(ov_value *value, void *v_index) {

    OV_ASSERT(0 != v_index);

    size_t i = *(size_t *)v_index;

    OV_ASSERT(i < sizeof(values_expected) / sizeof(values_expected[0]));

    if (!ov_value_match(values_expected[i], value)) {

        values_expected_found = false;
        values_mismatch_index = i;
    }

    *(size_t *)v_index += 1;

    ov_value_free(value);
}

/*----------------------------------------------------------------------------*/

static bool check_parse_stream(char const *in,
                               char const *expected_remainder,
                               ...) {

    size_t number_of_expected_values = 0;

    va_list va;

    va_start(va, expected_remainder);

    ov_value *value = va_arg(va, ov_value *);

    while (0 != value) {

        values_expected[number_of_expected_values] = value;
        ++number_of_expected_values;

        value = va_arg(va, ov_value *);
    };

    value = 0;

    va_end(va);

    values_expected_found = true;

    char const *remainder = 0;
    size_t num_found_values = 0;

    ov_value_parse_stream(
        in, 1 + strlen(in), value_checker, &num_found_values, &remainder);

    for (size_t i = 0; i < number_of_expected_values; ++i) {
        values_expected[i] = ov_value_free(values_expected[i]);
    }

    if (0 == remainder) return false;

    if (num_found_values != number_of_expected_values) goto error;

    return values_expected_found;

error:

    return false;
}

/*----------------------------------------------------------------------------*/

static int test_ov_value_parse_stream() {

    testrun(!ov_value_parse_stream(0, 0, 0, 0, 0));
    testrun(!ov_value_parse_stream("1", 0, 0, 0, 0));
    testrun(!ov_value_parse_stream(0, 2, 0, 0, 0));
    testrun(!ov_value_parse_stream("1", 2, 0, 0, 0));
    testrun(!ov_value_parse_stream("1", 0, dummy_consumer, 0, 0));
    testrun(!ov_value_parse_stream(0, 2, dummy_consumer, 0, 0));
    testrun(ov_value_parse_stream("1", 2, dummy_consumer, 0, 0));
    testrun(0 == received_arg);

    testrun(ov_value_parse_stream("1", 2, dummy_consumer, &received_arg, 0));

    testrun(&received_arg == received_arg);

    char const *remainder = 0;

    testrun(ov_value_parse_stream("1[", 2, dummy_consumer, 0, &remainder));
    testrun(0 == strcmp("[", remainder));

    testrun(0 == received_arg);

    remainder = 0;

    // Whitespaces only
    testrun(ov_value_parse_stream("  ", 2, dummy_consumer, 0, &remainder));
    testrun(0 == strcmp("  ", remainder));
    testrun(0 == received_arg);
    remainder = 0;

    testrun(
        ov_value_parse_stream("1[", 2, dummy_consumer, &remainder, &remainder));
    testrun(0 == strcmp("[", remainder));

    testrun(&remainder == received_arg);

    testrun(check_parse_stream("\"aA", "\"aA", 0));
    testrun(check_parse_stream(
        "1 \"aA\"", "", ov_value_number(1), ov_value_string("aA"), 0));

    // With leading whitespaces
    testrun(check_parse_stream("   \"aA", "   \"aA", 0));
    testrun(check_parse_stream(
        "    1 \"aA\"", "", ov_value_number(1), ov_value_string("aA"), 0));

    // One 'continuous' run
    values_expected[0] = ov_value_number(1);
    values_expected[1] = ov_value_string("Vala");
    values_expected[2] = 0;

    size_t index = 0;
    values_expected_found = true;

    char const *incomplete_string = "1\"Vala\"[1,";

    testrun(ov_value_parse_stream(incomplete_string,
                                  1 + strlen(incomplete_string),
                                  value_checker,
                                  &index,
                                  &remainder));

    testrun(2 == index);
    testrun(values_expected_found);

    values_expected[0] = ov_value_free(values_expected[0]);
    values_expected[1] = ov_value_free(values_expected[1]);

    testrun(0 == values_expected[0]);
    testrun(0 == values_expected[1]);

    // Now concatenate remainder of last run with 'new' data

    ov_buffer *buffer = ov_buffer_create(255);

    strcpy((char *)buffer->start, remainder);
    strcat((char *)buffer->start, "2, 3");

    buffer->length = strlen((char *)buffer->start) + 1;

    // No new values
    values_expected[0] = 0;

    index = 0;
    values_expected_found = true;

    testrun(ov_value_parse_stream((char *)buffer->start,
                                  buffer->length,
                                  value_checker,
                                  &index,
                                  &remainder));

    testrun(values_expected_found);
    testrun(0 == index);

    testrun(0 == strcmp((char *)buffer->start, remainder));

    // still unfinished

    strcat((char *)buffer->start, "    , ");

    buffer->length = strlen((char *)buffer->start) + 1;
    buffer->length = strlen((char *)buffer->start) + 1;

    index = 0;
    values_expected_found = true;

    testrun(ov_value_parse_stream((char *)buffer->start,
                                  buffer->length,
                                  value_checker,
                                  &index,
                                  &remainder));

    testrun(values_expected_found);
    testrun(0 == index);

    testrun(0 == strcmp((char *)buffer->start, remainder));

    // ... still unfinished

    strcat((char *)buffer->start, "    4 ");

    buffer->length = strlen((char *)buffer->start) + 1;
    buffer->length = strlen((char *)buffer->start) + 1;

    index = 0;
    values_expected_found = true;

    testrun(ov_value_parse_stream((char *)buffer->start,
                                  buffer->length,
                                  value_checker,
                                  &index,
                                  &remainder));

    testrun(values_expected_found);
    testrun(0 == index);

    testrun(0 == strcmp((char *)buffer->start, remainder));

    // List finished

    values_expected[0] = ov_value_list(ov_value_number(1),
                                       ov_value_number(2),
                                       ov_value_number(3),
                                       ov_value_number(4));

    values_expected[1] = 0;

    strcat((char *)buffer->start, "] \"");
    buffer->length = strlen((char *)buffer->start) + 1;

    index = 0;

    testrun(ov_value_parse_stream((char *)buffer->start,
                                  buffer->length,
                                  value_checker,
                                  &index,
                                  &remainder));

    testrun(values_expected_found);
    testrun(1 == index);

    testrun(0 == strcmp(" \"", remainder));

    values_expected[0] = ov_value_free(values_expected[0]);

    buffer = ov_buffer_free(buffer);
    testrun(0 == buffer);

    // Object incomplete

    char const *object_str = "  { \"key1\" : 12 } ";
    size_t len = strlen(object_str);

    buffer = ov_buffer_create(len);
    testrun(0 != buffer);

    strcpy((char *)buffer->start, object_str);
    buffer->length = len;

    values_expected[0] = ov_value_parse(buffer, 0);
    testrun(0 != values_expected[0]);

    buffer->length = 4;
    values_expected_found = true;
    index = 0;

    testrun(ov_value_parse_stream((char *)buffer->start,
                                  buffer->length,
                                  value_checker,
                                  &index,
                                  &remainder));

    testrun(values_expected_found);
    testrun(remainder == (char *)buffer->start);

    buffer->length = 6;

    testrun(ov_value_parse_stream((char *)buffer->start,
                                  buffer->length,
                                  value_checker,
                                  &index,
                                  &remainder));

    testrun(values_expected_found);
    testrun(remainder == (char *)buffer->start);

    buffer->length = 10;

    testrun(ov_value_parse_stream((char *)buffer->start,
                                  buffer->length,
                                  value_checker,
                                  &index,
                                  &remainder));

    testrun(values_expected_found);
    testrun(remainder == (char *)buffer->start);

    buffer->length = 12;

    testrun(ov_value_parse_stream((char *)buffer->start,
                                  buffer->length,
                                  value_checker,
                                  &index,
                                  &remainder));

    testrun(values_expected_found);
    testrun(remainder == (char *)buffer->start);

    buffer->length = 15;

    testrun(ov_value_parse_stream((char *)buffer->start,
                                  buffer->length,
                                  value_checker,
                                  &index,
                                  &remainder));

    testrun(values_expected_found);
    testrun(remainder == (char *)buffer->start);

    buffer->length = 17;

    testrun(ov_value_parse_stream((char *)buffer->start,
                                  buffer->length,
                                  value_checker,
                                  &index,
                                  &remainder));

    testrun(values_expected_found);
    testrun(remainder == (char *)(buffer->start + 17));

    buffer = ov_buffer_free(buffer);
    values_expected[0] = ov_value_free(values_expected[0]);

    testrun(0 == buffer);
    testrun(0 == values_expected[0]);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

OV_TEST_RUN("ov_value", test_ov_value_parse, test_ov_value_parse_stream);
