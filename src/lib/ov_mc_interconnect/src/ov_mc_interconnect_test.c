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
        @file           ov_mc_interconnect_test.c
        @author         Markus

        @date           2023-12-13


        ------------------------------------------------------------------------
*/
#include "ov_mc_interconnect.c"

#include <ov_test/ov_test.h>

#ifndef OV_TEST_CERT
#error "Must provide -D OV_TEST_CERT=value while compiling this file."
#endif

#ifndef OV_TEST_CERT_KEY
#error "Must provide -D OV_TEST_CERT_KEY=value while compiling this file."
#endif

#ifndef OV_TEST_CERT_ONE
#error "Must provide -D OV_TEST_CERT_ONE=value while compiling this file."
#endif

#ifndef OV_TEST_CERT_ONE_KEY
#error "Must provide -D OV_TEST_CERT_ONE_KEY=value while compiling this file."
#endif

#ifndef OV_TEST_CERT_TWO
#error "Must provide -D OV_TEST_CERT_TWO=value while compiling this file."
#endif

#ifndef OV_TEST_CERT_TWO_KEY
#error "Must provide -D OV_TEST_CERT_TWO_KEY=value while compiling this file."
#endif

#include "../include/ov_mc_interconnect_test_common.h"

#include <ov_core/ov_io_base.h>

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

/*----------------------------------------------------------------------------*/

static bool set_resources_dir(char *dest) {

  OV_ASSERT(0 != dest);

  char *res =
      strncpy(dest, ov_mc_interconnect_test_common_get_test_resource_dir(),
              PATH_MAX - 1);

  if (0 == res)
    return false;

  /* Check for plausibility ... */

  return true;
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_interconnect_create() {

  ov_event_loop *loop = ov_event_loop_default(
      (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

  testrun(loop);

  ov_socket_configuration signaling = ov_socket_load_dynamic_port(
      (ov_socket_configuration){.type = TLS, .host = "localhost"});

  ov_socket_configuration media = ov_socket_load_dynamic_port(
      (ov_socket_configuration){.type = UDP, .host = "localhost"});

  ov_socket_configuration internal = ov_socket_load_dynamic_port(
      (ov_socket_configuration){.type = UDP, .host = "localhost"});

  ov_mc_interconnect_config config = (ov_mc_interconnect_config){

      .loop = loop,
      .name = "name",
      .password = "password",
      .socket.signaling = signaling,
      .socket.media = media,
      .socket.internal = internal,

      .dtls.cert = OV_TEST_CERT,
      .dtls.key = OV_TEST_CERT_KEY,
      .dtls.ca.file = OV_TEST_CERT

  };

  set_resources_dir(config.tls.domains);

  ov_mc_interconnect *self = ov_mc_interconnect_create(config);
  testrun(self);
  testrun(self->dtls);
  testrun(self->event.engine);
  testrun(self->event.socket);
  testrun(-1 != self->socket.signaling);
  testrun(-1 != self->socket.media);
  testrun(self->session.by_signaling_remote);
  testrun(self->session.by_media_remote);
  testrun(self->loops);
  testrun(ov_mc_interconnect_cast(self));

  testrun(NULL == ov_mc_interconnect_free(self));
  testrun(NULL == ov_event_loop_free(loop));
  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_interconnect_free() {

  ov_event_loop *loop = ov_event_loop_default(
      (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

  testrun(loop);

  ov_socket_configuration signaling = ov_socket_load_dynamic_port(
      (ov_socket_configuration){.type = TLS, .host = "localhost"});

  ov_socket_configuration media = ov_socket_load_dynamic_port(
      (ov_socket_configuration){.type = UDP, .host = "localhost"});

  ov_socket_configuration internal = ov_socket_load_dynamic_port(
      (ov_socket_configuration){.type = UDP, .host = "localhost"});

  ov_mc_interconnect_config config = (ov_mc_interconnect_config){

      .loop = loop,
      .name = "name",
      .password = "password",
      .socket.signaling = signaling,
      .socket.media = media,
      .socket.internal = internal,

      .dtls.cert = OV_TEST_CERT,
      .dtls.key = OV_TEST_CERT_KEY,
      .dtls.ca.file = OV_TEST_CERT

  };

  set_resources_dir(config.tls.domains);

  ov_mc_interconnect *self = ov_mc_interconnect_create(config);

  ov_json_value *loops = ov_mc_interconnect_test_common_get_loops();
  ov_json_value_dump(stdout, loops);
  testrun(ov_mc_interconnect_load_loops(self, loops));
  loops = ov_json_value_free(loops);

  ov_socket_data sig = (ov_socket_data){.host = "127.0.0.1", .port = 12345};
  ov_socket_data med = (ov_socket_data){.host = "127.0.0.1", .port = 12346};

  ov_mc_interconnect_session *session =
      ov_mc_interconnect_session_create((ov_mc_interconnect_session_config){
          .loop = loop,
          .base = self,
          .dtls = self->dtls,
          .signaling = 1,
          .internal = self->config.socket.internal,
          .remote.interface = "test",
          .remote.signaling = sig,
          .remote.media = med

      });

  testrun(session);
  testrun(1 == ov_dict_count(self->session.by_signaling_remote));
  testrun(1 == ov_dict_count(self->session.by_media_remote));
  testrun(3 == ov_dict_count(self->loops));

  testrun(NULL == ov_mc_interconnect_free(self));
  testrun(NULL == ov_event_loop_free(loop));
  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_interconnect_config_from_json() {

  const char *str =
      "{"
      "\"interconnect\" :"
      "{"
      "\"name\" : \"OP server\","
      "\"password\" : \"test\","

      "\"socket\" :"
      "{"
      "\"client\" :true,"
      "\"signaling\": {"
      "\"host\" : \"127.0.0.1\","
      "\"port\" : 11111,"
      "\"type\" : \"TCP\""
      "},"
      "\"media\": {"
      "\"host\" : \"127.0.0.1\","
      "\"port\" : 22222,"
      "\"type\" : \"UDP\""
      "},"
      "\"internal\": {"
      "\"host\" : \"127.0.0.1\","
      "\"port\" : 33333,"
      "\"type\" : \"UDP\""
      "}"
      "},"
      "\"tls\" :"
      "{"
      "\"domains\":\"./src/service/ov_mc_interconnect/config/domains\","

      "\"client\" :"
      "{"
      "\"domain\" : \"localhost\","
      "\"CA file\" : \"resources/certificate/openvocs.test.crt\""
      "},"

      "\"dtls\" :"
      "{"
      "\"certificate\" : \"resources/certificate/openvocs.test.crt\","
      "\"key\" :  \"resources/certificate/openvocs.test.key\","
      "\"CA file\" : \"resources/certificate/openvocs.test.crt\""
      "}"
      "},"

      "\"limits\" :"
      "{"
      "\"threads\" : 4,"
      "\"reconnect_interval_secs\": 4,"
      "\"keepalive (sec)\": 300"
      "}"
      "}"
      "}";

  ov_json_value *conf = ov_json_value_from_string(str, strlen(str));
  testrun(conf);

  ov_mc_interconnect_config config = ov_mc_interconnect_config_from_json(conf);

  testrun(0 == strcmp("OP server", config.name));
  testrun(0 == strcmp("test", config.password));
  testrun(config.socket.client);
  testrun(0 == strcmp("127.0.0.1", config.socket.signaling.host));
  testrun(0 == strcmp("127.0.0.1", config.socket.media.host));
  testrun(0 == strcmp("127.0.0.1", config.socket.internal.host));
  testrun(11111 == config.socket.signaling.port);
  testrun(22222 == config.socket.media.port);
  testrun(33333 == config.socket.internal.port);

  testrun(0 == strcmp("./src/service/ov_mc_interconnect/config/domains",
                      config.tls.domains));
  testrun(0 == strcmp("localhost", config.tls.client.domain));
  testrun(0 == strcmp("resources/certificate/openvocs.test.crt",
                      config.tls.client.ca.file));

  testrun(0 ==
          strcmp(config.dtls.cert, "resources/certificate/openvocs.test.crt"));
  testrun(0 ==
          strcmp(config.dtls.key, "resources/certificate/openvocs.test.key"));
  testrun(0 == strcmp(config.dtls.ca.file,
                      "resources/certificate/openvocs.test.crt"));

  testrun(4 == config.limits.threads);
  testrun(4000000 == config.limits.client_connect_trigger_usec);
  testrun(300000000 == config.limits.keepalive_trigger_usec);

  testrun(NULL == ov_json_value_free(conf));
  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_interconnect_register_session() {

  ov_event_loop *loop = ov_event_loop_default(
      (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

  testrun(loop);

  ov_socket_configuration signaling = ov_socket_load_dynamic_port(
      (ov_socket_configuration){.type = TLS, .host = "localhost"});

  ov_socket_configuration media = ov_socket_load_dynamic_port(
      (ov_socket_configuration){.type = UDP, .host = "localhost"});

  ov_socket_configuration internal = ov_socket_load_dynamic_port(
      (ov_socket_configuration){.type = UDP, .host = "localhost"});

  ov_mc_interconnect_config config = (ov_mc_interconnect_config){

      .loop = loop,
      .name = "name",
      .password = "password",
      .socket.signaling = signaling,
      .socket.media = media,
      .socket.internal = internal,

      .dtls.cert = OV_TEST_CERT,
      .dtls.key = OV_TEST_CERT_KEY,
      .dtls.ca.file = OV_TEST_CERT

  };

  set_resources_dir(config.tls.domains);

  ov_mc_interconnect *self = ov_mc_interconnect_create(config);

  ov_json_value *loops = ov_mc_interconnect_test_common_get_loops();
  testrun(ov_mc_interconnect_load_loops(self, loops));
  loops = ov_json_value_free(loops);

  // indirect test using session creation

  ov_socket_data sig = (ov_socket_data){.host = "127.0.0.1", .port = 12345};
  ov_socket_data med = (ov_socket_data){.host = "127.0.0.1", .port = 12346};

  ov_mc_interconnect_session *session =
      ov_mc_interconnect_session_create((ov_mc_interconnect_session_config){
          .loop = loop,
          .base = self,
          .dtls = self->dtls,
          .signaling = 1,
          .internal = self->config.socket.internal,
          .remote.interface = "test",
          .remote.signaling = sig,
          .remote.media = med

      });

  testrun(session);
  testrun(1 == ov_dict_count(self->session.by_signaling_remote));
  testrun(1 == ov_dict_count(self->session.by_media_remote));
  testrun(3 == ov_dict_count(self->loops));

  testrun(NULL == ov_mc_interconnect_free(self));
  testrun(NULL == ov_event_loop_free(loop));
  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_interconnect_unregister_session() {

  ov_event_loop *loop = ov_event_loop_default(
      (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

  testrun(loop);

  ov_socket_configuration signaling = ov_socket_load_dynamic_port(
      (ov_socket_configuration){.type = TLS, .host = "localhost"});

  ov_socket_configuration media = ov_socket_load_dynamic_port(
      (ov_socket_configuration){.type = UDP, .host = "localhost"});

  ov_socket_configuration internal = ov_socket_load_dynamic_port(
      (ov_socket_configuration){.type = UDP, .host = "localhost"});

  ov_mc_interconnect_config config = (ov_mc_interconnect_config){

      .loop = loop,
      .name = "name",
      .password = "password",
      .socket.signaling = signaling,
      .socket.media = media,
      .socket.internal = internal,

      .dtls.cert = OV_TEST_CERT,
      .dtls.key = OV_TEST_CERT_KEY,
      .dtls.ca.file = OV_TEST_CERT

  };

  set_resources_dir(config.tls.domains);

  ov_mc_interconnect *self = ov_mc_interconnect_create(config);

  ov_json_value *loops = ov_mc_interconnect_test_common_get_loops();
  testrun(ov_mc_interconnect_load_loops(self, loops));
  loops = ov_json_value_free(loops);

  // indirect creation using session creation

  ov_socket_data sig = (ov_socket_data){.host = "127.0.0.1", .port = 12345};
  ov_socket_data med = (ov_socket_data){.host = "127.0.0.1", .port = 12346};

  ov_mc_interconnect_session *session =
      ov_mc_interconnect_session_create((ov_mc_interconnect_session_config){
          .loop = loop,
          .base = self,
          .dtls = self->dtls,
          .signaling = 1,
          .internal = self->config.socket.internal,
          .remote.interface = "test",
          .remote.signaling = sig,
          .remote.media = med

      });

  testrun(session);
  testrun(1 == ov_dict_count(self->session.by_signaling_remote));
  testrun(1 == ov_dict_count(self->session.by_media_remote));
  testrun(3 == ov_dict_count(self->loops));

  testrun(ov_mc_interconnect_unregister_session(
      self, (ov_socket_data){.host = "127.0.0.1", .port = 12345},
      (ov_socket_data){.host = "127.0.0.1", .port = 12346}));

  testrun(0 == ov_dict_count(self->session.by_signaling_remote));
  testrun(1 == ov_dict_count(self->session.by_media_remote));

  testrun(NULL == ov_mc_interconnect_free(self));
  testrun(NULL == ov_event_loop_free(loop));
  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_interconnect_get_loop() {

  ov_event_loop *loop = ov_event_loop_default(
      (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

  testrun(loop);

  ov_socket_configuration signaling = ov_socket_load_dynamic_port(
      (ov_socket_configuration){.type = TLS, .host = "localhost"});

  ov_socket_configuration media = ov_socket_load_dynamic_port(
      (ov_socket_configuration){.type = UDP, .host = "localhost"});

  ov_socket_configuration internal = ov_socket_load_dynamic_port(
      (ov_socket_configuration){.type = UDP, .host = "localhost"});

  ov_mc_interconnect_config config = (ov_mc_interconnect_config){

      .loop = loop,
      .name = "name",
      .password = "password",
      .socket.signaling = signaling,
      .socket.media = media,
      .socket.internal = internal,

      .dtls.cert = OV_TEST_CERT,
      .dtls.key = OV_TEST_CERT_KEY,
      .dtls.ca.file = OV_TEST_CERT

  };

  set_resources_dir(config.tls.domains);

  ov_mc_interconnect *self = ov_mc_interconnect_create(config);

  ov_json_value *loops = ov_mc_interconnect_test_common_get_loops();
  testrun(ov_mc_interconnect_load_loops(self, loops));
  loops = ov_json_value_free(loops);

  testrun(3 == ov_dict_count(self->loops));

  ov_mc_interconnect_loop *loop1 = ov_mc_interconnect_get_loop(self, "loop1");
  ov_mc_interconnect_loop *loop2 = ov_mc_interconnect_get_loop(self, "loop2");
  ov_mc_interconnect_loop *loop3 = ov_mc_interconnect_get_loop(self, "loop3");

  testrun(loop1);
  testrun(loop2);
  testrun(loop3);

  testrun(0 == strcmp("loop1", ov_mc_interconnect_loop_get_name(loop1)));
  testrun(0 == strcmp("loop2", ov_mc_interconnect_loop_get_name(loop2)));
  testrun(0 == strcmp("loop3", ov_mc_interconnect_loop_get_name(loop3)));

  testrun(NULL == ov_mc_interconnect_free(self));
  testrun(NULL == ov_event_loop_free(loop));
  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_mc_interconnect_get_media_socket() {

  ov_event_loop *loop = ov_event_loop_default(
      (ov_event_loop_config){.max.sockets = 100, .max.timers = 100});

  testrun(loop);

  ov_socket_configuration signaling = ov_socket_load_dynamic_port(
      (ov_socket_configuration){.type = TLS, .host = "localhost"});

  ov_socket_configuration media = ov_socket_load_dynamic_port(
      (ov_socket_configuration){.type = UDP, .host = "localhost"});

  ov_socket_configuration internal = ov_socket_load_dynamic_port(
      (ov_socket_configuration){.type = UDP, .host = "localhost"});

  ov_mc_interconnect_config config = (ov_mc_interconnect_config){

      .loop = loop,
      .name = "name",
      .password = "password",
      .socket.signaling = signaling,
      .socket.media = media,
      .socket.internal = internal,

      .dtls.cert = OV_TEST_CERT,
      .dtls.key = OV_TEST_CERT_KEY,
      .dtls.ca.file = OV_TEST_CERT

  };

  set_resources_dir(config.tls.domains);

  ov_mc_interconnect *self = ov_mc_interconnect_create(config);

  int socket = ov_mc_interconnect_get_media_socket(self);
  testrun(-1 != socket);
  testrun(socket == self->socket.media);

  testrun(NULL == ov_mc_interconnect_free(self));
  testrun(NULL == ov_event_loop_free(loop));
  return testrun_log_success();
}

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CLUSTER                                                    #CLUSTER
 *
 *      ------------------------------------------------------------------------
 */

OV_TEST_RUN("ov_mc_interconnect", ov_mc_interconnect_common_domains_init,

            test_ov_mc_interconnect_create, test_ov_mc_interconnect_free,
            test_ov_mc_interconnect_config_from_json,

            test_ov_mc_interconnect_register_session,
            test_ov_mc_interconnect_unregister_session,

            test_ov_mc_interconnect_get_loop,
            test_ov_mc_interconnect_get_media_socket,

            ov_mc_interconnect_common_domains_deinit);
