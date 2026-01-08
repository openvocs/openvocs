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
        @file           ov_mc_mixer_msg.c
        @author         Markus Töpfer

        @date           2022-11-11


        ------------------------------------------------------------------------
*/
#include "../include/ov_mc_mixer_msg.h"

ov_json_value *ov_mc_mixer_msg_register(const char *uuid, const char *type) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *par = NULL;

    if (!uuid || !type)
        goto error;

    out = ov_event_api_message_create(OV_KEY_REGISTER, NULL, 0);
    par = ov_event_api_set_parameter(out);
    if (!par)
        goto error;

    if (!ov_event_api_set_uuid(par, uuid))
        goto error;
    if (!ov_event_api_set_type(par, type))
        goto error;

    return out;

error:
    ov_json_value_free(val);
    ov_json_value_free(out);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_mc_mixer_msg_configure(ov_mc_mixer_core_config config) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *par = NULL;

    out = ov_event_api_message_create(OV_KEY_CONFIGURE, NULL, 0);
    par = ov_event_api_set_parameter(out);
    if (!par)
        goto error;

    val = ov_json_object();
    if (!ov_json_object_set(par, OV_KEY_VAD, val))
        goto error;

    ov_json_value *vad = val;

    val = ov_json_number(config.vad.zero_crossings_rate_threshold_hertz);
    if (!ov_json_object_set(vad, OV_KEY_ZERO_CROSSINGS_RATE_HERTZ, val))
        goto error;

    val = ov_json_number(config.vad.powerlevel_density_threshold_db);
    if (!ov_json_object_set(vad, OV_KEY_POWERLEVEL_DENSITY_DB, val))
        goto error;

    if (config.incoming_vad) {
        val = ov_json_true();
    } else {
        val = ov_json_false();
    }
    if (!ov_json_object_set(vad, OV_KEY_ENABLED, val))
        goto error;

    val = ov_json_number(config.samplerate_hz);
    if (!ov_json_object_set(par, OV_KEY_SAMPLE_RATE_HERTZ, val))
        goto error;

    val = ov_json_number(config.comfort_noise_max_amplitude);
    if (!ov_json_object_set(par, OV_KEY_COMFORT_NOISE, val))
        goto error;

    val = ov_json_number(config.max_num_frames_to_mix);
    if (!ov_json_object_set(par, OV_KEY_MAX_NUM_FRAMES, val))
        goto error;

    val = ov_json_number(config.limit.frame_buffer_max);
    if (!ov_json_object_set(par, OV_KEY_FRAME_BUFFER, val))
        goto error;

    if (config.normalize_input) {
        val = ov_json_true();
    } else {
        val = ov_json_false();
    }
    if (!ov_json_object_set(par, OV_KEY_NORMALIZE_INPUT, val))
        goto error;

    if (config.rtp_keepalive) {
        val = ov_json_true();
    } else {
        val = ov_json_false();
    }
    if (!ov_json_object_set(par, OV_KEY_RTP_KEEPALIVE, val))
        goto error;

    if (config.normalize_mixing_result_by_square_root) {
        val = ov_json_true();
    } else {
        val = ov_json_false();
    }
    if (!ov_json_object_set(par, OV_KEY_ROOT_MIX, val))
        goto error;

    return out;
error:
    ov_json_value_free(val);
    ov_json_value_free(out);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_mc_mixer_core_config
ov_mc_mixer_msg_configure_from_json(const ov_json_value *json) {

    ov_mc_mixer_core_config config = (ov_mc_mixer_core_config){0};

    if (!ov_event_api_event_is(json, OV_KEY_CONFIGURE))
        goto error;

    ov_json_value *par = ov_event_api_get_parameter(json);
    if (!par)
        goto error;

    config.vad.zero_crossings_rate_threshold_hertz = ov_json_number_get(
        ov_json_get(par, "/" OV_KEY_VAD "/" OV_KEY_ZERO_CROSSINGS_RATE_HERTZ));

    config.vad.powerlevel_density_threshold_db = ov_json_number_get(
        ov_json_get(par, "/" OV_KEY_VAD "/" OV_KEY_POWERLEVEL_DENSITY_DB));

    config.incoming_vad =
        ov_json_is_true(ov_json_get(par, "/" OV_KEY_VAD "/" OV_KEY_ENABLED));

    config.drop_no_va =
        ov_json_is_true(ov_json_get(par, "/" OV_KEY_VAD "/" OV_KEY_DROP));

    config.samplerate_hz =
        ov_json_number_get(ov_json_get(par, "/" OV_KEY_SAMPLE_RATE_HERTZ));

    config.comfort_noise_max_amplitude =
        ov_json_number_get(ov_json_get(par, "/" OV_KEY_COMFORT_NOISE));

    config.max_num_frames_to_mix =
        ov_json_number_get(ov_json_get(par, "/" OV_KEY_MAX_NUM_FRAMES));

    config.limit.frame_buffer_max =
        ov_json_number_get(ov_json_get(par, "/" OV_KEY_FRAME_BUFFER));

    if (ov_json_is_true(ov_json_get(par, "/" OV_KEY_NORMALIZE_INPUT))) {
        config.normalize_input = true;
    } else {
        config.normalize_input = false;
    }

    if (ov_json_is_true(ov_json_get(par, "/" OV_KEY_RTP_KEEPALIVE))) {
        config.rtp_keepalive = true;
    } else {
        config.rtp_keepalive = false;
    }

    if (ov_json_is_true(ov_json_get(par, "/" OV_KEY_ROOT_MIX))) {
        config.normalize_mixing_result_by_square_root = true;
    } else {
        config.normalize_mixing_result_by_square_root = false;
    }

    return config;

error:
    return (ov_mc_mixer_core_config){0};
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_mc_mixer_msg_acquire(const char *username,
                                       ov_mc_mixer_core_forward data) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *par = NULL;

    if (!username)
        goto error;

    out = ov_event_api_message_create(OV_KEY_ACQUIRE, NULL, 0);
    par = ov_event_api_set_parameter(out);
    if (!par)
        goto error;

    val = NULL;
    if (!ov_socket_configuration_to_json(data.socket, &val))
        goto error;
    if (!ov_json_object_set(par, OV_KEY_SOCKET, val))
        goto error;

    val = ov_json_number(data.ssrc);
    if (!ov_json_object_set(par, OV_KEY_SSRC, val))
        goto error;

    val = ov_json_number(data.payload_type);
    if (!ov_json_object_set(par, OV_KEY_PAYLOAD_TYPE, val))
        goto error;

    val = ov_json_string(username);
    if (!ov_json_object_set(par, OV_KEY_NAME, val))
        goto error;

    return out;

error:
    ov_json_value_free(out);
    ov_json_value_free(val);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_mc_mixer_msg_forward(const char *username,
                                       ov_mc_mixer_core_forward data) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *par = NULL;

    if (!username)
        goto error;

    out = ov_event_api_message_create(OV_KEY_FORWARD, NULL, 0);
    par = ov_event_api_set_parameter(out);
    if (!par)
        goto error;

    val = NULL;
    if (!ov_socket_configuration_to_json(data.socket, &val))
        goto error;
    if (!ov_json_object_set(par, OV_KEY_SOCKET, val))
        goto error;

    val = ov_json_number(data.ssrc);
    if (!ov_json_object_set(par, OV_KEY_SSRC, val))
        goto error;

    val = ov_json_number(data.payload_type);
    if (!ov_json_object_set(par, OV_KEY_PAYLOAD_TYPE, val))
        goto error;

    val = ov_json_string(username);
    if (!ov_json_object_set(par, OV_KEY_NAME, val))
        goto error;

    return out;

error:
    ov_json_value_free(out);
    ov_json_value_free(val);
    return NULL;
}

/*----------------------------------------------------------------------------*/

static ov_mc_mixer_core_forward
msg_forward_from_json(const ov_json_value *msg) {

    ov_mc_mixer_core_forward out = {0};

    if (!msg)
        goto error;

    const ov_json_value *par = ov_event_api_get_parameter(msg);
    if (!par)
        par = msg;

    out.socket = ov_socket_configuration_from_json(
        ov_json_get(par, "/" OV_KEY_SOCKET), (ov_socket_configuration){0});

    out.ssrc = ov_json_number_get(ov_json_get(par, "/" OV_KEY_SSRC));
    out.payload_type =
        ov_json_number_get(ov_json_get(par, "/" OV_KEY_PAYLOAD_TYPE));

    return out;
error:
    return (ov_mc_mixer_core_forward){0};
}

/*----------------------------------------------------------------------------*/

ov_mc_mixer_core_forward
ov_mc_mixer_msg_acquire_get_forward(const ov_json_value *m) {

    return msg_forward_from_json(m);
}

/*----------------------------------------------------------------------------*/

const char *ov_mc_mixer_msg_aquire_get_username(const ov_json_value *msg) {

    if (!msg)
        goto error;

    const ov_json_value *par = ov_event_api_get_parameter(msg);
    if (!par)
        par = msg;

    const char *loop = ov_json_string_get(ov_json_get(par, "/" OV_KEY_NAME));
    return loop;
error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_mc_mixer_msg_release(const char *name) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *par = NULL;

    if (!name)
        goto error;

    out = ov_event_api_message_create(OV_KEY_RELEASE, NULL, 0);
    par = ov_event_api_set_parameter(out);
    if (!par)
        goto error;

    val = ov_json_string(name);
    if (!ov_json_object_set(par, OV_KEY_NAME, val))
        goto error;

    return out;

error:
    ov_json_value_free(out);
    ov_json_value_free(val);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_mc_mixer_msg_join(ov_mc_loop_data data) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;

    if (0 == data.name[0] || 0 == data.socket.host[0])
        goto error;

    out = ov_event_api_message_create(OV_KEY_JOIN, NULL, 0);
    val = ov_mc_loop_data_to_json(data);
    if (!ov_json_object_set(out, OV_KEY_PARAMETER, val))
        goto error;

    return out;
error:
    ov_json_value_free(out);
    ov_json_value_free(val);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_mc_loop_data ov_mc_mixer_msg_join_from_json(const ov_json_value *msg) {

    if (!msg)
        goto error;

    const ov_json_value *par = ov_event_api_get_parameter(msg);
    if (!par)
        par = msg;

    return ov_mc_loop_data_from_json(par);
error:
    return (ov_mc_loop_data){0};
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_mc_mixer_msg_leave(const char *loopname) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *par = NULL;

    if (!loopname)
        goto error;

    out = ov_event_api_message_create(OV_KEY_LEAVE, NULL, 0);
    par = ov_event_api_set_parameter(out);
    if (!par)
        goto error;

    val = ov_json_string(loopname);
    if (!ov_json_object_set(par, OV_KEY_LOOP, val))
        goto error;

    return out;
error:
    ov_json_value_free(out);
    ov_json_value_free(val);
    return NULL;
}

/*----------------------------------------------------------------------------*/

const char *ov_mc_mixer_msg_leave_from_json(const ov_json_value *msg) {

    if (!msg)
        goto error;

    const ov_json_value *par = ov_event_api_get_parameter(msg);
    if (!par)
        par = msg;

    const char *loop = ov_json_string_get(ov_json_get(par, "/" OV_KEY_LOOP));
    return loop;
error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_mc_mixer_msg_volume(const char *loopname, uint8_t vol) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *par = NULL;

    if (!loopname)
        goto error;

    out = ov_event_api_message_create(OV_KEY_VOLUME, NULL, 0);
    par = ov_event_api_set_parameter(out);
    if (!par)
        goto error;

    val = ov_json_string(loopname);
    if (!ov_json_object_set(par, OV_KEY_LOOP, val))
        goto error;

    val = ov_json_number(vol);
    if (!ov_json_object_set(par, OV_KEY_VOLUME, val))
        goto error;

    return out;
error:
    ov_json_value_free(out);
    ov_json_value_free(val);
    return NULL;
}

/*----------------------------------------------------------------------------*/

const char *ov_mc_mixer_msg_volume_get_name(const ov_json_value *msg) {

    if (!msg)
        goto error;

    const ov_json_value *par = ov_event_api_get_parameter(msg);
    if (!par)
        par = msg;

    const char *loop = ov_json_string_get(ov_json_get(par, "/" OV_KEY_LOOP));
    return loop;
error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

uint8_t ov_mc_mixer_msg_volume_get_volume(const ov_json_value *msg) {

    if (!msg)
        goto error;

    const ov_json_value *par = ov_event_api_get_parameter(msg);
    if (!par)
        par = msg;

    double vol = ov_json_number_get(ov_json_get(par, "/" OV_KEY_VOLUME));

    if (vol > 100)
        vol = 100;

    if (vol < 0)
        vol = 0;

    return (uint8_t)vol;
error:
    return 0;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_mc_mixer_msg_shutdown() {

    return ov_event_api_message_create(OV_KEY_SHUTDOWN, NULL, 0);
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_mc_mixer_msg_state() {

    return ov_event_api_message_create(OV_KEY_STATE, NULL, 0);
}
