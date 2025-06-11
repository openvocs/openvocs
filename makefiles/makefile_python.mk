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
OV_PYTHON_DIST_DIR=/usr/local

OV_PYTHON_DIST=Python-3.7.0
OV_PYTHON_DIST_TAR=Python-3.7.0.tgz
OV_PYTHON_DIST_URI='https://www.python.org/ftp/python/3.7.0'


OV_PYTHON=$(OV_PYTHON_DIST_DIR)/bin/python3
OV_PYTHON_PIP=$(OV_PYTHON_DIST_DIR)/bin/pip3
OV_PYLINTER=flake8
OV_PYTHON_PIP_PACKAGES=virtualenv PyQt5 libtmux flake8

.phony: python_all python_check python_clean

python_all: python_dist $(OPENVOCS_PYTHON_VENV_REL)

python_clean:
	$(OV_QUIET)$(OV_RMDIR) $(OPENVOCS_PYTHON_VENV_ROOT) $(OV_PYTHON_LINTER_LOG_FILE)

python_check:
	$(OV_QUIET)$(OV_RM) $(OV_PYTHON_LINTER_LOG_FILE)
	$(OV_QUIET)make $(OV_PYTHON_LINTER_LOG_FILE)
	$(OV_QUIET)if [ -e $(OV_PYTHON_LINTER_LOG_FILE) ]; then \
		view $(OV_PYTHON_LINTER_LOG_FILE); \
	fi

python_dist: /usr/local/bin/python3.7

/usr/local/bin/python3.7:
	$(OV_QUIET)cd $(OV_TEMPDIR) && wget $(OV_PYTHON_DIST_URI)/$(OV_PYTHON_DIST_TAR)
	$(OV_QUIET)cd $(OV_TEMPDIR) && tar xf $(OV_PYTHON_DIST_TAR)
	$(OV_QUIET)cd $(OV_TEMPDIR)/$(OV_PYTHON_DIST) && \
		./configure && make && sudo make install

$(OV_TEMPDIR)/$(OV_PYTHON_DIST_TAR):

$(OPENVOCS_PYTHON_VENV_REL):
	$(OV_QUIET)$(OV_PYTHON) -m pip install --user --upgrade pip
	$(OV_QUIET)$(OV_PYTHON) -m pip install --user virtualenv
	$(OV_QUIET)$(OV_MKDIR) $@
	$(OV_QUIET)$(OV_PYTHON) -m venv $@
	$(OV_QUIET)bash -c "source $@/bin/activate; \
		python3 -m pip install --upgrade pip; \
		python3 -m pip install $(OV_PYTHON_PIP_PACKAGES)"

$(OV_PYTHON_LINTER_LOG_FILE): $(OV_BUILDDIR) $(OPENVOCS_PYTHON_VENV_REL)
	$(OV_QUIET)echo "Ensure that you ran make python_all && source $(OPENVOCS_ROOT)/env.sh"
	$(OV_QUIET)if $(OV_PYLINTER) python scripts --output-file=$(OV_PYTHON_LINTER_LOG_FILE); then \
		$(OV_RM) $(OV_PYTHON_LINTER_LOG_FILE); \
		echo "No Python Errors"; \
	fi

