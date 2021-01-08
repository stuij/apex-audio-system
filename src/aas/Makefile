# AAS Mix Library makefile
# NOTE: Disable printf_special in final version

# Tools and flags.
# CROSS = arm-thumb-elf-
CROSS = arm-none-eabi-
CC = $(CROSS)gcc
CFLAGS = -mthumb-interwork -fomit-frame-pointer -mcpu=arm7tdmi -ffast-math -fno-exceptions -c
# note: removed "-S", "-mapcs-stack-check -mapcs-frame" and "-Wall" above
# added "-ffixed-r14" and "-mlong-calls" below to work around compiler bug
# Optimise options: -funroll-loops
# Automatic inlining in arm code wastes IWRAM
THUMB = -mthumb -O3 -ffixed-r14 -funroll-loops
ARM = -marm -Os -mlong-calls
DEFAULT = $(THUMB)
# DEFAULT was $(THUMB)

AS = $(CROSS)as
ASFLAGS = -mthumb-interwork

#LD = $(CROSS)gcc
#MAP = map.out
#LDFLAGS = -mthumb-interwork -Xlinker -Map $(MAP) -nostartfiles -Tlnkscript

# Library and include paths.
#LIBS = -L../gbalib -lgba -lm
#INCLUDES = -I../gbalib

# Name of output targets.
TARGET = libAAS.a

# Files you want to go in IWRAM.
IWRAM =	AAS_Mixer.o

# Other source files.
SRC =	AAS_ASM.o AAS_Logo.o AAS_Main.o AAS_MOD.o AAS_Shared.o AAS_SFX.o

# Graphics related raw input files (e.g. sprites, palettes).
GFX =

# Sound related o input files.
SOUND =

# This rule builds a .h file of all your raw input file exports.
# So this creates a file (say) 'symbol.h' which has lines like (say):
# extern const u8 _binary_bob_raw_start[];
SYMBOLS = symbols.h
SYMBOL_PREFIX = "extern const u8 _binary_"
SYMBOL_SUFFIX1 = "_start[];"
SYMBOL_SUFFIX2 = "_end[];"



# Here follow the generic build rules.
all:  clean $(TARGET)

# Rule to build raw files into .o files, noting their exported symbols.
%.o: %.raw
	@$(CROSS)objcopy -I binary -O elf32-little $< temp.o 2> /dev/null
	@$(CROSS)ld -T convert.ls temp.o -o $@
	@interflip -mthumb-interwork $@
	@echo $(CROSS)objcopy -I binary -O elf32-little $< $@
	@echo -n $(SYMBOL_PREFIX) >> $(SYMBOLS)
	@echo -n "$<" | tr "[:punct:]" "_" >> $(SYMBOLS)
	@echo $(SYMBOL_SUFFIX1) >> $(SYMBOLS)
	@echo -n $(SYMBOL_PREFIX) >> $(SYMBOLS)
	@echo -n "$<" | tr "[:punct:]" "_" >> $(SYMBOLS)
	@echo $(SYMBOL_SUFFIX2) >> $(SYMBOLS)

release:
	make clean all
	rm -f *.o $(NAME) $(GFX) $(SOUND) $(SYMBOLS) $(MAP)

clean:
	rm -f *.o $(TARGET) $(NAME) $(GFX) $(SOUND) $(SYMBOLS) $(MAP)

%.o: %.s
	$(AS) $(ASFLAGS) -o $@ $<

%.o: %.thumb.c
	$(CC) $(THUMB) $(INCLUDES) $(CFLAGS) -c $< -o $@

%.o: %.arm.c
	$(CC) $(ARM) $(INCLUDES) $(CFLAGS) -c $< -o $@

%.o: %.c
	$(CC) $(DEFAULT) $(INCLUDES) $(CFLAGS) -c $< -o $@

%.text.iwram.o: %.o
	cp $< $@

$(TARGET): $(IWRAM) $(GFX) $(SOUND) $(SRC)
	$(CROSS)ar r $@ $(SRC) $(IWRAM) $(GFX) $(SOUND) $(LIBS)

# $(NAME): crt0.o $(IWRAM) $(GFX) $(SOUND) $(SRC)
#	touch $(MAP)
#	$(LD) $(LDFLAGS) -o $@ crt0.o $(SRC) $(IWRAM) $(GFX) $(SOUND) $(LIBS)
#	$(CROSS)size $@

#$(TARGET): $(NAME)
#	$(CROSS)objcopy -v -O binary $< $@
#	gbafix $@

