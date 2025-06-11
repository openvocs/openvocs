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
SS for Echo cancellator

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

from rich.python3.io.signalling_server import *
from rich.python3 import openvocs_requests

from rich.python3.service.tmux import Session

from configuration import *

#------------------------------------------------------------------------------

__filedir = os.path.dirname(os.path.abspath(__file__))
__scriptdir = os.path.dirname(os.path.abspath(argv[0]))

#------------------------------------------------------------------------------

def media_port_from_reply(msg):

    try:
        params = msg.get_reply_parameters()
        return params['media']['port']
    except:
        return "UNKNOWN"

#-------------------------------------------------------------------------------

def get_session():
    return Session(SESSION_NAME)

#-------------------------------------------------------------------------------

def start_rtp_sender(dest_port):

    print("Starting senders to port {}".format(dest_port))

    session = get_session()

    additional = "-t 2200"

    if INFILE1:
        additional="-a {}".format(INFILE1)

    session.start_rtp_sender(
            "send", 'localhost', dest_port, 13, additional=additional)

    additional = "-t 800"

    if INFILE2:
        additional="-a {}".format(INFILE2)

    session.start_rtp_sender(
            "send", 'localhost', dest_port, 1332, additional=additional)

#-------------------------------------------------------------------------------

def reconfigure_handler(server, event, msg, fd):

    media_port = media_port_from_reply(msg)

    start_rtp_sender(media_port)

#-------------------------------------------------------------------------------

initial_commands = [
        # 'msg reconfigure msid=1 host=0.0.0.0 port={} sync_header=true csrc_mode=ssrc'.format(MIXER_MEDIA_PORT),
        'msg reconfigure msid=1 host=0.0.0.0 sync_header=true csrc_mode=ssrc'.format(MIXER_MEDIA_PORT),
        'msg add_source ssid=1332',
        'msg add_source ssid=13',
        'msg add_destination host=127.0.0.1 port={} uuid=11fe55fb-2aee-44dd-8228-86564aaeaf15'.format(RTP_CLI_RECEIVER_PORT)
        ]

#-------------------------------------------------------------------------------

if __name__ == "__main__":

    parser=ArgumentParser(
            description="Start signalling server for echo cancellator")

    args = initialize_arguments(parser)

    args['port'] = SIGNALLING_SERVER['port']

    print(args)

    signalling_server, _ = start_signalling_server(
            **args, initial_commands=initial_commands)

    signalling_server.request_factory.add_requests(openvocs_requests.requests())

    signalling_server.request_factory.add_requests(
            openvocs_requests.requests('mixer'))

    signalling_server.event_handler('reconfigure', reconfigure_handler)

    signalling_server.serve_forever(poll_interval=0.1)
