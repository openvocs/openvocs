/***
        ------------------------------------------------------------------------

        Copyright (c) 2017 Markus Toepfer
        Copyright (c) 2018 German Aerospace Center DLR e.V. (GSOC)

        Licensed under the Apache License, Version 2.0 (the "License");
        you may not use this file except in compliance with the License.
        You may obtain a copy of the License at

                http://www.apache.org/licenses/LICENSE-2.0

        Unless required by applicable law or agreed to in writing, software
        distributed under the License is distributed on an "AS IS" BASIS,
        WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
        See the License for the specific language governing permissions and
        limitations under the License.

        This file is part of the openvocs project. http://openvocs.org

        ------------------------------------------------------------------------
*//**

        @file           ov_copyright.h
        @author         Markus Toepfer
        @date           2018-11-05

        @brief          Definition of a generic function set for copyright
                        statements, as well as a definition of some common
                        copyright statement functions.

        @NOTE           Implementation style: pointer base function, which may
                        be overriden with custom implementations for additional
                        copyright implementations.

        ------------------------------------------------------------------------
*/
#ifndef ov_copyright_h
#define ov_copyright_h

typedef struct ov_copyright ov_copyright;
typedef struct ov_copyright_gpl_v3_parameter ov_copyright_gpl_v3_parameter;
typedef struct ov_copyright_config ov_copyright_config;

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define COPYRIGHT_DEFAULT_INTRO "Copyright (c)"
#define COPYRIGHT_DEFAULT_TEXT "All rights reserved.\n"

/*
 *      ------------------------------------------------------------------------
 *
 *      Configuration data for GPL licences.
 *
 *      ------------------------------------------------------------------------
 */

typedef enum { GENERAL, LESSER, AFFERO } ov_source_file_gpl_v3_types;

struct ov_copyright_gpl_v3_parameter {

    ov_source_file_gpl_v3_types type;
    const char *program_name;
};

/*
 *      ------------------------------------------------------------------------
 *
 *      COPYRIGHT PARAMTER CONFIGURATION
 *
 *      ------------------------------------------------------------------------
 */

struct ov_copyright {

    char *(*generate_header_string)(const char *prefix, const char *intro,
                                    const char *year, const char *owner,
                                    const char *end_note, const char *suffix,
                                    uint16_t line_indent,
                                    bool indent_using_spaces,
                                    void *custom_data);

    char *(*generate_full_text_licence)(void *custom_data);
};

/*----------------------------------------------------------------------------*/

struct ov_copyright_config {

    const char *author;
    const char *owner;
    const char *note;

    ov_copyright copyright;
    ov_copyright_gpl_v3_parameter *gpl_parameter;
};

/**     ------------------------------------------------------------------------

        Default "generate_header_string"

        @param prefix           prefix to be written as is in front
        @param intro            e.g. "Copyright", "Copyright (c)"
        @param year             e.g. "2018", "2016,2017,2018"
        @param owner            e.g. "DLR e.V.", "Markus Toepfer"
        @param text             Copyright statement e.g. "All rights reserved."
        @param end_note         optional end note e.g. link to project homepage
        @param suffix           suffix to be written as is at the end
        @param indent           line indent to be set in front of each line,
                                except of prefix and suffix.
        @param spaces           if true, spaces will be added, tabs on false

        @returns                allocated c string or NULL
*/
char *ov_copyright_generate_header_string(const char *prefix, const char *intro,
                                          const char *year, const char *owner,
                                          const char *text,
                                          const char *end_note,
                                          const char *suffix, uint16_t indent,
                                          bool spaces);

/*      ------------------------------------------------------------------------
 *
 *      EXAMPLE IMPLEMENTATION OF SOME COPYRIGHTS
 *
 *      This part contains some common implementations, which may be used
 *      within custom tools and frameworks using ov_source_file_lib.
 *
 *      ------------------------------------------------------------------------
 */

/**     ------------------------------------------------------------------------

        Function set for a default copyright of form:

        ------------------------------------------------------------------------

        Copyright (c) YEAR NAME"
        All rights reserved.

        NOTE

        ------------------------------------------------------------------------

*/
struct ov_copyright ov_copyright_reserved();

/**     ------------------------------------------------------------------------

        Function set for a APACHE VERSION 2 copyright of form:

        ------------------------------------------------------------------------

        Copyright (c) YEAR NAME"

        Licensed under the Apache License, Version 2.0 (the "License");
        you may not use this file except in compliance with the License.
        You may obtain a copy of the License at

                http://www.apache.org/licenses/LICENSE-2.0

        Unless required by applicable law or agreed to in writing, software
        distributed under the License is distributed on an "AS IS" BASIS,
        WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
        See the License for the specific language governing permissions and
        limitations under the License.

        NOTE

        ------------------------------------------------------------------------

*/
struct ov_copyright ov_copyright_apache_version_2();

/**     ------------------------------------------------------------------------

        Function set for a BSD 3-Clause copyright of form:

        ------------------------------------------------------------------------

        Copyright (c) YEAR NAME

        Redistribution and use in source and binary forms, with or without
        modification, are permitted provided that the following conditions are
   met:
            * Redistributions of source code must retain the above copyright
              notice, this list of conditions and the following disclaimer.
            * Redistributions in binary form must reproduce the above copyright
              notice, this list of conditions and the following disclaimer in
   the documentation and/or other materials provided with the distribution.
            * Neither the name of the copyright holder nor the
              names of its contributors may be used to endorse or promote
   products derived from this software without specific prior written
   permission.

        THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
   IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
        DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
   DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
   SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
   CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
   OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

        NOTE

        ------------------------------------------------------------------------

*/
struct ov_copyright ov_copyright_bsd_3clause();

/**     ------------------------------------------------------------------------

        Function set for a BSD 2-Clause copyright of form:

        ------------------------------------------------------------------------

        Copyright (c) YEAR NAME

        Redistribution and use in source and binary forms, with or without
        modification, are permitted provided that the following conditions are
   met:

        1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
        2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

        THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
   IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
   LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE.

        NOTE

        ------------------------------------------------------------------------

*/
struct ov_copyright ov_copyright_bsd_2clause();

/**     ------------------------------------------------------------------------

        Function set for a MIT copyright of form:

        ------------------------------------------------------------------------

        Copyright (c) YEAR NAME

        Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"), to
   deal in the Software without restriction, including without limitation the
   rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
   sell copies of the Software, and to permit persons to whom the Software is
        furnished to do so, subject to the following conditions:

        The above copyright notice and this permission notice shall be included
   in all copies or substantial portions of the Software.

        THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
        FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
   THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
        LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   IN THE SOFTWARE.

        NOTE

        ------------------------------------------------------------------------

*/
struct ov_copyright ov_copyright_mit();

/**     ------------------------------------------------------------------------

        Function set for a GPL v3 copyright of form:

        ------------------------------------------------------------------------

        This file is part of [NAME]

        [NAME] is free software: you can redistribute it and/or modify
        it under the terms of the GNU [TYPE] Public License as published by
        the Free Software Foundation, either version 3 of the License, or
        (at your option) any later version.

        [NAME] is distributed in the hope that it will be useful,
        but WITHOUT ANY WARRANTY; without even the implied warranty of
        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
        GNU [TYPE] Public License for more details."

        You should have received a copy of the GNU [TYPE] Public License
        along with [PROGRAM_NAME]. If not, see <http://www.gnu.org/licenses/>.

        NOTE

        ------------------------------------------------------------------------

*/
struct ov_copyright ov_copyright_gpl_version_3();

#endif /* ov_copyright_h */
