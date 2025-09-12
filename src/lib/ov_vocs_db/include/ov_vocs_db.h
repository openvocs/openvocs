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
        @file           ov_vocs_db.h
        @author         Markus Töpfer

        @date           2022-07-23

        ov_vocs_db is the central database for vocs authentication and state
        management. It is some RBAC data store for authentication and state data
        used and required within a vocs system.

        Authentication structure is as follows. Users Roles and Loops may be
        clustered in domains and/or projects.

        The database ensures unique:
            1. domain IDs
            2. project IDs
            3. loop IDs
            4. role IDs
            5. user IDs

        {
            "domain1":
            {
                "id":"domain1",
                "projects":
                {
                    "project1":
                    {
                        "id":"project1",
                        "loops": { ... },
                        "roles" : { ... },
                        "users" : { ... }
                    }
                }
                "loops": { ... },
                "roles" : { ... },
                "users" : { ... }
            }
        }

        Within the structures any content may be set and persisted. The db
        verifies the overall structure of domains containing projects containing
        Role based Access Control data items.

    },


        ------------------------------------------------------------------------
*/
#ifndef ov_vocs_db_h
#define ov_vocs_db_h

#include <inttypes.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <ov_base/ov_json.h>
#include <ov_base/ov_socket.h>
#include <ov_core/ov_event_trigger.h>
#include <ov_core/ov_password.h>
#include <ov_sip/ov_sip_permission.h>

#include "ov_vocs_json.h"
#include "ov_vocs_permission.h"


#define OV_VOCS_DB_KEY_LDAP_UPDATE "ldap_update"

/*----------------------------------------------------------------------------*/

typedef enum ov_vocs_db_scope {

    OV_VOCS_DB_SCOPE_DOMAIN = 0,
    OV_VOCS_DB_SCOPE_PROJECT = 1

} ov_vocs_db_scope;

/*----------------------------------------------------------------------------*/

typedef struct ov_vocs_db_parent {

    ov_vocs_db_scope scope;
    char *id;

} ov_vocs_db_parent;

/*----------------------------------------------------------------------------*/

typedef enum ov_vocs_db_type {

    OV_VOCS_DB_TYPE_AUTH = 0,
    OV_VOCS_DB_TYPE_STATE = 1

} ov_vocs_db_type;

/*----------------------------------------------------------------------------*/

typedef enum ov_vocs_db_entity {

    OV_VOCS_DB_ENTITY_ERROR = -1,
    OV_VOCS_DB_DOMAIN = 0,
    OV_VOCS_DB_PROJECT = 1,
    OV_VOCS_DB_LOOP = 2,
    OV_VOCS_DB_ROLE = 3,
    OV_VOCS_DB_USER = 4

} ov_vocs_db_entity;

/*----------------------------------------------------------------------------*/

typedef struct ov_vocs_db ov_vocs_db;

/*----------------------------------------------------------------------------*/

typedef struct ov_vocs_db_config {

    struct {

        uint64_t thread_lock_usec;

    } timeout;

    struct {

        ov_password_hash_parameter params;
        size_t length;

    } password;

    ov_event_trigger *trigger;

} ov_vocs_db_config;

#include "ov_vocs_db_persistance.h"

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_vocs_db *ov_vocs_db_create(ov_vocs_db_config config);
ov_vocs_db *ov_vocs_db_free(ov_vocs_db *self);
ov_vocs_db *ov_vocs_db_cast(const void *self);

/*----------------------------------------------------------------------------*/

bool ov_vocs_db_dump(FILE *stream, ov_vocs_db *self);

/*----------------------------------------------------------------------------*/

ov_vocs_db_config ov_vocs_db_config_from_json(const ov_json_value *value);

ov_vocs_db_entity ov_vocs_db_entity_from_string(const char *string);

bool ov_vocs_db_set_persistance(ov_vocs_db *self, ov_vocs_db_persistance *persistance);

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC DB FUNCTIONS
 *
 *      {
 *          "key" : <val>,
 *          "id" : {},          // predef key containing id
 *          "name" : {},        // predef key containing name
 *      }
 *
 *      ------------------------------------------------------------------------
 */

/**
 *      Get parent information of an entity.
 *
 *      @param self     instance pointer
 *      @param entity   entity type
 *      @param id       entity id
 */
ov_vocs_db_parent ov_vocs_db_get_parent(ov_vocs_db *self,
                                        ov_vocs_db_entity entity,
                                        const char *id);

/*----------------------------------------------------------------------------*/

bool ov_vocs_db_parent_clear(ov_vocs_db_parent *parent);

/*----------------------------------------------------------------------------*/

/**
 *      Get a copy of the entity.
 *
 *      @param self     instance pointer
 *      @param entity   entity type
 *      @param id       entity id
 */
ov_json_value *ov_vocs_db_get_entity(ov_vocs_db *self,
                                     ov_vocs_db_entity entity,
                                     const char *id);

/*----------------------------------------------------------------------------*/

/**
 *      Get domain of the entity.
 *
 *      @param self     instance pointer
 *      @param entity   entity type
 *      @param id       entity id
 */
ov_json_value *ov_vocs_db_get_entity_domain(ov_vocs_db *self,
                                            ov_vocs_db_entity entity,
                                            const char *id);

/*----------------------------------------------------------------------------*/

/**
 *      Get a copy of the entity key.
 *
 *      @param self     instance pointer
 *      @param entity   entity type
 *      @param id       entity id
 *      @param key      key to get
 */
ov_json_value *ov_vocs_db_get_entity_key(ov_vocs_db *self,
                                         ov_vocs_db_entity entity,
                                         const char *id,
                                         const char *key);

/*----------------------------------------------------------------------------*/

/**
 *      Delete an entity
 *
 *      @param self     instance pointer
 *      @param entity   entity type
 *      @param id       entity id
 */
bool ov_vocs_db_delete_entity(ov_vocs_db *self,
                              ov_vocs_db_entity entity,
                              const char *id);

/*----------------------------------------------------------------------------*/

/**
 *      Create an entity
 *
 *      @param self     instance pointer
 *      @param entity   entity type
 *      @param id       id to create
 *      @param scope    domain or project scope for user role and loop,
 *                      MUST be OV_VOCS_DB_SCOPE_DOMAIN for domain and projects
 *      @param scope_id domain or project id
 */
bool ov_vocs_db_create_entity(ov_vocs_db *self,
                              ov_vocs_db_entity entity,
                              const char *id,
                              ov_vocs_db_scope scope,
                              const char *scope_id);

/*----------------------------------------------------------------------------*/

/**
 *      Update an entity key
 *
 *      @param self     instance pointer
 *      @param entity   entity type
 *      @param id       id to create
 *      @param key      key to set
 *      @param val      value to set
 */
bool ov_vocs_db_update_entity_key(ov_vocs_db *self,
                                  ov_vocs_db_entity entity,
                                  const char *id,
                                  const char *key,
                                  const ov_json_value *val);

/*----------------------------------------------------------------------------*/

/**
 *      Delete an entity key
 *
 *      @param self     instance pointer
 *      @param entity   entity type
 *      @param id       id to create
 *      @param key      key to set
 */
bool ov_vocs_db_delete_entity_key(ov_vocs_db *self,
                                  ov_vocs_db_entity entity,
                                  const char *id,
                                  const char *key);

/*----------------------------------------------------------------------------*/

/**
 *      Verify an entity item to be set.
 *
 *      This function may be used to verify some import before trying to import
 *      the data.
 *
 *      @param self     instance pointer
 *      @param entity   entity type
 *      @param id       id to update
 *      @param val      value to set
 *      @param errors   pointer to JSON object for error return
 */
bool ov_vocs_db_verify_entity_item(ov_vocs_db *self,
                                   ov_vocs_db_entity entity,
                                   const char *id,
                                   const ov_json_value *val,
                                   ov_json_value **errors);

/*----------------------------------------------------------------------------*/

/**
 *      Update an the whole entity item. Verifies first.
 *
 *      This function will only add/override existing keys and not delete
 *      any key set within the db, but not in val.
 *
 *      Use ov_vocs_db_delete_entity_key to delete some keys in objects.
 *
 *      @param self     instance pointer
 *      @param entity   entity type
 *      @param id       id to update
 *      @param val      value to set
 */
bool ov_vocs_db_update_entity_item(ov_vocs_db *self,
                                   ov_vocs_db_entity entity,
                                   const char *id,
                                   const ov_json_value *val,
                                   ov_json_value **errors);

/*
 *      ------------------------------------------------------------------------
 *
 *      INJECT / EJECT the database
 *
 *      ------------------------------------------------------------------------
 */

/**
 *      Inject a new data set.
 *
 *      @param self     instance pointer
 *      @param type     type of data
 *      @param data     dataset to inject
 */
bool ov_vocs_db_inject(ov_vocs_db *self,
                       ov_vocs_db_type type,
                       ov_json_value *data);

/*----------------------------------------------------------------------------*/

/**
 *      Eject a copy of the data set.
 *
 *      @param self     instance pointer
 *      @param type     type of data
 */
ov_json_value *ov_vocs_db_eject(ov_vocs_db *self, ov_vocs_db_type type);

/*
 *      ------------------------------------------------------------------------
 *
 *      AUTHENTICATION FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

/**
 *      Updata some user password.
 *
 *      @params self    instance pointer
 *      @params user    user id
 *      @params pass    clear text password
 */
bool ov_vocs_db_set_password(ov_vocs_db *self,
                             const char *user,
                             const char *pass);

/*----------------------------------------------------------------------------*/

/**
 *      Authenticate some user with password.
 *
 *      @params self    instance pointer
 *      @params user    user id
 *      @params pass    clear text password
 */
bool ov_vocs_db_authenticate(ov_vocs_db *self,
                             const char *user,
                             const char *pass);

/*----------------------------------------------------------------------------*/

/**
 *      Authorize some role for user.
 *
 *      @params self    instance pointer
 *      @params user    user id
 *      @params role    role id
 */
bool ov_vocs_db_authorize(ov_vocs_db *self, const char *user, const char *role);

/*----------------------------------------------------------------------------*/

/**
 *      Get permission level of some role in loop.
 *
 *      @params self    instance pointer
 *      @params role    role id
 *      @params loop    loop id
 */
ov_vocs_permission ov_vocs_db_get_permission(ov_vocs_db *self,
                                             const char *role,
                                             const char *loop);

/*
 *      ------------------------------------------------------------------------
 *
 *      ADMIN FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

bool ov_vocs_db_add_domain_admin(ov_vocs_db *self,
                                 const char *domain,
                                 const char *admin_id);

/*----------------------------------------------------------------------------*/

bool ov_vocs_db_add_project_admin(ov_vocs_db *self,
                                  const char *project,
                                  const char *admin_id);

/*----------------------------------------------------------------------------*/

/**
 *      Authorize some project admin.
 *
 *      @returns true if the user is some domain admin for the project,
 *      or the user is some actual project admin
 *
 *      @param self     instance pointer
 *      @param user     user id to check
 *      @param project  project id to check
 */
bool ov_vocs_db_authorize_project_admin(ov_vocs_db *self,
                                        const char *user,
                                        const char *project);

/*----------------------------------------------------------------------------*/

/**
 *      Authorize some domain admin.
 *
 *      @returns true if the user is some domain admin for the domain.
 *
 *      @param self     instance pointer
 *      @param user     user id to check
 *      @param project  project id to check
 */
bool ov_vocs_db_authorize_domain_admin(ov_vocs_db *self,
                                       const char *user,
                                       const char *domain);

/*----------------------------------------------------------------------------*/

/**
 *      Get some JSON array of domains, where the user is admin.
 *
 *      @param self     instance pointer
 *      @param user     user id to check
 */
ov_json_value *ov_vocs_db_get_admin_domains(ov_vocs_db *self, const char *user);

/*----------------------------------------------------------------------------*/

/**
 *      Get some JSON array of projects, where the user is admin.
 *
 *      @param self     instance pointer
 *      @param user     user id to check
 */
ov_json_value *ov_vocs_db_get_admin_projects(ov_vocs_db *self,
                                             const char *user);

/*
 *      ------------------------------------------------------------------------
 *
 *      CHECK IF ID EXISTS
 *
 *      ------------------------------------------------------------------------
 */

/*
    Check if some id exists within all indexed items
*/
bool ov_vocs_db_check_id_exists(ov_vocs_db *self, const char *id);

/*----------------------------------------------------------------------------*/

/*
    Check if some id exists within specific indexed items e.g. user
*/
bool ov_vocs_db_check_entity_id_exists(ov_vocs_db *self,
                                       ov_vocs_db_entity entity,
                                       const char *id);

/*
 *      ------------------------------------------------------------------------
 *
 *      DATA FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

/**
 *      Get a copy of the user's roles
 *
 *      @params self    instance pointer
 *      @params user    user id
 */
ov_json_value *ov_vocs_db_get_user_roles(ov_vocs_db *self, const char *user);

/*----------------------------------------------------------------------------*/

/**
 *      Get a copy of the role's loops
 *
 *      @params self    instance pointer
 *      @params role    role id
 */
ov_json_value *ov_vocs_db_get_role_loops(ov_vocs_db *self, const char *role);

/*----------------------------------------------------------------------------*/

/**
 *      Get a copy of the role's loops including some users state.
 *
 *      @params self    instance pointer
 *      @params role    role id
 */
ov_json_value *ov_vocs_db_get_user_role_loops(ov_vocs_db *self,
                                              const char *user,
                                              const char *role);

/*----------------------------------------------------------------------------*/

/**
 *      Get a copy of the role's layout.
 *
 *      @params self    instance pointer
 *      @params role    role id
 */
ov_json_value *ov_vocs_db_get_layout(ov_vocs_db *self, const char *role);

/*----------------------------------------------------------------------------*/

/**
 *      Set the role's layout.
 *
 *      @params self    instance pointer
 *      @params role    role id
 *      @params layout  layout of the role in form { "loopid" : pos }
 */
bool ov_vocs_db_set_layout(ov_vocs_db *self,
                           const char *role,
                           const ov_json_value *layout);

/*----------------------------------------------------------------------------*/

bool ov_vocs_db_set_keyset_layout(ov_vocs_db *self,
                                  const char *domain,
                                  const char *name,
                                  const ov_json_value *layout);

/*----------------------------------------------------------------------------*/

ov_json_value *ov_vocs_db_get_keyset_layout(ov_vocs_db *self,
                                            const char *domain,
                                            const char *name);

/*----------------------------------------------------------------------------*/

ov_json_value *ov_vocs_db_get_user_data(ov_vocs_db *self, const char *user);

/*----------------------------------------------------------------------------*/

bool ov_vocs_db_set_user_data(ov_vocs_db *self,
                              const char *user,
                              const ov_json_value *data);

/*
 *      ------------------------------------------------------------------------
 *
 *      STATE FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

bool ov_vocs_db_set_state(ov_vocs_db *self,
                          const char *user,
                          const char *role,
                          const char *loop,
                          ov_vocs_permission state);

/*----------------------------------------------------------------------------*/

ov_vocs_permission ov_vocs_db_get_state(ov_vocs_db *self,
                                        const char *user,
                                        const char *role,
                                        const char *loop);

/*----------------------------------------------------------------------------*/

ov_socket_configuration ov_vocs_db_get_multicast_group(ov_vocs_db *self,
                                                       const char *loop);

/*----------------------------------------------------------------------------*/

bool ov_vocs_db_set_volume(ov_vocs_db *self,
                           const char *user,
                           const char *role,
                           const char *loop,
                           uint8_t volume);

/*----------------------------------------------------------------------------*/

uint8_t ov_vocs_db_get_volume(ov_vocs_db *self,
                              const char *user,
                              const char *role,
                              const char *loop);

/*----------------------------------------------------------------------------*/

ov_json_value *ov_vocs_db_get_recorded_loops(ov_vocs_db *self);

/*----------------------------------------------------------------------------*/

bool ov_vocs_db_set_recorded(ov_vocs_db *self, const char *loop, bool on);

/*----------------------------------------------------------------------------*/

ov_json_value *ov_vocs_db_get_sip(ov_vocs_db *self);

/*----------------------------------------------------------------------------*/

bool ov_vocs_db_sip_allow_callout(ov_vocs_db *self,
                                  const char *loop,
                                  const char *role);

/*----------------------------------------------------------------------------*/

bool ov_vocs_db_sip_allow_callend(ov_vocs_db *self,
                                  const char *loop,
                                  const char *role);

/*----------------------------------------------------------------------------*/

ov_json_value *ov_vocs_db_get_all_loops(ov_vocs_db *self);

/*----------------------------------------------------------------------------*/

bool ov_vocs_db_remove_permission(ov_vocs_db *self, 
    ov_sip_permission permission);

/*----------------------------------------------------------------------------*/

ov_json_value *ov_vocs_db_get_all_loops_incl_domain(ov_vocs_db *self);

/*----------------------------------------------------------------------------*/

bool ov_vocs_db_send_vocs_trigger(ov_vocs_db *self, const ov_json_value *msg);

/*----------------------------------------------------------------------------*/

uint32_t ov_vocs_db_get_highest_port(ov_vocs_db *self);

#endif /* ov_vocs_db_h */
