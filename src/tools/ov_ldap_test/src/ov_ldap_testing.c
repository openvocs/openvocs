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
        @file           ov_ldap_test.c
        @author         Markus Töpfer

        @date           2022-03-07


        ------------------------------------------------------------------------
*/

#include <ov_base/ov_event_loop.h>
#include <ov_ldap/ov_ldap.h>
#include <stdio.h>
#include <stdlib.h>

struct dummy_userdata {

  int id;
};

/*----------------------------------------------------------------------------*/

static void dummy_callback(void *userdata, const char *uuid,
                           ov_ldap_auth_result res) {

  UNUSED(userdata);

  const char *str = "rejected";
  if (OV_LDAP_AUTH_GRANTED == res)
    str = "granted";

  fprintf(stderr, "AUTH %s %s", uuid, str);
  return;
}

/*----------------------------------------------------------------------------*/

int main(int argc, char **argv) {

  UNUSED(argc);
  UNUSED(argv);

  struct dummy_userdata userdata = {0};

  ov_event_loop *loop = ov_event_loop_default(
      (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

  ov_ldap_config config = (ov_ldap_config){

      .loop = loop,
      .host = "192.168.1.47",
      .user_dn_tree = "ou=people,dc=openvocs,dc=org"

  };

  ov_ldap *ldap = ov_ldap_create(config);
  if (!ldap)
    goto error;

  char *dn = "johndoe";
  char *pw = "2simple!";

  ov_ldap_authenticate_password(
      ldap, dn, pw, "1-2-3-4",
      (ov_ldap_auth_callback){.userdata = &userdata,
                              .callback = dummy_callback});

  loop->run(loop, OV_RUN_MAX);

  loop = ov_event_loop_free(loop);
  return EXIT_SUCCESS;
error:
  return EXIT_FAILURE;
}