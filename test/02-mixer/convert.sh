#!/usr/bin/env bash
#------------------------------------------------------------------------------
#
# Copyright (c) 2020 German Aerospace Center DLR e.V. (GSOC)
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
# @copyright (c) 2020 German Aerospace Center DLR e.V. (GSOC)
# @date 2020-09-15
#
# @license Apache 2.0
# @version 1.0.0
#
# @status Development
#
#------------------------------------------------------------------------------

./convert16 /tmp/in_1.pcm16 > in_1
./convert16 /tmp/in_2.pcm16 > in_2

./convert16 /tmp/out.pcm16 > out
./convert32 /tmp/out.pcm32 > out_32
