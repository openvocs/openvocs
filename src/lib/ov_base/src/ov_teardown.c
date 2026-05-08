/***
        ------------------------------------------------------------------------

        Copyright (c) 2024 German Aerospace Center DLR e.V. (GSOC)

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

#include "../include/ov_teardown.h"
#include "../include/ov_string.h"
#include "../include/ov_utils.h"

/*----------------------------------------------------------------------------*/

typedef struct destructor_entry {

    void (*destructor)(void);
    struct destructor_entry *next;
    char *name;

} DestructorEntry;

static DestructorEntry *g_destructors = 0;

/*----------------------------------------------------------------------------*/

DestructorEntry *destructor_entry(void (*destructor)(void), char const *name,
                                  DestructorEntry *next) {

    DestructorEntry *entry = calloc(1, sizeof(DestructorEntry));
    entry->destructor = destructor;
    entry->name = ov_string_dup(OV_OR_DEFAULT(name, "UNKNOWN"));
    entry->next = next;
    return entry;
}

/*----------------------------------------------------------------------------*/

DestructorEntry *get_destructors() {

    g_destructors =
        OV_OR_DEFAULT(g_destructors, destructor_entry(0, "teardown system", 0));
    return g_destructors;
}

/*----------------------------------------------------------------------------*/

static DestructorEntry const *find(DestructorEntry const *entry,
                                   void (*destructor)(void)) {

    for (; 0 != entry; entry = entry->next) {
        if (entry->destructor == destructor) {
            return entry;
        }
    }

    return 0;
}

/*----------------------------------------------------------------------------*/

bool ov_teardown_register(void (*destructor)(void), char const *name) {

    return ((0 != destructor) && (0 != find(get_destructors(), destructor))) ||
           ov_teardown_register_multiple(destructor, name);
}

/*----------------------------------------------------------------------------*/

bool ov_teardown_register_multiple(void (*destructor)(void), char const *name) {

    if (0 == destructor) {
        return false;
    } else {
        g_destructors = destructor_entry(destructor, name, get_destructors());
        return true;
    }
}

/*----------------------------------------------------------------------------*/

/*
 * @return 'next' entry or 0 in case of error
 */
static DestructorEntry *destruct(DestructorEntry *destructor) {

    if (0 != destructor) {
        if (0 != destructor->destructor) {
            ov_log_info("Destructing %s", ov_string_sanitize(destructor->name));
            destructor->destructor();
        }
        DestructorEntry *entry = destructor->next;
        ov_free(destructor->name);
        ov_free(destructor);
        return entry;
    } else {
        return destructor;
    }
}

/*----------------------------------------------------------------------------*/

bool ov_teardown() {

    while (g_destructors) {
        g_destructors = destruct(g_destructors);
    }

    return 0 == g_destructors;
}

/*----------------------------------------------------------------------------*/
