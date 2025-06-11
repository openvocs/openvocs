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

        ------------------------------------------------------------------------
*/
#ifndef OV_LOG_CONFIG_H
#define OV_LOG_CONFIG_H
/*----------------------------------------------------------------------------*/

#include "ov_json_value.h"
#include <stdbool.h>

/*----------------------------------------------------------------------------*/

/**
 * For the JSON format accepted, see
 */
ov_log_output ov_log_output_from_json(ov_json_value const *jval);

/*----------------------------------------------------------------------------*/

/**
 * Configures logger from JSON.
 *
 * Beware: Subsequent calls will overwrite what is specified, but the
 * logger will not be reset, meaning that if there are outputs for modules/funcs
 * already defined that are not being overwritten here, they will be
 * kept in place.
 *
 * Config looks like
 *
 * {
 *
 *    "systemd" : true,
 *    "file" : {
 *         "file" : "/tmp/log",
 *         "messages_per_file" : 10000,
 *         "num_files" : 4
 *    },
 *    "level" : warning,
 *
 *    "custom" : {
 *
 *        "ov_config_log.c" : {
 *
 *            "file" : "/tmp/ov_config_log.c",
 *            "systemd" : false,
 *            "level" : debug
 *
 *        },
 *
 *        "ov_ssid_translation_table.c" : {
 *
 *            "sytemd" : true,
 *            "level" : warning,
 *
 *            "functions" : {
 *
 *                "ov_ssid_translation_table_unlock" : {
 *
 *                    "systemd" true,
 *                    "level" : debug,
 *
 *                },
 *
 *                "ov_ssid_translation_table_get_empty" : {
 *
 *                    "systemd" true,
 *                    "level" : debug,
 *                    "file" : {
 *                        "file" : "/var/log/openvocs/tt_get_empty.log",
 *                        "messages_per_file" : 10000,
 *                        "num_files" : 4,
 *                    }
 *
 *                }
 *
 *            }
 *
 *        }
 *
 *    }
 *
 * }
 *
 * If logging should be conducted to stderr, set
 *
 * "file" : "stderr"
 *
 * If logging should be written to stdout, set
 *
 * "file" : "stdout"
 *
 */
bool ov_config_log_from_json(ov_json_value const *jval);

/*----------------------------------------------------------------------------*/
#endif
