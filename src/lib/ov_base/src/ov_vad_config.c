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

#include "../include/ov_vad_config.h"

#include "../include/ov_config_keys.h"
#include "../include/ov_constants.h"
#include "../include/ov_json.h"

/*----------------------------------------------------------------------------*/

ov_vad_config ov_vad_config_from_json(ov_json_value const *json) {

  ov_vad_config cfg = {
      .powerlevel_density_threshold_db = 0,
      .zero_crossings_rate_threshold_hertz = 0,
  };

  json = ov_json_get(json, "/" OV_KEY_VAD);

  if (0 == json) {
    goto error;
  }

  ov_json_value const *xcross =
      ov_json_get(json, "/" OV_KEY_ZERO_CROSSINGS_RATE_HERTZ);

  if (!ov_json_is_number(xcross)) {
    ov_log_error("/" OV_KEY_ZERO_CROSSINGS_RATE_HERTZ
                 " in config is not a number");
  } else {
    cfg.zero_crossings_rate_threshold_hertz = ov_json_number_get(xcross);
  }

  ov_json_value const *power_density =
      ov_json_get(json, "/" OV_KEY_POWERLEVEL_DENSITY_DB);

  if (!ov_json_is_number(power_density)) {
    ov_log_error("/" OV_KEY_POWERLEVEL_DENSITY_DB " in config is not a number");
  } else {
    cfg.powerlevel_density_threshold_db = ov_json_number_get(power_density);
  }

error:

  return cfg;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_vad_config_to_json(ov_vad_config vad_cfg,
                                     ov_json_value *target) {

  ov_json_value *jval = target;

  if (0 == jval) {
    jval = ov_json_object();
  }

  double zcr = vad_cfg.zero_crossings_rate_threshold_hertz;
  double pwr = vad_cfg.powerlevel_density_threshold_db;

  if (0 == zcr) {
    zcr = OV_DEFAULT_ZERO_CROSSINGS_RATE_THRESHOLD_HZ;
  }

  if (0 == pwr) {
    pwr = OV_DEFAULT_POWERLEVEL_DENSITY_THRESHOLD_DB;
  }

  ov_json_value *vad = ov_json_object();
  ov_json_object_set(jval, OV_KEY_VAD, vad);

  ov_json_object_set(vad, OV_KEY_ZERO_CROSSINGS_RATE_HERTZ,
                     ov_json_number(zcr));

  ov_json_object_set(vad, OV_KEY_POWERLEVEL_DENSITY_DB, ov_json_number(pwr));

  return jval;
}

/*----------------------------------------------------------------------------*/
