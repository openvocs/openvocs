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
        @file           ov_vocs_db.c
        @author         Markus Töpfer

        @date           2022-07-23


        ------------------------------------------------------------------------
*/
#include "../include/ov_vocs_db.h"
#include "../include/ov_vocs_db_app.h"
#include "../include/ov_vocs_db_persistance.h"

#include <limits.h>

#include <ov_base/ov_config_keys.h>
#include <ov_base/ov_dict.h>
#include <ov_base/ov_json.h>
#include <ov_base/ov_linked_list.h>
#include <ov_base/ov_list.h>
#include <ov_base/ov_string.h>
#include <ov_base/ov_thread_lock.h>

#include <ov_core/ov_event_api.h>

#define OV_VOCS_DB_MAGIC_BYTE 0xdb42
#define IMPL_DEFAULT_LOCK_USEC 100 * 1000 // 100ms
#define IMPL_DEFAULT_PASSWORD_LENGTH 32

struct ov_vocs_db {

  uint16_t magic_byte;
  ov_vocs_db_config config;

  ov_thread_lock lock;

  ov_vocs_db_persistance *persistance;

  struct {

    ov_dict *domains;
    ov_dict *projects;
    ov_dict *users;
    ov_dict *roles;
    ov_dict *loops;

  } index;

  struct {

    ov_json_value *domains;
    ov_json_value *state;

  } data;
};

/*
 *      ------------------------------------------------------------------------
 *
 *      PRIVATE FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

static bool index_clear(ov_vocs_db *self) {

  if (!self)
    goto error;

  if (!ov_dict_clear(self->index.domains))
    goto error;

  if (!ov_dict_clear(self->index.projects))
    goto error;

  if (!ov_dict_clear(self->index.users))
    goto error;

  if (!ov_dict_clear(self->index.roles))
    goto error;

  if (!ov_dict_clear(self->index.loops))
    goto error;

  return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

static bool vocs_db_clear(ov_vocs_db *self) {

  if (!self)
    goto error;

  index_clear(self);

  self->data.domains = ov_json_value_free(self->data.domains);
  self->data.state = ov_json_value_free(self->data.state);

  return true;
error:
  return false;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_vocs_db *ov_vocs_db_cast(const void *self) {

  if (!self)
    goto error;

  if (*(uint16_t *)self == OV_VOCS_DB_MAGIC_BYTE)
    return (ov_vocs_db *)self;
error:
  return NULL;
}

/*----------------------------------------------------------------------------*/

static void process_trigger(void *userdata, ov_json_value *input) {

  ov_vocs_db *db = ov_vocs_db_cast(userdata);
  if (!db || !input)
    goto error;

  ov_log_debug("Received event %s", ov_event_api_get_event(input));

  TODO("... to be implemented.");
  input = ov_json_value_free(input);
error:
  input = ov_json_value_free(input);
}

/*----------------------------------------------------------------------------*/

ov_vocs_db *ov_vocs_db_create(ov_vocs_db_config config) {

  ov_vocs_db *self = NULL;

  if (0 == config.timeout.thread_lock_usec)
    config.timeout.thread_lock_usec = IMPL_DEFAULT_LOCK_USEC;

  if (0 == config.password.length)
    config.password.length = IMPL_DEFAULT_PASSWORD_LENGTH;

  self = calloc(1, sizeof(ov_vocs_db));
  if (!self)
    goto error;

  self->magic_byte = OV_VOCS_DB_MAGIC_BYTE;
  self->config = config;

  if (!ov_thread_lock_init(&self->lock, config.timeout.thread_lock_usec))
    goto error;

  ov_dict_config dict_config = ov_dict_string_key_config(255);

  self->index.domains = ov_dict_create(dict_config);
  self->index.projects = ov_dict_create(dict_config);
  self->index.users = ov_dict_create(dict_config);
  self->index.roles = ov_dict_create(dict_config);
  self->index.loops = ov_dict_create(dict_config);

  if (!self->index.projects || !self->index.users || !self->index.roles ||
      !self->index.loops)
    goto error;

  if (config.trigger)
    ov_event_trigger_register_listener(
        config.trigger, "DB",
        (ov_event_trigger_data){.userdata = self, .process = process_trigger});

  return self;
error:
  ov_vocs_db_free(self);
  return NULL;
}

/*----------------------------------------------------------------------------*/

ov_vocs_db *ov_vocs_db_free(ov_vocs_db *self) {

  if (!self || !ov_vocs_db_cast(self))
    return self;

  int i = 0;
  int max = 100;

  for (i = 0; i < max; i++) {

    if (ov_thread_lock_try_lock(&self->lock))
      break;
  }

  if (i == max) {
    OV_ASSERT(1 == 0);
    return self;
  }

  vocs_db_clear(self);

  self->index.domains = ov_dict_free(self->index.domains);
  self->index.projects = ov_dict_free(self->index.projects);
  self->index.users = ov_dict_free(self->index.users);
  self->index.roles = ov_dict_free(self->index.roles);
  self->index.loops = ov_dict_free(self->index.loops);

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

bool ov_vocs_db_dump(FILE *stream, ov_vocs_db *self) {

  if (!stream || !self)
    goto error;
  if (!ov_thread_lock_try_lock(&self->lock))
    goto error;

  char *str = ov_json_value_to_string(self->data.domains);
  fprintf(stream, "DOMAINS ----->\n%s\n", str);
  str = ov_data_pointer_free(str);

  str = ov_json_value_to_string(self->data.state);
  fprintf(stream, "STATE ----->\n%s\n", str);
  str = ov_data_pointer_free(str);

  fprintf(stream, "\nINDEX domains ----->\n");
  ov_dict_dump(stream, self->index.domains);

  fprintf(stream, "\nINDEX projects ----->\n");
  ov_dict_dump(stream, self->index.projects);

  fprintf(stream, "\nINDEX users ----->\n");
  ov_dict_dump(stream, self->index.users);

  fprintf(stream, "\nINDEX roles ----->\n");
  ov_dict_dump(stream, self->index.roles);

  fprintf(stream, "\nINDEX loops ----->\n");
  ov_dict_dump(stream, self->index.loops);

  if (!ov_thread_lock_unlock(&self->lock)) {
    OV_ASSERT(1 == 0);
    goto error;
  }

  return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

ov_vocs_db_config ov_vocs_db_config_from_json(const ov_json_value *value) {

  ov_vocs_db_config config = {0};

  const ov_json_value *conf = ov_json_object_get(value, OV_KEY_DB);
  if (!conf)
    conf = value;

  config.timeout.thread_lock_usec = ov_json_number_get(
      ov_json_get(conf, "/" OV_KEY_TIMEOUT "/" OV_KEY_THREAD_LOCK_TIMEOUT));

  config.password.length = ov_json_number_get(
      ov_json_get(conf, "/" OV_KEY_PASSWORD "/" OV_KEY_LENGTH));

  config.password.params.workfactor = ov_json_number_get(
      ov_json_get(conf, "/" OV_KEY_PASSWORD "/" OV_KEY_WORKFACTOR));

  config.password.params.blocksize = ov_json_number_get(
      ov_json_get(conf, "/" OV_KEY_PASSWORD "/" OV_KEY_BLOCKSIZE));

  config.password.params.parallel = ov_json_number_get(
      ov_json_get(conf, "/" OV_KEY_PASSWORD "/" OV_KEY_PARALLEL));

  return config;
}

/*----------------------------------------------------------------------------*/

ov_vocs_db_entity ov_vocs_db_entity_from_string(const char *string) {

  if (!string)
    goto error;

  if (0 == strcmp(string, OV_KEY_USER)) {

    return OV_VOCS_DB_USER;

  } else if (0 == strcmp(string, OV_KEY_ROLE)) {

    return OV_VOCS_DB_ROLE;

  } else if (0 == strcmp(string, OV_KEY_LOOP)) {

    return OV_VOCS_DB_LOOP;

  } else if (0 == strcmp(string, OV_KEY_PROJECT)) {

    return OV_VOCS_DB_PROJECT;

  } else if (0 == strcmp(string, OV_KEY_DOMAIN)) {

    return OV_VOCS_DB_DOMAIN;
  }

error:
  return OV_VOCS_DB_ENTITY_ERROR;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      #INDEX FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

static ov_dict *get_index_dict(ov_vocs_db *self, ov_vocs_db_entity entity) {

  OV_ASSERT(self);

  ov_dict *dict = NULL;

  switch (entity) {

  case OV_VOCS_DB_DOMAIN:
    dict = self->index.domains;
    break;

  case OV_VOCS_DB_PROJECT:
    dict = self->index.projects;
    break;

  case OV_VOCS_DB_LOOP:
    dict = self->index.loops;
    break;

  case OV_VOCS_DB_ROLE:
    dict = self->index.roles;
    break;

  case OV_VOCS_DB_USER:
    dict = self->index.users;
    break;

  default:
    dict = NULL;
  }

  return dict;
}

/*----------------------------------------------------------------------------*/

static bool unindex_users(const void *key, void *val, void *data) {

  if (!key)
    return true;

  UNUSED(val);

  ov_vocs_db *self = ov_vocs_db_cast(data);
  if (!self)
    goto error;

  if (!ov_dict_del(self->index.users, key))
    goto error;

  return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

static bool unindex_roles(const void *key, void *val, void *data) {

  if (!key)
    return true;

  UNUSED(val);

  ov_vocs_db *self = ov_vocs_db_cast(data);
  if (!self)
    goto error;

  if (!ov_dict_del(self->index.roles, key))
    goto error;

  return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

static bool unindex_loops(const void *key, void *val, void *data) {

  if (!key)
    return true;

  UNUSED(val);

  ov_vocs_db *self = ov_vocs_db_cast(data);
  if (!self)
    goto error;

  if (!ov_dict_del(self->index.loops, key))
    goto error;

  return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

static bool unindex_project(ov_vocs_db *self, const char *id) {

  OV_ASSERT(self);
  OV_ASSERT(id);

  ov_json_value *v = ov_dict_get(self->index.projects, id);
  if (!v)
    goto done;

  ov_json_value const *old = ov_json_get(v, "/" OV_KEY_USERS);
  if (old &&
      !ov_json_object_for_each((ov_json_value *)old, self, unindex_users))
    goto error;

  old = ov_json_get(v, "/" OV_KEY_ROLES);
  if (old &&
      !ov_json_object_for_each((ov_json_value *)old, self, unindex_roles))
    goto error;

  old = ov_json_get(v, "/" OV_KEY_LOOPS);
  if (old &&
      !ov_json_object_for_each((ov_json_value *)old, self, unindex_loops))
    goto error;

  if (!ov_dict_del(self->index.projects, id))
    goto error;

done:
  return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

static bool unindex_projects(const void *key, void *val, void *data) {

  if (!key)
    return true;

  UNUSED(val);

  ov_vocs_db *self = ov_vocs_db_cast(data);
  if (!self)
    goto error;

  const char *id = (const char *)key;
  return unindex_project(self, id);
error:
  return false;
}

/*----------------------------------------------------------------------------*/

static bool unindex_domain(ov_vocs_db *self, const char *id) {

  OV_ASSERT(self);
  OV_ASSERT(id);

  ov_json_value const *domain = ov_json_object_get(self->data.domains, id);
  if (!domain)
    goto done;

  ov_json_value const *data = ov_json_get(domain, "/" OV_KEY_PROJECTS);

  if (data &&
      !ov_json_object_for_each((ov_json_value *)data, self, unindex_projects))
    goto error;

  data = ov_json_get(domain, "/" OV_KEY_USERS);
  if (data &&
      !ov_json_object_for_each((ov_json_value *)data, self, unindex_users))
    goto error;

  data = ov_json_get(domain, "/" OV_KEY_ROLES);
  if (data &&
      !ov_json_object_for_each((ov_json_value *)data, self, unindex_roles))
    goto error;

  data = ov_json_get(domain, "/" OV_KEY_LOOPS);
  if (data &&
      !ov_json_object_for_each((ov_json_value *)data, self, unindex_loops))
    goto error;

  ov_dict_del(self->index.domains, id);
done:
  return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

static bool reindex(const void *key, void *val, ov_dict *dict) {

  OV_ASSERT(key);
  OV_ASSERT(val);
  OV_ASSERT(dict);

  char *k = strdup((char *)key);
  return ov_dict_set(dict, k, val, NULL);
}

/*----------------------------------------------------------------------------*/

static bool reindex_users(const void *key, void *val, void *data) {

  if (!key)
    return true;

  ov_vocs_db *db = ov_vocs_db_cast(data);
  if (!db)
    goto error;

  // ov_log_debug("Reindex user %s", (char*) key);
  return reindex(key, val, db->index.users);
error:
  return false;
}

/*----------------------------------------------------------------------------*/

static bool reindex_roles(const void *key, void *val, void *data) {

  if (!key)
    return true;

  ov_vocs_db *db = ov_vocs_db_cast(data);
  if (!db)
    goto error;

  // ov_log_debug("Reindex role %s", (char*) key);
  return reindex(key, val, db->index.roles);
error:
  return false;
}

/*----------------------------------------------------------------------------*/

static bool reindex_loops(const void *key, void *val, void *data) {

  if (!key)
    return true;

  ov_vocs_db *db = ov_vocs_db_cast(data);
  if (!db)
    goto error;

  // ov_log_debug("Reindex loop %s", (char*) key);
  return reindex(key, val, db->index.loops);
error:
  return false;
}

/*----------------------------------------------------------------------------*/

static bool reindex_projects(const void *key, void *val, void *d) {

  if (!key)
    return true;

  ov_vocs_db *db = ov_vocs_db_cast(d);
  if (!db)
    goto error;

  ov_json_value const *data = ov_json_get(val, "/" OV_KEY_LOOPS);
  if (data &&
      !ov_json_object_for_each((ov_json_value *)data, db, reindex_loops))
    goto error;

  data = ov_json_get(val, "/" OV_KEY_ROLES);
  if (data &&
      !ov_json_object_for_each((ov_json_value *)data, db, reindex_roles))
    goto error;

  data = ov_json_get(val, "/" OV_KEY_USERS);
  if (data &&
      !ov_json_object_for_each((ov_json_value *)data, db, reindex_users))
    goto error;

  return reindex(key, val, db->index.projects);
error:
  return false;
}

/*----------------------------------------------------------------------------*/

static bool reindex_domains(const void *key, void *val, void *d) {

  if (!key)
    return true;

  ov_vocs_db *db = ov_vocs_db_cast(d);
  if (!db)
    goto error;

  ov_json_value const *data = ov_json_get(val, "/" OV_KEY_LOOPS);
  if (data &&
      !ov_json_object_for_each((ov_json_value *)data, db, reindex_loops))
    goto error;

  data = ov_json_get(val, "/" OV_KEY_ROLES);
  if (data &&
      !ov_json_object_for_each((ov_json_value *)data, db, reindex_roles))
    goto error;

  data = ov_json_get(val, "/" OV_KEY_USERS);
  if (data &&
      !ov_json_object_for_each((ov_json_value *)data, db, reindex_users))
    goto error;

  data = ov_json_get(val, "/" OV_KEY_PROJECTS);
  if (data &&
      !ov_json_object_for_each((ov_json_value *)data, db, reindex_projects))
    goto error;

  return reindex(key, val, db->index.domains);

error:
  return false;
}

/*----------------------------------------------------------------------------*/

static bool reindex_auth(ov_vocs_db *db) {

  OV_ASSERT(db);

  return ov_json_object_for_each(db->data.domains, db, reindex_domains);
}

/*
 *      ------------------------------------------------------------------------
 *
 *      #Generic FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

static bool delete_password_for_export(const void *key, void *val, void *data) {

  if (!key)
    return true;

  UNUSED(data);
  return ov_json_object_del(val, OV_KEY_PASSWORD);
}

/*----------------------------------------------------------------------------*/

static bool delete_password_in_project(const void *key, void *val, void *data) {

  if (!key)
    return true;

  UNUSED(data);

  ov_json_value const *users = ov_json_get(val, "/" OV_KEY_USERS);

  if (users)
    return ov_json_object_for_each((ov_json_value *)users, NULL,
                                   delete_password_for_export);

  return true;
}

/*----------------------------------------------------------------------------*/

static ov_json_value *lock_db_get(ov_vocs_db *self, const char *id,
                                  ov_dict *index) {

  ov_json_value *cpy = NULL;

  if (!self || !id || !index)
    goto error;

  if (!ov_thread_lock_try_lock(&self->lock))
    goto error;

  ov_json_value *src = ov_dict_get(index, id);
  if (!src)
    goto done;

  ov_json_value_copy((void **)&cpy, src);

done:
  if (!ov_thread_lock_unlock(&self->lock))
    OV_ASSERT(1 == 0);

  /* cleanup passwords for export in user domain or project export */

  ov_json_object_del(cpy, OV_KEY_PASSWORD);

  ov_json_object_for_each((ov_json_value *)ov_json_get(cpy, "/" OV_KEY_USERS),
                          NULL, delete_password_for_export);

  ov_json_object_for_each(
      (ov_json_value *)ov_json_get(cpy, "/" OV_KEY_PROJECTS), NULL,
      delete_password_in_project);

  return cpy;

error:
  return NULL;
}

/*----------------------------------------------------------------------------*/

static bool db_delete(ov_vocs_db *self, const char *id,
                      ov_vocs_db_entity entity) {

  bool result = false;

  if (!self || !id)
    goto error;

  ov_dict *index = get_index_dict(self, entity);

  ov_json_value *src = ov_dict_get(index, id);
  if (!src)
    return true;

  switch (entity) {

  case OV_VOCS_DB_DOMAIN:

    result = unindex_domain(self, id);
    break;

  case OV_VOCS_DB_PROJECT:

    result = unindex_project(self, id);

    break;

  case OV_VOCS_DB_LOOP:

    result = unindex_loops(id, NULL, self);

    break;

  case OV_VOCS_DB_ROLE:

    result = unindex_roles(id, NULL, self);

    break;

  case OV_VOCS_DB_USER:

    result = unindex_users(id, NULL, self);

    break;

  default:
    break;
  }

  if (!result) {
    ov_log_error("DB inconsistency failed to delete index id %s", id);
  }

  ov_json_value *parent = src->parent;
  OV_ASSERT(parent);

  result &= ov_json_object_del(parent, id);
  if (!result) {
    ov_log_error("DB inconsistency failed to delete id %s", id);
  }

  return result;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

ov_vocs_db_parent ov_vocs_db_get_parent(ov_vocs_db *self,
                                        ov_vocs_db_entity entity,
                                        const char *id) {

  ov_vocs_db_parent result = {0};

  if (!self || !id)
    goto error;

  if (!ov_thread_lock_try_lock(&self->lock))
    goto error;

  ov_dict *index = get_index_dict(self, entity);
  if (!index)
    goto done;
  /*
      {
          "users": {},
          "roles": {},
          "loops": {},
          "projects" : {
              "id" : {
                  "users": {},
                  "roles": {},
                  "loops": {}
              }
          }
      }
  */
  ov_json_value *item = ov_dict_get(index, id);
  if (!item)
    goto done;

  if (entity == OV_VOCS_DB_DOMAIN) {
    result.scope = OV_VOCS_DB_SCOPE_DOMAIN;
    result.id = strdup(id);
    goto done;
  }

  ov_json_value *first_up = item->parent;      // users, roles, loops, projects
  ov_json_value *second_up = first_up->parent; // project or domain
  // ov_json_value *third_up = second_up->parent; // root or projects

  if (ov_json_get(second_up, "/" OV_KEY_PROJECTS)) {
    result.scope = OV_VOCS_DB_SCOPE_DOMAIN;
    const char *id = ov_json_string_get(ov_json_get(second_up, "/" OV_KEY_ID));
    if (id)
      result.id = strdup(id);
  } else {
    result.scope = OV_VOCS_DB_SCOPE_PROJECT;
    const char *id = ov_json_string_get(ov_json_get(second_up, "/" OV_KEY_ID));
    if (id)
      result.id = strdup(id);
  }
done:

  if (!ov_thread_lock_unlock(&self->lock)) {
    OV_ASSERT(1 == 0);
    goto error;
  }

  return result;
error:
  return (ov_vocs_db_parent){0};
}

/*----------------------------------------------------------------------------*/

bool ov_vocs_db_parent_clear(ov_vocs_db_parent *parent) {

  if (!parent)
    goto error;

  parent->id = ov_data_pointer_free(parent->id);
  parent->scope = 0;
  return true;

error:
  return false;
}
/*----------------------------------------------------------------------------*/

ov_json_value *ov_vocs_db_get_entity(ov_vocs_db *self, ov_vocs_db_entity entity,
                                     const char *id) {

  if (!self || !id)
    goto error;
  ov_dict *index = get_index_dict(self, entity);

  return lock_db_get(self, id, index);
error:
  return NULL;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_vocs_db_get_entity_domain(ov_vocs_db *self,
                                            ov_vocs_db_entity entity,
                                            const char *id) {

  ov_json_value *out = NULL;

  if (!self || !id)
    goto error;

  const char *domain_id = NULL;
  const char *project_id = NULL;

  ov_dict *index = get_index_dict(self, entity);

  if (!ov_thread_lock_try_lock(&self->lock))
    goto error;

  ov_json_value *src = ov_dict_get(index, id);
  if (!src)
    goto done;

  ov_json_value *parent = src->parent;
  if (!parent)
    goto done;

  ov_json_value *project_or_domain = parent->parent;
  if (!project_or_domain)
    goto done;

  ov_json_value *projects_or_root = project_or_domain->parent;
  if (!projects_or_root)
    goto done;

  ov_json_value *root = projects_or_root->parent;

  if (projects_or_root == self->data.domains) {

    domain_id =
        ov_json_string_get(ov_json_object_get(project_or_domain, OV_KEY_ID));

  } else {

    project_id =
        ov_json_string_get(ov_json_object_get(project_or_domain, OV_KEY_ID));

    domain_id = ov_json_string_get(ov_json_object_get(root, OV_KEY_ID));
  }

  out = ov_json_object();
  if (!out)
    goto done;

  ov_json_object_set(out, OV_KEY_DOMAIN, ov_json_string(domain_id));
  if (project_id)
    ov_json_object_set(out, OV_KEY_PROJECT, ov_json_string(project_id));

done:
  if (!ov_thread_lock_unlock(&self->lock))
    OV_ASSERT(1 == 0);

  return out;
error:
  ov_json_value_free(out);
  return NULL;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_vocs_db_get_entity_key(ov_vocs_db *self,
                                         ov_vocs_db_entity entity,
                                         const char *id, const char *key) {

  ov_json_value *result = ov_json_null();

  if (!self || !id)
    goto error;
  ov_dict *index = get_index_dict(self, entity);

  if (!ov_thread_lock_try_lock(&self->lock))
    goto error;

  // never return a password
  if (0 == strcmp(key, OV_KEY_PASSWORD))
    goto done;

  ov_json_value *item = ov_dict_get(index, id);
  if (!item)
    goto done;
  ov_json_value *data = ov_json_object_get(item, key);
  if (data) {
    result = ov_json_value_free(result);
    ov_json_value_copy((void **)&result, data);
  }

done:
  if (!ov_thread_lock_unlock(&self->lock)) {
    OV_ASSERT(1 == 0);
    goto error;
  }

  return result;
error:
  ov_json_value_free(result);
  return NULL;
}

/*----------------------------------------------------------------------------*/

static bool delete_in_parent(const void *key, void *val, void *data,
                             const char *item) {

  if (!key)
    return true;

  const char *id = (const char *)data;
  ov_json_value *v = ov_json_value_cast(val);
  if (!id || !v)
    goto error;

  ov_json_value *entry = ov_json_object_get(v, item);

  if (entry)
    return ov_json_object_del(entry, id);

  return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

static bool delete_loop_in_parent(const void *key, void *val, void *data) {

  return delete_in_parent(key, val, data, OV_KEY_LOOPS);
}

/*----------------------------------------------------------------------------*/

static bool delete_role_in_parent(const void *key, void *val, void *data) {

  return delete_in_parent(key, val, data, OV_KEY_ROLES);
}

/*----------------------------------------------------------------------------*/

static bool delete_user_in_parent(const void *key, void *val, void *data) {

  return delete_in_parent(key, val, data, OV_KEY_USERS);
}

/*----------------------------------------------------------------------------*/

static bool delete_project_in_parent(const void *key, void *val, void *data) {

  return delete_in_parent(key, val, data, OV_KEY_PROJECTS);
}

/*----------------------------------------------------------------------------*/

bool ov_vocs_db_delete_entity(ov_vocs_db *self, ov_vocs_db_entity entity,
                              const char *id) {

  bool result = false;

  if (!self || !id)
    goto error;

  if (!ov_thread_lock_try_lock(&self->lock))
    goto error;

  result = db_delete(self, id, entity);

  switch (entity) {

  case OV_VOCS_DB_DOMAIN:

    result &= unindex_domain(self, id);
    break;

  case OV_VOCS_DB_PROJECT:

    result &= ov_dict_for_each(self->index.domains, (void *)id,
                               delete_project_in_parent);

    break;

  case OV_VOCS_DB_LOOP:

    result &= ov_dict_for_each(self->index.domains, (void *)id,
                               delete_loop_in_parent);

    result &= ov_dict_for_each(self->index.projects, (void *)id,
                               delete_loop_in_parent);

    break;

  case OV_VOCS_DB_ROLE:

    result &= ov_dict_for_each(self->index.domains, (void *)id,
                               delete_role_in_parent);

    result &= ov_dict_for_each(self->index.projects, (void *)id,
                               delete_role_in_parent);

    break;

  case OV_VOCS_DB_USER:

    result &=
        ov_dict_for_each(self->index.roles, (void *)id, delete_user_in_parent);

    break;

  default:
    result = false;
  }

  if (!ov_thread_lock_unlock(&self->lock)) {
    OV_ASSERT(1 == 0);
    goto error;
  }

  return result;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

static ov_json_value *get_create_parent(ov_json_value *data, const char *key) {

  OV_ASSERT(data);
  OV_ASSERT(key);

  ov_json_value *parent = ov_json_object_get(data, key);

  if (!parent) {

    parent = ov_json_object();

    if (!ov_json_object_set(data, key, parent)) {
      parent = ov_json_value_free(parent);
      goto error;
    }
  }
  return parent;
error:
  return NULL;
}

/*----------------------------------------------------------------------------*/

static ov_json_value *get_parent(ov_vocs_db *self, ov_vocs_db_entity entity,
                                 ov_vocs_db_scope scope, const char *scope_id) {

  OV_ASSERT(self);

  ov_json_value *data = NULL;
  ov_json_value *parent = NULL;

  switch (entity) {

  case OV_VOCS_DB_DOMAIN:

    if (scope != OV_VOCS_DB_SCOPE_DOMAIN)
      goto error;

    if (NULL == self->data.domains)
      self->data.domains = ov_json_object();

    parent = self->data.domains;

    goto done;
    break;

  default:
    break;
  }

  switch (scope) {

  case OV_VOCS_DB_SCOPE_PROJECT:

    data = ov_dict_get(self->index.projects, scope_id);
    if (!data)
      goto error;
    break;

  case OV_VOCS_DB_SCOPE_DOMAIN:

    data = ov_json_object_get(self->data.domains, scope_id);
    if (!data)
      goto error;
    break;
  }

  switch (entity) {

  case OV_VOCS_DB_DOMAIN:

    if (scope != OV_VOCS_DB_SCOPE_DOMAIN)
      goto error;
    parent = self->data.domains;
    break;

  case OV_VOCS_DB_PROJECT:

    if (scope != OV_VOCS_DB_SCOPE_DOMAIN)
      goto error;
    OV_ASSERT(data);
    parent = get_create_parent(data, OV_KEY_PROJECTS);
    break;

  case OV_VOCS_DB_USER:

    OV_ASSERT(data);
    parent = get_create_parent(data, OV_KEY_USERS);
    break;

  case OV_VOCS_DB_LOOP:

    OV_ASSERT(data);
    parent = get_create_parent(data, OV_KEY_LOOPS);
    break;

  case OV_VOCS_DB_ROLE:

    parent = get_create_parent(data, OV_KEY_ROLES);
    break;

  default:
    break;
  }

done:
  return parent;
error:
  return NULL;
}

/*----------------------------------------------------------------------------*/

bool ov_vocs_db_create_entity(ov_vocs_db *self, ov_vocs_db_entity entity,
                              const char *id, ov_vocs_db_scope scope,
                              const char *scope_id) {

  bool result = false;

  if (!self || !id)
    goto error;

  if (!ov_thread_lock_try_lock(&self->lock))
    goto error;

  /* check if id is already in use for the entity type */

  ov_dict *dict = get_index_dict(self, entity);

  switch (entity) {

  case OV_VOCS_DB_ROLE:

    if (0 == strcmp(id, OV_KEY_ADMIN))
      break;
    if (ov_dict_is_set(dict, id))
      goto done;

    break;

  default:
    if (ov_dict_is_set(dict, id))
      goto done;
  }

  ov_json_value *parent = get_parent(self, entity, scope, scope_id);
  if (!parent)
    goto done;

  ov_json_value *out = ov_json_object();
  ov_json_value *val = ov_json_string(id);

  if (!ov_json_object_set(out, OV_KEY_ID, val)) {
    out = ov_json_value_free(out);
    val = ov_json_value_free(val);
    goto done;
  }

  if (!ov_json_object_set(parent, id, out)) {
    out = ov_json_value_free(out);
    goto done;
  }

  switch (entity) {

  case OV_VOCS_DB_ROLE:

    if (0 == strcmp(id, OV_KEY_ADMIN)) {
      result = true;
      break;
    }

    result = reindex(id, out, dict);
    break;

  default:
    result = reindex(id, out, dict);
  }

done:

  if (!ov_thread_lock_unlock(&self->lock)) {
    OV_ASSERT(1 == 0);
    goto error;
  }

  // ov_vocs_db_persistance_persist(self->persistance);

  return result;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

struct container_parent {

  ov_vocs_db *db;
  ov_json_value *parent;
};

/*----------------------------------------------------------------------------*/

static bool user_already_set(const void *key, void *val, void *data) {

  if (!key)
    return true;
  UNUSED(val);

  struct container_parent *c = (struct container_parent *)data;

  /*
      Check if the user id is registered in db.

      If the user is already registered,
      but not part of the parent,
      some other project or domain is using the same id.
  */

  if (ov_dict_is_set(c->db->index.users, key)) {

    if (!ov_json_object_get(c->parent, key))
      goto error;
  }

  return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

struct container_users {

  ov_json_value *users;
  ov_list *list;
};

/*----------------------------------------------------------------------------*/

static bool add_user_to_drop(const void *key, void *val, void *data) {

  if (!key)
    return true;
  UNUSED(val);
  struct container_users *container = (struct container_users *)data;

  if (ov_json_object_get(container->users, key))
    return true;

  return ov_list_push(container->list, (void *)key);
}

/*----------------------------------------------------------------------------*/

static bool drop_user(void *val, void *data) {

  ov_json_value *users = ov_json_value_cast(data);
  return ov_json_object_del(users, val);
}

/*----------------------------------------------------------------------------*/

static bool add_new_user(const void *key, void *val, void *data) {

  if (!key)
    return true;
  UNUSED(val);

  ov_json_value *users = ov_json_value_cast(data);

  if (ov_json_object_get(users, key))
    return true;

  ov_json_value *copy = NULL;

  if (!ov_json_value_copy((void **)&copy, val))
    goto error;
  if (!ov_json_object_set(users, key, copy))
    goto error;

  return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

static bool update_users(ov_vocs_db *self, ov_json_value *parent,
                         const ov_json_value *users) {

  bool result = false;

  ov_list *list = NULL;

  OV_ASSERT(self);
  OV_ASSERT(parent);
  OV_ASSERT(users);

  if (!ov_json_is_object(users))
    goto error;

  ov_json_value *data = (ov_json_value *)ov_json_get(parent, "/" OV_KEY_USERS);

  if (!data) {
    data = ov_json_object();
    ov_json_object_set(parent, OV_KEY_USERS, data);
  }

  struct container_parent c =
      (struct container_parent){.db = self, .parent = data};

  if (!ov_json_object_for_each((ov_json_value *)users, &c, user_already_set))
    goto error;

  if (data && !ov_json_object_for_each(data, self, unindex_users))
    goto error;

  list = ov_linked_list_create((ov_list_config){0});

  struct container_users container =
      (struct container_users){.users = (ov_json_value *)users, .list = list};

  if (!ov_json_object_for_each(data, &container, add_user_to_drop))
    goto error;

  if (!ov_list_for_each(list, data, drop_user))
    goto error;

  if (!ov_json_object_for_each((ov_json_value *)users, data, add_new_user))
    goto error;

  result = ov_json_object_for_each(data, self, reindex_users);

error:
  ov_list_free(list);
  return result;
}

/*----------------------------------------------------------------------------*/

static bool role_already_set(const void *key, void *val, void *data) {

  if (!key)
    return true;
  UNUSED(val);

  struct container_parent *c = (struct container_parent *)data;

  if (0 == strcmp(key, OV_KEY_ADMIN))
    goto done;

  if (ov_dict_is_set(c->db->index.roles, key)) {

    if (!ov_json_object_get(c->parent, key))
      goto error;
  }

done:
  return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

static bool update_roles(ov_vocs_db *self, ov_json_value *parent,
                         const ov_json_value *roles) {

  bool result = false;
  ov_json_value *cpy = NULL;

  OV_ASSERT(self);
  OV_ASSERT(parent);
  OV_ASSERT(roles);

  if (!ov_json_is_object(roles))
    goto error;

  ov_json_value *data = (ov_json_value *)ov_json_get(parent, "/" OV_KEY_ROLES);
  if (!data) {
    data = ov_json_object();
    ov_json_object_set(parent, OV_KEY_ROLES, data);
  }

  struct container_parent c =
      (struct container_parent){.db = self, .parent = data};

  if (!ov_json_object_for_each((ov_json_value *)roles, &c, role_already_set))
    goto error;

  if (!ov_json_value_copy((void **)&cpy, roles))
    goto error;

  if (data && !ov_json_object_for_each(data, self, unindex_roles))
    goto error;
  if (!ov_json_object_set(parent, OV_KEY_ROLES, cpy))
    goto error;

  result = ov_json_object_for_each(cpy, self, reindex_roles);
  cpy = NULL;

error:
  ov_json_value_free(cpy);
  return result;
}

/*----------------------------------------------------------------------------*/

static bool loop_already_set(const void *key, void *val, void *data) {

  if (!key)
    return true;
  UNUSED(val);

  struct container_parent *c = (struct container_parent *)data;

  if (ov_dict_is_set(c->db->index.loops, key)) {

    if (!ov_json_object_get(c->parent, key))
      goto error;
  }

  return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

struct container_sip {

  ov_json_value **out;
  const ov_json_value *parent;
};

/*----------------------------------------------------------------------------*/

static bool push_to_whitelist(void *item, void *data) {

  ov_json_value *new = NULL;

  ov_json_value *value = ov_json_value_cast(item);
  ov_json_value *array = ov_json_value_cast(data);

  if (!ov_json_value_copy((void **)&new, value))
    goto error;
  if (!ov_json_array_push(array, new))
    goto error;

  return true;
error:
  ov_json_value_free(new);
  return false;
}

/*----------------------------------------------------------------------------*/

static bool add_sip_permission_for_loop(const void *key, void *val,
                                        void *data) {

  if (!key)
    return true;

  ov_json_value *loop = ov_json_value_cast(val);
  ov_json_value **out = (ov_json_value **)data;

  ov_json_value *sip = ov_json_object_get(loop, OV_KEY_SIP);
  ov_json_value *whitelist = ov_json_object_get(sip, OV_KEY_WHITELIST);

  if (!whitelist)
    return true;

  if (!*out)
    *out = ov_json_object();

  ov_json_value *loopdata = ov_json_object_get(*out, key);

  if (!loopdata) {

    loopdata = ov_json_object();
    ov_json_object_set(*out, key, loopdata);
  }

  ov_json_value *array = ov_json_array();
  ov_json_object_set(loopdata, OV_KEY_PERMIT, array);

  return ov_json_array_for_each(whitelist, array, push_to_whitelist);
}

/*----------------------------------------------------------------------------*/

static bool revoke_calls(void *item, void *data) {

  ov_json_value *loopdata = data;
  ov_json_value *new_whitelist = ov_json_object_get(loopdata, OV_KEY_PERMIT);

  size_t count = ov_json_array_count(new_whitelist);
  bool found = false;

  for (size_t i = 1; i <= count; i++) {

    bool ok = true;

    ov_json_value *new = ov_json_array_get(new_whitelist, i);

    ov_sip_permission old_permission = ov_sip_permission_from_json(item, &ok);
    if (!ok)
      goto error;

    ov_sip_permission new_permission = ov_sip_permission_from_json(new, &ok);
    if (!ok)
      goto error;

    if (ov_sip_permission_equals(old_permission, new_permission))
      found = true;
  }

  if (found)
    return true;

  ov_json_value *revoke = ov_json_object_get(data, OV_KEY_REVOKE);
  if (!revoke) {
    revoke = ov_json_array();
    ov_json_object_set(data, OV_KEY_REVOKE, revoke);
  }

  ov_json_value *out = NULL;
  ov_json_value_copy((void **)&out, item);

  ov_json_array_push(revoke, out);
  return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

static bool update_sip_parameter_processing(const char *loop_id,
                                            const ov_json_value *old,
                                            const ov_json_value *new,
                                            ov_json_value **out) {

  ov_json_value *old_whitelist = ov_json_object_get(old, OV_KEY_WHITELIST);
  ov_json_value *new_whitelist = ov_json_object_get(new, OV_KEY_WHITELIST);

  if (!*out)
    *out = ov_json_object();

  ov_json_value *loopdata = ov_json_object();
  if (!ov_json_object_set(*out, loop_id, loopdata))
    goto error;

  ov_json_value *cpy = NULL;
  if (!ov_json_value_copy((void **)&cpy, new_whitelist))
    goto error;

  if (!ov_json_object_set(loopdata, OV_KEY_PERMIT, cpy))
    goto error;

  if (old_whitelist) {

    ov_json_array_for_each(old_whitelist, loopdata, revoke_calls);
  }

  return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

static bool check_old_loop_in_new_loops(const void *key, void *val,
                                        void *data) {

  if (!key)
    return true;

  ov_json_value *loop = ov_json_value_cast(val);
  struct container_sip *container = (struct container_sip *)data;

  ov_json_value *new_loop = ov_json_object_get(container->parent, key);

  ov_json_value *old_sip = ov_json_object_get(loop, OV_KEY_SIP);
  ov_json_value *new_sip = ov_json_object_get(new_loop, OV_KEY_SIP);

  return update_sip_parameter_processing(key, old_sip, new_sip, container->out);
}

/*----------------------------------------------------------------------------*/

static bool check_new_loop_in_old_loops(const void *key, void *val,
                                        void *data) {

  if (!key)
    return true;

  ov_json_value *loop = ov_json_value_cast(val);
  struct container_sip *container = (struct container_sip *)data;

  ov_json_value *old_loop = ov_json_object_get(container->parent, key);

  if (old_loop)
    return true;

  return add_sip_permission_for_loop(key, loop, container->out);
}

/*----------------------------------------------------------------------------*/

static bool check_for_sip_update(const void *key, void *val, void *data) {

  if (!key)
    return true;

  ov_json_value *new_loop = ov_json_value_cast(val);
  ov_json_value *new_loops = new_loop->parent;

  struct container_sip *container = (struct container_sip *)data;
  ov_json_value *old_loops = (ov_json_value *)container->parent;

  if (!old_loops) {

    // we need to add all SIP permission of new_loops

    if (!ov_json_object_for_each(new_loops, container->out,
                                 add_sip_permission_for_loop))
      goto error;

  } else {

    // if oldloop is contained in newloops, update sip parameter
    // if oldloop is not contained in newloops, drop oldloop parameter
    // if newloop is not contained in oldloops, add sip parameter

    struct container_sip container2 =
        (struct container_sip){.out = container->out, .parent = new_loops};

    ov_json_object_for_each(old_loops, &container2,
                            check_old_loop_in_new_loops);

    container2 =
        (struct container_sip){.out = container->out, .parent = old_loops};

    ov_json_object_for_each(new_loops, &container2,
                            check_new_loop_in_old_loops);
  }

  return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

static bool check_sip_processing(const ov_json_value *old,
                                 const ov_json_value *new,
                                 ov_json_value **out) {

  struct container_sip container =
      (struct container_sip){.out = out, .parent = old};

  /*
      {
          "parent" :
          {
              "loops" :
              {
                  {
                      "loopid" :
                      {
                          SIP" :
                          {
                              "whitelist" :
                              [
                                  { ... }
                              ]
                          }
                      }
                  }
              }
          }
      }
  */

  if (!ov_json_object_for_each((ov_json_value *)new, &container,
                               check_for_sip_update))
    goto error;

  return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

static bool update_domain_projects(ov_vocs_db *self, ov_json_value *domain,
                                   const ov_json_value *projects,
                                   ov_json_value **out) {

  bool result = false;
  ov_json_value *cpy = NULL;

  OV_ASSERT(self);
  OV_ASSERT(domain);
  OV_ASSERT(projects);

  if (!ov_json_is_object(projects))
    goto error;

  if (!ov_json_value_copy((void **)&cpy, projects))
    goto error;

  ov_json_value *old =
      (ov_json_value *)ov_json_get(domain, "/" OV_KEY_PROJECTS);

  if (old && !ov_json_object_for_each(old, self, unindex_projects))
    goto error;

  if (!check_sip_processing(old, projects, out))
    goto error;

  if (!ov_json_object_set(domain, OV_KEY_PROJECTS, cpy))
    goto error;

  result = ov_json_object_for_each(cpy, self, reindex_projects);
  cpy = NULL;

error:
  ov_json_value_free(cpy);
  return result;
}

/*----------------------------------------------------------------------------*/

static bool update_loops(ov_vocs_db *self, ov_json_value *parent,
                         const ov_json_value *loops, ov_json_value **out) {

  bool result = false;
  ov_json_value *cpy = NULL;

  OV_ASSERT(self);
  OV_ASSERT(parent);
  OV_ASSERT(loops);

  if (!ov_json_is_object(loops))
    goto error;

  ov_json_value *data = (ov_json_value *)ov_json_get(parent, "/" OV_KEY_LOOPS);
  if (!data) {
    data = ov_json_object();
    ov_json_object_set(parent, OV_KEY_LOOPS, data);
  }

  struct container_parent c =
      (struct container_parent){.db = self, .parent = data};

  if (!ov_json_object_for_each((ov_json_value *)loops, &c, loop_already_set))
    goto error;

  if (!check_sip_processing(data, loops, out))
    goto error;

  if (!ov_json_value_copy((void **)&cpy, loops))
    goto error;

  if (data && !ov_json_object_for_each(data, self, unindex_loops))
    goto error;
  if (!ov_json_object_set(parent, OV_KEY_LOOPS, cpy))
    goto error;

  result = ov_json_object_for_each(cpy, self, reindex_loops);
  cpy = NULL;

error:
  ov_json_value_free(cpy);
  return result;
}

/*----------------------------------------------------------------------------*/

static bool update_domain(ov_vocs_db *self, ov_json_value *src, const char *id,
                          const char *key, const ov_json_value *val,
                          ov_json_value **out) {

  OV_ASSERT(self);
  OV_ASSERT(src);
  OV_ASSERT(id);
  OV_ASSERT(key);
  OV_ASSERT(val);

  bool result = false;

  if (0 == strcmp(key, OV_KEY_PROJECTS)) {

    result = update_domain_projects(self, src, val, out);

  } else if (0 == strcmp(key, OV_KEY_USERS)) {

    result = update_users(self, src, val);

  } else if (0 == strcmp(key, OV_KEY_ROLES)) {

    result = update_roles(self, src, val);

  } else if (0 == strcmp(key, OV_KEY_LOOPS)) {

    result = update_loops(self, src, val, out);

  } else if (0 == strcmp(key, OV_KEY_ID)) {

    // ignore
    result = true;
    goto done;

  } else {

    ov_json_value *data = NULL;
    if (!ov_json_value_copy((void **)&data, val))
      goto done;

    if (!ov_json_object_set(src, key, data)) {
      data = ov_json_value_free(data);
      result = false;
    } else {
      result = true;
    }
  }

done:
  return result;
}

/*----------------------------------------------------------------------------*/

static bool update_project(ov_vocs_db *self, ov_json_value *src, const char *id,
                           const char *key, const ov_json_value *val,
                           ov_json_value **out) {

  OV_ASSERT(self);
  OV_ASSERT(src);
  OV_ASSERT(id);
  OV_ASSERT(key);
  OV_ASSERT(val);

  bool result = false;

  if (0 == strcmp(key, OV_KEY_USERS)) {

    result = update_users(self, src, val);

  } else if (0 == strcmp(key, OV_KEY_ROLES)) {

    result = update_roles(self, src, val);

  } else if (0 == strcmp(key, OV_KEY_LOOPS)) {

    result = update_loops(self, src, val, out);

  } else if (0 == strcmp(key, OV_KEY_ID)) {

    // ignore
    result = true;
    goto done;

  } else {

    ov_json_value *data = NULL;
    if (!ov_json_value_copy((void **)&data, val))
      goto done;

    if (!ov_json_object_set(src, key, data)) {
      data = ov_json_value_free(data);
      result = false;
    } else {
      result = true;
    }
  }

done:
  return result;
}

/*----------------------------------------------------------------------------*/

static bool update_entity(ov_vocs_db *self, ov_json_value *data, const char *id,
                          const char *key, const ov_json_value *val,
                          ov_json_value **out) {

  OV_ASSERT(self);
  OV_ASSERT(data);
  OV_ASSERT(id);
  OV_ASSERT(key);
  OV_ASSERT(val);

  bool result = false;

  ov_json_value *old = ov_json_object_get(data, key);

  ov_json_value *new = NULL;
  if (!ov_json_value_copy((void **)&new, val))
    goto done;

  if (0 == ov_string_compare(key, OV_KEY_SIP)) {

    update_sip_parameter_processing(id, old, new, out);
  }

  result = ov_json_object_set(data, key, new);
  if (!result)
    new = ov_json_value_free(new);

done:
  return result;
}

/*----------------------------------------------------------------------------*/

bool ov_vocs_db_update_entity_key(ov_vocs_db *self, ov_vocs_db_entity entity,
                                  const char *id, const char *key,
                                  const ov_json_value *val) {

  bool result = false;

  if (!self || !id || !key || !val)
    goto error;

  if (0 == strcmp(key, OV_KEY_ID))
    goto error;

  if (!ov_thread_lock_try_lock(&self->lock))
    goto error;

  ov_dict *dict = get_index_dict(self, entity);

  ov_json_value *data = ov_dict_get(dict, id);
  if (!data)
    goto done;

  ov_json_value *out = NULL;

  switch (entity) {

  case OV_VOCS_DB_DOMAIN:

    result = update_domain(self, data, id, key, val, &out);
    break;

  case OV_VOCS_DB_PROJECT:

    result = update_project(self, data, id, key, val, &out);
    break;

  case OV_VOCS_DB_USER:

    result = update_entity(self, data, id, key, val, &out);
    break;

  case OV_VOCS_DB_LOOP:

    result = update_entity(self, data, id, key, val, &out);
    break;

  case OV_VOCS_DB_ROLE:

    result = update_entity(self, data, id, key, val, &out);
    break;

  default:
    break;
  }

done:

  if (!ov_thread_lock_unlock(&self->lock)) {
    OV_ASSERT(1 == 0);
    goto error;
  }

  ov_json_value *msg =
      ov_event_api_message_create(OV_VOCS_DB_UPDATE_DB, NULL, 0);

  if (!ov_event_trigger_send(self->config.trigger, "VOCS", msg))
    msg = ov_json_value_free(msg);

  // ov_vocs_db_persistance_persist(self->persistance);

  return result;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

bool ov_vocs_db_send_vocs_trigger(ov_vocs_db *self, const ov_json_value *msg) {

  if (!self || !msg)
    return false;

  ov_json_value *copy = NULL;
  ov_json_value_copy((void **)&copy, msg);

  return ov_event_trigger_send(self->config.trigger, "VOCS", copy);
}
/*----------------------------------------------------------------------------*/

static bool delete_domain_key(ov_vocs_db *self, ov_json_value *data,
                              const char *id, const char *key) {

  bool result = false;

  OV_ASSERT(self);
  OV_ASSERT(data);
  OV_ASSERT(id);
  OV_ASSERT(key);

  ov_json_value *content = ov_json_object_get(data, key);

  if (0 == strcmp(key, OV_KEY_PROJECTS)) {

    result = ov_json_object_for_each(content, self, unindex_projects);

  } else if (0 == strcmp(key, OV_KEY_USERS)) {

    result = ov_json_object_for_each(content, self, unindex_users);

  } else if (0 == strcmp(key, OV_KEY_ROLES)) {

    result = ov_json_object_for_each(content, self, unindex_roles);

  } else if (0 == strcmp(key, OV_KEY_LOOPS)) {

    result = ov_json_object_for_each(content, self, unindex_loops);

  } else {

    result = true;
  }

  result &= ov_json_object_del(data, key);
  return result;
}

/*----------------------------------------------------------------------------*/

static bool delete_project_key(ov_vocs_db *self, ov_json_value *data,
                               const char *id, const char *key) {

  bool result = false;

  OV_ASSERT(self);
  OV_ASSERT(data);
  OV_ASSERT(id);
  OV_ASSERT(key);

  ov_json_value *content = ov_json_object_get(data, key);

  if (0 == strcmp(key, OV_KEY_USERS)) {

    result = ov_json_object_for_each(content, self, unindex_users);

  } else if (0 == strcmp(key, OV_KEY_ROLES)) {

    result = ov_json_object_for_each(content, self, unindex_roles);

  } else if (0 == strcmp(key, OV_KEY_LOOPS)) {

    result = ov_json_object_for_each(content, self, unindex_loops);

  } else {

    result = true;
  }

  result &= ov_json_object_del(data, key);
  return result;
}

/*----------------------------------------------------------------------------*/

bool ov_vocs_db_delete_entity_key(ov_vocs_db *self, ov_vocs_db_entity entity,
                                  const char *id, const char *key) {

  bool result = false;

  if (!self || !id || !key)
    goto error;

  if (0 == strcmp(key, OV_KEY_ID))
    goto error;

  if (!ov_thread_lock_try_lock(&self->lock))
    goto error;

  ov_dict *dict = get_index_dict(self, entity);

  ov_json_value *data = ov_dict_get(dict, id);
  if (!data)
    goto done;

  switch (entity) {

  case OV_VOCS_DB_DOMAIN:

    result = delete_domain_key(self, data, id, key);
    break;

  case OV_VOCS_DB_PROJECT:

    result = delete_project_key(self, data, id, key);
    break;

  case OV_VOCS_DB_USER:
  case OV_VOCS_DB_LOOP:
  case OV_VOCS_DB_ROLE:

    result = ov_json_object_del(data, key);
    break;

  default:
    break;
  }

done:

  if (!ov_thread_lock_unlock(&self->lock)) {
    OV_ASSERT(1 == 0);
    goto error;
  }

  // ov_vocs_db_persistance_persist(self->persistance);

  return result;

error:
  return result;
}

/*----------------------------------------------------------------------------*/

static bool verify_user(ov_vocs_db *self, const char *id,
                        const ov_json_value *value, ov_json_value **errors) {

  bool result = false;
  ov_json_value *out = NULL;
  ov_json_value *val = NULL;

  OV_ASSERT(self);
  OV_ASSERT(id);
  OV_ASSERT(value);

  if (ov_json_get(value, "/" OV_KEY_ID)) {

    const char *id_set =
        ov_json_string_get(ov_json_object_get(value, OV_KEY_ID));

    if (0 != strcmp(id_set, id)) {

      result = false;
      out = ov_json_object();
      val = ov_json_string("id does not match");

      if (!ov_json_object_set(out, OV_KEY_ID, val)) {
        out = ov_json_value_free(out);
        val = ov_json_value_free(val);
      }
      goto done;
    }
  }

  if (ov_json_get(value, "/" OV_KEY_PASSWORD)) {

    out = ov_json_object();
    val = ov_json_string("user password contained.");

    if (!ov_json_object_set(out, OV_KEY_PASSWORD, val)) {
      out = ov_json_value_free(out);
      val = ov_json_value_free(val);
    }

    result = false;

  } else {

    result = true;
  }

done:
  if (errors) {
    *errors = out;
  } else {
    ov_json_value_free(out);
  }
  return result;
}

/*----------------------------------------------------------------------------*/

static bool verify_role(ov_vocs_db *self, const char *id,
                        const ov_json_value *value, ov_json_value **errors) {

  bool result = false;
  ov_json_value *err = NULL;
  ov_json_value *val = NULL;

  OV_ASSERT(self);
  OV_ASSERT(id);
  OV_ASSERT(value);

  /* Nothing special to check for role */

  if (ov_json_get(value, "/" OV_KEY_ID)) {

    const char *id_set =
        ov_json_string_get(ov_json_object_get(value, OV_KEY_ID));

    if (0 != strcmp(id_set, id)) {

      result = false;
      err = ov_json_object();
      val = ov_json_string("id does not match");

      if (!ov_json_object_set(err, OV_KEY_ID, val)) {
        err = ov_json_value_free(err);
        val = ov_json_value_free(val);
      }
      goto done;
    }
  }

  result = true;
done:

  if (errors) {
    *errors = err;
  } else {
    ov_json_value_free(err);
  }
  return result;
}

/*----------------------------------------------------------------------------*/

static bool verify_loop(ov_vocs_db *self, const char *id,
                        const ov_json_value *value, ov_json_value **errors) {

  bool result = false;
  ov_json_value *err = NULL;
  ov_json_value *val = NULL;

  OV_ASSERT(self);
  OV_ASSERT(id);
  OV_ASSERT(value);

  /* Nothing special to check for role */

  if (ov_json_get(value, "/" OV_KEY_ID)) {

    const char *id_set =
        ov_json_string_get(ov_json_object_get(value, OV_KEY_ID));

    if (0 != strcmp(id_set, id)) {

      result = false;
      err = ov_json_object();
      val = ov_json_string("id does not match");

      if (!ov_json_object_set(err, OV_KEY_ID, val)) {
        err = ov_json_value_free(err);
        val = ov_json_value_free(val);
      }
      goto done;
    }
  }

  result = true;
done:

  if (errors) {
    *errors = err;
  } else {
    ov_json_value_free(err);
  }
  return result;
}

/*----------------------------------------------------------------------------*/

struct container_verify {

  ov_vocs_db *db;
  ov_json_value const *data;
  ov_json_value *err;
};

/*----------------------------------------------------------------------------*/

static bool verify_id(struct container_verify *container, const char *key,
                      ov_dict *index, const char *error_msg) {

  OV_ASSERT(container);
  OV_ASSERT(key);
  OV_ASSERT(index);
  OV_ASSERT(error_msg);

  /* 1 check if id is some self owned id */
  if (ov_json_object_get(container->data, key))
    goto done;

  /* 2 check if id is in use somewhere else */
  if (ov_dict_is_set(index, key)) {

    if (!container->err)
      container->err = ov_json_object();

    ov_json_value *err = ov_json_string(error_msg);
    if (!ov_json_object_set(container->err, key, err))
      err = ov_json_value_free(err);

    goto error;
  }

done:
  return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

static bool verify_user_id(const void *key, void *val, void *data) {

  if (!key)
    return true;
  UNUSED(val);

  struct container_verify *container = (struct container_verify *)data;

  return verify_id(container, (const char *)key, container->db->index.users,
                   "user id already in use.");
}

/*----------------------------------------------------------------------------*/

static bool verify_role_id(const void *key, void *val, void *data) {

  if (!key)
    return true;
  UNUSED(val);

  struct container_verify *container = (struct container_verify *)data;

  return verify_id(container, (const char *)key, container->db->index.roles,
                   "role id already in use.");
}

/*----------------------------------------------------------------------------*/

static bool verify_loop_id(const void *key, void *val, void *data) {

  if (!key)
    return true;
  UNUSED(val);

  struct container_verify *container = (struct container_verify *)data;

  return verify_id(container, (const char *)key, container->db->index.loops,
                   "loop id already in use.");
}

/*----------------------------------------------------------------------------*/

static bool verify_project(ov_vocs_db *self, const char *id,
                           const ov_json_value *value, ov_json_value **errors) {

  bool result = false;
  OV_ASSERT(self);
  OV_ASSERT(id);
  OV_ASSERT(value);

  ov_json_value *project = ov_dict_get(self->index.projects, id);

  /*
      1. users MUST be already contained in project or not set.
      2. roles MUST be already contained in project or not set.
      3. loops MUST be already contained in project or not set.
  */

  ov_json_value const *data = NULL;

  struct container_verify container =
      (struct container_verify){.db = self, .err = NULL, .data = NULL};

  data = ov_json_get(value, "/" OV_KEY_USERS);
  container.data = ov_json_get(project, "/" OV_KEY_USERS);
  if (data && !ov_json_object_for_each((ov_json_value *)data, &container,
                                       verify_user_id))
    goto error;

  data = ov_json_get(value, "/" OV_KEY_ROLES);
  container.data = ov_json_get(project, "/" OV_KEY_ROLES);
  if (data && !ov_json_object_for_each((ov_json_value *)data, &container,
                                       verify_role_id))
    goto error;

  data = ov_json_get(value, "/" OV_KEY_LOOPS);
  container.data = ov_json_get(project, "/" OV_KEY_LOOPS);
  if (data && !ov_json_object_for_each((ov_json_value *)data, &container,
                                       verify_loop_id))
    goto error;

  result = true;

error:
  if (errors) {
    *errors = container.err;
  } else {
    container.err = ov_json_value_free(container.err);
  }
  return result;
}

/*----------------------------------------------------------------------------*/

static bool verify_project_id(const void *key, void *val, void *data) {

  if (!key)
    return true;
  UNUSED(val);

  struct container_verify *container = (struct container_verify *)data;

  if (!verify_project(container->db, (const char *)key, val, &container->err))
    return false;

  return verify_id(container, (const char *)key, container->db->index.loops,
                   "project id already in use.");
}

/*----------------------------------------------------------------------------*/

static bool verify_domain(ov_vocs_db *self, const char *id,
                          const ov_json_value *value, ov_json_value **errors) {

  bool result = false;

  OV_ASSERT(self);
  OV_ASSERT(id);
  OV_ASSERT(value);

  ov_json_value *domain = ov_dict_get(self->index.domains, id);

  /*
      1. users MUST be already contained in project or not set.
      2. roles MUST be already contained in project or not set.
      3. loops MUST be already contained in project or not set.
  */

  ov_json_value const *data = NULL;

  struct container_verify container =
      (struct container_verify){.db = self, .err = NULL, .data = NULL};

  data = ov_json_get(value, "/" OV_KEY_USERS);
  container.data = ov_json_get(domain, "/" OV_KEY_USERS);
  if (data && !ov_json_object_for_each((ov_json_value *)data, &container,
                                       verify_user_id))
    goto error;

  data = ov_json_get(value, "/" OV_KEY_ROLES);
  container.data = ov_json_get(domain, "/" OV_KEY_ROLES);
  if (data && !ov_json_object_for_each((ov_json_value *)data, &container,
                                       verify_role_id))
    goto error;

  data = ov_json_get(value, "/" OV_KEY_LOOPS);
  container.data = ov_json_get(domain, "/" OV_KEY_LOOPS);
  if (data && !ov_json_object_for_each((ov_json_value *)data, &container,
                                       verify_loop_id))
    goto error;

  data = ov_json_get(value, "/" OV_KEY_PROJECTS);
  container.data = ov_json_get(domain, "/" OV_KEY_PROJECTS);

  if (data && !ov_json_object_for_each((ov_json_value *)data, &container,
                                       verify_project_id))
    goto error;

  result = true;

error:
  if (errors) {
    *errors = container.err;
  } else {
    container.err = ov_json_value_free(container.err);
  }
  return result;
}

/*----------------------------------------------------------------------------*/

bool ov_vocs_db_verify_entity_item(ov_vocs_db *self, ov_vocs_db_entity entity,
                                   const char *id, const ov_json_value *val,
                                   ov_json_value **errors) {

  bool result = false;

  if (!self || !id || !val)
    goto error;

  if (!ov_thread_lock_try_lock(&self->lock))
    goto error;

  switch (entity) {

  case OV_VOCS_DB_DOMAIN:

    result = verify_domain(self, id, val, errors);
    break;

  case OV_VOCS_DB_PROJECT:

    result = verify_project(self, id, val, errors);
    break;

  case OV_VOCS_DB_USER:

    result = verify_user(self, id, val, errors);
    break;

  case OV_VOCS_DB_LOOP:

    result = verify_loop(self, id, val, errors);
    break;

  case OV_VOCS_DB_ROLE:

    result = verify_role(self, id, val, errors);
    break;

  default:
    break;
  }

  if (!ov_thread_lock_unlock(&self->lock)) {
    OV_ASSERT(1 == 0);
    goto error;
  }

  return result;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

struct container_db {

  ov_vocs_db *db;
  const char *id;
  ov_json_value *data;
  bool (*func)(ov_vocs_db *db, ov_json_value *data, const char *id,
               const char *key, const ov_json_value *val, ov_json_value **out);
  ov_json_value *out;
};

/*----------------------------------------------------------------------------*/

static bool update_key_val(const void *key, void *val, void *data) {

  if (!key)
    return true;

  struct container_db *c = (struct container_db *)data;
  if (!c || !val)
    goto error;

  OV_ASSERT(c->db);
  OV_ASSERT(c->id);
  OV_ASSERT(c->data);
  OV_ASSERT(c->func);

  return c->func(c->db, c->data, c->id, (const char *)key,
                 ov_json_value_cast(val), &c->out);

error:
  return false;
}

/*----------------------------------------------------------------------------*/

bool ov_vocs_db_update_entity_item(ov_vocs_db *self, ov_vocs_db_entity entity,
                                   const char *id, const ov_json_value *val,
                                   ov_json_value **errors) {

  bool result = false;

  ov_json_value *out = NULL;

  if (!ov_vocs_db_verify_entity_item(self, entity, id, val, errors))
    goto error;

  if (!ov_thread_lock_try_lock(&self->lock))
    goto error;

  ov_dict *dict = get_index_dict(self, entity);

  ov_json_value *data = ov_dict_get(dict, id);
  if (!data) {

    if (errors) {

      *errors = ov_json_object();

      ov_json_value *val = ov_json_string("ID not found.");
      if (!ov_json_object_set(*errors, id, val)) {
        val = ov_json_value_free(val);
      }
    }

    goto done;
  }

  struct container_db c =
      (struct container_db){.db = self, .id = id, .data = data, .out = NULL};

  switch (entity) {

  case OV_VOCS_DB_DOMAIN:

    c.func = update_domain;
    result = ov_json_object_for_each((ov_json_value *)val, &c, update_key_val);
    break;

  case OV_VOCS_DB_PROJECT:

    c.func = update_project;
    result = ov_json_object_for_each((ov_json_value *)val, &c, update_key_val);
    break;

  case OV_VOCS_DB_USER:
  case OV_VOCS_DB_LOOP:
  case OV_VOCS_DB_ROLE:

    c.func = update_entity;
    result = ov_json_object_for_each((ov_json_value *)val, &c, update_key_val);
    break;

  default:
    break;
  }

  out = c.out;

done:

  if (!ov_thread_lock_unlock(&self->lock)) {
    OV_ASSERT(1 == 0);
    goto error;
  }

  ov_json_value *msg =
      ov_event_api_message_create(OV_VOCS_DB_UPDATE_DB, NULL, 0);

  if (out) {

    ov_json_value *par = ov_event_api_set_parameter(msg);
    ov_json_object_set(par, OV_KEY_PROCESSING, out);
  }

  ov_event_trigger_send(self->config.trigger, "VOCS", msg);
  // ov_vocs_db_persistance_persist(self->persistance);

  return result;

error:
  return false;
}

/*----------------------------------------------------------------------------*/

static bool inject_auth(ov_vocs_db *self, ov_json_value *data) {

  OV_ASSERT(self);
  OV_ASSERT(data);

  bool result = false;

  if (!ov_thread_lock_try_lock(&self->lock))
    goto error;

  ov_log_info("DB inject new auth dataset");
  self->data.domains = ov_json_value_free(self->data.domains);
  self->data.domains = data;

  index_clear(self);

  result = reindex_auth(self);

  if (!ov_thread_lock_unlock(&self->lock)) {
    OV_ASSERT(1 == 0);
    goto error;
  }

  return result;

error:
  return false;
}

/*----------------------------------------------------------------------------*/

static bool inject_state(ov_vocs_db *self, ov_json_value *data) {

  OV_ASSERT(self);
  OV_ASSERT(data);

  if (!ov_thread_lock_try_lock(&self->lock))
    goto error;

  ov_log_info("DB inject new state dataset");
  self->data.state = ov_json_value_free(self->data.state);
  self->data.state = data;

  if (!ov_thread_lock_unlock(&self->lock)) {
    OV_ASSERT(1 == 0);
    goto error;
  }

  return true;

error:
  return false;
}

/*----------------------------------------------------------------------------*/

bool ov_vocs_db_inject(ov_vocs_db *self, ov_vocs_db_type type,
                       ov_json_value *data) {

  if (!self || !data)
    goto error;

  switch (type) {

  case OV_VOCS_DB_TYPE_AUTH:
    return inject_auth(self, data);

  case OV_VOCS_DB_TYPE_STATE:
    return inject_state(self, data);
  }

error:
  return false;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_vocs_db_eject(ov_vocs_db *self, ov_vocs_db_type type) {

  ov_json_value *out = NULL;

  if (!self)
    return NULL;

  if (!ov_thread_lock_try_lock(&self->lock))
    goto error;

  switch (type) {

  case OV_VOCS_DB_TYPE_AUTH:
    ov_json_value_copy((void **)&out, self->data.domains);
    break;

  case OV_VOCS_DB_TYPE_STATE:
    ov_json_value_copy((void **)&out, self->data.state);
    break;
  }

  if (!ov_thread_lock_unlock(&self->lock)) {
    OV_ASSERT(1 == 0);
    goto error;
  }

  return out;

error:
  return false;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      AUTHENTICATION FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

bool ov_vocs_db_set_password(ov_vocs_db *self, const char *user,
                             const char *pass) {

  bool result = false;

  if (!self || !user || !pass)
    goto error;

  if (!ov_thread_lock_try_lock(&self->lock))
    goto error;

  ov_json_value *data = ov_dict_get(self->index.users, user);
  if (!data)
    goto done;

  ov_json_value *val = ov_password_hash(pass, self->config.password.params,
                                        self->config.password.length);

  result = ov_json_object_set(data, OV_KEY_PASSWORD, val);
  if (!result)
    val = ov_json_value_free(val);

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

bool ov_vocs_db_authenticate(ov_vocs_db *self, const char *user,
                             const char *pass) {

  bool result = false;

  if (!self || !user || !pass)
    goto error;

  if (!ov_thread_lock_try_lock(&self->lock)) {
    ov_log_error("Could not lock to authenticate.");
    goto error;
  }

  ov_json_value *data = ov_dict_get(self->index.users, user);
  if (!data)
    goto done;

  /* Verify password */

  result =
      ov_password_is_valid(pass, ov_json_object_get(data, OV_KEY_PASSWORD));

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

bool ov_vocs_db_authorize(ov_vocs_db *self, const char *user,
                          const char *role) {

  bool result = false;

  if (!self || !user || !role)
    goto error;

  if (!ov_thread_lock_try_lock(&self->lock))
    goto error;

  ov_json_value *user_data = ov_dict_get(self->index.users, user);
  if (!user_data)
    goto done;
  ov_json_value *role_data = ov_dict_get(self->index.roles, role);
  if (!role_data)
    goto done;

  ov_json_value const *users = ov_json_get(role_data, "/" OV_KEY_USERS);
  if (ov_json_object_get(users, user))
    result = true;

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

ov_vocs_permission ov_vocs_db_get_permission(ov_vocs_db *self, const char *role,
                                             const char *loop) {

  ov_vocs_permission result = OV_VOCS_NONE;

  if (!self || !role || !loop)
    goto error;

  if (!ov_thread_lock_try_lock(&self->lock))
    goto error;

  ov_json_value *loop_data = ov_dict_get(self->index.loops, loop);
  if (!loop_data)
    goto done;

  ov_json_value const *roles = ov_json_get(loop_data, "/" OV_KEY_ROLES);
  ov_json_value *data = ov_json_object_get(roles, role);
  if (!data)
    goto done;

  if (ov_json_is_true(data)) {
    result = OV_VOCS_SEND;
  } else if (ov_json_is_false(data)) {
    result = OV_VOCS_RECV;
  }

done:
  if (!ov_thread_lock_unlock(&self->lock)) {
    OV_ASSERT(1 == 0);
    goto error;
  }

  return result;

error:
  return OV_VOCS_NONE;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      ADMIN FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

static bool authorize_domain_admin(ov_vocs_db *self, const char *user,
                                   const char *id) {

  OV_ASSERT(self);
  OV_ASSERT(user);

  ov_json_value *domain = ov_dict_get(self->index.domains, id);
  return ov_vocs_json_is_admin_user(domain, user);
}

/*----------------------------------------------------------------------------*/

bool ov_vocs_db_authorize_domain_admin(ov_vocs_db *self, const char *user,
                                       const char *id) {

  bool result = false;

  if (!self || !user || !id)
    goto error;

  if (!ov_thread_lock_try_lock(&self->lock))
    goto error;

  result = authorize_domain_admin(self, user, id);

  if (!ov_thread_lock_unlock(&self->lock)) {
    OV_ASSERT(1 == 0);
    goto error;
  }

  return result;

error:
  return false;
}

/*----------------------------------------------------------------------------*/

static bool authorize_project_admin(ov_vocs_db *self, const char *user,
                                    const char *id) {

  bool result = false;

  ov_json_value *project = ov_dict_get(self->index.projects, id);
  if (!project)
    goto done;

  result = ov_vocs_json_is_admin_user(project, user);
  if (result)
    goto done;

  ov_json_value *projects = project->parent;
  if (!projects)
    goto done;

  ov_json_value *domain = projects->parent;
  if (!domain)
    goto done;

  result = authorize_domain_admin(self, user, ov_vocs_json_get_id(domain));

done:
  return result;
}

/*----------------------------------------------------------------------------*/

bool ov_vocs_db_authorize_project_admin(ov_vocs_db *self, const char *user,
                                        const char *id) {

  bool result = false;

  if (!self || !user || !id)
    goto error;

  if (!ov_thread_lock_try_lock(&self->lock))
    goto error;

  result = authorize_project_admin(self, user, id);

  if (!ov_thread_lock_unlock(&self->lock)) {
    OV_ASSERT(1 == 0);
    goto error;
  }

  return result;

error:
  return false;
}

/*----------------------------------------------------------------------------*/

bool ov_vocs_db_add_project_admin(ov_vocs_db *self, const char *project,
                                  const char *id) {

  bool result = false;

  if (!self || !project || !id)
    goto error;

  if (!ov_thread_lock_try_lock(&self->lock))
    goto error;

  ov_json_value *project_obj = ov_dict_get(self->index.projects, project);
  if (!project_obj)
    goto done;

  ov_json_value *roles = ov_json_object_get(project_obj, OV_KEY_ROLES);
  if (!roles) {
    roles = ov_json_object();
    ov_json_object_set(project_obj, OV_KEY_ROLES, roles);
  }
  ov_json_value *admin = ov_json_object_get(roles, OV_KEY_ADMIN);
  if (!admin) {
    admin = ov_json_object();
    ov_json_object_set(roles, OV_KEY_ADMIN, admin);
  }

  ov_json_value *users = ov_json_object_get(admin, OV_KEY_USERS);
  if (!users) {
    users = ov_json_object();
    ov_json_object_set(admin, OV_KEY_USERS, users);
  }

  result = ov_json_object_set(users, id, ov_json_null());

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

bool ov_vocs_db_add_domain_admin(ov_vocs_db *self, const char *domain,
                                 const char *id) {

  bool result = false;

  if (!self || !domain || !id)
    goto error;

  if (!ov_thread_lock_try_lock(&self->lock))
    goto error;

  ov_json_value *domain_obj = ov_dict_get(self->index.domains, domain);
  if (!domain_obj)
    goto done;

  ov_json_value *roles = ov_json_object_get(domain_obj, OV_KEY_ROLES);
  if (!roles) {
    roles = ov_json_object();
    ov_json_object_set(domain_obj, OV_KEY_ROLES, roles);
  }
  ov_json_value *admin = ov_json_object_get(roles, OV_KEY_ADMIN);
  if (!admin) {
    admin = ov_json_object();
    ov_json_object_set(roles, OV_KEY_ADMIN, admin);
  }

  ov_json_value *users = ov_json_object_get(admin, OV_KEY_USERS);
  if (!users) {
    users = ov_json_object();
    ov_json_object_set(admin, OV_KEY_USERS, users);
  }

  result = ov_json_object_set(users, id, ov_json_null());

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

struct container_id {

  ov_vocs_db *db;
  const char *id;
  ov_json_value *out;

  const char *user;
};

/*----------------------------------------------------------------------------*/

static bool get_admin_domains(const void *key, void *val, void *data) {

  if (!key)
    return true;

  const char *id = (const char *)key;
  UNUSED(val);

  struct container_id *c = (struct container_id *)data;

  OV_ASSERT(c->db);
  OV_ASSERT(c->id);
  OV_ASSERT(c->out);

  if (authorize_domain_admin(c->db, c->id, id)) {

    if (!ov_json_array_push(c->out, ov_json_string(id))) {
      return false;
    }
  }

  return true;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_vocs_db_get_admin_domains(ov_vocs_db *self,
                                            const char *user) {

  if (!self || !user)
    goto error;

  ov_json_value *out = ov_json_array();

  if (!ov_thread_lock_try_lock(&self->lock))
    goto error;

  struct container_id c =
      (struct container_id){.id = user, .db = self, .out = out};

  ov_dict_for_each(self->index.domains, &c, get_admin_domains);

  if (!ov_thread_lock_unlock(&self->lock)) {
    OV_ASSERT(1 == 0);
    goto error;
  }

  return out;

error:
  return NULL;
}

/*----------------------------------------------------------------------------*/

static bool get_admin_projects(const void *key, void *value, void *data) {

  ov_json_value *out = NULL;
  ov_json_value *val = NULL;

  if (!key)
    return true;

  const char *id = (const char *)key;

  struct container_id *c = (struct container_id *)data;

  OV_ASSERT(c->db);
  OV_ASSERT(c->id);
  OV_ASSERT(c->out);

  if (authorize_project_admin(c->db, c->id, id)) {

    ov_json_value *project = ov_json_value_cast(value);

    const char *domain_id = ov_vocs_json_get_domain_id(project);
    const char *project_id = ov_vocs_json_get_id(project);
    const char *project_name = ov_vocs_json_get_name(project);

    out = ov_json_object();

    if (project_id) {
      val = ov_json_string(project_id);
    } else {
      val = ov_json_null();
    }
    if (!ov_json_object_set(out, OV_KEY_ID, val))
      goto error;

    if (domain_id) {
      val = ov_json_string(domain_id);
    } else {
      val = ov_json_null();
    }
    if (!ov_json_object_set(out, OV_KEY_DOMAIN, val))
      goto error;

    if (project_name) {
      val = ov_json_string(project_name);
    } else {
      val = ov_json_null();
    }
    if (!ov_json_object_set(out, OV_KEY_NAME, val))
      goto error;
    val = NULL;

    if (!ov_json_object_set(c->out, id, out))
      goto error;
  }

  return true;
error:
  ov_json_value_free(val);
  ov_json_value_free(out);
  return false;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_vocs_db_get_admin_projects(ov_vocs_db *self,
                                             const char *user) {

  if (!self || !user)
    goto error;

  ov_json_value *out = ov_json_object();

  if (!ov_thread_lock_try_lock(&self->lock))
    goto error;

  struct container_id c =
      (struct container_id){.id = user, .db = self, .out = out};

  ov_dict_for_each(self->index.projects, &c, get_admin_projects);

  if (!ov_thread_lock_unlock(&self->lock)) {
    OV_ASSERT(1 == 0);
    goto error;
  }

  return out;

error:
  return NULL;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      DATA FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

static bool add_role_for_export(const void *key, void *value, void *data) {

  if (!key)
    return true;

  ov_json_value *role_data = ov_json_value_cast(value);
  struct container_id *c = (struct container_id *)data;

  ov_json_value const *users = ov_json_get(role_data, "/" OV_KEY_USERS);
  if (!users)
    goto done;

  if (!ov_json_object_get(users, c->id))
    goto done;

  ov_json_value *copy = NULL;
  if (!ov_json_value_copy((void **)&copy, role_data))
    goto error;

  ov_json_object_del(copy, OV_KEY_USERS);

  if (!ov_json_object_set(c->out, key, copy)) {
    copy = ov_json_value_free(copy);
    goto error;
  }

  ov_json_value *roles = role_data->parent;
  OV_ASSERT(roles);
  if (!roles)
    goto done;

  ov_json_value *project = roles->parent;
  OV_ASSERT(project);
  if (!project)
    goto done;

  ov_json_value *v = ov_json_string(ov_vocs_json_get_id(project));
  if (!ov_json_object_set(copy, OV_KEY_PROJECT, v)) {
    v = ov_json_value_free(v);
    goto error;
  }

done:
  return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

static ov_json_value *get_user_roles(ov_vocs_db *self, const char *user) {

  OV_ASSERT(self);
  OV_ASSERT(user);

  ov_json_value *out = ov_json_object();

  struct container_id c =
      (struct container_id){.id = user, .db = self, .out = out};

  ov_dict_for_each(self->index.roles, &c, add_role_for_export);

  return out;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_vocs_db_get_user_roles(ov_vocs_db *self, const char *user) {

  if (!self || !user)
    goto error;
  if (!ov_thread_lock_try_lock(&self->lock))
    goto error;

  ov_json_value *out = get_user_roles(self, user);

  if (!ov_thread_lock_unlock(&self->lock)) {
    OV_ASSERT(1 == 0);
    goto error;
  }

  return out;

error:
  return NULL;
}

/*----------------------------------------------------------------------------*/

static bool add_loop_for_export(const void *key, void *value, void *data) {

  if (!key)
    return true;

  ov_json_value *copy = NULL;
  ov_json_value *itm = NULL;

  ov_json_value *loop_data = ov_json_value_cast(value);
  struct container_id *c = (struct container_id *)data;

  ov_json_value *roles = ov_json_object_get(loop_data, OV_KEY_ROLES);
  ov_json_value *role = ov_json_object_get(roles, c->id);
  if (!roles || !role)
    goto done;

  if (!ov_json_value_copy((void **)&copy, loop_data))
    goto error;

  ov_json_object_del(copy, OV_KEY_USERS);
  ov_json_object_del(copy, OV_KEY_ROLES);

  if (ov_json_is_true(role)) {
    itm = ov_json_string(ov_vocs_permission_to_string(OV_VOCS_SEND));
  } else {
    itm = ov_json_string(ov_vocs_permission_to_string(OV_VOCS_RECV));
  }

  if (!ov_json_object_set(copy, OV_KEY_PERMISSION, itm)) {
    itm = ov_json_value_free(itm);
    goto error;
  }

  if (!ov_json_object_get(copy, OV_KEY_TYPE)) {

    itm = ov_json_string(OV_KEY_AUDIO);
    if (!ov_json_object_set(copy, OV_KEY_TYPE, itm)) {
      itm = ov_json_value_free(itm);
      goto error;
    }
  }

  if (!ov_json_object_get(copy, OV_KEY_NAME)) {

    itm = ov_json_string(key);
    if (!ov_json_object_set(copy, OV_KEY_NAME, itm)) {
      itm = ov_json_value_free(itm);
      goto error;
    }
  }

  if (!ov_json_object_set(c->out, key, copy))
    goto error;

done:
  return true;
error:
  copy = ov_json_value_free(copy);
  return false;
}

/*----------------------------------------------------------------------------*/

static ov_json_value *get_role_loops(ov_vocs_db *self, const char *role) {

  OV_ASSERT(self);
  OV_ASSERT(role);

  ov_json_value *out = ov_json_object();

  struct container_id c =
      (struct container_id){.id = role, .db = self, .out = out};

  ov_dict_for_each(self->index.loops, &c, add_loop_for_export);

  return out;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_vocs_db_get_role_loops(ov_vocs_db *self, const char *role) {

  if (!self || !role)
    goto error;
  if (!ov_thread_lock_try_lock(&self->lock))
    goto error;

  ov_json_value *out = get_role_loops(self, role);

  if (!ov_thread_lock_unlock(&self->lock)) {
    OV_ASSERT(1 == 0);
    goto error;
  }

  return out;

error:
  return NULL;
}

/*----------------------------------------------------------------------------*/

static bool add_loop_for_loop_export(const void *key, void *value, void *data) {

  if (!key)
    return true;

  ov_json_value *copy = NULL;

  ov_json_value *loop_data = ov_json_value_cast(value);
  ov_json_value *base = ov_json_value_cast(data);

  if (!ov_json_value_copy((void **)&copy, loop_data))
    goto error;

  ov_json_object_del(copy, OV_KEY_USERS);
  ov_json_object_del(copy, OV_KEY_ROLES);

  if (!ov_json_object_set(base, key, copy))
    goto error;

  return true;
error:
  copy = ov_json_value_free(copy);
  return false;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_vocs_db_get_loops(ov_vocs_db *self) {

  if (!self)
    goto error;
  if (!ov_thread_lock_try_lock(&self->lock))
    goto error;

  ov_json_value *out = ov_json_object();

  ov_dict_for_each(self->index.loops, out, add_loop_for_loop_export);

  if (!ov_thread_lock_unlock(&self->lock)) {
    OV_ASSERT(1 == 0);
    goto error;
  }

  return out;

error:
  return NULL;
}

/*----------------------------------------------------------------------------*/

static ov_json_value *get_loop_settings(ov_vocs_db *self, const char *user,
                                        const char *role, const char *loop) {

  if (!self || !user || !role || !loop)
    goto error;

  ov_json_value *u = ov_json_object_get(self->data.state, user);
  ov_json_value *r = ov_json_object_get(u, role);
  ov_json_value *l = ov_json_object_get(r, loop);

  return l;
error:
  return NULL;
}

/*----------------------------------------------------------------------------*/

static bool add_user_loop_for_export(const void *key, void *value, void *data) {

  if (!key)
    return true;

  ov_json_value *copy = NULL;
  ov_json_value *itm = NULL;

  ov_json_value *loop_data = ov_json_value_cast(value);
  struct container_id *c = (struct container_id *)data;

  ov_json_value *role_data = ov_dict_get(c->db->index.roles, c->id);
  ov_json_value *layout = ov_json_object_get(role_data, OV_KEY_LAYOUT);

  ov_json_value *roles = ov_json_object_get(loop_data, OV_KEY_ROLES);
  ov_json_value *role = ov_json_object_get(roles, c->id);
  if (!roles || !role)
    goto done;

  if (!ov_json_value_copy((void **)&copy, loop_data))
    goto error;

  ov_json_object_del(copy, OV_KEY_USERS);
  ov_json_object_del(copy, OV_KEY_ROLES);

  if (ov_json_is_true(role)) {
    itm = ov_json_string(ov_vocs_permission_to_string(OV_VOCS_SEND));
  } else {
    itm = ov_json_string(ov_vocs_permission_to_string(OV_VOCS_RECV));
  }

  if (!ov_json_object_set(copy, OV_KEY_PERMISSION, itm)) {
    itm = ov_json_value_free(itm);
    goto error;
  }

  if (!ov_json_object_get(copy, OV_KEY_TYPE)) {

    itm = ov_json_string(OV_KEY_AUDIO);
    if (!ov_json_object_set(copy, OV_KEY_TYPE, itm)) {
      itm = ov_json_value_free(itm);
      goto error;
    }
  }

  if (!ov_json_object_get(copy, OV_KEY_NAME)) {

    itm = ov_json_string(key);
    if (!ov_json_object_set(copy, OV_KEY_NAME, itm)) {
      itm = ov_json_value_free(itm);
      goto error;
    }
  }

  ov_json_value *loop_settings = get_loop_settings(c->db, c->user, c->id, key);

  ov_json_value *state = ov_json_object_get(loop_settings, OV_KEY_STATE);
  ov_json_value *volume = ov_json_object_get(loop_settings, OV_KEY_VOLUME);

  ov_json_value *v = NULL;

  if (state) {

    if (!ov_json_value_copy((void **)&v, state))
      goto error;

  } else {

    v = ov_json_string(ov_vocs_permission_to_string(OV_VOCS_NONE));
  }

  if (!ov_json_object_set(copy, OV_KEY_STATE, v)) {
    v = ov_json_value_free(v);
    goto error;
  }

  v = NULL;

  if (volume) {

    if (!ov_json_value_copy((void **)&v, volume))
      goto error;

  } else {

    v = ov_json_number(50);
  }

  if (!ov_json_object_set(copy, OV_KEY_VOLUME, v)) {
    v = ov_json_value_free(v);
    goto error;
  }

  v = NULL;

  if (layout) {

    ov_json_value *position = ov_json_object_get(layout, key);
    if (position) {

      v = NULL;
      ov_json_value_copy((void **)&v, position);

      if (!ov_json_object_set(copy, OV_KEY_LAYOUT_POSITION, v)) {
        v = ov_json_value_free(v);
        goto error;
      }
    }
  }

  if (!ov_json_object_set(c->out, key, copy))
    goto error;

done:
  return true;
error:
  copy = ov_json_value_free(copy);
  return false;
}

/*----------------------------------------------------------------------------*/

static ov_json_value *get_user_role_loops(ov_vocs_db *self, const char *user,
                                          const char *role) {

  OV_ASSERT(self);
  OV_ASSERT(role);
  OV_ASSERT(user);

  ov_json_value *out = ov_json_object();

  struct container_id c =
      (struct container_id){.id = role, .db = self, .out = out, .user = user};

  ov_dict_for_each(self->index.loops, &c, add_user_loop_for_export);

  return out;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_vocs_db_get_user_role_loops(ov_vocs_db *self,
                                              const char *user,
                                              const char *role) {

  if (!self || !role || !user)
    goto error;
  if (!ov_thread_lock_try_lock(&self->lock))
    goto error;

  ov_json_value *out = get_user_role_loops(self, user, role);

  if (!ov_thread_lock_unlock(&self->lock)) {
    OV_ASSERT(1 == 0);
    goto error;
  }

  return out;

error:
  return NULL;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      STATE FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

static ov_json_value *get_set_loop(ov_vocs_db *self, const char *user,
                                   const char *role, const char *loop) {

  if (!self || !user || !role || !loop)
    goto error;

  if (!self->data.state)
    self->data.state = ov_json_object();

  if (!self->data.state)
    goto error;

  ov_json_value *u = ov_json_object_get(self->data.state, user);

  if (!u) {

    u = ov_json_object();

    if (!ov_json_object_set(self->data.state, user, u)) {
      u = ov_json_value_free(u);
      goto error;
    }
  }

  ov_json_value *r = ov_json_object_get(u, role);

  if (!r) {

    r = ov_json_object();

    if (!ov_json_object_set(u, role, r)) {
      r = ov_json_value_free(r);
      goto error;
    }
  }

  ov_json_value *l = ov_json_object_get(r, loop);

  if (!l) {

    l = ov_json_object();

    if (!ov_json_object_set(r, loop, l)) {
      l = ov_json_value_free(l);
      goto error;
    }
  }

  return l;

error:
  return NULL;
}

/*----------------------------------------------------------------------------*/

bool ov_vocs_db_set_state(ov_vocs_db *self, const char *user, const char *role,
                          const char *loop, ov_vocs_permission state) {

  bool result = false;

  if (!self || !user || !role || !loop)
    goto error;
  if (!ov_thread_lock_try_lock(&self->lock))
    goto error;

  ov_json_value *obj = get_set_loop(self, user, role, loop);
  if (!obj)
    goto done;

  ov_json_value *val = ov_json_string(ov_vocs_permission_to_string(state));
  result = ov_json_object_set(obj, OV_KEY_STATE, val);

  if (!result)
    val = ov_json_value_free(val);

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

ov_vocs_permission ov_vocs_db_get_state(ov_vocs_db *self, const char *user,
                                        const char *role, const char *loop) {

  ov_vocs_permission result = OV_VOCS_NONE;

  if (!self || !user || !role || !loop)
    goto error;
  if (!ov_thread_lock_try_lock(&self->lock))
    goto error;

  ov_json_value *obj = get_loop_settings(self, user, role, loop);
  if (!obj)
    goto done;

  ov_json_value *state = ov_json_object_get(obj, OV_KEY_STATE);
  result = ov_vocs_permission_from_json(state);

done:
  if (!ov_thread_lock_unlock(&self->lock)) {
    OV_ASSERT(1 == 0);
    goto error;
  }

  return result;

error:
  return OV_VOCS_NONE;
}

/*----------------------------------------------------------------------------*/

ov_socket_configuration ov_vocs_db_get_multicast_group(ov_vocs_db *self,
                                                       const char *loop) {

  if (!self || !loop)
    goto error;
  if (!ov_thread_lock_try_lock(&self->lock))
    goto error;

  ov_json_value *l = ov_dict_get(self->index.loops, loop);
  const ov_json_value *mc = ov_json_get(l, "/" OV_KEY_MULTICAST);

  ov_socket_configuration config =
      ov_socket_configuration_from_json(mc, (ov_socket_configuration){0});

  config.type = UDP;

  if (!ov_thread_lock_unlock(&self->lock)) {
    OV_ASSERT(1 == 0);
    goto error;
  }

  return config;

error:
  return (ov_socket_configuration){0};
}

/*----------------------------------------------------------------------------*/

bool ov_vocs_db_set_volume(ov_vocs_db *self, const char *user, const char *role,
                           const char *loop, uint8_t volume) {
  bool result = false;

  if (volume > 100)
    goto error;
  if (!self || !user || !role || !loop)
    goto error;
  if (!ov_thread_lock_try_lock(&self->lock))
    goto error;

  ov_json_value *obj = get_set_loop(self, user, role, loop);
  if (!obj)
    goto done;

  ov_json_value *val = ov_json_number(volume);
  result = ov_json_object_set(obj, OV_KEY_VOLUME, val);

  if (!result)
    val = ov_json_value_free(val);

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

uint8_t ov_vocs_db_get_volume(ov_vocs_db *self, const char *user,
                              const char *role, const char *loop) {

  uint8_t result = 0;

  if (!self || !user || !role || !loop)
    goto error;
  if (!ov_thread_lock_try_lock(&self->lock))
    goto error;

  ov_json_value *obj = get_loop_settings(self, user, role, loop);
  if (!obj)
    goto done;

  ov_json_value *vol = ov_json_object_get(obj, OV_KEY_VOLUME);
  result = ov_json_number_get(vol);

done:
  if (!ov_thread_lock_unlock(&self->lock)) {
    OV_ASSERT(1 == 0);
    goto error;
  }

  return result;

error:
  return 0;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_vocs_db_get_layout(ov_vocs_db *self, const char *role) {

  ov_json_value *out = NULL;

  if (!self || !role)
    goto error;
  if (!ov_thread_lock_try_lock(&self->lock))
    goto error;

  ov_json_value *role_data = ov_dict_get(self->index.roles, role);
  if (!role_data)
    goto done;

  ov_json_value *layout = ov_json_object_get(role_data, OV_KEY_LAYOUT);

  if (!layout) {
    out = ov_json_object();
  } else {
    ov_json_value_copy((void **)&out, layout);
  }

done:

  if (!ov_thread_lock_unlock(&self->lock)) {
    OV_ASSERT(1 == 0);
    goto error;
  }

  return out;
error:
  ov_json_value_free(out);
  return NULL;
}

/*----------------------------------------------------------------------------*/

bool ov_vocs_db_set_layout(ov_vocs_db *self, const char *role,
                           const ov_json_value *layout) {

  bool result = false;

  ov_json_value *val = NULL;
  if (!self || !role || !layout)
    goto error;

  if (!ov_json_value_copy((void **)&val, layout))
    goto error;

  if (!ov_thread_lock_try_lock(&self->lock))
    goto error;

  ov_json_value *role_data = ov_dict_get(self->index.roles, role);
  if (!role_data) {
    val = ov_json_value_free(val);
    goto done;
  }

  result = ov_json_object_set(role_data, OV_KEY_LAYOUT, val);
  if (!result)
    val = ov_json_value_free(val);

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

bool ov_vocs_db_check_id_exists(ov_vocs_db *self, const char *id) {

  if (!self || !id)
    goto error;

  if (!ov_thread_lock_try_lock(&self->lock))
    goto error;

  bool exists = false;

  if (ov_dict_get(self->index.domains, id)) {

    exists = true;
    goto done;
  }

  if (ov_dict_get(self->index.projects, id)) {

    exists = true;
    goto done;
  }

  if (ov_dict_get(self->index.users, id)) {

    exists = true;
    goto done;
  }

  if (ov_dict_get(self->index.roles, id)) {

    exists = true;
    goto done;
  }

  if (ov_dict_get(self->index.loops, id)) {

    exists = true;
    goto done;
  }

done:
  if (!ov_thread_lock_unlock(&self->lock)) {
    OV_ASSERT(1 == 0);
    goto error;
  }

  return exists;
error:
  // default to true on error
  return true;
}

/*----------------------------------------------------------------------------*/

bool ov_vocs_db_check_entity_id_exists(ov_vocs_db *self,
                                       ov_vocs_db_entity entity,
                                       const char *id) {

  if (!self || !id)
    goto error;

  ov_dict *dict = NULL;

  switch (entity) {

  case OV_VOCS_DB_DOMAIN:
    dict = self->index.domains;
    break;
  case OV_VOCS_DB_PROJECT:
    dict = self->index.projects;
    break;
  case OV_VOCS_DB_LOOP:
    dict = self->index.loops;
    break;
  case OV_VOCS_DB_ROLE:
    dict = self->index.roles;
    break;
  case OV_VOCS_DB_USER:
    dict = self->index.users;
    break;
  default:
    break;
  }

  if (!dict)
    goto error;

  if (!ov_thread_lock_try_lock(&self->lock))
    goto error;

  bool exists = false;

  if (ov_dict_get(dict, id)) {
    exists = true;
  }

  if (!ov_thread_lock_unlock(&self->lock)) {
    OV_ASSERT(1 == 0);
    goto error;
  }

  return exists;
error:
  // default to true on error
  return true;
}

/*----------------------------------------------------------------------------*/

static ov_json_value *get_set_layout(ov_json_value *domain) {

  if (!domain)
    goto error;

  ov_json_value *layout = ov_json_object_get(domain, OV_KEY_LAYOUT);
  if (!layout) {

    layout = ov_json_object();
    if (!ov_json_object_set(domain, OV_KEY_LAYOUT, layout)) {
      layout = ov_json_value_free(layout);
      goto error;
    }
  }

  return layout;

error:
  return NULL;
}

/*----------------------------------------------------------------------------*/

bool ov_vocs_db_set_keyset_layout(ov_vocs_db *self, const char *domain,
                                  const char *name,
                                  const ov_json_value *layout) {

  bool result = false;

  if (!self || !domain || !name || !layout)
    goto error;

  if (!ov_thread_lock_try_lock(&self->lock))
    goto error;

  ov_json_value *root = ov_dict_get(self->index.domains, domain);
  if (!root)
    goto done;

  ov_json_value *layout_container = get_set_layout(root);
  if (!layout_container)
    goto done;

  ov_json_value *copy = NULL;
  if (!ov_json_value_copy((void **)&copy, layout))
    goto done;

  if (!ov_json_object_set(layout_container, name, copy)) {
    copy = ov_json_value_free(copy);
    goto done;
  }

  result = true;

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

static ov_json_value *create_default_layout() {

  ov_json_value *out = NULL;
  ov_json_value *val = NULL;

  out = ov_json_object();
  if (!out)
    goto error;

  val = ov_json_number(1.0);
  if (!ov_json_object_set(out, "site_scaling", val))
    goto error;

  val = ov_json_number(1.5);
  if (!ov_json_object_set(out, "name_scaling", val))
    goto error;

  val = ov_json_number(1.0);
  if (!ov_json_object_set(out, "font_scaling", val))
    goto error;

  val = ov_json_string("auto_grid");
  if (!ov_json_object_set(out, "layout", val))
    goto error;

  val = ov_json_string("15.625rem");
  if (!ov_json_object_set(out, "loop_size", val))
    goto error;

  return out;

error:
  ov_json_value_free(out);
  ov_json_value_free(val);
  return NULL;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_vocs_db_get_keyset_layout(ov_vocs_db *self,
                                            const char *domain,
                                            const char *name) {

  ov_json_value *out = NULL;

  if (!self || !domain || !name)
    goto error;

  if (!ov_thread_lock_try_lock(&self->lock))
    goto error;

  ov_json_value *root = ov_dict_get(self->index.domains, domain);
  if (!root)
    goto done;

  ov_json_value *layout_container = get_set_layout(root);
  if (!layout_container)
    goto done;

  ov_json_value *layout = ov_json_object_get(layout_container, name);
  if (!layout) {

    out = create_default_layout();

  } else {

    if (!ov_json_value_copy((void **)&out, layout))
      goto done;
  }

done:

  if (!ov_thread_lock_unlock(&self->lock)) {
    OV_ASSERT(1 == 0);
    goto error;
  }

  return out;

error:
  ov_json_value_free(out);
  return NULL;
}

/*----------------------------------------------------------------------------*/

static ov_json_value *get_set_user(ov_vocs_db *self, const char *user) {

  if (!self || !user)
    goto error;

  if (!self->data.state)
    self->data.state = ov_json_object();

  if (!self->data.state)
    goto error;

  ov_json_value *u = ov_json_object_get(self->data.state, user);

  if (!u) {

    u = ov_json_object();

    if (!ov_json_object_set(self->data.state, user, u)) {
      u = ov_json_value_free(u);
      goto error;
    }
  }

  return u;

error:
  return NULL;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_vocs_db_get_user_data(ov_vocs_db *self, const char *user) {

  ov_json_value *out = NULL;

  if (!self || !user)
    goto error;

  if (!ov_thread_lock_try_lock(&self->lock))
    goto error;

  ov_json_value *root = get_set_user(self, user);
  if (!root)
    goto done;

  ov_json_value *data = ov_json_object_get(root, OV_KEY_DATA);
  if (!data) {
    out = ov_json_object();
  } else {
    ov_json_value_copy((void **)&out, data);
  }

done:

  if (!ov_thread_lock_unlock(&self->lock)) {
    OV_ASSERT(1 == 0);
    goto error;
  }

error:
  return out;
}

/*----------------------------------------------------------------------------*/

bool ov_vocs_db_set_user_data(ov_vocs_db *self, const char *user,
                              const ov_json_value *data) {

  bool result = false;

  if (!self || !user || !data)
    goto error;

  if (!ov_thread_lock_try_lock(&self->lock))
    goto error;

  ov_json_value *root = get_set_user(self, user);
  if (!root)
    goto done;

  ov_json_value *copy = NULL;
  if (!ov_json_value_copy((void **)&copy, data))
    goto done;

  if (!ov_json_object_set(root, OV_KEY_DATA, copy)) {
    copy = ov_json_value_free(copy);
    goto done;
  }

  result = true;

done:

  if (!ov_thread_lock_unlock(&self->lock)) {
    OV_ASSERT(1 == 0);
    goto error;
  }

error:
  return result;
}

/*----------------------------------------------------------------------------*/

static bool add_recorded_loop(const void *key, void *val, void *data) {

  if (!key)
    return true;
  ov_json_value *value = ov_json_value_cast(val);
  ov_json_value *array = ov_json_value_cast(data);

  ov_json_value *rec = ov_json_object_get(value, OV_KEY_RECORDED);
  ov_json_value *mc = ov_json_object_get(value, OV_KEY_MULTICAST);

  if (!rec || !mc)
    return true;

  if (ov_json_is_true(rec)) {

    ov_json_value *out = ov_json_object();
    ov_json_value *v = NULL;
    ov_json_value_copy((void **)&v, mc);

    if (!ov_json_object_set(out, OV_KEY_MULTICAST, v)) {
      out = ov_json_value_free(out);
      v = ov_json_value_free(v);
      goto error;
    }

    v = NULL;
    v = ov_json_string((char *)key);
    if (!ov_json_object_set(out, OV_KEY_LOOP, v)) {
      v = ov_json_value_free(v);
      goto error;
    }

    if (!ov_json_array_push(array, out)) {
      out = ov_json_value_free(out);
      goto error;
    }
  }

  return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_vocs_db_get_recorded_loops(ov_vocs_db *self) {

  ov_json_value *out = NULL;

  if (!self)
    goto error;

  if (!ov_thread_lock_try_lock(&self->lock))
    goto error;

  out = ov_json_array();
  if (!out)
    goto done;

  if (!ov_dict_for_each(self->index.loops, out, add_recorded_loop)) {

    out = ov_json_value_free(out);
  }

done:

  if (!ov_thread_lock_unlock(&self->lock)) {
    OV_ASSERT(1 == 0);
    goto error;
  }

error:
  return out;
}

/*----------------------------------------------------------------------------*/

bool ov_vocs_db_set_recorded(ov_vocs_db *self, const char *loop, bool on) {

  bool result = false;

  if (!self || !loop)
    goto error;

  if (!ov_thread_lock_try_lock(&self->lock))
    goto error;

  ov_json_value *data = ov_dict_get(self->index.loops, loop);
  if (!data)
    goto done;

  ov_json_value *val = NULL;
  if (on) {
    val = ov_json_true();
  } else {
    val = ov_json_false();
  }

  result = ov_json_object_set(data, OV_KEY_RECORDED, val);
  if (!result)
    val = ov_json_value_free(val);

done:

  if (!ov_thread_lock_unlock(&self->lock)) {
    OV_ASSERT(1 == 0);
    goto error;
  }

error:
  return result;
}

/*----------------------------------------------------------------------------*/

static bool get_sip_information(const void *key, void *value, void *data) {

  if (!key)
    return true;

  ov_json_value *arr = ov_json_value_cast(data);
  ov_json_value *out = NULL;
  ov_json_value *val = NULL;
  ov_json_value *sip = ov_json_object_get(value, OV_KEY_SIP);
  if (!sip)
    return true;

  val = ov_json_string(key);

  ov_json_value_copy((void **)&out, sip);
  ov_json_object_set(out, OV_KEY_LOOP, val);
  ov_json_array_push(arr, out);
  return true;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_vocs_db_get_sip(ov_vocs_db *self) {

  ov_json_value *array = NULL;

  if (!self)
    goto error;

  if (!ov_thread_lock_try_lock(&self->lock))
    goto error;

  array = ov_json_array();

  ov_dict_for_each(self->index.loops, array, get_sip_information);

  if (!ov_thread_lock_unlock(&self->lock)) {
    OV_ASSERT(1 == 0);
    goto error;
  }
  return array;

error:
  return NULL;
}

/*----------------------------------------------------------------------------*/

bool ov_vocs_db_sip_allow_callout(ov_vocs_db *self, const char *loop,
                                  const char *role) {

  bool result = false;

  if (!self || !loop || !role)
    goto error;

  if (!ov_thread_lock_try_lock(&self->lock))
    goto error;

  ov_json_value *data = ov_dict_get(self->index.loops, loop);
  if (!data)
    goto done;

  ov_json_value *sip = (ov_json_value *)ov_json_get(data, "/" OV_KEY_SIP);
  if (!sip) {
    result = true;
    goto done;
  }

  ov_json_value *roles = (ov_json_value *)ov_json_get(sip, "/" OV_KEY_ROLES);
  if (!roles) {
    result = true;
    goto done;
  }

  ov_json_value *permission = ov_json_object_get(roles, role);
  if (!permission)
    goto done;

  if (ov_json_is_true(permission))
    result = true;

done:
  if (!ov_thread_lock_unlock(&self->lock)) {
    OV_ASSERT(1 == 0);
    goto error;
  }
error:
  return result;
}

/*----------------------------------------------------------------------------*/

bool ov_vocs_db_sip_allow_callend(ov_vocs_db *self, const char *loop,
                                  const char *role) {

  bool result = false;

  if (!self || !loop || !role)
    goto error;

  if (!ov_thread_lock_try_lock(&self->lock))
    goto error;

  ov_json_value *data = ov_dict_get(self->index.loops, loop);
  if (!data) {
    result = true;
    goto done;
  }

  ov_json_value *sip = (ov_json_value *)ov_json_get(data, "/" OV_KEY_SIP);
  if (!sip) {
    result = true;
    goto done;
  }

  ov_json_value *roles = (ov_json_value *)ov_json_get(sip, "/" OV_KEY_ROLES);
  if (!roles) {
    result = true;
    goto done;
  }

  ov_json_value *permission = ov_json_object_get(roles, role);
  if (permission)
    result = true;

done:
  if (!ov_thread_lock_unlock(&self->lock)) {
    OV_ASSERT(1 == 0);
    goto error;
  }
error:
  return result;
}

/*----------------------------------------------------------------------------*/

static bool add_loop_to_result(const void *key, void *val, void *data) {

  if (!key)
    return true;

  ov_json_value *value = NULL;

  ov_json_value_copy((void **)&value, val);

  ov_json_object_del(value, OV_KEY_ROLES);
  ov_json_object_del(value, OV_KEY_USERS);
  ov_json_object_del(value, OV_KEY_SIP);

  ov_json_value *out = ov_json_value_cast(data);
  ov_json_object_set(out, key, value);
  return true;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_vocs_db_get_all_loops(ov_vocs_db *self) {

  ov_json_value *result = NULL;
  if (!self)
    goto error;

  if (!ov_thread_lock_try_lock(&self->lock))
    goto error;

  result = ov_json_object();

  ov_dict_for_each(self->index.loops, result, add_loop_to_result);

  if (!ov_thread_lock_unlock(&self->lock)) {
    OV_ASSERT(1 == 0);
    goto error;
  }
error:
  return result;
}

/*----------------------------------------------------------------------------*/

static bool add_loop_and_domain_to_result(const void *key, void *val,
                                          void *data) {

  if (!key)
    return true;

  ov_json_value *copy = NULL;
  const ov_json_value *project = NULL;
  const ov_json_value *domain = NULL;

  ov_json_value_copy((void **)&copy, val);

  ov_json_object_del(copy, OV_KEY_ROLES);
  ov_json_object_del(copy, OV_KEY_USERS);
  ov_json_object_del(copy, OV_KEY_SIP);

  ov_json_value *out = ov_json_value_cast(data);
  ov_json_object_set(out, key, copy);

  ov_json_value *loop = ov_json_value_cast(val);

  ov_json_value *loops = loop->parent;
  if (!loops)
    goto done;

  ov_json_value *project_or_domain = loops->parent;
  if (!project_or_domain)
    goto done;

  ov_json_value *project_or_domain_parent = project_or_domain->parent;

  if (project_or_domain_parent) {

    project = project_or_domain;

    ov_json_value *domain_projects = project->parent;
    if (!domain_projects)
      goto error;

    domain = domain_projects->parent;

  } else {

    project = NULL;
    domain = project_or_domain;
  }

  if (domain) {

    ov_json_value *dom = NULL;
    ov_json_value_copy((void **)&dom, domain);

    ov_json_object_del(dom, OV_KEY_PROJECTS);
    ov_json_object_del(dom, OV_KEY_LOOPS);
    ov_json_object_del(dom, OV_KEY_ROLES);
    ov_json_object_del(dom, OV_KEY_USERS);

    ov_json_object_set(copy, OV_KEY_DOMAIN, dom);
  }

  if (project) {

    ov_json_value *pro = NULL;
    ov_json_value_copy((void **)&pro, project);

    ov_json_object_del(pro, OV_KEY_PROJECTS);
    ov_json_object_del(pro, OV_KEY_LOOPS);
    ov_json_object_del(pro, OV_KEY_ROLES);
    ov_json_object_del(pro, OV_KEY_USERS);

    ov_json_object_set(copy, OV_KEY_PROJECT, pro);
  }

done:
  return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_vocs_db_get_all_loops_incl_domain(ov_vocs_db *self) {

  ov_json_value *result = NULL;
  if (!self)
    goto error;

  if (!ov_thread_lock_try_lock(&self->lock))
    goto error;

  result = ov_json_object();

  ov_dict_for_each(self->index.loops, result, add_loop_and_domain_to_result);

  if (!ov_thread_lock_unlock(&self->lock)) {
    OV_ASSERT(1 == 0);
    goto error;
  }
error:
  return result;
}

/*----------------------------------------------------------------------------*/

bool ov_vocs_db_remove_permission(ov_vocs_db *self,
                                  ov_sip_permission permission) {

  bool result = false;
  if (!self)
    goto error;

  if (!ov_thread_lock_try_lock(&self->lock))
    goto error;

  ov_json_value *loop = ov_dict_get(self->index.loops, permission.loop);
  if (!loop)
    goto done;

  ov_json_value *sip = ov_json_object_get(loop, "/" OV_KEY_SIP);
  if (!sip)
    goto done;

  ov_json_value *whitelist = ov_json_object_get(sip, OV_KEY_WHITELIST);
  if (!whitelist)
    goto done;

  size_t count = ov_json_array_count(whitelist);
  size_t del = 0;

  bool ok = true;

  for (size_t i = 1; i <= count; i++) {

    ov_json_value *obj = ov_json_array_get(whitelist, i);

    ov_sip_permission perm = ov_sip_permission_from_json(obj, &ok);

    if (ov_sip_permission_equals(permission, perm)) {
      del = i;
      break;
    }
  }

  if (0 != del) {
    result = ov_json_array_del(whitelist, del);
  }

done:
  if (!ov_thread_lock_unlock(&self->lock)) {
    OV_ASSERT(1 == 0);
    goto error;
  }
error:
  return result;
}

/*----------------------------------------------------------------------------*/

bool ov_vocs_db_add_permission(ov_vocs_db *self, ov_sip_permission permission) {

  if (!self)
    goto error;

  if (!ov_thread_lock_try_lock(&self->lock))
    goto error;

  ov_json_value *loop = ov_dict_get(self->index.loops, permission.loop);
  if (!loop)
    goto done;

  ov_json_value *sip = ov_json_object_get(loop, "/" OV_KEY_SIP);
  if (!sip) {

    sip = ov_json_object();
    ov_json_object_set(loop, OV_KEY_SIP, sip);
  }

  ov_json_value *whitelist = ov_json_object_get(sip, OV_KEY_WHITELIST);
  if (!whitelist) {

    whitelist = ov_json_array();
    ov_json_object_set(sip, OV_KEY_WHITELIST, whitelist);
  }

  size_t count = ov_json_array_count(whitelist);
  size_t active = 0;

  bool ok = true;

  for (size_t i = 1; i <= count; i++) {

    ov_json_value *obj = ov_json_array_get(whitelist, i);

    ov_sip_permission perm = ov_sip_permission_from_json(obj, &ok);

    if (ov_sip_permission_equals(permission, perm)) {
      active = i;
      break;
    }
  }

  if (0 == active) {

    ov_json_value *perm = ov_sip_permission_to_json(permission);
    ov_json_array_push(whitelist, perm);
  }

done:
  return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

bool ov_vocs_db_set_persistance(ov_vocs_db *self,
                                ov_vocs_db_persistance *persistance) {

  if (!self || !persistance)
    return false;

  self->persistance = persistance;
  return true;
}

/*----------------------------------------------------------------------------*/

static bool get_highest_port(const void *key, void *val, void *data) {

  if (!key)
    return true;

  ov_json_value *loop = ov_json_value_cast(val);
  uint32_t *high = (uint32_t *)data;

  const ov_json_value *mc = ov_json_get(loop, "/" OV_KEY_MULTICAST);
  uint32_t port = ov_json_number_get(ov_json_get(mc, "/" OV_KEY_PORT));

  if (port > *high)
    *high = port;

  return true;
}

/*----------------------------------------------------------------------------*/

uint32_t ov_vocs_db_get_highest_port(ov_vocs_db *self) {

  if (!self)
    goto error;

  if (!ov_thread_lock_try_lock(&self->lock))
    goto error;

  uint32_t result = 0;

  ov_dict_for_each(self->index.loops, &result, get_highest_port);

  if (!ov_thread_lock_unlock(&self->lock)) {
    OV_ASSERT(1 == 0);
    goto error;
  }

  return (uint32_t)result;

error:
  return 0;
}