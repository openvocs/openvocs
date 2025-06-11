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
from argparse import ArgumentParser
import json

#-------------------------------------------------------------------------------

from rich.python3.service.tmux import Session
import rich.python3.service_configuration as configuration

from configuration import *
#------------------------------------------------------------------------------

__filedir = os.path.dirname(os.path.abspath(__file__))
__scriptdir = os.path.dirname(os.path.abspath(argv[0]))

CONFIG_FILE = "{}/ov_recorder_config.json".format(__scriptdir)

#-------------------------------------------------------------------------------

def generate_config():

    config = {
        "recorder" : {
            "signalling_server" : SIGNALLING_SERVER,
        },
    }

    config = configuration.get_config("recorder", config=config)

    config = json.dumps(config, indent=4)

    with open(CONFIG_FILE, mode='w') as target:
        target.write(str(config))
        target.write("\n")

#-------------------------------------------------------------------------------

def get_session():
    return Session(SESSION_NAME)

#-------------------------------------------------------------------------------

def start_ss():
    get_session().start_process(
            "ss_recorder", "python3 {}/ss_recorder.py".format(__scriptdir))

#-------------------------------------------------------------------------------

def start_recorder():
    get_session().start_process("recorder",
            "{} -c {}".format(RECORDER_BINARY, CONFIG_FILE))

#-------------------------------------------------------------------------------

def start_receiver():
    get_session().start_rtp_receiver("recv", "localhost", RTP_CLI_RECEIVER_PORT)

#-------------------------------------------------------------------------------

if __name__ == "__main__":

    parser=ArgumentParser(description="Start recorder test session")

    parser.add_argument('--kill', action='store_true',
        help="Instead of start test session, kill running session")

    args = parser.parse_args()
    args = vars(args)

    if args['kill']:
        get_session().kill()
        exit(0)

    generate_config()
    start_ss()
    start_recorder()
    start_receiver()

    get_session().attach()

