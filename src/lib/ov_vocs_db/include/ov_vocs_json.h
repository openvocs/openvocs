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
        @file           ov_vocs_json.h
        @author         Markus Töpfer

        @date           2022-07-27


        ------------------------------------------------------------------------
*/
#ifndef ov_vocs_json_h
#define ov_vocs_json_h

#include <ov_base/ov_json.h>

const char *ov_vocs_json_get_id(const ov_json_value *val);
bool ov_vocs_json_set_id(ov_json_value *val, const char *id);

bool ov_vocs_json_set_session_id(ov_json_value *val, const char *id);

const char *ov_vocs_json_get_name(const ov_json_value *val);
const char *ov_vocs_json_get_domain_id(const ov_json_value *project);

bool ov_vocs_json_is_admin_user(const ov_json_value *val, const char *user);

#endif /* ov_vocs_json_h */
