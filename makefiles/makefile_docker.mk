# -*- Makefile -*-
#       ------------------------------------------------------------------------
#
#       Copyright 2021 German Aerospace Center DLR e.V. (GSOC)
#
#       Licensed under the Apache License, Version 2.0 (the "License");
#       you may not use this file except in compliance with the License.
#       You may obtain a copy of the License at
#
#               http://www.apache.org/licenses/LICENSE-2.0
#
#       Unless required by applicable law or agreed to in writing, software
#       distributed under the License is distributed on an "AS IS" BASIS,
#       WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#       See the License for the specific language governing permissions and
#       limitations under the License.
#
#       This file is part of the openvocs project. http://openvocs.org
#
#       ------------------------------------------------------------------------
#
#       Authors         Michael J. Beer
#
#       ------------------------------------------------------------------------
#
#       Some Flags that can be overwritten to control behaviour of makefile:
#
#       OV_QUIET    If set, commands to be executed are hidden
#       OV_FLAGS    contains compiler flags
#       OV_LIBS     contains libraries to link against (BE AWARE OF ORDER)
#       OV_TARGET   contains desired target (default: shared and static lib)
#                   may be overriden to executable
#
#       for example have a look @see src/lib/ov_base/makefile
#
#       ------------------------------------------------------------------------

# netrc template to use within devel container to log into git server
# If file is not there, an empty netrc file is used within the container
# Be sure to set this file in the gitignore file!
#
# The content should look like
#
# machine gitlab.dlr.de
#     login login_name
#     password login_password
#
OV_GIT_CREDENTIALS_FILE=$(HOME)/git_credentials

.PHONY: docker

docker:
	$(OV_QUIET) echo "Use 'make docker_all' to build all docker images"
	$(OV_QUIET) echo "Use 'make docker_backend' to build docker release images for backend services"
	$(OV_QUIET) echo "Use 'make docker_devel' to build docker devel image for building the openvocs"
	$(OV_QUIET) echo "Create a '$HOME/git_credentials' file if you wish to use the devel container and have git log in"

.PHONY: docker_all

# Build all docker images
docker_all: docker_backend docker_devel

.PHONY: docker_clean

docker_clean:
	$(OV_QUIET)$(OV_RM) Dockerfile
	$(OV_QUIET)$(OV_RM) netrc


.PHONY: docker_backend

docker_backend: build
	@echo "Creating docker images for backend"
	$(OV_QUIET)python3 scripts/dockerize.py --stype resmgr
	$(OV_QUIET)python3 scripts/dockerize.py --stype mixer
	$(OV_QUIET)python3 scripts/dockerize.py --stype echo_cancellator
	$(OV_QUIET)python3 scripts/dockerize.py --stype recorder

.PHONY:docker_devel

docker_devel: $(OV_BUILDDIR)/docker_devel

# Marker file to indicate that the docker devel container was built
$(OV_BUILDDIR)/docker_devel:  Dockerfile $(OV_BUILDDIR)/docker_build.sh netrc
	$(OV_QUIET) python3 scripts/dockerize.py --devel
	$(OV_QUIET) touch $@

Dockerfile: resources/docker/devel.dockerfile
	@echo "Creating $@"
	$(OV_QUIET)python3 scripts/dockerfile.py --target_dir . --from $^

$(OV_BUILDDIR)/docker_build.sh: resources/docker/build.sh $(OV_BUILDDIR)
	@echo "Creating $@"
	$(OV_QUIET) python3 scripts/template.py --target $@ --from resources/docker/build.sh

netrc:
	$(OV_QUIET)[ -e $@ ] || [ -e  $(OV_GIT_CREDENTIALS_FILE) ] && cp $(OV_GIT_CREDENTIALS_FILE) $@; \
	$(OV_QUIET)[ -e $@ ] || touch $@ # Default:create empty

.PHONY: docker_build

docker_build: $(OV_BUILDDIR)/docker_devel
	$(OV_QUIET) python3 scripts/docker_build.py
