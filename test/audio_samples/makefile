#-*- Makefile -*-
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
#
#
OV_SAMPLERATE=48000
BUILD_DIR=build

TEXT_TO_SPEECH_SOURCES=$(wildcard *.txt)
PCM16S_FILES=$(addprefix $(BUILD_DIR)/, $(addsuffix .pcm16s, $(basename $(TEXT_TO_SPEECH_SOURCES))))
WAV_FILES=$(addprefix $(BUILD_DIR)/, $(addsuffix .pcm16s, $(basename $(wildcard *.wav))))
TEMP_WAV_FILES=$(addsuffix .wav, $(basename $(TEXT_TO_SPEECH_SOURCES)))

-include $(OPENVOCS_ROOT)/makefiles/makefile_const.mk

.phony: all clean

all: $(PCM16S_FILES) $(WAV_FILES)
	echo $(PCM16S_FILES)

$(BUILD_DIR):
	$(OV_QUIET) $(OV_MKDIR) $(BUILD_DIR)

$(BUILD_DIR)/%.wav: %.txt $(BUILD_DIR)
	$(OV_QUIET) cat $< | espeak -s 120 -w $@

$(BUILD_DIR)/%.pcm16s: $(BUILD_DIR)/%.pcm.wav
	$(OV_QUIET) $(OPENVOCS_ROOT)/build/bin/ov_wav_to_pcm $< $@

$(BUILD_DIR)/%.pcm.wav: $(BUILD_DIR)/%.wav $(BUILD_DIR)
	$(OV_QUIET) mplayer -af volume=0,resample=$(OV_SAMPLERATE):0:1 $< -ao pcm:fast:file="$@"

$(BUILD_DIR)/%.pcm.wav: %.wav $(BUILD_DIR)
	$(OV_QUIET) mplayer -af volume=0,resample=$(OV_SAMPLERATE):0:1 $< -ao pcm:fast:file="$@"

clean:
	$(OV_QUIET) $(OV_RM) -fr $(BUILD_DIR) && echo "Removed $(BUILD_DIR)"
