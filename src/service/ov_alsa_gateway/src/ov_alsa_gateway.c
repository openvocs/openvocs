/***
        ------------------------------------------------------------------------

        Copyright (c) 2023 German Aerospace Center DLR e.V. (GSOC)

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
        @author         Michael Beer

        ------------------------------------------------------------------------
*/

#include "ov_alsa_audio_app.h"
#include "ov_base/ov_socket.h"
#include <ov_backend/ov_minion_app.h>
#include <ov_base/ov_mc_socket.h>
#include <ov_base/ov_string.h>
#include <ov_base/ov_teardown.h>
#include <ov_codec/ov_codec_factory.h>
#include <ov_os/ov_os_event_loop.h>

#define APP_NAME "alsa_gateway"
#define OPUS_CODEC "{\"codec\":\"opus\"}"

/*---------------------------------------------------------------------------*/

typedef struct {

    ov_json_value *original_config;

    ov_alsa_audio_app_config alsa;

} configuration;

configuration default_configuration = {

    .alsa =
        (ov_alsa_audio_app_config){
            .channels.output = {"plughw:0,0"},
            .channels.input = {"plughw:0,0"},
            .rtp_socket =
                {
                    .host = "0.0.0.0",
                    .port = 0,
                    .type = UDP,
                },
        },
};

/*----------------------------------------------------------------------------*/

static bool startup_configuration_clear(configuration *self) {

    if (0 != self) {
        ov_alsa_audio_app_config_clear(&self->alsa);
        self->original_config = ov_json_value_free(self->original_config);
        return true;
    } else {
        return false;
    }
}

/*----------------------------------------------------------------------------*/

static bool startup_configuration_from_json(configuration *cfg,
                                            ov_json_value const *jcfg) {

    if ((!ov_ptr_valid(cfg, "Config")) || (!ov_ptr_valid(jcfg, "JSON Value"))) {

        return false;

    } else {

        bool ok = false;

        ov_json_value_copy((void **)&cfg->original_config, jcfg);
        cfg->alsa = ov_alsa_audio_app_config_from_json(
            ov_json_get(jcfg, "/" OV_KEY_ALSA),
            default_configuration.alsa,
            &ok);

        return ok;
    }
}

/*----------------------------------------------------------------------------*/

static ProcessResult get_startup_configuration(int argc,
                                               char **argv,
                                               configuration *cfg) {

    ov_json_value *json_cfg = 0;

    ProcessResult result =
        ov_minion_app_process_cmdline(argc, argv, APP_NAME, &json_cfg);

    if ((CONTINUE == result) &&
        (!startup_configuration_from_json(cfg, json_cfg))) {
        result = EXIT_FAIL;
    }

    json_cfg = ov_json_value_free(json_cfg);

    return result;
}

/*----------------------------------------------------------------------------*/

static ov_event_loop *get_loop(ov_json_value *jcfg) {

    UNUSED(jcfg);

    ov_event_loop *loop = ov_os_event_loop(ov_event_loop_config_default());

    if (!ov_event_loop_setup_signals(loop)) {

        ov_log_error("Could not setup signal handling");
        loop = ov_event_loop_free(loop);
    }

    return loop;
}

/*----------------------------------------------------------------------------*/

static void enable_caching() { ov_alsa_audio_app_enable_caching(); };

/*----------------------------------------------------------------------------*/

static ProcessResult run_gateway(ov_event_loop *loop, configuration cfg) {

    ov_json_value *opus_json =
        ov_json_value_from_string(OPUS_CODEC, strlen(OPUS_CODEC));

    ov_alsa_audio_app *audio = ov_alsa_audio_app_create(loop, cfg.alsa);

    bool succeeded = ov_alsa_audio_app_start_playbacks(
                         audio,
                         (char const **)cfg.alsa.static_loops.output,
                         cfg.alsa.static_loops.output_ports,
                         OV_ALSA_MAX_DEVICES) &&
                     ov_alsa_audio_app_start_recordings(
                         audio,
                         (char const **)cfg.alsa.static_loops.input,
                         cfg.alsa.static_loops.input_ports,
                         OV_ALSA_MAX_DEVICES) &&
                     ov_alsa_audio_app_start_playback_thread(audio) &&
                     ov_event_loop_run(loop, OV_RUN_MAX);

    opus_json = ov_json_value_free(opus_json);

    audio = ov_alsa_audio_app_free(audio);

    if (succeeded) {
        return EXIT_OK;
    } else {
        return EXIT_FAIL;
    }
}

/*---------------------------------------------------------------------------*/

int main(int argc, char **argv) {

    ov_log_init();

    configuration cfg = {0};

    ProcessResult result = get_startup_configuration(argc, argv, &cfg);
    ov_event_loop *loop = get_loop(cfg.original_config);

    enable_caching();

    if (CONTINUE == result) {
        result = run_gateway(loop, cfg);
    }

    loop = ov_event_loop_free(loop);
    startup_configuration_clear(&cfg);

    ov_registered_cache_free_all();
    ov_codec_factory_free(0);

    ov_teardown();

    ov_log_close();

    return ov_minion_app_process_result_to_retval(result);
}

/*----------------------------------------------------------------------------*/
