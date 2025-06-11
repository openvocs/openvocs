# -*- Makefile -*-
#       ------------------------------------------------------------------------
#
#       Copyright 2021 German Aerospace Center DLR e.V. (GSOC)
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

# ccls configuration file for the
# https://github.com/neoclide/coc.nvim plugin
#
# Although .ccls IS a file, it should be rewritten unconditionally
.PHONY: .ccls
.ccls:
	$(OV_QUIET)echo "# Auto-generated" > $@
	$(OV_QUIET)echo "# DONT MODIFY - use 'make .ccls' to recreate" >> $@
	$(OV_QUIET)echo "#" >> $@
	$(OV_QUIET)echo "$(BUILD_DEFINITIONS) $(CFLAGS)" | sed -e 's/ /\n/g' >> $@

# For 'intelligent' features in code editors/ides
.phony: compile_commands.json
compile_commands.json: clean
ifeq ($(CC), clang)
	make -j 8; \
	$(OPENVOCS_ROOT)/scripts/compile_commands.sh $(OV_OBJDIR) $(OPENVOCS_ROOT)/compile_commands.json
else
	http_proxy="" https_proxy="" bear -- make
endif


.PHONY: clang_format
clang_format:
	$(shell find $(OPENVOCS_ROOT) -name "*.[c\|h]" -exec clang-format -style=file -i {} \;)
	@echo "[FORMAT] code formating done"
