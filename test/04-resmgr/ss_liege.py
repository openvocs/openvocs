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

def user_name_from_acquire_user(msg):

    return msg.get_parameters()['name']

#-------------------------------------------------------------------------------

def user_msid_from_acquire_user(msg):

    return msg.get_parameters()['msid']

#-------------------------------------------------------------------------------

def user_recv_socket_from_acquire_user(msg):

    params = msg.get_parameters()
    params = params['media']

    return (params['host'], params['port'])

#-------------------------------------------------------------------------------

def backend_input_socket_from_acquire_user(msg):

    params = msg.get_reply_parameters()
    return (params['media']['host'], params['media']['port'])

#-------------------------------------------------------------------------------

def backend_output_msid_from_acquire_user(msg):

    params = msg.get_reply_parameters()
    return params['msid']

#-------------------------------------------------------------------------------

_session = Session(SESSION_NAME)

#-------------------------------------------------------------------------------

def start_user_clients(server, event, msg, fd):

    error_code, error_msg = msg.get_reply_error()

    if 0 != error_code:
        log_error("Could not acquire user: {}".format((error_code, error_msg)))
        return

    try:
        user_name = user_name_from_acquire_user(msg)
        user_msid = user_msid_from_acquire_user(msg)
        out_host, out_port = user_recv_socket_from_acquire_user(msg)
        in_host, in_port = backend_input_socket_from_acquire_user(msg)
        backend_out_msid = backend_output_msid_from_acquire_user(msg)
    except:
        log_error("Acquire-user seems to have failed: Unrecognizeable reply")
        return

    _session.start_rtp_sender(
        "send-{}".format(user_name), in_host, in_port, user_msid, additional="-t 800")

    _session.start_rtp_receiver("recv-{}".format(user_name), out_host, out_port)

#-------------------------------------------------------------------------------

if __name__ == "__main__":

    parser=ArgumentParser(
            description="Start signalling server for echo cancellator")

    args = initialize_arguments(parser)

    port = RESMGR_CONFIG['liege']['port']
    host = RESMGR_CONFIG['liege']['host']

    print("Starting sclient to {}:{}".format(host, port))

    initial_commands = []

    for name, user in USERS.items():

        initial_commands.append(
                'msg acquire_user msid={} host={} port={} name={}'.format(
                    user['msid'], user['host'], user['port'], name))

    for loop in LOOPS:
        initial_commands.append("msg acquire_loop name={}".format(loop))

    signalling_server, _ = start_signalling_server(
        host=host, port=port, client=True, initial_commands=initial_commands)

    signalling_server.request_factory.add_requests(openvocs_requests.requests())

    signalling_server.request_factory.add_requests(
        openvocs_requests.requests('liege'))

    signalling_server.event_handler('acquire_user', start_user_clients)

    signalling_server.serve_forever(poll_interval=0.1)
