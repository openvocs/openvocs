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
        @file           ov_ice_core_dtls_cookie.c
        @author         Markus

        @date           2024-09-11


        ------------------------------------------------------------------------
*/
#include "../include/ov_ice_dtls_cookie.h"

#include <ov_base/ov_data_function.h>
#include <ov_base/ov_node.h>
#include <ov_base/ov_random.h>
#include <ov_base/ov_string.h>
#include <ov_base/ov_thread_lock.h>
#include <ov_base/ov_utf8.h>
#include <ov_encryption/ov_hash.h>

/*----------------------------------------------------------------------------*/

static ov_ice_dtls_cookie_store *global_dtls_ice_cookie_store;

/*----------------------------------------------------------------------------*/

#define OV_ICE_DTLS_COOKIE_STORE_MAGIC_BYTES 0xcc00
#define OV_ICE_DTLS_COOKIE_STORE_THREAD_LOCK_TIMEOUT_USEC 100000

/*----------------------------------------------------------------------------*/

typedef struct ov_ice_dtls_cookie {

  ov_node node;
  char secret[DTLS1_COOKIE_LENGTH];

} ov_ice_dtls_cookie;

/*----------------------------------------------------------------------------*/

struct ov_ice_dtls_cookie_store {

  uint16_t magic_bytes;
  ov_thread_lock lock;

  int cookie_counter;
  ov_ice_dtls_cookie *cookie;
};

/*----------------------------------------------------------------------------*/

ov_ice_dtls_cookie_store *ov_ice_dtls_cookie_store_create() {

  ov_ice_dtls_cookie_store *store = calloc(1, sizeof(ov_ice_dtls_cookie_store));

  if (!store)
    goto error;
  store->magic_bytes = OV_ICE_DTLS_COOKIE_STORE_MAGIC_BYTES;
  global_dtls_ice_cookie_store = store;

  if (!ov_thread_lock_init(&global_dtls_ice_cookie_store->lock,
                           OV_ICE_DTLS_COOKIE_STORE_THREAD_LOCK_TIMEOUT_USEC))
    goto error;

  return store;
error:
  return NULL;
}

/*----------------------------------------------------------------------------*/

ov_ice_dtls_cookie_store *
ov_ice_dtls_cookie_store_free(ov_ice_dtls_cookie_store *self) {

  if (!self)
    goto error;

  /* Delete all cookies */
  ov_ice_dtls_cookie *cookie = NULL;

  while (global_dtls_ice_cookie_store->cookie) {

    cookie = ov_node_pop((void **)&global_dtls_ice_cookie_store->cookie);
    cookie = ov_data_pointer_free(cookie);
  }

  ov_thread_lock_clear(&global_dtls_ice_cookie_store->lock);

  global_dtls_ice_cookie_store = NULL;
  self = ov_data_pointer_free(self);
  return NULL;
error:
  return self;
}

/*----------------------------------------------------------------------------*/

static bool create_cookie(size_t length) {

  ov_ice_dtls_cookie *cookie = NULL;

  if (NULL == global_dtls_ice_cookie_store)
    goto error;

  if (length >= DTLS1_COOKIE_LENGTH)
    length = DTLS1_COOKIE_LENGTH - 1;

  cookie = calloc(1, sizeof(ov_ice_dtls_cookie));
  if (!cookie)
    goto error;

  char *ptr = cookie->secret;
  if (!ov_random_string((char **)&ptr, length, NULL))
    goto error;

  bool result =
      ov_node_push((void **)&global_dtls_ice_cookie_store->cookie, cookie);

  if (!result)
    goto error;

  return true;
error:
  ov_data_pointer_free(cookie);
  return false;
}

/*----------------------------------------------------------------------------*/

bool ov_ice_dtls_cookie_store_initialize(ov_ice_dtls_cookie_store *self,
                                         size_t quantity, size_t length) {

  if (!self || quantity < 1 || length < 2)
    goto error;

  if (NULL == global_dtls_ice_cookie_store)
    goto error;

  /* Delete all cookies */
  ov_ice_dtls_cookie *cookie = NULL;

  if (!ov_thread_lock_try_lock(&global_dtls_ice_cookie_store->lock))
    goto error;

  while (global_dtls_ice_cookie_store->cookie) {

    cookie = ov_node_pop((void **)&global_dtls_ice_cookie_store->cookie);
    cookie = ov_data_pointer_free(cookie);
  }

  for (uint8_t i = 0; i < quantity; i++) {

    if (!create_cookie(length))
      goto error;
  }

  self->cookie_counter = ov_node_count(self->cookie);

  ov_thread_lock_unlock(&global_dtls_ice_cookie_store->lock);

  return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

int ov_ice_dtls_cookie_verify(SSL *ssl, const unsigned char *cookie,
                              unsigned int cookie_len) {

  if (!ssl || !cookie || !cookie_len)
    goto error;

  if (!global_dtls_ice_cookie_store)
    goto error;

  if (!ov_thread_lock_try_lock(&global_dtls_ice_cookie_store->lock))
    goto error;

  ov_ice_dtls_cookie *ice_cookie = global_dtls_ice_cookie_store->cookie;
  while (ice_cookie) {

    if (0 == memcmp(ice_cookie->secret, cookie, cookie_len)) {

      ov_thread_lock_unlock(&global_dtls_ice_cookie_store->lock);
      return 1;
    }

    ice_cookie = ov_node_next(ice_cookie);
  }

  ov_thread_lock_unlock(&global_dtls_ice_cookie_store->lock);

error:
  return 0;
}

/*----------------------------------------------------------------------------*/

int ov_ice_dtls_cookie_generate(SSL *ssl, unsigned char *cookie,
                                unsigned int *cookie_len) {

  if (!ssl || !cookie || !cookie_len)
    goto error;

  if (!global_dtls_ice_cookie_store)
    goto error;

  if (!ov_thread_lock_try_lock(&global_dtls_ice_cookie_store->lock))
    goto error;

  uint64_t number =
      ov_random_range(1, global_dtls_ice_cookie_store->cookie_counter);

  ov_ice_dtls_cookie *selected =
      ov_node_get(global_dtls_ice_cookie_store->cookie, number);

  OV_ASSERT(selected);

  if (!selected)
    goto done;

  memcpy(cookie, selected->secret, strlen((char *)selected->secret));
  *cookie_len = strlen((char *)selected->secret);

done:
  ov_thread_lock_unlock(&global_dtls_ice_cookie_store->lock);

  return 1;
error:
  return 0;
}