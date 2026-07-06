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

        @author         Michael J. Beer, DLR/GSOC

        ------------------------------------------------------------------------
*/
#include "../include/ov_format_aes.h"
#include <openssl/evp.h>
#include <openssl/rand.h>

uint32_t MAGIC_BYTES = 0x54ff1f3d;

#define IV_LEN 12
#define TAG_LEN 16

/*----------------------------------------------------------------------------*/

typedef struct {

    uint32_t magic_bytes;

    unsigned char key[OV_FORMAT_AES_KEY_LENGTH];
    unsigned char tag[TAG_LEN];
    size_t tag_available_octets;

    EVP_CIPHER_CTX *encrypt;
    EVP_CIPHER_CTX *decrypt;

} aes_data;

/*----------------------------------------------------------------------------*/

static aes_data *as_aes_data(void *data) {

    if (0 == data)
        return 0;

    aes_data *adata = data;

    if (MAGIC_BYTES != adata->magic_bytes) {
        return 0;
    }

    return adata;
}

/*----------------------------------------------------------------------------*/

static void *impl_create_data(ov_format *f, void *options) {

    UNUSED(f);

    if (0 == options) {

        ov_log_error("No AES parameters given - require at least key");
        goto error;
    }

    ov_format_aes_params *params = options;

    aes_data *adata = calloc(1, sizeof(aes_data));
    OV_ASSERT(0 != adata);

    adata->magic_bytes = MAGIC_BYTES;
    memcpy(adata->key, params->key, sizeof(params->key));

    return adata;

error:

    return 0;
}

/*----------------------------------------------------------------------------*/

static void *impl_free_data(void *data) {

    aes_data *adata = as_aes_data(data);

    if (0 == adata) {

        ov_log_error("Not an ", OV_FORMAT_AES_TYPE_STRING " format");
        goto error;
    }

    if (0 != adata->decrypt) {
        EVP_CIPHER_CTX_free(adata->decrypt);
    }

    if (0 != adata->encrypt) {
        EVP_CIPHER_CTX_free(adata->encrypt);
    }

    return ov_free(adata);

error:

    return data;
}

/*----------------------------------------------------------------------------*/

static bool initialize_decrypt(ov_format *f, aes_data *data) {

    // TODO:
    // for now we simplify here and assume that we can read the entire IV
    // at once - if we get really fancy for reading from TCP and the likes,
    // we would have to assume that we only get parts of iv at a time.
    // Perhaps later

    ov_log_info("AES decryption module not initialized yet - trying to read IV "
                "from format");

    ov_buffer in = ov_format_payload_read_chunk_nocopy(f, IV_LEN);
    if (!ov_ptr_valid(in.start, "Could not read AES IV from format") ||
        (!ov_cond_valid(in.length == IV_LEN,
                        "Available data less than AES IV - cannot initialize "
                        "AES decryption module"))) {
        return false;
    }

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if ((0 == ctx) ||
        (1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), 0, 0, 0)) ||
        (1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, IV_LEN, 0)) ||
        (1 != EVP_DecryptInit_ex(ctx, 0, 0, data->key, in.start))) {
        ov_log_error("Could not initialized AES decryption module");
        return false;
    }

    ov_log_info("Initialized AES decryptiont module successfully");
    return true;
}

/*----------------------------------------------------------------------------*/

static ov_buffer impl_next_chunk(ov_format *f, size_t requested_bytes,
                                 void *data) {

    // TODO:
    // Here we assume that whenever we do not get the amount of data from
    // the lower layer we request, its an error.
    // This simplifies reading the IV first.
    // For reading the actual data, we cannote assume this, because the
    // user might request 100 bytese from a file that only got 80 bytes of
    // encrypted data.

    aes_data *adata = as_aes_data(data);

    if ((!ov_ptr_valid(f, "No format to read from")) ||
        (!ov_ptr_valid(adata, "No valid AES format"))) {
        goto error;
    }

    if (0 != adata->encrypt) {
        ov_log_error("Format already encrypting - en- and decryption is not "
                     "possible at the same time");
        goto error;
    }

    if ((0 == adata->decrypt) && (!initialize_decrypt(f, adata))) {
        goto error;
    }

    ov_buffer in =
        ov_format_payload_read_chunk_nocopy(f, TAG_LEN + requested_bytes);

    if (!ov_ptr_valid(in.start, "Could not read from format")) {
        goto error;
    }

    if (TAG_LEN >= adata->tag_available_octets + in.length) {
        memcpy(adata->tag + adata->tag_available_octets, in.start, in.length);
        adata->tag_available_octets += in.length;
        ov_log_warning("Not enough data for tag yet - waiting");
        return (ov_buffer){0};
    };

    OV_ASSERT(adata->tag_available_octets + in.length > TAG_LEN);

    // Here, we got enough data for the final tag - any more data can be
    // decrypted

    size_t available_for_decryption =
        adata->tag_available_octets + in.length - TAG_LEN;

    if (in.length < TAG_LEN) {

        // merge everything
        unsigned char *buffer = malloc(adata->tag_available_octets + in.length);

        memcpy(buffer, adata->tag, adata->tag_available_octets);
        memcpy(buffer + adata->tag_available_octets, in.start, in.length);

        // now store tail of merged data as new tag
        memcpy(adata->tag, buffer + available_for_decryption, TAG_LEN);
        adata->tag_available_octets = TAG_LEN;

        ov_buffer out;
        out.start = malloc(available_for_decryption);
        out.capacity = available_for_decryption;
        out.length = available_for_decryption;

        if (1 != EVP_DecryptUpdate(adata->decrypt, out.start,
                                   (int *)&out.length, buffer,
                                   available_for_decryption)) {
            ov_log_error("Could not decode %zu bytes",
                         available_for_decryption);
            out.start = ov_free(out.start);
            out.length = 0;
            out.capacity = 0;
        }

        free(buffer);
        return out;
    }

    OV_ASSERT(in.length > TAG_LEN);

    ov_buffer out = (ov_buffer){
        .start = malloc(available_for_decryption),
        .capacity = available_for_decryption,
        .length = 0,
    };

    size_t old_tag_len = adata->tag_available_octets;

    // Decrypt whatever we stored as tag
    if ((0 < adata->tag_available_octets) &&
        (1 != EVP_DecryptUpdate(adata->decrypt, out.start, (int *)&old_tag_len,
                                adata->tag, adata->tag_available_octets))) {
        ov_log_error("Could not decode %zu bytes from stored tag",
                     adata->tag_available_octets);
        goto error;
    }

    size_t decrypted_in_len = in.length - TAG_LEN;

    // And decode whatever remains in `in` minus the tag at its tail
    if (1 != EVP_DecryptUpdate(adata->decrypt, out.start + old_tag_len,
                               (int *)&decrypted_in_len, in.start,
                               in.length - TAG_LEN)) {
        ov_log_error("Could not decode %zu bytes from freshly read data",
                     in.length - TAG_LEN);
        goto error;
    }

    out.length = old_tag_len + decrypted_in_len;

    // store tail of in as tag
    memcpy(adata->tag, in.start + in.length - TAG_LEN, TAG_LEN);
    adata->tag_available_octets = TAG_LEN;

    return out;

error:

    return (ov_buffer){0};
}

/*----------------------------------------------------------------------------*/

static ssize_t impl_write_chunk(ov_format *f, ov_buffer const *chunk,
                                void *data) {

    ssize_t retval = -1;
    ov_buffer *out = 0;

    if (0 == f) {

        ov_log_error("No format to write to");
        goto error;
    }

    if (0 == chunk) {

        ov_log_error("No chunk to write given");
        goto error;
    }

    codec_data *cdata = as_codec_data(data);

    if (0 == cdata) {

        ov_log_error("Not an " OV_FORMAT_AES_TYPE_STRING " format");
        goto error;
    }

    retval = ov_format_payload_write_chunk(f, out);

error:

    if (0 != out) {

        out = ov_buffer_free(out);
    }

    OV_ASSERT(0 == out);

    return retval;
}

/*----------------------------------------------------------------------------*/

bool ov_format_codec_install(ov_format_registry *registry) {

    ov_format_handler codec_handler = {

        .next_chunk = impl_next_chunk,
        .write_chunk = impl_write_chunk,
        .create_data = impl_create_data,
        .free_data = impl_free_data,

    };

    return ov_format_registry_register_type(OV_FORMAT_AES_TYPE_STRING,
                                            codec_handler, registry);
}

/*----------------------------------------------------------------------------*/
