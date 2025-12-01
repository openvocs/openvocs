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

#-----------------------------------------------------------------------------
# Local source file overrides

L_TEST_SOURCES =
L_HEADERS      =
L_SOURCES_C    =
L_SOURCES      =

#-----------------------------------------------------------------------------

include $(OPENVOCS_ROOT)/makefiles/makefile_const.mk

#-----------------------------------------------------------------------------

# We need to include the cflags for openssl on macos using pkg-config
# based library adding, as the include path is non within the standard include
# path
#
# TBD can we add the CFLAGS of the encryption makefile
# ... so to say somehow include dependend cflags? in a chain?
#
# NOT IF YOU WANNA STAY SANE!!!
# OV_FLAGS       += `pkg-config --cflags openssl`

#-----------------------------------------------------------------------------

OV_LIBS        = -L$(OV_LIBDIR)

OV_LIBS       += -l ov_log$(OV_EDITION)
OV_LIBS       += -l ov_test$(OV_EDITION)
OV_LIBS       += -l ov_arch$(OV_EDITION)
OV_LIBS       += -l ov_base$(OV_EDITION)

OV_LIBS       += -l m
OV_LIBS       += -pthread

#-----------------------------------------------------------------------------

include $(OPENVOCS_ROOT)/makefiles/makefile_targets.mk
