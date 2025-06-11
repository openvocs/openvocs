#!/usr/bin/env bash
#------------------------------------------------------------------------------
#
# Copyright (c) 2023 German Aerospace Center DLR e.V. (GSOC)
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
# @author Michael J. Beer, DLR/GSOC
# @copyright (c) 2023 German Aerospace Center DLR e.V. (GSOC)
#
#------------------------------------------------------------------------------

CONFDIR=/opt/openvocs/config

function start_mixers() {
    for i in `seq 20`; do
        $OPENVOCS_ROOT/build/bin/ov_mc_mixer -c $CONFDIR/ov_mc_mixer/config.json &
    done
}

$OPENVOCS_ROOT/build/bin/ov_mc_vocs -c $CONFDIR/ov_mc_vocs/config.json &
$OPENVOCS_ROOT/build/bin/ov_mc_ice_proxy -c $CONFDIR/ov_mc_ice_proxy/config.json &
start_mixers
