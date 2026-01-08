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
        @file           ov_vocs_json.c
        @author         Markus Töpfer

        @date           2022-07-27


        ------------------------------------------------------------------------
*/
#include "../include/ov_vocs_json.h"

#include <ov_base/ov_config_keys.h>

const char *ov_vocs_json_get_id(const ov_json_value *val) {

    ov_json_value *itm = ov_json_object_get(val, OV_KEY_ID);
    return ov_json_string_get(itm);
}

/*----------------------------------------------------------------------------*/

bool ov_vocs_json_set_id(ov_json_value *val, const char *id) {

    ov_json_value *out = NULL;
    if (!val || !id)
        goto error;

    out = ov_json_string(id);
    if (!ov_json_object_set(val, OV_KEY_ID, out))
        goto error;

    return true;
error:
    ov_json_value_free(out);
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_vocs_json_set_session_id(ov_json_value *val, const char *id) {

    ov_json_value *out = NULL;
    if (!val || !id)
        goto error;

    out = ov_json_string(id);
    if (!ov_json_object_set(val, OV_KEY_SESSION, out))
        goto error;

    return true;
error:
    ov_json_value_free(out);
    return false;
}

/*----------------------------------------------------------------------------*/

const char *ov_vocs_json_get_name(const ov_json_value *val) {

    ov_json_value *itm = ov_json_object_get(val, OV_KEY_NAME);
    return ov_json_string_get(itm);
}

/*----------------------------------------------------------------------------*/

bool ov_vocs_json_is_admin_user(const ov_json_value *val, const char *user) {

    if (!val || !user)
        goto error;

    ov_json_value *roles = ov_json_object_get(val, OV_KEY_ROLES);
    ov_json_value *role = ov_json_object_get(roles, OV_KEY_ADMIN);
    ov_json_value *users = ov_json_object_get(role, OV_KEY_USERS);

    if (ov_json_object_get(users, user))
        return true;

error:
    return false;
}

/*----------------------------------------------------------------------------*/

const char *ov_vocs_json_get_domain_id(const ov_json_value *project) {

    ov_json_value *projects = project->parent;
    if (!projects)
        goto error;

    ov_json_value *domain = projects->parent;
    if (!domain)
        goto error;

    return ov_vocs_json_get_id(domain);
error:
    return NULL;
}