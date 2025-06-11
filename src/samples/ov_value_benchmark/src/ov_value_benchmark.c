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
        @author         Michael Beer

        ------------------------------------------------------------------------
*/

#include <math.h>
#include <ov_base/ov_json_value.h>
#include <ov_base/ov_registered_cache.h>
#include <ov_base/ov_utils.h>
#include <ov_base/ov_value.h>
#include <ov_base/ov_value_parse.h>
#include <sys/resource.h>
#include <sys/time.h>

size_t NUM_RUNS = 10000;

size_t NUM_PARSE_RUNS = 10000;

char const *parse_str =
    "[\"aesir\", 4,\n"
    "[\"odin\", \"thor\", \"loki\", \"heimdall\", "
    "\"...\"],\n"
    "\"vanir\", 3,\n"
    "[\"freyr\", \"freya\", \"njoerdr\"],\n"
    "\"swartalfar\", 0, null]";

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

#define print_time(desc)                                                       \
    printf("\n%s   %li.%li\n",                                                 \
           desc,                                                               \
           after.time.secs - before.time_secs,                                 \
           after.time.usecs - before.time_usecs)

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

    printf("\n%s   %li.%li\n",
           "Time passed",
           after.time.tv_sec - before.time.tv_sec,
           (long)after.time.tv_usec - before.time.tv_usec);

    printf("\n");

    print_field("unshared memory size", idrss);
    print_field("unshared memory size", idrss);
}

/*****************************************************************************
                                    ov_value
 ****************************************************************************/

static void ov_value_sample() {

    ov_value *vnull = 0;
    ov_value *vtrue = 0;
    ov_value *vfalse = 0;
    ov_value *vnumber = 0;
    ov_value *vstring = 0;
    ov_value *vlist = 0;

    for (size_t i = 0; i < NUM_RUNS; ++i) {

        vnull = ov_value_null();
        vtrue = ov_value_true();
        vfalse = ov_value_false();
        vnumber = ov_value_number(i);
        vstring = ov_value_string("Hrafn");

        vlist = ov_value_list(ov_value_null(),
                              ov_value_string("Cthulhu naftagn"),
                              ov_value_true());

        vlist = ov_value_list(ov_value_true(),
                              ov_value_number(143),
                              vlist,
                              ov_value_string("Naglfar"));

        vnull = ov_value_free(vnull);
        vtrue = ov_value_free(vtrue);
        vfalse = ov_value_free(vfalse);
        vnumber = ov_value_free(vnumber);
        vstring = ov_value_free(vstring);
        vlist = ov_value_free(vlist);
    }
}

static void ov_value_parse_sample() {

    ov_buffer buf = {
        .start = (uint8_t *)parse_str,
        .length = strlen(parse_str) + 1,

    };

    for (size_t i = 0; i < NUM_PARSE_RUNS; ++i) {

        ov_value *v = ov_value_parse(&buf, 0);
        OV_ASSERT(0 != v);

        v = ov_value_free(v);
    }
}

/*****************************************************************************
                                 ov_json_value
 ****************************************************************************/

static ov_json_value *internal_get_json_array(ov_json_value *values[]) {

    ov_json_value *array = ov_json_array();

    while (0 != *values) {

        ov_json_array_push(array, *values);
        ++values;
    };

    return array;
}

#define get_json_array(...)                                                    \
    internal_get_json_array((ov_json_value *[]){__VA_ARGS__, 0})

/*----------------------------------------------------------------------------*/

static void ov_json_value_sample() {

    ov_json_value *vnull = 0;
    ov_json_value *vtrue = 0;
    ov_json_value *vfalse = 0;
    ov_json_value *vnumber = 0;
    ov_json_value *vstring = 0;
    ov_json_value *vlist = 0;

    for (size_t i = 0; i < NUM_RUNS; ++i) {

        vnull = ov_json_null();

        vtrue = ov_json_true();

        vfalse = ov_json_false();

        vnumber = ov_json_number(i);

        vstring = ov_json_string("Hrafn");

        vlist = get_json_array(
            ov_json_null(), ov_json_string("Cthulhu naftagn"), ov_json_true());

        vlist = get_json_array(ov_json_true(),
                               ov_json_number(143),
                               vlist,
                               ov_json_string("Naglfar"));

        vnull = ov_json_value_free(vnull);

        vtrue = ov_json_value_free(vtrue);

        vfalse = ov_json_value_free(vfalse);

        vnumber = ov_json_value_free(vnumber);

        vstring = ov_json_value_free(vstring);

        vlist = ov_json_value_free(vlist);
    }
}

/*----------------------------------------------------------------------------*/

static void ov_json_value_from_string_sample() {

    ov_buffer buf = {

        .start = (uint8_t *)parse_str,
        .length = strlen(parse_str) + 1,

    };

    for (size_t i = 0; i < NUM_PARSE_RUNS; ++i) {

        ov_json_value *v =
            ov_json_value_from_string((char *)buf.start, buf.length);
        OV_ASSERT(0 != v);

        v = ov_json_value_free(v);
    }
}

/*****************************************************************************
                                      MAIN
 ****************************************************************************/

size_t get_NUM_RUNS(int argc, char **argv) {

    if (2 > argc) return NUM_RUNS;

    char *endptr = 0;
    long NUM_RUNS_l = strtol(argv[1], &endptr, 0);

    if ((0 == endptr) || (0 != *endptr)) {

        fprintf(stderr,
                "Require number of runs to be a positive number, got %s\n",
                argv[1]);
        exit(EXIT_FAILURE);
    }

    if (0 > NUM_RUNS_l) {
        fprintf(stderr, "number of runs must be positive\n");
        exit(EXIT_FAILURE);
    }

    return (size_t)NUM_RUNS_l;
}

/*----------------------------------------------------------------------------*/

static bool enable_caching(int argc, char **argv) {

    UNUSED(argc);
    UNUSED(argv);

    return true;
}

/*----------------------------------------------------------------------------*/

int main(int argc, char **argv) {

    NUM_RUNS = get_NUM_RUNS(argc, argv);

    if (enable_caching(argc, argv)) {

        ov_value_enable_caching(50, 50, 50, 50);
    }

    printf("\n\n\nov_value:\n\n\n");

    struct usage usage = get_usage();

    ov_value_sample();
    print_usage(usage);

    printf("\n\n\nov_json_value:\n\n\n");

    usage = get_usage();

    ov_json_value_sample();
    print_usage(usage);

    printf("\n\n\nov_value_parse:\n\n\n");

    usage = get_usage();
    ov_value_parse_sample();
    print_usage(usage);

    printf("\n\n\nov_json_value_from_string:\n\n\n");

    usage = get_usage();
    ov_json_value_from_string_sample();
    print_usage(usage);

    ov_registered_cache_free_all();

    return EXIT_SUCCESS;
}
