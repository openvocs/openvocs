#!/usr/bin/env bash

# if [ "${OPENVOCS_ROOT}" == "" ]; then
#   OPENVOCS_ROOT=$(pwd)
# fi

OPENVOCS_ROOT=$(pwd)
OPENVOCS_LIBS=${OPENVOCS_ROOT}/build/lib
LD_LIBRARY_PATH=$OPENVOCS_LIBS

PYTHONPATH=$OPENVOCS_ROOT

OPENVOCS_PYTHON_VENV_ROOT=.python-venv
OPENVOCS_PYTHON_VENV_REL=$OPENVOCS_PYTHON_VENV_ROOT/openvocs_venv
OPENVOCS_PYTHON_VENV=$OPENVOCS_ROOT/$OPENVOCS_PYTHON_VENV_REL

# Switch on debug build

# DEBUG=1
DEBUG=

# Enable to show all commands triggered in make files in full length
# OV_QUIET=
# export OV_QUIET

# Compiler to use

CC=gcc
# CC=clang

# Switches for tests

OV_VALGRIND=valgrind
OPENVOCS_TEST_SPARSE_OUT=

export OPENVOCS_ROOT
export OPENVOCS_LIBS
export LD_LIBRARY_PATH
export DEBUG
export CC

export OPENVOCS_TEST_SPARSE_OUT
export OV_VALGRIND
export PYTHONPATH
export OPENVOCS_PYTHON_VENV
export OPENVOCS_PYTHON_VENV_REL
export OPENVOCS_PYTHON_VENV_ROOT

# clang requires the sdk-path for compilation
if [[ "$OSTYPE" == "darwin"* ]]; then
    export SDKROOT="`xcrun --show-sdk-path`"
fi

# And change the virtual python environment
[ -e "$OPENVOCS_PYTHON_VENV/bin/activate" ] && . "$OPENVOCS_PYTHON_VENV/bin/activate"
