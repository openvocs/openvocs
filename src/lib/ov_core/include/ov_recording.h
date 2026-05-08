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

        @author Michael J. Beer, DLR/GSOC
        @copyright (c) 2024 German Aerospace Center DLR e.V. (GSOC)

        ------------------------------------------------------------------------
*/
#ifndef OV_RECORDING_H
#define OV_RECORDING_H
/*----------------------------------------------------------------------------*/

#include <ov_base/ov_json_value.h>
#include <stdbool.h>
#include <time.h>

typedef struct {

    char *id;
    char *loop;
    char *uri;
    time_t start_epoch_secs;
    time_t end_epoch_secs;

} ov_recording;

/*----------------------------------------------------------------------------*/

bool ov_recording_set(ov_recording *self, char const *id, char const *loop,
                      char const *uri, time_t start_epoch_secs,
                      time_t end_epoch_secs);

bool ov_recording_clear(ov_recording *self);

/*----------------------------------------------------------------------------*/

ov_json_value *ov_recording_to_json(ov_recording recording);

ov_recording ov_recording_from_json(ov_json_value const *jval);

/*----------------------------------------------------------------------------*/
#endif
