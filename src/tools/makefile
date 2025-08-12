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

OV_TOOL_DIRS   = ov_source
OV_TOOL_DIRS   += ov_config_load_check
OV_TOOL_DIRS   += ov_pcap_analyser
OV_TOOL_DIRS   += ov_stun_server
OV_TOOL_DIRS   += ov_stun_client
OV_TOOL_DIRS   += ov_file_access_check
OV_TOOL_DIRS   += ov_domain_config_verify
OV_TOOL_DIRS   += ov_password
OV_TOOL_DIRS   += ov_ldap_test
OV_TOOL_DIRS   += ov_ldap_test_auth
OV_TOOL_DIRS   += ov_ldap_user_import
OV_TOOL_DIRS   += ov_bin_to_csv
OV_TOOL_DIRS   += ov_resample
OV_TOOL_DIRS   += ov_ssl_membio_testing
OV_TOOL_DIRS   += ov_test_mc
OV_TOOL_DIRS   += ov_mc_cli
OV_TOOL_DIRS   += ov_wav_to_pcm
OV_TOOL_DIRS   += ov_event_listener_cmdline
OV_TOOL_DIRS   += ov_ogg
OV_TOOL_DIRS   += ov_jitter
OV_TOOL_DIRS   += ov_test_coredump
OV_TOOL_DIRS   += ov_mc_socket_debug
OV_TOOL_DIRS   += ov_sip

ifeq ($(OV_UNAME), Linux)
	OV_TOOL_DIRS   += ov_alsa_cli
	OV_TOOL_DIRS   += ov_rtp_cli
endif

all    : target_build_all
depend : target_depend

#-----------------------------------------------------------------------------
target_depend:
	@echo "[DEPEND ] src/lib: scanning dependencies"
	@for dir in $(OV_TOOL_DIRS); do \
		echo "[DEPEND ] entering $$dir ..."; \
		$(MAKE) depend -C $$dir || exit 1; \
		echo "[DEPEND ] leaving $$dir ."; \
	done
	@echo "[DEPEND ] src/lib: dependencies done"
#-----------------------------------------------------------------------------
target_build_all:
	@for dir in $(OV_TOOL_DIRS); do \
		echo "[BUILD  ] entering $$dir ..."; \
		$(MAKE) -C $$dir || exit 1; \
		echo "[BUILD  ] leaving $$dir ."; \
	done
	@echo "[BUILD  ] build done"
#-----------------------------------------------------------------------------
target_test:
	@echo "[BUILD  ] building tests"
	@for dir in $(OV_TOOL_DIRS); do \
		echo "[BUILD  ] entering $$dir ..."; \
		$(MAKE) $(OV_QUIET_MAKE) target_test -C $$dir || exit 1; \
		echo "[BUILD  ] leaving $$dir ."; \
	done
	@echo "[BUILD  ] build tests done"
#-----------------------------------------------------------------------------

