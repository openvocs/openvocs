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

        Interface a plugin has to implement

        ------------------------------------------------------------------------
*/
#ifndef OV_PLUGIN_H
#define OV_PLUGIN_H

#include "ov_hashtable.h"
#include <stdbool.h>

/*----------------------------------------------------------------------------*/

#define OV_PLUGIN_LOAD "ov_plugin_load"

/**
 * This function is called when the plugins are initialized.
 * The load function receives a dictionary of symbols.
 * These symbols are void pointers and need to be cast into the correct
 * pointer types.
 * If you want to provide a function `void register_codec(ov_codec *codec)`,
 * use
 * `ov_plugin_system_register_symbol('register_codec_1', register_codec);`
 * `register_codec_1` will then be available within the `symbols` dictionary
 * when this function is called.
 * You should always put a version number on each symbol defined and alter
 * the version number whenever its interface changes!
 * @return String to identify the plugin later on. If more than one plugin use
 * the same id string, only one of the plugins will be installed arbitrarily.
 */
char const *ov_plugin_load(ov_hashtable const *symbols);

#define OV_PLUGIN_UNLOAD "ov_plugin_unload"

/**
 * Called before process terminates.
 */
void ov_plugin_unload();

/*----------------------------------------------------------------------------*/
#endif
