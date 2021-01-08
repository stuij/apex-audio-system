# Payback makefile
# NOTE: Disable printf_special in final version

# Tools and flags.
CROSS =
CC = $(CROSS)gcc
CFLAGS = -O3
#THUMB = -mthumb -O3 -ffixed-r14 -funroll-loops
#ARM = -marm -Os -mlong-calls
#DEFAULT = $(THUMB)
# DEFAULT was $(THUMB)

AS = $(CROSS)as
ASFLAGS =
#-mthumb-interwork

LD = $(CROSS)gcc
LDFLAGS = 
# "-s" removed
# "-mno-cygwin" removed otherwise readdir() returns incorrect value

# Name of output targets.
TARGET = conv2aas

# C source files.
SRC =	Main.o

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
	rm -f *.o $(GFX) $(SOUND) $(SYMBOLS) $(MAP)

clean:
	rm -f *.o $(TARGET) $(GFX) $(SOUND) $(SYMBOLS) $(MAP)

%.o: %.s
	$(AS) $(ASFLAGS) -o $@ $<

%.o: %.c
	$(CC) $(INCLUDES) $(CFLAGS) -c $< -o $@

%.text.iwram.o: %.o
	cp $< $@

$(TARGET): $(GFX) $(SOUND) $(SRC)
	$(LD) $(LDFLAGS) -o $@ $(SRC) $(GFX) $(SOUND) $(LIBS)
	$(CROSS)size $@
