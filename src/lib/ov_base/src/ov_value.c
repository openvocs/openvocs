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
#include "../include/ov_value.h"
#include "../include/ov_buffer.h"
#include "../include/ov_constants.h"
#include "../include/ov_hashtable.h"
#include "../include/ov_utils.h"
#include "../include/ov_vector_list.h"
#include <../include/ov_registered_cache.h>
#include <float.h>
#include <math.h>
#include <ov_arch/ov_arch_math.h>
#include <stdint.h>

/*----------------------------------------------------------------------------*/

static const char TO_ESCAPE[] = "\\\"";

#define MAGIC_BYTES 0x6276

#define OV_KEY_PATH_SEPARATOR '/'

/*----------------------------------------------------------------------------*/

struct ov_value {

    uint16_t magic_bytes;
    uint8_t type;

    struct ov_value *(*free)(struct ov_value *);

    struct ov_value *(*copy)(struct ov_value const *);

    size_t (*count)(struct ov_value const *);

    bool (*dump)(FILE *stream, ov_value const *);

    bool (*for_each)(ov_value const *,
                     bool (*func)(char const *key,
                                  ov_value const *,
                                  void *userdata),
                     void *userdata);

    bool (*match)(ov_value const *v1, ov_value const *v2);
};

/*----------------------------------------------------------------------------*/

static bool is_value(ov_value const *val) {

    if (0 == val) return false;

    return MAGIC_BYTES == val->magic_bytes;
}

/*****************************************************************************
                                Common functions
 ****************************************************************************/

ov_value *ov_value_free(ov_value *value) {

    if ((0 == value) || (0 == value->free)) goto error;

    return value->free(value);

error:

    return value;
}

/*----------------------------------------------------------------------------*/

ov_value *ov_value_copy(ov_value const *value) {

    if ((0 == value) || (0 == value->copy)) goto error;

    return value->copy(value);

error:

    return 0;
}

/*----------------------------------------------------------------------------*/

size_t ov_value_count(ov_value const *value) {
    if ((0 == value) || (0 == value->count)) goto error;

    return value->count(value);

error:

    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_value_dump(FILE *stream, ov_value const *value) {

    if ((0 == value) || (0 == value->dump)) goto error;

    return value->dump(stream, value);

error:

    return false;
}

/*----------------------------------------------------------------------------*/

char *ov_value_to_string(ov_value const *value) {

    char *dumped = 0;
    size_t length = 0;

    if (0 == value) goto error;

    FILE *string_stream = open_memstream(&dumped, &length);
    OV_ASSERT(0 != string_stream);

    bool succeeded_p = ov_value_dump(string_stream, value);

    fclose(string_stream);

    string_stream = 0;

    if (!succeeded_p) {
        goto error;
    }

    return dumped;

error:

    if (0 != dumped) {
        free(dumped);
    }

    return 0;
}

/*----------------------------------------------------------------------------*/

bool ov_value_for_each(ov_value const *value,
                       bool (*func)(char const *key,
                                    ov_value const *,
                                    void *userdata),
                       void *userdata) {

    if (0 == value) goto error;
    if (0 == value->for_each) goto error;

    return value->for_each(value, func, userdata);

error:

    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_value_match(ov_value const *v1, ov_value const *v2) {

    if (v1 == v2) return true;

    if ((0 == v1) || (0 == v2)) return false;

    OV_ASSERT(0 != v1);
    OV_ASSERT(0 != v1->match);

    return v1->match(v1, v2);
}

/*****************************************************************************
                "data functions" as required by ov_list
 ****************************************************************************/

static void *vptr_free(void *vptr) {

    if (!is_value(vptr)) return vptr;

    return ov_value_free(vptr);
}

/*----------------------------------------------------------------------------*/

static bool vptr_clear(void *vptr) {

    UNUSED(vptr);

    /* This is a total dummy ...  */
    return true;
}

/*----------------------------------------------------------------------------*/

void *vptr_copy(void **destination, const void *source) {

    if (!is_value(source)) goto error;

    ov_value *copy = ov_value_copy(source);

    if ((0 != destination) && (0 != *destination)) {

        *destination = copy;
    }

    return copy;

error:

    return 0;
}

/*----------------------------------------------------------------------------*/

static bool vptr_dump(FILE *stream, void const *vptr) {

    if (!is_value(vptr)) goto error;

    return ov_value_dump(stream, vptr);

error:

    return false;
}

/*****************************************************************************
                                  ATOM Helpers
 ****************************************************************************/

static ov_value *no_free_free(ov_value *value) {

    UNUSED(value);

    return 0;
}

/*----------------------------------------------------------------------------*/

static ov_value *no_copy_copy(ov_value const *value) {
    return (ov_value *)value;
}

/*----------------------------------------------------------------------------*/

static size_t count_for_non_composites(ov_value const *value) {

    UNUSED(value);

    return 1;
}

/*----------------------------------------------------------------------------*/

static bool for_each_for_non_composites(ov_value const *value,
                                        bool (*func)(char const *key,
                                                     ov_value const *val,
                                                     void *userdata),
                                        void *userdata) {

    if ((0 == value) || (0 == func)) {
        goto error;
    }

    if (!func(0, value, userdata)) {
        goto error;
    }

    return true;

error:

    return false;
}

/*----------------------------------------------------------------------------*/

static bool match_singletons(ov_value const *v1, ov_value const *v2) {

    return v1 == v2;
}

/*****************************************************************************
                                      null
 ****************************************************************************/

#define NULL_TYPE 0

/*----------------------------------------------------------------------------*/

static bool null_dump(FILE *stream, ov_value const *value) {

    UNUSED(value);

    if (0 == stream) return false;

    fprintf(stream, "null");

    return true;
}

/*----------------------------------------------------------------------------*/

static ov_value g_null = {

    .magic_bytes = MAGIC_BYTES,
    .type = NULL_TYPE,
    .free = no_free_free,
    .copy = no_copy_copy,
    .count = count_for_non_composites,
    .dump = null_dump,
    .for_each = for_each_for_non_composites,
    .match = match_singletons,

};

ov_value *ov_value_null() { return &g_null; }

bool ov_value_is_null(ov_value const *value) { return &g_null == value; }

/*****************************************************************************
                                      true
 ****************************************************************************/

#define TRUE_TYPE 1

/*----------------------------------------------------------------------------*/

static bool true_dump(FILE *stream, ov_value const *value) {

    UNUSED(value);
    if (0 == stream) return false;
    fprintf(stream, "true");

    return true;
}

/*----------------------------------------------------------------------------*/

static ov_value g_true = {

    .magic_bytes = MAGIC_BYTES,
    .type = TRUE_TYPE,
    .free = no_free_free,
    .copy = no_copy_copy,
    .count = count_for_non_composites,
    .dump = true_dump,
    .for_each = for_each_for_non_composites,
    .match = match_singletons,

};

ov_value *ov_value_true() { return &g_true; }

bool ov_value_is_true(ov_value const *value) { return &g_true == value; }

/*****************************************************************************
                                     false
 ****************************************************************************/

#define FALSE_TYPE 2

/*----------------------------------------------------------------------------*/

static bool false_dump(FILE *stream, ov_value const *value) {

    UNUSED(value);
    if (0 == stream) return false;
    fprintf(stream, "false");

    return true;
}

/*----------------------------------------------------------------------------*/

static ov_value g_false = {

    .magic_bytes = MAGIC_BYTES,
    .type = FALSE_TYPE,
    .free = no_free_free,
    .copy = no_copy_copy,
    .count = count_for_non_composites,
    .dump = false_dump,
    .for_each = for_each_for_non_composites,
    .match = match_singletons,

};

ov_value *ov_value_false() { return &g_false; }

bool ov_value_is_false(ov_value const *value) { return &g_false == value; }

/*****************************************************************************
                                     Number
 ****************************************************************************/

ov_registered_cache *g_number_cache = 0;

const uint16_t NUMBER_TYPE = 16;

typedef struct {

    ov_value pub;
    double data;

} Number;

/*----------------------------------------------------------------------------*/

static void *number_free_for_good(void *vptr) {

    if (0 != vptr) {
        free(vptr);
    }

    return 0;
}

static ov_value *number_free(ov_value *val) {

    if (0 == val) return val;

    if (0 != ov_registered_cache_put(g_number_cache, val)) {

        number_free_for_good(val);
    }

    return 0;
}

/*----------------------------------------------------------------------------*/

static ov_value *number_copy(ov_value const *val) {

    if (!ov_value_is_number(val)) goto error;

    double data = ov_value_get_number(val);

    return ov_value_number(data);

error:

    return 0;
}

/*----------------------------------------------------------------------------*/

static char const *get_format_string(double x) {

    // Prevent decimals being printed if we deal with an 'integer'

    double integral = 0.0;
    double decimals = modf(x, &integral);

    if (0 == decimals) {
        return "%.0lf";
    }

    return "%.10lf";
}

/*----------------------------------------------------------------------------*/

static bool number_dump(FILE *stream, ov_value const *val) {

    if ((0 == stream) || (!ov_value_is_number(val))) return false;

    Number *number = (Number *)val;

    const char *format_string = get_format_string(number->data);

    if (0 > fprintf(stream, format_string, number->data)) {
        goto error;
    }

    return true;

error:

    return false;
}

/*----------------------------------------------------------------------------*/

static bool match_number(ov_value const *v1, ov_value const *v2) {

    if (!ov_value_is_number(v1)) return false;
    if (!ov_value_is_number(v2)) return false;

    double d1 = ov_value_get_number(v1);
    double d2 = ov_value_get_number(v2);

    return OV_MAX_FLOAT_DELTA > fabs(d1 - d2);
}

/*----------------------------------------------------------------------------*/

ov_value *ov_value_number(double number) {

    Number *val = ov_registered_cache_get(g_number_cache);

    if (0 == val) {

        val = calloc(1, sizeof(Number));

        val->pub = (ov_value){

            .magic_bytes = MAGIC_BYTES,
            .type = NUMBER_TYPE,
            .free = number_free,
            .copy = number_copy,
            .count = count_for_non_composites,
            .dump = number_dump,
            .for_each = for_each_for_non_composites,
            .match = match_number,
        };
    }

    val->data = number;

    return &(val->pub);
}

/*----------------------------------------------------------------------------*/

bool ov_value_is_number(ov_value const *value) {

    if (!is_value(value)) goto error;

    if (NUMBER_TYPE != value->type) goto error;

    return true;

error:

    return false;
}

/*----------------------------------------------------------------------------*/

double ov_value_get_number(ov_value const *value) {

    if (!ov_value_is_number(value)) goto error;

    Number *number = (Number *)value;

    return number->data;

error:

    return 0;
}

/*****************************************************************************
                                     string
 ****************************************************************************/

ov_registered_cache *g_string_cache = 0;

const uint16_t STRING_TYPE = 16 + 4;

typedef struct {

    ov_value pub;
    char *data;

} String;

/*----------------------------------------------------------------------------*/

static void *string_free_for_good(void *vptr) {

    if (!is_value(vptr)) return vptr;

    String *s = vptr;

    if (STRING_TYPE != s->pub.type) return vptr;

    if (0 != s->data) free(s->data);

    free(s);

    return 0;
}

/*----------------------------------------------------------------------------*/

static ov_value *string_free(ov_value *val) {

    if (0 == val) {
        goto error;
    }

    if (STRING_TYPE != val->type) {
        goto error;
    }

    String *s = (String *)val;

    if (0 != s->data) {
        free(s->data);
        s->data = 0;
    }

    if (0 == ov_registered_cache_put(g_string_cache, val)) {

        return 0;
    }

    string_free_for_good(val);

    return 0;

error:

    return val;
}

/*----------------------------------------------------------------------------*/

static ov_value *string_copy(ov_value const *val) {

    if (0 == val) return 0;

    String *s = (String *)val;

    return ov_value_string(s->data);
}

/*----------------------------------------------------------------------------*/

static bool string_dump(FILE *stream, ov_value const *val) {

    if ((0 == stream) || (0 == val) || (STRING_TYPE != val->type)) {
        goto error;
    }

    String *string = (String *)val;

    char const *c_str = string->data;

    if (0 == c_str) {
        c_str = "";
    }

    fputc('"', stream);

    for (char const *ptr = c_str; 0 != *ptr; ++ptr) {

        if (0 != strchr(TO_ESCAPE, *ptr)) {
            fputc('\\', stream);
        }

        fputc(*ptr, stream);
    }

    fputc('"', stream);

    return true;

error:

    return false;
}

/*----------------------------------------------------------------------------*/

static bool string_match(ov_value const *v1, ov_value const *v2) {

    char const *s1 = ov_value_get_string(v1);
    char const *s2 = ov_value_get_string(v2);

    if ((0 == s1) || (0 == s2)) return false;

    return 0 == strcmp(s1, s2);
}

/*----------------------------------------------------------------------------*/

ov_value *ov_value_string(char const *string) {

    if (0 == string) goto error;

    String *val = ov_registered_cache_get(g_string_cache);

    if (0 == val) {

        val = calloc(1, sizeof(String));

        val->pub = (ov_value){

            .magic_bytes = MAGIC_BYTES,
            .type = STRING_TYPE,
            .free = string_free,
            .copy = string_copy,
            .count = count_for_non_composites,
            .dump = string_dump,
            .for_each = for_each_for_non_composites,
            .match = string_match,
        };
    }

    if (0 != val->data) {
        free(val->data);
    }

    val->data = strdup(string);

    return &(val->pub);

error:

    return 0;
}

/*----------------------------------------------------------------------------*/

char const *ov_value_get_string(ov_value const *value) {

    if ((0 == value) || (STRING_TYPE != value->type)) {
        goto error;
    }

    String const *string = (String const *)value;

    return string->data;

error:

    return 0;
}

/*****************************************************************************
                                      list
 ****************************************************************************/

static ov_registered_cache *g_list_cache = 0;

const uint16_t LIST_TYPE = 32;

typedef struct {

    ov_value pub;
    ov_list *data;

} List;

/*----------------------------------------------------------------------------*/

static void *list_free_for_good(void *vptr) {

    if (!is_value(vptr)) return vptr;

    List *l = vptr;

    if (LIST_TYPE != l->pub.type) return vptr;

    OV_ASSERT(0 != l->data);

    l->data = ov_list_free(l->data);

    OV_ASSERT(0 == l->data);

    free(l);

    return 0;
}

/*----------------------------------------------------------------------------*/

static ov_value *list_free(ov_value *value) {

    if (0 == value) {
        goto error;
    }

    if (LIST_TYPE != value->type) {
        goto error;
    }

    List *list = (List *)value;

    OV_ASSERT(0 != list->data);

    if (!ov_list_clear(list->data)) {
        goto error;
    }

    if (0 != ov_registered_cache_put(g_list_cache, value)) {

        list_free_for_good(value);
    }

    value = 0;

error:

    return value;
}

/*----------------------------------------------------------------------------*/

static bool copy_element_to_list(char const *key,
                                 ov_value const *value,
                                 void *userdata) {

    UNUSED(key);

    if ((0 == value) || (0 == userdata)) {
        goto error;
    }

    List *list = userdata;

    OV_ASSERT(0 != list->data);

    ov_value *copy = ov_value_copy(value);
    if (0 == copy) {
        goto error;
    }

    return ov_list_push(list->data, copy);

error:

    return false;
}

/*----------------------------------------------------------------------------*/

static ov_value *list_copy(ov_value const *list) {

    ov_value *copy = 0;

    if ((0 == list) || (LIST_TYPE != list->type)) {
        goto error;
    }

    copy = ov_value_list(0);

    if (0 == copy) {
        goto error;
    }

    if (!ov_value_for_each(list, copy_element_to_list, copy)) {
        goto error;
    }

    return copy;

error:

    copy = ov_value_free(copy);
    OV_ASSERT(0 == copy);

    return 0;
}

/*----------------------------------------------------------------------------*/

static size_t list_count(ov_value const *value) {

    if ((0 == value) || (LIST_TYPE != value->type)) {
        goto error;
    }

    List *l = (List *)value;

    OV_ASSERT(0 != l->data);

    return ov_list_count(l->data);

error:

    return 0;
}

/*----------------------------------------------------------------------------*/

struct dump_element_arg {

    FILE *stream;
    bool write_leading_comma;
};

static bool dump_list_element(char const *key,
                              ov_value const *value,
                              void *userdata) {

    UNUSED(key);

    if ((0 == value) || (0 == userdata)) {
        goto error;
    }

    struct dump_element_arg *arg = userdata;

    OV_ASSERT(0 != arg->stream);

    if (arg->write_leading_comma) {
        fprintf(arg->stream, ",");
    }

    arg->write_leading_comma = true;

    ov_value_dump(arg->stream, value);

    return true;

error:

    return false;
}

/*----------------------------------------------------------------------------*/

static bool list_dump(FILE *stream, ov_value const *value) {

    if ((0 == stream) || (0 == value) || (LIST_TYPE != value->type)) {
        goto error;
    }

    fprintf(stream, "[");

    struct dump_element_arg arg = {
        .stream = stream,
        .write_leading_comma = false,
    };

    bool retval = ov_value_for_each(value, dump_list_element, &arg);

    fprintf(stream, "]");

    return retval;

error:

    return false;
}

/*----------------------------------------------------------------------------*/

static bool list_for_each(ov_value const *value,
                          bool (*func)(char const *key,
                                       ov_value const *value,
                                       void *userdata),
                          void *userdata) {

    if ((0 == value) || (LIST_TYPE != value->type)) {
        goto error;
    }

    if (0 == func) {
        goto error;
    }

    List *l = (List *)value;

    OV_ASSERT(0 != l->data);

    const size_t len = ov_list_count(l->data);

    for (size_t i = 0; i < len; ++i) {

        if (!func(0, ov_list_get(l->data, 1 + i), userdata)) {
            goto error;
        }
    }

    return true;

error:

    return false;
}

/*----------------------------------------------------------------------------*/

static bool list_match(ov_value const *v1, ov_value const *v2) {

    if (0 == v1) return false;

    const size_t len = ov_value_count(v1);

    if (ov_value_count(v2) != len) return false;

    for (size_t i = 0; i < len; ++i) {

        if (!ov_value_match(ov_value_list_get((ov_value *)v1, i),
                            ov_value_list_get((ov_value *)v2, i))) {

            return false;
        }
    }

    return true;
}

/*----------------------------------------------------------------------------*/

ov_value *internal_ov_value_list(ov_value **values) {

    List *val = ov_registered_cache_get(g_list_cache);

    if (0 == val) {

        val = calloc(1, sizeof(List));

        val->pub = (ov_value){

            .magic_bytes = MAGIC_BYTES,
            .type = LIST_TYPE,
            .free = list_free,
            .copy = list_copy,
            .count = list_count,
            .dump = list_dump,
            .for_each = list_for_each,
            .match = list_match,

        };

        ov_list_config list_cfg = {
            .item.clear = vptr_clear,
            .item.free = vptr_free,
            .item.copy = vptr_copy,
            .item.dump = vptr_dump,
        };

        val->data = ov_list_create(list_cfg);
    }

    if (0 == val->data) goto error;

    if (0 == values) {
        goto finish;
    }

    while (0 != *values) {

        ov_list_push(val->data, *values);
        ++values;
    };

finish:

    return &(val->pub);

error:

    if (0 != val) {
        free(val);
    }

    return 0;
}

/*----------------------------------------------------------------------------*/

bool ov_value_is_list(ov_value const *value) {

    if (!is_value(value)) goto error;

    if (LIST_TYPE != value->type) goto error;

    return true;

error:

    return false;
}

/*----------------------------------------------------------------------------*/

ov_value *ov_value_list_get(ov_value *list, size_t i) {

    if ((0 == list) || (LIST_TYPE != list->type)) {
        goto error;
    }

    List *l = (List *)list;

    OV_ASSERT(0 != l->data);

    /* Unfortunately, ov_vector_list starts off its indices with '1' */
    return ov_list_get(l->data, 1 + i);

error:

    return 0;
}

/*----------------------------------------------------------------------------*/

ov_value *ov_value_list_set(ov_value *list, size_t i, ov_value *content) {

    if ((0 == list) || (LIST_TYPE != list->type)) {
        goto error;
    }

    List *l = (List *)list;

    OV_ASSERT(0 != l->data);

    ov_value *replaced = 0;

    /* Unfortunately, ov_vector_list starts off its indices with '1' */
    if (!ov_list_set(l->data, 1 + i, content, (void **)&replaced)) {
        goto error;
    }

    return replaced;

error:

    return 0;
}

/*----------------------------------------------------------------------------*/

bool ov_value_list_push(ov_value *list, ov_value *content) {

    if ((0 == list) || (0 == content) || (LIST_TYPE != list->type)) {
        goto error;
    }

    List *l = (List *)list;

    OV_ASSERT(0 != l->data);

    return ov_list_push(l->data, content);

error:

    return 0;
}

/*****************************************************************************
                                     Object
 ****************************************************************************/

ov_registered_cache *g_object_cache = 0;

const uint16_t OBJECT_TYPE = 64;

typedef struct {

    ov_value pub;
    ov_hashtable *data;

} Object;

/*----------------------------------------------------------------------------*/

bool ov_value_is_object(ov_value const *value) {

    if (!is_value(value)) goto error;

    if (OBJECT_TYPE != value->type) goto error;

    return true;

error:

    return false;
}

/*----------------------------------------------------------------------------*/

bool free_hashentry(const void *key, void const *value, void *arg) {

    UNUSED(key);
    UNUSED(arg);

    ov_value_free((void *)value);

    return true;
}

/*----------------------------------------------------------------------------*/

static ov_hashtable *free_hashtable(ov_hashtable *ht) {

    ov_hashtable_for_each(ht, free_hashentry, 0);

    return ov_hashtable_free(ht);
}

/*----------------------------------------------------------------------------*/

static void *object_free_for_good(void *vptr) {

    if (!is_value(vptr)) return vptr;

    Object *o = vptr;

    if (OBJECT_TYPE != o->pub.type) return vptr;

    o->data = free_hashtable(o->data);
    OV_ASSERT(0 == o->data);

    free(o);
    o = 0;

    return 0;
}

/*----------------------------------------------------------------------------*/

static ov_value *object_free(ov_value *value) {

    if ((0 == value) || (OBJECT_TYPE != value->type)) {

        goto error;
    }

    Object *o = (Object *)value;

    if (0 != o->data) {
        o->data = free_hashtable(o->data);
    }

    OV_ASSERT(0 == o->data);

    value = ov_registered_cache_put(g_object_cache, value);

    if (0 != value) {

        value = object_free_for_good(value);
    }

    value = 0;

error:

    return value;
}

/*----------------------------------------------------------------------------*/

static bool object_entry_copy(char const *key,
                              ov_value const *value,
                              void *varg) {

    ov_value *copy = ov_value_copy(value);

    if (0 == copy) return false;

    ov_value_object_set(varg, key, copy);

    return true;
}

/*----------------------------------------------------------------------------*/

static ov_value *object_copy(ov_value const *value) {

    ov_value *copy = ov_value_object();

    if (!ov_value_for_each(value, object_entry_copy, copy)) {

        copy = ov_value_free(copy);
    }

    return copy;
}

/*****************************************************************************
                                  object_count
 ****************************************************************************/

static bool object_counter(char const *key, ov_value const *value, void *arg) {

    UNUSED(key);
    UNUSED(value);

    size_t *counter = arg;

    OV_ASSERT(0 != counter);

    *counter += 1;

    return true;
}

/*----------------------------------------------------------------------------*/

static size_t object_count(ov_value const *value) {

    size_t counter = 0;

    ov_value_for_each(value, object_counter, &counter);

    return counter;
}

/*----------------------------------------------------------------------------*/

static bool dump_object_element(char const *key,
                                ov_value const *value,
                                void *userdata) {

    UNUSED(key);

    if ((0 == value) || (0 == userdata)) {
        goto error;
    }

    struct dump_element_arg *arg = userdata;

    OV_ASSERT(0 != arg->stream);

    if (arg->write_leading_comma) {
        fprintf(arg->stream, ",");
    }

    arg->write_leading_comma = true;

    fprintf(arg->stream, "\"%s\":", key);

    ov_value_dump(arg->stream, value);

    return true;

error:

    return false;
}

/*----------------------------------------------------------------------------*/

static bool object_dump(FILE *stream, ov_value const *value) {

    if ((0 == value) || (OBJECT_TYPE != value->type) || (0 == stream)) {
        goto error;
    }

    struct dump_element_arg arg = {
        .stream = stream,
        .write_leading_comma = false,
    };

    fprintf(stream, "{");

    bool retval = ov_value_for_each(value, dump_object_element, &arg);

    fprintf(stream, "}");

    return retval;

error:

    return false;
}

/*----------------------------------------------------------------------------*/

static bool object_for_each(ov_value const *value,
                            bool (*func)(char const *key,
                                         ov_value const *,
                                         void *userdata),
                            void *userdata) {

    if ((0 == value) || (OBJECT_TYPE != value->type) || (0 == func)) {
        goto error;
    }

    Object const *obj = (Object const *)value;

    ov_hashtable_for_each(obj->data,
                          (bool (*)(void const *, void const *, void *))func,
                          userdata);

    return true;

error:

    return false;
}

/*****************************************************************************
                                  object match
 ****************************************************************************/

struct object_entry_match_arg {

    bool matches;
    ov_value const *other_object;
};

/*----------------------------------------------------------------------------*/

static bool object_entry_match(char const *key,
                               ov_value const *value,
                               void *varg) {

    struct object_entry_match_arg *arg = varg;

    OV_ASSERT(0 != arg);

    ov_value const *other_value =
        ov_value_object_get((ov_value *)arg->other_object, key);

    if (!ov_value_match(value, other_value)) {

        arg->matches = false;
    }

    return arg->matches;
}

/*----------------------------------------------------------------------------*/

static bool object_match(ov_value const *v1, ov_value const *v2) {

    if (0 == v1) return false;

    struct object_entry_match_arg arg = {

        .matches = true,
        .other_object = v2,

    };

    ov_value_for_each(v1, object_entry_match, &arg);

    return arg.matches;
}
/*----------------------------------------------------------------------------*/

ov_value *ov_value_object() {

    size_t num_entries = 0;

    Object *obj = ov_registered_cache_get(g_object_cache);

    if (0 == obj) {

        obj = calloc(1, sizeof(Object));
        obj->pub = (ov_value){
            .magic_bytes = MAGIC_BYTES,
            .type = OBJECT_TYPE,
            .free = object_free,
            .copy = object_copy,
            .count = object_count,
            .dump = object_dump,
            .for_each = object_for_each,
            .match = object_match,

        };
    }

    if (0 == num_entries) {

        num_entries = 10;
    }

    obj->data = ov_hashtable_create_c_string(num_entries);

    //    for(size_t i = 0; 0 != pairs[i].key; ++i) {
    //
    //        ov_value_object_set(&obj->pub, pairs[i].key, pairs[i].value);
    //
    //    }

    return &obj->pub;
}

/*----------------------------------------------------------------------------*/

ov_value const *ov_value_object_get(ov_value const *value, char const *key) {

    ov_buffer *key_copy = 0;

    if ((0 == key) || (0 == value) || (OBJECT_TYPE != value->type)) {
        goto error;
    }

    Object *o = (Object *)value;

    char *separator_ptr = strchr(key, OV_KEY_PATH_SEPARATOR);

    if (0 == separator_ptr) {

        return ov_hashtable_get(o->data, key);
    }

    ptrdiff_t prefix_key_len = separator_ptr - key + 1;

    if (1 < prefix_key_len) {

        key_copy = ov_buffer_create(prefix_key_len);

        memcpy(key_copy->start, key, prefix_key_len);

        key_copy->start[prefix_key_len - 1] = 0;

        value = ov_hashtable_get(o->data, key_copy->start);

        key_copy = ov_buffer_free(key_copy);
    }

    separator_ptr += 1;

    return ov_value_object_get(value, separator_ptr);

error:

    key_copy = ov_buffer_free(key_copy);
    OV_ASSERT(0 == key_copy);

    return 0;
}

/*----------------------------------------------------------------------------*/

ov_value *ov_value_object_set(ov_value *value,
                              char const *key,
                              ov_value *content) {

    if ((0 == value) || (OBJECT_TYPE != value->type) || (0 == content)) {
        goto error;
    }

    Object *o = (Object *)value;

    return ov_hashtable_set(o->data, key, content);

error:

    return 0;
}

/*****************************************************************************
                                    CACHING
 ****************************************************************************/

void ov_value_enable_caching(size_t numbers,
                             size_t strings,
                             size_t lists,
                             size_t objects) {

    // Occasionally, we require a buffer for string manipulations,
    // thus enable / extend buffer caching

    ov_buffer_enable_caching(2);

    ov_registered_cache_config cfg = {

        .capacity = numbers,
        .item_free = number_free_for_good,
        .timeout_usec = 100 * 1000,

    };

    g_number_cache = ov_registered_cache_extend("ov_value_number", cfg);

    cfg.item_free = string_free_for_good;
    cfg.capacity = strings;

    g_string_cache = ov_registered_cache_extend("ov_value_strings", cfg);

    cfg.item_free = list_free_for_good;
    cfg.capacity = lists;

    g_list_cache = ov_registered_cache_extend("ov_value_lists", cfg);

    // every value list contains a list, but since the value do keep their
    // lists throughout their lifetime and ideally should be cached,
    // we do not need to extend the list cache appropriately

    cfg.item_free = object_free_for_good;
    cfg.capacity = objects;

    g_object_cache = ov_registered_cache_extend("ov_value_objects", cfg);
}
