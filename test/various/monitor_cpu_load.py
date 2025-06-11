#!/usr/local/bin/python3.7
#------------------------------------------------------------------------------
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
#------------------------------------------------------------------------------

"""
Comprehensive description of this python script

@author Michael J. Beer, DLR/GSOC
@copyright (c) 2022 German Aerospace Center DLR e.V. (GSOC)
@date 2022-11-08

@license Apache 2.0
@version 1.0.0

@status Development

"""

#------------------------------------------------------------------------------

from sys import argv
from collections import namedtuple
from collections.abc import Iterable, Iterator
from datetime import datetime
import subprocess
import time
import argparse

#-------------------------------------------------------------------------------

CPU_THRESHOLD = "50"
MON_INTERVAL_SECS = "300"
LOG_FILE = "/tmp/monitor_cpu_load.log"

#------------------------------------------------------------------------------

def process_list() -> str:

    return subprocess.run(['ps', "-heocomm,pid,pcpu"],
                          capture_output=True).stdout.decode('ascii')

#-------------------------------------------------------------------------------

ProcessInfo = namedtuple("ProcessInfo", "name pid cpu")

def to_3_tuple(l: list) -> tuple[str, str, str]:

    def get_or(l: list, i: int, default_val: str) -> str:
        try:
            return str(l[i])
        except:
            return default_val

    return (get_or(l, 0, ""), get_or(l, 1, ""), get_or(l, 2, ""))

#-------------------------------------------------------------------------------

def decompose(iter: Iterable[str]) -> Iterator[ProcessInfo]:

    for l in iter:
        yield ProcessInfo(*to_3_tuple(l.strip().split(maxsplit=2)))

#-------------------------------------------------------------------------------

if __name__ == "__main__":

    parser = argparse.ArgumentParser(description=f"""
       Looks for processes using more than CPU_LOAD_THRESHOLD cpu time (in %).
       If none is found, nothing is done.

       If one or more are found, the current time is printed and a list of
       all processes surpassing the CPU_LOAD_THRESHOLD ({CPU_THRESHOLD} by default)
       limit is printed to a log file ({LOG_FILE} by default) in the form of

       20221108 - 13:06:36
           zsh  69030  0.0
           python3  105885  0.0
           zsh  103565  0.1

       Performs this check once every x seconds ({MON_INTERVAL_SECS} by default)
       until the script is killed.
    """)

    parser.add_argument("-t", "--threshold_percent", default=CPU_THRESHOLD)
    parser.add_argument("-s", "--interval_secs", default=MON_INTERVAL_SECS)
    parser.add_argument("-o", "--out", default=LOG_FILE)

    args = vars(parser.parse_args())

    cpu_threshold = float(args.get("threshold_percent", CPU_THRESHOLD))
    mon_interval_secs = int(args.get("interval_secs", MON_INTERVAL_SECS))
    outfile = args.get("out", LOG_FILE)

    with open(outfile, "w+") as out:

        while(True):

            overloaded_processes = set()

            for process in decompose(process_list().splitlines()):
                if cpu_threshold <= float(process.cpu):
                    overloaded_processes.add(process)

            if overloaded_processes:
                out.write(datetime.now().strftime("%Y%m%d - %H:%M:%S"))
                out.write("\n")
                for process in overloaded_processes:
                    out.write("    ")
                    out.write(f"{process.name}  {process.pid}  {process.cpu}\n")

            time.sleep(mon_interval_secs)
