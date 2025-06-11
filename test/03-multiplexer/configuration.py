#!/usr/local/bin/python3.7
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

"""
Comprehensive description of this python script

@author Michael J. Beer, DLR/GSOC
@copyright (c) 2020 German Aerospace Center DLR e.V. (GSOC)
@date 2020-09-21

@license Apache 2.0
@version 1.0.0

@status Development

"""

#------------------------------------------------------------------------------

from rich.python3 import openvocs

#-------------------------------------------------------------------------------

SESSION_NAME="multiplexer"

SIGNALLING_SERVER_MULTIPLEXER = {

        "host" : "127.0.0.1",
        "port" : 30001,

        }

RTP_CLI_SENDER_SID = 2144

RTP_CLI_RECEIVER_PORT = 11111

MULTIPLEXER_BINARY=f'{openvocs.get_root()}/build/ov_multiplexer'

INFILE1=None
INFILE2=None
# INFILE2="{}/test/audio_samples/build/huginn_munin.pcm16s".format(openvocs.get_root())
