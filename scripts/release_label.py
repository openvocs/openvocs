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
Increases release label in repo and creates release info file build/release

@author Michael J. Beer, DLR/GSOC
@copyright (c) 2022 German Aerospace Center DLR e.V. (GSOC)
@date 2022-12-06

@license Apache 2.0
@version 1.0.0

@status Development

"""

# ------------------------------------------------------------------------------

from sys import version_info as python_version
assert (3, 7, 0) <= python_version

from typing import Optional

import re

from python.misc.utils import git
from python.openvocs import get_build_dir
from datetime import datetime

# -----------------------------------------------------------------------------

def get_current_release_number():

    def get_release_number(matcher: Optional[re.Match]) -> Optional[int]:

        if matcher:
            try:
                return int(matcher.groups()[0])
            except:
                return None
        else:
            return None


    def get_last_release_number():

        release_pattern = re.compile("release/([0-9]*).*$")

        stdout, _ = git('describe', '--tags', '--always', '--match', 'release/*')

        for line in stdout.decode('ascii').split():

            release_number = get_release_number(release_pattern.match(line))

            print(f"Old release number: {release_number}")
            if release_number:
                return release_number

        return 1

    try:

        return 1 + get_last_release_number()

    except:

        return 1

# ------------------------------------------------------------------------------

if __name__ == "__main__":

    current_release = f"release/{get_current_release_number()}"
    git('tag', current_release)

    with open(f"{get_build_dir()}/release", "w+") as release_info:
        release_info.write(f"{current_release}\n")
        release_info.write(f"{datetime.now()}\n")

