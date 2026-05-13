static bool set_async(ov_vocs *vocs, const char *uuid, int socket, const ov_json_value *msg){

    ov_json_value *out = NULL;

    if (!vocs || !uuid || !msg) goto error;

    if (!ov_json_value_copy((void**)&out, (void*)msg)) goto error;

    if (!ov_event_async_set(
        vocs->async,
        uuid,
        (ov_event_async_data){.socket = socket,
                              .value = out,
                              .timedout.userdata = vocs,
                              .timedout.callback = async_timedout},
            vocs->config.timeout.response_usec)) {
            goto error;
    }

    return true;
error:
    ov_json_value_free(out);
    return false;
}

/*----------------------------------------------------------------------------*/

static void async_timedout_sip(void *userdata, ov_event_async_data data) {

    ov_vocs *self = ov_vocs_cast(userdata);
    if (!self)
        goto error;

    ov_json_value *out = ov_event_api_create_error_response(
        data.value, OV_ERROR_TIMEOUT, OV_ERROR_DESC_TIMEOUT);
    env_send(self, data.socket, out);
    out = ov_json_value_free(out);

error:
    ov_event_async_data_clear(&data);
    return;
}

/*----------------------------------------------------------------------------*/

static bool set_async_sip(ov_vocs *vocs, const char *uuid, int socket, const ov_json_value *msg){

    ov_json_value *out = NULL;

    if (!vocs || !uuid || !msg) goto error;

    if (!ov_json_value_copy((void**)&out, (void*)msg)) goto error;

    if (!ov_event_async_set(
        vocs->async,
        uuid,
        (ov_event_async_data){.socket = socket,
                              .value = out,
                              .timedout.userdata = vocs,
                              .timedout.callback = async_timedout_sip},
            vocs->config.timeout.response_usec)) {
            goto error;
    }

    return true;
error:
    ov_json_value_free(out);
    return false;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      #API ADMIN EVENTS
 *
 *      ------------------------------------------------------------------------
 */

/*----------------------------------------------------------------------------*/

static void cb_start_recording(void *userdata, int socket, const char *uuid, 
    const char *loop, ov_result result){

    ov_json_value *out = NULL;
    UNUSED(socket);

    ov_vocs *self = ov_vocs_cast(userdata);
    if (!self || !uuid || !loop) goto error;

    ov_event_async_data async = ov_event_async_unset(self->async, uuid);
    if (0 == async.socket) goto error;

    if (0 != result.error_code){

        out = ov_event_api_create_error_response(async.value, result.error_code, result.message);

    } else {

        out = ov_event_api_create_success_response(async.value);
    }

    env_send(self, async.socket, out);
    out = ov_json_value_free(out);

error:
    ov_event_async_data_clear(&async);
    return;

}

/*----------------------------------------------------------------------------*/

static void admin_start_recording(void *userdata, const char *name,
        int socket, const ov_json_value *input) {

    ov_vocs *vocs = ov_vocs_cast(userdata);
    UNUSED(name);

    const char *uuid = ov_event_api_get_uuid(input);
    if (!uuid) goto error;

    const char *loop = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_LOOP));

    if (!uuid || !loop) {

        send_error_response(vocs,
                            input,
                            socket,
                            OV_ERROR_CODE_PARAMETER_ERROR,
                            OV_ERROR_DESC_PARAMETER_ERROR);

        goto done;
    }

    if (!set_async(vocs, uuid, socket, input)) goto error;

    if (!ov_vocs_recorder_start_loop_recording(
        vocs->recorder, uuid, loop, vocs, socket, cb_start_recording)) {
        goto done;
    }

done:
    return;
error:
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_stop_recording(void *userdata, int socket, const char *uuid, 
    const char *loop, ov_result result){

    UNUSED(socket);

    ov_json_value *out = NULL;

    ov_vocs *self = ov_vocs_cast(userdata);
    if (!self || !uuid || !loop) goto error;

    ov_event_async_data async = ov_event_async_unset(self->async, uuid);
    if (0 == async.socket) goto error;

    if (0 != result.error_code){

        out = ov_event_api_create_error_response(async.value, result.error_code, result.message);

    } else {

        out = ov_event_api_create_success_response(async.value);
    }

    env_send(self, async.socket, out);
    out = ov_json_value_free(out);

error:
    ov_event_async_data_clear(&async);
    return;

}

/*----------------------------------------------------------------------------*/

static void admin_stop_recording(void *userdata, const char *name,
        int socket, const ov_json_value *input) {

    ov_vocs *vocs = ov_vocs_cast(userdata);
    UNUSED(name);

    if (!vocs || !input) goto error;

    const char *uuid = ov_event_api_get_uuid(input);
    if (!uuid) goto error;

    const char *loop = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_LOOP));

    if (!uuid || !loop) {

        send_error_response(vocs,
                            input,
                            socket,
                            OV_ERROR_CODE_PARAMETER_ERROR,
                            OV_ERROR_DESC_PARAMETER_ERROR);

        goto done;
    }

    if (!set_async(vocs, uuid, socket, input)) goto error;

    if (!ov_vocs_recorder_stop_loop_recording(
        vocs->recorder, uuid, loop, vocs, socket, cb_stop_recording)) {

        send_error_response(vocs,
                            input,
                            socket,
                            OV_ERROR_CODE_PROCESSING_ERROR,
                            OV_ERROR_DESC_PROCESSING_ERROR);
    }

done:
    return ;
error:
    return ;
}

/*----------------------------------------------------------------------------*/

static void admin_broadcast(void *userdata, const char *name,
        int socket, const ov_json_value *input) {

    ov_vocs *vocs = ov_vocs_cast(userdata);
    UNUSED(name);

    if (!vocs || !input) goto error;

    if (!ov_event_api_set_type((ov_json_value*)input, OV_BROADCAST_KEY_SYSTEM_BROADCAST))
        goto error;

    ov_event_parameter parameter =
        (ov_event_parameter){.send.instance = vocs, .send.send = send_socket};

    if (!ov_broadcast_registry_send(
            vocs->broadcasts, OV_BROADCAST_KEY_SYSTEM_BROADCAST, &parameter, input, 
            OV_SYSTEM_BROADCAST)) {

        send_error_response(vocs,
                            input,
                            socket,
                            OV_ERROR_CODE_PROCESSING_ERROR,
                            OV_ERROR_DESC_PROCESSING_ERROR);
    } else {

        send_success_response(vocs, input, socket, NULL);
        
    }
    
    return ;
error:
    return ;
}

/*----------------------------------------------------------------------------*/

static void admin_get_recorded_loops(void *userdata, const char *name,
        int socket, const ov_json_value *input) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;

    ov_vocs *vocs = ov_vocs_cast(userdata);
    UNUSED(name);

    if (!vocs || !input) goto error;

    val = ov_vocs_recorder_get_recorded_loops(vocs->recorder);
    if (!val){
        val = ov_json_object();
    }

    out = ov_event_api_create_success_response(input);
    ov_json_value *res = ov_event_api_get_response(out);
    if (!ov_json_object_set(res, OV_KEY_RESULT, val)) goto error;

    val = NULL;

    env_send(vocs, socket, out);
    out = ov_json_value_free(out);
    return;
error:
    val = ov_json_value_free(val);
    out = ov_json_value_free(out);
    return;
}

/*----------------------------------------------------------------------------*/

static void admin_get_all_loops(void *userdata, const char *name,
        int socket, const ov_json_value *input) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;

    ov_vocs *vocs = ov_vocs_cast(userdata);
    UNUSED(name);

    if (!vocs || !input) goto error;

    val = ov_vocs_db_get_all_loops_incl_domain(vocs->config.db);
    if (!val){
        val = ov_json_object();
    }

    out = ov_event_api_create_success_response(input);
    ov_json_value *res = ov_event_api_get_response(out);
    if (!ov_json_object_set(res, "loops", val)) goto error;

    val = NULL;

    env_send(vocs, socket, out);
    out = ov_json_value_free(out);
    return;
error:
    val = ov_json_value_free(val);
    out = ov_json_value_free(out);
    return;
}

/*----------------------------------------------------------------------------*/

struct mixer_data {

    ov_vocs *vocs;
    int socket;
    const char *user;
    const ov_json_value *input;

};

/*----------------------------------------------------------------------------*/

static void cb_mixer_state(void *userdata, const char *uuid, const ov_json_value *state){

    ov_vocs *vocs = ov_vocs_cast(userdata);
    if (!vocs || !uuid || !state) goto error;

    ov_event_async_data adata = ov_event_async_unset(vocs->async, uuid);
    if (!adata.value) goto error;

    ov_json_value *res = ov_event_api_get_response(adata.value);
    ov_json_value *out = NULL;
    ov_json_value_copy((void**)&out, state);
    ov_json_object_set(res, OV_KEY_STATE, out);

    env_send(vocs, adata.socket, adata.value);

    adata.value = ov_json_value_free(adata.value);


error:
    return;
}

/*----------------------------------------------------------------------------*/

static bool call_mixer_state(const void *key, void *val, void *data){

    if (!key) return true;

    struct mixer_data *container = (struct mixer_data*) data;

    const char *user = ov_json_string_get(ov_json_get(
        ov_json_value_cast(val), "/"OV_KEY_USER));

    if (!user) return true;

    if (0 != ov_string_compare(user, container->user))
        return true;

    const char *session = ov_json_string_get(ov_json_get(
        ov_json_value_cast(val), "/"OV_KEY_SESSION));

    if (!session) return true;

    ov_id uuid = {0};
    ov_id_fill_with_uuid(uuid);

    ov_json_value *out = ov_event_api_create_success_response(container->input);
    if (!out) goto error;

    if (!ov_mc_backend_get_session_state(container->vocs->backend,
        uuid, session, container->vocs, cb_mixer_state)) {

        out = ov_json_value_free(out);
        goto error;
    
    }

    const char *role = ov_json_string_get(ov_json_get(
        ov_json_value_cast(val), "/"OV_KEY_ROLE));

    ov_json_value *res = ov_event_api_get_response(out);

    ov_json_object_set(res, OV_KEY_USER, ov_json_string(user));
    ov_json_object_set(res, OV_KEY_SESSION, ov_json_string(session));

    if (role)
        ov_json_object_set(res, OV_KEY_ROLE, ov_json_string(role));

    if (!set_async(container->vocs, uuid, container->socket, out)) goto error;

    return true;

error:
    return false;
}

/*----------------------------------------------------------------------------*/

static void admin_get_mixer_state(void *userdata, const char *name,
        int socket, const ov_json_value *input) {

    ov_json_value *out = NULL;

    ov_vocs *vocs = ov_vocs_cast(userdata);
    UNUSED(name);

    if (!vocs || !input) goto error;

    const char *user = ov_json_string_get(
        ov_json_get(input, "/"OV_KEY_PARAMETER"/"OV_KEY_USER));

    if (!user) {

        send_error_response(
            vocs,
            input,
            socket,
            OV_ERROR_CODE_PARAMETER_ERROR,
            OV_ERROR_DESC_PARAMETER_ERROR);

        goto error;
    }

    struct mixer_data container = (struct mixer_data){
        .vocs = vocs,
        .user = user,
        .socket = socket,
        .input = input
    };

    if (!ov_socket_storage_for_each(vocs->connections, &container, call_mixer_state)){

        send_error_response(
            vocs,
            input,
            socket,
            OV_ERROR_CODE_PROCESSING_ERROR,
            OV_ERROR_DESC_PROCESSING_ERROR);

        goto error;
    }

    return ;
error:
    out = ov_json_value_free(out);
    return ;
}


/*----------------------------------------------------------------------------*/

static void admin_state_mixer(void *userdata, const char *name,
        int socket, const ov_json_value *input) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *res = NULL;

    ov_vocs *vocs = ov_vocs_cast(userdata);
    UNUSED(name);

    if (!vocs || !input) goto error;

    ov_mc_backend_count count =
        ov_mc_backend_state_mixers(vocs->backend);

    out = ov_event_api_create_success_response(input);
    res = ov_event_api_get_response(out);

    val = ov_json_number(count.mixers);
    if (!ov_json_object_set(res, OV_KEY_MIXER, val)) goto error;

    val = ov_json_number(count.used);
    if (!ov_json_object_set(res, OV_KEY_USED, val)) goto error;

    env_send(vocs, socket, out);

    ov_json_value_free(out);
    return;
error:
    ov_json_value_free(out);
    ov_json_value_free(val);
    return;
}

/*----------------------------------------------------------------------------*/

static void admin_state_connections(void *userdata, const char *name,
        int socket, const ov_json_value *input) {

    ov_json_value *out = NULL;
    ov_json_value *obj = NULL;
    ov_json_value *val = NULL;
    ov_json_value *res = NULL;

    ov_vocs *vocs = ov_vocs_cast(userdata);
    UNUSED(name);

    if (!vocs || !input) goto error;

    out = ov_event_api_create_success_response(input);
    res = ov_event_api_get_response(out);

    obj = ov_json_object();
    if (!ov_json_object_set(res, OV_KEY_CONNECTIONS, obj)) {
        obj = ov_json_value_free(obj);
        goto error;
    }

    if (!ov_socket_storage_for_each_set_data(vocs->connections, obj)) goto error;

    env_send(vocs, socket, out);

    ov_json_value_free(out);
    return;
error:
    ov_json_value_free(val);
    ov_json_value_free(out);
    return;
}

/*----------------------------------------------------------------------------*/

static void admin_state_session(void *userdata, const char *name,
        int socket, const ov_json_value *input) {

    ov_vocs *vocs = ov_vocs_cast(userdata);
    UNUSED(name);

    if (!vocs || !input) goto error;

    const char *uuid = ov_event_api_get_uuid(input);
    if (!uuid) goto error;

    const char *session_id = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_SESSION));

    if (!uuid || !session_id) {

        send_error_response(vocs,
                            input,
                            socket,
                            OV_ERROR_CODE_PARAMETER_ERROR,
                            OV_ERROR_DESC_PARAMETER_ERROR);
        goto done;
    }

    if (!ov_mc_frontened_get_session_state(vocs->frontend, uuid, session_id)) {

        send_error_response(vocs,
                            input,
                            socket,
                            OV_ERROR_CODE_SESSION_UNKNOWN,
                            OV_ERROR_DESC_SESSION_UNKNOWN);

        goto done;
    }

    if (!set_async(vocs, uuid, socket, input)) goto error;

done:
    return;
error:
    return;
}

/*----------------------------------------------------------------------------*/

static bool add_connection_data(const void *key, void *val, void *data){

    if (!key) return true;

    ov_json_value *array = ov_json_value_cast(data);
    ov_json_value *copy = NULL;
    ov_json_value *item = ov_json_value_cast(val);

    ov_json_value_copy((void**)&copy, item);
    ov_json_array_push(array, copy);
    return true;
}

/*----------------------------------------------------------------------------*/

static void admin_get_clients(void *userdata, const char *name,
        int socket, const ov_json_value *input) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;

    ov_vocs *vocs = ov_vocs_cast(userdata);
    UNUSED(name);

    if (!vocs || !input) goto error;

    val = ov_json_array();

    if (!ov_socket_storage_for_each(
        vocs->connections, val, add_connection_data)) goto error;

    out = ov_event_api_create_success_response(input);
    ov_json_value *res = ov_event_api_get_response(out);
    ov_json_object_set(res, "result", val);

    env_send(vocs, socket, out);
    ov_json_value_free(out);

    return;
error:
    return;
}

/*----------------------------------------------------------------------------*/

static bool enable_api_admin(ov_vocs *vocs){

    if (!ov_event_broker_register(
        vocs->broker,
        OV_EVENT_START_RECORD,
        vocs,
        admin_start_recording)) goto error;

    if (!ov_event_broker_register(
        vocs->broker,
        OV_EVENT_STOP_RECORD,
        vocs,
        admin_stop_recording)) goto error;

    if (!ov_event_broker_register(
        vocs->broker,
        OV_EVENT_API_STATE_MIXER,
        vocs,
        admin_state_mixer)) goto error;

    if (!ov_event_broker_register(
        vocs->broker,
        OV_EVENT_API_STATE_CONNECTIONS,
        vocs,
        admin_state_connections)) goto error;

    if (!ov_event_broker_register(
        vocs->broker,
        OV_EVENT_API_STATE_SESSION,
        vocs,
        admin_state_session)) goto error;

    if (!ov_event_broker_register(
        vocs->broker,
        "admin_get_mixer_state",
        vocs,
        admin_get_mixer_state)) goto error;

    if (!ov_event_broker_register(
        vocs->broker,
        OV_KEY_BROADCAST,
        vocs,
        admin_broadcast)) goto error;

    if (!ov_event_broker_register(
        vocs->broker,
        "recording_get_recorded_loops",
        vocs,
        admin_get_recorded_loops)) goto error;

    if (!ov_event_broker_register(
        vocs->broker,
        "db_get_all_loops",
        vocs,
        admin_get_all_loops)) goto error;

    if (!ov_event_broker_register(
        vocs->broker,
        "db_get_clients",
        vocs,
        admin_get_clients)) goto error;

    return true;
error:
	return false;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      #API AUTH EVENTS
 *
 *      ------------------------------------------------------------------------
 */

static void client_logout(void *userdata, const char *name,
        int socket, const ov_json_value *input) {

    ov_json_value *data = NULL;

    ov_vocs *vocs = ov_vocs_cast(userdata);
    UNUSED(name);

    if (!vocs || !input || socket < 0) goto error;

    data = ov_socket_storage_get(vocs->connections, socket);
    const char *session =
        ov_json_string_get(ov_json_get(data, "/" OV_KEY_SESSION));
    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));
    const char *client =
        ov_json_string_get(ov_json_get(data, "/" OV_KEY_CLIENT));

    send_success_response(vocs, input, socket, NULL);

    ov_log_info("client logout %i|%s user %s", socket, session, user);

    ov_event_session_delete(vocs->user_sessions, client);

    /* Close connection socket if drop is not successfull */
    drop_connection(vocs, socket, true, true);

error:
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_ldap_authentication(void *userdata,
                                   const char *uuid,
                                   ov_ldap_auth_result result) {

    ov_json_value *out = NULL;
    ov_json_value *data = NULL;

    ov_event_async_data adata = {0};

    ov_vocs *vocs = ov_vocs_cast(userdata);
    OV_ASSERT(vocs);
    if (!vocs) goto error;

    adata = ov_event_async_unset(vocs->async, uuid);
    if (!adata.value) goto error;

    data = ov_socket_storage_get(vocs->connections, adata.socket);

    const char *client_id =
        ov_json_string_get(ov_json_object_get(adata.value, OV_KEY_CLIENT));

    const char *user = ov_json_string_get(
        ov_json_get(adata.value, "/" OV_KEY_PARAMETER "/" OV_KEY_USER));

    const char *session_id = NULL;

    switch (result) {

        case OV_LDAP_AUTH_REJECTED:

            ov_log_error(
                "LDAP AUTHENTICATE failed at %i | %s", adata.socket, user);

            send_error_response(vocs,
                                adata.value,
                                adata.socket,
                                OV_ERROR_CODE_AUTH,
                                OV_ERROR_DESC_AUTH);

            drop_connection(vocs, adata.socket, true, true);
            goto error;
            break;

        case OV_LDAP_AUTH_GRANTED:
            ov_log_info(
                "LDAP AUTHENTICATE granted at %i | %s", adata.socket, user);
            break;
    }

    session_id = ov_event_session_init(vocs->user_sessions, client_id, user);

    /* successful authenticated */

    if (!ov_broadcast_registry_set(
            vocs->broadcasts, user, adata.socket, OV_USER_BROADCAST)) {
        goto error;
    }

    if (!ov_broadcast_registry_set(vocs->broadcasts,
                                   OV_BROADCAST_KEY_SYSTEM_BROADCAST,
                                   adata.socket,
                                   OV_SYSTEM_BROADCAST)) {
        goto error;
    }

    ov_json_object_set(data, OV_KEY_CLIENT, ov_json_string(client_id));
    ov_json_object_set(data, OV_KEY_USER, ov_json_string(user));

    out = ov_json_object();
    if (!ov_vocs_json_set_id(out, user)) goto error;
    if (!ov_vocs_json_set_session_id(out, session_id)) goto error;

    result = send_success_response(vocs, adata.value, adata.socket, &out);
    if (result) {
        ov_log_info("VOCS AUTHENTICATE LDAP at %i | %s", adata.socket, user);
    } else {
        ov_log_error(
            "VOCS AUTHENTICATE LDAP failed at %i | %s", adata.socket, user);
    }

error:
    ov_json_value_free(out);
    ov_event_async_data_clear(&adata);    
    return;
}

/*----------------------------------------------------------------------------*/

static void client_login(void *userdata, const char *name,
                                            int socket, const ov_json_value *input) {

    ov_json_value *data = NULL;
    ov_json_value *out = NULL;

    bool result = false;
    ov_vocs *vocs = ov_vocs_cast(userdata);
    UNUSED(name);

    if (!vocs || !input || socket < 0) goto error;

    data = ov_socket_storage_get(vocs->connections, socket);
    if(!data) goto error;

    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));

    if (user) {

        send_error_response(vocs,
                            input,
                            socket,
                            OV_ERROR_CODE_ALREADY_AUTHENTICATED,
                            OV_ERROR_DESC_ALREADY_AUTHENTICATED);

        goto error;
    }

    const char *session_id = NULL;
    const char *pass = NULL;

    const char *uuid = ov_event_api_get_uuid(input);
    if (!uuid) goto error;

    const char *client_id =
        ov_json_string_get(ov_json_object_get(input, OV_KEY_CLIENT));

    const char *session_user =
        ov_event_session_get_user(vocs->user_sessions, client_id);

    user = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_USER));
    pass = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_PASSWORD));

    if (session_user && user && pass) {

        if (ov_event_session_verify(
                vocs->user_sessions, client_id, user, pass)) {

            ov_log_debug("relogin session for client %s", client_id);

            goto login_session_user;
        }
    }

    if (!user || !pass) {

        send_error_response(vocs,
                            input,
                            socket,
                            OV_ERROR_CODE_PARAMETER_ERROR,
                            OV_ERROR_DESC_PARAMETER_ERROR);

        goto error;
    }

    if (vocs->config.ldap.enable) {

        ov_log_debug("Requesting LDAP authentication for user %s", user);

        if (!set_async(vocs, uuid, socket, input)) goto error;

        ov_ldap_authenticate_password(
            vocs->ldap,
            user,
            pass,
            uuid,
            (ov_ldap_auth_callback){
                .userdata = vocs, .callback = cb_ldap_authentication});

        return;
    }

    if (!ov_vocs_db_authenticate(vocs->config.db, user, pass)) {

        send_error_response(
            vocs, input, socket, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        ov_log_error("VOCS AUTHENTICATE local failed at %i | %s", socket, user);
        goto error;
    }

login_session_user:

    session_id = ov_event_session_init(vocs->user_sessions, client_id, user);

    /* successful authenticated */

    if (!ov_broadcast_registry_set(
            vocs->broadcasts, user, socket, OV_USER_BROADCAST)) {
        goto error;
    }

    if (!ov_broadcast_registry_set(vocs->broadcasts,
                                   OV_BROADCAST_KEY_SYSTEM_BROADCAST,
                                   socket,
                                   OV_SYSTEM_BROADCAST)) {
        goto error;
    }

    ov_json_object_set(data, OV_KEY_CLIENT, ov_json_string(client_id));
    ov_json_object_set(data, OV_KEY_USER, ov_json_string(user));

    out = ov_json_object();
    if (!ov_vocs_json_set_id(out, user)) goto error;
    if (!ov_vocs_json_set_session_id(out, session_id)) goto error;

    result = send_success_response(vocs, input, socket, &out);
    if (result) {
        ov_log_info("VOCS AUTHENTICATE local at %i | %s", socket, user);
    } else {
        ov_log_error("VOCS AUTHENTICATE local failed at %i | %s", socket, user);
    }

    /* close socket if authentication messaging failed */
error:
    ov_json_value_free(out);
    return;
}

/*----------------------------------------------------------------------------*/

static void update_client_login(void *userdata, const char *name,
                                            int socket, const ov_json_value *input) {

    ov_json_value *out = NULL;

    ov_vocs *vocs = ov_vocs_cast(userdata);
    UNUSED(name);

    if (!vocs || !input || socket < 0) goto error;

    const char *client_id =
        ov_json_string_get(ov_json_object_get(input, OV_KEY_CLIENT));

    const char *session_id = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_SESSION));

    const char *user_id = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_USER));

    if (!client_id || !session_id || !user_id) {

        send_error_response(vocs,
                            input,
                            socket,
                            OV_ERROR_CODE_PARAMETER_ERROR,
                            OV_ERROR_DESC_PARAMETER_ERROR);

        goto error;
    }

    if (!ov_event_session_update(
            vocs->user_sessions, client_id, user_id, session_id)) {

        send_error_response(vocs,
                            input,
                            socket,
                            OV_ERROR_CODE_PROCESSING_ERROR,
                            OV_ERROR_DESC_PROCESSING_ERROR);

        goto error;
    }

    out = ov_json_object();
    if (!ov_vocs_json_set_id(
            out, ov_event_session_get_user(vocs->user_sessions, client_id)))
        goto error;

    if (!ov_vocs_json_set_session_id(out, session_id)) goto error;

    send_success_response(vocs, input, socket, &out);
    return;

    /* close socket if authentication messaging failed */
error:
    ov_json_value_free(out);
    return;
}

/*----------------------------------------------------------------------------*/

static void check_ldap_enabled(void *userdata, const char *name,
                            int socket, const ov_json_value *input) {

    ov_json_value *val = NULL;

    ov_vocs *vocs = ov_vocs_cast(userdata);
    UNUSED(name);

    if (!vocs || !input || socket < 0) goto error;

    if (vocs->config.ldap.enable) {
        val = ov_json_true();
    } else {
        val = ov_json_false();
    }

    send_success_response(vocs, input, socket, &val);
    return;

error:
    ov_json_value_free(val);
    return;
}

/*----------------------------------------------------------------------------*/

static bool enable_api_auth(ov_vocs *vocs){

    if (!ov_event_broker_register(
        vocs->broker,
        OV_EVENT_API_LOGOUT,
        vocs,
        client_logout)) goto error;

    if (!ov_event_broker_register(
        vocs->broker,
        OV_EVENT_API_LOGIN,
        vocs,
        client_login)) goto error;

    if (!ov_event_broker_register(
        vocs->broker,
        OV_EVENT_API_AUTHENTICATE,
        vocs,
        client_login)) goto error;

    if (!ov_event_broker_register(
        vocs->broker,
        OV_EVENT_API_UPDATE_LOGIN,
        vocs,
        update_client_login)) goto error;

    if (!ov_event_broker_register(
        vocs->broker,
        "is_ldap_enabled",
        vocs,
        check_ldap_enabled)) goto error;

    return true;
error:
    return false;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      #API CLIENT EVENTS
 *
 *      ------------------------------------------------------------------------
 */

/*----------------------------------------------------------------------------*/

static void client_media(void *userdata, const char *name,
        int socket, const ov_json_value *input) {

    ov_json_value *data = NULL;

    UNUSED(name);
    ov_vocs *vocs = ov_vocs_cast(userdata);

    if (!vocs || !input || socket < 0) goto error;

    data = ov_socket_storage_get(vocs->connections, socket);
    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));
    const char *session =
        ov_json_string_get(ov_json_get(data, "/" OV_KEY_SESSION));

    if (!user) goto error;

    const char *uuid = ov_event_api_get_uuid(input);
    const char *sdp = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_SDP));

    ov_media_type type = ov_media_type_from_string(ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_TYPE)));

    if (!uuid) {

        send_error_response(vocs,
                            input,
                            socket,
                            OV_ERROR_CODE_PARAMETER_ERROR,
                            OV_ERROR_DESC_PARAMETER_ERROR);

        goto error;
    }

    switch (type) {

        case OV_MEDIA_REQUEST:

            if (!ov_mc_frontend_create_session(
                    vocs->frontend, uuid, OV_VOCS_DEFAULT_SDP)) {

                send_error_response(vocs,
                                    input,
                                    socket,
                                    OV_ERROR_CODE_PROCESSING_ERROR,
                                    OV_ERROR_DESC_PROCESSING_ERROR);

                goto error;
            }

            break;

        case OV_MEDIA_OFFER:
        case OV_MEDIA_ANSWER:

            if (!sdp) {

                send_error_response(vocs,
                                    input,
                                    socket,
                                    OV_ERROR_CODE_PARAMETER_ERROR,
                                    OV_ERROR_DESC_PARAMETER_ERROR);

                goto error;
            }

            if (!session) {

                send_error_response(vocs,
                                    input,
                                    socket,
                                    OV_ERROR_CODE_SESSION_UNKNOWN,
                                    OV_ERROR_DESC_SESSION_UNKNOWN);

                goto error;
            }

            if (!ov_mc_frontend_update_session(
                    vocs->frontend, uuid, session, type, sdp)) {

                send_error_response(vocs,
                                    input,
                                    socket,
                                    OV_ERROR_CODE_PROCESSING_ERROR,
                                    OV_ERROR_DESC_PROCESSING_ERROR);

                goto error;
            }

            break;

        default:

            send_error_response(vocs,
                                input,
                                socket,
                                OV_ERROR_CODE_PARAMETER_ERROR,
                                OV_ERROR_DESC_PARAMETER_ERROR);

            goto error;
    }

    if (!set_async(vocs, uuid, socket, input)) goto error;

    return;
error:
    return;
}

/*----------------------------------------------------------------------------*/

static void client_candidate(void *userdata, const char *name,
        int socket, const ov_json_value *input) {

    ov_json_value *data = NULL;

    UNUSED(name);
    ov_vocs *vocs = ov_vocs_cast(userdata);

    if (!vocs || !input || socket < 0) goto error;

    data = ov_socket_storage_get(vocs->connections, socket);
    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));
    const char *session =
        ov_json_string_get(ov_json_get(data, "/" OV_KEY_SESSION));

    if (!user) goto error;

    const char *uuid = ov_event_api_get_uuid(input);
    ov_json_value *par = ov_event_api_get_parameter(input);
    ov_ice_candidate_info info = ov_ice_candidate_info_from_json(par);

    if (!uuid || !info.candidate || !info.ufrag) {

        send_error_response(vocs,
                            input,
                            socket,
                            OV_ERROR_CODE_PARAMETER_ERROR,
                            OV_ERROR_DESC_PARAMETER_ERROR);

        goto error;
    }

    if (NULL == session) {

        send_error_response(vocs,
                            input,
                            socket,
                            OV_ERROR_CODE_SESSION_UNKNOWN,
                            OV_ERROR_DESC_SESSION_UNKNOWN);

        goto error;
    }

    if (!ov_mc_frontend_candidate(vocs->frontend, uuid, session, &info)) {

        send_error_response(vocs,
                            input,
                            socket,
                            OV_ERROR_CODE_PROCESSING_ERROR,
                            OV_ERROR_DESC_PROCESSING_ERROR);

        goto error;
    }

    if (!set_async(vocs, uuid, socket, input)) goto error;
    return;

error:
    return;
}

/*----------------------------------------------------------------------------*/

static void client_end_of_candidates(void *userdata, const char *name,
        int socket, const ov_json_value *input) {

    ov_json_value *data = NULL;

    UNUSED(name);
    ov_vocs *vocs = ov_vocs_cast(userdata);

    if (!vocs || !input || socket < 0) goto error;

    data = ov_socket_storage_get(vocs->connections, socket);
    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));
    const char *session =
        ov_json_string_get(ov_json_get(data, "/" OV_KEY_SESSION));

    if (!user) goto error;

    const char *uuid = ov_event_api_get_uuid(input);

    if (!uuid) {

        send_error_response(vocs,
                            input,
                            socket,
                            OV_ERROR_CODE_PARAMETER_ERROR,
                            OV_ERROR_DESC_PARAMETER_ERROR);

        goto error;
    }

    if (NULL == session) {

        send_error_response(vocs,
                            input,
                            socket,
                            OV_ERROR_CODE_SESSION_UNKNOWN,
                            OV_ERROR_DESC_SESSION_UNKNOWN);

        goto error;
    }

    if (!ov_mc_frontend_end_of_candidates(vocs->frontend, uuid, session)) {

        send_error_response(vocs,
                            input,
                            socket,
                            OV_ERROR_CODE_PROCESSING_ERROR,
                            OV_ERROR_DESC_PROCESSING_ERROR);

        goto error;
    }

    if (!set_async(vocs, uuid, socket, input)) goto error;
    return;

error:
    return ;
}

/*----------------------------------------------------------------------------*/

static void client_authorize(void *userdata, const char *name,
        int socket, const ov_json_value *input) {

    ov_json_value *data = NULL;
    ov_json_value *out = NULL;
    bool result = false;

    UNUSED(name);
    ov_vocs *vocs = ov_vocs_cast(userdata);

    if (!vocs || !input || socket < 0) goto error;

    data = ov_socket_storage_get(vocs->connections, socket);
    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));
    const char *role = ov_json_string_get(ov_json_get(data, "/" OV_KEY_ROLE));

    if (!user) goto error;

    if (role) {

        send_error_response(vocs,
                            input,
                            socket,
                            OV_ERROR_CODE_NOT_IMPLEMENTED,
                            "current status: "
                            "changing a role MUST be done using logout/login.");

        goto error;
    }

    role = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_ROLE));

    if (!role) {

        send_error_response(vocs,
                            input,
                            socket,
                            OV_ERROR_CODE_PARAMETER_ERROR,
                            OV_ERROR_DESC_PARAMETER_ERROR);

        goto error;
    }

    /* process authorization request */

    if (!ov_vocs_db_authorize(vocs->config.db, user, role)) {

        send_error_response(
            vocs, input, socket, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto error;
    }

    ov_json_object_set(data, OV_KEY_ROLE, ov_json_string(role));

    if (!ov_broadcast_registry_set(
            vocs->broadcasts, role, socket, OV_ROLE_BROADCAST)) {
        goto error;
    }

    out = ov_json_object();
    if (!ov_vocs_json_set_id(out, role)) goto error;

    result = send_success_response(vocs, input, socket, &out);
    if (result) {
        ov_log_info("VOCS AUTHORIZE at %i | %s | %s", socket, user, role);
    } else {
        ov_log_error("VOCS AUTHORIZE failed at %i | %s", socket, user);
    }
error:
    ov_json_value_free(out);
    return ;
}

/*----------------------------------------------------------------------------*/

static void client_get(void *userdata, const char *name,
        int socket, const ov_json_value *input) {

    ov_json_value *val = NULL;
    ov_json_value *out = NULL;
    ov_json_value *data = NULL;

    UNUSED(name);
    ov_vocs *vocs = ov_vocs_cast(userdata);
    
    if (!vocs || !input || socket < 0) goto error;

    data = ov_socket_storage_get(vocs->connections, socket);
    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));

    if (!user) {

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_AUTH,
                                                 OV_ERROR_DESC_AUTH);

        goto response;
    }
    
    const char *id = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_ID));

    const char *type = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_TYPE));

    if (!id || !type) {

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_PARAMETER_ERROR,
                                                 OV_ERROR_DESC_PARAMETER_ERROR);

        goto response;
    }

    ov_vocs_db_entity entity = ov_vocs_db_entity_from_string(type);
    if (OV_VOCS_DB_ENTITY_ERROR == entity) {

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_PARAMETER_ERROR,
                                                 OV_ERROR_DESC_PARAMETER_ERROR);

        goto response;
    }

    /* all users are allowed to get anything */

    ov_json_value *result_obj = NULL;

    if (0 == strcmp(type, OV_KEY_USER)) {


        result_obj = ov_vocs_db_get_entity(vocs->config.db, OV_VOCS_DB_USER, user);

        val = ov_vocs_db_get_entity_domain(
            vocs->config.db, OV_VOCS_DB_USER, user);

        if (val) ov_json_object_set(result_obj, "origin", val);

        val = NULL;

    } else  {

        val = ov_vocs_db_get_entity(vocs->config.db, entity, id);
        if (!val) {
            val = ov_json_null();
        }

        result_obj = val;
    }

    out = ov_event_api_create_success_response(input);
    ov_json_value *res = ov_event_api_get_response(out);
    if (!ov_json_object_set(res, "data", result_obj)) goto error;

    val = ov_json_string(type);
    if (!ov_json_object_set(res, OV_KEY_TYPE, val)) goto error;

    val = NULL;

response:

    env_send(vocs, socket, out);
    out = ov_json_value_free(out);
    return ;
error:
    val = ov_json_value_free(val);
    out = ov_json_value_free(out);
    return ;
}

/*----------------------------------------------------------------------------*/

static void client_user_roles(void *userdata, const char *name,
        int socket, const ov_json_value *input) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;

    ov_json_value *data = NULL;
    UNUSED(name);
    ov_vocs *vocs = ov_vocs_cast(userdata);

    if (!vocs || !input || socket < 0) goto error;

    data = ov_socket_storage_get(vocs->connections, socket);
    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));

    if (!user) goto error;

    val = ov_vocs_db_get_user_roles(vocs->config.db, user);

    if (!val) {

        send_error_response(
            vocs, input, socket, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto error;
    }

    out = ov_json_object();
    if (!ov_json_object_set(out, OV_KEY_ROLES, val)) goto error;

    val = NULL;

    send_success_response(vocs, input, socket, &out);

error:
    ov_json_value_free(val);
    ov_json_value_free(out);
    return ;
}

/*----------------------------------------------------------------------------*/

struct container_loop_broadcasts {

    ov_vocs *vocs;
    int socket;

    const char *project;
    const char *user;
    const char *role;
};

/*----------------------------------------------------------------------------*/

static bool add_loop_broadcast_and_state(const void *key,
                                         void *val,
                                         void *data) {

    if (!key) return true;

    UNUSED(val);

    char *name = (char *)key;
    struct container_loop_broadcasts *c =
        (struct container_loop_broadcasts *)data;

    return ov_broadcast_registry_set(
        c->vocs->broadcasts, name, c->socket, OV_LOOP_BROADCAST);
}

/*----------------------------------------------------------------------------*/

static void client_role_loops(void *userdata, const char *name,
        int socket, const ov_json_value *input) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *data = NULL;

    UNUSED(name);
    ov_vocs *vocs = ov_vocs_cast(userdata);

    if (!vocs || !input || socket < 0) goto error;

    data = ov_socket_storage_get(vocs->connections, socket);
    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));
    const char *role = ov_json_string_get(ov_json_get(data, "/" OV_KEY_ROLE));

    if (!user || !role) {

        send_error_response(
            vocs, input, socket, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto error;
    }

    val = ov_vocs_db_get_user_role_loops(vocs->config.db, user, role);

    if (!val) {

        send_error_response(
            vocs, input, socket, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto error;
    }

    struct container_loop_broadcasts c = (struct container_loop_broadcasts){
        .vocs = vocs, .socket = socket, .user = user, .role = role};

    if (!ov_json_object_for_each(val, &c, add_loop_broadcast_and_state))
        goto error;

    out = ov_json_object();
    if (!ov_json_object_set(out, OV_KEY_LOOPS, val)) goto error;

    val = NULL;

    send_success_response(vocs, input, socket, &out);

error:
    ov_json_value_free(val);
    ov_json_value_free(out);
    return;
}

/*----------------------------------------------------------------------------*/

static void client_event_get_recording(void *userdata, const char *name,
        int socket, const ov_json_value *input) {

    ov_json_value *val = NULL;
    ov_json_value *data = NULL;

    UNUSED(name);
    ov_vocs *vocs = ov_vocs_cast(userdata);

    if (!vocs || !input || socket < 0) goto error;

    data = ov_socket_storage_get(vocs->connections, socket);
    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));
    //const char *role = ov_json_string_get(ov_json_get(data, "/" OV_KEY_ROLE));

    if (!user) {

        send_error_response(
            vocs, input, socket, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto error;
    }

    ov_db_recordings_get_params db_params = {0};

    db_params.loop = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_LOOP));
    db_params.user = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_USER));
    db_params.from_epoch_secs = ov_json_number_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_FROM));
    db_params.until_epoch_secs = ov_json_number_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_TO));

    val = ov_vocs_recorder_get_recording(vocs->recorder, db_params);
    if (!val) {

        val = ov_json_array();
        send_success_response(vocs, input, socket, &val);

    } else if (OV_DB_RECORDINGS_RESULT_TOO_BIG == val) {

        send_error_response(vocs,
                            input,
                            socket,
                            OV_ERROR_CODE_PROCESSING_ERROR,
                            "Search returned too many results - please confine "
                            "your search parameters");

    } else {
        send_success_response(vocs, input, socket, &val);
    }

error:
    ov_json_value_free(val);
    return;
}

/*----------------------------------------------------------------------------*/

static void client_event_sip_status(void *userdata, const char *name,
        int socket, const ov_json_value *input) {

    ov_json_value *val = NULL;
    ov_json_value *data = NULL;

    UNUSED(name);
    ov_vocs *vocs = ov_vocs_cast(userdata);

    if (!vocs || !input || socket < 0) goto error;

    data = ov_socket_storage_get(vocs->connections, socket);
    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));

    if (!user) {

        send_error_response(
            vocs, input, socket, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto error;
    }

    bool status = ov_mc_backend_sip_get_connect_status(vocs->sip);

    if (status) {
        val = ov_json_true();
    } else {
        val = ov_json_false();
    }

    send_success_response(vocs, input, socket, &val);

error:
    return;
}

/*----------------------------------------------------------------------------*/

static void client_switch_loop_state(void *userdata, const char *name,
        int socket, const ov_json_value *input) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *data = NULL;

    UNUSED(name);
    ov_vocs *vocs = ov_vocs_cast(userdata);

    if (!vocs || !input || socket < 0) goto error;

    data = ov_socket_storage_get(vocs->connections, socket);
    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));
    const char *sess =
        ov_json_string_get(ov_json_get(data, "/" OV_KEY_SESSION));
    const char *role = ov_json_string_get(ov_json_get(data, "/" OV_KEY_ROLE));

    if (!user || !role) {

        send_error_response(
            vocs, input, socket, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto error;
    }

    if (NULL == sess) {

        send_error_response(vocs,
                            input,
                            socket,
                            OV_ERROR_CODE_SESSION_UNKNOWN,
                            OV_ERROR_DESC_SESSION_UNKNOWN);

        goto error;
    }

    const char *uuid = ov_event_api_get_uuid(input);
    const char *loop = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_LOOP));
    const char *state = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_STATE));

    if (!uuid || !loop || !state) {

        send_error_response(vocs,
                            input,
                            socket,
                            OV_ERROR_CODE_PARAMETER_ERROR,
                            OV_ERROR_DESC_PARAMETER_ERROR);

        goto error;
    }

    if (!(ov_json_is_true(ov_json_get(data, "/" OV_KEY_ICE)) &&
          (ov_json_is_true(ov_json_get(data, "/" OV_KEY_MEDIA_READY)))))
        goto error;

    ov_vocs_permission requested = ov_vocs_permission_from_string(state);
    ov_vocs_permission permission =
        ov_vocs_db_get_permission(vocs->config.db, role, loop);

    if (!ov_vocs_permission_granted(permission, requested)) {

        send_error_response(vocs,
                            input,
                            socket,
                            OV_ERROR_CODE_AUTH_PERMISSION,
                            OV_ERROR_DESC_AUTH_PERMISSION);

        goto error;
    }

    const ov_json_value *loops = ov_json_get(data, "/" OV_KEY_LOOPS);

    ov_vocs_permission current = ov_vocs_permission_from_string(
        ov_json_string_get(ov_json_object_get(loops, loop)));

    if (current == requested) {

        ov_vocs_loop *l = ov_dict_get(vocs->loops, loop);

        if (!l) {

            l = ov_vocs_loop_create(loop);
            char *name = ov_string_dup(loop);

            if (!ov_dict_set(vocs->loops, name, l, NULL)) {
                l = ov_vocs_loop_free(l);
                name = ov_data_pointer_free(name);
                goto error;
            }
        }

        ov_json_value *participants = ov_vocs_loop_get_participants(l);

        out = ov_json_object();

        if (!ov_json_object_set(out, OV_KEY_PARTICIPANTS, participants))
            goto error;

        val = ov_json_string(ov_vocs_permission_to_string(current));
        if (!ov_json_object_set(out, OV_KEY_STATE, val)) goto error;

        val = ov_json_string(loop);
        if (!ov_json_object_set(out, OV_KEY_LOOP, val)) goto error;

        val = NULL;

        send_success_response(vocs, input, socket, &out);

        out = ov_json_value_free(out);
        goto done;
    }

    if (!perform_switch_loop_request(
            vocs, uuid, sess, user, role, loop, current, requested)) {

        send_error_response(vocs,
                            input,
                            socket,
                            OV_ERROR_CODE_PROCESSING_ERROR,
                            OV_ERROR_DESC_PROCESSING_ERROR);

        goto error;
    }

    if (!set_async(vocs, uuid, socket, input)) goto error;

done:
    return;

error:
    /* let socket close in case of switch media errors */
    out = ov_json_value_free(out);
    val = ov_json_value_free(val);
    return;
}

/*----------------------------------------------------------------------------*/

static void client_switch_loop_volume(void *userdata, const char *name,
        int socket, const ov_json_value *input) {

    ov_json_value *data = NULL;

    UNUSED(name);
    ov_vocs *vocs = ov_vocs_cast(userdata);

    if (!vocs || !input || socket < 0) goto error;

    data = ov_socket_storage_get(vocs->connections, socket);

    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));
    const char *role = ov_json_string_get(ov_json_get(data, "/" OV_KEY_ROLE));
    const char *sess =
        ov_json_string_get(ov_json_get(data, "/" OV_KEY_SESSION));

    if (!user || !role) {

        send_error_response(
            vocs, input, socket, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto error;
    }

    const char *uuid = ov_event_api_get_uuid(input);
    const char *loop = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_LOOP));
    uint64_t percent = ov_json_number_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_VOLUME));

    if (!uuid || !loop || (percent > 100)) {

        send_error_response(vocs,
                            input,
                            socket,
                            OV_ERROR_CODE_PARAMETER_ERROR,
                            OV_ERROR_DESC_PARAMETER_ERROR);

        goto error;
    }

    if (NULL == sess) {

        send_error_response(vocs,
                            input,
                            socket,
                            OV_ERROR_CODE_SESSION_UNKNOWN,
                            OV_ERROR_DESC_SESSION_UNKNOWN);

        goto error;
    }

    if (!ov_mc_backend_set_loop_volume(vocs->backend,
                                       uuid,
                                       sess,
                                       loop,
                                       percent,
                                       vocs,
                                       cb_backend_mixer_volume)) {

        send_error_response(vocs,
                            input,
                            socket,
                            OV_ERROR_CODE_PROCESSING_ERROR,
                            OV_ERROR_DESC_PROCESSING_ERROR);

        goto error;
    }

    if (!set_async(vocs, uuid, socket, input)) goto error;

    send_switch_volume_user_broadcast(vocs, socket, loop, percent);

    return;
error:
    return;
}

/*----------------------------------------------------------------------------*/

static void client_talking(void *userdata, const char *name,
        int socket, const ov_json_value *input) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *data = NULL;

    UNUSED(name);
    ov_vocs *vocs = ov_vocs_cast(userdata);

    if (!vocs || !input || socket < 0) goto error;

    data = ov_socket_storage_get(vocs->connections, socket);

    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));
    const char *role = ov_json_string_get(ov_json_get(data, "/" OV_KEY_ROLE));

    if (!user || !role) {

        send_error_response(
            vocs, input, socket, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto error;
    }

    const char *uuid = ov_event_api_get_uuid(input);
    const char *loop = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_LOOP));
    ov_json_value const *state =
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_STATE);

    bool off = ov_json_is_false(state);

    const char *client_id =
        ov_json_string_get(ov_json_get(input, "/" OV_KEY_CLIENT));

    ov_vocs_recorder_ptt(vocs->recorder, user, role, loop, off);

    if (!uuid || !loop || !state) {

        send_error_response(vocs,
                            input,
                            socket,
                            OV_ERROR_CODE_PARAMETER_ERROR,
                            OV_ERROR_DESC_PARAMETER_ERROR);

        goto error;
    }

    ov_vocs_permission permission =
        ov_vocs_db_get_permission(vocs->config.db, role, loop);

    if (permission != OV_VOCS_SEND) {

        send_error_response(vocs,
                            input,
                            socket,
                            OV_ERROR_CODE_AUTH_PERMISSION,
                            OV_ERROR_DESC_AUTH_PERMISSION);

        goto error;
    }

    out = ov_json_object();
    val = ov_json_string(user);
    if (!ov_json_object_set(out, OV_KEY_USER, val)) goto error;

    val = ov_json_string(role);
    if (!ov_json_object_set(out, OV_KEY_ROLE, val)) goto error;

    val = ov_json_string(loop);
    if (!ov_json_object_set(out, OV_KEY_LOOP, val)) goto error;

    val = NULL;
    if (!ov_json_value_copy((void **)&val, state)) goto error;

    if (!ov_json_object_set(out, OV_KEY_STATE, val)) goto error;

    val = NULL;

    send_success_response(vocs, input, socket, &out);

    if (!send_talking_loop_broadcast(
            vocs, socket, loop, state, client_id)) {

        ov_log_error(
            "Failed to send talking broadcast at %s from %s", loop, user);
    }

error:
    /* Do not close socket in case of broadcasting errors */
    ov_json_value_free(val);
    ov_json_value_free(out);
    return;
}

/*----------------------------------------------------------------------------*/

static void client_calling(void *userdata, const char *name,
        int socket, const ov_json_value *input) {

    ov_json_value *data = NULL;

    char *uuid_request = NULL;

    UNUSED(name);
    ov_vocs *vocs = ov_vocs_cast(userdata);

    if (!vocs || !input || socket < 0) goto error;

    data = ov_socket_storage_get(vocs->connections, socket);

    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));
    const char *role = ov_json_string_get(ov_json_get(data, "/" OV_KEY_ROLE));

    if (!user || !role) {

        send_error_response(
            vocs, input, socket, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto error;
    }

    const char *uuid = ov_event_api_get_uuid(input);
    const char *loop = ov_json_string_get(
        (ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_LOOP)));
    const char *dest = ov_json_string_get(
        (ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_DESTINATION)));
    const char *from = ov_json_string_get(
        (ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_FROM)));

    if (!uuid || !loop || !dest) {

        send_error_response(vocs,
                            input,
                            socket,
                            OV_ERROR_CODE_PARAMETER_ERROR,
                            OV_ERROR_DESC_PARAMETER_ERROR);

        goto error;
    }

    if (!ov_vocs_db_sip_allow_callout(vocs->config.db, loop, role)) {

        send_error_response(
            vocs, input, socket, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto error;
    }

    uuid_request = ov_mc_backend_sip_create_call(vocs->sip, loop, dest, from);

    if (!uuid_request) {

        send_error_response(vocs,
                            input,
                            socket,
                            OV_ERROR_CODE_PROCESSING_ERROR,
                            OV_ERROR_DESC_PROCESSING_ERROR);
        goto error;
    }

    if (!set_async(vocs, uuid, socket, input)) goto error;

error:
    uuid_request = ov_data_pointer_free(uuid_request);
    return;
}

/*----------------------------------------------------------------------------*/

static void client_hangup(void *userdata, const char *name,
        int socket, const ov_json_value *input) {

    ov_json_value *data = NULL;

    UNUSED(name);
    ov_vocs *vocs = ov_vocs_cast(userdata);

    if (!vocs || !input || socket < 0) goto error;

    data = ov_socket_storage_get(vocs->connections, socket);

    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));
    const char *role = ov_json_string_get(ov_json_get(data, "/" OV_KEY_ROLE));

    if (!user || !role) {

        send_error_response(
            vocs, input, socket, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto error;
    }

    const char *uuid = ov_event_api_get_uuid(input);
    const char *call_id = ov_json_string_get(
        (ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_CALL)));
    const char *loop = ov_json_string_get(
        (ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_LOOP)));

    if (!uuid || !call_id) {

        send_error_response(vocs,
                            input,
                            socket,
                            OV_ERROR_CODE_PARAMETER_ERROR,
                            OV_ERROR_DESC_PARAMETER_ERROR);

        goto error;
    }

    if (!ov_vocs_db_sip_allow_callend(vocs->config.db, loop, role)) {

        send_error_response(
            vocs, input, socket, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto error;
    }

    if (ov_mc_backend_sip_terminate_call(vocs->sip, call_id)) {

        send_success_response(vocs, input, socket, NULL);

    } else {

        send_error_response(vocs,
                            input,
                            socket,
                            OV_ERROR_CODE_PROCESSING_ERROR,
                            OV_ERROR_DESC_PROCESSING_ERROR);
    }

error:
    return;
}

/*----------------------------------------------------------------------------*/

static void client_permit_call(void *userdata, const char *name,
        int socket, const ov_json_value *input) {

    ov_json_value *data = NULL;

    UNUSED(name);
    ov_vocs *vocs = ov_vocs_cast(userdata);

    if (!vocs || !input || socket < 0) goto error;

    data = ov_socket_storage_get(vocs->connections, socket);

    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));
    const char *role = ov_json_string_get(ov_json_get(data, "/" OV_KEY_ROLE));

    if (!user || !role) {

        send_error_response(
            vocs, input, socket, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto error;
    }

    bool ok = true;

    const char *uuid = ov_event_api_get_uuid(input);
    ov_sip_permission permission = ov_sip_permission_from_json(
        ov_json_get(input, "/" OV_KEY_PARAMETER), &ok);

    if (!uuid || !ok) {

        send_error_response(vocs,
                            input,
                            socket,
                            OV_ERROR_CODE_PARAMETER_ERROR,
                            OV_ERROR_DESC_PARAMETER_ERROR);

        goto error;
    }

    if (ov_mc_backend_sip_create_permission(vocs->sip, permission)) {

        send_success_response(vocs, input, socket, NULL);

    } else {

        send_error_response(vocs,
                            input,
                            socket,
                            OV_ERROR_CODE_PROCESSING_ERROR,
                            OV_ERROR_DESC_PROCESSING_ERROR);
    }

error:
    return;
}

/*----------------------------------------------------------------------------*/

static void client_revoke_call(void *userdata, const char *name,
        int socket, const ov_json_value *input) {

    ov_json_value *data = NULL;

    UNUSED(name);
    ov_vocs *vocs = ov_vocs_cast(userdata);

    if (!vocs || !input || socket < 0) goto error;

    data = ov_socket_storage_get(vocs->connections, socket);

    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));
    const char *role = ov_json_string_get(ov_json_get(data, "/" OV_KEY_ROLE));

    if (!user || !role) {

        send_error_response(
            vocs, input, socket, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto error;
    }

    bool ok = true;

    const char *uuid = ov_event_api_get_uuid(input);
    ov_sip_permission permission = ov_sip_permission_from_json(
        ov_json_get(input, "/" OV_KEY_PARAMETER), &ok);

    if (!uuid || !ok) {

        send_error_response(vocs,
                            input,
                            socket,
                            OV_ERROR_CODE_PARAMETER_ERROR,
                            OV_ERROR_DESC_PARAMETER_ERROR);

        goto error;
    }

    if (ov_mc_backend_sip_terminate_permission(vocs->sip, permission)) {

        send_success_response(vocs, input, socket, NULL);

    } else {

        send_error_response(vocs,
                            input,
                            socket,
                            OV_ERROR_CODE_PROCESSING_ERROR,
                            OV_ERROR_DESC_PROCESSING_ERROR);
    }

error:
    return;
}

/*----------------------------------------------------------------------------*/

static bool client_call_function(ov_vocs *vocs,
                                 int socket,
                                 const ov_json_value *input,
                                 bool (*function)(ov_mc_backend_sip *sip,
                                                  const char *uuid)) {

    ov_json_value *data = NULL;

    if (!vocs || !input || socket < 0) goto error;

    data = ov_socket_storage_get(vocs->connections, socket);

    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));
    const char *role = ov_json_string_get(ov_json_get(data, "/" OV_KEY_ROLE));
    const char *uuid = ov_event_api_get_uuid(input);

    if (!user || !role || !uuid) {

        send_error_response(
            vocs, input, socket, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto error;
    }

    set_async_sip(vocs, uuid, socket, input);

    if (!function(vocs->sip, uuid)) {

        ov_event_async_data adata = ov_event_async_unset(vocs->async, uuid);

        send_error_response(vocs,
                            adata.value,
                            adata.socket,
                            OV_ERROR_CODE_PROCESSING_ERROR,
                            OV_ERROR_DESC_PROCESSING_ERROR);

        adata.value = ov_json_value_free(adata.value);

        goto error;
    }

    return true;

error:
    return false;
}

/*----------------------------------------------------------------------------*/

static void client_list_calls(void *userdata, const char *name,
        int socket, const ov_json_value *input) {

    UNUSED(name);
    ov_vocs *vocs = ov_vocs_cast(userdata);

    client_call_function(
        vocs, socket, input, ov_mc_backend_sip_list_calls);

    return;
}

/*----------------------------------------------------------------------------*/

static void client_list_call_permissions(void *userdata, const char *name,
        int socket, const ov_json_value *input) {

    UNUSED(name);
    ov_vocs *vocs = ov_vocs_cast(userdata);

    client_call_function(
        vocs, socket, input, ov_mc_backend_sip_list_permissions);

    return;
}

/*----------------------------------------------------------------------------*/

static void client_list_sip_status(void *userdata, const char *name,
        int socket, const ov_json_value *input) {

    UNUSED(name);
    ov_vocs *vocs = ov_vocs_cast(userdata);

    client_call_function(
        vocs, socket, input, ov_mc_backend_sip_get_status);

    return;
}

/*----------------------------------------------------------------------------*/

static void client_event_set_keyset_layout(void *userdata, const char *eventname,
        int socket, const ov_json_value *input) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *data = NULL;

    UNUSED(eventname);
    ov_vocs *vocs = ov_vocs_cast(userdata);

    /* checking input:

       {
           "event" : "set_keyset_layout",
           "parameter" :
           {
                "domain" : "<domainname>",
                "name"   : "<name>",
                "layout"   : {}
           }
       }

   */

    if (!vocs || !socket || !input) goto error;

    data = ov_socket_storage_get(vocs->connections, socket);

    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));

    if (!user) goto error;

    const char *domain = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_DOMAIN));

    const char *name = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_NAME));

    const ov_json_value *layout =
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_LAYOUT);

    if (!domain || !name || !layout) {

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_PARAMETER_ERROR,
                                                 OV_ERROR_DESC_PARAMETER_ERROR);

        goto response;
    }

    if (!ov_vocs_db_authorize_domain_admin(vocs->config.db, user, domain)) {

        out = ov_event_api_create_error_response(
            input, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto response;
    }

    if (!ov_vocs_db_set_keyset_layout(vocs->config.db, domain, name, layout)) {

        out =
            ov_event_api_create_error_response(input,
                                               OV_ERROR_CODE_PROCESSING_ERROR,
                                               OV_ERROR_DESC_PROCESSING_ERROR);

        goto response;
    }

    out = ov_event_api_create_success_response(input);

response:

    env_send(vocs, socket, out);
    out = ov_json_value_free(out);
    return;
error:
    val = ov_json_value_free(val);
    out = ov_json_value_free(out);
    return;
}

/*----------------------------------------------------------------------------*/

static void client_event_get_keyset_layout(void *userdata, const char *name,
        int socket, const ov_json_value *input) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *data = NULL;

    UNUSED(name);
    ov_vocs *vocs = ov_vocs_cast(userdata);

    /* checking input:

       {
           "event" : "get_keyset_layout",
           "parameter" :
           {
                "domain"   : "<domain_id>",
                "layout".  : "<layout_name>"
           }
       }

   */

    if (!vocs || !socket || !input) goto error;

    data = ov_socket_storage_get(vocs->connections, socket);

    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));

    if (!user) goto error;

    const char *domain = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_DOMAIN));

    const char *layout = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_LAYOUT));

    if (!domain || !layout) {

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_PARAMETER_ERROR,
                                                 OV_ERROR_DESC_PARAMETER_ERROR);

        goto response;
    }

    /* anyone can get the layout of some keyset */

    val = ov_vocs_db_get_keyset_layout(vocs->config.db, domain, layout);
    if (!val) {

        out =
            ov_event_api_create_error_response(input,
                                               OV_ERROR_CODE_PROCESSING_ERROR,
                                               OV_ERROR_DESC_PROCESSING_ERROR);

        goto response;
    }

    out = ov_event_api_create_success_response(input);
    ov_json_value *res = ov_event_api_get_response(out);
    ov_json_object_set(res, OV_KEY_LAYOUT, val);

response:

    env_send(vocs, socket, out);
    out = ov_json_value_free(out);
    return;
error:
    val = ov_json_value_free(val);
    out = ov_json_value_free(out);
    return;
}

/*----------------------------------------------------------------------------*/

static void client_event_set_user_data(void *userdata, const char *name,
        int socket, const ov_json_value *input) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *data = NULL;

    UNUSED(name);
    ov_vocs *vocs = ov_vocs_cast(userdata);

    if (!vocs || !socket || !input) goto error;

    data = ov_socket_storage_get(vocs->connections, socket);
    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));

    if (!user) {

        out = ov_event_api_create_error_response(
            input, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto response;
    }

    ov_json_value *fdata = ov_event_api_get_parameter(input);
    if (!fdata) {

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_PARAMETER_ERROR,
                                                 OV_ERROR_DESC_PARAMETER_ERROR);

        goto response;
    }

    if (!ov_vocs_db_set_user_data(vocs->config.db, user, fdata)) {

        out =
            ov_event_api_create_error_response(input,
                                               OV_ERROR_CODE_PROCESSING_ERROR,
                                               OV_ERROR_DESC_PROCESSING_ERROR);

        goto response;
    }

    out = ov_event_api_create_success_response(input);
    ov_json_value *res = ov_event_api_get_response(out);
    val = NULL;
    ov_json_value_copy((void **)&val, fdata);
    ov_json_object_set(res, OV_KEY_DATA, val);

    ov_event_parameter parameter =
        (ov_event_parameter){.send.instance = vocs, .send.send = send_socket};

    if (!ov_broadcast_registry_send(
            vocs->broadcasts, user, &parameter, out, OV_USER_BROADCAST))
        goto error;

response:

    env_send(vocs, socket, out);
    out = ov_json_value_free(out);
    return ;
error:
    val = ov_json_value_free(val);
    out = ov_json_value_free(out);
    return ;
}

/*----------------------------------------------------------------------------*/

static void client_event_get_user_data(void *userdata, const char *name,
        int socket, const ov_json_value *input) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *data = NULL;

    UNUSED(name);
    ov_vocs *vocs = ov_vocs_cast(userdata);

    if (!vocs || !socket || !input) goto error;

    data = ov_socket_storage_get(vocs->connections, socket);

    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));

    if (!user) {

        out = ov_event_api_create_error_response(
            input, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto response;
    }

    ov_json_value *fdata = ov_vocs_db_get_user_data(vocs->config.db, user);

    if (!fdata) {

        out =
            ov_event_api_create_error_response(input,
                                               OV_ERROR_CODE_PROCESSING_ERROR,
                                               OV_ERROR_DESC_PROCESSING_ERROR);

        goto response;
    }

    out = ov_event_api_create_success_response(input);
    ov_json_value *res = ov_event_api_get_response(out);
    ov_json_object_set(res, OV_KEY_DATA, fdata);

response:

    env_send(vocs, socket, out);
    out = ov_json_value_free(out);
    return;
error:
    val = ov_json_value_free(val);
    out = ov_json_value_free(out);
    return;
}

/*----------------------------------------------------------------------------*/

static void client_event_register(void *userdata, const char *name,
        int socket, const ov_json_value *input) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;

    UNUSED(name);
    ov_vocs *vocs = ov_vocs_cast(userdata);

    if (!vocs || !socket || !input) goto error;

    const char *uuid = ov_json_string_get(ov_json_get(input, "/client_id"));

    ov_client_registry_register(vocs->client_registry, uuid, socket);

    ov_json_value *data = ov_socket_storage_get(vocs->connections, socket);
    if (!data) data = ov_json_object();

    ov_socket_data remote ={0};
    ov_socket_get_data(socket, NULL, &remote);

    ov_json_value *sock = ov_socket_data_to_json(&remote);
    ov_json_object_set(data, "remote", sock);
    
    sock = ov_socket_data_to_json(&remote);
    ov_client_registry_set_data(vocs->client_registry, uuid, sock);

    if (!ov_broadcast_registry_set(vocs->broadcasts,
                                   OV_BROADCAST_KEY_SYSTEM_BROADCAST,
                                   socket,
                                   OV_SYSTEM_BROADCAST)) {
        goto error;
    }

    out = ov_event_api_create_success_response(input);

    env_send(vocs, socket, out);
    out = ov_json_value_free(out);
    return ;
error:
    val = ov_json_value_free(val);
    out = ov_json_value_free(out);
    return;
}

/*----------------------------------------------------------------------------*/

static bool enable_api_client(ov_vocs *vocs){

    if (!ov_event_broker_register(
        vocs->broker,
        OV_EVENT_API_MEDIA,
        vocs,
        client_media)) goto error;

    if (!ov_event_broker_register(
        vocs->broker,
        OV_ICE_STRING_CANDIDATE,
        vocs,
        client_candidate)) goto error;

    if (!ov_event_broker_register(
        vocs->broker,
        OV_ICE_STRING_END_OF_CANDIDATES,
        vocs,
        client_end_of_candidates)) goto error;

    if (!ov_event_broker_register(
        vocs->broker,
        OV_EVENT_API_AUTHORISE,
        vocs,
        client_authorize)) goto error;

    if (!ov_event_broker_register(
        vocs->broker,
        OV_EVENT_API_AUTHORIZE,
        vocs,
        client_authorize)) goto error;

    if (!ov_event_broker_register(
        vocs->broker,
        OV_EVENT_API_GET,
        vocs,
        client_get)) goto error;

    if (!ov_event_broker_register(
        vocs->broker,
        OV_EVENT_API_USER_ROLES,
        vocs,
        client_user_roles)) goto error;

    if (!ov_event_broker_register(
        vocs->broker,
        OV_EVENT_API_ROLE_LOOPS,
        vocs,
        client_role_loops)) goto error;

    if (!ov_event_broker_register(
        vocs->broker,
        OV_EVENT_API_SWITCH_LOOP_STATE,
        vocs,
        client_switch_loop_state)) goto error;

    if (!ov_event_broker_register(
        vocs->broker,
        OV_EVENT_API_SWITCH_LOOP_VOLUME,
        vocs,
        client_switch_loop_volume)) goto error;

    if (!ov_event_broker_register(
        vocs->broker,
        OV_EVENT_API_TALKING,
        vocs,
        client_talking)) goto error;

    if (!ov_event_broker_register(
        vocs->broker,
        "client_call",
        vocs,
        client_calling)) goto error;

    if (!ov_event_broker_register(
        vocs->broker,
        "client_hangup",
        vocs,
        client_hangup)) goto error;

    if (!ov_event_broker_register(
        vocs->broker,
        "client_permit_call",
        vocs,
        client_permit_call)) goto error;

    if (!ov_event_broker_register(
        vocs->broker,
        "client_revoke_call",
        vocs,
        client_revoke_call)) goto error;

    if (!ov_event_broker_register(
        vocs->broker,
        "client_list_calls",
        vocs,
        client_list_calls)) goto error;

    if (!ov_event_broker_register(
        vocs->broker,
        "client_list_call_permissions",
        vocs,
        client_list_call_permissions)) goto error;

    if (!ov_event_broker_register(
        vocs->broker,
        "client_list_sip_status",
        vocs,
        client_list_sip_status)) goto error;

    if (!ov_event_broker_register(
        vocs->broker,
        OV_VOCS_DB_SET_KEYSET_LAYOUT,
        vocs,
        client_event_set_keyset_layout)) goto error;

    if (!ov_event_broker_register(
        vocs->broker,
        OV_VOCS_DB_GET_KEYSET_LAYOUT,
        vocs,
        client_event_get_keyset_layout)) goto error;

    if (!ov_event_broker_register(
        vocs->broker,
        OV_VOCS_DB_SET_USER_DATA,
        vocs,
        client_event_set_user_data)) goto error;

    if (!ov_event_broker_register(
        vocs->broker,
        OV_VOCS_DB_GET_USER_DATA,
        vocs,
        client_event_get_user_data)) goto error;

    if (!ov_event_broker_register(
        vocs->broker,
        "client_get_recording",
        vocs,
        client_event_get_recording)) goto error;

    if (!ov_event_broker_register(
        vocs->broker,
        "is_sip_enabled",
        vocs,
        client_event_sip_status)) goto error;

    if (!ov_event_broker_register(
        vocs->broker,
        OV_KEY_REGISTER,
        vocs,
        client_event_register)) goto error;

    return true;
error:
    return false;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      AUTHORIZATION
 *
 *      ------------------------------------------------------------------------
 */

static bool authorize_connection_for_id(ov_vocs *vocs,
                                        int socket,
                                        ov_vocs_db_entity entity,
                                        const char *id) {

    ov_vocs_db_parent parent = {0};
    ov_json_value *data = NULL;

    if (!vocs || !socket || !id) goto error;

    data = ov_socket_storage_get(vocs->connections, socket);
    const char *user = ov_json_string_get(ov_json_get(data, "/"OV_KEY_USER));


    if (!user) goto error;

    parent = ov_vocs_db_get_parent(vocs->config.db, entity, id);

    if (NULL == parent.id) goto error;

    switch (parent.scope) {

        case OV_VOCS_DB_SCOPE_DOMAIN:

            if (!ov_vocs_db_authorize_domain_admin(
                    vocs->config.db, user, parent.id)) {
                goto error;
            }

            break;

        case OV_VOCS_DB_SCOPE_PROJECT:

            if (!ov_vocs_db_authorize_project_admin(
                    vocs->config.db, user, parent.id)) {
                goto error;
            }

            break;
    }

    ov_vocs_db_parent_clear(&parent);
    return true;
error:
    ov_vocs_db_parent_clear(&parent);
    return false;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      #API DB EVENTS
 *
 *      ------------------------------------------------------------------------
 */

/*----------------------------------------------------------------------------*/

static void cb_event_update_password(void *userdata, const char *name,
        int socket, const ov_json_value *input) {

    ov_json_value *out = NULL;
    ov_json_value *data = NULL;

    UNUSED(name);
    ov_vocs *vocs = ov_vocs_cast(userdata);

    if (!vocs || !socket || !input) goto error;

    data = ov_socket_storage_get(vocs->connections, socket);
    const char *conn_user = ov_json_string_get(ov_json_get(data, "/"OV_KEY_USER));

    const char *user = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_USER));
    const char *pass = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_PASSWORD));

    if (!user || !pass) {

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_PARAMETER_ERROR,
                                                 OV_ERROR_DESC_PARAMETER_ERROR);

        goto response;
    }

    if (vocs->ldap) {

        out = ov_event_api_create_error_response(
            input, OV_ERROR_CODE_AUTH_LDAP_USED, OV_ERROR_DESC_AUTH_LDAP_USED);

        goto response;
    }

    if (0 == ov_string_compare(user, conn_user)) {

        /* each user is allowed to update it's own password */

        if (!ov_vocs_db_set_password(vocs->config.db, user, pass)) {

            out = ov_event_api_create_error_response(
                input,
                OV_ERROR_CODE_PROCESSING_ERROR,
                OV_ERROR_DESC_PROCESSING_ERROR);

            goto response;
        }

        out = ov_event_api_create_success_response(input);
        goto response;

    } else {

        /* password reset from admin */

        ov_vocs_db_parent parent =
            ov_vocs_db_get_parent(vocs->config.db, OV_VOCS_DB_USER, user);

        switch (parent.scope) {

            case OV_VOCS_DB_SCOPE_DOMAIN:

                if (!ov_vocs_db_authorize_domain_admin(
                        vocs->config.db, conn_user, parent.id)) {

                    out = ov_event_api_create_error_response(
                        input, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

                    ov_vocs_db_parent_clear(&parent);

                    goto response;
                }

                break;

            case OV_VOCS_DB_SCOPE_PROJECT:

                if (!ov_vocs_db_authorize_project_admin(
                        vocs->config.db, conn_user, parent.id)) {

                    out = ov_event_api_create_error_response(
                        input, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

                    ov_vocs_db_parent_clear(&parent);

                    goto response;
                }

                break;
        }

        ov_vocs_db_parent_clear(&parent);

        /* admin of the project or the domain of the user,
         * update allowed */

        if (!ov_vocs_db_set_password(vocs->config.db, user, pass)) {

            out = ov_event_api_create_error_response(
                input,
                OV_ERROR_CODE_PROCESSING_ERROR,
                OV_ERROR_DESC_PROCESSING_ERROR);

            goto response;
        }

        out = ov_event_api_create_success_response(input);
        ov_cluster_send(vocs->cluster, input);
        //ov_vocs_db_persistance_broadcast(vocs->config.persistance, input);
        goto response;
    }

response:

    env_send(vocs, socket, out);
    out = ov_json_value_free(out);
    return;
error:
    out = ov_json_value_free(out);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_event_get_admin_domains(void *userdata, const char *name,
        int socket, const ov_json_value *input) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *data = NULL;

    UNUSED(name);
    ov_vocs *vocs = ov_vocs_cast(userdata);

    if (!vocs || !socket || !input) goto error;

    data = ov_socket_storage_get(vocs->connections, socket);
    const char *conn_user = ov_json_string_get(ov_json_get(data, "/"OV_KEY_USER));

    const char *user = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_USER));

    if (!user) user = conn_user;

    /* all users are allowed to send the request */

    val = ov_vocs_db_get_admin_domains(vocs->config.db, user);
    if (!val) {

        out =
            ov_event_api_create_error_response(input,
                                               OV_ERROR_CODE_PROCESSING_ERROR,
                                               OV_ERROR_DESC_PROCESSING_ERROR);

        goto response;
    }

    out = ov_event_api_create_success_response(input);
    ov_json_value *res = ov_event_api_get_response(out);
    ov_json_object_set(res, OV_KEY_DOMAINS, val);

response:

    env_send(vocs, socket, out);
    out = ov_json_value_free(out);
    return;
error:
    out = ov_json_value_free(out);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_event_get_admin_projects(void *userdata, const char *name,
        int socket, const ov_json_value *input) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *data = NULL;
    
    UNUSED(name);
    ov_vocs *vocs = ov_vocs_cast(userdata);

    if (!vocs || !socket || !input) goto error;

    data = ov_socket_storage_get(vocs->connections, socket);
    const char *conn_user = ov_json_string_get(ov_json_get(data, "/"OV_KEY_USER));

    const char *user = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_USER));

    if (!user) user = conn_user;

    /* all users are allowed to send the request */

    val = ov_vocs_db_get_admin_projects(vocs->config.db, user);
    if (!val) {

        out =
            ov_event_api_create_error_response(input,
                                               OV_ERROR_CODE_PROCESSING_ERROR,
                                               OV_ERROR_DESC_PROCESSING_ERROR);

        goto response;
    }

    out = ov_event_api_create_success_response(input);
    ov_json_value *res = ov_event_api_get_response(out);
    ov_json_object_set(res, OV_KEY_PROJECTS, val);

response:

    env_send(vocs, socket, out);
    out = ov_json_value_free(out);
    return;
error:
    out = ov_json_value_free(out);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_event_check_id_exists(void *userdata, const char *name,
        int socket, const ov_json_value *input) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *data = NULL;
    
    UNUSED(name);
    ov_vocs *vocs = ov_vocs_cast(userdata);

    if (!vocs || !socket || !input) goto error;

    data = ov_socket_storage_get(vocs->connections, socket);
    const char *conn_user = ov_json_string_get(ov_json_get(data, "/"OV_KEY_USER));

    if (!conn_user) {

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_AUTH,
                                                 OV_ERROR_DESC_AUTH);

        goto response;
    }

    const char *id = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_ID));

    const char *scope = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_SCOPE));

    if (!id) {

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_PARAMETER_ERROR,
                                                 OV_ERROR_DESC_PARAMETER_ERROR);

        goto response;
    }

    /* all users are allowed to send the request */

    if (scope) {

        /* check for unique entity id */

        ov_vocs_db_entity entity = ov_vocs_db_entity_from_string(scope);

        if (ov_vocs_db_check_entity_id_exists(vocs->config.db, entity, id)) {
            val = ov_json_true();
        } else {
            val = ov_json_false();
        }

    } else {

        /* check for unique ID */

        if (ov_vocs_db_check_id_exists(vocs->config.db, id)) {
            val = ov_json_true();
        } else {
            val = ov_json_false();
        }
    }

    out = ov_event_api_create_success_response(input);
    ov_json_object_set(out, "response", val);

response:

    env_send(vocs, socket, out);
    out = ov_json_value_free(out);
    return;
error:
    out = ov_json_value_free(out);
    return;
}


/*----------------------------------------------------------------------------*/

static void cb_event_delete(void *userdata, const char *name,
        int socket, const ov_json_value *input) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *data = NULL;
    
    UNUSED(name);
    ov_vocs *vocs = ov_vocs_cast(userdata);

    if (!vocs || !socket || !input) goto error;

    data = ov_socket_storage_get(vocs->connections, socket);
    const char *conn_user = ov_json_string_get(ov_json_get(data, "/"OV_KEY_USER));

    if (!conn_user) {

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_AUTH,
                                                 OV_ERROR_DESC_AUTH);

        goto response;
    }

    const char *id = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_ID));

    const char *type = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_TYPE));

    if (!id || !type) {

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_PARAMETER_ERROR,
                                                 OV_ERROR_DESC_PARAMETER_ERROR);

        goto response;
    }

    ov_vocs_db_entity entity = ov_vocs_db_entity_from_string(type);
    if (OV_VOCS_DB_ENTITY_ERROR == entity) {

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_PARAMETER_ERROR,
                                                 OV_ERROR_DESC_PARAMETER_ERROR);

        goto response;
    }

    if (!authorize_connection_for_id(vocs, socket, entity, id)) {

        out = ov_event_api_create_error_response(
            input, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto response;
    }

    if (!ov_vocs_db_delete_entity(vocs->config.db, entity, id)) {

        out =
            ov_event_api_create_error_response(input,
                                               OV_ERROR_CODE_PROCESSING_ERROR,
                                               OV_ERROR_DESC_PROCESSING_ERROR);

        goto response;
    }

    out = ov_event_api_create_success_response(input);
    ov_cluster_send(vocs->cluster, input);

response:

    env_send(vocs, socket, out);
    out = ov_json_value_free(out);
    return;
error:
    val = ov_json_value_free(val);
    out = ov_json_value_free(out);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_event_create(void *userdata, const char *name,
        int socket, const ov_json_value *input) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *data = NULL;
    
    UNUSED(name);
    ov_vocs *vocs = ov_vocs_cast(userdata);

    if (!vocs || !socket || !input) goto error;

    data = ov_socket_storage_get(vocs->connections, socket);
    const char *conn_user = ov_json_string_get(ov_json_get(data, "/"OV_KEY_USER));

    if (!conn_user) {

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_AUTH,
                                                 OV_ERROR_DESC_AUTH);

        goto response;
    }

    const char *id = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_ID));

    const char *type = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_TYPE));

    const char *scope_id = ov_json_string_get(ov_json_get(
        input, "/" OV_KEY_PARAMETER "/" OV_KEY_SCOPE "/" OV_KEY_ID));

    const char *scope_type = ov_json_string_get(ov_json_get(
        input, "/" OV_KEY_PARAMETER "/" OV_KEY_SCOPE "/" OV_KEY_TYPE));

    if (!id || !type || !scope_id || !scope_type) {

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_PARAMETER_ERROR,
                                                 OV_ERROR_DESC_PARAMETER_ERROR);

        goto response;
    }

    ov_vocs_db_entity entity = ov_vocs_db_entity_from_string(type);
    if (OV_VOCS_DB_ENTITY_ERROR == entity) {

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_PARAMETER_ERROR,
                                                 OV_ERROR_DESC_PARAMETER_ERROR);

        goto response;
    }

    ov_vocs_db_scope db_scope = OV_VOCS_DB_SCOPE_PROJECT;

    if (0 == strcmp(scope_type, OV_KEY_DOMAIN)) {

        db_scope = OV_VOCS_DB_SCOPE_DOMAIN;

        if (!ov_vocs_db_authorize_domain_admin(
                vocs->config.db, conn_user, scope_id)) {

            out = ov_event_api_create_error_response(
                input, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

            goto response;
        }

    } else if (!ov_vocs_db_authorize_project_admin(
                   vocs->config.db, conn_user, scope_id)) {

        out = ov_event_api_create_error_response(
            input, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto response;
    }

    if (!ov_vocs_db_create_entity(
            vocs->config.db, entity, id, db_scope, scope_id)) {

        out =
            ov_event_api_create_error_response(input,
                                               OV_ERROR_CODE_PROCESSING_ERROR,
                                               OV_ERROR_DESC_PROCESSING_ERROR);

        goto response;
    }

    out = ov_event_api_create_success_response(input);
    ov_cluster_send(vocs->cluster, input);

response:

    env_send(vocs, socket, out);
    out = ov_json_value_free(out);
    return;
error:
    val = ov_json_value_free(val);
    out = ov_json_value_free(out);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_event_get_key(void *userdata, const char *name,
        int socket, const ov_json_value *input) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *data = NULL;
    
    UNUSED(name);
    ov_vocs *vocs = ov_vocs_cast(userdata);

    if (!vocs || !socket || !input) goto error;

    data = ov_socket_storage_get(vocs->connections, socket);
    const char *conn_user = ov_json_string_get(ov_json_get(data, "/"OV_KEY_USER));

    if (!conn_user) {

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_AUTH,
                                                 OV_ERROR_DESC_AUTH);

        goto response;
    }

    const char *id = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_ID));

    const char *key = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_KEY));

    const char *type = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_TYPE));

    if (!id || !type || !key) {

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_PARAMETER_ERROR,
                                                 OV_ERROR_DESC_PARAMETER_ERROR);

        goto response;
    }

    ov_vocs_db_entity entity = ov_vocs_db_entity_from_string(type);
    if (OV_VOCS_DB_ENTITY_ERROR == entity) {

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_PARAMETER_ERROR,
                                                 OV_ERROR_DESC_PARAMETER_ERROR);

        goto response;
    }

    /* all users are allowed to get anything */

    val = ov_vocs_db_get_entity_key(vocs->config.db, entity, id, key);
    if (!val) {
        val = ov_json_null();
    }

    out = ov_event_api_create_success_response(input);
    ov_json_value *res = ov_event_api_get_response(out);
    if (!ov_json_object_set(res, OV_KEY_RESULT, val)) goto error;

    val = ov_json_string(type);
    if (!ov_json_object_set(res, OV_KEY_TYPE, val)) goto error;

    val = ov_json_string(id);
    if (!ov_json_object_set(res, OV_KEY_ID, val)) goto error;

    val = ov_json_string(key);
    if (!ov_json_object_set(res, OV_KEY_KEY, val)) goto error;

    val = NULL;

response:

    env_send(vocs, socket, out);
    out = ov_json_value_free(out);
    return ;
error:
    val = ov_json_value_free(val);
    out = ov_json_value_free(out);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_event_update_key(void *userdata, const char *name,
        int socket, const ov_json_value *input) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *data = NULL;
    
    UNUSED(name);
    ov_vocs *vocs = ov_vocs_cast(userdata);
    if (!vocs || !socket || !input) goto error;

    data = ov_socket_storage_get(vocs->connections, socket);
    const char *conn_user = ov_json_string_get(ov_json_get(data, "/"OV_KEY_USER));

    if (!conn_user) {

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_AUTH,
                                                 OV_ERROR_DESC_AUTH);

        goto response;
    }

    const char *id = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_ID));

    const char *key = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_KEY));

    const char *type = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_TYPE));

    const ov_json_value *udata =
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_DATA);

    if (!id || !type || !key || !udata) {

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_PARAMETER_ERROR,
                                                 OV_ERROR_DESC_PARAMETER_ERROR);

        goto response;
    }

    ov_vocs_db_entity entity = ov_vocs_db_entity_from_string(type);
    if (OV_VOCS_DB_ENTITY_ERROR == entity) {

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_PARAMETER_ERROR,
                                                 OV_ERROR_DESC_PARAMETER_ERROR);

        goto response;
    }

    if (!authorize_connection_for_id(vocs, socket, entity, id)) {

        out = ov_event_api_create_error_response(
            input, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto response;
    }

    if (!ov_vocs_db_update_entity_key(vocs->config.db, entity, id, key, udata)) {

        out =
            ov_event_api_create_error_response(input,
                                               OV_ERROR_CODE_PROCESSING_ERROR,
                                               OV_ERROR_DESC_PROCESSING_ERROR);

        goto response;
    }

    //ov_vocs_db_persistance_broadcast(vocs->config.persistance, input);
    out = ov_event_api_create_success_response(input);

    ov_cluster_send(vocs->cluster, input);

response:

    env_send(vocs, socket, out);
    out = ov_json_value_free(out);
    return;
error:
    val = ov_json_value_free(val);
    out = ov_json_value_free(out);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_event_delete_key(void *userdata, const char *name,
        int socket, const ov_json_value *input) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *data = NULL;
    
    UNUSED(name);
    ov_vocs *vocs = ov_vocs_cast(userdata);

    if (!vocs || !socket || !input) goto error;

    data = ov_socket_storage_get(vocs->connections, socket);
    const char *conn_user = ov_json_string_get(ov_json_get(data, "/"OV_KEY_USER));

    if (!conn_user) {

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_AUTH,
                                                 OV_ERROR_DESC_AUTH);

        goto response;
    }

    const char *id = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_ID));

    const char *key = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_KEY));

    const char *type = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_TYPE));

    if (!id || !type || !key) {

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_PARAMETER_ERROR,
                                                 OV_ERROR_DESC_PARAMETER_ERROR);

        goto response;
    }

    ov_vocs_db_entity entity = ov_vocs_db_entity_from_string(type);
    if (OV_VOCS_DB_ENTITY_ERROR == entity) {

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_PARAMETER_ERROR,
                                                 OV_ERROR_DESC_PARAMETER_ERROR);

        goto response;
    }

    if (!authorize_connection_for_id(vocs, socket, entity, id)) {

        out = ov_event_api_create_error_response(
            input, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto response;
    }

    if (!ov_vocs_db_delete_entity_key(vocs->config.db, entity, id, key)) {

        out =
            ov_event_api_create_error_response(input,
                                               OV_ERROR_CODE_PROCESSING_ERROR,
                                               OV_ERROR_DESC_PROCESSING_ERROR);

        goto response;
    }

    out = ov_event_api_create_success_response(input);
    ov_cluster_send(vocs->cluster, input);

response:

    env_send(vocs, socket, out);
    out = ov_json_value_free(out);
    return;
error:
    val = ov_json_value_free(val);
    out = ov_json_value_free(out);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_event_verify(void *userdata, const char *name,
        int socket, const ov_json_value *input) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *data = NULL;

    ov_json_value *errors = NULL;
    
    UNUSED(name);
    ov_vocs *vocs = ov_vocs_cast(userdata);

    if (!vocs || !socket || !input) goto error;

    data = ov_socket_storage_get(vocs->connections, socket);
    const char *conn_user = ov_json_string_get(ov_json_get(data, "/"OV_KEY_USER));

    if (!conn_user) {

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_AUTH,
                                                 OV_ERROR_DESC_AUTH);

        goto response;
    }

    const char *id = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_ID));

    const ov_json_value *udata =
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_DATA);

    const char *type = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_TYPE));

    if (!id || !type || !udata) {

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_PARAMETER_ERROR,
                                                 OV_ERROR_DESC_PARAMETER_ERROR);

        goto response;
    }

    ov_vocs_db_entity entity = ov_vocs_db_entity_from_string(type);
    if (OV_VOCS_DB_ENTITY_ERROR == entity) {

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_PARAMETER_ERROR,
                                                 OV_ERROR_DESC_PARAMETER_ERROR);

        goto response;
    }

    if (!authorize_connection_for_id(vocs, socket, entity, id)) {

        out = ov_event_api_create_error_response(
            input, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto response;
    }

    if (!ov_vocs_db_verify_entity_item(
            vocs->config.db, entity, id, udata, &errors)) {

        out =
            ov_event_api_create_error_response(input,
                                               OV_ERROR_CODE_PROCESSING_ERROR,
                                               OV_ERROR_DESC_PROCESSING_ERROR);

        if (errors) {

            ov_json_value *res = ov_event_api_get_response(out);
            ov_json_object_set(res, OV_KEY_ERRORS, errors);
        }

        goto response;
    }

    OV_ASSERT(errors == NULL);
    out = ov_event_api_create_success_response(input);

response:

    env_send(vocs, socket, out);
    out = ov_json_value_free(out);
    return;
error:
    val = ov_json_value_free(val);
    out = ov_json_value_free(out);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_event_update(void *userdata, const char *name,
        int socket, const ov_json_value *input) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *data = NULL;

    ov_json_value *errors = NULL;
    
    UNUSED(name);
    ov_vocs *vocs = ov_vocs_cast(userdata);

    if (!vocs || !socket || !input) goto error;

    data = ov_socket_storage_get(vocs->connections, socket);
    const char *conn_user = ov_json_string_get(ov_json_get(data, "/"OV_KEY_USER));

    if (!conn_user) {

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_AUTH,
                                                 OV_ERROR_DESC_AUTH);

        goto response;
    }

    const char *id = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_ID));

    const ov_json_value *udata =
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_DATA);

    const char *type = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_TYPE));

    if (!id || !type || !udata) {

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_PARAMETER_ERROR,
                                                 OV_ERROR_DESC_PARAMETER_ERROR);

        goto response;
    }

    ov_vocs_db_entity entity = ov_vocs_db_entity_from_string(type);
    if (OV_VOCS_DB_ENTITY_ERROR == entity) {

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_PARAMETER_ERROR,
                                                 OV_ERROR_DESC_PARAMETER_ERROR);

        goto response;
    }

    if (0 == strcmp(type, OV_KEY_DOMAIN)) {

        if (!ov_vocs_db_authorize_domain_admin(vocs->config.db, conn_user, id)) {

            out = ov_event_api_create_error_response(
                input, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

            goto response;
        }

    } else if (0 == strcmp(type, OV_KEY_PROJECT)) {

        if (!ov_vocs_db_authorize_project_admin(vocs->config.db, conn_user, id)) {

            out = ov_event_api_create_error_response(
                input, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

            goto response;
        }

    } else {

        if (!authorize_connection_for_id(vocs, socket, entity, id)) {

            out = ov_event_api_create_error_response(
                input, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

            goto response;
        }
    }

    if (!ov_vocs_db_update_entity_item(
            vocs->config.db, entity, id, udata, &errors)) {

        out =
            ov_event_api_create_error_response(input,
                                               OV_ERROR_CODE_PROCESSING_ERROR,
                                               OV_ERROR_DESC_PROCESSING_ERROR);

        if (errors) {

            ov_json_value *res = ov_event_api_get_response(out);
            ov_json_object_set(res, OV_KEY_ERRORS, errors);
        }

        goto response;
    }

    OV_ASSERT(errors == NULL);
    out = ov_event_api_create_success_response(input);

    ov_cluster_send(vocs->cluster, input);

response:

    env_send(vocs, socket, out);
    out = ov_json_value_free(out);
    return;
error:
    val = ov_json_value_free(val);
    out = ov_json_value_free(out);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_event_load(void *userdata, const char *name,
        int socket, const ov_json_value *input) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *data = NULL;
    
    UNUSED(name);
    ov_vocs *vocs = ov_vocs_cast(userdata);

    if (!vocs || !socket || !input) goto error;

    data = ov_socket_storage_get(vocs->connections, socket);
    const char *conn_user = ov_json_string_get(ov_json_get(data, "/"OV_KEY_USER));

    if (!conn_user) {

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_AUTH,
                                                 OV_ERROR_DESC_AUTH);

        goto response;
    }

    /* check if the user is some domain admin */

    val = ov_vocs_db_get_admin_domains(vocs->config.db, conn_user);
    if (!val || (0 == ov_json_array_count(val))) {

        out = ov_event_api_create_error_response(
            input, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        val = ov_json_value_free(val);

        goto response;
    }
    val = ov_json_value_free(val);

    if (!ov_vocs_db_persistance_load(vocs->config.persistance)) {

        out =
            ov_event_api_create_error_response(input,
                                               OV_ERROR_CODE_PROCESSING_ERROR,
                                               OV_ERROR_DESC_PROCESSING_ERROR);

        goto response;
    }

    out = ov_event_api_create_success_response(input);

response:

    env_send(vocs, socket, out);
    out = ov_json_value_free(out);
    return;
error:
    val = ov_json_value_free(val);
    out = ov_json_value_free(out);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_event_save(void *userdata, const char *name,
        int socket, const ov_json_value *input) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *data = NULL;
    
    UNUSED(name);
    ov_vocs *vocs = ov_vocs_cast(userdata);

    if (!vocs || !socket || !input) goto error;

    data = ov_socket_storage_get(vocs->connections, socket);
    const char *conn_user = ov_json_string_get(ov_json_get(data, "/"OV_KEY_USER));

    if (!conn_user) {

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_AUTH,
                                                 OV_ERROR_DESC_AUTH);

        goto response;
    }

    /* check if the user is some domain admin */
    val = ov_vocs_db_get_admin_domains(vocs->config.db, conn_user);
    if (!val || (0 == ov_json_array_count(val))) {

        out = ov_event_api_create_error_response(
            input, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        val = ov_json_value_free(val);

        goto response;
    }
    val = ov_json_value_free(val);

    if (!ov_vocs_db_persistance_save(vocs->config.persistance)) {

        out =
            ov_event_api_create_error_response(input,
                                               OV_ERROR_CODE_PROCESSING_ERROR,
                                               OV_ERROR_DESC_PROCESSING_ERROR);

        goto response;
    }

    out = ov_event_api_create_success_response(input);

response:

    env_send(vocs, socket, out);
    out = ov_json_value_free(out);
    return ;
error:
    val = ov_json_value_free(val);
    out = ov_json_value_free(out);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_event_set_layout(void *userdata, const char *name,
        int socket, const ov_json_value *input) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *data = NULL;
    
    UNUSED(name);
    ov_vocs *vocs = ov_vocs_cast(userdata);

    if (!vocs || !socket || !input) goto error;

    data = ov_socket_storage_get(vocs->connections, socket);
    const char *conn_user = ov_json_string_get(ov_json_get(data, "/"OV_KEY_USER));

    if (!conn_user) {

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_AUTH,
                                                 OV_ERROR_DESC_AUTH);

        goto response;
    }

    const char *id = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_ROLE));

    const ov_json_value *udata =
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_LAYOUT);

    if (!id || !udata) {

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_PARAMETER_ERROR,
                                                 OV_ERROR_DESC_PARAMETER_ERROR);

        goto response;
    }

    if (!authorize_connection_for_id(vocs, socket, OV_VOCS_DB_ROLE, id)) {

        out = ov_event_api_create_error_response(
            input, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto response;
    }

    if (!ov_vocs_db_set_layout(vocs->config.db, id, udata)) {

        out =
            ov_event_api_create_error_response(input,
                                               OV_ERROR_CODE_PROCESSING_ERROR,
                                               OV_ERROR_DESC_PROCESSING_ERROR);

        goto response;
    }

    out = ov_event_api_create_success_response(input);
    ov_cluster_send(vocs->cluster, input);

response:

    env_send(vocs, socket, out);
    out = ov_json_value_free(out);
    return;
error:
    val = ov_json_value_free(val);
    out = ov_json_value_free(out);
    return;
}

/*----------------------------------------------------------------------------*/


static void cb_event_get_layout(void *userdata, const char *name,
        int socket, const ov_json_value *input) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *data = NULL;
    
    UNUSED(name);
    ov_vocs *vocs = ov_vocs_cast(userdata);

    if (!vocs || !socket || !input) goto error;

    data = ov_socket_storage_get(vocs->connections, socket);
    const char *conn_user = ov_json_string_get(ov_json_get(data, "/"OV_KEY_USER));

    if (!conn_user) {

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_AUTH,
                                                 OV_ERROR_DESC_AUTH);

        goto response;
    }

    const char *id = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_ROLE));

    if (!id) {

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_PARAMETER_ERROR,
                                                 OV_ERROR_DESC_PARAMETER_ERROR);

        goto response;
    }

    /* any user can get the layout of some role */

    val = ov_vocs_db_get_layout(vocs->config.db, id);
    if (!val) {

        out =
            ov_event_api_create_error_response(input,
                                               OV_ERROR_CODE_PROCESSING_ERROR,
                                               OV_ERROR_DESC_PROCESSING_ERROR);

        goto response;
    }

    out = ov_event_api_create_success_response(input);
    ov_json_value *res = ov_event_api_get_response(out);
    ov_json_object_set(res, OV_KEY_LAYOUT, val);

response:

    env_send(vocs, socket, out);
    out = ov_json_value_free(out);
    return;
error:
    val = ov_json_value_free(val);
    out = ov_json_value_free(out);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_event_add_domain_admin(void *userdata, const char *name,
        int socket, const ov_json_value *input) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *data = NULL;
    
    UNUSED(name);
    ov_vocs *vocs = ov_vocs_cast(userdata);

    if (!vocs || !socket || !input) goto error;

    data = ov_socket_storage_get(vocs->connections, socket);
    const char *conn_user = ov_json_string_get(ov_json_get(data, "/"OV_KEY_USER));

    if (!conn_user) {

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_AUTH,
                                                 OV_ERROR_DESC_AUTH);

        goto response;
    }

    const char *domain = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_DOMAIN));

    const char *admin = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_USER));

    if (!domain || !admin) {

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_PARAMETER_ERROR,
                                                 OV_ERROR_DESC_PARAMETER_ERROR);

        goto response;
    }

    /* MUST be an admin of the domain */
    if (!ov_vocs_db_authorize_domain_admin(vocs->config.db, conn_user, domain)) {

        out = ov_event_api_create_error_response(
            input, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto response;
    }

    if (!ov_vocs_db_add_domain_admin(vocs->config.db, domain, admin)) {

        out =
            ov_event_api_create_error_response(input,
                                               OV_ERROR_CODE_PROCESSING_ERROR,
                                               OV_ERROR_DESC_PROCESSING_ERROR);

        goto response;
    }

    out = ov_event_api_create_success_response(input);

    ov_cluster_send(vocs->cluster, input);

response:

    env_send(vocs, socket, out);
    out = ov_json_value_free(out);
    return;
error:
    val = ov_json_value_free(val);
    out = ov_json_value_free(out);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_event_add_project_admin(void *userdata, const char *name,
        int socket, const ov_json_value *input) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *data = NULL;
    
    UNUSED(name);
    ov_vocs *vocs = ov_vocs_cast(userdata);

    if (!vocs || !socket || !input) goto error;

    data = ov_socket_storage_get(vocs->connections, socket);
    const char *conn_user = ov_json_string_get(ov_json_get(data, "/"OV_KEY_USER));

    if (!conn_user) {

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_AUTH,
                                                 OV_ERROR_DESC_AUTH);

        goto response;
    }

    const char *project = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_PROJECT));

    const char *admin = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_USER));

    if (!project || !admin) {

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_PARAMETER_ERROR,
                                                 OV_ERROR_DESC_PARAMETER_ERROR);

        goto response;
    }

    /* MUST be an admin of the project */
    if (!ov_vocs_db_authorize_project_admin(vocs->config.db, conn_user, project)) {

        out = ov_event_api_create_error_response(
            input, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto response;
    }

    if (!ov_vocs_db_add_project_admin(vocs->config.db, project, admin)) {

        out =
            ov_event_api_create_error_response(input,
                                               OV_ERROR_CODE_PROCESSING_ERROR,
                                               OV_ERROR_DESC_PROCESSING_ERROR);

        goto response;
    }

    out = ov_event_api_create_success_response(input);
    ov_cluster_send(vocs->cluster, input);
response:

    env_send(vocs, socket, out);
    out = ov_json_value_free(out);
    return ;
error:
    val = ov_json_value_free(val);
    out = ov_json_value_free(out);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_ldap_import(void *userdata, const char *uuid, ov_result result){

    ov_json_value *out = NULL;

    ov_vocs *self = ov_vocs_cast(userdata);
    if (!self || !uuid) goto error;

    ov_event_async_data async = ov_event_async_unset(self->async, uuid);

    if (OV_ERROR_NO_ERROR == result.error_code){

        out = ov_event_api_create_success_response(async.value);
    
    } else {

        out = ov_event_api_create_error_response(async.value,
            result.error_code, result.message);

    }

    env_send(self, async.socket, out);
    out = ov_json_value_free(out);
    ov_event_async_data_clear(&async);
error:
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_event_ldap_import(void *userdata, const char *name,
        int socket, const ov_json_value *input) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *data = NULL;
    
    UNUSED(name);
    ov_vocs *vocs = ov_vocs_cast(userdata);

    if (!vocs || !socket || !input) goto error;

    data = ov_socket_storage_get(vocs->connections, socket);
    const char *conn_user = ov_json_string_get(ov_json_get(data, "/"OV_KEY_USER));

    if (!conn_user) {

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_AUTH,
                                                 OV_ERROR_DESC_AUTH);

        goto response;
    }

    const char *uuid = ov_event_api_get_uuid(input);

    const char *host = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_HOST));

    const char *base = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_BASE));

    const char *domain = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_DOMAIN));

    const char *ldap_user = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_USER));

    const char *ldap_pass = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_PASSWORD));

    if (!host || !base || !domain || !ldap_user || !ldap_pass) {

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_PARAMETER_ERROR,
                                                 OV_ERROR_DESC_PARAMETER_ERROR);

        goto response;
    }

    /* MUST be an admin of the domain */
    if (!ov_vocs_db_authorize_domain_admin(vocs->config.db, conn_user, domain)) {

        out = ov_event_api_create_error_response(
            input, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto response;
    }

    if (!set_async(vocs, uuid, socket, input)) goto error;

    if (!ov_vocs_db_persistance_ldap_import(vocs->config.persistance,
                                            host,
                                            base,
                                            ldap_user,
                                            ldap_pass,
                                            domain,
                                            uuid,
                                            vocs,
                                            cb_ldap_import)){

        out =
            ov_event_api_create_error_response(input,
                                               OV_ERROR_CODE_PROCESSING_ERROR,
                                               OV_ERROR_DESC_PROCESSING_ERROR);

        goto response;
    }


    ov_cluster_send(vocs->cluster, input);

response:

    if (out) env_send(vocs, socket, out);
    out = ov_json_value_free(out);
    return;
error:
    val = ov_json_value_free(val);
    out = ov_json_value_free(out);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_event_set_user_data(void *userdata, const char *name,
        int socket, const ov_json_value *input) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *data = NULL;
    
    UNUSED(name);
    ov_vocs *vocs = ov_vocs_cast(userdata);

    if (!vocs || !socket || !input) goto error;

    data = ov_socket_storage_get(vocs->connections, socket);
    const char *conn_user = ov_json_string_get(ov_json_get(data, "/"OV_KEY_USER));

    if (!conn_user) {

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_AUTH,
                                                 OV_ERROR_DESC_AUTH);

        goto response;
    }

    if (!conn_user) {

        out = ov_event_api_create_error_response(
            input, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto response;
    }

    ov_json_value *udata = ov_event_api_get_parameter(input);
    if (!udata) {

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_PARAMETER_ERROR,
                                                 OV_ERROR_DESC_PARAMETER_ERROR);

        goto response;
    }

    if (!ov_vocs_db_set_user_data(vocs->config.db, conn_user, udata)) {

        out =
            ov_event_api_create_error_response(input,
                                               OV_ERROR_CODE_PROCESSING_ERROR,
                                               OV_ERROR_DESC_PROCESSING_ERROR);

        goto response;
    }

    out = ov_event_api_create_success_response(input);
    ov_json_value *res = ov_event_api_get_response(out);
    ov_json_object_set(res, OV_KEY_LAYOUT, val);

response:

    env_send(vocs, socket, out);
    out = ov_json_value_free(out);
    return;
error:
    val = ov_json_value_free(val);
    out = ov_json_value_free(out);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_event_get_user_data(void *userdata, const char *name,
        int socket, const ov_json_value *input) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *data = NULL;
    
    UNUSED(name);
    ov_vocs *vocs = ov_vocs_cast(userdata);

    if (!vocs || !socket || !input) goto error;

    data = ov_socket_storage_get(vocs->connections, socket);
    const char *conn_user = ov_json_string_get(ov_json_get(data, "/"OV_KEY_USER));

    if (!conn_user) {

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_AUTH,
                                                 OV_ERROR_DESC_AUTH);

        goto response;
    }

    if (!conn_user) {

        out = ov_event_api_create_error_response(
            input, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto response;
    }

    ov_json_value *udata = ov_vocs_db_get_user_data(vocs->config.db, conn_user);

    if (!udata) {

        out =
            ov_event_api_create_error_response(input,
                                               OV_ERROR_CODE_PROCESSING_ERROR,
                                               OV_ERROR_DESC_PROCESSING_ERROR);

        goto response;
    }

    out = ov_event_api_create_success_response(input);
    ov_json_value *res = ov_event_api_get_response(out);
    ov_json_object_set(res, OV_KEY_DATA, udata);

response:

    env_send(vocs, socket, out);
    out = ov_json_value_free(out);
    return;
error:
    val = ov_json_value_free(val);
    out = ov_json_value_free(out);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_event_get_highest_port(void *userdata, const char *name,
        int socket, const ov_json_value *input) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *data = NULL;
    
    UNUSED(name);
    ov_vocs *vocs = ov_vocs_cast(userdata);

    if (!vocs || !socket || !input) goto error;

    data = ov_socket_storage_get(vocs->connections, socket);
    const char *conn_user = ov_json_string_get(ov_json_get(data, "/"OV_KEY_USER));

    if (!conn_user) {

        out = ov_event_api_create_error_response(input,
                                                 OV_ERROR_CODE_AUTH,
                                                 OV_ERROR_DESC_AUTH);

        goto response;
    }

    uint32_t port = ov_vocs_db_get_highest_port(vocs->config.db);

    out = ov_event_api_create_success_response(input);
    ov_json_value *res = ov_event_api_get_response(out);
    ov_json_object_set(res, OV_KEY_PORT, ov_json_number(port));

response:
    env_send(vocs, socket, out);
    out = ov_json_value_free(out);
    return;
error:
    val = ov_json_value_free(val);
    out = ov_json_value_free(out);
    return;
}

/*----------------------------------------------------------------------------*/

static bool enable_api_db(ov_vocs *vocs){

    if (!vocs) goto error;

    if (!ov_event_broker_register(
        vocs->broker,
        OV_VOCS_DB_UPDATE_PASSWORD,
        vocs,
        cb_event_update_password)) goto error;

    if (!ov_event_broker_register(
        vocs->broker,
        OV_VOCS_DB_ADMIN_DOMAINS,
        vocs,
        cb_event_get_admin_domains)) goto error;

    if (!ov_event_broker_register(
        vocs->broker,
        OV_VOCS_DB_ADMIN_PROJECTS,
        vocs,
        cb_event_get_admin_projects)) goto error;

    if (!ov_event_broker_register(
        vocs->broker,
        OV_VOCS_DB_ID_EXISTS,
        vocs,
        cb_event_check_id_exists)) goto error;

    if (!ov_event_broker_register(
        vocs->broker,
        OV_VOCS_DB_DELETE,
        vocs,
        cb_event_delete)) goto error;

    if (!ov_event_broker_register(
        vocs->broker,
        OV_VOCS_DB_CREATE,
        vocs,
        cb_event_create)) goto error;

    if (!ov_event_broker_register(
        vocs->broker,
        OV_VOCS_DB_GET_KEY,
        vocs,
        cb_event_get_key)) goto error;

    if (!ov_event_broker_register(
        vocs->broker,
        OV_VOCS_DB_UPDATE_KEY,
        vocs,
        cb_event_update_key)) goto error;

    if (!ov_event_broker_register(
        vocs->broker,
        OV_VOCS_DB_DELETE_KEY,
        vocs,
        cb_event_delete_key)) goto error;

    if (!ov_event_broker_register(
        vocs->broker,
        OV_VOCS_DB_VERIFY,
        vocs,
        cb_event_verify)) goto error;

    if (!ov_event_broker_register(
        vocs->broker,
        OV_VOCS_DB_UPDATE,
        vocs,
        cb_event_update)) goto error;

    if (!ov_event_broker_register(
        vocs->broker,
        OV_VOCS_DB_LOAD,
        vocs,
        cb_event_load)) goto error;

    if (!ov_event_broker_register(
        vocs->broker,
        OV_VOCS_DB_SAVE,
        vocs,
        cb_event_save)) goto error;

    if (!ov_event_broker_register(
        vocs->broker,
        OV_VOCS_DB_SET_LAYOUT,
        vocs,
        cb_event_set_layout)) goto error;

    if (!ov_event_broker_register(
        vocs->broker,
        OV_VOCS_DB_GET_LAYOUT,
        vocs,
        cb_event_get_layout)) goto error;

    if (!ov_event_broker_register(
        vocs->broker,
        OV_VOCS_DB_ADD_DOMAIN_ADMIN,
        vocs,
        cb_event_add_domain_admin)) goto error;

    if (!ov_event_broker_register(
        vocs->broker,
        OV_VOCS_DB_ADD_PROJECT_ADMIN,
        vocs,
        cb_event_add_project_admin)) goto error;

    if (!ov_event_broker_register(
        vocs->broker,
        OV_VOCS_DB_LDAP_IMPORT,
        vocs,
        cb_event_ldap_import)) goto error;

    if (!ov_event_broker_register(
        vocs->broker,
        OV_VOCS_DB_SET_USER_DATA,
        vocs,
        cb_event_set_user_data)) goto error;

    if (!ov_event_broker_register(
        vocs->broker,
        OV_VOCS_DB_GET_USER_DATA,
        vocs,
        cb_event_get_user_data)) goto error;

    if (!ov_event_broker_register(
        vocs->broker,
        "db_get_highest_port",
        vocs,
        cb_event_get_highest_port)) goto error;

    return true;
error:
    return false;
}
