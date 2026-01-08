/***
        ------------------------------------------------------------------------

        Copyright (c) 2022 German Aerospace Center DLR e.V. (GSOC)

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
        @copyright (c) 2022 German Aerospace Center DLR e.V. (GSOC)

        ------------------------------------------------------------------------
*/
#ifndef OV_VAD_CONFIG_H
#define OV_VAD_CONFIG_H

#include "ov_json_value.h"

/*----------------------------------------------------------------------------*/

typedef struct {

    double zero_crossings_rate_threshold_hertz;
    double powerlevel_density_threshold_db;

} ov_vad_config;

/*----------------------------------------------------------------------------*/

typedef struct {

    double zero_crossings_per_sample;
    double powerlevel_density_per_sample;

} ov_vad_parameters;

/*----------------------------------------------------------------------------*/

ov_vad_config ov_vad_config_from_json(ov_json_value const *json);

/*----------------------------------------------------------------------------*/

ov_json_value *ov_vad_config_to_json(ov_vad_config vad_cfg,
                                     ov_json_value *target);

/*----------------------------------------------------------------------------*/
#endif
