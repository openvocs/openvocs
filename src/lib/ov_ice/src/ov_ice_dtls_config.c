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
        @file           ov_ice_core_dtls_config.c
        @author         Markus

        @date           2024-09-11


        ------------------------------------------------------------------------
*/
#include "../include/ov_ice_dtls_config.h"

#include <ov_base/ov_config_keys.h>
#include <ov_base/ov_file.h>

ov_ice_dtls_config ov_ice_dtls_config_from_json(const ov_json_value *input) {

    ov_ice_dtls_config out = {0};

    const ov_json_value *conf = ov_json_object_get(input, OV_ICE_SSL_KEY);
    if (!conf)
        conf = input;

    /*
     *      We perform a read access on the cert and key,
     *      to ensure the config is valid.
     */

    const char *cert = ov_json_string_get(
        ov_json_object_get(conf, OV_ICE_SSL_KEY_CERTIFICATE_FILE));

    const char *key = ov_json_string_get(
        ov_json_object_get(conf, OV_ICE_SSL_KEY_CERTIFICATE_KEY));

    if (!cert || !key) {
        ov_log_error("SSL JSON config without cert or key");
        goto error;
    }

    size_t bytes = 0;

    const char *error = ov_file_read_check(cert);

    if (error) {
        ov_log_error("SSL config cannot read certificate "
                     "at %s error %s",
                     cert, error);
        goto error;
    }

    error = ov_file_read_check(key);

    if (error) {
        ov_log_error("SSL config cannot read key "
                     "at %s error %s",
                     key, error);
        goto error;
    }

    bytes = snprintf(out.cert, PATH_MAX, "%s", cert);
    if (bytes != strlen(cert))
        goto error;

    bytes = snprintf(out.key, PATH_MAX, "%s", key);
    if (bytes != strlen(key))
        goto error;

    const char *string =
        ov_json_string_get(ov_json_object_get(conf, OV_ICE_SSL_KEY_CA_FILE));

    if (string) {

        error = ov_file_read_check(string);

        if (error) {
            ov_log_error("SSL config cannot read CA FILE "
                         "at %s error %s",
                         string, error);
            goto error;
        }

        bytes = snprintf(out.ca.file, PATH_MAX, "%s", string);
        if (bytes != strlen(string))
            goto error;
    }

    string =
        ov_json_string_get(ov_json_object_get(conf, OV_ICE_SSL_KEY_CA_PATH));

    if (string) {

        error = ov_file_read_check(string);

        if (!error) {

            ov_log_error("SSL config wrong path for CA PATH "
                         "at %s error %s",
                         string, error);
            goto error;

        } else if (0 != strcmp(error, OV_FILE_IS_DIR)) {

            ov_log_error("SSL config wrong path for CA PATH "
                         "at %s error %s",
                         string, error);
            goto error;
        }

        bytes = snprintf(out.ca.path, PATH_MAX, "%s", string);
        if (bytes != strlen(string))
            goto error;
    }

    string =
        ov_json_string_get(ov_json_object_get(conf, OV_ICE_SSL_KEY_DTLS_STRP));
    if (!string)
        string = OV_ICE_DTLS_SRTP_PROFILES;

    if (string) {
        bytes =
            snprintf(out.srtp.profile, OV_ICE_SRTP_PROFILE_MAX, "%s", string);
        if (bytes != strlen(string))
            goto error;
    }

    ov_json_value *dtls = ov_json_object_get(conf, OV_ICE_SSL_KEY_DTLS);

    if (!dtls)
        goto done;

    out.dtls.keys.quantity = ov_json_number_get(
        ov_json_object_get(dtls, OV_ICE_SSL_KEY_DTLS_KEY_QUANTITY));

    out.dtls.keys.length = ov_json_number_get(
        ov_json_object_get(dtls, OV_ICE_SSL_KEY_DTLS_KEY_LENGTH));

    out.dtls.keys.lifetime_usec = ov_json_number_get(
        ov_json_object_get(dtls, OV_ICE_SSL_KEY_DTLS_KEY_LIFETIME_USEC));

done:
    return out;

error:
    return (ov_ice_dtls_config){0};
}

/*----------------------------------------------------------------------------*/

bool ov_ice_dtls_config_init(ov_ice_dtls_config *c) {

    if (!c)
        goto error;

    if (0 == c->cert[0]) {

        ov_log_error("DTLS config without certificate");
        goto error;
    }

    if (0 == c->key[0]) {

        ov_log_error("DTLS config without key");
        goto error;
    }

    if (0 == c->srtp.profile[0])
        snprintf(c->srtp.profile, OV_ICE_SRTP_PROFILE_MAX, "%s",
                 OV_ICE_DTLS_SRTP_PROFILES);

    if (0 == c->reconnect_interval_usec)
        c->reconnect_interval_usec = OV_ICE_DTLS_RECONNECT_INTERVAL_USEC;

    if (0 == c->dtls.keys.lifetime_usec)
        c->dtls.keys.lifetime_usec = OV_ICE_DTLS_KEY_LIFETIME_USEC;

    if (0 == c->dtls.keys.quantity)
        c->dtls.keys.quantity = OV_ICE_DTLS_KEY_QUANTITY;

    if (0 == c->dtls.keys.length)
        c->dtls.keys.length = OV_ICE_DTLS_KEY_LENGTH;

    return true;

error:
    return false;
}