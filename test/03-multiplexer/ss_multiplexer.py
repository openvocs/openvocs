#!/usr/local/bin/python3.7
#     ------------------------------------------------------------------------
#
#     Copyright (c) 2019 German Aerospace Center DLR e.V. (GSOC)
#
#     Licensed under the Apache License, Version 2.0 (the "License");
#     you may not use this file except in compliance with the License.
#     You may obtain a copy of the License at
#
#             http://www.apache.org/licenses/LICENSE-2.0
#
#     Unless required by applicable law or agreed to in writing, software
#     distributed under the License is distributed on an "AS IS" BASIS,
#     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#     See the License for the specific language governing permissions and
#     limitations under the License.
#
#     This file is part of the openvocs project. https://openvocs.org
#
#     ------------------------------------------------------------------------
#
#     @file           ss.py
#     @author         Michael J. Beer
#
#     @date           2019-08-30
#
#     ------------------------------------------------------------------------

"""
SS for Multiplexer

@author Michael J. Beer, DLR/GSOC
@copyright 2020 German Aerospace Center DLR e.V. (GSOC)
@date 2020-07-28

@license Apache License 2.0
@version 1.0.0

@status Development

"""

#------------------------------------------------------------------------------

import os
from sys import argv

#-------------------------------------------------------------------------------

from configuration import *

from rich.python3.io.signalling_server import *
from rich.python3 import openvocs_requests
from rich.python3.service.tmux import *

#-------------------------------------------------------------------------------

def get_session():
    return Session(SESSION_NAME)

#------------------------------------------------------------------------------

def media_port_from_reply(msg):

    try:
        params = msg.get_reply_parameters()
        return params['media']['port']
    except:
        return "UNKNOWN"

#-------------------------------------------------------------------------------

def start_sender(ec_media_port, msid):

    session = get_session()

    additional = '-t 800'

    if INFILE2:
        additional="-a {}".format(INFILE2)

    session.start_rtp_sender(
            "send", "127.0.0.1", ec_media_port, msid, additional=additional)

#-------------------------------------------------------------------------------

def reconfigure_handler(server, event, msg, fd):

    media_port = media_port_from_reply(msg)

    start_sender(media_port, RTP_CLI_SENDER_SID)

#-------------------------------------------------------------------------------

initial_commands = [
    'msg reconfigure host=0.0.0.0',
    'msg add_source ssid={}'.format(RTP_CLI_SENDER_SID),
    'msg add_destination host=127.0.0.1 port={}'.format(RTP_CLI_RECEIVER_PORT),
    'msg add_destination host=127.0.0.1 port={}'.format(RTP_CLI_RECEIVER_PORT + 1),
]

#-------------------------------------------------------------------------------

if __name__ == "__main__":

    parser=ArgumentParser(
            description="Start signalling server for echo cancellator")

    args = initialize_arguments(parser)

    args['port'] = SIGNALLING_SERVER_MULTIPLEXER['port']

    print(args)

    signalling_server, _ = start_signalling_server(
            **args, initial_commands=initial_commands)

    signalling_server.request_factory.add_requests(openvocs_requests.requests())

    signalling_server.request_factory.add_requests(
            openvocs_requests.requests('multiplexer'))

    signalling_server.event_handler('reconfigure', reconfigure_handler)

    signalling_server.serve_forever(poll_interval=0.1)
