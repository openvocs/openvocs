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
import tempfile
from sys import argv
from argparse import ArgumentParser
import json

#------------------------------------------------------------------------------

from rich.python3.service.tmux import Session
from rich.python3 import openvocs
import rich.python3.service_configuration as configuration

#-------------------------------------------------------------------------------

__filedir = os.path.dirname(os.path.abspath(__file__))
__scriptdir = os.path.dirname(os.path.abspath(argv[0]))

###############################################################################
#                                CONFIGURATION
###############################################################################

CONFIG_FILE_TEMPLATE = "{}/ov_{}_config.json"
TEMP_DIR = tempfile.gettempdir()

CONFIG_FILES = {
        'mixer' : CONFIG_FILE_TEMPLATE.format(TEMP_DIR, 'mixer'),
        'echo_cancellator' : CONFIG_FILE_TEMPLATE.format(
            TEMP_DIR, 'echo_cancellator'),
        'resource_manager' : CONFIG_FILE_TEMPLATE.format(
            TEMP_DIR, 'resource_manager'),
        }

SESSION_NAME="backend_test"

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

###############################################################################
#                                   HELPERS
###############################################################################

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

_session = None

#-------------------------------------------------------------------------------

def start(service):

    _session.start_process(service,
        '{}/build/ov_{} -c {}'.format(openvocs.get_root(), service,
                CONFIG_FILES[service]))


#-------------------------------------------------------------------------------

def socket_to_str(socket):

    return "{}:{}".format(socket['host'], socket['port'])

#-------------------------------------------------------------------------------

if __name__ == "__main__":

    parser=ArgumentParser(description="""Start backend test session.

       BEWARE: Requires running tmux sesssion.

       If no tmux session is running, bring up a terminal, ensure tmux is
       installed, and type

       tmux

       then get back to execute this script...

       """)

    parser.add_argument('--kill', action='store_true',
            help="Instead of start test session, kill running session")

    parser.add_argument(
            '--mixers',
            type=int,
            metavar='mixers',
            default=3,
            help="Number of mixers to start")
    parser.add_argument(
            '--ecs',
            type=int,
            metavar='ecs',
            default=3,
            help="Number of echo cancellators to start")

    args = parser.parse_args()
    args = vars(args)

    _session =  Session(SESSION_NAME)

    if args['kill']:
        _session.kill()
        exit(0)

    num_mixers = args['mixers']
    num_ecs = args['ecs']

    generate_config()

    liege_socket = socket_to_str(RESMGR_CONFIG['liege'])
    monitor_socket = socket_to_str(RESMGR_CONFIG['monitor'])

    print(
            (f"Resource manager: Liege at: {liege_socket}    "
                f"Monitor at: {monitor_socket}"))

    start('resource_manager')

    for i in range(args['mixers']):
        start('mixer')

    for i in range(num_ecs):
        start('echo_cancellator')

    _session.attach()

