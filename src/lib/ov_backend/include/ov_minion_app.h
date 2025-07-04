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

        ov_minion_app is an extension to ov_signaling_app, that
        sets up and maintains a reconnecting signaling connection towards a
        liege.

        Upon reconnect, an adequate register request is sent to the liege.

        Further, requests are handled:

        * 'shutdown' - stop event loop
        * 'cache_report'
        * 'reconfigure'
        * 'add_source' / 'remove_source'
        * 'add_destination' / 'remove_destination'
        * 'list_sources'
        * 'list_destinations'
        * 'set_source_volume'
        * 'reconfigure_log'

        Whether a minion should support sources / dests is configured
        by the config.enable.sources / dests .

        How many sources / dests should be supported is configured by the
        liege (see reconfigure request def).
        If a minion was not configured to supported sources/dests, the liege
        cannot force sources/ dests to be supported.

        TO REWORK:

        This module is flawed.
        It does not provide a proper interface as:

        ov_minion_app_notify_liege(ov_app *app, ov_json_value const *);

        and manage the liege fd internally.

        It should have a simple void (*closed)(ov_app *);

        callback instead of the complicated version below.

        However, this means that minion app won't be compatible with
        signaling app any more.

        ------------------------------------------------------------------------
*/
#ifndef OV_MINION_APP_H
#define OV_MINION_APP_H
/*----------------------------------------------------------------------------*/

#include <ov_base/ov_registered_cache.h>

#include <ov_base/ov_socket.h>
#include <ov_base/ov_rtcp.h>
#include <ov_base/ov_rtp_frame.h>

/*----------------------------------------------------------------------------*/

typedef struct {

    ov_socket_configuration liege;
    uint32_t reconnect_interval_secs;
    uint64_t lock_timeout_usecs;
    ov_json_value const *startup_config_json;
} ov_minion_app_configuration;

bool ov_minion_app_configuration_from_json(ov_json_value const *jval,
                                           ov_minion_app_configuration *config);

/*----------------------------------------------------------------------------*/

typedef enum {

    EXIT_OK = -1,
    CONTINUE = 0,
    EXIT_FAIL,

} ProcessResult;

/*----------------------------------------------------------------------------*/

ProcessResult ov_minion_app_process_cmdline_optargs(int argc,
                                            char **argv,
                                            char const *app_name,
                                            ov_json_value **loaded_json,
                                            char const *optargs);

/*----------------------------------------------------------------------------*/

ProcessResult ov_minion_app_process_cmdline(int argc,
                                            char **argv,
                                            char const *app_name,
                                            ov_json_value **loaded_json);

int ov_minion_app_process_result_to_retval(ProcessResult result);

#endif
