/***

Copyright   2018, 2020        German Aerospace Center DLR e.V.,
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

     \author             Michael J. Beer, DLR/GSOC <michael.beer@dlr.de>
     \date               2018-01-19


    Generator for PCM16S data.
    It generates chunks of PCM data.
    The chunk length is determined by the parameters presented in
    `ov_pcm_gen_config`:

    It's all about an `ov_pcm_gen` struct.
    This contains  the function `generate_frame`(self, int16_t **buffer,
    uint64_t length)`:
    * Allocates a new buffer of sufficient length for one PCM chunk
    * Writes a chunk into the buffer
    * returns the &buffer in `buffer` argument and its length (in bytes) in
    length

    Currently supported are
    * Artificial sinus waves
    * Replays of raw PCM16s files

  */
/*---------------------------------------------------------------------------*/

#ifndef OV_PCM_GEN_H
#define OV_PCM_GEN_H

/*---------------------------------------------------------------------------*/
#include <inttypes.h>
#include <stdbool.h>
#include <unistd.h>

#include <ov_base/ov_buffer.h>
#include <ov_base/ov_cache.h>
#include <ov_base/ov_json.h>

/******************************************************************************
 *
 *  TYPEDEFS
 *
 ******************************************************************************/

typedef struct {

  double sample_rate_hertz;
  uint64_t frame_length_usecs;

} ov_pcm_gen_config;

/*----------------------------------------------------------------------------*/

typedef struct ov_pcm_gen {

  /**
   * Produces a new PCM chunk of length defined by ov_pcm_gen_config.
   */
  ov_buffer *(*generate_frame)(struct ov_pcm_gen *self);
  void *(*free)(void *self);

} ov_pcm_gen;

/*----------------------------------------------------------------------------*/

typedef enum {
  OV_SINUSOIDS = -1, /// specifc arg must be a pointer to ov_pcm_gen_sinusoids
  OV_WHITE_NOISE, /// specific arg must be a pointer to ov_pcm_gen_white_noise
  OV_FROM_FILE,   /// specific arg must be a c-string(the file name)
  OV_PCM_GEN_TYPE_LAST,
} ov_pcm_gen_type;

/*----------------------------------------------------------------------------*/

typedef struct {

  double frequency_hertz;

  struct {

    /* if 0, wobbling is disabled */
    double period_secs;
    double frequency_disp_hertz;
  } wobble;

} ov_pcm_gen_sinusoids;

/*----------------------------------------------------------------------------*/

typedef struct {

  char *file_name;
  ov_json_value *codec_config;

} ov_pcm_gen_from_file;

/*----------------------------------------------------------------------------*/

typedef struct {

  uint16_t max_amplitude;

} ov_pcm_gen_white_noise;

/******************************************************************************
 *
 *  FUNCTIONS
 *
 ******************************************************************************/

/**
 * Creates a new PCM Generator object.
 * @param specific Depending on type, must be either ov_pcm_gen_sinusoids or
 * ov_pcm_gen_stream
 * @param buffer_cache cache to use for retrieving new buffer, might be 0
 */
ov_pcm_gen *ov_pcm_gen_create(ov_pcm_gen_type type, ov_pcm_gen_config config,
                              void *restrict specific);

/*----------------------------------------------------------------------------*/

ov_pcm_gen *ov_pcm_gen_free(ov_pcm_gen *generator);

/*----------------------------------------------------------------------------*/

ov_buffer *ov_pcm_gen_generate_frame(ov_pcm_gen *generator);

/*----------------------------------------------------------------------------*/

bool ov_pcm_gen_config_print(FILE *out, ov_pcm_gen_config const *config,
                             size_t indentation_level);

/*----------------------------------------------------------------------------*/
#endif /* ov_pcm_gen.h */
