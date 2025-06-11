#!/usr/bin/env bash

###############################################################################
#
#   Copyright   2019    German Aerospace Center DLR e.V.,
#                       German Space Operations Center  GSOC
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
#
#   This file is part of the openvocs project. http://openvocs.org
#
# \author             Michael J. Beer, DLR/GSOC
# \date               2019-04-08
#
###############################################################################

CSRC_DIR=_src

rm -rf $CSRC_DIR
mkdir $CSRC_DIR

C_FILES=$(find */ -name *.c)

for F in $C_FILES; do
    TARGET=$(echo "$F" | sed -e 's|.*/\(.*\)|\1|' )
    ln $F $CSRC_DIR/$TARGET
done
