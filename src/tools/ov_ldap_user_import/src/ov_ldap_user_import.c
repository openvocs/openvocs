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
        @file           ov_ldap_user_import.c
        @author         Markus Töpfer

        @date           2022-03-10


        ------------------------------------------------------------------------
*/

#include <lber.h>
#include <ldap.h>
#include <stdio.h>
#include <stdlib.h>

#include <ov_base/ov_config.h>
#include <ov_base/ov_config_keys.h>
#include <ov_base/ov_json.h>
#include <ov_base/ov_utils.h>

#define CONFIG_PATH                                                            \
  OPENVOCS_ROOT                                                                \
  "/src/tools/ov_ldap_user_import/config/default_config.json"

/*----------------------------------------------------------------------------*/

static LDAP *ldap_bind(const char *host, const char *user, const char *pass) {

  int version = LDAP_VERSION3;
  LDAP *ld = NULL;
  LDAPMessage *res = NULL;
  int msgid = 0;
  int err = 0;

  char *dn = NULL;

  if (!host || !user || !pass)
    goto error;

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

    fprintf(stderr, "ldap_set_option(PROTOCOL_VERSION): %s\n",
            ldap_err2string(err));
    goto error;
  };

  struct timeval timeout = {.tv_sec = 3};

  err = ldap_set_option(ld, LDAP_OPT_NETWORK_TIMEOUT, &timeout);
  if (err != LDAP_SUCCESS) {

    fprintf(stderr, "ldap_set_option(SIZELIMIT): %s\n", ldap_err2string(err));
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
  if (ld)
    ldap_unbind_ext_s(ld, NULL, NULL);
  return NULL;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ldap_get_users(const char *host, const char *base,
                              const char *user, const char *pass,
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

  if (!base || !user || !host || !pass)
    goto error;

  char *filter = "(&(objectClass=posixAccount))";

  char *attrs[4] = {0};
  attrs[0] = "cn";
  attrs[1] = "sn";
  attrs[2] = "uid";
  attrs[3] = NULL;

  ld = ldap_bind(host, user, pass);
  if (!ld)
    goto error;

  int err = 0;

  struct timeval timeout = {.tv_sec = timeout_usec / 1000000,
                            .tv_usec = timeout_usec % 1000000};

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

      if (0 == strcmp(attribute, "sn"))
        surname = strdup(vals[0]->bv_val);

      if (0 == strcmp(attribute, "cn"))
        forename = strdup(vals[0]->bv_val);

      if (0 == strcmp(attribute, "uid"))
        uid = strdup(vals[0]->bv_val);

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

      if (!ov_json_object_set(val, OV_KEY_ID, userid))
        goto error;

      userid = NULL;

      if (forename) {

        // snprintf(name, 1000, "%s %s", forename, surname);
        username = ov_json_string(forename);

        if (!ov_json_object_set(val, OV_KEY_NAME, username))
          goto error;

        username = NULL;
      }

      char *str = ov_json_value_to_string(val);
      if (!str) {

        val = ov_json_value_free(val);

        fprintf(stdout, "\n\nMISSED UID %s %s\n\n", uid, forename);

      } else {

        if (!ov_json_object_set(out, uid, val))
          goto error;

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
  if (ld)
    ldap_unbind_ext_s(ld, NULL, NULL);
  return NULL;
}

/*----------------------------------------------------------------------------*/

static bool write_new_config(const ov_json_value *users, const char *domain,
                             const char *path) {

  ov_json_value *out = NULL;
  ov_json_value *val = NULL;

  if (!users || !domain || !path)
    goto error;

  val = NULL;
  if (!ov_json_value_copy((void **)&val, users))
    goto error;

  out = ov_json_object();
  if (!ov_json_object_set(out, OV_KEY_USERS, val))
    goto error;

  val = ov_json_string(domain);
  if (!ov_json_object_set(out, OV_KEY_ID, val))
    goto error;

  if (!ov_json_write_file(path, out))
    goto error;

  out = ov_json_value_free(out);

  ov_log_debug("Created config of users for domain %s at path %s", domain,
               path);

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
};

/*----------------------------------------------------------------------------*/

static bool add_new_user(const void *key, void *val, void *data) {

  if (!key)
    return true;

  char *user_id = (char *)key;
  ov_json_value *user = ov_json_value_cast(val);

  struct users_search *u = (struct users_search *)data;

  ov_json_value *active_users = ov_json_value_cast(u->active_users);

  if (ov_json_object_get(active_users, user_id)) {
    return true;
  } else {
    return ov_list_push(u->outdated, user_id);
  }

  ov_json_value *out = NULL;

  if (!ov_json_value_copy((void **)&out, user))
    goto error;

  if (!ov_json_object_set(active_users, user_id, out)) {
    out = ov_json_value_free(out);
    goto error;
  }

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

static bool write_users_object(const ov_json_value *users, const char *domain,
                               const char *path) {

  ov_list *list = NULL;

  ov_json_value *out = NULL;
  ov_json_value *val = NULL;

  if (!users || !domain || !path)
    goto error;

  ov_json_value *current = ov_json_read_file(path);
  if (!current)
    return write_new_config(users, domain, path);

  const char *domain_id =
      ov_json_string_get(ov_json_get(current, "/" OV_KEY_ID));
  if (!domain_id) {

    ov_log_error("Update for domain %s, "
                 "but no domain included in file at path %s",
                 domain, path);

    goto error;
  }

  if (0 != strcmp(domain_id, domain)) {

    ov_log_error("Update for domain %s, "
                 "but config of domain %s at path %s",
                 domain, domain_id, path);

    goto error;
  }

  ov_json_value const *active_users = ov_json_get(current, OV_KEY_USERS);
  if (!active_users) {

    out = NULL;
    if (!ov_json_value_copy((void **)&out, users))
      goto error;

    if (!ov_json_object_set(current, OV_KEY_USERS, out))
      goto error;

  } else {

    list = ov_list_create((ov_list_config){0});

    struct users_search container =
        (struct users_search){.active_users = active_users, .outdated = list};

    if (!ov_json_object_for_each((ov_json_value *)users, &container,
                                 add_new_user))
      goto error;

    if (!ov_list_for_each(list, (void *)active_users, drop_outdated))
      goto error;

    list = ov_list_free(list);
  }

  if (!ov_json_write_file(path, current))
    goto error;

  ov_log_debug("Update of users for domain %s at path %s - done", domain, path);

  return true;

error:
  ov_json_value_free(val);
  ov_json_value_free(out);
  ov_list_free(list);
  return false;
}

/*----------------------------------------------------------------------------*/

int main(int argc, char **argv) {

  ov_json_value *config = NULL;
  ov_json_value *users = NULL;

  const char *config_path = ov_config_path_from_command_line(argc, argv);
  if (!config_path)
    config_path = CONFIG_PATH;

  if (config_path == VERSION_REQUEST_ONLY)
    goto error;

  config = ov_config_load(config_path);
  if (!config)
    goto error;

  const char *host =
      ov_json_string_get(ov_json_get(config, "/" OV_KEY_LDAP "/" OV_KEY_HOST));

  const char *base =
      ov_json_string_get(ov_json_get(config, "/" OV_KEY_LDAP "/" OV_KEY_BASE));

  const char *user =
      ov_json_string_get(ov_json_get(config, "/" OV_KEY_LDAP "/" OV_KEY_USER));

  const char *pass = ov_json_string_get(
      ov_json_get(config, "/" OV_KEY_LDAP "/" OV_KEY_PASSWORD));

  const char *domain =
      ov_json_string_get(ov_json_get(config, "/" OV_KEY_DOMAIN));

  const char *target_path =
      ov_json_string_get(ov_json_get(config, "/" OV_KEY_PATH));

  uint64_t timeout = ov_json_number_get(
      ov_json_get(config, "/" OV_KEY_LDAP "/" OV_KEY_TIMEOUT_USEC));

  if (0 == timeout)
    timeout = 5000000;

  fprintf(stdout, "using host %s user %s pass %s path %s\n", host, user, pass,
          target_path);

  users = ldap_get_users(host, base, user, pass, timeout);
  if (!users) {
    fprintf(stderr, "failed to generate users object");
    goto error;
  }

  if (!write_users_object(users, domain, target_path))
    goto error;

  ov_json_value_free(config);
  return EXIT_SUCCESS;
error:
  ov_json_value_free(users);
  ov_json_value_free(config);
  return EXIT_FAILURE;
}
