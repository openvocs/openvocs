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
        @file           ov_data.h
        @author         Markus Töpfer

        @date           2020-04-09

        Definition of a generic data container struct with global magic_byte,
        custom type as well as custom free function.

        This struct contains the base functinality of most ov_* custom structs,
        and may be used as some generic container data structure.

        USAGE

                struct custom {

                        ov_data data;   <-- MUST be the first entry

                        ... whatever the custom struct needs
                        ... some function pointer
                        ... some data pointer
                        ... some sub structs

                };

        ------------------------------------------------------------------------
*/
#ifndef ov_data_h
#define ov_data_h

#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>

#define OV_DATA_MAGIC_BYTE 0xDDDD

typedef struct ov_data ov_data;

/*----------------------------------------------------------------------------*/

struct ov_data {

  const uint16_t magic_byte; /* generic magic byte */
  uint16_t type;             /* custom data type */

  ov_data *(*free)(ov_data *self); /* custom free method */
};

/*----------------------------------------------------------------------------*/

/**
        Cast some pointer to ov_data

        @param data     pointer to cast
        @returns        ov_data if the pointer starts with OV_DATA_MAGIC_BYTE
*/
ov_data *ov_data_cast(const void *data);

/*----------------------------------------------------------------------------*/

/**
        Cast some pointer as ov_data and call data->free

        @param data     pointer to cast
        @returns        result of data->free(data)
                        data on ERROR
*/
void *ov_data_free(void *data);

/*----------------------------------------------------------------------------*/

/**
        Initilize some pointer as ov_data

        @param data     pointer to cast
        @param size     sizeof(data), MUST be at least sizeof(ov_data)

        @returns        initialized ov_data with OV_DATA_MAGIC_BYTE set
*/
ov_data *ov_data_init(void *data, size_t size);

#endif /* ov_data_h */
