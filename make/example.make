ifdef COMPILE_WITH_LLVM
	ifeq ($(strip $(GBA_LLVM)),)
		$(error Please set GBA_LLVM in your environment. export GBA_LLVM=<path to gba-llvm installation>)
	endif

    # Library and include paths.
	LIBS = -L../../build/aas/lib -lAAS -ltonc
	INCLUDES = -I../../build/aas/include

	LDFLAGS = -Wl,-Map,$(MAP) --config armv4t-gba.cfg -Wl,-T,gba_cart.ld
else
    # Library and include paths.
	LIBS = -L../../build/aas/lib -lAAS -L$(DEVKITARM)/../libtonc/lib -ltonc
	INCLUDES = -I../../build/aas/include -I$(DEVKITARM)/../libtonc/include

	LDFLAGS = -Wl,-Map,$(MAP) -specs=gba.specs
endif

# Files you want to go in ROM (AAS_Data.o must go first)
SRC =	AAS_Data.o AASExample.o

MAP = map.out

# Name of output targets.
TMPNAME = $(SHORTNAME).tmp
TARGET = $(SHORTNAME).gba

# Files you want to go in IWRAM.
IWRAM =	

include ../../make/common.make

CONV2AAS_FILE = conv2aas
ifeq ($(OS),Windows_NT)
	CONV2AAS_FILE = conv2aas.exe
endif

AAS_Data.o:
	../../build/conv2aas/$(CONV2AAS_FILE) AAS_Data
	$(AS) $(ASFLAGS) -o $@ AAS_Data.s

$(TMPNAME): $(IWRAM) $(GFX) $(SOUND) $(SRC)
	touch $(MAP)
	$(LD) $(LDFLAGS) -o $@ $(SRC) $(IWRAM) $(GFX) $(SOUND) $(LIBS)
	$(SIZE) $@

$(TARGET): $(TMPNAME)
	$(OBJCOPY) -O binary $< $@
	$(TOOLS)gbafix $@

clean:
	rm -f *.o AAS_Data.h AAS_Data.s $(TARGET) $(TMPNAME) $(GFX) $(SOUND) $(SYMBOLS) $(MAP)
