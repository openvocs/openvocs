#!/usr/bin/env bash
#------------------------------------------------------------------------------
#
# Copyright (c) 2024 German Aerospace Center DLR e.V. (GSOC)
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#         http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# This file is part of the openvocs project. https://openvocs.org
#
#------------------------------------------------------------------------------
#
# Replaces variables in text files.
# Minimalistic - one call to 'sed' per variable - see below for supported
# variables
#
# @author Michael J. Beer, DLR/GSOC
# @copyright (c) 2024 German Aerospace Center DLR e.V. (GSOC)
# @date 2024-01-22
#
# @license Apache 2.0
# @version 1.0.0
#
# @status Development
#
#------------------------------------------------------------------------------

OV_VERSION=$1
OV_ARCH=$2
OV_BUILD_ID=$3

if [[ "X$OV_VERSION" == "X" ]]; then
    OV_VERSION="$0:VERSION_UNKNOWN-require_it_as_first_argument"
fi

if [[ "X$OV_ARCH" == "X" ]]; then
    OV_ARCH="$0:ARCHITECTURE_UNKNOWN-require_it_as_second_argument"
fi

if [[ "X$OV_BUILD_ID" == "X" ]]; then
    OV_BUILD_ID="$0:BUILD_ID_UNKNOWN-require_it_as_third_argument"
fi

sed -e "s|@OV_DEB_ARCH@|$OV_ARCH|" | \
    sed -e "s|@OV_RPM_ARCH@|$OV_ARCH|" | \
    sed -e "s|@OV_VERSION@|$OV_VERSION|" | \
    sed -e "s|@OV_BUILD_ID@|$OV_BUILD_ID|"


