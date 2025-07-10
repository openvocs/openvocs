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

        @author         Michael J. Beer

        ------------------------------------------------------------------------
*/

#include <sys/socket.h>

#include <ov_base/ov_rtp_frame_message.h>
#include <ov_base/ov_utils.h>

#include "../include/ov_minion_app.h"
#include "../include/ov_app.h"

#include <ov_base/ov_config.h>
#include <ov_base/ov_config_log.h>
#include <ov_base/ov_constants.h>
#include <ov_base/ov_error_codes.h>
#include <ov_base/ov_registered_cache.h>
#include <ov_base/ov_socket.h>
#include <ov_base/ov_version.h>
#include <ov_base/ov_config_keys.h>

#include <ov_core/ov_event_api.h>

/*----------------------------------------------------------------------------*/

ov_socket_configuration OV_MINION_APP_DEFAULT_LIEGE_SOCKET = {
    .host = "127.0.0.1",
    .port = 10001,
    .type = TCP,
};

/*----------------------------------------------------------------------------*/

static ov_json_value const *get_log_section_from_json(
    ov_json_value const *jval) {

    return ov_json_get(jval, "/" OV_KEY_LOGGING);
}

/*----------------------------------------------------------------------------*/

bool ov_minion_app_configure_log(ov_json_value const *jval) {

    if (0 == jval) {
        goto error;
    }

    ov_json_value const *jlog = get_log_section_from_json(jval);

    return ov_config_log_from_json(jlog);

error:

    return false;
}

/*----------------------------------------------------------------------------*/

static uint32_t get_u32_from_json(ov_json_value const *jval, char const *key) {

    double dval = ov_json_number_get(ov_json_get(jval, key));
    uint32_t val = (uint32_t)dval;

    if ((0 > dval) || (UINT32_MAX < dval)) {
        ov_log_error("%s exceeds max value of %" PRIu32, key, UINT32_MAX);
        return 0;
    } else {
        return val;
    }
}

/*----------------------------------------------------------------------------*/

static uint64_t get_u64_from_json(ov_json_value const *jval, char const *key) {

    double dval = ov_json_number_get(ov_json_get(jval, key));
    uint64_t val = (uint64_t)dval;

    if ((0 > dval) || ((double)UINT64_MAX < dval)) {
        ov_log_error("%s exceeds max value of %" PRIu64, key, UINT64_MAX);
        return 0;
    } else {
        return val;
    }
}

/*----------------------------------------------------------------------------*/

bool ov_minion_app_configuration_from_json(
    ov_json_value const *jval, ov_minion_app_configuration *config) {

    if (0 == config) {
        ov_log_error("Invalid output struct given (0 pointer)");
        goto error;
    }

    if (0 == jval) {
        ov_log_error("Invalid json given (0 pointer)");
        goto error;
    }

    ov_json_value const *liege_socket_json =
        ov_json_get(jval, "/" OV_KEY_LIEGE);

    ov_socket_configuration liege_cfg = ov_socket_configuration_from_json(
        liege_socket_json, OV_MINION_APP_DEFAULT_LIEGE_SOCKET);

    if ((0 == liege_cfg.host[0]) || (0 == liege_cfg.port) ||
        (TCP != liege_cfg.type)) {

        ov_log_error("No / malformed liege server socket given in config");
        goto error;
    }

    ov_log_info("Using Liege %s:%" PRIu16, liege_cfg.host, liege_cfg.port);

    uint32_t reconnect_timeout_secs =
        get_u32_from_json(jval, "/" OV_KEY_RECONNECT_INTERVAL_SECS);

    config->liege = liege_cfg;
    config->reconnect_interval_secs = OV_OR_DEFAULT(
        reconnect_timeout_secs, OV_DEFAULT_RECONNECT_INTERVAL_SECS);
    config->lock_timeout_usecs = OV_OR_DEFAULT(
        get_u64_from_json(jval, "/" OV_KEY_LOCK_TIMEOUT_MSECS) * 1000,
        OV_DEFAULT_LOCK_TIMEOUT_USECS);

    config->startup_config_json = jval;

    return true;

error:

    return false;
}

/******************************************************************************
 *                       ov_minion_app_process_cmdline
 ******************************************************************************/

static ov_json_value const *get_app_section(ov_json_value const *jval,
                                            char const *name) {

    char key_name[OV_APP_NAME_MAX + 1];

    int written_bytes = snprintf(key_name, OV_APP_NAME_MAX + 1, "/%s", name);

    if (OV_APP_NAME_MAX <= written_bytes) {

        ov_log_error("name too long");
        goto error;
    }

    return ov_json_get(jval, key_name);

error:

    return 0;
}

/*----------------------------------------------------------------------------*/

ProcessResult ov_minion_app_process_cmdline_optargs(int argc, char **argv,
                                                    char const *app_name,
                                                    ov_json_value **loaded_json,
                                                    char const *optargs) {
    ov_json_value *jval = 0;

    if ((0 == argv) || (0 == app_name) || (0 == loaded_json)) {
        ov_log_error("Got 0 pointer");
        goto error;
    }

    *loaded_json = 0;

    ov_app_parameters app_params = {0};

    char const *err_msg =
        ov_app_parse_command_line_optargs(argc, argv, &app_params, optargs);

    if (0 != err_msg) {
        ov_log_error("Could not parse command line: %s", err_msg);
        return EXIT_FAIL;
    }

    if (app_params.version_request) {
        OV_VERSION_PRINT(stderr);
        return EXIT_OK;
    }

    ov_log_info("Using config file %s", app_params.config_file);

    jval = ov_json_read_file(app_params.config_file);

    if (0 == jval) {
        ov_log_error("Could not load %s", app_params.config_file);
        goto error;
    }

    ov_json_value const *app_cfg = get_app_section(jval, app_name);

    ov_minion_app_configure_log(app_cfg);

    ov_json_value_copy((void **)loaded_json, app_cfg);
    jval = ov_json_value_free(jval);

    OV_ASSERT(0 == jval);

    return CONTINUE;

error:

    OV_ASSERT(0 == jval);

    return EXIT_FAIL;
}

/*----------------------------------------------------------------------------*/

ProcessResult ov_minion_app_process_cmdline(int argc, char **argv,
                                            char const *app_name,
                                            ov_json_value **loaded_json) {
    return ov_minion_app_process_cmdline_optargs(argc, argv, app_name,
                                                 loaded_json, 0);
}

/*****************************************************************************
                     ov_minion_app_process_result_to_retval
 ****************************************************************************/

int ov_minion_app_process_result_to_retval(ProcessResult result) {
    switch (result) {
        case CONTINUE:
        case EXIT_OK:

            return EXIT_SUCCESS;

        case EXIT_FAIL:

            return EXIT_FAILURE;

        default:

            ov_log_error("Unexpected ProcessResult: %i", result);
            abort();
    };
}

/*----------------------------------------------------------------------------*/
