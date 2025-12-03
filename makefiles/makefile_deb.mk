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

ifeq (, $(shell which dpkg))

.PHONY: deb
deb:
	$(OV_QUIET)echo "Your platform does not allow building debs ('dpkg' not found)"

else

OV_WHICH_DISTRO=$(shell lsb_release -si)

OV_DEB_ARCH=$(shell dpkg --print-architecture)

DEB_PACKAGE_BASE_NAME=openvocs_$(OV_VERSION)-$(OV_VERSION_BUILD_ID)_$(OV_DEB_ARCH)
DEB_BUILD_DIR=$(OV_BUILDDIR)/deb
DEB_PACKAGE_DIR=$(DEB_BUILD_DIR)/$(DEB_PACKAGE_BASE_NAME)

# Unfortunately, we use deb12 and ubuntu20.04, which provide different versions 
# of certain packages...
OV_DEB_CONTROL_FILE=$(OPENVOCS_ROOT)/resources/debian/control_$(OV_WHICH_DISTRO)

.PHONY: deb
deb: $(OV_BUILDDIR)/$(DEB_PACKAGE_BASE_NAME).deb

$(DEB_PACKAGE_DIR):
	$(OV_MKDIR) $(DEB_PACKAGE_DIR)

.PHONY: deb_dist_tree
deb_dist_tree: localdist $(DEB_PACKAGE_DIR)
	$(OV_QUIET)echo "Populating $(DEB_PACKAGE_DIR)"
	$(OV_QUIET)$(OV_COPY) $(OV_LOCAL_DIST_DIR)/*  $(DEB_PACKAGE_DIR)

$(DEB_BUILD_DIR)/$(DEB_PACKAGE_BASE_NAME).deb: deb_dist_tree \
                          $(DEB_PACKAGE_DIR)/DEBIAN
	$(OV_QUIET)echo "Building $@"
	$(OV_QUIET)$(OV_COPY) $(OV_LOCAL_DIST_DIR)/* $(DEB_PACKAGE_DIR)
	$(OV_QUIET)cd $(DEB_BUILD_DIR) && dpkg-deb --build --root-owner-group $(DEB_PACKAGE_BASE_NAME)

$(OV_BUILDDIR)/$(DEB_PACKAGE_BASE_NAME).deb: $(DEB_BUILD_DIR)/$(DEB_PACKAGE_BASE_NAME).deb
	$(OV_QUIET)mv $< $@

# control must be built every time since it incorporates VARIABLES that might
# change - make would not realize a change in the variables though
.PHONY: $(DEB_BUILD_DIR)/control
$(DEB_BUILD_DIR)/control: $(OV_DEB_CONTROL_FILE)
	$(OV_QUIET)cat $< | bash $(OPENVOCS_ROOT)/scripts/fill_placeholders.sh $(OV_VERSION) $(OV_DEB_ARCH) $(OV_VERSION_BUILD_ID) > $@

$(DEB_BUILD_DIR)/conffiles:
	$(OV_QUIET)ln $(OPENVOCS_ROOT)/resources/debian/conffiles $@

$(DEB_PACKAGE_DIR)/DEBIAN: $(DEB_BUILD_DIR)/control $(DEB_BUILD_DIR)/conffiles
	$(OV_QUIET)echo "Creating $@"
	$(OV_QUIET)$(OV_MKDIR) $@
	$(OV_QUIET)$(OV_COPY)  $< $@

endif
