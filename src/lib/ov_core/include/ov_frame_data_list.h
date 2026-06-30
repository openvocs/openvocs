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

        this list holds frame datas.
        It takes care that at each point in time there is only one
        frame data for each ssid.

        The data structure is not thread-safe.

        ------------------------------------------------------------------------
*/
#ifndef OV_FRAME_DATA_LIST_H
#define OV_FRAME_DATA_LIST_H
/*----------------------------------------------------------------------------*/

#include <stddef.h>

#include "ov_frame_data.h"

/*----------------------------------------------------------------------------*/

typedef struct {

    uint16_t magic_bytes;

    size_t capacity;
    ov_frame_data **frames;

} ov_frame_data_list;

/*----------------------------------------------------------------------------*/

void ov_frame_data_list_enable_caching(size_t capacity);

/*----------------------------------------------------------------------------*/

ov_frame_data_list *ov_frame_data_list_create(size_t num_entries);

/*----------------------------------------------------------------------------*/

ov_frame_data_list *ov_frame_data_list_free(ov_frame_data_list *list);

/*----------------------------------------------------------------------------*/

/**
 * Push another frame onto the list
 * If the list is full, returns the frame_data again.
 * If an older frame with the same SSID was found, overwrite it with the
 * new one and return the old one.
 *
 * Either way, if the return value is not 0, the returned frame_data must
 * probably be freed!
 */
ov_frame_data *ov_frame_data_list_push_data(ov_frame_data_list *list,
                                            ov_frame_data *data);

/*----------------------------------------------------------------------------*/

ov_frame_data *ov_frame_data_list_pop_data(ov_frame_data_list *list,
                                           uint32_t sid);

/*----------------------------------------------------------------------------*/
#endif
