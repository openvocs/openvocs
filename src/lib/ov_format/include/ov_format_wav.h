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

        @author Michael J. Beer, DLR/GSOC
        @copyright (c) 2020 German Aerospace Center DLR e.V. (GSOC)

        ------------------------------------------------------------------------
*/
#ifndef OV_FORMAT_WAV_H
#define OV_FORMAT_WAV_H
/*----------------------------------------------------------------------------*/

#include "ov_format.h"
#include "ov_format_registry.h"

/*----------------------------------------------------------------------------*/

extern char const *OV_FORMAT_WAV_TYPE_STRING;

/*****************************************************************************
                                  WAV options
 ****************************************************************************/

#define OV_FORMAT_WAV_PCM 0x0001
#define OV_FORMAT_WAV_G711_ALAW 0x0006
#define OV_FORMAT_WAV_G711_MULAW 0x0007

typedef struct {

  /* Only PCM format (== OV_FORMAT_WAV_PCM) allowed here */
  uint16_t format;

  uint16_t channels;
  uint32_t samplerate_hz;

  uint16_t blockAlignmentBytes;
  uint16_t bitsPerSample;

} ov_format_wav_options;

/*****************************************************************************
                    INSTALL function for the format registry
 ****************************************************************************/

bool ov_format_wav_install(ov_format_registry *registry);

/******************************************************************************
                                    Reading
*******************************************************************************/

bool ov_format_wav_get_header(ov_format *fmt, ov_format_wav_options *options);

/*----------------------------------------------------------------------------*/
#endif
