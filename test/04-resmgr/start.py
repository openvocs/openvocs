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
import time
from sys import argv, path
from argparse import ArgumentParser
import json

#------------------------------------------------------------------------------

from rich.python3.service.tmux import Session
from rich.python3 import openvocs
import rich.python3.service_configuration as configuration

from configuration import *

#-------------------------------------------------------------------------------

__filedir = os.path.dirname(os.path.abspath(__file__))
__scriptdir = os.path.dirname(os.path.abspath(argv[0]))

#-------------------------------------------------------------------------------

CONFIG_FILE_TEMPLATE = "{}/ov_{}_config.json"

CONFIG_FILES = {
        'mixer' : CONFIG_FILE_TEMPLATE.format(__scriptdir, 'mixer'),
        'echo_cancellator' : CONFIG_FILE_TEMPLATE.format(
            __scriptdir, 'echo_cancellator'),
        'resource_manager' : CONFIG_FILE_TEMPLATE.format(
            __scriptdir, 'resource_manager'),
        }

#-------------------------------------------------------------------------------

def generate_config():

    config_template = {
            "mixer" : {
                "signalling_server" : RESMGR_CONFIG['minions'],
                },

            "echo_cancellator" : {
                "signalling_server" : RESMGR_CONFIG['minions'],
                },

            "resource_manager" : RESMGR_CONFIG,

            }

    def create_config_file(service_type):

        config = configuration.get_config(service_type, config=config_template)
        config = json.dumps(config, indent=4)

        config_file = CONFIG_FILES[service_type]

        with open(config_file, mode='w') as target:
            target.write(str(config))
            target.write("\n")

    create_config_file("mixer")
    create_config_file("echo_cancellator")
    create_config_file("resource_manager")

#-------------------------------------------------------------------------------

_session = Session(SESSION_NAME)

#-------------------------------------------------------------------------------

def start(service):

    _session.start_process(service,
        f'{openvocs.get_root()}/build/ov_{service} -c {CONFIG_FILES[service]}')

#-------------------------------------------------------------------------------

def start_ss():

    _session.start_process("ss_liege", f"python3 {__scriptdir}/ss_liege.py")

#-------------------------------------------------------------------------------

def start_non_cancelled_sender():

    additional = '-t 2200'

    if INFILE1:
        additional="-a {}".format(INFILE1)

    _session.start_rtp_sender(
            "send", "localhost", ECHO_CANCELLATOR_BINARY,
            NON_CANCELLED_MSID, additional=additional)

#-------------------------------------------------------------------------------

def start_receiver():
    _session.start_rtp_receiver("recv", "localhost", RTP_CLI_RECEIVER_PORT)

#-------------------------------------------------------------------------------

if __name__ == "__main__":

    parser=ArgumentParser(description="Start mixer test session")

    parser.add_argument('--kill', action='store_true',
            help="Instead of start test session, kill running session")

    args = parser.parse_args()
    args = vars(args)

    if args['kill']:
        _session.kill()
        exit(0)

    generate_config()

    start('resource_manager')

    for i in range(NUM_MIXERS_TO_START):
        start('mixer')

    for i in range(NUM_ECS_TO_START):
        start('echo_cancellator')

    #Allow minions to register with resmgr
    time.sleep(20)

    start_ss()

    _session.attach()

