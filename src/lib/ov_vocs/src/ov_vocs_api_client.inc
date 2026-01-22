/*
 *      ------------------------------------------------------------------------
 *
 *      #API CLIENT EVENTS
 *
 *      ------------------------------------------------------------------------
 */

/*----------------------------------------------------------------------------*/

static bool client_media(ov_vocs *vocs,
                         int socket,
                         ov_json_value *input) {

    ov_json_value *data = NULL;

    if (!vocs || !input || socket < 0) goto error;

    data = ov_socket_json_get(vocs->connections, socket);
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

    if (!ov_event_async_set(
            vocs->async,
            uuid,
            (ov_event_async_data){.socket = socket,
                                  .value = input,
                                  .timedout.userdata = vocs,
                                  .timedout.callback = async_timedout},
            vocs->config.timeout.response_usec)) {

        char *str = ov_json_value_to_string(input);
        ov_log_error("failed to set async msg - closing %i | %s", socket, str);
        str = ov_data_pointer_free(str);

        goto error;
    }

    input = NULL;
    data = ov_json_value_free(data);
    return true;
error:
    /* let socket close in case of media setup errors */
    ov_json_value_free(input);
    ov_json_value_free(data);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool client_candidate(ov_vocs *vocs,
                             int socket,
                             ov_json_value *input) {

    ov_json_value *data = NULL;

    if (!vocs || !input || socket < 0) goto error;

    data = ov_socket_json_get(vocs->connections, socket);
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

    if (!ov_event_async_set(
            vocs->async,
            uuid,
            (ov_event_async_data){.socket = socket,
                                  .value = input,
                                  .timedout.userdata = vocs,
                                  .timedout.callback = async_timedout},
            vocs->config.timeout.response_usec)) {

        char *str = ov_json_value_to_string(input);
        ov_log_error("failed to set async msg - closing %i | %s", socket, str);
        str = ov_data_pointer_free(str);

        goto error;
    }

    data = ov_json_value_free(data);
    return true;

error:
    /* let socket close in case of media setup errors */
    ov_json_value_free(input);
    ov_json_value_free(data);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool client_end_of_candidates(ov_vocs *vocs,
                                     int socket,
                                     ov_json_value *input) {

    ov_json_value *data = NULL;

    if (!vocs || !input || socket < 0) goto error;

    data = ov_socket_json_get(vocs->connections, socket);
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

    if (!ov_event_async_set(
            vocs->async,
            uuid,
            (ov_event_async_data){.socket = socket,
                                  .value = input,
                                  .timedout.userdata = vocs,
                                  .timedout.callback = async_timedout},
            vocs->config.timeout.response_usec)) {

        char *str = ov_json_value_to_string(input);
        ov_log_error("failed to set async msg - closing %i | %s", socket, str);
        str = ov_data_pointer_free(str);

        goto error;
    }

    data = ov_json_value_free(data);
    return true;

error:
    /* let socket close in case of media setup errors */
    ov_json_value_free(input);
    ov_json_value_free(data);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool client_authorize(ov_vocs *vocs,
                             int socket,
                             ov_json_value *input) {

    ov_json_value *data = NULL;
    ov_json_value *out = NULL;
    bool result = false;

    if (!vocs || !input || socket < 0) goto error;

    data = ov_socket_json_get(vocs->connections, socket);
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

    ov_socket_json_set(vocs->connections, socket, &data);

    /* close socket if authorization messaging failed */
error:
    ov_json_value_free(out);
    ov_json_value_free(input);
    ov_json_value_free(data);
    return result;
}

/*----------------------------------------------------------------------------*/

static bool client_get(ov_vocs *vocs,
                       int socket,
                       ov_json_value *input) {

    ov_json_value *val = NULL;
    ov_json_value *out = NULL;
    ov_json_value *data = NULL;

    if (!vocs || !input || socket < 0) goto error;

    data = ov_socket_json_get(vocs->connections, socket);
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

        if (val) ov_json_object_set(result_obj, OV_KEY_DOMAIN, val);

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
    if (!ov_json_object_set(res, OV_KEY_RESULT, result_obj)) goto error;

    val = ov_json_string(type);
    if (!ov_json_object_set(res, OV_KEY_TYPE, val)) goto error;

    val = NULL;

response:

    env_send(vocs, socket, out);
    out = ov_json_value_free(out);
    input = ov_json_value_free(input);
    data = ov_json_value_free(data);
    return true;
error:
    val = ov_json_value_free(val);
    out = ov_json_value_free(out);
    input = ov_json_value_free(input);
    data = ov_json_value_free(data);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool client_user_roles(ov_vocs *vocs,
                              int socket,
                              ov_json_value *input) {

    bool result = false;
    ov_json_value *out = NULL;
    ov_json_value *val = NULL;

    ov_json_value *data = NULL;

    if (!vocs || !input || socket < 0) goto error;

    data = ov_socket_json_get(vocs->connections, socket);
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

    result = send_success_response(vocs, input, socket, &out);

error:
    ov_json_value_free(val);
    ov_json_value_free(out);
    ov_json_value_free(input);
    ov_json_value_free(data);
    return result;
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

static bool client_role_loops(ov_vocs *vocs,
                              int socket,
                              ov_json_value *input) {

    bool result = false;
    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *data = NULL;

    if (!vocs || !input || socket < 0) goto error;

    data = ov_socket_json_get(vocs->connections, socket);
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

    result = send_success_response(vocs, input, socket, &out);

error:
    ov_json_value_free(val);
    ov_json_value_free(out);
    ov_json_value_free(input);
    ov_json_value_free(data);
    return result;
}

/*----------------------------------------------------------------------------*/

static bool client_event_get_recording(ov_vocs *vocs,
                                       int socket,
                                       ov_json_value *input) {

    ov_json_value *val = NULL;
    ov_json_value *data = NULL;

    if (!vocs || !input || socket < 0) goto error;

    data = ov_socket_json_get(vocs->connections, socket);
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

        send_error_response(vocs,
                            input,
                            socket,
                            OV_ERROR_CODE_PROCESSING_ERROR,
                            OV_ERROR_DESC_PROCESSING_ERROR);

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
    ov_json_value_free(input);
    ov_json_value_free(data);
    return true;
}

/*----------------------------------------------------------------------------*/

static bool client_event_sip_status(ov_vocs *vocs,
                                    int socket,
                                    ov_json_value *input) {

    ov_json_value *val = NULL;
    ov_json_value *out = NULL;
    ov_json_value *data = NULL;

    if (!vocs || !input || socket < 0) goto error;

    data = ov_socket_json_get(vocs->connections, socket);
    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));

    if (!user) {

        send_error_response(
            vocs, input, socket, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto error;
    }

    bool status = ov_mc_backend_sip_get_connect_status(vocs->sip);

    out = ov_json_object();

    if (status) {
        val = ov_json_true();
    } else {
        val = ov_json_false();
    }

    ov_json_object_set(out, OV_KEY_CONNECTED, val);

    send_success_response(vocs, input, socket, &out);

error:
    ov_json_value_free(out);
    ov_json_value_free(input);
    ov_json_value_free(data);
    return true;
}

/*----------------------------------------------------------------------------*/

static bool client_switch_loop_state(ov_vocs *vocs,
                                     int socket,
                                     ov_json_value *input) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *data = NULL;

    if (!vocs || !input || socket < 0) goto error;

    data = ov_socket_json_get(vocs->connections, socket);
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
        input = ov_json_value_free(input);

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

    /* we requested either some loop aquisition or switch loop */

    if (!ov_event_async_set(
            vocs->async,
            uuid,
            (ov_event_async_data){.socket = socket,
                                  .value = input,
                                  .timedout.userdata = vocs,
                                  .timedout.callback = async_timedout},
            vocs->config.timeout.response_usec)) {

        char *str = ov_json_value_to_string(input);
        ov_log_error("failed to set async msg - closing %i | %s", socket, str);
        str = ov_data_pointer_free(str);

        goto error;
    }

done:
    data = ov_json_value_free(data);
    return true;

error:
    /* let socket close in case of switch media errors */
    ov_json_value_free(input);
    ov_json_value_free(data);
    out = ov_json_value_free(out);
    val = ov_json_value_free(val);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool client_switch_loop_volume(ov_vocs *vocs,
                                      int socket,
                                      ov_json_value *input) {

    ov_json_value *data = NULL;

    if (!vocs || !input || socket < 0) goto error;

    data = ov_socket_json_get(vocs->connections, socket);

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

    if (!ov_event_async_set(
            vocs->async,
            uuid,
            (ov_event_async_data){.socket = socket,
                                  .value = input,
                                  .timedout.userdata = vocs,
                                  .timedout.callback = async_timedout},
            vocs->config.timeout.response_usec)) {

        char *str = ov_json_value_to_string(input);
        ov_log_error("failed to set async msg - closing %i | %s", socket, str);
        str = ov_data_pointer_free(str);

        goto error;
    }

    send_switch_volume_user_broadcast(vocs, socket, loop, percent);

    ov_json_value_free(data);
    return true;
error:
    /* let socket close in case of switch media errors */
    ov_json_value_free(input);
    ov_json_value_free(data);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool client_talking(ov_vocs *vocs,
                           int socket,
                           ov_json_value *input) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *data = NULL;

    if (!vocs || !input || socket < 0) goto error;

    data = ov_socket_json_get(vocs->connections, socket);

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
    ov_json_value_free(input);
    ov_json_value_free(data);
    return true;
}

/*----------------------------------------------------------------------------*/

static bool client_calling(ov_vocs *vocs,
                           int socket,
                           ov_json_value *input) {

    ov_json_value *data = NULL;

    char *uuid_request = NULL;

    if (!vocs || !input || socket < 0) goto error;

    data = ov_socket_json_get(vocs->connections, socket);

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

    if (!ov_event_async_set(
            vocs->async,
            uuid_request,
            (ov_event_async_data){.socket = socket,
                                  .value = input,
                                  .timedout.userdata = vocs,
                                  .timedout.callback = async_timedout},
            vocs->config.timeout.response_usec)) {

        ov_log_error("failed to reset async");
        OV_ASSERT(1 == 0);
        goto error;
    }

    input = NULL;

error:
    /* Do not close socket in case of call errors */
    ov_json_value_free(input);
    ov_json_value_free(data);
    uuid_request = ov_data_pointer_free(uuid_request);
    return true;
}

/*----------------------------------------------------------------------------*/

static bool client_hangup(ov_vocs *vocs,
                          int socket,
                          ov_json_value *input) {

    ov_json_value *data = NULL;

    if (!vocs || !input || socket < 0) goto error;

    data = ov_socket_json_get(vocs->connections, socket);

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
    /* Do not close socket in case of call errors */
    ov_json_value_free(input);
    ov_json_value_free(data);
    return true;
}

/*----------------------------------------------------------------------------*/

static bool client_permit_call(ov_vocs *vocs,
                               int socket,
                               ov_json_value *input) {

    ov_json_value *data = NULL;

    if (!vocs || !input || socket < 0) goto error;

    data = ov_socket_json_get(vocs->connections, socket);

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
    /* Do not close socket in case of call errors */
    ov_json_value_free(input);
    ov_json_value_free(data);
    return true;
}

/*----------------------------------------------------------------------------*/

static bool client_revoke_call(ov_vocs *vocs,
                               int socket,
                               ov_json_value *input) {

    ov_json_value *data = NULL;

    if (!vocs || !input || socket < 0) goto error;

    data = ov_socket_json_get(vocs->connections, socket);

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
    /* Do not close socket in case of call errors */
    ov_json_value_free(input);
    ov_json_value_free(data);
    return true;
}

/*----------------------------------------------------------------------------*/

static bool client_call_function(ov_vocs *vocs,
                                 int socket,
                                 ov_json_value *input,
                                 bool (*function)(ov_mc_backend_sip *sip,
                                                  const char *uuid)) {

    ov_json_value *data = NULL;

    if (!vocs || !input || socket < 0) goto error;

    data = ov_socket_json_get(vocs->connections, socket);

    const char *user = ov_json_string_get(ov_json_get(data, "/" OV_KEY_USER));
    const char *role = ov_json_string_get(ov_json_get(data, "/" OV_KEY_ROLE));
    const char *uuid = ov_event_api_get_uuid(input);

    if (!user || !role || !uuid) {

        send_error_response(
            vocs, input, socket, OV_ERROR_CODE_AUTH, OV_ERROR_DESC_AUTH);

        goto error;
    }

    if (!ov_event_async_set(
            vocs->async,
            uuid,
            (ov_event_async_data){.socket = socket,
                                  .value = input,
                                  .timedout.userdata = vocs,
                                  .timedout.callback = async_timedout},
            vocs->config.timeout.response_usec)) {

        goto error;
    }

    input = NULL;

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

error:
    /* Do not close socket in case of call errors */
    ov_json_value_free(input);
    ov_json_value_free(data);
    return true;
}

/*----------------------------------------------------------------------------*/

static bool client_list_calls(ov_vocs *vocs,
                              int socket,
                              ov_json_value *input) {

    return client_call_function(
        vocs, socket, input, ov_mc_backend_sip_list_calls);
}

/*----------------------------------------------------------------------------*/

static bool client_list_call_permissions(ov_vocs *vocs,
                                         int socket,
                                         ov_json_value *input) {

    return client_call_function(
        vocs, socket, input, ov_mc_backend_sip_list_permissions);
}

/*----------------------------------------------------------------------------*/

static bool client_list_sip_status(ov_vocs *vocs,
                                   int socket,
                                   ov_json_value *input) {

    return client_call_function(
        vocs, socket, input, ov_mc_backend_sip_get_status);
}

/*----------------------------------------------------------------------------*/

static bool client_event_set_keyset_layout(ov_vocs *vocs,
                                           int socket,
                                           ov_json_value *input) {

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

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *data = NULL;

    if (!vocs || !socket || !input) goto error;

    data = ov_socket_json_get(vocs->connections, socket);

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
    input = ov_json_value_free(input);
    data = ov_json_value_free(data);
    return true;
error:
    val = ov_json_value_free(val);
    out = ov_json_value_free(out);
    input = ov_json_value_free(input);
    data = ov_json_value_free(data);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool client_event_get_keyset_layout(ov_vocs *vocs,
                                           int socket,
                                           ov_json_value *input) {

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

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *data = NULL;

    if (!vocs || !socket || !input) goto error;

    data = ov_socket_json_get(vocs->connections, socket);

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
    input = ov_json_value_free(input);
    data = ov_json_value_free(data);
    return true;
error:
    val = ov_json_value_free(val);
    out = ov_json_value_free(out);
    data = ov_json_value_free(data);
    input = ov_json_value_free(input);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool client_event_set_user_data(ov_vocs *vocs,
                                       int socket,
                                       ov_json_value *input) {

    /* checking input:

       {
           "event" : "set_user_data",
           "parameter" :
           {
                // data to set
           }
       }

   */

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *data = NULL;

    if (!vocs || !socket || !input) goto error;

    data = ov_socket_json_get(vocs->connections, socket);
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
    input = ov_json_value_free(input);
    data = ov_json_value_free(data);
    return true;
error:
    val = ov_json_value_free(val);
    out = ov_json_value_free(out);
    data = ov_json_value_free(data);
    input = ov_json_value_free(input);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool client_event_get_user_data(ov_vocs *vocs,
                                       int socket,
                                       ov_json_value *input) {

    /* checking input:

       {
           "event" : "get_user_data",
           "parameter" :
           {
           }
       }

   */

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *data = NULL;

    if (!vocs || !socket || !input) goto error;

    data = ov_socket_json_get(vocs->connections, socket);

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
    input = ov_json_value_free(input);
    data = ov_json_value_free(data);
    return true;
error:
    val = ov_json_value_free(val);
    out = ov_json_value_free(out);
    data = ov_json_value_free(data);
    input = ov_json_value_free(input);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool client_event_register(ov_vocs *vocs,
                                  int socket,
                                  ov_json_value *input) {

    /* checking input:

       {
           "event" : "register",
           "parameter" :
           {
           }
       }

   */

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;

    if (!vocs || !socket || !input) goto error;

    if (!ov_broadcast_registry_set(vocs->broadcasts,
                                   OV_BROADCAST_KEY_SYSTEM_BROADCAST,
                                   socket,
                                   OV_SYSTEM_BROADCAST)) {
        goto error;
    }

    out = ov_event_api_create_success_response(input);

    env_send(vocs, socket, out);
    out = ov_json_value_free(out);
    input = ov_json_value_free(input);
    return true;
error:
    val = ov_json_value_free(val);
    out = ov_json_value_free(out);
    input = ov_json_value_free(input);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool enable_api_client(ov_vocs *vocs){

	if (!set_function(vocs->io, OV_EVENT_API_MEDIA, client_media)) goto error;

    if (!set_function(vocs->io, OV_ICE_STRING_CANDIDATE, client_candidate))
        goto error;

    if (!set_function(vocs->io,
                      OV_ICE_STRING_END_OF_CANDIDATES,
                      client_end_of_candidates))
        goto error;

    if (!set_function(vocs->io, OV_EVENT_API_AUTHORISE, client_authorize))
        goto error;

    if (!set_function(vocs->io, OV_EVENT_API_AUTHORIZE, client_authorize))
        goto error;

    if (!set_function(vocs->io, OV_EVENT_API_GET, client_get)) goto error;

    if (!set_function(vocs->io, OV_EVENT_API_USER_ROLES, client_user_roles))
        goto error;

    if (!set_function(vocs->io, OV_EVENT_API_ROLE_LOOPS, client_role_loops))
        goto error;

    if (!set_function(
            vocs->io, OV_EVENT_API_SWITCH_LOOP_STATE, client_switch_loop_state))
        goto error;

    if (!set_function(vocs->io,
                      OV_EVENT_API_SWITCH_LOOP_VOLUME,
                      client_switch_loop_volume))
        goto error;

    if (!set_function(vocs->io, OV_EVENT_API_TALKING, client_talking))
        goto error;

    if (!set_function(vocs->io, OV_KEY_CALL, client_calling)) goto error;

    if (!set_function(vocs->io, OV_KEY_HANGUP, client_hangup)) goto error;

    if (!set_function(vocs->io, OV_KEY_PERMIT_CALL, client_permit_call))
        goto error;

    if (!set_function(vocs->io, OV_KEY_REVOKE_CALL, client_revoke_call))
        goto error;

    if (!set_function(vocs->io, OV_KEY_LIST_CALLS, client_list_calls))
        goto error;

    if (!set_function(vocs->io,
                      OV_KEY_LIST_CALL_PERMISSIONS,
                      client_list_call_permissions))
        goto error;

    if (!set_function(vocs->io, OV_KEY_LIST_SIP_STATUS, client_list_sip_status))
        goto error;

    if (!set_function(vocs->io,
                      OV_VOCS_DB_SET_KEYSET_LAYOUT,
                      client_event_set_keyset_layout))
        goto error;

    if (!set_function(vocs->io,
                      OV_VOCS_DB_GET_KEYSET_LAYOUT,
                      client_event_get_keyset_layout))
        goto error;

    if (!set_function(
            vocs->io, OV_VOCS_DB_SET_USER_DATA, client_event_set_user_data))
        goto error;

    if (!set_function(
            vocs->io, OV_VOCS_DB_GET_USER_DATA, client_event_get_user_data))
        goto error;

    if (!set_function(
            vocs->io, OV_KEY_GET_RECORDING, client_event_get_recording))
        goto error;

    if (!set_function(vocs->io, OV_KEY_SIP, client_event_sip_status))
        goto error;

    if (!set_function(vocs->io, OV_KEY_REGISTER, client_event_register))
        goto error;

    return true;
error:
	return false;
}