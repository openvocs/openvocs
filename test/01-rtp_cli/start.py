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

from rich.python3.service.tmux import Session

#------------------------------------------------------------------------------

__filedir = os.path.dirname(os.path.abspath(__file__))
__scriptdir = os.path.dirname(os.path.abspath(argv[0]))

#------------------------------------------------------------------------------

def get_session():

    return Session('rtp_cli')

#-------------------------------------------------------------------------------

def start_session(infile=None):

    session = get_session()

    additional = '-t 1200'

    if infile:

        print("Playing {}".format(infile))
        additional = '-a {}'.format(infile)

    session.start_rtp_sender("send", 'localhost', 11111, 13, additional=additional)
    session.start_rtp_receiver("recv", 'localhost', 11111)

    print("Running in session {}".format(session.get_name()))

    return session

#-------------------------------------------------------------------------------

if __name__ == "__main__":

    parser=ArgumentParser(description="Start rtp_cli test session")
    parser.add_argument('--infile', metavar='infile', help="File to replay instead of sine test signal")
    parser.add_argument('--kill', action='store_true', help="Instead of start test session, kill running session")

    args = parser.parse_args()
    args = vars(args)

    if args['kill']:
        get_session().kill()
        exit(0)

    if 'infile' in args:
        start_session(infile=args['infile'])

    start_session().attach()

