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
#       This file will be included by a module's makefile.
#
#       ------------------------------------------------------------------------

# OV_OBJ              = $(patsubst %.c,%.o,$(OV_SRC))
OV_OBJ_1            = $(notdir $(OV_SRC))
OV_OBJ_2            = $(addprefix $(OV_OBJDIR)/$(OV_DIRNAME)/,$(OV_OBJ_1))
OV_OBJ              = $(patsubst %.c,%.o,$(OV_OBJ_2))
OV_DEP              = $(patsubst %.c,%.d,$(OV_SRC))
OV_OBJ_EXEC         = $(addprefix $(OV_OBJDIR)/$(OV_DIRNAME)/,$(patsubst %,%.o,$(notdir $(OV_EXECUTABLE))))
OV_OBJ_TEST         = $(addprefix $(OV_OBJDIR)/$(OV_DIRNAME)/,$(patsubst %.c,%.o,$(notdir $(OV_TEST_SOURCES))))
OV_OBJ_IF_TEST      = $(addprefix $(OV_OBJDIR)/$(OV_DIRNAME)/,$(patsubst %.c,%.o,$(notdir $(OV_TEST_IF_SRC))))

OV_STATIC           = $(OV_LIBDIR)/$(OV_LIBNAME)$(OV_EDITION).a

OV_SHARED_LINKER_NAME = $(OV_LIBNAME)$(OV_EDITION).so
OV_SHARED_SONAME      = $(OV_SHARED_LINKER_NAME).$(OV_VERSION_MAJOR)
OV_SHARED_REAL        = $(OV_SHARED_LINKER_NAME).$(OV_VERSION)
OV_SHARED             = $(OV_LIBDIR)/$(OV_SHARED_REAL) $(OV_LIBDIR)/$(OV_SHARED_LINKER_NAME) $(OV_LIBDIR)/$(OV_SHARED_SONAME)

OV_TEST_RESOURCE        = $(wildcard resources/test/*)
OV_TEST_RESOURCE_TARGET = $(addprefix $(OV_TEST_RESOURCE_DIR)/,$(notdir $(OV_TEST_RESOURCE)))

L_SRCDIRS          := $(shell find . -name '*.c' -exec dirname {} \; | uniq)
VPATH              := $(L_SRCDIRS)

all                 : target_prepare target_build_all target_install
target_build_all    : target_sources $(OV_JAVASCRIPT_VERSION_FILE)
target_sources      : $(OV_TARGET)
depend              : target_depend
target_install      : target_install_header
target_test         : $(OV_TEST_RESOURCE) $(OV_TEST_RESOURCE_TARGET) $(OV_TEST_TARGET) $(OV_OBJ_IF_TEST)  $(OV_OBJ_TEST)
test 				: all target_test
install 			: target_system_install
uninstall           : target_system_uninstall

#-----------------------------------------------------------------------------
# Targets
#-----------------------------------------------------------------------------

$(OV_OBJDIR)/$(OV_DIRNAME)/%.o : %.c
	@echo "[CC     ] $<"
	$(OV_QUIET)$(CC) $(BUILD_DEFINITIONS) $(TEST_DEFINITIONS)\
	$(CFLAGS) $(OV_FLAGS) -MMD -c $< -o $@

$(OV_OBJDIR)/$(OV_DIRNAME)/%_test.o : %_test.c $(OV_OBJ_IF_TEST)
	@echo "[CC     ] $<"
	$(OV_QUIET)$(CC) \
		$(BUILD_DEFINITIONS) \
		$(TEST_DEFINITIONS) \
		$(CFLAGS) $(OV_FLAGS) -MMD -c $< -o $@

$(OV_OBJDIR)/$(OV_DIRNAME)/%_test_interface.o : %_test_interface.c $(OV_TEST_IF_SRC)
	@echo "[CC     ] $<"
	$(OV_QUIET)$(CC) $(TEST_DEFINITIONS) \
	 $(CFLAGS) $(OV_FLAGS) -MMD -c $< -o $@

$(OV_TESTDIR)/$(OV_DIRNAME)/%_test.run : $(OV_OBJDIR)/$(OV_DIRNAME)/%_test.o $(OV_OBJ_IF_TEST) $(OV_OBJ)
	$(eval NO_SELF_DEPENDENCY := $(filter-out $(<:%_test.o=%.o) , $(OV_OBJ)))
	$(eval NO_SELF_DEPENDENCY := $(filter-out $(OV_OBJ_EXEC) , $(NO_SELF_DEPENDENCY)))
	$(OV_QUIET)$(CC) -o $@  $< $(NO_SELF_DEPENDENCY) $(LFLAGS) $(OV_LIBS)
	@echo "[TEST EXEC] $(notdir $@ ) created"

# ... will create the directory for the resources and copy resources
$(OV_TEST_RESOURCE_TARGET): target_test_resource_prepare $(OV_TEST_RESOURCE)
	$(OV_QUIET) $(shell cp -r $(OV_TEST_RESOURCE) $(OV_TEST_RESOURCE_DIR)/)

#-----------------------------------------------------------------------------

-include $(OV_OBJDIR)/$(OV_DIRNAME)/*.d

#-----------------------------------------------------------------------------
$(OV_STATIC):  $(OV_OBJ)
	$(OV_QUIET)ar rcs $(OV_STATIC) $(OV_OBJ)
	$(OV_QUIET)ranlib $(OV_STATIC)
	@echo "[STATIC ] $(notdir $(OV_STATIC)) created"

#-----------------------------------------------------------------------------

$(OV_LIBDIR)/$(OV_SHARED_REAL): $(OV_OBJ)
ifeq ($(OV_UNAME), Linux)
	$(OV_QUIET)$(CC) -shared -o $@ $(OV_OBJ) $(LFLAGS) \
		-Wl,-soname,$(OV_SHARED_SONAME) \
		-Wl,--defsym -Wl,__OV_LD_VERSION=0x$(OV_VERSION_HEX) \
		-Wl,--defsym -Wl,__OV_LD_EDITION=0x$(OV_EDITION) \
		$(OV_LIBS)
else ifeq ($(OV_UNAME), Darwin)
	$(OV_QUIET)$(CC) -shared -o $@ $(OV_OBJ) $(LFLAGS) \
		-compatibility_version $(OV_VERSION) \
		-current_version $(OV_VERSION) \
		$(OV_LIBS)
else
	@echo "[SHARED ] OS $(OV_UNAME) unsupported yet."
endif
ifeq ($(OV_BUILD_MODE), STRIP)
	$(OV_QUIET)$(OV_STRIP) $@
endif
	@echo "[SHARED ] $(notdir $@) created"

#-----------------------------------------------------------------------------

$(OV_LIBDIR)/$(OV_SHARED_LINKER_NAME): $(OV_LIBDIR)/$(OV_SHARED_REAL)
	$(OV_QUIET)$(shell \
		cd $(OV_LIBDIR) ; \
		$(OV_SYMLINK) $(OV_SHARED_REAL) $(OV_SHARED_LINKER_NAME); )
	@echo "[LINK   ] $(notdir $@) created"

#-----------------------------------------------------------------------------

$(OV_LIBDIR)/$(OV_SHARED_SONAME): $(OV_LIBDIR)/$(OV_SHARED_REAL)
	$(OV_QUIET)$(shell \
		cd $(OV_LIBDIR) ; \
		$(OV_SYMLINK) $(OV_SHARED_REAL) $(OV_SHARED_SONAME);)
	@echo "[LINK   ] $(notdir $@) created"

#-----------------------------------------------------------------------------

$(OV_EXECUTABLE): $(OV_OBJ)
	$(OV_QUIET)$(CC) -o $(OV_EXECUTABLE) $(OV_OBJ) $(LFLAGS) $(OV_LIBS)
	@echo "[EXEC   ] $(notdir $(OV_EXECUTABLE)) created"

#-----------------------------------------------------------------------------

target_test_resource_prepare:
	$(OV_QUIET)$(OV_MKDIR) $(OV_TEST_RESOURCE_DIR) $(OV_NUL_STDERR)

#-----------------------------------------------------------------------------
target_depend:
	@echo "[DEPEND ] generated automagically, skipped"
#	@echo "[DEPEND ] CURDIR         : $(CURDIR)"
#	@echo "[DEPEND ] OV_BUILDDIR    : $(OV_BUILDDIR)"
#	@echo "[DEPEND ] OV_DIRNAME     : $(OV_DIRNAME)"
#

#-----------------------------------------------------------------------------
target_install_header:
	$(OV_QUIET)l_target=$(OV_BUILDDIR)/include/$(OV_DIRNAME); \
	$(OV_MKDIR) $$l_target; \
	for f in $(OV_HDR); do \
	  l_base=`basename $$f`; \
	  test -L  $$l_target/$$l_base || \
	  $(OV_SYMLINK) $(abspath $$f) $$l_target/$$l_base; \
	done; \
	echo "[INSTALL] symlinks created"

#-----------------------------------------------------------------------------

target_system_install: $(OV_EXECUTABLE)
	$(shell sudo cp $(OV_EXECUTABLE) $(OV_INSTALLDIR)/$(notdir $(OV_EXECUTABLE)))
	@echo "[INSTALLED AS] $(OV_INSTALLDIR)/$(notdir $(OV_EXECUTABLE))"

#-----------------------------------------------------------------------------

target_system_uninstall:
	$(shell sudo rm $(OV_INSTALLDIR)/$(notdir $(OV_EXECUTABLE)))
	@echo "[UNINSTALLED] $(OV_INSTALLDIR)/$(notdir $(OV_EXECUTABLE))"

#-----------------------------------------------------------------------------
target_prepare:
	$(OV_QUIET)$(OV_MKDIR) $(OV_OBJDIR)               $(OV_NUL_STDERR)
	$(OV_QUIET)$(OV_MKDIR) $(OV_OBJDIR)/$(OV_DIRNAME) $(OV_NUL_STDERR)
	$(OV_QUIET)$(OV_MKDIR) $(OV_TESTDIR)/$(OV_DIRNAME) $(OV_NUL_STDERR)
	$(OV_QUIET)$(OV_MKDIR) $(OV_LIBDIR)               $(OV_NUL_STDERR)
	$(OV_QUIET)$(OV_MKDIR) $(OV_BINDIR)               $(OV_NUL_STDERR)
	$(OV_QUIET)$(OV_MKDIR) $(OV_PLUGINDIR)            $(OV_NUL_STDERR)

#-----------------------------------------------------------------------------
$(OV_JAVASCRIPT_VERSION_FILE): $(OPENVOCS_ROOT)/makefiles/makefile_version.mk
	$(OV_QUIET)echo "// Auto-Generated - DO NOT MODIFY" > $@ && echo "VERSION_NUMBER = \"$(OV_VERSION)\";" >> $@
