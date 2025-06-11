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
import python.service.configuration as configuration

from configuration import *

#------------------------------------------------------------------------------

__scriptdir = os.path.dirname(os.path.abspath(argv[0]))

#-------------------------------------------------------------------------------

CONFIG_FILE_TEMPLATE = "{}/ov_{}_config.json"

CONFIG_FILES = {
        'multiplexer' : CONFIG_FILE_TEMPLATE.format(
            __scriptdir, 'multiplexer'),
        }

#-------------------------------------------------------------------------------

def generate_config():

    config_template = {

            "multiplexer" : {
                "signalling_server" : SIGNALLING_SERVER_MULTIPLEXER,
                },
            }

    def create_config_file(service_type):

        config = configuration.get_config(service_type, config=config_template)
        config = json.dumps(config, indent=4)

        config_file = CONFIG_FILES[service_type]

        with open(config_file, mode='w') as target:
            target.write(str(config))
            target.write("\n")

    create_config_file("multiplexer")

#-------------------------------------------------------------------------------

def get_session():
    return Session(SESSION_NAME)

#-------------------------------------------------------------------------------

def start_ss_multiplexer():

    get_session().start_process(
            "ss_multiplexer",
            "python3 {}/ss_multiplexer.py".format(__scriptdir))

#-------------------------------------------------------------------------------

def start_multiplexer():

    get_session().start_process("multiplexer",
            "{} -c {}".format(MULTIPLEXER_BINARY,
                CONFIG_FILES['multiplexer']))

#-------------------------------------------------------------------------------

def start_receiver():
    get_session().start_rtp_receiver("recv", "127.0.0.1", RTP_CLI_RECEIVER_PORT)

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
    start_multiplexer()
    start_ss_multiplexer()
    start_receiver()

    get_session().attach()

