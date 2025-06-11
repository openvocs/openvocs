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

from argparse import ArgumentParser
import json

#-------------------------------------------------------------------------------

from rich.python3.service.tmux import Session
import python.service.configuration as configuration

from configuration import *

#-------------------------------------------------------------------------------

__filedir = os.path.dirname(os.path.abspath(__file__))
__scriptdir = os.path.dirname(os.path.abspath(argv[0]))

CONFIG_FILE = f"{__scriptdir}/ov_mixer_config.json"

#-------------------------------------------------------------------------------

def generate_config():

    config = {
        "mixer" : {
            "signalling_server" : SIGNALLING_SERVER,
        },
    }

    config = configuration.get_config("mixer", config=config)

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
            "ss_mixer", "python3 {}/ss_mixer.py".format(__scriptdir))

#-------------------------------------------------------------------------------

def start_mixer():
    get_session().start_process("mixer",
            "{} -c {}".format(MIXER_BINARY, CONFIG_FILE))

#-------------------------------------------------------------------------------

def start_receiver():
    get_session().start_rtp_receiver("recv", "localhost", RTP_CLI_RECEIVER_PORT)

#-------------------------------------------------------------------------------

if __name__ == "__main__":

    parser=ArgumentParser(description="Start mixer test session")

    parser.add_argument('--kill', action='store_true',
        help="Instead of start test session, kill running session")

    args = parser.parse_args()
    args = vars(args)

    if args['kill']:
        get_session().kill()
        exit(0)

    generate_config()
    start_ss()
    start_mixer()
    start_receiver()

    get_session().attach()

