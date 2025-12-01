/***

Copyright   2017        German Aerospace Center DLR e.V.,
                        German Space Operations Center (GSOC)

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.

    This file is part of the openvocs project. http://openvocs.org

***/ /**

     \file               ov_codec.h
     \author             Michael J. Beer, DLR/GSOC <michael.beer@dlr.de>
     \date               2017-12-06

     \ingroup            empty

     Media Codec structure.

     Internally, the openvocs uses PCM, 16bit signed integer at 48kHz
     (more specific OV_DEFAULT_SAMPLERATE ) with platform specific endianness.
     Codecs are meant to convert to/from this media format.

     A bare codec only decodes/encodes to PCM 16bit signed integers without
     changing the sample rate.

     Automatic conversion of the sample rates can be enabled by
     `ov_codec_enable_resampling` .

     If the codec is created by the codec factory from JSON and the JSON
     contains a `samplerate_hertz` parameter, resampling is activated auto-
     matically.

     Generally, use the codec factory to create your codecs!

     JSON support:
     - To JSON: `ov_codec_to_json`
     - From JSON: ov_codec_factory_get_codec_from_json`, see ov_codec_factory.h

     Dont handle codecs on your own, let them register with a factory and
     then let the factory create them either via
     - the codec's name: Use ov_codec_factory_get_codec
     - JSON: See above.

     For codec implementers:

     Look at ov_codec_raw.h / ov_codec_raw.c .
     and `ov_codec_factory_create_standard` on how to implement and register
     with a factory.

     If there are standard accessors for certain parameters you want to use,

     FOR GOD'S SAKE USE THE STANDARD ACCESSORS!!!

 **/

#ifndef ov_codec_h
#define ov_codec_h

#include <limits.h>
#include <stddef.h>
#include <stdint.h>

#include <ov_base/ov_json_value.h>

/******************************************************************************
 *  TYPES
 ******************************************************************************/

struct ov_codec_struct;

typedef struct ov_codec_struct ov_codec;

/*****************************************************************************
                             Neat function wrappers
 ****************************************************************************/

ov_codec *ov_codec_free(ov_codec *codec);

/*----------------------------------------------------------------------------*/

char const *ov_codec_type_id(ov_codec const *codec);

/*----------------------------------------------------------------------------*/

/**
 * encodes data at `input` with length `length` and writes result to
 * `output`. Writes at most `max_out_length` bytes.
 * Input is expected to be 16bit signed integer PCM.
 * `length` therefore must be `input_samples * 2` .
 * @return number of written bytes or a negative number in case of
 * error;
 */
int32_t ov_codec_encode(ov_codec *codec, const uint8_t *input,
                        size_t length_bytes, uint8_t *output,
                        size_t max_out_length_bytes);

/*----------------------------------------------------------------------------*/

/**
 * Decodes data at `input` with length `length` and writes result to
 * `output`. Writes at most `max_out_length` bytes.
 * Output will be 16bit signed integer PCM.
 * For `n` samples, the output buffer must hold at least `2 * n` bytes.
 * @return number of written bytes or a negative number in case of
 * error;
 */
int32_t ov_codec_decode(ov_codec *codec, uint64_t seq_number,
                        const uint8_t *input, size_t length_bytes,
                        uint8_t *output, size_t max_out_length_bytes);

/*----------------------------------------------------------------------------*/

/**
 * Get standard payload type for RTP for this codec.
 * If there is no standard payload type (like for Opus, where the payload type
 * has to be negotiated amongst RTP peers), a negative number will be returned.
 */
int8_t ov_codec_get_rtp_payload_type(ov_codec const *codec);

/*----------------------------------------------------------------------------*/

/**
 * Create and return a JSON object resembling specific parameters.
 * These might include some definition of output bandwidth etc.
 */
ov_json_value *ov_codec_get_parameters(const ov_codec *);

uint32_t ov_codec_get_samplerate_hertz(ov_codec const *codec);

/******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

/**
 * Output:
 *
 * {
 *     'name' : type,
 *     [codec parameters]
 * }
 *
 * To turn a json description back into a codec, use
 * `ov_codec_factory_get_codec_from_json`
 *
 * Will not work properly unless codec->get_parameters has been set and works!
 *
 */
ov_json_value *ov_codec_to_json(const ov_codec *codec);

/******************************************************************************
 *                  ACCESSORS FOR STANDARD CODEC PARAMETERS
 ******************************************************************************/

/**
 * Returns the sample rate set in json or a default value (16 kHz)
 */
uint32_t ov_codec_parameters_get_sample_rate_hertz(const ov_json_value *json);

/**
 * Add/Alter the sample rate in json.
 * @return true if successful, false otherwise.
 */
bool ov_codec_parameters_set_sample_rate_hertz(ov_json_value *json,
                                               uint32_t sample_rate_hertz);

/*****************************************************************************
                             For the Codec Factory
 ****************************************************************************/

/**
 * If the codec is configured for something different than the
 * internally used OV_DEFAULT_SAMPLERATE,
 * resample to proper samplerate before encoding
 */
bool ov_codec_enable_resampling(ov_codec *codec);

/*****************************************************************************
                                   Data types
 ****************************************************************************/

/**
 * Internally used only
 */
struct resampling;

/**
 * Generic structure to keep a media codec.
 * For an actual implementation, a codec generator function must be implemented
 * that creates an appropriate coded structure.
 * @see ov_codec_factory
 */
struct ov_codec_struct {

  /**
   * Each particular codec should be identiable by a magic number.
   * This number should be kept in here, in order for `terminate`,
   * `encode`
   * etc. to verify that they operate on the correct codec structure.
   * Thus before using, each of those functions should first do a
   * if(MY_MAGIC_NUMBER != *(uint32_t)codec) goto error;
   *
   * before starting to use codec as codec strcture.
   */
  uint32_t type;

  /**
   * Return id of the codec type (like 'raw', 'opus' ...)
   */
  const char *(*type_id)(const ov_codec *codec);

  ov_codec *(*free)(ov_codec *codec);

  int32_t (*encode)(ov_codec *codec, const uint8_t *input, size_t length,
                    uint8_t *output, size_t max_out_length);

  int32_t (*decode)(ov_codec *codec, uint64_t seq_number, const uint8_t *input,
                    size_t length, uint8_t *output, size_t max_out_length);

  ov_json_value *(*get_parameters)(const ov_codec *);

  uint32_t (*get_samplerate_hertz)(const ov_codec *codec);

  /**
   * SSID of the stream this codec is associated with.
   */
  uint32_t ssid;

  /**
   * Payload type number for RTP streams of this codec if defined.
   * A negative number indicates that there is no fixed payload type for this
   * codec defined. You have to get your payload type somewhere else.
   * In that case, you can leave this function pointer at 0.
   */
  int8_t (*rtp_payload_type)(ov_codec const *);

  /**
   * Don't touch!
   */
  struct resampling *resampling;
};

/*----------------------------------------------------------------------------*/

#endif /* ov_codec_h */
