#!/usr/bin/env bash
#------------------------------------------------------------------------------
#
# Copyright (c) 2022 German Aerospace Center DLR e.V. (GSOC)
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
# Comprehensive description of this python script
#
# @author Michael J. Beer, DLR/GSOC
# @copyright (c) 2022 German Aerospace Center DLR e.V. (GSOC)
# @date 2022-03-30
#
# @license Apache 2.0
# @version 1.0.0
#
# @status Development
#
#------------------------------------------------------------------------------

SD_UNIT_FILES_TO_ENABLE="ov_ice_proxy ov_resource_manager ov_stun_server ov_vocs_service"

SD_UNIT_FILES="$SD_UNIT_FILES_TO_ENABLE ov_mixer@ ov_multiplexer@"
SD_TARGET_FILES="ov_mixer ov_multiplexer"


for unit in $SD_UNIT_FILES; do
    sudo cp resources/systemd/$unit.service /etc/systemd/system
done

for target in $SD_TARGET_FILES; do
    sudo cp resources/systemd/$target.target /etc/systemd/system
done

sudo systemctl daemon-reload

for unit in $SD_UNIT_FILES_TO_ENABLE; do
    sudo systemctl enable $unit
done

for target in $SD_TARGET_FILES; do
    sudo systemctl enable $target.target
done

for unit in $SD_UNIT_FILES_TO_ENABLE; do
    sudo systemctl restart $unit
done

for target in $SD_TARGET_FILES; do
    sudo systemctl restart $target.target
done
