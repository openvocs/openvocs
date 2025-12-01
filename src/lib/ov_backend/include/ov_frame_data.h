/***
        ------------------------------------------------------------------------

        Copyright (c) 2020 German Aerospace Center DLR e.V. (GSOC)

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

        @author         Michael J. Beer

        ------------------------------------------------------------------------
*/
#ifndef OV_FRAME_DATA_H
#define OV_FRAME_DATA_H
/*----------------------------------------------------------------------------*/

#include <stddef.h>

#include <ov_base/ov_buffer.h>
#include <ov_base/ov_list.h>
#include <ov_base/ov_rtp_frame.h>
#include <ov_base/ov_vad_config.h>

// #include "ov_ssid_translation_table.h"
#include <ov_base/ov_vad_config.h>
#include <ov_codec/ov_codec.h>

/*----------------------------------------------------------------------------*/

typedef struct {

  uint16_t magic_bytes;

  ov_buffer *pcm16s_32bit;

  uint16_t sequence_number;
  uint32_t timestamp;

  uint8_t payload_type;
  uint32_t ssid;

  size_t num_samples;

  ov_vad_parameters vad_params;

  int16_t max_amplitude;

  bool marked;

} ov_frame_data;

/*----------------------------------------------------------------------------*/

void ov_frame_data_enable_caching(size_t capacity);

ov_frame_data *ov_frame_data_create();

ov_frame_data *ov_frame_data_free(ov_frame_data *data);

/*****************************************************************************
                                    ENCODING
 ****************************************************************************/

/*----------------------------------------------------------------------------*/

ov_rtp_frame *ov_frame_data_encode_with_codec(ov_frame_data *frame_data,
                                              ov_codec *out_codec);

/*----------------------------------------------------------------------------*/

#endif
