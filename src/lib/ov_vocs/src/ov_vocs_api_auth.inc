/*
 *      ------------------------------------------------------------------------
 *
 *      #API AUTH EVENTS
 *
 *      ------------------------------------------------------------------------
 */

static bool client_logout(ov_vocs *vocs,
                          int socket,
                          ov_json_value *input) {

    ov_json_value *data = NULL;

    bool result = false;

    if (!vocs || !input || socket < 0) goto error;

    data = ov_socket_json_get(vocs->connections, socket);
    const char *session =
        ov_json_string_get(ov_json_get(data, "/" OV_KEY_SESSION));
    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));
    const char *client =
        ov_json_string_get(ov_json_get(data, "/" OV_KEY_CLIENT));

    send_success_response(vocs, input, socket, NULL);

    ov_log_info("client logout %i|%s user %s", socket, session, user);

    ov_event_session_delete(vocs->user_sessions, client);

    /* Close connection socket if drop is not successfull */
    result = drop_connection(vocs, socket, true, true);
    data = ov_json_value_free(data);

error:
    ov_json_value_free(input);
    ov_json_value_free(data);
    return result;
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

    data = ov_socket_json_get(vocs->connections, adata.socket);

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

    ov_socket_json_set(vocs->connections, adata.socket, &data);

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
    ov_json_value_free(data);
    return;
}

/*----------------------------------------------------------------------------*/

static bool client_login(ov_vocs *vocs,
                         int socket,
                         ov_json_value *input) {

    ov_json_value *data = NULL;
    ov_json_value *out = NULL;

    bool result = false;

    if (!vocs || !input || socket < 0) goto error;

    data = ov_socket_json_get(vocs->connections, socket);
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

        if (!ov_event_async_set(
                vocs->async,
                uuid,
                (ov_event_async_data){.socket = socket,
                                      .value = input,
                                      .timedout.userdata = vocs,
                                      .timedout.callback = async_timedout},
                vocs->config.timeout.response_usec)) {

            char *str = ov_json_value_to_string(input);
            ov_log_error(
                "failed to set async msg - closing %i | %s", socket, str);
            str = ov_data_pointer_free(str);

            goto error;
        }

        return ov_ldap_authenticate_password(
            vocs->ldap,
            user,
            pass,
            uuid,
            (ov_ldap_auth_callback){
                .userdata = vocs, .callback = cb_ldap_authentication});
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

    ov_socket_json_set(vocs->connections, socket, &data);

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
    ov_json_value_free(input);
    ov_json_value_free(data);
    return result;
}

/*----------------------------------------------------------------------------*/

static bool update_client_login(ov_vocs *vocs,
                                int socket,
                                ov_json_value *input) {

    bool result = false;
    ov_json_value *out = NULL;

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

    result = send_success_response(vocs, input, socket, &out);
    return true;

    /* close socket if authentication messaging failed */
error:
    ov_json_value_free(out);
    ov_json_value_free(input);
    return result;
}

/*----------------------------------------------------------------------------*/

static bool check_ldap_enabled(ov_vocs *vocs,
                                int socket,
                                ov_json_value *input) {

    bool result = false;
    ov_json_value *val = NULL;

    if (!vocs || !input || socket < 0) goto error;

    if (vocs->config.ldap.enable) {
        val = ov_json_true();
    } else {
        val = ov_json_false();
    }

    result = send_success_response(vocs, input, socket, &val);
    return true;

error:
    ov_json_value_free(val);
    ov_json_value_free(input);
    return result;
}

/*----------------------------------------------------------------------------*/

static bool enable_api_auth(ov_vocs *vocs){

	if (!set_function(vocs->io, OV_EVENT_API_LOGOUT, client_logout)) goto error;

    if (!set_function(vocs->io, OV_EVENT_API_LOGIN, client_login)) goto error;

    if (!set_function(vocs->io, OV_EVENT_API_AUTHENTICATE, client_login))
        goto error;

    if (!set_function(vocs->io, OV_EVENT_API_UPDATE_LOGIN, update_client_login))
        goto error;

    if (!set_function(vocs->io, "is_ldap_enabled", check_ldap_enabled))
        goto error;

    return true;
error:
	return false;
}