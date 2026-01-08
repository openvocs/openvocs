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
#include <ov_base/ov_registered_cache.h>
#include <ov_base/ov_utils.h>

#include <ov_base/ov_value.h>
#include <ov_base/ov_value_parse.h>

/*****************************************************************************

                         Sample code for application of

                               ov_value_for_each

 ****************************************************************************/

struct deep_count_arg {

    FILE *stream;
    size_t counter;
};

bool deep_count(char const *key, ov_value const *value, void *userdata) {

    // For lists, `key` is always zero - anyways, we don't need it

    OV_ASSERT(0 != value);

    struct deep_count_arg *arg = userdata;

    OV_ASSERT(0 != arg);

    fprintf(arg->stream, "   %5zu : ", arg->counter);

    if (0 != key) {

        fprintf(arg->stream, " '%s' => ", key);
    }
    ov_value_dump(arg->stream, value);
    fprintf(arg->stream, "\n");

    ++arg->counter;

    if (0 != ov_value_list_get((ov_value *)value, 0)) {

        // We are a non-empty list
        // -> Progress recursively to traverse deeply
        return ov_value_for_each(value, deep_count, arg);
    }

    return true;
}

/*----------------------------------------------------------------------------*/

int main(int argc, char **argv) {

    UNUSED(argc);
    UNUSED(argv);

    size_t NUMBERS_TO_CACHE = 20;
    size_t STRINGS_TO_CACHE = 30;
    size_t LISTS_TO_CACHE = 10;
    size_t OBJECTS_TO_CACHE = 20;

    // For the following, include
    //
    //     "ov_base/ov_value.h"

    /* Enable caching for value data structures */
    ov_value_enable_caching(NUMBERS_TO_CACHE, STRINGS_TO_CACHE, LISTS_TO_CACHE,
                            OBJECTS_TO_CACHE);

    /* Build simple Atoms */

    ov_value *t = ov_value_true();
    ov_value *f = ov_value_false();
    ov_value *n = ov_value_null();

    /* Build numbers */

    ov_value *the_answer = ov_value_number(42);
    ov_value *pi = ov_value_number(3.1415);

    /* Build strings */

    ov_value *ass = ov_value_string("vidarr");

    /* Build lists */

    ov_value *empty_list = ov_value_list(0);

    ov_value *aesir =
        ov_value_list(ov_value_string("odin"), ov_value_string("thor"),
                      ov_value_string("loki"), ov_value_string("heimdall"),
                      ov_value_string("..."));

    /* lists might contain any other values */

    ov_value *mezcla = ov_value_list(
        ov_value_null(),
        ov_value_list(ov_value_string("Cthulhu"), ov_value_string("naftagn")),
        ov_value_number(1337),
        ov_value_list(ov_value_list(0), ov_value_string("Aesir"), aesir));

    /* Values are incorporated into compounds, not copied, thus our `aesir`
     * is now part of the `mezcla` and should not be modified independently any
     * more
     */

    aesir = 0;

    /* values might be dumped to a stream */

    fprintf(stdout, "Our mezcla: ");
    ov_value_dump(stdout, mezcla);
    fprintf(stdout, "\n");

    /* Or turned into a string */

    char *t_str = ov_value_to_string(t);

    /* Check whether value is a particular atom */
    printf("%s is true: %s\n", t_str, ov_value_is_true(t) ? "true" : "false");
    printf("%s is false: %s\n", t_str, ov_value_is_false(t) ? "true" : "false");
    printf("%s is null: %s\n", t_str, ov_value_is_null(t) ? "true" : "false");

    free(t_str);
    t_str = 0;

    /* Atoms are singletons */

    OV_ASSERT(t == ov_value_true());
    OV_ASSERT(f == ov_value_false());
    OV_ASSERT(n == ov_value_null());

    /* List entries might be accessed by index */

    ov_value *first = ov_value_list_get(mezcla, 0);

    printf("first entry of ");
    ov_value_dump(stdout, mezcla);
    printf("is null: %s\n", ov_value_is_null(first) ? "true" : "false");

    first = 0;

    /* Or entries set via their index */

    ov_value_list_set(mezcla, 0, ov_value_true());

    /* Or appended at the end */

    ov_value_list_push(mezcla, ov_value_string("This is the end"));

    printf("Mezcla now looks like: ");
    ov_value_dump(stdout, mezcla);
    printf("\n");

    /* Usually, all functions return a value.
     * If they do not create one, they return true in case of success or
     * false in case of error.
     *
     * All functions are robust against 0 pointers:
     */

    if (!ov_value_list_push(0, t)) {
        printf("Pushing onto a 0 pointer failed");
    }

    printf("objects are created using `ov_value_object()`\n");
    ov_value *object = ov_value_object();

    printf("and entries set using `ov_value_object_set`\n");
    ov_value_object_set(object, "our_mezcla", mezcla);

    ov_value_dump(stdout, object);

    /* Again, mezcla is incorporated into our new object */
    mezcla = 0;

    printf("ov_value_object_set returns an old entry if there: ");

    mezcla = ov_value_object_set(object, "our_mezcla", ov_value_null());
    ov_value_dump(stdout, object);

    printf("and entries retrieved using ov_values_object_get()\n");
    ov_value_dump(stdout, ov_value_object_get(object, "our_mezcla"));

    /* Be sure to free any allocated values once you are done */

    object = ov_value_free(object);

    t = ov_value_free(t);
    f = ov_value_free(f);
    n = ov_value_free(n);
    the_answer = ov_value_free(the_answer);
    pi = ov_value_free(pi);
    ass = ov_value_free(ass);
    empty_list = ov_value_free(empty_list);

    /* Pro tipp:
     * ov_value_free - as all functions - is robust against 0 pointers.
     *
     * Don't bother writing something like
     *
     * if(0 != aesir) aesir = ov_value_free(aesir);
     */
    OV_ASSERT(0 == aesir);
    aesir = ov_value_free(aesir);
    OV_ASSERT(0 == aesir);

    mezcla = ov_value_free(mezcla);
    OV_ASSERT(0 == mezcla);

    /*
     * For parsing, include
     * ov_base/ov_value_parse.h
     */

    /* Parse from a string / buffer */

    char const *gods_str = "[\"aesir\", 4,\n"
                           "[\"odin\", \"thor\", \"loki\", \"heimdall\", "
                           "\"...\"],\n"
                           "\"vanir\", 3,\n"
                           "[\"freyr\", \"freya\", \"njoerdr\"],\n"
                           "\"swartalfar\", 0, null]";

    ov_buffer to_parse = {
        .start = (uint8_t *)gods_str,
        .length = strlen(gods_str),
    };

    ov_value *gods = ov_value_parse(&to_parse, 0);

    printf("For '%s'\n\nWe got:\n\n", gods_str);
    ov_value_dump(stdout, gods);
    printf("\n");

    gods = ov_value_free(gods);
    OV_ASSERT(0 == gods);

    /* Parse with trailing characters */

    char const *gods_with_trailing_chars =
        "[\"aesir\", 4,\n"
        "[\"odin\", \"thor\", \"loki\", \"heimdall\", \"...\"],\n"
        "\"vanir\", 3,\n"
        "[\"freyr\", \"freya\", \"njoerdr\"],\n"
        "\"swartalfar\", 0, null] We [got] LOST";

    to_parse = (ov_buffer){
        .start = (uint8_t *)gods_with_trailing_chars,
        .length = strlen(gods_with_trailing_chars),
    };

    char const *remainder = 0;

    gods = ov_value_parse(&to_parse, &remainder);

    printf("For '%s'\n\nWe got:\n\n", gods_with_trailing_chars);
    ov_value_dump(stdout, gods);
    printf("\n\nAnd trailing chars: '%s'\n\n", remainder);

    /* Parse objects */

    char const *object_string = "   \n"
                                " {   \"Aesir\":[\"Heimdall\",\"Loki\"  ,     "
                                "\"Vidarr\"] , "
                                "     \"Wanir\"   :   [ \"Njordr\", \"Frey\"   "
                                ",\"Freya\"],  "
                                "     \"reverted\" :{ \n"
                                "  \"Heimdall\" : \"Ass\"  , "
                                " \"Loki\": \"Ass\",  \n"
                                "\"Vidarr\" : \"Ass\","
                                "\"Njordr\" : \"Wanr\","
                                "\"Frey\" : \"Wanr\","
                                "\"Freya\" : \"Wanr\"  }} Non-conclusive List "
                                "of the dwellers of Asgard";

    to_parse = (ov_buffer){
        .start = (uint8_t *)object_string,
        .length = strlen(object_string) + 1,
    };

    object = ov_value_parse(&to_parse, &remainder);

    printf("%s:\n", remainder);
    ov_value_dump(stdout, object);

    // " and \ must be escaped in strings:

    char const *escaped_string = "A '\"' must be escaped like '\\\"', "
                                 "a '\\' must be escaped like '\\\\'";

    to_parse = (ov_buffer){
        .start = (uint8_t *)escaped_string,
        .length = strlen(escaped_string),
    };

    ov_value *string = ov_value_parse(&to_parse, 0);

    ov_value_dump(stdout, string);

    string = ov_value_free(string);

    // You can visit all immediate entries in an ov_value
    // using ov_value_for_each()
    //
    // If you want to visit *all* entries, use recursion
    // like in here (see `deep_count`) .
    struct deep_count_arg arg = {

        .stream = stderr,
        .counter = 0,

    };

    printf("Our list looks like:\n");

    ov_value_for_each(gods, deep_count, &arg);

    printf("Summarizing up to %zu entries\n", arg.counter);

    printf("for_each also works on our object:\n");

    arg.counter = 0;

    ov_value_for_each(object, deep_count, &arg);

    printf("Summarizing up to %zu entries\n", arg.counter);

    // You can do all kinds of stuff with for_each, implement own
    // counters, filters, serializers...

    gods = ov_value_free(gods);
    OV_ASSERT(0 == gods);

    object = ov_value_free(object);

    /* Don't forget to free all registered caches */
    ov_registered_cache_free_all();

    return EXIT_SUCCESS;
}
