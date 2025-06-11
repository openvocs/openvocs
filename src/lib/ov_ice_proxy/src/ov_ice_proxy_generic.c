/***
        ------------------------------------------------------------------------

        Copyright (c) 2024 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_ice_proxy_generic.c
        @author         Markus

        @date           2024-07-26


        ------------------------------------------------------------------------
*/
#include "../include/ov_ice_proxy_generic.h"

ov_ice_proxy_generic *ov_ice_proxy_generic_free(ov_ice_proxy_generic *self) {

    if (!ov_ice_proxy_generic_cast(self)) return self;

    return self->free(self);
}

/*----------------------------------------------------------------------------*/

ov_ice_proxy_generic *ov_ice_proxy_generic_cast(const void *data) {

    if (!data) return NULL;

    if (*(uint16_t *)data != OV_ICE_PROXY_GENERIC_MAGIC_BYTES) return NULL;

    return (ov_ice_proxy_generic *)data;
}

/*----------------------------------------------------------------------------*/

const char *ov_ice_proxy_generic_create_session(ov_ice_proxy_generic *self,
                                                ov_sdp_session *sdp) {

    if (!self || !sdp) return NULL;
    return self->session.create(self, sdp);
}

/*----------------------------------------------------------------------------*/

bool ov_ice_proxy_generic_drop_session(ov_ice_proxy_generic *self,
                                       const char *session_id) {

    if (!self || !session_id) return false;
    return self->session.drop(self, session_id);
}

/*----------------------------------------------------------------------------*/

bool ov_ice_proxy_generic_update_session(ov_ice_proxy_generic *self,
                                         const char *session_id,
                                         const ov_sdp_session *sdp) {

    if (!self || !sdp || !session_id) return false;
    return self->session.update(self, session_id, sdp);
}

/*----------------------------------------------------------------------------*/

bool ov_ice_proxy_generic_stream_candidate_in(
    ov_ice_proxy_generic *self,
    const char *session_id,
    uint32_t stream_id,
    const ov_ice_candidate *candidate) {

    if (!self || !candidate || !session_id) return false;
    return self->stream.candidate_in(self, session_id, stream_id, candidate);
}

/*----------------------------------------------------------------------------*/

bool ov_ice_proxy_generic_stream_end_of_candidates_in(
    ov_ice_proxy_generic *self, const char *session_id, uint32_t stream_id) {

    if (!self || !session_id) return false;
    return self->stream.end_of_candidates_in(self, session_id, stream_id);
}

/*----------------------------------------------------------------------------*/

uint32_t ov_ice_proxy_generic_stream_get_ssrc(ov_ice_proxy_generic *self,
                                              const char *session_id,
                                              uint32_t stream_id) {

    if (!self || !session_id) return 0;
    return self->stream.get_ssrc(self, session_id, stream_id);
}

/*----------------------------------------------------------------------------*/

ssize_t ov_ice_proxy_generic_stream_send(ov_ice_proxy_generic *self,
                                         const char *session_id,
                                         uint32_t stream_id,
                                         uint8_t *buffer,
                                         size_t size) {

    if (!self || !session_id || !buffer || !size) return 0;
    return self->stream.send(self, session_id, stream_id, buffer, size);
}

/*----------------------------------------------------------------------------*/

ov_ice_proxy_generic_config ov_ice_proxy_generic_config_from_json(
    const ov_json_value *input) {

    ov_ice_proxy_generic_config out = {0};

    const ov_json_value *config = ov_json_get(input, OV_KEY_PROXY);
    if (!config) config = input;

    out.external = ov_socket_configuration_from_json(
        ov_json_get(config, "/" OV_KEY_EXTERNAL), (ov_socket_configuration){0});

    out.dynamic.stun.server = ov_socket_configuration_from_json(
        ov_json_get(
            config, "/" OV_KEY_DYNAMIC "/" OV_KEY_STUN "/" OV_KEY_SERVER),
        (ov_socket_configuration){0});

    out.dynamic.turn.server = ov_socket_configuration_from_json(
        ov_json_get(
            config, "/" OV_KEY_DYNAMIC "/" OV_KEY_TURN "/" OV_KEY_SERVER),
        (ov_socket_configuration){0});

    const char *turn_user = ov_json_string_get(ov_json_get(
        config, "/" OV_KEY_DYNAMIC "/" OV_KEY_TURN "/" OV_KEY_USER));

    const char *turn_pass = ov_json_string_get(ov_json_get(
        config, "/" OV_KEY_DYNAMIC "/" OV_KEY_TURN "/" OV_KEY_PASS));

    if (turn_user)
        strncpy(out.dynamic.turn.username,
                turn_user,
                OV_ICE_PROXY_GENERIC_TURN_USERNAME_MAX);

    if (turn_pass)
        strncpy(out.dynamic.turn.password,
                turn_pass,
                OV_ICE_PROXY_GENERIC_TURN_PASSWORD_MAX);

    out.dynamic.ports.min = ov_json_number_get(
        ov_json_get(config, "/" OV_KEY_DYNAMIC "/" OV_KEY_PORT "/" OV_KEY_MIN));
    out.dynamic.ports.max = ov_json_number_get(
        ov_json_get(config, "/" OV_KEY_DYNAMIC "/" OV_KEY_PORT "/" OV_KEY_MAX));

    out.config.dtls = ov_ice_proxy_generic_dtls_config_from_json(config);

    out.config.limits.transaction_lifetime_usecs = ov_json_number_get(
        ov_json_get(config, "/" OV_KEY_LIMITS "/" OV_KEY_TRANSACTION_LIFETIME));

    out.config.timeouts.stun.connectivity_pace_usecs = ov_json_number_get(
        ov_json_get(config, "/" OV_KEY_TIMEOUTS "/" OV_KEY_CONNECTIVITY_PACE));

    out.config.timeouts.stun.session_timeout_usecs = ov_json_number_get(
        ov_json_get(config, "/" OV_KEY_TIMEOUTS "/" OV_KEY_SESSION));

    return out;
}