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
        @file           ov_stream_parameters.c
        @author         Michael J. Beer

        @date           2020-02-07

        ------------------------------------------------------------------------
*/

#include "ov_stream_parameters.h"
#include <ov_base/ov_config_keys.h>
#include <ov_base/ov_json.h>
#include <ov_base/ov_utils.h>

/*----------------------------------------------------------------------------*/

bool ov_stream_parameters_to_json(ov_stream_parameters const params,
                                  ov_json_value *target) {

    ov_json_value *spars = 0;

    if (!ov_ptr_valid(target, "No JSON to add stream parameters to")) {
        goto error;
    }

    spars = ov_json_object();

    OV_ASSERT(0 != spars);

    ov_json_value *copy = 0;

    if ((0 != params.codec_parameters) &&
        (!ov_json_value_copy((void **)&copy, params.codec_parameters))) {

        copy = 0;
    }

    if (0 != copy) {

        ov_json_object_set(spars, OV_KEY_CODEC, copy);
    }

    copy = 0;

    ov_json_object_set(target, OV_KEY_STREAM_PARAMETERS, spars);

    return true;

error:

    if (0 != spars) {
        spars = spars->free(spars);
    }

    OV_ASSERT(0 == spars);

    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_stream_parameters_from_json(ov_json_value const *jv,
                                    ov_stream_parameters *target_params) {

    if ((0 == jv) || (0 == target_params)) {
        goto error;
    }

    ov_json_value const *sparam = ov_json_get(jv, "/" OV_KEY_STREAM_PARAMETERS);

    ov_json_value const *codec_params = ov_json_get(sparam, "/" OV_KEY_CODEC);

    if (0 == codec_params) {
        ov_log_error(OV_KEY_CODEC " not found in json");
        goto error;
    }

    // Here it's crinching...
    target_params->codec_parameters = (ov_json_value *)codec_params;

    return true;

error:

    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_stream_parameters_clear(ov_stream_parameters *params) {

    if (0 == params) {
        goto error;
    }

    params->codec_parameters = ov_json_value_free(params->codec_parameters);

    return 0 != params->codec_parameters;

error:

    return false;
}

/*----------------------------------------------------------------------------*/
