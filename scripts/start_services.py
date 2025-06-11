#!/usr/local/bin/python3.7
#  -----------------------------------------------------------------------
#
#  Copyright (c) 2019 German Aerospace Center DLR e.V. (GSOC)
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#          http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
#
#  This file is part of the openvocs project. https://openvocs.org
#
#  -----------------------------------------------------------------------

"""
Start openvocs processes

@author Michael J. Beer, DLR/GSOC
@copyright 2020 German Aerospace Center DLR e.V. (GSOC)
@date 2020-03-27

@license Apache License 2.0
@version 1.0.0

@status Development

"""

# -----------------------------------------------------------------------------

import os
import sys
import signal
from argparse import ArgumentParser
import json

import time

from subprocess import Popen, DEVNULL

from logging import error, info

from typing import Optional, List, Tuple

# -----------------------------------------------------------------------------

import python.openvocs as ov
from python.misc.ensure import abort_if_not, ensure_dir_exists

# -----------------------------------------------------------------------------

import python.service.configuration as configuration

# -----------------------------------------------------------------------------

GEN_DIR_PREFIX = "/tmp/oger"
DUMP_FILE = f"{GEN_DIR_PREFIX}/dump.pcap"

SIGNALLING_SERVER = {

        "host": "127.0.0.1",
        "port": 10001,

        }

ZERO_CROSSINGS_RATE_HZ = 9000
POWERLEVEL_DENSITY_DBFS = -50
NORMALIZE_INPUT = False
MAX_INPUT_FRAMES = 0

# This resembles just the part of the config that changes - the
# config template is located under rich.python3.service_configuration
SERVICE_CONFIGS = {}


def SERVICE_CONFIGS_TEMPLATE():

    return {

        "mixer": {

            "mixer": {

                "ss": SIGNALLING_SERVER,

                "lock_timeout_msecs": 2000,

                "log": {

                    "systemd": False,
                    "level": "debug",
                    # "file": "/tmp/ov_mixer.log"

                }

            }

        },

        "multiplexer": {

            "multiplexer": {

                "ss": SIGNALLING_SERVER,

                "lock_timeout_msecs": 2000,

                "log": {

                    "systemd": False,
                    "level": "debug",
                    # "file": "/tmp/ov_echo_cancellator.log"

                    },

                },

            },

        "resource_manager": {

            "resource_manager": {

                "minions": SIGNALLING_SERVER,

                "liege": {

                    "host": SIGNALLING_SERVER['host'],
                    "port": SIGNALLING_SERVER["port"] + 1,

                    },

                "monitor": {

                    "host": SIGNALLING_SERVER['host'],
                    "port": SIGNALLING_SERVER["port"] + 2,

                    },

                "defaults": {
                        "vad": {
                            "zero_crossings_rate_hertz": ZERO_CROSSINGS_RATE_HZ,
                            "powerlevel_density_dbfs": POWERLEVEL_DENSITY_DBFS,
                        },

                        "normalize_input" : NORMALIZE_INPUT,
                        "max_num_frames" : MAX_INPUT_FRAMES,

                    },

                "log": {

                    "systemd": False,
                    "level": "debug",
                    "file": {
                        "file" : "/tmp/resmgr.log",
                        "num_files" : 5,
                        "messages_per_file" : 10000
                    },
                },

                "lock_timeout_msecs": 2000,

                },


            },

        }

# -----------------------------------------------------------------------------


def stop_processes(processes):

    for p in processes:
        p.terminate()

# -----------------------------------------------------------------------------


def remove_stopped(processes: List[Popen]) -> List[Popen]:

    stopped = list(filter(lambda p: p.poll() is not None, processes))

    for p in stopped:
        processes.remove(p)

    return stopped

# -----------------------------------------------------------------------------


def stop_and_exit(processes: List[Popen]) -> None:
    stop_processes(processes)
    sys.exit(0)

# -----------------------------------------------------------------------------


def process_name(p: Popen):

    if isinstance(p.args, list):
        return p.args[0]

    return args

# -----------------------------------------------------------------------------


def get_resmgr_socket(service_configs: dict) -> Tuple[str, int]:

    try:

        resmgr_config = service_configs['resource_manager']
        resmgr_config = resmgr_config['resource_manager']

        liege_config = resmgr_config['liege']

        host = liege_config['host']
        port = liege_config['port']

    except Exception:

        print("Resource manager config not found")
        sys.exit(1)

    return (host, int(port))

# -----------------------------------------------------------------------------


def set_log_file(config: dict, log_file: str) -> None:

    logging = config['log']
    logging['file'] = log_file

# ------------------------------------------------------------------------------


def setup_netdump(args: dict) -> Optional[Popen]:

    if not args['netdump']:
        return None

    cmdline = ["sudo", "/usr/bin/tcpdump",
               "-vvv", "-w", DUMP_FILE, "host", args['listen_on']]

    print(f'about to start {cmdline}')
    return Popen(cmdline)

# -----------------------------------------------------------------------------


def start_service_type(
        service_type: str,
        bin_prefix: str,
        config_prefix: str,
        number_of_instances: int = 1,
        log_dir: Optional[str] = None) -> List[Popen]:

    def generate_config_file(config_file: str, num: int) -> None:

        info(f"Writing config file {config_file}")

        if service_type not in SERVICE_CONFIGS:
            error("Unknown service name: {}\n".format(service_type))

        config = SERVICE_CONFIGS[service_type]

        if log_dir:
            log_file = f"{log_dir}/ov_{service_type}_{num}.log"
            set_log_file(config[service_type], log_file)
            print(f"{service_type} logging to {log_file}")

        config = configuration.get_config(service_type, config=config)

        config = json.dumps(config, indent=4)

        with open(config_file, mode='w') as target:
            target.write(str(config))
            target.write("\n")

    def start_service(config_file: str) -> Popen:

        cmdline = [f'{bin_prefix}/ov_{service_type}', "-c", config_file]
        print('about to start {}'.format(cmdline[0]))
        process = Popen(cmdline, stdout=DEVNULL, stderr=DEVNULL)
        print()
        return process

    running = []

    try:

        for num in range(number_of_instances):
            config_file = (f"{config_prefix}/"
                           f"ov_{service_type}_{num}_config.json")
            generate_config_file(config_file, num)
            running.append(start_service(config_file))

    except Exception:

        stop_processes(running)
        raise

    return running

# ------------------------------------------------------------------------------


def start_services(
        start_resmgr: bool,
        num_mixers: int,
        num_muxes: int,
        bin_prefix: str,
        config_prefix: str,
        log_dir: Optional[str] = None
        ) -> List[Popen]:

    """
    Start all requested services.
    Return a list of Process objects representing the running services
    """

    running_services = []

    # resource manager should start first in order to allow other processes
    # to connect

    try:

        if start_resmgr:
            running_services += start_service_type(
                    'resource_manager',
                    bin_prefix=bin_prefix,
                    config_prefix=config_prefix,
                    log_dir=log_dir)

    except Exception as ex:

        error(f"Failed to start resource manager: {str(ex)}")
        stop_and_exit(running_services)

    try:

        if mixers:
            running_services += start_service_type(
                    'mixer',
                    bin_prefix=bin_prefix,
                    config_prefix=config_prefix,
                    number_of_instances=num_mixers,
                    log_dir=log_dir)

    except Exception as ex:

        error(f"Failed to start  mixers: {str(ex)}")
        stop_and_exit(running_services)

    try:

        if num_muxes:
            running_services += start_service_type(
                    'multiplexer',
                    bin_prefix=bin_prefix,
                    config_prefix=config_prefix,
                    number_of_instances=num_muxes,
                    log_dir=log_dir)

    except Exception as ex:

        error(f"Failed to start echo cancellators: {str(ex)}")
        stop_and_exit(running_services)

    return running_services

# -----------------------------------------------------------------------------


def get_parsed_script_arguments() -> dict:

    port = SIGNALLING_SERVER['port']
    listen_on = os.environ.get('OV_IP', SIGNALLING_SERVER['host'])

    parser = ArgumentParser(description=f"""
    Start test system.

    Will start by default:

    1 resource manager (announce running resmgr and prevent "
                        starting a resmgr by '--resmgr HOST')
    0 echo cancellators (have ecs started by '--ecs')
    0 mixers (have mixers started by '--mixers')

    The signalling servers' minions port MUST be {port},
    The signalling server liege port MUST be {port + 1}.

    """)

    parser.add_argument('--listen_on', metavar='listen_on', type=str,
                        default=listen_on,
                        help="interface for the services to listen on")
    parser.add_argument('--mixers', metavar='mixers', type=int,
                        default=0, help="Number of mixers to start")
    parser.add_argument('--muxes', metavar='multiplexers', type=int,
                        default=0, help="Number of multiplexers to start")
    parser.add_argument('--resmgr', metavar='res_mgr',
                        type=str, default=None,
                        help=("Host of resource manager to connect to - "
                              "if omitted, a new resource manager is started "
                              "locally. "
                              f"It will open a monitor port at {port + 2}"))

    parser.add_argument('--log_dir',
                        type=str,
                        default=None,
                        help="Log dir to write log files to."
                             "If not set, no logs will be written")

    parser.add_argument('--netdump',
                        action="store_true",
                        default=False,
                        help=f"Dump network traffic to {DUMP_FILE}"
                             " - requires sudo")

    parser.add_argument('--noise_db',
            type=int,
            default=0,
            help=f"Configure for comfort noise")

    parser.add_argument('--normalize_input',
            type=bool,
            default=NORMALIZE_INPUT,
            help=(f"Turn on normalization input from user clients"
                  f" defaults to {NORMALIZE_INPUT}"
            ))

    parser.add_argument('--max_input_frames',
            type=int,
            default=MAX_INPUT_FRAMES,
            help=(f"Restrict number of frames to mix. "
                f"defaults to {MAX_INPUT_FRAMES}"
            ))

    parser.add_argument('--vad_zeroc_hz',
            type=int,
            default=ZERO_CROSSINGS_RATE_HZ,
            help=f"VAD Zero Crossings Rate in Hz - default is {ZERO_CROSSINGS_RATE_HZ}")

    parser.add_argument('--vad_powerlevel_db',
            type=int,
            default=POWERLEVEL_DENSITY_DBFS,
            help=f"VAD Powerlevel dB, default is {POWERLEVEL_DENSITY_DBFS}")

    return vars(parser.parse_args())

# ------------------------------------------------------------------------------


if __name__ == "__main__":

    openvocs_root = ov.get_root()
    abort_if_not(openvocs_root is not None, "OPENVOCS_ROOT is not defined")

    ensure_dir_exists(GEN_DIR_PREFIX)

    args = get_parsed_script_arguments()

    mixers = args['mixers']
    muxes = args['muxes']
    resmgr = args['resmgr']
    listen_on = args['listen_on']
    log_dir = args['log_dir']

    SIGNALLING_SERVER['host'] = resmgr
    start_resmgr = False

    if not resmgr:
        SIGNALLING_SERVER['host'] = listen_on
        start_resmgr = True

    # noise_db : int
    NORMALIZE_INPUT = args['normalize_input']
    ZERO_CROSSINGS_RATE_HZ = args['vad_zeroc_hz']
    POWERLEVEL_DENSITY_DBFS = args['vad_powerlevel_db']

    # Now effectively replace all occurences of 'SIGNALLING_SERVER['host'] etc.
    SERVICE_CONFIGS = SERVICE_CONFIGS_TEMPLATE()

    setup_netdump(args)

    running_services = start_services(
            start_resmgr,
            mixers,
            muxes,
            bin_prefix=f'{openvocs_root}/build/bin',
            config_prefix=GEN_DIR_PREFIX,
            log_dir=log_dir)

    (host, port) = get_resmgr_socket(SERVICE_CONFIGS)

    print("resource manager listening on: {}:{}".format(host, port))

    signal.signal(signal.SIGINT,
            lambda sig, frame: stop_and_exit(running_services))

    while running_services:

        for p in remove_stopped(running_services):
            print(f"{process_name(p)} has stopped")

        time.sleep(1)
