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

/*----------------------------------------------------------------------------*/

#include "ov_sip_permission.h"
#include <ov_base/ov_config_keys.h>
#include <ov_base/ov_json.h>
#include <ov_base/ov_string.h>
#include <ov_base/ov_utils.h>

/*----------------------------------------------------------------------------*/

static ov_json_value *json_string(char const *str) {

    if (0 == str) {
        return 0;
    } else {
        return ov_json_string(str);
    }
}

/*----------------------------------------------------------------------------*/

struct put_return {

    ov_json_value *value;
    bool ok;
};

static struct put_return put(ov_json_value *object, char const *key,
                             ov_json_value *value) {

    struct put_return ret = {0};

    if (ov_json_object_set(object, key, value)) {

        ret.ok = true;
        return ret;

    } else {

        ret.ok = false;
        ret.value = value;
        return ret;
    }
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_sip_permission_to_json(ov_sip_permission self) {

    ov_json_value *permission = ov_json_object();

    struct put_return rv_caller =
        put(permission, OV_KEY_CALLER, json_string(self.caller));

    struct put_return rv_callee =
        put(permission, OV_KEY_CALLEE, json_string(self.callee));

    struct put_return rv_loop =
        put(permission, OV_KEY_LOOP, json_string(self.loop));

    struct put_return rv_from =
        put(permission, OV_KEY_VALID_FROM, ov_json_number(self.from_epoch));

    struct put_return rv_until =
        put(permission, OV_KEY_VALID_UNTIL, ov_json_number(self.until_epoch));

    if ((rv_caller.ok || rv_callee.ok) && rv_loop.ok && rv_from.ok &&
        rv_until.ok) {

        return permission;

    } else {

        rv_caller.value = ov_json_value_free(rv_caller.value);
        rv_callee.value = ov_json_value_free(rv_callee.value);
        rv_loop.value = ov_json_value_free(rv_loop.value);
        rv_from.value = ov_json_value_free(rv_from.value);
        rv_until.value = ov_json_value_free(rv_until.value);
        permission = ov_json_value_free(permission);

        return 0;
    }
}

/*----------------------------------------------------------------------------*/

static bool is_valid_number(ov_json_value const *val) {

    if (!ov_json_is_number(val)) {
        char *str = ov_json_value_to_string(val);
        ov_log_error("%s is not a valid number", str);
        free(str);
        return false;
    } else {
        return true;
    }
}

/*----------------------------------------------------------------------------*/

static bool is_positive_number(double val) {

    if (0 > val) {

        ov_log_error("Positive number expected, but got %f", val);
        return false;

    } else {
        return true;
    }
}

/*----------------------------------------------------------------------------*/

struct uint64_from_json_retval {

    uint64_t value;
    bool ok;
    bool not_there;
};

static struct uint64_from_json_retval
uint64_from_json(ov_json_value const *jval) {

    struct uint64_from_json_retval retval = {
        .ok = false,
    };

    double val = ov_json_number_get(jval);

    if (0 == val) {
        retval.not_there = true;
        retval.value = 0;
    } else if (is_valid_number(jval) && is_positive_number(val)) {
        retval.ok = true;
        retval.value = (uint64_t)val;
    }
    return retval;
}

/*----------------------------------------------------------------------------*/

static void set(bool *var, bool value) {

    if (0 != var) {
        *var = value;
    }
}

/*----------------------------------------------------------------------------*/

ov_sip_permission ov_sip_permission_from_json(ov_json_value const *jval,
                                              bool *ok) {
    ov_sip_permission p = {0};

    char const *caller =
        ov_json_string_get(ov_json_get(jval, "/" OV_KEY_CALLER));

    char const *callee =
        ov_json_string_get(ov_json_get(jval, "/" OV_KEY_CALLEE));

    char const *loop = ov_json_string_get(ov_json_get(jval, "/" OV_KEY_LOOP));

    struct uint64_from_json_retval from_epoch =
        uint64_from_json(ov_json_get(jval, "/" OV_KEY_VALID_FROM));

    struct uint64_from_json_retval until_epoch =
        uint64_from_json(ov_json_get(jval, "/" OV_KEY_VALID_UNTIL));

    if (((0 != caller) || (0 != callee)) && (0 != loop) &&
        (from_epoch.not_there || from_epoch.ok) &&
        (until_epoch.not_there || until_epoch.ok)) {

        set(ok, true);

        p.caller = caller;
        p.callee = callee;
        p.loop = loop;
        p.from_epoch = from_epoch.value;
        p.until_epoch = until_epoch.value;

    } else {

        set(ok, false);
    }

    return p;
}

/*----------------------------------------------------------------------------*/

bool ov_sip_permission_equals(const ov_sip_permission p1,
                              const ov_sip_permission p2) {

    if (((0 != p2.from_epoch) && (p1.from_epoch != p2.from_epoch)) ||
        ((0 != p2.until_epoch) && (p1.until_epoch != p2.until_epoch)) ||
        (!ov_string_equal(p1.caller, p2.caller)) ||
        (!ov_string_equal(p1.callee, p2.callee)) ||
        (!ov_string_equal(p1.loop, p2.loop))) {
        return false;
    } else {
        return true;
    }
}

/*----------------------------------------------------------------------------*/

static bool permission_clear(void *source) {

    if (0 != source) {
        memset(source, 0, sizeof(*source));
        return true;
    } else {
        return false;
    }
}

/*----------------------------------------------------------------------------*/

static void *permission_copy(void **destination, const void *source) {

    ov_sip_permission *dest = 0;

    if (0 != source) {

        if (0 != destination) {
            dest = *destination;
        }

        dest = OV_OR_DEFAULT(dest, calloc(1, sizeof(ov_sip_permission)));

        if (0 != destination) {
            *destination = dest;
        }

        memcpy(dest, source, sizeof(*source));
    }

    return dest;
}

/*----------------------------------------------------------------------------*/

char const *str_with_prefix(char *target, size_t target_size,
                            char const *prefix, char const *str) {

    if (0 == str) {

        return "";

    } else if ((0 == prefix) || (0 == target)) {

        return str;

    } else {

        memset(target, 0, target_size);
        snprintf(target, target_size, "%s: %s\n", prefix, str);
        return target;
    }
}

/*----------------------------------------------------------------------------*/

static bool permission_dump(FILE *stream, const void *source) {

    ov_sip_permission const *perm = source;

    if ((0 != stream) && (0 != perm)) {

        char buf[255] = {0};

        fprintf(stream, "Permission:\n%s%s%s%" PRIu64 "%" PRIu64 "\n",
                str_with_prefix(buf, sizeof(buf), "Loop:   ", perm->loop),
                str_with_prefix(buf, sizeof(buf), "Caller: ", perm->caller),
                str_with_prefix(buf, sizeof(buf), "Callee: ", perm->callee),
                perm->from_epoch, perm->until_epoch);

        return true;

    } else {

        return false;
    }
}

/*----------------------------------------------------------------------------*/

ov_data_function ov_sip_permission_data_functions() {

    return (ov_data_function){
        .clear = permission_clear,
        .free = ov_free,
        .copy = permission_copy,
        .dump = permission_dump,
    };
}

/*----------------------------------------------------------------------------*/
