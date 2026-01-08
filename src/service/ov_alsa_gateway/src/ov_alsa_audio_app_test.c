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

        @author         Michael J. Beer, DLR/GSOC

        ------------------------------------------------------------------------
*/

#include "ov_alsa_audio_app.c"
#include <ov_test/ov_test.h>

/*----------------------------------------------------------------------------*/

static ov_json_value *json_from_string(char const *str) {

    if (0 != str) {
        return ov_json_value_from_string(str, strlen(str));
    } else {
        return 0;
    }
}

/*----------------------------------------------------------------------------*/

int ov_alsa_audio_app_config_from_json_test() {

    ov_alsa_audio_app_config default_cfg = {0};

    bool ok = false;

    ov_alsa_audio_app_config_from_json(0, default_cfg, &ok);

    testrun(ok);

    ov_json_value *jval = json_from_string("{"
                                           " \"channels\": {"
                                           "     \"input\": ["
                                           "         \"d1\""
                                           "     ],"
                                           "     \"output\": ["
                                           "         \"d2\""
                                           "     ]"
                                           " }"
                                           "}");

    testrun(0 != jval);

    ov_alsa_audio_app_config cfg =
        ov_alsa_audio_app_config_from_json(jval, default_cfg, &ok);

    testrun(ok);
    testrun(ov_string_equal(cfg.channels.input[0], "d1"));
    testrun(ov_string_equal(cfg.static_loops.input[0], 0));
    testrun(cfg.static_loops.input_ports[0] == 0);
    testrun(ov_string_equal(cfg.channels.output[0], "d2"));
    testrun(ov_string_equal(cfg.static_loops.output[0], 0));
    testrun(cfg.static_loops.output_ports[0] == 0);

    ov_alsa_audio_app_config_clear(&cfg);

    jval = ov_json_value_free(jval);

    jval = json_from_string("{"
                            " \"channels\": {"
                            "     \"input\": ["
                            "         {\"" OV_KEY_DEVICE
                            "\":\"din\", \"" OV_KEY_LOOP "\":\"1.11.111.2\"},"
                            "     ],"
                            "     \"output\": ["
                            "         {\"" OV_KEY_DEVICE
                            "\":\"dout\", \"" OV_KEY_LOOP "\":\"1.11.111.3\"},"
                            "     ]"
                            " }"
                            "}");

    testrun(0 != jval);

    cfg = ov_alsa_audio_app_config_from_json(jval, default_cfg, &ok);

    testrun(ok);
    testrun(ov_string_equal(cfg.channels.input[0], "din"));
    testrun(ov_string_equal(cfg.static_loops.input[0], "1.11.111.2"));
    testrun(0 == cfg.static_loops.input_ports[0]);
    testrun(cfg.channel_volumes.input[0] == 0.0);

    testrun(ov_string_equal(cfg.channels.output[0], "dout"));
    testrun(ov_string_equal(cfg.static_loops.output[0], "1.11.111.3"));
    testrun(cfg.static_loops.output_ports[0] == 0);

    ov_alsa_audio_app_config_clear(&cfg);

    jval = ov_json_value_free(jval);

    jval = json_from_string(
        "{"
        " \"channels\": {"
        "     \"input\": ["
        "         {\"" OV_KEY_DEVICE "\":\"din\", \"" OV_KEY_LOOP
        "\":\"1.11.111.2\", \"" OV_KEY_PORT "\": 8512}"
        "     ],"
        "     \"output\": ["
        "         {\"" OV_KEY_DEVICE "\":\"dout\", \"" OV_KEY_LOOP
        "\":\"1.11.111.3\", \"" OV_KEY_PORT "\": 777}"
        "     ]"
        " }"
        "}");

    testrun(0 != jval);

    cfg = ov_alsa_audio_app_config_from_json(jval, default_cfg, &ok);

    testrun(ok);
    testrun(ov_string_equal(cfg.channels.input[0], "din"));
    testrun(ov_string_equal(cfg.static_loops.input[0], "1.11.111.2"));
    testrun(8512 == cfg.static_loops.input_ports[0]);
    testrun(cfg.channel_volumes.input[0] == 0.0);

    testrun(ov_string_equal(cfg.channels.output[0], "dout"));
    testrun(ov_string_equal(cfg.static_loops.output[0], "1.11.111.3"));
    testrun(777 == cfg.static_loops.output_ports[0]);

    ov_alsa_audio_app_config_clear(&cfg);

    jval = ov_json_value_free(jval);

    jval = json_from_string(
        "{"
        " \"channels\": {"
        "     \"input\": ["
        "         {\"" OV_KEY_DEVICE "\":\"din\", \"" OV_KEY_LOOP
        "\":\"1.11.111.2\", \"" OV_KEY_PORT "\": 8512}"
        "     ],"
        "     \"output\": ["
        "         {\"" OV_KEY_DEVICE "\":\"dout\", \"" OV_KEY_LOOP
        "\":\"1.11.111.3\", \"" OV_KEY_PORT "\": 777}"
        "     ]"
        " },"
        " \"debug\" : {"
        "  \"rtp_logging\": \"/a/b/ce\""
        " }"
        "}");

    testrun(0 != jval);

    cfg = ov_alsa_audio_app_config_from_json(jval, default_cfg, &ok);

    testrun(ok);
    testrun(ov_string_equal(cfg.channels.input[0], "din"));
    testrun(ov_string_equal(cfg.static_loops.input[0], "1.11.111.2"));
    testrun(8512 == cfg.static_loops.input_ports[0]);
    testrun(cfg.channel_volumes.input[0] == 0.0);

    testrun(ov_string_equal(cfg.channels.output[0], "dout"));
    testrun(ov_string_equal(cfg.static_loops.output[0], "1.11.111.3"));
    testrun(777 == cfg.static_loops.output_ports[0]);

    testrun(ov_string_equal(cfg.debug.rtp_logging, "/a/b/ce"));

    ov_alsa_audio_app_config_clear(&cfg);

    jval = ov_json_value_free(jval);

#if OV_ALSA_MAX_DEVICE > 1

    jval = json_from_string("{"
                            " \"channels\": {"
                            "     \"input\": ["
                            "         {\"" OV_KEY_DEVICE
                            "\":\"din\", \"" OV_KEY_LOOP "\":\"1.11.111.2\"},"
                            "         \"d1\""
                            "     ],"
                            "     \"output\": ["
                            "         {\"" OV_KEY_DEVICE
                            "\":\"dout\", \"" OV_KEY_LOOP "\":\"1.11.111.3\"},"
                            "         \"d2\""
                            "     ]"
                            " }"
                            "}");

    testrun(0 != jval);

    cfg = ov_alsa_audio_app_config_from_json(jval, default_cfg, &ok);

    testrun(ok);
    testrun(ov_string_equal(cfg.channels.input[0], "din"));
    testrun(ov_string_equal(cfg.static_loops.input[0], "1.11.111.2"));
    testrun(0 == cfg.static_loops.input_ports[0]);
    testrun(cfg.channel_volumes.input[0] == 0.0);

    testrun(ov_string_equal(cfg.channels.output[0], "dout"));
    testrun(ov_string_equal(cfg.static_loops.output[0], "1.11.111.3"));
    testrun(cfg.static_loops.output_ports[0] == 0);

    testrun(ov_string_equal(cfg.channels.input[1], "d1"));
    testrun(0 == cfg.static_loops.input[1]);
    testrun(0 == cfg.static_loops.input_ports[1]);
    testrun(0 == cfg.channel_volumes.input[1]);
    testrun(cfg.channel_volumes.input[1] == 0.0);

    testrun(ov_string_equal(cfg.channels.output[1], "d2"));
    testrun(0 == cfg.static_loops.output[1]);
    testrun(cfg.static_loops.output_ports[1] == 0);

    ov_alsa_audio_app_config_clear(&cfg);

    jval = ov_json_value_free(jval);

    jval = json_from_string(
        "{"
        " \"channels\": {"
        "     \"input\": ["
        "         \"din\","
        "         {\"" OV_KEY_DEVICE "\":\"d1\", \"" OV_KEY_VOLUME
        "\":0.2, \"" OV_KEY_LOOP "\":\"2.11.111.2\"}"
        "     ],"
        "     \"output\": ["
        "         \"dout\","
        "         {\"" OV_KEY_DEVICE "\":\"d2\", \"" OV_KEY_VOLUME
        "\": 0.1, \"" OV_KEY_LOOP "\":\"2.11.111.3\"}"
        "     ]"
        " }"
        "}");

    testrun(0 != jval);

    cfg = ov_alsa_audio_app_config_from_json(jval, default_cfg, &ok);

    testrun(ok);

    testrun(ov_string_equal(cfg.channels.input[0], "din"));
    testrun(0 == cfg.static_loops.input[0]);
    testrun(0 == cfg.static_loops.input_ports[0]);
    testrun(0 == cfg.channel_volumes.input[0]);
    testrun(cfg.channel_volumes.input[0] == 0.0);

    testrun(ov_string_equal(cfg.channels.input[1], "d1"));
    testrun(ov_string_equal(cfg.static_loops.input[1], "2.11.111.2"));
    testrun(0 == cfg.static_loops.input_ports[1]);
    testrun(0 == cfg.channel_volumes.input[1]);
    testrun(fabs(cfg.channel_volumes.input[1] - 0.2) < 0.01);

    testrun(ov_string_equal(cfg.channels.output[0], "dout"));
    testrun(0 == cfg.static_loops.output[0]);
    testrun(cfg.static_loops.output_ports[0] == 0);

    testrun(ov_string_equal(cfg.channels.output[1], "d2"));
    testrun(ov_string_equal(cfg.static_loops.output[1], "2.11.111.3"));
    testrun(cfg.static_loops.output_ports[1] == 0);
    testrun(fabs(cfg.channel_volumes.output[1] - 0.1) < 0.01);

    ov_alsa_audio_app_config_clear(&cfg);

    jval = ov_json_value_free(jval);

    jval = json_from_string(
        "{"
        " \"channels\": {"
        "     \"input\": ["
        "         \"din\","
        "         {\"" OV_KEY_DEVICE "\":\"d1\", \"" OV_KEY_VOLUME
        "\":0.2, \"" OV_KEY_LOOP "\":\"2.11.111.2\", \"" OV_KEY_PORT "\": 312}"
        "     ],"
        "     \"output\": ["
        "         \"dout\","
        "         {\"" OV_KEY_DEVICE "\":\"d2\", \"" OV_KEY_VOLUME
        "\": 0.1, \"" OV_KEY_LOOP "\":\"2.11.111.3\", \"" OV_KEY_PORT "\": 188}"
        "     ]"
        " }"
        "}");

    testrun(0 != jval);

    cfg = ov_alsa_audio_app_config_from_json(jval, default_cfg, &ok);

    testrun(ok);

    testrun(ov_string_equal(cfg.channels.input[0], "din"));
    testrun(0 == cfg.static_loops.input[0]);
    testrun(0 == cfg.static_loops.input_ports[0]);
    testrun(0 == cfg.channel_volumes.input[0]);
    testrun(cfg.channel_volumes.input[0] == 0.0);

    testrun(ov_string_equal(cfg.channels.input[1], "d1"));
    testrun(ov_string_equal(cfg.static_loops.input[1], "2.11.111.2"));
    testrun(312 == cfg.static_loops.input_ports[1]);
    testrun(0 == cfg.channel_volumes.input[1]);
    testrun(fabs(cfg.channel_volumes.input[1] - 0.2) < 0.01);

    testrun(ov_string_equal(cfg.channels.output[0], "dout"));
    testrun(0 == cfg.static_loops.output[0]);
    testrun(cfg.static_loops.output_ports[0] == 0);

    testrun(ov_string_equal(cfg.channels.output[1], "d2"));
    testrun(ov_string_equal(cfg.static_loops.output[1], "2.11.111.3"));
    testrun(188 == cfg.static_loops.output_ports[1]);
    testrun(fabs(cfg.channel_volumes.output[1] - 0.1) < 0.01);

    ov_alsa_audio_app_config_clear(&cfg);

    jval = ov_json_value_free(jval);

#endif

    return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

OV_TEST_RUN("ov_alsa_audio_app", ov_alsa_audio_app_config_from_json_test);
