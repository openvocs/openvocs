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
        @file           ov_ice_proxy_multiplexing.c
        @author         Markus

        @date           2024-07-26


        ------------------------------------------------------------------------
*/
#include "../include/ov_ice_proxy_multiplexing.h"

#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>

#include <ov_base/ov_utils.h>

#include <openssl/conf.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/ssl.h>

#include <ov_base/ov_buffer.h>
#include <ov_base/ov_config_keys.h>
#include <ov_base/ov_convert.h>
#include <ov_base/ov_dict.h>
#include <ov_base/ov_file.h>
#include <ov_base/ov_id.h>
#include <ov_base/ov_linked_list.h>
#include <ov_base/ov_node.h>
#include <ov_base/ov_random.h>
#include <ov_base/ov_socket.h>
#include <ov_base/ov_string.h>

#include <ov_stun/ov_stun_attributes_rfc5245.h> // RFC ICE
#include <ov_stun/ov_stun_attributes_rfc5389.h> // RFC STUN
#include <ov_stun/ov_stun_attributes_rfc8489.h>
#include <ov_stun/ov_stun_attributes_rfc8656.h>
#include <ov_stun/ov_stun_binding.h>
#include <ov_stun/ov_stun_frame.h>

#include <ov_ice/ov_ice_candidate.h>
#include <ov_ice/ov_ice_string.h>

#include <ov_encryption/ov_hash.h>

#include <srtp2/srtp.h>

#define OV_ICE_UFRAG_MAX 256 // RFC 5245 SDP length
#define OV_ICE_PASS_MAX 256  // RFC 5245 SDP length

#define OV_ICE_UFRAG_MIN 4 // RFC 5245 SDP length
#define OV_ICE_PASS_MIN 22 // RFC 5245 SDP length

// Default DTLS keys and key length
#define OV_ICE_DTLS_FINGERPRINT_MAX 3 * OV_SHA512_SIZE + 10 // SPname/0 HEX

#define OV_ICE_SRTP_PROFILE_MAX 1024

#define OV_ICE_SRTP_KEY_MAX 48  // min 32 key + 14 salt
#define OV_ICE_SRTP_SALT_MAX 14 // 112 bit salt

#define OV_ICE_SSL_BUFFER_SIZE 16000 // max SSL buffer size

/*----------------------------------------------------------------------------*/

static const char *label_extractor_srtp = "EXTRACTOR-dtls_srtp";
static BIO_METHOD *dtls_filter_methods = NULL;

/*----------------------------------------------------------------------------*/

#define OV_ICE_PROXY_MAGIC_BYTES 0x1ce1

/*----------------------------------------------------------------------------*/

#define OV_ICE_PROXY_DTLS_KEYS_QUANTITY_DEFAULT 10
#define OV_ICE_PROXY_DTLS_KEYS_LENGTH_DEFAULT 20
#define OV_ICE_PROXY_DTLS_KEYS_LIFETIME_DEFAULT 300000000
#define OV_ICE_PROXY_DTLS_RECONNECT_DEFAULT 50000 // 50ms
#define OV_ICE_PROXY_DTLS_CONNECTION_MAGIC_BYTE 0x12FA

#define OV_ICE_PROXY_TRANSACTIONS_LIFETIME_DEFAULT 300000000
#define OV_ICE_PROXY_CONNECTIVITY_PACE_USECS 50000
#define OV_ICE_PROXY_SESSION_TIMEOUT_USECS 300000000

#define OV_ICE_PROXY_SSL_ERROR_STRING_BUFFER_SIZE 200

#define IMPL_STUN_ATTR_FRAMES 50

/*----------------------------------------------------------------------------*/

typedef struct ov_ice_proxy {

  ov_ice_proxy_generic public;

  ov_socket_data local;
  ov_ice_candidate candidate;
  ov_ice_candidate nat_ip;

  int socket;

  struct {

    ov_ice_proxy_generic_dtls_cookie_store *cookies;
    SSL_CTX *ctx;
    char fingerprint[ov_ice_proxy_generic_dtls_FINGERPRINT_MAX];

  } dtls;

  struct {

    uint32_t dtls_key_renew;
    uint32_t transactions_invalidate;

  } timer;

  ov_dict *sessions;
  ov_dict *streams;
  ov_dict *remote;
  ov_dict *transactions;

} ov_ice_proxy;

/*----------------------------------------------------------------------------*/

static ov_ice_proxy *as_ice_proxy(const void *data) {

  ov_ice_proxy_generic *generic = ov_ice_proxy_generic_cast(data);
  if (!generic)
    return NULL;

  if (generic->type == OV_ICE_PROXY_MAGIC_BYTES)
    return (ov_ice_proxy *)data;

  return NULL;
}

/*----------------------------------------------------------------------------*/

static bool init_config(ov_ice_proxy_generic_config *config) {

  if (!ov_ptr_valid(config, "Cannot initialize ICE Proxy Config - no config") ||
      !ov_ptr_valid(config->loop,
                    "Cannot initialize ICE Proxy Config - no loop") ||
      !ov_cond_valid(0 != config->external.host[0],
                     "Cannot initialize ICE Proxy Config - no host given") ||
      !ov_cond_valid(0 != config->config.dtls.cert[0],
                     "Cannot initialize ICE Proxy Config - no certificate "
                     "given") ||
      !ov_cond_valid(0 != config->config.dtls.key[0],
                     "Cannot initialize ICE Proxy Config - no key given")) {

    return false;
  }

  if (0 == config->config.limits.transaction_lifetime_usecs)
    config->config.limits.transaction_lifetime_usecs =
        OV_ICE_PROXY_TRANSACTIONS_LIFETIME_DEFAULT;

  if (0 == config->config.timeouts.stun.connectivity_pace_usecs)
    config->config.timeouts.stun.connectivity_pace_usecs =
        OV_ICE_PROXY_CONNECTIVITY_PACE_USECS;

  if (0 == config->config.timeouts.stun.session_timeout_usecs)
    config->config.timeouts.stun.session_timeout_usecs =
        OV_ICE_PROXY_SESSION_TIMEOUT_USECS;

  if (0 == config->config.dtls.dtls.keys.quantity)
    config->config.dtls.dtls.keys.quantity =
        OV_ICE_PROXY_DTLS_KEYS_QUANTITY_DEFAULT;

  if (0 == config->config.dtls.dtls.keys.length)
    config->config.dtls.dtls.keys.length =
        OV_ICE_PROXY_DTLS_KEYS_LENGTH_DEFAULT;

  if (DTLS1_COOKIE_LENGTH < config->config.dtls.dtls.keys.length)
    config->config.dtls.dtls.keys.length = DTLS1_COOKIE_LENGTH;

  if (0 == config->config.dtls.dtls.keys.lifetime_usec)
    config->config.dtls.dtls.keys.lifetime_usec =
        OV_ICE_PROXY_DTLS_KEYS_LIFETIME_DEFAULT;

  if (0 == config->config.dtls.srtp.profile[0])
    strcpy(config->config.dtls.srtp.profile,
           ov_ice_proxy_generic_dtls_SRTP_PROFILES);

  return true;
}

typedef struct Pair Pair;
typedef struct Stream Stream;
typedef struct Session Session;
typedef struct Transaction Transaction;

/*----------------------------------------------------------------------------*/

struct Pair {

  ov_node node;
  Stream *stream;

  ov_ice_pair_state state;
  char transaction_id[13];

  bool nominated;
  uint64_t priority;

  struct {

    ov_ice_proxy_generic_dtls_type type;
    bool handshaked;

    SSL *ssl;
    SSL_CTX *ctx;

    BIO *read;
    BIO *write;

  } dtls;

  struct {

    bool ready;
    char *profile;

    uint32_t key_len;
    uint32_t salt_len;

    struct {

      uint8_t key[OV_ICE_PROXY_SRTP_KEY_MAX];
      uint8_t salt[OV_ICE_PROXY_SRTP_SALT_MAX];

    } server;

    struct {

      uint8_t key[OV_ICE_PROXY_SRTP_KEY_MAX];
      uint8_t salt[OV_ICE_PROXY_SRTP_SALT_MAX];

    } client;

  } srtp;

  struct {

    ov_ice_candidate_type type;
    uint64_t priority;

  } local;

  struct {

    uint64_t priority;
    ov_socket_data socket;

  } remote;

  uint64_t success_count;
  uint64_t progress_count;
};

/*----------------------------------------------------------------------------*/

struct Stream {

  ov_node node;

  ov_id uuid;
  int index;

  Session *session;

  uint16_t format;

  ov_ice_proxy_generic_state state;

  struct {

    ov_ice_proxy_generic_state stun;
    ov_ice_proxy_generic_state dtls;
    ov_ice_proxy_generic_state srtp;

  } state_protocol;

  struct {

    uint32_t ssrc;
    char pass[OV_ICE_PASS_MAX];

    srtp_policy_t policy;
    uint8_t key[OV_ICE_PROXY_SRTP_KEY_MAX];

  } local;

  struct {

    uint32_t ssrc;
    bool gathered;

    char user[OV_ICE_UFRAG_MAX];
    char pass[OV_ICE_PASS_MAX];

    char fingerprint[OV_ICE_DTLS_FINGERPRINT_MAX];

    srtp_policy_t policy;
    uint8_t key[OV_ICE_PROXY_SRTP_KEY_MAX];

  } remote;

  struct {

    uint32_t nominate;

  } timer;

  ov_list *valid;
  ov_list *trigger;

  Pair *pairs;
  Pair *selected;
};

/*----------------------------------------------------------------------------*/

struct Session {

  ov_id uuid;

  ov_ice_proxy_generic_state state;

  ov_ice_proxy *proxy;

  bool controlling;

  uint64_t tiebreaker;

  struct {

    srtp_t session;

  } srtp;

  struct {

    uint32_t session_timeout;
    uint32_t connectivity;

  } timer;

  Stream *streams;
};

/*----------------------------------------------------------------------------*/

struct Transaction {

  uint64_t created;
  Pair *pair;
};

/*
 *      ------------------------------------------------------------------------
 *
 *      #transaction FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

static bool transaction_create(ov_ice_proxy *self, Pair *pair) {

  Transaction *transaction = NULL;
  if (!self || !pair)
    goto error;

  transaction = calloc(1, sizeof(Transaction));
  if (!transaction)
    goto error;

  transaction->created = ov_time_get_current_time_usecs();
  transaction->pair = pair;

  ov_stun_frame_generate_transaction_id((uint8_t *)pair->transaction_id);

  if (!ov_dict_set(self->transactions, ov_string_dup(pair->transaction_id),
                   transaction, NULL))
    goto error;

  return true;
error:
  transaction = ov_data_pointer_free(transaction);
  return false;
}

/*----------------------------------------------------------------------------*/

static Pair *transaction_unset(ov_ice_proxy *self, const char *transaction_id) {

  char key[13] = {0};

  if (!self || !transaction_id)
    goto error;

  strncpy(key, (char *)transaction_id, 12);

  Transaction *t = ov_dict_remove(self->transactions, key);
  if (!t)
    goto error;

  Pair *p = t->pair;
  t = ov_data_pointer_free(t);
  return p;

error:
  return NULL;
}

/*----------------------------------------------------------------------------*/

static void *transaction_free(void *self) {

  if (!self)
    return NULL;
  self = ov_data_pointer_free(self);
  return self;
}

/*----------------------------------------------------------------------------*/

struct container_timer {

  uint64_t now;
  uint64_t max;
  ov_list *list;
};

/*----------------------------------------------------------------------------*/

static bool create_timeout_key_list(const void *key, void *value, void *data) {

  if (!key)
    return true;

  if (!value || !data)
    goto error;

  struct container_timer *container = (struct container_timer *)data;
  Transaction *tdata = (Transaction *)value;

  uint64_t lifetime = tdata->created + container->max;

  if (container->now < lifetime)
    return true;

  // add to delete list
  return ov_list_push(container->list, (void *)key);

error:
  return false;
}

/*----------------------------------------------------------------------------*/

static bool del_key_list(void *key, void *data) {

  if (!key || !data)
    goto error;

  ov_dict *dict = ov_dict_cast(data);
  OV_ASSERT(dict);
  return ov_dict_del(dict, key);
error:
  return false;
}

/*----------------------------------------------------------------------------*/

static bool invalidate_transactions(uint32_t timer, void *self) {

  UNUSED(timer);
  ov_ice_proxy *proxy = as_ice_proxy(self);
  if (!proxy)
    goto error;
  proxy->timer.transactions_invalidate = OV_TIMER_INVALID;

  ov_list *list = ov_list_create((ov_list_config){0});

  struct container_timer container = {

      .now = ov_time_get_current_time_usecs(),
      .max = proxy->public.config.config.limits.transaction_lifetime_usecs,
      .list = list,
  };

  if (!ov_dict_for_each(proxy->transactions, &container,
                        create_timeout_key_list))
    goto error;

  if (!ov_list_for_each(list, proxy->transactions, del_key_list))
    goto error;

  list = ov_list_free(list);

  proxy->timer.transactions_invalidate = ov_event_loop_timer_set(
      proxy->public.config.loop,
      proxy->public.config.config.limits.transaction_lifetime_usecs, proxy,
      invalidate_transactions);

  return true;
error:
  return false;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      #pair FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

/*----------------------------------------------------------------------------*/

static bool register_pair(Pair *pair) {

  if (!pair)
    goto error;
  if (!pair->stream)
    goto error;
  if (0 == pair->remote.socket.host[0])
    goto error;

  char *key = calloc(1, OV_HOST_NAME_MAX + 20);
  snprintf(key, OV_HOST_NAME_MAX + 20, "%s:%i", pair->remote.socket.host,
           pair->remote.socket.port);

  ov_dict_set(pair->stream->session->proxy->remote, key, pair->stream, NULL);
  return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

static bool unregister_pair(Pair *pair) {

  char key[OV_HOST_NAME_MAX + 20] = {0};

  if (!pair)
    goto error;
  if (!pair->stream)
    goto error;
  if (0 == pair->remote.socket.host[0])
    goto error;

  snprintf(key, OV_HOST_NAME_MAX + 20, "%s:%i", pair->remote.socket.host,
           pair->remote.socket.port);

  ov_dict_del(pair->stream->session->proxy->remote, key);
  return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

static void *pair_free(void *self) {

  if (!self)
    return NULL;
  Pair *pair = (Pair *)self;

  if (!pair->stream)
    goto done;

  if (0 != pair->transaction_id[0])
    transaction_unset(pair->stream->session->proxy, pair->transaction_id);

  unregister_pair(pair);

  pair->srtp.profile = ov_data_pointer_free(pair->srtp.profile);

  if (pair->dtls.ssl) {
    SSL_free(pair->dtls.ssl);
    pair->dtls.ssl = NULL;
  }

  if (pair->dtls.ctx) {
    SSL_CTX_free(pair->dtls.ctx);
    pair->dtls.ctx = NULL;
  }

  ov_node_unplug((void **)&pair->stream->pairs, pair);
  if (pair == pair->stream->selected)
    pair->stream->selected = NULL;

done:
  pair = ov_data_pointer_free(pair);
  return pair;
}

/*----------------------------------------------------------------------------*/

static Pair *pair_create(Stream *stream) {

  Pair *pair = NULL;
  if (!stream)
    goto error;

  pair = calloc(1, sizeof(Pair));
  if (!pair)
    goto error;

  pair->stream = stream;
  if (!ov_node_push((void **)&stream->pairs, pair))
    goto error;

  return pair;
error:
  pair_free(pair);
  return NULL;
}

/*----------------------------------------------------------------------------*/

static ssize_t pair_send(Pair *pair, const uint8_t *buffer, size_t size) {

  if (!pair || !buffer || (0 == size))
    goto error;

  OV_ASSERT(pair->stream);
  OV_ASSERT(pair->stream->session);
  Stream *stream = pair->stream;
  Session *session = stream->session;
  ov_ice_proxy *proxy = session->proxy;
  OV_ASSERT(proxy);

  if (-1 == proxy->socket)
    goto error;

  struct sockaddr local_address = {0};
  socklen_t len = sizeof(struct sockaddr);
  getsockname(proxy->socket, &local_address, &len);

  struct sockaddr_storage dest = {0};

  if (!ov_socket_fill_sockaddr_storage(&dest, local_address.sa_family,
                                       pair->remote.socket.host,
                                       pair->remote.socket.port))
    goto error;

  len = sizeof(struct sockaddr_in);
  if (dest.ss_family == AF_INET6)
    len = sizeof(struct sockaddr_in6);

  ssize_t out =
      sendto(proxy->socket, buffer, size, 0, (struct sockaddr *)&dest, len);
  /*
      ov_log_debug("pair %p send %zi bytes to %s:%i",
          pair, out, pair->remote.socket.host, pair->remote.socket.port);
  */
  return out;
error:
  return -1;
}

/*----------------------------------------------------------------------------*/

static uint32_t type_preference(ov_ice_candidate_type type) {

  uint32_t preference = 0;

  // 0 (lowest) - 126 (MAX)

  switch (type) {

  case OV_ICE_HOST:

    preference = 126;
    break;

  case OV_ICE_SERVER_REFLEXIVE:

    preference = 100;
    break;

  case OV_ICE_PEER_REFLEXIVE:

    // MUST be higher than ICE_SERVER_REFLEXIVE
    preference = 110;
    break;

  case OV_ICE_RELAYED:

    preference = 0;
    break;

  default:
    preference = 0;
    break;
  }

  return preference;
}

/*----------------------------------------------------------------------------*/

static bool pair_calculate_local_priority(Pair *pair) {

  if (!pair)
    return false;

  /*
   *      This is an implementation of RFC 8445 5.1.2.1,
   *      using
   *
   *      RFC 8421 Guidelines for Multihomed and IPv4/IPv6 Dual-Stack
   *               Interactive Connectivity Establishment (ICE)
   *
   */

  pair->local.priority = (1 << 24) * type_preference(pair->local.type) +
                         (1 << 8) * 65535 + (1) * (256 - 1);

  return true;
}

/*----------------------------------------------------------------------------*/

static bool pair_calculate_priority(Pair *pair) {

  if (!pair)
    return false;

  OV_ASSERT(pair->stream);
  OV_ASSERT(pair->stream->session);

  Session *session = pair->stream->session;

  uint32_t g = 0;
  uint32_t d = 0;
  uint32_t min = 0;
  uint32_t max = 0;
  uint32_t add = 0;

  if (!session->controlling) {

    g = pair->remote.priority;
    d = pair->local.priority;

  } else {

    d = pair->remote.priority;
    g = pair->local.priority;
  }

  min = d;
  max = d;
  add = 0;

  if (g > d) {

    add = 1;
    max = g;
    min = d;

  } else {

    max = d;
    min = g;
  }

  pair->priority = (0x100000000 * min) + (2 * max) + add;
  return true;
}

/*----------------------------------------------------------------------------*/

static Pair *get_pair_by_remote(Stream *stream, const ov_socket_data *remote) {

  if (!stream || !remote)
    goto error;

  Pair *pair = stream->pairs;
  while (pair) {

    if ((pair->remote.socket.port == remote->port) &&
        (0 == ov_string_compare(pair->remote.socket.host, remote->host))) {

      break;
    }

    pair = ov_node_next(pair);
  }

  return pair;
error:
  return NULL;
}

/*----------------------------------------------------------------------------*/

static Pair *get_pair_by_candidate(Stream *stream, const ov_ice_candidate *c) {

  if (!stream || !c)
    goto error;

  Pair *pair = stream->pairs;
  while (pair) {

    if ((pair->remote.socket.port == c->port) &&
        (0 == ov_string_compare(pair->remote.socket.host, c->addr))) {

      break;
    }

    pair = ov_node_next(pair);
  }

  return pair;
error:
  return NULL;
}

/*----------------------------------------------------------------------------*/

static bool send_stun_binding_request(ov_ice_proxy *self, Pair *pair) {

  size_t size = OV_UDP_PAYLOAD_OCTETS;
  uint8_t buffer[size];
  memset(buffer, 0, size);

  if (!self || !pair)
    goto error;

  OV_ASSERT(pair->stream);
  OV_ASSERT(pair->stream->session);
  Stream *stream = pair->stream;
  Session *session = stream->session;

  if (!transaction_create(self, pair))
    goto error;

  bool usecandidate = false;

  if (session->controlling) {
    usecandidate = pair->nominated;
  }

  const char *user = stream->uuid;
  const char *peer_user = stream->remote.user;
  const char *peer_pass = stream->remote.pass;

  size_t required = 20; // header
  size_t len = 0;

  char username[513] = {0};

  if (!snprintf(username, 513, "%s:%s", peer_user, user))
    goto error;

  required += ov_stun_message_integrity_encoding_length();
  required += ov_stun_fingerprint_encoding_length();
  required += ov_stun_ice_priority_encoding_length();

  len = ov_stun_username_encoding_length((uint8_t *)username, strlen(username));

  if (0 == len)
    goto error;

  required += len;

  if (usecandidate)
    required += ov_stun_ice_use_candidate_encoding_length();

  if (session->controlling) {
    required += ov_stun_ice_controlling_encoding_length();
  } else {
    required += ov_stun_ice_controlled_encoding_length();
  }

  /*
   *      Prepare the buffer
   */

  if (required > OV_UDP_PAYLOAD_OCTETS)
    goto error;

  size = required;
  uint8_t *ptr = buffer;
  uint8_t *start = buffer;

  /*
   *      Write content to the buffer.
   */

  if (!ov_stun_frame_set_request(ptr, size))
    goto error;

  if (!ov_stun_frame_set_method(ptr, size, STUN_BINDING))
    goto error;

  if (!ov_stun_frame_set_magic_cookie(ptr, size))
    goto error;

  if (!ov_stun_frame_set_length(ptr, size, size - 20))
    goto error;

  if (!ov_stun_frame_set_transaction_id(ptr, size,
                                        (uint8_t *)pair->transaction_id))
    goto error;

  ptr = buffer + 20;

  if (session->controlling) {

    if (!ov_stun_ice_controlling_encode(ptr, size - (ptr - start), &ptr,
                                        session->tiebreaker))
      goto error;

  } else {

    if (!ov_stun_ice_controlled_encode(ptr, size - (ptr - start), &ptr,
                                       session->tiebreaker))
      goto error;
  }

  if (usecandidate) {

    if (!ov_stun_ice_use_candidate_encode(ptr, size - (ptr - start), &ptr))
      goto error;
  }

  if (!ov_stun_ice_priority_encode(ptr, size - (ptr - start), &ptr,
                                   pair->priority))
    goto error;

  if (!ov_stun_username_encode(ptr, size - (ptr - start), &ptr,
                               (uint8_t *)username, strlen(username)))
    goto error;

  if (!ov_stun_add_message_integrity(start, size, ptr, &ptr,
                                     (uint8_t *)peer_pass, strlen(peer_pass)))
    goto error;

  if (!ov_stun_add_fingerprint(start, size, ptr, &ptr))
    goto error;

  ssize_t out = pair_send(pair, start, ptr - start);

  if (out <= 0) {

    transaction_unset(self, pair->transaction_id);
    goto error;
  }

  if (pair->state != OV_ICE_PAIR_SUCCESS)
    pair->state = OV_ICE_PAIR_PROGRESS;

  return true;
error:
  if (pair)
    pair->state = OV_ICE_PAIR_FAILED;
  return false;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      #state FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

static void session_state_change(Session *session) {

  if (!session)
    goto error;

  Stream *stream = session->streams;

  bool completed = true;
  bool failed = false;

  while (stream) {

    switch (stream->state) {

    case OV_ICE_PROXY_GENERIC_RUNNING:
      completed = false;
      break;

    case OV_ICE_PROXY_GENERIC_FAILED:
      failed = true;
      break;

    default:
      break;
    }

    stream = ov_node_next(stream);
  }

  if (failed) {

    session->state = OV_ICE_PROXY_GENERIC_FAILED;
    ov_log_debug("session %s - failed", session->uuid);

  } else if (completed) {

    session->state = OV_ICE_PROXY_GENERIC_COMPLETED;
    ov_log_debug("session %s - completed", session->uuid);

    if (session->proxy->public.config.callbacks.session.state)
      session->proxy->public.config.callbacks.session.state(
          session->proxy->public.config.callbacks.userdata, session->uuid,
          session->state);
  }

error:
  return;
}

/*----------------------------------------------------------------------------*/

static bool get_profile(Pair *pair);

/*----------------------------------------------------------------------------*/

static void stream_state_change(Stream *stream) {

  if (!stream)
    goto error;

  if (stream->selected) {

    if (!stream->selected->srtp.profile)
      get_profile(stream->selected);
  }

  if ((stream->state_protocol.stun == OV_ICE_PROXY_GENERIC_COMPLETED) &&
      (stream->state_protocol.dtls == OV_ICE_PROXY_GENERIC_COMPLETED) &&
      (stream->state_protocol.srtp == OV_ICE_PROXY_GENERIC_COMPLETED)) {

    stream->state = OV_ICE_PROXY_GENERIC_COMPLETED;

    ov_log_debug("Session %s stream %i|%s - completed", stream->session->uuid,
                 stream->index, stream->uuid);
  }

  if ((stream->state_protocol.stun == OV_ICE_PROXY_GENERIC_FAILED) ||
      (stream->state_protocol.dtls == OV_ICE_PROXY_GENERIC_FAILED) ||
      (stream->state_protocol.srtp == OV_ICE_PROXY_GENERIC_FAILED)) {

    stream->state = OV_ICE_PROXY_GENERIC_FAILED;

    ov_log_debug("Session %s stream %i|%s - failed", stream->session->uuid,
                 stream->index, stream->uuid);
  }

  session_state_change(stream->session);
error:
  return;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      #stream FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

/*----------------------------------------------------------------------------*/

static Stream *stream_free(Stream *self) {

  if (!self)
    return NULL;

  self->valid = ov_list_free(self->valid);
  self->trigger = ov_list_free(self->trigger);

  if (OV_TIMER_INVALID != self->timer.nominate) {

    ov_event_loop_timer_unset(self->session->proxy->public.config.loop,
                              self->timer.nominate, NULL);

    self->timer.nominate = OV_TIMER_INVALID;
  }

  Pair *pair = NULL;
  while (self->pairs) {

    pair = (Pair *)ov_node_pop((void **)&self->pairs);
    pair = pair_free(pair);
  }

  self = ov_data_pointer_free(self);
  return self;
}

/*----------------------------------------------------------------------------*/

static uint32_t create_ssrc() {

  uint32_t ssrc = ov_random_uint32();

  while (ssrc <= OV_MAX_ANALOG_SSRC) {

    ssrc = ov_random_uint32();
  }

  return ssrc;
}

/*----------------------------------------------------------------------------*/

static Stream *stream_create(Session *session, int i) {

  Stream *stream = NULL;

  if (!session)
    goto error;

  stream = calloc(1, sizeof(Stream));
  if (!stream)
    goto error;

  stream->session = session;
  stream->index = i;
  stream->local.ssrc = create_ssrc();
  stream->state = OV_ICE_PROXY_GENERIC_RUNNING;
  stream->state_protocol.stun = OV_ICE_PROXY_GENERIC_RUNNING;
  stream->state_protocol.dtls = OV_ICE_PROXY_GENERIC_INIT;
  stream->state_protocol.srtp = OV_ICE_PROXY_GENERIC_INIT;

  stream->valid = ov_linked_list_create((ov_list_config){0});
  stream->trigger = ov_linked_list_create((ov_list_config){0});
  if (!stream->valid || !stream->trigger)
    goto error;

  ov_id_fill_with_uuid(stream->uuid);

  if (!ov_ice_string_fill_random(stream->local.pass, OV_ICE_PASS_MIN))
    goto error;

  if (!ov_node_push((void **)&session->streams, stream))
    goto error;

  if (!ov_dict_set(session->proxy->streams, ov_string_dup(stream->uuid), stream,
                   NULL))
    goto error;

  return stream;
error:
  stream_free(stream);
  return NULL;
}

/*----------------------------------------------------------------------------*/

static Stream *get_stream_by_remote(ov_ice_proxy *self,
                                    const ov_socket_data *remote) {

  char key[OV_HOST_NAME_MAX + 20] = {0};
  if (!self || !remote)
    goto error;

  snprintf(key, OV_HOST_NAME_MAX + 20, "%s:%i", remote->host, remote->port);
  return ov_dict_get(self->remote, key);
error:
  return NULL;
}

/*----------------------------------------------------------------------------*/

static bool add_stream_by_remote(ov_ice_proxy *self, Stream *stream,
                                 const ov_socket_data *remote) {

  char key[OV_HOST_NAME_MAX + 20] = {0};
  if (!self || !remote || !stream)
    goto error;

  snprintf(key, OV_HOST_NAME_MAX + 20, "%s:%i", remote->host, remote->port);
  return ov_dict_set(self->remote, strdup(key), stream, NULL);
error:
  return NULL;
}

/*----------------------------------------------------------------------------*/

static Stream *get_stream_by_ufrag(ov_ice_proxy *self, const char *ufrag,
                                   size_t ufrag_in_len) {

  char key[ufrag_in_len + 1];
  if (!self || !ufrag)
    goto error;

  snprintf(key, ufrag_in_len + 1, "%s", ufrag);
  return ov_dict_get(self->streams, key);
error:
  return NULL;
}

/*----------------------------------------------------------------------------*/

static Pair *stream_add_peer_reflexive_pair(Stream *stream,
                                            const ov_socket_data *remote,
                                            uint64_t priority) {

  Pair *pair = NULL;
  if (!stream || !remote)
    goto error;

  pair = pair_create(stream);
  if (!pair)
    goto error;

  pair->remote.socket = *remote;
  pair->remote.priority = priority;
  pair->local.type = OV_ICE_PEER_REFLEXIVE;
  pair->state = OV_ICE_PAIR_SUCCESS;

  register_pair(pair);

  pair_calculate_local_priority(pair);
  pair_calculate_priority(pair);

  ov_log_debug("%s added peer reflexive pair from %s:%i", stream->uuid,
               remote->host, remote->port);

  return pair;
error:
  return NULL;
}

/*----------------------------------------------------------------------------*/

static bool stream_order(Stream *stream) {

  if (!stream)
    goto error;

  Pair *pair = NULL;
  Pair *next = stream->pairs;
  Pair *walk = NULL;

  while (next) {

    pair = next;
    next = ov_node_next(next);

    OV_ASSERT(pair);

    /*
     *      Inline order to position based on
     *      priority.
     */

    walk = stream->pairs;
    while (walk != pair) {

      if (walk->priority > pair->priority) {
        walk = ov_node_next(walk);
        continue;
      }

      if (walk == pair)
        break;

      if (!ov_node_insert_before((void **)&stream->pairs, pair, walk))
        goto error;

      break;
    }
  }

  return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

static bool pairs_are_redundant(Pair *pair, Pair *check) {

  OV_ASSERT(pair);
  OV_ASSERT(check);

  if (!pair || !check)
    return false;

  if (pair == check)
    return false;

  /* local is always the same */

  if (pair->remote.socket.port != check->remote.socket.port)
    return false;

  if (0 != strncmp(pair->remote.socket.host, check->remote.socket.host,
                   OV_HOST_NAME_MAX))
    return false;

  return true;
}

/*----------------------------------------------------------------------------*/

static bool stream_prune(Stream *stream) {

  if (!stream)
    goto error;

  Pair *pair = stream->pairs;
  Pair *next = NULL;
  Pair *drop = NULL;

  while (pair) {

    if (pair == stream->selected) {

      pair = ov_node_next(pair);
      continue;
    }

    switch (pair->state) {

    case OV_ICE_PAIR_FROZEN:
    case OV_ICE_PAIR_WAITING:
      break;

    default:
      pair = ov_node_next(pair);
      continue;
    }

    /*
     *      We check redundancy for the rest of the
     *      pairs. Pairs are expected to be priority
     *      ordered, so we remove the lower priority.
     */

    next = ov_node_next(pair);

    while (next) {

      drop = next;
      next = ov_node_next(next);

      if (drop == stream->selected)
        continue;

      if (pairs_are_redundant(pair, drop)) {

        ov_node_unplug((void **)&stream->pairs, drop);
        drop = pair_free(drop);
      }
    }

    pair = ov_node_next(pair);
  }

  return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

static bool send_nominated_update(uint32_t timer, void *s) {

  UNUSED(timer);

  Stream *stream = (Stream *)s;
  stream->timer.nominate = OV_TIMER_INVALID;

  Pair *pair = stream->pairs;
  while (pair) {

    if (pair->nominated)
      break;
    pair = ov_node_next(pair);
  }

  if (!pair)
    goto error;

  send_stun_binding_request(stream->session->proxy, pair);

  stream->timer.nominate = ov_event_loop_timer_set(
      stream->session->proxy->public.config.loop,
      OV_ICE_PROXY_CONNECTIVITY_PACE_USECS, stream, send_nominated_update);

  return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

static bool stream_update_stun(Stream *stream) {

  if (!stream)
    goto error;

  if (stream->state_protocol.stun == OV_ICE_PROXY_GENERIC_COMPLETED)
    return true;

  Session *session = stream->session;

  Pair *pair = NULL;
  Pair *valid = NULL;
  Pair *trash = NULL;
  Pair *selected = NULL;

  if (stream->remote.gathered) {

    bool all_pairs_failed = true;

    pair = stream->pairs;
    while (pair) {

      if (pair->state != OV_ICE_PAIR_FAILED) {
        all_pairs_failed = false;
        break;
      }

      pair = ov_node_next(pair);
    }

    if (all_pairs_failed) {
      stream->state_protocol.stun = OV_ICE_PROXY_GENERIC_FAILED;
      stream->state = OV_ICE_PROXY_GENERIC_FAILED;
      return true;
    }
  }

  void *next = stream->valid->iter(stream->valid);
  while (next) {

    next = stream->valid->next(stream->valid, next, (void **)&valid);

    if (valid->nominated) {

      pair = stream->pairs;
      while (pair) {

        trash = NULL;

        switch (pair->state) {

        case OV_ICE_PAIR_FROZEN:
        case OV_ICE_PAIR_WAITING:
          trash = pair;
          break;

        default:
          break;
        }

        pair = ov_node_next(pair);
        trash = pair_free(trash);
      }

    } else if (session->controlling) {

      if (valid->nominated) {
        selected = NULL;
        break;
      }

      if (valid->success_count >= 3) {

        if (!selected) {

          selected = valid;

        } else if (selected->priority < valid->priority) {

          selected = valid;
        }
      }
    }
  }

  if (selected) {

    if (0 == ov_list_get_pos(stream->trigger, selected))
      if (!ov_list_queue_push(stream->trigger, selected))
        goto error;

    selected->nominated = true;

    stream->timer.nominate = ov_event_loop_timer_set(
        stream->session->proxy->public.config.loop,
        OV_ICE_PROXY_CONNECTIVITY_PACE_USECS, stream, send_nominated_update);
  }

  return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

static bool stream_callback_selected_ready(Stream *stream) {

  int r = srtp_err_status_ok;

  if (!stream)
    goto error;
  if (!stream->selected)
    goto error;
  if (!stream->selected->srtp.profile)
    goto error;

  stream->state_protocol.srtp = OV_ICE_PROXY_GENERIC_RUNNING;

  srtp_t srtp_session = stream->session->srtp.session;
  if (!srtp_session)
    goto error;

  /* Step 1 - prepare policy */

  srtp_crypto_policy_set_rtp_default(&stream->local.policy.rtp);
  srtp_crypto_policy_set_rtcp_default(&stream->local.policy.rtcp);

  srtp_crypto_policy_set_rtp_default(&stream->remote.policy.rtp);
  srtp_crypto_policy_set_rtcp_default(&stream->remote.policy.rtcp);

  if (0 == ov_string_compare("SRTP_AES128_CM_SHA1_80",
                             stream->selected->srtp.profile)) {

    srtp_crypto_policy_set_aes_cm_128_hmac_sha1_80(&stream->local.policy.rtp);
    srtp_crypto_policy_set_aes_cm_128_hmac_sha1_80(&stream->local.policy.rtcp);

    srtp_crypto_policy_set_aes_cm_128_hmac_sha1_80(&stream->remote.policy.rtp);
    srtp_crypto_policy_set_aes_cm_128_hmac_sha1_80(&stream->remote.policy.rtcp);

  } else if (0 == ov_string_compare("SRTP_AES128_CM_SHA1_32",
                                    stream->selected->srtp.profile)) {

    srtp_crypto_policy_set_aes_cm_128_hmac_sha1_32(&stream->local.policy.rtp);
    srtp_crypto_policy_set_aes_cm_128_hmac_sha1_32(&stream->local.policy.rtcp);

    srtp_crypto_policy_set_aes_cm_128_hmac_sha1_80(&stream->remote.policy.rtp);
    srtp_crypto_policy_set_aes_cm_128_hmac_sha1_80(&stream->remote.policy.rtcp);

  } else {

    goto error;
  }

  /* Step 2 - prepare keys */

  memset(stream->local.key, 0, OV_ICE_SRTP_KEY_MAX);
  memset(stream->remote.key, 0, OV_ICE_SRTP_KEY_MAX);

  switch (stream->selected->dtls.type) {

  case OV_ICE_PROXY_GENERIC_DTLS_ACTIVE:

    /* Local policy is client policy,
     * incoming streams will use the server key. */

    memcpy(stream->local.key, stream->selected->srtp.server.key,
           stream->selected->srtp.key_len);

    memcpy(stream->remote.key, stream->selected->srtp.client.key,
           stream->selected->srtp.key_len);

    srtp_append_salt_to_key(stream->local.key, stream->selected->srtp.key_len,
                            stream->selected->srtp.server.salt,
                            stream->selected->srtp.salt_len);

    srtp_append_salt_to_key(stream->remote.key, stream->selected->srtp.key_len,
                            stream->selected->srtp.client.salt,
                            stream->selected->srtp.salt_len);

    break;

  case OV_ICE_PROXY_GENERIC_DTLS_PASSIVE:

    /* Local policy is server policy,
     * incoming streams will use the client key. */

    memcpy(stream->local.key, stream->selected->srtp.client.key,
           stream->selected->srtp.key_len);

    memcpy(stream->remote.key, stream->selected->srtp.server.key,
           stream->selected->srtp.key_len);

    srtp_append_salt_to_key(stream->local.key, stream->selected->srtp.key_len,
                            stream->selected->srtp.client.salt,
                            stream->selected->srtp.salt_len);

    srtp_append_salt_to_key(stream->remote.key, stream->selected->srtp.key_len,
                            stream->selected->srtp.server.salt,
                            stream->selected->srtp.salt_len);

    break;

    break;
  default:
    goto error;
  }

  /* Step 3 prepare streams */

  stream->local.policy.ssrc.type = ssrc_specific;
  stream->local.policy.ssrc.value = stream->remote.ssrc;
  stream->local.policy.key = stream->local.key;
  stream->local.policy.next = NULL;

  stream->remote.policy.ssrc.type = ssrc_specific;
  stream->remote.policy.ssrc.value = stream->local.ssrc;
  stream->remote.policy.key = stream->remote.key;
  stream->remote.policy.next = NULL;

  r = srtp_add_stream(srtp_session, &stream->remote.policy);

  switch (r) {

  case srtp_err_status_ok:
    break;

  default:
    goto error;
    break;
  }

  r = srtp_add_stream(srtp_session, &stream->local.policy);

  switch (r) {

  case srtp_err_status_ok:
    break;

  default:
    goto error;
    break;
  }

  ov_log_debug("Session %s stream %i|%s - completed SRTP",
               stream->session->uuid, stream->index, stream->uuid);

  stream->state_protocol.srtp = OV_ICE_PROXY_GENERIC_COMPLETED;
  stream_state_change(stream);

  return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

static bool stream_complete_dtls(Stream *stream, Pair *pair) {

  if (!stream || !pair)
    goto error;

  stream->state_protocol.dtls = OV_ICE_PROXY_GENERIC_COMPLETED;

  ov_log_debug("Session %s stream %i|%s - completed DTLS",
               stream->session->uuid, stream->index, stream->uuid);

  if (!stream->selected) {

    // STUN not done
    ov_log_debug("STUN and DTLS intermixed.");

    return true;
  }

  if (pair != stream->selected) {

    ov_log_error("STUN and DTLS selected different pairs.");

    stream->state_protocol.dtls = OV_ICE_PROXY_GENERIC_FAILED;
    stream->state_protocol.srtp = OV_ICE_PROXY_GENERIC_FAILED;

    return true;
  }

  stream_callback_selected_ready(stream);

  return true;
error:
  return false;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      #session FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

static bool change_tie_breaker(Session *session, uint64_t min, uint64_t max) {

  if (!session)
    goto error;

  uint64_t new = session->tiebreaker;

  while (new == session->tiebreaker) {

    new = ov_random_range(min, max);
  }

  session->tiebreaker = new;

  return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

static void *session_free(void *data) {

  if (!data)
    return NULL;
  Session *session = (Session *)data;

  if (OV_TIMER_INVALID != session->timer.session_timeout) {

    ov_event_loop_timer_unset(session->proxy->public.config.loop,
                              session->timer.session_timeout, NULL);

    session->timer.session_timeout = OV_TIMER_INVALID;
  }

  if (OV_TIMER_INVALID != session->timer.connectivity) {

    ov_event_loop_timer_unset(session->proxy->public.config.loop,
                              session->timer.connectivity, NULL);

    session->timer.connectivity = OV_TIMER_INVALID;
  }

  if (session->proxy->public.config.callbacks.session.drop)
    session->proxy->public.config.callbacks.session.drop(
        session->proxy->public.config.callbacks.userdata, session->uuid);

  if (session->srtp.session) {

    if (srtp_err_status_ok != srtp_dealloc(session->srtp.session))
      ov_log_error("ICE %s failed to deallocate SRTP session", session->uuid);

    session->srtp.session = NULL;
  }

  Stream *stream = NULL;
  while (session->streams) {

    stream = (Stream *)ov_node_pop((void **)&session->streams);
    stream = stream_free(stream);
  }

  session = ov_data_pointer_free(session);
  return session;
}

/*----------------------------------------------------------------------------*/

static bool timeout_session(uint32_t id, void *s) {

  UNUSED(id);

  Session *session = (Session *)s;
  session->timer.session_timeout = OV_TIMER_INVALID;

  OV_ASSERT(session->proxy);

  ov_dict_del(session->proxy->sessions, session->uuid);
  return true;
}

/*----------------------------------------------------------------------------*/

static Session *session_create(ov_ice_proxy *self) {

  Session *session = NULL;
  if (!self)
    goto error;

  session = calloc(1, sizeof(Session));
  if (!session)
    goto error;

  session->proxy = self;
  session->state = OV_ICE_PROXY_GENERIC_RUNNING;
  session->controlling = true;

  ov_id_fill_with_uuid(session->uuid);
  change_tie_breaker(session, UINT32_MAX, UINT64_MAX);

  srtp_err_status_t r = srtp_create(&session->srtp.session, NULL);

  switch (r) {

  case srtp_err_status_ok:
    break;

  default:
    ov_log_error("ICE session %s srtp failed %i", session->uuid, r);
    goto error;
  }

  if (!ov_dict_set(self->sessions, ov_string_dup(session->uuid), session, NULL))
    goto error;

  session->timer.session_timeout = ov_event_loop_timer_set(
      self->public.config.loop,
      self->public.config.config.timeouts.stun.session_timeout_usecs, session,
      timeout_session);

  return session;
error:
  session_free(session);
  return NULL;
}

/*----------------------------------------------------------------------------*/

static bool session_change_role(Session *self, uint64_t remote_tiebreaker) {

  if (!self)
    goto error;

  if (self->controlling) {

    self->controlling = false;

    if (0 != remote_tiebreaker) {

      /*
       *      We SHALL be controlled,
       *      so our new tiebreaker MUST be
       *      smaller as the received tiebreaker.
       */

      while (self->tiebreaker >= remote_tiebreaker) {

        if (!change_tie_breaker(self, 0, remote_tiebreaker))
          goto error;
      }

    } else {

      self->tiebreaker = 0;
    }

  } else {

    self->controlling = true;

    if (0 != remote_tiebreaker) {

      /*
       *      We SHALL controll,
       *      so our new tiebreaker MUST be
       *      greater as the received tiebreaker.
       */

      while (self->tiebreaker <= remote_tiebreaker) {

        if (!change_tie_breaker(self, remote_tiebreaker, 0))
          goto error;
      }
    }
  }

  Stream *stream = self->streams;
  Pair *pair = NULL;

  while (stream) {

    pair = stream->pairs;
    while (pair) {

      pair_calculate_priority(pair);
      pair = ov_node_next(pair);
    }

    stream = ov_node_next(stream);
  }

  return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

static bool session_unfreeze(Session *session) {

  Stream *stream = NULL;
  Pair *pair = NULL;

  if (!session)
    goto error;

  if (!session->streams)
    return true;

  /* We have only the Mutiplexed candidate as candidate for all
   * sessions and streams. So we do have just one foundation to use */

  stream = session->streams;

  /*
   *      For all pairs with the same foundation, it sets the state of
   *      the pair with the lowest component ID to Waiting.  If there is
   *      more than one such pair, the one with the highest priority is
   *      used.
   */

  while (stream) {

    pair = stream->pairs;
    pair->state = OV_ICE_PAIR_WAITING;
    stream = ov_node_next(stream);
  }

  return true;
error:
  return false;
}
/*----------------------------------------------------------------------------*/

static bool perform_connectivity_check(Stream *stream);

/*----------------------------------------------------------------------------*/

static bool connectivity_check_send_next(uint32_t id, void *data) {

  Stream *stream = (Stream *)(data);

  OV_ASSERT(stream);
  OV_ASSERT(stream->session);

  Session *session = stream->session;
  if (!session)
    goto error;

  OV_ASSERT(id == session->timer.connectivity);

  session->timer.connectivity = OV_TIMER_INVALID;

  if (!perform_connectivity_check(stream)) {
    goto error;
  }

  return true;
error:
  if (session)
    session->state = OV_ICE_PROXY_GENERIC_FAILED;
  return false;
}

/*----------------------------------------------------------------------------*/

bool perform_connectivity_check(Stream *stream) {

  OV_ASSERT(stream);
  OV_ASSERT(stream->session);
  OV_ASSERT(stream->session->proxy);

  if (!stream)
    goto error;

  Session *session = stream->session;
  if (!session)
    goto error;

  ov_ice_proxy *proxy = session->proxy;
  if (!proxy)
    goto error;

  ov_event_loop *loop = proxy->public.config.loop;
  if (!loop)
    goto error;

  /*
   *      Search the next stream active state with
   *      pairs available.
   */

  while (stream) {

    if (OV_ICE_PROXY_GENERIC_RUNNING != stream->state_protocol.stun) {

      stream = ov_node_next(stream);
      continue;
    }

    if (!stream->pairs) {

      stream = ov_node_next(stream);
      continue;
    }

    break;
  }

  /*
   *      No stream in RUNNING state with pairs,
   *      reschedule next check for session,
   *      (async with ta delay offset)
   */

  if (!stream)
    goto reschedule_session;

  /*
   *      Send a connectivity check on the
   *      next pair waiting.
   */

  Pair *pair = ov_list_queue_pop(stream->trigger);

  if (!pair) {

    pair = stream->pairs;

    while (pair) {

      if (pair->state == OV_ICE_PAIR_WAITING)
        break;

      pair = ov_node_next(pair);
    }
  }

  if (!pair) {

    /*
     *      Perform a connectivity check on the next
     *      stream in sync, otherwise restart at session async.
     */

    stream = ov_node_next(stream);

    if (!stream)
      goto reschedule_session;

    return perform_connectivity_check(stream);
  }

  switch (pair->state) {

  case OV_ICE_PAIR_PROGRESS:

    if (pair->progress_count > 100) {
      pair->state = OV_ICE_PAIR_FAILED;
      return perform_connectivity_check(session->streams);
    }

    break;

  default:
    break;
  }

  if (!send_stun_binding_request(proxy, pair)) {

    /*
     *      Try connectivity check on next pair of
     *      stream.
     */

    pair->state = OV_ICE_PAIR_FAILED;
    return perform_connectivity_check(stream);
  }

  if (OV_TIMER_INVALID == session->timer.connectivity) {

    session->timer.connectivity = loop->timer.set(
        loop, proxy->public.config.config.timeouts.stun.connectivity_pace_usecs,
        stream, connectivity_check_send_next);

    if (OV_TIMER_INVALID == session->timer.connectivity)
      goto error;
  }

  return true;

reschedule_session:

  if (OV_TIMER_INVALID == session->timer.connectivity) {

    session->timer.connectivity = loop->timer.set(
        loop, proxy->public.config.config.timeouts.stun.connectivity_pace_usecs,
        session->streams, connectivity_check_send_next);

    if (OV_TIMER_INVALID == session->timer.connectivity)
      goto error;
  }

  session_unfreeze(session);
  return true;
error:

  return false;
}

/*----------------------------------------------------------------------------*/

static bool session_checklists_run(Session *session) {

  if (!session)
    goto error;

  /*
   *  We will start running the checklists,
   *  if the session state is NOT completted,
   *  and NOT in error.
   */

  switch (session->state) {

  case OV_ICE_PROXY_GENERIC_INIT:
  case OV_ICE_PROXY_GENERIC_RUNNING:
  case OV_ICE_PROXY_GENERIC_FAILED:
    break;

  case OV_ICE_PROXY_GENERIC_ERROR:
    goto error;
    break;

  case OV_ICE_PROXY_GENERIC_COMPLETED:
    return true;
  }

  if (OV_TIMER_INVALID == session->timer.connectivity) {

    if (!session->streams)
      return true;

    session_unfreeze(session);

    if (!perform_connectivity_check(session->streams))
      goto error;

    session->state = OV_ICE_PROXY_GENERIC_RUNNING;
  }

  return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

static bool session_update_stun(Session *session) {

  if (!session)
    goto error;

  ov_event_loop *loop = session->proxy->public.config.loop;

  Stream *stream = session->streams;

  bool all_completed = true;
  bool all_failed = true;

  while (stream) {

    if (stream->state_protocol.stun == OV_ICE_PROXY_GENERIC_RUNNING)
      stream_update_stun(stream);

    switch (stream->state_protocol.stun) {

    case OV_ICE_PROXY_GENERIC_COMPLETED:
      all_failed = false;
      break;

    case OV_ICE_PROXY_GENERIC_RUNNING:
      all_failed = false;
      all_completed = false;
      break;

    default:
      break;
    }

    stream = ov_node_next(stream);
  }

  if (all_completed) {

    session->state = OV_ICE_PROXY_GENERIC_COMPLETED;

    if (OV_TIMER_INVALID != session->timer.session_timeout) {

      loop->timer.unset(loop, session->timer.session_timeout, NULL);

      session->timer.session_timeout = OV_TIMER_INVALID;
    }

    stream_state_change(stream);

  } else if (all_failed) {

    session->state = OV_ICE_PROXY_GENERIC_FAILED;

    if (OV_TIMER_INVALID != session->timer.session_timeout) {

      loop->timer.unset(loop, session->timer.session_timeout, NULL);

      session->timer.session_timeout = OV_TIMER_INVALID;
    }

    stream_state_change(stream);
  }

  session_checklists_run(session);
  return true;
error:
  return false;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      #FINGERPRINT
 *
 *      ------------------------------------------------------------------------
 */

static char *fingerprint_format_RFC8122(const char *source, size_t length) {

  /*
  hash-func              =  "sha-1" / "sha-224" / "sha-256" /
                       "sha-384" / "sha-512" /
                       "md5" / "md2" / token
                       ; Additional hash functions can only come
                       ; from updates to RFC 3279

  fingerprint            =  2UHEX *(":" 2UHEX)
                       ; Each byte in upper-case hex, separated
                       ; by colons.

  UHEX                   =  DIGIT / %x41-46 ; A-F uppercase
  */

  char *fingerprint = NULL;

  if (!source)
    return NULL;

  size_t hex_len = 2 * length + 1;
  char hex[hex_len + 1];
  memset(hex, 0, hex_len);
  uint8_t *ptr = (uint8_t *)hex;

  if (!ov_convert_binary_to_hex((uint8_t *)source, length, &ptr, &hex_len))
    goto error;

  size_t size = hex_len + length;
  fingerprint = calloc(size + 1, sizeof(char));

  for (size_t i = 0; i < length; i++) {

    fingerprint[(i * 3) + 0] = toupper(hex[(i * 2) + 0]);
    fingerprint[(i * 3) + 1] = toupper(hex[(i * 2) + 1]);
    if (i < length - 1)
      fingerprint[(i * 3) + 2] = ':';
  }

  return fingerprint;

error:
  if (fingerprint)
    free(fingerprint);
  return NULL;
}

/*----------------------------------------------------------------------------*/

static char *X509_fingerprint_create(const X509 *cert, ov_hash_function type) {

  unsigned char mdigest[EVP_MAX_MD_SIZE] = {0};
  unsigned int mdigest_size = 0;

  char *fingerprint = NULL;

  const EVP_MD *func = ov_hash_function_to_EVP(type);
  if (!func || !cert)
    return NULL;

  if (0 < X509_digest(cert, func, mdigest, &mdigest_size)) {
    fingerprint = fingerprint_format_RFC8122((char *)mdigest, mdigest_size);
  }

  return fingerprint;
}

/*----------------------------------------------------------------------------*/

static bool create_fingerprint_cert(const char *path, ov_hash_function hash,
                                    char *out) {

  char *x509_fingerprint = NULL;

  if (!path || !out)
    goto error;

  const char *hash_string = ov_hash_function_to_RFC8122_string(hash);
  if (!hash_string)
    goto error;

  X509 *x = NULL;

  FILE *fp = fopen(path, "r");

  if (!PEM_read_X509(fp, &x, NULL, NULL)) {
    fclose(fp);
    goto error;
  }

  x509_fingerprint = X509_fingerprint_create(x, hash);
  fclose(fp);
  X509_free(x);

  if (!x509_fingerprint)
    goto error;

  size_t size = strlen(x509_fingerprint) + strlen(hash_string) + 2;

  OV_ASSERT(size < ov_ice_proxy_generic_dtls_FINGERPRINT_MAX);
  if (size >= ov_ice_proxy_generic_dtls_FINGERPRINT_MAX)
    goto error;

  memset(out, 0, ov_ice_proxy_generic_dtls_FINGERPRINT_MAX);

  if (!snprintf(out, ov_ice_proxy_generic_dtls_FINGERPRINT_MAX, "%s %s",
                hash_string, x509_fingerprint))
    goto error;

  out[ov_ice_proxy_generic_dtls_FINGERPRINT_MAX - 1] = 0;
  x509_fingerprint = ov_data_pointer_free(x509_fingerprint);
  return true;
error:
  x509_fingerprint = ov_data_pointer_free(x509_fingerprint);
  return false;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      #io FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

static void plain_stun_response(ov_ice_proxy *self, uint8_t *buffer,
                                size_t length, const ov_socket_data *remote) {

  if (!self || !buffer || !remote)
    goto error;

  socklen_t len = sizeof(struct sockaddr_in);
  if (remote->sa.ss_family == AF_INET6)
    len = sizeof(struct sockaddr_in6);

  /* plain STUN request processing */

  if (!ov_stun_frame_set_success_response(buffer, length))
    goto error;

  if (!ov_stun_xor_mapped_address_encode(buffer + 20, length - 20, buffer, NULL,
                                         &remote->sa))
    goto error;

  size_t out = 20 + ov_stun_xor_mapped_address_encoding_length(&remote->sa);

  if (!ov_stun_frame_set_length(buffer, length, out - 20))
    goto error;

  // just to be sure, we nullify the rest of the buffer
  memset(buffer + out, 0, length - out);

  ssize_t send = sendto(self->socket, buffer, out, 0,
                        (const struct sockaddr *)&remote->sa, len);

  UNUSED(send);

error:
  return;
}

/*----------------------------------------------------------------------------*/

static bool io_stun_request(ov_ice_proxy *self, uint8_t *buffer, size_t size,
                            const ov_socket_data *remote, size_t attr_size,
                            uint8_t *attr[]) {

  Stream *stream = NULL;

  size_t name_length = OV_UDP_PAYLOAD_OCTETS;
  uint8_t name[OV_UDP_PAYLOAD_OCTETS];
  memset(name, 0, name_length);

  size_t response_length = OV_UDP_PAYLOAD_OCTETS;
  uint8_t response[OV_UDP_PAYLOAD_OCTETS];
  memset(response, 0, response_length);

  uint8_t *ptr = NULL;
  uint8_t *next = NULL;
  uint32_t priority = 0;

  /*
   *      Prepare required attribute frame pointers for ICE
   */

  uint8_t *attr_username =
      ov_stun_attributes_get_type(attr, attr_size, STUN_USERNAME);

  uint8_t *attr_integrity =
      ov_stun_attributes_get_type(attr, attr_size, STUN_MESSAGE_INTEGRITY);

  uint8_t *attr_priority =
      ov_stun_attributes_get_type(attr, attr_size, ICE_PRIORITY);

  uint8_t *attr_controlling =
      ov_stun_attributes_get_type(attr, attr_size, ICE_CONTROLLING);

  uint8_t *attr_controlled =
      ov_stun_attributes_get_type(attr, attr_size, ICE_CONTROLLED);

  uint8_t *attr_use_candidate =
      ov_stun_attributes_get_type(attr, attr_size, ICE_USE_CANDIDATE);

  if (!attr_priority) {

    plain_stun_response(self, buffer, size, remote);
    goto ignore;
  }

  if (!attr_username || !attr_integrity || !attr_priority)
    goto bad_request;

  if (!attr_controlling && !attr_controlled)
    goto bad_request;

  if (attr_controlling && attr_controlled)
    goto bad_request;

  if (!ov_stun_ice_priority_decode(attr_priority,
                                   size - (attr_priority - buffer), &priority))
    goto bad_request;

  if (!ov_stun_username_decode(attr_username, size - (buffer - attr_username),
                               &ptr, &name_length))
    goto unauthorized;

  const char *pass = NULL;

  uint8_t *colon = memchr(ptr, ':', name_length);
  if (!colon)
    goto unauthorized;

  ssize_t ufrag_in_len = colon - ptr;
  if (ufrag_in_len < 1)
    goto bad_request;

  stream = get_stream_by_ufrag(self, (char *)ptr, ufrag_in_len);
  if (!stream)
    goto unauthorized;

  OV_ASSERT(stream->session);

  pass = stream->local.pass;
  if (!pass)
    goto error;

  if (!ov_stun_check_message_integrity((uint8_t *)buffer, size, attr, attr_size,
                                       (uint8_t *)pass, strlen(pass), true))
    goto unauthorized;

  uint64_t attr_tiebreaker = 0;
  uint64_t sess_tiebreaker = stream->session->tiebreaker;

  if (stream->session->controlling) {

    if (attr_controlling) {

      if (!ov_stun_ice_controlling_decode(attr_controlling,
                                          size - (attr_controlling - buffer),
                                          &attr_tiebreaker))
        goto bad_request;

      if (sess_tiebreaker >= attr_tiebreaker)
        goto role_conflict;

      session_change_role(stream->session, attr_tiebreaker);
    }

  } else {

    if (attr_controlled) {

      /*
       *      Session is NOT controlling,
       *      attr_controlled is present.
       */

      if (!ov_stun_ice_controlled_decode(attr_controlled,
                                         size - (attr_controlling - buffer),
                                         &attr_tiebreaker))

        goto bad_request;

      if (sess_tiebreaker < attr_tiebreaker)
        goto role_conflict;

      session_change_role(stream->session, attr_tiebreaker);
    }
  }

  /*
   *      No conflicts, no errors,
   *      generate a success reponse
   *
   */

  if (!ov_stun_frame_set_success_response(response, response_length))
    goto error;

  if (!ov_stun_frame_set_magic_cookie(response, response_length))
    goto error;

  if (!ov_stun_frame_set_method(response, response_length, STUN_BINDING))
    goto error;

  if (!ov_stun_frame_set_transaction_id(
          response, response_length,
          ov_stun_frame_get_transaction_id(buffer, size)))
    goto error;

  // add XOR MAPPED ADDRESS
  if (!ov_stun_xor_mapped_address_encode(response + 20, response_length - 20,
                                         response, &next, &remote->sa))
    goto error;

  if (!ov_stun_frame_set_length(response, response_length,
                                (next - response) - 20))
    goto error;

  response_length = (next - response);

  // add a message integrity
  if (!ov_stun_add_message_integrity(response, OV_UDP_PAYLOAD_OCTETS,
                                     response + response_length, &next,
                                     (uint8_t *)pass, strlen(pass)))
    goto error;

  response_length = (next - response);

  Pair *pair = get_pair_by_remote(stream, remote);

  if (!pair) {

    pair = stream_add_peer_reflexive_pair(stream, remote, priority);

    stream_order(stream);
    stream_prune(stream);

    pair->state = OV_ICE_PAIR_WAITING;
  }

  OV_ASSERT(pair);

  if (attr_use_candidate && !stream->session->controlling) {

    pair->nominated = true;
    stream->selected = pair;
    send_stun_binding_request(self, pair);
  }

  switch (pair->state) {

  case OV_ICE_PAIR_SUCCESS:
    pair->progress_count = 0;
    break;

  case OV_ICE_PAIR_WAITING:
  case OV_ICE_PAIR_FROZEN:
  case OV_ICE_PAIR_PROGRESS:
  case OV_ICE_PAIR_FAILED:

    if (0 == ov_list_get_pos(stream->trigger, pair)) {

      if (ov_list_queue_push(stream->trigger, pair)) {

        pair->state = OV_ICE_PAIR_WAITING;

        if (OV_ICE_PROXY_GENERIC_FAILED == stream->state_protocol.stun) {
          stream->state_protocol.stun = OV_ICE_PROXY_GENERIC_RUNNING;
          stream_state_change(stream);
        }

      } else {
        pair->state = OV_ICE_PAIR_FAILED;
      }
    }

    break;
  }

  session_update_stun(stream->session);

  goto send_response;

role_conflict:

  response_length = ov_stun_error_code_generate_response(
      buffer, size, response, response_length,
      ov_stun_error_code_set_ice_role_conflict);

  if (0 == response_length)
    goto error;

  goto send_response;

bad_request:

  response_length = ov_stun_error_code_generate_response(
      buffer, size, response, response_length,
      ov_stun_error_code_set_bad_request);

  if (0 == response_length)
    goto error;

  goto send_response;

unauthorized:

  response_length = ov_stun_error_code_generate_response(
      buffer, size, response, response_length,
      ov_stun_error_code_set_unauthorized);

  if (0 == response_length)
    goto error;

  goto send_response;

send_response:

  // set new length
  if (!ov_stun_attribute_set_length(response, OV_UDP_PAYLOAD_OCTETS,
                                    response_length - 20))
    goto error;

  // add a fingerprint
  if (!ov_stun_add_fingerprint(response, OV_UDP_PAYLOAD_OCTETS,
                               response + response_length, NULL))
    goto error;

  response_length += ov_stun_fingerprint_encoding_length();

  ssize_t bytes = -1;

  socklen_t len = sizeof(struct sockaddr_in);
  if (remote->sa.ss_family == AF_INET6)
    len = sizeof(struct sockaddr_in6);

  bytes = sendto(self->socket, response, response_length, 0,
                 (struct sockaddr *)&remote->sa, len);
  UNUSED(bytes);

ignore:
  return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

static bool io_stun_success(ov_ice_proxy *self, uint8_t *buffer, size_t size,
                            const ov_socket_data *remote, size_t attr_size,
                            uint8_t *attr[]) {

  if (!self || !buffer || !size || !remote || !attr_size || !attr)
    goto ignore;

  Pair *pair = transaction_unset(
      self, (char *)ov_stun_frame_get_transaction_id(buffer, size));

  if (!pair)
    goto ignore;

  /*
   *      Prepare required attribute frame pointers.
   */

  uint8_t *xmap =
      ov_stun_attributes_get_type(attr, attr_size, STUN_XOR_MAPPED_ADDRESS);

  uint8_t *prio = ov_stun_attributes_get_type(attr, attr_size, ICE_PRIORITY);

  if (!xmap)
    goto ignore;

  ov_socket_data xor_mapped = {0};
  struct sockaddr_storage *xor_ptr = &xor_mapped.sa;

  if (!ov_stun_xor_mapped_address_decode(xmap, size - (xmap - buffer), buffer,
                                         &xor_ptr))
    goto ignore;

  xor_mapped = ov_socket_data_from_sockaddr_storage(&xor_mapped.sa);

  uint32_t priority = 0;

  if (prio) {

    if (!ov_stun_ice_priority_decode(prio, size - (prio - buffer), &priority))
      goto ignore;
  }

  Stream *stream = pair->stream;
  Session *session = stream->session;

  /*
   *      Check if we get a symetric response
   */

  if ((pair->remote.socket.port != remote->port) ||
      (0 != strncmp(pair->remote.socket.host, remote->host, OV_HOST_NAME_MAX)))
    goto ignore;

  if ((xor_mapped.port != self->public.config.external.port) ||
      (0 != memcmp(xor_mapped.host, self->public.config.external.host,
                   OV_HOST_NAME_MAX))) {

    /* We received some peer reflexive response */

    pair = stream_add_peer_reflexive_pair(stream, remote, priority);

    OV_ASSERT(pair);

    stream_order(stream);
    stream_prune(stream);
  }

  pair->state = OV_ICE_PAIR_SUCCESS;
  pair->success_count++;
  pair->progress_count = 0;

  if (0 == ov_list_get_pos(stream->valid, pair)) {
    if (!ov_list_push(stream->valid, pair)) {
      pair->state = OV_ICE_PAIR_FAILED;
      goto error;
    }
  }

  /* add retriggered check to increase success count */
  if (0 == ov_list_get_pos(stream->trigger, pair))
    ov_list_queue_push(stream->trigger, pair);

  if (!session->controlling) {

    if (pair->nominated) {

      stream->state_protocol.stun = OV_ICE_PROXY_GENERIC_COMPLETED;
      ov_log_debug("Stream %s completed STUN", stream->uuid);
      stream_state_change(stream);
    }

  } else if (session->controlling) {

    if (pair->nominated) {

      /*
       *  We received a success response on the nominated pair,
       *  we support one component id per stream.
       */

      stream->selected = pair;

      ov_log_debug("Stream %s completed STUN", stream->uuid);
      stream->state_protocol.stun = OV_ICE_PROXY_GENERIC_COMPLETED;

      if (stream->state_protocol.dtls == OV_ICE_PROXY_GENERIC_COMPLETED)
        stream_callback_selected_ready(stream);

      stream_state_change(stream);

      if (OV_TIMER_INVALID != stream->timer.nominate) {

        ov_event_loop_timer_unset(session->proxy->public.config.loop,
                                  stream->timer.nominate, NULL);

        stream->timer.nominate = OV_TIMER_INVALID;
      }
    }
  }

  session_update_stun(session);
ignore:
  return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

static bool io_stun_error(ov_ice_proxy *self, uint8_t *buffer, size_t size,
                          const ov_socket_data *remote, size_t attr_size,
                          uint8_t *attr[]) {

  Stream *stream = get_stream_by_remote(self, remote);

  if (!stream || !buffer || !size || !remote || !attr_size || !attr)
    goto ignore;

  Pair *pair = transaction_unset(
      self, (char *)ov_stun_frame_get_transaction_id(buffer, size));

  if (!pair)
    goto ignore;

  pair->state = OV_ICE_PAIR_FAILED;

  OV_ASSERT(pair->stream);
  OV_ASSERT(pair->stream->session);

  session_update_stun(pair->stream->session);

ignore:
  return true;
}

/*----------------------------------------------------------------------------*/

static bool io_external_stun(ov_ice_proxy *self, uint8_t *buffer, size_t size,
                             const ov_socket_data *remote) {

  size_t attr_size = IMPL_STUN_ATTR_FRAMES;
  uint8_t *attr[attr_size];
  memset(attr, 0, attr_size * sizeof(uint8_t *));

  if (!self || !buffer || !size || !remote)
    goto error;

  uint16_t method = ov_stun_frame_get_method(buffer, size);

  if (!ov_stun_frame_is_valid(buffer, size))
    goto ignore;

  if (!ov_stun_frame_has_magic_cookie(buffer, size))
    goto ignore;

  if (!ov_stun_frame_slice(buffer, size, attr, attr_size))
    goto ignore;

  if (!ov_stun_check_fingerprint(buffer, size, attr, attr_size, false))
    goto ignore;

  switch (method) {

  case STUN_BINDING:
    break;

  default:
    goto ignore;
  }

  if (ov_stun_frame_class_is_indication(buffer, size))
    goto ignore;

  if (ov_stun_frame_class_is_request(buffer, size)) {

    return io_stun_request(self, buffer, size, remote, attr_size, attr);

  } else if (ov_stun_frame_class_is_success_response(buffer, size)) {

    return io_stun_success(self, buffer, size, remote, attr_size, attr);

  } else if (ov_stun_frame_class_is_error_response(buffer, size)) {

    return io_stun_error(self, buffer, size, remote, attr_size, attr);
  }

ignore:
  return true;

error:
  return false;
}

/*---------------------------------------------------------------------------*/

static bool
srtp_get_key_length_of_profile(const SRTP_PROTECTION_PROFILE *profile,
                               uint32_t *keylen, uint32_t *saltlen) {

  if (!profile || !keylen || !saltlen)
    goto error;

  if (!profile->id)
    goto error;

  switch (profile->id) {

  case SRTP_AES128_CM_SHA1_80:
    *keylen = 16;
    *saltlen = 14;
    break;

  case SRTP_AES128_CM_SHA1_32:
    *keylen = 16;
    *saltlen = 14;
    break;

  case SRTP_AEAD_AES_128_GCM:
    *keylen = 16;
    *saltlen = 12;
    break;

  case SRTP_AEAD_AES_256_GCM:
    *keylen = 32;
    *saltlen = 12;
    break;

  default:
    goto error;
  }

  return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

static const char *dtls_get_srtp_keys(Pair *pair, uint32_t *key_len,
                                      uint32_t *salt_len, uint8_t *server_key,
                                      uint8_t *server_salt, uint8_t *client_key,
                                      uint8_t *client_salt) {

  size_t size =
      2 * OV_ICE_PROXY_SRTP_KEY_MAX + 2 * OV_ICE_PROXY_SRTP_SALT_MAX + 1;

  uint8_t buffer[size];
  memset(buffer, 0, size);

  if (!pair || !pair->dtls.ssl || !key_len || !salt_len || !server_key ||
      !server_salt || !client_key || !client_salt)
    goto error;

  uint32_t keylen = 0;
  uint32_t saltlen = 0;

  SRTP_PROTECTION_PROFILE *profile =
      SSL_get_selected_srtp_profile(pair->dtls.ssl);

  if (!srtp_get_key_length_of_profile(profile, &keylen, &saltlen))
    goto error;

  if (keylen > *key_len)
    goto error;

  if (saltlen > *salt_len)
    goto error;

  /*      The total length of keying material obtained
          should be equal to two times the sum of
          the master key length and the salt length
          as defined for the protection profile in use.

          This provides the client write master key,
          the server write master key,
          the client write master salt and
          the server write master salt in that order.

  */

  if (1 != SSL_export_keying_material(pair->dtls.ssl, buffer, size,
                                      label_extractor_srtp,
                                      strlen(label_extractor_srtp), NULL, 0, 0))
    goto error;

  *key_len = keylen;
  *salt_len = saltlen;

  uint8_t *ptr = buffer;

  if (!memcpy(client_key, ptr, keylen))
    return false;
  ptr += keylen;
  if (!memcpy(server_key, ptr, keylen))
    return false;
  ptr += keylen;
  if (!memcpy(client_salt, ptr, saltlen))
    return false;
  ptr += saltlen;
  if (!memcpy(server_salt, ptr, saltlen))
    return false;
  ptr += saltlen;

  return profile->name;
error:
  return NULL;
}

/*----------------------------------------------------------------------------*/

static bool srtp_unset_data(Pair *pair) {

  if (!pair)
    return false;

  pair->srtp.ready = false;
  pair->srtp.profile = ov_data_pointer_free(pair->srtp.profile);
  pair->srtp.key_len = OV_ICE_PROXY_SRTP_KEY_MAX;
  pair->srtp.salt_len = OV_ICE_PROXY_SRTP_SALT_MAX;

  memset(pair->srtp.server.key, 0, OV_ICE_PROXY_SRTP_KEY_MAX);
  memset(pair->srtp.client.key, 0, OV_ICE_PROXY_SRTP_KEY_MAX);
  memset(pair->srtp.server.salt, 0, OV_ICE_PROXY_SRTP_SALT_MAX);
  memset(pair->srtp.client.salt, 0, OV_ICE_PROXY_SRTP_SALT_MAX);

  return true;
}

/*----------------------------------------------------------------------------*/

static bool get_profile(Pair *pair) {

  srtp_unset_data(pair);

  const char *profile = dtls_get_srtp_keys(
      pair, &pair->srtp.key_len, &pair->srtp.salt_len, pair->srtp.server.key,
      pair->srtp.server.salt, pair->srtp.client.key, pair->srtp.client.salt);

  if (profile) {

    pair->srtp.ready = true;
    pair->srtp.profile = ov_data_pointer_free(pair->srtp.profile);
    pair->srtp.profile = strdup(profile);

    stream_complete_dtls(pair->stream, pair);
  }

  return true;
}

/*----------------------------------------------------------------------------*/

static long dtls_bio_ctrl(BIO *bio, int cmd, long num, void *ptr) {

  UNUSED(bio);
  UNUSED(num);
  UNUSED(ptr);

  switch (cmd) {
  case BIO_CTRL_FLUSH:
    return 1;
  case BIO_CTRL_WPENDING:
  case BIO_CTRL_PENDING:
    return 0L;
  default:
    break;
  }

  return 0;
}

/*----------------------------------------------------------------------------*/

static int dtls_bio_write(BIO *bio, const char *in, int size) {

  if (size <= 0)
    goto error;

  Pair *pair = (Pair *)BIO_get_data(bio);

  return pair_send(pair, (uint8_t *)in, size);
error:
  return -1;
}

/*----------------------------------------------------------------------------*/

static int dtls_bio_create(BIO *bio) {

  BIO_set_init(bio, 1);
  BIO_set_data(bio, NULL);
  BIO_set_shutdown(bio, 0);
  return 1;
}

/*----------------------------------------------------------------------------*/

static int dtls_bio_free(BIO *bio) {

  if (bio == NULL) {
    return 0;
  }

  BIO_set_data(bio, NULL);
  return 1;
}

/*----------------------------------------------------------------------------*/

static void dtls_filter_init() {

  dtls_filter_methods = BIO_meth_new(BIO_TYPE_BIO, "DTLS writer");

  if (!dtls_filter_methods) {
    goto error;
  }
  BIO_meth_set_write(dtls_filter_methods, dtls_bio_write);
  BIO_meth_set_ctrl(dtls_filter_methods, dtls_bio_ctrl);
  BIO_meth_set_create(dtls_filter_methods, dtls_bio_create);
  BIO_meth_set_destroy(dtls_filter_methods, dtls_bio_free);
error:
  return;
}

/*----------------------------------------------------------------------------*/

static void dtls_filter_deinit() {

  if (dtls_filter_methods) {
    BIO_meth_free(dtls_filter_methods);
    dtls_filter_methods = NULL;
  }
}

/*----------------------------------------------------------------------------*/

static BIO *dtls_filter_pair_bio_create(Pair *pair) {

  BIO *bio = BIO_new(dtls_filter_methods);

  if (bio == NULL) {
    return NULL;
  }

  BIO_set_data(bio, pair);
  return bio;
}

/*----------------------------------------------------------------------------*/

static bool dtls_handshake_passive(ov_ice_proxy *self, Pair *pair,
                                   uint8_t *buffer, size_t size) {

  char errorstring[OV_ICE_PROXY_SSL_ERROR_STRING_BUFFER_SIZE] = {0};
  int errorcode = -1;
  int n = 0, r = 0;

  if (!self || !pair || !buffer || !size)
    goto error;

  if (pair->dtls.ssl) {

    SSL_free(pair->dtls.ssl);
    pair->dtls.ssl = NULL;
  }

  pair->dtls.handshaked = false;
  pair->dtls.type = OV_ICE_PROXY_GENERIC_DTLS_PASSIVE;
  pair->dtls.ssl = SSL_new(self->dtls.ctx);

  if (!pair->dtls.ssl)
    goto error;

  SSL_set_accept_state(pair->dtls.ssl);

  OV_ASSERT(1 == SSL_is_server(pair->dtls.ssl));

  r = SSL_set_tlsext_use_srtp(pair->dtls.ssl,
                              self->public.config.config.dtls.srtp.profile);
  if (0 != r)
    goto error;

  pair->dtls.read = BIO_new(BIO_s_mem());
  if (!pair->dtls.read)
    goto error;

  pair->dtls.write = dtls_filter_pair_bio_create(pair);
  if (!pair->dtls.write)
    goto error;

  BIO_set_mem_eof_return(pair->dtls.read, -1);
  BIO_set_mem_eof_return(pair->dtls.write, -1);

  SSL_set_bio(pair->dtls.ssl, pair->dtls.read, pair->dtls.write);
  SSL_set_options(pair->dtls.ssl, SSL_OP_COOKIE_EXCHANGE);

  r = BIO_write(pair->dtls.read, buffer, size);
  if (r < 0)
    goto error;

  BIO_ADDR *peer_bio = BIO_ADDR_new();

  r = DTLSv1_listen(pair->dtls.ssl, peer_bio);

  if (peer_bio) {
    BIO_ADDR_free(peer_bio);
    peer_bio = NULL;
  }

  if (r >= 1) {

    pair->dtls.handshaked = true;
    get_profile(pair);
    goto done;

  } else if (r == 0) {

    /*
     *      Non Fatal error, usercode is expected to
     *      retry operation. (man DTLSv1_listen)
     */

    SSL_free(pair->dtls.ssl);
    pair->dtls.ssl = NULL;

  } else {

    n = SSL_get_error(pair->dtls.ssl, r);

    switch (n) {

    case SSL_ERROR_NONE:
    case SSL_ERROR_WANT_READ:
    case SSL_ERROR_WANT_CONNECT:
    case SSL_ERROR_WANT_ACCEPT:
    case SSL_ERROR_WANT_X509_LOOKUP:
    case SSL_ERROR_WANT_WRITE:
      break;

    case SSL_ERROR_ZERO_RETURN:
      // connection close
      // ov_log_debug("FD %i connection closed", socket);
      goto error;
      break;

    case SSL_ERROR_SYSCALL:

      ov_log_error("SSL_ERROR_SYSCALL - "
                   "errno %d | %s",
                   errno, strerror(errno));

      goto error;

      break;

    case SSL_ERROR_SSL:

      errorcode = ERR_get_error();
      ERR_error_string_n(errorcode, errorstring,
                         OV_ICE_PROXY_SSL_ERROR_STRING_BUFFER_SIZE);

      ov_log_error("socket SSL_ERROR_SSL - "
                   "%s",
                   errorstring);
      goto error;

      break;

    default:

      goto error;
      break;
    }
  }

done:
  return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

static bool dtls_io(Pair *pair, uint8_t *buffer, size_t size) {

  char buf[OV_ICE_PROXY_SSL_BUFFER_SIZE] = {0};

  int r = 0;
  ssize_t out = 0;

  if (!pair || !buffer || !size)
    goto error;

  if (!pair->dtls.ssl)
    goto error;
  if (!pair->dtls.read)
    goto error;

  r = BIO_write(pair->dtls.read, buffer, size);
  if (r < 0)
    goto error;

  if (!SSL_is_init_finished(pair->dtls.ssl)) {

    switch (pair->dtls.type) {

    case OV_ICE_PROXY_GENERIC_DTLS_PASSIVE:

      r = SSL_do_handshake(pair->dtls.ssl);

      if (SSL_is_init_finished(pair->dtls.ssl)) {
        pair->dtls.handshaked = true;
      }

      break;

    default:
      goto error;
    }

    get_profile(pair);

  } else {

    // just read empty
    out = SSL_read(pair->dtls.ssl, buf, OV_ICE_PROXY_SSL_BUFFER_SIZE);
    UNUSED(out);

    if (!pair->srtp.profile)
      get_profile(pair);
  }

  return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

static bool io_external_ssl(ov_ice_proxy *self, uint8_t *buffer, size_t size,
                            const ov_socket_data *remote) {

  if (!self || !buffer || !size || !remote)
    goto error;

  Stream *stream = get_stream_by_remote(self, remote);
  if (!stream)
    goto ignore;

  Pair *pair = get_pair_by_remote(stream, remote);
  if (!pair)
    goto ignore;

  if (!pair->dtls.ssl) {

    if (!dtls_handshake_passive(self, pair, buffer, size))
      goto ignore;

    goto ignore;
  }

  dtls_io(pair, buffer, size);

ignore:
  return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

static bool io_external_rtp(ov_ice_proxy *self, uint8_t *buffer, size_t size,
                            const ov_socket_data *remote) {

  if (!self || !buffer || !size || !remote)
    goto error;

  Stream *stream = get_stream_by_remote(self, remote);
  if (!stream)
    goto ignore;

  int l = size;

  srtp_t srtp_session = stream->session->srtp.session;

  if (!srtp_session)
    goto error;

  srtp_err_status_t r = srtp_unprotect(srtp_session, buffer, &l);

  switch (r) {

  case srtp_err_status_ok:
    // ov_log_debug("SRTP unprotect success");
    break;

  default:
    // ov_log_error("SRTP unprotect error");
    goto ignore;
    break;
  }

  /* We change the SSRC to the proxy SSRC and forward the RTP Frame
   * internal */

  uint32_t u32 = htonl(stream->local.ssrc);
  memcpy(buffer + 8, &u32, 4);

  /* Forward to callback */

  ov_ice_proxy *proxy = stream->session->proxy;

  if (proxy->public.config.callbacks.stream.io)
    proxy->public.config.callbacks.stream.io(
        proxy->public.config.callbacks.userdata, stream->session->uuid,
        stream->index, buffer, l);

ignore:
  return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

static bool io_external(int socket, uint8_t events, void *userdata) {

  uint8_t buffer[OV_UDP_PAYLOAD_OCTETS] = {0};
  ov_socket_data remote = {};
  socklen_t src_addr_len = sizeof(remote.sa);

  ov_ice_proxy *self = as_ice_proxy(userdata);
  if (!self)
    goto error;

  if ((events & OV_EVENT_IO_CLOSE) || (events & OV_EVENT_IO_ERR)) {

    ov_log_debug("%i - closing", socket);
    return true;
  }

  OV_ASSERT(events & OV_EVENT_IO_IN);

  ssize_t bytes = recvfrom(socket, (char *)buffer, OV_UDP_PAYLOAD_OCTETS, 0,
                           (struct sockaddr *)&remote.sa, &src_addr_len);

  if (bytes < 1)
    goto error;

  if (!ov_socket_parse_sockaddr_storage(&remote.sa, remote.host,
                                        OV_HOST_NAME_MAX, &remote.port))
    goto error;

  /*  -----------------------------------------------------------------
   *      RFC 7983 paket forwarding
   *
   *                        BYTE 1
   *                  +----------------+
   *                  |        [0..3] -+--> forward to STUN
   *                  |                |
   *                  |      [16..19] -+--> forward to ZRTP
   *                  |                |
   *      packet -->  |      [20..63] -+--> forward to DTLS
   *                  |                |
   *                  |      [64..79] -+--> forward to TURN Channel
   *                  |                |
   *                  |    [128..191] -+--> forward to RTP/RTCP
   *                  +----------------+
   *   -----------------------------------------------------------------
   *
   *      We will get ANY non STUN non DTLS data here.
   *
   *      If some SSL header is present,
   *      process SSL
   */

  if (buffer[0] <= 3)
    return io_external_stun(self, buffer, bytes, &remote);

  if (buffer[0] <= 63 && buffer[0] >= 20) {

    return io_external_ssl(self, buffer, bytes, &remote);
  }

  if (buffer[0] <= 191 && buffer[0] >= 128) {

    return io_external_rtp(self, buffer, bytes, &remote);
  }

  return true;
error:
  return false;
}

/*
 *      ------------------------------------------------------------------------
 *
 *      #dtls FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

static bool load_certificates(SSL_CTX *ctx,
                              const ov_ice_proxy_generic_config *config,
                              const char *type) {

  if (!ctx || !config || !type)
    goto error;

  if (SSL_CTX_use_certificate_chain_file(ctx, config->config.dtls.cert) != 1) {

    ov_log_error("ICE %s config failure load certificate "
                 "from %s | error %d | %s",
                 type, config->config.dtls.cert, errno, strerror(errno));
    goto error;
  }

  if (SSL_CTX_use_PrivateKey_file(ctx, config->config.dtls.key,
                                  SSL_FILETYPE_PEM) != 1) {

    ov_log_error("ICE %s config failure load key "
                 "from %s | error %d | %s",
                 type, config->config.dtls.key, errno, strerror(errno));
    goto error;
  }

  if (SSL_CTX_check_private_key(ctx) != 1) {

    ov_log_error("ICE %s config failure private key for\n"
                 "CERT | %s\n"
                 " KEY | %s",
                 type, config->config.dtls.cert, config->config.dtls.key);
    goto error;
  }

  ov_log_info("ICE loaded %s certificate \n file %s\n key %s\n", type,
              config->config.dtls.cert, config->config.dtls.key);

  return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

static bool renew_dtls_keys(uint32_t timer_id, void *data) {

  UNUSED(timer_id);
  ov_ice_proxy *self = (ov_ice_proxy *)data;

  if (!ov_ice_proxy_generic_dtls_cookie_store_initialize(
          self->dtls.cookies,
          self->public.config.config.dtls.dtls.keys.quantity,
          self->public.config.config.dtls.dtls.keys.length))
    goto error;

  self->timer.dtls_key_renew = self->public.config.loop->timer.set(
      self->public.config.loop,
      self->public.config.config.dtls.dtls.keys.lifetime_usec, self,
      renew_dtls_keys);

  if (OV_TIMER_INVALID == self->timer.dtls_key_renew)
    goto error;

  return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

static bool configure_dtls(ov_ice_proxy *self) {

  OV_ASSERT(self);

  if (!self)
    goto error;

  if (!self->dtls.ctx) {
    self->dtls.ctx = SSL_CTX_new(DTLS_server_method());
  }

  if (!self->dtls.ctx)
    goto error;

  if (!self->dtls.cookies) {
    self->dtls.cookies = ov_ice_proxy_generic_dtls_cookie_store_create();
    if (!self->dtls.cookies)
      goto error;
  }

  if (OV_TIMER_INVALID != self->timer.dtls_key_renew) {

    self->public.config.loop->timer.unset(self->public.config.loop,
                                          self->timer.dtls_key_renew, NULL);
  }

  self->timer.dtls_key_renew = OV_TIMER_INVALID;

  if (!load_certificates(self->dtls.ctx, &self->public.config, "DTLS"))
    goto error;

  if (!ov_ice_proxy_generic_dtls_cookie_store_initialize(
          self->dtls.cookies,
          self->public.config.config.dtls.dtls.keys.quantity,
          self->public.config.config.dtls.dtls.keys.length))
    goto error;

  SSL_CTX_set_min_proto_version(self->dtls.ctx, DTLS1_2_VERSION);
  SSL_CTX_set_max_proto_version(self->dtls.ctx, DTLS1_2_VERSION);

  SSL_CTX_set_cookie_generate_cb(self->dtls.ctx,
                                 ov_ice_proxy_generic_dtls_cookie_generate);

  SSL_CTX_set_cookie_verify_cb(self->dtls.ctx,
                               ov_ice_proxy_generic_dtls_cookie_verify);

  self->timer.dtls_key_renew = self->public.config.loop->timer.set(
      self->public.config.loop,
      self->public.config.config.dtls.dtls.keys.lifetime_usec, self,
      renew_dtls_keys);

  if (OV_TIMER_INVALID == self->timer.dtls_key_renew)
    goto error;

  if (0 == SSL_CTX_set_tlsext_use_srtp(
               self->dtls.ctx, self->public.config.config.dtls.srtp.profile)) {

    ov_log_debug("SSL context enabled SRTP profiles %s",
                 self->public.config.config.dtls.srtp.profile);

  } else {

    ov_log_error("SSL failed to enabled SRTP profiles %s",
                 self->public.config.config.dtls.srtp.profile);

    goto error;
  }

  return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

static ov_ice_proxy_generic *ov_ice_proxy_free(ov_ice_proxy_generic *in) {

  ov_ice_proxy *self = as_ice_proxy(in);
  if (!self)
    goto error;

  self->sessions = ov_dict_free(self->sessions);
  self->streams = ov_dict_free(self->streams);
  self->remote = ov_dict_free(self->remote);
  self->transactions = ov_dict_free(self->transactions);

  self->candidate.string = ov_data_pointer_free(self->candidate.string);

  if (-1 != self->socket) {

    ov_event_loop_unset(self->public.config.loop, self->socket, NULL);
    close(self->socket);
    self->socket = -1;
  }

  if (OV_TIMER_INVALID == self->timer.dtls_key_renew) {

    ov_event_loop_timer_unset(self->public.config.loop,
                              self->timer.dtls_key_renew, NULL);

    self->timer.dtls_key_renew = OV_TIMER_INVALID;
  }

  if (self->dtls.ctx) {
    SSL_CTX_free(self->dtls.ctx);
    self->dtls.ctx = NULL;
  }

  self->dtls.cookies =
      ov_ice_proxy_generic_dtls_cookie_store_free(self->dtls.cookies);

  dtls_filter_deinit();
  // clean other openssl initializations
  // TBD check if this are all required
  // FIPS_mode_set(0);
  EVP_cleanup();
  CRYPTO_cleanup_all_ex_data();
  ERR_free_strings();
  CONF_modules_finish();
  RAND_cleanup();

  self = ov_data_pointer_free(self);

  return NULL;
error:
  return in;
}

/*----------------------------------------------------------------------------*/

ov_sdp_connection get_connection(ov_ice_proxy *self, ov_sdp_description *desc) {

  ov_sdp_connection c = {0};
  if (!self || !desc)
    goto error;

  switch (self->local.sa.ss_family) {

  case AF_INET:
    c.addrtype = "IP4";
    break;

  case AF_INET6:
    c.addrtype = "IP6";
    break;

  default:
    goto error;
  }

  desc->media.port = self->local.port;
  c.nettype = "IN";
  c.address = self->local.host;

  return c;

error:
  return (ov_sdp_connection){0};
}

/*----------------------------------------------------------------------------*/

const char *ov_ice_proxy_create_session(ov_ice_proxy_generic *generic,
                                        ov_sdp_session *sdp) {

  char ssrc[OV_UDP_PAYLOAD_OCTETS] = {0};
  ov_ice_proxy *self = as_ice_proxy(generic);

  if (!self || !sdp)
    goto error;

  uint32_t count = ov_node_count(sdp->description);

  Stream *stream = NULL;
  Session *session = session_create(self);
  if (!session)
    goto error;

  sdp->name = session->uuid;
  sdp->origin.name = "-";
  sdp->origin.id = ov_time_get_current_time_usecs();
  sdp->origin.version = ov_time_get_current_time_usecs();

  sdp->origin.connection.nettype = "IN";
  sdp->origin.connection.addrtype = "IP4";
  sdp->origin.connection.address = "0.0.0.0";

  if (!ov_sdp_attribute_add(&sdp->attributes, OV_ICE_STRING_OPTIONS,
                            OV_ICE_STRING_TRICKLE))
    goto error;

  ov_sdp_description *description = NULL;

  ov_sdp_session_persist(sdp);

  for (size_t i = 0; i < count; i++) {

    stream = stream_create(session, i);
    if (!stream)
      goto error;

    description = ov_node_get(sdp->description, i + 1);

    ov_sdp_list *formats = sdp->description->media.formats;
    if (ov_node_count(formats) != 1)
      goto error;
    const char *fmt = formats->value;
    ov_convert_string_to_uint16(fmt, strlen(fmt), &stream->format);

    /*
     *      Set stream parameter
     */

    if (!ov_sdp_attribute_add(&description->attributes, OV_ICE_STRING_USER,
                              stream->uuid))
      goto error;

    if (!ov_sdp_attribute_add(&description->attributes, OV_ICE_STRING_PASS,
                              stream->local.pass))
      goto error;

    if (!ov_sdp_attribute_add(&description->attributes, OV_KEY_SETUP,
                              OV_ICE_STRING_PASSIVE))
      goto error;

    if (!ov_sdp_attribute_add(&description->attributes, OV_KEY_RTCP_MUX, NULL))
      goto error;

    if (!ov_sdp_attribute_add(&description->attributes, OV_KEY_FINGERPRINT,
                              self->dtls.fingerprint))
      goto error;

    description->connection = ov_sdp_connection_create();
    if (!description->connection)
      goto error;

    *description->connection = get_connection(self, description);

    /*
     *      Add SSRC and CNAME
     *
     *      We use RFC 7022 long term cname,
     *      but set the session uuid instead of
     *      a persistant uuid.
     */

    if (!memset(ssrc, 0, OV_UDP_PAYLOAD_OCTETS))
      goto error;

    if (!snprintf(ssrc, OV_UDP_PAYLOAD_OCTETS, "%" PRIu32 " cname:%s",
                  stream->local.ssrc, stream->uuid))
      goto error;

    if (!ov_sdp_attribute_add(&description->attributes, OV_KEY_SSRC, ssrc))
      goto error;

    if (!ov_sdp_attribute_add(&description->attributes, OV_KEY_CANDIDATE,
                              self->candidate.string))
      goto error;

    if (self->nat_ip.string) {

      if (!ov_sdp_attribute_add(&description->attributes, OV_KEY_CANDIDATE,
                                self->nat_ip.string))
        goto error;
    }

    if (!ov_sdp_attribute_add(&description->attributes,
                              OV_ICE_STRING_END_OF_CANDIDATES, NULL))
      goto error;

    ov_sdp_session_persist(sdp);
  }

  return session->uuid;

error:
  return NULL;
}

/*----------------------------------------------------------------------------*/

static bool ov_ice_proxy_session_drop(ov_ice_proxy_generic *generic,
                                      const char *session_id) {

  ov_ice_proxy *self = as_ice_proxy(generic);

  if (!self || !session_id)
    goto error;

  return ov_dict_del(self->sessions, session_id);
error:
  return false;
}

/*----------------------------------------------------------------------------*/

static bool sdp_verify_input(Session *session, const ov_sdp_session *sdp) {

  if (!session || !sdp)
    goto error;

  if (ov_node_count(sdp->description) != ov_node_count(session->streams))
    goto error;

  bool sendonly = false;
  bool recvonly = false;
  bool sendrecv = false;

  ov_sdp_description *desc = sdp->description;

  while (desc) {

    sendonly = false;
    recvonly = false;
    sendrecv = false;

    if ((0 != strcmp(desc->media.protocol, OV_KEY_PROTOCOL_SAVPF)) &&
        (0 != strcmp(desc->media.protocol, OV_KEY_PROTOCOL_SAVP))) {
      goto error;
    }

    if (ov_sdp_attribute_is_set(desc->attributes, OV_KEY_RECV_ONLY))
      recvonly = true;

    if (ov_sdp_attribute_is_set(desc->attributes, OV_KEY_SEND_ONLY))
      sendonly = true;

    if (ov_sdp_attribute_is_set(desc->attributes, OV_KEY_SEND_RECV))
      sendrecv = true;

    if (sendrecv)
      if (recvonly || sendonly)
        goto error;

    if (recvonly && sendonly)
      goto error;

    desc = ov_node_next(desc);
  }

  return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

static bool check_ice_trickle(const ov_sdp_list *attributes, bool *trickle) {

  if (!trickle)
    return false;
  *trickle = false;

  if (!attributes)
    return true;

  const char *ptr = ov_sdp_attribute_get(attributes, OV_ICE_STRING_OPTIONS);
  if (!ptr)
    return true;

  if (0 != strcmp(ptr, OV_ICE_STRING_TRICKLE)) {
    return false;
  }

  *trickle = true;
  return true;
}

/*----------------------------------------------------------------------------*/

static bool stream_add_candidate(Stream *stream, const ov_ice_candidate *c) {

  ov_socket_data remote = {0};

  if (!stream || !c)
    goto error;

  memcpy(remote.host, c->addr, OV_HOST_NAME_MAX);
  remote.port = c->port;

  OV_ASSERT(stream->session);
  OV_ASSERT(stream->session->proxy);

  Stream *set = get_stream_by_remote(stream->session->proxy, &remote);

  if (set) {

    if (set == stream)
      return true;

    ov_log_debug("Candidate %s:%i already registered for different stream.",
                 c->addr, c->port);

    goto error;
  }

  Pair *pair = get_pair_by_candidate(stream, c);
  if (pair)
    return true;

  pair = pair_create(stream);
  pair->remote.socket.port = c->port;
  memcpy(pair->remote.socket.host, c->addr, OV_HOST_NAME_MAX);
  pair->remote.priority = c->priority;
  register_pair(pair);

  pair_calculate_local_priority(pair);
  pair_calculate_priority(pair);

  ov_log_debug("%s added pair for %s:%i", stream->uuid, c->addr, c->port);

  return add_stream_by_remote(stream->session->proxy, stream, &remote);

error:
  return false;
}

/*----------------------------------------------------------------------------*/

static bool ov_ice_proxy_session_update(ov_ice_proxy_generic *generic,
                                        const char *session_id,
                                        const ov_sdp_session *sdp) {

  ov_ice_proxy *self = as_ice_proxy(generic);

  if (!self || !session_id || !sdp)
    goto error;

  Session *session = ov_dict_get(self->sessions, session_id);
  if (!session)
    goto error;

  if (!sdp_verify_input(session, sdp))
    goto error;

  const char *fingerprint = NULL;
  const char *fingerprint_stream = NULL;
  const char *fingerprint_session = NULL;

  uint32_t ssrc = 0;
  const char *cname = NULL;

  ov_sdp_list *attr = NULL;
  ov_sdp_list *next = NULL;

  const char *ptr = NULL;

  bool trickle_session = false;
  bool trickle_stream = false;

  const char *user_session = NULL;
  const char *pass_session = NULL;
  const char *user = NULL;
  const char *pass = NULL;
  const char *setup = NULL;
  const char *end_of_candidates = NULL;

  fingerprint_session =
      ov_sdp_attribute_get(sdp->attributes, OV_KEY_FINGERPRINT);

  user_session = ov_sdp_attribute_get(sdp->attributes, OV_ICE_STRING_USER);
  pass_session = ov_sdp_attribute_get(sdp->attributes, OV_ICE_STRING_PASS);

  if (!check_ice_trickle(sdp->attributes, &trickle_session))
    goto error;

  ov_sdp_description *desc = sdp->description;
  Stream *stream = session->streams;

  while (desc) {

    user = NULL;
    pass = NULL;
    setup = NULL;
    cname = NULL;
    fingerprint_stream = NULL;
    end_of_candidates = NULL;
    ssrc = 0;

    next = desc->attributes;
    while (next) {

      attr = next;
      next = ov_node_next(next);

      if (!user && (0 == strcmp(attr->key, OV_ICE_STRING_USER))) {
        user = attr->value;
        continue;
      }

      if (!pass && (0 == strcmp(attr->key, OV_ICE_STRING_PASS))) {
        pass = attr->value;
        continue;
      }

      if (!setup && (0 == strcmp(attr->key, OV_KEY_SETUP))) {
        setup = attr->value;
        continue;
      }

      if (!fingerprint_stream && (0 == strcmp(attr->key, OV_KEY_FINGERPRINT))) {
        fingerprint_stream = attr->value;
        continue;
      }

      if (!trickle_stream && (0 == strcmp(attr->key, OV_ICE_STRING_OPTIONS))) {

        if (NULL == attr->value)
          goto error;

        if (0 != strcmp(attr->value, OV_ICE_STRING_TRICKLE))
          goto error;

        trickle_stream = true;
        continue;
      }

      if (!cname && (0 == strcmp(attr->key, OV_KEY_SSRC))) {

        if (!attr->value)
          goto error;

        cname = memchr(attr->value, ' ', strlen(attr->value));
        if (!cname)
          goto error;

        if (!ov_convert_string_to_uint32(attr->value, (cname - attr->value),
                                         &ssrc))
          goto error;

        cname++;

        ptr = memchr(cname, ':', strlen(attr->value) - (cname - attr->value));
        if (!ptr) {
          cname = NULL;
          ssrc = 0;
          continue;
        }

        if (0 == strncmp(cname, OV_ICE_STRING_CNAME, ptr - cname)) {
          cname = ptr + 1;
        } else {
          cname = NULL;
          ssrc = 0;
        }
      }

      if (!end_of_candidates &&
          (0 == strcmp(attr->key, OV_ICE_STRING_END_OF_CANDIDATES))) {
        end_of_candidates = attr->key;
        continue;
      }
    }

    /* Step2 - check required attributes are set */

    if (!trickle_session && !trickle_stream)
      goto error;

    if (!user) {

      if (pass)
        goto error;

      if (!user_session || !pass_session)
        goto error;

      user = user_session;
      pass = pass_session;

    } else {

      if (!pass)
        goto error;
    }

    if (fingerprint_stream) {
      fingerprint = fingerprint_stream;
    } else {
      fingerprint = fingerprint_session;
    }

    if (!setup || !fingerprint)
      goto error;

    if (0 == ssrc)
      goto error;

    stream->remote.ssrc = ssrc;
    if (end_of_candidates)
      stream->remote.gathered = true;

    strncpy(stream->remote.user, user, OV_ICE_UFRAG_MAX);
    strncpy(stream->remote.pass, pass, OV_ICE_PASS_MAX);
    strncpy(stream->remote.fingerprint, fingerprint,
            OV_ICE_DTLS_FINGERPRINT_MAX);

    /* Step3 - add ICE candidates */

    const char *value = NULL;

    while (ov_sdp_attributes_iterate(&desc->attributes, OV_ICE_STRING_CANDIDATE,
                                     &value)) {

      ov_ice_candidate *c = ov_ice_candidate_from_string(value, strlen(value));
      if (!c)
        continue;

      stream_add_candidate(stream, c);
      c = ov_ice_candidate_free(c);
    }

    if (end_of_candidates)
      stream->remote.gathered = true;

    stream_order(stream);
    stream_prune(stream);

    desc = ov_node_next(desc);
    stream = ov_node_next(stream);
  }

  return true;

error:
  return false;
}

/*----------------------------------------------------------------------------*/

static bool
ov_ice_proxy_stream_candidate_in(ov_ice_proxy_generic *generic,
                                 const char *session_id, uint32_t stream_id,
                                 const ov_ice_candidate *candidate) {

  ov_ice_proxy *self = as_ice_proxy(generic);
  if (!self || !session_id || !candidate)
    goto error;

  Session *session = ov_dict_get(self->sessions, session_id);
  if (!session)
    goto error;

  OV_ASSERT(session->proxy);
  OV_ASSERT(session->proxy == self);

  Stream *stream = ov_node_get(session->streams, stream_id + 1);
  if (!stream)
    goto error;

  OV_ASSERT(stream->session == session);
  OV_ASSERT(stream->session->proxy == self);

  return stream_add_candidate(stream, candidate);

error:
  return false;
}

/*----------------------------------------------------------------------------*/

static bool ov_ice_proxy_stream_end_of_candidates_in(
    ov_ice_proxy_generic *generic, const char *session_id, uint32_t stream_id) {

  ov_ice_proxy *self = as_ice_proxy(generic);
  if (!self || !session_id)
    goto error;

  Session *session = ov_dict_get(self->sessions, session_id);
  if (!session)
    goto error;

  Stream *stream = ov_node_get(session->streams, stream_id + 1);
  if (!stream)
    goto error;

  stream->remote.gathered = true;
  return true;

error:
  return false;
}

/*----------------------------------------------------------------------------*/

static uint32_t ov_ice_proxy_stream_get_ssrc(ov_ice_proxy_generic *generic,
                                             const char *uuid,
                                             uint32_t stream_id) {

  ov_ice_proxy *self = as_ice_proxy(generic);
  if (!self || !uuid)
    goto error;

  Session *session = ov_dict_get(self->sessions, uuid);
  if (!session)
    goto error;

  Stream *stream = ov_node_get(session->streams, stream_id + 1);
  if (!stream)
    goto error;

  return stream->local.ssrc;
error:
  return 0;
}

/*----------------------------------------------------------------------------*/

static ssize_t ov_ice_proxy_stream_send(ov_ice_proxy_generic *generic,
                                        const char *session_id,
                                        uint32_t stream_id, uint8_t *buffer,
                                        size_t bytes) {

  ov_ice_proxy *self = as_ice_proxy(generic);
  if (!self || !session_id || !buffer || !bytes)
    goto error;

  Session *session = ov_dict_get(self->sessions, session_id);
  if (!session)
    goto error;

  Stream *stream = ov_node_get(session->streams, stream_id + 1);
  if (!stream)
    goto error;

  if (!stream->selected)
    goto error;
  if (!session->srtp.session)
    goto error;

  srtp_t srtp_session = session->srtp.session;
  if (!srtp_session)
    goto error;

  uint32_t u32 = htonl(stream->local.ssrc);
  memcpy(buffer + 8, &u32, 4);

  // we set payload type to format
  buffer[1] = 0x80 & buffer[1];
  buffer[1] |= (uint8_t)stream->format;

  int out = bytes;

  srtp_err_status_t r = srtp_protect(srtp_session, buffer, &out);

  switch (r) {

  case srtp_err_status_ok:
    break;

  default:
    ov_log_error("SRTP protect error %i", r);
    goto error;
    break;
  }

  return pair_send(stream->selected, buffer, out);

error:
  return -1;
}

/*----------------------------------------------------------------------------*/

ov_ice_proxy_generic *
ov_ice_proxy_multiplexing_create(ov_ice_proxy_generic_config config) {

  ov_ice_proxy *self = NULL;

  if (!ov_cond_valid(init_config(&config),
                     "Cannot create ICE Proxy - Config invalid"))
    goto error;

  self = calloc(1, sizeof(ov_ice_proxy));
  if (!self)
    goto error;

  self->public.magic_bytes = OV_ICE_PROXY_GENERIC_MAGIC_BYTES;
  self->public.type = OV_ICE_PROXY_MAGIC_BYTES;
  self->public.config = config;

  // init openssl
  SSL_library_init();
  SSL_load_error_strings();
  dtls_filter_init();

  if (!ov_cond_valid(configure_dtls(self),
                     "Cannot create ICE Proxy - Configuring DTLS failed"))
    goto error;

  if (!ov_cond_valid(
          create_fingerprint_cert(self->public.config.config.dtls.cert,
                                  OV_HASH_SHA256, self->dtls.fingerprint),
          "Cannot create ICE Proxy - Could not create fingerprint cert"))
    goto error;

  self->socket = ov_socket_create(self->public.config.external, false, NULL);

  if (-1 == self->socket) {

    ov_log_error("Could not open socket %s:%i",
                 self->public.config.external.host,
                 self->public.config.external.port);
    goto error;
  }
  ov_log_debug("opened socket %s:%i", self->public.config.external.host,
               self->public.config.external.port);

  uint8_t event = OV_EVENT_IO_IN | OV_EVENT_IO_ERR | OV_EVENT_IO_CLOSE;

  if (!ov_event_loop_set(config.loop, self->socket, event, self, io_external))
    goto error;

  ov_dict_config d_config = ov_dict_string_key_config(255);

  self->remote = ov_dict_create(d_config);
  if (!self->remote)
    goto error;

  self->streams = ov_dict_create(d_config);
  if (!self->streams)
    goto error;

  d_config.value.data_function.free = session_free;
  self->sessions = ov_dict_create(d_config);
  if (!self->sessions)
    goto error;

  d_config.value.data_function.free = transaction_free;
  self->transactions = ov_dict_create(d_config);
  if (!self->transactions)
    goto error;

  self->timer.transactions_invalidate = ov_event_loop_timer_set(
      config.loop, config.config.limits.transaction_lifetime_usecs, self,
      invalidate_transactions);

  if (OV_TIMER_INVALID == self->timer.transactions_invalidate)
    goto error;

  ov_socket_get_data(self->socket, &self->local, NULL);
  self->candidate.type = OV_ICE_HOST;
  self->candidate.transport = UDP;
  self->candidate.component_id = 1;
  self->candidate.priority = (1 << 24) * type_preference(OV_ICE_HOST) +
                             (1 << 8) * 65535 + (1) * (256 - 1);

  if (!ov_ice_string_fill_random((char *)self->candidate.foundation, 32))
    goto error;

  memcpy(self->candidate.addr, self->local.host, OV_HOST_NAME_MAX);
  self->candidate.port = self->local.port;

  self->candidate.string = ov_ice_candidate_to_string(&self->candidate);
  if (!self->candidate.string)
    goto error;

  self->public.free = ov_ice_proxy_free;

  self->public.session.create = ov_ice_proxy_create_session;
  self->public.session.drop = ov_ice_proxy_session_drop;
  self->public.session.update = ov_ice_proxy_session_update;

  self->public.stream.candidate_in = ov_ice_proxy_stream_candidate_in;
  self->public.stream.end_of_candidates_in =
      ov_ice_proxy_stream_end_of_candidates_in;
  self->public.stream.get_ssrc = ov_ice_proxy_stream_get_ssrc;
  self->public.stream.send = ov_ice_proxy_stream_send;

  srtp_init();

  return ov_ice_proxy_generic_cast(self);
error:
  ov_ice_proxy_free(ov_ice_proxy_generic_cast(self));
  return NULL;
}
