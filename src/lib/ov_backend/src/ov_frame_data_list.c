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

        ------------------------------------------------------------------------
*/
/*----------------------------------------------------------------------------*/

#include <ov_base/ov_utils.h>

#include <ov_base/ov_constants.h>
#include <ov_base/ov_registered_cache.h>

#include "../include/ov_frame_data_list.h"

/*----------------------------------------------------------------------------*/

static ov_registered_cache *g_list_cache = 0;

/*----------------------------------------------------------------------------*/

#define FRAME_DATA_LIST_MAGIC_BYTES 0xb34d

static ov_frame_data_list *as_frame_data_list(void *vptr) {

    if (0 == vptr)
        return 0;

    ov_frame_data_list *list = vptr;

    if (FRAME_DATA_LIST_MAGIC_BYTES != list->magic_bytes)
        return 0;

    return list;
}

/*----------------------------------------------------------------------------*/

static bool clear_frame_data_list_nocheck(ov_frame_data_list *list) {

    OV_ASSERT(0 != list);

    for (size_t i = 0; i < list->capacity; ++i) {

        if (0 == list->frames[i]) {
            continue;
        }

        list->frames[i] = ov_frame_data_free(list->frames[i]);

        OV_ASSERT(0 == list->frames[i]);
    }

    return true;
}

/*----------------------------------------------------------------------------*/

void *free_frame_data_list(void *vptr) {

    ov_frame_data_list *list = as_frame_data_list(vptr);

    if (0 == list) {
        return 0;
    }

    clear_frame_data_list_nocheck(list);

    OV_ASSERT(0 != list->frames);

    free(list->frames);
    free(list);

    list = 0;

    return list;
}

/*----------------------------------------------------------------------------*/

void ov_frame_data_list_enable_caching(size_t capacity) {

    ov_registered_cache_config cfg = {

        .capacity = capacity,
        .item_free = free_frame_data_list,

    };

    g_list_cache = ov_registered_cache_extend("frame_data_list", cfg);
}

/*----------------------------------------------------------------------------*/

ov_frame_data_list *ov_frame_data_list_create(size_t num_entries) {

    if (0 == num_entries) {
        ov_log_error("Not creating a list without any entries");
        goto error;
    }

    ov_frame_data_list *list = ov_registered_cache_get(g_list_cache);

    if (0 == list) {
        list = calloc(1, sizeof(ov_frame_data_list));
        list->magic_bytes = FRAME_DATA_LIST_MAGIC_BYTES;
        list->frames = calloc(num_entries, sizeof(ov_frame_data *));
        list->capacity = num_entries;
    }

    OV_ASSERT(0 != list);
    OV_ASSERT(0 != list->frames);
    OV_ASSERT(FRAME_DATA_LIST_MAGIC_BYTES == list->magic_bytes);

    if (num_entries > list->capacity) {

        list->frames =
            realloc(list->frames, num_entries * sizeof(ov_frame_data *));

        memset(list->frames, 0, num_entries * sizeof(ov_frame_data_list *));

        list->capacity = num_entries;
    }

    OV_ASSERT(0 != list);
    OV_ASSERT(num_entries <= list->capacity);

    return list;

error:

    return 0;
}

/*----------------------------------------------------------------------------*/

ov_frame_data_list *ov_frame_data_list_free(ov_frame_data_list *list) {

    if (0 == list) {
        return 0;
    }

    clear_frame_data_list_nocheck(list);

    list = ov_registered_cache_put(g_list_cache, list);

    if (0 != list) {
        free_frame_data_list(list);
    }

    list = 0;

    return list;
}

/*----------------------------------------------------------------------------*/

ov_frame_data *ov_frame_data_list_push_data(ov_frame_data_list *list,
                                            ov_frame_data *data) {

    if ((0 == list) || (0 == data)) {
        goto error;
    }

    OV_ASSERT(0 != list->frames);

    ssize_t index_to_insert = -1;

    for (size_t i = 0; i < list->capacity; ++i) {

        if (0 == list->frames[i]) {
            index_to_insert = i;
            continue;
        }

        if (data->ssid == list->frames[i]->ssid) {

            ov_log_warning("Frame with SSID %" PRIu32 " already there",
                           data->ssid);
            index_to_insert = i;
            break;
        }
    }

    if (-1 < index_to_insert) {

        ov_frame_data *old = list->frames[index_to_insert];
        list->frames[index_to_insert] = data;
        data = old;
    }

    if (0 > index_to_insert) {
        ov_log_error("Frame data list full");
    }

error:

    return data;
}

/*----------------------------------------------------------------------------*/

ov_frame_data *ov_frame_data_list_pop_data(ov_frame_data_list *list,
                                           uint32_t sid) {

    if (0 == list)
        goto error;

    for (size_t i = 0; i < list->capacity; ++i) {

        if (0 == list->frames[i])
            continue;

        if (sid == list->frames[i]->ssid) {

            ov_frame_data *data = list->frames[i];
            list->frames[i] = 0;

            return data;
        }
    }

error:

    return 0;
}
