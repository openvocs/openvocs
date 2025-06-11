#!/usr/local/bin/python3.7
# -----------------------------------------------------------------------------
#
# Copyright (c) 2022 German Aerospace Center DLR e.V. (GSOC)
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
# -----------------------------------------------------------------------------

"""
Comprehensive description of this python script

@author Michael J. Beer, DLR/GSOC
@copyright (c) 2022 German Aerospace Center DLR e.V. (GSOC)
@date 2022-02-10

@license Apache 2.0
@version 1.0.0

@status Development

"""

# ------------------------------------------------------------------------------

from __future__ import annotations # Refer to type of class within class itself

# -----------------------------------------------------------------------------

from sys import version_info as python_version
assert (3, 7, 0) <= python_version

import json
import time

from typing import Callable, Optional, Iterator, Tuple
from argparse import ArgumentParser
from subprocess import Popen
from tempfile import gettempdir
from random import choices
from string import ascii_letters
from os import remove as remove_file

from python.openvocs import panic
from python.service import start_client, start_service, remove_stopped
from python.service import ServiceType
from python.service import configuration

from rich.python3.io.signalling_server import SignallingServer
from rich.python3.io.signalling_server import start_signalling_server
from rich.python3 import openvocs_requests


# -----------------------------------------------------------------------------

def parse_command_line():

    """
    """

    output_file = "mixed.pcm16s_be"

    parser = ArgumentParser(description=f"""

    Use a mixer to mix N input files into one single file.

    For testing the mixer implementation.

    """)

    parser.add_argument('--inputs', metavar='inputs', type=str,
            required=True,
            help="List of input files names, "
            "separated by comma (','). Should be raw signed integer 16bit BE")

    parser.add_argument('--output', metavar='output', type=str,
            default=output_file,
            help=f"Name of output file. Defaults to {output_file}")

    return vars(parser.parse_args())

# ------------------------------------------------------------------------------

def file_exists(path: str) -> bool:

    try:
        with open(path) as _:
            return True
    except:
        return False

# ------------------------------------------------------------------------------

def get_input_file_names(inputs_str : str) -> list[str]:

    inputs = inputs_str.split(',')

    # Check whether files exist

    for i in inputs:
        if not file_exists(i):
            panic(f"File does not exist: {i}")

    return inputs

# ------------------------------------------------------------------------------

def generate_sids(no : int) -> list[int]:

    return [i for i in range(2, no + 2)]

# ------------------------------------------------------------------------------

running_processes = []

# ------------------------------------------------------------------------------

def start_record_client(port : int, output_file_name : str) -> Popen:

    p = start_client(host='localhost', port=port, output=output_file_name)

    if not p:
        panic("Could not start recording client")

    running_processes.append(p)

    return p

# ------------------------------------------------------------------------------


def start_send_client(port: int, input_file_name: str, sid: int) -> Popen:

    p = start_client(
            host='localhost', port=port, input=input_file_name, sid=sid)

    if not p:
        panic("Could not start sending client")

    running_processes.append(p)

    return p

# ------------------------------------------------------------------------------


def process_name(p: Popen):

    if isinstance(p.args, list):
        return p.args[0]

    return p.args

# ------------------------------------------------------------------------------

def prepare_config_for_mixer(ss_port: int) -> str:

    config = {
                "mixer": {
                    "ss": {
                        "host": "127.0.0.1",
                        "port": ss_port,
                        },
                    "lock_timeout_msecs": 2000,
                    "log": {

                        "systemd": False,
                        "level": "debug",
                        "file": "/tmp/ov_mixer.log"
                    }
                }
        }

    config = configuration.get_config("mixer", config=config)
    config = json.dumps(config, indent=4)

    random_file_name = ''.join(choices(ascii_letters, k=7))
    tempfile = f"{gettempdir()}/{random_file_name}"

    with open(tempfile, mode='w') as target:
            target.write(str(config))
            target.write("\n")

    print(f"Using config file {tempfile}")

    return tempfile

# ------------------------------------------------------------------------------

def prepare_mixer(ss_port: int) -> Popen:

    path_of_config_file = prepare_config_for_mixer(ss_port)
    p = start_service(ServiceType.MIXER, path_of_config_file)
    time.sleep(3)
    remove_file(path_of_config_file)
    return p

# -----------------------------------------------------------------------------

def media_port_from_reply(msg) -> Optional[int]:

    try:
        params = msg.get_reply_parameters()
        return int(params['media']['port'])
    except:
        return None

# ------------------------------------------------------------------------------

def make_add_destination_handler(
        record_port: int,
        mixer_media_port: int,
        inputs: Iterator[Tuple[str, int]],
        output_file: str) -> Callable:

    """
    Create a callback for 'add_destination'

    Starts all sender/receiver processes
    """


    def add_destination_handler(
            server: SignallingServer,
            event: str,
            msg,
            fd: int) -> None:

        print("About to start processes")
        if not start_record_client(record_port, output_file):
            panic("Could not start receiver")

        for infile, sid in inputs:
            if not start_send_client(mixer_media_port, infile, sid):
                panic(f"Could not start sender for {infile}")

    return add_destination_handler

# ------------------------------------------------------------------------------

if __name__ == "__main__":

    cmd_args = parse_command_line()

    input_file_names = get_input_file_names(cmd_args['inputs'])
    output_file_name = cmd_args['output']

    ss_port = 32144
    mixer_media_port = 61942
    record_port = 46132

    # Mixer port retrieved from mixer after startup
    mixer_process = prepare_mixer(ss_port)

    assert mixer_process

    running_processes.append(mixer_process)

    # Start SS and have it handle all further setup

    initial_commands = [
            f'msg reconfigure msid=1 host=127.0.0.1 port={mixer_media_port}']

    no_inputs = len(input_file_names)
    assert no_inputs > 0

    sids_to_use_for_replay_files = generate_sids(no_inputs)
    assert no_inputs == len(sids_to_use_for_replay_files)

    for sid in sids_to_use_for_replay_files:
        initial_commands.append(f'msg add_source ssid={sid} codec=opus')

    initial_commands.append(
            f'msg add_destination host=127.0.0.1 port={record_port} codec=opus')

    signalling_server, _ = start_signalling_server(port=ss_port,
            initial_commands= initial_commands)

    signalling_server.request_factory.add_requests(openvocs_requests.requests())

    signalling_server.request_factory.add_requests(
            openvocs_requests.requests('mixer'))


    signalling_server.event_handler('add_destination',
            make_add_destination_handler(
                record_port,
                mixer_media_port,
                zip(input_file_names, sids_to_use_for_replay_files),
                output_file_name))

    signalling_server.serve_forever(poll_interval=0.1)

    while running_processes:

        for p in remove_stopped(running_processes):
            print(f"{process_name(p)} has stopped")

        time.sleep(1)

# ------------------------------------------------------------------------------
