# Project ov_recorder

This module is self supported and may be build, tested, installed and
run independently.

## Overview

* [Description](#description)
* [Usage](#usage)
* [Installation](#installation)
* [Requirements](#requirements)
* [Structure](#structure)
* [Tests](#tests)
* [Tips](#tips)
* [Copyright](#copyright)

## Description

This is a recorder:

- It receives RTP streams and writes them as files into the file system
- It reads files from the file system and streams it to arbitrary sockets as RTP

## Usage

...

## Installation

...

### build sources

\`\`\`bash
make
\`\`\`

### build documentation

\`\`\`bash
make documentation
\`\`\`

### test sources

\`\`\`bash
make tested
\`\`\`

### install binaries

\`\`\`bash
sudo make install
\`\`\`

### uninstall binaries

\`\`\`bash
sudo make uninstall
\`\`\`

## Requirements

## Structure

### Default structure of the folder:

\`\`\`
<pre>
.
├── README.MD
├── .gitignore
├── makefile
├── makefile_common.mk
│
├── copyright
│   └── ... 
│
├── doxygen
│   ├── documentation
│   └── doxygen.config
│
├── docs
│   ├── CHANGELOG.MD
│   └── ...
│
├── include
│   ├── ov_example_app_echo.h
│   └── ...
│
├── src
│   ├── ov_example_app_echo.c
│   └── ...
│
└── tests
    ├── resources
    ├── tools
    │   ├── testrun.h
    │   ├── testrun_runner.sh
    │   ├── testrun_gcov.sh
    │   ├── testrun_gprof.sh
    │   ├── testrun_simple_coverage_tests.sh
    │   ├── testrun_simple_unit_tests.sh
    │   ├── testrun_simple_acceptance_tests.sh
    │   └── testrun_simple_loc.sh
    │
    ├── acceptance
    │   ├── ...
    │   └── ...
    │
    └── unit
        ├── ov_example_app_echo_test.c
        └── ...

</pre>
\`\`\`

## Tests

All test sources will be recompiled on each make run. That means,
all module tests will be created new on any change in any source file.

### Test a project (all files contained in tests/unit)

Test compile and run
~~~
make tested
~~~

## Tips

## Copyright

    Copyright (c) 2019 German Aerospace Center DLR e.V. (GSOC)

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

            http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.

    This file is part of the openvocs project. https://openvocs.org

