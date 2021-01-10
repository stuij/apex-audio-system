# AAS Mix Library makefile
# NOTE: Disable printf_special in final version

# Tools and flags.
# CROSS = arm-thumb-elf-
CROSS ?= arm-none-eabi-

# Name of output targets.
TARGET = libAAS.a

# Files you want to go in IWRAM.
IWRAM =	AAS_Mixer.o

# Other source files.
SRC =	AAS_ASM.o AAS_Logo.o AAS_Main.o AAS_MOD.o AAS_Shared.o AAS_SFX.o

include ../../make/common.make

$(TARGET): $(IWRAM) $(GFX) $(SOUND) $(SRC)
	$(CROSS)ar r $@ $(SRC) $(IWRAM) $(GFX) $(SOUND) $(LIBS)

clean:
	rm -f *.o $(TARGET) $(NAME) $(GFX) $(SOUND) $(SYMBOLS) $(MAP)
