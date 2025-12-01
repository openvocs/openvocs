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
#       Create RPM archives for SUSE distributions
#
#       ------------------------------------------------------------------------

-include $(OPENVOCS_ROOT)/makefiles/makefile_const.mk

OV_RPM_ARCH=x86_64

#-------------------------------------------------------------------------------

RPM_BUILD_DIR=$(OV_BUILDDIR)/rpmbuild

#-------------------------------------------------------------------------------

PACKAGE_BASE_NAME=openvocs-$(OV_VERSION)
RPM_PACKAGE_BASE_NAME=$(PACKAGE_BASE_NAME)-$(OV_VERSION_BUILD_ID).$(OV_RPM_ARCH)
RPM_PACKAGE_DIR=$(RPM_BUILD_DIR)/$(RPM_PACKAGE_BASE_NAME)

#-------------------------------------------------------------------------------

.PHONY: rpm
rpm: $(OV_BUILDDIR)/$(RPM_PACKAGE_BASE_NAME).rpm $(RPM_PLUGINS) $(RPM_SIP)

$(RPM_BUILD_DIR)/SOURCES/$(PACKAGE_BASE_NAME).tar.gz: localdist
	echo "Creating $@"
	$(OV_QUIET)$(OV_MKDIR) $(dir $@)
	$(OV_QUIET)$(OV_RMDIR) $(RPM_BUILD_DIR)/$(PACKAGE_BASE_NAME)
	$(OV_QUIET)$(OV_MKDIR)  $(RPM_BUILD_DIR)/$(PACKAGE_BASE_NAME)
	$(OV_QUIET)$(OV_COPY) $(OV_LOCAL_DIST_DIR)/* $(RPM_BUILD_DIR)/$(PACKAGE_BASE_NAME)
	cd $(RPM_BUILD_DIR); tar cf - $(PACKAGE_BASE_NAME) | gzip - > $@

# the spec must be built every time since it incorporates VARIABLES that might
# change - make would not realize a change in the variables though
.PHONY: $(RPM_BUILD_DIR)/SPECS/openvocs.spec
$(RPM_BUILD_DIR)/SPECS/openvocs.spec: $(OPENVOCS_ROOT)/resources/suse/openvocs.spec
	$(OV_MKDIR) $(dir $@)
	echo "Building openvocs.spec for $(OV_VERSION) $(OV_RPM_ARCH) $(OV_VERSION_BUILD_ID)"
	$(OV_QUIET)cat $< | bash $(OPENVOCS_ROOT)/scripts/fill_placeholders.sh $(OV_VERSION) $(OV_RPM_ARCH) $(OV_VERSION_BUILD_ID) > $@

$(RPM_BUILD_DIR)/BUILD:
	$(OV_MKDIR) $(dir $@)

$(RPM_BUILD_DIR)/RPMS:
	$(OV_MKDIR) $(dir $@)

$(RPM_BUILD_DIR)/SRPMS:
	$(OV_MKDIR) $(dir $@)

.PHONY: rpm_dist_tree
rpm_dist_tree: localdist \
	$(RPM_BUILD_DIR)/BUILD \
	$(RPM_BUILD_DIR)/RPMS \
	$(RPM_BUILD_DIR)/SRPMS \
	$(RPM_BUILD_DIR)/SOURCES/$(PACKAGE_BASE_NAME).tar.gz \
	$(RPM_BUILD_DIR)/SPECS/openvocs.spec
	$(OV_QUIET)echo "Populating $(RPM_PACKAGE_DIR)"

$(RPM_BUILD_DIR)/RPMS/$(OV_RPM_ARCH)/$(RPM_PACKAGE_BASE_NAME).rpm: rpm_dist_tree
	$(OV_QUIET)echo "Building $@"
	rpmbuild --define "_topdir $(RPM_BUILD_DIR)" -bb $(RPM_BUILD_DIR)/SPECS/openvocs.spec

$(OV_BUILDDIR)/$(RPM_PACKAGE_BASE_NAME).rpm: $(RPM_BUILD_DIR)/RPMS/$(OV_RPM_ARCH)/$(RPM_PACKAGE_BASE_NAME).rpm
	$(OV_QUIET)mv $< $@
