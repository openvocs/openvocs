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

@author Michael J. Beer, DLR/GSOC
@copyright (c) 2020 German Aerospace Center DLR e.V. (GSOC)
@date 2020-09-21

@license Apache 2.0
@version 1.0.0

@status Development

"""

#------------------------------------------------------------------------------

import os
from sys import argv

#------------------------------------------------------------------------------

from rich.python3 import openvocs

#------------------------------------------------------------------------------

__filedir = os.path.dirname(os.path.abspath(__file__))
__scriptdir = os.path.dirname(os.path.abspath(argv[0]))

#-------------------------------------------------------------------------------

SESSION_NAME="recorder"

SIGNALLING_SERVER = {

        "host" : "localhost",
        "port" : 10001,

        }

RTP_CLI_RECEIVER_PORT = 11111

RECORDER_BINARY='{}/build/ov_recorder'.format(openvocs.get_root())

RECORDING_UUID='11fe55fb-2aee-44dd-1228-86564aaeaf36'
TEST_USER='hermodr'
TEST_LOOP='hel'
RTP_CLI_SEND_SSID=2311

INFILE1=None
INFILE2=None
