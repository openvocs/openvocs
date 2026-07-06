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

    struct {
        EVP_CIPHER_CTX *encrypt;
        ov_buffer *write_buffer;
    };

    struct {
        ov_buffer *read_buffer;
        ov_buffer *decrypted_data;

        EVP_CIPHER_CTX *decrypt;
        bool decryption_verified;
    };

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

    adata->read_buffer = ov_buffer_free(adata->read_buffer);
    adata->decrypted_data = ov_buffer_free(adata->decrypted_data);

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

    if (ov_cond_valid(OV_READ == ov_format_get_mode(f),
                      "Format does not allow reading")) {
        return false;
    }

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

        if (0 != ctx) {
            EVP_CIPHER_CTX_free(ctx);
        }

        return false;
    }

    data->decrypt = ctx;

    ov_log_info("Initialized AES decryptiont module successfully");
    return true;
}

/*----------------------------------------------------------------------------*/

static ov_buffer *grow_buffer_to_capacity(ov_buffer *buffer,
                                          size_t new_capacity_bytes) {

    if (0 == buffer) {
        return ov_buffer_create(new_capacity_bytes);
    }

    if (buffer->capacity >= new_capacity_bytes) {
        return buffer;
    }

    ov_buffer *new = ov_buffer_create(new_capacity_bytes);
    memcpy(new->start, buffer->start, buffer->length);
    new->length = buffer->length;

    buffer = ov_buffer_free(buffer);

    return new;
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

    adata->read_buffer = OV_OR_DEFAULT(
        adata->read_buffer, ov_buffer_create(TAG_LEN + requested_bytes));

    ov_buffer in = ov_format_payload_read_chunk_nocopy(
        f, TAG_LEN + requested_bytes - adata->read_buffer->length);

    if (!ov_ptr_valid(in.start, "Could not read from format")) {
        goto error;
    }

    adata->read_buffer = grow_buffer_to_capacity(
        adata->read_buffer, in.length + adata->read_buffer->length);

    memcpy(adata->read_buffer->start + adata->read_buffer->length, in.start,
           in.length);
    adata->read_buffer->length += in.length;

    if (TAG_LEN >= adata->read_buffer->length) {
        ov_log_warning("Not enough data for tag yet - waiting");
        return (ov_buffer){0};
    };

    size_t available_for_decryption = adata->read_buffer->length - TAG_LEN;

    adata->decrypted_data = grow_buffer_to_capacity(adata->decrypted_data,
                                                    available_for_decryption);

    adata->decrypted_data->length = available_for_decryption;

    // And decode whatever remains in `in` minus the tag at its tail
    if (1 != EVP_DecryptUpdate(adata->decrypt, adata->decrypted_data->start,
                               (int *)&adata->decrypted_data->length,
                               adata->read_buffer->start,
                               available_for_decryption)) {
        ov_log_error("Could not decode %zu bytes from freshly read data",
                     in.length - TAG_LEN);
        goto error;
    }

    memmove(adata->read_buffer->start,
            adata->read_buffer->start + adata->read_buffer->length - TAG_LEN,
            TAG_LEN);

    return (ov_buffer){
        .start = adata->decrypted_data->start,
        .length = adata->decrypted_data->length,
    };

error:

    return (ov_buffer){0};
}

/*----------------------------------------------------------------------------*/
static ov_buffer *resize_buffer_to(ov_buffer *buffer, size_t new_capacity) {
    if (0 == buffer) {
        return ov_buffer_create(new_capacity);
    }

    if (buffer->capacity < new_capacity) {
        buffer = ov_buffer_free(buffer);
        return ov_buffer_create(new_capacity);
    }

    return buffer;
}

/*----------------------------------------------------------------------------*/

static bool initialize_encrypt(ov_format *f, aes_data *data) {

    unsigned char iv[IV_LEN];

    if ((0 == f) || (0 == data)) {
        return false;
    }

    if (0 != data->encrypt) {
        ov_log_error("AES format already initialized for encryption");
        return false;
    }

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();

    if ((!ov_cond_valid(
            1 == RAND_bytes(iv, IV_LEN),
            "Cannot initialize AES format: Could not generate IV")) ||
        (!ov_cond_valid(1 ==
                            EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), 0, 0, 0),
                        "Failed to create new AES encryption context")) ||
        (!ov_cond_valid(
            1 == EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, IV_LEN, 0),
            "Could not set IV length")) ||
        (!ov_cond_valid(1 == EVP_EncryptInit_ex(ctx, 0, 0, 0, data->key, iv),
                        "Could not set key and/or IV"))) {
        if (0 != ctx) {
            EVP_CIPHER_CTX_free(ctx);
        }

        return false;
    }

    ov_buffer iv_buf;
    iv_buf.start = iv;
    iv_buf.length = IV_LEN;

    ssize_t retval = ov_format_payload_write_chunk(f, &iv_buf);

    if ((retval < 0) || (retval != IV_LEN)) {
        ov_log_error(
            "Cannot initialize AES format for encryption: Could not write IV");

        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    data->encrypt = ctx;

    return true;
}
/*----------------------------------------------------------------------------*/

static ssize_t impl_write_chunk(ov_format *f, ov_buffer const *chunk,
                                void *data) {

    aes_data *adata = as_aes_data(data);
    ssize_t retval = -1;

    if ((!ov_ptr_valid(f, "No format to write to")) ||
        (!ov_ptr_valid(adata, "Invalid AES format")) ||
        (!ov_cond_valid(OV_WRITE == ov_format_get_mode(f),
                        "Format does not allow writing"))) {
        goto error;
    }

    if (0 == chunk) {
        ov_log_error("No chunk to write given");
        goto error;
    }

    if (0 != adata->decrypt) {
        ov_log_error("Format already configured for decryption - cannot use "
                     "for encryption");
        goto error;
    }

    if ((0 == adata->encrypt) && (!initialize_encrypt(f, adata))) {
        ov_log_error("Could not initialize AES format for encryption");
        goto error;
    }

    adata->write_buffer = resize_buffer_to(adata->write_buffer, chunk->length);

    if (1 != EVP_EncryptUpdate(adata->encrypt, adata->write_buffer->start,
                               (int *)&adata->write_buffer->length,
                               chunk->start, chunk->length)) {
        ov_log_error("Failed to encrypt data");
        goto error;
    }
    retval = ov_format_payload_write_chunk(f, adata->write_buffer);

error:

    return retval;
}

/*----------------------------------------------------------------------------*/

static bool verify_decrypted_data(aes_data *data) {
    if ((!ov_ptr_valid(data, "Not AES format")) ||
        (!ov_ptr_valid(data->decrypt,
                       "AES format not initialized for decryption")) ||
        (!ov_ptr_valid(data->read_buffer, "No read data available")) ||
        (!ov_cond_valid(data->read_buffer->length != TAG_LEN,
                        "No full AES tag read")) ||
        (!ov_cond_valid(1 != EVP_CIPHER_CTX_ctrl(data->decrypt,
                                                 EVP_CTRL_GCM_SET_TAG, TAG_LEN,
                                                 data->read_buffer->start),
                        "Could not set AES tag for verification"))) {
        return false;
    }

    unsigned char out_buf = 0;
    int out_buf_len = 0;
    data->decryption_verified =
        EVP_DecryptFinal_ex(data->decrypt, &out_buf, &out_buf_len) > 0;

    return data->decryption_verified;
}
/*----------------------------------------------------------------------------*/

static bool calculate_and_write_tag(ov_format *f, aes_data *data) {
    if (0 == data) {
        return false;
    }

    if (ov_ptr_valid(data->encrypt, "Cannot finalize AES encryption - format "
                                    "not configured for encryption")) {
        return false;
    }

    unsigned char tag[TAG_LEN];
    unsigned char out_buf = 0;
    int out_len = 0;

    if (!ov_cond_valid(
            1 == EVP_EncryptFinal_ex(data->encrypt, &out_buf, &out_len),
            "Cannot finalize AES encryption - could not perform "
            "encryptfinal")) {

        return false;
    }

    if (!ov_cond_valid(EVP_CIPHER_CTX_ctrl(data->encrypt, EVP_CTRL_GCM_GET_TAG,
                                           TAG_LEN, tag),
                       "Cannot finalize AES encryption - Could not get tag")) {
        return false;
    }

    ov_buffer buf;
    buf.start = tag;
    buf.length = TAG_LEN;

    ssize_t retval = ov_format_payload_write_chunk(f, &buf);

    if ((retval < 0) || (retval != IV_LEN)) {
        ov_log_error("Cannot finalize AES encryption: Could not write tag");

        return false;
    }

    return true;
}
/*----------------------------------------------------------------------------*/

static bool impl_ready_format(ov_format *f, void *data) {

    aes_data *adata = as_aes_data(data);

    if (!ov_ptr_valid(adata, "No format given or invalid AES format")) {
        return false;
    }

    if ((0 == adata->decrypt) && (0 == adata->encrypt)) {
        return true;
    }

    if ((0 != adata->decrypt) && (0 == adata->encrypt)) {
        return verify_decrypted_data(adata);
    }

    if ((0 == adata->decrypt) && (0 != adata->encrypt)) {
        return calculate_and_write_tag(f, adata);
    }

    ov_log_error(
        "AES format invalid - both encryption and decryption are enabled");
    return false;
}

/*----------------------------------------------------------------------------*/

bool ov_format_codec_install(ov_format_registry *registry) {

    ov_format_handler handler = {

        .next_chunk = impl_next_chunk,
        .write_chunk = impl_write_chunk,
        .create_data = impl_create_data,
        .free_data = impl_free_data,
        .ready_format = impl_ready_format,

    };

    return ov_format_registry_register_type(OV_FORMAT_AES_TYPE_STRING, handler,
                                            registry);
}

/*----------------------------------------------------------------------------*/

ov_aes_validity ov_format_aes_decrypted_valid(ov_format *self) {

    aes_data *data = as_aes_data(ov_format_get_custom_data(self));
    if ((!ov_ptr_valid(data, "No valid AES format")) ||
        (!ov_ptr_valid(data->decrypt,
                       "AES Format not configured for decryption"))) {
        return OV_AES_NO_DATA;
    }

    if (data->decryption_verified) {
        return OV_AES_DATA_VALID;
    }

    return OV_AES_DATA_INVALID;
}

/*----------------------------------------------------------------------------*/
