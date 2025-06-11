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
        @file           ov_recorder.c
        @author         Michael Beer

        ------------------------------------------------------------------------
*/
#include "ov_recorder_app.h"
#include "ov_recorder_config.h"
#include <ov_backend/ov_minion_app.h>
#include <ov_base/ov_config_keys.h>
#include <ov_base/ov_json_value.h>
#include <ov_base/ov_registered_cache.h>
#include <ov_base/ov_teardown.h>
#include <ov_base/ov_utils.h>
#include <ov_codec/ov_codec_factory.h>
#include <ov_format/ov_format_registry.h>
#include <ov_os/ov_os_event_loop.h>

/*----------------------------------------------------------------------------*/

#define APP_NAME "recorder"

const char *CONFIG_FILE = "/etc/openvocs/ov_recorder.json";

/*----------------------------------------------------------------------------*/

typedef struct {

    ov_event_loop_config loop;
    ov_recorder_config recorder;
    ov_registered_cache_sizes *caches;

} startup_configuration;

/*----------------------------------------------------------------------------*/

static ProcessResult get_startup_configuration_unsafe(
    int argc, char **argv, startup_configuration *cfg) {

    OV_ASSERT(0 != argv);
    OV_ASSERT(0 != cfg);

    ov_json_value *json_cfg = 0;

    ProcessResult result =
        ov_minion_app_process_cmdline(argc, argv, APP_NAME, &json_cfg);

    if (CONTINUE != result) {

        goto finish;
    }

    ov_minion_app_configuration minion_app_cfg = {0};

    if ((!ov_ptr_valid(json_cfg,
                       "Could not load recorder configuration from config "
                       "file")) ||
        (!ov_cond_valid(
            ov_minion_app_configuration_from_json(json_cfg, &minion_app_cfg),
            "Could not parse json config"))) {

        result = EXIT_FAIL;
        goto finish;
    }

    cfg->loop = (ov_event_loop_config){
        .max.sockets = 20,
        .max.timers = 6,

    };

    char const *repository_root_path_const = ov_json_string_get(
        ov_json_get(json_cfg, "/" OV_KEY_REPOSITORY_ROOT_PATH));

    char *repository_root_path = 0;

    if (0 != repository_root_path_const) {
        repository_root_path = strdup(repository_root_path_const);
    }

    ov_vad_config vad_params = {0};

    ov_json_value const *vad_config = ov_json_get(json_cfg, "/" OV_KEY_VAD);

    uint64_t silence_cutoff_interval_secs = 0;
    ov_json_value const *silence_cutoff_interval_secs_json = 0;

    if (0 != vad_config) {

        fprintf(stderr, "VAD Config found\n");

        silence_cutoff_interval_secs_json =
            ov_json_get(vad_config, "/silence_cutoff_interval_secs");
        vad_params = ov_vad_config_from_json(json_cfg);
    }

    if (ov_json_is_number(silence_cutoff_interval_secs_json)) {

        silence_cutoff_interval_secs =
            ov_json_number_get(silence_cutoff_interval_secs_json);
    }

    fprintf(stderr,
            "VAD parameters: x-cross: %f    power: %f   silence cutoff "
            "interval: %" PRIu64 "\n",
            vad_params.zero_crossings_rate_threshold_hertz,
            vad_params.powerlevel_density_threshold_db,
            silence_cutoff_interval_secs);

    char const *codec_config = "{\"" OV_KEY_CODEC "\": \"opus\"}";
    // char const *codec_config = "{\"" OV_KEY_CODEC "\": \"pcm16s\", \""
    // OV_KEY_ENDIANNESS "\":\"" OV_KEY_LITTLE_ENDIAN "\"}";

    cfg->recorder = (ov_recorder_config){

        .reconnect_interval_secs = minion_app_cfg.reconnect_interval_secs,
        .liege_socket = minion_app_cfg.liege,

        .num_threads = 4,

        .defaults =
            {

                // .file_format = strdup("wav"),
                .file_format = strdup("ogg/opus"),
                .codec = ov_json_value_from_string(
                    codec_config, strlen(codec_config)),
                .vad = vad_params,
                .silence_cutoff_interval_secs = silence_cutoff_interval_secs,

            },

        .repository_root_path = repository_root_path,

        .original_config = json_cfg,

        .lock_timeout_usecs = minion_app_cfg.lock_timeout_usecs,

    };

    json_cfg = 0;

    OV_ASSERT(0 != cfg->recorder.defaults.codec);

finish:

    json_cfg = ov_json_value_free(json_cfg);

    OV_ASSERT(0 == json_cfg);

    return result;
}

/*----------------------------------------------------------------------------*/

static void enable_caching(ov_registered_cache_sizes *cfg) { UNUSED(cfg); }

/*----------------------------------------------------------------------------*/

static void clear_startup_configuration(startup_configuration cfg) {

    /* nothing to be done for cfg.loop */

    ov_recorder_config_clear(&cfg.recorder);
    cfg.caches = ov_registered_cache_sizes_free(cfg.caches);
}

/*---------------------------------------------------------------------------*/

int main(int argc, char **argv) {

    ov_log_init();

    int retval = EXIT_FAILURE;

    startup_configuration cfg = {0};

    ov_recorder_app *recorder_app = 0;
    ov_event_loop *loop = 0;

    ProcessResult result = get_startup_configuration_unsafe(argc, argv, &cfg);

    if (EXIT_FAIL == result) goto error;

    if (EXIT_OK == result) goto finish;

    enable_caching(cfg.caches);
    cfg.caches = ov_registered_cache_sizes_free(cfg.caches);
    OV_ASSERT(0 == cfg.caches);

    loop = ov_os_event_loop(cfg.loop);

    if (0 == loop) {

        ov_log_error("Could not create loop");
        goto error;
    }

    if (!ov_event_loop_setup_signals(loop)) {

        ov_log_error("Could not setup signal handling");
        goto error;
    }

    recorder_app = ov_recorder_app_create(APP_NAME, cfg.recorder, loop);

    if (0 == recorder_app) {

        ov_log_error("Could not create recorder app");
        goto error;
    }

    loop->run(loop, OV_RUN_MAX);

    recorder_app = ov_recorder_app_free(recorder_app);

finish:

    retval = EXIT_SUCCESS;

error:

    loop = ov_event_loop_free(loop);

    OV_ASSERT(0 == loop);

    clear_startup_configuration(cfg);

    ov_format_registry_clear(0);
    ov_codec_factory_free(0);

    ov_teardown();

    ov_log_close();

    return retval;
}
