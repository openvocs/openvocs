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

        ------------------------------------------------------------------------
*/
#ifndef OV_FORMAT_OGG_OPUS_H
#define OV_FORMAT_OGG_OPUS_H
/*----------------------------------------------------------------------------*/

#include "ov_format.h"
#include "ov_format_registry.h"

/*----------------------------------------------------------------------------*/

extern char const *OV_FORMAT_OGG_OPUS_TYPE_STRING;

extern const uint16_t OV_OGG_OPUS_PRESKIP_SAMPLES;

/*----------------------------------------------------------------------------*/

typedef struct {

    uint16_t preskip_samples;
    float output_gain_db;
    uint32_t samplerate_hz;

} ov_format_ogg_opus_options;

/*****************************************************************************
                    INSTALL function for the format registry
 ****************************************************************************/

bool ov_format_ogg_opus_install(ov_format_registry *registry);

/*****************************************************************************
                                OPUS Meta-Infos
 ****************************************************************************/

char const *ov_format_ogg_opus_comment(ov_format *self, char const *key);

/*----------------------------------------------------------------------------*/

/**
 * This function will only succeed as long as no actual payload has been written
 * to the format.
 * This is due to the fact that the comment section must go first into the ogg
 * data and inserting data in the middle of a large file is expensive.
 */
bool ov_format_ogg_opus_comment_set(ov_format *self,
                                    char const *key,
                                    char const *value);

/*----------------------------------------------------------------------------*/
#endif
