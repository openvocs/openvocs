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
        @file           ov_ice_config_from_generic.c
        @author         Markus

        @date           2024-09-11


        ------------------------------------------------------------------------
*/
#include "../include/ov_ice_config_from_generic.h"

ov_ice_config ov_ice_config_from_generic(ov_ice_proxy_generic_config c) {

    ov_ice_config config = (ov_ice_config){

        .loop = c.loop,

        .dtls.reconnect_interval_usec = c.config.dtls.reconnect_interval_usec,
        .dtls.dtls.keys.quantity = c.config.dtls.dtls.keys.quantity,
        .dtls.dtls.keys.length = c.config.dtls.dtls.keys.length,
        .dtls.dtls.keys.lifetime_usec = c.config.dtls.dtls.keys.lifetime_usec,

        .limits.transaction_lifetime_usecs =
            c.config.limits.transaction_lifetime_usecs,
        .limits.stun.connectivity_pace_usecs =
            c.config.timeouts.stun.connectivity_pace_usecs,
        .limits.stun.session_timeout_usecs =
            c.config.timeouts.stun.session_timeout_usecs,
        .limits.stun.keepalive_usecs = c.config.timeouts.stun.keepalive_usecs,

    };

    if (0 != c.config.dtls.cert[0])
        memcpy(config.dtls.cert, c.config.dtls.cert, PATH_MAX);

    if (0 != c.config.dtls.key[0])
        memcpy(config.dtls.key, c.config.dtls.key, PATH_MAX);

    if (0 != c.config.dtls.ca.file[0])
        memcpy(config.dtls.ca.file, c.config.dtls.ca.file, PATH_MAX);

    if (0 != c.config.dtls.ca.path[0])
        memcpy(config.dtls.ca.path, c.config.dtls.ca.path, PATH_MAX);

    return config;
}