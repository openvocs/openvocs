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
        @file           ov_domain.c
        @author         Markus Töpfer

        @date           2020-12-18


        ------------------------------------------------------------------------
*/
#include "../include/ov_domain.h"

#include <ov_base/ov_utils.h>
#include <unistd.h>

#include <ov_base/ov_config_keys.h>
#include <ov_base/ov_dir.h>
#include <ov_base/ov_file.h>

/*
 *      ------------------------------------------------------------------------
 *
 *      #PRIVATE FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

static const ov_domain init = {.magic_byte = OV_DOMAIN_MAGIC_BYTE};

/*----------------------------------------------------------------------------*/

static bool load_certificate(SSL_CTX *ctx, const ov_domain_config *config) {

    if (!ctx || !config) goto error;

    if (SSL_CTX_use_certificate_chain_file(ctx, config->certificate.cert) !=
        1) {
        ov_log_error(
            "%s failed to load certificate "
            "from %s | error %d | %s",
            config->name,
            config->certificate.cert,
            errno,
            strerror(errno));
        goto error;
    }

    if (SSL_CTX_use_PrivateKey_file(
            ctx, config->certificate.key, SSL_FILETYPE_PEM) != 1) {
        ov_log_error(
            "%s failed to load key "
            "from %s | error %d | %s",
            config->name,
            config->certificate.key,
            errno,
            strerror(errno));
        goto error;
    }

    if (SSL_CTX_check_private_key(ctx) != 1) {
        ov_log_error(
            "%s failure private key for\n"
            "CERT | %s\n"
            " KEY | %s",
            config->name,
            config->certificate.cert,
            config->certificate.key);
        goto error;
    }

    ov_log_debug("DOMAIN %.*s loaded SSL certificate \n file %s\n key %s\n",
                 (int)config->name.length,
                 config->name.start,
                 config->certificate.cert,
                 config->certificate.key);

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool init_tls_context(ov_domain *domain) {

    if (!domain) goto error;

    if (domain->context.tls) {
        ov_log_error("TLS context for domain |%.*s| already set",
                     (int)domain->config.name.length,
                     domain->config.name.start);
        goto error;
    }

    domain->context.tls = SSL_CTX_new(TLS_server_method());
    if (!domain->context.tls) {
        ov_log_error("TLS context for domain |%.*s| - failure",
                     (int)domain->config.name.length,
                     domain->config.name.start);
        goto error;
    }

    SSL_CTX_set_min_proto_version(domain->context.tls, TLS1_2_VERSION);

    if (load_certificate(domain->context.tls, &domain->config)) return true;

    /* Failure certificate load - cleanup */
    SSL_CTX_free(domain->context.tls);
    domain->context.tls = NULL;

error:
    return false;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      #GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

bool ov_domain_init(ov_domain *domain) {

    if (!domain) goto error;

    if (!memcpy(domain, &init, sizeof(ov_domain))) goto error;

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

void ov_domain_deinit_tls_context(ov_domain *domain) {

    if (!domain) goto error;

    if (domain->context.tls) {
        SSL_CTX_free(domain->context.tls);
        domain->context.tls = NULL;
    }

error:
    return;
}

/*----------------------------------------------------------------------------*/

bool ov_domain_array_clean(size_t size, ov_domain *array) {

    if (!array) goto error;

    for (size_t i = 0; i < size; i++) {
        array[i].config = (ov_domain_config){0};
        array[i].websocket.config = (ov_websocket_message_config){0};
        ov_domain_deinit_tls_context(&array[i]);
        array[i].websocket.uri = ov_dict_free(array[i].websocket.uri);
        array[i].event_handler.uri = ov_dict_free(array[i].event_handler.uri);
    }

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

ov_domain *ov_domain_array_free(size_t size, ov_domain *array) {

    if (!ov_domain_array_clean(size, array)) return array;

    return ov_data_pointer_free(array);
}

/*----------------------------------------------------------------------------*/

bool ov_domain_config_verify(const ov_domain_config *config) {

    if (!config) goto error;

    if (0 == config->name.start[0]) goto error;

    if (!ov_dir_access_to_path(config->path)) {
        ov_log_error("Unsufficient access to path|%s| domain |%.*s|",
                     config->path,
                     (int)config->name.length,
                     config->name.start);
        goto error;
    }

    if (0 != ov_file_read_check(config->certificate.cert)) {
        ov_log_error("Unsufficient access to certificate |%s| domain |%.*s|",
                     config->certificate.cert,
                     (int)config->name.length,
                     config->name.start);
        goto error;
    }

    if (0 != ov_file_read_check(config->certificate.key)) {
        ov_log_error(
            "Unsufficient access to certificate key |%s| domain "
            "|%.*s|",
            config->certificate.key,
            (int)config->name.length,
            config->name.start);
        goto error;
    }

    if (0 != config->certificate.ca.file[0]) {

        if (0 != ov_file_read_check(config->certificate.ca.file)) {
            ov_log_error(
                "Unsufficient access to certificate authority file"
                " |%s| domain |%.*s|",
                config->certificate.ca.file,
                (int)config->name.length,
                config->name.start);

            goto error;
        }
    }

    if (0 != config->certificate.ca.path[0]) {

        if (!ov_dir_access_to_path(config->certificate.ca.path)) {
            ov_log_error(
                "Unsufficient access to certificate authority path"
                " |%s| domain |%.*s|",
                config->certificate.ca.path,
                (int)config->name.length,
                config->name.start);

            goto error;
        }
    }

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

struct container1 {

    size_t next;
    size_t max;
    ov_domain *array;
};

/*----------------------------------------------------------------------------*/

static bool add_config_to_array(const void *key, void *value, void *data) {

    if (!key) return true;

    ov_json_value *val = ov_json_value_cast(value);
    if (!val) goto error;

    struct container1 *container = (struct container1 *)data;

    OV_ASSERT(container->next < container->max);

    if (container->next == container->max) goto error;

    if (!ov_domain_init(&container->array[container->next])) goto error;

    container->array[container->next].config =
        ov_domain_config_from_json(value);

    container->next++;
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_domain_load(const char *path,
                    size_t *array_size,
                    ov_domain **array_out) {

    ov_json_value *conf = NULL;
    ov_domain *arr = NULL;

    size_t entries = 0;

    if (!path || !array_size || !array_out) goto error;

    if (*array_out) return false;

    conf = ov_json_read_dir(path, NULL);
    if (!conf) {
        ov_log_error("Failed to read domain config dir %s", path);
        goto error;
    }

    ov_log_debug("Reading domains at %s", path);

    entries = ov_json_object_count(conf);
    if (0 == entries) goto error;

    size_t size = entries * sizeof(ov_domain);

    arr = calloc(1, size);
    if (!arr) goto error;

    struct container1 container = (struct container1){

        .next = 0, .max = entries, .array = arr

    };

    if (!ov_json_object_for_each(conf, &container, add_config_to_array))
        goto error;

    bool default_selected = false;

    /* Verify the domain configurations load certificates */
    for (size_t i = 0; i < entries; i++) {

        if (!ov_domain_config_verify(&arr[i].config)) goto cleanup;

        if (arr[i].config.is_default) {

            if (true == default_selected) {
                ov_log_error("More than ONE domain configured as default");
                goto cleanup;
            }

            default_selected = true;
        }

        ov_log_debug("Loaded domain %s", arr[i].config.name.start);

        if (!init_tls_context(&arr[i])) goto cleanup;
    }

    conf = ov_json_value_free(conf);
    *array_out = arr;
    *array_size = entries;
    return true;

cleanup:

    for (size_t i = 0; i < entries; i++) {
        ov_domain_deinit_tls_context(&arr[i]);
    }

error:
    arr = ov_domain_array_free(entries, arr);
    conf = ov_json_value_free(conf);
    if (array_size) *array_size = 0;
    return false;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      #CONFIG
 *
 *      ------------------------------------------------------------------------
 */

ov_domain_config ov_domain_config_from_json(const ov_json_value *value) {

    ov_domain_config cfg = {0};
    if (!value) goto error;

    const ov_json_value *config = ov_json_object_get(value, OV_KEY_DOMAIN);
    if (!config) config = value;

    const char *str = NULL;
    size_t bytes = 0;

    /* domain name */

    str = ov_json_string_get(ov_json_object_get(config, OV_KEY_NAME));
    if (str) {

        /* NOTE this will work for ASCII domains, but not non ascii */

        cfg.name.length = strlen(str);
        if (cfg.name.length > PATH_MAX) goto error;

        memcpy(cfg.name.start, str, cfg.name.length);
    }

    /* document root */

    str = ov_json_string_get(ov_json_object_get(config, OV_KEY_PATH));
    if (str) {

        bytes = snprintf(cfg.path, PATH_MAX, "%s", str);
        if ((bytes < 1) || (bytes == PATH_MAX)) goto error;
    }

    /* default */

    if (ov_json_is_true(ov_json_object_get(config, OV_KEY_DEFAULT)))
        cfg.is_default = true;

    /* certificate */

    ov_json_value *cert = ov_json_object_get(config, OV_KEY_CERTIFICATE);
    if (!cert) goto done;

    str = ov_json_string_get(ov_json_object_get(cert, OV_KEY_FILE));
    if (!str) goto error;

    bytes = snprintf(cfg.certificate.cert, PATH_MAX, "%s", str);
    if ((bytes < 1) || (bytes == PATH_MAX)) goto error;

    str = ov_json_string_get(ov_json_object_get(cert, OV_KEY_KEY));
    if (!str) goto error;

    bytes = snprintf(cfg.certificate.key, PATH_MAX, "%s", str);
    if ((bytes < 1) || (bytes == PATH_MAX)) goto error;

    /* (optional) Authority */
    ov_json_value *auth = ov_json_object_get(cert, OV_KEY_AUTHORITY);
    if (!auth) goto done;

    str = ov_json_string_get(ov_json_object_get(auth, OV_KEY_FILE));
    if (str) {

        bytes = snprintf(cfg.certificate.ca.file, PATH_MAX, "%s", str);
        if ((bytes < 1) || (bytes == PATH_MAX)) goto error;
    }

    str = ov_json_string_get(ov_json_object_get(auth, OV_KEY_PATH));
    if (str) {

        bytes = snprintf(cfg.certificate.ca.path, PATH_MAX, "%s", str);
        if ((bytes < 1) || (bytes == PATH_MAX)) goto error;
    }

done:
    return cfg;
error:
    return (ov_domain_config){0};
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_domain_config_to_json(ov_domain_config config) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;

    out = ov_json_object();
    if (!out) goto error;

    /* domain name */

    if (0 == config.name.start[0]) {
        val = ov_json_null();
    } else {
        val = ov_json_string((char *)config.name.start);
    }

    if (!ov_json_object_set(out, OV_KEY_NAME, val)) goto error;

    /* document root */

    if (0 == config.path[0]) {
        val = ov_json_null();
    } else {
        val = ov_json_string(config.path);
    }

    if (!ov_json_object_set(out, OV_KEY_PATH, val)) goto error;

    /* default */

    if (config.is_default) {
        val = ov_json_true();
        if (!ov_json_object_set(out, OV_KEY_DEFAULT, val)) goto error;
    }

    /* certificate */

    val = ov_json_object();
    if (!ov_json_object_set(out, OV_KEY_CERTIFICATE, val)) goto error;

    ov_json_value *cert = val;
    val = NULL;

    if (0 == config.certificate.cert[0]) {
        val = ov_json_null();
    } else {
        val = ov_json_string(config.certificate.cert);
    }

    if (!ov_json_object_set(cert, OV_KEY_FILE, val)) goto error;

    if (0 == config.certificate.key[0]) {
        val = ov_json_null();
    } else {
        val = ov_json_string(config.certificate.key);
    }

    if (!ov_json_object_set(cert, OV_KEY_KEY, val)) goto error;

    val = ov_json_object();
    if (!ov_json_object_set(cert, OV_KEY_AUTHORITY, val)) goto error;

    ov_json_value *auth = val;
    val = NULL;

    if (0 == config.certificate.ca.file[0]) {
        val = ov_json_null();
    } else {
        val = ov_json_string(config.certificate.ca.file);
    }

    if (!ov_json_object_set(auth, OV_KEY_FILE, val)) goto error;

    if (0 == config.certificate.ca.path[0]) {
        val = ov_json_null();
    } else {
        val = ov_json_string(config.certificate.ca.path);
    }

    if (!ov_json_object_set(auth, OV_KEY_PATH, val)) goto error;

    return out;
error:
    out = ov_json_value_free(out);
    val = ov_json_value_free(val);
    return NULL;
}
