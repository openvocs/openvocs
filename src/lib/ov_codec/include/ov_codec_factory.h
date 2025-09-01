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

     \file               ov_codec_factory.h
     \author             Michael J. Beer, DLR/GSOC <michael.beer@dlr.de>
     \date               2017-12-06

     \ingroup            empty

     Voice codec factory.
     Codecs might be registered for certain types.
     If a new codec instance is required for a particular codec type,
     it can be requested.

 **/

#ifndef ov_codec_factory_h
#define ov_codec_factory_h
#include "ov_codec.h"
#include <ov_base/ov_result.h>

/******************************************************************************
 *  BASIC typedefs
 ******************************************************************************/

/**
 * Forward declaration - actual declaration does not matter for users
 */
struct ov_codec_factory_struct;

typedef struct ov_codec_factory_struct ov_codec_factory;

typedef ov_codec *(*ov_codec_generator)(uint32_t ssid,
                                        const ov_json_value *parameters);

/******************************************************************************
 *  Create
 ******************************************************************************/

/**
 * Create new empty codec factory
 */
ov_codec_factory *ov_codec_factory_create();

/**
 * Get default factory with pre - setup standard codecs (raw, opus)
 */
ov_codec_factory *ov_codec_factory_create_standard();

/******************************************************************************
 *  Access & manipulation
 ******************************************************************************/

/**
 * @return 0 if termination successful, pointer to factory otherwise.
 * @param factory The factory to free or 0 if the global factory should be freed
 */
ov_codec_factory *ov_codec_factory_free(ov_codec_factory *factory);

/**
 * Install a new codec type into the factory.
 * new_codec_for() should return a new ov_codec struct suitable to
 * encode/decode a stream with a given ssid from/to codec type.
 * @param factory Factory to install codec to or 0 if install to global factory
 */
ov_codec_generator ov_codec_factory_install_codec(
    ov_codec_factory *factory,
    const char *codec_name,
    ov_codec_generator generate_codec);

/**
 * Get a new codec to decode a particular stream.
 * If the json contains a different sample rate than the OV_DEFAULT_SAMPLERATE
 * (i.e. the internally used one), the data is resampled automatically before
 * encoded.
 * @param factory factory to get codec from or 0 if global factory should be
 * used
 * @return pointer to new codec or 0 in case of error.
 */
ov_codec *ov_codec_factory_get_codec(ov_codec_factory *factory,
                                     const char *codec_name,
                                     uint32_t ssid,
                                     const ov_json_value *parameters);

/**
 * Same as get_codec, but gets all information out of json
 */
ov_codec *ov_codec_factory_get_codec_from_json(ov_codec_factory *factory,
                                               const ov_json_value *json,
                                               uint32_t ssid);

/*****************************************************************************
                     Support for codecs with plugin system
 ****************************************************************************/

/**
 * Exports "ov_codec_factory_install_codec_1" to be used in plugins that
 * want to register codecs.
 * This symbol effectively gets them the
 * ov_codec_factory_install_codec function.
 */
bool ov_codec_factory_export_symbols_for_plugins();

/*****************************************************************************
                                 PLUGIN SUPPORT
                        Prefer the ov plugin system way!
 ****************************************************************************/

#define OV_CODEC_CREATE_FUNCNAME "openvocs_plugin_codec_create"
#define OV_CODEC_ID_FUNCNAME "openvocs_plugin_codec_id"

/**
 * Load and install a codec from a shared object file
 * The shared object needs to provide an
 * `ov_codec_generator openvocs_plugin_codec_create`
 * as well as
 * `char const *openvocs_plugin_codec_id()`
 * The codec will then be installed under the id `ov_codec_id()`
 * in the given factory, or - if factory is 0, in the default factory
 *
 */
bool ov_codec_factory_install_codec_from_so(ov_codec_factory *self,
                                            char const *so_path,
                                            ov_result *res);

/**
 * Load and install all codecs found in shared objects within a certain
 * directory.
 * It iterates over all files within `path`, loads every SO file and
 * looks for an exported function
 * `ov_codec_generator ov_codec_install(ov_codec_factory *factory)`
 *
 * If found, the function is called and hence the codec installed.
 *
 * @return number of codecs installed or negative in case of error
 */
ssize_t ov_codec_factory_install_codec_from_so_dir(ov_codec_factory *self,
                                                   char const *so_path,
                                                   ov_result *res);

/*----------------------------------------------------------------------------*/

#endif /* ov_codec_factory_h */
