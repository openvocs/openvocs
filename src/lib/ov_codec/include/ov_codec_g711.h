/***

Copyright   2018        German Aerospace Center DLR e.V.,
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

     \file               ov_codec_g711.h
     \author             Michael J. Beer, DLR/GSOC <michael.beer@dlr.de>
     \date               2018-06-05

     \ingroup            Implements G711 A-law and u-law

 **/

#ifndef ov_codec_g711_h
#define ov_codec_g711_h
#include "ov_codec_factory.h"

/******************************************************************************
 *  FUNCTIONS
 ******************************************************************************/

const char *ov_codec_g711_id();

ov_codec_generator ov_codec_g711_install(ov_codec_factory *factory);

#endif /* ov_codec_raw_h */
