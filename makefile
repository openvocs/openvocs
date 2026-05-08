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

# note the '-' ...
-include ./makefiles/makefile_const.mk

# define (sub)directories to dive in
OV_DIRECTORIES   = src/lib
OV_DIRECTORIES  += src/os
OV_DIRECTORIES  += src/service
OV_DIRECTORIES  += src/tools
OV_DIRECTORIES  += src/samples

ifeq ($(OV_PLUGINS_PRESENT), 1)
    OV_DIRECTORIES  += $(OV_PLUGINS_DIR)
endif

#.............................................................................
.PHONY: all depend clean debug
.PHONY: target_prepare target_build_id_file target_build_all target_debug

all                 : target_prepare target_build_all target_test

no_tests            : target_build_all

# Disable parallel builds
# NOTPARALLEL:

# dependencies for parallel build
target_prepare      : target_build_id_file
target_stop         : target_build_all
target_build_all    : target_prepare
target_test         : target_build_all
#.............................................................................
build               : target_build_all
#.............................................................................
depend              : target_prepare target_depend
#.............................................................................
clean               : target_clean
#.............................................................................
debug               : target_debug
#.............................................................................
test                : target_test $(OV_TEST_RESOURCE) $(OV_LOCAL_TEST_RESOURCE)
#.............................................................................
build_info          : target_build_info
#.............................................................................

target_depend:
	@echo "[DEPEND ] scanning dependencies"
	@for dir in $(OV_DIRECTORIES); do \
		echo "[DEPEND ] entering $$dir ..."; \
		$(MAKE) depend $(OV_QUIET_MAKE) -C $$dir || exit 1; \
		echo "[DEPEND ] leaving $$dir ."; \
	done
	@echo "[DEPEND ] dependencies done"

#-----------------------------------------------------------------------------

target_build_all:
	@echo "[BUILD  ] building all"
	@for dir in $(OV_DIRECTORIES); do \
		echo "[BUILD  ] entering $$dir ..."; \
		$(MAKE) $(OV_QUIET_MAKE) -C $$dir || exit 1; \
		echo "[BUILD  ] leaving $$dir ."; \
	done
	@echo "[BUILD  ] build done"

#-----------------------------------------------------------------------------

target_test:
	@echo "[BUILD  ] building tests"
	@for dir in $(OV_DIRECTORIES); do \
		echo "[BUILD  ] entering $$dir ..."; \
		$(MAKE) $(OV_QUIET_MAKE) target_test -C $$dir || exit 1; \
		echo "[BUILD  ] leaving $$dir ."; \
	done
	@echo "[BUILD  ] build tests done"

#-----------------------------------------------------------------------------

target_test_resources:
	@echo "[COPY  ] copying generic test resources"
	$(shell cp $(OV_RESOURCE_GENERIC)/* $(OV_TESTRESOURCES)/)
	@echo "[COPY  ] copying generic test resources done"

#-----------------------------------------------------------------------------

target_clean:
	@echo "[CLEAN  ] clean up ..."
	$(OV_QUIET)$(OV_RM) $(OV_JAVASCRIPT_VERSION_FILE)
	$(OV_QUIET)$(OV_RMDIR) $(OV_BUILDDIR)
	$(OV_QUIET)$(shell find $(OPENVOCS_ROOT) -name "*.pyc" -exec rm {} \;)
	$(OV_QUIET)$(shell find $(OPENVOCS_ROOT) -name "__pycache__" -exec rm -r {} \;)
	$(OV_QUIET)$(OV_RM) $(OV_TARBALL_RELEASE_ARCHIVE)
ifeq ($(CC), clang)
	$(OV_QUIET)$(OV_RM) $(OV_COMPILE_COMMANDS)
endif
	@echo "[CLEAN  ] done"

#-----------------------------------------------------------------------------

target_debug:
	$(foreach var,$(.VARIABLES),$(info $(var) = $($(var))))

#-----------------------------------------------------------------------------

$(OV_VERSION_BUILD_ID_FILE):
	$(error "Build system error - $(OV_VERSION_BUILD_ID_FILE) must be present at this point")

target_build_id_file: $(OV_VERSION_BUILD_ID_FILE)
	@echo $$(($$(cat $(OV_VERSION_BUILD_ID_FILE)) + 1)) > $(OV_VERSION_BUILD_ID_FILE)

#-----------------------------------------------------------------------------

$(OV_BUILDDIR): target_prepare

target_prepare:
# check if OPENVOCS_ROOT environment variable is set
ifndef OPENVOCS_ROOT
	$(error OPENVOCS_ROOT is not defined. Aborting.)
endif
#
# check if build directory already exists
ifneq ($(wildcard $(OV_BUILDDIR)/.*),)
	@echo "[PREPARE] build dir FOUND"
else
	@echo "[PREPARE] build dir NOT found, creating ..."
	$(OV_QUIET)$(OV_MKDIR) $(OV_BUILDDIR) $(OV_NUL_STDERR)
	$(OV_QUIET)test -d $(OV_BUILDDIR) || exit 1
	$(OV_QUIET)$(OV_MKDIR) $(OV_OBJDIR) $(OV_NUL_STDERR)
	$(OV_QUIET)$(OV_MKDIR) $(OV_LIBDIR) $(OV_NUL_STDERR)
	$(OV_QUIET)$(OV_MKDIR) $(OV_BINDIR) $(OV_NUL_STDERR)
	$(OV_QUIET)$(OV_MKDIR) $(OV_TESTDIR) $(OV_NUL_STDERR)
endif

#-----------------------------------------------------------------------------

target_build_info:
	$(OV_QUIET)echo "Compiler " $$($$CC --version)

#-----------------------------------------------------------------------------

print:
	@echo "temp release dir: $(OV_TEMP_RELEASE_DIR)"
	@echo "temp res:         $(OV_TEMP_RELEASE_RESOURCES)"
	@echo "res:              $(OV_RELEASE_RESOURCES)"
	@echo "archive:          $(OV_RELEASE_ARCHIVE)"

print-%  : ; @echo $* = $($*)

#-----------------------------------------------------------------------------

.phony: tarball

tarball: $(OV_BUILDDIR) $(OV_RELEASE_ARCHIVE)

$(OV_TEMP_RELEASE_DIR): $(OV_TEMPDIR)
	$(OV_QUIET)$(OV_MKDIR) $@

$(OV_TEMP_RELEASE_DIR)/bin: $(OPENVOCS_ROOT)/build/bin $(OV_TEMP_RELEASE_DIR)
	$(OV_QUIET)$(OV_COPY)  $< $(dir $@)

$(OV_TEMP_RELEASE_DIR)/lib: $(OPENVOCS_ROOT)/build/lib $(OV_TEMP_RELEASE_DIR)
	$(OV_QUIET)$(OV_COPY)  $< $(dir $@)

$(OV_TEMP_RELEASE_DIR)/%: $(OPENVOCS_ROOT)/% $(OV_TEMP_RELEASE_DIR)
	$(OV_QUIET)$(OV_MKDIR) $(dir $@)
	$(OV_QUIET)$(OV_COPY)  $< $(dir $@)

$(OV_RELEASE_CONFIG_DIR):
	$(OV_QUIET)mkdir -p $(OV_RELEASE_CONFIG_DIR)

release_config: $(OV_RELEASE_CONFIG_DIR)
	$(OV_QUIET)rich/generate_config.py --target_path $(OV_RELEASE_CONFIG_DIR)

$(OV_RELEASE_ARCHIVE): build $(OV_TEMP_RELEASE_RESOURCES)
	@echo "[RELEASE  ] Create release archive $@"
	$(OV_QUIET)tar -cf - -C $(OV_TEMPDIR)  $(patsubst $(OV_TEMPDIR)/%,%,$(OV_TEMP_RELEASE_DIR)) | gzip - > $@
	$(OV_QUIET)$(OV_RMDIR) $(OV_TEMP_RELEASE_DIR)
	@echo "[RELEASE  ] done"

#-----------------------------------------------------------------------------

# Packaging for different distributions

# Prerequisite for all specific packaging makefiles...
-include ./makefiles/makefile_localdist.mk

-include ./makefiles/makefile_deb.mk
-include ./makefiles/makefile_rpm.mk

#-----------------------------------------------------------------------------

-include ./makefiles/makefile_build_tools.mk

#-----------------------------------------------------------------------------

-include ./makefiles/makefile_lib.mk

#-----------------------------------------------------------------------------
