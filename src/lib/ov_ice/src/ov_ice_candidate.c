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
        @file           ov_ice_candidate.c
        @author         Markus

        @date           2024-09-18


        ------------------------------------------------------------------------
*/
#include "../include/ov_ice_candidate.h"

#define IMPL_DEFAULT_BUFFER_SIZE 512

#include <ov_base/ov_utils.h>

#include <ov_base/ov_constants.h>

#include <ov_base/ov_cache.h>
#include <ov_base/ov_config_keys.h>
#include <ov_base/ov_convert.h>
#include <ov_base/ov_string.h>

#include <ov_core/ov_event_api.h>

#include "ov_ice_string.h"

ov_ice_candidate *ov_ice_candidate_create(ov_ice_stream *stream) {

    ov_ice_candidate *c = calloc(1, sizeof(ov_ice_candidate));
    if (!c) return NULL;

    c->node.type = OV_ICE_CANDIDATE_MAGIC_BYTES;
    c->stream = stream;

    return c;
}

/*----------------------------------------------------------------------------*/

ov_ice_candidate *ov_ice_candidate_cast(const void *data) {

    if (!data) goto error;

    ov_node *node = (ov_node *)data;

    if (node->type == OV_ICE_CANDIDATE_MAGIC_BYTES)
        return (ov_ice_candidate *)data;
error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

void *ov_ice_candidate_free(void *self) {

    ov_ice_candidate *candidate = ov_ice_candidate_cast(self);
    if (!candidate) return self;

    if (candidate->stream) {

        ov_ice_pair *pair = candidate->stream->pairs;
        ov_ice_pair *drop = NULL;

        while (pair) {

            drop = NULL;

            if (pair->local == candidate) drop = pair;

            if (pair->remote == candidate) drop = pair;

            pair = ov_node_next(pair);
            drop = ov_ice_pair_free(drop);
        }

        ov_list_remove_if_included(candidate->stream->valid, candidate);
        ov_list_remove_if_included(candidate->stream->trigger, candidate);

        ov_node_remove_if_included(
            (void **)&candidate->stream->candidates.local, candidate);
        ov_node_remove_if_included(
            (void **)&candidate->stream->candidates.remote, candidate);
    }

    candidate->string = ov_data_pointer_free(candidate->string);

    ov_ice_candidate_extension *ext = NULL;

    while (candidate->ext) {

        ext = ov_node_pop((void **)&candidate->ext);
        if (ext) {
            ext->key = ov_buffer_free(ext->key);
            ext->val = ov_buffer_free(ext->val);
            ext = ov_data_pointer_free(ext);
        }
    }

    if (candidate->base) {

        if (OV_TIMER_INVALID != candidate->server.timer.keepalive) {

            ov_event_loop_timer_unset(
                ov_ice_get_event_loop(candidate->base->stream->session->ice),
                candidate->server.timer.keepalive,
                NULL);

            candidate->server.timer.keepalive = OV_TIMER_INVALID;
        }

        ov_list_remove_if_included(candidate->base->candidates, candidate);
    }

    candidate->turn.realm = ov_data_pointer_free(candidate->turn.realm);
    candidate->turn.nonce = ov_data_pointer_free(candidate->turn.nonce);

    candidate = ov_data_pointer_free(candidate);
    return NULL;
}
/*----------------------------------------------------------------------------*/

const char *ov_ice_candidate_type_to_string(ov_ice_candidate_type type) {

    switch (type) {

        case OV_ICE_HOST:
            return ICE_STRING_HOST;

        case OV_ICE_SERVER_REFLEXIVE:
            return ICE_STRING_SERVER_REFLEXIVE;

        case OV_ICE_PEER_REFLEXIVE:
            return ICE_STRING_PEER_REFLEXIVE;

        case OV_ICE_RELAYED:
            return ICE_STRING_RELAYED;

        default:
            break;
    }

    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_ice_candidate_type ov_ice_candidate_type_from_string(const char *string,
                                                        size_t length) {

    if (!string) goto error;

    if (0 == strncmp(ICE_STRING_HOST, string, length)) return OV_ICE_HOST;

    if (0 == strncmp(ICE_STRING_PEER_REFLEXIVE, string, length))
        return OV_ICE_PEER_REFLEXIVE;

    if (0 == strncmp(ICE_STRING_SERVER_REFLEXIVE, string, length))
        return OV_ICE_SERVER_REFLEXIVE;

    if (0 == strncmp(ICE_STRING_RELAYED, string, length)) return OV_ICE_RELAYED;

error:
    return OV_ICE_INVALID;
}

/*----------------------------------------------------------------------------*/

char *ov_ice_candidate_to_string(const ov_ice_candidate *candidate) {

    char *string = NULL;
    char *ptr = NULL;

    if (!candidate) goto error;

    ov_ice_candidate_extension *ext = NULL;

    size_t size = IMPL_DEFAULT_BUFFER_SIZE;
    size_t len = 0;
    size_t count = 0;

    string = calloc(size, sizeof(char));
    if (!string) goto error;

    const char *transport = "udp";

    switch (candidate->transport) {

        case UDP:
        case DTLS:
            transport = "udp";
            break;
        case TCP:
        case TLS:
            transport = "tcp";
            break;
        default:
            goto error;
    }

    while (true) {

        count++;

        ptr = string;
        len = snprintf(ptr,
                       size,
                       "%s %i %s %i %s %i %s %s",
                       candidate->foundation,
                       candidate->component_id,
                       transport,
                       candidate->priority,
                       candidate->addr,
                       candidate->port,
                       ICE_STRING_TYPE,
                       ov_ice_candidate_type_to_string(candidate->type));

        if ((len == 0) || (len >= size)) goto reallocate;

        ptr += len;

        if (0 != candidate->raddr[0]) {

            len = snprintf(ptr,
                           size - (ptr - string),
                           " raddr %s rport %i",
                           candidate->raddr,
                           candidate->rport);

            if ((len == 0) || (len >= size)) goto reallocate;

            ptr += len;
        }

        ext = candidate->ext;

        while (ext) {

            if (!ext->key || !ext->key->start || !ext->val || !ext->val->start)
                goto error;

            len = snprintf(ptr,
                           size - (ptr - string),
                           " %.*s %.*s",
                           (int)ext->key->length,
                           (char *)ext->key->start,
                           (int)ext->val->length,
                           (char *)ext->val->start);

            if ((len == 0) || (len >= size)) goto reallocate;

            ptr += len;

            ext = ov_node_next(ext);
        }

        break;

    reallocate:
        count++;
        if (100 == count) goto error;

        size = size + IMPL_DEFAULT_BUFFER_SIZE;
        ptr = realloc(string, size);
        if (!ptr) goto error;
        string = ptr;
    }

    return string;
error:
    ov_data_pointer_free(string);
    return NULL;
}

/*----------------------------------------------------------------------------*/

bool ov_ice_candidate_parse(ov_ice_candidate *candidate,
                            const char *string,
                            size_t length) {

    char transport[OV_UDP_PAYLOAD_OCTETS] = {0};

    if (!candidate || !string || length < 1) goto error;

    char *ptr = (char *)string;
    char *space = strchr(ptr, ' ');

    size_t len = 0;

    if (!space || !ov_ice_string_is_foundation(ptr, space - ptr)) goto error;

    if (!memcpy(candidate->foundation, ptr, space - ptr)) goto error;

    ptr = space + 1;
    space = memchr(ptr, ' ', length - (ptr - string));

    if (!space || !ov_ice_string_is_component_id(ptr, space - ptr)) goto error;

    if (!ov_string_to_uint64(
            ptr, space - ptr, (uint64_t *)&candidate->component_id))
        goto error;

    ptr = space + 1;
    space = memchr(ptr, ' ', length - (ptr - string));

    if (!space || !ov_ice_string_is_transport(ptr, space - ptr) ||
        !ov_ice_string_is_transport_extension(ptr, space - ptr))
        goto error;

    if (!memcpy(transport, ptr, space - ptr)) goto error;

    candidate->transport = ov_socket_transport_from_string(transport);

    ptr = space + 1;
    space = memchr(ptr, ' ', length - (ptr - string));

    if (!space || !ov_ice_string_is_priority(ptr, space - ptr)) goto error;

    if (!ov_string_to_uint64(
            ptr, space - ptr, (uint64_t *)&candidate->priority))
        goto error;

    ptr = space + 1;
    space = memchr(ptr, ' ', length - (ptr - string));

    if (!space || !ov_ice_string_is_connection_address(ptr, space - ptr))
        goto error;

    if (!strncpy(candidate->addr, ptr, space - ptr)) goto error;

    ptr = space + 1;
    space = memchr(ptr, ' ', length - (ptr - string));

    if (!space || !ov_ice_string_is_connection_port(ptr, space - ptr))
        goto error;

    if (!ov_string_to_uint64(ptr, space - ptr, (uint64_t *)&candidate->port))
        goto error;

    ptr = space + 1;
    space = memchr(ptr, ' ', length - (ptr - string));

    if (!space || (0 != strncmp(ptr, ICE_STRING_TYPE, space - ptr))) goto error;

    ptr = space + 1;
    space = memchr(ptr, ' ', length - (ptr - string));

    if (!space) {
        len = strlen(string) - (ptr - string);
    } else {
        len = space - ptr;
    }

    if (len > OV_UDP_PAYLOAD_OCTETS) goto error;

    candidate->type = ov_ice_candidate_type_from_string(ptr, len);

    if (OV_ICE_INVALID == candidate->type) goto error;

    if (!space) goto done;

    ptr = space + 1;

    char *raddr = ptr;
    char *rport = NULL;

    space = memchr(ptr, ' ', length - (ptr - string));

    if (space &&
        (0 == strncmp(ptr, ICE_STRING_RADDR, strlen(ICE_STRING_RADDR)))) {

        raddr = space + 1;

        space = memchr(raddr, ' ', length - (raddr - string));
        if (!space) goto error;

        if (!ov_ice_string_is_connection_address(raddr, space - raddr))
            goto error;

        if (!strncpy(candidate->raddr, (char *)raddr, space - raddr))
            goto error;

        rport = space + 1;
        if (0 != strncmp(rport, ICE_STRING_RPORT, strlen(ICE_STRING_RPORT)))
            goto error;

        space = memchr(rport, ' ', length - (rport - string));
        if (!space || (space - rport) != 5) goto error;

        rport = space + 1;
        space = memchr(rport, ' ', length - (rport - string));

        if (space) {
            len = space - rport;
            ptr = space + 1;
        } else {
            len = length - (rport - string);
            ptr = rport + len;
        }

        if (!ov_string_to_uint64(
                (char *)rport, len, (uint64_t *)&candidate->rport))
            goto error;
    }

    // parse all candidate extensions

    char *key = ptr;
    size_t key_len = 0;
    char *val = NULL;
    size_t val_len = 0;

    space = memchr(ptr, ' ', length - (ptr - string));

    while (space) {

        key = ptr;
        space = memchr(key, ' ', length - (key - string));
        key_len = space - key;
        val = space + 1;
        space = memchr(val, ' ', length - (val - string));

        if (space) {

            val_len = space - val;
            ptr = space + 1;

        } else {

            val_len = length - (val - string);
            ptr = val + val_len;
        }

        if (!val || (val_len < 1) || (key_len < 1)) goto error;

        ov_ice_candidate_extension *ext =
            calloc(1, sizeof(ov_ice_candidate_extension));
        if (!ext) goto error;

        if (!ov_node_push((void **)&candidate->ext, ext)) {
            free(ext);
            goto error;
        }

        ext->key = ov_buffer_create(key_len);
        if (!ov_buffer_set(ext->key, key, key_len)) goto error;

        ext->val = ov_buffer_create(val_len);
        if (!ov_buffer_set(ext->val, val, val_len)) goto error;
    }

    if (length - (ptr - string) != 0) goto error;

done:
    return true;
error:
    return false;
}

/*----------------------------------------------------------------------------*/

ov_ice_candidate *ov_ice_candidate_from_string(const char *string,
                                               size_t length) {

    ov_ice_candidate *candidate = ov_ice_candidate_create(NULL);
    if (!candidate) goto error;

    if (!ov_ice_candidate_parse(candidate, string, length)) goto error;

    return candidate;
error:
    ov_ice_candidate_free(candidate);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_ice_candidate *ov_ice_candidate_from_string_candidate(const char *string,
                                                         size_t length) {

    const char *pre = "candidate:";
    size_t pre_len = strlen(pre);

    if (!string) goto error;

    if (length < pre_len + 5) goto error;

    if (0 != strncmp(string, pre, pre_len)) goto error;

    return ov_ice_candidate_from_string(string + pre_len, length - pre_len);
error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

static ov_json_value *create_candidate_extensions(
    ov_ice_candidate_extension *ext) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;

    if (!ext) goto error;

    out = ov_json_object();
    if (!out) goto error;

    while (ext) {

        if (!ext->val || !ext->key) goto error;

        val = ov_json_string((char *)ext->val);
        if (!ov_json_object_set(out, (char *)ext->key, val)) goto error;

        ext = ov_node_next(ext);
    }

    return out;
error:
    ov_json_value_free(out);
    ov_json_value_free(val);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_ice_candidate_to_json(const ov_ice_candidate *candidate) {

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;

    if (!candidate) goto error;

    out = ov_json_object();
    if (!out) goto error;

    const char *str = ov_ice_candidate_type_to_string(candidate->type);
    if (str) {
        val = ov_json_string(str);
    } else {
        val = ov_json_null();
    }
    if (!ov_json_object_set(out, OV_KEY_TYPE, val)) goto error;

    str = ov_socket_transport_to_string(candidate->transport);
    if (str) {
        val = ov_json_string(str);
    } else {
        val = ov_json_null();
    }
    if (!ov_json_object_set(out, OV_KEY_TRANSPORT, val)) goto error;

    if (0 != candidate->foundation[0]) {
        val = ov_json_string((char *)candidate->foundation);
    } else {
        val = ov_json_null();
    }
    if (!ov_json_object_set(out, OV_KEY_FOUNDATION, val)) goto error;

    val = ov_json_number(candidate->component_id);
    if (!ov_json_object_set(out, OV_KEY_COMPONENT, val)) goto error;

    val = ov_json_number(candidate->priority);
    if (!ov_json_object_set(out, OV_KEY_PRIORITY, val)) goto error;

    if (!candidate->addr[0]) {
        val = ov_json_null();
    } else {
        val = ov_json_string(candidate->addr);
    }
    if (!ov_json_object_set(out, OV_KEY_ADDR, val)) goto error;

    val = ov_json_number(candidate->port);
    if (!ov_json_object_set(out, OV_KEY_PORT, val)) goto error;

    if (!candidate->raddr[0]) {
        val = ov_json_null();
    } else {
        val = ov_json_string(candidate->raddr);
    }
    if (!ov_json_object_set(out, OV_KEY_RADDR, val)) goto error;

    val = ov_json_number(candidate->rport);
    if (!ov_json_object_set(out, OV_KEY_RPORT, val)) goto error;

    if (!candidate->ext) {
        val = ov_json_null();
    } else {
        val = create_candidate_extensions(candidate->ext);
    }

    if (!ov_json_object_set(out, OV_KEY_EXTENTIONS, val)) goto error;

    if (0 != candidate->server.socket.host[0]) {

        val = NULL;
        if (!ov_socket_configuration_to_json(candidate->server.socket, &val))
            goto error;

        if (!ov_json_object_set(out, OV_KEY_SERVER, val)) goto error;
    }

    return out;
error:
    ov_json_value_free(out);
    ov_json_value_free(val);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_ice_candidate ov_ice_candidate_from_json(const ov_json_value *candidate) {

    if (!ov_json_is_object(candidate)) goto error;

    ov_ice_candidate out =
        (ov_ice_candidate){.node.type = OV_ICE_CANDIDATE_MAGIC_BYTES};

    ov_json_value const *value = NULL;
    const char *string = NULL;

    value = ov_json_get(candidate, "/" OV_KEY_TYPE);
    if (ov_json_is_string(value)) {

        string = ov_json_string_get(value);
        if (!string) goto error;

        out.type = ov_ice_candidate_type_from_string(string, strlen(string));
    }

    value = ov_json_get(candidate, "/" OV_KEY_TRANSPORT);
    if (ov_json_is_string(value)) {

        string = ov_json_string_get(value);
        if (!string) goto error;

        out.transport = ov_socket_transport_from_string(string);
    }

    value = ov_json_get(candidate, "/" OV_KEY_FOUNDATION);
    if (ov_json_is_string(value)) {

        string = ov_json_string_get(value);
        if (!string) goto error;
        memcpy(out.foundation, string, strlen(string));
    }

    value = ov_json_get(candidate, "/" OV_KEY_COMPONENT);
    if (value) {
        out.component_id = (uint16_t)ov_json_number_get(value);
    }

    value = ov_json_get(candidate, "/" OV_KEY_PRIORITY);
    if (value) {
        out.priority = (uint32_t)ov_json_number_get(value);
    }

    value = ov_json_get(candidate, "/" OV_KEY_ADDR);
    if (ov_json_is_string(value)) {
        string = ov_json_string_get(value);
        if (!string) goto error;
        strncpy(out.addr, string, OV_HOST_NAME_MAX);
    }

    value = ov_json_get(candidate, "/" OV_KEY_PORT);
    if (value) {
        out.port = (uint16_t)ov_json_number_get(value);
    }

    value = ov_json_get(candidate, "/" OV_KEY_RADDR);
    if (ov_json_is_string(value)) {
        string = ov_json_string_get(value);
        if (!string) goto error;
        strncpy(out.raddr, string, OV_HOST_NAME_MAX);
    }

    value = ov_json_get(candidate, "/" OV_KEY_RPORT);
    if (value) {
        out.rport = (uint16_t)ov_json_number_get(value);
    }

    return out;
error:
    return (ov_ice_candidate){};
}

/*----------------------------------------------------------------------------*/

ov_ice_candidate_info ov_ice_candidate_info_from_json(
    const ov_json_value *input) {

    ov_ice_candidate_info info = (ov_ice_candidate_info){0};
    if (!input) goto error;

    info.candidate =
        ov_json_string_get(ov_json_object_get(input, OV_KEY_CANDIDATE));

    info.SDPMlineIndex = ov_json_number_get(
        ov_json_object_get(input, OV_ICE_STRING_SDP_MLINEINDEX));

    info.SDPMid =
        ov_json_number_get(ov_json_object_get(input, OV_ICE_STRING_SDP_MID));

    info.ufrag =
        ov_json_string_get(ov_json_object_get(input, OV_ICE_STRING_UFRAG));

    if (!info.ufrag)
        info.ufrag = ov_json_string_get(ov_json_object_get(input, OV_KEY_USER));

    return info;
error:
    return (ov_ice_candidate_info){0};
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_ice_candidate_info_to_json(ov_ice_candidate_info info) {

    ov_json_value *val = NULL;
    ov_json_value *out = NULL;

    if (!info.ufrag) goto error;

    out = ov_json_object();

    if (info.candidate) {
        val = ov_json_string(info.candidate);
        if (!ov_json_object_set(out, OV_KEY_CANDIDATE, val)) goto error;
    }

    val = ov_json_string(info.ufrag);
    if (!ov_json_object_set(out, OV_ICE_STRING_UFRAG, val)) goto error;

    val = ov_json_number(info.SDPMlineIndex);
    if (!ov_json_object_set(out, OV_ICE_STRING_SDP_MLINEINDEX, val)) goto error;

    val = ov_json_number(info.SDPMid);
    if (!ov_json_object_set(out, OV_ICE_STRING_SDP_MID, val)) goto error;

    return out;
error:
    ov_json_value_free(val);
    ov_json_value_free(out);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_ice_candidate *ov_ice_candidate_from_json_string(
    const ov_json_value *input) {

    const char *pre = "candidate:";
    size_t pre_len = strlen(pre);

    if (!input) goto error;

    const ov_json_value *in = ov_json_object_get(input, OV_KEY_CANDIDATE);
    if (!in) in = input;

    const char *src = ov_json_string_get(in);
    if (!src) goto error;

    size_t src_len = strlen(src);

    if (src_len < pre_len + 5) goto error;

    if (0 != strncmp(src, pre, pre_len)) goto error;

    return ov_ice_candidate_from_string(src + pre_len, src_len - pre_len);
error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_ice_candidate_json_info(ov_ice_candidate *candidate,
                                          const char *session_uuid,
                                          const char *ufrag,
                                          const int stream_id) {

    char string[OV_UDP_PAYLOAD_OCTETS] = {0};

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *par = NULL;

    if (!candidate || !session_uuid) goto error;

    out = ov_event_api_message_create(OV_KEY_CANDIDATE, NULL, 0);
    if (!out) goto error;

    par = ov_event_api_set_parameter(out);

    if (NULL == candidate->string)
        candidate->string = ov_ice_candidate_to_string(candidate);

    snprintf(string, OV_UDP_PAYLOAD_OCTETS, "candidate:%s", candidate->string);
    val = ov_json_string(string);
    if (!ov_json_object_set(par, OV_KEY_CANDIDATE, val)) goto error;

    val = ov_json_string(session_uuid);
    if (!ov_json_object_set(par, OV_KEY_SESSION, val)) goto error;

    val = ov_json_string(ufrag);
    if (!ov_json_object_set(par, OV_ICE_STRING_UFRAG, val)) goto error;

    val = ov_json_number(stream_id);
    if (!ov_json_object_set(par, OV_ICE_STRING_SDP_MLINEINDEX, val)) goto error;

    val = ov_json_number(stream_id);
    if (!ov_json_object_set(par, OV_ICE_STRING_SDP_MID, val)) goto error;

    return out;

error:
    ov_json_value_free(val);
    ov_json_value_free(out);
    return NULL;
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

static uint32_t local_preference(ov_ice_candidate *candidate,
                                 uint32_t interfaces) {

    uint32_t preference = 0;

    OV_ASSERT(candidate);
    OV_ASSERT(candidate->base);
    OV_ASSERT(candidate->base->stream);

    if (!candidate || !candidate->base) goto error;

    ov_ice_base *base = candidate->base;
    ov_ice_stream *stream = base->stream;

    // 0 (lowest) - 65535 (MAX)
    if (1 == interfaces) return 65535;

    switch (base->local.data.sa.ss_family) {

        case AF_INET:

            preference = 40000;
            break;

        case AF_INET6:

            preference = 50000;
            break;

        default:
            goto error;
    }

    /*
     *      We need to check if type and component id are the same,
     *      to reduce the preference of similar sockets.
     */

    ov_ice_candidate *test = stream->candidates.local;
    uint64_t pos = 0;
    uint64_t slot = 0;
    bool same = false;

    while (test) {

        if (test == candidate) slot = pos;

        pos++;

        if ((test != candidate) &&
            (test->base->local.data.sa.ss_family ==
             candidate->base->local.data.sa.ss_family) &&
            (test->type == candidate->type)) {

            same = true;
        }

        test = ov_node_next(test);
    }

    if (same) preference = preference - slot;

    return preference;
error:
    return 0;
}

/*----------------------------------------------------------------------------*/

bool ov_ice_candidate_calculate_priority(ov_ice_candidate *cand,
                                         uint32_t interfaces) {

    OV_ASSERT(cand);
    OV_ASSERT(interfaces > 0);

    if (!cand) return false;

    /*
     *      This is an implementation of RFC 8445 5.1.2.1,
     *      using
     *
     *      RFC 8421 Guidelines for Multihomed and IPv4/IPv6 Dual-Stack
     *               Interactive Connectivity Establishment (ICE)
     *
     */

    cand->priority = (1 << 24) * type_preference(cand->type) +
                     (1 << 8) * local_preference(cand, interfaces) +
                     (1) * (256 - cand->component_id);

    return true;
}
