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

        @author Michael J. Beer, DLR/GSOC
        @copyright (c) 2022 German Aerospace Center DLR e.V. (GSOC)

        Simple functionality:

        # Serialize:

        ```

        ov_value *value = ...; // If our serde deals with ov_values ...
        ov_result res = {0};
        ov_serde_data data = {
        .data_type = OUR_DATA_TYPE,
        .data = value,
        };

        if ( ! ov_serde_serialize(serde, &data, outstream, &res)) {
            ov_log_error("Could not write value to outstream: %s", res.message);
            ov_serde_clear_buffer(serde);
        };
        ```

        # Deserialize

        ```
        ov_serde_data *data = 0;
        ov_result res = {0};

        ElementParseState state = OV_SERDE_PROGRESS;

        while(OV_SERDE_PROGRESS == state) {
            ov_serde_add_raw(serde, get_data(), &res);
        }

        switch(state) {

            case OV_SERDE_ERROR:
                ov_serde_clear_buffer(serde);
                ov_log_error("Could not parse: %s", res.message);
                break;

            case OV_SERDE_PROGRESS:
                // cannot happen
                break;

            case OV_SERDE_END:
                while(data = ov_serde_pop_datum(serde, res)) {
                    process_data(data);
                }
                break;
        }

        if(0 != data) && (OUR_DATA_TYPE != data)) {
            ov_log_error("Unexpected data");
            return 0;
        } else {
            return data->data;
        }
        ```


        ------------------------------------------------------------------------
*/
#ifndef OV_SERDE_H
#define OV_SERDE_H

#include "ov_buffer.h"
#include "ov_result.h"
#include <inttypes.h>

/*----------------------------------------------------------------------------*/

typedef enum { OV_SERDE_ERROR, OV_SERDE_PROGRESS, OV_SERDE_END } ov_serde_state;

typedef struct {
    uint64_t data_type;
    void *data;
} ov_serde_data;

/*----------------------------------------------------------------------------*/

typedef struct ov_serde ov_serde;

struct ov_serde {

    uint64_t magic_bytes;

    /**
     * @param res optional - if there, might be filled with result
     */
    ov_serde_state (*add_raw)(ov_serde *self, ov_buffer const *raw,
                              ov_result *res);

    ov_serde_data (*pop_datum)(ov_serde *self, ov_result *res);

    bool (*clear_buffer)(ov_serde *self);

    /**
     * @param res optional - if there, might be filled with result
     */
    bool (*serialize)(ov_serde *self, int fh, ov_serde_data data,
                      ov_result *res);

    ov_serde *(*free)(ov_serde *self);
};

/*****************************************************************************
                                   FUNCTIONS
 ****************************************************************************/

ov_serde_state ov_serde_add_raw(ov_serde *self, ov_buffer const *raw,
                                ov_result *res);

/*----------------------------------------------------------------------------*/

extern ov_serde_data OV_SERDE_DATA_NO_MORE;

/**
 * Returns OV_SERDE_DATA_NO_MORE if none available
 */
ov_serde_data ov_serde_pop_datum(ov_serde *self, ov_result *res);

/*----------------------------------------------------------------------------*/

bool ov_serde_clear_buffer(ov_serde *self);

/*----------------------------------------------------------------------------*/

bool ov_serde_serialize(ov_serde *self, int fh, ov_serde_data data,
                        ov_result *res);

/*----------------------------------------------------------------------------*/

ov_serde *ov_serde_free(ov_serde *self);

/*----------------------------------------------------------------------------*/

char *ov_serde_to_string(ov_serde *self, ov_serde_data data, ov_result *res);

/*----------------------------------------------------------------------------*/
#endif
