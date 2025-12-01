/***
        ------------------------------------------------------------------------

        Copyright (c) 2023 German Aerospace Center DLR e.V. (GSOC)

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

        Wrapping OGG as an ov_format.

        This OGG implementation does not support stream interleaving when
        writing:
        If you write, only one stream will be written.
        You can stop the current stream and start a new one by
        `ov_format_ogg_new_stream`

        ------------------------------------------------------------------------
*/
#ifndef OV_FORMAT_OGG_H
#define OV_FORMAT_OGG_H
/*----------------------------------------------------------------------------*/

#include "ov_format.h"
#include "ov_format_registry.h"

/*----------------------------------------------------------------------------*/

extern char const *OV_FORMAT_OGG_TYPE_STRING;

/*----------------------------------------------------------------------------*/

typedef struct {

  uint64_t chunk_length_ms;
  uint64_t samplerate_hz;

  uint32_t stream_serial;

} ov_format_ogg_options;

/*****************************************************************************
                    INSTALL function for the format registry
 ****************************************************************************/

bool ov_format_ogg_install(ov_format_registry *registry);

/*----------------------------------------------------------------------------*/

/**
 * Force new Ogg page in same stream.
 * The sample position of the page to be finished can be overwritten by
 * setting `sample_position_old_page` to something greater than -1
 */
bool ov_format_ogg_new_page(ov_format *f, int64_t sample_position_old_page);

/**
 * Start a new stream to write to.
 * This implies that the last stream is terminated.
 */
bool ov_format_ogg_new_stream(ov_format *self, uint32_t stream_serial);

/**
 * Select the stream to extract with `next_chunk`
 */
bool ov_format_ogg_select_stream(ov_format *self, uint32_t stream_serial);

/*----------------------------------------------------------------------------*/
#endif
