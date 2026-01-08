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
        @file           ov_ldap.c
        @author         Markus Töpfer

        @date           2022-03-06


        ------------------------------------------------------------------------
*/
#include "../include/ov_ldap.h"

#include <ldap.h>
#include <ov_base/ov_config_keys.h>
#include <ov_base/ov_dict.h>
#include <ov_base/ov_error_codes.h>
#include <ov_base/ov_utils.h>
#include <ov_core/ov_event_api.h>

#define IMPL_DEFAULT_TYPE 0x0001
#define IMPL_DN_LENGTH 1280

typedef struct Ldap {

    ov_ldap public;

    ov_thread_loop *thread_loop;
    ov_dict *store;

} Ldap;

/*----------------------------------------------------------------------------*/

#define AS_DEFAULT_LDAP(x)                                                     \
    (((ov_ldap_cast(x) != 0) && (IMPL_DEFAULT_TYPE == ((ov_ldap *)x)->type))   \
         ? (Ldap *)(x)                                                         \
         : 0)

/*----------------------------------------------------------------------------*/

ov_ldap *ov_ldap_cast(const void *data) {

    if (!data)
        return NULL;

    if (*(uint16_t *)data != OV_LDAP_MAGIC_BYTE)
        return NULL;

    return (ov_ldap *)data;
}

/*----------------------------------------------------------------------------*/

static ov_ldap *impl_free(ov_ldap *self) {

    Ldap *ldap = AS_DEFAULT_LDAP(self);
    if (!ldap)
        return self;

    ov_thread_loop_stop_threads(ldap->thread_loop);
    ldap->thread_loop = ov_thread_loop_free(ldap->thread_loop);
    ldap->store = ov_dict_free(ldap->store);
    ldap = ov_data_pointer_free(ldap);
    return NULL;
}

/*----------------------------------------------------------------------------*/

static bool impl_authenticate_password(ov_ldap *self, const char *user,
                                       const char *password, const char *uuid,
                                       ov_ldap_auth_callback callback) {

    ov_json_value *out = NULL;
    ov_json_value *par = NULL;
    ov_json_value *val = NULL;
    ov_thread_message *msg = NULL;

    Ldap *ldap = AS_DEFAULT_LDAP(self);
    if (!ldap || !user || !password || !uuid)
        goto error;

    out = ov_event_api_message_create(OV_EVENT_API_AUTHENTICATE, uuid, 0);

    par = ov_event_api_set_parameter(out);

    val = ov_json_string(user);
    if (!ov_json_object_set(par, OV_KEY_NAME, val))
        goto error;

    val = ov_json_string(password);
    if (!ov_json_object_set(par, OV_KEY_PASSWORD, val))
        goto error;

    val = NULL;

    msg = ov_thread_message_standard_create(OV_GENERIC_MESSAGE, out);

    if (!msg)
        goto error;

    if (!ov_thread_loop_send_message(ldap->thread_loop, msg,
                                     OV_RECEIVER_THREAD))
        goto error;

    ov_ldap_auth_callback *cb = calloc(1, sizeof(ov_ldap_auth_callback));
    *cb = callback;
    char *key = strdup(uuid);
    if (!ov_dict_set(ldap->store, key, cb, NULL)) {
        key = ov_data_pointer_free(key);
        cb = ov_data_pointer_free(cb);
        goto error;
    }

    return true;
error:
    ov_json_value_free(out);
    ov_json_value_free(val);
    ov_thread_message_free(msg);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool handle_thread_authenticate_request(ov_thread_loop *loop,
                                               ov_thread_message *msg) {

    char dn[IMPL_DN_LENGTH] = {0};
    char server[IMPL_DN_LENGTH] = {0};

    ov_json_value *out = NULL;

    LDAP *ld = NULL;
    LDAPMessage *res = NULL;
    int err = 0;

    if (!loop || !msg)
        goto error;

    ov_ldap *ldap = ov_ldap_cast(ov_thread_loop_get_data(loop));
    OV_ASSERT(ldap);

    int version = LDAP_VERSION3;

    ov_json_value *in = msg->json_message;

    const char *user = ov_json_string_get(
        ov_json_get(in, "/" OV_KEY_PARAMETER "/" OV_KEY_NAME));
    const char *pass = ov_json_string_get(
        ov_json_get(in, "/" OV_KEY_PARAMETER "/" OV_KEY_PASSWORD));

    OV_ASSERT(user);
    OV_ASSERT(pass);

    snprintf(dn, IMPL_DN_LENGTH, "uid=%s,%s", user, ldap->config.user_dn_tree);
    snprintf(server, IMPL_DN_LENGTH, "ldap://%s/", ldap->config.host);

    struct berval cred =
        (struct berval){.bv_len = strlen(pass), .bv_val = (char *)pass};

    if ((err = ldap_initialize(&ld, server)) != LDAP_SUCCESS) {
        // fprintf(stderr, "ldap_initialize(): %s\n", ldap_err2string(err));
        goto error_response;
    };

    err = ldap_set_option(ld, LDAP_OPT_PROTOCOL_VERSION, &version);
    if (err != LDAP_SUCCESS) {
        // fprintf(stderr, "ldap_set_option(PROTOCOL_VERSION): %s\n",
        // ldap_err2string(err));
        goto error_response;
    };

    struct timeval nettime = (struct timeval){
        .tv_sec = ldap->config.timeout.network_timeout_usec / 1000000,
        .tv_usec = ldap->config.timeout.network_timeout_usec % 1000000};

    err = ldap_set_option(ld, LDAP_OPT_NETWORK_TIMEOUT, &nettime);
    if (err != LDAP_SUCCESS) {
        // fprintf(stderr, "ldap_set_option(SIZELIMIT): %s\n",
        // ldap_err2string(err));
        goto error_response;
    }

    /* Print out an informational message. */
    ov_log_debug("LDAP binding to server %s:%d as %s\n", server, LDAP_PORT, dn);

    // printf( "Binding to server %s:%d\n", server, LDAP_PORT );
    // printf( "as the DN %s ...\n", dn );

    int msgid = 0;

    if (ldap_sasl_bind(ld, dn, LDAP_SASL_SIMPLE, &cred, NULL, NULL, &msgid) !=
        LDAP_SUCCESS) {

        // perror( "ldap_sasl_bind" );
        // fprintf(stderr, "sasl_bind timeout %i\n", (int)nettime.tv_sec);
        goto error_response;
    }

    /* wait nettime for a result */
    /*
        struct timeval t = (struct timeval) {
            .tv_sec = 2,
            .tv_usec = 0
        };
    */
    // fprintf(stderr, "time %i\n", (int)nettime.tv_sec);

    err = ldap_result(ld, msgid, 0, &nettime, &res);

    switch (err) {
    case -1:
        ldap_get_option(ld, LDAP_OPT_RESULT_CODE, &err);
        // fprintf(stderr, "ldap_result(): %s\n", ldap_err2string(err));
        goto error_response;

    case 0:
        // fprintf(stderr, "ldap_result(): timeout expired\n");
        ldap_abandon_ext(ld, msgid, NULL, NULL);
        goto error_response;
    default:
        break;
    };

    char *matched = NULL;

    ldap_parse_result(ld, res, &err, &matched, NULL, NULL, NULL, 0);
    if (err != LDAP_SUCCESS) {
        // fprintf(stderr, "ldap_result(): %s\n", ldap_err2string(err));
        goto error_response;
    };

    /* success */

    out = ov_event_api_create_success_response(in);
    goto done;

error_response:

    /* failure */

    out = ov_event_api_create_error_response(in, OV_ERROR_CODE, OV_ERROR_DESC);

done:

    msg->json_message = ov_json_value_free(msg->json_message);
    msg->json_message = out;

    ov_thread_loop_send_message(loop, msg, OV_RECEIVER_EVENT_LOOP);
    ldap_unbind_ext_s(ld, NULL, NULL);
    return true;
error:
    if (ld)
        ldap_unbind_ext_s(ld, NULL, NULL);

    ov_thread_message_free(msg);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool handle_thread_message(ov_thread_loop *loop,
                                  ov_thread_message *msg) {

    if (!loop || !msg)
        goto error;

    if (ov_event_api_event_is(msg->json_message, OV_EVENT_API_AUTHENTICATE))
        return handle_thread_authenticate_request(loop, msg);

    /* Unexpected message in thread */

    char *str = ov_json_value_to_string(msg->json_message);
    ov_log_error("Received unknown event %s", str);
    str = ov_data_pointer_free(str);
    msg = ov_thread_message_free(msg);

    OV_ASSERT(1 == 0);

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool handle_loop_authenticate_request(ov_thread_loop *loop,
                                             ov_thread_message *msg) {

    ov_ldap_auth_callback *cb = NULL;
    if (!loop || !msg)
        goto error;

    Ldap *ldap = AS_DEFAULT_LDAP(ov_thread_loop_get_data(loop));
    OV_ASSERT(ldap);

    const char *uuid = ov_event_api_get_uuid(msg->json_message);
    OV_ASSERT(uuid);

    cb = ov_dict_remove(ldap->store, uuid);
    if (!cb->userdata || !cb->callback)
        goto error;

    int err = ov_event_api_get_error_code(msg->json_message);

    switch (err) {

    case 0:
        cb->callback(cb->userdata, uuid, OV_LDAP_AUTH_GRANTED);
        break;

    default:
        cb->callback(cb->userdata, uuid, OV_LDAP_AUTH_REJECTED);
        break;
    }

    cb = ov_data_pointer_free(cb);
    ov_thread_message_free(msg);
    return true;
error:
    cb = ov_data_pointer_free(cb);
    ov_thread_message_free(msg);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool handle_loop_message(ov_thread_loop *loop, ov_thread_message *msg) {

    if (!loop || !msg)
        goto error;

    if (ov_event_api_event_is(msg->json_message, OV_EVENT_API_AUTHENTICATE))
        return handle_loop_authenticate_request(loop, msg);

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool impl_ldap_reconfigure(ov_ldap *self, ov_ldap_config config) {

    Ldap *ldap = AS_DEFAULT_LDAP(self);
    if (!ldap)
        goto error;

    if (!config.loop)
        goto error;

    if (0 == config.timeout.network_timeout_usec)
        config.timeout.network_timeout_usec =
            OV_LDAP_DEFAULT_NETWORK_TIMEOUT_USEC;

    if (0 == config.threads.message_queue_capacity)
        config.threads.message_queue_capacity =
            OV_LDAP_DEFAULT_THREAD_QUEUE_CAPACITY;

    if (0 == config.threads.lock_timeout_usecs)
        config.threads.lock_timeout_usecs =
            OV_LDAP_DEFAULT_THREAD_LOCK_TIMEOUT_USEC;

    if (0 == config.threads.num_threads)
        config.threads.num_threads = OV_LDAP_DEFAULT_THREADS;

    if (!ov_thread_loop_stop_threads(ldap->thread_loop))
        goto error;

    if (!ov_thread_loop_reconfigure(ldap->thread_loop, config.threads))
        goto error;

    if (!ov_thread_loop_start_threads(ldap->thread_loop))
        goto error;

    ldap->public.config = config;

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

ov_ldap *ov_ldap_create(ov_ldap_config config) {

    Ldap *ldap = NULL;

    if (!config.loop)
        goto error;

    if (0 == config.timeout.network_timeout_usec)
        config.timeout.network_timeout_usec =
            OV_LDAP_DEFAULT_NETWORK_TIMEOUT_USEC;

    if (0 == config.threads.message_queue_capacity)
        config.threads.message_queue_capacity =
            OV_LDAP_DEFAULT_THREAD_QUEUE_CAPACITY;

    if (0 == config.threads.lock_timeout_usecs)
        config.threads.lock_timeout_usecs =
            OV_LDAP_DEFAULT_THREAD_LOCK_TIMEOUT_USEC;

    if (0 == config.threads.num_threads)
        config.threads.num_threads = OV_LDAP_DEFAULT_THREADS;

    ldap = calloc(1, sizeof(Ldap));
    if (!ldap)
        goto error;

    ldap->public.magic_byte = OV_LDAP_MAGIC_BYTE;
    ldap->public.type = IMPL_DEFAULT_TYPE;

    ldap->public.config = config;

    ldap->public.free = impl_free;
    ldap->public.authenticate.password = impl_authenticate_password;
    ldap->public.reconfigure = impl_ldap_reconfigure;

    ov_dict_config d_config = ov_dict_string_key_config(255);
    d_config.value.data_function.free = ov_data_pointer_free;

    ldap->store = ov_dict_create(d_config);
    if (!ldap->store)
        goto error;

    ldap->thread_loop = ov_thread_loop_create(
        config.loop,
        (ov_thread_loop_callbacks){
            .handle_message_in_thread = handle_thread_message,
            .handle_message_in_loop = handle_loop_message},
        ldap);

    if (!ldap->thread_loop)
        goto error;

    if (!ov_thread_loop_reconfigure(ldap->thread_loop, config.threads))
        goto error;

    if (!ov_thread_loop_start_threads(ldap->thread_loop))
        goto error;

    return ov_ldap_cast(ldap);
error:
    ov_ldap_free((ov_ldap *)ldap);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_ldap *ov_ldap_free(ov_ldap *self) {

    if (!self || !self->free)
        return self;

    return self->free(self);
}

/*----------------------------------------------------------------------------*/

bool ov_ldap_authenticate_password(ov_ldap *self, const char *user,
                                   const char *password, const char *uuid,
                                   ov_ldap_auth_callback callback) {

    if (!self || !self->authenticate.password)
        return false;

    return self->authenticate.password(self, user, password, uuid, callback);
}

/*----------------------------------------------------------------------------*/

bool ov_ldap_reconfigure(ov_ldap *self, ov_ldap_config config) {

    if (!self || !self->reconfigure)
        return false;

    return self->reconfigure(self, config);
}

/*----------------------------------------------------------------------------*/

ov_ldap_config ov_ldap_config_from_json(const ov_json_value *val) {

    ov_ldap_config config = {0};

    const ov_json_value *conf = ov_json_object_get(val, OV_KEY_LDAP);
    if (!conf)
        conf = val;

    if (!conf)
        goto error;

    ov_json_value *threads = ov_json_object_get(conf, OV_KEY_THREADS);
    config.threads = ov_thread_loop_config_from_json(threads);

    const char *str = ov_json_string_get(ov_json_get(conf, "/" OV_KEY_HOST));
    if (str)
        strncpy(config.host, str, OV_HOST_NAME_MAX);

    str = ov_json_string_get(ov_json_get(conf, "/" OV_KEY_USER_DN_TREE));
    if (str)
        strncpy(config.user_dn_tree, str, OV_LDAP_USER_DN_TREE);

    config.timeout.network_timeout_usec = ov_json_number_get(
        ov_json_get(conf, "/" OV_KEY_TIMEOUT "/" OV_KEY_NETWORK));

    return config;

error:
    return (ov_ldap_config){0};
}
