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
#include "../include/ov_id.h"
#include "../include/ov_random.h"
#include "../include/ov_utils.h"

#include <string.h>

/*----------------------------------------------------------------------------*/

bool ov_id_valid(char const *id) {

    if ((0 == id) || (0 == id[0])) {
        return false;
    } else {
        return 37 > strnlen(id, 37);
    }
}

/*----------------------------------------------------------------------------*/

bool ov_id_clear(ov_id id) {

    if (0 != id) {
        memset(id, 0, sizeof(ov_id));
        return true;
    } else {
        return false;
    }
}

/*----------------------------------------------------------------------------*/

bool ov_id_set(ov_id id, char const *str) {

    if ((0 == id) || (!ov_id_valid(str))) {
        return false;
    } else {
        memset(id, 0, sizeof(ov_id));
        strncpy(id, str, sizeof(ov_id) - 1);
        return true;
    }
}

/*----------------------------------------------------------------------------*/

char *ov_id_dup(char const *id) {

    if (!ov_id_valid(id)) {
        return 0;
    } else {
        return strndup(id, sizeof(ov_id));
    }
}

/*----------------------------------------------------------------------------*/

static char *add_dash_nocheck(char *wptr) {

    *wptr = '-';
    return wptr + 1;
}

/*----------------------------------------------------------------------------*/

static char *add_random_digits(char *wptr, size_t num_digits_to_add) {

    // ov_random_string creates terminal 0, thus we produce one more
    // and ignore it afterwards
    ov_random_string(&wptr, num_digits_to_add + 1, "01234567890abcdef");
    return wptr + num_digits_to_add;
}

/*----------------------------------------------------------------------------*/

bool ov_id_fill_with_uuid(ov_id target) {

    if (ov_ptr_valid(target, "Cannot fill ID: No ID to write to")) {

        //  8       -  4 - 4  - 4  -      12
        // "cdfe55fb-2ade-44dd-8228-96554aaeaf6c"

        char *wptr = add_random_digits(target, 8);
        wptr = add_dash_nocheck(wptr);

        wptr = add_random_digits(wptr, 4);
        wptr = add_dash_nocheck(wptr);

        wptr = add_random_digits(wptr, 4);
        wptr = add_dash_nocheck(wptr);

        wptr = add_random_digits(wptr, 4);
        wptr = add_dash_nocheck(wptr);

        wptr = add_random_digits(wptr, 12);
        *wptr = 0;

        return true;

    } else {
        return false;
    }
}

/*----------------------------------------------------------------------------*/

bool ov_id_match(char const *restrict id1, char const *restrict id2) {

    if ((0 == id1) || (0 == id2)) {
        return id1 == id2;
    } else {
        for (size_t i = 0; i < sizeof(ov_id) - 1; ++i) {
            if ((0 == id1[i]) && (0 == id2[i])) {
                return true;
            } else if (id1[i] != id2[i]) {
                return false;
            }
        }

        char trailer1 = id1[sizeof(ov_id) - 1];
        return (0 == trailer1) && (trailer1 == id2[sizeof(ov_id) - 1]);
    }
}

/*----------------------------------------------------------------------------*/

bool ov_id_array_reset(ov_id ids[], size_t capacity) {

    if (!ov_ptr_valid(ids, "Cannot reset ID array: No array")) {

        return false;

    } else {

        for (size_t i = 0; i < capacity; ++i) {
            ov_id_clear(ids[i]);
        }

        return true;
    }
}

/*----------------------------------------------------------------------------*/

bool ov_id_array_add(ov_id ids[], size_t capacity, char const *id) {

    if ((!ov_ptr_valid(ids, "Cannot add ID to array: No array")) ||
        (!ov_ptr_valid(id, "Cannot add ID to array: No ID"))) {

        return false;

    } else {

        for (size_t i = 0; i < capacity; ++i) {
            if (!ov_id_valid(ids[i])) {
                return ov_id_set(ids[i], id);
            }
        }

        return false;
        ;
    }
}

/*----------------------------------------------------------------------------*/

bool ov_id_array_del(ov_id ids[], size_t capacity, char const *id) {

    if ((!ov_ptr_valid(ids, "Cannot add ID to array: No array")) ||
        (!ov_ptr_valid(id, "Cannot add ID to array: No ID"))) {

        return false;

    } else {

        for (size_t i = 0; i < capacity; ++i) {
            if (ov_id_match(ids[i], id)) {
                return ov_id_clear(ids[i]);
            }
        }

        return true;
    }
}

/*----------------------------------------------------------------------------*/

ssize_t ov_id_array_get_index(ov_id const *ids, size_t capacity,
                              char const *id) {

    if ((!ov_ptr_valid(ids, "Cannot add ID to array: No array")) ||
        (!ov_ptr_valid(id, "Cannot add ID to array: No ID"))) {

        return -1;

    } else {

        for (size_t i = 0; i < capacity; ++i) {
            if (ov_id_match(ids[i], id)) {
                return i;
            }
        }

        return -1;
    }
}

/*----------------------------------------------------------------------------*/

ssize_t ov_id_array_next(ov_id const *ids, size_t capacity,
                         ssize_t last_index) {

    if (-1 > last_index) {
        last_index = -1;
    }

    if (!ov_ptr_valid(ids, "Cannot iterate over ID array - no array")) {
        return -1;
    } else {

        for (size_t i = 1 + last_index; i < capacity; ++i) {

            if (ov_id_valid(ids[i])) {
                return i;
            }
        }

        return -1;
    }
}

/*----------------------------------------------------------------------------*/
