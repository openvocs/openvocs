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
#ifndef OV_RECORDER_APP_H
#define OV_RECORDER_APP_H
/*----------------------------------------------------------------------------*/

#include "ov_recorder_config.h"
#include <ov_base/ov_event_loop.h>

/*----------------------------------------------------------------------------*/

struct ov_recorder_app_struct;

typedef struct ov_recorder_app_struct ov_recorder_app;

/*----------------------------------------------------------------------------*/

ov_recorder_app *ov_recorder_app_create(char const *app_name,
                                        ov_recorder_config cfg,
                                        ov_event_loop *loop);

/*----------------------------------------------------------------------------*/

ov_recorder_app *ov_recorder_app_free(ov_recorder_app *self);

/*----------------------------------------------------------------------------*/
#endif
