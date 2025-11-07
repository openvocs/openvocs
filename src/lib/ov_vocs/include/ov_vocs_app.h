/***
        ------------------------------------------------------------------------

        Copyright (c) 2025 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_vocs_app.h
        @author         Töpfer, Markus

        @date           2025-11-07


        ------------------------------------------------------------------------
*/
#ifndef ov_vocs_app_h
#define ov_vocs_app_h

typedef struct ov_vocs_app ov_vocs_app;

typedef struct ov_vocs_app_config {

    ov_event_loop *loop;
    
    ov_vocs_core *core;
    ov_vocs_env env;

    ov_vocs_db *db;
    ov_vocs_db_persistance *persistance;

    ov_ldap *ldap;

    struct {

        char path[PATH_MAX];

    } sessions;

    struct {

        uint64_t response_usec;

    } limits;

} ov_vocs_app_config;

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

ov_vocs_app *ov_vocs_app_create(ov_vocs_app_config config);
ov_vocs_app *ov_vocs_app_free(ov_vocs_app *self);
ov_vocs_app *ov_vocs_app_cast(const void *self);


#endif /* ov_vocs_app_h */
