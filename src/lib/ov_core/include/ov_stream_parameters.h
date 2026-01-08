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
        @file           ov_stream_parameters.h
        @author         Michael J. Beer

        @date           2020-02-07

        ------------------------------------------------------------------------
*/
#ifndef OV_STREAM_PARAMETERS_H
#define OV_STREAM_PARAMETERS_H

#include <ov_base/ov_json_value.h>

/*----------------------------------------------------------------------------*/

typedef struct {

    ov_json_value *codec_parameters;

} ov_stream_parameters;

/*----------------------------------------------------------------------------*/

bool ov_stream_parameters_to_json(ov_stream_parameters const params,
                                  ov_json_value *target);

/*----------------------------------------------------------------------------*/

bool ov_stream_parameters_from_json(ov_json_value const *jv,
                                    ov_stream_parameters *target_params);

/*----------------------------------------------------------------------------*/

bool ov_stream_parameters_clear(ov_stream_parameters *params);

/*----------------------------------------------------------------------------*/
#endif
