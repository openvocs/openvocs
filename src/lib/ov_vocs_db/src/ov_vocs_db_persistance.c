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
        @file           ov_vocs_db_persistance.c
        @author         Markus

        @date           2022-07-26


        ------------------------------------------------------------------------
*/
#include "../include/ov_vocs_db_persistance.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <lber.h>
#include <ldap.h>

#include <fcntl.h> 
#include <sys/dir.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <ov_base/ov_config_keys.h>
#include <ov_base/ov_dir.h>
#include <ov_base/ov_error_codes.h>
#include <ov_base/ov_file.h>
#include <ov_base/ov_json.h>
#include <ov_base/ov_thread_lock.h>
#include <ov_base/ov_thread_loop.h>
#include <ov_base/ov_thread_message.h>
#include <ov_base/ov_utils.h>

#include <ov_core/ov_event_app.h>
#include <ov_core/ov_event_api.h>
#include <ov_core/ov_broadcast_registry.h>

#define OV_VOCS_DB_PERSISTANCE_MAGIC_BYTE 0x01db
#define IMPL_DEFAULT_LOCK_USEC 100 * 1000          // 100ms
#define IMPL_DEFAULT_LDAP_TIMEOUT_USEC 5000 * 1000 // 5sec
#define IMPL_DEFAULT_PATH "/opt/vocsdb"

#define IMPL_GIT_AUTHOR_NAME "system"
#define IMPL_GIT_AUTHOR_EMAIL "persistance@email"

struct ov_vocs_db_persistance {

    uint16_t magic_byte;
    ov_vocs_db_persistance_config config;

    ov_thread_lock lock;
    ov_thread_loop *thread_loop;

    struct {

        uint32_t auth_snapshot;
        uint32_t state_snapshot;

    } timer;

    ov_event_app *app;
    int socket;

    ov_broadcast_registry *broadcasts;
};

/*----------------------------------------------------------------------------*/

static bool auth_prepare_dir(const char *path) {

    struct stat statbuf = {0};
    errno = 0;

    char sub[PATH_MAX] = {0};

    OV_ASSERT(path);

    if (!ov_dir_access_to_path(path)) {

        if (!ov_dir_tree_create(path)) {
            ov_log_error("Faile to create dir %s", path);
        }

        OV_ASSERT(ov_dir_access_to_path(path));
    }

    /* we clean all config at path */

    DIR *dir = opendir(path);
    if (!dir) {
        goto error;
    }

    struct dirent *entry = NULL;

    while ((entry = readdir(dir)) != NULL) {

        /* we ignore all dot (./ ../ .git/) */
        if (entry->d_name[0] == '.') continue;

        memset(sub, 0, PATH_MAX);
        snprintf(sub, PATH_MAX, "%s/%s", path, entry->d_name);

        if (0 != stat(sub, &statbuf)) goto error;

        mode_t mode = statbuf.st_mode & S_IFMT;

        switch (mode) {

            case S_IFDIR:

                /* remove the child directory with all of it's childs */
                ov_dir_tree_remove(sub);
                break;

            case S_IFREG:
            case S_IFLNK:

                /* remove the file/link */
                unlink(sub);
                break;

            default:
                // ignore
                continue;
        }
    }

    closedir(dir);

    int r = chmod (path, S_IRWXU | S_IRWXG | S_IROTH);
    if (r != 0){

        ov_log_error("Failed to change file access of %s", path);
    } else {

        ov_log_debug("Changed rigths of %s", path);
    }

    return true;

error:
    return false;
}

/*----------------------------------------------------------------------------*/

struct container_write {

    const char *path;
    bool git;
};

/*----------------------------------------------------------------------------*/

static bool save_main_config(const ov_json_value *value,
                             const char *root_path) {

    ov_json_value *out = NULL;

    OV_ASSERT(value);
    OV_ASSERT(root_path);

    char path[PATH_MAX] = {0};

    ssize_t bytes = snprintf(
        path, PATH_MAX, "%s/%s", root_path, OV_VOCS_DB_PERSISTANCE_CONFIG_FILE);

    if (bytes == -1) goto error;

    if (bytes == PATH_MAX) goto error;

    const char *dir_check = ov_file_read_check(root_path);

    if (dir_check && (0 != strcmp(dir_check, OV_FILE_IS_DIR))) {

        if (!ov_dir_tree_create(root_path)) {
            ov_log_error("%s - failed to create directory", root_path);
            goto error;
        }

        

    }

    int r = chmod (root_path,  S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    if (r != 0){

        ov_log_error("Failed to change file access of %s", root_path);
    } 

    if (!ov_json_value_copy((void **)&out, value)) goto error;

    ov_json_object_del(out, OV_KEY_PROJECTS);

    if (!ov_json_write_file(path, out)) goto error;

    r = chmod (path, S_IRUSR | S_IWUSR | S_IWGRP | S_IRGRP | S_IROTH);
    if (r != 0){

        ov_log_error("Failed to change file access of %s", path);
    }

    ov_json_value_free(out);
    return true;
error:
    ov_json_value_free(out);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool save_config(const void *key, void *item, void *data) {

    if (!key) return true;

    if (!item || !data) goto error;

    struct container_write *container = (struct container_write *)data;

    ov_json_value *domain = ov_json_value_cast(item);
    if (!domain) goto error;

    char path[PATH_MAX] = {0};

    ssize_t bytes =
        snprintf(path, PATH_MAX, "%s/%s", container->path, (char *)key);

    if (bytes == -1) goto error;

    if (bytes == PATH_MAX) goto error;

    /* We expect to be here:
     *
     *  {
     *      "domain_name" :         // <- key
     *      {                       // <- item
     *
     *          "id" : ...
     *          "name" : ...
     *          "domain": ...
     *
     *          "users" : {},
     *          "roles" : {},
     *          "loops" : {},
     *          "projects" : {}
     *      }
     *  }
     *
     * We create some folder for each domain and save the config to
     * the domain folder at:
     *
     *      config.json
     *
     * We will create a dedicated folder for each project of the domain:
     *
     *      project
     */

    if (!save_main_config(domain, path)) goto error;

    ov_json_value *projects = ov_json_object_get(domain, OV_KEY_PROJECTS);
    if (!projects) return true;

    /* Change root path and write all projects to subfolder in the same
     * way we wrote the domain */

    struct container_write sub = *container;
    sub.path = path;

    return ov_json_object_for_each(projects, &sub, save_config);

error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool persist_auth(ov_vocs_db_persistance *self) {

    OV_ASSERT(self);
    bool result = false;

    ov_json_value *out =
        ov_vocs_db_eject(self->config.db, OV_VOCS_DB_TYPE_AUTH);
    if (!out) goto done;

    char path[PATH_MAX + 10] = {0};
    snprintf(path, PATH_MAX + 10, "%s/auth", self->config.path);

    if (!auth_prepare_dir(path)) goto done;


    int r = chmod (path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    if (r != 0){

        ov_log_error("Failed to change file access of %s", path);
    } 

    struct container_write container = {

        .path = path};

    result = ov_json_object_for_each(out, &container, save_config);
done:
    out = ov_json_value_free(out);
    return result;
}

/*----------------------------------------------------------------------------*/

static bool timer_auth_snapshot(uint32_t id, void *data) {

    ov_vocs_db_persistance *self = ov_vocs_db_persistance_cast(data);

    OV_ASSERT(self);
    OV_ASSERT(id == self->timer.auth_snapshot);

    self->timer.auth_snapshot = OV_TIMER_INVALID;

    if (!ov_thread_lock_try_lock(&self->lock)) goto reenable_timer;

    if (!persist_auth(self)) ov_log_error("DB failed to persist auth snapshot");

    if (!ov_thread_lock_unlock(&self->lock)) {
        OV_ASSERT(1 == 0);
    }

reenable_timer:

    self->timer.auth_snapshot = ov_event_loop_timer_set(
        self->config.loop,
        self->config.timeout.auth_snapshot_seconds * 1000 * 1000,
        self,
        timer_auth_snapshot);

    return true;
}

/*----------------------------------------------------------------------------*/

static bool persist_state(ov_vocs_db_persistance *self) {

    OV_ASSERT(self);
    bool result = true;

    ov_json_value *out =
        ov_vocs_db_eject(self->config.db, OV_VOCS_DB_TYPE_STATE);
    if (!out) goto done;

    char path[PATH_MAX + 20] = {0};
    snprintf(path, PATH_MAX + 20, "%s/state.json", self->config.path);

    result = ov_json_write_file(path, out);
    out = ov_json_value_free(out);

done:
    return result;
}

/*----------------------------------------------------------------------------*/

static bool timer_state_snapshot(uint32_t id, void *data) {

    ov_vocs_db_persistance *self = ov_vocs_db_persistance_cast(data);

    OV_ASSERT(self);
    OV_ASSERT(id == self->timer.state_snapshot);

    self->timer.state_snapshot = OV_TIMER_INVALID;
    ov_event_loop *loop = self->config.loop;

    if (!ov_thread_lock_try_lock(&self->lock)) goto reenable_timer;

    if (!persist_state(self))
        ov_log_error("DB failed to persist state snapshot");

    if (!ov_thread_lock_unlock(&self->lock)) {
        OV_ASSERT(1 == 0);
    }

reenable_timer:

    self->timer.state_snapshot = ov_event_loop_timer_set(
        loop,
        self->config.timeout.state_snapshot_seconds * 1000 * 1000,
        self,
        timer_state_snapshot);

    return true;
}

/*----------------------------------------------------------------------------*/

static bool handle_in_thread(ov_thread_loop *loop, ov_thread_message *msg);
static bool handle_in_loop(ov_thread_loop *loop, ov_thread_message *msg);

/*----------------------------------------------------------------------------*/

static void cb_socket_connected(void *userdata, int socket){

    ov_vocs_db_persistance *self = ov_vocs_db_persistance_cast(userdata);
    if (!self) goto error;

    self->socket = socket;

    ov_json_value *msg = ov_event_api_message_create(OV_KEY_REGISTER, NULL, 0);
    ov_event_app_send(self->app, socket, msg);
    msg = ov_json_value_free(msg);

error:
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_socket_close(void *userdata, int socket){

    ov_vocs_db_persistance *self = ov_vocs_db_persistance_cast(userdata);
    if (!self) goto error;

    ov_broadcast_registry_unset(self->broadcasts, socket);

    ov_log_debug("unegisterd connection %i", socket);

error:
    return;

}

/*----------------------------------------------------------------------------*/

static void app_cb_register(void *userdata, const char *name, int socket, 
    ov_json_value *input){

    ov_vocs_db_persistance *self = ov_vocs_db_persistance_cast(userdata);
    if (!self || !name) goto error;

    if (!ov_broadcast_registry_set(self->broadcasts, 
        OV_KEY_UPDATE, socket, OV_SYSTEM_BROADCAST)) goto error;

    ov_log_debug("Registerd new connection %i", socket);

error:
    ov_json_value_free(input);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_event_update_password(void *userdata, const char *name, int socket, 
    ov_json_value *input) {

    UNUSED(name);
    UNUSED(socket);

    ov_vocs_db_persistance *self = ov_vocs_db_persistance_cast(userdata);
    if (!self || !input) goto error;

    const char *user = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_USER));
    const char *pass = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_PASSWORD));

    if (!user || !pass) {
        goto done;
    }

    if (!ov_vocs_db_set_password(self->config.db, user, pass)) {
        goto done;
    }

done:
error:
    input = ov_json_value_free(input);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_event_delete(void *userdata, const char *name, int socket, 
    ov_json_value *input) {

    UNUSED(name);
    UNUSED(socket);

    ov_vocs_db_persistance *self = ov_vocs_db_persistance_cast(userdata);
    if (!self || !input) goto error;


    const char *id = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_ID));

    const char *type = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_TYPE));

    if (!id || !type) {

        goto done;
    }

    ov_vocs_db_entity entity = ov_vocs_db_entity_from_string(type);
    if (OV_VOCS_DB_ENTITY_ERROR == entity) {
        goto done;
    }

    if (!ov_vocs_db_delete_entity(self->config.db, entity, id)) {
       goto done;
    }

done:
error:
    input = ov_json_value_free(input);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_event_create(void *userdata, const char *name, int socket, 
    ov_json_value *input) {

    UNUSED(name);
    UNUSED(socket);

    ov_vocs_db_persistance *self = ov_vocs_db_persistance_cast(userdata);
    if (!self || !input) goto error;


    const char *id = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_ID));

    const char *type = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_TYPE));

    const char *scope_id = ov_json_string_get(ov_json_get(
        input, "/" OV_KEY_PARAMETER "/" OV_KEY_SCOPE "/" OV_KEY_ID));

    const char *scope_type = ov_json_string_get(ov_json_get(
        input, "/" OV_KEY_PARAMETER "/" OV_KEY_SCOPE "/" OV_KEY_TYPE));

    if (!id || !type || !scope_id || !scope_type) {

        goto done;
    }

    ov_vocs_db_entity entity = ov_vocs_db_entity_from_string(type);
    if (OV_VOCS_DB_ENTITY_ERROR == entity) {

        goto done;
    }

    ov_vocs_db_scope db_scope = OV_VOCS_DB_SCOPE_PROJECT;

    if (0 == strcmp(scope_type, OV_KEY_DOMAIN)) {

        db_scope = OV_VOCS_DB_SCOPE_DOMAIN;

    }

    if (!ov_vocs_db_create_entity(
            self->config.db, entity, id, db_scope, scope_id)) {

        goto done;
    }

done:
error:
    input = ov_json_value_free(input);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_event_update_key(void *userdata, const char *name, int socket, 
    ov_json_value *input) {

    UNUSED(name);
    UNUSED(socket);

    ov_vocs_db_persistance *self = ov_vocs_db_persistance_cast(userdata);
    if (!self || !input) goto error;


    const char *id = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_ID));

    const char *key = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_KEY));

    const char *type = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_TYPE));

    const ov_json_value *data =
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_DATA);

    if (!id || !type || !key || !data) {

        goto done;
    }

    ov_vocs_db_entity entity = ov_vocs_db_entity_from_string(type);
    if (OV_VOCS_DB_ENTITY_ERROR == entity) {

        goto done;
    }

    if (!ov_vocs_db_update_entity_key(self->config.db, entity, id, key, data)) {

        goto done;
    }

done:
error:
    input = ov_json_value_free(input);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_event_delete_key(void *userdata, const char *name, int socket, 
    ov_json_value *input) {

    UNUSED(name);
    UNUSED(socket);

    ov_vocs_db_persistance *self = ov_vocs_db_persistance_cast(userdata);
    if (!self || !input) goto error;


    const char *id = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_ID));

    const char *key = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_KEY));

    const char *type = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_TYPE));

    if (!id || !type || !key) {

        goto done;
    }

    ov_vocs_db_entity entity = ov_vocs_db_entity_from_string(type);
    if (OV_VOCS_DB_ENTITY_ERROR == entity) {

        goto done;
    }

    if (!ov_vocs_db_delete_entity_key(self->config.db, entity, id, key)) {

        goto done;
    }

done:
error:
    input = ov_json_value_free(input);
    return ;
}

/*----------------------------------------------------------------------------*/

static void cb_event_update(void *userdata, const char *name, int socket, 
    ov_json_value *input) {

    UNUSED(name);
    UNUSED(socket);

    ov_vocs_db_persistance *self = ov_vocs_db_persistance_cast(userdata);
    if (!self || !input) goto error;

    ov_json_value *errors = NULL;

    const char *id = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_ID));

    const ov_json_value *data =
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_DATA);

    const char *type = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_TYPE));

    if (!id || !type || !data) {

        goto done;
    }

    ov_vocs_db_entity entity = ov_vocs_db_entity_from_string(type);
    if (OV_VOCS_DB_ENTITY_ERROR == entity) {

        goto done;
    }

    if (!ov_vocs_db_update_entity_item(
            self->config.db, entity, id, data, &errors)) {

        if (errors) {

            ov_json_value_free(errors);
        }

        goto done;
    }

done:
error:
    input = ov_json_value_free(input);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_event_save(void *userdata, const char *name, int socket, 
    ov_json_value *input) {

    UNUSED(name);
    UNUSED(socket);

    ov_vocs_db_persistance *self = ov_vocs_db_persistance_cast(userdata);
    if (!self || !input) goto error;


    ov_vocs_db_persistance_save(self);

error:
    input = ov_json_value_free(input);
    return ;
}

/*----------------------------------------------------------------------------*/

static void cb_event_set_layout(void *userdata, const char *name, int socket, 
    ov_json_value *input) {

    UNUSED(name);
    UNUSED(socket);

    ov_vocs_db_persistance *self = ov_vocs_db_persistance_cast(userdata);
    if (!self || !input) goto error;


    const char *id = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_ROLE));

    const ov_json_value *data =
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_LAYOUT);

    if (!id || !data) {

        goto done;
    }

    if (!ov_vocs_db_set_layout(self->config.db, id, data)) {

        goto done;
    }

done:
error:
    input = ov_json_value_free(input);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_event_add_domain_admin(void *userdata, const char *name, int socket, 
    ov_json_value *input) {

    UNUSED(name);
    UNUSED(socket);

    ov_vocs_db_persistance *self = ov_vocs_db_persistance_cast(userdata);
    if (!self || !input) goto error;


    const char *domain = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_DOMAIN));

    const char *admin = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_USER));

    if (!domain || !admin) {

        goto done;
    }

    if (!ov_vocs_db_add_domain_admin(self->config.db, domain, admin)) {

        goto done;
    }

done:
error:
    
    input = ov_json_value_free(input);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_event_add_project_admin(void *userdata, const char *name, int socket, 
    ov_json_value *input) {

    UNUSED(name);
    UNUSED(socket);

    ov_vocs_db_persistance *self = ov_vocs_db_persistance_cast(userdata);
    if (!self || !input) goto error;


    const char *project = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_PROJECT));

    const char *admin = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_USER));

    if (!project || !admin) {

        goto done;
    }

    if (!ov_vocs_db_add_project_admin(self->config.db, project, admin)) {

        goto done;
    }

done:
error:
    input = ov_json_value_free(input);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_event_ldap_import(void *userdata, const char *name, int socket, 
    ov_json_value *input) {

    UNUSED(name);
    UNUSED(socket);

    ov_vocs_db_persistance *self = ov_vocs_db_persistance_cast(userdata);
    if (!self || !input) goto error;


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

        goto done;
    }

    if (!ov_vocs_db_persistance_ldap_import(self,
                                            host,
                                            base,
                                            ldap_user,
                                            ldap_pass,
                                            domain)) {

        goto done;
    }

done:
error:
    input = ov_json_value_free(input);
    return;
}

/*----------------------------------------------------------------------------*/

static void cb_event_set_keyset_layout(void *userdata, const char *name, int socket, 
    ov_json_value *input) {

    UNUSED(name);
    UNUSED(socket);

    ov_vocs_db_persistance *self = ov_vocs_db_persistance_cast(userdata);
    if (!self || !input) goto error;


    const char *domain = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_DOMAIN));

    const char *n = ov_json_string_get(
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_NAME));

    const ov_json_value *layout =
        ov_json_get(input, "/" OV_KEY_PARAMETER "/" OV_KEY_LAYOUT);

    if (!domain || !n || !layout) {

        goto done;
    }

    if (!ov_vocs_db_set_keyset_layout(self->config.db, domain, n, layout)) {

        goto done;
    }

done:
error:
    input = ov_json_value_free(input);
    return;
}

/*----------------------------------------------------------------------------*/

static bool register_app_callbacks(ov_vocs_db_persistance *self){

    if (!self) goto error;

    if (!ov_event_app_register(
        self->app,
        OV_KEY_REGISTER,
        self,
        app_cb_register)) goto error;

     if (!ov_event_app_register(
        self->app,
        "update_password",
        self,
        cb_event_update_password)) goto error;

    if (!ov_event_app_register(
        self->app,
        "delete",
        self,
        cb_event_delete)) goto error;

    if (!ov_event_app_register(
        self->app,
        "create",
        self,
        cb_event_create)) goto error;

    if (!ov_event_app_register(
        self->app,
        "update_key",
        self,
        cb_event_update_key)) goto error;

    if (!ov_event_app_register(
        self->app,
        "delete_key",
        self,
        cb_event_delete_key)) goto error;

    if (!ov_event_app_register(
        self->app,
        "update",
        self,
        cb_event_update)) goto error;

    if (!ov_event_app_register(
        self->app,
        "save",
        self,
        cb_event_save)) goto error;

    if (!ov_event_app_register(
        self->app,
        "set_layout",
        self,
        cb_event_set_layout)) goto error;

    if (!ov_event_app_register(
        self->app,
        "set_keyset_layout",
        self,
        cb_event_set_keyset_layout)) goto error;

    if (!ov_event_app_register(
        self->app,
        "add_domain_admin",
        self,
        cb_event_add_domain_admin)) goto error;

    if (!ov_event_app_register(
        self->app,
        "add_project_admin",
        self,
        cb_event_add_project_admin)) goto error;

    if (!ov_event_app_register(
        self->app,
        "ldap_import",
        self,
        cb_event_ldap_import)) goto error;

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

ov_vocs_db_persistance *ov_vocs_db_persistance_create(
    ov_vocs_db_persistance_config config) {

    ov_vocs_db_persistance *self = NULL;

    if (!config.loop || !config.db) goto error;

    if (0 == config.timeout.thread_lock_usec)
        config.timeout.thread_lock_usec = IMPL_DEFAULT_LOCK_USEC;

    if (0 == config.timeout.ldap_request_usec)
        config.timeout.ldap_request_usec = IMPL_DEFAULT_LDAP_TIMEOUT_USEC;

    if (0 == config.path[0]) strncpy(config.path, IMPL_DEFAULT_PATH, PATH_MAX);

    self = calloc(1, sizeof(ov_vocs_db_persistance));
    if (!self) goto error;

    self->magic_byte = OV_VOCS_DB_PERSISTANCE_MAGIC_BYTE;
    self->config = config;

    if (!ov_thread_lock_init(&self->lock, config.timeout.thread_lock_usec))
        goto error;

    if (0 != config.timeout.state_snapshot_seconds) {

        self->timer.state_snapshot = config.loop->timer.set(
            config.loop,
            config.timeout.state_snapshot_seconds * 1000 * 1000,
            self,
            timer_state_snapshot);

        if (OV_TIMER_INVALID == self->timer.state_snapshot) goto error;
    }

    if (0 != config.timeout.auth_snapshot_seconds) {

        self->timer.auth_snapshot = config.loop->timer.set(
            config.loop,
            config.timeout.auth_snapshot_seconds * 1000 * 1000,
            self,
            timer_auth_snapshot);

        if (OV_TIMER_INVALID == self->timer.auth_snapshot) goto error;
    }

    self->thread_loop = ov_thread_loop_create(
        config.loop,
        (ov_thread_loop_callbacks){.handle_message_in_thread = handle_in_thread,
                                   .handle_message_in_loop = handle_in_loop},
        self);

    if (!ov_thread_loop_reconfigure(
            self->thread_loop,
            (ov_thread_loop_config){
                .message_queue_capacity = 100,
                .lock_timeout_usecs = config.timeout.thread_lock_usec,
                .num_threads = 2}))
        goto error;

    if (!ov_thread_loop_start_threads(self->thread_loop)) goto error;

    self->app = ov_event_app_create((ov_event_app_config){
        .io = config.io,
        .callbacks.userdata = self,
        .callbacks.connected = cb_socket_connected,
        .callbacks.close = cb_socket_close
    });

    if (!self->app) goto error;

    if (self->config.cluster.manager){

        ov_log_debug("Starting DB cluster manager.");

        self->socket = ov_event_app_open_listener(
            self->app,
            (ov_io_socket_config){
                .socket = self->config.cluster.socket,
            });

        if (-1 == self->socket){
            
            ov_log_error("Failed to open cluster socket %s:%i",
                self->config.cluster.socket.host,
                self->config.cluster.socket.port);
            goto error;
        
        } else {

             ov_log_debug("opened cluster socket %s:%i",
                self->config.cluster.socket.host,
                self->config.cluster.socket.port);

        }

    } else {

        self->socket = ov_event_app_open_connection(
            self->app,
            (ov_io_socket_config){
                .auto_reconnect = true,
                .socket = self->config.cluster.socket,
            });
    }

    self->broadcasts = ov_broadcast_registry_create(
        (ov_event_broadcast_config){0});

    if (!self->broadcasts) goto error;

    if (!register_app_callbacks(self)) goto error;

    return self;
error:
    ov_vocs_db_persistance_free(self);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_vocs_db_persistance *ov_vocs_db_persistance_free(
    ov_vocs_db_persistance *self) {

    if (!self || !ov_vocs_db_persistance_cast(self)) return self;

    int i = 0;
    int max = 100;

    for (i = 0; i < max; i++) {

        if (ov_thread_lock_try_lock(&self->lock)) break;
    }

    if (i == max) {
        OV_ASSERT(1 == 0);
        return self;
    }

    self->app = ov_event_app_free(self->app);
    self->broadcasts = ov_broadcast_registry_free(self->broadcasts);

    ov_event_loop *loop = self->config.loop;

    if (loop && OV_TIMER_INVALID != self->timer.state_snapshot) {

        loop->timer.unset(loop, self->timer.state_snapshot, NULL);
        self->timer.state_snapshot = OV_TIMER_INVALID;
    }

    if (loop && OV_TIMER_INVALID != self->timer.auth_snapshot) {

        loop->timer.unset(loop, self->timer.auth_snapshot, NULL);
        self->timer.auth_snapshot = OV_TIMER_INVALID;
    }

    ov_thread_loop_stop_threads(self->thread_loop);

    self->thread_loop = ov_thread_loop_free(self->thread_loop);

    /* unlock */
    if (!ov_thread_lock_unlock(&self->lock)) {
        OV_ASSERT(1 == 0);
        return self;
    }

    /* clear lock */
    if (!ov_thread_lock_clear(&self->lock)) {
        OV_ASSERT(1 == 0);
        return self;
    }

    self = ov_data_pointer_free(self);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_vocs_db_persistance *ov_vocs_db_persistance_cast(const void *self) {

    if (!self) goto error;

    if (*(uint16_t *)self == OV_VOCS_DB_PERSISTANCE_MAGIC_BYTE)
        return (ov_vocs_db_persistance *)self;
error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

static bool auth_add_project(ov_json_value *config, const char *path) {

    /* We expect to be at some project folder of some domain folder */

    OV_ASSERT(config);
    OV_ASSERT(path);

    ov_json_value *val = NULL;

    char config_file[PATH_MAX] = {0};
    ssize_t bytes = snprintf(config_file,
                             PATH_MAX,
                             "%s/%s",
                             path,
                             OV_VOCS_DB_PERSISTANCE_CONFIG_FILE);

    if ((bytes < 0) || (bytes == PATH_MAX)) goto error;

    val = ov_json_read_file(config_file);
    if (!val) {
        goto done;
    }

    const char *id = ov_json_string_get(ov_json_object_get(val, OV_KEY_ID));

    if (!id) {
        val = ov_json_value_free(val);
        goto done;
    }

    if (!ov_json_object_set(config, id, val)) goto error;

done:
    return true;
error:
    val = ov_json_value_free(val);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool auth_load_domain(ov_json_value *data, const char *path) {

    /* We expect to be at some domain folder */

    OV_ASSERT(data);
    OV_ASSERT(path);

    struct stat statbuf = {0};
    char sub[PATH_MAX] = {0};

    bool result = false;
    ov_json_value *config = NULL;
    ov_json_value *projects = NULL;

    ssize_t bytes = snprintf(
        sub, PATH_MAX, "%s/%s", path, OV_VOCS_DB_PERSISTANCE_CONFIG_FILE);

    if ((bytes < 0) || (bytes == PATH_MAX)) goto error;

    config = ov_json_read_file(sub);
    if (!config) {
        goto done;
    }

    projects = ov_json_object();

    /* We do add the projects of the domain, configured in sub pathes */

    DIR *dir = opendir(path);
    if (!dir) {
        goto error;
    }

    struct dirent *entry = NULL;

    result = true;

    while ((entry = readdir(dir)) != NULL) {

        /* we ignore all dot files (./ ../ .git/) */
        if (entry->d_name[0] == '.') continue;

        memset(sub, 0, PATH_MAX);
        snprintf(sub, PATH_MAX, "%s/%s", path, entry->d_name);

        if (0 != stat(sub, &statbuf)) {
            result = false;
            break;
        }

        mode_t mode = statbuf.st_mode & S_IFMT;

        switch (mode) {

            case S_IFDIR:
                break;

            default:
                /* ignore files */
                continue;
        }

        result = auth_add_project(projects, sub);

        if (false == result) break;
    }

    closedir(dir);

done:

    if (!result) goto error;

    /* We set the config loaded with the path as key in auth->data */

    const char *id = ov_json_string_get(ov_json_object_get(config, OV_KEY_ID));

    if (!id) {
        ov_log_error("domain config at %s without id - ignoring", path);
        result = false;
        goto error;
    }

    /* add gathered projects */
    if (!ov_json_object_set(config, OV_KEY_PROJECTS, projects)) goto error;

    projects = NULL;

    /* Finaly add domain config to auth->data */
    if (!ov_json_object_set(data, id, config)) goto error;

    return result;
error:
    config = ov_json_value_free(config);
    projects = ov_json_value_free(projects);
    return result;
}

/*----------------------------------------------------------------------------*/

static ov_json_value *load_auth_from_path(const char *path) {

    struct stat statbuf = {0};
    char sub[PATH_MAX] = {0};

    ov_json_value *out = ov_json_object();

    /* We expect some folder with subfolders for each domain */

    DIR *dir = opendir(path);
    if (!dir) {
        ov_log_error("failed to open directory %s", path);
        goto error;
    }

    struct dirent *entry = NULL;

    while ((entry = readdir(dir)) != NULL) {

        /* we ignore all dot files (./ ../ .git/) */
        if (entry->d_name[0] == '.') continue;

        memset(sub, 0, PATH_MAX);
        snprintf(sub, PATH_MAX, "%s/%s", path, entry->d_name);

        if (0 != stat(sub, &statbuf)) goto error;

        mode_t mode = statbuf.st_mode & S_IFMT;

        switch (mode) {

            case S_IFDIR:
                break;

            default:
                continue;
        }

        if (!auth_load_domain(out, sub)) break;
    }

    closedir(dir);
    return out;
error:
    ov_json_value_free(out);
    return NULL;
}

/*----------------------------------------------------------------------------*/

bool ov_vocs_db_persistance_load(ov_vocs_db_persistance *self) {

    bool result = false;
    if (!self) goto error;
    if (!ov_thread_lock_try_lock(&self->lock)) goto error;

    char path[PATH_MAX + 20] = {0};
    snprintf(path, PATH_MAX + 20, "%s/auth", self->config.path);

    ov_json_value *val = load_auth_from_path(path);
    if (!val) goto done;

    result = ov_vocs_db_inject(self->config.db, OV_VOCS_DB_TYPE_AUTH, val);
    if (!result) {
        val = ov_json_value_free(val);
        goto done;
    }

    ov_log_debug("DB load auth from %s", path);

    snprintf(path, PATH_MAX + 20, "%s/state.json", self->config.path);

    val = ov_json_read_file(path);
    if (!val) goto done;

    result &= ov_vocs_db_inject(self->config.db, OV_VOCS_DB_TYPE_STATE, val);
    if (!result) {
        val = ov_json_value_free(val);
        goto done;
    }

    ov_log_debug("DB load state from %s", path);

done:
    if (!ov_thread_lock_unlock(&self->lock)) {
        OV_ASSERT(1 == 0);
        goto error;
    }

    return result;

error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_vocs_db_persistance_save(ov_vocs_db_persistance *self) {

    bool result = false;

    if (!self) goto error;

    if (!ov_thread_lock_try_lock(&self->lock)) goto error;

    result = persist_auth(self);
    result &= persist_state(self);

    if (!ov_thread_lock_unlock(&self->lock)) {
        OV_ASSERT(1 == 0);
        goto error;
    }

    return result;

error:
    return false;
}

/*----------------------------------------------------------------------------*/

ov_vocs_db_persistance_config ov_vocs_db_persistance_config_from_json(
    const ov_json_value *value) {

    ov_vocs_db_persistance_config config = {0};

    const ov_json_value *conf = ov_json_object_get(value, OV_KEY_DB);
    if (!conf) conf = value;

    config.timeout.thread_lock_usec = ov_json_number_get(
        ov_json_get(conf, "/" OV_KEY_TIMEOUT "/" OV_KEY_THREAD_LOCK_TIMEOUT));

    config.timeout.ldap_request_usec = ov_json_number_get(
        ov_json_get(conf, "/" OV_KEY_TIMEOUT "/" OV_KEY_LDAP));

    config.timeout.state_snapshot_seconds = ov_json_number_get(ov_json_get(
        conf, "/" OV_KEY_TIMEOUT "/" OV_KEY_STATE_SNAPSHOT_TIMEOUT));

    config.timeout.auth_snapshot_seconds = ov_json_number_get(
        ov_json_get(conf, "/" OV_KEY_TIMEOUT "/" OV_KEY_AUTH_SNAPSHOT_TIMEOUT));

    const char *path = ov_json_string_get(ov_json_get(conf, "/" OV_KEY_PATH));
    if (path) strncpy(config.path, path, PATH_MAX);

    const ov_json_value *cluster = ov_json_get(conf, "/"OV_KEY_CLUSTER);

    if (ov_json_is_true(ov_json_object_get(cluster, OV_KEY_MANAGER)))
        config.cluster.manager = true;

    config.cluster.socket = ov_socket_configuration_from_json(
        ov_json_object_get(cluster, OV_KEY_SOCKET), (ov_socket_configuration){0});

    return config;
}

/*----------------------------------------------------------------------------*/

static LDAP *ldap_bind(const char *host, const char *user, const char *pass) {

    int version = LDAP_VERSION3;
    LDAP *ld = NULL;
    LDAPMessage *res = NULL;
    int msgid = 0;
    int err = 0;

    char *dn = NULL;

    if (!host || !user || !pass) goto error;

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

        fprintf(stderr,
                "ldap_set_option(PROTOCOL_VERSION): %s\n",
                ldap_err2string(err));
        goto error;
    };

    struct timeval timeout = {.tv_sec = 3};

    err = ldap_set_option(ld, LDAP_OPT_NETWORK_TIMEOUT, &timeout);
    if (err != LDAP_SUCCESS) {

        fprintf(
            stderr, "ldap_set_option(SIZELIMIT): %s\n", ldap_err2string(err));
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
    if (ld) ldap_unbind_ext_s(ld, NULL, NULL);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ldap_get_users(const char *host,
                              const char *base,
                              const char *user,
                              const char *pass,
                              uint64_t timeout_usec) {

    ov_json_value *out = NULL;
    ov_json_value *username = NULL;
    ov_json_value *userid = NULL;
    ov_json_value *val = NULL;

    char *surname = NULL;
    char *forename = NULL;
    char *uid = NULL;

    LDAP *ld = NULL;
    LDAPMessage *res = NULL;

    if (!base || !user || !host || !pass) goto error;

    char *filter = "(&(objectClass=posixAccount))";

    char *attrs[4] = {0};
    attrs[0] = "cn";
    attrs[1] = "sn";
    attrs[2] = "uid";
    attrs[3] = NULL;

    ld = ldap_bind(host, user, pass);
    if (!ld) goto error;

    int err = 0;

    struct timeval timeout = {
        .tv_sec = timeout_usec / 1000000, .tv_usec = timeout_usec % 1000000};

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
    // char name[1000] = {0};

    // loops through entries, attributes, and values
    LDAPMessage *entry = ldap_first_entry(ld, res);
    while ((entry)) {
        BerElement *ber = NULL;

        char *attribute = ldap_first_attribute(ld, entry, &ber);
        while ((attribute)) {

            struct berval **vals = ldap_get_values_len(ld, entry, attribute);
            for (int pos = 0; pos < ldap_count_values_len(vals); pos++) {
                // printf("%i %s: %s\n", pos, attribute, vals[pos]->bv_val);
            }

            if (0 == strcmp(attribute, "sn")) surname = strdup(vals[0]->bv_val);

            if (0 == strcmp(attribute, "cn"))
                forename = strdup(vals[0]->bv_val);

            if (0 == strcmp(attribute, "uid")) uid = strdup(vals[0]->bv_val);

            attribute = ldap_next_attribute(ld, entry, ber);
            ldap_value_free_len(vals);
        };

        ber_free(ber, 0);

        LDAPMessage *new_entry = ldap_next_entry(ld, entry);
        ldap_memfree(entry);
        entry = new_entry;

        if (uid) {

            userid = ov_json_string(uid);
            val = ov_json_object();

            if (!ov_json_object_set(val, OV_KEY_ID, userid)) goto error;

            userid = NULL;

            if (forename) {

                // snprintf(name, 1000, "%s %s", forename, surname);
                username = ov_json_string(forename);

                if (!ov_json_object_set(val, OV_KEY_NAME, username)) goto error;

                username = NULL;
            }

            char *str = ov_json_value_to_string(val);
            if (!str) {

                val = ov_json_value_free(val);

                ov_log_error("MISSED UID %s %s\n\n", uid, forename);

            } else {

                if (!ov_json_object_set(out, uid, val)) goto error;

                val = NULL;
            }

            str = ov_data_pointer_free(str);
        }

        surname = ov_data_pointer_free(surname);
        forename = ov_data_pointer_free(forename);
        uid = ov_data_pointer_free(uid);
    };

    ldap_unbind_ext_s(ld, NULL, NULL);

    return out;
error:
    ov_data_pointer_free(surname);
    ov_data_pointer_free(forename);
    ov_data_pointer_free(uid);
    ov_json_value_free(username);
    ov_json_value_free(userid);
    ov_json_value_free(val);
    ov_json_value_free(out);
    if (ld) ldap_unbind_ext_s(ld, NULL, NULL);
    return NULL;
}

/*----------------------------------------------------------------------------*/

static bool write_new_config(const ov_json_value *users,
                             const char *domain,
                             const char *path) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;

    if (!users || !domain || !path) goto error;

    val = NULL;
    if (!ov_json_value_copy((void **)&val, users)) goto error;

    out = ov_json_object();
    if (!ov_json_object_set(out, OV_KEY_USERS, val)) goto error;

    val = ov_json_string(domain);
    if (!ov_json_object_set(out, OV_KEY_ID, val)) goto error;

    if (!ov_json_write_file(path, out)) goto error;

    out = ov_json_value_free(out);

    ov_log_debug(
        "Created config of users for domain %s at path %s", domain, path);

    return true;
error:
    ov_json_value_free(val);
    ov_json_value_free(out);
    return false;
}

/*----------------------------------------------------------------------------*/

struct users_search {

    ov_json_value const *active_users;
    ov_list *outdated;
    ov_json_value *out;
};

/*----------------------------------------------------------------------------*/

static bool add_new_user(const void *key, void *val, void *data) {

    if (!key) return true;

    char *user_id = (char *)key;
    ov_json_value *user = ov_json_value_cast(val);

    struct users_search *u = (struct users_search *)data;

    ov_json_value *active_users = ov_json_value_cast(u->active_users);

    if (ov_json_object_get(active_users, user_id)) {
        return true;
    } else {

        ov_json_value *item = ov_json_object_get(u->out, OV_KEY_DELETE);
        ov_json_object_set(item, user_id, ov_json_null());
        
        return ov_list_push(u->outdated, user_id);
    }

    ov_json_value *out = NULL;

    if (!ov_json_value_copy((void **)&out, user)) goto error;
    ov_json_value *ldap = ov_json_true();
    ov_json_object_set(out, OV_KEY_LDAP, ldap);

    if (!ov_json_object_set(active_users, user_id, out)) {
        out = ov_json_value_free(out);
        goto error;
    }

    ov_json_value *item = ov_json_object_get(u->out, OV_KEY_ADD);
    ov_json_object_set(item, user_id, ov_json_null());

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool drop_outdated(void *item, void *data) {

    char *key = (char *)item;
    ov_json_value *users = ov_json_value_cast(data);
    return ov_json_object_del(users, key);
}

/*----------------------------------------------------------------------------*/

static ov_json_value *write_users_object(const ov_json_value *users,
                               const char *domain,
                               const char *root_path) {

    char dir_path[PATH_MAX] = {0};
    char file_path[PATH_MAX + 20] = {0};

    ov_list *list = NULL;

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *changes = NULL;

    if (!users || !domain || !root_path) goto error;

    changes = ov_json_object();
    ov_json_object_set(changes, OV_KEY_EVENT, ov_json_string(OV_VOCS_DB_KEY_LDAP_UPDATE));
    ov_json_object_set(changes, OV_KEY_DELETE, ov_json_object());
    ov_json_object_set(changes, OV_KEY_ADD, ov_json_object());

    snprintf(dir_path, PATH_MAX, "%s/auth/%s", root_path, domain);
    snprintf(file_path,
             PATH_MAX + 20,
             "%s/%s",
             dir_path,
             OV_VOCS_DB_PERSISTANCE_CONFIG_FILE);

    ov_dir_tree_create(dir_path);

    ov_json_value *current = ov_json_read_file(file_path);
    if (!current) {

        ov_json_value const *active_users = ov_json_get(current, OV_KEY_USERS);
        ov_json_value *item = ov_json_object_get(changes, OV_KEY_ADD);
        ov_json_value_copy((void**)&val, active_users);
        ov_json_object_set(item, OV_KEY_USERS, val);
        if (!write_new_config(users, domain, file_path))
            goto error;

        return changes;
    }
    const char *domain_id =
        ov_json_string_get(ov_json_get(current, "/" OV_KEY_ID));
    if (!domain_id) {

        ov_log_error(
            "Update for domain %s, "
            "but no domain included in file at path %s",
            domain,
            file_path);

        goto error;
    }

    if (0 != strcmp(domain_id, domain)) {

        ov_log_error(
            "Update for domain %s, "
            "but config of domain %s at path %s",
            domain,
            domain_id,
            file_path);

        goto error;
    }

    ov_json_value const *active_users = ov_json_get(current, OV_KEY_USERS);
    if (!active_users) {

        out = NULL;
        if (!ov_json_value_copy((void **)&out, users)) goto error;

        if (!ov_json_object_set(current, OV_KEY_USERS, out)) goto error;

    } else {

        list = ov_list_create((ov_list_config){0});

        

        struct users_search container = (struct users_search){
            .active_users = active_users, .outdated = list, .out = changes};

        if (!ov_json_object_for_each(
                (ov_json_value *)users, &container, add_new_user))
            goto error;

        if (!ov_list_for_each(list, (void *)active_users, drop_outdated))
            goto error;

        list = ov_list_free(list);
    }

    if (!ov_json_write_file(file_path, current)) goto error;

    ov_log_debug(
        "Update of users for domain %s at path %s - done", domain, file_path);

    return changes;

error:
    ov_json_value_free(val);
    ov_json_value_free(out);
    ov_json_value_free(changes);
    ov_list_free(list);
    return NULL;
}

/*----------------------------------------------------------------------------*/

bool handle_in_thread(ov_thread_loop *loop, ov_thread_message *msg) {

    ov_json_value *users = NULL;
    ov_vocs_db_persistance *self = NULL;

    if (!loop || !msg) goto error;

    self = ov_thread_loop_get_data(loop);

    OV_ASSERT(msg->json_message);

    const char *user =
        ov_json_string_get(ov_json_get(msg->json_message, "/" OV_KEY_USER));

    const char *pass =
        ov_json_string_get(ov_json_get(msg->json_message, "/" OV_KEY_PASSWORD));

    const char *host =
        ov_json_string_get(ov_json_get(msg->json_message, "/" OV_KEY_HOST));

    const char *base =
        ov_json_string_get(ov_json_get(msg->json_message, "/" OV_KEY_BASE));

    const char *domain =
        ov_json_string_get(ov_json_get(msg->json_message, "/" OV_KEY_DOMAIN));

    users = ldap_get_users(
        host, base, user, pass, self->config.timeout.ldap_request_usec);

    if (!users) {
        ov_log_error("Failed to import LDAP users from %s as %s", host, user);
    }

    ov_json_value *changes = write_users_object(users, domain, self->config.path);
    if (!changes) goto error;

    if (!ov_vocs_db_persistance_load(self)){

        ov_log_error("Failed to reload changes.");

    }
    
    ov_vocs_db_send_vocs_trigger(self->config.db, changes);
    changes = ov_json_value_free(changes);

    ov_thread_message_free(msg);
    return true;
error:
    ov_thread_message_free(msg);
    return false;
}

/*----------------------------------------------------------------------------*/

bool handle_in_loop(ov_thread_loop *loop, ov_thread_message *msg) {

    if (!loop || !msg) goto error;

    ov_thread_message_free(msg);
    return true;
error:
    ov_thread_message_free(msg);
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_vocs_db_persistance_ldap_import(ov_vocs_db_persistance *self,
                                        const char *host,
                                        const char *base,
                                        const char *user,
                                        const char *pass,
                                        const char *domain) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_thread_message *msg = NULL;

    if (!self || !host || !base || !user || !pass || !domain) goto error;

    out = ov_json_object();

    val = ov_json_string(host);
    if (!ov_json_object_set(out, OV_KEY_HOST, val)) goto error;

    val = ov_json_string(base);
    if (!ov_json_object_set(out, OV_KEY_BASE, val)) goto error;

    val = ov_json_string(user);
    if (!ov_json_object_set(out, OV_KEY_USER, val)) goto error;

    val = ov_json_string(pass);
    if (!ov_json_object_set(out, OV_KEY_PASSWORD, val)) goto error;

    val = ov_json_string(domain);
    if (!ov_json_object_set(out, OV_KEY_DOMAIN, val)) goto error;

    msg = ov_thread_message_standard_create(1, out);
    if (!msg) goto error;

    if (!ov_thread_loop_send_message(
            self->thread_loop, msg, OV_RECEIVER_THREAD))
        goto error;

    return true;
error:
    ov_json_value_free(out);
    ov_json_value_free(val);
    ov_thread_message_free(msg);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool send_socket(void *userdata, int socket, const ov_json_value *input) {

    ov_vocs_db_persistance *self = ov_vocs_db_persistance_cast(userdata);
    if (!self || !input) return false;

    return ov_event_app_send(self->app, socket, input);
}

/*----------------------------------------------------------------------------*/

bool ov_vocs_db_persistance_broadcast(ov_vocs_db_persistance *self, const ov_json_value *input){

    bool result = false;

    if (!self) goto error;

    if (!ov_vocs_db_persistance_save(self)) goto error;

    if (!self->config.cluster.manager) return true;
    
    ov_event_parameter parameter =
        (ov_event_parameter){.send.instance = self, .send.send = send_socket};

    ov_log_debug("Sending update broadcast.");

    result = ov_broadcast_registry_send(self->broadcasts, OV_KEY_UPDATE, 
        &parameter, input, OV_SYSTEM_BROADCAST);

    return result;
error:
    return false;
}