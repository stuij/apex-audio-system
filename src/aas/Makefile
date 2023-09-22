# AAS Mix Library makefile
# NOTE: Disable printf_special in final version
# comment out to compile with GCC
ifdef COMPILE_WITH_LLVM
	ifeq ($(strip $(GBA_LLVM)),)
		$(error Please set GBA_LLVM in your environment. export GBA_LLVM=<path to gba-llvm installation>)
	endif

	AR = $(GBA_LLVM)/bin/llvm-ar
else
	AR = $(GBA_LLVM)/bin/llvm-ar

    # Tools and flags.
    # CROSS = arm-thumb-elf-
	CROSS ?= arm-none-eabi-
endif

# Name of output targets.
TARGET = libAAS.a

# Files you want to go in IWRAM.
IWRAM =	AAS_Mixer.o

# Other source files.
SRC =	AAS_ASM.o AAS_Logo.o AAS_Main.o AAS_MOD.o AAS_Shared.o AAS_SFX.o

include ../../make/common.make

$(TARGET): $(IWRAM) $(GFX) $(SOUND) $(SRC)
	$(AR) r $@ $(SRC) $(IWRAM) $(GFX) $(SOUND) $(LIBS)

clean:
	rm -f *.o $(TARGET) $(NAME) $(GFX) $(SOUND) $(SYMBOLS) $(MAP)
