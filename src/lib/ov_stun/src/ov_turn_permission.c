/***
        ------------------------------------------------------------------------

        Copyright (c) 2022 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_turn_permission.c
        @author         Markus Töpfer

        @date           2022-01-30


        ------------------------------------------------------------------------
*/
#include "../include/ov_turn_permission.h"

#include <ov_base/ov_time.h>

ov_turn_permission *ov_turn_permission_create(const char *host,
                                              uint64_t expire_usec) {

    ov_turn_permission *self = calloc(1, sizeof(ov_turn_permission));
    if (!self || !host)
        goto error;

    if (!strncpy(self->host, host, OV_HOST_NAME_MAX))
        goto error;

    self->updated = ov_time_get_current_time_usecs();
    self->lifetime_usec = expire_usec;

    return self;

error:
    ov_turn_permission_free(self);
    return NULL;
}

/*----------------------------------------------------------------------------*/

ov_turn_permission *ov_turn_permission_free(ov_turn_permission *self) {

    if (!self)
        return NULL;

    self = ov_data_pointer_free(self);
    return NULL;
}
