/***
        ------------------------------------------------------------------------

        Copyright (c) 2024 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_ice_proxy_generic_dtls_cookie_test.c
        @author         Markus

        @date           2024-07-26


        ------------------------------------------------------------------------
*/
#include "ov_ice_proxy_generic_dtls_cookie.c"
#include <openssl/ssl.h>
#include <ov_test/testrun.h>

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CASES                                                      #CASES
 *
 *      ------------------------------------------------------------------------
 */

int test_ov_ice_proxy_generic_dtls_cookie_store_create() {

  ov_ice_proxy_generic_dtls_cookie_store *store =
      ov_ice_proxy_generic_dtls_cookie_store_create();
  testrun(store);
  testrun(store == global_dtls_ice_cookie_store);
  testrun(ov_thread_lock_try_lock(&store->lock));
  testrun(ov_thread_lock_unlock(&store->lock));
  testrun(NULL == store->cookie);
  testrun(NULL == ov_ice_proxy_generic_dtls_cookie_store_free(store));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_ice_proxy_generic_dtls_cookie_store_free() {

  ov_ice_proxy_generic_dtls_cookie_store *store =
      ov_ice_proxy_generic_dtls_cookie_store_create();
  testrun(store);
  testrun(store == global_dtls_ice_cookie_store);
  testrun(NULL == ov_ice_proxy_generic_dtls_cookie_store_free(store));
  testrun(NULL == global_dtls_ice_cookie_store);

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_ice_proxy_generic_dtls_cookie_store_initialize() {

  ov_ice_proxy_generic_dtls_cookie_store *store =
      ov_ice_proxy_generic_dtls_cookie_store_create();
  testrun(store);

  testrun(!ov_ice_proxy_generic_dtls_cookie_store_initialize(NULL, 0, 0));
  testrun(!ov_ice_proxy_generic_dtls_cookie_store_initialize(store, 0, 1));
  testrun(!ov_ice_proxy_generic_dtls_cookie_store_initialize(store, 1, 0));
  testrun(!ov_ice_proxy_generic_dtls_cookie_store_initialize(store, 1, 1));

  testrun(ov_ice_proxy_generic_dtls_cookie_store_initialize(store, 1, 2));
  testrun(1 == ov_node_count(store->cookie));
  testrun(1 == strlen(store->cookie->secret));

  for (size_t i = 1; i < 10; i++) {

    testrun(ov_ice_proxy_generic_dtls_cookie_store_initialize(store, i, 10));
    testrun(i == ov_node_count(store->cookie));
    testrun(9 == strlen(store->cookie->secret));
  }

  testrun(NULL == ov_ice_proxy_generic_dtls_cookie_store_free(store));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_ice_proxy_generic_dtls_cookie_generate() {

  ov_ice_proxy_generic_dtls_cookie_store *store =
      ov_ice_proxy_generic_dtls_cookie_store_create();
  testrun(store);

  testrun(ov_ice_proxy_generic_dtls_cookie_store_initialize(store, 10, 10));

  unsigned char cookie[DTLS1_COOKIE_LENGTH] = {0};
  unsigned int len = DTLS1_COOKIE_LENGTH;
  SSL_CTX *ctx = SSL_CTX_new(DTLS_method());
  SSL *ssl = SSL_new(ctx);

  testrun(0 == ov_ice_proxy_generic_dtls_cookie_generate(NULL, NULL, NULL));
  testrun(0 == ov_ice_proxy_generic_dtls_cookie_generate(ssl, cookie, NULL));
  testrun(0 == ov_ice_proxy_generic_dtls_cookie_generate(NULL, cookie, &len));
  testrun(0 == ov_ice_proxy_generic_dtls_cookie_generate(ssl, NULL, &len));

  testrun(1 == ov_ice_proxy_generic_dtls_cookie_generate(ssl, cookie, &len));
  testrun(0 != cookie[0]);
  testrun(9 == strlen((char *)cookie));
  testrun(1 == ov_ice_proxy_generic_dtls_cookie_verify(ssl, cookie,
                                                       strlen((char *)cookie)));

  ov_ice_proxy_generic_dtls_cookie *c = store->cookie;
  bool check = false;
  while (c) {

    if (0 == memcmp(c->secret, cookie, strlen((char *)cookie)))
      check = true;

    c = ov_node_next(c);
  }
  testrun(check);

  SSL_free(ssl);
  SSL_CTX_free(ctx);
  testrun(NULL == ov_ice_proxy_generic_dtls_cookie_store_free(store));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

int test_ov_ice_proxy_generic_dtls_cookie_verify() {

  ov_ice_proxy_generic_dtls_cookie_store *store =
      ov_ice_proxy_generic_dtls_cookie_store_create();
  testrun(store);

  testrun(ov_ice_proxy_generic_dtls_cookie_store_initialize(store, 10, 10));

  unsigned char cookie[DTLS1_COOKIE_LENGTH] = {0};
  unsigned int len = DTLS1_COOKIE_LENGTH;
  SSL_CTX *ctx = SSL_CTX_new(DTLS_method());
  SSL *ssl = SSL_new(ctx);

  testrun(1 == ov_ice_proxy_generic_dtls_cookie_generate(ssl, cookie, &len));
  testrun(0 != cookie[0]);
  testrun(9 == strlen((char *)cookie));
  testrun(1 == ov_ice_proxy_generic_dtls_cookie_verify(ssl, cookie,
                                                       strlen((char *)cookie)));
  testrun(0 == ov_ice_proxy_generic_dtls_cookie_verify(NULL, cookie,
                                                       strlen((char *)cookie)));
  testrun(0 == ov_ice_proxy_generic_dtls_cookie_verify(ssl, NULL,
                                                       strlen((char *)cookie)));
  testrun(0 == ov_ice_proxy_generic_dtls_cookie_verify(ssl, cookie, 0));

  testrun(ov_ice_proxy_generic_dtls_cookie_store_initialize(store, 10, 10));

  testrun(0 == ov_ice_proxy_generic_dtls_cookie_verify(ssl, cookie,
                                                       strlen((char *)cookie)));

  SSL_free(ssl);
  SSL_CTX_free(ctx);
  testrun(NULL == ov_ice_proxy_generic_dtls_cookie_store_free(store));

  return testrun_log_success();
}

/*----------------------------------------------------------------------------*/

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST CLUSTER                                                    #CLUSTER
 *
 *      ------------------------------------------------------------------------
 */

int all_tests() {

  testrun_init();
  testrun_test(test_ov_ice_proxy_generic_dtls_cookie_store_create);
  testrun_test(test_ov_ice_proxy_generic_dtls_cookie_store_free);
  testrun_test(test_ov_ice_proxy_generic_dtls_cookie_store_initialize);

  testrun_test(test_ov_ice_proxy_generic_dtls_cookie_generate);
  testrun_test(test_ov_ice_proxy_generic_dtls_cookie_verify);

  return testrun_counter;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      TEST EXECUTION                                                  #EXEC
 *
 *      ------------------------------------------------------------------------
 */

testrun_run(all_tests);
