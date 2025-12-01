# -*- Makefile -*-
#       ------------------------------------------------------------------------
#
#       Copyright 2025 German Aerospace Center DLR e.V. (GSOC)
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
#       Authors         Markus Töpfer
#
#       ------------------------------------------------------------------------
#
#       Install lib 
#
#    

-include $(OPENVOCS_ROOT)/makefiles/makefile_const.mk

OV_LIB_INSTALL_DIR      := /usr

OV_LIB_SOURCE_ROOT 		:= $(OPENVOCS_ROOT)/src/lib
OV_LIB_SOURCE_DIR 		:= $(sort $(dir $(wildcard $(OV_LIB_SOURCE_ROOT)/*/)))
OV_LIB_NAMES            := $(notdir $(patsubst %/,%,$(dir $(OV_LIB_SOURCE_DIR))))
OV_LIB_NAME_EDITION     := $(addsuffix $(OV_EDITION), $(OV_LIB_NAMES))

OV_LIB_FILES_0=$(addsuffix $(OV_EDITION).so, $(OV_LIB_NAMES))
OV_LIB_FILES_1=$(addsuffix $(OV_EDITION).so.$(OV_VERSION_MAJOR), $(OV_LIB_NAMES))
OV_LIB_FILES_2=$(addsuffix $(OV_EDITION).so.$(OV_VERSION_MAJOR).$(OV_VERSION_MINOR).$(OV_VERSION_PATCH), $(OV_LIB_NAMES))
OV_LIB_FILES_ABSPATH=$(addprefix $(OV_LIB_INSTALL_DIR)/lib/lib, $(OV_LIB_FILES_0) $(OV_LIB_FILES_1) $(OV_LIB_FILES_2))

target_install_lib_header : 
	$(OV_QUIET) sudo $(OV_MKDIR) $(OV_LIB_INSTALL_DIR)/include/openvocs
	$(OV_QUIET)for item in $(OV_LIB_NAMES) ; do \
        sudo $(OV_MKDIR) $(OV_LIB_INSTALL_DIR)/include/openvocs/$$item"$(OV_EDITION)"; \
        sudo $(OV_COPY) $(OV_LIB_SOURCE_ROOT)/$$item/include/*.h $(OV_LIB_INSTALL_DIR)/include/openvocs/$$item"$(OV_EDITION)"/. ;\
    done

target_install_lib_sources : $(LIB_FILES_ABSPATH)
	$(OV_QUIET) echo "Creating $?"
	$(OV_QUIET) sudo $(OV_MKDIR) $(OV_LIB_INSTALL_DIR)/lib/openvocs
	for f in $?; do sudo $(OV_COPY) $$f $(OV_LIB_INSTALL_DIR)/lib/openvocs; done

target_install_lib_pkg_config :
	$(OV_QUIET)for item in $(OV_LIB_NAME_EDITION) ; do \
		sudo touch $(OV_LIB_INSTALL_DIR)/lib/pkgconfig/lib$$item.pc ;\
		sudo chmod a+w $(OV_LIB_INSTALL_DIR)/lib/pkgconfig/lib$$item.pc ;\
		sudo echo $$item.pc > $(OV_LIB_INSTALL_DIR)/lib/pkgconfig/lib$$item.pc ;\
		sudo echo "prefix=$(OV_LIB_INSTALL_DIR)" >> $(OV_LIB_INSTALL_DIR)/lib/pkgconfig/lib$$item.pc ;\
		sudo echo "exec_prefix=$(OV_LIB_INSTALL_DIR)" >> $(OV_LIB_INSTALL_DIR)/lib/pkgconfig/lib$$item.pc ;\
		sudo echo "includedir=$(OV_LIB_INSTALL_DIR)/include" >> $(OV_LIB_INSTALL_DIR)/lib/pkgconfig/lib$$item.pc ;\
		sudo echo "libdir=$(OV_LIB_INSTALL_DIR)/lib" >> $(OV_LIB_INSTALL_DIR)/lib/pkgconfig/lib$$item.pc ;\
		sudo echo "" >> $(OV_LIB_INSTALL_DIR)/lib/pkgconfig/lib$$item.pc ;\
		sudo echo "Name:lib$$item" >> $(OV_LIB_INSTALL_DIR)/lib/pkgconfig/lib$$item.pc ;\
		sudo echo "Description: The $$item Library of openvocs" >> $(OV_LIB_INSTALL_DIR)/lib/pkgconfig/lib$$item.pc ;\
		sudo echo "Version:$(OV_VERSION_MAJOR).$(OV_VERSION_MINOR).$(OV_VERSION_PATCH)" >> $(OV_LIB_INSTALL_DIR)/lib/pkgconfig/lib$$item.pc ;\
		sudo echo "Cflags: -I$(OV_LIB_INSTALL_DIR)/include/openvocs/$$item" >> $(OV_LIB_INSTALL_DIR)/lib/pkgconfig/lib$$item.pc ;\
		sudo echo "Libs: -L$(OV_LIB_INSTALL_DIR)/lib/openvocs/ -llib$$item" >> $(OV_LIB_INSTALL_DIR)/lib/pkgconfig/lib$$item.pc ;\
    done

.phony: target_install_lib
target_install_lib :  target_install_lib_header target_install_lib_sources target_install_lib_pkg_config