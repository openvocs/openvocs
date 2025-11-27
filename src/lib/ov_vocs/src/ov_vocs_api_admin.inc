/*
 *      ------------------------------------------------------------------------
 *
 *      #API ADMIN EVENTS
 *
 *      ------------------------------------------------------------------------
 */


/*----------------------------------------------------------------------------*/

static bool admin_start_recording(ov_vocs *vocs,
                       int socket,
                       const ov_event_parameter *params,
                       ov_json_value *input) {

	UNUSED(params);

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

        input = ov_json_value_free(input);
        goto done;
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

    if (!ov_vocs_recorder_start_recording(vocs->recorder, loop, uuid)) {
        input = ov_json_value_free(input);
        goto done;
    }

    

done:
    return true;
error:
    ov_json_value_free(input);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool admin_stop_recording(ov_vocs *vocs,
                       int socket,
                       const ov_event_parameter *params,
                       ov_json_value *input) {

	UNUSED(params);

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

        input = ov_json_value_free(input);
        goto done;
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

    if (!ov_vocs_recorder_stop_recording(vocs->recorder, loop, uuid)) {

        send_error_response(vocs,
                            input,
                            socket,
                            OV_ERROR_CODE_PROCESSING_ERROR,
                            OV_ERROR_DESC_PROCESSING_ERROR);

        input = ov_json_value_free(input);
        goto done;
    }

done:
    return true;
error:
    ov_json_value_free(input);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool admin_broadcast(ov_vocs *vocs,
                       int socket,
                       const ov_event_parameter *params,
                       ov_json_value *input) {

	UNUSED(params);

    if (!vocs || !input) goto error;

    if (!ov_event_api_set_type(input, OV_BROADCAST_KEY_SYSTEM_BROADCAST))
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
    
    ov_json_value_free(input);
    return true;
error:
    ov_json_value_free(input);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool admin_get_recorded_loops(ov_vocs *vocs,
                       int socket,
                       const ov_event_parameter *params,
                       ov_json_value *input) {

	UNUSED(params);

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;

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
    input = ov_json_value_free(input);
    return true;
error:
    val = ov_json_value_free(val);
    out = ov_json_value_free(out);
    input = ov_json_value_free(input);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool admin_get_all_loops(ov_vocs *vocs,
                       int socket,
                       const ov_event_parameter *params,
                       ov_json_value *input) {

	UNUSED(params);

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;

    if (!vocs || !input) goto error;

    val = ov_vocs_db_get_all_loops_incl_domain(vocs->config.db);
    if (!val){
        val = ov_json_object();
    }

    out = ov_event_api_create_success_response(input);
    ov_json_value *res = ov_event_api_get_response(out);
    if (!ov_json_object_set(res, OV_KEY_RESULT, val)) goto error;

    val = NULL;

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

struct mixer_data {

    ov_vocs *vocs;
    int socket;
    const char *user;
    ov_json_value *input;

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

    if (!ov_event_async_set(
                container->vocs->async,
                uuid,
                (ov_event_async_data){.socket = container->socket,
                                      .value = out,
                                      .timedout.userdata = container->vocs,
                                      .timedout.callback = async_timedout},
                container->vocs->config.timeout.response_usec)) {

            ov_log_error("failed to set async");
            goto error;
    }

    return true;

error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool admin_get_mixer_state(ov_vocs *vocs,
                       int socket,
                       const ov_event_parameter *params,
                       ov_json_value *input) {

	UNUSED(params);

    ov_json_value *out = NULL;

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

    if (!ov_socket_json_for_each(vocs->connections, &container, call_mixer_state)){

        send_error_response(
            vocs,
            input,
            socket,
            OV_ERROR_CODE_PROCESSING_ERROR,
            OV_ERROR_DESC_PROCESSING_ERROR);

        goto error;
    }

    return true;
error:
    out = ov_json_value_free(out);
    input = ov_json_value_free(input);
    return false;
}


/*----------------------------------------------------------------------------*/

static bool admin_state_mixer(ov_vocs *vocs,
                       int socket,
                       const ov_event_parameter *params,
                       ov_json_value *input) {

	UNUSED(params);

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *res = NULL;

    if (!vocs || !input) goto error;

    ov_mc_backend_registry_count count =
        ov_mc_backend_state_mixers(vocs->backend);

    out = ov_event_api_create_success_response(input);
    res = ov_event_api_get_response(out);

    val = ov_json_number(count.mixers);
    if (!ov_json_object_set(res, OV_KEY_MIXER, val)) goto error;

    val = ov_json_number(count.used);
    if (!ov_json_object_set(res, OV_KEY_USED, val)) goto error;

    env_send(vocs, socket, out);

    ov_json_value_free(out);
    ov_json_value_free(input);
    return true;
error:
    ov_json_value_free(out);
    ov_json_value_free(val);
    ov_json_value_free(input);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool admin_state_connections(ov_vocs *vocs,
                       int socket,
                       const ov_event_parameter *params,
                       ov_json_value *input) {

	UNUSED(params);

    ov_json_value *out = NULL;
    ov_json_value *obj = NULL;
    ov_json_value *val = NULL;
    ov_json_value *res = NULL;

    if (!vocs || !input) goto error;

    out = ov_event_api_create_success_response(input);
    res = ov_event_api_get_response(out);

    obj = ov_json_object();
    if (!ov_json_object_set(res, OV_KEY_CONNECTIONS, obj)) {
        obj = ov_json_value_free(obj);
        goto error;
    }

    if (!ov_socket_json_for_each_set_data(vocs->connections, obj)) goto error;

    env_send(vocs, socket, out);

    ov_json_value_free(out);
    ov_json_value_free(input);
    return true;
error:
    ov_json_value_free(val);
    ov_json_value_free(out);
    ov_json_value_free(input);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool admin_state_session(ov_vocs *vocs,
                       int socket,
                       const ov_event_parameter *params,
                       ov_json_value *input) {

	UNUSED(params);

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

        input = ov_json_value_free(input);
        goto done;
    }

    if (!ov_mc_frontened_get_session_state(vocs->frontend, uuid, session_id)) {

        send_error_response(vocs,
                            input,
                            socket,
                            OV_ERROR_CODE_SESSION_UNKNOWN,
                            OV_ERROR_DESC_SESSION_UNKNOWN);

        input = ov_json_value_free(input);
        goto done;
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

done:
    return true;
error:
    ov_json_value_free(input);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool enable_api_admin(ov_vocs *vocs){

	if (!set_function(vocs->io, OV_EVENT_START_RECORD, admin_start_recording)) goto error;

    if (!set_function(vocs->io, OV_EVENT_STOP_RECORD, admin_stop_recording)) goto error;

    if (!set_function(vocs->io, OV_EVENT_API_STATE_MIXER, admin_state_mixer))
        goto error;

    if (!set_function(vocs->io, OV_EVENT_API_STATE_CONNECTIONS, admin_state_connections))
        goto error;

    if (!set_function(vocs->io, OV_EVENT_API_STATE_SESSION, admin_state_session))
        goto error;

    if (!set_function(vocs->io, "get_mixer_state", admin_get_mixer_state))
        goto error;

    if (!set_function(vocs->io, OV_KEY_BROADCAST, admin_broadcast))
        goto error;

    if (!set_function(vocs->io, "get_recorded_loops", admin_get_recorded_loops))
        goto error;

    if (!set_function(vocs->io, "get_all_loops", admin_get_all_loops))
        goto error;

    return true;
error:
	return false;
}