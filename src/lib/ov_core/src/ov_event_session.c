/***
        ------------------------------------------------------------------------

        Copyright (c) 2023 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_event_session.c
        @author         Markus Töpfer

        @date           2023-12-09


        ------------------------------------------------------------------------
*/
#include "../include/ov_event_session.h"

#include <ov_base/ov_config_keys.h>
#include <ov_base/ov_dict.h>
#include <ov_base/ov_dir.h>
#include <ov_base/ov_id.h>
#include <ov_base/ov_linked_list.h>
#include <ov_base/ov_string.h>
#include <ov_base/ov_time.h>

#define OV_EVENT_SESSION_MAGIC_BYTES 0x531d
#define OV_EVENT_SESSION_DEFAULT_LIFETIME 3600000000             // 60 min
#define OV_EVENT_SESSION_DEFAULT_LIFETIME_CHECK 60 * 1000 * 1000 // 1 min

/*----------------------------------------------------------------------------*/

struct ov_event_session {

    uint16_t magic_bytes;
    ov_event_session_config config;

    ov_dict *sessions;

    struct {

        uint32_t invalidate;

    } timer;
};

/*----------------------------------------------------------------------------*/

typedef struct Session {

    uint64_t last_update;

    ov_id id;
    char *client;
    char *user;

} Session;

/*----------------------------------------------------------------------------*/

static void *session_free(void *self) {

    if (!self) goto error;

    Session *s = (Session *)self;
    s->client = ov_data_pointer_free(s->client);
    s->user = ov_data_pointer_free(s->user);
    self = ov_data_pointer_free(self);

error:
    return self;
}

/*----------------------------------------------------------------------------*/

static bool check_config(ov_event_session_config *config) {

    if (!config->loop) goto error;

    if (0 == config->limit.max_lifetime_usec)
        config->limit.max_lifetime_usec = OV_EVENT_SESSION_DEFAULT_LIFETIME;

    if (0 == config->path[0])
        strncpy(config->path, OV_EVENT_SESSIONS_PATH, PATH_MAX);

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

struct container {

    uint64_t now;
    ov_event_session *self;
    ov_list *expired;
};

/*----------------------------------------------------------------------------*/

static bool delete_session(void *key, void *data) {

    ov_event_session *self = ov_event_session_cast(data);
    if (!self || !key) goto error;

    return ov_dict_del(self->sessions, key);
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool check_invalid_session(const void *key, void *val, void *data) {

    if (!key) return true;

    Session *s = (Session *)val;
    struct container *c = (struct container *)data;

    if (!s || !c) goto error;

    if ((c->now - s->last_update) > c->self->config.limit.max_lifetime_usec) {

        ov_list_push(c->expired, (void *)key);
    }

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool invalidate_expired_sessions(uint32_t timer, void *data) {

    ov_event_session *self = ov_event_session_cast(data);
    UNUSED(timer);
    if (!self) goto error;

    struct container container = (struct container){
        .now = ov_time_get_current_time_usecs(),
        .self = self,
        .expired = ov_linked_list_create((ov_list_config){0})};

    ov_dict_for_each(self->sessions, &container, check_invalid_session);

    ov_list_for_each(container.expired, self, delete_session);

    container.expired = ov_list_free(container.expired);

    self->timer.invalidate =
        ov_event_loop_timer_set(self->config.loop,
                                OV_EVENT_SESSION_DEFAULT_LIFETIME_CHECK,
                                self,
                                invalidate_expired_sessions);

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

ov_event_session *ov_event_session_create(ov_event_session_config config) {

    ov_event_session *self = NULL;
    if (!check_config(&config)) goto error;

    self = calloc(1, sizeof(ov_event_session));
    if (!self) goto error;

    self->magic_bytes = OV_EVENT_SESSION_MAGIC_BYTES;
    self->config = config;

    ov_dict_config d_config = ov_dict_string_key_config(255);
    d_config.value.data_function.free = session_free;

    self->sessions = ov_dict_create(d_config);
    if (!self->sessions) goto error;

    self->timer.invalidate =
        ov_event_loop_timer_set(self->config.loop,
                                OV_EVENT_SESSION_DEFAULT_LIFETIME_CHECK,
                                self,
                                invalidate_expired_sessions);

    if (self->timer.invalidate == OV_TIMER_INVALID) goto error;

    ov_event_session_load(self);

    return self;

error:
    ov_event_session_free(self);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_event_session *ov_event_session_free(ov_event_session *self) {

    if (!ov_event_session_cast(self)) goto error;

    if (OV_TIMER_INVALID != self->timer.invalidate)
        ov_event_loop_timer_unset(
            self->config.loop, self->timer.invalidate, NULL);

    self->sessions = ov_dict_free(self->sessions);
    self = ov_data_pointer_free(self);
error:
    return self;
}

/*----------------------------------------------------------------------------*/

ov_event_session *ov_event_session_cast(const void *self) {

    if (!self) goto error;

    if (*(uint16_t *)self == OV_EVENT_SESSION_MAGIC_BYTES)
        return (ov_event_session *)self;
error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

const char *ov_event_session_init(ov_event_session *self,
                                  const char *client,
                                  const char *user) {

    if (!self || !client || !user) goto error;

    Session *s = calloc(1, sizeof(Session));
    if (!s) goto error;

    char *key = ov_string_dup(client);
    if (!ov_dict_set(self->sessions, key, s, NULL)) {
        key = ov_data_pointer_free(key);
        s = ov_data_pointer_free(s);
        goto error;
    }

    s->last_update = ov_time_get_current_time_usecs();
    s->client = ov_string_dup(client);
    s->user = ov_string_dup(user);
    ov_id_fill_with_uuid(s->id);

    ov_event_session_save(self);

    return s->id;

error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

bool ov_event_session_delete(ov_event_session *self, const char *client) {

    if (!self || !client) goto error;

    bool result = ov_dict_del(self->sessions, client);

    ov_event_session_save(self);
    return result;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_event_session_update(ov_event_session *self,
                             const char *client,
                             const char *user,
                             const char *id) {

    if (!self || !client || !id) goto error;

    Session *s = ov_dict_get(self->sessions, client);
    if (!s) goto error;

    if (!ov_id_match(id, s->id)) goto error;
    if (0 != strcmp(s->user, user)) goto error;

    s->last_update = ov_time_get_current_time_usecs();

    ov_event_session_save(self);
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

const char *ov_event_session_get_user(ov_event_session *self,
                                      const char *client) {

    if (!self || !client) goto error;

    Session *s = ov_dict_get(self->sessions, client);
    if (!s) goto error;

    return s->user;
error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

bool ov_event_session_verify(ov_event_session *self,
                             const char *client,
                             const char *user,
                             const char *id) {

    if (!self || !client || !user || !id) goto error;

    Session *s = ov_dict_get(self->sessions, client);
    if (!s) goto error;

    if (!ov_id_match(id, s->id)) goto error;

    if (0 != strcmp(s->user, user)) goto error;

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static ov_json_value *session_to_json(Session *s) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;

    if (!s) goto error;

    out = ov_json_object();

    val = ov_json_number(s->last_update);
    if (!ov_json_object_set(out, OV_KEY_LAST_UPDATE, val)) goto error;

    val = ov_json_string((char *)s->id);
    if (!ov_json_object_set(out, OV_KEY_ID, val)) goto error;

    val = ov_json_string((char *)s->client);
    if (!ov_json_object_set(out, OV_KEY_CLIENT, val)) goto error;

    val = ov_json_string((char *)s->user);
    if (!ov_json_object_set(out, OV_KEY_USER, val)) goto error;

    return out;
error:
    ov_json_value_free(val);
    ov_json_value_free(out);
    return NULL;
}

/*----------------------------------------------------------------------------*/

static Session *session_from_json(ov_json_value *item) {

    Session *s = NULL;
    if (!item) goto error;

    s = calloc(1, sizeof(Session));

    s->last_update =
        ov_json_number_get(ov_json_object_get(item, OV_KEY_LAST_UPDATE));

    const char *string = NULL;

    string = ov_json_string_get(ov_json_object_get(item, OV_KEY_ID));

    if (string) ov_id_set(s->id, string);

    string = ov_json_string_get(ov_json_object_get(item, OV_KEY_CLIENT));

    if (string) s->client = ov_string_dup(string);

    string = ov_json_string_get(ov_json_object_get(item, OV_KEY_USER));

    if (string) s->user = ov_string_dup(string);

    return s;
error:
    session_free(s);
    return NULL;
}

/*----------------------------------------------------------------------------*/

static bool add_session_to_out(const void *key, void *value, void *data) {

    if (!key) return true;

    ov_json_value *out = ov_json_value_cast(data);

    Session *session = (Session *)value;

    ov_json_value *val = session_to_json(session);
    if (!ov_json_object_set(out, (char *)key, val)) goto error;

    return true;
error:
    ov_json_value_free(val);
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_event_session_save(ov_event_session *self) {

    ov_json_value *out = NULL;

    if (!self) goto error;

    if (!ov_dir_access_to_path(self->config.path)) {

        if (!ov_dir_tree_create(self->config.path)) {
            ov_log_error("Faile to create dir %s", self->config.path);
        }

        OV_ASSERT(ov_dir_access_to_path(self->config.path));
    }

    out = ov_json_object();

    if (!ov_dict_for_each(self->sessions, out, add_session_to_out)) goto error;

    char path[PATH_MAX + 20] = {0};
    snprintf(path,
             PATH_MAX + 20,
             "%s/%s",
             self->config.path,
             OV_EVENT_SESSIONS_FILE);

    if (!ov_json_write_file(path, out)) {

        ov_log_error("Failed to save events at %s", path);
        goto error;
    }

    ov_json_value_free(out);
    return true;
error:
    ov_json_value_free(out);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool add_session_data(const void *key, void *value, void *data) {

    if (!key) return true;

    ov_json_value *val = ov_json_value_cast(value);
    ov_event_session *self = ov_event_session_cast(data);

    Session *s = session_from_json(val);
    if (!s) goto error;

    char *k = ov_string_dup((char *)key);

    if (!ov_dict_set(self->sessions, k, s, NULL)) {
        k = ov_data_pointer_free(k);
        s = ov_data_pointer_free(s);
        goto error;
    }

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_event_session_load(ov_event_session *self) {

    ov_json_value *data = NULL;

    if (!self) goto error;

    char path[PATH_MAX + 20] = {0};
    snprintf(path,
             PATH_MAX + 20,
             "%s/%s",
             self->config.path,
             OV_EVENT_SESSIONS_FILE);

    data = ov_json_read_file(path);
    if (!data) {
        ov_log_error("Failed to read sessions from %s", path);
        goto error;
    }

    if (!ov_json_object_for_each(data, self, add_session_data)) goto error;

    ov_json_value_free(data);
    return true;
error:
    ov_json_value_free(data);
    return false;
}