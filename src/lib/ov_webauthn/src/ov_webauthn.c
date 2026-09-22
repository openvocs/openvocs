/***
        ------------------------------------------------------------------------

        Copyright (c) 2026 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_webauthn.c
        @author         Töpfer, Markus

        @date           2026-09-14


        ------------------------------------------------------------------------
*/
#include "../include/ov_webauthn.h"


#include <ov_base/ov_dump.h>
#include <ov_base/ov_base64.h>
#include <ov_base/ov_id.h>
#include <ov_base/ov_time.h>
#include <ov_base/ov_string.h>
#include <ov_base/ov_cbor.h>

#include <openssl/rand.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/ec.h>
#include <openssl/pem.h>

/*----------------------------------------------------------------------------*/

#define OV_WEBAUTHN_MAGIC_BYTES 0xef56

/*----------------------------------------------------------------------------*/

struct ov_webauthn {

    uint16_t magic_bytes;
    ov_webauthn_config config;

    ov_json_value *challenges;

    struct {

        uint32_t challenge_drop;

    } timer;

};

/*----------------------------------------------------------------------------*/

static bool catch_outdated(const void *key, void *val, void *data){

    if (!key) return true;

    const char *timestamp = ov_json_string_get(ov_json_get(val, "/timestamp"));
    ov_list *outdated = ov_list_cast(data);

    ov_time created = ov_timestamp_from_string(timestamp);
    uint64_t created_epoch = ov_time_to_epoch(created);
    ov_time now = ov_timestamp_create();
    uint64_t now_epoch = ov_time_to_epoch(now);

    if ( (now_epoch - created_epoch) > 60)
        ov_list_push(outdated, (char*) key);

    return true;
}

/*----------------------------------------------------------------------------*/

static bool drop_outdated(void *item, void *data){

    char *key = (char*) item;
    ov_log_debug("Delete challenge %s", key);
    ov_json_value *challenges = ov_json_value_cast(data);
    return ov_json_object_del(challenges, key);
}

/*----------------------------------------------------------------------------*/

static bool drop_outdated_challenges(uint32_t timer, void *userdata){

    UNUSED(timer);
    ov_webauthn *self = ov_webauthn_cast(userdata);

    self->timer.challenge_drop = ov_event_loop_timer_set(
        self->config.loop,
        1000000,
        self, 
        drop_outdated_challenges);

    ov_list *outdated = ov_list_create( (ov_list_config){0});
    ov_json_object_for_each(self->challenges, outdated, catch_outdated);
    ov_list_for_each(outdated, self->challenges, drop_outdated);
    outdated = ov_list_free(outdated);
    return true;
}

/*----------------------------------------------------------------------------*/

ov_webauthn *ov_webauthn_create(ov_webauthn_config config){

    ov_webauthn *self = NULL;

    self = calloc(1, sizeof(ov_webauthn));
    if (!self) goto error;

    self->magic_bytes = OV_WEBAUTHN_MAGIC_BYTES;
    self->config = config;

    self->challenges = ov_json_object();

    self->timer.challenge_drop = ov_event_loop_timer_set(
        self->config.loop,
        1000000,
        self, 
        drop_outdated_challenges);

    ov_cbor_configure((ov_cbor_config){
        .limits.string_size = 2048,
        .limits.utf8_string_size = 2048,
        .limits.array_size = 500,
        .limits.undef_length_array = 2048,
        .limits.map_size = 100,
        .limits.undef_length_map = 2028
    });

    return self;
error:
    ov_webauthn_free(self);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_webauthn *ov_webauthn_free(ov_webauthn *self){

    if (!ov_webauthn_cast(self)) return self;

    self->challenges = ov_json_value_free(self->challenges);

    self = ov_data_pointer_free(self);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_webauthn *ov_webauthn_cast(const void *data){

    if (!data)
        goto error;

    if (*(uint16_t *)data == OV_WEBAUTHN_MAGIC_BYTES)
        return (ov_webauthn *)data;
error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_webauthn_config ov_webauthn_config_from_json(const ov_json_value *input){

    ov_webauthn_config config = {0};

    const ov_json_value *conf = ov_json_object_get(input, "auth");
    if (!conf) conf = input;

    return config;
}

/*---------------------------------------------------------------------------*/

static const char *create_mfa_user_id(ov_json_value *user_data){

    ov_id uuid = {0};
    ov_id_fill_with_uuid(uuid);

    if (!user_data) return NULL;

    ov_json_value *auth = ov_json_object_get(user_data, "auth");
    
    if (!auth){

        auth = ov_json_object();
        ov_json_object_set(user_data, "auth", auth);

    }

    ov_json_value *id = ov_json_object();  
    ov_json_object_set(auth, uuid, id);
    ov_json_value *persistend_uuid = ov_json_string(uuid);
    ov_json_object_set(id, "uuid", persistend_uuid);
    return ov_json_string_get(persistend_uuid);
}

/*---------------------------------------------------------------------------*/

static const char *create_mfa_challenge(
    ov_webauthn *self,
    const char *user_id,
    const char *domain){

    if (!user_id || !domain) goto error;
    
    unsigned char challenge_bytes[33];
    RAND_bytes(challenge_bytes, sizeof(challenge_bytes));
    challenge_bytes[32] = 0;

    uint8_t *challenge = NULL;
    size_t challenge_length = 0;

    ov_base64_url_encode(challenge_bytes, 32, &challenge, &challenge_length);
    ov_base64_url_strip_equals(challenge, &challenge_length);

    ov_json_value *store = ov_json_object();
    ov_json_object_set(self->challenges, (char*) challenge, store);

    ov_json_value *persisted_challenge = ov_json_string((char*)challenge);
    ov_json_object_set(store, "challenge", persisted_challenge);

    char *time = ov_timestamp(false);
    ov_json_object_set(store, "timestamp", ov_json_string(time));

    ov_json_object_set(store, "user", ov_json_string(user_id));
    ov_json_object_set(store, "domain", ov_json_string(domain));

    time = ov_data_pointer_free(time);
    challenge = ov_data_pointer_free(challenge);

    return ov_json_string_get(persisted_challenge);
error:
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_webauthn_create_registration_challenge(ov_webauthn *self,
    const char *username, const char *domain){

    ov_json_value *out = NULL;
    ov_json_value *val = NULL;
    ov_json_value *rp = NULL;
    ov_json_value *user = NULL;
    ov_json_value *array = NULL;

    if (!self || !username || !domain) goto error;

    ov_json_value *userdata = ov_vocs_db_get_user_data(self->config.db, username);
    if (!userdata) goto error;

    const char *userid = create_mfa_user_id(userdata);
    const char *challenge = create_mfa_challenge(self, username, domain);

    if (!userid || !challenge) goto error;

    out = ov_json_object();
    ov_json_object_set(out, "challenge", ov_json_string(challenge));

    rp = ov_json_object();
    ov_json_object_set(out, "rp", rp);
    ov_json_object_set(rp, "name", ov_json_string("openvocs"));
    ov_json_object_set(rp, "id", ov_json_string(domain));

    user = ov_json_object();
    ov_json_object_set(out, "user", user);
    ov_json_object_set(user, "id", ov_json_string(userid));
    ov_json_object_set(user, "name", ov_json_string(username));
    ov_json_object_set(user, "displayName", ov_json_string(username));

    array = ov_json_array();
    ov_json_object_set(out, "pubKeyCredParams", array);
    val = ov_json_object();
    ov_json_array_push(array, val);
    ov_json_object_set(val, "type", ov_json_string("public-key"));
    ov_json_object_set(val, "alg", ov_json_number(-7));

    ov_json_object_set(out, "attestation", ov_json_string("none"));

    return out;
error:
    ov_json_value_free(out);
    return NULL;
}

/*----------------------------------------------------------------------------*/

static bool check_origin(const char *origin, const char *domain){

    /* domain is stored without the scheme and port, 
     * so we cannot check the port, but at least scheme and domainname here. */

    char data[1024] = {0};
    snprintf(data, 1024, "https://%s", domain);
    char *ptr = memchr(origin, ':', strlen(origin));

    if (ptr){

        if (0 == memcmp(data, origin, ptr - origin))
            return true;

    } else {

        if (0 == memcmp(data, origin, strlen(data)))
            return true;
    }

    return false;
}

/*----------------------------------------------------------------------------*/

static ov_json_value *check_client_data(ov_webauthn *self, const char *data){

    uint8_t *clientJson = NULL;
    size_t clientJsonLength = 0;
    ov_json_value *clientData = NULL;

    if (!ov_base64_url_decode((uint8_t*) data, strlen(data), 
        &clientJson, &clientJsonLength))
        goto error;

    clientData = ov_json_read((char*) clientJson, clientJsonLength);
    if (!clientData) goto error;

    const char *str = ov_json_string_get(ov_json_get(clientData, "/type"));
    if (0 != ov_string_compare(str, "webauthn.create")) goto error;

    str = ov_json_string_get(ov_json_get(clientData, "/challenge"));
    ov_json_value *challenge = ov_json_object_get(self->challenges, str);
    if (!challenge) goto error;

    str = ov_json_string_get(ov_json_get(clientData, "/origin"));
    const char *domain = ov_json_string_get(ov_json_get(challenge, "/domain"));
    if (!check_origin(str, domain)) goto error;

    ov_data_pointer_free(clientJson);
    ov_json_value_free(clientData);
    return challenge;
error:
    ov_data_pointer_free(clientJson);
    ov_json_value_free(clientData);
    return NULL;
}

/*----------------------------------------------------------------------------*/

static ov_json_value *cose_to_json(ov_cbor *cose){

    ov_json_value *out = NULL;

    // Standard COSE Map Keys for ES256

    if (ov_cbor_map_count(cose) != 5) goto error;

    ov_cbor *key = ov_cbor_uint(1);
    ov_cbor *type = ov_cbor_map_get(cose, key);
    ov_cbor_set_uint(key, 3);
    ov_cbor *alg = ov_cbor_map_get(cose,key);
    key = ov_cbor_free(key);
    key = ov_cbor_int(-1);
    ov_cbor *crv = ov_cbor_map_get(cose, key);
    ov_cbor_set_int(key, -2);
    ov_cbor *xcbor = ov_cbor_map_get(cose, key);
    ov_cbor_set_int(key, -3);
    ov_cbor *ycbor = ov_cbor_map_get(cose, key);
    key = ov_cbor_free(key);

    if (!type || !alg || !crv || !xcbor || !ycbor) goto error;

    if (2 != ov_cbor_get_uint(type)) goto error;
    if (-7 != ov_cbor_get_int(alg)) goto error;
    if (1 != ov_cbor_get_uint(crv)) goto error;

    uint8_t *x = NULL;
    uint8_t *y = NULL;
    size_t len = 0;

    if (!ov_cbor_get_byte_string(xcbor, &x, &len))goto error;
    if (len != 32) goto error;
    len = 0;
    if (!ov_cbor_get_byte_string(ycbor, &y, &len))goto error;
    if (len != 32) goto error;

    // all data fit to ES256 key

    out = ov_json_object();

    if (!ov_json_object_set(out, "type", ov_json_number(2))) goto error;
    if (!ov_json_object_set(out, "alg", ov_json_number(-7))) goto error;
    if (!ov_json_object_set(out, "crv", ov_json_number(1))) goto error;
    
    uint8_t *xb64 = NULL;
    uint8_t *yb64 = NULL;

    len = 0;
    if (!ov_base64_url_encode(x, 32, &xb64, &len)) goto error;
    if (!ov_base64_url_strip_equals(xb64, &len)) goto error;
    len = 0 ;
    if (!ov_base64_url_encode(y, 32, &yb64, &len)) goto error;
    if (!ov_base64_url_strip_equals(yb64, &len)) goto error;
    len = 0 ;

    if (!ov_json_object_set(out, "x", ov_json_string((char*)xb64))) goto error;
    if (!ov_json_object_set(out, "y", ov_json_string((char*)yb64))) goto error;

    xb64 = ov_data_pointer_free(xb64); 
    yb64 = ov_data_pointer_free(yb64); 

    return out;

error:
    out = ov_json_value_free(out);
    xb64 = ov_data_pointer_free(xb64); 
    yb64 = ov_data_pointer_free(yb64); 
    return NULL;
}   

/*----------------------------------------------------------------------------*/

static bool store_attestation_data(ov_json_value *auth, 
    const uint8_t *credential_id,
    size_t credential_id_len, 
    uint32_t sign_count,
    uint8_t *rp_id_hash,
    uint8_t *aaguid,
    ov_cbor *cose){

    ov_json_value *data = NULL;
    ov_json_value *cert = NULL;

    uint8_t *credential_id_b64 = NULL;
    size_t credential_id_b64_len = 0;

    uint8_t *rp_id_hash_b64 = NULL;
    size_t rp_id_hash_b64_len = 0;

    uint8_t *aaguid_b64 = NULL;
    size_t aaguid_b64_len = 0;

    if (!auth || !credential_id || !rp_id_hash || !aaguid || !cose) goto error;

    if (!ov_base64_url_encode(credential_id, credential_id_len, &credential_id_b64, &credential_id_b64_len))
        goto error;

    if (!ov_base64_url_strip_equals(credential_id_b64, &credential_id_b64_len))
        goto error;

    if (!ov_base64_url_encode(rp_id_hash, 32, &rp_id_hash_b64, &rp_id_hash_b64_len))
        goto error;

    if (!ov_base64_url_strip_equals(rp_id_hash_b64, &rp_id_hash_b64_len))
        goto error;

    if (!ov_base64_url_encode(aaguid, 16, &aaguid_b64, &aaguid_b64_len))
        goto error;

    if (!ov_base64_url_strip_equals(aaguid_b64, &aaguid_b64_len))
        goto error;

    data = ov_json_object();
    
    if (!ov_json_object_set(auth, (char*)credential_id_b64, data)){
        data = ov_json_value_free(data);
        goto error;
    }

    if (!ov_json_object_set(data, "sign_count", ov_json_number(sign_count)))
        goto error;

    if (!ov_json_object_set(data, "credential_id", ov_json_string((char*)credential_id_b64)))
        goto error;

    if (!ov_json_object_set(data, "rp_id_hash", ov_json_string((char*)rp_id_hash_b64)))
        goto error;

    if (!ov_json_object_set(data, "aaguid", ov_json_string((char*)aaguid_b64)))
        goto error;

    cert = cose_to_json(cose);
    if (!ov_json_object_set(data, "certificate", cert)){
        cert = ov_json_value_free(cert);
        goto error;
    }

    ov_data_pointer_free(credential_id_b64);
    ov_data_pointer_free(rp_id_hash_b64);
    ov_data_pointer_free(aaguid_b64);

    return true;
error:

    ov_data_pointer_free(credential_id_b64);
    ov_data_pointer_free(rp_id_hash_b64);
    ov_data_pointer_free(aaguid_b64);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool check_and_store_attestation_data(ov_webauthn *self, 
    ov_json_value *userdata,
    const char *data){

    UNUSED(self);

    uint8_t *credential_id = NULL;
    uint8_t *next = NULL;
    uint8_t *attestationObject = NULL;
    size_t attestationObjectLength = 0;
    ov_cbor *cbor = NULL;
    ov_cbor *cose = NULL;

    uint8_t *temp = NULL;
    size_t templen = 0;

    if (!ov_base64_url_add_equals((uint8_t*)data, strlen(data), 
        &temp, &templen)) goto error;

    if (!ov_base64_url_decode(temp, templen, 
        &attestationObject, &attestationObjectLength))
        goto error;

    temp = ov_data_pointer_free(temp);
    
    ov_cbor_match match = ov_cbor_decode(attestationObject, attestationObjectLength,
                               &cbor, &next);

    if (match != ov_CBOR_MATCH_FULL) goto error;

    ov_cbor *fmt = ov_cbor_map_get_utf8_string(cbor, "fmt");
    ov_cbor *attStmt = ov_cbor_map_get_utf8_string(cbor, "attStmt");
    ov_cbor *authData = ov_cbor_map_get_utf8_string(cbor, "authData");

    if (!fmt || !attStmt || !authData) goto error;

    uint8_t *buffer = NULL;
    size_t size = 0;

    if (!ov_cbor_get_utf8(fmt, &buffer, &size)) goto error;
    if (0 != memcmp(buffer, "none", size)) goto error;

    // 4. Unpack the Authenticator Data (authData) binary layout:
    // Layout specification:
    // [RP ID Hash: 32 bytes] [Flags: 1 byte] [Sign Count: 4 bytes] 
    // [AAGUID: 16 bytes] [Cred ID Len: 2 bytes] [Credential ID: variable] [COSE PubKey: rest]
    
    uint8_t *byte_string = NULL;
    size_t byte_string_length = 0;

    ov_cbor_get_byte_string(authData, &byte_string, &byte_string_length);

    const uint8_t *ptr = byte_string;

    uint8_t rp_id_hash[32];
    memcpy(rp_id_hash, ptr, 32);
    ptr += 32;

    uint8_t flags = *ptr;
    ptr += 1;

    int has_attested_data = (flags & 0x40);
    if (!has_attested_data) goto error;

    uint32_t sign_count = ((uint32_t)ptr[0] << 24) | ((uint32_t)ptr[1] << 16) | 
                          ((uint32_t)ptr[2] << 8) | (uint32_t)ptr[3];
    ptr += 4;
    UNUSED(sign_count);

    uint8_t aaguid[16];
    memcpy(aaguid, ptr, 16);
    ptr += 16;

    uint16_t cred_id_len = ((uint16_t)ptr[0] << 8) | (uint16_t)ptr[1];
    ptr += 2;

    credential_id = calloc(1, cred_id_len + 1);
    if (!credential_id) goto error;
    
    memcpy(credential_id, ptr, cred_id_len);
    ptr += cred_id_len;

    uint8_t *cose_pubkey_ptr = (uint8_t*) ptr;
    size_t cose_pubkey_len = byte_string_length - (size_t)(ptr - byte_string);

    match = ov_cbor_decode(cose_pubkey_ptr, cose_pubkey_len,
                               &cose, &next);

    if (match != ov_CBOR_MATCH_FULL) goto error;

    if (!store_attestation_data(
        userdata,
        credential_id,
        cred_id_len,
        sign_count,
        rp_id_hash,
        aaguid,
        cose)) goto error;

    ov_data_pointer_free(credential_id);
    ov_data_pointer_free(attestationObject);
    ov_cbor_free(cbor);
    ov_cbor_free(cose);

    return true;
error:
    ov_data_pointer_free(attestationObject);
    ov_cbor_free(cbor);
    ov_cbor_free(cose);
    ov_data_pointer_free(credential_id);
    temp = ov_data_pointer_free(temp);
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_webauthn_process_registration_challenge(ov_webauthn *self, const ov_json_value *data){

    ov_json_value *userdata = NULL;

    if (!self || !data) goto error;

    const char *clientDataJson = ov_json_string_get(
        ov_json_get(data, "/response/clientDataJSON"));

    const char *attestationObjectData = ov_json_string_get(
        ov_json_get(data, "/response/attestationObject"));

    if (!clientDataJson || !attestationObjectData) goto error;

    ov_json_value *challenge = check_client_data(self, clientDataJson);
    if (!challenge) goto error;

    const char *user = ov_json_string_get(ov_json_get(challenge, "/user"));
    if (!user) goto error;

    userdata = ov_vocs_db_get_user_data(self->config.db, user);
    if (!userdata) goto error;

    ov_json_value *auth = ov_json_object_get(userdata, "auth");
    
    if (!auth){
        auth = ov_json_object();
        ov_json_object_set(userdata, "auth", auth);
    }

    if (!check_and_store_attestation_data(self, auth, attestationObjectData))
        goto error;

    ov_vocs_db_set_user_data(self->config.db, user, userdata);

    userdata = ov_json_value_free(userdata);
    return true;
error:
    userdata = ov_json_value_free(userdata);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool add_credential_id(const void *key, void *val, void *data){

    if (!key) return true;
    UNUSED(val);

    ov_json_value *arr = ov_json_value_cast(data);
    ov_json_value *out = ov_json_object();
    ov_json_array_push(arr, out);
    ov_json_object_set(out, "type", ov_json_string("public-key"));
    ov_json_object_set(out, "id", ov_json_string((char*)key));
    return true;
}

/*----------------------------------------------------------------------------*/

ov_json_value *ov_webauthn_create_login_challenge(ov_webauthn *self, 
    const char *user, const char *domain){

    ov_json_value *out = NULL;
    ov_json_value *arr = NULL;

    ov_json_value *userdata = ov_vocs_db_get_user_data(self->config.db, user);
    if (!userdata) goto error;

    ov_json_value *auth = ov_json_object_get(userdata, "auth");
    if (!auth) goto error;

    const char *challenge = create_mfa_challenge(self, user, domain);
    if (!challenge) goto error;

    out = ov_json_object();
    ov_json_object_set(out, "challenge", ov_json_string(challenge));
    ov_json_object_set(out, "rpId", ov_json_string(domain));
    ov_json_object_set(out, "userVerification", ov_json_string("preferred"));

    arr = ov_json_array();
    ov_json_object_set(out, "allowCredentials", arr);
    ov_json_object_for_each(auth, arr, add_credential_id);

    return out;
error:
    ov_json_value_free(out);
    return NULL;
}

/*----------------------------------------------------------------------------*/

static ov_json_value *check_login_data(ov_webauthn *self, const char *data){

    uint8_t *clientJson = NULL;
    size_t clientJsonLength = 0;
    ov_json_value *clientData = NULL;

    if (!ov_base64_url_decode((uint8_t*) data, strlen(data), 
        &clientJson, &clientJsonLength))
        goto error;

    clientData = ov_json_read((char*) clientJson, clientJsonLength);
    if (!clientData) goto error;

    const char *str = ov_json_string_get(ov_json_get(clientData, "/type"));
    if (0 != ov_string_compare(str, "webauthn.get")) goto error;

    str = ov_json_string_get(ov_json_get(clientData, "/challenge"));
    ov_json_value *challenge = ov_json_object_get(self->challenges, str);
    if (!challenge) goto error;

    str = ov_json_string_get(ov_json_get(clientData, "/origin"));
    const char *domain = ov_json_string_get(ov_json_get(challenge, "/domain"));
    if (!check_origin(str, domain)) goto error;

    ov_data_pointer_free(clientJson);
    ov_json_value_free(clientData);
    return challenge;
error:
    ov_data_pointer_free(clientJson);
    ov_json_value_free(clientData);
    return NULL;
}

/*----------------------------------------------------------------------------*/

static bool check_rp_id_hash(uint8_t *hash, uint8_t *hash_b64){

    uint8_t *stored = NULL;
    size_t stored_len = 0;

    uint8_t *temp = NULL;
    size_t templen = 0;

    if (!ov_base64_url_add_equals((uint8_t*)hash_b64, strlen((char*)hash_b64), 
        &temp, &templen)) goto error;

    if (!ov_base64_url_decode(temp, templen, 
        &stored, &stored_len))
        goto error;

    temp = ov_data_pointer_free(temp);

    if (stored_len != 32) goto error;
    if (0 != memcmp(stored, hash, 32)) goto error;

    stored = ov_data_pointer_free(stored);
    return true;
error:
    stored = ov_data_pointer_free(stored);
    return false;
}

/*----------------------------------------------------------------------------*/

static bool check_authenticator_data(ov_json_value *key_data, 
    const ov_json_value *auth_data){

    uint8_t *auth_bytes = NULL;
    size_t auth_bytes_len = 0;

    if (!key_data || !auth_data) goto error;

    const char *str = ov_json_string_get(auth_data);
    if (!str) goto error;

    uint8_t *temp = NULL;
    size_t templen = 0;

    if (!ov_base64_url_add_equals((uint8_t*)str, strlen(str), 
        &temp, &templen)) goto error;

    if (!ov_base64_url_decode(temp, templen, 
        &auth_bytes, &auth_bytes_len))
        goto error;

    temp = ov_data_pointer_free(temp);

    if (auth_bytes_len < 37) goto error;

    const uint8_t *ptr = auth_bytes;

    // 2. RP ID Hash (32 bytes)
    uint8_t rp_id_hash[32];
    memcpy(rp_id_hash, ptr, 32);
    ptr += 32;

    // 3. Flags (1 byte)
    uint8_t flags = *ptr;
    ptr += 1;

    // Check User Present (UP) flag (Bit 0: 0x01)
    if (!(flags & 0x01)) goto error;

    uint32_t sign_count = ((uint32_t)ptr[0] << 24) | 
                          ((uint32_t)ptr[1] << 16) | 
                          ((uint32_t)ptr[2] << 8)  | 
                          (uint32_t)ptr[3];
    ptr += 4;

    uint64_t sign_count_stored = ov_json_number_get(ov_json_get(key_data,
        "/sign_count"));

    if (sign_count <= sign_count_stored && 
        !(sign_count == 0 && sign_count_stored == 0)) {
        ov_log_error("SECURITY ALERT: Possible cloned authenticator detected!");
        goto error;
    }

    ov_json_object_set(key_data, "sign_count", ov_json_number(sign_count));

    if (!check_rp_id_hash(rp_id_hash, 
        (uint8_t*) ov_json_string_get(ov_json_get(key_data, "/rp_id_hash")))){
        ov_log_error("SECURITY ALERT: rp_id_hash mismatch!");
        goto error;
    }

    auth_bytes = ov_data_pointer_free(auth_bytes);
    return true;
error:
    auth_bytes = ov_data_pointer_free(auth_bytes);
    return false;
}

/*----------------------------------------------------------------------------*/

static uint8_t *unbase_b64(const uint8_t *data){

    uint8_t *unbased = NULL;
    size_t unbased_len = 0;

    uint8_t *temp = NULL;
    size_t templen = 0;

    if (!ov_base64_url_add_equals((uint8_t*)data, strlen((char*) data), 
        &temp, &templen)) goto error;

    if (!ov_base64_url_decode(temp, templen, 
        &unbased, &unbased_len))
        goto error;

    temp = ov_data_pointer_free(temp);
    return unbased;
error:
    temp = ov_data_pointer_free(temp);
    return NULL;
}

/*----------------------------------------------------------------------------*/

static bool check_certificate(ov_json_value *key_data, const ov_json_value *data){

    uint8_t *x = NULL;
    uint8_t *y = NULL;

    EVP_PKEY *pkey = NULL;
    EVP_MD_CTX *md_ctx = NULL;
    unsigned char *verification_msg = NULL;

    if (!key_data || !data) goto error;

    const char *auth_data = ov_json_string_get(
        ov_json_get(data, "/response/authenticatorData"));

    const char *client_data = ov_json_string_get(
        ov_json_get(data, "/response/clientDataJSON"));

    const char *signature = ov_json_string_get(
        ov_json_get(data, "/response/signature"));

    if (!auth_data || !client_data || !signature) goto error;

    ov_json_value *cert = ov_json_object_get(key_data, "certificate");
    if (!cert) goto error;

    const char *xb64 = ov_json_string_get(ov_json_get(cert, "/x"));
    const char *yb64 = ov_json_string_get(ov_json_get(cert, "/y"));

    if (!xb64 || !yb64) goto error;

    x = unbase_b64((uint8_t*)xb64);
    y = unbase_b64((uint8_t*)yb64);

    // 1. Compute SHA-256 hash of clientDataJSON
    uint8_t client_data_hash[32];
    SHA256((uint8_t*)client_data, strlen(client_data), client_data_hash);

    // 2. Concatenate: verification_msg = authenticatorData || SHA256(clientDataJSON)
    size_t msg_len = strlen(auth_data) + 32;
    verification_msg = calloc(1, msg_len);
    if (!verification_msg) goto error;

    memcpy(verification_msg, auth_data, strlen(auth_data));
    memcpy(verification_msg + strlen(auth_data), client_data_hash, 32);

    // 3. Reconstruct the OpenSSL EVP_PKEY from X and Y coordinates (NIST P-256)
    // Build uncompressed point: 0x04 || X (32 bytes) || Y (32 bytes)
    unsigned char pubkey_point[65];
    pubkey_point[0] = 0x04;
    memcpy(pubkey_point + 1, x, 32);
    memcpy(pubkey_point + 33, y, 32);

    EVP_PKEY_CTX *pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_EC, NULL);
    if (!pctx) goto error;

    if (EVP_PKEY_keygen_init(pctx) <= 0 ||
        EVP_PKEY_CTX_set_ec_paramgen_curve_nid(pctx, NID_X9_62_prime256v1) <= 0) {
        EVP_PKEY_CTX_free(pctx);
        goto error;
    }

    // Load public key point into EVP_PKEY structure
    OSSL_PARAM params[] = {
        OSSL_PARAM_octet_string("pub", pubkey_point, sizeof(pubkey_point)),
        OSSL_PARAM_END
    };
    
    EVP_PKEY_fromdata_init(pctx);
    if (EVP_PKEY_fromdata(pctx, &pkey, EVP_PKEY_PUBLIC_KEY, params) <= 0) {
        EVP_PKEY_CTX_free(pctx);
        goto error;
    }
    EVP_PKEY_CTX_free(pctx);

    // 4. Initialize OpenSSL Digest Verify context for SHA-256 / ECDSA
    md_ctx = EVP_MD_CTX_new();
    if (!md_ctx) goto error;

    if (EVP_DigestVerifyInit(md_ctx, NULL, EVP_sha256(), NULL, pkey) <= 0) {
        goto error;
    }

    // 5. Execute signature verification
    // Note: WebAuthn signatures from the browser are in raw ASN.1/DER format 
    // which OpenSSL's EVP_DigestVerify natively accepts for ECDSA.
    int verify_status = EVP_DigestVerify(md_ctx, 
        (uint8_t*) signature, strlen(signature), verification_msg, msg_len);

    if (verify_status != 1) {
        ov_log_error("Cryptographic signature verification failed!\n");
        goto error;
    }

    if (md_ctx) EVP_MD_CTX_free(md_ctx);
    if (pkey) EVP_PKEY_free(pkey);
    if (verification_msg) free(verification_msg);
    x = ov_data_pointer_free(x);
    y = ov_data_pointer_free(y);
    return true;
error:
    if (md_ctx) EVP_MD_CTX_free(md_ctx);
    if (pkey) EVP_PKEY_free(pkey);
    if (verification_msg) free(verification_msg);
    x = ov_data_pointer_free(x);
    y = ov_data_pointer_free(y);
    return true;
}

/*----------------------------------------------------------------------------*/

bool ov_webauthn_process_login_challenge(ov_webauthn *self, 
    const ov_json_value *data){

    ov_json_value *userdata = NULL;

    if (!self || !data) goto error;

    const char *key_id = ov_json_string_get(ov_json_get(data, "/id"));
    if (!key_id) goto error;

    ov_json_value *challenge = check_login_data(self, 
        ov_json_string_get(ov_json_get(data, "/response/clientDataJSON")));
    if (!challenge) goto error;

    const char *user = ov_json_string_get(ov_json_get(challenge, "/user"));
    if (!user) goto error;

    userdata = ov_vocs_db_get_user_data(self->config.db, user);
    if (!userdata) goto error;

    ov_json_value *auth = ov_json_object_get(userdata, "auth");
    if (!auth) goto error;

    ov_json_value *key_data = ov_json_object_get(auth, key_id);
    if (!key_data) goto error;

    if (!check_authenticator_data(key_data, 
        ov_json_get(data, "/response/authenticatorData"))) goto error;

    if (!check_certificate(key_data, data)) goto error;

    ov_json_value_free(userdata);
    return true;
error:
    ov_json_value_free(userdata);
    return false;
}