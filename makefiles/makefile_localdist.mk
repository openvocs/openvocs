# -*- Makefile -*-
#       ------------------------------------------------------------------------
#
#       Copyright 2023 German Aerospace Center DLR e.V. (GSOC)
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
#       Create deb archives for ubuntu/debian
#
#       ------------------------------------------------------------------------

-include makefiles/makefile_const.mk

LIB_FILES_0=$(addsuffix $(OV_EDITION).so, $(OV_LIBS_TO_PACKAGE))
LIB_FILES_1=$(addsuffix $(OV_EDITION).so.$(OV_VERSION_MAJOR), $(OV_LIBS_TO_PACKAGE))
LIB_FILES_2=$(addsuffix $(OV_EDITION).so.$(OV_VERSION_MAJOR).$(OV_VERSION_MINOR).$(OV_VERSION_PATCH), $(OV_LIBS_TO_PACKAGE))
LIB_FILES_ABSPATH=$(addprefix $(OPENVOCS_ROOT)/build/lib/libov_, $(LIB_FILES_0) $(LIB_FILES_1) $(LIB_FILES_2))

BIN_FILES_ABSPATH=$(addprefix $(OPENVOCS_ROOT)/build/bin/ov_, $(OV_BINARIES_TO_PACKAGE))

$(OV_LOCAL_DIST_DIR):
	mkdir -p $(OV_LOCAL_DIST_DIR)

.phony: localdist

localdist: build \
           $(OV_LOCAL_DIST_DIR) \
           $(OV_LOCAL_DIST_DIR)/usr/bin \
           $(OV_LOCAL_DIST_DIR)/usr/lib \
           $(OV_LOCAL_DIST_DIR)/etc/systemd/system \
           $(OV_LOCAL_DIST_DIR)/etc/openvocs \
           $(OV_LOCAL_DIST_DIR)/etc/openvocs/ov_mc_vocs \
           $(OV_LOCAL_DIST_DIR)/srv/openvocs

$(OV_LOCAL_DIST_DIR)/usr/bin: $(BIN_FILES_ABSPATH)
	$(OV_QUIET)echo "Creating $@"
	$(OV_QUIET)$(OV_MKDIR) $@
	for f in $?; do $(OV_COPY)  $$f $@; done

$(OV_LOCAL_DIST_DIR)/usr/lib: $(LIB_FILES_ABSPATH)
	$(OV_QUIET)echo "Creating $@"
	$(OV_QUIET)$(OV_MKDIR) $@
	for f in $?; do $(OV_COPY)  $$f $@; done

$(OV_LOCAL_DIST_DIR)/etc/openvocs: $(OPENVOCS_ROOT)/resources/certificate $(OPENVOCS_ROOT)/resources/debian/scripts/ov_config.sh
	$(OV_QUIET)echo "Creating $@"
	$(OV_QUIET)$(OV_MKDIR) $@/ov_mc_ice_proxy
	$(OV_QUIET)$(OV_MKDIR) $@/ov_mc_mixer
	$(OV_QUIET)$(OV_MKDIR) $@/ov_mc_alsa
	$(OV_QUIET)$(OV_MKDIR) $@/ov_mc_vad
	$(OV_QUIET)for item in $?; do $(OV_COPY) $$item $@; done

$(OV_LOCAL_DIST_DIR)/etc/openvocs/ov_mc_vocs: $(OPENVOCS_ROOT)/resources/config/mime $(OPENVOCS_ROOT)/resources/config/auth
	$(OV_QUIET)echo "Creating $@"
	$(OV_QUIET)$(OV_MKDIR) $@/domains
	$(OV_QUIET)$(OV_MKDIR) $@/settings
	$(OV_QUIET)for item in $?; do $(OV_COPY) $$item $@; done

$(OV_LOCAL_DIST_DIR)/etc/systemd/system: $(OPENVOCS_ROOT)/resources/systemd/global
	$(OV_QUIET)echo "Creating $@"
	$(OV_QUIET)$(OV_MKDIR) $@
	$(OV_QUIET)$(OV_COPY)  $</* $@

$(OV_LOCAL_DIST_DIR)/srv/openvocs:  $(OPENVOCS_ROOT)/src/HTML
	$(OV_QUIET)echo "Creating $@"
	$(OV_QUIET)$(OV_MKDIR) $@
	$(OV_QUIET)$(OV_COPY) $< $@
	$(OV_RM) $@/.gitignore
	$(OV_RM) $@/.vscode
	$(OV_RM) $@/HTML/.gitignore
	$(OV_RMDIR) $@/HTML/.vscode
