# Project openvocs

Project openvocs contains all tools to build openvocs service backends.

## Quickstart

This README is Quick introduction How to start using the environment.

## Description

### General overview

Within the development some types of implementations are done under src
1. **lib**          core libraries
2. **service**      services using the libraries
3. **tools**        supporting development tools
4. **HTML**         html clients used for openvocs
5. **samples**      some samples to learn coding with openvocs

#### lib
The libraries provides the core functions to build services for an openvocs system implementation. Modules required to build an openvocs system are for example the openvocs_event_protocol, a websocket_protocol for transmission of the openvocs_event_protocol, a json_protocol for packaging of the openvocs_event_protocol, a server to wait for protocol messages and so on.
**lib** contains all abtract interfaces for openvocs services and the definition and implementation of actual required protocols and functionalities.

#### service
A service on top of the library may be a complete openvocs instance, as client backend, or a part of a system backend. e.g. an authentication service, a media switching server, a signaling service and so on. An implemented service may provide the whole capability needed for a voice communication backend, or only a part of the functionality needed.

Each service have an independent documentation describing the service.

#### tools
Tools are executable helper programs which may enhance the working experience of the specific intendent use case of the tool.

## Installation

Building openvocs is straight forward and requires 3 steps:

1) install required dependencies
2) source env.h to setup the build environment
3) run make

### Linux

#### Manual setup

To list required packages run

```
source env.sh
scripts/show_packages.sh
```

This will list supported distributions.

Run it again with the name of one of those distributions

```
source env.sh
scripts/show_packages.sh debian
```
If your distribution is not included you may check for similar packages on your own.

To install all required and proposed packages you may run
```
for item in `./scripts/show_packages.sh debian debian`; do sudo apt install -y $item; done
```

You will have to take into consideration that openvocs creates abundantly lot of logging, which might caues problems if you don't restrict the size of the logs retained.
You will have to ensure you got access to the systemd journal if you want to inspect the logs.

#### Build from source

1. Set environment variables for the build: `source env.sh`
2. Compile: `make`

### MacOS

To build openvocs on macos a few tools should be installed

#### Build a package for Debian:

```
make deb
```
Once the build is completed the package will be avaialable under the ./build folder.
THe Debian based packages a file like openvocs_1.2.2-1505_amd64.deb will be build.
You may install the package using dpkg

```
dpkg -i build/openvocs_<version>-<build_id>_amd64.deb
```


#### Testing

To run some tests under MacOS the dynamic library path MUST be hand over to
the testrun e.g.

```
DYLD_LIBRARY_PATH=$OPENVOCS_LIBS ./build/test/ov_base/ov_config_test
```

To run all tests of some module, you may used
```
./testrunnner.sh ov_base
```

## Contributors
Here you find the main contributors to the material:

- Markus Töpfer
- Michael Beer
- Anja Bertard
- Tobias Kolb
- Falk Schiffner

## Contact
http://openvocs.org


## License
Please see the file LICENSE.md for further information about how the content is licensed.

### DLR logo
The DLR logo (src/HTML/images/dlr-logo.svg and src/HTML/images/dlr-logo_white.svg) is the property of the German Aerospace Center (DLR) e. V., protected by copyright and registered trademark.
It is not included under this project's open-source license and may not be used, reproduced, or modified without prior written permission from DLR.
Any external usage, including forks or publicly accessible versions, requires separate authorization. In the absence of permission, the logo must be removed; responsibility lies with the respective users. 
