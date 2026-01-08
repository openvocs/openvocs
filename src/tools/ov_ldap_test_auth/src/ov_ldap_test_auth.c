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
        @file           ov_ldap_test_auth.c
        @author         Markus Töpfer

        @date           2022-03-04


        ------------------------------------------------------------------------
*/

#include <lber.h>
#include <ldap.h>
#include <ov_base/ov_config_keys.h>
#include <ov_base/ov_json_value.h>
#include <ov_base/ov_utils.h>
#include <sasl/sasl.h>
#include <stdio.h>
#include <stdlib.h>

static bool authenticate_user(const char *host) {

  int version = LDAP_VERSION3;
  LDAPMessage *res;
  LDAP *ld;
  int msgid = 0;
  struct timeval zerotime;
  int err = 0;

  char server[OV_HOST_NAME_MAX] = {0};
  snprintf(server, OV_HOST_NAME_MAX, "ldap://%s", host);

  char *dn = "uid=johndoe,ou=people,dc=openvocs,dc=org";
  char *pw = "2simple!";

  struct berval cred = (struct berval){.bv_len = strlen(pw), .bv_val = pw};

  zerotime.tv_sec = zerotime.tv_usec = 0L;

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

  /* Print out an informational message. */
  printf("Binding to server %s:%d as %s\n", host, LDAP_PORT, dn);

  if (ldap_sasl_bind(ld, dn, LDAP_SASL_SIMPLE, &cred, NULL, NULL, &msgid) !=
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

  ldap_unbind_ext_s(ld, NULL, NULL);
  return true;

error:

  if (ld)
    ldap_unbind_ext_s(ld, NULL, NULL);

  return false;
}

/*----------------------------------------------------------------------------*/

static LDAP *ldap_bind(const char *host) {

  char *dn = "cn=admin,dc=openvocs,dc=org";
  char *pw = "2simple!";

  int version = LDAP_VERSION3;
  LDAP *ld = NULL;
  LDAPMessage *res = NULL;
  int msgid = 0;
  int err = 0;

  char server[OV_HOST_NAME_MAX] = {0};
  snprintf(server, OV_HOST_NAME_MAX, "ldap://%s", host);

  struct berval cred = (struct berval){.bv_len = strlen(pw), .bv_val = pw};

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

  printf("binding to server %s:%d as %s\n", host, LDAP_PORT, dn);

  if (ldap_sasl_bind(ld, dn, LDAP_SASL_SIMPLE, &cred, NULL, NULL, &msgid) !=
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

/*
static bool get_user_parameter(const char *host){
*/
/*
    Commandline search:

    ldapsearch -x -H "ldap://192.168.1.47"
    -D cn=admin,dc=openvocs,dc=org
    -W -b "dc=openvocs,dc=org"
    '(&(objectClass=posixAccount))' cn sn uid

*/
/*
    char *base = "dc=openvocs,dc=org";
    char *filter = "(&(objectClass=posixAccount))";

    char *attrs[4] = { 0 };

    attrs[0] = "cn";
    attrs[1] = "sn";
    attrs[2] = "uid";
    attrs[3] = NULL;


    LDAPMessage *res = NULL;
    LDAP *ld = ldap_bind(host);
    if (!ld)
        goto error;

    int err = 0;

    struct timeval  timeout = {
        .tv_sec = 3
    };

    err = ldap_search_ext_s(
        ld,                         // LDAP            * ld
        base,                       // char            * base
        LDAP_SCOPE_SUBTREE,         // int               scope
        filter,                     // char            * filter
        attrs,                       // char            * attrs[]
        0,                          // int               attrsonly
        NULL,                       // LDAPControl    ** serverctrls
        NULL,                       // LDAPControl    ** clientctrls
        &timeout,                   // struct timeval  * timeout
        0,                          // int               sizelimit
        &res                        // LDAPMessage    ** res
    );

    if (err != LDAP_SUCCESS){
        fprintf(stderr, "ldap_search_ext_s(): %s\n", ldap_err2string(err));
        goto error;
    };

    if (!(ldap_count_entries(ld, res))){
        printf("0 entries found.\n");
        goto error;

    };
    printf("# %i entries found.\n", ldap_count_entries(ld, res));

    // loops through entries, attributes, and values
    LDAPMessage *entry = ldap_first_entry(ld, res);
    while((entry))
    {
        char *dn = ldap_get_dn(ld, entry);
        fprintf(stderr, "dn: %s\n", dn);
        ldap_memfree(dn);

        BerElement *ber = NULL;

        char *attribute = ldap_first_attribute(ld, entry, &ber);
        while((attribute))
        {
            struct berval **vals = ldap_get_values_len(ld, entry, attribute);
            for(int pos = 0; pos < ldap_count_values_len(vals); pos++){
                printf("%s: %s\n", attribute, vals[pos]->bv_val);
                //ldap_value_free_len(vals);
                attribute = ldap_next_attribute(ld, entry, ber);
            }
        };
        ber_free(ber, 0);

        printf("\n");

        LDAPMessage* new_entry = ldap_next_entry(ld, entry);
        ldap_memfree(entry);
        entry = new_entry;
   };



    ldap_unbind_ext_s(ld, NULL, NULL);
    return true;
error:
    if (res)
        ldap_msgfree(res);
    if (ld)
        ldap_unbind_ext_s(ld, NULL, NULL);
    return false;
}
*/
/*----------------------------------------------------------------------------*/

static bool get_all_users(const char *host) {

  /*
      Commandline search:

      ldapsearch -x -H "ldap://192.168.1.47"
      -D cn=admin,dc=openvocs,dc=org
      -W -b "dc=openvocs,dc=org"
      '(&(objectClass=posixAccount))' cn sn uid

  */

  char *base = "dc=openvocs,dc=org";
  char *filter = "(&(objectClass=posixAccount))";

  char *attrs[4] = {0};

  attrs[0] = "cn";
  attrs[1] = "sn";
  attrs[2] = "uid";
  attrs[3] = NULL;

  LDAPMessage *res = NULL;
  LDAP *ld = ldap_bind(host);
  if (!ld)
    goto error;

  int err = 0;

  struct timeval timeout = {.tv_sec = 3};

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
  printf("# %i entries found.\n", ldap_count_entries(ld, res));

  ov_json_value *users = ov_json_object();

  char name[255] = {0};

  // loops through entries, attributes, and values
  LDAPMessage *entry = ldap_first_entry(ld, res);
  while ((entry)) {
    char *dn = ldap_get_dn(ld, entry);
    fprintf(stderr, "dn: %s\n", dn);
    ldap_memfree(dn);

    BerElement *ber = NULL;

    char *surname = NULL;
    char *forename = NULL;
    char *uid = NULL;

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

    /*fprintf(stdout, "Forename %s, Surname %s, uid %s\n",
            forename, surname, uid);
    */
    snprintf(name, 255, "%s %s", forename, surname);
    ov_json_value *username = ov_json_string(name);
    ov_json_value *userid = ov_json_string(uid);
    ov_json_value *out = ov_json_object();
    ov_json_object_set(out, OV_KEY_ID, userid);
    ov_json_object_set(out, OV_KEY_NAME, username);
    ov_json_object_set(users, uid, out);

    free(surname);
    free(forename);
    free(uid);

    ber_free(ber, 0);

    printf("\n");

    LDAPMessage *new_entry = ldap_next_entry(ld, entry);
    ldap_memfree(entry);
    entry = new_entry;
  };

  char *str = ov_json_value_to_string(users);
  fprintf(stdout, "%s\n", str);
  str = ov_data_pointer_free(str);

  ldap_unbind_ext_s(ld, NULL, NULL);
  return true;
error:
  if (res)
    ldap_msgfree(res);
  if (ld)
    ldap_unbind_ext_s(ld, NULL, NULL);
  return false;
}

/*----------------------------------------------------------------------------*/

int main(int argc, char **argv) {

  UNUSED(argc);
  UNUSED(argv);

  // const char *host = "192.168.1.47";
  const char *host = "localhost";

  if (!authenticate_user(host))
    goto error;
  /*
      if (!get_user_parameter(host))
          goto error;
  */

  if (!get_all_users(host))
    goto error;

  return EXIT_SUCCESS;
error:
  return EXIT_FAILURE;
}