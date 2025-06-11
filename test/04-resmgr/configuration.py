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

import os
from sys import argv

#------------------------------------------------------------------------------

from rich.python3 import openvocs

#------------------------------------------------------------------------------

__filedir = os.path.dirname(os.path.abspath(__file__))
__scriptdir = os.path.dirname(os.path.abspath(argv[0]))

#-------------------------------------------------------------------------------

SESSION_NAME="resmgr"

# BEWARE: DON'T use symbolic host names like 'localhost', as they unpredictably
# might be resolved to IPv4 or IPv6 addresses

RESMGR_CONFIG = {

        "minions": {
            "host": "127.0.0.1",
            "port": 10001
        },
        "liege": {
            "host": "127.0.0.1",
            "port": 10002,
            "type": "TCP"
        },
        "monitor": {
            "host": "127.0.0.1",
            "port": 10003,
            "type": "TCP"
        }
    }

USERS = {
        'ymir' : {

            'msid' : 13,
            'host' : '127.0.0.1',
            'port' : 11112,

        },

        'buri' : {
            'msid' : 1332,
            'host' : '127.0.0.1',
            'port' : 11113,
        },

    }

LOOPS = [ 'ratatoskr' ]

NUM_ECS_TO_START = len(USERS)
NUM_MIXERS_TO_START = len(USERS) + len(LOOPS)

BINARY = {
        'mixer' : f'{openvocs.get_root()}/build/ov_mixer',
        'echo_cancellator' : f'{openvocs.get_root()}/build/ov_echo_cancellator',
        'resmgr' : f'{openvocs.get_root()}/build/ov_resmgr'
}
