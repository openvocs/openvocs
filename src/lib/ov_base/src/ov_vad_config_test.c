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

        @author         Michael J. Beer, DLR/GSOC

        ------------------------------------------------------------------------
*/

#include "ov_vad_config.c"

#include <ov_base/ov_config_keys.h>

#include <math.h>
#include <ov_test/ov_test.h>
#include <stdbool.h>

#define DEQUALS(x, y) (0.00001 > fabs(x - y))

/*----------------------------------------------------------------------------*/

static bool vad_config_equals(ov_vad_config const c1, ov_vad_config const c2) {

    if (!DEQUALS(c1.powerlevel_density_threshold_db,
                 c2.powerlevel_density_threshold_db)) {
        return false;
    }

    if (!DEQUALS(c1.zero_crossings_rate_threshold_hertz,
                 c2.zero_crossings_rate_threshold_hertz)) {
        return false;
    }

    return true;
}

/*----------------------------------------------------------------------------*/

static int ov_vad_config_from_json_test() {

    ov_vad_config cfg = ov_vad_config_from_json(0);

    testrun(DEQUALS(0, cfg.powerlevel_density_threshold_db));

    testrun(DEQUALS(0, cfg.zero_crossings_rate_threshold_hertz));

    ov_json_value *jval = ov_json_object();

    cfg = ov_vad_config_from_json(jval);

    testrun(DEQUALS(0, cfg.powerlevel_density_threshold_db));

    testrun(DEQUALS(0, cfg.zero_crossings_rate_threshold_hertz));

    ov_json_value *j_vad_cfg = ov_json_object();

    ov_json_object_set(jval, OV_KEY_VAD, j_vad_cfg);

    testrun(ov_json_object_set(j_vad_cfg, OV_KEY_POWERLEVEL_DENSITY_DB,
                               ov_json_number(2)));

    cfg = ov_vad_config_from_json(jval);

    testrun(DEQUALS(cfg.powerlevel_density_threshold_db, 2));

    testrun(DEQUALS(0, cfg.zero_crossings_rate_threshold_hertz));

    testrun(ov_json_object_set(j_vad_cfg, OV_KEY_ZERO_CROSSINGS_RATE_HERTZ,
                               ov_json_number(17)));

    cfg = ov_vad_config_from_json(jval);

    testrun(DEQUALS(2, cfg.powerlevel_density_threshold_db));
    testrun(DEQUALS(17, cfg.zero_crossings_rate_threshold_hertz));

    jval = ov_json_value_free(jval);

    testrun(0 == jval);

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

static int ov_vad_config_to_json_test() {

    ov_json_value *jval = ov_json_object();

    ov_vad_config vcfg = ov_vad_config_from_json(jval);

    ov_vad_config cfg = {
        .powerlevel_density_threshold_db = 0,
        .zero_crossings_rate_threshold_hertz = 0,
    };

    jval = ov_json_value_free(jval);

    testrun(vad_config_equals(cfg, vcfg));

    cfg = (ov_vad_config){
        .powerlevel_density_threshold_db = 51,
        .zero_crossings_rate_threshold_hertz = 0.021,
    };

    jval = ov_vad_config_to_json(cfg, jval);
    testrun(0 != jval);

    vcfg = ov_vad_config_from_json(jval);

    jval = ov_json_value_free(jval);

    testrun(vad_config_equals(cfg, vcfg));

    cfg = (ov_vad_config){
        .powerlevel_density_threshold_db = 67,
        .zero_crossings_rate_threshold_hertz = 0.04,
    };

    jval = ov_json_object();
    testrun(0 != jval);

    testrun(jval == ov_vad_config_to_json(cfg, jval));

    vcfg = ov_vad_config_from_json(jval);

    jval = ov_json_value_free(jval);

    testrun(vad_config_equals(cfg, vcfg));

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

OV_TEST_RUN("ov_vad_config", ov_vad_config_from_json_test,
            ov_vad_config_to_json_test);
