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

#include "../include/ov_plugin_system.h"
#include "../include/ov_hashtable.h"
#include "../include/ov_plugin.h"
#include "../include/ov_teardown.h"
#include <../include/ov_string.h>
#include <../include/ov_utils.h>
#include <dlfcn.h>
#include <ftw.h>

/*----------------------------------------------------------------------------*/

ov_hashtable *g_plugins = 0;

static ov_hashtable *get_g_plugins() {

    if (0 == g_plugins) {

        g_plugins = ov_hashtable_create_c_string(20);
    }

    return g_plugins;
}

/*----------------------------------------------------------------------------*/

typedef struct {

    void *handle;
    char *spath;

} plugin_entry;

static plugin_entry *plugin_entry_create(char const *spath, void *handle) {

    if (ov_ptr_valid(handle, "Cannot create plugin entry: No valid handle")) {

        plugin_entry *entry = calloc(1, sizeof(plugin_entry));

        entry->handle = handle;
        entry->spath = ov_string_dup(spath);

        return entry;

    } else {
        return 0;
    }
}

/*----------------------------------------------------------------------------*/

static plugin_entry *plugin_entry_free(plugin_entry *self) {

    if (0 != self) {

        self->handle = 0;
        self->spath = ov_free(self->spath);
    }

    return ov_free(self);
}

/*----------------------------------------------------------------------------*/

static bool log_dl_error(char const *id, bool error_happened) {

    if (error_happened) {

        ov_log_error("Loading/Unloading %s failed: %s",
                     ov_string_sanitize(id),
                     ov_string_sanitize(dlerror()));
    }

    return error_happened;
}

/*****************************************************************************
                                 CLOSE / UNLOAD
 ****************************************************************************/

static bool close_so(char const *id, void *handle) {

    if (ov_ptr_valid(handle, "Cannot close plugin - invalid handle")) {

        return log_dl_error(id, (0 == dlclose(handle)));

    } else {

        return false;
    }
}

/*----------------------------------------------------------------------------*/

static bool unload_module(void const *id,
                          void const *restrict entry,
                          void *arg) {

    UNUSED(arg);

    if (0 != entry) {

        void *handle = ((plugin_entry *)entry)->handle;
        entry = plugin_entry_free((plugin_entry *)entry);

        void (*unload)() =
            ov_plugin_system_get_symbol(handle, OV_PLUGIN_UNLOAD);

        if (0 != unload) {
            unload();
            ov_log_info("Unloaded plugin %s", ov_string_sanitize(id));
        } else {
            ov_log_warning(
                "Could not unload plugin %s", ov_string_sanitize(id));
        }

        return close_so(id, (void *)handle);

    } else {
        return true;
    }
}

/*----------------------------------------------------------------------------*/

static ov_hashtable *g_symbols_for_plugins = 0;

/*----------------------------------------------------------------------------*/

bool ov_plugin_system_close() {

    ov_hashtable_for_each(g_plugins, unload_module, 0);

    g_plugins = ov_hashtable_free(g_plugins);
    g_symbols_for_plugins = ov_hashtable_free(g_symbols_for_plugins);

    return (0 == g_plugins) && (0 == g_symbols_for_plugins);
}

/*****************************************************************************
                                    LOADING
 ****************************************************************************/

static size_t open_dls = 0;

static void *load_so(char const *path) {

    void *handle = 0;

    if (ov_ptr_valid(path, "Cannot open SO: No path")) {

        handle = dlopen(path, RTLD_LAZY);
        log_dl_error(path, 0 == handle);
    }

    if (0 != handle) {
        ++open_dls;
    }

    return handle;
}

/*----------------------------------------------------------------------------*/

static bool register_so_handle_id(char const *path,
                                  char const *id,
                                  void *handle) {

    if (ov_ptr_valid(id, "Cannot register plugin - it did not provide an id")) {

        void *old = ov_hashtable_set(
            get_g_plugins(), id, plugin_entry_create(path, handle));

        if (0 != old) {

            ov_log_error("Plugin %s overwritten by %s",
                         ov_string_sanitize(id),
                         ov_string_sanitize(path));

            unload_module(id, old, 0);
        }

        return true;
    } else {

        return false;
    }
}

/*----------------------------------------------------------------------------*/

static char const *initialize_plugin(char const *path, void *handle) {

    char const *(*load_plugin)(ov_hashtable const *) =
        ov_plugin_system_get_symbol(handle, OV_PLUGIN_LOAD);

    if (0 != load_plugin) {

        return load_plugin(g_symbols_for_plugins);

    } else {

        ov_log_warning(
            "Could not load plugin %s - it does not provide " OV_PLUGIN_LOAD,
            ov_string_sanitize(path));

        return 0;
    }
}

/*----------------------------------------------------------------------------*/

static void system_close(void) { ov_plugin_system_close(); }

/*----------------------------------------------------------------------------*/

/**
 * @returns handle of loaded so or 0 in case of Error
 */
void *ov_plugin_system_load(char const *path, ov_plugin_load_flags flags) {

    void *handle = load_so(path);
    char const *id = initialize_plugin(path, handle);

    if (0 == handle) {

        ov_log_error("Could not load plugin %s", ov_string_sanitize(path));

    } else if (0 == id) {

        ov_log_error("Could not load plugin %s - could not initialize plugin",
                     ov_string_sanitize(path));
        dlclose(handle);
        handle = 0;

    } else if (!flags.simple) {
        register_so_handle_id(path, id, handle);
        ov_teardown_register(system_close, "Plugin system");
    }

    return handle;
}

/*----------------------------------------------------------------------------*/

bool ov_plugin_system_unload(char const *id) {

    return unload_module(id, ov_hashtable_remove(get_g_plugins(), id), 0);
}

/*****************************************************************************
                                 Load from dir
 ****************************************************************************/

struct so_dir_loader_args_struct {

    size_t so_counter;

} so_dir_loader_args;

static int so_dir_loader(const char *fpath,
                         const struct stat *sb,
                         int typeflag,
                         struct FTW *ftwbuf) {

    UNUSED(sb);
    UNUSED(ftwbuf);

    if (FTW_F == typeflag) {

        if (0 != ov_plugin_system_load(fpath, (ov_plugin_load_flags){0})) {

            ++so_dir_loader_args.so_counter;
            ov_log_info("Loaded plugin from %s", fpath);

        } else {
            ov_log_info(
                "Could not load plugin from %s", ov_string_sanitize(fpath));
        }
    }

    return 0;
}

/*----------------------------------------------------------------------------*/

/**
 * Load all plugins from a particular directory.
 * Automatic registering will be performed.
 */
ssize_t ov_plugin_system_load_dir(char const *path) {

    so_dir_loader_args = (struct so_dir_loader_args_struct){
        .so_counter = 0,
    };

    path = OV_OR_DEFAULT(path, OV_PLUGINS_INSTALLDIR);

    ov_log_info("Loading plugins from %s", ov_string_sanitize(path));

    if (ov_ptr_valid(path, "Cannot load from dir: Not a valid path") &&
        (0 == nftw(path, so_dir_loader, 10, 0))) {

        return so_dir_loader_args.so_counter;

    } else {

        return -1;
    }
}

/*----------------------------------------------------------------------------*/

void *ov_plugin_system_get_plugin(char const *id) {

    plugin_entry *entry = ov_hashtable_get(get_g_plugins(), id);

    if (entry) {
        return entry->handle;
    } else {
        return 0;
    }
}

/*----------------------------------------------------------------------------*/

void *ov_plugin_system_get_symbol(void *restrict handle,
                                  char const *restrict symname) {

    if (ov_ptr_valid(handle, "Cannot get symbol - invalid plugin handle") &&
        ov_ptr_valid(
            symname, "Cannot get plugin symbol - invalid symbol name")) {
        return dlsym(handle, symname);
    } else {
        return 0;
    }
}

/*----------------------------------------------------------------------------*/

bool ov_plugin_system_export_symbol_for_plugin(char const *name, void *symbol) {

    if (ov_ptr_valid(name, "Cannot register symbols for plugin: No name")) {

        if (0 == g_symbols_for_plugins) {

            g_symbols_for_plugins = ov_hashtable_create_c_string(20);
        }

        ov_hashtable_set(g_symbols_for_plugins, name, symbol);

        return true;

    } else {

        return false;
    }
}

/*****************************************************************************
                                     TESTS
 ****************************************************************************/

#define OV_PLUGIN_SYSTEM_TEST_ID "base_plugin_test"

static bool plugin_loaded = false;

char const *ov_plugin_load(ov_hashtable const *symbols) {

    plugin_loaded = true;
    void (*count_loaded)() = ov_hashtable_get(symbols, "counter_1");

    if (0 != count_loaded) {
        count_loaded();
    } else {
        fprintf(stderr, "Could not get symbol 'counter_1'\n");
    }

    return OV_PLUGIN_SYSTEM_TEST_ID;
}

/*----------------------------------------------------------------------------*/

void ov_plugin_unload() { plugin_loaded = false; }

/*----------------------------------------------------------------------------*/

bool plugin_state(void) { return plugin_loaded; }

/*----------------------------------------------------------------------------*/
