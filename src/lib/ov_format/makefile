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

OV_FLAGS       =

##-----------------------------------------------------------------------------

OV_LIBS        = -L$(OV_LIBDIR)

OV_LIBS       += -l ov_test$(OV_EDITION)
OV_LIBS       += -l ov_log$(OV_EDITION)
OV_LIBS       += -l ov_base$(OV_EDITION)
OV_LIBS       += -l ov_arch$(OV_EDITION)
OV_LIBS       += -l ov_encryption$(OV_EDITION)
OV_LIBS       += -l ov_codec$(OV_EDITION)

OV_LIBS       += -lm
OV_LIBS       += -pthread

#-----------------------------------------------------------------------------

include $(OPENVOCS_ROOT)/makefiles/makefile_targets.mk
