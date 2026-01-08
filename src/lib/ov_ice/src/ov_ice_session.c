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
        @file           ov_ice_session.c
        @author         Markus

        @date           2024-09-18


        ------------------------------------------------------------------------
*/
#include "../include/ov_ice_session.h"

#include <ov_base/ov_random.h>
#include <ov_base/ov_sdp_attribute.h>
#include <ov_base/ov_time.h>

/*----------------------------------------------------------------------------*/

static bool change_tie_breaker(ov_ice_session *session, uint64_t min,
                               uint64_t max) {

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

static bool timeout_session(uint32_t id, void *s) {

    UNUSED(id);

    ov_ice_session *session = (ov_ice_session *)s;
    session->timer.session_timeout = OV_TIMER_INVALID;

    OV_ASSERT(session->ice);

    ov_ice_drop_session(session->ice, session->uuid);
    return true;
}

/*----------------------------------------------------------------------------*/

static bool start_trickling(uint32_t timer_id, void *data) {

    ov_ice_session *session = ov_ice_session_cast(data);
    if (!session)
        goto error;
    UNUSED(timer_id);
    session->timer.trickling = OV_TIMER_INVALID;
    session->trickling_started = true;

    ov_ice_stream *stream = session->streams;
    while (stream) {

        ov_ice_stream_trickle_candidates(stream);
        stream = ov_node_next(stream);
    }

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool start_nominate(uint32_t timer_id, void *data) {

    ov_ice_session *session = ov_ice_session_cast(data);
    if (!session)
        goto error;
    UNUSED(timer_id);
    session->timer.nominate_timeout = OV_TIMER_INVALID;
    session->nominate_started = true;

    ov_ice_stream *stream = session->streams;
    while (stream) {

        ov_ice_stream_gathering_stop(stream);
        stream = ov_node_next(stream);
    }

    ov_ice_session_update(session);
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

ov_ice_session *create_session(ov_ice *ice) {

    ov_ice_session *session = NULL;

    if (!ice)
        goto error;

    session = calloc(1, sizeof(ov_ice_session));
    if (!session)
        goto error;

    session->node.type = OV_ICE_SESSION_MAGIC_BYTES;
    ov_id_fill_with_uuid(session->uuid);

    session->ice = ice;
    session->state = OV_ICE_RUNNING;
    session->controlling = true;

    change_tie_breaker(session, UINT32_MAX, UINT64_MAX);

    srtp_err_status_t r = srtp_create(&session->srtp.session, NULL);

    switch (r) {

    case srtp_err_status_ok:
        break;

    default:
        ov_log_error("ICE session %s srtp failed %i", session->uuid, r);
        goto error;
    }

    ov_ice_config config = ov_ice_get_config(ice);

    session->timer.session_timeout = ov_event_loop_timer_set(
        config.loop, config.limits.stun.session_timeout_usecs, session,
        timeout_session);

    session->timer.trickling = ov_event_loop_timer_set(
        config.loop, config.limits.stun.connectivity_pace_usecs, session,
        start_trickling);

    session->timer.nominate_timeout = ov_event_loop_timer_set(
        config.loop, config.limits.stun.connectivity_pace_usecs * 10, session,
        start_nominate);

    return session;

error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_ice_session *ov_ice_session_create_offer(ov_ice *ice, ov_sdp_session *sdp) {

    char ssrc[OV_UDP_PAYLOAD_OCTETS] = {0};

    ov_ice_session *session = NULL;

    if (!ice || !sdp)
        goto error;

    session = create_session(ice);
    if (!session)
        goto error;

    uint32_t count = ov_node_count(sdp->description);

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

    ov_sdp_session_persist(sdp);

    ov_ice_stream *stream = NULL;
    ov_sdp_description *description = NULL;

    for (size_t i = 0; i < count; i++) {

        stream = ov_ice_stream_create(session, i);
        if (!stream)
            goto error;

        description = ov_node_get(sdp->description, i + 1);
        if (!description)
            goto error;

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
                                  OV_ICE_STRING_ACTIVE_PASSIVE))
            goto error;

        if (!ov_sdp_attribute_add(&description->attributes, OV_KEY_RTCP_MUX,
                                  NULL))
            goto error;

        if (!ov_sdp_attribute_add(&description->attributes, OV_KEY_FINGERPRINT,
                                  ov_ice_get_fingerprint(stream->session->ice)))
            goto error;

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

        description->connection = ov_sdp_connection_create();
        if (!description->connection)
            goto error;

        *description->connection =
            ov_ice_stream_get_connection(stream, description);

        ov_sdp_session_persist(sdp);
    }

    return session;

error:
    ov_ice_session_free(session);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_ice_session *ov_ice_session_create_answer(ov_ice *ice, ov_sdp_session *sdp) {

    char ssrc[OV_UDP_PAYLOAD_OCTETS] = {0};

    ov_ice_session *session = NULL;

    if (!ice || !sdp)
        goto error;

    session = create_session(ice);
    if (!session)
        goto error;

    session = create_session(ice);
    if (!session)
        goto error;

    uint32_t count = ov_node_count(sdp->description);

    sdp->name = session->uuid;
    sdp->origin.name = "-";
    sdp->origin.id = ov_time_get_current_time_usecs();
    sdp->origin.version = ov_time_get_current_time_usecs();

    sdp->origin.connection.nettype = "IN";
    sdp->origin.connection.addrtype = "IP4";
    sdp->origin.connection.address = "0.0.0.0";

    const char *fingerprint_session =
        ov_sdp_attribute_get(sdp->attributes, OV_KEY_FINGERPRINT);
    const char *fingerprint_stream = NULL;
    const char *fingerprint = NULL;

    const char *user = NULL;
    const char *pass = NULL;
    const char *ptr = NULL;

    uint32_t remote_ssrc = 0;
    const char *cname = NULL;
    const char *end_of_candidates = NULL;

    ov_sdp_list *attr = NULL;
    ov_sdp_list *next = NULL;

    ov_ice_stream *stream = NULL;
    ov_sdp_description *description = NULL;

    for (size_t i = 0; i < count; i++) {

        stream = ov_ice_stream_create(session, i);
        if (!stream)
            goto error;

        description = ov_node_get(sdp->description, i + 1);
        if (!description)
            goto error;

        fingerprint_stream =
            ov_sdp_attribute_get(description->attributes, OV_KEY_FINGERPRINT);

        if (fingerprint_stream) {
            fingerprint = fingerprint_stream;
        } else {
            fingerprint = fingerprint_session;
        }

        if (!ov_sdp_attribute_is_set(description->attributes, OV_KEY_RTCP_MUX))
            goto error;

        user =
            ov_sdp_attribute_get(description->attributes, OV_ICE_STRING_USER);
        pass =
            ov_sdp_attribute_get(description->attributes, OV_ICE_STRING_PASS);

        if (!fingerprint || !user || !pass)
            goto error;

        const char *setup =
            ov_sdp_attribute_get(description->attributes, OV_KEY_SETUP);
        if (!setup)
            goto error;

        if (0 == ov_string_compare(setup, OV_ICE_STRING_ACTIVE_PASSIVE)) {

            ov_sdp_attribute_del(&description->attributes, OV_KEY_SETUP);
            ov_sdp_attribute_add(&description->attributes, OV_ICE_STRING_ACTIVE,
                                 NULL);
            ov_ice_stream_set_active(stream, fingerprint);

        } else if (0 == ov_string_compare(setup, OV_ICE_STRING_ACTIVE)) {

            ov_sdp_attribute_del(&description->attributes, OV_KEY_SETUP);
            ov_sdp_attribute_add(&description->attributes,
                                 OV_ICE_STRING_PASSIVE, NULL);
            ov_ice_stream_set_passive(stream);

        } else if (0 == ov_string_compare(setup, OV_ICE_STRING_PASSIVE)) {

            ov_sdp_attribute_del(&description->attributes, OV_KEY_SETUP);
            ov_sdp_attribute_add(&description->attributes, OV_ICE_STRING_ACTIVE,
                                 NULL);
            ov_ice_stream_set_active(stream, fingerprint);

        } else {

            goto error;
        }

        next = description->attributes;
        while (next) {

            attr = next;
            next = ov_node_next(next);

            if (!cname && (0 == strcmp(attr->key, OV_KEY_SSRC))) {

                if (!attr->value)
                    goto error;

                cname = memchr(attr->value, ' ', strlen(attr->value));
                if (!cname)
                    goto error;

                if (!ov_convert_string_to_uint32(
                        attr->value, (cname - attr->value), &remote_ssrc))
                    goto error;

                cname++;

                ptr = memchr(cname, ':',
                             strlen(attr->value) - (cname - attr->value));
                if (!ptr) {
                    cname = NULL;
                    remote_ssrc = 0;
                    continue;
                }

                if (0 == strncmp(cname, OV_ICE_STRING_CNAME, ptr - cname)) {
                    cname = ptr + 1;
                } else {
                    cname = NULL;
                    remote_ssrc = 0;
                }
            }

            if (!end_of_candidates &&
                (0 == strcmp(attr->key, OV_ICE_STRING_END_OF_CANDIDATES))) {
                end_of_candidates = attr->key;
                continue;
            }
        }

        stream->remote.ssrc = remote_ssrc;

        const char *out = NULL;

        ov_sdp_list *attribute = description->attributes;
        while (ov_sdp_attributes_iterate(&attribute, OV_KEY_CANDIDATE, &out)) {

            ov_ice_candidate *c =
                ov_ice_candidate_from_string(out, strlen(out));
            if (!c)
                continue;

            ov_ice_stream_candidate(stream, c);
            c = ov_ice_candidate_free(c);
        }

        if (end_of_candidates)
            stream->remote.gathered = true;

        memcpy(stream->remote.user, user, OV_ICE_STUN_USER_MAX);
        memcpy(stream->remote.pass, pass, OV_ICE_STUN_PASS_MAX);
        memcpy(stream->remote.fingerprint, fingerprint,
               OV_ICE_DTLS_FINGERPRINT_MAX);

        ov_sdp_attribute_del(&description->attributes, OV_ICE_STRING_USER);
        ov_sdp_attribute_del(&description->attributes, OV_ICE_STRING_PASS);
        ov_sdp_attribute_del(&description->attributes, OV_KEY_FINGERPRINT);
        ov_sdp_attribute_del(&description->attributes, OV_KEY_SSRC);
        ov_sdp_attribute_del(&description->attributes, OV_KEY_CANDIDATE);

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
                                  OV_ICE_STRING_ACTIVE_PASSIVE))
            goto error;

        if (!ov_sdp_attribute_add(&description->attributes, OV_KEY_RTCP_MUX,
                                  NULL))
            goto error;

        if (!ov_sdp_attribute_add(&description->attributes, OV_KEY_FINGERPRINT,
                                  ov_ice_get_fingerprint(stream->session->ice)))
            goto error;

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

        description->connection = ov_sdp_connection_create();
        if (!description->connection)
            goto error;

        *description->connection =
            ov_ice_stream_get_connection(stream, description);

        ov_sdp_session_persist(sdp);
    }

    return session;

error:
    ov_ice_session_free(session);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_ice_session *ov_ice_session_cast(const void *data) {

    if (!data)
        goto error;

    ov_node *node = (ov_node *)data;

    if (node->type == OV_ICE_SESSION_MAGIC_BYTES)
        return (ov_ice_session *)data;
error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

void *ov_ice_session_free(void *self) {

    ov_ice_session *session = ov_ice_session_cast(self);
    if (!session)
        return self;

    ov_event_loop *loop = ov_ice_get_event_loop(session->ice);

    if (OV_TIMER_INVALID != session->timer.session_timeout) {

        ov_event_loop_timer_unset(loop, session->timer.session_timeout, NULL);

        session->timer.session_timeout = OV_TIMER_INVALID;
    }

    if (OV_TIMER_INVALID != session->timer.nominate_timeout) {

        ov_event_loop_timer_unset(loop, session->timer.nominate_timeout, NULL);

        session->timer.nominate_timeout = OV_TIMER_INVALID;
    }

    if (OV_TIMER_INVALID != session->timer.trickling) {

        ov_event_loop_timer_unset(loop, session->timer.trickling, NULL);

        session->timer.trickling = OV_TIMER_INVALID;
    }

    if (OV_TIMER_INVALID != session->timer.connectivity) {

        ov_event_loop_timer_unset(loop, session->timer.connectivity, NULL);

        session->timer.connectivity = OV_TIMER_INVALID;
    }

    if (session->srtp.session) {

        if (srtp_err_status_ok != srtp_dealloc(session->srtp.session))
            ov_log_error("ICE %s failed to deallocate SRTP session",
                         session->uuid);

        session->srtp.session = NULL;
    }

    ov_ice_session_drop_callback(session->ice, session->uuid);

    ov_ice_stream *stream = ov_node_pop((void **)&session->streams);

    while (stream) {

        stream = ov_ice_stream_free(stream);
        stream = ov_node_pop((void **)&session->streams);
    }

    session = ov_data_pointer_free(session);
    return NULL;
}

/*----------------------------------------------------------------------------*/

static bool sdp_verify_input(ov_ice_session *session,
                             const ov_sdp_session *sdp) {

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

bool ov_ice_session_process_answer_in(ov_ice_session *session,
                                      const ov_sdp_session *sdp) {

    if (!session || !sdp)
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
    ov_ice_stream *stream = session->streams;

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

            if (!fingerprint_stream &&
                (0 == strcmp(attr->key, OV_KEY_FINGERPRINT))) {
                fingerprint_stream = attr->value;
                continue;
            }

            if (!trickle_stream &&
                (0 == strcmp(attr->key, OV_ICE_STRING_OPTIONS))) {

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

                if (!ov_convert_string_to_uint32(attr->value,
                                                 (cname - attr->value), &ssrc))
                    goto error;

                cname++;

                ptr = memchr(cname, ':',
                             strlen(attr->value) - (cname - attr->value));
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

        if (fingerprint_stream) {
            fingerprint = fingerprint_stream;
        } else {
            fingerprint = fingerprint_session;
        }

        if (!setup || !fingerprint)
            goto error;

        if (0 == ov_string_compare(setup, OV_ICE_STRING_ACTIVE_PASSIVE)) {

            ov_ice_stream_set_active(stream, fingerprint);

        } else if (0 == ov_string_compare(setup, OV_ICE_STRING_ACTIVE)) {

            ov_ice_stream_set_passive(stream);

        } else if (0 == ov_string_compare(setup, OV_ICE_STRING_PASSIVE)) {

            ov_ice_stream_set_active(stream, fingerprint);

        } else {

            goto error;
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

        if (0 == ssrc)
            goto error;

        stream->remote.ssrc = ssrc;
        if (end_of_candidates)
            stream->remote.gathered = true;

        strncpy(stream->remote.user, user, OV_ICE_STUN_USER_MAX);
        strncpy(stream->remote.pass, pass, OV_ICE_STUN_PASS_MAX);
        strncpy(stream->remote.fingerprint, fingerprint,
                OV_ICE_DTLS_FINGERPRINT_MAX);

        /* Step3 - add ICE candidates */

        const char *value = NULL;

        while (ov_sdp_attributes_iterate(&desc->attributes,
                                         OV_ICE_STRING_CANDIDATE, &value)) {

            ov_ice_candidate *c =
                ov_ice_candidate_from_string(value, strlen(value));
            if (!c)
                continue;

            ov_ice_stream_candidate(stream, c);
            c = ov_ice_candidate_free(c);
        }

        if (end_of_candidates)
            stream->remote.gathered = true;

        ov_ice_stream_order(stream);
        ov_ice_stream_prune(stream);

        desc = ov_node_next(desc);
        stream = ov_node_next(stream);
    }

    return true;

error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_ice_session_candidate(ov_ice_session *session, int stream_id,
                              const ov_ice_candidate *candidate) {

    if (!session || !candidate)
        return false;

    return ov_ice_stream_candidate(ov_node_get(session->streams, stream_id + 1),
                                   candidate);
}

/*----------------------------------------------------------------------------*/

bool ov_ice_session_end_of_candidates(ov_ice_session *session, int stream_id) {

    if (!session)
        return false;

    return ov_ice_stream_end_of_candidates(
        ov_node_get(session->streams, stream_id + 1));
}

/*----------------------------------------------------------------------------*/

uint32_t ov_ice_session_get_stream_ssrc(ov_ice_session *session,
                                        int stream_id) {

    if (!session)
        return 0;

    return ov_ice_stream_get_stream_ssrc(
        ov_node_get(session->streams, stream_id + 1));
}

/*----------------------------------------------------------------------------*/

ssize_t ov_ice_session_stream_send(ov_ice_session *session, int stream_id,
                                   const uint8_t *buffer, size_t size) {

    if (!session)
        return -1;

    return ov_ice_stream_send_stream(
        ov_node_get(session->streams, stream_id + 1), buffer, size);
}

/*----------------------------------------------------------------------------*/

static bool have_same_foundation(const ov_ice_candidate *a,
                                 const ov_ice_candidate *b) {

    OV_ASSERT(a);
    OV_ASSERT(b);

    /*      An arbitrary string that is the same for two candidates
     *      that have the same type, base IP address, protocol
     *      (UDP, TCP, etc.), and STUN or TURN server.
     *      If any of these are different,
     *      then the foundation will be different.
     *
     *      Two candidate pairs with the same foundation pairs are
     *      likely to have similar network characteristics.
     *      Foundations are used in the frozen algorithm.
     */

    if (a->type != b->type)
        return false;

    if (a->transport != b->transport)
        return false;

    if (0 != strncmp(a->addr, b->addr, OV_HOST_NAME_MAX))
        return false;

    if ((0 == a->server.socket.host[0]) && (0 == b->server.socket.host[0]))
        return true;

    /*
     *      One of the bases has STUN or TURN enabled.
     *
     *      Check if both use the same STUN / TURN SERVER
     */

    if (a->server.type != b->server.type)
        return false;

    if ((0 != a->server.socket.host[0]) && (0 != b->server.socket.host[0])) {

        if (0 == strncmp(a->server.socket.host, b->server.socket.host,
                         OV_HOST_NAME_MAX))
            return true;
    }

    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_ice_session_set_state_on_foundation(ov_ice_session *self,
                                            ov_ice_pair *pair) {

    if (!self || !pair)
        goto error;

    ov_ice_stream *stream = self->streams;
    ov_ice_pair *test = NULL;

    bool foundation_active = false;
    bool foundation_success = false;

    while (stream) {

        test = stream->pairs;

        while (test) {

            if (test == pair) {
                test = ov_node_next(test);
                continue;
            }

            if ((pair->local == test->local) ||
                have_same_foundation(pair->local, test->local)) {

                foundation_active = true;

                if (test->state == OV_ICE_PAIR_SUCCESS) {
                    foundation_success = true;
                    break;
                }
            }

            if (foundation_success)
                break;

            test = ov_node_next(test);
        }

        if (foundation_success)
            break;

        stream = ov_node_next(stream);
    }

    if (foundation_success || !foundation_active)
        pair->state = OV_ICE_PAIR_WAITING;

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

void ov_ice_session_dump_pairs(ov_ice_session *session) {

    ov_ice_stream *stream = session->streams;
    ov_ice_pair *pair = NULL;
    while (stream) {

        pair = stream->pairs;
        while (pair) {

            ov_log_debug("ICE %s|%i %p %s %" PRIu64 " %s:%i <-> %s:%i %s",
                         session->uuid, stream->index, pair,
                         ov_ice_candidate_type_to_string(pair->local->type),
                         pair->priority, pair->local->addr, pair->local->port,
                         pair->remote->addr, pair->remote->port,
                         ov_ice_pair_state_to_string(pair->state));

            pair = ov_node_next(pair);
        }

        stream = ov_node_next(stream);
    }

    return;
}

/*----------------------------------------------------------------------------*/

static bool drop_unselected(ov_ice_session *session) {

    ov_ice_stream *stream = session->streams;
    ov_ice_pair *pair = NULL;
    ov_ice_pair *drop = NULL;

    while (stream) {

        pair = stream->pairs;
        while (pair) {

            drop = pair;
            pair = ov_node_next(pair);

            if (drop == stream->selected)
                continue;

            drop = ov_ice_pair_free(drop);
        }

        stream = ov_node_next(stream);
    }

    return true;
}

/*----------------------------------------------------------------------------*/

bool ov_ice_session_update(ov_ice_session *session) {

    if (!session)
        goto error;

    if (session->state != OV_ICE_RUNNING)
        return true;

    ov_ice_config config = ov_ice_get_config(session->ice);

    bool all_completed = true;
    bool failed = false;

    ov_ice_stream *stream = session->streams;
    ov_event_loop *loop = ov_ice_get_event_loop(session->ice);

    while (stream) {

        if (stream->state == OV_ICE_RUNNING)
            ov_ice_stream_update(stream);

        switch (stream->state) {

        case OV_ICE_COMPLETED:
            break;

        case OV_ICE_RUNNING:
            all_completed = false;
            break;

        case OV_ICE_FAILED:
            failed = true;

        default:
            break;
        }

        stream = ov_node_next(stream);
    }

    if (failed) {

        session->state = OV_ICE_FAILED;

        if (config.callbacks.session.state)
            config.callbacks.session.state(config.callbacks.userdata,
                                           session->uuid, session->state);

        ov_ice_drop_session(session->ice, session->uuid);
        goto done;
    }

    if (all_completed) {

        if (OV_TIMER_INVALID != session->timer.trickling) {

            loop->timer.unset(loop, session->timer.trickling, NULL);

            session->timer.trickling = OV_TIMER_INVALID;
        }

        if (OV_TIMER_INVALID != session->timer.nominate_timeout) {

            loop->timer.unset(loop, session->timer.nominate_timeout, NULL);

            session->timer.nominate_timeout = OV_TIMER_INVALID;
        }

        if (OV_TIMER_INVALID != session->timer.session_timeout) {

            loop->timer.unset(loop, session->timer.session_timeout, NULL);

            session->timer.session_timeout = OV_TIMER_INVALID;
        }

        session->state = OV_ICE_COMPLETED;
        ov_log_info("ICE %s completed.", session->uuid);

        drop_unselected(session);

        if (config.callbacks.session.state)
            config.callbacks.session.state(config.callbacks.userdata,
                                           session->uuid, session->state);
    }

done:
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_ice_session_set_foundation(ov_ice_session *session,
                                   ov_ice_candidate *candidate) {

    if (!session || !candidate)
        goto error;

    ov_ice_stream *stream = session->streams;
    ov_ice_candidate *cand = NULL;

    while (stream) {

        cand = stream->candidates.local;
        while (cand) {

            if (cand == candidate) {
                cand = ov_node_next(cand);
                continue;
            }

            if (have_same_foundation(candidate, cand)) {

                if (0 == cand->foundation[0])
                    goto done;

                memcpy(candidate->foundation, cand->foundation, 32);

                return true;
            }

            cand = ov_node_next(cand);
        }

        stream = ov_node_next(stream);
    }

done:
    return ov_ice_string_fill_random((char *)candidate->foundation, 32);

error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_ice_session_unfreeze_foundation(ov_ice_session *session,
                                        const uint8_t *foundation) {

    if (!session || !foundation)
        goto error;

    ov_ice_stream *stream = session->streams;
    ov_ice_pair *pair = NULL;

    while (stream) {

        pair = stream->pairs;
        while (pair) {

            OV_ASSERT(pair->local);

            if (pair->state == OV_ICE_PAIR_FROZEN)
                if (0 == memcmp(foundation, pair->local->foundation, 32))
                    pair->state = OV_ICE_PAIR_WAITING;

            pair = ov_node_next(pair);
        }

        stream = ov_node_next(stream);
    }

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_ice_session_restart(ov_ice_session *session) {

    if (!session)
        goto error;

    ov_ice_pair *pair = NULL;
    ov_ice_pair *next = NULL;

    ov_ice_stream *stream = session->streams;
    while (stream) {

        ov_list_clear(stream->valid);
        ov_list_clear(stream->trigger);
        next = stream->pairs;

        /* reset all pairs, but selected one */

        while (next) {

            if (next == stream->selected) {
                next = ov_node_next(next);
                continue;
            }

            pair = next;
            next = ov_node_next(next);

            ov_node_unplug((void **)&stream->pairs, pair);
            pair = ov_ice_pair_free(pair);
        }

        ov_ice_candidate *remote = stream->candidates.remote;
        ov_ice_candidate *local = stream->candidates.local;

        while (remote) {

            local = stream->candidates.local;

            while (local) {

                if (local->component_id != remote->component_id) {
                    local = ov_node_next(local);
                    continue;
                }

                /*
                 *      Try to create a new pair,
                 *      this is expected to fail,
                 *      if remote and local use
                 *      different IP versions.
                 */

                pair = ov_ice_pair_create(stream, local, remote);

                if (!pair) {
                    local = ov_node_next(local);
                    continue;
                }

                /*
                 *      Find pairs with the same foundation.
                 */

                pair->state = OV_ICE_PAIR_FROZEN;

                if (!ov_ice_session_set_state_on_foundation(stream->session,
                                                            pair)) {
                    pair = ov_ice_pair_free(pair);
                    goto error;
                }

                local = ov_node_next(local);
            }

            remote = ov_node_next(remote);
        }

        if (!ov_ice_stream_order(stream))
            goto error;

        if (!ov_ice_stream_prune(stream))
            goto error;

        stream->state = OV_ICE_RUNNING;
        stream = ov_node_next(stream);
    }

    session->state = OV_ICE_RUNNING;
    return ov_ice_session_checklists_run(session);
error:
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_ice_session_change_role(ov_ice_session *session,
                                uint64_t remote_tiebreaker) {

    if (!session)
        goto error;

    session->controlling = !session->controlling;

    if (session->controlling) {

        change_tie_breaker(session, remote_tiebreaker, UINT64_MAX);

    } else {

        change_tie_breaker(session, 0, remote_tiebreaker);
    }

    ov_ice_stream *stream = session->streams;

    while (stream) {

        ov_ice_stream_recalculate_priorities(stream);
        stream = ov_node_next(stream);
    }

    ov_ice_session_restart(session);

    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

static bool foundation_done(ov_list *list, ov_ice_pair *pair) {

    OV_ASSERT(list);
    OV_ASSERT(pair);

    void *next = list->iter(list);

    uint8_t *foundation;

    while (next) {

        next = list->next(list, next, (void **)&foundation);
        OV_ASSERT(foundation);

        if (!foundation || !pair->local)
            continue;

        if (memcmp(foundation, pair->local->foundation, 32))
            return true;
    }

    return false;
}

/*----------------------------------------------------------------------------*/

static bool session_unfreeze(ov_ice_session *session) {

    ov_list *list = NULL;
    ov_ice_stream *stream = NULL;

    if (!session)
        goto error;

    if (!session->streams)
        return true;

    stream = session->streams;

    /*
     *      For all pairs with the same foundation, it sets the state of
     *      the pair with the lowest component ID to Waiting.  If there is
     *      more than one such pair, the one with the highest priority is
     *      used.
     */

    list = ov_list_create((ov_list_config){0});
    if (!list)
        goto error;

    ov_ice_pair *pair = stream->pairs;

    while (pair) {

        if (foundation_done(list, pair)) {

            pair = ov_node_next(pair);
            continue;
        }

        /*
         *      We walk all pairs,
         *      try to find the foundation,
         *      and select the foundation with the lowest component_id
         *      and highest priority.
         *
         *      @NOTE pairs are priority ordered, only a lower
         *      component_id MUST change selected.
         *
         *      @NOTE we only use component id 1,
         *      so the pair is the one we need to select.
         */

        pair->state = OV_ICE_PAIR_WAITING;
        ov_list_push(list, (char *)pair->local->foundation);

        pair = ov_node_next(pair);
    }

    list = ov_list_free(list);
    return true;
error:
    ov_list_free(list);
    return false;
}
/*----------------------------------------------------------------------------*/

static bool perform_connectivity_check(ov_ice_stream *stream);

/*----------------------------------------------------------------------------*/

static bool connectivity_check_send_next(uint32_t id, void *data) {

    ov_ice_stream *stream = ov_ice_stream_cast(data);

    OV_ASSERT(stream);
    OV_ASSERT(stream->session);

    ov_ice_session *session = stream->session;
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
        session->state = OV_ICE_FAILED;
    return false;
}

/*----------------------------------------------------------------------------*/

bool perform_connectivity_check(ov_ice_stream *stream) {

    OV_ASSERT(stream);

    if (!stream)
        goto error;

    ov_ice_session *session = stream->session;
    if (!session)
        goto error;

    ov_ice_config config = ov_ice_get_config(session->ice);

    ov_event_loop *loop = ov_ice_get_event_loop(session->ice);
    if (!loop)
        goto error;

    /*
     *      Search the next stream active state with
     *      pairs available.
     */

    while (stream) {

        if (OV_ICE_RUNNING != stream->state) {

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

    ov_ice_pair *pair = ov_list_queue_pop(stream->trigger);

    if (pair) {

        OV_ASSERT(ov_ice_pair_cast(pair));
        if (!ov_ice_pair_cast(pair))
            goto error;

    } else {

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

    if (!ov_ice_pair_send_stun_binding_request(pair)) {

        /*
         *      Try connectivity check on next pair of
         *      stream.
         */

        pair->state = OV_ICE_PAIR_FAILED;
        return perform_connectivity_check(stream);
    }

    if (OV_TIMER_INVALID == session->timer.connectivity) {

        session->timer.connectivity =
            loop->timer.set(loop, config.limits.stun.connectivity_pace_usecs,
                            stream, connectivity_check_send_next);

        if (OV_TIMER_INVALID == session->timer.connectivity)
            goto error;
    }

    return true;

reschedule_session:

    if (OV_TIMER_INVALID == session->timer.connectivity) {

        session->timer.connectivity =
            loop->timer.set(loop, config.limits.stun.connectivity_pace_usecs,
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

bool ov_ice_session_checklists_run(ov_ice_session *session) {

    if (!session)
        goto error;

    /*
     *  We will start running the checklists,
     *  if the session state is NOT completted,
     *  and NOT in error.
     */

    switch (session->state) {

    case OV_ICE_INIT:
    case OV_ICE_RUNNING:
    case OV_ICE_FAILED:
        break;

    case OV_ICE_ERROR:
        goto error;
        break;

    case OV_ICE_COMPLETED:
        return true;
    }

    if (OV_TIMER_INVALID == session->timer.connectivity) {

        if (!session->streams)
            return true;

        session_unfreeze(session);

        if (!perform_connectivity_check(session->streams))
            goto error;

        session->state = OV_ICE_RUNNING;
    }

    return true;
error:
    if (session)
        session->state = OV_ICE_FAILED;
    return false;
}

/*----------------------------------------------------------------------------*/

void ov_ice_session_state_change(ov_ice_session *session) {

    if (!session)
        goto error;

    ov_ice_stream *stream = session->streams;

    bool completed = true;
    bool failed = false;

    while (stream) {

        switch (stream->state) {

        case OV_ICE_RUNNING:
            completed = false;
            break;

        case OV_ICE_FAILED:
            failed = true;
            break;

        default:
            break;
        }

        stream = ov_node_next(stream);
    }

    if (failed) {

        session->state = OV_ICE_FAILED;

    } else if (completed) {

        session->state = OV_ICE_COMPLETED;

        ov_ice_config config = ov_ice_get_config(session->ice);

        if (config.callbacks.session.state)
            config.callbacks.session.state(config.callbacks.userdata,
                                           session->uuid, session->state);
    }

error:
    return;
}