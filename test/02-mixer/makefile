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
IN_1_SRC=/tmp/in_1.pcm16
IN_2_SRC=/tmp/in_2.pcm16
OUT16_SRC=/tmp/out.pcm16
OUT32_SRC=/tmp/out.pcm32


.phony: all
all: build/in_1 build/in_2 build/out16 build/out32

build/in_1: build/convert16
	build/convert16 $(IN_1_SRC) > $@

build/in_2: build/convert16
	build/convert16 $(IN_2_SRC) > $@

build/out16: build/convert16
	build/convert16 $(OUT16_SRC) > $@

build/out32: build/convert32
	build/convert32 $(OUT32_SRC) > $@

.phony: clean
clean:
	rm -fr build

build/%: build/%.o
	gcc -o $@ $^

build/%.o: %.c build
	gcc -c $< -o $@ -std=c11

build:
	mkdir -p build
