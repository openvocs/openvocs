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
#       DTN_QUIET    If set, commands to be executed are hidden
#       DTN_FLAGS    contains compiler flags
#       DTN_LIBS     contains libraries to link against (BE AWARE OF ORDER)
#       DTN_TARGET   contains desired target (default: shared and static lib)
#                   may be overriden to executable
#
#       for example have a look @see src/lib/DTN_base/makefile
#
#       ------------------------------------------------------------------------

# note the '-' ...
-include $(OPENVOCS_ROOT)/makefiles/makefile_const.mk

# define (sub)directories to dive in
DTN_DIRECTORIES   = src/lib
DTN_DIRECTORIES  += src/os
DTN_DIRECTORIES  += src/service
DTN_DIRECTORIES  += src/tools
DTN_DIRECTORIES  += src/samples

ifeq ($(DTN_PLUGINS_PRESENT), 1)
    DTN_DIRECTORIES  += $(DTN_PLUGINS_DIR)
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
test                : target_test $(DTN_TEST_RESOURCE) $(DTN_LOCAL_TEST_RESOURCE)
#.............................................................................
build_info          : target_build_info
#.............................................................................

target_depend:
	@echo "[DEPEND ] scanning dependencies"
	@for dir in $(DTN_DIRECTORIES); do \
		echo "[DEPEND ] entering $$dir ..."; \
		$(MAKE) depend $(DTN_QUIET_MAKE) -C $$dir || exit 1; \
		echo "[DEPEND ] leaving $$dir ."; \
	done
	@echo "[DEPEND ] dependencies done"

#-----------------------------------------------------------------------------

target_build_all:
	@echo "[BUILD  ] building all"
	@for dir in $(DTN_DIRECTORIES); do \
		echo "[BUILD  ] entering $$dir ..."; \
		$(MAKE) $(DTN_QUIET_MAKE) -C $$dir || exit 1; \
		echo "[BUILD  ] leaving $$dir ."; \
	done
	@echo "[BUILD  ] build done"

#-----------------------------------------------------------------------------

target_test:
	@echo "[BUILD  ] building tests"
	@for dir in $(DTN_DIRECTORIES); do \
		echo "[BUILD  ] entering $$dir ..."; \
		$(MAKE) $(DTN_QUIET_MAKE) target_test -C $$dir || exit 1; \
		echo "[BUILD  ] leaving $$dir ."; \
	done
	@echo "[BUILD  ] build tests done"

#-----------------------------------------------------------------------------

target_test_resources:
	@echo "[COPY  ] copying generic test resources"
	$(shell cp $(DTN_RESOURCE_GENERIC)/* $(DTN_TESTRESOURCES)/)
	@echo "[COPY  ] copying generic test resources done"

#-----------------------------------------------------------------------------

target_clean:
	@echo "[CLEAN  ] clean up ..."
	$(DTN_QUIET)rm -rf $(DTN_JAVASCRIPT_VERSION_FILE)
	$(DTN_QUIET)$(DTN_RMDIR) $(DTN_BUILDDIR)
	$(DTN_QUIET)$(shell find $(OPENVOCS_ROOT) -name "*.pyc" -exec rm {} \;)
	$(DTN_QUIET)$(shell find $(OPENVOCS_ROOT) -name "__pycache__" -exec rm -r {} \;)
	$(DTN_QUIET)$(DTN_RM) $(DTN_TARBALL_RELEASE_ARCHIVE)
ifeq ($(CC), clang)
	$(DTN_QUIET)$(DTN_RM) $(DTN_COMPILE_COMMANDS)
endif
	@echo "[CLEAN  ] done"

#-----------------------------------------------------------------------------

target_debug:
	$(foreach var,$(.VARIABLES),$(info $(var) = $($(var))))

#-----------------------------------------------------------------------------

$(DTN_VERSION_BUILD_ID_FILE):
	$(error "Build system error - $(DTN_VERSION_BUILD_ID_FILE) must be present at this point")

target_build_id_file: $(DTN_VERSION_BUILD_ID_FILE)
	@echo $$(($$(cat $(DTN_VERSION_BUILD_ID_FILE)) + 1)) > $(DTN_VERSION_BUILD_ID_FILE)

#-----------------------------------------------------------------------------

$(DTN_BUILDDIR): target_prepare

target_prepare:
# check if OPENVOCS_ROOT environment variable is set
ifndef OPENVOCS_ROOT
	$(error OPENVOCS_ROOT is not defined. Aborting.)
endif
#
# check if build directory already exists
ifneq ($(wildcard $(DTN_BUILDDIR)/.*),)
	@echo "[PREPARE] build dir FOUND"
else
	@echo "[PREPARE] build dir NOT found, creating ..."
	$(DTN_QUIET)$(DTN_MKDIR) $(DTN_BUILDDIR) $(DTN_NUL_STDERR)
	$(DTN_QUIET)test -d $(DTN_BUILDDIR) || exit 1
	$(DTN_QUIET)$(DTN_MKDIR) $(DTN_OBJDIR) $(DTN_NUL_STDERR)
	$(DTN_QUIET)$(DTN_MKDIR) $(DTN_LIBDIR) $(DTN_NUL_STDERR)
	$(DTN_QUIET)$(DTN_MKDIR) $(DTN_BINDIR) $(DTN_NUL_STDERR)
	$(DTN_QUIET)$(DTN_MKDIR) $(DTN_TESTDIR) $(DTN_NUL_STDERR)
endif

#-----------------------------------------------------------------------------

target_build_info:
	$(DTN_QUIET)echo "Compiler " $$($$CC --version)

#-----------------------------------------------------------------------------

print:
	@echo "temp release dir: $(DTN_TEMP_RELEASE_DIR)"
	@echo "temp res:         $(DTN_TEMP_RELEASE_RESOURCES)"
	@echo "res:              $(DTN_RELEASE_RESOURCES)"
	@echo "archive:          $(DTN_RELEASE_ARCHIVE)"

print-%  : ; @echo $* = $($*)

#-----------------------------------------------------------------------------

-include $(OPENVOCS_ROOT)/makefiles/makefile_build_tools.mk

#-----------------------------------------------------------------------------

-include $(OPENVOCS_ROOT)/makefiles/makefile_lib.mk

#-----------------------------------------------------------------------------
