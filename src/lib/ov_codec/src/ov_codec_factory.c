/***

Copyright   2017        German Aerospace Center DLR e.V.,
                        German Space Operations Center (GSOC)

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.

    This file is part of the openvocs project. http://openvocs.org

***/ /**

     \file               ov_codec_factory.c
     \author             Michael J. Beer, DLR/GSOC <michael.beer@dlr.de>
     \date               2017-12-06

     \ingroup            empty

     \brief              empty

 **/

#include <ov_base/ov_config_keys.h>
#include <ov_base/ov_json.h>

#include "../include/ov_codec_factory.h"
#include "../include/ov_codec_g711.h"
#include "../include/ov_codec_opus.h"
#include "../include/ov_codec_pcm16_signed.h"
#include "../include/ov_codec_raw.h"
#include "ov_base/ov_error_codes.h"
#include <dlfcn.h>
#include <ftw.h>
#include <ov_base/ov_string.h>
#include <ov_base/ov_utils.h>
#include <ov_base/ov_plugin_system.h>

/*****************************************************************************
                                 Global Factory
 ****************************************************************************/

static ov_codec_factory *g_factory = 0;

/*----------------------------------------------------------------------------*/

static ov_codec_factory *get_global_factory() {

    if (0 != g_factory) {

        return g_factory;
    }

    g_factory = ov_codec_factory_create_standard();

    return g_factory;
}

/******************************************************************************
 * INTERNALs
 ******************************************************************************/

struct ov_codec_factory_struct {
    /* Ok, the factory is basically a dictionary comprised of a linked list.
     * The last element is empty for the sake of simple implementation, thus
     * The linked list ALWAYS contains at least one entry.
     * This entry must be all-zeroed out.
     * This entry denotes the head of the list.
     * Therefore, a new element will be added as head->next always.
     */
    struct ov_codec_factory_struct *next;
    char *codec_name;
    ov_codec_generator get_codec_for;
    void *shared_object_handle;
};

typedef struct ov_codec_factory_struct codec_entry;

/*
 * The heart of the linked list thingy and rather UN-intuitive -
 * but since its purely internal this does not matter that much...
 * Always either returns 0 or a pointer p such that
 * p->next->codec_name equals codec_name
 */
static codec_entry *find_entry(const codec_entry *list, const char *codec_name);

/******************************************************************************
 *
 *  PUBLIC FUNCTIONS
 *
 ******************************************************************************/

ov_codec_factory *ov_codec_factory_create() {

    ov_codec_factory *factory =
        calloc(1, sizeof(struct ov_codec_factory_struct));

    return factory;
}

/*---------------------------------------------------------------------------*/

ov_codec_factory *ov_codec_factory_create_standard() {

    ov_codec_factory *factory = ov_codec_factory_create();

    if (0 != ov_codec_raw_install(factory)) {

        goto error;
    }

    if (0 != ov_codec_pcm16_signed_install(factory)) {

        goto error;
    }

    if (0 != ov_codec_opus_install(factory)) {

        goto error;
    }

    if (0 != ov_codec_g711_install(factory)) {

        goto error;
    }

    return factory;

error:

    if (0 == factory) factory = ov_codec_factory_free(factory);

    return 0;
}

/*----------------------------------------------------------------------------*/

static size_t open_dls = 0;

static void *open_dl_handle(char const *path) {

    void *handle = 0;

    if (ov_ptr_valid(path, "Cannot open SO: No path")) {

        handle = dlopen(path, RTLD_NOW);

        char *so_error = dlerror();

        if (0 != so_error) {
            ov_log_error("Error during loading SO: %s", so_error);
        }
    }

    if (0 != handle) {
        ++open_dls;
    }

    return handle;
}

/*----------------------------------------------------------------------------*/

static void *close_dl_handle(void *handle) {

    if ((0 != handle) && (0 == dlclose(handle))) {
        --open_dls;

        char *so_error = dlerror();

        if (0 != so_error) {
            ov_log_error("Error closing SO: %s", so_error);
        }
    }

    return handle;
}

/*----------------------------------------------------------------------------*/

ov_codec_factory *ov_codec_factory_free(ov_codec_factory *factory) {

    if (0 == factory) {
        factory = get_global_factory();
        g_factory = 0;
    }

    OV_ASSERT(0 != factory);

    codec_entry *head = factory;

    while (0 != head->next) {

        head = head->next;
        free(factory);
        factory = head;

        head->codec_name = ov_free(head->codec_name);
        head->shared_object_handle =
            close_dl_handle(head->shared_object_handle);
    }

    free(head);

    return 0;
}

/*---------------------------------------------------------------------------*/

static codec_entry *new_entry(ov_codec_factory *factory,
                              char const *codec_name) {

    codec_entry *entry = calloc(1, sizeof(codec_entry));
    entry->codec_name = ov_string_dup(codec_name);
    entry->next = factory->next;
    factory->next = entry;
    entry = factory;

    return entry;
}

/*----------------------------------------------------------------------------*/

static bool codec_factory_install_codec(ov_codec_factory *factory,
                                        const char *codec_name,
                                        ov_codec_generator generate_codec,
                                        void *dlhandle,
                                        ov_codec_generator *old_generator) {

    factory = OV_OR_DEFAULT(factory, get_global_factory());

    if ((0 == codec_name) || (0 == generate_codec)) {

        return false;

    } else {

        codec_entry *entry = find_entry(factory, codec_name);

        entry = OV_OR_DEFAULT(entry, new_entry(factory, codec_name));

        entry->next->shared_object_handle =
            close_dl_handle(entry->next->shared_object_handle);

        ov_codec_generator old = entry->next->get_codec_for;

        entry->next->get_codec_for = generate_codec;
        entry->next->shared_object_handle = dlhandle;

        if (0 != old_generator) {
            *old_generator = old;
        }

        return true;
    }
}

/*----------------------------------------------------------------------------*/

ov_codec_generator ov_codec_factory_install_codec(
    ov_codec_factory *factory,
    const char *codec_name,
    ov_codec_generator generate_codec) {

    ov_codec_generator old_gen = 0;
    codec_factory_install_codec(
        factory, codec_name, generate_codec, 0, &old_gen);
    return old_gen;
}

/*---------------------------------------------------------------------------*/

ov_codec *ov_codec_factory_get_codec(ov_codec_factory *factory,
                                     const char *codec_name,
                                     uint32_t ssid,
                                     const ov_json_value *parameters) {

    if (0 == factory) {
        factory = get_global_factory();
    }

    OV_ASSERT(0 != factory);

    if (0 == codec_name) goto error;

    codec_entry *entry = find_entry(factory, codec_name);

    ov_codec *codec = 0;

    if ((0 != entry) && (0 != entry->next)) {

        OV_ASSERT(0 != entry->next->get_codec_for);

        codec = entry->next->get_codec_for(ssid, parameters);
    }

    if (0 != codec) {
        ov_codec_enable_resampling(codec);
    }

    return codec;

error:

    return 0;
}

/******************************************************************************
 * INTERNAL FUNCTIONS
 ******************************************************************************/

static codec_entry *find_entry(const codec_entry *list,
                               const char *codec_name) {

    if (0 == list) goto error;

    codec_entry *c = (codec_entry *)list;

    OV_ASSERT(0 != c);

    while (0 != c->next) {

        OV_ASSERT(0 != c->next);

        if (0 == strcmp(codec_name, c->next->codec_name)) {

            OV_ASSERT(0 != c->next);

            return c;
        }

        c = c->next;
    }

    OV_ASSERT(0 == c->next);

error:

    return 0;
}

/*---------------------------------------------------------------------------*/

ov_codec *ov_codec_factory_get_codec_from_json(ov_codec_factory *factory,
                                               const ov_json_value *json,
                                               uint32_t ssid) {

    if (0 == factory) {

        factory = get_global_factory();
    }

    if (0 == json) goto error;

    char const *codec_type;
    codec_type = ov_json_string_get(ov_json_get(json, "/" OV_KEY_CODEC));

    if (0 == codec_type) {

        ov_log_error("Invalid codec configuration: %s missing", OV_KEY_CODEC);
        goto error;
    }

    return ov_codec_factory_get_codec(factory, codec_type, ssid, json);

error:

    return 0;
}

/*****************************************************************************
                     Support for codecs with plugin system
 ****************************************************************************/

bool ov_codec_factory_export_symbols_for_plugins() {
    return ov_plugin_system_export_symbol_for_plugin(
        "ov_codec_factory_install_codec_1", ov_codec_factory_install_codec);
}

/*****************************************************************************
                             SHARED OBJECT SUPPORT
 ****************************************************************************/

#define OV_CODEC_CREATE_FUNCNAME "openvocs_plugin_codec_create"
#define OV_CODEC_ID_FUNCNAME "openvocs_plugin_codec_id"

static bool install_codec(
    ov_codec_factory *self, void *dlhandle, char const *(*id_func)(void),
    ov_codec *(*create_func)(uint32_t ssid, const ov_json_value *parameters),
    ov_result *res) {
    if (0 == id_func) {
        ov_result_set(res, OV_ERROR_CODE_NOT_IMPLEMENTED,
                      "Shared object does not provide a " OV_CODEC_ID_FUNCNAME);
        return false;

    } else if (0 == create_func) {
        ov_result_set(res, OV_ERROR_CODE_NOT_IMPLEMENTED,
                      "Shared object does not provide "
                      "a " OV_CODEC_CREATE_FUNCNAME);
        return false;

    } else if (codec_factory_install_codec(self, id_func(), create_func,
                                           dlhandle, 0)) {
        ov_result_set(res, OV_ERROR_NOERROR, 0);
        return true;
    } else {
        ov_result_set(res, OV_ERROR_INTERNAL_SERVER,
                      "Could not install codec from SO");
        return false;
    }
}

/*----------------------------------------------------------------------------*/

static bool install_codec_from_handle(ov_codec_factory *self, void *handle,
                                      ov_result *res) {
    if (0 != handle) {
        return install_codec(self, handle, dlsym(handle, OV_CODEC_ID_FUNCNAME),
                             dlsym(handle, OV_CODEC_CREATE_FUNCNAME), res);

    } else {
        ov_result_set(res, OV_ERROR_BAD_ARG,
                      "Shared object could not be opened");
        return false;
    }
}

/*----------------------------------------------------------------------------*/

bool ov_codec_factory_install_codec_from_so(ov_codec_factory *self,
                                            char const *so_path,
                                            ov_result *res) {
    void *handle = open_dl_handle(so_path);

    if (install_codec_from_handle(self, handle, res)) {
        return true;

    } else {
        handle = close_dl_handle(handle);
        return false;
    }
}

/*----------------------------------------------------------------------------*/

struct so_dir_loader_args_struct {
    size_t so_counter;
    ov_codec_factory *factory;

} so_dir_loader_args;

static int so_dir_loader(const char *fpath, const struct stat *sb, int typeflag,
                         struct FTW *ftwbuf) {
    UNUSED(sb);
    UNUSED(ftwbuf);

    ov_result res = {0};

    if (FTW_F == typeflag) {
        if (ov_codec_factory_install_codec_from_so(so_dir_loader_args.factory,
                                                   fpath, &res)) {
            ++so_dir_loader_args.so_counter;
            ov_log_info("Loaded codec from %s", fpath);

        } else {
            ov_log_info("Could not load codec from %s: %s",
                        ov_string_sanitize(fpath), ov_result_get_message(res));
        }

        ov_result_clear(&res);
    }

    return 0;
}

/*----------------------------------------------------------------------------*/

ssize_t ov_codec_factory_install_codec_from_so_dir(ov_codec_factory *self,
                                                   char const *so_path,
                                                   ov_result *res) {
    so_dir_loader_args = (struct so_dir_loader_args_struct){
        .so_counter = 0,
        .factory = self,
    };

    if (ov_ptr_valid(so_path, "Cannot load from dir: Not a valid path") &&
        (0 == nftw(so_path, so_dir_loader, 10, 0))) {
        return so_dir_loader_args.so_counter;
    } else {
        ov_result_set(res, OV_ERROR_BAD_ARG,
                      "Cannot load from dir: Not a valid path");
        return -1;
    }
}

/*----------------------------------------------------------------------------*/
