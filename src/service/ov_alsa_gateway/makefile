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


include $(OPENVOCS_ROOT)/makefiles/makefile_const.mk

#-----------------------------------------------------------------------------

L_TEST_SOURCES       = $(wildcard src/*_test.c)
L_HEADERS            = $(wildcard **/**/*.h **/*.h *.h)
L_SOURCES_C          = $(wildcard **/**/*.c **/*.c *.c)
L_SOURCES            = $(filter-out $(L_TEST_SOURCES), $(L_SOURCES_C))

OV_HDR               = $(L_HEADERS)
OV_SRC               = $(L_SOURCES)
OV_EXECUTABLE        = $(OV_BINDIR)/$(OV_DIRNAME)
OV_TARGET            = $(OV_EXECUTABLE)

#-----------------------------------------------------------------------------

OV_FLAGS       += `pkg-config --cflags openssl`

##-----------------------------------------------------------------------------

OV_STATIC_LIBS   =


OV_LIBS        = $(OV_STATIC_LIBS)

OV_LIBS       += -pthread

OV_LIBS       += -L$(OV_LIBDIR)

OV_LIBS       += -l ov_log2
OV_LIBS       += -l ov_base2
OV_LIBS       += -l ov_backend2
OV_LIBS       += -l ov_os2
OV_LIBS       += -l ov_os_linux2
OV_LIBS       += -l ov_arch2
OV_LIBS       += -l ov_core2
OV_LIBS       += -l ov_codec2
OV_LIBS       += -l ov_test2
OV_LIBS       += -l ov_pcm_gen2
OV_LIBS       += -l ov_pcm16s2
OV_LIBS       += -l ov_vocs2

OV_LIBS       += -l m

#-----------------------------------------------------------------------------

include $(OPENVOCS_ROOT)/makefiles/makefile_targets.mk

