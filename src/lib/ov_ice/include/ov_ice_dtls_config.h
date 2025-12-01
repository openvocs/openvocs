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
        @file           ov_ice_core_dtls_config.h
        @author         Markus Töpfer

        @date           2024-09-11


        ------------------------------------------------------------------------
*/
#ifndef ov_ice_dtls_config_h
#define ov_ice_dtls_config_h

#include <ov_base/ov_socket.h>
#include <ov_encryption/ov_hash.h>

#define OV_ICE_SSL_KEY "ssl"
#define OV_ICE_SSL_KEY_CERTIFICATE_FILE "certificate"
#define OV_ICE_SSL_KEY_CERTIFICATE_KEY "key"
#define OV_ICE_SSL_KEY_CA_FILE "CA file"
#define OV_ICE_SSL_KEY_CA_PATH "CA path"
#define OV_ICE_SSL_KEY_DTLS_STRP "srtp"
#define OV_ICE_SSL_KEY_DTLS "dtls"
#define OV_ICE_SSL_KEY_DTLS_KEY_QUANTITY "quantity"
#define OV_ICE_SSL_KEY_DTLS_KEY_LENGTH "length"
#define OV_ICE_SSL_KEY_DTLS_KEY_LIFETIME_USEC "lifetime usec"

/*----------------------------------------------------------------------------*/

#define OV_ICE_DTLS_SRTP_PROFILES                                              \
  "SRTP_AES128_CM_SHA1_80:"                                                    \
  "SRTP_AES128_CM_SHA1_32"

#define OV_ICE_SRTP_PROFILE_MAX 1024

#define OV_ICE_SRTP_KEY_MAX 48  // min 32 key + 14 salt
#define OV_ICE_SRTP_SALT_MAX 14 // 112 bit salt

#define OV_ICE_SSL_BUFFER_SIZE 16000 // max SSL buffer size

// Default DTLS keys and key length
#define OV_ICE_DTLS_FINGERPRINT_MAX 3 * OV_SHA512_SIZE + 10 // SPname/0 HEX

#define OV_ICE_DTLS_RECONNECT_INTERVAL_USEC 50000 // 50ms
#define OV_ICE_DTLS_KEY_LIFETIME_USEC 60000000    // 1min
#define OV_ICE_DTLS_KEY_QUANTITY 20
#define OV_ICE_DTLS_KEY_LENGTH 20

/*----------------------------------------------------------------------------*/

typedef enum {

  OV_ICE_DTLS_ACTIVE = 1,
  OV_ICE_DTLS_PASSIVE = 2

} ov_ice_dtls_type;

/*----------------------------------------------------------------------------*/

typedef struct ov_ice_dtls_config {

  /*
   *      Certificate configuration for SSL,
   *      including pathes to certificate, key and verification chains.
   */

  char cert[PATH_MAX];
  char key[PATH_MAX];

  struct {

    char file[PATH_MAX]; // path to CA verify file
    char path[PATH_MAX]; // path to CAs to use

  } ca;

  /*
   *      SRTP specific configuration
   */

  struct {

    char profile[OV_ICE_SRTP_PROFILE_MAX];

  } srtp;

  /*
   *      DTLS specific configuration
   */

  struct {

    struct {

      size_t quantity; // amount of DTLS keys used for cookies
      size_t length;   // min length of DTLS cookie

      uint64_t lifetime_usec; // lifetime of keys in usecs

    } keys;

  } dtls;

  uint64_t reconnect_interval_usec; // handshaking interval

} ov_ice_dtls_config;

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_ice_dtls_config ov_ice_dtls_config_from_json(const ov_json_value *v);

bool ov_ice_dtls_config_init(ov_ice_dtls_config *c);

#endif /* ov_ice_core_dtls_config_h */
