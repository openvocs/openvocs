# -*- Makefile -*-
#       ------------------------------------------------------------------------
#
#       Copyright 2020 German Aerospace Center DLR e.V. (GSOC)
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
#       Authors         Udo Haering, Michael J. Beer, Markus Töpfer
#       Date            2020-01-21
#
#       ------------------------------------------------------------------------

include $(OPENVOCS_ROOT)/makefiles/makefile_version.mk

# generic source files
G_HEADERS            := $(wildcard include/*.h)
G_SOURCES_C        := $(wildcard src/*.c)
G_TEST_SOURCES     := $(wildcard src/*_test.c)
G_TEST_IF_SRC    := $(wildcard src/*_test_interface.c)
G_SOURCES        := $(filter-out $(G_TEST_SOURCES), $(G_SOURCES_C))

# const variables to use (generic may be overriden using L_* )
OV_HDR            := $(if $(L_HEADERS),$(L_HEADERS),$(G_HEADERS))
OV_SRC            := $(if $(L_SOURCES),$(L_SOURCES),$(G_SOURCES))
OV_TEST_SOURCES    := $(if $(L_TEST_SOURCES),$(L_TEST_SOURCES),$(G_TEST_SOURCES))
OV_TEST_IF_SRC    := $(if $(L_TEST_IF_SRC),$(L_TEST_IF_SRC),$(G_TEST_IF_SRC))

# shell commands
OV_MKDIR           := mkdir -p
OV_RM              := rm -f
OV_RMDIR           := rm -rf
OV_SYMLINK         := ln -s
OV_COPY            := cp -r

# redirection (suppressing printouts from command line tools))
OV_NUL             := /dev/null
OV_NUL_STDERR      := 2>$(OV_NUL) || true
OV_NUL_STDOUT      := 1>$(OV_NUL) || true
OV_DEV_NULL        := $(OV_NUL_STDOUT) $(OV_NUL_STDERR)

# make commands quiet
OV_QUIET           ?= @
#OV_QUIET            =

# prevent make from displaying 'Entering/Leaving directory ...'
OV_QUIET_MAKE      ?= --no-print-directory

# directories
OV_SOURCEDIR       := $(OPENVOCS_ROOT)/src
OV_BUILDDIR        := $(OPENVOCS_ROOT)/build
OV_BUILDDIR        := $(abspath $(OV_BUILDDIR))
OV_OBJDIR          := $(OV_BUILDDIR)/obj
OV_LIBDIR          := $(OV_BUILDDIR)/lib
OV_BINDIR          := $(OV_BUILDDIR)/bin
OV_TESTDIR         := $(OV_BUILDDIR)/test
OV_PLUGINDIR       := $(OV_BUILDDIR)/plugins
OV_INSTALLDIR      := /usr/local/bin
OV_PLUGINS_INSTALLDIR   := /usr/lib/openvocs/plugins

OV_TEMPDIR         := /tmp

OV_INSTALL_PREFIX  := /usr/local

OV_TEST_CERT        := $(OPENVOCS_ROOT)/resources/certificate/openvocs.test.crt
OV_TEST_CERT_KEY   := $(OPENVOCS_ROOT)/resources/certificate/openvocs.test.key
OV_TEST_CERT_ONE       := $(OPENVOCS_ROOT)/resources/certificate/one.crt
OV_TEST_CERT_ONE_KEY   := $(OPENVOCS_ROOT)/resources/certificate/one.key
OV_TEST_CERT_TWO       := $(OPENVOCS_ROOT)/resources/certificate/two.crt
OV_TEST_CERT_TWO_KEY   := $(OPENVOCS_ROOT)/resources/certificate/two.key


# # dependencies
# # ==> to be changed !!
# OV_DEPENDFILE      := .depend

# include paths
OV_INC             := -Iinclude -I$(OV_BUILDDIR) -I$(OV_BUILDDIR)/include

OV_DIRNAME         := $(notdir $(CURDIR))
OV_LIBNAME         := lib$(OV_DIRNAME)

OV_UNAME             := $(shell uname)

#.............................................................................
# distinguish libs and executables
#
# IMPORTANT:
# this _MUST_ be defined as a recursively expanded variable!!
OV_TARGET           = $(OV_STATIC) $(OV_SHARED)
OV_TEST_TARGET      = $(addprefix $(OV_TESTDIR)/$(OV_DIRNAME)/,$(patsubst %.c,%.run, $(notdir $(OV_TEST_SOURCES))))
#.............................................................................
# test sources directory
OV_TEST_RESOURCE_DIR := $(OV_BUILDDIR)/test/$(OV_DIRNAME)/resources/

#.............................................................................

BUILD_DEFINITIONS = -D OPENVOCS_ROOT='"$(OPENVOCS_ROOT)"' \
                    -D OV_VERSION='"$(OV_VERSION)"' \
                    -D OV_VERSION_BUILD_ID='"$(OV_VERSION_BUILD_ID)"' \
                    -D OV_VERSION_COMMIT_ID='"$(OV_VERSION_COMMIT_ID)"' \
                    -D OV_VERSION_BUILD_DATE='"$(OV_VERSION_BUILD_DATE)"' \
                    -D OV_VERSION_COMPILER='"$(OV_VERSION_COMPILER)"' \
                    -D OV_PLUGINS_INSTALLDIR='"$(OV_PLUGINS_INSTALLDIR)"' \

#.............................................................................

TEST_DEFINITIONS =  -D OV_TEST_RESOURCE_DIR='"$(OV_TEST_RESOURCE_DIR)"' \
                    -D OV_TEST_CERT='"$(OV_TEST_CERT)"' \
                    -D OV_TEST_CERT_KEY='"$(OV_TEST_CERT_KEY)"' \
                    -D OV_TEST_CERT_ONE='"$(OV_TEST_CERT_ONE)"' \
                    -D OV_TEST_CERT_ONE_KEY='"$(OV_TEST_CERT_ONE_KEY)"' \
                    -D OV_TEST_CERT_TWO='"$(OV_TEST_CERT_TWO)"' \
                    -D OV_TEST_CERT_TWO_KEY='"$(OV_TEST_CERT_TWO_KEY)"' \

#.............................................................................

OV_RELEASE_SCRIPTS := scripts/ov_vocs_mc_config.sh
OV_RELEASE_DATA := resources/certificate

OV_RELEASE_RESOURCES_CORE  :=    lib bin env.sh $(OV_RELEASE_SCRIPTS) \
                                $(OV_RELEASE_DATA) \
                                resources/config src/HTML

OV_RELEASE_RESOURCES  :=     $(OV_RELEASE_RESOURCES_CORE) \
                            resources/systemd

OV_RELEASE_PYTHON_RESOURCES  := $(OPENVOCS_PYTHON_VENV_REL) python \
    rich scripts/start_services.py

OV_RELEASE_NAME       := openvocs
OV_TEMP_RELEASE_DIR   := $(OV_TEMPDIR)/$(OV_RELEASE_NAME)
OV_RELEASE_ARCHIVE    := $(OV_BUILDDIR)/$(OV_RELEASE_NAME).tar.gz
OV_TEMP_RELEASE_RESOURCES  := $(addprefix $(OV_TEMP_RELEASE_DIR)/,$(OV_RELEASE_RESOURCES))
OV_RELEASE_CONFIG_DIR := $(OV_TEMP_RELEASE_DIR)/config

#.............................................................................

# Build distribution organised the default linux root tree manner:
# - configs goes to etc/
# - binaries go to usr/lib/
#   ...
# Used for package managers to build their packages.
OV_LOCAL_DIST_DIR=$(OV_BUILDDIR)/localdist
OV_SIP_LOCAL_DIST_DIR=$(OV_BUILDDIR)/localdist_sip
OV_PLUGINS_LOCAL_DIST_DIR=$(OV_BUILDDIR)/localdist_plugins

OV_LIBS_TO_PACKAGE=arch backend base codec core database encryption format ice ldap \
            log os os_linux pcm16s pcm_gen sip snmp stun test value vm vocs vocs_db \
            ice ice_proxy vad

OV_BINARIES_TO_PACKAGE=stun_server \
    alsa_gateway mc_ice_proxy mc_mixer mc_vocs mc_interconnect \
    mc_socket_debug alsa_cli rtp_cli test_mc mc_vad mc_cli\
    domain_config_verify \
    ldap_test ldap_test_auth ldap_user_import password \
    stun_client \

OV_SIP_BINARIES_TO_PACKAGE=sip_gateway

#.............................................................................

# CLANG specifics
CFLAGS             += -fstrict-aliasing -Wno-trigraphs -O0

#.............................................................................
#
# these _must_ not be prefixed ...
# -Wno-missing-braces is disabled with GCC by default, but clang still uses it
#

CFLAGS             += -Werror -Wall -Wextra -fPIC $(OV_INC)
CFLAGS             += -std=c11 -D _DEFAULT_SOURCE -D _POSIX_C_SOURCE=200809
CFLAGS             += -D _XOPEN_SOURCE=500
CFLAGS             += -DDEBUG -g
# To make clang accept C11 '{0}' initialisation ...
CFLAGS             +=  -Wno-missing-braces

ifeq ($(OV_UNAME), Darwin)
    CFLAGS         +=  -D _DARWIN_C_SOURCE=__DARWIN_C_FULL
endif

# when clang is used write some compile_command.json per file
ifeq ($(CC), clang)
    CFLAGS         +=  -MJ $@.json
endif

# Debug flags - switch on on command line like
# DEBUG=1 make test -j 4

ifdef DEBUG

CFLAGS             += -DDEBUG
CFLAGS             += -g

endif

LFLAGS              = -rdynamic

#.............................................................................

OV_PLUGINS_DIR      = src/plugins
OV_PLUGINS_PRESENT  = $(shell if [ -d $(OV_PLUGINS_DIR) ]; then echo 1; else echo 0; fi)

#.............................................................................

OV_JAVASCRIPT_VERSION_FILE=$(OPENVOCS_ROOT)/src/HTML/VERSION.js

# PYTHON configuration

OV_PYTHON_LINTER_LOG_FILE=$(OV_BUILDDIR)/python_format_errors.txt
