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
        @file           ov_ice_base.c
        @author         Markus

        @date           2024-09-18


        ------------------------------------------------------------------------
*/
#include "../include/ov_ice_base.h"

#include <ov_base/ov_dump.h>
#include <ov_base/ov_linked_list.h>

#include <ov_stun/ov_stun_attributes_rfc5245.h> // RFC ICE
#include <ov_stun/ov_stun_attributes_rfc5389.h> // RFC STUN
#include <ov_stun/ov_stun_attributes_rfc8489.h>
#include <ov_stun/ov_stun_attributes_rfc8656.h>
#include <ov_stun/ov_stun_binding.h>
#include <ov_stun/ov_stun_frame.h>
#include <ov_stun/ov_turn_allocate.h>
#include <ov_stun/ov_turn_channel_bind.h>
#include <ov_stun/ov_turn_create_permission.h>
#include <ov_stun/ov_turn_data.h>
#include <ov_stun/ov_turn_refresh.h>
#include <ov_stun/ov_turn_send.h>

#define IMPL_STUN_ATTR_FRAMES 50

/*----------------------------------------------------------------------------*/

static bool io_external_ssl(ov_ice_base *base,
                            const uint8_t *buffer,
                            size_t size,
                            const ov_socket_data *remote) {

    OV_ASSERT(base);
    OV_ASSERT(remote);

    if (!base || !remote) goto error;

    ov_ice_stream *stream = base->stream;

    ov_ice_pair *pair = stream->pairs;
    while (pair) {

        OV_ASSERT(pair->remote);
        OV_ASSERT(0 != pair->remote->addr[0]);

        if ((pair->remote->port == remote->port) &&
            (0 == strcmp(pair->remote->addr, remote->host))) {

            break;
        }

        pair = ov_node_next(pair);
    }

    if (!pair) {

        ov_log_error("got input from %s:%i, not a valid remote candidate",
                     remote->host,
                     remote->port);

        goto ignore;
    }

    if (!pair->dtls.handshaked) {

        if (ov_ice_debug_dtls(base->stream->session->ice))
            ov_log_debug("DTLS handshake at pair %p %s|%i %s %" PRIu64
                         " %s:%i <-> %s:%i %s",
                         pair,
                         base->stream->session->uuid,
                         base->stream->index,
                         ov_ice_candidate_type_to_string(pair->local->type),
                         pair->priority,
                         pair->local->addr,
                         pair->local->port,
                         pair->remote->addr,
                         pair->remote->port,
                         ov_ice_pair_state_to_string(pair->state));

        return ov_ice_pair_handshake_passive(pair, buffer, size);
    }

    return ov_ice_pair_dtls_io(pair, buffer, size);

ignore:
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool io_external_rtp(ov_ice_base *base,
                            uint8_t *buffer,
                            size_t size,
                            const ov_socket_data *remote) {

    if (!base || !buffer || !size || !remote) goto error;

    ov_ice_stream *stream = base->stream;
    ov_ice_config config = ov_ice_get_config(stream->session->ice);

    int l = size;

    srtp_t srtp_session = stream->session->srtp.session;

    if (!srtp_session) goto error;

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

    if (config.callbacks.stream.io)
        config.callbacks.stream.io(config.callbacks.userdata,
                                   stream->session->uuid,
                                   stream->index,
                                   buffer,
                                   size);
ignore:
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool set_unique_remote_foundation(ov_ice_candidate *candidate,
                                         ov_ice_stream *stream) {

    OV_ASSERT(candidate);
    OV_ASSERT(stream);

    if (!candidate || !stream) goto error;

    ov_ice_candidate *next = NULL;
    bool unique = true;

    do {

        unique = true;

        if (!ov_ice_string_fill_random((char *)candidate->foundation, 32))
            goto error;

        next = stream->candidates.remote;

        while (next) {

            if (0 == memcmp(next->foundation, candidate->foundation, 32)) {

                unique = false;
                break;
            }

            next = ov_node_next(next);
        }

    } while (!unique);

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool io_process_stun_request(ov_ice_base *base,
                                    const ov_socket_data *remote,
                                    const uint8_t *buffer,
                                    size_t length,
                                    uint8_t *attr[],
                                    size_t attr_size) {

    size_t name_length = OV_UDP_PAYLOAD_OCTETS;
    uint8_t name[OV_UDP_PAYLOAD_OCTETS];
    memset(name, 0, name_length);

    ov_list *pair_list = NULL;

    size_t response_length = OV_UDP_PAYLOAD_OCTETS;
    uint8_t response[OV_UDP_PAYLOAD_OCTETS];
    memset(response, 0, response_length);

    if (!base || !remote || !buffer || !length || !attr || !attr_size)
        goto error;

    ov_ice_pair *pair = NULL;
    ov_ice_stream *stream = base->stream;
    ov_ice_session *session = stream->session;
    ov_ice *ice = session->ice;
    pair_list = ov_linked_list_create((ov_list_config){0});

    /*
     *      Find existing pairs
     */

    pair = stream->pairs;
    while (pair) {

        OV_ASSERT(0 != pair->remote->addr[0]);

        if (pair->local->gathering != OV_ICE_GATHERING_SUCCESS) {

            pair = ov_node_next(pair);
            continue;
        }

        if ((0 == memcmp(pair->remote->addr, remote->host, OV_HOST_NAME_MAX)) &&
            (pair->remote->port == remote->port))
            ov_list_push(pair_list, pair);

        pair = ov_node_next(pair);
    }

    uint8_t *ptr = NULL;
    uint8_t *next = NULL;
    uint32_t priority = 0;

    /*
     *      Prepare required attribute frame pointers.
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

    if (!attr_username || !attr_integrity || !attr_priority) goto bad_request;

    if (!attr_controlling && !attr_controlled) goto bad_request;

    if (!ov_stun_ice_priority_decode(
            attr_priority, length - (attr_priority - buffer), &priority))
        goto bad_request;

    if (!ov_stun_username_decode(attr_username,
                                 length - (buffer - attr_username),
                                 &ptr,
                                 &name_length))
        goto error;

    /*
     *      Authenticate the request
     */

    const char *pass = NULL;

    uint8_t *colon = memchr(ptr, ':', name_length);
    if (!colon) goto unauthorized;

    ssize_t ufrag_in_len = colon - ptr;
    if (ufrag_in_len < 1) goto bad_request;

    if (0 != stream->uuid[0]) {

        if (ufrag_in_len != (ssize_t)strlen(stream->uuid)) goto unauthorized;

        if (0 != strncmp(stream->uuid, (char *)ptr, ufrag_in_len))
            goto unauthorized;

        pass = stream->local.pass;

    } else {

        goto unauthorized;
    }

    if (!pass) goto unauthorized;

    if (!ov_stun_check_message_integrity((uint8_t *)buffer,
                                         length,
                                         attr,
                                         attr_size,
                                         (uint8_t *)pass,
                                         strlen(pass),
                                         true))
        goto unauthorized;

    /*
     *      Check for role conflicts
     */

    uint64_t attr_tiebreaker = 0;

    if (session->controlling) {

        /*
         *      Session is controlling, conflict if
         *      attr_controlling is present.
         */

        if (attr_controlling) {

            if (!ov_stun_ice_controlling_decode(
                    attr_controlling,
                    length - (attr_controlling - buffer),
                    &attr_tiebreaker))
                goto bad_request;

            if (session->tiebreaker >= attr_tiebreaker) goto role_conflict;

            ov_ice_session_change_role(session, attr_tiebreaker);
        }

    } else {

        if (attr_controlled) {

            /*
             *      Session is NOT controlling,
             *      attr_controlled is present.
             */

            if (!ov_stun_ice_controlled_decode(
                    attr_controlled,
                    length - (attr_controlling - buffer),
                    &attr_tiebreaker))

                goto bad_request;

            if (session->tiebreaker < attr_tiebreaker) goto role_conflict;

            ov_ice_session_change_role(session, attr_tiebreaker);
        }
    }

    /*
     *      No conflicts, no errors,
     *      generate a success reponse
     *
     */

    if (!ov_stun_frame_set_success_response(response, response_length))
        goto error;

    if (!ov_stun_frame_set_magic_cookie(response, response_length)) goto error;

    if (!ov_stun_frame_set_method(response, response_length, STUN_BINDING))
        goto error;

    if (!ov_stun_frame_set_transaction_id(
            response,
            response_length,
            ov_stun_frame_get_transaction_id(buffer, length)))
        goto error;

    // add XOR MAPPED ADDRESS
    if (!ov_stun_xor_mapped_address_encode(
            response + 20, response_length - 20, response, &next, &remote->sa))
        goto error;

    if (!ov_stun_frame_set_length(
            response, response_length, (next - response) - 20))
        goto error;

    response_length = (next - response);

    // add a message integrity
    if (!ov_stun_add_message_integrity(response,
                                       OV_UDP_PAYLOAD_OCTETS,
                                       response + response_length,
                                       &next,
                                       (uint8_t *)pass,
                                       strlen(pass)))
        goto error;

    response_length = (next - response);

    if (ov_list_is_empty(pair_list)) {

        /*
         *      We received some peer reflexive
         *      remote candidate.
         */
        if (ov_ice_debug_stun(stream->session->ice))
            ov_log_debug("New PEER-REFLEXIVE candidate from %s:%i",
                         remote->host,
                         remote->port);

        ov_ice_candidate *local_candidate = ov_list_get(base->candidates, 1);
        ov_ice_candidate *remote_candidate = ov_ice_candidate_create(stream);

        if (!remote_candidate) goto error;

        remote_candidate->type = OV_ICE_PEER_REFLEXIVE;
        remote_candidate->priority = priority;
        remote_candidate->port = remote->port;
        remote_candidate->component_id = 1,
        memcpy(remote_candidate->addr, remote->host, OV_HOST_NAME_MAX);
        remote_candidate->transport = UDP;

        if (!set_unique_remote_foundation(remote_candidate, stream)) {
            remote_candidate = ov_ice_candidate_free(remote_candidate);
            goto error;
        }

        pair = ov_ice_pair_create(stream, local_candidate, remote_candidate);

        if (!pair) {
            remote_candidate = ov_ice_candidate_free(remote_candidate);
            goto error;
        }

        if (!ov_node_push(
                (void **)&stream->candidates.remote, remote_candidate)) {
            remote_candidate = ov_ice_candidate_free(remote_candidate);
            pair = ov_ice_pair_free(pair);
            goto error;
        }

        /*
         *      Set the pair to success to avoid pruning
         */

        pair->state = OV_ICE_PAIR_SUCCESS;

        if (!ov_ice_stream_order(stream)) {
            remote_candidate = ov_ice_candidate_free(remote_candidate);
            pair = ov_ice_pair_free(pair);
            goto error;
        }

        if (!ov_ice_stream_prune(stream)) {
            remote_candidate = ov_ice_candidate_free(remote_candidate);
            pair = ov_ice_pair_free(pair);
            goto error;
        }

        pair->state = OV_ICE_PAIR_WAITING;

        ov_list_push(pair_list, pair);
    }

    pair = ov_list_pop(pair_list);

    OV_ASSERT(pair);

    while (pair) {

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

                        if (OV_ICE_FAILED == stream->state)
                            stream->state = OV_ICE_RUNNING;

                    } else {
                        pair->state = OV_ICE_PAIR_FAILED;
                    }
                }

                break;
        }

        if (stream->session->controlling && attr_use_candidate) {

            pair->nominated = true;

            stream->selected = pair;
            stream->stun = OV_ICE_COMPLETED;

            ov_log_info(
                "ICE session %s|%i selected pair at %s:%i from %s:%i - state "
                "%s",
                stream->session->uuid,
                stream->index,
                pair->local->addr,
                pair->local->port,
                pair->remote->addr,
                pair->remote->port,
                ov_ice_pair_state_to_string(pair->state));

            if (stream->type == OV_ICE_DTLS_ACTIVE)
                ov_ice_pair_handshake_active(pair);
        }

        ov_ice_session_update(stream->session);
        ov_ice_session_checklists_run(stream->session);

        // set new length
        if (!ov_stun_attribute_set_length(
                response, OV_UDP_PAYLOAD_OCTETS, response_length - 20))
            goto error;

        // add a fingerprint
        if (!ov_stun_add_fingerprint(response,
                                     OV_UDP_PAYLOAD_OCTETS,
                                     response + response_length,
                                     NULL))
            goto error;

        response_length += ov_stun_fingerprint_encoding_length();

        ssize_t bytes = ov_ice_pair_send(pair, response, response_length);

        if (-1 == bytes) ov_log_error("Failed to send response.");

        if (ov_ice_debug_stun(ice))
            ov_log_debug("STUN request at %s %s:%i from %s:%i - state %s",
                         ov_ice_candidate_type_to_string(pair->local->type),
                         pair->local->addr,
                         pair->local->port,
                         pair->remote->addr,
                         pair->remote->port,
                         ov_ice_pair_state_to_string(pair->state));

        pair = ov_list_pop(pair_list);
    }

    ov_ice_session_update(stream->session);
    ov_ice_session_checklists_run(stream->session);
    goto done;

role_conflict:

    response_length = ov_stun_error_code_generate_response(
        buffer,
        length,
        response,
        response_length,
        ov_stun_error_code_set_ice_role_conflict);

    if (0 == response_length) goto error;

    goto send_response;

bad_request:

    response_length = ov_stun_error_code_generate_response(
        buffer,
        length,
        response,
        response_length,
        ov_stun_error_code_set_bad_request);

    if (0 == response_length) goto error;

    goto send_response;

unauthorized:

    response_length = ov_stun_error_code_generate_response(
        buffer,
        length,
        response,
        response_length,
        ov_stun_error_code_set_unauthorized);

    if (0 == response_length) goto error;

    goto send_response;

send_response:

    // set new length
    if (!ov_stun_attribute_set_length(
            response, OV_UDP_PAYLOAD_OCTETS, response_length - 20))
        goto error;

    // add a fingerprint
    if (!ov_stun_add_fingerprint(
            response, OV_UDP_PAYLOAD_OCTETS, response + response_length, NULL))
        goto error;

    response_length += ov_stun_fingerprint_encoding_length();

    ssize_t bytes = -1;

    /* If we don't have a pair the request was a direct request (NO TURN)
     * and matches a peer reflexive request, but may be missing some
     * attributes, for completeness we send the response direct at the
     * socket to the remote */

    socklen_t len = sizeof(struct sockaddr_in);
    if (remote->sa.ss_family == AF_INET6) len = sizeof(struct sockaddr_in6);

    bytes = sendto(base->socket,
                   response,
                   response_length,
                   0,
                   (struct sockaddr *)&remote->sa,
                   len);

    if (-1 == bytes) ov_log_error("Failed to send response.");

done:
    ov_list_free(pair_list);
    return true;
error:
    ov_list_free(pair_list);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool send_keepalive_to_server(uint32_t timer_id, void *data) {

    ov_ice_candidate *candidate = ov_ice_candidate_cast(data);
    if (!candidate) goto error;
    UNUSED(timer_id);

    ov_event_loop *loop =
        ov_ice_get_event_loop(candidate->base->stream->session->ice);

    if (!ov_ice_base_send_stun_binding_request(candidate->base, candidate))
        goto error;

    candidate->server.timer.keepalive =
        ov_event_loop_timer_set(loop,
                                candidate->server.limits.keepalive_time_usecs,
                                candidate,
                                send_keepalive_to_server);

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool io_process_stun_candidate_success_response(
    ov_ice_base *base,
    ov_ice_candidate *candidate,
    const ov_socket_data *remote,
    const uint8_t *buffer,
    size_t length,
    uint8_t *attr[],
    size_t attr_size) {

    if (!base || !candidate || !remote || !buffer || !length || !attr ||
        !attr_size)
        goto error;

    ov_event_loop *loop = ov_ice_get_event_loop(base->stream->session->ice);
    ov_ice_config config = ov_ice_get_config(base->stream->session->ice);

    if (OV_ICE_SERVER_REFLEXIVE != candidate->type) goto ignore;

    if (candidate->gathering == OV_ICE_GATHERING_SUCCESS) goto ignore;

    candidate->gathering = OV_ICE_GATHERING_SUCCESS;

    uint8_t *xmap =
        ov_stun_attributes_get_type(attr, attr_size, STUN_XOR_MAPPED_ADDRESS);

    if (!xmap) goto ignore;

    ov_socket_data xor_mapped = {0};
    struct sockaddr_storage *xor_ptr = &xor_mapped.sa;

    if (!ov_stun_xor_mapped_address_decode(
            xmap, length - (xmap - buffer), buffer, &xor_ptr))
        goto ignore;

    xor_mapped = ov_socket_data_from_sockaddr_storage(&xor_mapped.sa);
    memcpy(candidate->addr, xor_mapped.host, OV_HOST_NAME_MAX);
    candidate->port = xor_mapped.port;

    memcpy(candidate->raddr, base->local.data.host, OV_HOST_NAME_MAX);
    candidate->rport = base->local.data.port;

    if (OV_TIMER_INVALID == candidate->server.timer.keepalive) {

        candidate->server.timer.keepalive =
            loop->timer.set(loop,
                            config.limits.stun.keepalive_usecs,
                            candidate,
                            send_keepalive_to_server);
    }

    ov_ice_stream_process_remote_gathered(base->stream, candidate);

ignore:
    return true;

error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool io_process_stun_pair_success_response(ov_ice_base *base,
                                                  ov_ice_pair *pair,
                                                  const ov_socket_data *remote,
                                                  const uint8_t *buffer,
                                                  size_t length,
                                                  uint8_t *attr[],
                                                  size_t attr_size) {

    ov_ice_candidate *candidate = NULL;

    if (!base || !pair || !remote || !buffer || !length || !attr || !attr_size)
        goto error;

    /*
     *      Prepare required attribute frame pointers.
     */

    uint8_t *xmap =
        ov_stun_attributes_get_type(attr, attr_size, STUN_XOR_MAPPED_ADDRESS);

    uint8_t *prio = ov_stun_attributes_get_type(attr, attr_size, ICE_PRIORITY);

    if (!xmap) goto ignore;

    ov_socket_data xor_mapped = {0};
    struct sockaddr_storage *xor_ptr = &xor_mapped.sa;

    if (!ov_stun_xor_mapped_address_decode(
            xmap, length - (xmap - buffer), buffer, &xor_ptr))
        goto ignore;

    xor_mapped = ov_socket_data_from_sockaddr_storage(&xor_mapped.sa);

    uint32_t priority = 0;

    if (prio) {

        if (!ov_stun_ice_priority_decode(
                prio, length - (prio - buffer), &priority))
            goto ignore;
    }

    /*
     *      Check if we get a symetric response
     */

    if ((pair->remote->port != remote->port) ||
        (0 != strncmp(pair->remote->addr, remote->host, OV_HOST_NAME_MAX)))
        goto ignore;

    if ((xor_mapped.port != pair->local->port) ||
        (0 != memcmp(xor_mapped.host, pair->local->addr, OV_HOST_NAME_MAX))) {

        /*
         *      We received some PEER_REFLEXIVE
         *      response.
         */

        if (ov_ice_debug_stun(base->stream->session->ice))
            ov_log_debug("New PEER-REFLEXIVE candidate from %s:%i",
                         remote->host,
                         remote->port);

        candidate = ov_ice_candidate_create(base->stream);
        if (!candidate) goto ignore;

        candidate->type = OV_ICE_PEER_REFLEXIVE;
        candidate->base = base;
        candidate->component_id = 1;
        candidate->gathering = OV_ICE_GATHERING_SUCCESS;
        candidate->rport = base->local.data.port;
        memcpy(candidate->raddr, base->local.data.host, OV_HOST_NAME_MAX);
        candidate->port = xor_mapped.port;
        memcpy(candidate->addr, xor_mapped.host, OV_HOST_NAME_MAX);
        candidate->priority = priority;
        candidate->server.timer.keepalive = OV_TIMER_INVALID;

        if (!ov_ice_session_set_foundation(base->stream->session, candidate))
            goto error;

        if (!ov_node_push((void **)&base->stream->candidates.local, candidate))
            goto error;

        if (!ov_list_push(base->candidates, candidate)) goto error;

        ov_ice_candidate_calculate_priority(
            candidate, ov_ice_get_interfaces(base->stream->session->ice));

        if (!ov_ice_stream_sort_priority(base->stream)) goto error;

        pair = ov_ice_pair_create(base->stream, candidate, pair->remote);

        if (!pair) {
            candidate = ov_ice_candidate_free(candidate);
            goto ignore;
        };

        /*
         *      Set the pair to success to avoid pruning
         */

        pair->state = OV_ICE_PAIR_SUCCESS;

        if (!ov_ice_stream_order(base->stream)) goto error;

        if (!ov_ice_stream_prune(base->stream)) goto error;

        if (!ov_list_queue_push(base->stream->trigger, pair)) goto error;
    }

    pair->state = OV_ICE_PAIR_SUCCESS;
    pair->success_count++;
    pair->progress_count = 0;

    if (0 == ov_list_get_pos(base->stream->valid, pair)) {
        if (!ov_list_push(base->stream->valid, pair)) {
            pair->state = OV_ICE_PAIR_FAILED;
            goto error;
        }
    }

    /* add retriggered check to increase success count */
    if (0 == ov_list_get_pos(base->stream->trigger, pair))
        ov_list_queue_push(base->stream->trigger, pair);

    ov_ice_session_unfreeze_foundation(
        base->stream->session, pair->local->foundation);

    if (!base->stream->session->controlling) {

        if (pair->nominated) {

            base->stream->state = OV_ICE_COMPLETED;
        }

    } else {

        if (pair->nominated) {

            base->stream->selected = pair;
            base->stream->state = OV_ICE_COMPLETED;

            if (ov_ice_debug_stun(base->stream->session->ice))
                ov_log_debug(
                    "ICE session %s|%i selected pair at %s:%i from %s:%i - "
                    "state "
                    "%s",
                    base->stream->session->uuid,
                    base->stream->index,
                    base->local.data.host,
                    base->local.data.port,
                    pair->remote->addr,
                    pair->remote->port,
                    ov_ice_pair_state_to_string(pair->state));

            switch (base->stream->type) {

                case OV_ICE_DTLS_ACTIVE:

                    ov_ice_pair_handshake_active(pair);

                    break;
                default:
                    break;
            }

            base->stream->selected->state = OV_ICE_PAIR_SUCCESS;
            base->stream->state = OV_ICE_COMPLETED;
        }
    }

    ov_ice_session_update(base->stream->session);

ignore:
    return true;

error:
    if (pair) pair->state = OV_ICE_PAIR_FAILED;
    return false;
}

/*----------------------------------------------------------------------------*/

static bool io_process_stun_success_response(ov_ice_base *base,
                                             const ov_socket_data *remote,
                                             const uint8_t *buffer,
                                             size_t length,
                                             uint8_t *attr[],
                                             size_t attr_size) {

    if (!base || !remote || !buffer || !length || !attr || !attr_size)
        goto error;

    ov_ice *ice = base->stream->session->ice;

    void *data = ov_ice_transaction_get(
        ice, ov_stun_frame_get_transaction_id(buffer, length));

    if (ov_ice_candidate_cast(data))
        return io_process_stun_candidate_success_response(
            base, data, remote, buffer, length, attr, attr_size);

    if (ov_ice_pair_cast(data))
        return io_process_stun_pair_success_response(
            base, data, remote, buffer, length, attr, attr_size);

    if (ov_ice_debug_stun(ice))
        ov_log_debug("Transaction ID invalid - ignoring");

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool io_process_stun_candidate_error_response(
    ov_ice_base *base,
    ov_ice_candidate *candidate,
    const ov_socket_data *remote,
    const uint8_t *buffer,
    size_t length,
    uint8_t *attr[],
    size_t attr_size) {

    if (!candidate || !base || !remote || !buffer || !length || !attr ||
        !attr_size)
        goto error;

    candidate->gathering = OV_ICE_GATHERING_FAILED;

    return true;

error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool io_process_stun_pair_error_response(ov_ice_base *base,
                                                ov_ice_pair *pair,
                                                const ov_socket_data *remote,
                                                const uint8_t *buffer,
                                                size_t length,
                                                uint8_t *attr[],
                                                size_t attr_size) {

    if (!pair || !base || !remote || !buffer || !length || !attr || !attr_size)
        goto error;

    /*
     *      Prepare required attribute frame pointers.
     */

    uint8_t *error =
        ov_stun_attributes_get_type(attr, attr_size, STUN_ERROR_CODE);

    if (!error) goto ignore;

    uint16_t errorcode =
        ov_stun_error_code_decode_code(error, length - (error - buffer));

    /*
     *      Check role conflict
     */

    if (ICE_ROLE_CONFLICT == errorcode) {

        ov_ice_session_change_role(
            base->stream->session, base->stream->session->tiebreaker);

    } else {

        pair->state = OV_ICE_PAIR_FAILED;

        if (pair->nominated && base->stream->session->controlling) {

            pair->nominated = false;
            base->stream->state = OV_ICE_FAILED;
        }
    }

    ov_ice_session_update(base->stream->session);

ignore:
    return true;

error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool io_process_stun_error_response(ov_ice_base *base,
                                           const ov_socket_data *remote,
                                           const uint8_t *buffer,
                                           size_t length,
                                           uint8_t *attr[],
                                           size_t attr_size) {

    if (!base || !remote || !buffer || !length || !attr || !attr_size)
        goto error;

    ov_ice *ice = base->stream->session->ice;

    void *data = ov_ice_transaction_get(
        ice, ov_stun_frame_get_transaction_id(buffer, length));

    if (ov_ice_candidate_cast(data))
        return io_process_stun_candidate_error_response(
            base, data, remote, buffer, length, attr, attr_size);

    if (ov_ice_pair_cast(data))
        return io_process_stun_pair_error_response(
            base, data, remote, buffer, length, attr, attr_size);

    if (ov_ice_debug_stun(ice))
        ov_log_debug("Transaction ID invalid - ignoring");

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool io_base_stun_process(ov_ice_base *base,
                                 const ov_socket_data *remote,
                                 const uint8_t *buffer,
                                 size_t length,
                                 uint8_t *attr[],
                                 size_t attr_size) {

    if (!base || !remote || !buffer || !length || !attr || !attr_size)
        goto error;

    ov_ice *ice = base->stream->session->ice;

    /*
     *      (1)     Check the outer framing format,
     *              as well as the STUN method.
     */

    if (STUN_BINDING != ov_stun_frame_get_method(buffer, length)) goto ignore;

    /*
     *      (2)     Any indication is dropped silently.
     */

    if (ov_stun_frame_class_is_indication(buffer, length)) return true;

    /*
     *      (3)     We received some actual STUN request
     *              or response.
     */

    if (ov_stun_frame_class_is_request(buffer, length)) {

        if (ov_ice_debug_stun(ice))
            ov_log_debug("STUN binding request at %s:%i from %s:%i",
                         base->local.data.host,
                         base->local.data.port,
                         remote->host,
                         remote->port);

        return io_process_stun_request(
            base, remote, buffer, length, attr, attr_size);

    } else if (ov_stun_frame_class_is_success_response(buffer, length)) {

        if (ov_ice_debug_stun(ice))
            ov_log_debug("STUN binding success at %s:%i from %s:%i",
                         base->local.data.host,
                         base->local.data.port,
                         remote->host,
                         remote->port);

        return io_process_stun_success_response(
            base, remote, buffer, length, attr, attr_size);

    } else if (ov_stun_frame_class_is_error_response(buffer, length)) {

        if (ov_ice_debug_stun(ice))
            ov_log_debug("STUN binding error at %s:%i from %s:%i",
                         base->local.data.host,
                         base->local.data.port,
                         remote->host,
                         remote->port);

        return io_process_stun_error_response(
            base, remote, buffer, length, attr, attr_size);

    } else {

        /*
         *      We SHOULD never reach this point,
         *      as the request MUST be either
         *      indication, request or response.
         *
         *      Dump only for implementation testing.
         */

        ov_dump_binary_as_hex(stderr, (uint8_t *)buffer, length);

        OV_ASSERT(1 == 0);
    }

ignore:
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool io_base_turn_process_allocate(ov_ice_base *base,
                                          const ov_socket_data *remote,
                                          const uint8_t *buffer,
                                          size_t length,
                                          uint8_t *attr[],
                                          size_t attr_size) {

    if (!base || !remote || !buffer || !length || !attr || !attr_size)
        goto error;

    TODO("... to be implemented");
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool io_base_turn_process_refresh(ov_ice_base *base,
                                         const ov_socket_data *remote,
                                         const uint8_t *buffer,
                                         size_t length,
                                         uint8_t *attr[],
                                         size_t attr_size) {

    if (!base || !remote || !buffer || !length || !attr || !attr_size)
        goto error;

    TODO("... to be implemented");
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool io_base_turn_process_data(ov_ice_base *base,
                                      const ov_socket_data *remote,
                                      const uint8_t *buffer,
                                      size_t length,
                                      uint8_t *attr[],
                                      size_t attr_size) {

    if (!base || !remote || !buffer || !length || !attr || !attr_size)
        goto error;

    TODO("... to be implemented");
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool io_base_turn_process_create_permission(ov_ice_base *base,
                                                   const ov_socket_data *remote,
                                                   const uint8_t *buffer,
                                                   size_t length,
                                                   uint8_t *attr[],
                                                   size_t attr_size) {

    if (!base || !remote || !buffer || !length || !attr || !attr_size)
        goto error;

    TODO("... to be implemented");
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool io_base_turn_process_channel_bind(ov_ice_base *base,
                                              const ov_socket_data *remote,
                                              const uint8_t *buffer,
                                              size_t length,
                                              uint8_t *attr[],
                                              size_t attr_size) {

    if (!base || !remote || !buffer || !length || !attr || !attr_size)
        goto error;

    TODO("... to be implemented");
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool io_external_stun(ov_ice_base *base,
                             uint8_t *buffer,
                             size_t size,
                             const ov_socket_data *remote) {

    size_t attr_size = IMPL_STUN_ATTR_FRAMES;
    uint8_t *attr[attr_size];
    memset(attr, 0, attr_size * sizeof(uint8_t *));

    if (!base || !buffer || !size || !remote) goto error;

    uint16_t method = ov_stun_frame_get_method(buffer, size);

    if (!ov_stun_frame_is_valid(buffer, size)) goto ignore;

    if (!ov_stun_frame_has_magic_cookie(buffer, size)) goto ignore;

    if (!ov_stun_frame_slice(buffer, size, attr, attr_size)) goto ignore;

    if (!ov_stun_check_fingerprint(buffer, size, attr, attr_size, false))
        goto ignore;

    switch (method) {

        case STUN_BINDING:
            return io_base_stun_process(
                base, remote, buffer, size, attr, attr_size);

        case TURN_ALLOCATE:
            return io_base_turn_process_allocate(
                base, remote, buffer, size, attr, attr_size);

        case TURN_REFRESH:
            return io_base_turn_process_refresh(
                base, remote, buffer, size, attr, attr_size);

        case TURN_DATA:
            return io_base_turn_process_data(
                base, remote, buffer, size, attr, attr_size);

        case TURN_CREATE_PERMISSION:
            return io_base_turn_process_create_permission(
                base, remote, buffer, size, attr, attr_size);

        case TURN_CHANNEL_BIND:
            return io_base_turn_process_channel_bind(
                base, remote, buffer, size, attr, attr_size);

        default:

            ov_log_error("ignoring unrecognized STUN method %" PRIu16, method);
    }

ignore:
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool io_external(int socket, uint8_t events, void *userdata) {

    /*
     *      ANY IO recevied from external
     */

    uint8_t buffer[OV_UDP_PAYLOAD_OCTETS] = {0};

    ov_socket_data remote = {};
    socklen_t src_addr_len = sizeof(remote.sa);

    ov_ice_base *base = ov_ice_base_cast(userdata);

    if (!base) goto error;

    OV_ASSERT(socket == base->socket);

    if (socket < 0) goto error;

    if ((events & OV_EVENT_IO_CLOSE) || (events & OV_EVENT_IO_ERR)) {

        ov_log_error("ICE BASE %i - closing", socket);
        base = ov_ice_base_free(base);
        return true;

    } else if ((events & OV_EVENT_IO_OUT)) {

        OV_ASSERT(1 == 0);
        return true;
    }

    OV_ASSERT(events & OV_EVENT_IO_IN);

    /* PEEK read to check for content */

    ssize_t bytes = recvfrom(socket,
                             (char *)buffer,
                             OV_UDP_PAYLOAD_OCTETS,
                             0,
                             (struct sockaddr *)&remote.sa,
                             &src_addr_len);

    if (bytes < 1) goto error;

    if (!ov_socket_parse_sockaddr_storage(
            &remote.sa, remote.host, OV_HOST_NAME_MAX, &remote.port))
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

    if (buffer[0] <= 3) return io_external_stun(base, buffer, bytes, &remote);

    if (buffer[0] <= 63 && buffer[0] >= 20) {

        return io_external_ssl(base, buffer, bytes, &remote);
    }

    if (buffer[0] <= 191 && buffer[0] >= 128) {

        return io_external_rtp(base, buffer, bytes, &remote);
    }

    // ignore
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

ov_ice_base *ov_ice_base_create(ov_ice_stream *stream, int socket) {

    ov_ice_base *base = NULL;
    if (!stream || !socket) goto error;

    ov_event_loop *loop = ov_ice_get_event_loop(stream->session->ice);

    base = calloc(1, sizeof(ov_ice_base));
    if (!base) goto error;

    base->node.type = OV_ICE_BASE_MAGIC_BYTES;
    base->socket = socket;
    base->stream = stream;
    ov_id_fill_with_uuid(base->uuid);

    base->candidates = ov_linked_list_create((ov_list_config){0});

    if (!ov_socket_get_data(base->socket, &base->local.data, NULL)) goto error;

    if (!ov_node_push((void **)&stream->bases, base)) goto error;

    ov_ice_candidate *candidate = ov_ice_candidate_create(stream);
    if (!candidate) goto error;

    if (!ov_node_push((void **)&stream->candidates.local, candidate)) {
        candidate = ov_ice_candidate_free(candidate);
        goto error;
    }

    candidate->type = OV_ICE_HOST;
    if (!strncpy(candidate->addr, base->local.data.host, OV_HOST_NAME_MAX))
        goto error;

    candidate->transport = UDP;
    candidate->port = base->local.data.port;
    candidate->component_id = 1;
    candidate->base = base;
    candidate->server.timer.keepalive = OV_TIMER_INVALID;
    candidate->gathering = OV_ICE_GATHERING_SUCCESS;
    ov_ice_candidate_calculate_priority(
        candidate, ov_ice_get_interfaces(base->stream->session->ice));

    if (!ov_list_push(base->candidates, candidate)) goto error;

    uint8_t event = OV_EVENT_IO_IN | OV_EVENT_IO_ERR | OV_EVENT_IO_CLOSE;

    if (!loop->callback.set(loop, base->socket, event, base, io_external))
        goto error;

    if (!ov_ice_session_set_foundation(base->stream->session, candidate))
        goto error;

    ov_log_debug("created ICE base at %s:%i",
                 base->local.data.host,
                 base->local.data.port);

    return base;
error:
    ov_ice_base_free(base);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_ice_base *ov_ice_base_cast(const void *data) {

    if (!data) goto error;

    ov_node *node = (ov_node *)data;

    if (node->type == OV_ICE_BASE_MAGIC_BYTES) return (ov_ice_base *)data;
error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

void *ov_ice_base_free(void *self) {

    ov_ice_base *base = ov_ice_base_cast(self);
    if (!base) return self;

    ov_ice_candidate *candidate = ov_list_pop(base->candidates);
    while (candidate) {

        candidate = ov_ice_candidate_free(candidate);
        candidate = ov_list_pop(base->candidates);
    }

    base->candidates = ov_list_free(base->candidates);
    base = ov_data_pointer_free(base);
    return NULL;
}

/*----------------------------------------------------------------------------*/

bool ov_ice_base_send_stun_binding_request(ov_ice_base *base,
                                           ov_ice_candidate *candidate) {

    uint8_t buffer[OV_UDP_PAYLOAD_OCTETS] = {0};
    uint8_t *ptr = buffer;
    uint8_t *nxt = NULL;

    if (!base || !candidate) goto error;

    if (candidate->base != base) goto error;

    if (!ov_ice_transaction_create(base->stream->session->ice,
                                   candidate->transaction_id,
                                   13,
                                   candidate))
        goto error;

    if (!ov_stun_generate_binding_request_plain(ptr,
                                                OV_UDP_PAYLOAD_OCTETS,
                                                &nxt,
                                                candidate->transaction_id,
                                                NULL,
                                                0,
                                                false)) {

        ov_ice_transaction_unset(
            base->stream->session->ice, candidate->transaction_id);

        goto error;
    }

    struct sockaddr_storage dest = {0};

    if (!ov_socket_fill_sockaddr_storage(&dest,
                                         base->local.data.sa.ss_family,
                                         candidate->server.socket.host,
                                         candidate->server.socket.port))
        goto error;

    socklen_t len = sizeof(struct sockaddr_in);
    if (dest.ss_family == AF_INET6) len = sizeof(struct sockaddr_in6);

    ssize_t out =
        sendto(base->socket, ptr, nxt - ptr, 0, (struct sockaddr *)&dest, len);

    if (out <= 0) {

        ov_ice_transaction_unset(
            base->stream->session->ice, candidate->transaction_id);

        goto error;
    }

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_ice_base_send_turn_allocate_request(ov_ice_base *base,
                                            ov_ice_candidate *candidate) {

    uint8_t buffer[OV_UDP_PAYLOAD_OCTETS] = {0};
    uint8_t *ptr = buffer;
    uint8_t *nxt = NULL;

    if (!base || !candidate) goto error;

    if (candidate->base != base) goto error;

    if (!ov_ice_transaction_create(base->stream->session->ice,
                                   candidate->transaction_id,
                                   13,
                                   candidate))
        goto error;

    if (!ov_turn_allocate_generate_request(ptr,
                                           OV_UDP_PAYLOAD_OCTETS,
                                           &nxt,
                                           candidate->transaction_id,
                                           NULL,
                                           0,
                                           true))
        goto error;

    struct sockaddr_storage dest = {0};

    if (!ov_socket_fill_sockaddr_storage(&dest,
                                         base->local.data.sa.ss_family,
                                         candidate->server.socket.host,
                                         candidate->server.socket.port))
        goto error;

    socklen_t len = sizeof(struct sockaddr_in);
    if (dest.ss_family == AF_INET6) len = sizeof(struct sockaddr_in6);

    ssize_t out =
        sendto(base->socket, ptr, nxt - ptr, 0, (struct sockaddr *)&dest, len);

    if (out <= 0) {

        ov_ice_transaction_unset(
            base->stream->session->ice, candidate->transaction_id);

        goto error;
    }

    return true;
error:
    return false;
}