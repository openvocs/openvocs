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

        ------------------------------------------------------------------------
*/
#ifndef OV_SIP_PERMISSION_H
#define OV_SIP_PERMISSION_H
/*----------------------------------------------------------------------------*/

#include <ov_base/ov_json_value.h>

typedef struct {

  char const *caller;
  char const *callee;
  char const *loop;
  uint64_t from_epoch;
  uint64_t until_epoch;

} ov_sip_permission;

/*----------------------------------------------------------------------------*/

ov_json_value *ov_sip_permission_to_json(ov_sip_permission self);

/*----------------------------------------------------------------------------*/

ov_sip_permission ov_sip_permission_from_json(ov_json_value const *jval,
                                              bool *ok);

/*----------------------------------------------------------------------------*/

bool ov_sip_permission_equals(const ov_sip_permission p1,
                              const ov_sip_permission p2);

/*----------------------------------------------------------------------------*/

ov_data_function ov_sip_permission_data_functions();

/*----------------------------------------------------------------------------*/
#endif
