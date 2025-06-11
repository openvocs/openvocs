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

        @author Michael J. Beer, DLR/GSOC
        @copyright (c) 2021 German Aerospace Center DLR e.V. (GSOC)

        A format that does nothing more than encode / decode anything  written
        to / read from it with a specific codec and forward it to its lower
        layer format

        Use as:

        ov_format_codec_install(0);

        ov_format *lower_layer = ...;

        ov_format *coding_fmt = ov_format_as(lower_layer, "codec", &my_codec);

        ov_format_payload_write_chunk(coding_fmt, chunk);

        ...

        ------------------------------------------------------------------------
*/
#ifndef OV_FORMAT_CODEC_H
#define OV_FORMAT_CODEC_H
/*----------------------------------------------------------------------------*/

#include "ov_format_registry.h"

#define OV_FORMAT_CODEC_TYPE_STRING "codec"

bool ov_format_codec_install(ov_format_registry *registry);

/*----------------------------------------------------------------------------*/
#endif
