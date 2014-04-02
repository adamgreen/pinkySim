#Set this to @ to keep the makefile quiet
SILENCE = @

#---- Outputs ----#
COMPONENT_NAME = thunk2real
CPPUTEST_LIB_DIR = ../lib
CPPUTEST_USE_VPATH = Y

#--- Inputs ----#
PROJECT_HOME_DIR = .
CPPUTEST_HOME = ../CppUTest

CPP_PLATFORM = Gcc

CPPUTEST_CPPFLAGS += -fno-common -DTHUNK2REAL
CPPUTEST_WARNINGFLAGS += -Wall 
CPPUTEST_WARNINGFLAGS += -Werror 
CPPUTEST_WARNINGFLAGS += -Wswitch-default 
CPPUTEST_WARNINGFLAGS += -Wswitch-enum
CPPUTEST_WARNINGFLAGS += -Wno-unused-parameter
CPPUTEST_WARNINGFLAGS += -Wno-overlength-strings
CPPUTEST_CFLAGS += -std=gnu99
CPPUTEST_CFLAGS += -Wextra 
CPPUTEST_CFLAGS += -pedantic 
CPPUTEST_CFLAGS += -Wstrict-prototypes

SRC_DIRS = \
	src\


TEST_SRC_DIRS = \
	../libpinkysim/tests \
	
INCLUDE_DIRS =\
  $(CPPUTEST_HOME)/include/ \
  ../include/               \
  tests/                    \

USER_LIBS = ../lib/libcommon.a ../lib/libgdbremote.a ../lib/libcommserial.a

include $(CPPUTEST_HOME)/build/MakefileWorker.mk
