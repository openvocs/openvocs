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
        @file           ov_vocs_env.h
        @author         Markus Töpfer

        @date           2024-01-10


        ------------------------------------------------------------------------
*/
#ifndef ov_vocs_env_h
#define ov_vocs_env_h

/*----------------------------------------------------------------------------*/

typedef struct ov_vocs_env {

    void *userdata;

    bool (*close)(void *userdata, int socket);
    bool (*send)(void *userdata, int socket, const ov_json_value *msg);

} ov_vocs_env;

#endif /* ov_vocs_env_h */
