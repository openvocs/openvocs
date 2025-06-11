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

OV_EXECUTABLE        = $(OV_BINDIR)/$(OV_DIRNAME)
OV_TARGET            = $(OV_EXECUTABLE)

TMP_OUT              = /tmp/opus_channels.out

#-----------------------------------------------------------------------------

OV_FLAGS       = `pkg-config --cflags opus`

#-----------------------------------------------------------------------------

OV_LIBS        = -L$(OV_LIBDIR)
# OV_LIBS       += -pthread
OV_LIBS       += `pkg-config --libs opus`
OV_LIBS       += -l ov_log2
OV_LIBS       += -l ov_base2
# OV_LIBS       += -l ov_os2
OV_LIBS       += -l ov_pcm16s2
# OV_LIBS       += -l ov_codec2
# OV_LIBS       += -l ov_backend2

#-----------------------------------------------------------------------------
include $(OPENVOCS_ROOT)/makefile_targets.mk

PLOTDIR = graphics

PLOTS = 1_in_1_out.jpeg 1_in_2_out.jpeg 2_in_1_out.jpeg  2_in_2_out.jpeg interleaved_input.jpeg

.phony: all
all:  $(addprefix $(PLOTDIR)/, $(PLOTS))

.phony: clean_all
clean_all: clean
	$(OV_QUIET)$(RM) -r $(PLOTDIR)

$(PLOTDIR):
	$(OV_QUIET)$(OV_MKDIR) $(PLOTDIR)

$(PLOTDIR)/1_in_1_out.jpeg: $(OV_EXECUTABLE) $(PLOTDIR)
	$(OV_QUIET)echo "[GENERATING $@]"
	$(OV_QUIET)$(OV_EXECUTABLE) 1 1 > $(TMP_OUT) 2> /dev/null
	$(OV_QUIET)echo " set t jpeg size 1024,768; set o '$@'; plot '$(TMP_OUT)' u :1, '$(TMP_OUT)' u :2" | gnuplot
	$(OV_QUIET)$(RM) $(TMP_OUT)

$(PLOTDIR)/2_in_1_out.jpeg: $(OV_EXECUTABLE) $(PLOTDIR)
	$(OV_QUIET)echo "[GENERATING $@]"
	$(OV_QUIET)$(OV_EXECUTABLE) 2 1 > $(TMP_OUT)
	$(OV_QUIET)echo " set t jpeg size 1024,768; set o '$@'; plot '$(TMP_OUT)' u :1, '$(TMP_OUT)' u :2" | gnuplot
	$(OV_QUIET)$(RM) $(TMP_OUT)

$(PLOTDIR)/1_in_2_out.jpeg: $(OV_EXECUTABLE) $(PLOTDIR)
	$(OV_QUIET)echo "[GENERATING $@]"
	$(OV_QUIET)$(OV_EXECUTABLE) 1 2 > $(TMP_OUT)
	$(OV_QUIET)echo " set t jpeg size 1024,768; set o '$@'; plot '$(TMP_OUT)' u :1, '$(TMP_OUT)' u :2" | gnuplot
	$(OV_QUIET)$(RM) $(TMP_OUT)

$(PLOTDIR)/2_in_2_out.jpeg: $(OV_EXECUTABLE) $(PLOTDIR)
	$(OV_QUIET)echo "[GENERATING $@]"
	$(OV_QUIET)$(OV_EXECUTABLE) 2 2 > $(TMP_OUT) 2> /dev/null
	$(OV_QUIET)echo " set t jpeg size 1024,768; set o '$@'; plot '$(TMP_OUT)' u :1, '$(TMP_OUT)' u :2" | gnuplot
	$(OV_QUIET)$(RM) $(TMP_OUT)

$(PLOTDIR)/interleaved_input.jpeg: $(OV_EXECUTABLE) $(PLOTDIR)
	$(OV_QUIET)echo "[GENERATING $@]"
	$(OV_QUIET)$(OV_EXECUTABLE) 1 1 > $(TMP_OUT)
	$(OV_QUIET)echo " set t jpeg size 1024,768; set o '$@'; plot '$(TMP_OUT)' u :1" | gnuplot
	$(OV_QUIET)$(RM) $(TMP_OUT)
