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
        @file           ov_ice_stream.c
        @author         Markus

        @date           2024-09-18


        ------------------------------------------------------------------------
*/
#include "../include/ov_ice_stream.h"

#include <ov_base/ov_linked_list.h>
#include <ov_base/ov_random.h>

/*----------------------------------------------------------------------------*/

static uint32_t create_ssrc() {

  uint32_t ssrc = ov_random_uint32();

  while (ssrc <= OV_MAX_ANALOG_SSRC) {

    ssrc = ov_random_uint32();
  }

  return ssrc;
}

/*----------------------------------------------------------------------------*/

ov_ice_stream *ov_ice_stream_create(ov_ice_session *session, int index) {

  ov_ice_stream *stream = NULL;
  if (!session)
    goto error;

  stream = calloc(1, sizeof(ov_ice_stream));
  if (!stream)
    goto error;

  stream->node.type = OV_ICE_STREAM_MAGIC_BYTES;
  stream->session = session;
  stream->index = index;
  stream->type = OV_ICE_DTLS_PASSIVE;

  if (!ov_node_push((void **)&session->streams, stream))
    goto error;

  stream->local.ssrc = create_ssrc();
  ov_id_fill_with_uuid(stream->uuid);

  stream->valid = ov_linked_list_create((ov_list_config){0});
  stream->trigger = ov_linked_list_create((ov_list_config){0});
  if (!stream->valid || !stream->trigger)
    goto error;

  if (!ov_ice_string_fill_random(stream->local.pass, OV_ICE_STUN_PASS_MIN))
    goto error;

  if (!ov_ice_gather_candidates(session->ice, stream))
    goto error;

  stream->state = OV_ICE_RUNNING;
  return stream;
error:
  ov_ice_stream_free(stream);
  return NULL;
}

/*----------------------------------------------------------------------------*/

ov_ice_stream *ov_ice_stream_cast(const void *data) {

  if (!data)
    goto error;

  ov_node *node = (ov_node *)data;

  if (node->type == OV_ICE_STREAM_MAGIC_BYTES)
    return (ov_ice_stream *)data;
error:
  return NULL;
}

/*----------------------------------------------------------------------------*/

void *ov_ice_stream_free(void *self) {

  ov_ice_stream *stream = ov_ice_stream_cast(self);
  if (!stream)
    return self;

  if (stream->session) {

    ov_node_remove_if_included((void **)&stream->session->streams, stream);
  }

  if (OV_TIMER_INVALID != stream->timer.nominate) {

    ov_event_loop_timer_unset(ov_ice_get_event_loop(stream->session->ice),
                              stream->timer.nominate, NULL);

    stream->timer.nominate = OV_TIMER_INVALID;
  }

  stream->valid = ov_list_free(stream->valid);
  stream->trigger = ov_list_free(stream->trigger);

  ov_ice_base *base = NULL;

  while (stream->bases) {

    base = ov_node_pop((void **)&stream->bases);
    base = ov_ice_base_free(base);
  }

  ov_ice_pair *pair = NULL;

  while (stream->pairs) {

    pair = ov_node_pop((void **)&stream->pairs);
    pair = ov_ice_pair_free(pair);
  }

  ov_ice_candidate *candidate = NULL;

  while (stream->candidates.local) {

    candidate = ov_node_pop((void **)&stream->candidates.local);
    candidate = ov_ice_candidate_free(candidate);
  }

  while (stream->candidates.remote) {

    candidate = ov_node_pop((void **)&stream->candidates.remote);
    candidate = ov_ice_candidate_free(candidate);
  }

  stream = ov_data_pointer_free(stream);
  return NULL;
}

/*----------------------------------------------------------------------------*/

static ov_ice_candidate *
ov_ice_stream_select_default_candidate(const ov_ice_stream *stream) {

  OV_ASSERT(stream);
  if (!stream)
    goto error;

  ov_ice_candidate *candidate = stream->candidates.local;
  ov_ice_candidate *selected = NULL;

  while (candidate) {

    if (candidate->gathering != OV_ICE_GATHERING_SUCCESS) {

      candidate = ov_node_next(candidate);
      continue;
    }

    if (!selected) {
      selected = candidate;
      candidate = ov_node_next(candidate);
      continue;
    }

    switch (candidate->type) {

    case OV_ICE_RELAYED:

      if (selected->type != OV_ICE_RELAYED)
        selected = candidate;

      break;

    case OV_ICE_PEER_REFLEXIVE:

      selected = candidate;
      break;

    case OV_ICE_SERVER_REFLEXIVE:

      if (selected->type == OV_ICE_HOST)
        selected = candidate;

      break;

    case OV_ICE_HOST:

      if (selected->type == OV_ICE_INVALID)
        selected = candidate;

      break;

    default:
      break;
    }

    candidate = ov_node_next(candidate);
  }

  return selected;
error:
  return NULL;
}

/*----------------------------------------------------------------------------*/

ov_sdp_connection ov_ice_stream_get_connection(ov_ice_stream *stream,
                                               ov_sdp_description *desc) {

  ov_sdp_connection c = {0};

  if (!stream || !desc)
    goto error;

  ov_ice_candidate *candidate = ov_ice_stream_select_default_candidate(stream);
  OV_ASSERT(candidate);

  switch (candidate->base->local.data.sa.ss_family) {

  case AF_INET:
    c.addrtype = "IP4";
    break;

  case AF_INET6:
    c.addrtype = "IP6";
    break;

  default:
    goto error;
  }

  desc->media.port = candidate->port;
  c.nettype = "IN";
  c.address = candidate->addr;

  return c;
error:
  return (ov_sdp_connection){0};
}

/*----------------------------------------------------------------------------*/

bool ov_ice_stream_candidate(ov_ice_stream *stream, const ov_ice_candidate *c) {

  if (!stream || !c)
    goto error;

  /*
   *      Check input candidate,
   *      first check additional ov_ice_candidate functions,
   *      to avoid to add some wrong candidate
   */

  if (c->transport != UDP)
    if (c->transport != DTLS)
      goto done;

  if (OV_ICE_INVALID == c->type)
    goto error;

  if (0 == c->addr[0])
    goto error;

  if (0 == c->foundation[0])
    goto error;

  if (0 != c->server.socket.host[0])
    goto error;

  if (0 == c->component_id)
    goto error;

  /*
   *      Finaly check of the candidate is already included.
   */

  ov_ice_candidate *next = stream->candidates.remote;
  ov_ice_candidate *test = NULL;
  while (next) {

    test = next;
    next = ov_node_next(next);

    if (test == c)
      return true;

    if (test->port != c->port)
      continue;

    if (0 != strncmp(test->addr, c->addr, OV_HOST_NAME_MAX))
      continue;

    if (test->rport != c->rport)
      continue;

    if (0 != test->raddr[0]) {

      if (0 != c->raddr[0])
        if (0 != strncmp(test->raddr, c->raddr, OV_HOST_NAME_MAX))
          continue;
    }

    return true;
  }

  ov_ice_candidate *candidate = ov_ice_candidate_create(stream);
  *candidate = *c;

  candidate->node.type = OV_ICE_CANDIDATE_MAGIC_BYTES;
  candidate->node.next = NULL;
  candidate->node.prev = NULL;

  ov_node_push((void **)&stream->candidates.remote, candidate);

  /*
   *      The new remote candidate MUST be paired with the local
   *      candidates of the component.
   */

  ov_ice_candidate *local = stream->candidates.local;
  ov_ice_pair *pair = NULL;

  while (local) {

    if (local->component_id != candidate->component_id) {
      local = ov_node_next(local);
      continue;
    }

    /*
     *      Try to create a new pair,
     *      this is expected to fail,
     *      if remote and local use
     *      different IP versions.
     */

    pair = ov_ice_pair_create(stream, local, candidate);

    if (!pair) {
      local = ov_node_next(local);
      continue;
    }

    /*
     *      Find pairs with the same foundation.
     */

    pair->state = OV_ICE_PAIR_FROZEN;

    if (!ov_ice_session_set_state_on_foundation(stream->session, pair)) {
      pair = ov_ice_pair_free(pair);
      goto error;
    }

    local = ov_node_next(local);
  }

  if (!ov_ice_stream_order(stream))
    goto error;

  if (!ov_ice_stream_prune(stream))
    goto error;

  /*
   *      We do not limit the amount of pairs here.
   */

done:
  return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

bool ov_ice_stream_end_of_candidates(ov_ice_stream *stream) {

  if (!stream)
    return false;
  stream->remote.gathered = true;
  return true;
}

/*----------------------------------------------------------------------------*/

uint32_t ov_ice_stream_get_stream_ssrc(ov_ice_stream *stream) {

  if (!stream)
    return 0;
  return stream->local.ssrc;
}

/*----------------------------------------------------------------------------*/

ssize_t ov_ice_stream_send_stream(ov_ice_stream *stream, const uint8_t *buffer,
                                  size_t size) {

  if (!stream || !buffer || !size)
    goto error;
  if (!stream->selected)
    goto error;

  return ov_ice_pair_send(stream->selected, buffer, size);
error:
  return -1;
}

/*----------------------------------------------------------------------------*/

bool ov_ice_stream_set_active(ov_ice_stream *stream, const char *fingerprint) {

  if (!stream || !fingerprint)
    return false;

  memcpy(stream->remote.fingerprint, fingerprint, OV_ICE_DTLS_FINGERPRINT_MAX);

  stream->type = OV_ICE_DTLS_ACTIVE;
  return true;
}

/*----------------------------------------------------------------------------*/

bool ov_ice_stream_set_passive(ov_ice_stream *stream) {

  if (!stream)
    return false;
  stream->type = OV_ICE_DTLS_PASSIVE;
  return true;
}

/*----------------------------------------------------------------------------*/

bool ov_ice_stream_order(ov_ice_stream *stream) {

  if (!stream)
    goto error;

  ov_ice_pair *pair = NULL;
  ov_ice_pair *next = stream->pairs;
  ov_ice_pair *walk = NULL;

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

static bool pairs_are_redundant(ov_ice_pair *pair, ov_ice_pair *check) {

  OV_ASSERT(pair);
  OV_ASSERT(check);

  if (!pair || !check)
    return false;

  if (pair == check)
    return false;

  if (pair->local != check->local)
    return false;

  if (pair->remote != check->remote)
    return false;

  return true;
}

/*----------------------------------------------------------------------------*/

bool ov_ice_stream_prune(ov_ice_stream *stream) {

  OV_ASSERT(stream);
  if (!stream)
    goto error;

  ov_ice_pair *pair = stream->pairs;
  ov_ice_pair *next = NULL;
  ov_ice_pair *drop = NULL;

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

      if (pairs_are_redundant(pair, drop))
        drop = ov_ice_pair_free(drop);
    }

    pair = ov_node_next(pair);
  }

  return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

bool ov_ice_stream_trickle_candidates(ov_ice_stream *stream) {

  bool all_trickled = true;

  if (!stream)
    goto error;

  if (!stream->session->trickling_started)
    goto done;

  ov_ice *ice = stream->session->ice;

  ov_ice_candidate *candidate = stream->candidates.local;
  while (candidate) {

    if (candidate->trickled) {

      candidate = ov_node_next(candidate);
      continue;
    }

    switch (candidate->gathering) {

    case OV_ICE_GATHERING_SUCCESS:

      if (NULL == candidate->string)
        candidate->string = ov_ice_candidate_to_string(candidate);

      ov_ice_trickle_candidate(ice, stream->session->uuid, stream->uuid,
                               stream->index, candidate);

      candidate->trickled = true;

      break;
    default:
      all_trickled = false;
      break;
    }

    candidate = ov_node_next(candidate);
  }

  if (all_trickled) {

    stream->local.gathered = true;

    ov_ice_trickle_end_of_candidates(ice, stream->session->uuid, stream->index);
  }

done:
  return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

static bool pair_new_remote_gathered(ov_ice_stream *stream,
                                     ov_ice_candidate *candidate) {

  if (!stream || !candidate)
    goto error;

  ov_ice_candidate *remote = stream->candidates.remote;
  ov_ice_pair *pair = NULL;

  while (remote) {

    if (candidate->component_id != remote->component_id) {
      remote = ov_node_next(remote);
      continue;
    }

    pair = ov_ice_pair_create(stream, candidate, remote);

    if (!pair) {
      remote = ov_node_next(remote);
      continue;
    }

    switch (candidate->type) {

    case OV_ICE_SERVER_REFLEXIVE:
      break;

    case OV_ICE_TURN_SERVER:

      if (!ov_ice_pair_create_turn_permission(pair))
        goto error;

      break;

    default:
      OV_ASSERT(1 == 0);
      goto error;
    };

    pair->state = OV_ICE_PAIR_WAITING;

    if (!ov_ice_session_set_state_on_foundation(stream->session, pair)) {
      pair = ov_ice_pair_free(pair);
    }

    if (0 == ov_list_get_pos(stream->trigger, pair))
      ov_list_push(stream->trigger, pair);

    remote = ov_node_next(remote);
  }

  if (!ov_ice_stream_order(stream))
    goto error;

  if (!ov_ice_stream_prune(stream))
    goto error;

  return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

bool ov_ice_stream_process_remote_gathered(ov_ice_stream *stream,
                                           ov_ice_candidate *candidate) {

  if (!stream || !candidate)
    goto error;

  switch (candidate->type) {

  case OV_ICE_SERVER_REFLEXIVE:
  case OV_ICE_RELAYED:
    break;
  default:
    OV_ASSERT(1 == 0);
    goto error;
  };

  ov_ice_stream_trickle_candidates(stream);
  pair_new_remote_gathered(stream, candidate);
  ov_ice_stream_update(stream);

  return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

bool ov_ice_stream_update(ov_ice_stream *stream) {

  if (!stream)
    goto error;

  if ((OV_ICE_COMPLETED == stream->stun) &&
      (OV_ICE_COMPLETED == stream->dtls) && (OV_ICE_COMPLETED == stream->srtp))
    stream->state = OV_ICE_COMPLETED;

  if (!stream->local.gathered) {

    ov_ice_stream_trickle_candidates(stream);
    return true;

  } else if (stream->remote.gathered) {

    bool all_pairs_failed = true;

    ov_ice_pair *pair = stream->pairs;
    while (pair) {

      if (pair->state != OV_ICE_PAIR_FAILED) {
        all_pairs_failed = false;
        break;
      }

      pair = ov_node_next(pair);
    }

    if (all_pairs_failed) {
      stream->state = OV_ICE_FAILED;
      goto done;
    }
  }

  ov_ice_pair *pair = NULL;
  ov_ice_pair *valid = NULL;
  ov_ice_pair *trash = NULL;
  ov_ice_pair *selected = NULL;

  void *next = stream->valid->iter(stream->valid);
  while (next) {

    next = stream->valid->next(stream->valid, next, (void **)&valid);

    if (valid->nominated) {

      /*
       *      Remove all frozen and waiting
       *      pairs for the same component.
       */

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
        trash = ov_ice_pair_free(trash);
      }

    } else if (stream->session->controlling &&
               stream->session->nominate_started) {

      /*
       *  We nominate a pair after some successfull exchanges.
       */

      if (valid->nominated) {
        selected = NULL;
        break;
      }

      if (valid->success_count >= 5) {

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
  }

done:
  return true;

error:
  return false;
}

/*----------------------------------------------------------------------------*/

bool ov_ice_stream_sort_priority(ov_ice_stream *stream) {

  if (!stream)
    goto error;

  ov_ice_candidate *next = stream->candidates.local;
  ov_ice_candidate *cand = NULL;
  ov_ice_candidate *walk = NULL;

  while (next) {

    cand = next;
    next = ov_node_next(next);

    walk = stream->candidates.local;
    while (walk) {

      if (walk == cand)
        break;

      if (walk->priority < cand->priority) {

        if (!ov_node_insert_before((void **)&stream->candidates.local, cand,
                                   walk))
          goto error;

        break;
      }

      walk = ov_node_next(walk);
    }
  }

  return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

bool ov_ice_stream_recalculate_priorities(ov_ice_stream *stream) {

  if (!stream)
    goto error;

  ov_ice_pair *pair = stream->pairs;
  while (pair) {

    if (!ov_ice_pair_calculate_priority(pair, stream->session))
      goto error;

    pair = ov_node_next(pair);
  }

  return true;
error:
  return false;
}

/*----------------------------------------------------------------------------*/

bool ov_ice_stream_cb_selected_srtp_ready(ov_ice_stream *stream) {

  int r = 0;
  if (!stream)
    goto error;

  srtp_t srtp_session = stream->session->srtp.session;
  if (!srtp_session)
    goto error;

  stream->srtp = OV_ICE_INIT;

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

  case OV_ICE_DTLS_ACTIVE:

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

  case OV_ICE_DTLS_PASSIVE:

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

  stream->srtp = OV_ICE_COMPLETED;

  ov_log_info("ICE %s|%i completed srtp.", stream->session->uuid,
              stream->index);

  return true;
error:
  if (stream)
    stream->srtp = OV_ICE_FAILED;
  return false;
}

/*----------------------------------------------------------------------------*/

bool ov_ice_stream_gathering_stop(ov_ice_stream *stream) {

  if (!stream)
    goto error;

  ov_ice_candidate *next = stream->candidates.local;
  ov_ice_candidate *cand = NULL;

  while (next) {

    cand = next;
    next = ov_node_next(next);

    if (OV_ICE_GATHERING_SUCCESS != cand->gathering) {

      ov_log_debug("Dropping candidate type %s addr %s:%i raddr %s:%i",
                   ov_ice_candidate_type_to_string(cand->type), cand->addr,
                   cand->port, cand->raddr, cand->rport);

      cand = ov_ice_candidate_free(cand);
    }
  }

  ov_ice_stream_trickle_candidates(stream);

  return true;
error:
  return false;
}