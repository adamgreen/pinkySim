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
#  Forwards Declaration of Main Rules
#######################################
.PHONY : all clean

all:
clean:


#  Names of tools for cross-compiling ARM Cortex-M binaries.
ARMV6M_GCC     := arm-none-eabi-gcc
ARMV6M_GPP     := arm-none-eabi-g++
ARMV6M_AS      := arm-none-eabi-gcc
ARMV6M_LD      := arm-none-eabi-g++
ARMV6M_AR      := arm-none-eabi-ar
ARMV6M_OBJDUMP := arm-none-eabi-objdump
ARMV6M_OBJCOPY := arm-none-eabi-objcopy

# Handle Windows and *nix differences.
ifeq "$(OS)" "Windows_NT"
    MAKEDIR = mkdir $(subst /,\,$(dir $@))
    REMOVE := del /q
    REMOVE_DIR := rd /s /q
    QUIET := >nul 2>nul & exit 0
else
    MAKEDIR = mkdir -p $(dir $@)
    REMOVE := rm
    REMOVE_DIR := rm -r -f
    QUIET := > /dev/null 2>&1 ; exit 0
endif

# Flags to use when cross-compiling ARMv6-M binaries.
ARMV6M_GCCFLAGS    := -O2 -g3 -mcpu=cortex-m0 -mthumb -mthumb-interwork -Wall -Wextra -Werror -Wno-unused-parameter -MMD -MP
ARMV6M_GCCFLAGS    += -ffunction-sections -fdata-sections -fno-exceptions -fno-delete-null-pointer-checks -fomit-frame-pointer
ARMV6M_GPPFLAGS    := $(ARMV6M_GCCFLAGS) -fno-rtti
ARMV6M_GCCFLAGS    += -std=gnu90
ARMV6M_LDFLAGS      = -mcpu=cortex-m0 -mthumb -Wl,-Map=$(basename $@).map,--cref,--gc-sections -TpinkySim.ld -u _exit
ARMV6M_ASFLAGS     := -mcpu=cortex-m0 -mthumb -x assembler-with-cpp -MMD -MP
ARMV6M_DISASMFLAGS := -d -f -M reg-names-std --demangle

# Output directories for intermediate object files.
OBJDIR        := obj
ARMV6M_OBJDIR := $(OBJDIR)

# Output directories for final libraries.
LIBDIR        := lib
ARMV6M_LIBDIR := $(LIBDIR)

# Start out with empty pre-req lists.  Add modules as we go.
ALL_TARGETS  :=

# Start out with an empty header file dependency list.  Add module files as we go.
DEPS :=

# Useful macros.
objs = $(addprefix $2/,$(addsuffix .o,$(basename $(wildcard $1/*.c $1/*.cpp $1/*.S))))
armv6m_objs = $(call objs,$1,$(ARMV6M_OBJDIR))
add_deps = $(patsubst %.o,%.d,$(ARMV6M_$1_OBJ))
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
define disasm_exe
	@echo Extracting disassembly to $@
	$Q $(MAKEDIR)
	$Q $($1_OBJDUMP) $(ARMV6M_DISASMFLAGS) $< >$@
endef
define create_bin
	@echo Extracting $@
	$Q $(MAKEDIR)
	$Q $($1_OBJCOPY) -O binary $< $@
endef
define make_library # ,LIBRARY,src_dirs,libname.a,includes
    ARMV6M_$1_OBJ    := $(foreach i,$2,$(call armv6m_objs,$i))
    ARMV6M_$1_LIB    := $(ARMV6M_LIBDIR)/$3
    DEPS             += $$(call add_deps,$1)
    ALL_TARGETS      += $$(ARMV6M_$1_LIB)
    $$(ARMV6M_$1_LIB) : INCLUDES := $4
    $$(ARMV6M_$1_LIB) : $$(ARMV6M_$1_OBJ)
		$$(call build_lib,ARMV6M)
endef
define make_sample # ,APP2BUILD,app_src_dirs,includes,other_libs
    ARMV6M_$1_APP_OBJ    := $(foreach i,$2,$(call armv6m_objs,$i))
    ARMV6M_$1_APP_ELF    := $(ARMV6M_OBJDIR)/$1.elf
    ARMV6M_$1_APP_DISASM := $(ARMV6M_OBJDIR)/$1.disasm
    ARMV6M_$1_APP_BIN    := $1.bin
    DEPS                 += $$(call add_deps,$1_APP)
    ALL_TARGETS += $$(ARMV6M_$1_APP_DISASM) $$(ARMV6M_$1_APP_BIN)
    $$(ARMV6M_$1_APP_ELF) : INCLUDES := $3
    $$(ARMV6M_$1_APP_ELF) : $$(ARMV6M_$1_APP_OBJ) $4
		$$(call link_exe,ARMV6M)
    $$(ARMV6M_$1_APP_DISASM) : $$(ARMV6M_$1_APP_ELF)
		$$(call disasm_exe,ARMV6M)
    $$(ARMV6M_$1_APP_BIN) : $$(ARMV6M_$1_APP_ELF)
		$$(call create_bin,ARMV6M)
endef


#######################################
# libstartup.a
$(eval $(call make_library,LIBSTARTUP,libstartup,libstartup.a,../include))

#######################################
# FileTest_Sample
$(eval $(call make_sample,FileTest_Sample,FileTest,include,$(ARMV6M_LIBSTARTUP_LIB)))

#######################################
# CommandLine_Sample
$(eval $(call make_sample,CommandLine_Sample,CommandLine,include,$(ARMV6M_LIBSTARTUP_LIB)))

#######################################
# StdIO_Sample
$(eval $(call make_sample,StdIO_Sample,StdIO,include,$(ARMV6M_LIBSTARTUP_LIB)))


#######################################
#  Actual Definition of Main Rules
#######################################
all : $(ALL_TARGETS)

clean :
	@echo Cleaning samples
	$Q $(REMOVE_DIR) $(OBJDIR) $(QUIET)
	$Q $(REMOVE_DIR) $(LIBDIR) $(QUIET)
	$Q $(REMOVE) *.bin $(QUIET)


# *** Pattern Rules ***
$(ARMV6M_OBJDIR)/%.o : %.c
	@echo Compiling $<
	$Q $(MAKEDIR)
	$Q $(ARMV6M_GCC) $(ARMV6M_GCCFLAGS) $(call includes,$(INCLUDES)) -c $< -o $@

$(ARMV6M_OBJDIR)/%.o : %.S
	@echo Assembling $<
	$Q $(MAKEDIR)
	$Q $(ARMV6M_AS) $(ARMV6M_ASFLAGS) $(call includes,$(INCLUDES)) -c $< -o $@


# *** Pull in header dependencies if not performing a clean build. ***
ifneq "$(findstring clean,$(MAKECMDGOALS))" "clean"
    -include $(DEPS)
endif
