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
        @file           ov_data.c
        @author         Markus Töpfer

        @date           2020-04-09


        ------------------------------------------------------------------------
*/
#include "../include/ov_data.h"

#include <string.h>

static const ov_data init_data = (ov_data){.magic_byte = OV_DATA_MAGIC_BYTE};

/*----------------------------------------------------------------------------*/

ov_data *ov_data_cast(const void *data) {

    if (!data) return NULL;

    if (*(uint16_t *)data == OV_DATA_MAGIC_BYTE) return (ov_data *)data;

    return NULL;
}

/*----------------------------------------------------------------------------*/

void *ov_data_free(void *self) {

    ov_data *data = ov_data_cast(self);
    if (data && data->free) return data->free(data);

    return self;
}

/*----------------------------------------------------------------------------*/

ov_data *ov_data_init(void *self, size_t size) {

    if (!self || size < sizeof(ov_data)) goto error;

    memcpy(self, &init_data, sizeof(ov_data));
    return ov_data_cast(self);

error:
    return NULL;
}