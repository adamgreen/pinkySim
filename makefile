# Directories to be built
DIRS=CppUTest libcommon libpinkysim
TESTDIRS=libgdbremote
DIRSCLEAN = $(addsuffix .clean,$(DIRS) $(TESTDIRS))

all: $(DIRS)

clean: $(DIRSCLEAN)

tests: $(DIRS) $(TESTDIRS)

$(DIRS):
	@echo Building $@
	@ $(MAKE) -C $@ all

$(DIRSCLEAN): %.clean:
	@echo Cleaning $*
	@ $(MAKE) -C $*  clean

$(TESTDIRS):
	@echo Building $@
	@ $(MAKE) -C $@ all

.PHONY: all clean test $(DIRS) $(DIRSCLEAN) $(TESTDIRS)
