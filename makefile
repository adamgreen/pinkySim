# Copyright 2014 Adam Green (https://github.com/adamgreen)
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# User can set VERBOSE variable to have all commands echoed to console for debugging purposes.
ifdef VERBOSE
    Q :=
else
    Q := @
endif


#######################################
#  High Level Make Rules
#######################################
.PHONY : all test clean gcov

all : RUN_CPPUTEST_TESTS RUN_LIBMRICORE_TESTS RUN_LIBCOMMON_TESTS RUN_LIBPINKYSIM_TESTS

test : all RUN_LIBGDBREMOTE_TESTS LIBCOMMSERIAL_LIB RUN_LIBTHUNK2REAL_TESTS

gcov : RUN_CPPUTEST_TESTS GCOV_LIBMRICORE GCOV_LIBCOMMON GCOV_LIBPINKYSIM GCOV_LIBGDBREMOTE

clean : 
	@echo Cleaning pinkysim
	$Q $(REMOVE_DIR) $(OBJDIR) $(QUIET)
	$Q $(REMOVE_DIR) $(LIBDIR) $(QUIET)
	$Q $(REMOVE_DIR) $(GCOVDIR) $(QUIET)
	$Q $(REMOVE) *_tests$(EXE) $(QUIET)
	$Q $(REMOVE) *_tests_gcov$(EXE) $(QUIET)


# Make sure that gcov goal isn't used with incompatible rules.
ifeq "$(findstring gcov,$(MAKECMDGOALS))" "gcov"
    ifeq "$(findstring all,$(MAKECMDGOALS))" "all"
        $(error Can't use 'all' and 'gcov' goals together.)
    endif
    ifeq "$(findstring host,$(MAKECMDGOALS))" "host"
        $(error Can't use 'host' and 'gcov' goals together.)
    endif
    ifeq "$(findstring test,$(MAKECMDGOALS))" "test"
        $(error Can't use 'test' and 'gcov' goals together.)
    endif
endif

#  Names of tools for compiling binaries to run on this host system.
HOST_GCC := gcc
HOST_GPP := g++
HOST_AS  := gcc
HOST_LD  := g++
HOST_AR  := ar

# Handle Windows and *nix differences.
ifeq "$(OS)" "Windows_NT"
    MAKEDIR = mkdir $(subst /,\,$(dir $@))
    REMOVE := del /q
    REMOVE_DIR := rd /s /q
    QUIET := >nul 2>nul & exit 0
    EXE := .exe
else
    MAKEDIR = mkdir -p $(dir $@)
    REMOVE := rm
    REMOVE_DIR := rm -r -f
    QUIET := > /dev/null 2>&1 ; exit 0
    EXE :=
endif

# Flags to use when compiling binaries to run on this host system.
HOST_GCCFLAGS := -O2 -g3 -Wall -Wextra -Werror -Wno-unused-parameter -MMD -MP
HOST_GCCFLAGS += -ffunction-sections -fdata-sections -fno-common
HOST_GCCFLAGS += -include mri/CppUTest/include/CppUTest/MemoryLeakDetectorMallocMacros.h
HOST_GPPFLAGS := $(HOST_GCCFLAGS) -include mri/CppUTest/include/CppUTest/MemoryLeakDetectorNewMacros.h
HOST_GCCFLAGS += -std=gnu90
HOST_ASFLAGS  := -x assembler-with-cpp -MMD -MP

# Output directories for intermediate object files.
OBJDIR        := obj
HOST_OBJDIR   := $(OBJDIR)

# Output directory for gcov files.
GCOVDIR := gcov

# Output directories for final libraries.
LIBDIR        := lib
HOST_LIBDIR   := $(LIBDIR)

# Modify some things if we want to run code coverage as part of this build.
ifeq "$(findstring gcov,$(MAKECMDGOALS))" "gcov"
    HOST_OBJDIR   := $(GCOVDIR)/$(HOST_OBJDIR)
    HOST_LIBDIR   := $(GCOVDIR)/$(HOST_LIBDIR)
    HOST_GCCFLAGS += -fprofile-arcs -ftest-coverage
    HOST_GPPFLAGS += -fprofile-arcs -ftest-coverage
    HOST_LDFLAGS  += -fprofile-arcs -ftest-coverage
    GCOV          := _gcov
else
    GCOV :=
endif

# Most of the needed headers are located here.
INCLUDES := include

# Start out with an empty header file dependency list.  Add module files as we go.
DEPS :=

# Useful macros.
objs = $(addprefix $2/,$(addsuffix .o,$(basename $(wildcard $1/*.c $1/*.cpp $1/*.S))))
host_objs = $(call objs,$1,$(HOST_OBJDIR))
add_deps = $(patsubst %.o,%.d,$(HOST_$1_OBJ))
includes = $(patsubst %,-I%,$1)
define build_lib
	@echo Building $@
	$Q $(MAKEDIR)
	$Q $($1_AR) -rc $@ $?
endef
define link_exe
	@echo Building $@
	$Q $(MAKEDIR)
	$Q $($1_LD) $($1_LDFLAGS) $^ -o $@
endef
define run_gcov
	$Q $(REMOVE) $1_output.txt $(QUIET)
	$Q mkdir -p gcov/$1_tests $(QUIET)
	$Q $(foreach i,$(HOST_$1_OBJ),gcov -object-directory=$(dir $i) $(notdir $i) >> $1_output.txt ;)
	$Q mv $1_output.txt gcov/$1_tests/ $(QUIET)
	$Q mv *.gcov gcov/$1_tests/ $(QUIET)
	$Q mri/CppUTest/scripts/filterGcov.sh gcov/$1_tests/$1_output.txt /dev/null gcov/$1_tests/$1.txt
	$Q cat gcov/$1_tests/$1.txt
endef


#######################################
# libCppUtest.a
HOST_CPPUTEST_OBJ := $(call host_objs,mri/CppUTest/src/CppUTest) $(call host_objs,mri/CppUTest/src/Platforms/Gcc)
HOST_CPPUTEST_LIB := $(HOST_LIBDIR)/libCppUTest.a
$(HOST_CPPUTEST_LIB) : INCLUDES := $(INCLUDES) mri/CppUTest/include
$(HOST_CPPUTEST_LIB) : $(HOST_CPPUTEST_OBJ)
	$(call build_lib,HOST)
DEPS += $(call add_deps,CPPUTEST)

# Unit tests
HOST_CPPUTEST_TESTS_OBJ := $(call host_objs,mri/CppUTest/tests)
HOST_CPPUTEST_TESTS_EXE := CppUTest_tests$(GCOV)
$(HOST_CPPUTEST_TESTS_EXE) : INCLUDES := $(INCLUDES) mri/CppUTest/include
$(HOST_CPPUTEST_TESTS_EXE) : $(HOST_CPPUTEST_TESTS_OBJ) $(HOST_CPPUTEST_LIB)
	$(call link_exe,HOST)
RUN_CPPUTEST_TESTS : $(HOST_CPPUTEST_TESTS_EXE)
	$Q $(HOST_CPPUTEST_TESTS_EXE)
DEPS += $(call add_deps,CPPUTEST_TESTS)


#######################################
# Native memory access objects
HOST_NATIVE_MEM_OBJ   := $(call host_objs,mri/memory/native)
DEPS += $(call add_deps,NATIVE_MEM)


#######################################
# libmricore.a
HOST_LIBMRICORE_OBJ   := $(call host_objs,mri/core)
HOST_LIBMRICORE_LIB   := $(HOST_LIBDIR)/libmricore.a
$(HOST_LIBMRICORE_LIB) : INCLUDES := mri/include
$(HOST_LIBMRICORE_LIB) : $(HOST_LIBMRICORE_OBJ)
	$(call build_lib,HOST)
DEPS += $(call add_deps,LIBMRI)

# Unit tests
HOST_LIBMRICORE_TESTS_OBJ := $(call host_objs,mri/tests/tests) $(call host_objs,mri/tests/mocks)
HOST_LIBMRICORE_TESTS_EXE := mri_tests$(GCOV)
$(HOST_LIBMRICORE_TESTS_EXE) : INCLUDES := mri/include mri/CppUTest/include tests/tests mri/tests/mocks
$(HOST_LIBMRICORE_TESTS_EXE) : $(HOST_LIBMRICORE_TESTS_OBJ) \
                               $(HOST_NATIVE_MEM_OBJ) \
                               $(HOST_LIBMRICORE_LIB) \
                               $(HOST_CPPUTEST_LIB)
	$(call link_exe,HOST)
RUN_LIBMRICORE_TESTS : $(HOST_LIBMRICORE_TESTS_EXE)
	$Q $(HOST_LIBMRICORE_TESTS_EXE)
DEPS += $(call add_deps,LIBMRICORE_TESTS)
GCOV_LIBMRICORE : RUN_LIBMRICORE_TESTS
	$(call run_gcov,LIBMRICORE)


#######################################
# libcommon.a
HOST_LIBCOMMON_OBJ := $(call host_objs,libcommon/src)
HOST_LIBCOMMON_LIB := $(HOST_LIBDIR)/libcommon.a
$(HOST_LIBCOMMON_LIB) : $(HOST_LIBCOMMON_OBJ)
	$(call build_lib,HOST)
DEPS += $(call add_deps,LIBCOMMON)

# Unit tests
HOST_LIBCOMMON_TESTS_OBJ := $(call host_objs,libcommon/tests)
HOST_LIBCOMMON_TESTS_EXE := libcommon_tests$(GCOV)
$(HOST_LIBCOMMON_TESTS_EXE) : INCLUDES := $(INCLUDES) mri/CppUTest/include
$(HOST_LIBCOMMON_TESTS_EXE) : $(HOST_LIBCOMMON_TESTS_OBJ) $(HOST_LIBCOMMON_LIB) $(HOST_CPPUTEST_LIB)
	$(call link_exe,HOST)
RUN_LIBCOMMON_TESTS : $(HOST_LIBCOMMON_TESTS_EXE)
	$Q $(HOST_LIBCOMMON_TESTS_EXE)
GCOV_LIBCOMMON : RUN_LIBCOMMON_TESTS
	$(call run_gcov,LIBCOMMON)
DEPS += $(call add_deps,LIBCOMMON_TESTS)


#######################################
# libpinkysim.a
HOST_LIBPINKYSIM_OBJ := $(call host_objs,libpinkysim/src)
HOST_LIBPINKYSIM_LIB := $(HOST_LIBDIR)/libpinkysim.a
$(HOST_LIBPINKYSIM_LIB) : $(HOST_LIBPINKYSIM_OBJ)
	$(call build_lib,HOST)
DEPS += $(call add_deps,LIBPINKYSIM)

# Unit tests
HOST_LIBPINKYSIM_TESTS_OBJ := $(call host_objs,libpinkysim/tests)
HOST_LIBPINKYSIM_TESTS_EXE := libpinkysim_tests$(GCOV)
$(HOST_LIBPINKYSIM_TESTS_EXE) : INCLUDES := $(INCLUDES) mri/CppUTest/include
$(HOST_LIBPINKYSIM_TESTS_EXE) : $(HOST_LIBPINKYSIM_TESTS_OBJ) \
                                $(HOST_LIBPINKYSIM_LIB) \
                                $(HOST_LIBCOMMON_LIB) \
                                $(HOST_CPPUTEST_LIB)
	$(call link_exe,HOST)
RUN_LIBPINKYSIM_TESTS : $(HOST_LIBPINKYSIM_TESTS_EXE)
	$Q $(HOST_LIBPINKYSIM_TESTS_EXE)
GCOV_LIBPINKYSIM : RUN_LIBPINKYSIM_TESTS
	$(call run_gcov,LIBPINKYSIM)
DEPS += $(call add_deps,LIBPINKYSIM_TESTS)


#######################################
# libgdbremote.a
HOST_LIBGDBREMOTE_OBJ := $(call host_objs,libgdbremote/src)
HOST_LIBGDBREMOTE_LIB := $(HOST_LIBDIR)/libgdbremote.a
$(HOST_LIBGDBREMOTE_LIB) : $(HOST_LIBGDBREMOTE_OBJ)
	$(call build_lib,HOST)
DEPS += $(call add_deps,LIBGDBREMOTE)

# Unit tests
HOST_LIBGDBREMOTE_TESTS_OBJ := $(call host_objs,libgdbremote/tests) $(call host_objs,libgdbremote/mocks)
HOST_LIBGDBREMOTE_TESTS_EXE := libgdbremote_tests$(GCOV)
$(HOST_LIBGDBREMOTE_TESTS_EXE) : INCLUDES := $(INCLUDES) libgdbremote/mocks mri/CppUTest/include
$(HOST_LIBGDBREMOTE_TESTS_EXE) : $(HOST_LIBGDBREMOTE_TESTS_OBJ) \
                                 $(HOST_LIBGDBREMOTE_LIB) \
                                 $(HOST_LIBCOMMON_LIB) \
                                 $(HOST_CPPUTEST_LIB)
	$(call link_exe,HOST)
RUN_LIBGDBREMOTE_TESTS : $(HOST_LIBGDBREMOTE_TESTS_EXE)
	$Q $(HOST_LIBGDBREMOTE_TESTS_EXE)
GCOV_LIBGDBREMOTE : RUN_LIBGDBREMOTE_TESTS
	$(call run_gcov,LIBGDBREMOTE)
DEPS += $(call add_deps,LIBGDBREMOTE_TESTS)


#######################################
# libcommserial.a
HOST_LIBCOMMSERIAL_OBJ := $(call host_objs,libcommserial/src)
HOST_LIBCOMMSERIAL_LIB := $(HOST_LIBDIR)/libcommserial.a
$(HOST_LIBCOMMSERIAL_LIB) : $(HOST_LIBCOMMSERIAL_OBJ)
	$(call build_lib,HOST)
LIBCOMMSERIAL_LIB : $(HOST_LIBCOMMSERIAL_LIB)
DEPS += $(call add_deps,LIBCOMMSERIAL)


#######################################
# libthunk2real.a
HOST_LIBTHUNK2REAL_OBJ := $(call host_objs,libthunk2real/src) $(call host_objs,libthunk2real/mocks)
HOST_LIBTHUNK2REAL_LIB := $(HOST_LIBDIR)/libthunk2real.a
$(HOST_LIBTHUNK2REAL_LIB) : $(HOST_LIBTHUNK2REAL_OBJ)
	$(call build_lib,HOST)
DEPS += $(call add_deps,LIBTHUNK2REAL)

# Unit tests
# Note: The actual tests here come from libpinkysim/tests
HOST_LIBTHUNK2REAL_TESTS_OBJ := $(addprefix $(HOST_OBJDIR)/libthunk2real/,$(addsuffix .o,$(basename $(wildcard libpinkysim/tests/*.cpp))))
HOST_LIBTHUNK2REAL_TESTS_EXE := libthunk2real_tests$(GCOV)
$(HOST_LIBTHUNK2REAL_TESTS_EXE) : INCLUDES := $(INCLUDES) mri/CppUTest/include
$(HOST_LIBTHUNK2REAL_TESTS_EXE) : $(HOST_LIBTHUNK2REAL_TESTS_OBJ) \
                                  $(HOST_LIBTHUNK2REAL_LIB) \
                                  $(HOST_LIBCOMMON_LIB) \
                                  $(HOST_LIBGDBREMOTE_LIB) \
                                  $(HOST_LIBCOMMSERIAL_LIB) \
                                  $(HOST_CPPUTEST_LIB)
	$(call link_exe,HOST)
RUN_LIBTHUNK2REAL_TESTS : $(HOST_LIBTHUNK2REAL_TESTS_EXE)
	$Q $(HOST_LIBTHUNK2REAL_TESTS_EXE)
DEPS += $(call add_deps,LIBTHUNK2REAL_TESTS)
$(HOST_OBJDIR)/libthunk2real/%.o : %.cpp
	@echo Compiling $<
	$Q $(MAKEDIR)
	$Q $(HOST_GPP) $(HOST_GPPFLAGS) -DTHUNK2REAL $(call includes,$(INCLUDES)) -c $< -o $@



# *** Pattern Rules ***
$(HOST_OBJDIR)/%.o : %.c
	@echo Compiling $<
	$Q $(MAKEDIR)
	$Q $(HOST_GCC) $(HOST_GCCFLAGS) $(call includes,$(INCLUDES)) -c $< -o $@

$(HOST_OBJDIR)/%.o : %.cpp
	@echo Compiling $<
	$Q $(MAKEDIR)
	$Q $(HOST_GPP) $(HOST_GPPFLAGS) $(call includes,$(INCLUDES)) -c $< -o $@


# *** Pull in header dependencies if not performing a clean build. ***
ifneq "$(findstring clean,$(MAKECMDGOALS))" "clean"
    -include $(DEPS)
endif
