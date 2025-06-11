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

        @author         Michael J. Beer, DLR/GSOC

        ------------------------------------------------------------------------
*/

#include "ov_recorder_config.h"

/*----------------------------------------------------------------------------*/

bool ov_recorder_config_clear(ov_recorder_config *cfg) {

    if (0 == cfg) {
        return false;
    }

    cfg->defaults.file_format = ov_free(cfg->defaults.file_format);

    cfg->defaults.codec = ov_json_value_free(cfg->defaults.codec);

    cfg->repository_root_path = ov_free(cfg->repository_root_path);

    cfg->original_config = ov_json_value_free(cfg->original_config);

    memset(cfg, 0, sizeof(*cfg));

    return true;
}

/*----------------------------------------------------------------------------*/
