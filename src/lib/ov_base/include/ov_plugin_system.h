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

        @author Michael J. Beer, DLR/GSOC
        @copyright (c) 2023 German Aerospace Center DLR e.V. (GSOC)

        ------------------------------------------------------------------------
*/
#ifndef OV_PLUGIN_SYSTEM_H
#define OV_PLUGIN_SYSTEM_H
/*----------------------------------------------------------------------------*/

#include <stdbool.h>
#include <stdio.h>

/**
 * Unload all registered modules and free all resources.
 */
bool ov_plugin_system_close();

typedef struct {

    /** Only perform load and do not any automatic registering etc. */
    bool simple : 1;

} ov_plugin_load_flags;

/**
 * @returns handle of loaded so or 0 in case of Error
 */
void *ov_plugin_system_load(char const *path, ov_plugin_load_flags flags);

bool ov_plugin_system_unload(char const *id);

/**
 * Load all plugins from a particular directory.
 * Automatic registering will be performed.
 * @return number of loaded plugins or negative number in case of error
 */
ssize_t ov_plugin_system_load_dir(char const *path);

void *ov_plugin_system_get_plugin(char const *id);

void *ov_plugin_system_get_symbol(void *restrict handle,
                                  char const *restrict symname);

/**
 * Define symbols that should be available for a plugin.
 * When a plugin is loaded, it is handed a dictionary of all defined
 * symbols.
 * Beware to always put a version number onto a symbol string, and change
 * the version number whenever the symbol's interace changes:
 *
 * You got a function
 *
 * `bool register_codec(ov_codec *codec)`
 *
 * that each 'codec' module requires.
 * Define the symbol via
 *
 * `ov_plugin_system_register_symbol("register_codec_1", register_codec);`
 *
 * When a module is loaded, its ov_plugin_load() method now can get the
 * pointer to `register_codec(ov_codec *codec)` via the symbol dictionary it
 * is called with.
 *
 * You then decide that you need to alter the function
 *
 * `bool register_codec(ov_codec *codec)`
 *
 * to
 *
 * `bool register_codec(ov_codec *codec, char const *name);`
 *
 * Register it as a symbol with a different version number:
 *
 * `ov_plugin_system_register_symbol("register_codec_2", register_codec);`
 *
 * even if the function name stays the same, to prevent a plugin to get a
 * pointer to a function `bool (*)(ov_codec *, char const *)`
 * when it expects a function `bool (*)(ov_codec *)`.
 */
bool ov_plugin_system_export_symbol_for_plugin(char const *name, void *symbol);

/*----------------------------------------------------------------------------*/
#endif
