
static void cluster_update_password(ov_vocs *self, const ov_json_value *msg){

	const char *user = ov_json_string_get(
        ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_USER));
    
    const char *pass = ov_json_string_get(
        ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_PASSWORD));

    if (!ov_vocs_db_set_password(self->config.db, user, pass)) {

    	ov_log_error("failed to update password for user %s", user);
    	goto error;
   	}

   	ov_log_info("updated password for user %s", user);

error:
	return;
}

/*----------------------------------------------------------------------------*/

static void cluster_delete(ov_vocs *self, const ov_json_value *msg){

	const char *id = ov_json_string_get(
        ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_ID));

    const char *type = ov_json_string_get(
        ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_TYPE));

    ov_vocs_db_entity entity = ov_vocs_db_entity_from_string(type);

    if (!ov_vocs_db_delete_entity(self->config.db, entity, id)) {

        ov_log_error("Failed to delete id %s", id);
        goto error;
    }

   	ov_log_info("deleted id %s", id);

error:
	return;
}

/*----------------------------------------------------------------------------*/

static void cluster_create(ov_vocs *self, const ov_json_value *msg){

    const char *id = ov_json_string_get(
        ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_ID));

    const char *type = ov_json_string_get(
        ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_TYPE));

    const char *scope_id = ov_json_string_get(ov_json_get(
        msg, "/" OV_KEY_PARAMETER "/" OV_KEY_SCOPE "/" OV_KEY_ID));

    const char *scope_type = ov_json_string_get(ov_json_get(
        msg, "/" OV_KEY_PARAMETER "/" OV_KEY_SCOPE "/" OV_KEY_TYPE));

    ov_vocs_db_entity entity = ov_vocs_db_entity_from_string(type);

    ov_vocs_db_scope db_scope = OV_VOCS_DB_SCOPE_PROJECT;

    if (0 == strcmp(scope_type, OV_KEY_DOMAIN)) {

        db_scope = OV_VOCS_DB_SCOPE_DOMAIN;

    }

    if (!ov_vocs_db_create_entity(
            self->config.db, entity, id, db_scope, scope_id)) {

    	ov_log_error("failed to create id %s", id);

        goto error;
    }

   	ov_log_info("created id %s", id);

error:
	return;
}

/*----------------------------------------------------------------------------*/

static void cluster_update_key(ov_vocs *self, const ov_json_value *msg){

    const char *id = ov_json_string_get(
        ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_ID));

    const char *key = ov_json_string_get(
        ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_KEY));

    const char *type = ov_json_string_get(
        ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_TYPE));

    const ov_json_value *udata =
        ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_DATA);

    ov_vocs_db_entity entity = ov_vocs_db_entity_from_string(type);

    if (!ov_vocs_db_update_entity_key(self->config.db, entity, id, key, udata)) {

        ov_log_error("failed to update id %s", id);

        goto error;
    }

   	ov_log_info("updated id %s", id);

error:
	return;
}

/*----------------------------------------------------------------------------*/

static void cluster_delete_key(ov_vocs *self, const ov_json_value *msg){

    const char *id = ov_json_string_get(
        ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_ID));

    const char *key = ov_json_string_get(
        ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_KEY));

    const char *type = ov_json_string_get(
        ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_TYPE));

    ov_vocs_db_entity entity = ov_vocs_db_entity_from_string(type);

    if (!ov_vocs_db_delete_entity_key(self->config.db, entity, id, key)) {

        ov_log_error("failed to delete id %s", id);

        goto error;
    }

   	ov_log_info("deleted id %s", id);

error:
	return;
}

/*----------------------------------------------------------------------------*/

static void cluster_update(ov_vocs *self, const ov_json_value *msg){

    const char *id = ov_json_string_get(
        ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_ID));

    const ov_json_value *udata =
        ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_DATA);

    const char *type = ov_json_string_get(
        ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_TYPE));

    ov_vocs_db_entity entity = ov_vocs_db_entity_from_string(type);

    if (!ov_vocs_db_update_entity_item(
            self->config.db, entity, id, udata, NULL)) {

        ov_log_error("failed to update id %s", id);

        goto error;
    }

   	ov_log_info("updated id %s", id);

error:
	return;
}

/*----------------------------------------------------------------------------*/

static void cluster_set_layout(ov_vocs *self, const ov_json_value *msg){

    const char *id = ov_json_string_get(
        ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_ROLE));

    const ov_json_value *udata =
        ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_LAYOUT);

    if (!ov_vocs_db_set_layout(self->config.db, id, udata)) {

        ov_log_error("failed to set layout for %s", id);
        goto error;
    }

   	ov_log_info("set layout for id %s", id);

error:
	return;
}

/*----------------------------------------------------------------------------*/

static void cluster_add_domain_admin(ov_vocs *self, const ov_json_value *msg){

    const char *domain = ov_json_string_get(
        ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_DOMAIN));

    const char *admin = ov_json_string_get(
        ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_USER));

    if (!ov_vocs_db_add_domain_admin(self->config.db, domain, admin)) {

        ov_log_error("failed to add domain admin %s|%s", domain, admin);
        goto error;
    }

   	ov_log_info("added domain admin %s|%s", domain, admin);

error:
	return;
}

/*----------------------------------------------------------------------------*/

static void cluster_add_project_admin(ov_vocs *self, const ov_json_value *msg){

    const char *project = ov_json_string_get(
        ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_PROJECT));

    const char *admin = ov_json_string_get(
        ov_json_get(msg, "/" OV_KEY_PARAMETER "/" OV_KEY_USER));

    if (!ov_vocs_db_add_project_admin(self->config.db, project, admin)) {

        ov_log_error("failed to add project admin %s|%s", project, admin);
        goto error;
    }

   	ov_log_info("added project admin %s|%s", project, admin);

error:
	return;
}

/*----------------------------------------------------------------------------*/

static void cluster_ldap_import(ov_vocs *self, const ov_json_value *msg){

    UNUSED(msg);

  	if (!ov_vocs_db_ldap_import(self->config.db, self->config.ldap.config)){

        ov_log_error("failed to trigger LDAP import");
        goto error;
    }

   	ov_log_info("LDAP import success");

error:
	return;
}

/*----------------------------------------------------------------------------*/

static void io_cluster(void *userdata, ov_json_value *msg){

	ov_vocs *self = ov_vocs_cast(userdata);
	if (!self || !msg) goto error;

	if (ov_event_api_event_is(msg, OV_VOCS_DB_UPDATE_PASSWORD)){

		cluster_update_password(self, msg);

	} else if (ov_event_api_event_is(msg, OV_VOCS_DB_DELETE)){

		cluster_delete(self, msg);

	} else if (ov_event_api_event_is(msg, OV_VOCS_DB_CREATE)){

		cluster_create(self, msg);

	} else if (ov_event_api_event_is(msg, OV_VOCS_DB_UPDATE_KEY)){

		cluster_update_key(self, msg);

	} else if (ov_event_api_event_is(msg, OV_VOCS_DB_DELETE_KEY)){

		cluster_delete_key(self, msg);

	} else if (ov_event_api_event_is(msg, OV_VOCS_DB_UPDATE)){

		cluster_update(self, msg);

	} else if (ov_event_api_event_is(msg, OV_VOCS_DB_SET_LAYOUT)){

		cluster_set_layout(self, msg);

	} else if (ov_event_api_event_is(msg, OV_VOCS_DB_ADD_DOMAIN_ADMIN)){

		cluster_add_domain_admin(self, msg);

	} else if (ov_event_api_event_is(msg, OV_VOCS_DB_ADD_PROJECT_ADMIN)){

		cluster_add_project_admin(self, msg);

	} else if (ov_event_api_event_is(msg, OV_VOCS_DB_LDAP_IMPORT)){

		cluster_ldap_import(self, msg);

	} else {

		char *str = ov_json_value_to_string(msg);
		ov_log_error("Unknown cluster message %s", str);
		str = ov_data_pointer_free(str);
	} 

error:
	msg = ov_json_value_free(msg);
	return;
}

/*----------------------------------------------------------------------------*/