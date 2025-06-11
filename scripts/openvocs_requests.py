#!/usr/local/bin/python3.7
# -----------------------------------------------------------------------------
#
#  Copyright   2019    German Aerospace Center DLR e.V.,
#  German Space Operations Center (GSOC)
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
#
#  This file is part of the openvocs project. http://openvocs.org
#
# -----------------------------------------------------------------------------

"""
Defines all requests of the openvocs protocol.
For the format of an openvocs message that transmits such a request, see the
openvocs .

If called via command line, refer to documentation of main() .


@author Michael J. Beer, DLR/GSOC
@copyright 2019 German Aerospace Center DLR e.V., (GSOC)
@date 2019-01-03

@license Apache License
@version 1.0.0

@status Production

"""

# -----------------------------------------------------------------------------

import sys

# -----------------------------------------------------------------------------

from rich.python3.openvocs import RequestFactory
from rich.python3.openvocs_requests import service_types, requests

# -----------------------------------------------------------------------------


def usage():

    sys.stderr.write("""
     USAGE:

        python3 openvocs_requests.py [SERVICE_TYPE|-h|--help]

    If no parameters are given, prints this help, along with all
    possible service types.
    """)

# -----------------------------------------------------------------------------


def main():

    """
    USAGE:

        python3 openvocs_requests.py [SERVICE_TYPE|-h|--help]


    See usage() for further information
    """

    service_type = None

    argv = sys.argv

    if len(argv) > 1:
        service_type = argv.pop()

    if not service_type:
        usage()
        sys.stderr.write("\nPossible service types:\n")
        sys.stderr.write("\n")
        sys.stderr.write("    \n".join(service_types()))
        sys.exit(1)

    if service_type == '--help' or service_type == '-h':
        usage()
        sys.exit(1)

    service_requests = requests(service_type)

    if not service_requests:
        usage()
        sys.exit(1)

    request_descriptions = []

    rf = RequestFactory()
    rf.add_requests(service_requests)
    rnames = rf.get_known_requests()
    msgs = [rf.get_message_from_string("msg " + rname) for rname in rnames]
    request_descriptions = [
            f"{rname}\t{msg}" for rname, msg in zip(rnames, msgs)]

    print("\n\n\n".join(request_descriptions))

# -----------------------------------------------------------------------------


if __name__ == "__main__":
    main()
