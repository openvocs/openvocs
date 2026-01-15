# -*- Makefile -*-


#-----------------------------------------------------------------------------

OV_EDITION            := 2

#-----------------------------------------------------------------------------
OV_VERSION_BUILD_ID_FILE := $(OPENVOCS_ROOT)/.ov_build_id.txt

OV_VERSION_MAJOR      := 2
OV_VERSION_MINOR      := 5
OV_VERSION_PATCH      := 0
OV_VERSION            := $(OV_VERSION_MAJOR).$(OV_VERSION_MINOR).$(OV_VERSION_PATCH)
OV_VERSION_HEX        := $(shell printf "%02i%02i%02i\n" $(OV_VERSION_MAJOR) $(OV_VERSION_MINOR) $(OV_VERSION_PATCH))
OV_VERSION_BUILD_ID   := $(shell [ ! -e $(OV_VERSION_BUILD_ID_FILE) ] && echo 0 > $(OV_VERSION_BUILD_ID_FILE); cat $(OV_VERSION_BUILD_ID_FILE))
OV_VERSION_BUILD_DATE := $(shell date '+%Y.%m.%d_%H:%M:%S')
OV_VERSION_COMPILER   := $(shell echo $(CC) && cc -dumpversion)
OV_VERSION_COMMIT_ID  := $(shell git log --format="%H" -n 1)



#-----------------------------------------------------------------------------
