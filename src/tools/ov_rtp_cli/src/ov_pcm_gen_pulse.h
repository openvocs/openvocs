/***
        ------------------------------------------------------------------------

        Copyright (c) 2021 German Aerospace Center DLR e.V. (GSOC)

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
        @copyright (c) 2021 German Aerospace Center DLR e.V. (GSOC)

        ------------------------------------------------------------------------
*/
#ifndef OV_PCM_GEN_PULSE_H
#define OV_PCM_GEN_PULSE_H
/*----------------------------------------------------------------------------*/

#include <ov_pcm_gen/ov_pcm_gen.h>

#define OV_PULSEAUDIO (OV_PCM_GEN_TYPE_LAST + 1)

typedef struct {

  char const *server;

} ov_pcm_gen_pulse;

ov_pcm_gen *ov_pcm_gen_pulse_create(ov_pcm_gen_config config,
                                    ov_pcm_gen_pulse *pulse_config);

/*----------------------------------------------------------------------------*/
#endif
