/***
        ------------------------------------------------------------------------

        Copyright (c) 2026 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_ldap_role_import.c
        @author         Töpfer, Markus

        @date           2026-05-21


        ------------------------------------------------------------------------
*/

#include <lber.h>
#include <ldap.h>
#include <stdio.h>
#include <stdlib.h>

#include <ov_base/ov_config.h>
#include <ov_base/ov_config_keys.h>
#include <ov_base/ov_json.h>
#include <ov_base/ov_utils.h>

#define CONFIG_PATH                                                            \
    OPENVOCS_ROOT                                                              \
    "/src/tools/ov_ldap_role_import/config/default_config.json"

/*----------------------------------------------------------------------------*/

static LDAP *ldap_bind(const char *host, const char *user, const char *pass) {

    int version = LDAP_VERSION3;
    LDAP *ld = NULL;
    LDAPMessage *res = NULL;
    int msgid = 0;
    int err = 0;

    char *dn = NULL;

    if (!host || !user || !pass)
        goto error;

    char server[OV_HOST_NAME_MAX] = {0};
    snprintf(server, OV_HOST_NAME_MAX, "ldap://%s", host);

    struct berval cred =
        (struct berval){.bv_len = strlen(pass), .bv_val = (char *)pass};

    /* initialize the server */

    err = ldap_initialize(&ld, server);

    if (err != LDAP_SUCCESS) {

        fprintf(stderr, "ldap_initialize(): %s\n", ldap_err2string(err));
        goto error;
    }

    err = ldap_set_option(ld, LDAP_OPT_PROTOCOL_VERSION, &version);

    if (err != LDAP_SUCCESS) {

        fprintf(stderr, "ldap_set_option(PROTOCOL_VERSION): %s\n",
                ldap_err2string(err));
        goto error;
    };

    struct timeval timeout = {.tv_sec = 3};

    err = ldap_set_option(ld, LDAP_OPT_NETWORK_TIMEOUT, &timeout);
    if (err != LDAP_SUCCESS) {

        fprintf(stderr, "ldap_set_option(SIZELIMIT): %s\n",
                ldap_err2string(err));
        goto error;
    };

    printf("binding to server %s:%d as %s\n", host, LDAP_PORT, user);

    if (ldap_sasl_bind(ld, user, LDAP_SASL_SIMPLE, &cred, NULL, NULL, &msgid) !=
        LDAP_SUCCESS) {

        perror("ldap_sasl_bind");
    }

    err = ldap_result(ld, msgid, 0, &timeout, &res);

    switch (err) {
    case -1:

        ldap_get_option(ld, LDAP_OPT_RESULT_CODE, &err);
        fprintf(stderr, "ldap_result(): %s\n", ldap_err2string(err));
        goto error;

    case 0:

        fprintf(stderr, "ldap_result(): timeout expired\n");
        ldap_abandon_ext(ld, msgid, NULL, NULL);
        goto error;

    default:
        break;
    };

    ldap_parse_result(ld, res, &err, &dn, NULL, NULL, NULL, 0);
    if (err != LDAP_SUCCESS) {

        fprintf(stderr, "ldap_result(): %s\n", ldap_err2string(err));
        goto error;
    };

    fprintf(stdout, "authentication success\n");

    return ld;
error:
    if (ld)
        ldap_unbind_ext_s(ld, NULL, NULL);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ldap_get_roles(const char *host, const char *base,
                              const char *user, const char *pass,
                              uint64_t timeout_usec) {

    char name[PATH_MAX] = {0};

    ov_json_value *out = NULL;
    ov_json_value *username = NULL;
    ov_json_value *userid = NULL;
    ov_json_value *val = NULL;

    LDAP *ld = NULL;
    LDAPMessage *res = NULL;

    if (!base || !user || !host || !pass)
        goto error;

    char *filter = "(&(objectClass=*))";

    char *attrs[3] = {0};
    attrs[0] = "member";
    attrs[1] = "cn";
    attrs[2] = NULL;

    ld = ldap_bind(host, user, pass);
    if (!ld)
        goto error;

    int err = 0;

    struct timeval timeout = {.tv_sec = timeout_usec / 1000000,
                              .tv_usec = timeout_usec % 1000000};

    err = ldap_search_ext_s(ld,                 // LDAP            * ld
                            base,               // char            * base
                            LDAP_SCOPE_SUBTREE, // int               scope
                            filter,             // char            * filter
                            attrs,              // char            * attrs[]
                            0,                  // int               attrsonly
                            NULL,               // LDAPControl    ** serverctrls
                            NULL,               // LDAPControl    ** clientctrls
                            &timeout,           // struct timeval  * timeout
                            0,                  // int               sizelimit
                            &res                // LDAPMessage    ** res
    );

    if (err != LDAP_SUCCESS) {
        fprintf(stderr, "ldap_search_ext_s(): %s\n", ldap_err2string(err));
        goto error;
    };

    if (!(ldap_count_entries(ld, res))) {
        printf("0 entries found.\n");
        goto error;
    };

    out = ov_json_object();

    // loops through entries, attributes, and values
    LDAPMessage *entry = ldap_first_entry(ld, res);
    while ((entry)) {
        
        BerElement *ber = NULL;

        ov_json_value *role = NULL;

        char *attribute = ldap_first_attribute(ld, entry, &ber);
        while ((attribute)) {

            struct berval **vals = ldap_get_values_len(ld, entry, attribute);

            if (0 == strcmp(attribute, "cn")){

                role = ov_json_object();
                ov_json_object_set(out, vals[0]->bv_val, role);
            }

            attribute = ldap_next_attribute(ld, entry, ber);
            ldap_value_free_len(vals);
        };

        ber_free(ber, 0);

        attribute = ldap_first_attribute(ld, entry, &ber);
        while ((attribute)) {

            struct berval **vals = ldap_get_values_len(ld, entry, attribute);

            for (int pos = 0; pos < ldap_count_values_len(vals); pos++) {
                
                if (0 == strcmp(attribute, "member")){

                    size_t len = strlen(vals[pos]->bv_val);

                    char *ptr1 = vals[pos]->bv_val;
                    char *start = memchr(ptr1, '=', len) + 1;
                    char *end = NULL;

                    if (start){

                        end = memchr(start, ',', len - (start - ptr1));
                        if (end){
                            memset(name, 0, PATH_MAX);
                            snprintf(name, PATH_MAX, "%.*s", (int) (end - start), start);
                            ov_json_object_set(role, name, ov_json_true());
                        }
                    }
                }
            }

            attribute = ldap_next_attribute(ld, entry, ber);
            ldap_value_free_len(vals);
        };

        ber_free(ber, 0);

        LDAPMessage *new_entry = ldap_next_entry(ld, entry);
        ldap_memfree(entry);
        entry = new_entry;
    };

    ldap_unbind_ext_s(ld, NULL, NULL);

    return out;
error:
    ov_json_value_free(username);
    ov_json_value_free(userid);
    ov_json_value_free(val);
    ov_json_value_free(out);
    if (ld)
        ldap_unbind_ext_s(ld, NULL, NULL);
    return NULL;
}

/*----------------------------------------------------------------------------*/

int main(int argc, char **argv) {

    ov_json_value *config = NULL;
    ov_json_value *roles = NULL;

    const char *config_path = ov_config_path_from_command_line(argc, argv);
    if (!config_path)
        config_path = CONFIG_PATH;

    if (config_path == VERSION_REQUEST_ONLY)
        goto error;

    config = ov_config_load(config_path);
    if (!config)
        goto error;

    const char *host = ov_json_string_get(
        ov_json_get(config, "/" OV_KEY_LDAP "/" OV_KEY_HOST));

    const char *base = ov_json_string_get(
        ov_json_get(config, "/" OV_KEY_LDAP "/" OV_KEY_BASE));

    const char *user = ov_json_string_get(
        ov_json_get(config, "/" OV_KEY_LDAP "/" OV_KEY_USER));

    const char *pass = ov_json_string_get(
        ov_json_get(config, "/" OV_KEY_LDAP "/" OV_KEY_PASSWORD));

    const char *target_path =
        ov_json_string_get(ov_json_get(config, "/" OV_KEY_PATH));

    uint64_t timeout = ov_json_number_get(
        ov_json_get(config, "/" OV_KEY_LDAP "/" OV_KEY_TIMEOUT_USEC));

    if (0 == timeout)
        timeout = 5000000;

    fprintf(stdout, "using host %s user %s pass %s path %s\n", host, user, pass,
            target_path);

    roles = ldap_get_roles(host, base, user, pass, timeout);
    if (!roles) {
        fprintf(stderr, "failed to generate roles object");
        goto error;
    }

    char *str = ov_json_value_to_string(roles);
    ov_log_debug("%s", str);
    str = ov_data_pointer_free(str);

    ov_json_value_free(roles);
    ov_json_value_free(config);
    return EXIT_SUCCESS;
error:
    ov_json_value_free(roles);
    ov_json_value_free(config);
    return EXIT_FAILURE;
}