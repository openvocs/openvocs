/***
        ------------------------------------------------------------------------

        Copyright (c) 2020 German Aerospace Center DLR e.V. (GSOC)

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
        @copyright (c) 2020 German Aerospace Center DLR e.V. (GSOC)

        Registry for format handlers.
        Basically, a database associating a format type (a string like 'wav')
        with a specific format handler.

        In general, there is a global registry and all functions operate on
        this global registry.
        In that case, just pass 0 as registry pointer whereever there is one
        required.

        This registry is currently *NOT* thread-safe, but:

        In 'normal' operations, it is only read and neither the registry
        nor the handlers are modified. 

        Typically, new handlers will be inserted into the registry at
        start-up, and the registry torn down only before process termination.

        As long as you use it like that, there should be no need to syncronize.

        If you want to use a local registry instead of the global one, create one
        using ov_format_registry_create and pass its pointer into the registry
        functions.

        ------------------------------------------------------------------------
*/
#ifndef OV_FORMAT_REGISTRY_H
#define OV_FORMAT_REGISTRY_H
/*----------------------------------------------------------------------------*/

#include "ov_format.h"

/*----------------------------------------------------------------------------*/

typedef struct ov_format_registry_struct ov_format_registry;

/**
 * Convert any format into a format of type 'format_type'
 * Do not fiddle around with the wrapped ov_format object any more!
 * File types can be nested, that is, it is possible to do a
 *
 * ov_format *my_file = ov_format_open("my.mkv", OV_READ);
 * // Purely artificial example ...
 * my_format = ov_format_as(
 *                 ov_format_as(my_file, "ogg", 0, registry),
 *                 "matroska", 0, registry);
 *
 * @param options format specific option pointer. If in doubt, use 0
 */
ov_format *ov_format_as(ov_format *f, char const *format_type, void *options,
                        ov_format_registry *registry);

/*----------------------------------------------------------------------------*/

/**
 * Register new format type
 * There are no invalid handlers - if a handler has not set some of its
 * pointers, the resp. functionality will be disabled with this handler. E.g. if
 * handler.next_chunk == 0, reading will not be possible with this handler.
 *
 * @param format_type must not be 0
 * @param handler see above
 * @param registry if 0, global registry will be used
 */
bool ov_format_registry_register_type(char const *format_type,
                                      ov_format_handler handler,
                                      ov_format_registry *registry);

/*----------------------------------------------------------------------------*/

/**
 * Removes a format type from registry.
 * removed_hanlder can be 0.
 * If removed_handler is not 0, e.g. points towards a ov_format_handler, the
 * removed handler will be copied into removed_handler.
 * @return true if registry does not contain format_type any longer.
 */
bool ov_format_registry_unregister_type(char const *format_type,
                                        ov_format_handler *removed_handler,
                                        ov_format_registry *registry);

/*----------------------------------------------------------------------------*/

/**
 * Tries to dispose of registry.
 * If registry is 0, the global registry will be cleared.
 * @return 0 on success, registry (or something != 0 if registry is 0) otherwise
 */
ov_format_registry *ov_format_registry_clear(ov_format_registry *registry);

/*----------------------------------------------------------------------------*/

ov_format_registry *ov_format_registry_create();

/*----------------------------------------------------------------------------*/

bool ov_format_registry_register_default(ov_format_registry *registry);

/*----------------------------------------------------------------------------*/
#endif
