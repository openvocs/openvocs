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
    @file           ov_os.h
    @author         Markus Töpfer

    @date           2020-05-22


    ------------------------------------------------------------------------
    */
#ifndef ov_os_h
#define ov_os_h

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

/**
 * Try to spawn a new procname
 * @return PID on success, negative value in case of error
 *
 * Error values include
 *
 * - OV_ERROR_BAD_ARG : Arguments are invalid - most likely 0 pointers
 * - OV_ERROR_CODE_NOT_FOUND_ERROR : Binary not found
 * - OV_ERROR_CODE_UNKNOWN_ERROR : Unknown error occured
 */
int ov_os_spawn(char const *working_dir, char const *procname,
                char const *const *cmdline);

#endif /* ov_os_h */
