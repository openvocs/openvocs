/***
        ------------------------------------------------------------------------

        Copyright (c) 2021 German Aerospace Center DLR e.V. (GSOC)

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
        @file           ov_dir.h

        @author         Michael Beer
        @author         Markus Töpfer

        @date           2021-02-12


        ------------------------------------------------------------------------
*/
#ifndef ov_dir_h
#define ov_dir_h

#include <stdbool.h>

/*
 *      ------------------------------------------------------------------------
 *
 *      GENERIC FUNCTIONS
 *
 *      ------------------------------------------------------------------------
 */

/**
    Remove some directory tree with all files and subdirectories.

    NOTE this function will work only on directories with file and dir
    content, but fail is some mnt points or sockets are contained
    within the tree on purpose.

    NOTE this is rm -rf for some directory tree with contained files

    @param path     path to remove
*/
bool ov_dir_tree_remove(const char *path);

/*----------------------------------------------------------------------------*/

/**
    Ensures that path exists as a directory
    @return true if path exists when returning from function, false otherwise
 */
bool ov_dir_tree_create(char const *path);

/*----------------------------------------------------------------------------*/

/**
    Check if some path is a dir and access to the dir is given
*/
bool ov_dir_access_to_path(const char *path);

#endif /* ov_dir_h */
